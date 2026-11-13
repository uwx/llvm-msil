//===-- MSILWriterInstructions.cpp - Instruction selection and emission ----===//
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
// Value loading
//===----------------------------------------------------------------------===//

void MSILWriter::printValueLoad(const Value *V) {
  if (const auto *GV = dyn_cast<GlobalVariable>(V)) {
    // Both external and internal globals are fields holding a pointer to the
    // variable's storage. A thread-local global's storage is per thread, so
    // its address comes from an accessor that also initializes this thread's
    // copy on first use.
    const char *PtrTy = pointersAsInt64() ? "int64" : "void*";
    if (GV->isThreadLocal() && !GV->isDeclaration())
      out() << "\tcall\t" << PtrTy << " " << getThreadLocalRef(GV) << "\n";
    else
      out() << "\tldsfld\t" << PtrTy << " '" << GV->getName() << "'\n";
    return;
  }
  if (const auto *Fn = dyn_cast<Function>(V)) {
    // The buffer-form signature describes how *this* module calls the
    // definition; a `calli` through the pointer would name its own signature
    // and get the arguments wrong. Nothing that reaches here can pass the
    // buffer, so say so rather than emit a method that faults.
    if (isEmittedVarArgDef(Fn))
      fatalError("cannot take the address of vararg function '" +
                         Fn->getName() +
                         "'; call it directly instead");
    out() << "\tldftn\t"
        << getCallSignature(Fn->getFunctionType(), nullptr, getFunctionRef(Fn))
        << "\n";
    // A function pointer is a pointer-typed value: keep it in its slot type.
    if (pointersAsInt64())
      out() << "\tconv.u8\n";
    return;
  }
  if (isa<Argument>(V))
    out() << "\tldarg\t'" << getValueName(V) << "'\n";
  else if (isa<Instruction>(V))
    out() << "\tldloc\t'" << getValueName(V) << "'\n";
  else if (const auto *CE = dyn_cast<ConstantExpr>(V))
    printConstantExpr(CE);
  else
    printConstLoad(cast<Constant>(V));
}

void MSILWriter::printValueSave(const Value *V) {
  Type *Ty = V->getType();
  // Store narrow integers in canonical form so unsigned loads are correct.
  // A value computed by a 32-bit IL op arrives as int32 even when its IR type
  // is i64, so always normalize to the declared width here. i128 is a value
  // type: conv has nothing to normalize, and the local already matches.
  if (Ty->isIntegerTy() && !Ty->isIntegerTy(32) && !isWideInt(Ty))
    out() << '\t' << widthConv(Ty) << '\n';
  if (isa<Argument>(V))
    out() << "\tstarg\t'" << getValueName(V) << "'\n";
  else
    out() << "\tstloc\t'" << getValueName(V) << "'\n";
}

void MSILWriter::printValueLoadAsInt(const Value *V, bool Signed) {
  printValueLoad(V);
  Type *Ty = V->getType();
  if (!Ty->isIntegerTy())
    return;
  unsigned N = Ty->getIntegerBitWidth();
  // i128 is stored as a value type, never as a stack int, so it has no
  // canonical form to restore and no conv applies.
  if (N == 32 || N == 64 || N == 128)
    return;
  if (N == 1) {
    // i1 is held as 0 or 1, so conv.i1 would leave 1 as 1. A signed i1 is 0 or
    // -1 (IR `sext i1 %x` is exactly negation); an unsigned one is already
    // canonical.
    if (Signed)
      out() << "\tneg\n";
    return;
  }
  if (N == 16) {
    out() << (Signed ? "\tconv.i2\n" : "\tconv.u2\n");
    return;
  }
  if (N == 8) {
    out() << (Signed ? "\tconv.i1\n" : "\tconv.u1\n");
    return;
  }
  // A width with no CLR type of its own (i24, from an APInt constant, is the
  // one that shows up in practice) lives in an int32/int64 local with only its
  // low N bits defined; there is no conv that isolates N bits. Mask instead,
  // and sign-extend from bit N-1 when the caller needs a signed value.
  unsigned Reg = N < 32 ? 32 : 64;
  if (N < Reg) {
    APInt Mask = APInt::getLowBitsSet(Reg, N);
    out() << "\tldc." << (Reg == 32 ? "i4" : "i8") << "\t"
          << Mask.getSExtValue() << "\n\tand\n";
    if (Signed) {
      unsigned Sh = Reg - N;
      out() << "\tldc.i4\t" << Sh << "\n\tshl\n"
            << "\tldc.i4\t" << Sh << "\n\tshr\n";
    }
  }
}

//===----------------------------------------------------------------------===//
// Constants
//===----------------------------------------------------------------------===//

// newer LLVM spells a splat as a vector-typed ConstantInt/ConstantFP
static bool isLaneConstant(const Constant *C) {
  if (isa<ConstantVector>(C) || isa<ConstantDataVector>(C))
    return true;
  return C->getType()->isVectorTy() && (isa<ConstantInt>(C) || isa<ConstantFP>(C));
}

void MSILWriter::printVectorConstant(const Constant *C) {
  auto *VTy = cast<FixedVectorType>(C->getType());
  Type *ElemTy = VTy->getElementType();
  uint64_t ElemSize = DL->getTypeAllocSize(ElemTy).getFixedValue();
  std::string Tmp = newTempLocal(VTy);
  out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t" << getTypeName(VTy) << "\n";
  std::string Sfx = getMemSuffix(ElemTy);
  for (unsigned I = 0, E = VTy->getNumElements(); I != E; ++I) {
    const Constant *Elt = C->getAggregateElement(I);
    if (!Elt || isa<UndefValue>(Elt) || Elt->isNullValue())
      continue;
    out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
    printPtrLoad(I * ElemSize);
    out() << "\tconv.i8\n\tadd\n";
    printConstLoad(Elt);
    out() << "\tstind." << Sfx << "\n";
  }
  out() << "\tldloc\t'" << Tmp << "'\n";
}

