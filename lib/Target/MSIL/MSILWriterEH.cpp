//===-- MSILWriterEH.cpp - C++ exception handling (Itanium landing pads) ---===//
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
// C++ exceptions
//===----------------------------------------------------------------------===//

// has to match the public methods of MSIL.Runtime.CxxAbi
//
// These are the functions that only the *exception* machinery calls. They are
// the ones whose presence means the C++ runtime is in use, so they are also
// what `usesCxxRuntime` tests for.
bool MSILWriter::isCxxRuntimeFunction(StringRef Name) {
  static const StringLiteral Names[] = {
      "__cxa_allocate_exception",
      "__cxa_free_exception",
      "__cxa_allocate_dependent_exception",
      "__cxa_free_dependent_exception",
      "__cxa_init_primary_exception",
      "__cxa_throw",
      "__cxa_begin_catch",
      "__cxa_end_catch",
      "__cxa_rethrow",
      "__cxa_get_exception_ptr",
      "__cxa_current_exception_type",
      "__cxa_increment_exception_refcount",
      "__cxa_decrement_exception_refcount",
      "__cxa_current_primary_exception",
      "__cxa_rethrow_primary_exception",
      "__cxa_uncaught_exception",
      "__cxa_uncaught_exceptions",
      "__cxa_get_globals",
      "__cxa_get_globals_fast",
      "_ZN10__cxxabiv119__getExceptionClassEPK17_Unwind_Exception",
      "_ZN10__cxxabiv121__isOurExceptionClassEPK17_Unwind_Exception",
  };
  return is_contained(Names, Name);
}

// Allocation operators and the exit-time hooks. CxxAbi defines them so an
// exception-using program needs no separate C++ runtime, but a program that
// never throws must not be dragged onto MSIL.Runtime by `new` alone: that
// would make the DLL mandatory for any C++ program and change the output of
// every module that allocates. They bind to CxxAbi only when
// `usesCxxRuntime()` is already true for another reason.
bool MSILWriter::isCxxSupportFunction(StringRef Name) {
  static const StringLiteral Names[] = {
      "__cxa_atexit",
      "__cxa_thread_atexit_impl",
      "__cxa_pure_virtual",
      "_ZSt9terminatev",
      // operator new/delete. The Itanium-manglable ones come in two spellings
      // because `size_t` is `unsigned long` on ELF and `unsigned long long`
      // on mingw, so the same operator is `_Znwm` on one and `_Znwy` on the
      // other; both have to reach the same CxxAbi member.
      "_Znwm", "_Znam", "_ZdlPv", "_ZdaPv", "_ZdlPvm", "_ZdaPvm",
      "_Znwy", "_Znay", "_ZdlPvy", "_ZdaPvy",
  };
  return is_contained(Names, Name);
}

// The CxxAbi member a C symbol binds to. Usually the symbol's own name; the
// mingw allocation operators share the ELF member because the parameter is an
// i64 either way and CxxAbi spells each one once.
StringRef MSILWriter::getCxxRuntimeMember(StringRef Name) {
  if (Name == "_Znwy" || Name == "_Znay")
    return Name == "_Znwy" ? "_Znwm" : "_Znam";
  if (Name == "_ZdlPvy")
    return "_ZdlPvm";
  if (Name == "_ZdaPvy")
    return "_ZdaPvm";
  return Name;
}

// glibc keeps the exit ones in libc_nonshared.a, so they can't be pinvoked,
// and the pthread ones take managed callbacks that glibc would otherwise call
// on threads the runtime doesn't know. they only go to the runtime in a
// module that already depends on it
bool MSILWriter::isRuntimeHookFunction(StringRef Name) {
  return Name == "atexit" || Name == "at_quick_exit" || Name == "quick_exit" ||
         isThreadHookFunction(Name);
}

// a managed thread routine or key destructor can't run without the runtime,
// so these pull it in on their own
bool MSILWriter::isThreadHookFunction(StringRef Name) {
  return Name == "pthread_create" || Name == "pthread_key_create" ||
         Name == "pthread_key_delete";
}

bool MSILWriter::isCxxRuntimeGlobal(const GlobalVariable &G) {
  if (!G.isDeclaration())
    return false;
  StringRef Name = G.getName();
  // `__dso_handle` is deliberately absent. Clang names it in every ELF module
  // that registers an `__cxa_atexit`, so it is evidence that a global
  // destructor exists, not that the exception runtime does; counting it made
  // MSIL.Runtime.dll mandatory for an ordinary C++ program and changed its
  // output. It still gets bound (bindsExternalGlobal) when the runtime is in
  // use for another reason, and otherwise stays the private field it was.
  return Name.starts_with("_ZTVN10__cxxabiv1") || Name.starts_with("_ZTI") ||
         Name.starts_with("_ZTS");
}

// once a module talks to MSIL.Runtime anyway, every external global goes
// through it: data symbols like glibc's stdout can't be pinvoked
bool MSILWriter::bindsExternalGlobal(const GlobalVariable &G) const {
  return UsesCxxRuntime && G.isDeclaration() && !G.isThreadLocal();
}

static bool isAbiTypeInfoVTable(const GlobalVariable &G) {
  return !G.isDeclaration() && G.getName().starts_with("_ZTVN10__cxxabiv1");
}

bool MSILWriter::hasCxxRuntimeGlobals() const {
  return any_of(Mod->globals(), [](const GlobalVariable &G) {
    return isCxxRuntimeGlobal(G);
  });
}

