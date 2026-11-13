//===-- MSILTargetMachine.cpp - TargetMachine for the MSIL ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MSILTargetMachine.h"
#include "MSILWriter.h"
#include "TargetInfo/MSILTargetInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/EHPersonalities.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/Local.h"

namespace llvm {

namespace {
// itanium landing pads are emitted as try regions by the writer, any other
// personality just loses its unwind edges
class MSILLowerForeignInvokes : public FunctionPass {
public:
  static char ID;
  MSILLowerForeignInvokes() : FunctionPass(ID) {}

  StringRef getPassName() const override {
    return "MSIL lower non-Itanium invokes";
  }

  bool runOnFunction(Function &F) override {
    if (!F.hasPersonalityFn())
      return false;
    EHPersonality Personality = classifyEHPersonality(F.getPersonalityFn());
    if (Personality == EHPersonality::GNU_CXX ||
        Personality == EHPersonality::GNU_C)
      return false;
    bool Changed = false;
    for (BasicBlock &BB : F)
      if (auto *II = dyn_cast<InvokeInst>(BB.getTerminator())) {
        changeToCall(II);
        Changed = true;
      }
    return Changed;
  }
};
char MSILLowerForeignInvokes::ID = 0;
} // namespace

// IL pointers are native ints of the host machine, so the module is compiled
// for the 64-bit CLR that ilasm and the runtime default to.
// shortcut: 64-bit CLR only; add a 32-bit layout if it is ever needed.
static const char *const MSILDataLayout =
    "e-m:e-p:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128";

MSILTargetMachine::MSILTargetMachine(const Target &T, const Triple &TT,
                                     StringRef CPU, StringRef FS,
                                     const TargetOptions &Options,
                                     std::optional<Reloc::Model> RM,
                                     std::optional<CodeModel::Model> CM,
                                     CodeGenOptLevel OL, bool JIT)
    : TargetMachine(T, MSILDataLayout, TT, CPU, FS, Options) {}

bool MSILTargetMachine::addPassesToEmitFile(
    PassManagerBase &PM, raw_pwrite_stream &Out, raw_pwrite_stream *DwoOut,
    CodeGenFileType FileType, bool DisableVerify,
    MachineModuleInfoWrapperPass *MMIWP) {
  if (FileType != CodeGenFileType::AssemblyFile)
    return true;

  // Prepare the IR for the constructs IL can express: no switch, no EH other
  // than Itanium landing pads, canonical CFG, atomics lowered to libcalls, and
  // SSA locals.
  //
  // The writer links the portable C library, and that happens after these
  // passes have run, so the library's own switches and allocas would reach it
  // unlowered. The writer therefore re-runs this subset on everything the
  // link added; the summary is in MSILWriter::lowerLinkedFunctions.
  PM.add(new MSILLowerForeignInvokes());
  PM.add(createUnreachableBlockEliminationPass());
  PM.add(createLowerSwitchPass());
  PM.add(createCFGSimplificationPass());
  // CFGSimplify can fold LowerSwitch's comparison tree back into a switch
  // (e.g. when several cases are contiguous), and the writer cannot emit a
  // switch at all. Lower again after the CFG cleanup; idempotent when no
  // switch remains.
  PM.add(createLowerSwitchPass());
  PM.add(createAtomicExpandLegacyPass());
  PM.add(createExpandReductionsPass());
  PM.add(createPromoteMemoryToRegisterPass());
  PM.add(new MSILWriter(Out));
  return false;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void LLVMInitializeMSILTarget() {
  RegisterTargetMachine<MSILTargetMachine> X(getTheMSILTarget());
}

} // namespace llvm