void MSILWriter::printConstLoad(const Constant *C) {
  Type *Ty = C->getType();

  if (isLaneConstant(C)) {
    printVectorConstant(C);
    return;
  }

  if (const auto *CI = dyn_cast<ConstantInt>(C)) {
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 1) {
      out() << "\tldc.i4." << (CI->isZero() ? "0" : "1") << '\n';
      return;
    }
    if (N == 128) {
      const APInt &V = CI->getValue();
      // Use the ctor only when the value needs both halves; op_Implicit is the
      // only way to build the common small case.
      if (V.isSignedIntN(64)) {
        out() << "\tldc.i8\t" << V.getSExtValue() << "\n\tcall\t" << Int128Ty
              << ' ' << Int128Ref << "::op_Implicit(int64)\n";
      } else {
        // .ctor(uint64 upper, uint64 lower): the halves are pushed in that
        // order, and ilasm reads ldc.i8 as signed.
        out() << "\tldc.i8\t" << (int64_t)V.getHiBits(64).getZExtValue()
              << "\n\tldc.i8\t" << (int64_t)V.getLoBits(64).getZExtValue()
              << "\n\tnewobj\tinstance void " << Int128Ty
              << "::.ctor(uint64, uint64)\n";
      }
      return;
    }
    out() << "\tldc." << (N <= 32 ? "i4" : "i8") << '\t' << CI->getSExtValue()
        << '\n';
    return;
  }

  if (const auto *FP = dyn_cast<ConstantFP>(C)) {
    const APFloat V = emittedFloat(FP->getValueAPF());
    // the JIT holds float constants as doubles, and the round trip quiets a
    // signaling NaN, so its bits go in as an int32 and are reread
    if (Ty->isFloatTy() && V.isSignaling()) {
      std::string Bits = newTempLocal(Type::getInt32Ty(Ty->getContext()));
      out() << "\tldc.i4\t" << (int32_t)V.bitcastToAPInt().getZExtValue()
            << "\n\tstloc\t'" << Bits << "'\n\tldloca\t'" << Bits
            << "'\n\tconv.u8\n\tldind.r4\n";
      return;
    }
    SmallString<40> Str;
    // A NaN or an infinity has no decimal literal ilasm accepts, and its
    // hex form is silently read as zero; the bit-pattern form is the only one
    // that round-trips.
    if (V.isNaN() || V.isInfinity()) {
      const APInt Bits = V.bitcastToAPInt();
      Str = "(";
      for (unsigned I = 0; I < Bits.getBitWidth(); I += 8) {
        if (I)
          Str.push_back(' ');
        raw_svector_ostream(Str) << format_hex_no_prefix(
            Bits.extractBitsAsZExtValue(8, I), 2);
      }
      Str.push_back(')');
    } else {
      V.toString(Str, 0, 0);
      // ilasm needs a '.' or exponent to lex the literal as floating point.
      if (Str.find_first_of(".eE") == StringRef::npos)
        Str += ".0";
    }
    out() << '\t' << (Ty->isFloatTy() ? "ldc.r4" : "ldc.r8") << '\t' << Str
        << '\n';
    return;
  }

  if (isa<UndefValue>(C) || isa<ConstantPointerNull>(C) ||
      isa<ConstantAggregateZero>(C) || C->isNullValue()) {
    if (isWideInt(Ty)) {
      out() << "\tldc.i8\t0\n\tcall\t" << Int128Ty << ' ' << Int128Ref
            << "::op_Implicit(int64)\n";
      return;
    }
    if (movesAsBlob(Ty)) {
      std::string Tmp = newTempLocal(Ty);
      out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t" << getTypeName(Ty)
          << "\n\tldloc\t'" << Tmp << "'\n";
    } else if (pointersAsInt64() && Ty->isPointerTy()) {
      // A null pointer in an int64 slot is `ldc.i8 0`; int32 would rely on the
      // implicit widening, which is fine but reads as a pointer-sized literal.
      out() << "\tldc.i8\t0\n";
    } else {
      out() << "\tldc.i4.0\n";
    }
    return;
  }

  if (isa<GlobalVariable>(C) || isa<Function>(C)) {
    printValueLoad(C);
    return;
  }

  if (const auto *CE = dyn_cast<ConstantExpr>(C)) {
    printConstantExpr(CE);
    return;
  }

  if (C->getType()->isAggregateType() || C->getType()->isVectorTy()) {
    // Build the blob field by field in a scratch local.
    std::string Tmp = newTempLocal(C->getType());
    out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t" << getTypeName(C->getType())
          << "\n";
    // initobj zeroed the padding, so only the non-zero leaves are stored.
    std::function<void(const Constant *, uint64_t)> Store =
        [&](const Constant *V, uint64_t At) {
          Type *Ty = V->getType();
          if (isa<ConstantAggregateZero>(V) ||
              (V->isNullValue() && !Ty->isAggregateType()))
            return;
          if (auto *STy = dyn_cast<StructType>(Ty)) {
            const StructLayout *SL = DL->getStructLayout(STy);
            for (unsigned I = 0; I != V->getNumOperands(); ++I)
              Store(cast<Constant>(V->getOperand(I)),
                    At + SL->getElementOffset(I));
            return;
          }
          if (auto *ATy = dyn_cast<ArrayType>(Ty)) {
            uint64_t Sz = DL->getTypeAllocSize(ATy->getElementType())
                              .getFixedValue();
            for (unsigned I = 0; I != ATy->getNumElements(); ++I)
              Store(cast<Constant>(V->getOperand(I)), At + I * Sz);
            return;
          }
          if (auto *VTy = dyn_cast<VectorType>(Ty)) {
            uint64_t Sz = DL->getTypeAllocSize(VTy->getElementType())
                              .getFixedValue();
            for (unsigned I = 0, E = V->getNumOperands(); I != E; ++I)
              Store(cast<Constant>(V->getOperand(I)), At + I * Sz);
            return;
          }
          out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
          if (At) {
            printPtrLoad(At);
            out() << "\tconv.i8\n\tadd\n";
          }
          printConstLoad(V);
          if (movesAsBlob(Ty))
            out() << "\tstobj\t" << getTypeName(Ty) << "\n";
          else
            out() << "\tstind." << getMemSuffix(Ty) << "\n";
        };
    Store(C, 0);
    out() << "\tldloc\t'" << Tmp << "'\n";
    return;
  }

  fatalError("unsupported constant: " + Twine(str(C)));
}

void MSILWriter::printConstantExpr(const ConstantExpr *CE) {
  switch (CE->getOpcode()) {
  case Instruction::GetElementPtr:
    if (auto *GEP = cast<GEPOperator>(CE); GEP->getType()->isVectorTy())
      printVectorGep(GEP);
    else
      printGepInstruction(GEP);
    return;
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
    if (isWideInt(CE->getType()) || isWideInt(CE->getOperand(0)->getType())) {
      printCastInstruction(CE->getOpcode(), CE->getOperand(0), CE->getType());
      return;
    }
    printValueLoad(CE->getOperand(0));
    return;
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    // A 128-bit constant expression would need the method-call expansion; none
    // of these survive to the writer for i128 anyway, and the scalar path
    // below would emit raw IL opcodes that do not exist for it.
    if (isWideInt(CE->getType()))
      fatalError("unsupported 128-bit constant expression");
    // InstCombine rewrites GEPs into pointer-offset arithmetic, so pointer
    // math shows up as constant expressions, not just as casts.
    const char *Op = nullptr;
    switch (CE->getOpcode()) {
    case Instruction::Add: Op = "add"; break;
    case Instruction::Sub: Op = "sub"; break;
    case Instruction::Mul: Op = "mul"; break;
    case Instruction::Shl: Op = "shl"; break;
    case Instruction::LShr: Op = "shr.un"; break;
    case Instruction::AShr: Op = "shr"; break;
    case Instruction::And: Op = "and"; break;
    case Instruction::Or: Op = "or"; break;
    default: Op = "xor"; break;
    }
    if (CE->getOpcode() == Instruction::LShr)
      printValueLoadAsInt(CE->getOperand(0), false);
    else
      printValueLoad(CE->getOperand(0));
    printValueLoad(CE->getOperand(1));
    out() << '\t' << Op << '\n';
    return;
  }
  default:
    printCastInstruction(CE->getOpcode(), CE->getOperand(0), CE->getType());
    return;
  }
}

//===----------------------------------------------------------------------===//
// Memory access
//===----------------------------------------------------------------------===//

void MSILWriter::printIndirectLoad(const Value *Ptr, Type *Ty) {
  unsigned Field = 0;
  const Value *Base = nullptr;
  if (StructType *STy = structFieldOf(Ptr, Ty, Field, Base)) {
    printValueLoad(Base);
    printPtrToNative();
    out() << "\tldfld " << getTypeName(Ty) << " '"
          << getAggregateName(STy) << "'::'f" << Field << "'\n";
    return;
  }
  printValueLoad(Ptr);
  printPtrToNative();
  if (movesAsBlob(Ty))
    out() << "\tldobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
}

void MSILWriter::printIndirectSave(const Value *Ptr, const Value *Val) {
  unsigned Field = 0;
  const Value *Base = nullptr;
  // stfld wants the address first, same as stind, so the order is unchanged.
  if (StructType *STy = structFieldOf(Ptr, Val->getType(), Field, Base)) {
    printValueLoad(Base);
    printPtrToNative();
    printValueLoad(Val);
    out() << "\tstfld " << getTypeName(Val->getType()) << " '"
          << getAggregateName(STy) << "'::'f" << Field << "'\n";
    return;
  }
  printValueLoad(Ptr);
  printPtrToNative();
  printValueLoad(Val);
  if (movesAsBlob(Val->getType()))
    out() << "\tstobj\t" << getTypeName(Val->getType()) << '\n';
  else
    out() << "\tstind." << getMemSuffix(Val->getType()) << '\n';
}

//===----------------------------------------------------------------------===//
// Casts
//===----------------------------------------------------------------------===//

