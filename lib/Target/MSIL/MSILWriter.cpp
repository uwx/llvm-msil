//===-- MSILWriter.cpp - Library for converting LLVM code to MSIL ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This library converts LLVM IR to CIL text that `ilasm` can assemble.
//
// Model
// -----
//  * Scalars live in locals of a storage type wide enough for the value
//    (i8/i16/i1 -> int32, i32 -> int32, i64 -> int64, ptr -> void*). Width is
//    normalized explicitly (conv.i1/i2) whenever a narrow value is stored or
//    compared, so no bits are lost and unsigned compares see the right bits.
//  * Pointers are unmanaged `void*`; the CLR never moves them, so IR pointer
//    arithmetic maps directly onto byte addresses.
//  * Aggregates and vectors are explicit-layout value classes, moved with
//    ldobj/stobj or plain local loads and addressed through a pointer cast.
//    There is no SIMD: vector arithmetic, shuffles and reductions are expanded
//    lane by lane.
//  * A `.data`-backed static field is read-only, so initialized globals are a
//    read-only template blob plus a writable heap pointer filled in by
//    $MSIL_Init at startup.
//  * Allocas become localloc in a single prologue.
//
//===----------------------------------------------------------------------===//

#include "MSILWriter.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/CodeGen/IntrinsicLowering.h"
#include "llvm/Demangle/Demangle.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Operator.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace {
/// Render an IR type or constant for a diagnostic.
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

bool isWideInt(Type *Ty) { return Ty->isIntegerTy(128); }

/// Types that move with ldobj/stobj rather than ldind/stind.
bool movesAsBlob(Type *Ty) {
  return Ty->isAggregateType() || Ty->isVectorTy() || isWideInt(Ty);
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
} // namespace

// Naming the emitted class lets C# call into the module without reflection:
// `MSIL.Program.Main()`. Without it everything is a free-standing member of
// `<Module>`, which is legitimate IL but awkward to reference from C#.
static cl::opt<std::string> ExternClassName(
    "msil-class-name", cl::desc("Emit members inside this class instead of "
                                "<Module>, so C# can call them directly"),
    cl::value_desc("namespace.Name"), cl::init(""));

// A load or store whose address is `getelementptr inbounds %S, ptr %p, 0, N`
// becomes ldfld/stfld on the struct class, so the member is named rather than
// reached by adding an offset to a raw pointer. The pointer that goes on the
// stack is the GEP's *base*: ldfld adds the member offset itself, so handing
// it the already-offset pointer would add the offset a second time.
StructType *MSILWriter::structFieldOf(const Value *Ptr, unsigned &Field,
                                      const Value *&Base) {
  const auto *GEP = dyn_cast<GEPOperator>(Ptr);
  if (!GEP || GEP->getNumOperands() != 3)
    return nullptr;
  auto *STy = dyn_cast<StructType>(GEP->getSourceElementType());
  if (!STy || STy->isOpaque())
    return nullptr;
  // Indices must be [0, N]: a non-zero leading index would walk an array.
  auto *First = dyn_cast<ConstantInt>(GEP->getOperand(1));
  auto *Second = dyn_cast<ConstantInt>(GEP->getOperand(2));
  if (!First || !First->isZero() || !Second)
    return nullptr;
  Field = Second->getZExtValue();
  if (Field >= STy->getNumElements())
    return nullptr;
  Base = GEP->getPointerOperand();
  return STy;
}
// Binding external C functions to managed code rather than the CRT. Both are
// off by default, so the emitted module is unchanged unless asked.
static cl::opt<std::string> LibcSpec(
    "msil-libc",
    cl::desc("Bind external C functions to managed code instead of the Windows "
             "CRT. 'builtin' emits the platform-independent implementation into "
             "this module; '<assembly>[:<type>]' calls a type you supply "
             "(type defaults to MSIL.Libc). Functions the built-in does not "
             "implement, and all vararg functions, still pinvoke the CRT."),
    cl::value_desc("builtin|assembly[:type]"), cl::init(""));

static cl::opt<std::string> AllocSpec(
    "msil-alloc",
    cl::desc("Route malloc/free/calloc/realloc to methods of the same name on "
             "this type, so unmanaged allocations can be instrumented"),
    cl::value_desc("assembly:type"), cl::init(""));

// On by default: a pinvoke into the CRT is the one thing that ties the output
// to Windows, and System.Math covers nearly every libm call a C program makes.
// Off for a target that must match a specific CRT's rounding bit-for-bit.
static cl::opt<bool> MathBind(
    "msil-math", cl::desc("Bind libm calls to System.Math/System.MathF "
                          "instead of pinvoking the C runtime"),
    cl::init(true));

// A symbol this module imports from a native library other than the CRT. The
// CRT split in getLibraryFor is a name heuristic, which cannot know about a
// third-party DLL, so these are consulted first.
static cl::list<std::string> LibMapSpec(
    "msil-lib-map",
    cl::desc("Bind matching imported symbols to a native library, e.g. "
             "'SDL_*=SDL2.dll'. <pattern>=<dll>, '*' matches any suffix; "
             "checked before the C runtime heuristic"),
    cl::value_desc("pattern=dll"));

char MSILWriter::ID = 0;

MSILWriter::MSILWriter(raw_pwrite_stream &Sink)
    : ModulePass(ID), Out(&Sink) {}

//===----------------------------------------------------------------------===//
// Emission primitives
//===----------------------------------------------------------------------===//

void MSILWriter::printSimpleInstruction(const char *Inst, const char *Operand) {
  out() << '\t' << Inst;
  if (Operand)
    out() << '\t' << Operand;
  out() << '\n';
}

void MSILWriter::printPtrLoad(uint64_t N) {
  if (N <= 8)
    out() << "\tldc.i4." << N << '\n';
  else if (isInt<32>(N))
    out() << "\tldc.i4\t" << N << '\n';
  else
    out() << "\tldc.i8\t" << N << '\n';
}

void MSILWriter::printBinaryInstruction(const char *Name, const Value *Left,
                                        const Value *Right) {
  printValueLoad(Left);
  printValueLoad(Right);
  printSimpleInstruction(Name);
}

void MSILWriter::printNot() { out() << "\tldc.i4.0\n\tceq\n"; }

//===----------------------------------------------------------------------===//
// Names
//===----------------------------------------------------------------------===//

std::string MSILWriter::getValueName(const Value *V) {
  if (const auto *F = dyn_cast<Function>(V))
    return F->getName().str();
  if (const auto *GV = dyn_cast<GlobalVariable>(V))
    return GV->getName().str();
  auto It = LocalIDs.find(V);
  assert(It != LocalIDs.end() && "value used before it was numbered");
  return "l" + utostr(It->second);
}

// "`Class'::" when members are wrapped in a class, "" otherwise. ilasm needs
// even intra-class helper references qualified once a class is involved, so
// every reference site goes through here.
std::string MSILWriter::ClassPrefix() const {
  return ExternClassName.empty() ? std::string()
                                 : "'" + ExternClassName + "'::";
}

// A helper called from $MSIL_Startup cannot be private once it lives inside a
// class, since <Module> is a different type.
const char *MSILWriter::HelperVis() const {
  return ExternClassName.empty() ? "private" : "assembly";
}

// A function moved into the emitted class is no longer a global member, so a
// reference to it from outside that class has to name the class.
std::string MSILWriter::getFunctionRef(const Function *F) const {
  if (ExternClassName.empty() || F->isDeclaration())
    return "'" + F->getName().str() + "'";
  return "'" + ExternClassName + "'::'" + F->getName().str() + "'";
}

std::string MSILWriter::getLabelName(const BasicBlock *BB) {
  auto It = LabelIDs.find(BB);
  assert(It != LabelIDs.end() && "block used before it was numbered");
  return "IL_" + utostr(It->second);
}

std::string MSILWriter::newTempLocal(Type *Ty) {
  std::string Name = "t" + utostr(getUniqID());
  ExtraLocals.emplace_back(getLocalType(Ty), Name);
  return Name;
}

//===----------------------------------------------------------------------===//
// Types
//===----------------------------------------------------------------------===//

std::string MSILWriter::getAggregateName(Type *Ty) {
  auto It = AggregateNames.find(Ty);
  if (It != AggregateNames.end())
    return It->second;
  std::string Name;
  if (auto *STy = dyn_cast<StructType>(Ty))
    Name = STy->hasName() ? STy->getName().str()
                          : "$anon" + utostr(AggregateTypes.size());
  else
    Name = "$agg" + utostr(AggregateTypes.size());
  AggregateNames[Ty] = Name;
  AggregateTypes.push_back(Ty);
  return Name;
}

std::string MSILWriter::getTypeName(Type *Ty) {
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:
    return "void";
  case Type::IntegerTyID: {
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 1)
      return "bool";
    if (N == 128)
      return Int128Ty;
    if (N <= 32)
      return "int" + utostr(N);
    // The CLR has no integer wider than 64 bits; wide IR integers (i33 from
    // overflow-avoiding transforms, i64, ...) use an int64 slot.
    return "int64";
  }
  case Type::FloatTyID:
    return "float32";
  case Type::DoubleTyID:
    return "float64";
  case Type::PointerTyID:
    return "void*";
  case Type::FixedVectorTyID:
    // A register-wide vector travels as its Vector<T>: the blob class is
    // still emitted, but only for the shapes the lane path handles.
    if (isSimdVectorTy(Ty))
      return getVectorTypeName(Ty);
    return "valuetype '" + getAggregateName(Ty) + "'";
  case Type::StructTyID:
  case Type::ArrayTyID:
    return "valuetype '" + getAggregateName(Ty) + "'";
  default:
    report_fatal_error("MSIL: unsupported type " + Twine(str(Ty)));
  }
}

std::string MSILWriter::getLocalType(Type *Ty) {
  if (Ty->isIntegerTy()) {
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 1)
      return "bool";
    if (N == 128)
      return Int128Ty;
    return N <= 32 ? "int32" : "int64";
  }
  // A register-wide vector is held in its Vector<T>, so every local of that
  // type is the SIMD register rather than a byte blob.
  if (isSimdVectorTy(Ty))
    return getVectorTypeName(Ty);
  return getTypeName(Ty);
}

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

namespace {
StringRef memSuffixOf(Type *Ty) {
  if (Ty->isPointerTy())
    return "i";
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    switch (Ty->getIntegerBitWidth()) {
    case 1:
    case 8:
      return "i1";
    case 16:
      return "i2";
    case 32:
      return "i4";
    case 64:
      return "i8";
    }
    break;
  case Type::FloatTyID:
    return "r4";
  case Type::DoubleTyID:
    return "r8";
  default:
    break;
  }
  report_fatal_error("MSIL: unsupported memory type");
}

/// conv opcode that normalizes an int on the stack to \p Ty's declared width.
const char *widthConv(Type *Ty) {
  unsigned N = Ty->getIntegerBitWidth();
  if (N == 1 || N == 8)
    return "conv.u1";
  if (N == 16)
    return "conv.u2";
  if (N > 32)
    return "conv.i8";
  return "conv.i4";
}
} // namespace

std::string MSILWriter::getMemSuffix(Type *Ty) {
  if (movesAsBlob(Ty))
    return "";
  return memSuffixOf(Ty).str();
}

// The Windows x64 calling convention passes a vararg floating-point value in
// both its SSE slot and a general-purpose slot, and the CRT's va_arg reads the
// general-purpose one. The CLR does not duplicate, so such arguments cross the
// pinvoke boundary as their raw bit pattern in an int64.
std::string MSILWriter::getVarArgTypeName(Type *Ty) {
  return Ty->isFloatingPointTy() ? "int64" : getTypeName(Ty);
}

std::string MSILWriter::getCallSignature(FunctionType *FTy, const CallBase *CB,
                                         StringRef Name) {
  std::string Sig;
  if (FTy->isVarArg())
    Sig += "vararg ";
  Sig += getTypeName(FTy->getReturnType());
  Sig += " ";
  Sig += Name.str();
  Sig += "(";
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
    if (I)
      Sig += ", ";
    Sig += getTypeName(FTy->getParamType(I));
  }
  // A vararg call site must name the trailing argument types: the CLR needs
  // them to know how many bytes the callee pops.
  if (FTy->isVarArg() && CB) {
    unsigned Fixed = FTy->getNumParams();
    if (CB->arg_size() > Fixed) {
      Sig += ", ..., ";
      for (unsigned I = Fixed, E = CB->arg_size(); I != E; ++I) {
        if (I != Fixed)
          Sig += ", ";
        Sig += getVarArgTypeName(CB->getArgOperand(I)->getType());
      }
    }
  }
  Sig += ")";
  return Sig;
}

//===----------------------------------------------------------------------===//
// Value loading
//===----------------------------------------------------------------------===//

void MSILWriter::printValueLoad(const Value *V) {
  if (const auto *GV = dyn_cast<GlobalVariable>(V)) {
    // Both external and internal globals are fields holding a pointer to the
    // variable's storage.
    out() << "\tldsfld\tvoid* '" << GV->getName() << "'\n";
    return;
  }
  if (const auto *F = dyn_cast<Function>(V)) {
    out() << "\tldftn\t"
        << getCallSignature(F->getFunctionType(), nullptr,
                            "'" + getValueName(F) + "'")
        << "\n";
    return;
  }
  if (isa<Argument>(V))
    out() << "\tldarg\t'" << getValueName(V) << "'\n";
  else if (isa<Instruction>(V))
    out() << "\tldloc\t'" << getValueName(V) << "'\n";
  else if (const auto *CE = dyn_cast<ConstantExpr>(V))
    printConstantExpr(CE);
  else
    printConstLoad(cast<Constant>(V));
}

void MSILWriter::printValueSave(const Value *V) {
  Type *Ty = V->getType();
  // Store narrow integers in canonical form so unsigned loads are correct.
  // A value computed by a 32-bit IL op arrives as int32 even when its IR type
  // is i64, so always normalize to the declared width here. i128 is a value
  // type: conv has nothing to normalize, and the local already matches.
  if (Ty->isIntegerTy() && !Ty->isIntegerTy(32) && !isWideInt(Ty))
    out() << '\t' << widthConv(Ty) << '\n';
  if (isa<Argument>(V))
    out() << "\tstarg\t'" << getValueName(V) << "'\n";
  else
    out() << "\tstloc\t'" << getValueName(V) << "'\n";
}

void MSILWriter::printValueLoadAsInt(const Value *V, bool Signed) {
  printValueLoad(V);
  Type *Ty = V->getType();
  if (!Ty->isIntegerTy())
    return;
  unsigned N = Ty->getIntegerBitWidth();
  // i128 is stored as a value type, never as a stack int, so it has no
  // canonical form to restore and no conv applies.
  if (N == 32 || N == 64 || N == 128)
    return;
  if (N == 16)
    out() << (Signed ? "\tconv.i2\n" : "\tconv.u2\n");
  else
    out() << (Signed ? "\tconv.i1\n" : "\tconv.u1\n");
}

//===----------------------------------------------------------------------===//
// Constants
//===----------------------------------------------------------------------===//

