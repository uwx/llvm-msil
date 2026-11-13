//===-- MSILWriterRuntime.cpp - Managed runtime binding, libc and pinvokes ---===//
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

// The portable C library, compiled to IR and embedded as a string literal by
// the build. lib/Target/MSIL/libc/*.c is the source of truth.
//
// Two ABI builds exist because long and wchar_t follow the target triple and
// the library's signatures have to match the program's. A build with no clang
// falls back to the checked-in Windows-ABI IR alone.
#include "libc-win.inc"
#if __has_include("libc-sysv.inc")
#include "libc-sysv.inc"
#define MSIL_HAVE_LIBC_SYSV
#endif

// The portable C++ runtime (libcxx/cxxrt.cpp), compiled to IR and embedded the
// same way. Optional: a build without clang++ has no copies, and the option
// that asks for it then reports that the tool cannot supply it.
#if __has_include("cxxrt-win.inc")
#include "cxxrt-win.inc"
#define MSIL_HAVE_CXXRT
#if __has_include("cxxrt-sysv.inc")
#include "cxxrt-sysv.inc"
#define MSIL_HAVE_CXXRT_SYSV
#endif
#if __has_include("cxxrt-mingw.inc")
#include "cxxrt-mingw.inc"
#define MSIL_HAVE_CXXRT_MINGW
#endif
#endif

using namespace llvm;

std::string MSILWriter::getCName(const Function *F) const {
  // The C runtime exports C names, but a C++ translation unit may call them
  // through a mangled declaration (`_Z6printfPKcz`). Recover the base name
  // when the mangled form denotes a global function in the C runtime.
  StringRef Name = F->getName();
  if (!Name.starts_with("_Z"))
    return Name.str();
  std::string D = demangle(Name.str());
  size_t Paren = D.find('(');
  if (Paren == std::string::npos)
    return Name.str();
  StringRef Base = StringRef(D).substr(0, Paren);
  if (Base.empty())
    return Name.str();
  return Base.str();
}

std::string MSILWriter::getImportName(const Function *F) const {
  std::string Name = getCName(F);
  // The renames below are MSVC CRT spellings; an ELF target exports the C
  // names themselves.
  if (Triple(Mod->getTargetTriple()).isOSBinFormatELF())
    return Name;
  // POSIX-only names (from an implicit declaration, so the MSVC header never
  // rewrote them) only exist under their underscore spelling in the CRT.
  // shortcut: hand-written list; grow it as programs need more names. Also
  // _snprintf does not NUL-terminate on truncation, unlike C99 snprintf.
  static const char *const PosixRenames[][2] = {
      {"strdup", "_strdup"},       {"strcasecmp", "_stricmp"},
      {"strncasecmp", "_strnicmp"}, {"stricmp", "_stricmp"},
      {"strnicmp", "_strnicmp"},   {"strndup", "_strndup"},
      {"snprintf", "_snprintf"},   {"vsnprintf", "_vsnprintf"},
      {"scprintf", "_scprintf"},   {"snscanf", "_snscanf"},
      {"itoa", "_itoa"},           {"ltoa", "_ltoa"},
      {"ultoa", "_ultoa"},         {"i64toa", "_i64toa"},
      {"ui64toa", "_ui64toa"},
  };
  for (auto &R : PosixRenames)
    if (Name == R[0])
      return R[1];
  return Name;
}

// Read the export table of every -msil-lib. A name that is not a path is
// resolved the way the platform loader would resolve it, so
// `-msil-lib=user32.dll` finds the system copy without naming a directory.
void MSILWriter::resolveLibExports() {
  for (const std::string &Spec : LibSpec) {
    std::string Path = Spec;
    if (!sys::fs::exists(Path)) {
      // A bare name is looked for the way the loader would: the application
      // directory, then the system one. findProgramByName is for executables
      // on Windows -- it appends `.exe` -- so a `.dll` never matches.
      SmallString<256> ExeDir(sys::fs::getMainExecutable(nullptr, nullptr));
      sys::path::remove_filename(ExeDir);
      SmallString<256> SysDir;
      if (sys::path::has_root_name(ExeDir))
        SysDir.append(sys::path::root_name(ExeDir));
      SysDir.append(sys::path::get_separator());
      SysDir.append("Windows");
      SysDir.append(sys::path::get_separator());
      SysDir.append("System32");
      bool Found = false;
      for (const SmallString<256> &Dir : {ExeDir, SysDir}) {
        SmallString<256> Candidate(Dir);
        sys::path::append(Candidate, Spec);
        if (sys::fs::exists(Candidate)) {
          Path = Candidate.str().str();
          Found = true;
          break;
        }
      }
      // A name with no directory is also worth trying on PATH, which is where
      // a development SDK usually puts its DLLs.
      if (!Found) {
        ErrorOr<std::string> OnPath = sys::findProgramByName(Spec);
        if (OnPath) {
          Path = *OnPath;
          Found = true;
        }
      }
      if (!Found)
        fatalError(Twine("-msil-lib could not find '") + Spec +
                           "'");
    }
    Expected<object::OwningBinary<object::Binary>> BinOrErr =
        object::createBinary(Path);
    if (!BinOrErr)
      fatalError(Twine("-msil-lib cannot read '") + Spec + "': " +
                         toString(BinOrErr.takeError()));
    object::Binary *Bin = BinOrErr->getBinary();
    const object::ObjectFile *Obj = dyn_cast<object::ObjectFile>(Bin);
    if (!Obj)
      fatalError(Twine("-msil-lib '") + Spec +
                         "' is not an object file with an export table");
    std::string Dll = sys::path::filename(Path).str();
    if (const auto *Elf = dyn_cast<object::ELFObjectFileBase>(Obj)) {
      addElfExports(*Elf, Dll);
      continue;
    }
    const object::COFFObjectFile *Coff =
        dyn_cast<object::COFFObjectFile>(Obj);
    if (!Coff)
      fatalError(Twine("-msil-lib '") + Spec +
                         "' is neither a COFF DLL nor an ELF shared object");
    // First library to export a symbol wins, so an earlier -msil-lib can
    // shadow a later one exactly as an earlier -l would.
    for (const object::ExportDirectoryEntryRef &E :
         Coff->export_directories()) {
      StringRef Name;
      if (E.getSymbolName(Name) || Name.empty())
        continue;
      LibExports.try_emplace(Name.str(), Dll);
    }
  }
}