void MSILWriter::printCastInstruction(unsigned Op, const Value *V, Type *Ty) {
  // i128 conversions are the value type's conversion operators; no IL conv
  // applies to a value type.
  Type *SrcTy = V->getType();
  if (isWideInt(Ty) || isWideInt(SrcTy)) {
    switch (Op) {
    case Instruction::Trunc:
      // op_Explicit(int64) keeps the low half, then narrow as usual.
      printValueLoad(V);
      out() << "\tcall\tint64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n";
      if (Ty->getIntegerBitWidth() <= 32)
        out() << '\t' << widthConv(Ty) << '\n';
      return;
    case Instruction::ZExt:
    case Instruction::PtrToInt:
    case Instruction::PtrToAddr:
      // A narrow or 64-bit value is held zero-extended on the stack, so conv.u8
      // is the whole zero-extension.
      printValueLoad(V);
      out() << "\tconv.u8\n\tcall\t" << Int128Ty << ' ' << Int128Ref
            << "::op_Implicit(uint64)\n";
      return;
    case Instruction::SExt:
      printValueLoadAsInt(V, true);
      out() << "\tconv.i8\n\tcall\t" << Int128Ty << ' ' << Int128Ref
            << "::op_Implicit(int64)\n";
      return;
    case Instruction::IntToPtr:
      printValueLoad(V);
      out() << "\tcall\tint64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n\tconv.i\n";
      return;
    case Instruction::UIToFP:
      printValueLoad(V);
      out() << "\tcall\tfloat64 " << UInt128Ref << "::op_Explicit(" << UInt128Ty
            << ")\n";
      if (Ty->isFloatTy())
        out() << "\tconv.r4\n";
      return;
    case Instruction::SIToFP:
      printValueLoad(V);
      out() << "\tcall\tfloat64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n";
      if (Ty->isFloatTy())
        out() << "\tconv.r4\n";
      return;
    case Instruction::FPToUI:
    case Instruction::FPToSI: {
      // The conversion operators take float64; the float32 case widens first.
      const char *T = Op == Instruction::FPToUI ? UInt128Ty : Int128Ty;
      printValueLoad(V);
      if (SrcTy->isFloatTy())
        out() << "\tconv.r8\n";
      out() << "\tcall\t" << T << ' ' << T << "::op_Explicit(float64)\n";
      return;
    }
    case Instruction::BitCast:
    case Instruction::AddrSpaceCast: {
      // A 128-bit bitcast is always to or from a same-size blob; move the
      // bytes through memory because the two value types cannot be assigned
      // to each other.
      std::string Tmp = newTempLocal(SrcTy);
      printValueLoad(V);
      out() << "\tstloc\t'" << Tmp << "'\n\tldloca\t'" << Tmp
            << "'\n\tconv.u8\n\tldobj\t" << getTypeName(Ty) << '\n';
      return;
    }
    default:
      fatalError("unsupported 128-bit cast: " +
                         Twine(Instruction::getOpcodeName(Op)));
    }
  }

  bool VectorInvolved = Ty->isVectorTy() || V->getType()->isVectorTy();
  if (Op == Instruction::BitCast && VectorInvolved) {
    if (V->getType() == Ty)
      printValueLoad(V);
    else
      printVectorBitcast(V, Ty);
    return;
  }

  // The scalar paths below would truncate the aggregate to a single value.
  if (auto *DstVTy = dyn_cast<VectorType>(Ty)) {
    if (auto *SrcVTy = dyn_cast<VectorType>(V->getType())) {
      if (SrcVTy->getElementCount() == DstVTy->getElementCount() &&
          SrcVTy->getElementType() != DstVTy->getElementType()) {
        printVectorCast(Op, V, Ty);
        return;
      }
    }
  }

  switch (Op) {
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast: {
    Type *SrcTy = V->getType();
    if (SrcTy == Ty)
      return;
    // IL is typed on the evaluation stack: `stloc` into a local of another
    // type converts, it does not reinterpret. Round-trip through memory of the
    // source type so the bits survive unchanged.
    if ((Ty->isFloatingPointTy() && SrcTy->isIntegerTy()) ||
        (Ty->isIntegerTy() && SrcTy->isFloatingPointTy())) {
      std::string Tmp = newTempLocal(SrcTy);
      printValueLoad(V);
      out() << "\tstloc\t'" << Tmp << "'\n\tldloca\t'" << Tmp
            << "'\n\tconv.u8\n\tldind." << getMemSuffix(Ty) << '\n';
      return;
    }
    printValueLoad(V);
    return;
  }

  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::PtrToAddr:
    printValueLoad(V);
    if (Ty->isIntegerTy(1))
      out() << "\tldc.i4.0\n\tcgt.un\n";
    else if (Ty->isIntegerTy(8))
      out() << "\tconv.u1\n";
    else if (Ty->isIntegerTy(16))
      out() << "\tconv.u2\n";
    else if (Ty->isIntegerTy(32))
      out() << "\tconv.u4\n";
    else
      out() << "\tconv.u8\n";
    return;

  case Instruction::Trunc:
    printValueLoad(V);
    // Truncate to the destination width (wide destinations saturate at i64).
    out() << "\tconv.u8\n";
    if (Ty->getIntegerBitWidth() <= 32)
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::ZExt:
    // Mask the source to its declared width first. An i8/i16 argument arrives
    // sign-extended on the int32 stack, so plain conv.u8 would keep the high
    // bits (0xFFFFFFAF instead of 0xAF) before the destination truncation.
    printValueLoadAsInt(V, false);
    if (Ty->getIntegerBitWidth() > 32)
      out() << "\tconv.u8\n";
    else if (Ty->getIntegerBitWidth() < 32)
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::SExt:
    printValueLoadAsInt(V, true);
    if (Ty->getIntegerBitWidth() > 32)
      out() << "\tconv.i8\n";
    else
      out() << "\tconv.i4\n";
    return;

  case Instruction::UIToFP:
    printValueLoad(V);
    out() << "\tconv.r.un\n";
    out() << (Ty->isFloatTy() ? "\tconv.r4\n" : "\tconv.r8\n");
    return;
  case Instruction::SIToFP:
    // Narrow integers are held zero-extended, so the sign must be restored
    // before the conversion or the value reads as a large positive number.
    printValueLoadAsInt(V, true);
    out() << (Ty->isFloatTy() ? "\tconv.r4\n" : "\tconv.r8\n");
    return;
  case Instruction::FPToUI:
    printValueLoad(V);
    out() << "\tconv.u4\n";
    if (Ty->isIntegerTy(64))
      out() << "\tconv.u8\n";
    else if (!Ty->isIntegerTy(32))
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::FPToSI:
    printValueLoad(V);
    out() << "\tconv.i4\n";
    if (Ty->isIntegerTy(64))
      out() << "\tconv.i8\n";
    else if (!Ty->isIntegerTy(32))
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::FPExt:
    printValueLoad(V);
    out() << "\tconv.r8\n";
    return;
  case Instruction::FPTrunc:
    printValueLoad(V);
    // With -msil-fp80 an f80->f64 truncation is between two float64 slots and
    // converts nothing; only a genuinely narrower destination needs conv.r4.
    if (V->getType()->isX86_FP80Ty() && fp80MappedToDouble() &&
        !Ty->isFloatTy())
      return;
    out() << "\tconv.r4\n";
    return;
  default:
    fatalError("unsupported cast: " +
                       Twine(Instruction::getOpcodeName(Op)));
  }
}

//===----------------------------------------------------------------------===//
// GEP
//===----------------------------------------------------------------------===//

void MSILWriter::printGepInstruction(const User *GEP) {
  printValueLoad(GEP->getOperand(0));
  out() << "\tconv.u8\n";

  for (gep_type_iterator I = gep_type_begin(GEP), E = gep_type_end(GEP); I != E;
       ++I) {
    const Value *Idx = I.getOperand();
    if (StructType *STy = I.getStructTypeOrNull()) {
      unsigned Field = cast<ConstantInt>(Idx)->getZExtValue();
      uint64_t Off = DL->getStructLayout(STy)->getElementOffset(Field);
      if (Off) {
        printPtrLoad(Off);
        out() << "\tconv.i8\n\tadd\n";
      }
      continue;
    }

    uint64_t ElemSize =
        I.getSequentialElementStride(*DL).getFixedValue();
    if (const auto *CI = dyn_cast<ConstantInt>(Idx)) {
      const APInt &A = CI->getValue();
      if (A.isZero())
        continue;
      if (A.isNegative()) {
        printPtrLoad(A.abs().getZExtValue() * ElemSize);
        out() << "\tconv.i8\n\tsub\n";
      } else {
        printPtrLoad(A.getZExtValue() * ElemSize);
        out() << "\tconv.i8\n\tadd\n";
      }
      continue;
    }
    printValueLoad(Idx);
    out() << "\tconv.i8\n";
    printPtrLoad(ElemSize);
    out() << "\tconv.i8\n\tmul\n\tadd\n";
  }
}

//===----------------------------------------------------------------------===//
// Comparisons
//===----------------------------------------------------------------------===//

