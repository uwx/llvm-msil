//===-- MSILWriterGlobals.cpp - Type collection, globals and startup -------===//
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
// Module emission
//===----------------------------------------------------------------------===//

void MSILWriter::collectType(Type *Ty) {
  if (!Ty)
    return;
  if (isWideInt(Ty))
    NeedsSystemRuntime = true;
  if (auto *STy = dyn_cast<StructType>(Ty)) {
    if (AggregateNames.count(Ty))
      return;
    getAggregateName(Ty);
    for (Type *Elt : STy->elements())
      collectType(Elt);
    return;
  }
  if (auto *ATy = dyn_cast<ArrayType>(Ty)) {
    if (AggregateNames.count(Ty))
      return;
    getAggregateName(Ty);
    collectType(ATy->getElementType());
    return;
  }
  if (auto *VTy = dyn_cast<VectorType>(Ty)) {
    // IL has no SIMD instructions; model a vector as a byte blob.
    if (!AggregateNames.count(Ty))
      getAggregateName(Ty);
    collectType(VTy->getElementType());
  }
}

void MSILWriter::collectConstant(const Constant *C) {
  // A global can appear inside its own initializer (`getelementptr @g`), and a
  // global is an operand of its own initializer, so descending into one here
  // recurses forever. Every global's type is collected by collectTypes anyway.
  if (!C || isa<GlobalVariable>(C))
    return;
  collectType(C->getType());
  for (const Value *Op : C->operands())
    if (const auto *OC = dyn_cast<Constant>(Op))
      collectConstant(OC);
}

/// True when some value, local or signature in \p M is a register-wide vector
/// and therefore names Vector<T>. Asked before any body is emitted, since the
/// assembly reference has to precede every type reference.
bool MSILWriter::vectorNeeded(Module &M) {
  for (Function &F : M) {
    if (vectorTypeIn(F.getFunctionType()))
      return true;
    if (F.isDeclaration())
      continue;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        if (vectorTypeIn(I.getType()))
          return true;
        for (const Use &U : I.operands())
          if (vectorTypeIn(U->getType()))
            return true;
      }
  }
  for (GlobalVariable &G : M.globals())
    if (vectorTypeIn(G.getValueType()))
      return true;
  return false;
}

/// True when \p Ty is or contains a register-wide vector type.
bool MSILWriter::vectorTypeIn(Type *Ty) {
  if (!Ty)
    return false;
  if (isSimdVectorTy(Ty))
    return true;
  if (auto *FTy = dyn_cast<FunctionType>(Ty)) {
    if (vectorTypeIn(FTy->getReturnType()))
      return true;
    for (Type *P : FTy->params())
      if (vectorTypeIn(P))
        return true;
    return false;
  }
  if (auto *STy = dyn_cast<StructType>(Ty))
    for (Type *E : STy->elements())
      if (vectorTypeIn(E))
        return true;
  if (auto *ATy = dyn_cast<ArrayType>(Ty))
    return vectorTypeIn(ATy->getElementType());
  return false;
}

