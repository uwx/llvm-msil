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
  void printAggregateLaneAddr(const std::string &Base, Type *EltTy,
                              uint64_t Off);

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
  // Module emission.
  //===------------------------------------------------------------------===//
  void lowerIntrinsics(Function &F);
  std::string getImportName(const Function *F) const;
  StringRef getLibraryFor(StringRef Symbol);
  void collectPInvokes();
  void collectTypes();
  void collectType(Type *Ty);
  void collectConstant(const Constant *C);
  void printAggregateDeclarations();
  void printExternals();
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

  std::vector<std::pair<std::string, std::string>> ExtraLocals;
  DenseMap<const Value *, std::string> ArgIterators;
  unsigned NumVA = 0;

  /// A global's read-only initializer image; the writable storage is a heap
  /// block allocated and filled from it by $MSIL_Init.
  std::vector<std::pair<const GlobalVariable *, std::string>> Templates;

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
  };
  std::vector<PInvoke> PInvokes;
  std::map<std::pair<const Function *, std::string>, std::string> PInvokeNames;

  /// Set once a 128-bit integer reaches the output; decides whether the
  /// System.Runtime reference is emitted.
  bool NeedsSystemRuntime = false;

  uint64_t UniqID = 0;
  uint64_t getUniqID() { return ++UniqID; }
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_MSIL_MSILWRITER_H