void MSILWriter::addElfExports(const object::ELFObjectFileBase &Elf,
                               StringRef Dll) {
  for (const object::ELFSymbolRef &Sym : Elf.getDynamicSymbolIterators()) {
    Expected<uint32_t> Flags = Sym.getFlags();
    if (!Flags) {
      consumeError(Flags.takeError());
      continue;
    }
    if (*Flags & object::SymbolRef::SF_Undefined)
      continue;
    if (!(*Flags & object::SymbolRef::SF_Global))
      continue;
    Expected<StringRef> Name = Sym.getName();
    if (!Name) {
      consumeError(Name.takeError());
      continue;
    }
    if (Name->empty())
      continue;
    LibExports.try_emplace(Name->str(), Dll.str());
  }
}

void MSILWriter::resolveLibcOptions() {
  // Only one mapping exists, so a typo must not silently leave the default.
  if (!Fp80Spec.empty() && Fp80Spec != "double")
    fatalError("-msil-fp80 only knows 'double', got '" +
                       Twine(Fp80Spec) + "'");
  resolveLibExports();
  // `assembly:type`, or just `assembly` when a default type is given.
  auto Parse = [](StringRef Spec, StringRef DefaultType, StringRef Option) {
    ManagedType T;
    size_t Colon = Spec.rfind(':');
    T.Assembly = (Colon == StringRef::npos ? Spec : Spec.substr(0, Colon)).str();
    T.Type = Colon == StringRef::npos ? DefaultType.str()
                                      : Spec.substr(Colon + 1).str();
    if (T.Assembly.empty() || T.Type.empty())
      fatalError(Option + " needs <assembly>:<type>, got '" + Spec + "'");
    return T;
  };
  if (!AllocSpec.empty())
    AllocType = Parse(AllocSpec, "", "-msil-alloc");
  for (const std::string &Spec : LibMapSpec) {
    size_t Eq = Spec.find('=');
    if (Eq == std::string::npos || Eq + 1 == Spec.size())
      fatalError(Twine("-msil-lib-map needs <pattern>=<dll>, "
                               "got '") + Spec + "'");
    LibMap.emplace_back(StringRef(Spec).substr(0, Eq).str(),
                        StringRef(Spec).substr(Eq + 1).str());
  }
  // The C++ runtime is a separate link from the C one: a C++ program needs
  // both. Validated here so a typo fails at once instead of at run time.
  if (!LibcxxSpec.empty()) {
    if (LibcxxSpec != "builtin")
      fatalError("-msil-libcxx only knows 'builtin', got '" +
                         Twine(LibcxxSpec) + "'");
#ifndef MSIL_HAVE_CXXRT
    fatalError("-msil-libcxx=builtin needs the portable C++ "
                       "runtime, which this build did not compile (no "
                       "clang++)");
#else
    EmitCxxRt = true;
#endif
  }
  if (LibcSpec.empty())
    return;
  if (LibcSpec == "builtin") {
    // 'builtin' is the portable C library: a wide, tested surface. 'il' keeps
    // the smaller hand-written subset, for a module that must link no C in.
    EmitPortable = true;
    // Nothing needs rebinding: the linked definitions carry the C names, so
    // an empty type leaves every declaration to the link. A user-supplied
    // type still takes precedence and is handled by the spec above.
    LibcType = {"", ""};
    return;
  }
  if (LibcSpec == "il") {
    EmitLibc = true;
    LibcType = {"", "$MSIL_Libc"};
    return;
  }
  LibcType = Parse(LibcSpec, "MSIL.Libc", "-msil-libc");
}

bool MSILWriter::getManagedTarget(const Function *F, ManagedType &Out) const {
  // A vararg function cannot cross a managed boundary in a way ilasm accepts,
  // so it keeps its pinvoke thunk.
  if (F->getFunctionType()->isVarArg())
    return false;
  bool RuntimeHook = UsesCxxRuntime && isRuntimeHookFunction(F->getName());
  // The exception machinery always binds. The allocation operators and the
  // exit-time hooks only do when the runtime is already in use, so a program
  // that throws pays for the DLL but a program that just calls `new` keeps
  // binding to the C library as it did before.
  if (isCxxRuntimeFunction(F->getName()) ||
      (UsesCxxRuntime && isCxxSupportFunction(F->getName())) || RuntimeHook) {
    Out = {CxxRuntimeAssembly, CxxAbiTypeName,
           getCxxRuntimeMember(F->getName()).str(), ""};
    return true;
  }
  // Held by value: getCName returns a temporary, and a StringRef over it
  // would dangle before the first use below.
  std::string Name = getCName(F);
  // Math/MathF first: these are platform ABI, so they must win over a
  // -msil-libc the user supplied, which is about malloc and friends.
  if (MathBind && getMathTarget(Name, F->getReturnType(), Out))
    return true;
  // The allocator hook wins: it exists precisely to intercept these calls.
  if (!AllocType.Type.empty() &&
      (Name == "malloc" || Name == "free" || Name == "calloc" ||
       Name == "realloc")) {
    Out = AllocType;
    return true;
  }
  // In portable mode LibcType is {"", ""}, so the guard below would reject
  // every symbol. The 18 names the IL libc implements must be routed to it
  // first; everything else stays a declaration for the linked C library.
  if (EmitPortable && isILLibcSymbol(Name)) {
    Out = {"", "$MSIL_Libc"};
    return true;
  }
  if (LibcType.Type.empty())
    return false;
  // The hand-written IL subset implements a fixed set; anything else keeps
  // its pinvoke, so a program can use both. The portable library is linked
  // selectively, so a name it does not define simply stays a declaration.
  if (EmitLibc && !EmitPortable && !isBuiltinSymbol(Name))
    return false;
  Out = LibcType;
  return true;
}

