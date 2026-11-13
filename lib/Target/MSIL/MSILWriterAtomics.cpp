//===-- MSILWriterAtomics.cpp - Atomic operations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//===----------------------------------------------------------------------===//

#include "MSILWriter.h"
#include "MSILWriterShared.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Object/Binary.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Object/COFF.h"
#include "llvm/Object/ELFObjectFile.h"
#include "llvm/Object/ObjectFile.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/Config/llvm-config.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Operator.h"
#include "llvm/TargetParser/Triple.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/xxhash.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"

using namespace llvm;

//===----------------------------------------------------------------------===//
// Atomics
//===----------------------------------------------------------------------===//
//
// The CLR has no atomic instructions, but it has `System.Threading.Interlocked`
// for int32/int64/native int, the `volatile.` prefix for any access width, and
// `Monitor` for the rest. Three verified facts shape the lowering:
//
//  * `Interlocked` takes a managed `T&`. An unmanaged `void*` is accepted as
//    one, so pointers need no pinning or marshalling. This holds both on the
//    .NET Framework 4.0 `mscorlib` a bare exe binds to and on .NET 9.
//  * Only int32/int64/native-int have `Add`; `Exchange` and `CompareExchange`
//    add float32/float64. `And`/`Or` do not exist on .NET Framework, so those
//    use a CompareExchange loop rather than a call.
//  * Narrow integers have no overload at all. They are serialized on
//    `$MSIL::atomicLock`; NOTES-atomics.md records what that does and does
//    not guarantee.

StringRef MSILWriter::interlockedTypeName(Type *Ty) {
  if (Ty->isPointerTy())
    return "native int";
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    switch (Ty->getIntegerBitWidth()) {
    case 32:
      return "int32";
    case 64:
      return "int64";
    default:
      return "";
    }
  case Type::FloatTyID:
    return "float32";
  case Type::DoubleTyID:
    return "float64";
  // A mapped x86_fp80 is a binary64 value, so it uses the same overload. It
  // would otherwise fall into the lock path, which is written for narrow
  // integers.
  case Type::X86_FP80TyID:
    return fp80MappedToDouble() ? "float64" : StringRef();
  default:
    return "";
  }
}

// `volatile.` is a prefix, not an opcode: it goes between the address and the
// ldind/stind it applies to. It is the only way to ask for an atomic access of
// a width Interlocked does not cover.
//
// i128 is a value type, not a primitive, so it moves with ldobj/stobj like an
// aggregate does -- and `ldind.` with an empty suffix is not IL at all, which
// is what a 16-byte access used to emit. The CLR defines ldobj/stobj of a
// 16-byte type as a single 16-byte access when the type is 16 bytes or
// smaller, and the JIT lowers it to one movups (or a lock cmpxchg16b when it
// is atomic), so this is the same instruction the native code would use.
void MSILWriter::printAtomicLoad(const Value *Ptr, Type *Ty) {
  printValueLoad(Ptr);
  printPtrToNative();
  out() << "\tvolatile.\n";
  if (movesAsBlob(Ty))
    out() << "\tldobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
}

void MSILWriter::printAtomicStore(const Value *Ptr, const Value *Val) {
  Type *Ty = Val->getType();
  printValueLoad(Ptr);
  printPtrToNative();
  printValueLoad(Val);
  out() << "\tvolatile.\n";
  if (movesAsBlob(Ty))
    out() << "\tstobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tstind." << getMemSuffix(Ty) << '\n';
}

// Run Body with the module's atomic lock held. `leave` is the only way out of a
// protected region, and the finally runs Monitor.Exit on both the normal and
// the exceptional path, so a throwing body cannot leak the lock.
void MSILWriter::printUnderAtomicLock(function_ref<void()> Body) {
  NeedsAtomicLock = true;
  std::string Lock = "t" + utostr(getUniqID());
  std::string Done = "IL_unlock" + utostr(getUniqID());
  ExtraLocals.emplace_back("object", Lock);
  out() << "\tldsfld\tobject '$MSIL::atomicLock'\n\tstloc\t'" << Lock
        << "'\n\tldloc\t'" << Lock << "'\n";
  out() << "\tcall\tvoid [mscorlib]System.Threading.Monitor::Enter(object)\n";
  out() << "\t.try\n\t{\n";
  Body();
  out() << "\tleave\t" << Done << "\n\t}\n\tfinally\n\t{\n";
  out() << "\tldloc\t'" << Lock << "'\n";
  out() << "\tcall\tvoid [mscorlib]System.Threading.Monitor::Exit(object)\n";
  out() << "\tendfinally\n\t}\n" << Done << ":\n";
}