bool MSILWriter::usesCxxRuntime() const {
  if (hasCxxRuntimeGlobals() || any_of(Mod->globals(), isAbiTypeInfoVTable))
    return true;
  for (const Function &Fn : *Mod) {
    if (Fn.isDeclaration()) {
      if (isCxxRuntimeFunction(Fn.getName()) ||
          isThreadHookFunction(Fn.getName()))
        return true;
      continue;
    }
    for (const BasicBlock &BB : Fn)
      if (isa<InvokeInst>(BB.getTerminator()) ||
          isa<ResumeInst>(BB.getTerminator()) || BB.isLandingPad())
        return true;
  }
  return false;
}

bool MSILWriter::needsInitializer() const {
  if (!Templates.empty())
    return true;
  return any_of(Mod->globals(), [this](const GlobalVariable &G) {
    return bindsExternalGlobal(G);
  });
}

// selector 0 means no clause matched, so ids start at 1
unsigned MSILWriter::getEHTypeId(const Value *TypeInfo) {
  const Value *Key = TypeInfo->stripPointerCasts();
  auto Inserted = EHTypeIds.try_emplace(Key, EHTypeIds.size() + 1);
  return Inserted.first->second;
}

// only CxxException is caught, so abort/unreachable/faults skip C++ handlers
// like they do natively
void MSILWriter::printInvoke(const InvokeInst *II) {
  const BasicBlock *From = II->getParent();
  std::string Label = "IL_invoke" + utostr(getUniqID());
  out() << "\t.try\n\t{\n";
  printCallInstruction(II);
  if (!II->getType()->isVoidTy())
    printValueSave(II);
  out() << "\tleave\t" << Label << "_normal\n\t}\n";
  out() << "\tcatch " << CxxExceptionRef << "\n\t{\n";
  out() << "\tcall\tvoid* " << CxxAbiRef << "::Landing(" << CxxExceptionTy
        << ")\n";
  out() << "\tstloc\t'" << ExceptionLocal << "'\n";
  out() << "\tleave\t" << Label << "_unwind\n\t}\n";

  out() << Label << "_unwind:\n";
  printPHICopy(From, II->getUnwindDest());
  out() << "\tbr\t" << getLabelName(II->getUnwindDest()) << '\n';
  out() << Label << "_normal:\n";
  printPHICopy(From, II->getNormalDest());
  out() << "\tbr\t" << getLabelName(II->getNormalDest()) << '\n';
}

void MSILWriter::printLandingPad(const LandingPadInst *LP) {
  auto *ResultTy = cast<StructType>(LP->getType());
  Type *I32 = Type::getInt32Ty(Mod->getContext());
  std::string Selector = newTempLocal(I32);
  std::string Result = newTempLocal(ResultTy);
  std::string Done = "IL_lpad" + utostr(getUniqID());

  out() << "\tldc.i4.0\n\tstloc\t'" << Selector << "'\n";
  for (unsigned I = 0, E = LP->getNumClauses(); I != E; ++I) {
    if (LP->isFilter(I))
      fatalError("landingpad filter clauses (dynamic exception "
                         "specifications) are not supported");
    const Value *TypeInfo = LP->getClause(I);
    std::string Next = Done + "_next" + utostr(I);
    out() << "\tldloc\t'" << ExceptionLocal << "'\n";
    printValueLoad(TypeInfo);
    out() << "\tcall\tbool " << CxxAbiRef << "::Matches(void*, void*)\n";
    out() << "\tbrfalse\t" << Next << '\n';
    out() << "\tldc.i4\t" << getEHTypeId(TypeInfo) << "\n\tstloc\t'" << Selector
          << "'\n\tbr\t" << Done << '\n';
    out() << Next << ":\n";
  }
  out() << Done << ":\n";

  const StructLayout *Layout = DL->getStructLayout(ResultTy);
  printAggregateLaneAddr(Result, ResultTy->getElementType(0), 0);
  out() << "\tldloc\t'" << ExceptionLocal << "'\n\tstind.i\n";
  printAggregateLaneAddr(Result, I32, Layout->getElementOffset(1));
  out() << "\tldloc\t'" << Selector << "'\n\tstind.i4\n";
  out() << "\tldloc\t'" << Result << "'\n";
}

void MSILWriter::printResume(const ResumeInst *RI) {
  const Value *Pair = RI->getValue();
  std::string PairLocal = newTempLocal(Pair->getType());
  printValueLoad(Pair);
  out() << "\tstloc\t'" << PairLocal << "'\n";
  printAggregateLaneAddr(PairLocal, PointerType::getUnqual(Mod->getContext()),
                         0);
  out() << "\tldind.i\n";
  out() << "\tcall\t" << CxxExceptionTy << ' ' << CxxAbiRef
        << "::Resume(void*)\n\tthrow\n";
}

void MSILWriter::printCxxRuntimeSymbols() {
  for (const GlobalVariable &G : Mod->globals()) {
    if (!bindsExternalGlobal(G))
      continue;
    out() << "\tldstr\t\"" << G.getName() << "\"\n";
    out() << "\tcall\tvoid* " << CxxAbiRef << "::Symbol(string)\n";
    out() << "\tstsfld\tvoid* '" << G.getName() << "'\n";
  }
}

// libc++abi's own type_info vtables, so a catch can use its can_catch
void MSILWriter::printAbiVTableRegistrations() {
  if (!UsesCxxRuntime)
    return;
  for (const GlobalVariable &G : Mod->globals()) {
    if (!isAbiTypeInfoVTable(G))
      continue;
    out() << "\tldstr\t\"" << G.getName() << "\"\n";
    printValueLoad(&G);
    out() << "\tcall\tvoid " << CxxAbiRef
          << "::RegisterVTable(string, void*)\n";
  }
}
