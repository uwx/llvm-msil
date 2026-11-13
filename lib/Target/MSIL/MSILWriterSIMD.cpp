//===-- MSILWriterSIMD.cpp - SIMD (System.Numerics.Vector<T>) lowering -----===//
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
// SIMD types
//===----------------------------------------------------------------------===//
//
// A vector is held in System.Numerics.Vector<T> when its bit width is exactly
// one hardware register (Vector<T>.Count * sizeof(T)), because that is the
// only width the type can represent: it has no length of its own, and the
// spare lanes of a narrower vector would have to be padded -- padding that
// measured 5x slower than the plain scalar lane loop it replaced. Every other
// shape keeps that loop.

namespace {
constexpr unsigned kVecBytes = 32;
} // namespace

bool MSILWriter::isSimdVectorTy(Type *Ty) const {
  auto *VTy = dyn_cast<VectorType>(Ty);
  if (!VTy || VTy->isScalableTy())
    return false;
  Type *EltTy = VTy->getElementType();
  if (!EltTy->isIntegerTy() && !EltTy->isFloatTy() && !EltTy->isDoubleTy())
    return false;
  // The CLR has no 8- or 16-bit SIMD arithmetic; those widen through int32
  // lanes only, which Vector<T> would have to do lane by lane anyway.
  if (EltTy->isIntegerTy() && EltTy->getIntegerBitWidth() < 32)
    return false;
  // Vector<T> is one 32-byte register on this target (Count * sizeof(T)).
  return VTy->getElementCount().getFixedValue() * EltTy->getPrimitiveSizeInBits() ==
         kVecBytes * 8;
}

//===----------------------------------------------------------------------===//
// SIMD operations
//===----------------------------------------------------------------------===//
//
// A vector whose bit width is exactly one SIMD register lives in a
// System.Numerics.Vector<T> local; every other shape keeps the scalar lane
// loop. Vector<T> has no length of its own and its Count is the register
// width, so only an exact fit avoids padding, and padded lanes measured
// 5x slower than the scalar loop they replaced.
//
// The value crosses between the blob and the register with cpblk, the only
// transfer that needs no managed byref. Consecutive operations then stay in
// the register: one cpblk round trip per operation measured 16x slower than
// the scalar loop, while a whole loop body in the register (array load,
// multiply-add, array store) measured 4.1x faster.

namespace {
const char *const VecNs = "[System.Numerics.Vectors]System.Numerics.Vector`1";
} // namespace

std::string MSILWriter::getVectorElemName(Type *EltTy) {
  if (EltTy->isIntegerTy(32))
    return "int32";
  if (EltTy->isIntegerTy(64))
    return "int64";
  if (EltTy->isFloatTy())
    return "float32";
  if (EltTy->isDoubleTy())
    return "float64";
  return "";
}

std::string MSILWriter::getVectorTypeName(Type *VTy) {
  std::string E = getVectorElemName(VTy->getScalarType());
  if (E.empty())
    return "";
  UsesVector = true;
  return "valuetype [System.Numerics.Vectors]System.Numerics.Vector`1<" + E +
         ">";
}

// A GEP with a vector index produces one pointer per lane.
void MSILWriter::printVectorGep(const GEPOperator *GEP) {
  auto *RTy = cast<VectorType>(GEP->getType());
  Type *EltTy = RTy->getElementType();
  uint64_t PtrSize = DL->getTypeAllocSize(EltTy).getFixedValue();
  unsigned N = RTy->getElementCount().getFixedValue();
  std::string Base = newTempLocal(GEP->getPointerOperandType());
  std::string R = newTempLocal(RTy);
  printValueLoad(GEP->getOperand(0));
  out() << "\tstloc\t'" << Base << "'\n";
  out() << "\tldloca\t'" << R << "'\n\tinitobj\t" << getTypeName(RTy) << "\n";
  for (unsigned I = 0; I < N; ++I) {
    printAggregateLaneAddr(R, EltTy, I * PtrSize);
    out() << "\tldloc\t'" << Base << "'\n\tconv.u8\n";
    for (gep_type_iterator It = gep_type_begin(GEP), E = gep_type_end(GEP);
         It != E; ++It) {
      const Value *Idx = It.getOperand();
      if (StructType *STy = It.getStructTypeOrNull()) {
        uint64_t Off = DL->getStructLayout(STy)->getElementOffset(
            cast<ConstantInt>(Idx)->getZExtValue());
        if (Off)
          out() << "\tldc.i8\t" << Off << "\n\tadd\n";
        continue;
      }
      uint64_t ElemSize = It.getSequentialElementStride(*DL).getFixedValue();
      if (Idx->getType()->isVectorTy()) {
        // Take this lane's index out of the vector.
        std::string IV = newTempLocal(Idx->getType());
        printValueLoad(Idx);
        out() << "\tstloc\t'" << IV << "'\n";
        printAggregateLaneAddr(IV, Idx->getType()->getScalarType(),
                               I * DL->getTypeAllocSize(
                                       Idx->getType()->getScalarType())
                                       .getFixedValue());
        out() << "\tldind.i8\n";
      } else {
        printValueLoad(Idx);
        out() << "\tconv.i8\n";
      }
      out() << "\tldc.i8\t" << ElemSize << "\n\tmul\n\tadd\n";
    }
    out() << "\tstind." << getMemSuffix(EltTy) << "\n";
  }
  out() << "\tldloc\t'" << R << "'\n";
}