// The C libm names that have a managed equivalent with identical semantics.
// Everything else keeps its CRT pinvoke: `exp10` has no managed counterpart,
// and fmin/fmax deliberately do not because Math.Min/Max propagate NaN where
// the C functions return the other operand. `exp2` binds to
// System.Double/Single::Exp2 rather than a Math member.
bool MSILWriter::getMathTarget(StringRef Symbol, Type *Ty,
                               ManagedType &Out) const {
  bool IsF32 = Ty->isFloatTy();
  // A linked `floorl` returns x86_fp80 under -msil-fp80, which is a binary64
  // value in the emitted IL, so it takes the double member like any other.
  bool IsF80 = Ty->isX86_FP80Ty() && fp80MappedToDouble();
  if (!IsF32 && !Ty->isDoubleTy() && !IsF80)
    return false;

  // C name -> Math/MathF member. Where the C name is already the managed one,
  // the member is left empty and getCName's own name is used.
  static const StringMap<const char *> Members = {
      {"sqrt", "Sqrt"},   {"fabs", "Abs"},     {"floor", "Floor"},
      {"ceil", "Ceiling"}, {"trunc", "Truncate"}, {"round", "Round"},
      {"sin", "Sin"},     {"cos", "Cos"},      {"tan", "Tan"},
      {"asin", "Asin"},   {"acos", "Acos"},    {"atan", "Atan"},
      {"sinh", "Sinh"},   {"cosh", "Cosh"},    {"tanh", "Tanh"},
      {"exp", "Exp"},     {"log", "Log"},      {"log2", "Log2"},
      {"log10", "Log10"}, {"pow", "Pow"},      {"copysign", "CopySign"},
      {"scalbn", "ScaleB"}, {"ldexp", "ScaleB"}, {"exp2", "Exp2"},
      // rint, nearbyint and roundeven all round to even under the default
      // rounding mode, which is Math.Round with no MidpointRounding argument.
      // (fabs is not here: the intrinsic lowering already clears the sign bit,
      // which is exact and cheaper than a call.)
      {"rint", "Round"}, {"nearbyint", "Round"}, {"roundeven", "Round"},
  };
  // The C names carry an `f` suffix on the float spelling, and an `l` one on
  // the long double spelling the ELF front end emits (`floorl`). Under
  // -msil-fp80 that value is a binary64, so the double member is the match.
  StringRef Base = Symbol;
  if (IsF32 && Base.ends_with("f") && Base != "fabs" && !Members.count(Base))
    Base = Base.drop_back();
  if (IsF80 && Base.ends_with("l") && !Members.count(Base))
    Base = Base.drop_back();

  auto It = Members.find(Base);
  if (It == Members.end()) {
    // lrint/llrint/lround/llround are the integer-returning roundings; the
    // managed Round returns the float, so those keep their pinvoke.
    if (Base == "fmin" || Base == "fmax")
      return false;
    return false;
  }

  std::string Member = It->second;
  Out.Assembly = "System.Runtime";
  Out.Type = IsF32 ? "System.MathF" : "System.Math";
  // Exp2 is a static member of the primitive types, not of Math/MathF.
  if (Base == "exp2")
    Out.Type = IsF32 ? "System.Single" : "System.Double";
  Out.Member = Member;
  Out.ExtraArg.clear();
  // Math.Round is banker's rounding, which is C's round() only for roundeven;
  // C's round() rounds half away from zero, i.e. MidpointRounding.AwayFromZero.
  // `roundeven` maps to Round by default, so only `round` takes the argument.
  if (Base == "round")
    Out.ExtraArg = "\tldc.i4.1\n";
  return true;
}

std::string MSILWriter::getManagedName(const ManagedType &T,
                                       StringRef Member) const {
  std::string S = T.Assembly.empty() ? "" : "[" + T.Assembly + "]";
  return S + T.Type + "::" + Member.str();
}

void MSILWriter::printManagedForwarder(const Function *F, const ManagedType &T,
                                       StringRef Name) {
  FunctionType *FTy = F->getFunctionType();
  // With -msil-pointers=i64 the C-visible declaration uses int64 pointers, but
  // the managed member it forwards to (the hand-written IL libc, a user's
  // allocator) keeps the CLR native-int void*. Convert at this one boundary:
  // conv.u on a pointer argument going in, conv.i8 on a pointer result out.
  std::string Params;
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
    if (I)
      Params += ", ";
    Type *PTy = FTy->getParamType(I);
    Params += (pointersAsInt64() && PTy->isPointerTy()) ? "void*"
                                                       : getTypeName(PTy);
  }
  // A member whose signature is wider than the C function's takes the extra
  // argument as a literal before the forwarded ones.
  if (!T.ExtraArg.empty()) {
    std::string Base = std::string(", ")
        + (T.Type == "System.MathF" ? "valuetype " : "valuetype ")
        + "[System.Runtime]System.MidpointRounding";
    if (Params.empty())
      Params = Base.substr(2);
    else
      Params += Base;
  }
  out() << ".method public static hidebysig "
      << getCallSignature(FTy, nullptr, "'" + Name.str() + "'")
      << " cil managed\n{\n";
  out() << "\t.maxstack\t" << std::max<unsigned>(1, FTy->getNumParams() + 1)
      << "\n";
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
    out() << "\tldarg\t" << I << '\n';
    if (pointersAsInt64() && FTy->getParamType(I)->isPointerTy())
      out() << "\tconv.u\n";
  }
  out() << T.ExtraArg;
  // By value: the ternary would otherwise yield a temporary std::string and
  // the StringRef would point into it after it dies.
  std::string Member = T.Member.empty() ? getCName(F) : T.Member;
  Type *RTy = FTy->getReturnType();
  bool PtrRet = pointersAsInt64() && RTy->isPointerTy();
  out() << "\tcall\t" << (PtrRet ? "void*" : getTypeName(RTy)) << ' '
      << getManagedName(T, Member) << '(' << Params << ")\n";
  if (PtrRet)
    out() << "\tconv.i8\n";
  out() << "\tret\n}\n";
}

//===----------------------------------------------------------------------===//
// The built-in libc
//===----------------------------------------------------------------------===//

// These are byte loops rather than calls into something faster, because the
// point is a runtime that depends on nothing but the CLR itself. `ldind.u1`
// zero-extends, which is exactly what C's unsigned-char comparisons want.
// Every parameter is unnamed, so bodies address arguments positionally.
// None of the methods recurse, so `.maxstack 4` cannot be amplified by depth.

static const char *const LibcStrlen = R"(
	.locals init (int64 'i')
	ldc.i4.0
	conv.i8
	stloc 'i'
	br IL_libc_test
IL_libc_loop:
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
IL_libc_test:
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	brtrue IL_libc_loop
	ldloc 'i'
	ret)";

