//===-- MSILWideIntegers.cpp - Promote i65..i127 to i128 ------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// An iN value (64 < N < 128) is carried as an i128 sign-extended from bit N-1.
// Bitwise ops and ashr keep that form, anything that can carry past bit N-1
// re-normalizes, and the unsigned ops work on the value masked to N bits.
//
//===----------------------------------------------------------------------===//

#include "MSILWideIntegers.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {

unsigned wideWidth(Type *Ty) {
  auto *IT = dyn_cast<IntegerType>(Ty);
  if (!IT)
    return 0;
  unsigned N = IT->getBitWidth();
  return N > 64 && N < 128 ? N : 0;
}

bool containsWide(Type *Ty) {
  if (wideWidth(Ty))
    return true;
  if (auto *ST = dyn_cast<StructType>(Ty))
    return any_of(ST->elements(), containsWide);
  return false;
}

bool touchesWide(const Instruction &I) {
  if (containsWide(I.getType()))
    return true;
  return any_of(I.operands(),
                [](const Value *Op) { return containsWide(Op->getType()); });
}

[[noreturn]] void unsupported(const Instruction &I) {
  std::string Text;
  raw_string_ostream(Text) << I;
  report_fatal_error("MSIL: unsupported operation on an integer wider than 64 "
                     "bits: " +
                     Twine(Text));
}

class WidePromoter {
public:
  explicit WidePromoter(Function &F)
      : F(F), I128(Type::getInt128Ty(F.getContext())) {}

  bool run();

private:
  Function &F;
  Type *I128;
  DenseMap<Value *, Value *> Promoted;
  DenseMap<Value *, std::pair<Value *, Value *>> OverflowResults;
  SmallVector<std::pair<PHINode *, PHINode *>, 4> Phis;
  SmallVector<Instruction *, 16> Dead;

  Value *promoted(Value *V);
  Value *normalize(IRBuilder<> &B, Value *V, unsigned N);
  Value *masked(IRBuilder<> &B, Value *V, unsigned N);
  void promoteCast(CastInst &I);
  void promoteBinary(BinaryOperator &I);
  void promoteCompare(ICmpInst &I);
  void promoteOverflow(IntrinsicInst &I);
  void promoteExtract(ExtractValueInst &I);
  void promoteLoad(LoadInst &I);
  void promoteStore(StoreInst &I);
  void replaceLegal(Instruction &Old, Value *New);
  void replaceWide(Instruction &Old, Value *New);
};

Value *WidePromoter::promoted(Value *V) {
  if (auto *CI = dyn_cast<ConstantInt>(V))
    return ConstantInt::get(I128, CI->getValue().sext(128));
  if (isa<UndefValue>(V))
    return ConstantInt::get(I128, 0);
  auto It = Promoted.find(V);
  if (It == Promoted.end()) {
    std::string Text;
    raw_string_ostream(Text) << *V;
    report_fatal_error("MSIL: a wide integer has no i128 form yet: " +
                       Twine(Text));
  }
  return It->second;
}

Value *WidePromoter::normalize(IRBuilder<> &B, Value *V, unsigned N) {
  Value *Shift = ConstantInt::get(I128, 128 - N);
  return B.CreateAShr(B.CreateShl(V, Shift), Shift);
}

Value *WidePromoter::masked(IRBuilder<> &B, Value *V, unsigned N) {
  return B.CreateAnd(V, ConstantInt::get(I128, APInt::getLowBitsSet(128, N)));
}

void WidePromoter::replaceLegal(Instruction &Old, Value *New) {
  Old.replaceAllUsesWith(New);
  Dead.push_back(&Old);
}

void WidePromoter::replaceWide(Instruction &Old, Value *New) {
  Promoted[&Old] = New;
  Dead.push_back(&Old);
}

