//===-- MSILTargetInfo.cpp - MSIL Target Implementation -------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MSILTargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"
using namespace llvm;

Target &llvm::getTheMSILTarget() {
  static Target TheMSILTarget;
  return TheMSILTarget;
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeMSILTargetInfo() {
  // MSIL has no architecture of its own; it is selected by name (-march=msil)
  // rather than by target triple, so intentionally match no triple.
  RegisterTarget<> X(getTheMSILTarget(), "msil", "MSIL backend", "MSIL");
}

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeMSILTargetMC() {}