void MSILWriter::printConstLoad(const Constant *C) {
  Type *Ty = C->getType();

  if (const auto *CI = dyn_cast<ConstantInt>(C)) {
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 1) {
      out() << "\tldc.i4." << (CI->isZero() ? "0" : "1") << '\n';
      return;
    }
    if (N == 128) {
      const APInt &V = CI->getValue();
      // Use the ctor only when the value needs both halves; op_Implicit is the
      // only way to build the common small case.
      if (V.isSignedIntN(64)) {
        out() << "\tldc.i8\t" << V.getSExtValue() << "\n\tcall\t" << Int128Ty
              << ' ' << Int128Ref << "::op_Implicit(int64)\n";
      } else {
        // .ctor(uint64 upper, uint64 lower): the halves are pushed in that
        // order, and ilasm reads ldc.i8 as signed.
        out() << "\tldc.i8\t" << (int64_t)V.getHiBits(64).getZExtValue()
              << "\n\tldc.i8\t" << (int64_t)V.getLoBits(64).getZExtValue()
              << "\n\tnewobj\tinstance void " << Int128Ty
              << "::.ctor(uint64, uint64)\n";
      }
      return;
    }
    out() << "\tldc." << (N <= 32 ? "i4" : "i8") << '\t' << CI->getSExtValue()
        << '\n';
    return;
  }

  if (const auto *FP = dyn_cast<ConstantFP>(C)) {
    const APFloat &V = FP->getValueAPF();
    SmallString<40> Str;
    // A NaN or an infinity has no decimal literal ilasm accepts, and its
    // hex form is silently read as zero; the bit-pattern form is the only one
    // that round-trips.
    if (V.isNaN() || V.isInfinity()) {
      const APInt Bits = V.bitcastToAPInt();
      Str = "(";
      for (unsigned I = 0; I < Bits.getBitWidth(); I += 8) {
        if (I)
          Str.push_back(' ');
        raw_svector_ostream(Str) << format_hex_no_prefix(
            Bits.extractBitsAsZExtValue(8, I), 2);
      }
      Str.push_back(')');
    } else {
      V.toString(Str, 0, 0);
      // ilasm needs a '.' or exponent to lex the literal as floating point.
      if (Str.find_first_of(".eE") == StringRef::npos)
        Str += ".0";
    }
    out() << '\t' << (Ty->isFloatTy() ? "ldc.r4" : "ldc.r8") << '\t' << Str
        << '\n';
    return;
  }

  if (isa<UndefValue>(C) || isa<ConstantPointerNull>(C) ||
      isa<ConstantAggregateZero>(C) || C->isNullValue()) {
    if (isWideInt(Ty)) {
      out() << "\tldc.i8\t0\n\tcall\t" << Int128Ty << ' ' << Int128Ref
            << "::op_Implicit(int64)\n";
      return;
    }
    if (movesAsBlob(Ty)) {
      std::string Tmp = newTempLocal(Ty);
      out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t" << getTypeName(Ty)
          << "\n\tldloc\t'" << Tmp << "'\n";
    } else {
      out() << "\tldc.i4.0\n";
    }
    return;
  }

  if (isa<GlobalVariable>(C) || isa<Function>(C)) {
    printValueLoad(C);
    return;
  }

  if (auto *CV = dyn_cast<ConstantVector>(C)) {
    // A non-splat ConstantVector (may hold poison/undef lanes, which stay 0).
    Type *ElemTy = CV->getType()->getElementType();
    uint64_t ElemSize = DL->getTypeAllocSize(ElemTy).getFixedValue();
    std::string Tmp = newTempLocal(CV->getType());
    out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t"
          << getTypeName(CV->getType()) << "\n";
    std::string Sfx = getMemSuffix(ElemTy);
    for (unsigned I = 0, E = CV->getType()->getNumElements(); I != E; ++I) {
      const Constant *Elt = CV->getAggregateElement(I);
      if (!Elt || isa<UndefValue>(Elt) || Elt->isNullValue())
        continue;
      out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
      printPtrLoad(I * ElemSize);
      out() << "\tconv.i8\n\tadd\n";
      printConstLoad(Elt);
      out() << "\tstind." << Sfx << "\n";
    }
    out() << "\tldloc\t'" << Tmp << "'\n";
    return;
  }

  if (const auto *CDV = dyn_cast<ConstantDataVector>(C)) {
    // Build the blob lane by lane in a scratch local.
    Type *ElemTy = CDV->getType()->getElementType();
    uint64_t ElemSize = DL->getTypeAllocSize(ElemTy).getFixedValue();
    std::string Tmp = newTempLocal(CDV->getType());
    out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t"
          << getTypeName(CDV->getType()) << "\n";
    std::string Sfx = getMemSuffix(ElemTy);
    for (unsigned I = 0, E = CDV->getNumElements(); I != E; ++I) {
      const Constant *Elt = CDV->getElementAsConstant(I);
      if (!Elt || isa<UndefValue>(Elt) || Elt->isNullValue())
        continue;
      out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
      printPtrLoad(I * ElemSize);
      out() << "\tconv.i8\n\tadd\n";
      printConstLoad(Elt);
      out() << "\tstind." << Sfx << "\n";
    }
    out() << "\tldloc\t'" << Tmp << "'\n";
    return;
  }

  if (C->getType()->isAggregateType() || C->getType()->isVectorTy()) {
    // Build the blob field by field in a scratch local.
    std::string Tmp = newTempLocal(C->getType());
    out() << "\tldloca\t'" << Tmp << "'\n\tinitobj\t" << getTypeName(C->getType())
          << "\n";
    // initobj zeroed the padding, so only the non-zero leaves are stored.
    std::function<void(const Constant *, uint64_t)> Store =
        [&](const Constant *V, uint64_t At) {
          Type *Ty = V->getType();
          if (isa<ConstantAggregateZero>(V) ||
              (V->isNullValue() && !Ty->isAggregateType()))
            return;
          if (auto *STy = dyn_cast<StructType>(Ty)) {
            const StructLayout *SL = DL->getStructLayout(STy);
            for (unsigned I = 0; I != V->getNumOperands(); ++I)
              Store(cast<Constant>(V->getOperand(I)),
                    At + SL->getElementOffset(I));
            return;
          }
          if (auto *ATy = dyn_cast<ArrayType>(Ty)) {
            uint64_t Sz = DL->getTypeAllocSize(ATy->getElementType())
                              .getFixedValue();
            for (unsigned I = 0; I != ATy->getNumElements(); ++I)
              Store(cast<Constant>(V->getOperand(I)), At + I * Sz);
            return;
          }
          if (auto *VTy = dyn_cast<VectorType>(Ty)) {
            uint64_t Sz = DL->getTypeAllocSize(VTy->getElementType())
                              .getFixedValue();
            for (unsigned I = 0, E = V->getNumOperands(); I != E; ++I)
              Store(cast<Constant>(V->getOperand(I)), At + I * Sz);
            return;
          }
          out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
          if (At) {
            printPtrLoad(At);
            out() << "\tconv.i8\n\tadd\n";
          }
          printConstLoad(V);
          if (movesAsBlob(Ty))
            out() << "\tstobj\t" << getTypeName(Ty) << "\n";
          else
            out() << "\tstind." << getMemSuffix(Ty) << "\n";
        };
    Store(C, 0);
    out() << "\tldloc\t'" << Tmp << "'\n";
    return;
  }

  report_fatal_error("MSIL: unsupported constant: " + Twine(str(C)));
}

void MSILWriter::printConstantExpr(const ConstantExpr *CE) {
  switch (CE->getOpcode()) {
  case Instruction::GetElementPtr:
    if (auto *GEP = cast<GEPOperator>(CE); GEP->getType()->isVectorTy())
      printVectorGep(GEP);
    else
      printGepInstruction(GEP);
    return;
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
    if (isWideInt(CE->getType()) || isWideInt(CE->getOperand(0)->getType())) {
      printCastInstruction(CE->getOpcode(), CE->getOperand(0), CE->getType());
      return;
    }
    printValueLoad(CE->getOperand(0));
    return;
  case Instruction::Add:
  case Instruction::Sub:
  case Instruction::Mul:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    // A 128-bit constant expression would need the method-call expansion; none
    // of these survive to the writer for i128 anyway, and the scalar path
    // below would emit raw IL opcodes that do not exist for it.
    if (isWideInt(CE->getType()))
      report_fatal_error("MSIL: unsupported 128-bit constant expression");
    // InstCombine rewrites GEPs into pointer-offset arithmetic, so pointer
    // math shows up as constant expressions, not just as casts.
    const char *Op = nullptr;
    switch (CE->getOpcode()) {
    case Instruction::Add: Op = "add"; break;
    case Instruction::Sub: Op = "sub"; break;
    case Instruction::Mul: Op = "mul"; break;
    case Instruction::Shl: Op = "shl"; break;
    case Instruction::LShr: Op = "shr.un"; break;
    case Instruction::AShr: Op = "shr"; break;
    case Instruction::And: Op = "and"; break;
    case Instruction::Or: Op = "or"; break;
    default: Op = "xor"; break;
    }
    printValueLoad(CE->getOperand(0));
    printValueLoad(CE->getOperand(1));
    out() << '\t' << Op << '\n';
    return;
  }
  default:
    printCastInstruction(CE->getOpcode(), CE->getOperand(0), CE->getType());
    return;
  }
}

//===----------------------------------------------------------------------===//
// Memory access
//===----------------------------------------------------------------------===//

void MSILWriter::printIndirectLoad(const Value *Ptr, Type *Ty) {
  unsigned Field = 0;
  const Value *Base = nullptr;
  if (StructType *STy = structFieldOf(Ptr, Field, Base)) {
    printValueLoad(Base);
    out() << "\tldfld " << getTypeName(STy->getElementType(Field)) << " '"
          << getAggregateName(STy) << "'::'f" << Field << "'\n";
    return;
  }
  printValueLoad(Ptr);
  if (movesAsBlob(Ty))
    out() << "\tldobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
}

void MSILWriter::printIndirectSave(const Value *Ptr, const Value *Val) {
  unsigned Field = 0;
  const Value *Base = nullptr;
  // stfld wants the address first, same as stind, so the order is unchanged.
  if (StructType *STy = structFieldOf(Ptr, Field, Base)) {
    printValueLoad(Base);
    printValueLoad(Val);
    out() << "\tstfld " << getTypeName(STy->getElementType(Field)) << " '"
          << getAggregateName(STy) << "'::'f" << Field << "'\n";
    return;
  }
  printValueLoad(Ptr);
  printValueLoad(Val);
  if (movesAsBlob(Val->getType()))
    out() << "\tstobj\t" << getTypeName(Val->getType()) << '\n';
  else
    out() << "\tstind." << getMemSuffix(Val->getType()) << '\n';
}

//===----------------------------------------------------------------------===//
// Casts
//===----------------------------------------------------------------------===//

void MSILWriter::printCastInstruction(unsigned Op, const Value *V, Type *Ty) {
  // i128 conversions are the value type's conversion operators; no IL conv
  // applies to a value type.
  Type *SrcTy = V->getType();
  if (isWideInt(Ty) || isWideInt(SrcTy)) {
    switch (Op) {
    case Instruction::Trunc:
      // op_Explicit(int64) keeps the low half, then narrow as usual.
      printValueLoad(V);
      out() << "\tcall\tint64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n";
      if (Ty->getIntegerBitWidth() <= 32)
        out() << '\t' << widthConv(Ty) << '\n';
      return;
    case Instruction::ZExt:
    case Instruction::PtrToInt:
    case Instruction::PtrToAddr:
      // A narrow or 64-bit value is held zero-extended on the stack, so conv.u8
      // is the whole zero-extension.
      printValueLoad(V);
      out() << "\tconv.u8\n\tcall\t" << Int128Ty << ' ' << Int128Ref
            << "::op_Implicit(uint64)\n";
      return;
    case Instruction::SExt:
      printValueLoadAsInt(V, true);
      out() << "\tconv.i8\n\tcall\t" << Int128Ty << ' ' << Int128Ref
            << "::op_Implicit(int64)\n";
      return;
    case Instruction::IntToPtr:
      printValueLoad(V);
      out() << "\tcall\tint64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n\tconv.i\n";
      return;
    case Instruction::UIToFP:
      printValueLoad(V);
      out() << "\tcall\tfloat64 " << UInt128Ref << "::op_Explicit(" << UInt128Ty
            << ")\n";
      if (Ty->isFloatTy())
        out() << "\tconv.r4\n";
      return;
    case Instruction::SIToFP:
      printValueLoad(V);
      out() << "\tcall\tfloat64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n";
      if (Ty->isFloatTy())
        out() << "\tconv.r4\n";
      return;
    case Instruction::FPToUI:
    case Instruction::FPToSI: {
      // The conversion operators take float64; the float32 case widens first.
      const char *T = Op == Instruction::FPToUI ? UInt128Ty : Int128Ty;
      printValueLoad(V);
      if (SrcTy->isFloatTy())
        out() << "\tconv.r8\n";
      out() << "\tcall\t" << T << ' ' << T << "::op_Explicit(float64)\n";
      return;
    }
    case Instruction::BitCast:
    case Instruction::AddrSpaceCast: {
      // A 128-bit bitcast is always to or from a same-size blob; move the
      // bytes through memory because the two value types cannot be assigned
      // to each other.
      std::string Tmp = newTempLocal(SrcTy);
      printValueLoad(V);
      out() << "\tstloc\t'" << Tmp << "'\n\tldloca\t'" << Tmp
            << "'\n\tconv.u8\n\tldobj\t" << getTypeName(Ty) << '\n';
      return;
    }
    default:
      report_fatal_error("MSIL: unsupported 128-bit cast: " +
                         Twine(Instruction::getOpcodeName(Op)));
    }
  }

  // Vector casts and bitcasts are byte-level moves between two blobs; the
  // scalar paths below would truncate the aggregate to a single value.
  if (auto *DstVTy = dyn_cast<VectorType>(Ty)) {
    if (Op == Instruction::BitCast && V->getType() == Ty) {
      printValueLoad(V);
      return;
    }
    if (auto *SrcVTy = dyn_cast<VectorType>(V->getType())) {
      if (SrcVTy->getElementCount() == DstVTy->getElementCount() &&
          SrcVTy->getElementType() != DstVTy->getElementType()) {
        printVectorCast(Op, V, Ty);
        return;
      }
    }
    if (Op == Instruction::BitCast) {
      printValueLoad(V);
      return;
    }
  }

  switch (Op) {
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast: {
    Type *SrcTy = V->getType();
    if (SrcTy == Ty)
      return;
    // IL is typed on the evaluation stack: `stloc` into a local of another
    // type converts, it does not reinterpret. Round-trip through memory of the
    // source type so the bits survive unchanged.
    if ((Ty->isFloatingPointTy() && SrcTy->isIntegerTy()) ||
        (Ty->isIntegerTy() && SrcTy->isFloatingPointTy())) {
      std::string Tmp = newTempLocal(SrcTy);
      printValueLoad(V);
      out() << "\tstloc\t'" << Tmp << "'\n\tldloca\t'" << Tmp
            << "'\n\tconv.u8\n\tldind." << getMemSuffix(Ty) << '\n';
      return;
    }
    printValueLoad(V);
    return;
  }

  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::PtrToAddr:
    printValueLoad(V);
    if (Ty->isIntegerTy(1))
      out() << "\tldc.i4.0\n\tcgt.un\n";
    else if (Ty->isIntegerTy(8))
      out() << "\tconv.u1\n";
    else if (Ty->isIntegerTy(16))
      out() << "\tconv.u2\n";
    else if (Ty->isIntegerTy(32))
      out() << "\tconv.u4\n";
    else
      out() << "\tconv.u8\n";
    return;

  case Instruction::Trunc:
    printValueLoad(V);
    // Truncate to the destination width (wide destinations saturate at i64).
    out() << "\tconv.u8\n";
    if (Ty->getIntegerBitWidth() <= 32)
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::ZExt:
    printValueLoad(V);
    // Zero-extend: mask off the source's high bits first.
    out() << "\tconv.u8\n";
    if (Ty->getIntegerBitWidth() <= 32)
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::SExt:
    printValueLoadAsInt(V, true);
    if (Ty->getIntegerBitWidth() > 32)
      out() << "\tconv.i8\n";
    else
      out() << "\tconv.i4\n";
    return;

  case Instruction::UIToFP:
    printValueLoad(V);
    out() << "\tconv.r.un\n";
    out() << (Ty->isFloatTy() ? "\tconv.r4\n" : "\tconv.r8\n");
    return;
  case Instruction::SIToFP:
    // Narrow integers are held zero-extended, so the sign must be restored
    // before the conversion or the value reads as a large positive number.
    printValueLoadAsInt(V, true);
    out() << (Ty->isFloatTy() ? "\tconv.r4\n" : "\tconv.r8\n");
    return;
  case Instruction::FPToUI:
    printValueLoad(V);
    out() << "\tconv.u4\n";
    if (Ty->isIntegerTy(64))
      out() << "\tconv.u8\n";
    else if (!Ty->isIntegerTy(32))
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::FPToSI:
    printValueLoad(V);
    out() << "\tconv.i4\n";
    if (Ty->isIntegerTy(64))
      out() << "\tconv.i8\n";
    else if (!Ty->isIntegerTy(32))
      out() << '\t' << widthConv(Ty) << '\n';
    return;
  case Instruction::FPExt:
    printValueLoad(V);
    out() << "\tconv.r8\n";
    return;
  case Instruction::FPTrunc:
    printValueLoad(V);
    out() << "\tconv.r4\n";
    return;
  default:
    report_fatal_error("MSIL: unsupported cast: " +
                       Twine(Instruction::getOpcodeName(Op)));
  }
}