// Leave `<bits of A> != <bits of B>` on the stack. IL has no bitwise float
// compare and ceq is false for NaN, so both values are re-read as integers.
// The CLR's own float CompareExchange is bitwise, which is what makes this the
// right test for "did the exchange happen".
void MSILWriter::printFloatBitsNe(const std::string &A, const std::string &B,
                                  Type *Ty) {
  Type *ITy = Ty->isFloatTy() ? Type::getInt32Ty(Ty->getContext())
                              : Type::getInt64Ty(Ty->getContext());
  std::string ABits = newTempLocal(ITy), BBits = newTempLocal(ITy);
  const char *Sfx = Ty->isFloatTy() ? "i4" : "i8";
  out() << "\tldloca\t'" << A << "'\n\tconv.u8\n\tldind." << Sfx
        << "\n\tstloc\t'" << ABits << "'\n";
  out() << "\tldloca\t'" << B << "'\n\tconv.u8\n\tldind." << Sfx
        << "\n\tstloc\t'" << BBits << "'\n";
  out() << "\tldloc\t'" << ABits << "'\n\tldloc\t'" << BBits << "'\n\tceq\n";
  printNot();
}

/// The Instruction opcode that computes an i128 atomicrmw's new value.
///
/// AtomicRMWInst::BinOp is its own enum; only the arithmetic names it shares
/// with Instruction mean the same thing, and printWideBinaryOn takes the
/// Instruction spelling because that is what printInstruction passes it.
static unsigned wideRMWOpcode(AtomicRMWInst::BinOp Op) {
  switch (Op) {
  case AtomicRMWInst::Add:
  case AtomicRMWInst::FAdd:
    return Instruction::Add;
  case AtomicRMWInst::Sub:
  case AtomicRMWInst::FSub:
    return Instruction::Sub;
  case AtomicRMWInst::And:
    return Instruction::And;
  case AtomicRMWInst::Or:
    return Instruction::Or;
  case AtomicRMWInst::Xor:
    return Instruction::Xor;
  default:
    // Min/Max/UMin/UMax/Nand have no Int128 method; printMinMaxOf is the
    // scalar path and there is no value-type equivalent.
    fatalError("unsupported 128-bit atomicrmw operation " + Twine((unsigned)Op));
  }
}

static bool isIntMinMax(AtomicRMWInst::BinOp Op) {
  return Op == AtomicRMWInst::Min || Op == AtomicRMWInst::Max ||
         Op == AtomicRMWInst::UMin || Op == AtomicRMWInst::UMax;
}

void MSILWriter::printMinMaxOf(AtomicRMWInst::BinOp Op, const std::string &Old,
                               const std::string &Val, Type *Ty) {
  bool Signed = Op == AtomicRMWInst::Min || Op == AtomicRMWInst::Max;
  bool KeepGreater = Op == AtomicRMWInst::Max || Op == AtomicRMWInst::UMax;
  unsigned Bits = Ty->getIntegerBitWidth();
  // narrow locals hold the zero-extended form
  auto LoadOperand = [&](const std::string &Local) {
    out() << "\tldloc\t'" << Local << "'\n";
    if (Signed && Bits == 8)
      out() << "\tconv.i1\n";
    else if (Signed && Bits == 16)
      out() << "\tconv.i2\n";
  };
  std::string Keep = "IL_keepold" + utostr(getUniqID());
  std::string Done = "IL_minmax" + utostr(getUniqID());
  LoadOperand(Old);
  LoadOperand(Val);
  out() << '\t' << (KeepGreater ? "cgt" : "clt") << (Signed ? "" : ".un")
        << "\n\tbrtrue\t" << Keep << '\n';
  out() << "\tldloc\t'" << Val << "'\n\tbr\t" << Done << '\n';
  out() << Keep << ":\n\tldloc\t'" << Old << "'\n";
  out() << Done << ":\n";
}