void MSILWriter::collectTypes() {
  for (Function &F : *Mod) {
    collectType(F.getReturnType());
    for (Argument &A : F.args())
      collectType(A.getType());
    if (F.isDeclaration())
      continue;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        collectType(I.getType());
        // Type-indexed operands (GEP source element, alloca) need classes too.
        if (auto *GEP = dyn_cast<GetElementPtrInst>(&I))
          collectType(GEP->getSourceElementType());
        if (auto *AI = dyn_cast<AllocaInst>(&I))
          collectType(AI->getAllocatedType());
        // A call to an emitted vararg definition materializes a buffer of
        // trailing arguments as a local (see printVarArgBuffer), and that
        // local's type needs a class like any other aggregate.
        if (auto *CB = dyn_cast<CallBase>(&I))
          if (isEmittedVarArgDef(CB->getCalledFunction()))
            getAggregateName(ArrayType::get(Type::getInt64Ty(Mod->getContext()),
                                            varArgSlots(CB)));
      }
  }
  for (GlobalVariable &G : Mod->globals()) {
    collectType(G.getValueType());
    // Globals get a template blob class, so even scalar globals need one.
    if (!G.isDeclaration()) {
      getAggregateName(G.getValueType());
      collectConstant(G.getInitializer());
    }
  }
  // Vector constants materialize into scratch locals, which need a class.
  for (Function &F : *Mod)
    for (BasicBlock &BB : F)
      for (Instruction &I : BB)
        for (Value *Op : I.operands())
          if (auto *CV = dyn_cast<Constant>(Op))
            if (CV->getType()->isVectorTy())
              getAggregateName(CV->getType());

  // llvm.global_ctors holds {priority, function, data} triples; the functions
  // are called from $MSIL_Startup, lowest priority first and in order of
  // appearance within one priority (libc++ sets up iostreams at 100).
  std::vector<std::pair<uint64_t, const Function *>> Ctors;
  if (const GlobalVariable *CtorsVar = Mod->getNamedGlobal("llvm.global_ctors"))
    if (const auto *Arr = dyn_cast_or_null<ConstantArray>(CtorsVar->getInitializer()))
      for (const Use &U : Arr->operands())
        if (const auto *CS = dyn_cast<ConstantStruct>(U.get()))
          if (const auto *Fn = dyn_cast<Function>(CS->getOperand(1)->stripPointerCasts()))
            if (!Fn->isDeclaration()) {
              uint64_t Priority =
                  cast<ConstantInt>(CS->getOperand(0))->getZExtValue();
              Ctors.emplace_back(Priority, Fn);
            }
  llvm::stable_sort(Ctors, [](const auto &A, const auto &B) {
    return A.first < B.first;
  });
  for (const auto &Ctor : Ctors)
    GlobalCtors.push_back(Ctor.second);
}

void MSILWriter::printAggregateDeclarations() {
  // getAggregateName may append to AggregateTypes for a nested type that was
  // not collected up front, so index rather than hold iterators.
  for (size_t I = 0; I < AggregateTypes.size(); ++I) {
    Type *Ty = AggregateTypes[I];
    std::string N = getAggregateName(Ty);
    uint64_t S = DL->getTypeAllocSize(Ty).getFixedValue();
    // Vectors are addressed one element per byte-sized slot, but the data
    // layout packs an i1 vector into ceil(N/8) bytes; size the class for the
    // lane addressing the emitter actually uses.
    // shortcut: an i1 vector nested inside a struct still follows the packed
    // layout, so it must not be mixed with scalar members that follow it.
    if (auto *VTy = dyn_cast<VectorType>(Ty))
      S = VTy->getElementCount().getFixedValue() *
          DL->getTypeAllocSize(VTy->getElementType()).getFixedValue();

    // A struct's members are declared at the offsets the LLVM data layout
    // chose, so a field can be reached with ldfld/stfld instead of pointer
    // arithmetic. Naming them keeps the layout in one place: the same numbers
    // drive both the declaration and the access.
    if (auto *STy = dyn_cast<StructType>(Ty)) {
      if (!STy->isOpaque()) {
        const StructLayout *SL = DL->getStructLayout(STy);
        out() << ".class value explicit ansi sealed '" << N << "'\n{\n"
              << "\t.pack 1\n\t.size " << S << "\n";
        for (unsigned F = 0; F < STy->getNumElements(); ++F)
          out() << "\t.field [" << SL->getElementOffset(F).getFixedValue()
                << "] public " << getTypeName(STy->getElementType(F)) << " 'f"
                << F << "'\n";
        out() << "}\n";
        continue;
      }
    }
    out() << ".class value explicit ansi sealed '" << N << "' { .pack 1 .size "
        << S << " }\n";
  }
  out() << '\n';
}

