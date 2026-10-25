//===-- MSILWriter.h - Library for converting LLVM code to MSIL --*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file declares the MSILWriter that is used by the MSIL backend.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MSIL_MSILWRITER_H
#define LLVM_LIB_TARGET_MSIL_MSILWRITER_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLFunctionalExtras.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include <map>
#include <string>
#include <vector>

namespace llvm {

class IntrinsicLowering;
class CallBase;
class IntrinsicInst;
class BranchInst;
class BasicBlock;
class Instruction;
class VectorType;
class AtomicRMWInst;
class AtomicCmpXchgInst;

/// Emits a whole module as CIL text suitable for `ilasm`.
///
/// IL requires every type, field and method signature before any body, so this
/// is a ModulePass rather than a per-function pass.
class MSILWriter : public ModulePass {
public:
  static char ID;

  explicit MSILWriter(raw_pwrite_stream &Out);

  bool runOnModule(Module &M) override;

private:
  /// Current output; may point at a scratch buffer while a body is emitted.
  raw_ostream *Out;
  raw_ostream &out() { return *Out; }
  Module *Mod = nullptr;
  const DataLayout *DL = nullptr;
  std::unique_ptr<IntrinsicLowering> IL;

  //===------------------------------------------------------------------===//
  // Names and types.
  //===------------------------------------------------------------------===//
  std::string getValueName(const Value *V);
  std::string getFunctionRef(const Function *F) const;
  std::string getLabelName(const BasicBlock *BB);
  std::string getAggregateName(Type *Ty);

  /// IL type used in method signatures, fields and aggregate bodies.
  std::string getTypeName(Type *Ty);

  /// IL type of a local slot; integers narrower than 32 bits widen to int32.
  std::string getLocalType(Type *Ty);

  /// ldind/stind suffix, or "" for aggregates (which use ldobj/stobj).
  std::string getMemSuffix(Type *Ty);

  std::string getCallSignature(FunctionType *FTy, const CallBase *CB,
                               StringRef Name);
  std::string getVarArgTypeName(Type *Ty);

  //===------------------------------------------------------------------===//
  // Values.
  //===------------------------------------------------------------------===//
  void printValueLoad(const Value *V);
  void printValueSave(const Value *V);

  /// Load an integer and normalize it to its declared width.
  void printValueLoadAsInt(const Value *V, bool Signed);

  void printConstLoad(const Constant *C);
  void printConstantExpr(const ConstantExpr *CE);

  void printPtrLoad(uint64_t N);
  void printSimpleInstruction(const char *Inst, const char *Operand = nullptr);
  void printBinaryInstruction(const char *Name, const Value *Left,
                              const Value *Right);
  void printNot();

  StructType *structFieldOf(const Value *Ptr, Type *AccessTy, unsigned &Field,
                            const Value *&Base);
  void printIndirectLoad(const Value *Ptr, Type *Ty);
  void printIndirectSave(const Value *Ptr, const Value *Val);
  void printCastInstruction(unsigned Op, const Value *V, Type *Ty);
  void printWideBinary(unsigned Op, const Value *L, const Value *R);
  void printWideICmp(unsigned Pred, const Value *L, const Value *R);
  void printGepInstruction(const User *GEP);
  void printVectorGep(const GEPOperator *GEP);
  /// Expand vector element-wise arithmetic into a per-lane loop.
  void printVectorBinary(StringRef Op, const Instruction *Inst,
                        bool ZeroExtLanes = false, bool ShiftOp = false);
  void printVectorCast(unsigned Op, const Value *V, Type *Ty);
  void printVectorCmp(unsigned Pred, const Value *L, const Value *R,
                      VectorType *ResTy, bool IsFP);

  //===------------------------------------------------------------------===//
  // SIMD (System.Numerics.Vector<T>).
  //===------------------------------------------------------------------===//
  /// True when \p Ty is a vector held in a `Vector<T>` local: an element-wise
  /// vector whose bit width is exactly one SIMD register on this target.
  bool isSimdVectorTy(Type *Ty) const;

  /// `<elem>` of the Vector<T> for \p VTy (int32, int64, float32, float64).
  std::string getVectorElemName(Type *EltTy);

  /// Full IL name of the `Vector<T>` that holds \p VTy.
  std::string getVectorTypeName(Type *VTy);