static const char *const LibcStrcmp = R"(
	.locals init (int64 'i', int32 'x', int32 'y')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_loop:
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	stloc 'x'
	ldarg.1
	ldloc 'i'
	add
	ldind.u1
	stloc 'y'
	ldloc 'x'
	ldloc 'y'
	bne.un IL_libc_diff
	ldloc 'x'
	brfalse IL_libc_same
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_loop
IL_libc_diff:
	ldloc 'x'
	ldloc 'y'
	sub
	ret
IL_libc_same:
	ldc.i4.0
	ret)";

static const char *const LibcStrncmp = R"(
	.locals init (int64 'i', int32 'x', int32 'y')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_loop:
	ldloc 'i'
	ldarg.2
	bge IL_libc_same
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	stloc 'x'
	ldarg.1
	ldloc 'i'
	add
	ldind.u1
	stloc 'y'
	ldloc 'x'
	ldloc 'y'
	bne.un IL_libc_diff
	ldloc 'x'
	brfalse IL_libc_same
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_loop
IL_libc_diff:
	ldloc 'x'
	ldloc 'y'
	sub
	ret
IL_libc_same:
	ldc.i4.0
	ret)";

static const char *const LibcMemcmp = R"(
	.locals init (int64 'i', int32 'x', int32 'y')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_loop:
	ldloc 'i'
	ldarg.2
	bge IL_libc_same
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	stloc 'x'
	ldarg.1
	ldloc 'i'
	add
	ldind.u1
	stloc 'y'
	ldloc 'x'
	ldloc 'y'
	bne.un IL_libc_diff
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_loop
IL_libc_diff:
	ldloc 'x'
	ldloc 'y'
	sub
	ret
IL_libc_same:
	ldc.i4.0
	ret)";

static const char *const LibcStrcpy = R"(
	.locals init (int64 'i', int32 'c')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_loop:
	ldarg.1
	ldloc 'i'
	add
	ldind.u1
	stloc 'c'
	ldarg.0
	ldloc 'i'
	add
	ldloc 'c'
	stind.i1
	ldloc 'c'
	brfalse IL_libc_done
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_loop
IL_libc_done:
	ldarg.0
	ret)";

static const char *const LibcStrncpy = R"(
	.locals init (int64 'i', int32 'c')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_loop:
	ldloc 'i'
	ldarg.2
	bge IL_libc_done
	ldarg.1
	ldloc 'i'
	add
	ldind.u1
	stloc 'c'
	ldarg.0
	ldloc 'i'
	add
	ldloc 'c'
	stind.i1
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	ldloc 'c'
	brtrue IL_libc_loop
IL_libc_pad:
	ldloc 'i'
	ldarg.2
	bge IL_libc_done
	ldarg.0
	ldloc 'i'
	add
	ldc.i4.0
	stind.i1
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_pad
IL_libc_done:
	ldarg.0
	ret)";

static const char *const LibcStrcat = R"(
	.locals init (int64 'i', int64 'j', int32 'c')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_find:
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	brfalse IL_libc_copy_init
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_find
IL_libc_copy_init:
	ldc.i4.0
	conv.i8
	stloc 'j'
IL_libc_copy:
	ldarg.1
	ldloc 'j'
	add
	ldind.u1
	stloc 'c'
	ldarg.0
	ldloc 'i'
	add
	ldloc 'c'
	stind.i1
	ldloc 'c'
	brfalse IL_libc_done
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	ldloc 'j'
	ldc.i4.1
	conv.i8
	add
	stloc 'j'
	br IL_libc_copy
IL_libc_done:
	ldarg.0
	ret)";

static const char *const LibcStrchr = R"(
	.locals init (int64 'i', int32 'c')
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_loop:
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	stloc 'c'
	ldloc 'c'
	ldarg.1
	conv.u1
	beq IL_libc_found
	ldloc 'c'
	brfalse IL_libc_missing
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_loop
IL_libc_found:
	ldarg.0
	ldloc 'i'
	add
	ret
IL_libc_missing:
	ldc.i4.0
	conv.u
	ret)";

static const char *const LibcStrrchr = R"(
	.locals init (int64 'i', int32 'c', void* 'r')
	ldc.i4.0
	conv.i8
	stloc 'i'
	ldc.i4.0
	conv.u
	stloc 'r'
IL_libc_loop:
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	stloc 'c'
	ldloc 'c'
	ldarg.1
	conv.u1
	bne.un IL_libc_not
	ldarg.0
	ldloc 'i'
	add
	stloc 'r'
IL_libc_not:
	ldloc 'c'
	brfalse IL_libc_done
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_loop
IL_libc_done:
	ldloc 'r'
	ret)";

static const char *const LibcStrstr = R"(
	.locals init (int64 'i', int64 'j', int32 'c')
	ldarg.1
	ldind.u1
	brtrue IL_libc_outer_init
	ldarg.0
	ret
IL_libc_outer_init:
	ldc.i4.0
	conv.i8
	stloc 'i'
IL_libc_outer:
	ldarg.0
	ldloc 'i'
	add
	ldind.u1
	brfalse IL_libc_missing
	ldc.i4.0
	conv.i8
	stloc 'j'
IL_libc_inner:
	ldarg.1
	ldloc 'j'
	add
	ldind.u1
	stloc 'c'
	ldloc 'c'
	brfalse IL_libc_found
	ldarg.0
	ldloc 'i'
	add
	ldloc 'j'
	add
	ldind.u1
	ldloc 'c'
	bne.un IL_libc_next
	ldloc 'j'
	ldc.i4.1
	conv.i8
	add
	stloc 'j'
	br IL_libc_inner
IL_libc_next:
	ldloc 'i'
	ldc.i4.1
	conv.i8
	add
	stloc 'i'
	br IL_libc_outer
IL_libc_found:
	ldarg.0
	ldloc 'i'
	add
	ret
IL_libc_missing:
	ldc.i4.0
	conv.u
	ret)";

// strdup is a builtin that is not a symbol: it is the one caller inside the
// libc, and calling malloc/memcpy by name keeps the -msil-alloc hook in effect.
static const char *const LibcStrdup = R"(
	.locals init (void* 'p', int64 'n')
	ldarg.0
	call	int64 '$MSIL_Libc'::strlen(void*)
	ldc.i4.1
	conv.i8
	add
	stloc 'n'
	ldloc 'n'
	call	void* '$MSIL_Libc'::malloc(int64)
	stloc 'p'
	ldloc 'p'
	brfalse IL_libc_done
	ldloc 'p'
	ldarg.0
	ldloc 'n'
	call	void* '$MSIL_Libc'::memcpy(void*, void*, int64)
	pop