void MSILWriter::printStaticConstant(const Constant *C, uint64_t &Offset,
                                     std::string &Buffer) {
  Type *Ty = C->getType();
  auto Add = [&](const std::string &S, uint64_t Size) {
    if (!Buffer.empty())
      Buffer += ", ";
    Buffer += S;
    Offset += Size;
  };
  auto PadTo = [&](uint64_t Target) {
    if (Target > Offset) {
      if (!Buffer.empty())
        Buffer += ", ";
      Buffer += "int8 (0) [" + utostr(Target - Offset) + "]";
      Offset = Target;
    }
  };

  // Only contiguous null bytes mean "all zero"; a zeroed aggregate with
  // poisoned padding or bitcast elements must still be walked.
  bool ScalarZeroBytes = C->isNullValue() || isa<UndefValue>(C);
  if (isa<ConstantAggregateZero>(C) ||
      (ScalarZeroBytes && !Ty->isAggregateType())) {
    uint64_t Size = DL->getTypeAllocSize(Ty).getFixedValue();
    if (!Buffer.empty())
      Buffer += ", ";
    Buffer += "int8 (0) [" + utostr(Size) + "]";
    Offset += Size;
    return;
  }
  if (isLaneConstant(C)) {
    auto *VTy = cast<FixedVectorType>(Ty);
    uint64_t ElemSize =
        DL->getTypeAllocSize(VTy->getElementType()).getFixedValue();
    uint64_t Start = Offset;
    for (unsigned I = 0, E = VTy->getNumElements(); I != E; ++I) {
      PadTo(Start + I * ElemSize);
      printStaticConstant(C->getAggregateElement(I), Offset, Buffer);
    }
    PadTo(Start + VTy->getNumElements() * ElemSize);
    return;
  }
  if (const auto *CI = dyn_cast<ConstantInt>(C)) {
    // The .data element width must match the field layout: writing an i8
    // array as int32 would pad every character to four bytes.
    // ilasm range-checks .data elements against the signed width, so emit
    // sign-extended literals rather than the bit pattern.
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 128) {
      // The template is a raw byte blob, so emit the two halves as int64.
      const APInt &V = CI->getValue();
      Add("int64 (" + std::to_string((int64_t)V.getLoBits(64).getZExtValue())
          + ")",
          8);
      return Add(
          "int64 (" + std::to_string((int64_t)V.getHiBits(64).getZExtValue()) +
              ")",
          8);
    }
    std::string Val = std::to_string(CI->getSExtValue());
    if (N <= 8)
      return Add("int8 (" + Val + ")", 1);
    if (N <= 16)
      return Add("int16 (" + Val + ")", 2);
    if (N <= 32)
      return Add("int32 (" + Val + ")", 4);
    return Add("int64 (" + Val + ")", 8);
  }
  if (const auto *FP = dyn_cast<ConstantFP>(C)) {
    // -msil-fp80 stores the narrowed binary64 bits in the low half of the
    // member's 16-byte slot, then pads: the template has to be as long as the
    // class ilasm sizes it against, or the field and the blob disagree.
    uint64_t Start = Offset;
    uint64_t Bits = emittedFloat(FP->getValueAPF()).bitcastToAPInt().getZExtValue();
    Add(Ty->isFloatTy()
            ? "int32 (" + std::to_string((uint32_t)Bits) + ")"
            : "int64 (" + std::to_string(Bits) + ")",
        Ty->isFloatTy() ? 4 : 8);
    PadTo(Start + DL->getTypeAllocSize(Ty).getFixedValue());
    return;
  }
  if (isa<ConstantPointerNull>(C))
    return Add("int64 (0)", 8);
  if (isa<GlobalVariable>(C) || isa<Function>(C) || isa<ConstantExpr>(C)) {
    // A global's address is its writable heap copy, which $MSIL_Init allocates
    // after the templates are emitted, so these start as zero and are patched.
    DynInits.push_back({CurGV, Offset, C});
    return Add("int64 (0)", 8);
  }

  if (auto *STy = dyn_cast<StructType>(Ty)) {
    const StructLayout *SL = DL->getStructLayout(STy);
    uint64_t Start = Offset;
    for (unsigned I = 0, E = C->getNumOperands(); I != E; ++I) {
      PadTo(Start + SL->getElementOffset(I));
      printStaticConstant(cast<Constant>(C->getOperand(I)), Offset, Buffer);
    }
    PadTo(Start + DL->getTypeAllocSize(Ty).getFixedValue());
    return;
  }
  if (const auto *ATy = dyn_cast<ArrayType>(Ty)) {
    // ConstantDataSequential (string literals, packed arrays) has no operands;
    // walk it through getElementAsConstant instead.
    const auto *CDS = dyn_cast<ConstantDataSequential>(C);
    uint64_t EltSize =
        DL->getTypeAllocSize(ATy->getElementType()).getFixedValue();
    uint64_t Start = Offset;
    unsigned NumElts = CDS ? CDS->getNumElements() : C->getNumOperands();
    for (unsigned I = 0; I != NumElts; ++I) {
      PadTo(Start + I * EltSize);
      const Constant *Elt =
          CDS ? CDS->getElementAsConstant(I) : cast<Constant>(C->getOperand(I));
      printStaticConstant(Elt, Offset, Buffer);
    }
    PadTo(Start + DL->getTypeAllocSize(Ty).getFixedValue());
    return;
  }

  fatalError("unsupported global initializer: " + Twine(str(C)));
}