  /// Emit a SIMD select, optionally fusing the comparison that produces the
  /// mask: a Vector<T> compare yields a full-width mask, which cannot be
  /// stored in the i1-vector blob, so a one-use compare is consumed here.
  bool tryPrintSimdSelect(const Instruction *Inst);

  bool tryPrintSimdBinary(const Instruction *Inst);
  /// Emit a SIMD compare and leave the mask on the stack as a Vector<T>.
  bool tryPrintSimdCmpToStack(const User *Cmp, bool IsFP);
  bool tryPrintSimdCast(const Instruction *Inst);
  void printAggregateLaneAddr(const std::string &Base, Type *EltTy,
                              uint64_t Off);

  /// Rewrite intrinsics that IntrinsicLowering does not know into a call to
  /// the libm function that implements them. Returns true if \p CI was one.
  bool lowerLibmIntrinsic(CallInst *CI);

  /// Rewrite an intrinsic that has no runtime support into IR that computes
  /// the same value. Returns true if \p CI was one.
  bool lowerExpandedIntrinsic(CallInst *CI);

  //===------------------------------------------------------------------===//
  // Instructions.
  //===------------------------------------------------------------------===//
  void printInstruction(const Instruction *Inst);
  void printCallInstruction(const CallBase *CB);
  void printFunctionCall(const Value *FnVal, const CallBase *CB);
  void printIntrinsicCall(const IntrinsicInst *II);
  void printBranchInstruction(const BranchInst *Inst);
  void printPHICopy(const BasicBlock *From, const BasicBlock *To);
  void printICmp(unsigned Predicate, const Value *L, const Value *R,
                 bool EmitOperands = true);
  void printFCmp(unsigned Predicate, const Value *L, const Value *R,
                 bool EmitOperands = true);

  /// Emit a fresh IL local of \p Ty, used for bitcast punning.
  std::string newTempLocal(Type *Ty);

  //===------------------------------------------------------------------===//
  // Atomics.
  //===------------------------------------------------------------------===//
  /// Load/store with the `volatile.` prefix, which the CLR defines as a
  /// release/acquire access. Every atomic memory access the IR asks for is
  /// at least relaxed, and IL has no weaker atomic access.
  void printAtomicLoad(const Value *Ptr, Type *Ty);
  void printAtomicStore(const Value *Ptr, const Value *Val);

  /// Emit an `atomicrmw`, leaving the previous value on the stack.
  void printAtomicRMW(const AtomicRMWInst *RMW);
  void printAtomicCmpXchg(const AtomicCmpXchgInst *CX);

  /// Run \p Body with `$MSIL::atomicLock` held.
  void printUnderAtomicLock(function_ref<void()> Body);

  /// Leave `<bits of A> != <bits of B>` on the stack for a float value.
  void printFloatBitsNe(const std::string &A, const std::string &B, Type *Ty);

  /// The Interlocked overload name for a type, or "" when it has none.
  static StringRef interlockedTypeName(Type *Ty);

  /// Emit the shared lock object, if a narrow atomic needed one.
  void printAtomicLock();

  //===------------------------------------------------------------------===//
  // Module emission.
  //===------------------------------------------------------------------===//
  void lowerIntrinsics(Function &F);
  /// The C name a symbol denotes, with a C++ mangled declaration demangled.
  std::string getCName(const Function *F) const;
  /// The CRT export getCName names, after POSIX-only spellings are mapped.
  std::string getImportName(const Function *F) const;
  StringRef getLibraryFor(StringRef Symbol);
  StringRef getLibraryForCRT(StringRef Symbol);
  void collectPInvokes();
  /// True when any type in \p M needs the Vector<T> assembly reference.
  bool vectorNeeded(Module &M);
  /// True when \p Ty is or contains a register-wide vector type.
  bool vectorTypeIn(Type *Ty);
  void collectTypes();
  void collectType(Type *Ty);
  void collectConstant(const Constant *C);
  void printAggregateDeclarations();
  void printExternals();

  //===------------------------------------------------------------------===//
  // Managed C runtime (-msil-libc, -msil-alloc).
  //===------------------------------------------------------------------===//
  /// A user-supplied managed type. An empty assembly means this module.
  struct ManagedType {
    std::string Assembly;
    std::string Type;
    /// Overrides the member name for this one function; empty uses the C
    /// symbol's own name. The libm bindings need it because the C name and
    /// the `System.Math` name disagree (`llrint` vs a rounding call).
    std::string Member;
    /// IL to push an extra argument before the forwarded ones, for members
    /// whose signature is wider than the C function's.
    std::string ExtraArg;
  };