void MSILWriter::printVectorBinary(StringRef Op, const Instruction *Inst,
                                   bool ZeroExtLanes, bool ShiftOp) {
  auto *VTy = cast<VectorType>(Inst->getType());
  Type *EltTy = VTy->getElementType();
  uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
  unsigned N = VTy->getElementCount().getFixedValue();
  std::string A = newTempLocal(VTy), B = newTempLocal(VTy),
              R = newTempLocal(VTy);
  printValueLoad(Inst->getOperand(0));
  out() << "\tstloc\t'" << A << "'\n";
  printValueLoad(Inst->getOperand(1));
  out() << "\tstloc\t'" << B << "'\n";
  out() << "\tldloca\t'" << R << "'\n\tinitobj\t" << getTypeName(VTy)
        << "\n";
  std::string Sfx = getMemSuffix(EltTy);
  // ldind.i1/i2 sign-extend; unsigned lane ops need the high bits cleared.
  // A 4- or 8-byte lane already occupies its full width on the stack, so
  // re-extending it would only truncate: conv.u2 there cuts an i32 lane to
  // 16 bits.
  const char *Ext = !ZeroExtLanes  ? ""
                    : ElemSize == 1 ? "\tconv.u1\n"
                    : ElemSize == 2 ? "\tconv.u2\n"
                                    : "";
  for (unsigned I = 0; I < N; ++I) {
    // &R[I] = A[I] op B[I]
    printAggregateLaneAddr(R, EltTy, I * ElemSize);
    printAggregateLaneAddr(A, EltTy, I * ElemSize);
    out() << "\tldind." << Sfx << "\n" << Ext;
    printAggregateLaneAddr(B, EltTy, I * ElemSize);
    out() << "\tldind." << Sfx << "\n";
    if (ShiftOp)
      out() << "\tconv.i4\n";
    else
      out() << Ext;
    out() << "\t" << Op << "\n";
    out() << "\tstind." << Sfx << "\n";
  }
  out() << "\tldloc\t'" << R << "'\n";
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

bool MSILWriter::tryPrintSimdBinary(const Instruction *Inst) {
  Type *Ty = Inst->getType();
  if (!isSimdVectorTy(Ty))
    return false;
  Type *EltTy = Ty->getScalarType();
  std::string E = getVectorElemName(EltTy);
  if (E.empty())
    return false;
  std::string VT = getVectorTypeName(Ty);
  bool IsInt = EltTy->isIntegerTy();
  unsigned Opc = Inst->getOpcode();

  // Vector<T> has no remainder and no unsigned divide; only UDiv needs the
  // unsigned spelling, and it gets there through the conversion operator.
  if (Opc == Instruction::URem || Opc == Instruction::SRem ||
      Opc == Instruction::FRem)
    return false;
  bool NeedU = IsInt && Opc == Instruction::UDiv;
  std::string NVU = "[System.Numerics.Vectors]System.Numerics.Vector`1";
  std::string U = NeedU ? (EltTy->isIntegerTy(32) ? "uint32" : "uint64") : E;
  std::string UT = std::string(NVU) + "<" + U + ">";

  const char *Name = nullptr;
  bool Shift = false;
  switch (Opc) {
  case Instruction::Add:
  case Instruction::FAdd: Name = "Add"; break;
  case Instruction::Sub:
  case Instruction::FSub: Name = "Subtract"; break;
  case Instruction::Mul:
  case Instruction::FMul: Name = "Multiply"; break;
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv: Name = "Divide"; break;
  case Instruction::And: Name = "BitwiseAnd"; break;
  case Instruction::Or: Name = "BitwiseOr"; break;
  // The static helper is Xor; only the instance operator is op_ExclusiveOr.
  case Instruction::Xor: Name = "Xor"; break;
  case Instruction::Shl: Name = "ShiftLeft"; Shift = true; break;
  case Instruction::LShr: Name = "ShiftRightLogical"; Shift = true; break;
  case Instruction::AShr: Name = "ShiftRightArithmetic"; Shift = true; break;
  default: return false;
  }

  if (Shift) {
    // llvm requires a uniform shift count, so lane 0 carries it for all.
    std::string Cnt = newTempLocal(Inst->getOperand(1)->getType());
    printValueLoad(Inst->getOperand(1));
    out() << "\tstloc\t'" << Cnt << "'\n";
    printValueLoad(Inst->getOperand(0));
    printAggregateLaneAddr(Cnt, EltTy, 0);
    out() << "\tldind." << getMemSuffix(EltTy) << "\n\tconv.i4\n";
    // The shift helpers take a concrete Vector<T>, unlike the arithmetic ones.
    out() << "\tcall\tvaluetype " << NVU << "<" << U << "> "
          << "[System.Numerics.Vectors]System.Numerics.Vector::" << Name
          << "(valuetype " << NVU << "<" << U << ">, int32)\n";
    return true;
  }

  // The bit-reinterpreting helpers are static and generic over the source,
  // which is the only spelling ilasm and the runtime both accept.
  auto ToUnsigned = [&] {
    emitVecCall(out(), E, "AsVectorUInt32", std::string("valuetype ") + NVU +
                                                    "<!!0>");
  };
  printValueLoad(Inst->getOperand(0));
  if (NeedU)
    ToUnsigned();
  printValueLoad(Inst->getOperand(1));
  if (NeedU)
    ToUnsigned();
  emitVecCall(out(), U, Name,
              std::string("valuetype ") + NVU +
                  "<!!0>, valuetype " + NVU + "<!!0>");
  if (NeedU)
    emitVecCall(out(), U, "AsVectorInt32",
                std::string("valuetype ") + NVU + "<!!0>");
  return true;
}

bool MSILWriter::tryPrintSimdCmpToStack(const User *Inst, bool IsFP) {
  // The comparison's own type is <N x i1>; it is the operand type that has to
  // be a whole register, since the mask is as wide as the operands.
  auto *OpTy = dyn_cast<VectorType>(Inst->getOperand(0)->getType());
  if (!OpTy || !isSimdVectorTy(OpTy))
    return false;
  Type *EltTy = OpTy->getElementType();
  std::string E = getVectorElemName(EltTy);
  if (E.empty())
    return false;
  StringRef NV = "[System.Numerics.Vectors]System.Numerics.Vector`1";
  std::string VT = getVectorTypeName(OpTy);
  if (VT.empty())
    return false;

  unsigned Pred = IsFP ? cast<FCmpInst>(Inst)->getPredicate()
                       : cast<ICmpInst>(Inst)->getPredicate();
  // Only the ordered predicates have a single Vector<T> helper; the rest fall
  // back to the lane loop. The lanes are int32/int64, so the helpers compare
  // signed and unsigned predicates take the lane loop too.
  const char *Name = nullptr;
  bool Invert = false;
  if (IsFP) {
    switch (Pred) {
    case CmpInst::FCMP_OEQ: Name = "Equals"; break;
    case CmpInst::FCMP_OLT: Name = "LessThan"; break;
    case CmpInst::FCMP_OGT: Name = "GreaterThan"; break;
    case CmpInst::FCMP_OLE: Name = "LessThanOrEqual"; break;
    case CmpInst::FCMP_OGE: Name = "GreaterThanOrEqual"; break;
    default: return false;
    }
  } else {
    switch (Pred) {
    case CmpInst::ICMP_EQ: Name = "Equals"; break;
    case CmpInst::ICMP_NE: Name = "Equals"; Invert = true; break;
    case CmpInst::ICMP_SLT: Name = "LessThan"; break;
    case CmpInst::ICMP_SGT: Name = "GreaterThan"; break;
    case CmpInst::ICMP_SLE: Name = "LessThanOrEqual"; break;
    case CmpInst::ICMP_SGE: Name = "GreaterThanOrEqual"; break;
    default: return false;
    }
  }

  printValueLoad(Inst->getOperand(0));
  printValueLoad(Inst->getOperand(1));
  emitVecCall(out(), E, Name,
              std::string("valuetype ") + NV.str() + "<!!0>, valuetype " +
                  NV.str() + "<!!0>");
  if (Invert) {
    // A mask is all-ones where the comparison holds; xor flips every lane.
    printConstLoad(Constant::getAllOnesValue(OpTy));
    emitVecCall(out(), E, "Xor",
                std::string("valuetype ") + NV.str() + "<!!0>, valuetype " +
                  NV.str() + "<!!0>");
  }
  return true;
}

bool MSILWriter::tryPrintSimdSelect(const Instruction *Inst) {
  Type *Ty = Inst->getType();
  if (!isSimdVectorTy(Ty))
    return false;
  std::string VT = getVectorTypeName(Ty);
  std::string E = getVectorElemName(Ty->getScalarType());

  // The comparison's result is an i1 vector, which lives in a packed blob:
  // it cannot hold the full-width mask a Vector<T> compare produces. So the
  // compare is only worth doing on the register when its one use is this
  // select, where the mask never has to be materialised.
  const Value *Cond = Inst->getOperand(0);
  const auto *Cmp = dyn_cast<Instruction>(Cond);
  bool FuseCmp = Cmp && Cmp->hasOneUse() &&
                 (isa<ICmpInst>(Cmp) || isa<FCmpInst>(Cmp)) &&
                 isSimdVectorTy(Cmp->getOperand(0)->getType());
  if (!FuseCmp)
    return false;

  if (!tryPrintSimdCmpToStack(Cmp, isa<FCmpInst>(Cmp)))
    return false;
  printValueLoad(Inst->getOperand(1));
  printValueLoad(Inst->getOperand(2));
  StringRef NV = "[System.Numerics.Vectors]System.Numerics.Vector`1";
  emitVecCall(out(), E, "ConditionalSelect",
              std::string("valuetype ") + NV.str() + "<!!0>, valuetype " +
                  NV.str() + "<!!0>, valuetype " + NV.str() + "<!!0>");
  return true;
}

// Emits the byte address of `Base + Off` as an unmanaged pointer.
void MSILWriter::printAggregateLaneAddr(const std::string &Base, Type *EltTy,
                                        uint64_t Off) {
  out() << "\tldloca\t'" << Base << "'\n\tconv.u8\n";
  if (Off) {
    printPtrLoad(Off);
    out() << "\tconv.i8\n\tadd\n";
  }
}

void MSILWriter::printVectorCast(unsigned Op, const Value *V, Type *Ty) {
  // IL has no vector casts; widen or narrow each lane in memory.
  auto *SrcVTy = cast<VectorType>(V->getType());
  auto *DstVTy = cast<VectorType>(Ty);
  Type *SrcElt = SrcVTy->getElementType(), *DstElt = DstVTy->getElementType();
  uint64_t SrcSize = DL->getTypeAllocSize(SrcElt).getFixedValue();
  uint64_t DstSize = DL->getTypeAllocSize(DstElt).getFixedValue();
  unsigned N = DstVTy->getElementCount().getFixedValue();
  std::string S = newTempLocal(SrcVTy), R = newTempLocal(DstVTy);
  printValueLoad(V);
  out() << "\tstloc\t'" << S << "'\n";
  out() << "\tldloca\t'" << R << "'\n\tinitobj\t" << getTypeName(DstVTy)
        << "\n";
  for (unsigned I = 0; I < N; ++I) {
    printAggregateLaneAddr(R, DstElt, I * DstSize);
    printAggregateLaneAddr(S, SrcElt, I * SrcSize);
    // Load the lane widened by ldind, then narrow it to the destination width.
    out() << "\tldind." << getMemSuffix(SrcElt) << "\n";
    switch (Op) {
    case Instruction::ZExt:
      if (SrcElt->getIntegerBitWidth() <= 32) {
        out() << '\t' << widthConv(SrcElt) << '\n';
        if (DstElt->getIntegerBitWidth() > 32)
          out() << "\tconv.u8\n";
      }
      break;
    case Instruction::SExt:
      // an i1 lane is stored as 0/1, true has to become all ones
      if (SrcElt->isIntegerTy(1))
        out() << "\tldc.i4.1\n\tand\n\tneg\n";
      out() << (DstElt->getIntegerBitWidth() > 32 ? "\tconv.i8\n"
                                                  : "\tconv.i4\n");
      break;
    case Instruction::Trunc:
      if (DstElt->getIntegerBitWidth() <= 32)
        out() << '\t' << widthConv(DstElt) << '\n';
      break;
    case Instruction::UIToFP:
      out() << "\tconv.r.un\n\tconv.r"
            << (DstElt->isFloatTy() ? '4' : '8') << '\n';
      break;
    case Instruction::SIToFP:
      out() << "\tconv.r" << (DstElt->isFloatTy() ? '4' : '8') << '\n';
      break;
    case Instruction::FPToUI:
      out() << "\tconv.u4\n";
      if (DstElt->getIntegerBitWidth() > 32)
        out() << "\tconv.u8\n";
      break;
    case Instruction::FPToSI:
      out() << "\tconv.i4\n";
      if (DstElt->getIntegerBitWidth() > 32)
        out() << "\tconv.i8\n";
      break;
    case Instruction::FPExt:
      out() << "\tconv.r8\n";
      break;
    case Instruction::FPTrunc:
      out() << "\tconv.r4\n";
      break;
    default:
      break;
    }
    out() << "\tstind." << getMemSuffix(DstElt) << "\n";
  }
  out() << "\tldloc\t'" << R << "'\n";
}

// A bitcast keeps the bits, so it goes through memory of the source type. The
// one exception is an i1 vector: a lane is a byte in the blob but a bit in the
// integer it is cast to or from.
void MSILWriter::printVectorBitcast(const Value *V, Type *Ty) {
  Type *SrcTy = V->getType();
  std::string Src = newTempLocal(SrcTy);
  printValueLoad(V);
  out() << "\tstloc\t'" << Src << "'\n";

  auto *SrcVTy = dyn_cast<FixedVectorType>(SrcTy);
  auto *DstVTy = dyn_cast<FixedVectorType>(Ty);
  bool PacksBits = SrcVTy && SrcVTy->getElementType()->isIntegerTy(1);
  bool UnpacksBits = DstVTy && DstVTy->getElementType()->isIntegerTy(1);
  if ((PacksBits && !Ty->isIntegerTy()) ||
      (UnpacksBits && !SrcTy->isIntegerTy()))
    fatalError("unsupported i1 vector bitcast: " +
                       Twine(str(SrcTy)) + " to " + Twine(str(Ty)));

  Type *I1 = Type::getInt1Ty(Mod->getContext());
  if (PacksBits) {
    bool Wide = Ty->getIntegerBitWidth() > 32;
    out() << (Wide ? "\tldc.i8\t0\n" : "\tldc.i4.0\n");
    for (unsigned I = 0, E = SrcVTy->getNumElements(); I != E; ++I) {
      printAggregateLaneAddr(Src, I1, I);
      out() << "\tldind.u1\n\tldc.i4.1\n\tand\n";
      if (Wide)
        out() << "\tconv.u8\n";
      out() << "\tldc.i4\t" << I << "\n\tshl\n\tor\n";
    }
    return;
  }

  if (UnpacksBits) {
    bool Wide = SrcTy->getIntegerBitWidth() > 32;
    std::string Dst = newTempLocal(Ty);
    out() << "\tldloca\t'" << Dst << "'\n\tinitobj\t" << getTypeName(Ty)
          << "\n";
    for (unsigned I = 0, E = DstVTy->getNumElements(); I != E; ++I) {
      printAggregateLaneAddr(Dst, I1, I);
      out() << "\tldloc\t'" << Src << "'\n\tldc.i4\t" << I << "\n\tshr.un\n";
      if (Wide)
        out() << "\tconv.u4\n";
      out() << "\tldc.i4.1\n\tand\n\tstind.i1\n";
    }
    out() << "\tldloc\t'" << Dst << "'\n";
    return;
  }

  out() << "\tldloca\t'" << Src << "'\n\tconv.u8\n";
  if (movesAsBlob(Ty))
    out() << "\tldobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
}