void MSILWriter::printGlobals() {
  // A `.data`-backed static field lives in a read-only segment, so any store
  // through it faults. Emit the initializer as a read-only template blob and
  // give the variable a writable heap pointer that $MSIL_Init fills in.
  for (GlobalVariable &G : Mod->globals()) {
    if (G.isDeclaration())
      continue;
    std::string Buffer;
    uint64_t Offset = 0;
    size_t FirstDynInit = DynInits.size();
    CurGV = &G;
    printStaticConstant(G.getInitializer(), Offset, Buffer);
    CurGV = nullptr;

    std::string Name = G.getName().str();
    std::string Cls = getAggregateName(G.getValueType());
    // ilasm rejects both an empty and a zero-count `.data`, and a zero-sized
    // aggregate has nothing to copy anyway: just give it a pointer.
    bool Empty = DL->getTypeAllocSize(G.getValueType()) == 0;
    // A constant global is never stored through, so its read-only image can be
    // its storage and $MSIL_Init only has to point the address at it. A
    // pointer-valued initializer is the exception: relocations are patched
    // into the writable copy at startup, which the image cannot take.
    bool Writable = !G.isConstant() || Empty || DynInits.size() != FirstDynInit;
    if (!Empty) {
      // $MSIL_Init runs from inside the emitted class and reads this blob, so
      // it has to be reachable from there.
      out() << ".field static " << HelperVis() << " valuetype '" << Cls << "' '"
          << Name << "$tpl' at '" << Name << "$tpl$data'\n";
      out() << ".data '" << Name << "$tpl$data' = { " << Buffer << " }\n";
    }
    out() << ".field static "
        << (G.hasLocalLinkage() ? std::string(HelperVis()) + " " : "public ")
        << getTypeName(G.getType()) << " '" << Name << "'\n";
    // A thread-local's pointer slot is per thread, so $MSIL_Init cannot fill
    // it once for the whole program: each thread's first access does instead
    // (printThreadLocalAccessor). A module-level field takes the attribute;
    // this was verified against the runtime, which gives each thread its own
    // slot and its own default value.
    if (G.isThreadLocal())
      out() << ".custom instance void [mscorlib]System.ThreadStaticAttribute"
               "::.ctor() = ( 01 00 00 00 )\n";
    out() << '\n';
    // Zero-sized globals still get an entry so $MSIL_Init gives each its own
    // address; there is just no template to copy from.
    Templates.push_back({&G, Empty ? "" : Name + "$tpl", Writable,
                         G.isThreadLocal()});
  }
}

