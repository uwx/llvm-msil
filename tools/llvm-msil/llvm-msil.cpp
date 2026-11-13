//===-- llvm-msil.cpp - LLVM to MSIL (CIL) translator ---------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This tool translates an LLVM module into CIL text, which `ilasm` turns into
// a .NET assembly.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/LegacyPassManager.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include <memory>

using namespace llvm;

extern "C" void LLVMInitializeMSILTargetInfo();
extern "C" void LLVMInitializeMSILTarget();

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input>"),
                                          cl::init("-"));
static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"));

int main(int argc, char **argv) {
  InitLLVM X(argc, argv);
  LLVMInitializeMSILTargetInfo();
  LLVMInitializeMSILTarget();

  PassRegistry &Registry = *PassRegistry::getPassRegistry();
  initializeCore(Registry);
  initializeAnalysis(Registry);
  initializeCodeGen(Registry);
  initializeScalarOpts(Registry);
  initializeTransformUtils(Registry);
  initializeTarget(Registry);

  cl::ParseCommandLineOptions(argc, argv, "LLVM to MSIL translator\n");

  LLVMContext Context;
  SMDiagnostic Err;
  std::unique_ptr<Module> M = parseIRFile(InputFilename, Err, Context);
  if (!M) {
    Err.print(argv[0], errs());
    return 1;
  }

  // MSIL is selected by name rather than by triple.
  std::string Error;
  Triple TargetName("msil");
  const Target *TheTarget = TargetRegistry::lookupTarget(TargetName, Error);
  if (!TheTarget) {
    errs() << argv[0] << ": " << Error << "\n";
    return 1;
  }

  std::unique_ptr<TargetMachine> TM(TheTarget->createTargetMachine(
      Triple(M->getTargetTriple()), "", "", TargetOptions(), std::nullopt));
  if (!TM) {
    errs() << argv[0] << ": could not create the MSIL target machine\n";
    return 1;
  }

  // clang has already folded struct offsets and pointer widths into the IR
  // with its own layout, so only a module that has none gets the MSIL one.
  if (M->getDataLayoutStr().empty())
    M->setDataLayout(TM->createDataLayout());

  if (OutputFilename.empty()) {
    if (InputFilename == "-")
      OutputFilename = "-";
    else {
      OutputFilename = InputFilename;
      for (StringRef Ext : {".bc", ".ll"})
        if (StringRef(OutputFilename).ends_with(Ext))
          OutputFilename.resize(OutputFilename.size() - Ext.size());
      OutputFilename += ".il";
    }
  }

  std::error_code EC;
  ToolOutputFile Out(OutputFilename, EC, sys::fs::OF_Text);
  if (EC) {
    errs() << argv[0] << ": " << EC.message() << "\n";
    return 1;
  }

  legacy::PassManager PM;
  if (TM->addPassesToEmitFile(PM, Out.os(), nullptr,
                              CodeGenFileType::AssemblyFile)) {
    errs() << argv[0] << ": MSIL cannot emit this file type\n";
    return 1;
  }
  PM.run(*M);

  Out.keep();
  return 0;
}