//===----------------------------------------------------------------------===//
// GEP
//===----------------------------------------------------------------------===//

void MSILWriter::printGepInstruction(const User *GEP) {
  printValueLoad(GEP->getOperand(0));
  out() << "\tconv.u8\n";

  for (gep_type_iterator I = gep_type_begin(GEP), E = gep_type_end(GEP); I != E;
       ++I) {
    const Value *Idx = I.getOperand();
    if (StructType *STy = I.getStructTypeOrNull()) {
      unsigned Field = cast<ConstantInt>(Idx)->getZExtValue();
      uint64_t Off = DL->getStructLayout(STy)->getElementOffset(Field);
      if (Off) {
        printPtrLoad(Off);
        out() << "\tconv.i8\n\tadd\n";
      }
      continue;
    }

    uint64_t ElemSize =
        I.getSequentialElementStride(*DL).getFixedValue();
    if (const auto *CI = dyn_cast<ConstantInt>(Idx)) {
      const APInt &A = CI->getValue();
      if (A.isZero())
        continue;
      if (A.isNegative()) {
        printPtrLoad(A.abs().getZExtValue() * ElemSize);
        out() << "\tconv.i8\n\tsub\n";
      } else {
        printPtrLoad(A.getZExtValue() * ElemSize);
        out() << "\tconv.i8\n\tadd\n";
      }
      continue;
    }
    printValueLoad(Idx);
    out() << "\tconv.i8\n";
    printPtrLoad(ElemSize);
    out() << "\tconv.i8\n\tmul\n\tadd\n";
  }
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
    out() << "\tstind.i\n";
  }
  out() << "\tldloc\t'" << R << "'\n";
}

//===----------------------------------------------------------------------===//
// Comparisons
//===----------------------------------------------------------------------===//

// Lane-wise vector comparison: compare lane I of both blobs into lane I of the
// result (an i1 vector, one byte per lane).
void MSILWriter::printVectorCmp(unsigned Pred, const Value *L, const Value *R,
                                VectorType *ResTy, bool IsFP) {
  auto *SrcTy = cast<VectorType>(L->getType());
  Type *EltTy = SrcTy->getElementType();
  uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
  unsigned N = ResTy->getElementCount().getFixedValue();
  std::string A = newTempLocal(SrcTy), B = newTempLocal(SrcTy),
              Res = newTempLocal(ResTy);
  printValueLoad(L);
  out() << "\tstloc\t'" << A << "'\n";
  printValueLoad(R);
  out() << "\tstloc\t'" << B << "'\n";
  out() << "\tldloca\t'" << Res << "'\n\tinitobj\t"
        << getTypeName(ResTy) << "\n";
  for (unsigned I = 0; I < N; ++I) {
    printAggregateLaneAddr(Res, ResTy->getElementType(), I);
    printAggregateLaneAddr(A, EltTy, I * ElemSize);
    out() << "\tldind." << getMemSuffix(EltTy) << "\n";
    printAggregateLaneAddr(B, EltTy, I * ElemSize);
    out() << "\tldind." << getMemSuffix(EltTy) << "\n";
    if (IsFP)
      printFCmp(Pred, nullptr, nullptr, /*EmitOperands=*/false);
    else
      printICmp(Pred, nullptr, nullptr, /*EmitOperands=*/false);
    out() << "\tstind.i1\n";
  }
  out() << "\tldloc\t'" << Res << "'\n";
}

void MSILWriter::printICmp(unsigned Pred, const Value *L, const Value *R,
                           bool EmitOperands) {
  auto LoadL = [&] { if (EmitOperands) printValueLoad(L); };
  auto LoadR = [&] { if (EmitOperands) printValueLoad(R); };
  bool U = Pred == CmpInst::ICMP_ULT || Pred == CmpInst::ICMP_ULE ||
           Pred == CmpInst::ICMP_UGT || Pred == CmpInst::ICMP_UGE;
  if (EmitOperands) {
    printValueLoadAsInt(L, !U);
    printValueLoadAsInt(R, !U);
  }

  switch (Pred) {
  case CmpInst::ICMP_EQ:
    out() << "\tceq\n";
    return;
  case CmpInst::ICMP_NE:
    out() << "\tceq\n";
    printNot();
    return;
  case CmpInst::ICMP_ULT:
  case CmpInst::ICMP_SLT:
    out() << (U ? "\tclt.un\n" : "\tclt\n");
    return;
  case CmpInst::ICMP_UGT:
  case CmpInst::ICMP_SGT:
    out() << (U ? "\tcgt.un\n" : "\tcgt\n");
    return;
  case CmpInst::ICMP_ULE:
  case CmpInst::ICMP_SLE:
    out() << (U ? "\tcgt.un\n" : "\tcgt\n");
    printNot();
    return;
  case CmpInst::ICMP_UGE:
  case CmpInst::ICMP_SGE:
    out() << (U ? "\tclt.un\n" : "\tclt\n");
    printNot();
    return;
  default:
    llvm_unreachable("unknown icmp predicate");
  }
}

void MSILWriter::printFCmp(unsigned Pred, const Value *L, const Value *R,
                           bool EmitOperands) {
  auto LoadL = [&] { if (EmitOperands) printValueLoad(L); };
  auto LoadR = [&] { if (EmitOperands) printValueLoad(R); };
  // ceq is false when either operand is NaN; clt/cgt are "unordered or ...".
  auto Cmp = [&](const char *Op) {
    LoadL();
    LoadR();
    out() << '\t' << Op << '\n';
  };
  auto Ordered = [&](const char *Op) {
    // (L Op R) && (L == L) && (R == R)
    Cmp(Op);
    LoadL();
    LoadL();
    out() << "\tceq\n\tand\n";
    LoadR();
    LoadR();
    out() << "\tceq\n\tand\n";
  };
  auto Unordered = [&]() {
    LoadL();
    LoadL();
    out() << "\tceq\n";
    printNot();
    LoadR();
    LoadR();
    out() << "\tceq\n";
    printNot();
    out() << "\tor\n";
  };

  switch (Pred) {
  case CmpInst::FCMP_OEQ:
    Ordered("ceq");
    return;
  case CmpInst::FCMP_UNE:
    Cmp("ceq");
    printNot();
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_OLT:
    Ordered("clt");
    return;
  case CmpInst::FCMP_OGT:
    Ordered("cgt");
    return;
  case CmpInst::FCMP_OLE:
    Ordered("cgt");
    printNot();
    return;
  case CmpInst::FCMP_OGE:
    Ordered("clt");
    printNot();
    return;
  case CmpInst::FCMP_ULT:
    Cmp("clt");
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_UGT:
    Cmp("cgt");
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_ULE:
    Cmp("cgt");
    printNot();
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_UGE:
    Cmp("clt");
    printNot();
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_ORD:
    LoadL();
    LoadL();
    out() << "\tceq\n";
    LoadR();
    LoadR();
    out() << "\tceq\n\tand\n";
    return;
  case CmpInst::FCMP_UNO:
    Unordered();
    return;
  case CmpInst::FCMP_UEQ:
    Cmp("ceq");
    Unordered();
    out() << "\tor\n";
    return;
  case CmpInst::FCMP_ONE:
    Cmp("ceq");
    printNot();
    LoadL();
    LoadL();
    out() << "\tceq\n\tand\n";
    LoadR();
    LoadR();
    out() << "\tceq\n\tand\n";
    return;
  case CmpInst::FCMP_FALSE:
    out() << "\tldc.i4.0\n";
    return;
  case CmpInst::FCMP_TRUE:
    out() << "\tldc.i4.1\n";
    return;
  default:
    llvm_unreachable("unknown fcmp predicate");
  }
}

//===----------------------------------------------------------------------===//
// 128-bit integers
//===----------------------------------------------------------------------===//

// System.Int128 has no IL arithmetic: every operator is a static method on the
// value type. Operands are already the right value type on the stack, so each
// case is just "load, load, call".
namespace {
/// Reinterpret the Int128 on the stack as UInt128. Used wherever an operation
/// only exists on the unsigned type, or must be unsigned.
void emitAsUInt128(raw_ostream &Out) {
  Out << "\tcall\t" << UInt128Ty << ' ' << Int128Ref << "::op_Explicit("
      << Int128Ty << ")\n";
}

/// And back. The reverse conversion exists on UInt128, not Int128.
void emitFromUInt128(raw_ostream &Out) {
  Out << "\tcall\t" << Int128Ty << ' ' << UInt128Ref << "::op_Explicit("
      << UInt128Ty << ")\n";
}
} // namespace

void MSILWriter::printWideBinary(unsigned Op, const Value *L,
                                 const Value *R) {
  const char *Name;
  switch (Op) {
  case Instruction::Add:  Name = "op_Addition"; break;
  case Instruction::Sub:  Name = "op_Subtraction"; break;
  case Instruction::Mul:  Name = "op_Multiply"; break;
  case Instruction::UDiv:
  case Instruction::SDiv: Name = "op_Division"; break;
  case Instruction::URem:
  case Instruction::SRem: Name = "op_Modulus"; break;
  case Instruction::And:  Name = "op_BitwiseAnd"; break;
  case Instruction::Or:   Name = "op_BitwiseOr"; break;
  case Instruction::Xor:  Name = "op_ExclusiveOr"; break;
  case Instruction::Shl:  Name = "op_LeftShift"; break;
  case Instruction::LShr: Name = "op_UnsignedRightShift"; break;
  case Instruction::AShr: Name = "op_RightShift"; break;
  default:
    report_fatal_error("MSIL: unsupported 128-bit operation");
  }

  // Unsigned division and remainder have no Int128 method, so compute on
  // UInt128 and reinterpret back.
  if (Op == Instruction::UDiv || Op == Instruction::URem) {
    printValueLoad(L);
    emitAsUInt128(out());
    printValueLoad(R);
    emitAsUInt128(out());
    out() << "\tcall\t" << UInt128Ty << ' ' << UInt128Ref << "::" << Name
          << "(" << UInt128Ty << ", " << UInt128Ty << ")\n";
    emitFromUInt128(out());
    return;
  }

  printValueLoad(L);
  printValueLoad(R);
  // The shift methods take an int32 count and reduce it modulo 128, which is
  // LLVM's shl/lshr/ashr semantics for an out-of-range amount. The IR count is
  // i128 like the value, so it needs a conversion before conv.i4.
  bool Shift = Op == Instruction::Shl || Op == Instruction::LShr ||
               Op == Instruction::AShr;
  if (Shift) {
    if (isWideInt(R->getType()))
      out() << "\tcall\tint64 " << Int128Ref << "::op_Explicit(" << Int128Ty
            << ")\n";
    out() << "\tconv.i4\n";
  }
  out() << "\tcall\t" << Int128Ty << ' ' << Int128Ref << "::" << Name << "("
        << Int128Ty << ", " << (Shift ? "int32" : Int128Ty) << ")\n";
}

// Comparisons return bool from op_Equality/op_LessThan/.... The relational
// forms are strict, so <= and >= negate the mirror form, as printICmp does.
void MSILWriter::printWideICmp(unsigned Pred, const Value *L, const Value *R) {
  bool U = Pred == CmpInst::ICMP_ULT || Pred == CmpInst::ICMP_ULE ||
           Pred == CmpInst::ICMP_UGT || Pred == CmpInst::ICMP_UGE;
  const char *T = U ? UInt128Ty : Int128Ty;
  // <= and >= are the negation of the strict predicate on the same operands.
  unsigned Mirror = Pred;
  bool Negate = false;
  switch (Pred) {
  case CmpInst::ICMP_ULE: Mirror = CmpInst::ICMP_UGT; Negate = true; break;
  case CmpInst::ICMP_UGE: Mirror = CmpInst::ICMP_ULT; Negate = true; break;
  case CmpInst::ICMP_SLE: Mirror = CmpInst::ICMP_SGT; Negate = true; break;
  case CmpInst::ICMP_SGE: Mirror = CmpInst::ICMP_SLT; Negate = true; break;
  default: break;
  }
  const char *Name;
  switch (Mirror) {
  case CmpInst::ICMP_EQ:  Name = "op_Equality"; break;
  case CmpInst::ICMP_NE:  Name = "op_Inequality"; break;
  case CmpInst::ICMP_ULT:
  case CmpInst::ICMP_SLT: Name = "op_LessThan"; break;
  case CmpInst::ICMP_UGT:
  case CmpInst::ICMP_SGT: Name = "op_GreaterThan"; break;
  default:
    llvm_unreachable("unknown icmp predicate");
  }

  printValueLoad(L);
  if (U)
    emitAsUInt128(out());
  printValueLoad(R);
  if (U)
    emitAsUInt128(out());
  out() << "\tcall\tbool " << (U ? UInt128Ref : Int128Ref) << "::" << Name
        << "(" << T << ", " << T << ")\n";
  if (Negate)
    printNot();
}