void MSILWriter::printThreadLocalAccessor(const GlobalImage &Image) {
  const GlobalVariable &G = *Image.GV;
  uint64_t Size = DL->getTypeAllocSize(G.getValueType()).getFixedValue();
  std::string Name = G.getName().str();
  std::string Tpl = "valuetype '" + getAggregateName(G.getValueType()) + "' '" +
                    Image.Template + "'";
  // The declaration cannot name the class it is inside; call sites do.
  out() << "\n.method static " << HelperVis() << " "
        << getTypeName(G.getType()) << " '" << Name
        << "$tls'() cil managed\n{\n";
  out() << "\t.maxstack\t8\n";
  // The constant case pins the slot at the shared image and needs no scratch.
  if (Image.Writable || !Size)
    out() << "\t.locals init (" << getTypeName(G.getType()) << " 'p')\n";
  // A [ThreadStatic] slot is only ever read or written by the thread that owns
  // it, so two threads cannot race on one slot and this thread's first touches
  // are sequential: no lock is needed.
  out() << "\tldsfld\t" << getTypeName(G.getType()) << " '" << Name << "'\n";
  out() << "\tbrfalse\tIL_tls_init\n";
  out() << "\tldsfld\t" << getTypeName(G.getType()) << " '" << Name
        << "'\n\tret\n";
  out() << "IL_tls_init:\n";
  if (!Image.Writable && Size) {
    // A constant global cannot be stored through on any thread, so every
    // thread's slot points at the one read-only image. Comparing the address
    // of the same variable across threads then still compares equal.
    printImageAddress(G, Image.Template);
    if (pointersAsInt64())
      out() << "\tconv.u8\n";
    out() << "\tstsfld\t" << getTypeName(G.getType()) << " '" << Name << "'\n";
  } else {
    // The same bytes $MSIL_Init would have produced, but for this thread.
    printGlobalAlloc(G);
    if (pointersAsInt64())
      out() << "\tconv.u8\n";
    out() << "\tstloc\t'p'\n";
    if (Size) {
      // The zero-sized case has no bytes to copy, so the destination push
      // belongs inside this branch or the stack is left one item deep.
      out() << "\tldloc\t'p'\n";
      out() << "\tldsflda\t" << Tpl << "\n\tconv.u8\n";
      out() << "\tldc.i4\t" << Size << "\n\tcpblk\n";
    }
    // Pointer-valued initializer fields are patched into each thread's copy
    // here rather than once at startup, so a thread-local's initializer may
    // name another thread-local and each thread gets its own address. The slot
    // is published first: a global whose initializer names itself then finds
    // its storage already there instead of recurring forever.
    out() << "\tldloc\t'p'\n\tstsfld\t" << getTypeName(G.getType()) << " '"
          << Name << "'\n";
    for (const DynInit &DI : DynInits) {
      if (DI.GV != &G)
        continue;
      out() << "\tldloc\t'p'\n";
      if (DI.Offset) {
        printPtrLoad(DI.Offset);
        out() << "\tconv.i8\n\tadd\n";
      }
      out() << "\tconv.u8\n";
      if (isa<Function>(DI.Value)) {
        out() << "\tldftn\t"
            << getCallSignature(cast<Function>(DI.Value)->getFunctionType(),
                                nullptr,
                                getFunctionRef(cast<Function>(DI.Value)), false,
                                isEmittedVarArgDef(cast<Function>(DI.Value)))
            << "\n";
      } else {
        printValueLoad(DI.Value);
      }
      out() << "\tstind.i\n";
    }
  }
  out() << "\tldsfld\t" << getTypeName(G.getType()) << " '" << Name
        << "'\n\tret\n}\n";
}

void MSILWriter::printAllocHelper() {
  out() << "\n.method static " << HelperVis()
        << " void* '$MSIL_Alloc'(int64 size) cil managed\n{\n";
  out() << "\t.maxstack\t2\n";
  out() << "\tldarg.0\n\tconv.i\n";
  out() << "\tcall\tnative int [mscorlib]System.Runtime.InteropServices.Marshal::"
         "AllocHGlobal(native int)\n";
  out() << "\tret\n}\n";

  // malloc already gives 16, so only a stricter global pays for the slack
  out() << "\n.method static " << HelperVis()
        << " void* '$MSIL_AllocAligned'(int64 size, int64 align) cil managed\n"
           "{\n";
  out() << "\t.maxstack\t3\n";
  out() << "\tldarg.1\n\tldc.i8\t" << MallocAlign << "\n\tbgt.un\tIL_slack\n";
  out() << "\tldarg.0\n";
  out() << "\tcall\tvoid* " << ClassPrefix() << "'$MSIL_Alloc'(int64)\n";
  out() << "\tret\n";
  out() << "IL_slack:\n";
  out() << "\tldarg.0\n\tldarg.1\n\tadd\n\tldc.i8\t1\n\tsub\n";
  out() << "\tcall\tvoid* " << ClassPrefix() << "'$MSIL_Alloc'(int64)\n";
  out() << "\tconv.u8\n\tldarg.1\n\tadd\n\tldc.i8\t1\n\tsub\n";
  out() << "\tldarg.1\n\tneg\n\tand\n";
  out() << "\tret\n}\n";

  // ilasm packs .data blobs back to back, so a constant's image is only
  // usable in place when it happens to land aligned
  out() << "\n.method static " << HelperVis()
        << " void* '$MSIL_PlaceImage'(void* image, int64 size, int64 align) "
           "cil managed\n{\n";
  out() << "\t.maxstack\t3\n";
  out() << "\t.locals init (void* 'p')\n";
  out() << "\tldarg.0\n\tconv.u8\n\tldarg.2\n\tldc.i8\t1\n\tsub\n\tand\n";
  out() << "\tbrtrue\tIL_copy\n";
  out() << "\tldarg.0\n\tret\n";
  out() << "IL_copy:\n";
  out() << "\tldarg.1\n\tldarg.2\n";
  out() << "\tcall\tvoid* " << ClassPrefix()
        << "'$MSIL_AllocAligned'(int64, int64)\n";
  out() << "\tstloc\t'p'\n";
  out() << "\tldloc\t'p'\n\tldarg.0\n\tldarg.1\n\tconv.u4\n\tcpblk\n";
  out() << "\tldloc\t'p'\n\tret\n}\n";
}