  /// Read the two options into LibcType/AllocType; reports a malformed one.
  void resolveLibcOptions();

  /// Where an external function binds instead of the CRT, if anywhere.
  bool getManagedTarget(const Function *F, ManagedType &Out) const;

  /// `[assembly]Type::Member`, or `Type::Member` for this module.
  std::string getManagedName(const ManagedType &T, StringRef Member) const;

  /// `System.Math`/`System.MathF` binding for a C libm symbol, when the C99
  /// spelling has a managed equivalent with identical semantics.
  bool getMathTarget(StringRef Symbol, Type *Ty, ManagedType &Out) const;

  /// An IL method with the C symbol's signature and name \p Name, calling the
  /// managed method \p T provides for it.
  void printManagedForwarder(const Function *F, const ManagedType &T,
                             StringRef Name);

  /// The built-in platform-independent libc, emitted into this module.
  void printBuiltinLibc();
  /// One method of it. \p Body is the instruction text; \p Params is the
  /// signature's parameter list in the same spelling getTypeName produces.
  void printLibcMethod(StringRef Name, StringRef Ret, StringRef Params,
                       StringRef Body);
  void printGlobals();
  void printStaticConstant(const Constant *C, uint64_t &Offset,
                           std::string &Buffer);
  void printFunctions();
  void printFunction(const Function &F);
  void printAllocHelper();
  void printArgHelper();
  void printModuleStartup();
  void printInitializer();


  //===------------------------------------------------------------------===//
  // State.
  //===------------------------------------------------------------------===//
  DenseMap<const Value *, unsigned> LocalIDs;
  DenseMap<const BasicBlock *, unsigned> LabelIDs;
  unsigned NextLocalID = 0;
  unsigned NextLabelID = 0;

  std::map<Type *, std::string> AggregateNames;
  std::vector<Type *> AggregateTypes;

  std::string ClassPrefix() const;
  const char *HelperVis() const;

  std::vector<std::pair<std::string, std::string>> ExtraLocals;
  DenseMap<const Value *, std::string> ArgIterators;
  unsigned NumVA = 0;

  /// Set once any Vector<T> operation is emitted; the assembly reference has
  /// to follow the type references, so it is written after the functions.
  bool UsesVector = false;

  /// A global's initializer image. A writable global gets a heap copy filled
  /// from its image by $MSIL_Init; a constant global is never written, so the
  /// image is its storage and $MSIL_Init only points the address at it.
  struct GlobalImage {
    const GlobalVariable *GV;
    /// The template field's name; empty for a zero-sized global.
    std::string Template;
    bool Writable;
  };
  std::vector<GlobalImage> Templates;

  struct DynInit {
    const GlobalVariable *GV;
    uint64_t Offset;
    const Constant *Value;
  };
  std::vector<DynInit> DynInits;
  const GlobalVariable *CurGV = nullptr;

  /// A pinvoke thunk. `ilasm` rejects `vararg` pinvokes, so every distinct
  /// call signature of an external vararg function gets a concrete stub.
  struct PInvoke {
    std::string Name;
    std::string Signature;
    std::string Export;
    std::string Symbol;
  };
  std::vector<PInvoke> PInvokes;
  std::map<std::pair<const Function *, std::string>, std::string> PInvokeNames;
  /// -msil-lib-map entries as (pattern, dll), parsed once per module.
  std::vector<std::pair<std::string, std::string>> LibMap;

  /// Set by resolveLibcOptions; an empty Type means "use the CRT".
  ManagedType LibcType;
  ManagedType AllocType;
  /// Emit the built-in libc's own IL, rather than referring to a user's.
  bool EmitLibc = false;

  /// A Math/MathF forwarder was emitted, so System.Runtime is referenced.
  bool UsesMath = false;

  /// Set once a 128-bit integer reaches the output; decides whether the
  /// System.Runtime reference is emitted.
  bool NeedsSystemRuntime = false;

  uint64_t UniqID = 0;
  uint64_t getUniqID() { return ++UniqID; }

  /// Set when a narrow atomic access needed `$MSIL::atomicLock`.
  bool NeedsAtomicLock = false;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MSIL_MSILWRITER_H