//===----------------------------------------------------------------------===//
// Instructions
//===----------------------------------------------------------------------===//

void MSILWriter::printIntrinsicCall(const IntrinsicInst *II) {
  // printFunctions assigned a slot for every va_list before emission.
  auto ArgIter = [&](const Value *V) { return ArgIterators.lookup(V); };

  switch (II->getIntrinsicID()) {
  case Intrinsic::vastart: {
    // va_start is void: initialize the ArgIterator and store its address
    // back into the va_list the front end allocated.
    std::string Name = ArgIter(II->getArgOperand(0));
    out() << "\tldloca\t'" << Name << "'\n";
    out() << "\targlist\n";
    out() << "\tcall\tinstance void "
           "[mscorlib]System.ArgIterator::.ctor("
           "valuetype [mscorlib]System.RuntimeArgumentHandle)\n";
    printValueLoad(II->getArgOperand(0));
    out() << "\tldloca\t'" << Name << "'\n\tconv.u8\n\tstind.i\n";
    return;
  }
  case Intrinsic::vaend:
    return;
  case Intrinsic::vacopy: {
    std::string Dst = ArgIter(II->getArgOperand(0));
    // The source is a va_list pointer; copy the struct it points at.
    printValueLoad(II->getArgOperand(1));
    out() << "\tldobj\tvaluetype [mscorlib]System.ArgIterator\n";
    out() << "\tstloc\t'" << Dst << "'\n";
    return;
  }
  default:
    report_fatal_error("MSIL: intrinsic survived lowering: " +
                       II->getCalledFunction()->getName());
  }
}

void MSILWriter::printFunctionCall(const Value *FnVal, const CallBase *CB) {
  FunctionType *FTy = CB->getFunctionType();
  const Function *Callee = dyn_cast<Function>(FnVal);

  // Arguments must reach the pinvoke as the thunk declares them: a vararg
  // float travels as its int64 bit pattern (see getVarArgTypeName).
  unsigned Fixed = FTy->getNumParams();
  for (unsigned I = 0, E = CB->arg_size(); I != E; ++I) {
    const Value *Arg = CB->getArgOperand(I);
    printValueLoad(Arg);
    if (FTy->isVarArg() && I >= Fixed && Arg->getType()->isFloatingPointTy()) {
      // Reinterpret, do not convert. Storing an r8 into an int64 local through
      // stloc *converts* it, so go through memory instead: the bits of the
      // local are what the callee's va_arg reads.
      std::string Tmp = newTempLocal(Arg->getType());
      out() << "\tstloc\t'" << Tmp << "'\n\tldloca\t'" << Tmp
            << "'\n\tconv.u8\n\tldind.i8\n";
    }
  }

  if (!Callee) {
    // Indirect call: the target sits above the arguments on the stack.
    printValueLoad(FnVal);
    out() << "\tcalli\t" << getCallSignature(FTy, CB, "") << '\n';
    return;
  }

  if (Callee->isDeclaration()) {
    // Calls to external vararg functions bind to a concrete pinvoke thunk
    // whose signature lists the actual trailing argument types.
    auto It = PInvokeNames.find({Callee, getCallSignature(FTy, CB, "")});
    if (It != PInvokeNames.end()) {
      std::string Sig = getTypeName(FTy->getReturnType()) + " '" + It->second +
                        "'(";
      for (unsigned I = 0, E = CB->arg_size(); I != E; ++I) {
        if (I)
          Sig += ", ";
        Type *Ty = CB->getArgOperand(I)->getType();
        Sig += (FTy->isVarArg() && I >= Fixed) ? getVarArgTypeName(Ty)
                                              : getTypeName(Ty);
      }
      Sig += ")";
      out() << "\tcall\t" << Sig << '\n';
      return;
    }
  }

  out() << "\tcall\t"
      << getCallSignature(FTy, CB, getFunctionRef(Callee)) << '\n';
}

void MSILWriter::printCallInstruction(const CallBase *CB) {
  if (const auto *II = dyn_cast<IntrinsicInst>(CB)) {
    printIntrinsicCall(II);
    return;
  }
  printFunctionCall(CB->getCalledOperand(), CB);
}

void MSILWriter::printPHICopy(const BasicBlock *From, const BasicBlock *To) {
  // Phi copies are one parallel assignment: an incoming value may itself be a
  // phi of this block, and writing that local first would lose the old value.
  // Read every source before writing any destination.
  SmallVector<std::pair<std::string, const PHINode *>, 8> Copies;
  for (const PHINode &Phi : To->phis()) {
    const Value *V = Phi.getIncomingValueForBlock(From);
    if (isa<UndefValue>(V))
      continue;
    std::string Tmp = newTempLocal(Phi.getType());
    printValueLoad(V);
    out() << "\tstloc\t'" << Tmp << "'\n";
    Copies.push_back({Tmp, &Phi});
  }
  for (auto &C : Copies) {
    out() << "\tldloc\t'" << C.first << "'\n";
    printValueSave(C.second);
  }
}