uint64_t MSILWriter::getGlobalAlign(const GlobalVariable &G) const {
  Align Alignment =
      G.getAlign().value_or(DL->getABITypeAlign(G.getValueType()));
  return Alignment.value();
}

void MSILWriter::printImageAddress(const GlobalVariable &G,
                                   StringRef Template) {
  uint64_t Size = DL->getTypeAllocSize(G.getValueType()).getFixedValue();
  uint64_t Alignment = getGlobalAlign(G);
  out() << "\tldsflda\tvaluetype '" << getAggregateName(G.getValueType())
        << "' '" << Template << "'\n";
  out() << "\tconv.u8\n";
  if (Alignment <= 1)
    return;
  out() << "\tldc.i8\t" << Size << "\n\tldc.i8\t" << Alignment << "\n";
  out() << "\tcall\tvoid* " << ClassPrefix()
        << "'$MSIL_PlaceImage'(void*, int64, int64)\n";
}

void MSILWriter::printGlobalAlloc(const GlobalVariable &G) {
  uint64_t Size = DL->getTypeAllocSize(G.getValueType()).getFixedValue();
  uint64_t Alignment = getGlobalAlign(G);
  out() << "\tldc.i8\t" << (Size ? Size : 1) << "\n";
  if (Alignment <= MallocAlign) {
    out() << "\tcall\tvoid* " << ClassPrefix() << "'$MSIL_Alloc'(int64)\n";
    return;
  }
  out() << "\tldc.i8\t" << Alignment << "\n";
  out() << "\tcall\tvoid* " << ClassPrefix()
        << "'$MSIL_AllocAligned'(int64, int64)\n";
}

void MSILWriter::printInitializer() {
  if (!needsInitializer())
    return;
  out() << "\n.method static " << HelperVis()
        << " void '$MSIL_Init'() cil managed\n{\n";
  out() << "\t.maxstack\t8\n";
  printCxxRuntimeSymbols();
  for (auto &T : Templates) {
    // A thread-local's slot belongs to each thread, so its accessor fills it
    // on first use rather than this one-time startup pass.
    if (T.ThreadLocal)
      continue;
    const GlobalVariable &G = *T.GV;
    uint64_t Size = DL->getTypeAllocSize(G.getValueType()).getFixedValue();
    out() << "\t// " << G.getName() << "\n";
    if (!T.Writable && Size) {
      // A constant global is never stored through, so the image is the
      // storage. `ldsflda` still gives each global its own address, so
      // address comparisons stay distinct.
      printImageAddress(G, T.Template);
      if (pointersAsInt64())
        out() << "\tconv.u8\n";
      out() << "\tstsfld\t" << getTypeName(G.getType()) << " '"
            << G.getName() << "'\n";
      continue;
    }
    // Allocate the real, writable storage. A zero-sized global still needs a
    // distinct address: LLVM gives every global its own, so comparing two of
    // them must not compare equal.
    printGlobalAlloc(G);
    if (pointersAsInt64())
      out() << "\tconv.u8\n";
    out() << "\tstsfld\t" << getTypeName(G.getType()) << " '" << G.getName()
          << "'\n";
    if (!Size)
      continue;
    // Copy the template bytes in.
    out() << "\tldsfld\t" << getTypeName(G.getType()) << " '" << G.getName()
          << "'\n";
    out() << "\tldsflda\tvaluetype '" << getAggregateName(G.getValueType())
        << "' '" << T.Template << "'\n";
    out() << "\tconv.u8\n";
    out() << "\tldc.i4\t" << Size << "\n";
    out() << "\tcpblk\n";
  }
  // Pointer fields the assembly's relocations could not express.
  for (const DynInit &DI : DynInits) {
    // A thread-local's own pointer fields are patched by its accessor, per
    // thread; its slot is still null here.
    if (DI.GV->isThreadLocal())
      continue;
    out() << "\tldsfld\t" << getTypeName(DI.GV->getType()) << " '"
          << DI.GV->getName() << "'\n";
    if (DI.Offset) {
      printPtrLoad(DI.Offset);
      out() << "\tconv.i8\n\tadd\n";
    }
    out() << "\tconv.u8\n";
    if (isa<Function>(DI.Value)) {
      out() << "\tldftn\t"
          << getCallSignature(cast<Function>(DI.Value)->getFunctionType(),
                              nullptr,
                              getFunctionRef(cast<Function>(DI.Value)), false,
                              isEmittedVarArgDef(cast<Function>(DI.Value)))
          << "\n";
    } else {
      printValueLoad(DI.Value);
    }
    out() << "\tstind.i\n";
  }
  printAbiVTableRegistrations();
  out() << "\tret\n}\n";

  for (const GlobalImage &T : Templates)
    if (T.ThreadLocal)
      printThreadLocalAccessor(T);
}