// Lane-wise vector comparison: compare lane I of both blobs into lane I of the
// result (an i1 vector, one byte per lane).
void MSILWriter::printVectorCmp(unsigned Pred, const Value *L, const Value *R,
                                VectorType *ResTy, bool IsFP) {
  auto *SrcTy = cast<VectorType>(L->getType());
  Type *EltTy = SrcTy->getElementType();
  uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
  unsigned N = ResTy->getElementCount().getFixedValue();
  std::string A = newTempLocal(SrcTy), B = newTempLocal(SrcTy),
              Res = newTempLocal(ResTy);
  printValueLoad(L);
  out() << "\tstloc\t'" << A << "'\n";
  printValueLoad(R);
  out() << "\tstloc\t'" << B << "'\n";
  out() << "\tldloca\t'" << Res << "'\n\tinitobj\t"
        << getTypeName(ResTy) << "\n";
  std::string Sfx = getMemSuffix(EltTy);
  for (unsigned I = 0; I < N; ++I) {
    auto LoadLaneA = [&] {
      printAggregateLaneAddr(A, EltTy, I * ElemSize);
      out() << "\tldind." << Sfx << "\n";
    };
    auto LoadLaneB = [&] {
      printAggregateLaneAddr(B, EltTy, I * ElemSize);
      out() << "\tldind." << Sfx << "\n";
    };
    printAggregateLaneAddr(Res, ResTy->getElementType(), I);
    if (IsFP) {
      printFCmp(Pred, LoadLaneA, LoadLaneB);
    } else {
      LoadLaneA();
      LoadLaneB();
      printICmp(Pred, nullptr, nullptr, /*EmitOperands=*/false);
    }
    out() << "\tstind.i1\n";
  }
  out() << "\tldloc\t'" << Res << "'\n";
}

void MSILWriter::printICmp(unsigned Pred, const Value *L, const Value *R,
                           bool EmitOperands) {
  auto LoadL = [&] { if (EmitOperands) printValueLoad(L); };
  auto LoadR = [&] { if (EmitOperands) printValueLoad(R); };
  bool U = Pred == CmpInst::ICMP_ULT || Pred == CmpInst::ICMP_ULE ||
           Pred == CmpInst::ICMP_UGT || Pred == CmpInst::ICMP_UGE;
  if (EmitOperands) {
    printValueLoadAsInt(L, !U);
    printValueLoadAsInt(R, !U);
  }

  switch (Pred) {
  case CmpInst::ICMP_EQ:
    out() << "\tceq\n";
    return;
  case CmpInst::ICMP_NE:
    out() << "\tceq\n";
    printNot();
    return;
  case CmpInst::ICMP_ULT:
  case CmpInst::ICMP_SLT:
    out() << (U ? "\tclt.un\n" : "\tclt\n");
    return;
  case CmpInst::ICMP_UGT:
  case CmpInst::ICMP_SGT:
    out() << (U ? "\tcgt.un\n" : "\tcgt\n");
    return;
  case CmpInst::ICMP_ULE:
  case CmpInst::ICMP_SLE:
    out() << (U ? "\tcgt.un\n" : "\tcgt\n");
    printNot();
    return;
  case CmpInst::ICMP_UGE:
  case CmpInst::ICMP_SGE:
    out() << (U ? "\tclt.un\n" : "\tclt\n");
    printNot();
    return;
  default:
    llvm_unreachable("unknown icmp predicate");
  }
}

void MSILWriter::printFCmp(unsigned Pred, function_ref<void()> LoadL,
                           function_ref<void()> LoadR) {
  // ceq/clt/cgt are false for NaN, clt.un/cgt.un are true
  auto Cmp = [&](const char *Op) {
    LoadL();
    LoadR();
    out() << '\t' << Op << '\n';
  };
  auto Ordered = [&](const char *Op) {
    // (L Op R) && (L == L) && (R == R)
    Cmp(Op);
    LoadL();
    LoadL();
    out() << "\tceq\n\tand\n";
    LoadR();
    LoadR();
    out() << "\tceq\n\tand\n";
  };
  auto Unordered = [&]() {
    LoadL();
    LoadL();
    out() << "\tceq\n";
    printNot();
    LoadR();
    LoadR();
    out() << "\tceq\n";
    printNot();
    out() << "\tor\n";
  };

  switch (Pred) {
  case CmpInst::FCMP_OEQ:
    Ordered("ceq");
    return;
  case CmpInst::FCMP_UNE:
    Cmp("ceq");
    printNot();
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_OLT:
    Ordered("clt");
    return;
  case CmpInst::FCMP_OGT:
    Ordered("cgt");
    return;
  case CmpInst::FCMP_OLE:
    Cmp("cgt.un");
    printNot();
    return;
  case CmpInst::FCMP_OGE:
    Cmp("clt.un");
    printNot();
    return;
  case CmpInst::FCMP_ULT:
    Cmp("clt");
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_UGT:
    Cmp("cgt");
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_ULE:
    Cmp("cgt");
    printNot();
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_UGE:
    Cmp("clt");
    printNot();
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_ORD:
    LoadL();
    LoadL();
    out() << "\tceq\n";
    LoadR();
    LoadR();
    out() << "\tceq\n\tand\n";
    return;
  case CmpInst::FCMP_UNO:
    Unordered();
    return;
  case CmpInst::FCMP_UEQ:
    Cmp("ceq");
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_ONE:
    Cmp("ceq");
    printNot();
    LoadL();
    LoadL();
    out() << "\tceq\n\tand\n";
    LoadR();
    LoadR();
    out() << "\tceq\n\tand\n";
    return;
  case CmpInst::FCMP_FALSE:
    out() << "\tldc.i4.0\n";
    return;
  case CmpInst::FCMP_TRUE:
    out() << "\tldc.i4.1\n";
    return;
  default:
    llvm_unreachable("unknown fcmp predicate");
  }
}

//===----------------------------------------------------------------------===//
// 128-bit integers
//===----------------------------------------------------------------------===//

// System.Int128 has no IL arithmetic: every operator is a static method on the
// value type. Operands are already the right value type on the stack, so each
// case is just "load, load, call".
namespace {
/// Reinterpret the Int128 on the stack as UInt128. Used wherever an operation
/// only exists on the unsigned type, or must be unsigned.
void emitAsUInt128(raw_ostream &Out) {
  Out << "\tcall\t" << UInt128Ty << ' ' << Int128Ref << "::op_Explicit("
      << Int128Ty << ")\n";
}

/// And back. The reverse conversion exists on UInt128, not Int128.
void emitFromUInt128(raw_ostream &Out) {
  Out << "\tcall\t" << Int128Ty << ' ' << UInt128Ref << "::op_Explicit("
      << UInt128Ty << ")\n";
}
} // namespace

void MSILWriter::printWideBinary(unsigned Op, const Value *L,
                                 const Value *R) {
  printWideBinaryOn(Op, [&] { printValueLoad(L); },
                    [&] { printValueLoad(R); },
                    R && isWideInt(R->getType()));
}

// The same, with the operands pushed by the caller: an atomic read-modify-write
// keeps the value it read in a local, not in an IR value, so it cannot name one.
void MSILWriter::printWideBinaryOn(unsigned Op, function_ref<void()> LoadL,
                                   function_ref<void()> LoadR, bool WideR) {
  const char *Name;
  switch (Op) {
  case Instruction::Add:  Name = "op_Addition"; break;
  case Instruction::Sub:  Name = "op_Subtraction"; break;
  case Instruction::Mul:  Name = "op_Multiply"; break;
  case Instruction::UDiv:
  case Instruction::SDiv: Name = "op_Division"; break;
  case Instruction::URem:
  case Instruction::SRem: Name = "op_Modulus"; break;
  case Instruction::And:  Name = "op_BitwiseAnd"; break;
  case Instruction::Or:   Name = "op_BitwiseOr"; break;
  case Instruction::Xor:  Name = "op_ExclusiveOr"; break;
  case Instruction::Shl:  Name = "op_LeftShift"; break;
  case Instruction::LShr: Name = "op_UnsignedRightShift"; break;
  case Instruction::AShr: Name = "op_RightShift"; break;
  default:
    fatalError("unsupported 128-bit operation");
  }

  // Unsigned division and remainder have no Int128 method, so compute on
  // UInt128 and reinterpret back.
  if (Op == Instruction::UDiv || Op == Instruction::URem) {
    LoadL();
    emitAsUInt128(out());
    LoadR();
    emitAsUInt128(out());
    out() << "\tcall\t" << UInt128Ty << ' ' << UInt128Ref << "::" << Name
          << "(" << UInt128Ty << ", " << UInt128Ty << ")\n";
    emitFromUInt128(out());
    return;
  }

  LoadL();
  LoadR();
  // The shift methods take an int32 count and reduce it modulo 128, which is
  // LLVM's shl/lshr/ashr semantics for an out-of-range amount. The IR count is
  // i128 like the value, so it needs a conversion before conv.i4.
  bool Shift = Op == Instruction::Shl || Op == Instruction::LShr ||
               Op == Instruction::AShr;
  if (Shift) {
    if (WideR)
      out() << "\tcall\tint64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n";
    out() << "\tconv.i4\n";
  }
  out() << "\tcall\t" << Int128Ty << ' ' << Int128Ref << "::" << Name << "("
        << Int128Ty << ", " << (Shift ? "int32" : Int128Ty) << ")\n";
}