void MSILWriter::printBranchInstruction(const BranchInst *Inst) {
  const BasicBlock *From = Inst->getParent();
  if (Inst->isUnconditional()) {
    printPHICopy(From, Inst->getSuccessor(0));
    out() << "\tbr\t" << getLabelName(Inst->getSuccessor(0)) << '\n';
    return;
  }

  const BasicBlock *T = Inst->getSuccessor(0);
  const BasicBlock *F = Inst->getSuccessor(1);
  bool TPhi = !T->phis().empty();
  bool FPhi = !F->phis().empty();

  printValueLoad(Inst->getCondition());

  if (!TPhi && !FPhi) {
    out() << "\tbrtrue\t" << getLabelName(T) << '\n';
    out() << "\tbr\t" << getLabelName(F) << '\n';
    return;
  }

  // Phi copies may only run on the edge actually taken, so each edge that
  // needs copies gets a private trampoline block.
  std::string TTramp = "IL_phi" + utostr(getUniqID());
  out() << "\tbrtrue\t" << TTramp << '\n';
  printPHICopy(From, F);
  out() << "\tbr\t" << getLabelName(F) << '\n';
  out() << TTramp << ":\n";
  printPHICopy(From, T);
  out() << "\tbr\t" << getLabelName(T) << '\n';
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
  const char *Ext = ZeroExtLanes ? (ElemSize == 1 ? "\tconv.u1\n"
                                                  : "\tconv.u2\n")
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
  // back to the lane loop.
  const char *Name = nullptr;
  bool Swap = false, Invert = false;
  if (IsFP) {
    switch (Pred) {
    case CmpInst::FCMP_OEQ: Name = "Equals"; break;
    case CmpInst::FCMP_OLT: Name = "LessThan"; break;
    case CmpInst::FCMP_OGT: Name = "GreaterThan"; break;
    case CmpInst::FCMP_OLE: Name = "GreaterThan"; Swap = true; break;
    case CmpInst::FCMP_OGE: Name = "LessThan"; Swap = true; break;
    default: return false;
    }
  } else {
    switch (Pred) {
    case CmpInst::ICMP_EQ: Name = "Equals"; break;
    case CmpInst::ICMP_NE: Name = "Equals"; Invert = true; break;
    case CmpInst::ICMP_SLT:
    case CmpInst::ICMP_ULT: Name = "LessThan"; break;
    case CmpInst::ICMP_SGT:
    case CmpInst::ICMP_UGT: Name = "GreaterThan"; break;
    case CmpInst::ICMP_SLE:
    case CmpInst::ICMP_ULE: Name = "GreaterThan"; Swap = true; break;
    case CmpInst::ICMP_SGE:
    case CmpInst::ICMP_UGE: Name = "LessThan"; Swap = true; break;
    default: return false;
    }
  }

  printValueLoad(Inst->getOperand(Swap ? 1 : 0));
  printValueLoad(Inst->getOperand(Swap ? 0 : 1));
  emitVecCall(out(), E, Name,
              std::string("valuetype ") + NV.str() + "<!!0>, valuetype " +
                  NV.str() + "<!!0>");
  if (Invert) {
    // A mask is all-ones where the comparison holds; xor flips every lane.
    printConstLoad(Constant::getAllOnesValue(OpTy));
    emitVecCall(out(), E, "op_ExclusiveOr",
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
      if (SrcElt->getIntegerBitWidth() <= 32)
        out() << '\t' << widthConv(SrcElt) << '\n';
      break;
    case Instruction::SExt:
      out() << "\tconv.i4\n";
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

//===----------------------------------------------------------------------===//
// Atomics
//===----------------------------------------------------------------------===//
//
// The CLR has no atomic instructions, but it has `System.Threading.Interlocked`
// for int32/int64/native int, the `volatile.` prefix for any access width, and
// `Monitor` for the rest. Three verified facts shape the lowering:
//
//  * `Interlocked` takes a managed `T&`. An unmanaged `void*` is accepted as
//    one, so pointers need no pinning or marshalling. This holds both on the
//    .NET Framework 4.0 `mscorlib` a bare exe binds to and on .NET 9.
//  * Only int32/int64/native-int have `Add`; `Exchange` and `CompareExchange`
//    add float32/float64. `And`/`Or` do not exist on .NET Framework, so those
//    use a CompareExchange loop rather than a call.
//  * Narrow integers have no overload at all. They are serialized on
//    `$MSIL::atomicLock`; NOTES-atomics.md records what that does and does
//    not guarantee.

StringRef MSILWriter::interlockedTypeName(Type *Ty) {
  if (Ty->isPointerTy())
    return "native int";
  switch (Ty->getTypeID()) {
  case Type::IntegerTyID:
    switch (Ty->getIntegerBitWidth()) {
    case 32:
      return "int32";
    case 64:
      return "int64";
    default:
      return "";
    }
  case Type::FloatTyID:
    return "float32";
  case Type::DoubleTyID:
    return "float64";
  default:
    return "";
  }
}

// `volatile.` is a prefix, not an opcode: it goes between the address and the
// ldind/stind it applies to. It is the only way to ask for an atomic access of
// a width Interlocked does not cover.
void MSILWriter::printAtomicLoad(const Value *Ptr, Type *Ty) {
  printValueLoad(Ptr);
  out() << "\tvolatile.\n";
  if (Ty->isAggregateType() || Ty->isVectorTy())
    out() << "\tldobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
}

void MSILWriter::printAtomicStore(const Value *Ptr, const Value *Val) {
  Type *Ty = Val->getType();
  printValueLoad(Ptr);
  printValueLoad(Val);
  out() << "\tvolatile.\n";
  if (Ty->isAggregateType() || Ty->isVectorTy())
    out() << "\tstobj\t" << getTypeName(Ty) << '\n';
  else
    out() << "\tstind." << getMemSuffix(Ty) << '\n';
}

// Run Body with the module's atomic lock held. `leave` is the only way out of a
// protected region, and the finally runs Monitor.Exit on both the normal and
// the exceptional path, so a throwing body cannot leak the lock.
void MSILWriter::printUnderAtomicLock(function_ref<void()> Body) {
  NeedsAtomicLock = true;
  std::string Lock = "t" + utostr(getUniqID());
  std::string Done = "IL_unlock" + utostr(getUniqID());
  ExtraLocals.emplace_back("object", Lock);
  out() << "\tldsfld\tobject '$MSIL::atomicLock'\n\tstloc\t'" << Lock
        << "'\n\tldloc\t'" << Lock << "'\n";
  out() << "\tcall\tvoid [mscorlib]System.Threading.Monitor::Enter(object)\n";
  out() << "\t.try\n\t{\n";
  Body();
  out() << "\tleave\t" << Done << "\n\t}\n\tfinally\n\t{\n";
  out() << "\tldloc\t'" << Lock << "'\n";
  out() << "\tcall\tvoid [mscorlib]System.Threading.Monitor::Exit(object)\n";
  out() << "\tendfinally\n\t}\n" << Done << ":\n";
}

// Leave `<bits of A> != <bits of B>` on the stack. IL has no bitwise float
// compare and ceq is false for NaN, so both values are re-read as integers.
// The CLR's own float CompareExchange is bitwise, which is what makes this the
// right test for "did the exchange happen".
void MSILWriter::printFloatBitsNe(const std::string &A, const std::string &B,
                                  Type *Ty) {
  Type *ITy = Ty->isFloatTy() ? Type::getInt32Ty(Ty->getContext())
                              : Type::getInt64Ty(Ty->getContext());
  std::string ABits = newTempLocal(ITy), BBits = newTempLocal(ITy);
  const char *Sfx = Ty->isFloatTy() ? "i4" : "i8";
  out() << "\tldloca\t'" << A << "'\n\tconv.u8\n\tldind." << Sfx
        << "\n\tstloc\t'" << ABits << "'\n";
  out() << "\tldloca\t'" << B << "'\n\tconv.u8\n\tldind." << Sfx
        << "\n\tstloc\t'" << BBits << "'\n";
  out() << "\tldloc\t'" << ABits << "'\n\tldloc\t'" << BBits << "'\n\tceq\n";
  printNot();
}

void MSILWriter::printAtomicRMW(const AtomicRMWInst *RMW) {
  Type *RTy = RMW->getType();
  AtomicRMWInst::BinOp Op = RMW->getOperation();
  StringRef ILTy = interlockedTypeName(RTy);

  // The forms C atomics actually produce: exchange, add, and sub (which
  // Interlocked spells as an add of the two's complement).
  if (!ILTy.empty() &&
      (Op == AtomicRMWInst::Xchg || Op == AtomicRMWInst::Add ||
       Op == AtomicRMWInst::Sub)) {
    printValueLoad(RMW->getPointerOperand());
    printValueLoad(RMW->getValOperand());
    if (Op == AtomicRMWInst::Sub)
      out() << "\tneg\n";
    out() << "\tcall\t" << ILTy << " [mscorlib]System.Threading.Interlocked::"
          << (Op == AtomicRMWInst::Xchg ? "Exchange" : "Add") << "(" << ILTy
          << "&, " << ILTy << ")\n";
    return;
  }

  // Everything else is a CompareExchange loop, which needs no overload beyond
  // the one for the value's own type.
  std::string Old = newTempLocal(RTy), Val = newTempLocal(RTy),
              New = newTempLocal(RTy);
  printValueLoad(RMW->getValOperand());
  out() << "\tstloc\t'" << Val << "'\n";

  auto Compute = [&] {
    if (Op == AtomicRMWInst::Xchg) {
      // The new value is simply the operand, with no old value read.
      out() << "\tldloc\t'" << Val << "'\n";
      return;
    }
    out() << "\tldloc\t'" << Old << "'\n\tldloc\t'" << Val << "'\n";
    switch (Op) {
    case AtomicRMWInst::Add:
    case AtomicRMWInst::FAdd:
      out() << "\tadd\n";
      return;
    case AtomicRMWInst::Sub:
    case AtomicRMWInst::FSub:
      out() << "\tsub\n";
      return;
    case AtomicRMWInst::And:
      out() << "\tand\n";
      return;
    case AtomicRMWInst::Or:
      out() << "\tor\n";
      return;
    case AtomicRMWInst::Xor:
      out() << "\txor\n";
      return;
    case AtomicRMWInst::Nand:
      out() << "\tand\n";
      printNot();
      return;
    default:
      report_fatal_error("MSIL: unsupported atomicrmw operation " +
                         Twine((unsigned)Op));
    }
  };

  if (ILTy.empty()) {
    // No Interlocked overload (i8/i16): the lock is what makes the
    // read-modify-write indivisible. `conv.uN` normalizes the loaded value to
    // the zero-extended form every other narrow value has.
    printUnderAtomicLock([&] {
      printValueLoad(RMW->getPointerOperand());
      out() << "\tldind." << getMemSuffix(RTy) << "\n\t" << widthConv(RTy)
            << "\n\tstloc\t'" << Old << "'\n";
      Compute();
      out() << "\tstloc\t'" << New << "'\n";
      printValueLoad(RMW->getPointerOperand());
      out() << "\tldloc\t'" << New << "'\n\tstind." << getMemSuffix(RTy)
            << "\n";
    });
    out() << "\tldloc\t'" << Old << "'\n";
    return;
  }

  std::string Prev = newTempLocal(RTy);
  std::string Loop = "IL_rmw" + utostr(getUniqID());
  out() << Loop << ":\n";
  printAtomicLoad(RMW->getPointerOperand(), RTy);
  out() << "\tstloc\t'" << Old << "'\n";
  Compute();
  out() << "\tstloc\t'" << New << "'\n";
  printValueLoad(RMW->getPointerOperand());
  out() << "\tldloc\t'" << New << "'\n\tldloc\t'" << Old << "'\n";
  out() << "\tcall\t" << ILTy << " [mscorlib]System.Threading.Interlocked::"
        << "CompareExchange(" << ILTy << "&, " << ILTy << ", " << ILTy
        << ")\n\tstloc\t'" << Prev << "'\n";
  if (RTy->isFloatingPointTy()) {
    printFloatBitsNe(Prev, Old, RTy);
    out() << "\tbrtrue\t" << Loop << "\n";
  } else {
    out() << "\tldloc\t'" << Prev << "'\n\tldloc\t'" << Old
          << "'\n\tbne.un\t" << Loop << "\n";
  }
  out() << "\tldloc\t'" << Old << "'\n";
}

void MSILWriter::printAtomicCmpXchg(const AtomicCmpXchgInst *CX) {
  Type *VTy = CX->getNewValOperand()->getType();
  StringRef ILTy = interlockedTypeName(VTy);
  std::string Old = newTempLocal(VTy);

  if (ILTy.empty()) {
    std::string Done = "IL_cx" + utostr(getUniqID());
    printUnderAtomicLock([&] {
      printValueLoad(CX->getPointerOperand());
      out() << "\tldind." << getMemSuffix(VTy) << "\n\t" << widthConv(VTy)
            << "\n\tstloc\t'" << Old << "'\n";
      // Both sides of the compare must be in the same form: `Old` was
      // zero-extended above, so the comparand has to be too.
      printValueLoad(CX->getCompareOperand());
      out() << '\t' << widthConv(VTy) << "\n\tldloc\t'" << Old
            << "'\n\tbne.un\t" << Done << "\n";
      printValueLoad(CX->getPointerOperand());
      printValueLoad(CX->getNewValOperand());
      out() << "\tstind." << getMemSuffix(VTy) << "\n" << Done << ":\n";
    });
  } else {
    printValueLoad(CX->getPointerOperand());
    printValueLoad(CX->getNewValOperand());
    printValueLoad(CX->getCompareOperand());
    out() << "\tcall\t" << ILTy << " [mscorlib]System.Threading.Interlocked::"
          << "CompareExchange(" << ILTy << "&, " << ILTy << ", " << ILTy
          << ")\n\tstloc\t'" << Old << "'\n";
  }

  // The result is { old, success }; success is re-derived from the same
  // comparison, and the struct's second field sits at the first's alloc size.
  std::string Res = newTempLocal(CX->getType());
  out() << "\tldloca\t'" << Res << "'\n\tconv.u8\n\tldloc\t'" << Old
        << "'\n\tstind." << getMemSuffix(VTy) << "\n";
  printAggregateLaneAddr(Res, Type::getInt8Ty(CX->getContext()),
                         DL->getTypeAllocSize(VTy).getFixedValue());
  // Compare in the same zero-extended form `Old` was stored in, or a negative
  // narrow comparand would never match.
  printValueLoad(CX->getCompareOperand());
  out() << '\t' << widthConv(VTy) << "\n\tldloc\t'" << Old
        << "'\n\tceq\n\tstind.i1\n";
  out() << "\tldloc\t'" << Res << "'\n";
}

// The lock is a single shared object, emitted only for a module that has a
// narrow atomic access. `<Module>.cctor` runs before any static method of the
// module, so it is initialized before the first atomic operation can reach it.
void MSILWriter::printAtomicLock() {
  if (!NeedsAtomicLock)
    return;
  out() << "\n.field static assembly object '$MSIL::atomicLock'\n";
  out() << "\n.method static void .cctor() cil managed\n{\n";
  out() << "\t.maxstack\t2\n";
  out() << "\tnewobj\tinstance void [mscorlib]System.Object::.ctor()\n";
  out() << "\tstsfld\tobject '$MSIL::atomicLock'\n\tret\n}\n";
}
void MSILWriter::printInstruction(const Instruction *Inst) {
  const Value *Op0 = Inst->getNumOperands() > 0 ? Inst->getOperand(0) : nullptr;
  const Value *Op1 = Inst->getNumOperands() > 1 ? Inst->getOperand(1) : nullptr;
  Type *Ty = Inst->getType();

  // A register-wide vector runs on Vector<T>; every other shape, and every
  // operation Vector<T> cannot express, keeps the lane-by-lane expansion.
  if (Ty->isVectorTy()) {
    if (tryPrintSimdBinary(Inst))
      return;
    if (Inst->getOpcode() == Instruction::Select && tryPrintSimdSelect(Inst))
      return;

    const char *Op = nullptr;
    bool ZeroExt = false, Shift = false;
    switch (Inst->getOpcode()) {
    case Instruction::Add: Op = "add"; break;
    case Instruction::Sub: Op = "sub"; break;
    case Instruction::Mul: Op = "mul"; break;
    case Instruction::And: Op = "and"; break;
    case Instruction::Or:  Op = "or"; break;
    case Instruction::Xor: Op = "xor"; break;
    case Instruction::UDiv: Op = "div.un"; ZeroExt = true; break;
    case Instruction::URem: Op = "rem.un"; ZeroExt = true; break;
    case Instruction::SDiv: Op = "div"; break;
    case Instruction::SRem: Op = "rem"; break;
    case Instruction::FAdd: Op = "add"; break;
    case Instruction::FSub: Op = "sub"; break;
    case Instruction::FMul: Op = "mul"; break;
    case Instruction::FDiv: Op = "div"; break;
    case Instruction::FRem: Op = "rem"; break;
    case Instruction::Shl: Op = "shl"; Shift = true; break;
    case Instruction::LShr: Op = "shr.un"; ZeroExt = Shift = true; break;
    case Instruction::AShr: Op = "shr"; Shift = true; break;
    default: break;
    }
    if (Op) {
      printVectorBinary(Op, Inst, ZeroExt, Shift);
      return;
    }
  }

  // i128 has no IL arithmetic; every operator is a static method call.
  if (isWideInt(Ty) && isWideBinaryOpcode(Inst->getOpcode())) {
    printWideBinary(Inst->getOpcode(), Op0, Op1);
    return;
  }

  switch (Inst->getOpcode()) {
  case Instruction::PHI:
  case Instruction::Alloca:
    return; // PHIs materialize on edges; allocas run in the prologue.
  case Instruction::Ret:
    if (Value *RV = cast<ReturnInst>(Inst)->getReturnValue())
      printValueLoad(RV);
    out() << "\tret\n";
    return;
  case Instruction::Br:
    printBranchInstruction(cast<BranchInst>(Inst));
    return;
  case Instruction::Unreachable:
    out() << "\tldstr\t\"unreachable\"\n";
    out() << "\tnewobj\tinstance void "
           "[mscorlib]System.Exception::.ctor(string)\n\tthrow\n";
    return;
  case Instruction::Add:
  case Instruction::FAdd:
    printBinaryInstruction("add", Op0, Op1);
    return;
  case Instruction::Sub:
  case Instruction::FSub:
    printBinaryInstruction("sub", Op0, Op1);
    return;
  case Instruction::Mul:
  case Instruction::FMul:
    printBinaryInstruction("mul", Op0, Op1);
    return;
  case Instruction::UDiv:
    printBinaryInstruction("div.un", Op0, Op1);
    return;
  case Instruction::SDiv:
  case Instruction::FDiv: {
    // Same reason as SIToFP: narrow operands are stored zero-extended.
    bool Sgn = cast<Instruction>(Inst)->getOpcode() == Instruction::SDiv;
    if (Sgn) {
      printValueLoadAsInt(Op0, true);
      printValueLoadAsInt(Op1, true);
      out() << "\tdiv\n";
      return;
    }
    printBinaryInstruction("div", Op0, Op1);
    return;
  }
  case Instruction::URem:
    printBinaryInstruction("rem.un", Op0, Op1);
    return;
  case Instruction::SRem:
  case Instruction::FRem: {
    bool Sgn = cast<Instruction>(Inst)->getOpcode() == Instruction::SRem;
    if (Sgn) {
      printValueLoadAsInt(Op0, true);
      printValueLoadAsInt(Op1, true);
      out() << "\trem\n";
      return;
    }
    printBinaryInstruction("rem", Op0, Op1);
    return;
  }
  case Instruction::And:
    printBinaryInstruction("and", Op0, Op1);
    return;
  case Instruction::Or:
    printBinaryInstruction("or", Op0, Op1);
    return;
  case Instruction::Xor:
    printBinaryInstruction("xor", Op0, Op1);
    return;
  case Instruction::Shl:
    printValueLoad(Op0);
    printValueLoad(Op1);
    out() << "\tconv.i4\n\tshl\n";
    return;
  case Instruction::LShr:
    printValueLoad(Op0);
    printValueLoad(Op1);
    out() << "\tconv.i4\n\tshr.un\n";
    return;
  case Instruction::AShr:
    printValueLoadAsInt(Op0, true);
    printValueLoad(Op1);
    out() << "\tconv.i4\n\tshr\n";
    return;
  case Instruction::ICmp:
    if (auto *RTy = dyn_cast<VectorType>(Ty))
      printVectorCmp(cast<ICmpInst>(Inst)->getPredicate(), Op0, Op1, RTy,
                     /*IsFP=*/false);
    else if (isWideInt(Op0->getType()))
      printWideICmp(cast<ICmpInst>(Inst)->getPredicate(), Op0, Op1);
    else
      printICmp(cast<ICmpInst>(Inst)->getPredicate(), Op0, Op1);
    return;
  case Instruction::FCmp:
    if (auto *RTy = dyn_cast<VectorType>(Ty))
      printVectorCmp(cast<FCmpInst>(Inst)->getPredicate(), Op0, Op1, RTy,
                     /*IsFP=*/true);
    else
      printFCmp(cast<FCmpInst>(Inst)->getPredicate(), Op0, Op1);
    return;
  case Instruction::Select: {
    if (auto *VTy = dyn_cast<VectorType>(Ty)) {
      // A vector predicate is a blob, not a boolean, so branch per lane.
      auto *CTy = cast<VectorType>(Inst->getOperand(0)->getType());
      Type *EltTy = VTy->getElementType();
      uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
      unsigned N = VTy->getElementCount().getFixedValue();
      std::string C = newTempLocal(CTy), T = newTempLocal(VTy),
                  R = newTempLocal(VTy);
      printValueLoad(Inst->getOperand(0));
      out() << "\tstloc\t'" << C << "'\n";
      printValueLoad(Inst->getOperand(1));
      out() << "\tstloc\t'" << T << "'\n";
      printValueLoad(Inst->getOperand(2));
      out() << "\tstloc\t'" << R << "'\n";
      std::string Sfx = getMemSuffix(EltTy);
      for (unsigned I = 0; I < N; ++I) {
        std::string L = "IL_sel" + utostr(getUniqID());
        printAggregateLaneAddr(C, CTy->getElementType(), I);
        out() << "\tldind.i1\n\tbrfalse\t" << L << "_e\n";
        printAggregateLaneAddr(R, EltTy, I * ElemSize);
        printAggregateLaneAddr(T, EltTy, I * ElemSize);
        out() << "\tldind." << Sfx << "\n\tstind." << Sfx << "\n" << L
              << "_e:\n";
      }
      out() << "\tldloc\t'" << R << "'\n";
      return;
    }
    std::string L = "IL_sel" + utostr(getUniqID());
    printValueLoad(Op0);
    out() << "\tbrtrue\t" << L << "_t\n";
    printValueLoad(Inst->getOperand(2));
    out() << "\tbr\t" << L << "_e\n";
    out() << L << "_t:\n";
    printValueLoad(Inst->getOperand(1));
    out() << L << "_e:\n";
    return;
  }
  case Instruction::Load: {
    const auto *LI = cast<LoadInst>(Inst);
    if (LI->isAtomic())
      printAtomicLoad(LI->getPointerOperand(), Ty);
    else
      printIndirectLoad(LI->getPointerOperand(), Ty);
    return;
  }
  case Instruction::Store: {
    const auto *SI = cast<StoreInst>(Inst);
    if (SI->isAtomic())
      printAtomicStore(SI->getPointerOperand(), SI->getValueOperand());
    else
      printIndirectSave(SI->getPointerOperand(), SI->getValueOperand());
    return;
  }
  case Instruction::GetElementPtr:
    if (auto *GEP = dyn_cast<GEPOperator>(Inst); GEP->getType()->isVectorTy())
      printVectorGep(GEP);
    else
      printGepInstruction(GEP);
    return;
  case Instruction::Trunc:
  case Instruction::ZExt:
  case Instruction::SExt:
  case Instruction::FPTrunc:
  case Instruction::FPExt:
  case Instruction::UIToFP:
  case Instruction::SIToFP:
  case Instruction::FPToUI:
  case Instruction::FPToSI:
  case Instruction::PtrToInt:
  case Instruction::IntToPtr:
  case Instruction::PtrToAddr:
  case Instruction::BitCast:
  case Instruction::AddrSpaceCast:
    printCastInstruction(Inst->getOpcode(), Op0,
                         cast<CastInst>(Inst)->getDestTy());
    return;
  case Instruction::Call:
  case Instruction::Invoke:
    printCallInstruction(cast<CallBase>(Inst));
    return;
  case Instruction::VAArg: {
    // The va_list is an unmanaged pointer to the ArgIterator. `call instance`
    // needs a managed pointer, so route it through a byref local.
    std::string Ref = "var" + utostr(getUniqID());
    ExtraLocals.push_back({"valuetype [mscorlib]System.ArgIterator&", Ref});
    printValueLoad(Op0);
    out() << "\tstloc\t'" << Ref << "'\n";
    out() << "\tldloc\t'" << Ref << "'\n";
    out() << "\tcall\tinstance typedref "
           "[mscorlib]System.ArgIterator::GetNextArg()\n";
    out() << "\trefanyval\t" << getTypeName(Ty) << "\n";
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
    return;
  }
  case Instruction::ExtractElement: {
    // Vectors are byte blobs: take the address of the element and load it.
    if (isWideInt(Ty))
      report_fatal_error("MSIL: unsupported 128-bit vector element");
    auto *VTy = cast<VectorType>(Inst->getOperand(0)->getType());
    uint64_t ElemSize = DL->getTypeAllocSize(VTy->getElementType())
                            .getFixedValue();
    std::string Tmp = newTempLocal(Inst->getOperand(0)->getType());
    printValueLoad(Inst->getOperand(0));
    out() << "\tstloc\t'" << Tmp << "'\n";
    out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
    printValueLoad(Inst->getOperand(1));
    out() << "\tconv.i8\n";
    printPtrLoad(ElemSize);
    out() << "\tconv.i8\n\tmul\n\tadd\n";
    out() << "\tldind." << getMemSuffix(Ty) << '\n';
    return;
  }
  case Instruction::InsertElement: {
    if (isWideInt(Inst->getOperand(1)->getType()))
      report_fatal_error("MSIL: unsupported 128-bit vector element");
    auto *VTy = cast<VectorType>(Inst->getOperand(0)->getType());
    uint64_t ElemSize = DL->getTypeAllocSize(VTy->getElementType())
                            .getFixedValue();
    std::string Tmp = newTempLocal(VTy);
    printValueLoad(Inst->getOperand(0));
    out() << "\tstloc\t'" << Tmp << "'\n";
    out() << "\tldloca\t'" << Tmp << "'\n\tconv.u8\n";
    printValueLoad(Inst->getOperand(2));
    out() << "\tconv.i8\n";
    printPtrLoad(ElemSize);
    out() << "\tconv.i8\n\tmul\n\tadd\n";
    printValueLoad(Inst->getOperand(1));
    out() << "\tstind." << getMemSuffix(Inst->getOperand(1)->getType())
        << "\n\tldloc\t'" << Tmp << "'\n";
    return;
  }
  case Instruction::ShuffleVector: {
    // Build the result lane by lane, reading each source lane through byte
    // pointers. Undef lanes are left zero.
    auto *SVI = cast<ShuffleVectorInst>(Inst);
    auto *SrcTy = cast<VectorType>(SVI->getOperand(0)->getType());
    auto *DstTy = cast<VectorType>(Ty);
    Type *EltTy = DstTy->getElementType();
    if (isWideInt(EltTy))
      report_fatal_error("MSIL: unsupported 128-bit vector element");
    uint64_t ElemSize = DL->getTypeAllocSize(EltTy).getFixedValue();
    uint64_t NumSrc = SrcTy->getElementCount().getFixedValue();
    // The mask lives on the instruction, not among its operands.
    ArrayRef<int> Mask = SVI->getShuffleMask();
    std::string SrcA = newTempLocal(SrcTy);
    std::string SrcB = newTempLocal(SrcTy);
    std::string Dst = newTempLocal(DstTy);
    printValueLoad(Inst->getOperand(0));
    out() << "\tstloc\t'" << SrcA << "'\n";
    printValueLoad(Inst->getOperand(1));
    out() << "\tstloc\t'" << SrcB << "'\n";
    out() << "\tldloca\t'" << Dst << "'\n\tinitobj\t" << getTypeName(DstTy)
        << "\n";
    unsigned NumDst = DstTy->getElementCount().getFixedValue();
    for (unsigned I = 0; I < NumDst; ++I) {
      // Poison/undef lanes (-1) are left zero.
      int M = Mask[I];
      if (M < 0)
        continue;
      if ((uint64_t)M >= 2 * NumSrc)
        continue;
      const std::string &Src = M < NumSrc ? SrcA : SrcB;
      uint64_t Lane = (uint64_t)M < NumSrc ? M : M - NumSrc;
      // destination lane address
      out() << "\tldloca\t'" << Dst << "'\n\tconv.u8\n";
      printPtrLoad(I * ElemSize);
      out() << "\tconv.i8\n\tadd\n";
      // source lane value
      out() << "\tldloca\t'" << Src << "'\n\tconv.u8\n";
      printPtrLoad(Lane * ElemSize);
      out() << "\tconv.i8\n\tadd\n\tldind." << getMemSuffix(EltTy) << "\n";
      out() << "\tstind." << getMemSuffix(EltTy) << "\n";
    }
    out() << "\tldloc\t'" << Dst << "'\n";
    return;
  }
  case Instruction::FNeg:
    // 0 - x negates every IEEE value exactly, -0 and NaN included. (IL has no
    // neg instruction; negation is normally done as 0 - x anyway.)
    printConstLoad(ConstantFP::getZero(Ty));
    printValueLoad(Op0);
    out() << "\tsub\n";
    return;
  case Instruction::Freeze:
    printValueLoad(Op0);
    return;
  case Instruction::ExtractValue: {
    // Aggregates are byte blobs, so take the address of the field.
    const auto *EV = cast<ExtractValueInst>(Inst);
    SmallVector<unsigned> Idx(EV->idx_begin(), EV->idx_end());
    Type *AggTy = EV->getAggregateOperand()->getType();
    uint64_t Off = DL->getStructLayout(cast<StructType>(AggTy))
                       ->getElementOffset(Idx[0])
                       .getFixedValue();
    for (unsigned I = 1; I < Idx.size(); ++I) {
      AggTy = cast<StructType>(AggTy)->getElementType(Idx[I - 1]);
      Off += DL->getStructLayout(cast<StructType>(AggTy))
                 ->getElementOffset(Idx[I])
                 .getFixedValue();
    }
    std::string Tmp = newTempLocal(EV->getAggregateOperand()->getType());
    printValueLoad(EV->getAggregateOperand());
    out() << "\tstloc\t'" << Tmp << "'\n";
    printAggregateLaneAddr(Tmp, Ty, Off);
    if (movesAsBlob(Ty))
      out() << "\tldobj\t" << getTypeName(Ty) << '\n';
    else
      out() << "\tldind." << getMemSuffix(Ty) << '\n';
    return;
  }
  case Instruction::InsertValue: {
    const auto *IV = cast<InsertValueInst>(Inst);
    SmallVector<unsigned> Idx(IV->idx_begin(), IV->idx_end());
    Type *AggTy = IV->getAggregateOperand()->getType();
    uint64_t Off = DL->getStructLayout(cast<StructType>(AggTy))
                       ->getElementOffset(Idx[0])
                       .getFixedValue();
    for (unsigned I = 1; I < Idx.size(); ++I) {
      AggTy = cast<StructType>(AggTy)->getElementType(Idx[I - 1]);
      Off += DL->getStructLayout(cast<StructType>(AggTy))
                 ->getElementOffset(Idx[I])
                 .getFixedValue();
    }
    std::string Tmp = newTempLocal(IV->getAggregateOperand()->getType());
    printValueLoad(IV->getAggregateOperand());
    out() << "\tstloc\t'" << Tmp << "'\n";
    printAggregateLaneAddr(Tmp, Ty, Off);
    printValueLoad(IV->getInsertedValueOperand());
    if (movesAsBlob(Ty))
      out() << "\tstobj\t" << getTypeName(Ty) << '\n';
    else
      out() << "\tstind." << getMemSuffix(Ty) << '\n';
    out() << "\tldloc\t'" << Tmp << "'\n";
    return;
  }
  case Instruction::Fence:
    // Every CLR atomic access is already a full barrier (volatile. is
    // release/acquire, Interlocked is seq_cst), so a standalone fence has
    // nothing left to order. The call is kept so the ordering is not merely
    // assumed: it is what the JIT emits for a seq_cst fence.
    out() << "\tcall\tvoid [mscorlib]System.Threading.Interlocked::"
           "MemoryBarrier()\n";
    return;
  case Instruction::AtomicRMW:
    printAtomicRMW(cast<AtomicRMWInst>(Inst));
    return;
  case Instruction::AtomicCmpXchg:
    printAtomicCmpXchg(cast<AtomicCmpXchgInst>(Inst));
    return;
  default:
    report_fatal_error("MSIL: unsupported instruction " +
                       Twine(Inst->getOpcodeName()));
  }
}

//===----------------------------------------------------------------------===//
// Functions
//===----------------------------------------------------------------------===//

void MSILWriter::printFunction(const Function &F) {
  FunctionType *FTy = F.getFunctionType();

  // The evaluation stack never holds more than one value per argument of the
  // widest call, plus the addresses a vector expansion materialises at once
  // (three, each needing a two-value address computation on top). Deriving this
  // from the IR keeps the bound tight: maxstack is reserved per frame, and a
  // method that recurses deeply blows the thread stack when it is inflated.
  unsigned MaxOperands = 8;
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB) {
      unsigned N = I.getNumOperands() + 1;
      if (const auto *CB = dyn_cast<CallBase>(&I))
        N = std::max<unsigned>(N, CB->arg_size() + 1);
      if (I.getType()->isVectorTy())
        N += 6;
      MaxOperands = std::max(MaxOperands, N);
    }
  MaxOperands += 8;

  out() << "\n.method static " << (F.hasLocalLinkage() ? "private " : "public ")
      << "hidebysig " << (FTy->isVarArg() ? "vararg " : "")
      << getTypeName(F.getReturnType()) << " '" << getValueName(&F) << "'(";
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
    if (I)
      out() << ", ";
    out() << getTypeName(FTy->getParamType(I)) << " '"
        << getValueName(F.getArg(I)) << "'";
  }
  out() << ") cil managed\n{\n";

  // Emit the body into a buffer first: instruction emission can add
  // temporaries (vector bitcast/shuffle slots) that must be declared before
  // the body starts.
  raw_ostream *FileOut = Out;
  std::string BodyStr;
  raw_string_ostream Body(BodyStr);
  Out = &Body;

  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB)
      if (const auto *AI = dyn_cast<AllocaInst>(&I)) {
        uint64_t Size =
            DL->getTypeAllocSize(AI->getAllocatedType()).getFixedValue();
        // localloc takes a native int; a 32-bit size would make it read the
        // size from the wrong width on a 64-bit CLR.
        if (const auto *CI = dyn_cast<ConstantInt>(AI->getArraySize()))
          out() << "\tldc.i8\t" << CI->getZExtValue() * Size << "\n";
        else {
          printValueLoad(AI->getArraySize());
          out() << "\tconv.i8\n";
          out() << "\tldc.i8\t" << Size << "\n\tmul\n";
        }
        out() << "\tlocalloc\n\tstloc\t'" << getValueName(AI) << "'\n";
      }

  for (const BasicBlock &BB : F) {
    out() << getLabelName(&BB) << ":\n";
    for (const Instruction &I : BB) {
      out() << "\n// " << I << '\n';
      printInstruction(&I);
      if (I.getType()->isVoidTy() || isa<PHINode>(&I) || isa<AllocaInst>(&I) ||
          I.isTerminator())
        continue;
      printValueSave(&I);
    }
  }
  Body.flush();
  Out = FileOut;

  // A call pushes one value per argument, fewer than everything counted below
  // (arguments + results), so this bound covers the whole body. The count must
  // stay near the true maximum: an inflated maxstack is reserved per frame and
  // deep recursion then overflows the thread stack.
  out() << "\t.maxstack\t" << MaxOperands << '\n';

  for (const Argument &A : F.args())
    out() << "\t.locals init (" << getLocalType(A.getType()) << " '"
        << getValueName(&A) << "')\n";
  for (const BasicBlock &BB : F)
    for (const Instruction &I : BB)
      if (!I.getType()->isVoidTy())
        out() << "\t.locals init (" << getLocalType(I.getType()) << " '"
            << getValueName(&I) << "')\n";
  for (auto &Extra : ExtraLocals)
    out() << "\t.locals init (" << Extra.first << " '" << Extra.second << "')\n";

  out() << BodyStr;
  out() << "}\n";
}