void WidePromoter::promoteCast(CastInst &I) {
  IRBuilder<> B(&I);
  Value *Src = I.getOperand(0);
  unsigned SrcN = wideWidth(Src->getType());
  unsigned DstN = wideWidth(I.getType());
  unsigned DstBits = I.getType()->getIntegerBitWidth();

  if (!SrcN) {
    // narrow source: both extensions leave a value that fits below bit N-1
    if (I.getOpcode() == Instruction::ZExt)
      return replaceWide(I, B.CreateZExt(Src, I128));
    if (I.getOpcode() == Instruction::SExt)
      return replaceWide(I, B.CreateSExt(Src, I128));
    if (I.getOpcode() == Instruction::Trunc &&
        Src->getType()->isIntegerTy(128))
      return replaceWide(I, normalize(B, Src, DstN));
    unsupported(I);
  }

  Value *V = promoted(Src);
  switch (I.getOpcode()) {
  case Instruction::Trunc:
    if (DstN)
      return replaceWide(I, normalize(B, V, DstN));
    return replaceLegal(I, B.CreateTrunc(V, I.getType()));
  case Instruction::ZExt:
    if (DstN)
      return replaceWide(I, masked(B, V, SrcN));
    if (DstBits == 128)
      return replaceLegal(I, masked(B, V, SrcN));
    break;
  case Instruction::SExt:
    if (DstN)
      return replaceWide(I, V);
    if (DstBits == 128)
      return replaceLegal(I, V);
    break;
  default:
    break;
  }
  unsupported(I);
}

void WidePromoter::promoteBinary(BinaryOperator &I) {
  unsigned N = wideWidth(I.getType());
  IRBuilder<> B(&I);
  Value *L = promoted(I.getOperand(0));
  Value *R = promoted(I.getOperand(1));
  switch (I.getOpcode()) {
  case Instruction::And:
    return replaceWide(I, B.CreateAnd(L, R));
  case Instruction::Or:
    return replaceWide(I, B.CreateOr(L, R));
  case Instruction::Xor:
    return replaceWide(I, B.CreateXor(L, R));
  case Instruction::AShr:
    return replaceWide(I, B.CreateAShr(L, R));
  case Instruction::Add:
    return replaceWide(I, normalize(B, B.CreateAdd(L, R), N));
  case Instruction::Sub:
    return replaceWide(I, normalize(B, B.CreateSub(L, R), N));
  case Instruction::Mul:
    return replaceWide(I, normalize(B, B.CreateMul(L, R), N));
  case Instruction::Shl:
    return replaceWide(I, normalize(B, B.CreateShl(L, R), N));
  case Instruction::LShr:
    return replaceWide(I, normalize(B, B.CreateLShr(masked(B, L, N), R), N));
  case Instruction::UDiv:
    return replaceWide(
        I, normalize(B, B.CreateUDiv(masked(B, L, N), masked(B, R, N)), N));
  case Instruction::URem:
    return replaceWide(
        I, normalize(B, B.CreateURem(masked(B, L, N), masked(B, R, N)), N));
  case Instruction::SDiv:
    return replaceWide(I, normalize(B, B.CreateSDiv(L, R), N));
  case Instruction::SRem:
    return replaceWide(I, normalize(B, B.CreateSRem(L, R), N));
  default:
    unsupported(I);
  }
}

void WidePromoter::promoteCompare(ICmpInst &I) {
  unsigned N = wideWidth(I.getOperand(0)->getType());
  IRBuilder<> B(&I);
  Value *L = promoted(I.getOperand(0));
  Value *R = promoted(I.getOperand(1));
  if (I.isUnsigned()) {
    L = masked(B, L, N);
    R = masked(B, R, N);
  }
  replaceLegal(I, B.CreateICmp(I.getPredicate(), L, R));
}

void WidePromoter::promoteOverflow(IntrinsicInst &I) {
  unsigned N = wideWidth(I.getArgOperand(0)->getType());
  IRBuilder<> B(&I);
  Value *L = promoted(I.getArgOperand(0));
  Value *R = promoted(I.getArgOperand(1));
  Value *Mask = ConstantInt::get(I128, APInt::getLowBitsSet(128, N));
  Value *Result = nullptr;
  Value *Overflow = nullptr;
  switch (I.getIntrinsicID()) {
  case Intrinsic::sadd_with_overflow:
  case Intrinsic::ssub_with_overflow: {
    // both operands fit in N bits, so the exact result fits in i128
    Value *Exact = I.getIntrinsicID() == Intrinsic::sadd_with_overflow
                       ? B.CreateAdd(L, R)
                       : B.CreateSub(L, R);
    Result = normalize(B, Exact, N);
    Overflow = B.CreateICmpNE(Exact, Result);
    break;
  }
  case Intrinsic::uadd_with_overflow: {
    Value *Exact = B.CreateAdd(masked(B, L, N), masked(B, R, N));
    Result = normalize(B, Exact, N);
    Overflow = B.CreateICmpUGT(Exact, Mask);
    break;
  }
  case Intrinsic::usub_with_overflow: {
    Value *UL = masked(B, L, N), *UR = masked(B, R, N);
    Result = normalize(B, B.CreateSub(UL, UR), N);
    Overflow = B.CreateICmpULT(UL, UR);
    break;
  }
  default:
    unsupported(I);
  }
  OverflowResults[&I] = {Result, Overflow};
  Dead.push_back(&I);
}