// Comparisons return bool from op_Equality/op_LessThan/.... The relational
// forms are strict, so <= and >= negate the mirror form, as printICmp does.
void MSILWriter::printWideICmp(unsigned Pred, const Value *L, const Value *R) {
  bool U = Pred == CmpInst::ICMP_ULT || Pred == CmpInst::ICMP_ULE ||
           Pred == CmpInst::ICMP_UGT || Pred == CmpInst::ICMP_UGE;
  const char *T = U ? UInt128Ty : Int128Ty;
  // <= and >= are the negation of the strict predicate on the same operands.
  unsigned Mirror = Pred;
  bool Negate = false;
  switch (Pred) {
  case CmpInst::ICMP_ULE: Mirror = CmpInst::ICMP_UGT; Negate = true; break;
  case CmpInst::ICMP_UGE: Mirror = CmpInst::ICMP_ULT; Negate = true; break;
  case CmpInst::ICMP_SLE: Mirror = CmpInst::ICMP_SGT; Negate = true; break;
  case CmpInst::ICMP_SGE: Mirror = CmpInst::ICMP_SLT; Negate = true; break;
  default: break;
  }
  const char *Name;
  switch (Mirror) {
  case CmpInst::ICMP_EQ:  Name = "op_Equality"; break;
  case CmpInst::ICMP_NE:  Name = "op_Inequality"; break;
  case CmpInst::ICMP_ULT:
  case CmpInst::ICMP_SLT: Name = "op_LessThan"; break;
  case CmpInst::ICMP_UGT:
  case CmpInst::ICMP_SGT: Name = "op_GreaterThan"; break;
  default:
    llvm_unreachable("unknown icmp predicate");
  }

  printValueLoad(L);
  if (U)
    emitAsUInt128(out());
  printValueLoad(R);
  if (U)
    emitAsUInt128(out());
  out() << "\tcall\tbool " << (U ? UInt128Ref : Int128Ref) << "::" << Name
        << "(" << T << ", " << T << ")\n";
  if (Negate)
    printNot();
}

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

void MSILWriter::printIntrinsicCall(const IntrinsicInst *II) {
  switch (II->getIntrinsicID()) {
  case Intrinsic::vastart: {
    // va_start fills the va_list slot the front end allocated so the C that
    // follows reads the trailing arguments the same way on both ABIs.
    //
    // The trailing arguments arrive as the last parameter, a pointer to a
    // buffer of 8-byte slots (see isEmittedVarArgDef). On Windows the slot is
    // the cursor, so it gets that pointer. On ELF the slot is a
    // __va_list_tag and every argument is forced down the overflow path --
    // gp_offset past its limit, fp_offset past its limit, so the front end's
    // own va_arg takes overflow_arg_area every time -- and that field gets
    // the pointer. Both then advance 8 bytes for every read.
    if (!F->isVarArg())
      fatalError("va_start in a non-vararg function");
    // The buffer is the method's last parameter, appended to the declared
    // ones; IR arguments do not include it.
    auto Field = [&](uint64_t Offset) {
      printValueLoad(II->getArgOperand(0));
      if (Offset)
        out() << "\tldc.i8\t" << Offset << "\n\tadd\n";
    };
    auto Buffer = [&] { out() << "\tldarg\t'$va_args'\n\tconv.u\n"; };
    if (vaIsTag()) {
      // overflow_arg_area is field 2, at offset 8: the buffer of slots.
      Field(8);
      Buffer();
      out() << "\tstind.i\n";
      // gp_offset (field 0) and fp_offset (field 1) past their limits, which
      // is what makes the front end's own va_arg take the stack path every
      // time, so both triples advance the same 8-byte slots.
      Field(0);
      out() << "\tldc.i4\t48\n\tstind.i4\n";
      Field(4);
      out() << "\tldc.i4\t304\n\tstind.i4\n";
    } else {
      // The slot *is* the cursor.
      Field(0);
      Buffer();
      out() << "\tstind.i\n";
    }
    return;
  }
  case Intrinsic::vaend:
    return;
  case Intrinsic::vacopy: {
    // The destination is the va_list the front end allocated; the source is a
    // pointer to one. A va_list here is plain memory (a cursor on Windows, a
    // __va_list_tag on ELF), so the copy is byte for byte. Copying the tag
    // rather than sharing it is what makes `va_list a = b` independent, which
    // is the C rule.
    printValueLoad(II->getArgOperand(0));
    printValueLoad(II->getArgOperand(1));
    out() << "\tldc.i4\t" << vaSlotSize() << "\n\tcpblk\n";
    return;
  }
  default: {
    // The call and callee types are included because the interesting failures
    // are overload mismatches: the intrinsic exists, but with a signature the
    // lowering did not recognise, so the name alone does not explain it.
    std::string Msg;
    raw_string_ostream OS(Msg);
    OS << "intrinsic survived lowering: ";
    if (Function *G = dyn_cast<Function>(II->getCalledOperand()))
      OS << G->getName() << " as " << *II->getFunctionType();
    else
      OS << "an indirect call";
    fatalError(Twine(Msg));
  }
  }
}

// How a trailing vararg crosses: the one CLR type both the callee's read and
// the pinvoke thunk agree on. See getPortableVarArgTypeName and
// getVarArgTypeName; a definition this module emits reads every slot back as
// int64, so its own calls normalize the same way.
void MSILWriter::printVarArgValue(const Value *Arg, bool Portable,
                                  bool AsSlot) {
  Type *Ty = Arg->getType();
  printValueLoad(Arg);
  if (Ty->isFloatingPointTy() && (AsSlot || getVarArgTypeName(Ty) == "int64")) {
    // A slot is 8 bytes, and C's default promotions make a float vararg a
    // double before it reaches the IR; widen one that somehow did not, so the
    // read below does not take four bytes of neighbouring memory.
    if (AsSlot && Ty->isFloatTy())
      out() << "\tconv.r8\n";
    Type *ITy = AsSlot ? Type::getDoubleTy(Mod->getContext()) : Ty;
    // Reinterpret, do not convert. Storing an r8 into an int64 local through
    // stloc *converts* it, so go through memory instead: the bits of the
    // local are what the callee reads.
    std::string Tmp = newTempLocal(ITy);
    out() << "\tstloc\t'" << Tmp << "'\n\tldloca\t'" << Tmp
          << "'\n\tconv.u8\n\tldind.i8\n";
    return;
  }
  if ((Portable || AsSlot) && Ty->isIntegerTy()) {
    // Normalize to the declared width first so the sign is right, then widen.
    // A narrow vararg arrives zero-extended, and conv.i8 alone would take
    // that as the value.
    unsigned Bits = Ty->getIntegerBitWidth();
    out() << '\t' << widthConv(Ty) << '\n';
    if (Bits < 32) {
      // Sign-extend from the declared width: a narrow vararg arrives
      // zero-extended, and conv.i8 alone would take that at face value.
      out() << "\tconv.i" << Bits << "\n";
    }
    if (Bits <= 32)
      out() << "\tconv.i8\n";
  }
}

// A call to a vararg definition this module emits takes a pointer to a buffer
// of trailing arguments instead of the arguments themselves (see
// isEmittedVarArgDef). The buffer lives in a local: a `localloc` would put it
// in the caller's frame only until the call returns, which is enough, but a
// local array is simpler and the CLR zero-initializes it. Every slot is one
// int64, so the buffer is `int64[n]`.
void MSILWriter::printVarArgBuffer(const CallBase *CB) {
  unsigned Fixed = CB->getFunctionType()->getNumParams();
  unsigned N = varArgSlots(CB);
  Type *I64 = Type::getInt64Ty(Mod->getContext());
  std::string Buf = newTempLocal(ArrayType::get(I64, N));

  for (unsigned I = Fixed, E = CB->arg_size(); I != E; ++I) {
    out() << "\tldloca\t'" << Buf << "'\n\tconv.u8\n";
    if (I - Fixed)
      out() << "\tldc.i8\t" << (I - Fixed) * 8 << "\n\tadd\n";
    printVarArgValue(CB->getArgOperand(I), /*Portable=*/false,
                     /*AsSlot=*/true);
    out() << "\tstind.i8\n";
  }
  out() << "\tldloca\t'" << Buf << "'\n\tconv.u\n";
}