void MSILWriter::printFunctions() {
  for (Function &F : *Mod) {
    if (F.isDeclaration())
      continue;
    LocalIDs.clear();
    LabelIDs.clear();
    ExtraLocals.clear();
    ArgIterators.clear();
    NextLocalID = 0;
    NextLabelID = 0;

    for (const Argument &A : F.args())
      LocalIDs[&A] = NextLocalID++;
    for (const BasicBlock &BB : F) {
      LabelIDs[&BB] = NextLabelID++;
      for (const Instruction &I : BB)
        if (!I.getType()->isVoidTy())
          LocalIDs[&I] = NextLocalID++;
    }

    // Anticipate va_list slots so declarations precede their uses.
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB) {
        const auto *CB = dyn_cast<CallBase>(&I);
        if (!CB)
          continue;
        const auto *II = dyn_cast<IntrinsicInst>(CB);
        if (!II)
          continue;
        switch (II->getIntrinsicID()) {
        case Intrinsic::vastart:
        case Intrinsic::vacopy: {
          const Value *V = II->getArgOperand(0);
          if (!ArgIterators.count(V)) {
            std::string Name = "va" + utostr(NumVA++);
            ArgIterators[V] = Name;
            ExtraLocals.push_back(
                {"valuetype [mscorlib]System.ArgIterator", Name});
          }
          break;
        }
        default:
          break;
        }
      }

    printFunction(F);
  }
}

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
  if (!ArgTy->isFloatTy() && !ArgTy->isDoubleTy())
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
  case Intrinsic::strip_invariant_group:
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
    Value *X = CI->getArgOperand(0);
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
    CI->replaceAllUsesWith(R);
    CI->eraseFromParent();
    return true;
  }
  default:
    return false;
  }
}

