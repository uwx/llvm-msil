//===-- MSILWriterShared.h - Helpers shared by the MSIL emitter --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// What more than one translation unit of the emitter needs: the option set, the
// IL spellings of the runtime types it binds to, and the predicates that decide
// how a type is represented.
//
// The helpers below live in an unnamed namespace, so each unit gets its own
// copy exactly as it had when the emitter was one file and they were `static`.
// That keeps them free of a link-time dependency between units and free of an
// ODR obligation: none of them is large enough for the duplication to matter,
// and the compiler discards the ones a unit does not call.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MSIL_MSILWRITERSHARED_H
#define LLVM_LIB_TARGET_MSIL_MSILWRITERSHARED_H

#include "MSILWriter.h"
#include "MSILWideIntegers.h"
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

// The command-line options. Defined once, in MSILWriter.cpp and read by every
// unit: a module is emitted by all of them at once, so the option set is
// module-wide state rather than a per-unit setting.
namespace cl = llvm::cl;
extern cl::opt<std::string> ExternClassName;
extern cl::opt<bool> LibraryMode;
extern cl::opt<std::string> AssemblyName;
extern cl::opt<std::string> LibcSpec;
extern cl::opt<std::string> LibcxxSpec;
extern cl::opt<std::string> AllocSpec;
extern cl::opt<bool> MathBind;
extern cl::list<std::string> LibMapSpec;
extern cl::list<std::string> LibSpec;
extern cl::opt<std::string> Fp80Spec;
extern cl::opt<std::string> PointerSpec;

/// Whether the build embedded a portable C++ runtime (libcxx/cxxrt.cpp). The
/// IR is included only by MSILWriterRuntime.cpp, the unit that links it; this
/// asks the same question the same way, so the option that needs it can be
/// rejected before any emission starts.
#if __has_include("cxxrt-win.inc")
inline constexpr bool HavePortableCxxRt = true;
#else
inline constexpr bool HavePortableCxxRt = false;
#endif

/// The alignment every allocation the emitted helpers hand out guarantees.
inline constexpr uint64_t MallocAlign = 16;

