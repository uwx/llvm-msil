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
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetLoweringObjectFile.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils.h"

namespace llvm {

// IL pointers are native ints of the host machine, so the module is compiled
// for the 64-bit CLR that ilasm and the runtime default to.
// shortcut: 64-bit CLR only; add a 32-bit layout if it is ever needed.
static const char *const MSILDataLayout =
    "e-m:e-i64:64-f80:128-n8:16:32:64-S128-p:64:64:64:32";

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

  // Prepare the IR for the constructs IL can express: no invoke/switch/EH,
  // canonical CFG, atomics lowered to libcalls, and SSA locals.
  PM.add(createLowerInvokePass());
  PM.add(createUnreachableBlockEliminationPass());
  PM.add(createLowerSwitchPass());
  PM.add(createCFGSimplificationPass());
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