IL_libc_done:
	ldloc 'p'
	ret)";

namespace {
/// One method of the built-in libc.
struct LibcMethod {
  const char *Name;
  const char *Ret;
  const char *Params;
  const char *Body;
};
} // namespace

void MSILWriter::printLibcMethod(StringRef Name, StringRef Ret,
                                 StringRef Params, StringRef Body) {
  out() << ".method public static hidebysig " << Ret << " '" << Name << "'("
      << Params << ") cil managed\n{\n\t.maxstack\t4\n"
      << Body << "\n}\n\n";
}

//===----------------------------------------------------------------------===//
// The portable C library
//===----------------------------------------------------------------------===//

void MSILWriter::linkPortableLibc() {
  LLVMContext &Ctx = Mod->getContext();
  SMDiagnostic Err;
  // The library's C signatures carry the ABI's long and wchar_t, so the copy
  // has to be the one built for the program's triple. A Linux-triple program
  // declares strtol as returning i64; the Windows-ABI copy defines i32, and
  // linking the two leaves the call unresolved.
  const char *IR = PortableLibcIR;
#ifdef MSIL_HAVE_LIBC_SYSV
  if (Triple(Mod->getTargetTriple()).isOSBinFormatELF())
    IR = PortableLibcIRSysV;
#endif
  std::unique_ptr<MemoryBuffer> Buf =
      MemoryBuffer::getMemBuffer(IR, "portable-libc", false);
  std::unique_ptr<Module> Libc = parseAssembly(*Buf, Err, Ctx);
  if (!Libc)
    fatalError("the embedded portable libc does not parse: " +
                       Err.getMessage());

  // The embedded module was compiled for the same pointer size, so the layouts
  // agree; adopt this module's so the link does not warn about a mismatch.
  Libc->setDataLayout(Mod->getDataLayout());
  Libc->setTargetTriple(Mod->getTargetTriple());
  // wchar_size and friends come from the triple that compiled the library, and
  // the linker refuses two modules that disagree. The program's values are the
  // ones that describe this module, so they replace the library's.
  SmallVector<Module::ModuleFlagEntry, 8> Flags;
  Mod->getModuleFlagsMetadata(Flags);
  for (const Module::ModuleFlagEntry &Flag : Flags)
    Libc->setModuleFlag(Flag.Behavior, Flag.Key->getString(), Flag.Val);

  // Only what the module actually reaches is merged: LinkOnlyNeeded follows
  // references from the module being linked into, and skips the rest, so a
  // program that never calls printf does not carry it.
  if (Linker::linkModules(*Mod, std::move(Libc), Linker::LinkOnlyNeeded))
    fatalError("linking the portable libc failed");
  // Every seam function the linked code refers to now needs its IL.
  bool AnySeam = false;
  for (Function &F : *Mod)
    if (F.isDeclaration() && F.getName().starts_with("__msil_"))
      AnySeam = true;
  UsesLibcSeam = AnySeam;
}

// The C++ runtime is linked the same way the C library is: parse the embedded
// IR, adopt this module's layout/triple/flags, then LinkOnlyNeeded so only the
// symbols the program actually references are merged.
void MSILWriter::linkPortableCxxRt() {
#ifdef MSIL_HAVE_CXXRT
  LLVMContext &Ctx = Mod->getContext();
  SMDiagnostic Err;
  // Which copy is right is the C++ ABI's question, not the object format's:
  // a mingw triple is COFF and still Itanium-mangled, but its `size_t` is
  // `unsigned long long`, so its operators are `_Znwy`, not `_Znwm`. Each
  // triple therefore has its own copy.
  Triple TT(Mod->getTargetTriple());
  const char *IR = PortableCxxRtIR;
#ifdef MSIL_HAVE_CXXRT_SYSV
  if (TT.isOSBinFormatELF())
    IR = PortableCxxRtIRSysV;
#endif
#ifdef MSIL_HAVE_CXXRT_MINGW
  if (TT.isWindowsGNUEnvironment())
    IR = PortableCxxRtIRMingw;
#endif
  std::unique_ptr<MemoryBuffer> Buf =
      MemoryBuffer::getMemBuffer(IR, "portable-cxxrt", false);
  std::unique_ptr<Module> Rt = parseAssembly(*Buf, Err, Ctx);
  if (!Rt)
    fatalError("the embedded portable C++ runtime does not "
                       "parse: " + Err.getMessage());
  Rt->setDataLayout(Mod->getDataLayout());
  Rt->setTargetTriple(Mod->getTargetTriple());
  SmallVector<Module::ModuleFlagEntry, 8> Flags;
  Mod->getModuleFlagsMetadata(Flags);
  for (const Module::ModuleFlagEntry &Flag : Flags)
    Rt->setModuleFlag(Flag.Behavior, Flag.Key->getString(), Flag.Val);
  // The MSVC local-static protocol reads a per-thread epoch the front end
  // declares as an external thread-local. This module has one thread and no
  // way to reach another module's thread-local storage (an external thread-local
  // is a diagnostic for exactly that reason), so rewrite the declaration into an
  // ordinary external global of the same type before the link: the runtime
  // defines it, and with a single thread a plain global is the same thing.
  if (GlobalVariable *Epoch = Mod->getGlobalVariable("_Init_thread_epoch")) {
    if (Epoch->isThreadLocal() && Epoch->isDeclaration()) {
      auto *Plain = new GlobalVariable(
          *Mod, Epoch->getValueType(), /*isConstant=*/false,
          GlobalValue::ExternalLinkage, nullptr, "_Init_thread_epoch");
      Epoch->replaceAllUsesWith(Plain);
      Epoch->eraseFromParent();
      // The name was taken while the thread-local declaration still existed, so
      // the new global got a numbered suffix; take the plain name now that it is
      // free, or the runtime's definition would not merge with the uses.
      Plain->setName("_Init_thread_epoch");
    }
  }
  // LinkOnlyNeeded keeps a source definition only when the destination already
  // declares the name. Nothing in the IR refers to the teardown hook -- the
  // startup stub calls it in emitted IL -- so declare it here to pull its body
  // in. It is a no-op when no destructor was registered.
  if (!Mod->getFunction("__msil_cxx_finalize"))
    Mod->getOrInsertFunction("__msil_cxx_finalize",
                             FunctionType::get(Type::getVoidTy(Ctx), false));
  if (Linker::linkModules(*Mod, std::move(Rt), Linker::LinkOnlyNeeded))
    fatalError("linking the portable C++ runtime failed");
  // The runtime calls __msil_write on its abort path, which needs the seam
  // emitted even if the C library did not pull it in.
  for (Function &F : *Mod)
    if (F.isDeclaration() && F.getName().starts_with("__msil_"))
      UsesLibcSeam = true;
#endif
}