// A library has no .entrypoint, so nothing runs the module's one-time setup.
// MSILInitialize is that entry point, with a bool guard so a second call is a
// no-op. The guard is not thread-safe by itself; a managed wrapper's type
// initializer is expected to provide that guarantee.
void MSILWriter::printLibraryInitializer() {
  std::string C = ClassPrefix();
  out() << "\n.field private static bool '$MSIL_Initialized'\n";
  out() << "\n.method public static void 'MSILInitialize'() cil managed\n{\n";
  out() << "\t.maxstack\t8\n";
  out() << "\tldsfld\tbool " << C << "'$MSIL_Initialized'\n";
  out() << "\tbrtrue\tIL_msil_init_done\n";
  out() << "\tldc.i4.1\n";
  out() << "\tstsfld\tbool " << C << "'$MSIL_Initialized'\n";
  if (needsInitializer())
    out() << "\tcall\tvoid " << C << "'$MSIL_Init'()\n";
  for (const Function *Ctor : GlobalCtors)
    out() << "\tcall\t" << getTypeName(Ctor->getReturnType()) << " "
          << getFunctionRef(Ctor) << "()\n";
  out() << "IL_msil_init_done:\n\tret\n}\n";
}

void MSILWriter::printModuleStartup() {
  // A library has no entry point; MSILInitialize() is its one-time entry.
  if (LibraryMode)
    return;
  const Function *Main = Mod->getFunction("main");
  out() << "\n.method static public int32 '$MSIL_Startup'() cil managed\n{\n";
  out() << "\t.entrypoint\n\t.maxstack\t8\n";

  // IL puts .entrypoint in <Module>, so this method stays outside the class
  // and every helper it reaches has to be named through the class.
  std::string C = ExternClassName.empty() ? "" : "'" + ExternClassName + "'::";

  if (needsInitializer())
    out() << "\tcall\tvoid " << C << "'$MSIL_Init'()\n";

  // __attribute__((constructor)) functions, in priority order, before main.
  // An ELF clang emits these as llvm.global_ctors; MSVC has no such attribute,
  // so a Windows build never produces the list. The entries are {priority,
  // function, data}; a null function is the terminator clang pads with.
  for (const Function *Ctor : GlobalCtors)
    out() << "\tcall\t" << getTypeName(Ctor->getReturnType()) << " "
          << getFunctionRef(Ctor) << "()\n";

  if (!Main || Main->isDeclaration()) {
    out() << "\tldc.i4.0\n\tret\n}\n";
    return;
  }
  // Build argv on the unmanaged heap so the program's `char **` works with
  // the plain pointer arithmetic the backend emits.
  // shortcut: argument strings are leaked; a real main() usually exits soon.
  bool WantArgs = Main->arg_size() >= 1;
  if (WantArgs) {
    out() << "\tcall\tvoid " << C << "'$MSIL_MakeArgs'()\n";
    out() << "\tldsfld\t" << (pointersAsInt64() ? "int64" : "void*") << " " << C
          << "'$MSIL::argc'\n\tldind.i4\n";
    if (Main->arg_size() >= 2)
      out() << "\tldsfld\t" << (pointersAsInt64() ? "int64" : "void*") << " "
            << C << "'$MSIL::argv'\n";
  }

  out() << "\tcall\t" << getTypeName(Main->getReturnType()) << " "
        << getFunctionRef(Main) << "(";
  for (unsigned I = 0, E = Main->arg_size(); I != E; ++I) {
    if (I)
      out() << ", ";
    out() << getTypeName(Main->getArg(I)->getType());
  }
  out() << ")\n";
  // Exit-time destructors run after main returns and before the process exits.
  // The value main returned is still on the stack below; a void call leaves it
  // where it is, so the order here is "call main, call finalize, return".
  if (EmitCxxRt)
    out() << "\tcall\tvoid __msil_cxx_finalize()\n";
  if (Main->getReturnType()->isVoidTy())
    out() << "\tldc.i4.0\n";
  else
    out() << "\tconv.i4\n";
  out() << "\tret\n}\n";
}