void MSILWriter::printAtomicRMW(const AtomicRMWInst *RMW) {
  Type *RTy = RMW->getType();
  AtomicRMWInst::BinOp Op = RMW->getOperation();
  StringRef ILTy = interlockedTypeName(RTy);

  // The forms C atomics actually produce: exchange, add, and sub (which
  // Interlocked spells as an add of the two's complement).
  if (!ILTy.empty() &&
      (Op == AtomicRMWInst::Xchg || Op == AtomicRMWInst::Add ||
       Op == AtomicRMWInst::Sub)) {
    printValueLoad(RMW->getPointerOperand());
    printPtrToNative();
    printValueLoad(RMW->getValOperand());
    if (Op == AtomicRMWInst::Sub)
      out() << "\tneg\n";
    out() << "\tcall\t" << ILTy << " [mscorlib]System.Threading.Interlocked::"
          << (Op == AtomicRMWInst::Xchg ? "Exchange" : "Add") << "(" << ILTy
          << "&, " << ILTy << ")\n";
    // Interlocked.Add returns the new value, atomicrmw the old one
    if (Op != AtomicRMWInst::Xchg) {
      printValueLoad(RMW->getValOperand());
      out() << (Op == AtomicRMWInst::Add ? "\tsub\n" : "\tadd\n");
    }
    return;
  }

  // Everything else is a CompareExchange loop, which needs no overload beyond
  // the one for the value's own type.
  std::string Old = newTempLocal(RTy), Val = newTempLocal(RTy),
              New = newTempLocal(RTy);
  printValueLoad(RMW->getValOperand());
  out() << "\tstloc\t'" << Val << "'\n";

  auto Compute = [&] {
    if (Op == AtomicRMWInst::Xchg) {
      // The new value is simply the operand, with no old value read.
      out() << "\tldloc\t'" << Val << "'\n";
      return;
    }
    if (isIntMinMax(Op)) {
      printMinMaxOf(Op, Old, Val, RTy);
      return;
    }
    out() << "\tldloc\t'" << Old << "'\n\tldloc\t'" << Val << "'\n";
    switch (Op) {
    case AtomicRMWInst::Add:
    case AtomicRMWInst::FAdd:
      out() << "\tadd\n";
      return;
    case AtomicRMWInst::Sub:
    case AtomicRMWInst::FSub:
      out() << "\tsub\n";
      return;
    case AtomicRMWInst::And:
      out() << "\tand\n";
      return;
    case AtomicRMWInst::Or:
      out() << "\tor\n";
      return;
    case AtomicRMWInst::Xor:
      out() << "\txor\n";
      return;
    case AtomicRMWInst::Nand:
      out() << "\tand\n";
      printNot();
      return;
    default:
      fatalError("unsupported atomicrmw operation " + Twine((unsigned)Op));
    }
  };

  if (ILTy.empty()) {
    // No Interlocked overload: i8/i16 (no narrow overload exists at all) and
    // i128 (a value type, not a primitive). The lock is what makes the
    // read-modify-write indivisible.
    //
    // For a narrow integer `conv.uN` normalizes the loaded value to the
    // zero-extended form every other narrow value has. i128 has no such form:
    // it is an ordinary Int128 value, and its operations are the value type's
    // own methods, so it skips the normalization and the IL arithmetic.
    bool Wide = isWideInt(RTy);
    if (!Wide && !RTy->isIntegerTy())
      fatalError("an atomic operation on " + Twine(str(RTy)) +
                 " is not supported");
    printUnderAtomicLock([&] {
      printAtomicLoad(RMW->getPointerOperand(), RTy);
      if (!Wide)
        out() << '\t' << widthConv(RTy) << '\n';
      out() << "\tstloc\t'" << Old << "'\n";
      if (Wide) {
        // xchg stores the operand as it is; everything else is one of the
        // value type's own operators, which is why the opcode is translated
        // rather than passed through (AtomicRMWInst::BinOp is not
        // Instruction's opcode enum, even though the names overlap).
        if (Op == AtomicRMWInst::Xchg)
          printValueLoad(RMW->getValOperand());
        else
          printWideBinaryOn(wideRMWOpcode(Op),
                            [&] { out() << "\tldloc\t'" << Old << "'\n"; },
                            [&] { printValueLoad(RMW->getValOperand()); },
                            isWideInt(RMW->getValOperand()->getType()));
      } else
        Compute();
      // The computed value is what goes back to memory. Storing the operand
      // instead would make every narrow and 16-byte read-modify-write write
      // its argument rather than its result.
      out() << "\tstloc\t'" << New << "'\n";
      printValueLoad(RMW->getPointerOperand());
      printPtrToNative();
      out() << "\tldloc\t'" << New << "'\n";
      if (movesAsBlob(RTy))
        out() << "\tstobj\t" << getTypeName(RTy) << '\n';
      else
        out() << "\tstind." << getMemSuffix(RTy) << '\n';
    });
    out() << "\tldloc\t'" << Old << "'\n";
    return;
  }

  std::string Prev = newTempLocal(RTy);
  std::string Loop = "IL_rmw" + utostr(getUniqID());
  out() << Loop << ":\n";
  printAtomicLoad(RMW->getPointerOperand(), RTy);
  out() << "\tstloc\t'" << Old << "'\n";
  Compute();
  out() << "\tstloc\t'" << New << "'\n";
  printValueLoad(RMW->getPointerOperand());
  printPtrToNative();
  out() << "\tldloc\t'" << New << "'\n\tldloc\t'" << Old << "'\n";
  out() << "\tcall\t" << ILTy << " [mscorlib]System.Threading.Interlocked::"
        << "CompareExchange(" << ILTy << "&, " << ILTy << ", " << ILTy
        << ")\n\tstloc\t'" << Prev << "'\n";
  if (RTy->isFloatingPointTy()) {
    printFloatBitsNe(Prev, Old, RTy);
    out() << "\tbrtrue\t" << Loop << "\n";
  } else {
    out() << "\tldloc\t'" << Prev << "'\n\tldloc\t'" << Old
          << "'\n\tbne.un\t" << Loop << "\n";
  }
  out() << "\tldloc\t'" << Old << "'\n";
}