// The address of the caller's va_list slot. The C passes &ap, so on Windows
// that is already the address of the cursor, since a va_list there *is* the
// cursor. On ELF the C passes the address of a variable holding a pointer to
// the __va_list_tag (that is what a va_list is there); one more load reaches
// the tag itself. Writing through the returned address is what makes the
// advance visible to the C.
std::string MSILWriter::vaListPtr() {
  std::string B;
  raw_string_ostream OS(B);
  OS << "\n\tldarg.0";
  if (pointersAsInt64())
    OS << "\n\tconv.u";
  if (vaIsTag())
    OS << "\n\tldind.i";
  return B;
}

// The address of the cursor inside the va_list. \p Tag is a local holding the
// tag on ELF; on Windows the va_list slot is the cursor, so \p Arg is used.
std::string MSILWriter::vaCursorAddr(StringRef Tag) {
  std::string B;
  raw_string_ostream OS(B);
  if (vaIsTag())
    OS << "\n\tldloc\t'" << Tag << "'"
          "\n\tldc.i8\t8\n\tadd";  // &tag->overflow_arg_area
  else {
    OS << "\n\tldarg.0";           // the va_list slot is the cursor
    if (pointersAsInt64())
      OS << "\n\tconv.u";
  }
  return B;
}

// Every seam accessor reads one 8-byte slot out of the caller's buffer and
// advances the cursor.
//
// \p Convert turns the loaded int64 into the accessor's return type: nothing
// for int64, a memory round trip for a double's bits, `conv.u` for an address.
std::string MSILWriter::vaAccessorBody(StringRef Convert) {
  std::string B;
  raw_string_ostream OS(B);
  // 'tag' holds the va_list pointer on ELF and is unused on Windows.
  OS << "\n\t.locals init (native int 'tag', native int 'c', int64 'v')"
     << vaListPtr() << "\n\tstloc\t'tag'"
     << vaCursorAddr("tag") << "\n\tldind.i\n\tstloc\t'c'";
  // The value, before the cursor moves.
  OS << "\n\tldloc\t'c'\n\tldind.i8\n\tstloc\t'v'";
  // Advance the cursor the caller owns.
  OS << vaCursorAddr("tag") << "\n\tldloc\t'c'"
        "\n\tldc.i8\t8\n\tadd\n\tstind.i";
  OS << Convert << "\tret";
  return B;
}

void MSILWriter::printLibcSeam() {
  // Only the accessors the linked code actually calls are emitted.
  llvm::StringSet<> Wanted;
  for (Function &F : *Mod)
    if (F.isDeclaration() && F.getName().starts_with("__msil_"))
      Wanted.insert(F.getName());

  // Module-level, because the linked C calls these by their global names; a
  // class member would need the class prefix at every call site.
  //
  // A vararg argument crosses into the linked C as one of four CLR types,
  // matching what printVarArgBuffer writes at the call site.
  // The linked C declares these as taking a pointer; with -msil-pointers=i64
  // that declaration is an int64, so the definition has to spell it the same.
  std::string P = pointersAsInt64() ? "int64" : "void*";
  if (Wanted.count("__msil_va_i32"))
    printLibcMethod("__msil_va_i32", "int32", P,
                    vaAccessorBody("\n\tldloc\t'v'\n\tconv.i4\n"));
  if (Wanted.count("__msil_va_i64"))
    printLibcMethod("__msil_va_i64", "int64", P,
                    vaAccessorBody("\n\tldloc\t'v'\n"));
  if (Wanted.count("__msil_va_f64"))
    // The call site stored the double's raw bits in the slot, so the read
    // reinterprets them; conv.r8 would turn the integer into a value.
    printLibcMethod("__msil_va_f64", "float64", P,
                    vaAccessorBody("\n\tldloca\t'v'\n\tconv.u8\n\tldind.r8\n"));
  if (Wanted.count("__msil_va_ptr"))
    // A pointer is stored as its address, so the slot holds the value the
    // return type wants: an int64 in i64 mode, a native int otherwise.
    printLibcMethod("__msil_va_ptr", P, P,
                    vaAccessorBody(pointersAsInt64()
                                       ? "\n\tldloc\t'v'\n"
                                       : "\n\tldloc\t'v'\n\tconv.u\n"));

  if (Wanted.count("__msil_write"))
    printLibcMethod(
        "__msil_write", "void", P + ", int64",
        "\tldarg.0\n\tconv.i\n\tldarg.1\n\tconv.i4\n"
        "\tcall\tstring [mscorlib]System.Runtime.InteropServices.Marshal::"
        "PtrToStringAnsi(native int, int32)\n"
        "\tcall\tvoid [mscorlib]System.Console::Write(string)\n\tret");
}