void MSILWriter::printArgHelper() {
  // Allocate argc/argv and fill them from Environment.GetCommandLineArgs().
  const char *PtrTy = pointersAsInt64() ? "int64" : "void*";
  out() << "\n.field static " << HelperVis() << " " << PtrTy << " '$MSIL::argc'\n";
  out() << ".field static " << HelperVis() << " " << PtrTy << " '$MSIL::argv'\n";
  out() << "\n.method static " << HelperVis()
        << " void '$MSIL_MakeArgs'() cil managed\n{\n";
  out() << "\t.maxstack\t4\n\t.locals init (int32 'n')\n"
           "\t.locals init (void* 'p')\n"
           "\t.locals init (string[] 'a')\n"
           "\t.locals init (int32 'i')\n";
  // argv = GetCommandLineArgs()
  out() << "\tcall\tstring[] [mscorlib]System.Environment::"
           "GetCommandLineArgs()\n\tstloc\t'a'\n";
  out() << "\tldloc\t'a'\n\tldlen\n\tconv.i4\n\tstloc\t'n'\n";
  // argc storage
  std::string CP = ClassPrefix();
  out() << "\tldc.i4.4\n\tcall\tvoid* " << CP << "'$MSIL_Alloc'(int64)\n";
  if (pointersAsInt64())
    out() << "\tconv.u8\n";
  out() << "\tstsfld\t" << PtrTy << " " << CP << "'$MSIL::argc'\n";
  out() << "\tldsfld\t" << PtrTy << " " << CP
        << "'$MSIL::argc'\n\tldloc\t'n'\n\tstind.i4\n";
  // argv = (char**)alloca(n * 8)
  out() << "\tldloc\t'n'\n\tconv.i8\n\tldc.i4.8\n\tconv.i8\n\tmul\n\tcall\tvoid* "
        << CP << "'$MSIL_Alloc'(int64)\n";
  if (pointersAsInt64())
    out() << "\tconv.u8\n";
  out() << "\tstsfld\t" << PtrTy << " " << CP << "'$MSIL::argv'\n";
  // for i in 0..n: argv[i] = StringToHGlobalAnsi(a[i])
  out() << "\tldc.i4.0\n\tstloc\t'i'\n";
  out() << "IL_args_loop:\n";
  out() << "\tldloc\t'i'\n\tldloc\t'n'\n\tbge\tIL_args_done\n";
  out() << "\tldsfld\t" << PtrTy << " " << CP
        << "'$MSIL::argv'\n\tldloc\t'i'\n\tconv.i8\n"
           "\tldc.i4.8\n\tconv.i8\n\tmul\n\tadd\n";
  out() << "\tldloc\t'a'\n\tldloc\t'i'\n\tldelem.ref\n";
  out() << "\tcall\tnative int [mscorlib]System.Runtime.InteropServices."
           "Marshal::StringToHGlobalAnsi(string)\n";
  out() << "\tstind.i\n";
  out() << "\tldloc\t'i'\n\tldc.i4.1\n\tadd\n\tstloc\t'i'\n"
           "\tbr\tIL_args_loop\n";
  out() << "IL_args_done:\n\tret\n}\n";
}