void MSILWriter::printFunctionCall(const Value *FnVal, const CallBase *CB) {
  FunctionType *FTy = CB->getFunctionType();
  // A direct call may name the callee through a bitcast; the definition is
  // what decides whether it takes a buffer.
  const Function *Callee =
      dyn_cast<Function>(FnVal->stripPointerCasts());
  bool EmittedDef = isEmittedVarArgDef(Callee);

  // Arguments must reach the callee as its signature declares them. A call to
  // an emitted vararg definition replaces its trailing arguments with the
  // buffer, so only the fixed ones are pushed here.
  unsigned Fixed = FTy->getNumParams();
  // Decided from the call site, not the callee: after a front-end wrapper is
  // dropped the operand is a declaration, and the name is what says which
  // library the call belongs to.
  bool Portable = EmitPortable && Callee && !Callee->isIntrinsic() &&
                  isPortableLibcSymbol(getCName(Callee));
  unsigned E = EmittedDef ? Fixed : CB->arg_size();
  for (unsigned I = 0; I != E; ++I) {
    if (!EmittedDef && FTy->isVarArg() && I >= Fixed)
      printVarArgValue(CB->getArgOperand(I), Portable, /*AsSlot=*/false);
    else
      printValueLoad(CB->getArgOperand(I));
  }
  if (EmittedDef)
    printVarArgBuffer(CB);

  if (!Callee) {
    // Indirect call: the target sits above the arguments on the stack.
    //
    // The signature is taken as if the call were not vararg. A C function
    // pointer with no prototype (`void (*routine)()`) is vararg in the IR, but
    // every call through it passes only the fixed arguments, and the CLR
    // rejects `calli vararg` outright on some runtimes (CoreCLR on Linux
    // raises InvalidProgramException). Dropping the marker is the same call:
    // no trailing argument types are being named anyway.
    printValueLoad(FnVal);
    // A function pointer is a pointer-typed value: with -msil-pointers=i64 it
    // sits in the stack as an int64, and calli wants a native int.
    printPtrToNative();
    std::string Sig;
    Sig += getTypeName(FTy->getReturnType());
    Sig += " (";
    for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
      if (I)
        Sig += ", ";
      Sig += getTypeName(FTy->getParamType(I));
    }
    Sig += ")";
    out() << "\tcalli\t" << Sig << '\n';
    return;
  }

  if (Callee->isDeclaration()) {
    // Calls to external vararg functions bind to a concrete pinvoke thunk
    // whose signature lists the actual trailing argument types.
    auto It = PInvokeNames.find({Callee, getCallSignature(FTy, CB, "", Portable)});
    if (It != PInvokeNames.end()) {
      std::string Sig = getTypeName(FTy->getReturnType()) + " '" + It->second +
                        "'(";
      for (unsigned I = 0, E = CB->arg_size(); I != E; ++I) {
        if (I)
          Sig += ", ";
        Type *Ty = CB->getArgOperand(I)->getType();
        Sig += (FTy->isVarArg() && I >= Fixed)
                   ? (Portable ? getPortableVarArgTypeName(Ty)
                               : getVarArgTypeName(Ty))
                   : getTypeName(Ty);
      }
      Sig += ")";
      out() << "\tcall\t" << Sig << '\n';
      return;
    }
  }

  out() << "\tcall\t"
      << getCallSignature(FTy, CB, getFunctionRef(Callee), Portable,
                          EmittedDef) << '\n';
}

void MSILWriter::printCallInstruction(const CallBase *CB) {
  if (const auto *II = dyn_cast<IntrinsicInst>(CB)) {
    printIntrinsicCall(II);
    return;
  }
  printFunctionCall(CB->getCalledOperand(), CB);
}

void MSILWriter::printPHICopy(const BasicBlock *From, const BasicBlock *To) {
  // Phi copies are one parallel assignment: an incoming value may itself be a
  // phi of this block, and writing that local first would lose the old value.
  // Read every source before writing any destination.
  SmallVector<std::pair<std::string, const PHINode *>, 8> Copies;
  for (const PHINode &Phi : To->phis()) {
    const Value *V = Phi.getIncomingValueForBlock(From);
    if (isa<UndefValue>(V))
      continue;
    std::string Tmp = newTempLocal(Phi.getType());
    printValueLoad(V);
    out() << "\tstloc\t'" << Tmp << "'\n";
    Copies.push_back({Tmp, &Phi});
  }
  for (auto &C : Copies) {
    out() << "\tldloc\t'" << C.first << "'\n";
    printValueSave(C.second);
  }
}

void MSILWriter::printBranchInstruction(const Instruction *Inst) {
  const BasicBlock *From = Inst->getParent();
  bool Unconditional = Inst->getNumSuccessors() == 1;
  if (Unconditional) {
    printPHICopy(From, Inst->getSuccessor(0));
    out() << "\tbr\t" << getLabelName(Inst->getSuccessor(0)) << '\n';
    return;
  }

  const BasicBlock *T = Inst->getSuccessor(0);
  const BasicBlock *F = Inst->getSuccessor(1);
  bool TPhi = !T->phis().empty();
  bool FPhi = !F->phis().empty();

  const Value *Condition = Inst->getOperand(0);
  printValueLoad(Condition);

  if (!TPhi && !FPhi) {
    out() << "\tbrtrue\t" << getLabelName(T) << '\n';
    out() << "\tbr\t" << getLabelName(F) << '\n';
    return;
  }

  // Phi copies may only run on the edge actually taken, so each edge that
  // needs copies gets a private trampoline block.
  std::string TTramp = "IL_phi" + utostr(getUniqID());
  out() << "\tbrtrue\t" << TTramp << '\n';
  printPHICopy(From, F);
  out() << "\tbr\t" << getLabelName(F) << '\n';
  out() << TTramp << ":\n";
  printPHICopy(From, T);
  out() << "\tbr\t" << getLabelName(T) << '\n';
}