void MSILWriter::printBuiltinLibc() {
  // Emit only what the module can reach, so the libc costs nothing a program
  // does not use. strdup calls malloc and memcpy, so it pulls both in.
  llvm::StringSet<> Wanted;
  for (Function &F : *Mod)
    if (F.isDeclaration() && !F.isIntrinsic())
      Wanted.insert(getCName(&F));
  // With a hook, the module's allocator calls bind straight to it; the only
  // remaining user of the libc's own allocator wrappers is strdup.
  if (!AllocType.Type.empty())
    for (const char *N : {"malloc", "calloc", "realloc", "free"})
      Wanted.erase(N);
  if (Wanted.count("strdup")) {
    Wanted.insert("malloc");
    Wanted.insert("memcpy");
  }

  // NativeMemory lives in the implementation assembly; System.Runtime forwards
  // many types but not that one.
  out() << ".assembly extern System.Private.CoreLib { .ver 9:0:0:0\n"
           "  .publickeytoken = (B0 3F 5F 7F 11 D5 0A 3A ) }\n";
  out() << ".class public auto ansi abstract sealed beforefieldinit "
           "'$MSIL_Libc' extends [mscorlib]System.Object\n{\n";

  // The allocator hook, when given, replaces the four allocating calls only:
  // the rest of the libc still runs on NativeMemory, and because strdup calls
  // malloc by name, its block goes through the hook as well. The two paths
  // differ in more than the callee: NativeMemory takes `native uint` sizes
  // where a hook keeps the C `int64`, so only the NativeMemory body converts.
  static const char *const Mem =
      "[System.Private.CoreLib]System.Runtime.InteropServices.NativeMemory::";
  const bool Hook = !AllocType.Type.empty();
  std::string HMalloc = getManagedName(AllocType, "malloc");
  std::string HCalloc = getManagedName(AllocType, "calloc");
  std::string HRealloc = getManagedName(AllocType, "realloc");
  std::string HFree = getManagedName(AllocType, "free");

  if (Wanted.count("malloc"))
    printLibcMethod("malloc", "void*", "int64",
                    Hook ? "\tldarg.0\n\tcall\tvoid* " + HMalloc +
                               "(int64)\n\tret"
                         : "\tldarg.0\n\tconv.u\n\tcall\tvoid* " +
                               std::string(Mem) + "Alloc(native uint)\n\tret");
  if (Wanted.count("calloc"))
    printLibcMethod(
        "calloc", "void*", "int64, int64",
        Hook ? "\tldarg.0\n\tldarg.1\n\tcall\tvoid* " + HCalloc +
                   "(int64, int64)\n\tret"
             : "\tldarg.0\n\tconv.u\n\tldarg.1\n\tconv.u\n\tcall\tvoid* " +
                   std::string(Mem) +
                   "AllocZeroed(native uint, native uint)\n\tret");
  if (Wanted.count("realloc"))
    printLibcMethod(
        "realloc", "void*", "void*, int64",
        Hook ? "\tldarg.0\n\tldarg.1\n\tcall\tvoid* " + HRealloc +
                   "(void*, int64)\n\tret"
             : "\tldarg.0\n\tldarg.1\n\tconv.u\n\tcall\tvoid* " +
                   std::string(Mem) + "Realloc(void*, native uint)\n\tret");
  if (Wanted.count("free"))
    printLibcMethod(
        "free", "void", "void*",
        Hook ? "\tldarg.0\n\tcall\tvoid " + HFree + "(void*)\n\tret"
             : "\tldarg.0\n\tcall\tvoid " + std::string(Mem) + "Free(void*)"
                   "\n\tret");

  // NativeMemory.Copy is overlap-safe, so it is memmove too; the two differ
  // only in the promise memcpy does not make. Both take (source, destination,
  // count), the reverse of memcpy's order.
  static const LibcMethod Methods[] = {
      {"memcmp", "int32", "void*, void*, int64", LibcMemcmp},
      {"strlen", "int64", "void*", LibcStrlen},
      {"strcmp", "int32", "void*, void*", LibcStrcmp},
      {"strncmp", "int32", "void*, void*, int64", LibcStrncmp},
      {"strcpy", "void*", "void*, void*", LibcStrcpy},
      {"strncpy", "void*", "void*, void*, int64", LibcStrncpy},
      {"strcat", "void*", "void*, void*", LibcStrcat},
      {"strchr", "void*", "void*, int32", LibcStrchr},
      {"strrchr", "void*", "void*, int32", LibcStrrchr},
      {"strstr", "void*", "void*, void*", LibcStrstr},
      {"strdup", "void*", "void*", LibcStrdup},
  };
  for (const char *Name : {"memcpy", "memmove"})
    if (Wanted.count(Name))
      printLibcMethod(Name, "void*", "void*, void*, int64",
                      "\tldarg.1\n\tldarg.0\n\tldarg.2\n\tconv.u\n\tcall\tvoid " +
                          std::string(Mem) +
                          "Copy(void*, void*, native uint)\n\tldarg.0\n\tret");
  if (Wanted.count("memset"))
    printLibcMethod("memset", "void*", "void*, int32, int64",
                    "\tldarg.0\n\tldarg.2\n\tconv.u\n\tldarg.1\n\tconv.u1\n"
                    "\tcall\tvoid " +
                        std::string(Mem) +
                        "Fill(void*, native uint, uint8)\n\tldarg.0\n\tret");
  for (const LibcMethod &M : Methods)
    if (Wanted.count(M.Name))
      printLibcMethod(M.Name, M.Ret, M.Params, M.Body);
  out() << "}\n\n";
}

StringRef MSILWriter::getLibraryFor(StringRef Symbol) {
  // An explicit map wins over everything: it exists to name a DLL that neither
  // the Win32 table nor the CRT heuristic has any way to know about.
  for (const auto &Pat : LibMap) {
    StringRef P = Pat.first;
    if (P.ends_with("*")) {
      if (Symbol.starts_with(P.drop_back()))
        return Pat.second;
    } else if (Symbol == P) {
      return Pat.second;
    }
  }
  if (!LibExports.empty()) {
    StringRef Dll = getLibraryForExports(Symbol);
    if (!Dll.empty())
      return Dll;
  }
  return getLibraryForCRT(Symbol);
}

// The exports of every library named with -msil-lib, bound to the module's
// symbol by name. This is the linker's rule: a library that exports the symbol
// takes it, in the order the libraries were given.
StringRef MSILWriter::getLibraryForExports(StringRef Symbol) {
  auto It = LibExports.find(Symbol);
  return It == LibExports.end() ? StringRef() : It->second;
}

