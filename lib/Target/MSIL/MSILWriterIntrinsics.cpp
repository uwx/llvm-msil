//===-- MSILWriterIntrinsics.cpp - Intrinsic lowering ----------------------===//
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
// Intrinsic lowering
//===----------------------------------------------------------------------===//

// IntrinsicLowering only knows a handful of libm functions, and it resolves
// the ones it does know to a bare CRT call. Rewriting every math intrinsic to
// the C symbol here means one binding decision later, in getManagedTarget,
// rather than two spellings that can disagree.
bool MSILWriter::lowerLibmIntrinsic(CallInst *CI) {
  // float spelling, double spelling
  const char *F32 = nullptr, *F64 = nullptr;
  switch (CI->getCalledFunction()->getIntrinsicID()) {
  case Intrinsic::sqrt:      F32 = "sqrtf";      F64 = "sqrt";      break;
  case Intrinsic::sin:       F32 = "sinf";       F64 = "sin";       break;
  case Intrinsic::cos:       F32 = "cosf";       F64 = "cos";       break;
  case Intrinsic::tan:       F32 = "tanf";       F64 = "tan";       break;
  case Intrinsic::exp:       F32 = "expf";       F64 = "exp";       break;
  case Intrinsic::log:       F32 = "logf";       F64 = "log";       break;
  case Intrinsic::log2:      F32 = "log2f";      F64 = "log2";      break;
  case Intrinsic::log10:     F32 = "log10f";     F64 = "log10";     break;
  case Intrinsic::exp2:      F32 = "exp2f";      F64 = "exp2";      break;
  case Intrinsic::exp10:     F32 = "exp10f";     F64 = "exp10";     break;
  case Intrinsic::floor:     F32 = "floorf";     F64 = "floor";     break;
  case Intrinsic::ceil:      F32 = "ceilf";      F64 = "ceil";      break;
  case Intrinsic::trunc:     F32 = "truncf";     F64 = "trunc";     break;
  case Intrinsic::round:     F32 = "roundf";     F64 = "round";     break;
  case Intrinsic::roundeven: F32 = "roundevenf"; F64 = "roundeven"; break;
  case Intrinsic::rint:      F32 = "rintf";      F64 = "rint";      break;
  case Intrinsic::nearbyint: F32 = "nearbyintf"; F64 = "nearbyint"; break;
  case Intrinsic::sinh:      F32 = "sinhf";      F64 = "sinh";      break;
  case Intrinsic::cosh:      F32 = "coshf";      F64 = "cosh";      break;
  case Intrinsic::tanh:      F32 = "tanhf";      F64 = "tanh";      break;
  case Intrinsic::asin:      F32 = "asinf";      F64 = "asin";      break;
  case Intrinsic::acos:      F32 = "acosf";      F64 = "acos";      break;
  case Intrinsic::atan:      F32 = "atanf";      F64 = "atan";      break;
  case Intrinsic::pow:       F32 = "powf";       F64 = "pow";       break;
  case Intrinsic::fabs:      F32 = "fabsf";      F64 = "fabs";      break;
  case Intrinsic::lround:    F32 = "lroundf";    F64 = "lround";    break;
  case Intrinsic::llround:   F32 = "llroundf";   F64 = "llround";   break;
  case Intrinsic::lrint:     F32 = "lrintf";     F64 = "lrint";     break;
  case Intrinsic::llrint:    F32 = "llrintf";    F64 = "llrint";    break;
  default:
    return false;
  }
  Type *RetTy = CI->getType();
  Type *ArgTy = CI->getArgOperand(0)->getType();
  // Under -msil-fp80 an x86_fp80 is a binary64 value, so it must take the
  // double spelling. IntrinsicLowering would otherwise pick the `l` one
  // (`sqrtl`), whose SysV ABI passes an 80-bit x87 value the CLR cannot
  // produce.
  bool IsF80 = ArgTy->isX86_FP80Ty() && fp80MappedToDouble();
  if (!ArgTy->isFloatTy() && !ArgTy->isDoubleTy() && !IsF80)
    return false;
  const char *Name = ArgTy->isFloatTy() ? F32 : F64;
  // The C symbol takes the intrinsic's own operands, which is one argument
  // for the unary functions and two for pow.
  SmallVector<Type *, 2> ParamTys;
  for (Value *A : CI->args())
    ParamTys.push_back(A->getType());
  Module *M = CI->getModule();
  FunctionCallee FC = M->getOrInsertFunction(
      Name, FunctionType::get(RetTy, ParamTys, false));
  IRBuilder<> B(CI);
  Value *New = B.CreateCall(FC, SmallVector<Value *, 2>(CI->args()));
  CI->replaceAllUsesWith(New);
  CI->eraseFromParent();
  return true;
}