void WidePromoter::promoteExtract(ExtractValueInst &I) {
  auto It = OverflowResults.find(I.getAggregateOperand());
  if (It == OverflowResults.end() || I.getNumIndices() != 1)
    unsupported(I);
  if (I.getIndices()[0] == 0)
    return replaceWide(I, It->second.first);
  replaceLegal(I, It->second.second);
}

// the load reads the whole 16-byte alloc size, which an iN slot always has
void WidePromoter::promoteLoad(LoadInst &I) {
  if (!I.isSimple())
    unsupported(I);
  IRBuilder<> B(&I);
  LoadInst *Whole = B.CreateAlignedLoad(I128, I.getPointerOperand(), I.getAlign());
  replaceWide(I, normalize(B, Whole, wideWidth(I.getType())));
}

// the store must write only its store size: the low 8 bytes, then the rest
// one byte at a time
void WidePromoter::promoteStore(StoreInst &I) {
  if (!I.isSimple())
    unsupported(I);
  Value *Ptr = I.getPointerOperand();
  unsigned StoreBytes = (wideWidth(I.getValueOperand()->getType()) + 7) / 8;
  IRBuilder<> B(&I);
  Value *V = promoted(I.getValueOperand());
  B.CreateAlignedStore(B.CreateTrunc(V, B.getInt64Ty()), Ptr, I.getAlign());
  for (unsigned Byte = 8; Byte < StoreBytes; ++Byte) {
    Value *Shifted = B.CreateLShr(V, ConstantInt::get(I128, Byte * 8));
    Value *Addr = B.CreateConstInBoundsGEP1_64(B.getInt8Ty(), Ptr, Byte);
    B.CreateStore(B.CreateTrunc(Shifted, B.getInt8Ty()), Addr);
  }
  Dead.push_back(&I);
}

bool WidePromoter::run() {
  bool Any = false;
  for (BasicBlock &BB : F)
    for (Instruction &I : BB)
      Any |= touchesWide(I);
  for (Argument &A : F.args())
    if (wideWidth(A.getType()))
      report_fatal_error("MSIL: a parameter wider than 64 bits and narrower "
                         "than 128 is not supported in " +
                         F.getName());
  if (!Any)
    return false;

  // phis first, so a loop's back edge can name a value defined later
  for (BasicBlock &BB : F)
    for (PHINode &P : BB.phis())
      if (wideWidth(P.getType())) {
        auto *New = PHINode::Create(I128, P.getNumIncomingValues(), "",
                                    P.getIterator());
        Phis.push_back({&P, New});
        replaceWide(P, New);
      }

  ReversePostOrderTraversal<Function *> RPOT(&F);
  for (BasicBlock *BB : RPOT)
    for (Instruction &I : *BB) {
      if (isa<PHINode>(I) || !touchesWide(I))
        continue;
      if (auto *Cast = dyn_cast<CastInst>(&I))
        promoteCast(*Cast);
      else if (auto *Bin = dyn_cast<BinaryOperator>(&I))
        promoteBinary(*Bin);
      else if (auto *Cmp = dyn_cast<ICmpInst>(&I))
        promoteCompare(*Cmp);
      else if (auto *Sel = dyn_cast<SelectInst>(&I)) {
        IRBuilder<> B(Sel);
        replaceWide(*Sel, B.CreateSelect(Sel->getCondition(),
                                         promoted(Sel->getTrueValue()),
                                         promoted(Sel->getFalseValue())));
      } else if (auto *II = dyn_cast<IntrinsicInst>(&I))
        promoteOverflow(*II);
      else if (auto *EV = dyn_cast<ExtractValueInst>(&I))
        promoteExtract(*EV);
      else if (auto *Load = dyn_cast<LoadInst>(&I))
        promoteLoad(*Load);
      else if (auto *Store = dyn_cast<StoreInst>(&I))
        promoteStore(*Store);
      else
        unsupported(I);
    }

  for (auto &[Old, New] : Phis)
    for (unsigned I = 0, E = Old->getNumIncomingValues(); I != E; ++I)
      New->addIncoming(promoted(Old->getIncomingValue(I)),
                       Old->getIncomingBlock(I));

  for (Instruction *I : Dead)
    I->dropAllReferences();
  for (Instruction *I : Dead)
    I->eraseFromParent();
  return true;
}

} // namespace