namespace llvm {

/// Report a construct the emitter cannot express and stop, without a crash.
///
/// report_fatal_error's default aborts, and the Windows CRT answers abort()
/// with a modal dialog that blocks a batch run until someone clicks it. It also
/// prints a stack dump that says nothing about the module. Passing
/// GenCrashDiag=false keeps the `LLVM ERROR:` line -- which is the whole
/// diagnostic -- and exits with status 1 instead.
///
/// Every rejection in the emitter goes through here so the behaviour is one
/// decision, not thirty-five.
[[noreturn]] inline void fatalError(const Twine &Msg) {
  report_fatal_error(Msg, /*GenCrashDiag=*/false);
}

/// Whether -msil-fp80 asked for the lossy binary64 mapping.
bool fp80MappedToDouble();

/// Whether -msil-pointers=i64 keeps every pointer in an int64 slot.
bool pointersAsInt64();

namespace {

template <typename T> std::string str(const T *V) {
  std::string S;
  raw_string_ostream OS(S);
  V->print(OS);
  return S;
}

// A 128-bit integer is a .NET value type from System.Runtime, not a primitive:
// there is no IL arithmetic for it, so every operator below becomes a call.
const char *const Int128Ty = "valuetype [System.Runtime]System.Int128";
const char *const UInt128Ty = "valuetype [System.Runtime]System.UInt128";
// The same types without the `valuetype` keyword, for `Type::Method` after a
// call's return type.
const char *const Int128Ref = "[System.Runtime]System.Int128";
const char *const UInt128Ref = "[System.Runtime]System.UInt128";

const char *const CxxRuntimeAssembly = "MSIL.Runtime";
const char *const CxxAbiTypeName = "MSIL.Runtime.CxxAbi";
const char *const CxxAbiRef = "[MSIL.Runtime]MSIL.Runtime.CxxAbi";
const char *const CxxExceptionRef = "[MSIL.Runtime]MSIL.Runtime.CxxException";
const char *const CxxExceptionTy = "class [MSIL.Runtime]MSIL.Runtime.CxxException";
const char *const ExceptionLocal = "$exn";

std::string asILComment(const Instruction &I) {
  std::string Text;
  raw_string_ostream(Text) << I;
  std::string Comment = "// ";
  for (char C : Text) {
    Comment += C;
    if (C == '\n')
      Comment += "// ";
  }
  return Comment;
}

bool isWideInt(Type *Ty) { return Ty->isIntegerTy(128); }

/// Types that move with ldobj/stobj rather than ldind/stind.
bool movesAsBlob(Type *Ty) {
  return Ty->isAggregateType() || Ty->isVectorTy() || isWideInt(Ty);
}

/// The value a floating-point constant is emitted as. With -msil-fp80 an
/// x86_fp80 becomes binary64, so the rounding has to happen here, once: `ldc.r8`
/// would otherwise read the 80-bit decimal and round it again, and the NaN and
/// infinity bit-pattern path would write ten bytes where eight are expected.
APFloat emittedFloat(const APFloat &V) {
  // Semantics are compared by address everywhere in LLVM, since fltSemantics
  // has no equality operator.
  if (&V.getSemantics() != &APFloat::x87DoubleExtended() ||
      !fp80MappedToDouble())
    return V;
  APFloat D = V;
  bool LosesInfo;
  D.convert(APFloat::IEEEdouble(), APFloat::rmNearestTiesToEven, &LosesInfo);
  return D;
}

/// An intrinsic that inspects a float's bits works on a double when the type
/// is a mapped x86_fp80: bitcasting the IR i80 would name an integer width IL
/// has no type for. These two convert in and out, and cost nothing in the
/// emitted IL because both slots are float64.
Value *narrowF80(Value *X, IRBuilder<> &B) {
  return X->getType()->isX86_FP80Ty() && fp80MappedToDouble()
             ? B.CreateFPTrunc(X, B.getDoubleTy())
             : X;
}
Value *widenF80(Value *V, Type *FTy, IRBuilder<> &B) {
  return FTy->isX86_FP80Ty() && fp80MappedToDouble() ? B.CreateFPExt(V, FTy) : V;
}

/// Operators that are an IL opcode for scalars but a method call for i128.
bool isWideBinaryOpcode(unsigned Op) {
  switch (Op) {
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
    return true;
  default:
    return false;
  }
}

/// The stdio names the portable library defines. A front end that saw only a
/// prototype emits a wrapper definition for these; the wrapper is meant for
/// the CRT and has to be dropped when the portable implementation is linked.
bool isPortableStdioName(StringRef N) {
  static const char *const Names[] = {
      "printf",  "vprintf",  "sprintf",  "snprintf",
      "vsprintf", "vsnprintf", "puts",   "putchar",
  };
  return is_contained(Names, N);
}

/// The symbols the portable C library (libc/*.c) defines. A declaration with
/// one of these names is satisfied by the link instead of a pinvoke.
bool isPortableLibcSymbol(StringRef N) {
  static const char *const Names[] = {
      // ctype.h
      "isalnum", "isalpha", "isascii", "isblank", "iscntrl", "isdigit",
      "isgraph", "islower", "isprint", "ispunct", "isspace", "isupper",
      "isxdigit", "toascii", "tolower", "toupper",
      // string.h additions
      "strnlen", "strncat", "strspn", "strcspn", "strpbrk", "strtok",
      "strtok_r", "strcasecmp", "strncasecmp", "memchr", "memrchr",
      "strcoll", "strxfrm",
      "memccpy", "mempcpy", "stpcpy", "stpncpy", "strsep", "strcasestr",
      "strndup", "strchrnul", "explicit_bzero",
      // stdlib.h
      "atoi", "atol", "atoll", "strtol", "strtoul", "strtoll", "strtoull",
      "strtoimax", "strtoumax", "abs", "labs", "llabs", "div", "ldiv",
      "lldiv", "imaxdiv", "imaxabs", "ffs", "ffsl", "ffsll", "qsort",
      "bsearch", "rand", "srand",
      // stdio.h
      "printf", "vprintf", "sprintf", "snprintf", "vsprintf", "vsnprintf",
      "puts", "putchar", "fputc", "fputs", "fwrite", "strerror",
  };
  return is_contained(Names, N);
}

constexpr unsigned kVecBytes = 32;

const char *const VecNs = "[System.Numerics.Vectors]System.Numerics.Vector`1";

StringRef memSuffixOf(Type *Ty) {
  if (Ty->isPointerTy())
    // A pointer in an int64 slot is loaded/stored as an int64; only the
    // default void* representation has the native-int `i` suffix.
    return pointersAsInt64() ? "i8" : "i";
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID: {
    // only widths whose store size is exactly the CLR access, or a store
    // would write past the value
    unsigned N = Ty->getIntegerBitWidth();
    if (N <= 8)
      return "i1";
    if (N <= 16)
      return "i2";
    if (N > 24 && N <= 32)
      return "i4";
    if (N > 56 && N <= 64)
      return "i8";
    break;
  }
  case Type::FloatTyID:
    return "r4";
  case Type::DoubleTyID:
    return "r8";
  case Type::X86_FP80TyID:
    if (fp80MappedToDouble())
      return "r8";
    break;
  default:
    break;
  }
  report_fatal_error("MSIL: unsupported memory type " + Twine(str(Ty)));
}

/// Instructions that normalize an int on the stack to \p Ty's declared width.
const char *widthConv(Type *Ty) {
  unsigned N = Ty->getIntegerBitWidth();
  // i1 has no conv that isolates one bit: conv.u1 keeps the low eight, so a
  // `trunc i64 %j to i1` would leave 2 as 2 and every `j & 1` value would test
  // true on odd rows. Masking is the only form that produces 0 or 1.
  if (N == 1)
    return "ldc.i4.1\n\tand";
  if (N == 8)
    return "conv.u1";
  if (N == 16)
    return "conv.u2";
  if (N > 32)
    return "conv.i8";
  return "conv.i4";
}

static bool isLaneConstant(const Constant *C) {
  if (isa<ConstantVector>(C) || isa<ConstantDataVector>(C))
    return true;
  return C->getType()->isVectorTy() && (isa<ConstantInt>(C) || isa<ConstantFP>(C));
}

/// Emit a call to a static Vector<T> helper: `E` names the element type, and
/// the parameter list is spelled with the method's own !!0 as ilasm requires.
static void emitVecCall(raw_ostream &OS, StringRef E, const char *Name,
                        const std::string &Params) {
  StringRef NV = "[System.Numerics.Vectors]System.Numerics.Vector`1";
  OS << "\tcall\tvaluetype " << NV << "<!!0> "
     << "[System.Numerics.Vectors]System.Numerics.Vector::" << Name << "<" << E
     << ">(" << Params << ")\n";
}

static bool isIntMinMax(AtomicRMWInst::BinOp Op) {
  return Op == AtomicRMWInst::Min || Op == AtomicRMWInst::Max ||
         Op == AtomicRMWInst::UMin || Op == AtomicRMWInst::UMax;
}

static bool isAbiTypeInfoVTable(const GlobalVariable &G) {
  return !G.isDeclaration() && G.getName().starts_with("_ZTVN10__cxxabiv1");
}

/// Intrinsics the emitter discards: a debugger or allocator cannot use them
/// here, and they carry no value.
// asm("" ...) is only a compiler barrier, which test harnesses and benchmarks
// use to keep a value alive; "+r"(x) hands x back unchanged
bool lowerEmptyInlineAsm(CallInst *CI) {
  auto *Asm = dyn_cast<InlineAsm>(CI->getCalledOperand());
  if (!Asm || !StringRef(Asm->getAsmString()).trim().empty())
    return false;
  if (!CI->getType()->isVoidTy()) {
    auto Same = find_if(CI->args(), [CI](const Use &Arg) {
      return Arg->getType() == CI->getType();
    });
    if (Same == CI->arg_end())
      return false;
    CI->replaceAllUsesWith(Same->get());
  }
  CI->eraseFromParent();
  return true;
}

bool isDiscardableIntrinsic(Intrinsic::ID ID) {
  return ID == Intrinsic::lifetime_start || ID == Intrinsic::lifetime_end ||
         ID == Intrinsic::dbg_declare || ID == Intrinsic::dbg_value ||
         ID == Intrinsic::dbg_label;
}


// the doubled-width check would need i256 once the operands are i128
static Value *addSubOverflowed(IRBuilder<> &B, bool Signed, bool Sub,
                               Value *A, Value *C, Value *Res) {
  if (!Signed)
    return Sub ? B.CreateICmpULT(A, C) : B.CreateICmpULT(Res, A);
  Value *Zero = Constant::getNullValue(A->getType());
  Value *ResLeftA = B.CreateXor(A, Res);
  Value *SecondCondition = Sub ? B.CreateXor(A, C) : B.CreateXor(C, Res);
  Value *SignBitSet = B.CreateAnd(ResLeftA, SecondCondition);
  return B.CreateICmpSLT(SignBitSet, Zero);
}

static Value *mulOverflowed(IRBuilder<> &B, bool Signed, Value *A, Value *C,
                            Value *Res) {
  Type *Ty = A->getType();
  unsigned N = Ty->getIntegerBitWidth();
  Value *Zero = ConstantInt::get(Ty, 0);
  Value *One = ConstantInt::get(Ty, 1);
  Value *AIsZero = B.CreateICmpEQ(A, Zero);
  if (!Signed) {
    Value *Divisor = B.CreateSelect(AIsZero, One, A);
    Value *Quotient = B.CreateUDiv(Res, Divisor);
    Value *Mismatch = B.CreateICmpNE(Quotient, C);
    return B.CreateAnd(B.CreateNot(AIsZero), Mismatch);
  }
  // MIN / -1 traps, so -1 is checked on its own
  Value *MinusOne = ConstantInt::getAllOnesValue(Ty);
  Value *Min = ConstantInt::get(Ty, APInt::getSignedMinValue(N));
  Value *AIsMinusOne = B.CreateICmpEQ(A, MinusOne);
  Value *Trivial = B.CreateOr(AIsZero, AIsMinusOne);
  Value *Divisor = B.CreateSelect(Trivial, One, A);
  Value *Quotient = B.CreateSDiv(Res, Divisor);
  Value *Mismatch = B.CreateICmpNE(Quotient, C);
  Value *MinusOneOverflow = B.CreateICmpEQ(C, Min);
  Value *General = B.CreateAnd(B.CreateNot(Trivial), Mismatch);
  Value *ByMinusOne = B.CreateAnd(AIsMinusOne, MinusOneOverflow);
  return B.CreateOr(General, ByMinusOne);
}

static Value *saturatingAddSub(IRBuilder<> &B, bool Signed, bool Sub,
                               Value *A, Value *C) {
  Type *Ty = A->getType();
  unsigned N = Ty->getIntegerBitWidth();
  Value *Res = Sub ? B.CreateSub(A, C) : B.CreateAdd(A, C);
  Value *Overflow = addSubOverflowed(B, Signed, Sub, A, C, Res);
  Value *Limit = nullptr;
  if (Signed) {
    Value *Zero = ConstantInt::get(Ty, 0);
    Value *Min = ConstantInt::get(Ty, APInt::getSignedMinValue(N));
    Value *Max = ConstantInt::get(Ty, APInt::getSignedMaxValue(N));
    Limit = B.CreateSelect(B.CreateICmpSLT(A, Zero), Min, Max);
  } else {
    Limit = Sub ? ConstantInt::get(Ty, 0) : ConstantInt::getAllOnesValue(Ty);
  }
  return B.CreateSelect(Overflow, Limit, Res);
}


/// The set the built-in IL libc implements, and that libc/*.c deliberately
/// leaves to IL: these must bind to $MSIL_Libc rather than a linked C copy.
bool isILLibcSymbol(StringRef N) {
  static const char *const Names[] = {
      "malloc",  "free",    "calloc",  "realloc", "memcpy", "memmove",
      "memset",  "memcmp",  "strlen",  "strcmp",  "strncmp", "strcpy",
      "strncpy", "strcat",  "strchr",  "strrchr", "strstr", "strdup",
  };
  return is_contained(Names, N);
}

/// The set the built-in libc implements; a symbol outside it keeps its pinvoke.
bool isBuiltinSymbol(StringRef N) {
  if (isILLibcSymbol(N))
    return true;
  // The portable C library covers the string, character, conversion and stdio
  // surface too. Anything it defines is bound to the linked copy; the check is
  // a name list because a symbol has to be classified before the link happens.
  return isPortableLibcSymbol(N);
}

/// True when \p Ty is or contains an x86_fp80.
static bool mentionsF80(Type *Ty) {
  if (Ty->isX86_FP80Ty())
    return true;
  if (auto *FTy = dyn_cast<FunctionType>(Ty)) {
    if (mentionsF80(FTy->getReturnType()))
      return true;
    for (Type *P : FTy->params())
      if (mentionsF80(P))
        return true;
    return false;
  }
  if (auto *STy = dyn_cast<StructType>(Ty))
    for (Type *E : STy->elements())
      if (mentionsF80(E))
        return true;
  if (auto *ATy = dyn_cast<ArrayType>(Ty))
    return mentionsF80(ATy->getElementType());
  return false;
}

static Value *convertIntegerOperand(IRBuilder<> &B, Value *V, Type *To) {
  if (V->getType() == To)
    return V;
  if (To->isIntegerTy(1))
    return B.CreateICmpNE(V, Constant::getNullValue(V->getType()));
  return B.CreateZExtOrTrunc(V, To);
}

static bool onlyIntegerWidthsDiffer(FunctionType *Site, FunctionType *Callee) {
  if (Site->getNumParams() != Callee->getNumParams() ||
      Site->isVarArg() != Callee->isVarArg())
    return false;
  auto Compatible = [](Type *A, Type *B) {
    return A == B || (A->isIntegerTy() && B->isIntegerTy());
  };
  if (!Compatible(Site->getReturnType(), Callee->getReturnType()))
    return false;
  for (unsigned I = 0, E = Site->getNumParams(); I != E; ++I)
    if (!Compatible(Site->getParamType(I), Callee->getParamType(I)))
      return false;
  return true;
}

// clang declares the generic __atomic_* libcalls returning bool, while
// compiler-rt defines them returning int. a method is called by its exact
// signature, so the call has to use the definition's
static void matchCallsToDefinitions(Function &F) {
  SmallVector<CallInst *, 4> Mismatched;
  for (Instruction &I : instructions(F)) {
    auto *Call = dyn_cast<CallInst>(&I);
    if (!Call)
      continue;
    auto *Callee = dyn_cast<Function>(Call->getCalledOperand());
    if (!Callee || Callee->isDeclaration())
      continue;
    FunctionType *SiteTy = Call->getFunctionType();
    FunctionType *CalleeTy = Callee->getFunctionType();
    if (SiteTy != CalleeTy && onlyIntegerWidthsDiffer(SiteTy, CalleeTy))
      Mismatched.push_back(Call);
  }
  for (CallInst *Call : Mismatched) {
    auto *Callee = cast<Function>(Call->getCalledOperand());
    FunctionType *CalleeTy = Callee->getFunctionType();
    IRBuilder<> B(Call);
    SmallVector<Value *, 8> Args;
    for (unsigned I = 0, E = Call->arg_size(); I != E; ++I)
      Args.push_back(convertIntegerOperand(B, Call->getArgOperand(I),
                                           CalleeTy->getParamType(I)));
    CallInst *Matched = B.CreateCall(CalleeTy, Callee, Args);
    Matched->setCallingConv(Call->getCallingConv());
    Value *Result = convertIntegerOperand(B, Matched, Call->getType());
    if (!Call->getType()->isVoidTy())
      Call->replaceAllUsesWith(Result);
    Call->eraseFromParent();
  }
}

// ilasm rejects a name over 1023 characters, and deeply nested templates get
// there. The hash keeps it the same in every module that defines or imports it.
static constexpr size_t MaxILNameLength = 1000;

static void shortenLongName(GlobalValue &GV) {
  StringRef Name = GV.getName();
  if (GV.isDeclaration() || Name.size() <= MaxILNameLength)
    return;
  std::string Hash = utohexstr(xxh3_64bits(Name), true, 16);
  std::string Short =
      (Name.take_front(MaxILNameLength - Hash.size() - 1) + "$" + Hash).str();
  GV.setName(Short);
}

static void shortenLongNames(Module &M) {
  for (Function &F : M)
    shortenLongName(F);
  for (GlobalVariable &G : M.globals())
    shortenLongName(G);
}


} // namespace

} // namespace llvm

#endif // LLVM_LIB_TARGET_MSIL_MSILWRITERSHARED_H
