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
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"
#include "MSILTargetMachine.h"
#include <memory>

using namespace llvm;

extern "C" void LLVMInitializeMSILTargetInfo();
extern "C" void LLVMInitializeMSILTarget();

/// Print a fatal diagnostic as one line and exit, with no crash banner, stack
/// dump or dialog.
///
/// The emitter rejects a construct it cannot express through
/// report_fatal_error, whose default path also runs the interrupt handlers.
/// On Windows those put up a modal dialog, so a batch run stops until someone
/// clicks it, and on both hosts they print a backtrace of the compiler rather
/// than of the program being compiled. None of that helps the person reading
/// the message: the rejection names the symbol or type that caused it.
static void reportAsDiagnostic(void *, const char *Reason, bool) {
  errs() << "llvm-msil: error: " << Reason << "\n";
  exit(1);
}

static cl::opt<std::string> InputFilename(cl::Positional, cl::desc("<input>"),
                                          cl::init("-"));
static cl::opt<std::string> OutputFilename("o", cl::desc("Output filename"),
                                           cl::value_desc("filename"));

/// Write the two files `dotnet prog.exe` needs beside the emitted IL.
///
/// Neither is something the IL can express: the CLR looks for them next to the
/// assembly it is given, by name. Writing them here is what turns
/// `llvm-msil prog.ll` into a runnable program instead of a file plus a recipe
/// the user has to follow from the README.
///
///  * `prog.runtimeconfig.json` names the framework the assembly binds to.
///    Without it `dotnet` reports that hostpolicy.dll is missing, and on
///    Windows a bare `prog.exe` silently binds .NET Framework's mscorlib
///    instead -- which has neither Int128 nor Vector<T>.
///  * `MSIL.Runtime.dll` is the C++ exception runtime, needed only when the
///    module references it. It is found next to this executable (the build
///    copies it there) or through MSIL_RUNTIME_DIR.
static void writeDeploymentFiles(StringRef ILPath, bool NeedsRuntime) {
  if (ILPath == "-")
    return;
  SmallString<256> Stem(ILPath);
  if (StringRef(Stem).ends_with(".il"))
    Stem.resize(Stem.size() - 3);

  SmallString<256> Config(Stem);
  Config += ".runtimeconfig.json";
  std::error_code EC;
  raw_fd_ostream Cfg(Config, EC, sys::fs::OF_Text);
  if (EC) {
    errs() << "llvm-msil: warning: cannot write " << Config << ": "
           << EC.message() << "\n";
  } else {
    // The version is a floor, not a pin: `dotnet` rolls forward to whatever
    // compatible runtime is installed, and 9.0 is the first with Int128.
    Cfg << "{\n"
           "  \"runtimeOptions\": {\n"
           "    \"tfm\": \"net9.0\",\n"
           "    \"framework\": {\n"
           "      \"name\": \"Microsoft.NETCore.App\",\n"
           "      \"version\": \"9.0.0\"\n"
           "    }\n"
           "  }\n"
           "}\n";
  }

  if (!NeedsRuntime)
    return;

  // The runtime is a build artifact of this project, not of the module, so it
  // is located rather than embedded: beside the tool (CMake puts it there for
  // a standalone build), then in MSIL_RUNTIME_DIR.
  SmallString<256> Exe = sys::fs::getMainExecutable(
      "", reinterpret_cast<void *>(&writeDeploymentFiles));
  sys::path::remove_filename(Exe);
  SmallVector<SmallString<256>, 3> Dirs;
  Dirs.push_back(Exe);
  if (const char *Env = getenv("MSIL_RUNTIME_DIR"))
    Dirs.emplace_back(Env);
  for (const auto &Dir : Dirs) {
    SmallString<256> Src(Dir);
    sys::path::append(Src, "MSIL.Runtime.dll");
    if (!sys::fs::exists(Src))
      continue;
    SmallString<256> Dst(Stem);
    Dst += ".dll";
    sys::path::remove_filename(Dst);
    sys::path::append(Dst, "MSIL.Runtime.dll");
    if (sys::fs::equivalent(Src, Dst))
      return;
    if (std::error_code CopyEC = sys::fs::copy_file(Src, Dst)) {
      errs() << "llvm-msil: warning: cannot copy " << Src << " to " << Dst
             << ": " << CopyEC.message() << "\n";
    }
    return;
  }
  errs() << "llvm-msil: warning: this module uses MSIL.Runtime, but "
            "MSIL.Runtime.dll was not found beside llvm-msil or in "
            "MSIL_RUNTIME_DIR; the assembled program will not start\n";
}

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
  install_fatal_error_handler(reportAsDiagnostic);

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
  // The IL is written; now put beside it the two files the CLR looks for by
  // name, so the output is a program rather than a recipe.
  writeDeploymentFiles(OutputFilename,
                       static_cast<MSILTargetMachine *>(TM.get())
                           ->emittedRuntimeReference());
  return 0;
}