void MSILWriter::lowerIntrinsics(Function &F) {
  // A vararg function whose body merely forwards to a CRT internal (e.g. the
  // `printf` wrapper clang emits from MSVC's stdio.h) cannot work: it passes a
  // va_list that only exists in the native ABI. Drop the definition so calls
  // bind to the real exported function through a pinvoke thunk instead.
  if (F.isVarArg() && !F.isDeclaration()) {
    bool ForwardsToCRT = false;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB)
        if (auto *CB = dyn_cast<CallBase>(&I))
          if (auto *Callee = CB->getCalledFunction())
            if (Callee->getName().starts_with("__stdio_common_") ||
                Callee->getName().starts_with("__acrt_"))
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
        Function *Callee = CI->getCalledFunction();
        if (!Callee || !Callee->isIntrinsic())
          continue;
        switch (Callee->getIntrinsicID()) {
        case Intrinsic::vastart:
        case Intrinsic::vaend:
        case Intrinsic::vacopy:
          break; // Emitted directly.
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
        Type *Wide = B.getIntNTy(N * 2);
        Value *WA = Signed ? B.CreateSExt(A, Wide) : B.CreateZExt(A, Wide);
        Value *WC = Signed ? B.CreateSExt(C, Wide) : B.CreateZExt(C, Wide);
        bool Mul = RID == Intrinsic::umul_with_overflow ||
                   RID == Intrinsic::smul_with_overflow;
        Value *W = Mul ? B.CreateMul(WA, WC)
                       : Sub ? B.CreateSub(WA, WC) : B.CreateAdd(WA, WC);
        Value *Res = B.CreateTrunc(W, IT);
        Value *WRes = Signed ? B.CreateSExt(Res, Wide) : B.CreateZExt(Res, Wide);
        Value *Overflow = B.CreateICmpNE(W, WRes);
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
        // Thread-locals are ordinary globals here: a .NET assembly built this
        // way has one thread, and globals are already heap-indirected.
        // shortcut: no real [ThreadStatic]/TLS storage; upgrade if a program
        // is ever run multithreaded.
        CI->replaceAllUsesWith(CI->getArgOperand(0));
        CI->eraseFromParent();
        break;
      case Intrinsic::fabs: {
        // Clear the sign bit; a compare-and-negate would be wrong for NaN.
        IRBuilder<> B(CI);
        Value *X = CI->getArgOperand(0);
        Type *IT = B.getIntNTy(X->getType()->getPrimitiveSizeInBits());
        Value *Bits = B.CreateBitCast(X, IT);
        Value *Mask = ConstantInt::get(
            IT, APInt::getSignedMaxValue(IT->getIntegerBitWidth()));
        CI->replaceAllUsesWith(B.CreateBitCast(
            B.CreateAnd(Bits, Mask), X->getType()));
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
        Type *Wide = B.getIntNTy(N * 2);
        Value *WA = B.CreateSExt(A, Wide);
        Value *WC = B.CreateSExt(C, Wide);
        Value *W = nullptr;
        bool Signed = Callee->getIntrinsicID() == Intrinsic::ssub_sat ||
                      Callee->getIntrinsicID() == Intrinsic::sadd_sat;
        bool Sub = Callee->getIntrinsicID() == Intrinsic::usub_sat ||
                   Callee->getIntrinsicID() == Intrinsic::ssub_sat;
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
      default:
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
                break;
              }
    if (!Remaining)
      break;
  }
}

//===----------------------------------------------------------------------===//
// Module emission
//===----------------------------------------------------------------------===//

void MSILWriter::collectType(Type *Ty) {
  if (!Ty)
    return;
  if (isWideInt(Ty))
    NeedsSystemRuntime = true;
  if (auto *STy = dyn_cast<StructType>(Ty)) {
    if (AggregateNames.count(Ty))
      return;
    getAggregateName(Ty);
    for (Type *Elt : STy->elements())
      collectType(Elt);
    return;
  }
  if (auto *ATy = dyn_cast<ArrayType>(Ty)) {
    if (AggregateNames.count(Ty))
      return;
    getAggregateName(Ty);
    collectType(ATy->getElementType());
    return;
  }
  if (auto *VTy = dyn_cast<VectorType>(Ty)) {
    // IL has no SIMD instructions; model a vector as a byte blob.
    if (!AggregateNames.count(Ty))
      getAggregateName(Ty);
    collectType(VTy->getElementType());
  }
}

void MSILWriter::collectConstant(const Constant *C) {
  // A global can appear inside its own initializer (`getelementptr @g`), and a
  // global is an operand of its own initializer, so descending into one here
  // recurses forever. Every global's type is collected by collectTypes anyway.
  if (!C || isa<GlobalVariable>(C))
    return;
  collectType(C->getType());
  for (const Value *Op : C->operands())
    if (const auto *OC = dyn_cast<Constant>(Op))
      collectConstant(OC);
}

/// True when some value, local or signature in \p M is a register-wide vector
/// and therefore names Vector<T>. Asked before any body is emitted, since the
/// assembly reference has to precede every type reference.
bool MSILWriter::vectorNeeded(Module &M) {
  for (Function &F : M) {
    if (vectorTypeIn(F.getFunctionType()))
      return true;
    if (F.isDeclaration())
      continue;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        if (vectorTypeIn(I.getType()))
          return true;
        for (const Use &U : I.operands())
          if (vectorTypeIn(U->getType()))
            return true;
      }
  }
  for (GlobalVariable &G : M.globals())
    if (vectorTypeIn(G.getValueType()))
      return true;
  return false;
}

/// True when \p Ty is or contains a register-wide vector type.
bool MSILWriter::vectorTypeIn(Type *Ty) {
  if (!Ty)
    return false;
  if (isSimdVectorTy(Ty))
    return true;
  if (auto *FTy = dyn_cast<FunctionType>(Ty)) {
    if (vectorTypeIn(FTy->getReturnType()))
      return true;
    for (Type *P : FTy->params())
      if (vectorTypeIn(P))
        return true;
    return false;
  }
  if (auto *STy = dyn_cast<StructType>(Ty))
    for (Type *E : STy->elements())
      if (vectorTypeIn(E))
        return true;
  if (auto *ATy = dyn_cast<ArrayType>(Ty))
    return vectorTypeIn(ATy->getElementType());
  return false;
}

void MSILWriter::collectTypes() {
  for (Function &F : *Mod) {
    collectType(F.getReturnType());
    for (Argument &A : F.args())
      collectType(A.getType());
    if (F.isDeclaration())
      continue;
    for (BasicBlock &BB : F)
      for (Instruction &I : BB) {
        collectType(I.getType());
        // Type-indexed operands (GEP source element, alloca) need classes too.
        if (auto *GEP = dyn_cast<GetElementPtrInst>(&I))
          collectType(GEP->getSourceElementType());
        if (auto *AI = dyn_cast<AllocaInst>(&I))
          collectType(AI->getAllocatedType());
      }
  }
  for (GlobalVariable &G : Mod->globals()) {
    collectType(G.getValueType());
    // Globals get a template blob class, so even scalar globals need one.
    if (!G.isDeclaration()) {
      getAggregateName(G.getValueType());
      collectConstant(G.getInitializer());
    }
  }
  // Vector constants materialize into scratch locals, which need a class.
  for (Function &F : *Mod)
    for (BasicBlock &BB : F)
      for (Instruction &I : BB)
        for (Value *Op : I.operands())
          if (auto *CV = dyn_cast<Constant>(Op))
            if (CV->getType()->isVectorTy())
              getAggregateName(CV->getType());
}

void MSILWriter::printAggregateDeclarations() {
  // getAggregateName may append to AggregateTypes for a nested type that was
  // not collected up front, so index rather than hold iterators.
  for (size_t I = 0; I < AggregateTypes.size(); ++I) {
    Type *Ty = AggregateTypes[I];
    std::string N = getAggregateName(Ty);
    uint64_t S = DL->getTypeAllocSize(Ty).getFixedValue();
    // Vectors are addressed one element per byte-sized slot, but the data
    // layout packs an i1 vector into ceil(N/8) bytes; size the class for the
    // lane addressing the emitter actually uses.
    // shortcut: an i1 vector nested inside a struct still follows the packed
    // layout, so it must not be mixed with scalar members that follow it.
    if (auto *VTy = dyn_cast<VectorType>(Ty))
      S = VTy->getElementCount().getFixedValue() *
          DL->getTypeAllocSize(VTy->getElementType()).getFixedValue();

    // A struct's members are declared at the offsets the LLVM data layout
    // chose, so a field can be reached with ldfld/stfld instead of pointer
    // arithmetic. Naming them keeps the layout in one place: the same numbers
    // drive both the declaration and the access.
    if (auto *STy = dyn_cast<StructType>(Ty)) {
      if (!STy->isOpaque()) {
        const StructLayout *SL = DL->getStructLayout(STy);
        out() << ".class value explicit ansi sealed '" << N << "'\n{\n"
              << "\t.pack 1\n\t.size " << S << "\n";
        for (unsigned F = 0; F < STy->getNumElements(); ++F)
          out() << "\t.field [" << SL->getElementOffset(F).getFixedValue()
                << "] public " << getTypeName(STy->getElementType(F)) << " 'f"
                << F << "'\n";
        out() << "}\n";
        continue;
      }
    }
    out() << ".class value explicit ansi sealed '" << N << "' { .pack 1 .size "
        << S << " }\n";
  }
  out() << '\n';
}

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
  };
  for (auto &R : PosixRenames)
    if (Name == R[0])
      return R[1];
  return Name;
}

namespace {
/// The set the built-in libc implements; a symbol outside it keeps its pinvoke.
bool isBuiltinSymbol(StringRef N) {
  static const char *const Names[] = {
      "malloc",  "free",    "calloc",  "realloc", "memcpy", "memmove",
      "memset",  "memcmp",  "strlen",  "strcmp",  "strncmp", "strcpy",
      "strncpy", "strcat",  "strchr",  "strrchr", "strstr", "strdup",
  };
  return is_contained(Names, N);
}
} // namespace

void MSILWriter::resolveLibcOptions() {
  // `assembly:type`, or just `assembly` when a default type is given.
  auto Parse = [](StringRef Spec, StringRef DefaultType, StringRef Option) {
    ManagedType T;
    size_t Colon = Spec.rfind(':');
    T.Assembly = (Colon == StringRef::npos ? Spec : Spec.substr(0, Colon)).str();
    T.Type = Colon == StringRef::npos ? DefaultType.str()
                                      : Spec.substr(Colon + 1).str();
    if (T.Assembly.empty() || T.Type.empty())
      report_fatal_error("MSIL: " + Option +
                         " needs <assembly>:<type>, got '" + Spec + "'");
    return T;
  };
  if (!AllocSpec.empty())
    AllocType = Parse(AllocSpec, "", "-msil-alloc");
  for (const std::string &Spec : LibMapSpec) {
    size_t Eq = Spec.find('=');
    if (Eq == std::string::npos || Eq + 1 == Spec.size())
      report_fatal_error(Twine("MSIL: -msil-lib-map needs <pattern>=<dll>, "
                               "got '") + Spec + "'");
    LibMap.emplace_back(StringRef(Spec).substr(0, Eq).str(),
                        StringRef(Spec).substr(Eq + 1).str());
  }
  if (LibcSpec.empty())
    return;
  if (LibcSpec == "builtin") {
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
  if (LibcType.Type.empty())
    return false;
  // The built-in libc implements a fixed set; anything else keeps its pinvoke,
  // so a program can use both.
  if (EmitLibc && !isBuiltinSymbol(Name))
    return false;
  Out = LibcType;
  return true;
}

// The C libm names that have a System.Math/MathF equivalent with identical
// semantics. Everything else keeps its CRT pinvoke: `exp2` and `exp10` have no
// managed counterpart at all, and fmin/fmax deliberately do not because
// Math.Min/Max propagate NaN where the C functions return the other operand.
bool MSILWriter::getMathTarget(StringRef Symbol, Type *Ty,
                               ManagedType &Out) const {
  bool IsF32 = Ty->isFloatTy();
  if (!IsF32 && !Ty->isDoubleTy())
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
      {"scalbn", "ScaleB"},
      // rint, nearbyint and roundeven all round to even under the default
      // rounding mode, which is Math.Round with no MidpointRounding argument.
      // (fabs is not here: the intrinsic lowering already clears the sign bit,
      // which is exact and cheaper than a call.)
      {"rint", "Round"}, {"nearbyint", "Round"}, {"roundeven", "Round"},
  };
  // The C names carry an `f` suffix on the float spelling.
  StringRef Base = Symbol;
  if (IsF32 && Base.ends_with("f") && Base != "fabs" && !Members.count(Base))
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
  std::string Params;
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I) {
    if (I)
      Params += ", ";
    Params += getTypeName(FTy->getParamType(I));
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
  for (unsigned I = 0, E = FTy->getNumParams(); I != E; ++I)
    out() << "\tldarg\t" << I << '\n';
  out() << T.ExtraArg;
  // By value: the ternary would otherwise yield a temporary std::string and
  // the StringRef would point into it after it dies.
  std::string Member = T.Member.empty() ? getCName(F) : T.Member;
  out() << "\tcall\t" << getTypeName(FTy->getReturnType()) << ' '
      << getManagedName(T, Member) << '(' << Params << ")\n";
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
  out() << ".class public auto ansi abstract sealed beforefieldinit '"
      << LibcType.Type << "' extends [mscorlib]System.Object\n{\n";

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
  // An explicit map wins over everything: it exists to name a DLL the CRT
  // heuristic below has no way to know about.
  for (const auto &Pat : LibMap) {
    StringRef P = Pat.first;
    if (P.ends_with("*")) {
      if (Symbol.starts_with(P.drop_back()))
        return Pat.second;
    } else if (Symbol == P) {
      return Pat.second;
    }
  }
  return getLibraryForCRT(Symbol);
}

StringRef MSILWriter::getLibraryForCRT(StringRef Symbol) {
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
      std::string Key = getCallSignature(FTy, CB, "");
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
        Sig += I >= Fixed ? getVarArgTypeName(Ty) : getTypeName(Ty);
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
    out() << ".method public static hidebysig pinvokeimpl(\""
        << getLibraryFor(F.getName()) << "\" as \"" << getImportName(&F)
        << "\" cdecl) "
        << getCallSignature(F.getFunctionType(), nullptr, "'" + Name.str() + "'")
        << " preservesig {}\n";
  };

  for (Function &F : *Mod) {
    if (!F.isDeclaration() || F.isIntrinsic() || F.getFunctionType()->isVarArg())
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
    out() << ".field static private void* '" << G.getName() << "'\n";
    AnyExternVar = true;
  }
  if (AnyExternVar)
    out() << '\n';
}