bool llvm::promoteWideIntegers(Function &F) {
  WidePromoter Promoter(F);
  return Promoter.run();
}

static bool isOddWidthAccess(Type *Ty, const DataLayout &DL) {
  auto *ITy = dyn_cast<IntegerType>(Ty);
  if (!ITy || ITy->getBitWidth() > 64)
    return false;
  uint64_t Bytes = DL.getTypeStoreSize(ITy).getFixedValue();
  return !isPowerOf2_64(Bytes);
}

static SmallVector<std::pair<uint64_t, uint64_t>, 3>
splitPieces(uint64_t Bytes) {
  SmallVector<std::pair<uint64_t, uint64_t>, 3> Pieces;
  uint64_t Offset = 0;
  while (Offset != Bytes) {
    uint64_t Size = llvm::bit_floor(Bytes - Offset);
    Pieces.push_back({Offset, Size});
    Offset += Size;
  }
  return Pieces;
}

static Value *pieceAddress(IRBuilder<> &B, Value *Base, uint64_t Offset) {
  if (!Offset)
    return Base;
  return B.CreateConstInBoundsGEP1_64(B.getInt8Ty(), Base, Offset);
}

static void splitLoad(LoadInst *Load, const DataLayout &DL) {
  IRBuilder<> B(Load);
  auto *ITy = cast<IntegerType>(Load->getType());
  uint64_t Bytes = DL.getTypeStoreSize(ITy).getFixedValue();
  Value *Result = nullptr;
  for (auto [Offset, Size] : splitPieces(Bytes)) {
    Type *PieceTy = B.getIntNTy(Size * 8);
    Value *Ptr = pieceAddress(B, Load->getPointerOperand(), Offset);
    LoadInst *Piece = B.CreateAlignedLoad(
        PieceTy, Ptr, commonAlignment(Load->getAlign(), Offset),
        Load->isVolatile());
    Value *Widened = B.CreateZExtOrTrunc(Piece, ITy);
    if (!Result) {
      Result = Widened;
      continue;
    }
    Value *Shifted = B.CreateShl(Widened, Offset * 8);
    Result = B.CreateOr(Result, Shifted);
  }
  Load->replaceAllUsesWith(Result);
  Load->eraseFromParent();
}

static void splitStore(StoreInst *Store, const DataLayout &DL) {
  IRBuilder<> B(Store);
  Value *Val = Store->getValueOperand();
  uint64_t Bytes = DL.getTypeStoreSize(Val->getType()).getFixedValue();
  for (auto [Offset, Size] : splitPieces(Bytes)) {
    Type *PieceTy = B.getIntNTy(Size * 8);
    Value *Shifted = Offset ? B.CreateLShr(Val, Offset * 8) : Val;
    Value *Piece = B.CreateZExtOrTrunc(Shifted, PieceTy);
    Value *Ptr = pieceAddress(B, Store->getPointerOperand(), Offset);
    B.CreateAlignedStore(Piece, Ptr, commonAlignment(Store->getAlign(), Offset),
                         Store->isVolatile());
  }
  Store->eraseFromParent();
}

bool llvm::splitOddWidthAccesses(Function &F) {
  const DataLayout &DL = F.getDataLayout();
  SmallVector<LoadInst *, 8> Loads;
  SmallVector<StoreInst *, 8> Stores;
  for (Instruction &I : instructions(F)) {
    if (auto *Load = dyn_cast<LoadInst>(&I)) {
      if (!Load->isAtomic() && isOddWidthAccess(Load->getType(), DL))
        Loads.push_back(Load);
    } else if (auto *Store = dyn_cast<StoreInst>(&I)) {
      Type *Ty = Store->getValueOperand()->getType();
      if (!Store->isAtomic() && isOddWidthAccess(Ty, DL))
        Stores.push_back(Store);
    }
  }
  for (LoadInst *Load : Loads)
    splitLoad(Load, DL);
  for (StoreInst *Store : Stores)
    splitStore(Store, DL);
  return !Loads.empty() || !Stores.empty();
}