void MSILWriter::printInstruction(const Instruction *Inst) {
  const Value *Op0 = Inst->getNumOperands() > 0 ? Inst->getOperand(0) : nullptr;
  const Value *Op1 = Inst->getNumOperands() > 1 ? Inst->getOperand(1) : nullptr;
  Type *Ty = Inst->getType();

  // A register-wide vector runs on Vector<T>; every other shape, and every
  // operation Vector<T> cannot express, keeps the lane-by-lane expansion.
  if (Ty->isVectorTy()) {
    if (tryPrintSimdBinary(Inst))
      return;
    if (Inst->getOpcode() == Instruction::Select && tryPrintSimdSelect(Inst))
      return;

    const char *Op = nullptr;
    bool ZeroExt = false, Shift = false;
    switch (Inst->getOpcode()) {
    case Instruction::Add: Op = "add"; break;
    case Instruction::Sub: Op = "sub"; break;
    case Instruction::Mul: Op = "mul"; break;
    case Instruction::And: Op = "and"; break;
    case Instruction::Or:  Op = "or"; break;
    case Instruction::Xor: Op = "xor"; break;
    case Instruction::UDiv: Op = "div.un"; ZeroExt = true; break;
    case Instruction::URem: Op = "rem.un"; ZeroExt = true; break;
    case Instruction::SDiv: Op = "div"; break;
    case Instruction::SRem: Op = "rem"; break;
    case Instruction::FAdd: Op = "add"; break;
    case Instruction::FSub: Op = "sub"; break;
    case Instruction::FMul: Op = "mul"; break;
    case Instruction::FDiv: Op = "div"; break;
    case Instruction::FRem: Op = "rem"; break;
    case Instruction::Shl: Op = "shl"; Shift = true; break;
    case Instruction::LShr: Op = "shr.un"; ZeroExt = Shift = true; break;
    case Instruction::AShr: Op = "shr"; Shift = true; break;
    default: break;
    }
    if (Op) {
      printVectorBinary(Op, Inst, ZeroExt, Shift);
      return;
    }
  }

  // i128 has no IL arithmetic; every operator is a static method call.
  if (isWideInt(Ty) && isWideBinaryOpcode(Inst->getOpcode())) {
    printWideBinary(Inst->getOpcode(), Op0, Op1);
    return;
  }

  switch (Inst->getOpcode()) {
  case Instruction::PHI:
  case Instruction::Alloca:
    return; // PHIs materialize on edges; allocas run in the prologue.
  case Instruction::Ret:
    if (Value *RV = cast<ReturnInst>(Inst)->getReturnValue())
      printValueLoad(RV);
    out() << "\tret\n";
    return;
#if LLVM_VERSION_MAJOR >= 23
  case Instruction::UncondBr:
  case Instruction::CondBr:
#else
  case Instruction::Br:
#endif
    printBranchInstruction(Inst);
    return;
  case Instruction::Unreachable:
    out() << "\tldstr\t\"unreachable\"\n";
    out() << "\tnewobj\tinstance void "
           "[mscorlib]System.Exception::.ctor(string)\n\tthrow\n";
    return;
  case Instruction::Add:
  case Instruction::FAdd:
    printBinaryInstruction("add", Op0, Op1);
    return;
  case Instruction::Sub:
  case Instruction::FSub:
    printBinaryInstruction("sub", Op0, Op1);
    return;
  case Instruction::Mul:
  case Instruction::FMul:
    printBinaryInstruction("mul", Op0, Op1);
    return;
  case Instruction::UDiv:
    printBinaryInstruction("div.un", Op0, Op1);
    return;
  case Instruction::SDiv:
  case Instruction::FDiv: {
    // Same reason as SIToFP: narrow operands are stored zero-extended.
    bool Sgn = cast<Instruction>(Inst)->getOpcode() == Instruction::SDiv;
    if (Sgn) {
      printValueLoadAsInt(Op0, true);
      printValueLoadAsInt(Op1, true);
      out() << "\tdiv\n";
      return;
    }
    printBinaryInstruction("div", Op0, Op1);
    return;
  }
  case Instruction::URem:
    printBinaryInstruction("rem.un", Op0, Op1);
    return;
  case Instruction::SRem:
  case Instruction::FRem: {
    bool Sgn = cast<Instruction>(Inst)->getOpcode() == Instruction::SRem;
    if (Sgn) {
      printValueLoadAsInt(Op0, true);
      printValueLoadAsInt(Op1, true);
      out() << "\trem\n";
      return;
    }
    printBinaryInstruction("rem", Op0, Op1);
    return;
  }
  case Instruction::And:
    printBinaryInstruction("and", Op0, Op1);
    return;
  case Instruction::Or:
    printBinaryInstruction("or", Op0, Op1);
    return;
  case Instruction::Xor:
    printBinaryInstruction("xor", Op0, Op1);
    return;
  case Instruction::Shl:
    printValueLoad(Op0);
    printValueLoad(Op1);
    out() << "\tconv.i4\n\tshl\n";
    return;
  case Instruction::LShr:
    // A logical shift of the declared width, but the IL stack is int32 and
    // loading an i8/i16 argument sign-extends it. Zero-extend first, or
    // `lshr i8 0xf0, 2` becomes 0x3ffffffc >> ... -> 0xfc instead of 0x3c.
    printValueLoadAsInt(Op0, false);
    printValueLoad(Op1);
    out() << "\tconv.i4\n\tshr.un\n";
    return;
  case Instruction::AShr:
    printValueLoadAsInt(Op0, true);
    printValueLoad(Op1);
    out() << "\tconv.i4\n\tshr\n";
    return;
  case Instruction::ICmp:
    if (auto *RTy = dyn_cast<VectorType>(Ty))
      printVectorCmp(cast<ICmpInst>(Inst)->getPredicate(), Op0, Op1, RTy,
                     /*IsFP=*/false);
    else if (isWideInt(Op0->getType()))
      printWideICmp(cast<ICmpInst>(Inst)->getPredicate(), Op0, Op1);
    else
      printICmp(cast<ICmpInst>(Inst)->getPredicate(), Op0, Op1);
    return;
  case Instruction::FCmp:
    if (auto *RTy = dyn_cast<VectorType>(Ty))
      printVectorCmp(cast<FCmpInst>(Inst)->getPredicate(), Op0, Op1, RTy,
                     /*IsFP=*/true);
    else
      printFCmp(
          cast<FCmpInst>(Inst)->getPredicate(), [&] { printValueLoad(Op0); },
          [&] { printValueLoad(Op1); });
    return;
  case Instruction::Select: {
    if (auto *VTy = dyn_cast<VectorType>(Ty)) {
      // A vector predicate is a blob, not a boolean, so branch per lane.
      auto *CTy = cast<VectorType>(Inst->getOperand(0)->getType());
      Type *EltTy = VTy->getElementType();
      uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
      unsigned N = VTy->getElementCount().getFixedValue();
      std::string C = newTempLocal(CTy), T = newTempLocal(VTy),
                  R = newTempLocal(VTy);
      printValueLoad(Inst->getOperand(0));
      out() << "\tstloc\t'" << C << "'\n";
      printValueLoad(Inst->getOperand(1));
      out() << "\tstloc\t'" << T << "'\n";
      printValueLoad(Inst->getOperand(2));
      out() << "\tstloc\t'" << R << "'\n";
      std::string Sfx = getMemSuffix(EltTy);
      for (unsigned I = 0; I < N; ++I) {
        std::string L = "IL_sel" + utostr(getUniqID());
        printAggregateLaneAddr(C, CTy->getElementType(), I);
        out() << "\tldind.i1\n\tbrfalse\t" << L << "_e\n";
        printAggregateLaneAddr(R, EltTy, I * ElemSize);
        printAggregateLaneAddr(T, EltTy, I * ElemSize);
        out() << "\tldind." << Sfx << "\n\tstind." << Sfx << "\n" << L
              << "_e:\n";
      }
      out() << "\tldloc\t'" << R << "'\n";
      return;
    }
    std::string L = "IL_sel" + utostr(getUniqID());
    printValueLoad(Op0);
    out() << "\tbrtrue\t" << L << "_t\n";
    printValueLoad(Inst->getOperand(2));
    out() << "\tbr\t" << L << "_e\n";
    out() << L << "_t:\n";
    printValueLoad(Inst->getOperand(1));
    out() << L << "_e:\n";
    return;
  }
  case Instruction::Load: {
    const auto *LI = cast<LoadInst>(Inst);
    if (LI->isAtomic())
      printAtomicLoad(LI->getPointerOperand(), Ty);
    else
      printIndirectLoad(LI->getPointerOperand(), Ty);
    return;
  }
  case Instruction::Store: {
    const auto *SI = cast<StoreInst>(Inst);
    if (SI->isAtomic())
      printAtomicStore(SI->getPointerOperand(), SI->getValueOperand());
    else
      printIndirectSave(SI->getPointerOperand(), SI->getValueOperand());
    return;
  }
  case Instruction::GetElementPtr:
    if (auto *GEP = dyn_cast<GEPOperator>(Inst); GEP->getType()->isVectorTy())
      printVectorGep(GEP);
    else
      printGepInstruction(GEP);
    return;
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::PtrToAddr:
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
    printCastInstruction(Inst->getOpcode(), Op0,
                         cast<CastInst>(Inst)->getDestTy());
    return;
  case Instruction::Call:
    printCallInstruction(cast<CallBase>(Inst));
    return;
  case Instruction::Invoke:
    printInvoke(cast<InvokeInst>(Inst));
    return;
  case Instruction::LandingPad:
    printLandingPad(cast<LandingPadInst>(Inst));
    return;
  case Instruction::Resume:
    printResume(cast<ResumeInst>(Inst));
    return;
  case Instruction::VAArg: {
    // Front ends expand va_arg into plain IR for the triples this backend
    // targets (clang does on x86-64), so this is the fallback for any that
    // leaves the instruction in. The front end's own va_arg then reads the
    // tag or cursor directly, not this.
    //
    // A slot is one 8-byte CLR value whatever the C type, so read int64 and
    // let the local's type reinterpret it -- the same bits printVarArgBuffer
    // wrote. The cursor sits inside the caller's va_list, which has to be
    // advanced in place (the accessors do the same).
    std::string Tag = newTempLocal(Type::getInt64Ty(Mod->getContext()));
    std::string Cursor = newTempLocal(Type::getInt64Ty(Mod->getContext()));
    printValueLoad(Op0);
    out() << "\tstloc\t'" << Tag << "'\n";
    if (vaIsTag()) {
      out() << "\tldloc\t'" << Tag << "'\n\tldc.i8\t8\n\tadd\n";
    } else {
      out() << "\tldloc\t'" << Tag << "'\n";
    }
    out() << "\tldind.i\n\tstloc\t'" << Cursor << "'\n";
    if (vaIsTag())
      out() << "\tldloc\t'" << Tag << "'\n\tldc.i8\t8\n\tadd\n";
    else
      out() << "\tldloc\t'" << Tag << "'\n";
    out() << "\tldloc\t'" << Cursor << "'\n\tldc.i8\t8\n\tadd\n\tstind.i\n";
    out() << "\tldloc\t'" << Cursor << "'\n\tldind.i8\n";
    std::string Val = newTempLocal(Ty);
    out() << "\tstloc\t'" << Val << "'\n\tldloc\t'" << Val << "'\n";
    return;
  }
  case Instruction::ExtractElement: {
    // Vectors are byte blobs: take the address of the element and load it.
    // An i128 lane never reaches here: getTypeName rejects the vector type
    // itself, before any body is emitted.
    auto *VTy = cast<VectorType>(Inst->getOperand(0)->getType());
    uint64_t ElemSize = DL->getTypeAllocSize(VTy->getElementType())
                            .getFixedValue();
    std::string Tmp = newTempLocal(Inst->getOperand(0)->getType());
    printValueLoad(Inst->getOperand(0));
    out() << "\tstloc\t'" << Tmp << "'\n";
    out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
    printValueLoad(Inst->getOperand(1));
    out() << "\tconv.i8\n";
    printPtrLoad(ElemSize);
    out() << "\tconv.i8\n\tmul\n\tadd\n";
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
    return;
  }
  case Instruction::InsertElement: {
    auto *VTy = cast<VectorType>(Inst->getOperand(0)->getType());
    uint64_t ElemSize = DL->getTypeAllocSize(VTy->getElementType())
                            .getFixedValue();
    std::string Tmp = newTempLocal(VTy);
    printValueLoad(Inst->getOperand(0));
    out() << "\tstloc\t'" << Tmp << "'\n";
    out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
    printValueLoad(Inst->getOperand(2));
    out() << "\tconv.i8\n";
    printPtrLoad(ElemSize);
    out() << "\tconv.i8\n\tmul\n\tadd\n";
    printValueLoad(Inst->getOperand(1));
    out() << "\tstind." << getMemSuffix(Inst->getOperand(1)->getType())
        << "\n\tldloc\t'" << Tmp << "'\n";
    return;
  }
  case Instruction::ShuffleVector: {
    // Build the result lane by lane, reading each source lane through byte
    // pointers. Undef lanes are left zero.
    auto *SVI = cast<ShuffleVectorInst>(Inst);
    auto *SrcTy = cast<VectorType>(SVI->getOperand(0)->getType());
    auto *DstTy = cast<VectorType>(Ty);
    Type *EltTy = DstTy->getElementType();
    uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
    uint64_t NumSrc = SrcTy->getElementCount().getFixedValue();
    // The mask lives on the instruction, not among its operands.
    ArrayRef<int> Mask = SVI->getShuffleMask();
    std::string SrcA = newTempLocal(SrcTy);
    std::string SrcB = newTempLocal(SrcTy);
    std::string Dst = newTempLocal(DstTy);
    printValueLoad(Inst->getOperand(0));
    out() << "\tstloc\t'" << SrcA << "'\n";
    printValueLoad(Inst->getOperand(1));
    out() << "\tstloc\t'" << SrcB << "'\n";
    out() << "\tldloca\t'" << Dst << "'\n\tinitobj\t" << getTypeName(DstTy)
        << "\n";
    unsigned NumDst = DstTy->getElementCount().getFixedValue();
    for (unsigned I = 0; I < NumDst; ++I) {
      // Poison/undef lanes (-1) are left zero.
      int M = Mask[I];
      if (M < 0)
        continue;
      if ((uint64_t)M >= 2 * NumSrc)
        continue;
      const std::string &Src = M < NumSrc ? SrcA : SrcB;
      uint64_t Lane = (uint64_t)M < NumSrc ? M : M - NumSrc;
      // destination lane address
      out() << "\tldloca\t'" << Dst << "'\n\tconv.u8\n";
      printPtrLoad(I * ElemSize);
      out() << "\tconv.i8\n\tadd\n";
      // source lane value
      out() << "\tldloca\t'" << Src << "'\n\tconv.u8\n";
      printPtrLoad(Lane * ElemSize);
      out() << "\tconv.i8\n\tadd\n\tldind." << getMemSuffix(EltTy) << "\n";
      out() << "\tstind." << getMemSuffix(EltTy) << "\n";
    }
    out() << "\tldloc\t'" << Dst << "'\n";
    return;
  }
  case Instruction::FNeg:
    // 0 - x negates every IEEE value exactly, -0 and NaN included. (IL has no
    // neg instruction; negation is normally done as 0 - x anyway.)
    printConstLoad(ConstantFP::getZero(Ty));
    printValueLoad(Op0);
    out() << "\tsub\n";
    return;
  case Instruction::Freeze:
    printValueLoad(Op0);
    return;
  case Instruction::ExtractValue: {
    // Aggregates are byte blobs, so take the address of the field.
    const auto *EV = cast<ExtractValueInst>(Inst);
    SmallVector<unsigned> Idx(EV->idx_begin(), EV->idx_end());
    Type *AggTy = EV->getAggregateOperand()->getType();
    uint64_t Off = DL->getStructLayout(cast<StructType>(AggTy))
                       ->getElementOffset(Idx[0])
                       .getFixedValue();
    for (unsigned I = 1; I < Idx.size(); ++I) {
      AggTy = cast<StructType>(AggTy)->getElementType(Idx[I - 1]);
      Off += DL->getStructLayout(cast<StructType>(AggTy))
                 ->getElementOffset(Idx[I])
                 .getFixedValue();
    }
    std::string Tmp = newTempLocal(EV->getAggregateOperand()->getType());
    printValueLoad(EV->getAggregateOperand());
    out() << "\tstloc\t'" << Tmp << "'\n";
    printAggregateLaneAddr(Tmp, Ty, Off);
    if (movesAsBlob(Ty))
      out() << "\tldobj\t" << getTypeName(Ty) << '\n';
    else
      out() << "\tldind." << getMemSuffix(Ty) << '\n';
    return;
  }
  case Instruction::InsertValue: {
    const auto *IV = cast<InsertValueInst>(Inst);
    SmallVector<unsigned> Idx(IV->idx_begin(), IV->idx_end());
    Type *AggTy = IV->getAggregateOperand()->getType();
    uint64_t Off = DL->getStructLayout(cast<StructType>(AggTy))
                       ->getElementOffset(Idx[0])
                       .getFixedValue();
    for (unsigned I = 1; I < Idx.size(); ++I) {
      AggTy = cast<StructType>(AggTy)->getElementType(Idx[I - 1]);
      Off += DL->getStructLayout(cast<StructType>(AggTy))
                 ->getElementOffset(Idx[I])
                 .getFixedValue();
    }
    std::string Tmp = newTempLocal(IV->getAggregateOperand()->getType());
    printValueLoad(IV->getAggregateOperand());
    out() << "\tstloc\t'" << Tmp << "'\n";
    // The address is the field's, but what is stored is the inserted value,
    // whose type is the element's, not the aggregate's: `insertvalue {i64,i1}
    // ..., i64 %x, 0` stores an i64 through the address of field 0.
    Type *EltTy = IV->getInsertedValueOperand()->getType();
    printAggregateLaneAddr(Tmp, EltTy, Off);
    printValueLoad(IV->getInsertedValueOperand());
    if (movesAsBlob(EltTy))
      out() << "\tstobj\t" << getTypeName(EltTy) << '\n';
    else
      out() << "\tstind." << getMemSuffix(EltTy) << '\n';
    out() << "\tldloc\t'" << Tmp << "'\n";
    return;
  }
  case Instruction::Fence:
    // Every CLR atomic access is already a full barrier (volatile. is
    // release/acquire, Interlocked is seq_cst), so a standalone fence has
    // nothing left to order. The call is kept so the ordering is not merely
    // assumed: it is what the JIT emits for a seq_cst fence.
    out() << "\tcall\tvoid [mscorlib]System.Threading.Interlocked::"
           "MemoryBarrier()\n";
    return;
  case Instruction::AtomicRMW:
    printAtomicRMW(cast<AtomicRMWInst>(Inst));
    return;
  case Instruction::AtomicCmpXchg:
    printAtomicCmpXchg(cast<AtomicCmpXchgInst>(Inst));
    return;
  default:
    fatalError("unsupported instruction " +
                       Twine(Inst->getOpcodeName()));
  }
}