void MSILWriter::printAtomicCmpXchg(const AtomicCmpXchgInst *CX) {
  Type *VTy = CX->getNewValOperand()->getType();
  StringRef ILTy = interlockedTypeName(VTy);
  std::string Old = newTempLocal(VTy);

  bool Wide = isWideInt(VTy);
  if (ILTy.empty()) {
    if (!Wide && !VTy->isIntegerTy())
      fatalError("an atomic compare-exchange on " + Twine(str(VTy)) +
                 " is not supported");
    std::string Done = "IL_cx" + utostr(getUniqID());
    printUnderAtomicLock([&] {
      printAtomicLoad(CX->getPointerOperand(), VTy);
      if (!Wide)
        out() << '\t' << widthConv(VTy) << '\n';
      out() << "\tstloc\t'" << Old << "'\n";
      if (Wide) {
        // Int128 compares through its own equality operator, which leaves a
        // bool; there is no IL compare for a value type.
        printValueLoad(CX->getCompareOperand());
        out() << "\tldloc\t'" << Old << "'\n\tcall\tbool " << Int128Ref
              << "::op_Equality(" << Int128Ty << ", " << Int128Ty
              << ")\n\tbrfalse\t" << Done << "\n";
      } else {
        // Both sides of the compare must be in the same form: `Old` was
        // zero-extended above, so the comparand has to be too.
        printValueLoad(CX->getCompareOperand());
        out() << '\t' << widthConv(VTy) << "\n\tldloc\t'" << Old
              << "'\n\tbne.un\t" << Done << "\n";
      }
      printAtomicStore(CX->getPointerOperand(), CX->getNewValOperand());
      out() << Done << ":\n";
    });
  } else {
    printValueLoad(CX->getPointerOperand());
    printPtrToNative();
    printValueLoad(CX->getNewValOperand());
    printValueLoad(CX->getCompareOperand());
    out() << "\tcall\t" << ILTy << " [mscorlib]System.Threading.Interlocked::"
          << "CompareExchange(" << ILTy << "&, " << ILTy << ", " << ILTy
          << ")\n\tstloc\t'" << Old << "'\n";
  }

  // The result is { old, success }; success is re-derived from the same
  // comparison, and the struct's second field sits at the first's alloc size.
  std::string Res = newTempLocal(CX->getType());
  out() << "\tldloca\t'" << Res << "'\n\tconv.u8\n\tldloc\t'" << Old
        << "'\n";
  if (movesAsBlob(VTy))
    out() << "\tstobj\t" << getTypeName(VTy) << "\n";
  else
    out() << "\tstind." << getMemSuffix(VTy) << "\n";
  printAggregateLaneAddr(Res, Type::getInt8Ty(CX->getContext()),
                         DL->getTypeAllocSize(VTy).getFixedValue());
  // Compare in the same form `Old` was stored in: a value type through its
  // own equality operator, a narrow integer after the zero-extension, or a
  // negative narrow comparand would never match.
  printValueLoad(CX->getCompareOperand());
  if (Wide)
    out() << "\tldloc\t'" << Old << "'\n\tcall\tbool " << Int128Ref
          << "::op_Equality(" << Int128Ty << ", " << Int128Ty << ")\n";
  else
    out() << '\t' << widthConv(VTy) << "\n\tldloc\t'" << Old
          << "'\n\tceq\n";
  out() << "\tstind.i1\n";
  out() << "\tldloc\t'" << Res << "'\n";
}

// The lock is a single shared object, emitted only for a module that has a
// narrow atomic access. `<Module>.cctor` runs before any static method of the
// module, so it is initialized before the first atomic operation can reach it.
void MSILWriter::printAtomicLock() {
  if (!NeedsAtomicLock)
    return;
  out() << "\n.field static assembly object '$MSIL::atomicLock'\n";
  out() << "\n.method static void .cctor() cil managed\n{\n";
  out() << "\t.maxstack\t2\n";
  out() << "\tnewobj\tinstance void [mscorlib]System.Object::.ctor()\n";
  out() << "\tstsfld\tobject '$MSIL::atomicLock'\n\tret\n}\n";
}