bool MSILWriter::lowerExpandedIntrinsic(CallInst *CI) {
  Intrinsic::ID ID = CI->getCalledFunction()->getIntrinsicID();
  IRBuilder<> B(CI);
  Type *Ty = CI->getType();
  switch (ID) {
  case Intrinsic::minnum:
  case Intrinsic::maxnum:
  case Intrinsic::minimum:
  case Intrinsic::maximum:
  case Intrinsic::minimumnum:
  case Intrinsic::maximumnum: {
    // The IEEE 754 min/max family. fmin/fmax have exactly the semantics of
    // `num` (return the non-NaN operand, NaN only if both are NaN) and they
    // already order -0.0 below +0.0, which is what the langref asks for, so
    // the two num forms are the libm call outright.
    Value *A = CI->getArgOperand(0), *C = CI->getArgOperand(1);
    bool IsMin = ID == Intrinsic::minnum || ID == Intrinsic::minimum ||
                 ID == Intrinsic::minimumnum;
    if (ID == Intrinsic::minnum || ID == Intrinsic::maxnum) {
      const char *Name = Ty->isFloatTy() ? (IsMin ? "fminf" : "fmaxf")
                                         : (IsMin ? "fmin" : "fmax");
      FunctionCallee FC = CI->getModule()->getOrInsertFunction(
          Name, FunctionType::get(Ty, {Ty, Ty}, false));
      CI->replaceAllUsesWith(B.CreateCall(FC, {A, C}));
      CI->eraseFromParent();
      return true;
    }
    // minimum/maximum propagate a NaN operand and minimumnum/maximumnum
    // return the non-NaN one; both order -0.0 below +0.0. The ordered
    // compare selects the right operand except when it is the NaN, and the
    // unordered test picks the other operand in that case. For the
    // propagating pair that other operand is still a NaN, so the same code
    // works for both.
    Value *Cmp = IsMin ? B.CreateFCmpOLT(A, C) : B.CreateFCmpOGT(A, C);
    Value *Pick = B.CreateSelect(Cmp, A, C);
    Value *Other = B.CreateSelect(Cmp, C, A);
    CI->replaceAllUsesWith(
        B.CreateSelect(B.CreateFCmpUNO(Pick, Pick), Other, Pick));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::powi: {
    // Math.Pow takes a double exponent, so an integer one becomes a convert
    // and one call. Unlike a square-and-multiply loop this keeps the libm
    // accuracy the intrinsic promises.
    Value *Base = CI->getArgOperand(0);
    Value *Exp = CI->getArgOperand(1);
    auto *ITy = dyn_cast<IntegerType>(Exp->getType());
    if (!ITy)
      return false;
    const char *Name = Ty->isFloatTy() ? "powf" : "pow";
    FunctionCallee FC = CI->getModule()->getOrInsertFunction(
        Name, FunctionType::get(Ty, {Ty, Ty}, false));
    CI->replaceAllUsesWith(
        B.CreateCall(FC, {Base, B.CreateSIToFP(Exp, Ty)}));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::ldexp: {
    // x * 2^n is Math.ScaleB, which has exactly that definition.
    Value *X = CI->getArgOperand(0), *E = CI->getArgOperand(1);
    const char *Name = Ty->isFloatTy() ? "scalbnf" : "scalbn";
    FunctionCallee FC = CI->getModule()->getOrInsertFunction(
        Name, FunctionType::get(Ty, {Ty, E->getType()}, false));
    CI->replaceAllUsesWith(B.CreateCall(FC, {X, E}));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::frexp:
  case Intrinsic::modf: {
    // both return a pair the C spelling hands back through a pointer. a float
    // goes through the double function, which is exact for both, since msvcrt
    // has no frexpf
    Value *X = CI->getArgOperand(0);
    Type *FTy = X->getType();
    if (!FTy->isFloatTy() && !FTy->isDoubleTy())
      return false;
    bool IsFrexp = ID == Intrinsic::frexp;
    auto *PairTy = cast<StructType>(Ty);
    Type *DoubleTy = B.getDoubleTy();
    Type *OutTy = IsFrexp ? PairTy->getElementType(1) : DoubleTy;
    BasicBlock &Entry = CI->getFunction()->getEntryBlock();
    IRBuilder<> EntryB(&Entry, Entry.getFirstInsertionPt());
    AllocaInst *Out = EntryB.CreateAlloca(OutTy);
    FunctionCallee FC = CI->getModule()->getOrInsertFunction(
        IsFrexp ? "frexp" : "modf",
        FunctionType::get(DoubleTy, {DoubleTy, B.getPtrTy()}, false));
    Value *Wide = B.CreateFPExt(X, DoubleTy);
    Value *First = B.CreateFPTrunc(B.CreateCall(FC, {Wide, Out}), FTy);
    Value *Second = B.CreateLoad(OutTy, Out);
    if (!IsFrexp)
      Second = B.CreateFPTrunc(Second, FTy);
    Value *Pair = B.CreateInsertValue(PoisonValue::get(PairTy), First, 0);
    Pair = B.CreateInsertValue(Pair, Second, 1);
    CI->replaceAllUsesWith(Pair);
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::fptosi_sat:
  case Intrinsic::fptoui_sat: {
    // Convert, then clamp to the target's range: the compare has to happen in
    // floating point, since converting an out-of-range value is poison.
    Value *X = CI->getArgOperand(0);
    bool Signed = ID == Intrinsic::fptosi_sat;
    auto *DstITy = cast<IntegerType>(Ty);
    unsigned N = DstITy->getBitWidth();
    APInt Lo = Signed ? APInt::getSignedMinValue(N) : APInt::getMinValue(N);
    APInt Hi = Signed ? APInt::getSignedMaxValue(N) : APInt::getMaxValue(N);
    Value *FLo = ConstantFP::get(X->getType(), Signed ? (double)Lo.getSExtValue()
                                                     : (double)Lo.getZExtValue());
    Value *FHi = ConstantFP::get(X->getType(), Signed ? (double)Hi.getSExtValue()
                                                     : (double)Hi.getZExtValue());
    Value *LoC = ConstantInt::get(Ty, Lo);
    Value *HiC = ConstantInt::get(Ty, Hi);
    // The convert is poison below Lo and above Hi, so guard it with the
    // clamped range: -inf maps to Lo, +inf to Hi.
    Value *Below = Signed ? B.CreateFCmpOLE(X, FLo) : B.CreateFCmpOLE(X, FLo);
    Value *Above = B.CreateFCmpOGE(X, FHi);
    Value *Clamped = B.CreateSelect(
        Below, FLo, B.CreateSelect(Above, FHi, X));
    Value *Conv = Signed ? B.CreateFPToSI(Clamped, Ty)
                         : B.CreateFPToUI(Clamped, Ty);
    // The end points have to be exact, not the clamped float: Lo is
    // representable but Hi may round up, so select them directly.
    CI->replaceAllUsesWith(B.CreateSelect(
        Below, LoC, B.CreateSelect(Above, HiC, Conv)));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::scmp:
  case Intrinsic::ucmp: {
    // Return -1, 0 or 1 as the sign of the comparison result.
    Value *A = CI->getArgOperand(0), *C = CI->getArgOperand(1);
    bool Signed = ID == Intrinsic::scmp;
    Value *Lt = Signed ? B.CreateICmpSLT(A, C) : B.CreateICmpULT(A, C);
    Value *Gt = Signed ? B.CreateICmpSGT(A, C) : B.CreateICmpUGT(A, C);
    CI->replaceAllUsesWith(B.CreateSelect(
        Lt, ConstantInt::get(Ty, -1),
        B.CreateSelect(Gt, ConstantInt::get(Ty, 1), ConstantInt::get(Ty, 0))));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::smul_fix:
  case Intrinsic::umul_fix:
  case Intrinsic::smul_fix_sat:
  case Intrinsic::umul_fix_sat: {
    // Fixed-point multiply: the product is scaled back down by 2^Scale, which
    // is a right shift of the wide result. The saturating forms clamp; the
    // plain forms truncate the same way a native multiply would.
    Value *A = CI->getArgOperand(0), *C = CI->getArgOperand(1);
    auto *Scale = dyn_cast<ConstantInt>(CI->getArgOperand(2));
    if (!Scale)
      return false;
    unsigned N = cast<IntegerType>(Ty)->getBitWidth();
    bool Signed = ID == Intrinsic::smul_fix || ID == Intrinsic::smul_fix_sat;
    bool Saturating = ID == Intrinsic::smul_fix_sat ||
                      ID == Intrinsic::umul_fix_sat;
    Type *Wide = B.getIntNTy(N * 2);
    Value *WA = Signed ? B.CreateSExt(A, Wide) : B.CreateZExt(A, Wide);
    Value *WC = Signed ? B.CreateSExt(C, Wide) : B.CreateZExt(C, Wide);
    Value *W = B.CreateMul(WA, WC);
    Value *Shifted = Signed
                         ? B.CreateAShr(W, Scale->getZExtValue())
                         : B.CreateLShr(W, Scale->getZExtValue());
    if (!Saturating) {
      CI->replaceAllUsesWith(B.CreateTrunc(Shifted, Ty));
      CI->eraseFromParent();
      return true;
    }
    APInt Lo = Signed ? APInt::getSignedMinValue(N) : APInt::getMinValue(N);
    APInt Hi = Signed ? APInt::getSignedMaxValue(N) : APInt::getMaxValue(N);
    Value *WLo = Signed ? B.CreateSExt(ConstantInt::get(Ty, Lo), Wide)
                        : B.CreateZExt(ConstantInt::get(Ty, Lo), Wide);
    Value *WHi = Signed ? B.CreateSExt(ConstantInt::get(Ty, Hi), Wide)
                        : B.CreateZExt(ConstantInt::get(Ty, Hi), Wide);
    Value *Lt = Signed ? B.CreateICmpSLT(Shifted, WLo)
                       : B.CreateICmpULT(Shifted, WLo);
    Value *Gt = Signed ? B.CreateICmpSGT(Shifted, WHi)
                       : B.CreateICmpUGT(Shifted, WHi);
    CI->replaceAllUsesWith(B.CreateSelect(
        Lt, ConstantInt::get(Ty, Lo),
        B.CreateSelect(Gt, ConstantInt::get(Ty, Hi),
                       B.CreateTrunc(Shifted, Ty))));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::ptrmask:
  case Intrinsic::launder_invariant_group:
#if LLVM_VERSION_MAJOR < 24
  case Intrinsic::strip_invariant_group:
#endif
    // A pointer's bits are its address, and the invariant-group markers only
    // exist to give the optimiser something to reason about. Masking the
    // address integer and converting back is the ptrmask definition, and the
    // markers are the identity.
    if (ID == Intrinsic::ptrmask) {
      Value *P = CI->getArgOperand(0);
      Type *IT = B.getIntPtrTy(Mod->getDataLayout());
      CI->replaceAllUsesWith(B.CreateIntToPtr(
          B.CreateAnd(B.CreatePtrToInt(P, IT), CI->getArgOperand(1)),
          P->getType()));
    } else {
      CI->replaceAllUsesWith(CI->getArgOperand(0));
    }
    CI->eraseFromParent();
    return true;

  case Intrinsic::debugtrap:
  case Intrinsic::donothing:
  case Intrinsic::sideeffect:
  case Intrinsic::assume:
  case Intrinsic::experimental_noalias_scope_decl:
    // No observable behaviour in managed code: the debugger has no trap
    // instruction to break on, and the rest are optimiser annotations.
    CI->eraseFromParent();
    return true;

  case Intrinsic::objectsize: {
    // A global's size is fixed at compile time; everything else (heap blocks,
    // unmanaged memory) has no size query here, and -1 is the "unknown"
    // answer the intrinsic already defines.
    Value *P = CI->getArgOperand(0);
    uint64_t Size = 0;
    bool Known = false;
    if (auto *GV = dyn_cast<GlobalVariable>(P->stripPointerCasts()))
      if (GV->hasInitializer()) {
        Size = Mod->getDataLayout().getTypeAllocSize(GV->getValueType());
        Known = true;
      }
    CI->replaceAllUsesWith(ConstantInt::get(
        Ty, Known ? APInt(Ty->getIntegerBitWidth(), Size) : APInt::getAllOnes(
                                                                 Ty->getIntegerBitWidth())));
    CI->eraseFromParent();
    return true;
  }
  case Intrinsic::is_fpclass: {
    // The class bits follow the langref: 0 snan, 1 qnan, 2 ninf, 3 nnormal,
    // 4 nsubnormal, 5 nzero, 6 pzero, 7 psubnormal, 8 pnormal, 9 pinf.
    // sNaN cannot be distinguished from qNaN without inspecting the
    // mantissa, so both NaN bits answer the same test.
    Value *X = narrowF80(CI->getArgOperand(0), B);
    Type *FTy = X->getType();
    APInt M = cast<ConstantInt>(CI->getArgOperand(1))->getValue();
    auto Has = [&](unsigned Bit) { return M[Bit]; };
    Value *False = ConstantInt::getFalse(CI->getContext());
    Value *True = ConstantInt::getTrue(CI->getContext());
    Value *R = False;
    Value *Zero = ConstantFP::get(FTy, 0.0);
    // A sign test that works for -0.0 too, where a compare would not.
    Value *SignBit = B.CreateBitCast(
        X, B.getIntNTy(FTy->getPrimitiveSizeInBits()));
    Value *Negative =
        B.CreateICmpSLT(SignBit, ConstantInt::get(SignBit->getType(), 0));
    auto Add = [&](Value *Cond) { R = B.CreateOr(R, B.CreateAnd(Cond, True)); };

    if (Has(0) || Has(1)) {
      Add(B.CreateFCmpUNO(X, X));
    }
    if (Has(2) || Has(3) || Has(4) || Has(5) || Has(6) || Has(7) || Has(8) ||
        Has(9)) {
      // fabs first so that the magnitude tests are sign-blind. Every ordered
      // compare against a NaN is false, so the NaN case drops out by itself.
      Value *AbsX = B.CreateUnaryIntrinsic(Intrinsic::fabs, X);
      Value *IsZero = B.CreateFCmpOEQ(AbsX, Zero);
      Value *IsInf = B.CreateFCmpOEQ(AbsX, ConstantFP::getInfinity(FTy));
      // The smallest normal is the boundary between normal and subnormal.
      double MinNormal = FTy->isFloatTy()
                             ? (double)std::numeric_limits<float>::min()
                             : std::numeric_limits<double>::min();
      Value *MinNorm = ConstantFP::get(FTy, MinNormal);
      Value *IsSub = B.CreateFCmpOLT(AbsX, MinNorm);
      Value *IsNormal = B.CreateAnd(B.CreateFCmpOGE(AbsX, MinNorm),
                                    B.CreateNot(IsInf));
      Value *NotNeg = B.CreateNot(Negative);
      if (Has(2)) Add(B.CreateAnd(IsInf, Negative));
      if (Has(3)) Add(B.CreateAnd(IsNormal, Negative));
      if (Has(4)) Add(B.CreateAnd(IsSub, Negative));
      if (Has(5)) Add(B.CreateAnd(IsZero, Negative));
      if (Has(6)) Add(B.CreateAnd(IsZero, NotNeg));
      if (Has(7)) Add(B.CreateAnd(IsSub, NotNeg));
      if (Has(8)) Add(B.CreateAnd(IsNormal, NotNeg));
      if (Has(9)) Add(B.CreateAnd(IsInf, NotNeg));
    }
    CI->replaceAllUsesWith(widenF80(R, CI->getType(), B));
    CI->eraseFromParent();
    return true;
  }
  default:
    return false;
  }
}

// A front end that saw only a prototype for printf emits a wrapper definition
// that reaches the CRT. The portable library supplies the real implementation,
// so the wrapper has to step aside -- and its callers have to be re-pointed at
// a declaration of the same name, or the linker drops the linked definition as
// unreachable and the call binds to nothing.
//
// This runs before collectPInvokes, which would otherwise mint a pinvoke thunk
// for the vararg call sites while they still refer to the wrapper.
void MSILWriter::dropStdioWrappers() {
  SmallVector<Function *, 8> Wrappers;
  for (Function &F : *Mod)
    if (!F.isDeclaration() && F.isVarArg() && isPortableStdioName(F.getName()))
      Wrappers.push_back(&F);
  for (Function *F : Wrappers) {
    FunctionType *FTy = F->getFunctionType();
    std::string Name = F->getName().str();
    // A declaration of the same name may already exist; if so it is the one
    // to point at, since creating another would be renamed `printf.1`.
    Function *Impl = nullptr;
    for (Function &G : Mod->functions())
      if (&G != F && G.isDeclaration() && G.getName() == Name) {
        Impl = &G;
        break;
      }
    if (!Impl) {
      // `printf.1` etc. are illegal IL identifiers, so the new declaration
      // must keep the exact name. The old one still owns it until it goes.
      F->setName(Name + "$wrapper");
      Impl = Function::Create(FTy, GlobalValue::ExternalLinkage, Name, Mod);
    }
    Impl->setAttributes(F->getAttributes());
    // The wrapper's own body also names it, so the uses have to be redirected
    // by hand after the definition is gone; replaceAllUsesWith would leave
    // the recursive reference behind.
    SmallVector<Use *, 8> Uses;
    for (Use &U : F->uses())
      Uses.push_back(&U);
    for (Use *U : Uses)
      U->set(Impl);
    F->deleteBody();
    F->eraseFromParent();
  }
}

// The prep passes in MSILTargetMachine run before this pass, so anything the
// portable library link added still has the constructs IL cannot express.
// Running the same subset again is idempotent for code that already went
// through it.
void MSILWriter::lowerLinkedFunctions() {
  if (!EmitPortable)
    return;
  legacy::FunctionPassManager FPM(Mod);
  FPM.add(createUnreachableBlockEliminationPass());
  FPM.add(createLowerSwitchPass());
  FPM.add(createCFGSimplificationPass());
  // CFGSimplify can rebuild a switch from LowerSwitch's comparison tree; the
  // writer cannot emit one, so lower once more after the cleanup.
  FPM.add(createLowerSwitchPass());
  FPM.add(createPromoteMemoryToRegisterPass());
  FPM.doInitialization();
  for (Function &F : *Mod)
    if (!F.isDeclaration())
      FPM.run(F);
  FPM.doFinalization();
}

void MSILWriter::lowerIntrinsics(Function &F) {
  // A vararg function whose body merely forwards to a CRT internal (e.g. the
  // `printf` wrapper clang emits from MSVC's stdio.h) cannot work: it passes a
  // va_list that only exists in the native ABI. Drop the definition so calls
  // bind to the real exported function through a pinvoke thunk instead.
  //
  // The forward can be two hops: `snprintf` wraps `vsnprintf`, which is the
  // definition that reaches `__stdio_common_vsprintf`. Both get dropped, so
  // both become declarations and calls bind to the exported functions. The
  // callee is matched by name, not linkage: the intermediate wrapper is still
  // a definition when the outer one is examined.
  if (F.isVarArg() && !F.isDeclaration()) {
    // The portable library defines these itself, so a front-end wrapper with
    // the same name must step aside or the link keeps the wrapper and the
    // real implementation is dead code. The wrapper only exists to reach the
    // CRT in a way va_arg cannot on this backend.
    // printf and friends are linkonce_odr wrappers, so the name is the only
    // reliable test: a user definition of one of these would be their own
    // implementation of the C library, which is what is being replaced.
    // The portable library defines these; see dropStdioWrappers, which runs
    // before this and has already re-pointed the calls.
    auto IsCRTEntry = [](const Function *Callee) {
      StringRef N = Callee->getName();
      if (N.starts_with("__stdio_common_") || N.starts_with("__acrt_"))
        return true;
      return N == "printf" || N == "vprintf" || N == "sprintf" ||
             N == "vsprintf" || N == "snprintf" || N == "vsnprintf" ||
             N == "_snprintf" || N == "_vsnprintf";
    };
    // Only a function that *is* the C library entry may be dropped. Testing
    // the callee alone was not enough: SDL's SDL_sscanf forwards to the same
    // __stdio_common_vsscanf that clang's printf wrapper reaches, but SDL_sscanf
    // is not a CRT symbol, so there is nothing for a pinvoke thunk to resolve
    // against. Dropping it left the thunks calling msvcrt!SDL_sscanf, which
    // throws; SDL's error handler then called it again, forever.
    //
    // A non-CRT vararg definition is emitted as a real method instead, with
    // va_start lowering to System.ArgIterator, so a call to it binds locally.
    bool IsCRTSymbol = IsCRTEntry(&F) ||
                       F.getName().starts_with("__stdio_common_") ||
                       F.getName().starts_with("__acrt_");
    bool ForwardsToCRT = false;
    if (IsCRTSymbol)
      for (BasicBlock &BB : F)
        for (Instruction &I : BB)
          if (auto *CB = dyn_cast<CallBase>(&I))
            if (auto *Callee = CB->getCalledFunction())
              if (IsCRTEntry(Callee))
                ForwardsToCRT = true;
    if (ForwardsToCRT) {
      F.deleteBody();
      F.setComdat(nullptr);
      return;
    }
  }
  // Lowering can introduce new intrinsic calls (scalarized vector lanes, and
  // IntrinsicLowering's libm stubs), so repeat until none but the directly
  // emitted ones are left. Terminate on "nothing left to rewrite", not on the
  // instruction count: scalarizing a vector narrows the count but grows the
  // function, so the count would stop the loop early and strand the scalars.
  for (;;) {
    for (BasicBlock &BB : F) {
      for (auto It = BB.begin(), End = BB.end(); It != End;) {
        Instruction *I = &*It++;
        auto *CI = dyn_cast<CallInst>(I);
        if (!CI)
          continue;
        if (lowerEmptyInlineAsm(CI))
          continue;
        // getCalledFunction returns null when the call's function type differs
        // from the callee's, which is what a vararg intrinsic call site or a
        // mismatched attribute group produces. The intrinsic is still named,
        // so fall back to the name for the ones the CLR cannot express.
        Function *Callee = CI->getCalledFunction();
        if (!Callee) {
          if (Function *F = dyn_cast<Function>(CI->getCalledOperand()))
            if (F->isIntrinsic() && isDiscardableIntrinsic(F->getIntrinsicID())) {
              CI->eraseFromParent();
              continue;
            }
          continue;
        }
        if (!Callee->isIntrinsic())
          continue;
        switch (Callee->getIntrinsicID()) {
        case Intrinsic::vastart:
        case Intrinsic::vaend:
        case Intrinsic::vacopy:
          break; // Emitted directly.
        case Intrinsic::lifetime_start:
        case Intrinsic::lifetime_end:
        case Intrinsic::dbg_declare:
        case Intrinsic::dbg_value:
        case Intrinsic::dbg_label:
          // Bookkeeping for a debugger and an allocator; the CLR has no
          // lifetime markers, and locals already live for the whole method.
          // A call-site attribute group (`#6` in the IR) makes the call carry
          // a different function type, so `getCalledFunction` no longer sees
          // the intrinsic; matching the name covers both spellings.
          CI->eraseFromParent();
          break;
        case Intrinsic::sqrt:
      case Intrinsic::sin:
      case Intrinsic::cos:
      case Intrinsic::tan:
      case Intrinsic::exp:
      case Intrinsic::exp2:
      case Intrinsic::exp10:
      case Intrinsic::log:
      case Intrinsic::log2:
      case Intrinsic::log10:
      case Intrinsic::sinh:
      case Intrinsic::cosh:
      case Intrinsic::tanh:
      case Intrinsic::asin:
      case Intrinsic::acos:
      case Intrinsic::atan:
      case Intrinsic::floor:
      case Intrinsic::ceil:
      case Intrinsic::trunc:
      case Intrinsic::round:
      case Intrinsic::roundeven:
      case Intrinsic::rint:
      case Intrinsic::nearbyint:
      case Intrinsic::pow:
      case Intrinsic::copysign:
      case Intrinsic::lround:
      case Intrinsic::llround:
      case Intrinsic::lrint:
      case Intrinsic::llrint: {
        // One C symbol per intrinsic, so the CRT-or-Math choice happens in
        // exactly one place (getManagedTarget). A vector lowers to one scalar
        // call per lane of the same intrinsic.
        auto *VTy = dyn_cast<VectorType>(CI->getType());
        if (!VTy) {
          if (!lowerLibmIntrinsic(CI))
            IL->LowerIntrinsicCall(CI);
          break;
        }
        IRBuilder<> B(CI);
        unsigned N = VTy->getElementCount().getFixedValue();
        Value *Acc = PoisonValue::get(VTy);
        for (unsigned I = 0; I < N; ++I) {
          std::vector<Value *> Args;
          std::vector<Type *> ArgTys;
          for (Value *A : CI->args()) {
            Args.push_back(B.CreateExtractElement(A, (uint64_t)I));
            ArgTys.push_back(A->getType()->getScalarType());
          }
          // The scalar overload, not the vector one the call names.
          Function *Scalar =
              Intrinsic::getOrInsertDeclaration(Mod, Callee->getIntrinsicID(), ArgTys);
          Value *Lane = B.CreateCall(Scalar, Args);
          Acc = B.CreateInsertElement(Acc, Lane, (uint64_t)I);
        }
        CI->replaceAllUsesWith(Acc);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::trap: {
        // No host trap; abort() stops the program the same way.
        IRBuilder<> B(CI);
        B.CreateCall(Mod->getOrInsertFunction(
            "abort", FunctionType::get(B.getVoidTy(), false)));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::sshl_sat:
      case Intrinsic::ushl_sat: {
        // Saturating shift: clamp when the shift overflows the narrow type.
        IRBuilder<> B(CI);
        Value *A = CI->getArgOperand(0), *S = CI->getArgOperand(1);
        unsigned N = A->getType()->getIntegerBitWidth();
        bool Signed = Callee->getIntrinsicID() == Intrinsic::sshl_sat;
        Type *Wide = B.getIntNTy(N * 2);
        Value *WA = Signed ? B.CreateSExt(A, Wide) : B.CreateZExt(A, Wide);
        Value *WS = B.CreateZExt(S, Wide);
        Value *W = B.CreateShl(WA, WS);
        APInt Lo = Signed ? APInt::getSignedMinValue(N) : APInt::getMinValue(N);
        APInt Hi = Signed ? APInt::getSignedMaxValue(N) : APInt::getMaxValue(N);
        Value *CLo = ConstantInt::get(A->getType(), Lo);
        Value *CHi = ConstantInt::get(A->getType(), Hi);
        Value *WLo = Signed ? B.CreateSExt(CLo, Wide) : B.CreateZExt(CLo, Wide);
        Value *WHi = Signed ? B.CreateSExt(CHi, Wide) : B.CreateZExt(CHi, Wide);
        Value *Lt = Signed ? B.CreateICmpSLT(W, WLo) : B.CreateICmpULT(W, WLo);
        Value *Gt = Signed ? B.CreateICmpSGT(W, WHi) : B.CreateICmpUGT(W, WHi);
        CI->replaceAllUsesWith(B.CreateSelect(Lt, ConstantInt::get(A->getType(),
                                                                   Lo),
                                              B.CreateSelect(Gt,
                                                             ConstantInt::get(
                                                                 A->getType(),
                                                                 Hi),
                                                             B.CreateTrunc(W,
                                                                           A->getType()))));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::uadd_with_overflow:
      case Intrinsic::usub_with_overflow:
      case Intrinsic::umul_with_overflow:
      case Intrinsic::sadd_with_overflow:
      case Intrinsic::ssub_with_overflow:
      case Intrinsic::smul_with_overflow: {
        // {result, overflow} is built lane by lane into a two-field struct.
        IRBuilder<> B(CI);
        auto RID = Callee->getIntrinsicID();
        Value *A = CI->getArgOperand(0), *C = CI->getArgOperand(1);
        Type *IT = A->getType();
        unsigned N = IT->getIntegerBitWidth();
        bool Signed = RID == Intrinsic::sadd_with_overflow ||
                      RID == Intrinsic::ssub_with_overflow ||
                      RID == Intrinsic::smul_with_overflow;
        bool Sub = RID == Intrinsic::usub_with_overflow ||
                   RID == Intrinsic::ssub_with_overflow;
        bool Mul = RID == Intrinsic::umul_with_overflow ||
                   RID == Intrinsic::smul_with_overflow;
        Value *Res = nullptr;
        Value *Overflow = nullptr;
        if (N * 2 > 128) {
          Res = Mul ? B.CreateMul(A, C)
                    : Sub ? B.CreateSub(A, C) : B.CreateAdd(A, C);
          Overflow = Mul ? mulOverflowed(B, Signed, A, C, Res)
                         : addSubOverflowed(B, Signed, Sub, A, C, Res);
        } else {
          Type *Wide = B.getIntNTy(N * 2);
          Value *WA = Signed ? B.CreateSExt(A, Wide) : B.CreateZExt(A, Wide);
          Value *WC = Signed ? B.CreateSExt(C, Wide) : B.CreateZExt(C, Wide);
          Value *W = Mul ? B.CreateMul(WA, WC)
                         : Sub ? B.CreateSub(WA, WC) : B.CreateAdd(WA, WC);
          Res = B.CreateTrunc(W, IT);
          Value *WRes =
              Signed ? B.CreateSExt(Res, Wide) : B.CreateZExt(Res, Wide);
          Overflow = B.CreateICmpNE(W, WRes);
        }
        Value *Pair = UndefValue::get(CI->getType());
        Pair = B.CreateInsertValue(Pair, Res, 0);
        Pair = B.CreateInsertValue(Pair, Overflow, 1);
        CI->replaceAllUsesWith(Pair);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::is_constant: {
        // Answered statically: this is a module-wide question, not a runtime
        // one.
        Value *V = CI->getArgOperand(0);
        CI->replaceAllUsesWith(
            ConstantInt::get(CI->getType(), isa<Constant>(V)));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::canonicalize: {
        // Only the all-NaN inputs differ from a plain copy: canonicalise means
        // "replace any NaN with the canonical quiet NaN". Every other value is
        // returned unchanged, so the copy is the whole operation.
        CI->replaceAllUsesWith(CI->getArgOperand(0));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::threadlocal_address:
        // A thread-local global is already per-thread storage in the emitted
        // code, so the reference itself is the address; printValueLoad turns
        // it into the accessor call.
        CI->replaceAllUsesWith(CI->getArgOperand(0));
        CI->eraseFromParent();
        break;
      case Intrinsic::fabs: {
        // Clear the sign bit; a compare-and-negate would be wrong for NaN.
        IRBuilder<> B(CI);
        Value *Orig = CI->getArgOperand(0);
        Value *X = narrowF80(Orig, B);
        Type *IT = B.getIntNTy(X->getType()->getPrimitiveSizeInBits());
        Value *Bits = B.CreateBitCast(X, IT);
        Value *Mask = ConstantInt::get(
            IT, APInt::getSignedMaxValue(IT->getIntegerBitWidth()));
        CI->replaceAllUsesWith(widenF80(
            B.CreateBitCast(B.CreateAnd(Bits, Mask), X->getType()),
            Orig->getType(), B));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::vector_reduce_mul:
      case Intrinsic::vector_reduce_smax:
      case Intrinsic::vector_reduce_smin:
      case Intrinsic::vector_reduce_umax:
      case Intrinsic::vector_reduce_umin:
      case Intrinsic::vector_reduce_fadd:
      case Intrinsic::vector_reduce_fmul:
      case Intrinsic::vector_reduce_fmax:
      case Intrinsic::vector_reduce_fmin: {
        IRBuilder<> B(CI);
        auto RID = Callee->getIntrinsicID();
        unsigned ArgNo =
            (RID == Intrinsic::vector_reduce_fadd ||
             RID == Intrinsic::vector_reduce_fmul)
                ? 1
                : 0;
        Value *V = CI->getArgOperand(ArgNo);
        bool HasInit = RID == Intrinsic::vector_reduce_fadd ||
                       RID == Intrinsic::vector_reduce_fmul;
        Value *Acc = HasInit ? CI->getArgOperand(0)
                             : B.CreateExtractElement(V, (uint64_t)0);
        unsigned N = cast<VectorType>(V->getType())
                         ->getElementCount()
                         .getFixedValue();
        for (unsigned I = HasInit ? 0 : 1; I < N; ++I) {
          Value *E = B.CreateExtractElement(V, (uint64_t)I);
          switch (RID) {
          case Intrinsic::vector_reduce_mul:
            Acc = B.CreateMul(Acc, E);
            break;
          case Intrinsic::vector_reduce_and:
            Acc = B.CreateAnd(Acc, E);
            break;
          case Intrinsic::vector_reduce_or:
            Acc = B.CreateOr(Acc, E);
            break;
          case Intrinsic::vector_reduce_xor:
            Acc = B.CreateXor(Acc, E);
            break;
          case Intrinsic::vector_reduce_smax:
            Acc = B.CreateSelect(B.CreateICmpSGT(E, Acc), E, Acc);
            break;
          case Intrinsic::vector_reduce_smin:
            Acc = B.CreateSelect(B.CreateICmpSLT(E, Acc), E, Acc);
            break;
          case Intrinsic::vector_reduce_umax:
            Acc = B.CreateSelect(B.CreateICmpUGT(E, Acc), E, Acc);
            break;
          case Intrinsic::vector_reduce_umin:
            Acc = B.CreateSelect(B.CreateICmpULT(E, Acc), E, Acc);
            break;
          case Intrinsic::vector_reduce_fadd:
          case Intrinsic::vector_reduce_fmul:
            Acc = RID == Intrinsic::vector_reduce_fadd ? B.CreateFAdd(Acc, E)
                                                       : B.CreateFMul(Acc, E);
            break;
          case Intrinsic::vector_reduce_fmax:
          case Intrinsic::vector_reduce_fmin: {
            // The langref leaves the NaN and signed-zero order open, so a
            // plain ordered compare is a faithful implementation.
            Value *C = RID == Intrinsic::vector_reduce_fmax
                           ? B.CreateFCmpOGT(E, Acc)
                           : B.CreateFCmpOLT(E, Acc);
            Acc = B.CreateSelect(C, E, Acc);
            break;
          }
          default:
            Acc = B.CreateAdd(Acc, E);
            break;
          }
        }
        CI->replaceAllUsesWith(Acc);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::abs: {
        // abs(x) = x < 0 ? -x : x, with the poison-on-INT_MIN flag ignored.
        IRBuilder<> B(CI);
        Value *X = CI->getArgOperand(0);
        Value *Neg = B.CreateNeg(X);
        Value *Cmp = B.CreateICmpSLT(X, ConstantInt::get(X->getType(), 0));
        CI->replaceAllUsesWith(B.CreateSelect(Cmp, Neg, X));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::smax:
      case Intrinsic::smin:
      case Intrinsic::umax:
      case Intrinsic::umin: {
        IRBuilder<> B(CI);
        Value *A = CI->getArgOperand(0), *C = CI->getArgOperand(1);
        bool IsMax = Callee->getIntrinsicID() == Intrinsic::smax ||
                     Callee->getIntrinsicID() == Intrinsic::umax;
        bool IsSigned = Callee->getIntrinsicID() == Intrinsic::smax ||
                        Callee->getIntrinsicID() == Intrinsic::smin;
        Value *Cmp = IsSigned ? B.CreateICmpSLT(A, C) : B.CreateICmpULT(A, C);
        CI->replaceAllUsesWith(IsMax ? B.CreateSelect(Cmp, C, A)
                                     : B.CreateSelect(Cmp, A, C));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::usub_sat:
      case Intrinsic::uadd_sat:
      case Intrinsic::ssub_sat:
      case Intrinsic::sadd_sat: {
        // Saturating arithmetic: compute wide, clamp to the result range.
        IRBuilder<> B(CI);
        // A vector result has no single wide type; scalarize into per-lane
        // calls of the scalar overload, which the fixpoint loop expands.
        if (auto *VTy = dyn_cast<VectorType>(CI->getType())) {
          Type *EltTy = VTy->getElementType();
          Function *Scalar = Intrinsic::getOrInsertDeclaration(
              Mod, Callee->getIntrinsicID(), EltTy);
          unsigned NV = VTy->getElementCount().getFixedValue();
          Value *Acc = PoisonValue::get(VTy);
          for (unsigned I = 0; I < NV; ++I) {
            Value *Lane = B.CreateCall(
                Scalar,
                {B.CreateExtractElement(CI->getArgOperand(0), (uint64_t)I),
                 B.CreateExtractElement(CI->getArgOperand(1), (uint64_t)I)});
            Acc = B.CreateInsertElement(Acc, Lane, (uint64_t)I);
          }
          CI->replaceAllUsesWith(Acc);
          CI->eraseFromParent();
          break;
        }
        Value *A = CI->getArgOperand(0), *C = CI->getArgOperand(1);
        unsigned N = A->getType()->getIntegerBitWidth();
        bool Signed = Callee->getIntrinsicID() == Intrinsic::ssub_sat ||
                      Callee->getIntrinsicID() == Intrinsic::sadd_sat;
        bool Sub = Callee->getIntrinsicID() == Intrinsic::usub_sat ||
                   Callee->getIntrinsicID() == Intrinsic::ssub_sat;
        if (N * 2 > 128) {
          CI->replaceAllUsesWith(saturatingAddSub(B, Signed, Sub, A, C));
          CI->eraseFromParent();
          break;
        }
        Type *Wide = B.getIntNTy(N * 2);
        Value *WA = B.CreateSExt(A, Wide);
        Value *WC = B.CreateSExt(C, Wide);
        Value *W = nullptr;
        if (!Signed) {
          WA = B.CreateZExt(A, Wide);
          WC = B.CreateZExt(C, Wide);
        }
        W = Sub ? B.CreateSub(WA, WC) : B.CreateAdd(WA, WC);
        // Clamp the wide result to [min,max] of the narrow type, then trunc.
        APInt Min = Signed ? APInt::getSignedMinValue(N)
                           : APInt::getMinValue(N);
        APInt Max = Signed ? APInt::getSignedMaxValue(N)
                           : APInt::getMaxValue(N);
        Value *WMn = B.CreateSExt(ConstantInt::get(A->getType(), Min), Wide);
        Value *WMx = B.CreateSExt(ConstantInt::get(A->getType(), Max), Wide);
        if (!Signed) {
          WMn = B.CreateZExt(ConstantInt::get(A->getType(), Min), Wide);
          WMx = B.CreateZExt(ConstantInt::get(A->getType(), Max), Wide);
        }
        Value *Lo = B.CreateICmpSLT(W, WMn);
        Value *Hi = B.CreateICmpSGT(W, WMx);
        if (!Signed) {
          Lo = B.CreateICmpULT(W, WMn);
          Hi = B.CreateICmpUGT(W, WMx);
        }
        Value *Clamped = B.CreateSelect(Lo, WMn, B.CreateSelect(Hi, WMx, W));
        CI->replaceAllUsesWith(B.CreateTrunc(Clamped, A->getType()));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::bitreverse: {
        // Reverse the bits with a shift-and-mask sequence.
        IRBuilder<> B(CI);
        Value *X = CI->getArgOperand(0);
        unsigned N = X->getType()->getIntegerBitWidth();
        Value *R = ConstantInt::get(X->getType(), 0);
        for (unsigned I = 0; I < N; ++I) {
          Value *Bit = B.CreateAnd(B.CreateLShr(X, I),
                                   ConstantInt::get(X->getType(), 1));
          R = B.CreateOr(R, B.CreateShl(Bit, N - 1 - I));
        }
        CI->replaceAllUsesWith(R);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::fshl:
      case Intrinsic::fshr: {
        // fshl(a,b,s) = (a << s) | (b >> (N-s)), fshr is the mirror. A zero
        // shift must not become a shift by N (undefined), so that case is
        // selected out; a constant shift folds entirely.
        IRBuilder<> B(CI);
        Value *A = CI->getArgOperand(0), *Bv = CI->getArgOperand(1),
              *S = CI->getArgOperand(2);
        Type *T = A->getType();
        unsigned N = T->getIntegerBitWidth();
        bool Left = Callee->getIntrinsicID() == Intrinsic::fshl;
        if (isa<ConstantInt>(S) && isa<ConstantInt>(A) &&
            isa<ConstantInt>(Bv)) {
          const APInt &Av = cast<ConstantInt>(A)->getValue();
          const APInt &Bvv = cast<ConstantInt>(Bv)->getValue();
          const APInt &Sv = cast<ConstantInt>(S)->getValue();
          CI->replaceAllUsesWith(ConstantInt::get(
              T, Left ? APIntOps::fshl(Av, Bvv, Sv) : APIntOps::fshr(Av, Bvv, Sv)));
          CI->eraseFromParent();
          break;
        }
        Value *Amt = B.CreateAnd(S, ConstantInt::get(T, N - 1));
        Value *Inv = B.CreateAnd(
            B.CreateSub(ConstantInt::get(T, 0), Amt),
            ConstantInt::get(T, N - 1)); // (N - s) mod N
        Value *Lo = Left ? B.CreateShl(A, Amt) : B.CreateLShr(A, Inv);
        Value *Hi = Left ? B.CreateLShr(Bv, Inv) : B.CreateShl(Bv, Amt);
        Value *R = B.CreateOr(Lo, Hi);
        CI->replaceAllUsesWith(
            B.CreateSelect(B.CreateICmpEQ(Amt, ConstantInt::get(T, 0)),
                           Left ? A : Bv, R));
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::fmuladd:
      case Intrinsic::fma: {
        // No CLR instruction for fused multiply-add; expand to a*b+c.
        IRBuilder<> B(CI);
        Value *A = CI->getArgOperand(0), *Bv = CI->getArgOperand(1),
              *C = CI->getArgOperand(2);
        Value *R = B.CreateFAdd(B.CreateFMul(A, Bv), C);
        CI->replaceAllUsesWith(R);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::experimental_cttz_elts: {
        // The number of trailing zero elements, which is the index of the
        // first non-zero one, or the element count when they are all zero.
        // IL has no SIMD, so this becomes one compare and select per lane;
        // walking down from the last lane leaves the lowest index winning.
        // The is_zero_poison flag needs no handling: returning the count for
        // an all-zero vector is one of the values poison permits. Clang
        // reaches this at -O2, for the two-lane i1 a vectorized `a && b`
        // produces.
        IRBuilder<> B(CI);
        auto *VTy = cast<VectorType>(CI->getArgOperand(0)->getType());
        unsigned N = VTy->getElementCount().getFixedValue();
        Value *V = CI->getArgOperand(0);
        Value *Zero = Constant::getNullValue(VTy->getElementType());
        Value *R = ConstantInt::get(CI->getType(), N);
        for (unsigned I = N; I-- > 0;) {
          Value *E = B.CreateExtractElement(V, (uint64_t)I);
          R = B.CreateSelect(B.CreateICmpNE(E, Zero),
                             ConstantInt::get(CI->getType(), I), R);
        }
        CI->replaceAllUsesWith(R);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::vector_reduce_add:
      case Intrinsic::vector_reduce_and:
      case Intrinsic::vector_reduce_or:
      case Intrinsic::vector_reduce_xor: {
        // IL has no SIMD, so reduce with a short extract/combine chain.
        auto *VTy = cast<VectorType>(CI->getArgOperand(0)->getType());
        unsigned N = VTy->getElementCount().getFixedValue();
        IRBuilder<> B(CI);
        Value *Acc = B.CreateExtractElement(CI->getArgOperand(0),
                                            (uint64_t)0);
        for (unsigned I = 1; I < N; ++I) {
          Value *E = B.CreateExtractElement(CI->getArgOperand(0),
                                            (uint64_t)I);
          switch (Callee->getIntrinsicID()) {
          case Intrinsic::vector_reduce_add: Acc = B.CreateAdd(Acc, E); break;
          case Intrinsic::vector_reduce_and: Acc = B.CreateAnd(Acc, E); break;
          case Intrinsic::vector_reduce_or: Acc = B.CreateOr(Acc, E); break;
          default: Acc = B.CreateXor(Acc, E); break;
          }
        }
        CI->replaceAllUsesWith(Acc);
        CI->eraseFromParent();
        break;
      }
      case Intrinsic::eh_typeid_for: {
        unsigned TypeId = getEHTypeId(CI->getArgOperand(0));
        CI->replaceAllUsesWith(ConstantInt::get(CI->getType(), TypeId));
        CI->eraseFromParent();
        break;
      }
      default:
        // IntrinsicLowering does not know the memory-use markers, and what it
        // does with an unknown lifetime intrinsic is rewrite it into the
        // declaration form -- which is still a lifetime intrinsic. Drop them
        // before it gets the chance.
        if (isDiscardableIntrinsic(Callee->getIntrinsicID())) {
          CI->eraseFromParent();
          break;
        }
        // The families IntrinsicLowering has never heard of are expanded
        // here; whatever is left it either lowers or reports on.
        if (!lowerExpandedIntrinsic(CI) && !lowerLibmIntrinsic(CI))
          IL->LowerIntrinsicCall(CI);
        break;
      }
    }
  }
    // Anything still calling a non-directly-emitted intrinsic needs another
    // pass; the lowering cases above always make progress, so this ends.
    bool Remaining = false;
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB)
        if (auto *CI = dyn_cast<CallInst>(&I))
          if (Function *Callee = CI->getCalledFunction())
            if (Callee->isIntrinsic())
              switch (Callee->getIntrinsicID()) {
              default:
                Remaining = true;
                break;
              case Intrinsic::vastart:
              case Intrinsic::vaend:
              case Intrinsic::vacopy:
              case Intrinsic::lifetime_start:
              case Intrinsic::lifetime_end:
              case Intrinsic::dbg_declare:
              case Intrinsic::dbg_value:
              case Intrinsic::dbg_label:
                break;
              }
    if (!Remaining)
      break;
  }
}