void MSILWriter::printStaticConstant(const Constant *C, uint64_t &Offset,
                                     std::string &Buffer) {
  Type *Ty = C->getType();
  auto Add = [&](const std::string &S, uint64_t Size) {
    if (!Buffer.empty())
      Buffer += ", ";
    Buffer += S;
    Offset += Size;
  };
  auto PadTo = [&](uint64_t Target) {
    if (Target > Offset) {
      if (!Buffer.empty())
        Buffer += ", ";
      Buffer += "int8 (0) [" + utostr(Target - Offset) + "]";
      Offset = Target;
    }
  };

  // Only contiguous null bytes mean "all zero"; a zeroed aggregate with
  // poisoned padding or bitcast elements must still be walked.
  if (isa<ConstantAggregateZero>(C) ||
      (C->isNullValue() && !Ty->isAggregateType())) {
    uint64_t Size = DL->getTypeAllocSize(Ty).getFixedValue();
    if (!Buffer.empty())
      Buffer += ", ";
    Buffer += "int8 (0) [" + utostr(Size) + "]";
    Offset += Size;
    return;
  }
  if (const auto *CI = dyn_cast<ConstantInt>(C)) {
    // The .data element width must match the field layout: writing an i8
    // array as int32 would pad every character to four bytes.
    // ilasm range-checks .data elements against the signed width, so emit
    // sign-extended literals rather than the bit pattern.
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 128) {
      // The template is a raw byte blob, so emit the two halves as int64.
      const APInt &V = CI->getValue();
      Add("int64 (" + std::to_string((int64_t)V.getLoBits(64).getZExtValue())
          + ")",
          8);
      return Add(
          "int64 (" + std::to_string((int64_t)V.getHiBits(64).getZExtValue()) +
              ")",
          8);
    }
    std::string Val = std::to_string(CI->getSExtValue());
    if (N <= 8)
      return Add("int8 (" + Val + ")", 1);
    if (N <= 16)
      return Add("int16 (" + Val + ")", 2);
    if (N <= 32)
      return Add("int32 (" + Val + ")", 4);
    return Add("int64 (" + Val + ")", 8);
  }
  if (const auto *FP = dyn_cast<ConstantFP>(C)) {
    uint64_t Bits = FP->getValueAPF().bitcastToAPInt().getZExtValue();
    return Add(Ty->isFloatTy()
                   ? "int32 (" + std::to_string((uint32_t)Bits) + ")"
                   : "int64 (" + std::to_string(Bits) + ")",
               Ty->isFloatTy() ? 4 : 8);
  }
  if (isa<ConstantPointerNull>(C))
    return Add("int64 (0)", 8);
  if (isa<GlobalVariable>(C) || isa<Function>(C) || isa<ConstantExpr>(C)) {
    // A global's address is its writable heap copy, which $MSIL_Init allocates
    // after the templates are emitted, so these start as zero and are patched.
    DynInits.push_back({CurGV, Offset, C});
    return Add("int64 (0)", 8);
  }

  if (auto *STy = dyn_cast<StructType>(Ty)) {
    const StructLayout *SL = DL->getStructLayout(STy);
    uint64_t Start = Offset;
    for (unsigned I = 0, E = C->getNumOperands(); I != E; ++I) {
      PadTo(Start + SL->getElementOffset(I));
      printStaticConstant(cast<Constant>(C->getOperand(I)), Offset, Buffer);
    }
    PadTo(Start + DL->getTypeAllocSize(Ty).getFixedValue());
    return;
  }
  if (const auto *ATy = dyn_cast<ArrayType>(Ty)) {
    // ConstantDataSequential (string literals, packed arrays) has no operands;
    // walk it through getElementAsConstant instead.
    const auto *CDS = dyn_cast<ConstantDataSequential>(C);
    uint64_t EltSize =
        DL->getTypeAllocSize(ATy->getElementType()).getFixedValue();
    uint64_t Start = Offset;
    unsigned NumElts = CDS ? CDS->getNumElements() : C->getNumOperands();
    for (unsigned I = 0; I != NumElts; ++I) {
      PadTo(Start + I * EltSize);
      const Constant *Elt =
          CDS ? CDS->getElementAsConstant(I) : cast<Constant>(C->getOperand(I));
      printStaticConstant(Elt, Offset, Buffer);
    }
    PadTo(Start + DL->getTypeAllocSize(Ty).getFixedValue());
    return;
  }

  report_fatal_error("MSIL: unsupported global initializer: " + Twine(str(C)));
}

void MSILWriter::printGlobals() {
  // A `.data`-backed static field lives in a read-only segment, so any store
  // through it faults. Emit the initializer as a read-only template blob and
  // give the variable a writable heap pointer that $MSIL_Init fills in.
  for (GlobalVariable &G : Mod->globals()) {
    if (G.isDeclaration())
      continue;
    std::string Buffer;
    uint64_t Offset = 0;
    size_t FirstDynInit = DynInits.size();
    CurGV = &G;
    printStaticConstant(G.getInitializer(), Offset, Buffer);
    CurGV = nullptr;

    std::string Name = G.getName().str();
    std::string Cls = getAggregateName(G.getValueType());
    // ilasm rejects both an empty and a zero-count `.data`, and a zero-sized
    // aggregate has nothing to copy anyway: just give it a pointer.
    bool Empty = DL->getTypeAllocSize(G.getValueType()) == 0;
    // A constant global is never stored through, so its read-only image can be
    // its storage and $MSIL_Init only has to point the address at it. A
    // pointer-valued initializer is the exception: relocations are patched
    // into the writable copy at startup, which the image cannot take.
    bool Writable = !G.isConstant() || Empty || DynInits.size() != FirstDynInit;
    if (!Empty) {
      // $MSIL_Init runs from inside the emitted class and reads this blob, so
      // it has to be reachable from there.
      out() << ".field static " << HelperVis() << " valuetype '" << Cls << "' '"
          << Name << "$tpl' at '" << Name << "$tpl$data'\n";
      out() << ".data '" << Name << "$tpl$data' = { " << Buffer << " }\n";
    }
    out() << ".field static "
        << (G.hasLocalLinkage() ? std::string(HelperVis()) + " " : "public ")
        << "void* '" << Name << "'\n\n";
    // Zero-sized globals still get an entry so $MSIL_Init gives each its own
    // address; there is just no template to copy from.
    Templates.push_back({&G, Empty ? "" : Name + "$tpl", Writable});
  }
}

void MSILWriter::printAllocHelper() {
  out() << "\n.method static " << HelperVis()
        << " void* '$MSIL_Alloc'(int64 size) cil managed\n{\n";
  out() << "\t.maxstack\t2\n";
  out() << "\tldarg.0\n\tconv.i\n";
  out() << "\tcall\tnative int [mscorlib]System.Runtime.InteropServices.Marshal::"
         "AllocHGlobal(native int)\n";
  out() << "\tret\n}\n";
}

void MSILWriter::printInitializer() {
  if (Templates.empty())
    return;
  out() << "\n.method static " << HelperVis()
        << " void '$MSIL_Init'() cil managed\n{\n";
  out() << "\t.maxstack\t8\n";
  for (auto &T : Templates) {
    const GlobalVariable &G = *T.GV;
    uint64_t Size = DL->getTypeAllocSize(G.getValueType()).getFixedValue();
    out() << "\t// " << G.getName() << "\n";
    if (!T.Writable && Size) {
      // A constant global is never stored through, so the image is the
      // storage. `ldsflda` still gives each global its own address, so
      // address comparisons stay distinct.
      out() << "\tldsflda\tvaluetype '" << getAggregateName(G.getValueType())
          << "' '" << T.Template << "'\n";
      out() << "\tconv.u8\n";
      out() << "\tstsfld\tvoid* '" << G.getName() << "'\n";
      continue;
    }
    // Allocate the real, writable storage. A zero-sized global still needs a
    // distinct address: LLVM gives every global its own, so comparing two of
    // them must not compare equal.
    out() << "\tldc.i8\t" << (Size ? Size : 1) << "\n";
    out() << "\tcall\tvoid* " << ClassPrefix() << "'$MSIL_Alloc'(int64)\n";
    out() << "\tstsfld\tvoid* '" << G.getName() << "'\n";
    if (!Size)
      continue;
    // Copy the template bytes in.
    out() << "\tldsfld\tvoid* '" << G.getName() << "'\n";
    out() << "\tldsflda\tvaluetype '" << getAggregateName(G.getValueType())
        << "' '" << T.Template << "'\n";
    out() << "\tconv.u8\n";
    out() << "\tldc.i4\t" << Size << "\n";
    out() << "\tcpblk\n";
  }
  // Pointer fields the assembly's relocations could not express.
  for (const DynInit &DI : DynInits) {
    out() << "\tldsfld\tvoid* '" << DI.GV->getName() << "'\n";
    if (DI.Offset) {
      printPtrLoad(DI.Offset);
      out() << "\tconv.i8\n\tadd\n";
    }
    out() << "\tconv.u8\n";
    if (isa<Function>(DI.Value)) {
      out() << "\tldftn\t"
          << getCallSignature(cast<Function>(DI.Value)->getFunctionType(),
                              nullptr,
                              "'" + getValueName(DI.Value) + "'")
          << "\n";
    } else {
      printValueLoad(DI.Value);
    }
    out() << "\tstind.i\n";
  }
  out() << "\tret\n}\n";
}

void MSILWriter::printModuleStartup() {
  const Function *Main = Mod->getFunction("main");
  out() << "\n.method static public int32 '$MSIL_Startup'() cil managed\n{\n";
  out() << "\t.entrypoint\n\t.maxstack\t8\n";

  // IL puts .entrypoint in <Module>, so this method stays outside the class
  // and every helper it reaches has to be named through the class.
  std::string C = ExternClassName.empty() ? "" : "'" + ExternClassName + "'::";

  if (!Templates.empty())
    out() << "\tcall\tvoid " << C << "'$MSIL_Init'()\n";

  if (!Main || Main->isDeclaration()) {
    out() << "\tldc.i4.0\n\tret\n}\n";
    return;
  }
  // Build argv on the unmanaged heap so the program's `char **` works with
  // the plain pointer arithmetic the backend emits.
  // shortcut: argument strings are leaked; a real main() usually exits soon.
  bool WantArgs = Main->arg_size() >= 1;
  if (WantArgs) {
    out() << "\tcall\tvoid " << C << "'$MSIL_MakeArgs'()\n";
    out() << "\tldsfld\tvoid* " << C << "'$MSIL::argc'\n\tldind.i4\n";
    if (Main->arg_size() >= 2)
      out() << "\tldsfld\tvoid* " << C << "'$MSIL::argv'\n";
  }

  out() << "\tcall\t" << getTypeName(Main->getReturnType()) << " "
        << getFunctionRef(Main) << "(";
  for (unsigned I = 0, E = Main->arg_size(); I != E; ++I) {
    if (I)
      out() << ", ";
    out() << getTypeName(Main->getArg(I)->getType());
  }
  out() << ")\n";
  if (Main->getReturnType()->isVoidTy())
    out() << "\tldc.i4.0\n";
  else
    out() << "\tconv.i4\n";
  out() << "\tret\n}\n";
}

void MSILWriter::printArgHelper() {
  // Allocate argc/argv and fill them from Environment.GetCommandLineArgs().
  out() << "\n.field static " << HelperVis() << " void* '$MSIL::argc'\n";
  out() << ".field static " << HelperVis() << " void* '$MSIL::argv'\n";
  out() << "\n.method static " << HelperVis()
        << " void '$MSIL_MakeArgs'() cil managed\n{\n";
  out() << "\t.maxstack\t4\n\t.locals init (int32 'n')\n"
           "\t.locals init (void* 'p')\n"
           "\t.locals init (string[] 'a')\n"
           "\t.locals init (int32 'i')\n";
  // argv = GetCommandLineArgs()
  out() << "\tcall\tstring[] [mscorlib]System.Environment::"
           "GetCommandLineArgs()\n\tstloc\t'a'\n";
  out() << "\tldloc\t'a'\n\tldlen\n\tconv.i4\n\tstloc\t'n'\n";
  // argc storage
  std::string CP = ClassPrefix();
  out() << "\tldc.i4.4\n\tcall\tvoid* " << CP << "'$MSIL_Alloc'(int64)\n"
           "\tstsfld\tvoid* " << CP << "'$MSIL::argc'\n";
  out() << "\tldsfld\tvoid* " << CP << "'$MSIL::argc'\n\tldloc\t'n'\n\tstind.i4\n";
  // argv = (char**)alloca(n * 8)
  out() << "\tldloc\t'n'\n\tconv.i8\n\tldc.i4.8\n\tconv.i8\n\tmul\n\tcall\tvoid* "
        << CP << "'$MSIL_Alloc'(int64)\n\tstsfld\tvoid* " << CP
        << "'$MSIL::argv'\n";
  // for i in 0..n: argv[i] = StringToHGlobalAnsi(a[i])
  out() << "\tldc.i4.0\n\tstloc\t'i'\n";
  out() << "IL_args_loop:\n";
  out() << "\tldloc\t'i'\n\tldloc\t'n'\n\tbge\tIL_args_done\n";
  out() << "\tldsfld\tvoid* " << CP << "'$MSIL::argv'\n\tldloc\t'i'\n\tconv.i8\n"
           "\tldc.i4.8\n\tconv.i8\n\tmul\n\tadd\n";
  out() << "\tldloc\t'a'\n\tldloc\t'i'\n\tldelem.ref\n";
  out() << "\tcall\tnative int [mscorlib]System.Runtime.InteropServices."
           "Marshal::StringToHGlobalAnsi(string)\n";
  out() << "\tstind.i\n";
  out() << "\tldloc\t'i'\n\tldc.i4.1\n\tadd\n\tstloc\t'i'\n"
           "\tbr\tIL_args_loop\n";
  out() << "IL_args_done:\n\tret\n}\n";
}

bool MSILWriter::runOnModule(Module &M) {
  Mod = &M;
  DL = &M.getDataLayout();
  IL = std::make_unique<IntrinsicLowering>(*DL);
  resolveLibcOptions();

  // An alias is just another name for its target; resolve it away so an alias
  // of a function is not mistaken for a variable.
  for (GlobalAlias &A : M.aliases())
    A.replaceAllUsesWith(A.getAliasee());

  for (Function &F : M)
    if (!F.isDeclaration())
      lowerIntrinsics(F);

  collectTypes();

  // The assembly table has to be written before any body, so a Math binding
  // has to be discovered here rather than when its forwarder is emitted
  // below. Lowering has already created every libm declaration by this point.
  if (MathBind)
    for (Function &F : M) {
      ManagedType Probe;
      if (F.isDeclaration() && !F.isIntrinsic() &&
          getManagedTarget(&F, Probe) && Probe.Assembly == "System.Runtime") {
        UsesMath = true;
        break;
      }
    }

  out() << ".assembly extern mscorlib {}\n";
  // Int128/UInt128 and System.Math/MathF both live in System.Runtime, and a
  // framework reference only resolves with the real identity.
  if (NeedsSystemRuntime || UsesMath)
    out() << ".assembly extern System.Runtime { .ver 9:0:0:0\n"
             "  .publickeytoken = (B0 3F 5F 7F 11 D5 0A 3A ) }\n";
  // A user-supplied runtime is a separate assembly, so it needs a reference.
  // The built-in one lives in this module, and its CoreLib reference is
  // emitted with its class declaration.
  if (!LibcType.Type.empty() && !EmitLibc && !LibcType.Assembly.empty())
    out() << ".assembly extern '" << LibcType.Assembly << "' {}\n";
  if (!AllocType.Type.empty())
    out() << ".assembly extern '" << AllocType.Assembly << "' {}\n";
  // Whether any body needs Vector<T> is only known once they are all written,
  // so settle that before emitting the header.
  UsesVector = vectorNeeded(M);
  if (UsesVector)
    out() << ".assembly extern System.Numerics.Vectors { .ver 9:0:0:0\n"
             "  .publickeytoken = (B0 3F 5F 7F 11 D5 0A 3A ) }\n";
  out() << ".assembly MSIL {}\n\n";

  // Class declarations must precede the globals that name them as templates.
  printAggregateDeclarations();
  collectPInvokes();
  printExternals();
  if (EmitLibc)
    printBuiltinLibc();
  printGlobals();

  // Everything the C# side should be able to call or read goes inside this
  // class. The entry point cannot: IL requires .entrypoint to be a member of
  // <Module>, so $MSIL_Startup stays outside and calls in.
  bool Wrap = !ExternClassName.empty();
  if (Wrap)
    out() << ".class public auto ansi beforefieldinit '" << ExternClassName
          << "' extends [mscorlib]System.Object\n{\n";

  printFunctions();
  printAllocHelper();
  printArgHelper();
  printInitializer();

  if (Wrap)
    out() << "}\n";
  // The lock lives on <Module> so a class-wrapped module can still reach it:
  // a field's accessibility is checked against the declaring type, and a
  // private <Module> field is not visible from inside a class.
  printAtomicLock();
  printModuleStartup();
  return false;
}