StringRef MSILWriter::getLibraryForCRT(StringRef Symbol) {
  // glibc keeps libm separate, so a program using it passes
  // -msil-lib=<its libm.so.6> and the export table decides. The split below
  // is a Windows fact.
  if (Triple(Mod->getTargetTriple()).isOSBinFormatELF())
    return "libc.so.6";
  // The C runtime is split: msvcrt.dll has the C89 functions, and every C99
  // addition is only exported from ucrtbase.dll. This list is checked against
  // the real export tables rather than guessed — `ldexp`, `sinh` and `atan`
  // are C89 and live in msvcrt, while `trunc`, `round`, `exp2`, `log2` and
  // `scalbn` are C99 and do not.
  // shortcut: name-based split, not an import-library driven map.
  static const StringSet<> Ucrt = {
      "rint",     "rintf",     "nearbyint", "nearbyintf",
      "lround",   "lroundf",   "llround",   "llroundf",
      "lrint",    "lrintf",    "llrint",    "llrintf",
      "trunc",    "truncf",    "round",     "roundf",
      "exp2",     "exp2f",     "log2",      "log2f",
      "scalbn",   "scalbnf",   "copysign",  "copysignf",
      "fmin",     "fminf",     "fmax",      "fmaxf"};
  if (Symbol.starts_with("__acrt_") || Symbol.starts_with("__stdio_") ||
      Symbol.starts_with("_except") || Symbol.starts_with("__crt") ||
      Ucrt.count(Symbol))
    return "ucrtbase.dll";
  return "msvcrt.dll";
}

void MSILWriter::collectPInvokes() {
  for (Function &F : *Mod) {
    if (!F.isDeclaration() || F.isIntrinsic())
      continue;
    FunctionType *FTy = F.getFunctionType();
    if (!FTy->isVarArg())
      continue;
    for (User *U : F.users()) {
      auto *CB = dyn_cast<CallBase>(U);
      if (!CB)
        continue;
      // A vararg external is always a pinvoke (getManagedTarget refuses a
      // vararg signature), and SysV passes a long double on the stack where
      // the narrowed float64 would go in an SSE register: the callee reads
      // the wrong bytes. printf("%Lf") is the common way to hit this.
      if (fp80MappedToDouble()) {
        bool F80 = mentionsF80(FTy->getReturnType());
        for (Type *P : FTy->params())
          F80 |= mentionsF80(P);
        for (const Value *A : CB->args())
          F80 |= mentionsF80(A->getType());
        if (F80)
          fatalError(
              "external '" + F.getName() +
              "' is called with an x86_fp80 argument or result, which "
              "-msil-fp80=double cannot narrow across a pinvoke boundary; "
              "convert to double at the call site");
      }
      // The same normalization the call site will use, or the key would not
      // match and the call would name a thunk that was never emitted.
      bool Portable = EmitPortable && isPortableLibcSymbol(getCName(&F));
      std::string Key = getCallSignature(FTy, CB, "", Portable);
      if (PInvokeNames.count({&F, Key}))
        continue;
      std::string Name = F.getName().str() + "$" +
                         utostr(PInvokeNames.size());
      // Rebuild the signature without the trailing varargs, then name it.
      std::string Sig;
      Sig += getTypeName(FTy->getReturnType());
      Sig += " ";
      Sig += Name;
      Sig += "(";
      unsigned Fixed = FTy->getNumParams();
      for (unsigned I = 0, E = CB->arg_size(); I != E; ++I) {
        if (I)
          Sig += ", ";
        Type *Ty = CB->getArgOperand(I)->getType();
        Sig += I >= Fixed
                   ? (Portable ? getPortableVarArgTypeName(Ty)
                               : getVarArgTypeName(Ty))
                   : getTypeName(Ty);
      }
      Sig += ")";
      PInvokeNames[{&F, Key}] = Name;
      PInvokes.push_back({Name, Sig, getImportName(&F), F.getName().str()});
    }
  }
}

void MSILWriter::printExternals() {
  // Binds external declarations to managed code instead of the CRT. A symbol
  // with no managed target keeps its pinvoke.
  auto EmitExternal = [&](const Function &F, StringRef Name) {
    ManagedType T;
    if (getManagedTarget(&F, T)) {
      if (T.Assembly == "System.Runtime")
        UsesMath = true;
      printManagedForwarder(&F, T, Name);
      return;
    }
    // With -msil-fp80 the type is narrowed to binary64, but the symbol is a
    // real long-double function: an 80-bit x87 argument and return cannot be
    // satisfied by an 8-byte SSE signature, so the pinvoke would return wrong
    // numbers rather than fail. Only a System.Math binding is safe, since that
    // is a managed method with the narrowed signature. Anything else is a
    // diagnostic, which is the point of the flag being opt-in and honest.
    if (fp80MappedToDouble() &&
        mentionsF80(F.getFunctionType()))
      fatalError(
          "external '" + F.getName() +
          "' takes or returns x86_fp80 and has no System.Math binding, so "
          "-msil-fp80=double cannot narrow its pinvoke safely");
    out() << ".method public static hidebysig pinvokeimpl(\""
        << getLibraryFor(F.getName()) << "\" as \"" << getImportName(&F)
        << "\" cdecl) "
        << getCallSignature(F.getFunctionType(), nullptr, "'" + Name.str() + "'")
        << " preservesig {}\n";
  };

  for (Function &F : *Mod) {
    if (!F.isDeclaration() || F.isIntrinsic() || F.getFunctionType()->isVarArg())
      continue;
    // The seam methods are emitted as IL by printLibcSeam; a pinvoke of the
    // same name would be the one the JIT resolved.
    if (F.getName().starts_with("__msil_"))
      continue;
    EmitExternal(F, getValueName(&F));
  }
  for (const PInvoke &P : PInvokes)
    out() << ".method public static hidebysig pinvokeimpl(\""
        << getLibraryFor(P.Symbol) << "\" as \"" << P.Export
        << "\" cdecl) " << P.Signature << " preservesig {}\n";
  out() << '\n';

  bool AnyExternVar = false;
  for (GlobalVariable &G : Mod->globals()) {
    if (!G.isDeclaration())
      continue;
    // A declared thread-local has no storage this module can name: there is
    // no initializer to copy and no way to learn which thread's slot the
    // definition means. Emitting the ordinary pointer static would silently
    // alias all threads onto one address, so say so instead.
    if (G.isThreadLocal())
      fatalError("external thread-local '" + G.getName() +
                         "' is not supported; define it in this module");
    // A global the C++ runtime owns is a field the module's own code stores
    // into from $MSIL_Init, so it has to be visible to the assembly; anything
    // else stays private.
    const char *Visibility = bindsExternalGlobal(G) ? "assembly" : "private";
    out() << ".field static " << Visibility << " " << getTypeName(G.getType())
          << " '" << G.getName() << "'\n";
    AnyExternVar = true;
  }
  if (AnyExternVar)
    out() << '\n';
}
