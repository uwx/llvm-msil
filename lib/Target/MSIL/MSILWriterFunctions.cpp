//===-- MSILWriterFunctions.cpp - Function and module emission -------------===//
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
// Functions
//===----------------------------------------------------------------------===//

void MSILWriter::printFunction(const Function &F) {
  FunctionType *FTy = F.getFunctionType();

  // The evaluation stack never holds more than one value per argument of the
  // widest call, plus the addresses a vector expansion materialises at once
  // (three, each needing a two-value address computation on top). Deriving this
  // from the IR keeps the bound tight: maxstack is reserved per frame, and a
  // method that recurses deeply blows the thread stack when it is inflated.
  unsigned MaxOperands = 8;
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB) {
      unsigned N = I.getNumOperands() + 1;
      if (const auto *CB = dyn_cast<CallBase>(&I))
        N = std::max<unsigned>(N, CB->arg_size() + 1);
      if (I.getType()->isVectorTy())
        N += 6;
      MaxOperands = std::max(MaxOperands, N);
    }
  MaxOperands += 8;

  // A vararg definition is emitted with a fixed signature: its declared
  // parameters, then the buffer of trailing arguments as a `void*`. Emitting
  // it as `vararg` would make CoreCLR on Linux reject every method that names
  // it, including its own callers.
  bool VarArgDef = isEmittedVarArgDef(&F);
  out() << "\n.method static " << (F.hasLocalLinkage() ? "private " : "public ")
      << "hidebysig ";
  out() << getTypeName(F.getReturnType()) << " '" << getValueName(&F) << "'(";
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
    if (I)
      out() << ", ";
    out() << getTypeName(FTy->getParamType(I)) << " '"
        << getValueName(F.getArg(I)) << "'";
  }
  if (VarArgDef) {
    if (FTy->getNumParams())
      out() << ", ";
    out() << "void* '$va_args'";
  }
  out() << ") cil managed\n{\n";

  // Emit the body into a buffer first: instruction emission can add
  // temporaries (vector bitcast/shuffle slots) that must be declared before
  // the body starts.
  raw_ostream *FileOut = Out;
  std::string BodyStr;
  raw_string_ostream Body(BodyStr);
  Out = &Body;

  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB)
      if (const auto *AI = dyn_cast<AllocaInst>(&I)) {
        uint64_t Size =
            DL->getTypeAllocSize(AI->getAllocatedType()).getFixedValue();
        // localloc takes a native int; a 32-bit size would make it read the
        // size from the wrong width on a 64-bit CLR.
        if (const auto *CI = dyn_cast<ConstantInt>(AI->getArraySize()))
          out() << "\tldc.i8\t" << CI->getZExtValue() * Size << "\n";
        else {
          printValueLoad(AI->getArraySize());
          out() << "\tconv.i8\n";
          out() << "\tldc.i8\t" << Size << "\n\tmul\n";
        }
        out() << "\tlocalloc\n\tstloc\t'" << getValueName(AI) << "'\n";
      }

  bool HasInvoke = any_of(F, [](const BasicBlock &BB) {
    return isa<InvokeInst>(BB.getTerminator());
  });
  if (HasInvoke)
    ExtraLocals.emplace_back("void*", ExceptionLocal);

  for (const BasicBlock &BB : F) {
    out() << getLabelName(&BB) << ":\n";
    for (const Instruction &I : BB) {
      out() << '\n' << asILComment(I) << '\n';
      printInstruction(&I);
      if (I.getType()->isVoidTy() || isa<PHINode>(&I) || isa<AllocaInst>(&I) ||
          I.isTerminator())
        continue;
      printValueSave(&I);
    }
  }
  Body.flush();
  Out = FileOut;

  // A call pushes one value per argument, fewer than everything counted below
  // (arguments + results), so this bound covers the whole body. The count must
  // stay near the true maximum: an inflated maxstack is reserved per frame and
  // deep recursion then overflows the thread stack.
  out() << "\t.maxstack\t" << MaxOperands << '\n';

  // One `.locals init` per local is valid IL but repeats the header thousands
  // of times in a large function (the portable libc's printf has over two
  // thousand locals). IL allows several declarations separated by commas, so
  // emit one directive.
  std::string Locals;
  auto AddLocal = [&](StringRef Type, StringRef Name) {
    if (!Locals.empty())
      Locals += ", ";
    Locals += Type.str() + " '" + Name.str() + "'";
  };
  for (const Argument &A : F.args())
    AddLocal(getLocalType(A.getType()), getValueName(&A));
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB)
      if (!I.getType()->isVoidTy())
        AddLocal(getLocalType(I.getType()), getValueName(&I));
  for (auto &Extra : ExtraLocals)
    AddLocal(Extra.first, Extra.second);
  if (!Locals.empty())
    out() << "\t.locals init (" << Locals << ")\n";

  out() << BodyStr;
  out() << "}\n";
}

void MSILWriter::printFunctions() {
  for (Function &Fn : *Mod) {
    if (Fn.isDeclaration())
      continue;
    LocalIDs.clear();
    LabelIDs.clear();
    ExtraLocals.clear();
    F = &Fn;
    NextLocalID = 0;
    NextLabelID = 0;

    for (const Argument &A : Fn.args())
      LocalIDs[&A] = NextLocalID++;
    for (const BasicBlock &BB : Fn) {
      LabelIDs[&BB] = NextLabelID++;
      for (const Instruction &I : BB)
        if (!I.getType()->isVoidTy())
          LocalIDs[&I] = NextLocalID++;
    }

    printFunction(Fn);
  }
  F = nullptr;
}
