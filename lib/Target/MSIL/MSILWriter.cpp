//===-- MSILWriter.cpp - Library for converting LLVM code to MSIL ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
//
// This library converts LLVM IR to CIL text that `ilasm` can assemble.
//
// Model
// -----
//  * Scalars live in locals of a storage type wide enough for the value
//    (i8/i16/i1 -> int32, i32 -> int32, i64 -> int64, ptr -> void*). Width is
//    normalized explicitly whenever a narrow value is stored or compared, so
//    no bits are lost and unsigned compares see the right bits: conv.u1/conv.u2
//    for i8/i16, and an and-mask for i1, which has no conv that isolates one bit
//    (conv.u1 would keep the low eight). See widthConv.
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

// Naming the emitted class lets C# call into the module without reflection:
// `MSIL.Program.Main()`. Without it everything is a free-standing member of
// `<Module>`, which is legitimate IL but awkward to reference from C#.
cl::opt<std::string> ExternClassName(
    "msil-class-name", cl::desc("Emit members inside this class instead of "
                                "<Module>, so C# can call them directly"),
    cl::value_desc("namespace.Name"), cl::init(""));

// Library output: no .entrypoint, and a public static MSILInitialize() that
// the caller invokes once. A class name is mandatory, because that method must
// be reachable from managed code.
cl::opt<bool> LibraryMode(
    "msil-library",
    cl::desc("Emit a library (no .entrypoint) with a public static "
             "MSILInitialize() that must be called once before other members"),
    cl::init(false));

// The assembly's own identity, so the .il and the emitted DLL name agree.
cl::opt<std::string> AssemblyName(
    "msil-assembly-name",
    cl::desc("Name of the emitted assembly's .assembly directive"),
    cl::value_desc("name"), cl::init("MSIL"));

// A load or store whose address is `getelementptr inbounds %S, ptr %p, 0, N`

// Binding external C functions to managed code rather than the CRT. Both are
// off by default, so the emitted module is unchanged unless asked.
cl::opt<std::string> LibcSpec(
    "msil-libc",
    cl::desc("Bind external C functions to managed code instead of the Windows "
             "CRT. 'builtin' links the portable C library (string, ctype, "
             "stdlib, printf) into this module; 'il' emits the smaller "
             "hand-written IL subset instead; '<assembly>[:<type>]' calls a "
             "type you supply (type defaults to MSIL.Libc). Functions neither "
             "implements still pinvoke the CRT."),
    cl::value_desc("builtin|assembly[:type]"), cl::init(""));

// The C++ runtime: the half of a C++ implementation that lives in the library
// rather than the headers -- operator new/delete, the local-static guards, and
// exit-time destructors. 'builtin' links the portable one into this module;
// off by default, so a C module is unaffected and a C++ module that supplies
// its own can opt out.
cl::opt<std::string> LibcxxSpec(
    "msil-libcxx",
    cl::desc("Link a C++ runtime into the module. 'builtin' links the portable "
             "one (operator new/delete, local-static guards, exit-time "
             "destructors), which is what a program built with freestanding "
             "libc++ headers needs. Its C++ part needs -fno-exceptions and "
             "-fno-rtti."),
    cl::value_desc("builtin"), cl::init(""));

cl::opt<std::string> AllocSpec(
    "msil-alloc",
    cl::desc("Route malloc/free/calloc/realloc to methods of the same name on "
             "this type, so unmanaged allocations can be instrumented"),
    cl::value_desc("assembly:type"), cl::init(""));

// On by default: a pinvoke into the CRT is the one thing that ties the output
// to Windows, and System.Math covers nearly every libm call a C program makes.
// Off for a target that must match a specific CRT's rounding bit-for-bit.
cl::opt<bool> MathBind(
    "msil-math", cl::desc("Bind libm calls to System.Math/System.MathF "
                          "instead of pinvoking the C runtime"),
    cl::init(true));

// A symbol this module imports from a native library other than the CRT. The
// CRT split in getLibraryFor is a name heuristic, which cannot know about a
// third-party DLL, so these are consulted first.
cl::list<std::string> LibMapSpec(
    "msil-lib-map",
    cl::desc("Bind matching imported symbols to a native library, e.g. "
             "'SDL_*=SDL2.dll'. <pattern>=<dll>, '*' matches any suffix; "
             "checked before the C runtime heuristic"),
    cl::value_desc("pattern=dll"));

// The library a program links against. Unlike -msil-lib-map this names no
// symbol: the exports are read from the library's own export table and a
// module symbol matching one is bound to it, which is what a linker does.
// Repeatable, and searched in the order given.
cl::list<std::string> LibSpec(
    "msil-lib",
    cl::desc("Bind imported symbols to this library if it exports them, e.g. "
             "'ucrtbase.dll' or a path to one. The exports are read from the "
             "file, so no symbol list is needed; searched in order"),
    cl::value_desc("dll"));

// `long double` on an x86 ELF target is x86_fp80, and the CLR has no 80-bit
// float. Mapping it to binary64 is lossy in both directions: 53 mantissa bits
// instead of 64, so a value round-trips changed, and 11 exponent bits instead
// of 15, so a large one overflows to infinity. That is why it is opt-in.
//
// Only the value representation changes. An x86_fp80 stays 16 bytes in the
// module's data layout, so struct member offsets, `sizeof` and pointer strides
// keep saying what the native ABI says; each 16-byte slot holds an 8-byte
// float64 and the rest is padding. Rewriting the type itself would instead
// shrink those strides while C-level `sizeof` constants (already folded into
// the IR) stayed 16, silently breaking array indexing.
cl::opt<std::string> Fp80Spec(
    "msil-fp80",
    cl::desc("Map x86_fp80 (long double on the x86 ELF target) to binary64 "
             "instead of rejecting it. Lossy: 53 mantissa bits and 11 "
             "exponent bits instead of 64 and 15"),
    cl::value_desc("double"), cl::init(""));

namespace llvm {

/// Whether -msil-fp80 asked for the lossy binary64 mapping.
bool fp80MappedToDouble() { return Fp80Spec == "double"; }

// Pointer representation. By default a pointer is an unmanaged `void*` (the
// CLR's native int), which is the natural IL spelling and what the JIT uses as
// an address directly.
//
// With -msil-pointers=i64 every pointer is instead stored as an `int64`. The
// module is still laid out LP64: the data layout is untouched, so sizeof,
// struct offsets and pointer strides keep saying what the native ABI says. Only
// the *value* representation in the IL changes -- a pointer-typed slot holds
// the address as a 64-bit integer, and each dereference converts it back to a
// native int with `conv.u` first.
//
// Why: on a 64-bit host this is a straight bitmap with no indirection, and the
// JIT treats conv.u (int64 -> native int) as a no-op, so the emitted address is
// the real one. On a 32-bit host real addresses fit and the upper 32 bits are
// simply zero, so conv.u still yields the right pointer. What it costs is
// space: a struct that embeds a pointer pays 8 bytes for it on a 32-bit host
// where the native layout would use 4, and its layout no longer matches the
// native 32-bit struct, so interop with native code expecting real 32-bit
// layouts needs a translation shim rather than a direct pointer pass.
static cl::opt<std::string> PointerSpec(
    "msil-pointers",
    cl::desc("How a pointer is represented in IL. 'i64' keeps every pointer in "
             "an int64 slot and converts with conv.u before each dereference; "
             "the default is the unmanaged void* (native int)"),
    cl::value_desc("i64"), cl::init(""));

/// Whether -msil-pointers=i64 keeps pointers in int64 slots.
bool pointersAsInt64() { return PointerSpec == "i64"; }

} // namespace llvm

char MSILWriter::ID = 0;

MSILWriter::MSILWriter(raw_pwrite_stream &Sink)
    : ModulePass(ID), Out(&Sink) {}

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

// One accessor per thread-local global, named after it. The accessor is
// emitted inside the same class as the field it touches, so it is qualified
// the same way a function is.
std::string MSILWriter::getThreadLocalRef(const GlobalVariable *GV) const {
  std::string Name = "'" + GV->getName().str() + "$tls'()";
  return ExternClassName.empty() ? Name : "'" + ExternClassName + "'::" + Name;
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
  // x86_fp80 has no IL type of its own; with -msil-fp80 it is a binary64
  // value, and the enclosing 16-byte slot stays 16 bytes either way.
  if (Ty->isX86_FP80Ty()) {
    if (!fp80MappedToDouble())
      fatalError("unsupported type " + Twine(str(Ty)));
    return "float64";
  }
  switch (Ty->getTypeID()) {
  case Type::VoidTyID:
    return "void";
  case Type::IntegerTyID: {
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 1)
      return "bool";
    if (N == 128)
      return Int128Ty;
    // Wider than i128 has no representation at all: the widest .NET integer is
    // Int128, and the i65..i127 promotion (MSILWideIntegers.cpp) is built on
    // it. Saying so here is the single check, rather than letting a value fall
    // into the int64 slot below and be silently truncated.
    if (N > 128)
      fatalError("an integer wider than 128 bits has no IL type: " +
                 Twine(str(Ty)));
    // The CLR has no integer wider than 64 bits; wide IR integers (i33 from
    // overflow-avoiding transforms, i64, ...) use an int64 slot.
    if (N > 32)
      return "int64";
    // the slot matches the alloc size, so an i24 field is still 4 bytes
    return "int" + utostr(llvm::bit_ceil(std::max(N, 8u)));
  }
  case Type::FloatTyID:
    return "float32";
  case Type::DoubleTyID:
    return "float64";
  case Type::PointerTyID:
    // -msil-pointers=i64 keeps the address in an int64 slot; the default is the
    // CLR native int (`void*`), which the JIT uses as an address directly.
    return pointersAsInt64() ? "int64" : "void*";
  case Type::FixedVectorTyID:
    // A vector of i128 lanes has no IL type name at all: the blob class is
    // named `$aggN` but its body would have to spell each lane, and Int128 is
    // a value type no IL element access handles. The lane path rejects
    // extract/insert on one, but a vector-typed parameter or return reaches
    // this function first, so the check has to be here to cover both.
    if (Ty->getScalarType()->isIntegerTy(128))
      fatalError("a vector of 128-bit integers has no IL type: " +
                 Twine(str(Ty)));
    // A register-wide vector travels as its Vector<T>: the blob class is
    // still emitted, but only for the shapes the lane path handles.
    if (isSimdVectorTy(Ty))
      return getVectorTypeName(Ty);
    return "valuetype '" + getAggregateName(Ty) + "'";
  case Type::StructTyID:
  case Type::ArrayTyID:
    return "valuetype '" + getAggregateName(Ty) + "'";
  default:
    fatalError("unsupported type " + Twine(str(Ty)));
  }
}

std::string MSILWriter::getLocalType(Type *Ty) {
  if (Ty->isIntegerTy()) {
    unsigned N = Ty->getIntegerBitWidth();
    if (N == 1)
      return "bool";
    if (N == 128)
      return Int128Ty;
    if (N > 128)
      fatalError("an integer wider than 128 bits has no IL type: " +
                 Twine(str(Ty)));
    return N <= 32 ? "int32" : "int64";
  }
  // A register-wide vector is held in its Vector<T>, so every local of that
  // type is the SIMD register rather than a byte blob.
  if (isSimdVectorTy(Ty))
    return getVectorTypeName(Ty);
  return getTypeName(Ty);
}

// becomes ldfld/stfld on the struct class, so the member is named rather than
// reached by adding an offset to a raw pointer. The pointer that goes on the
// stack is the GEP's *base*: ldfld adds the member offset itself, so handing
// it the already-offset pointer would add the offset a second time.
//
// Only an access whose type is exactly the field's type qualifies. A narrower
// or wider access through the same address (`load i32` from the start of a
// `{ i32, i32 }` member) is a different value with a different stack type, so
// it must go through the pointer path: ldfld of the declared type would push
// the whole member where an int32 is expected, and the JIT rejects the method.
StructType *MSILWriter::structFieldOf(const Value *Ptr, Type *AccessTy,
                                      unsigned &Field, const Value *&Base) {
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
  if (STy->getElementType(Field) != AccessTy)
    return nullptr;
  Base = GEP->getPointerOperand();
  return STy;
}

std::string MSILWriter::getMemSuffix(Type *Ty) {
  if (movesAsBlob(Ty))
    return "";
  return memSuffixOf(Ty).str();
}

// Dereference points in the emitted IL expect a native int (a managed/unmanaged
// pointer) on the stack. With -msil-pointers=i64 the address is an int64 there,
// so convert it first. On a 64-bit JIT conv.u is a no-op; on a 32-bit one it
// truncates to the address width, which is correct because such an address
// fits in the low 32 bits.
void MSILWriter::printPtrToNative() {
  if (pointersAsInt64())
    out() << "\tconv.u\n";
}

// A vararg definition is emitted as an ordinary method with a fixed
// parameter list: the declared parameters, then one `void*` for a buffer of
// trailing arguments. CoreCLR on Linux refuses a `vararg` method outright, and
// refuses any method that merely *refers* to one -- a `call` or even an
// `ldftn` makes the JIT reject the referring method with
// `InvalidProgramException: Vararg calling convention not supported` -- so no
// `vararg` method is emitted at all and the call sites build the buffer.
//
// The buffer holds one 8-byte slot per trailing argument: an integer widened
// to int64, a float as its int64 bit pattern, a pointer as its address. Each
// slot is read back as int64, which is the normalization the portable
// library's accessors already assume, so its C does not change.
bool MSILWriter::isEmittedVarArgDef(const Function *F) {
  return F && F->isVarArg() && !F->isDeclaration();
}

// Windows' va_list *is* the cursor. On ELF the front end declares a
// __va_list_tag whose fields are gp_offset, fp_offset, overflow_arg_area and
// reg_save_area; va_start below forces every argument down the overflow path,
// so both triples advance the same 8-byte slots.
bool MSILWriter::vaIsTag() const {
  return Triple(Mod->getTargetTriple()).isOSBinFormatELF();
}

// Whether the module's C++ ABI is Itanium's. The environment carries that:
// `x86_64-w64-windows-gnu` is COFF but Itanium, `x86_64-pc-windows-msvc` is
// both COFF and MSVC.
bool MSILWriter::usesItaniumCxxAbi() const {
  return !Triple(Mod->getTargetTriple()).isWindowsMSVCEnvironment();
}

uint64_t MSILWriter::vaSlotSize() const { return vaIsTag() ? 24 : 8; }

// Leave the va_list's cursor (the address of the next slot) on the stack.
// \p Ptr is a pointer to the va_list. On Windows that pointer is the cursor's
// own address, so one load reads it. On ELF it points at the __va_list_tag
// itself (va_start wrote the tag there, not a pointer to it), and the cursor
// is the tag's overflow_arg_area at offset 8.
void MSILWriter::printVaCursor(const Value *Ptr) {
  printValueLoad(Ptr);
  if (vaIsTag()) {
    out() << "\tldc.i8\t8\n\tadd\n";
  }
  out() << "\tldind.i\n";
}

// The Windows x64 calling convention passes a vararg floating-point value in
// both its SSE slot and a general-purpose slot, and the CRT's va_arg reads the
// general-purpose one. The CLR does not duplicate, so such arguments cross the
// pinvoke boundary as their raw bit pattern in an int64.
std::string MSILWriter::getVarArgTypeName(Type *Ty) {
  // A double crosses as its bits only for the Windows x64 convention this
  // comment describes; the SysV ABI passes it in an SSE register and the CLR
  // pinvoke stub follows the ABI. The choice follows the module's triple, not
  // the host, so the same IR emits the same IL everywhere.
  bool WindowsABI = !Triple(Mod->getTargetTriple()).isOSBinFormatELF();
  return Ty->isFloatingPointTy() && WindowsABI ? "int64"
                                               : getTypeName(Ty);
}

// Calls into the portable library normalize every vararg into the slot types
// its seam accessors read back: one int64 per slot, an address for a pointer.
// It has to be the same normalization everywhere: whether a call reaches the
// library directly or through a declaration the link resolves, the IL
// signature the call site names must agree with the callee's.
std::string MSILWriter::getPortableVarArgTypeName(Type *Ty) {
  if (Ty->isPointerTy())
    return "void*";
  if (Ty->isIntegerTy() || Ty->isFloatingPointTy())
    return "int64";
  return getTypeName(Ty);
}

bool MSILWriter::callsPortableLibc(const Function *Callee) const {
  return EmitPortable && Callee && !Callee->isIntrinsic() &&
         isPortableLibcSymbol(getCName(Callee));
}

// \p VarArgDef says the name denotes a vararg definition this module emits in
// buffer form (see isEmittedVarArgDef): its signature is then the fixed
// parameters plus one `void*` for the buffer, and no `vararg` marker.
std::string MSILWriter::getCallSignature(FunctionType *FTy, const CallBase *CB,
                                         StringRef Name, bool PortableVarArgs,
                                         bool VarArgDef) {
  std::string Sig;
  if (FTy->isVarArg() && !VarArgDef)
    Sig += "vararg ";
  Sig += getTypeName(FTy->getReturnType());
  Sig += " ";
  Sig += Name.str();
  Sig += "(";
  unsigned Fixed = FTy->getNumParams();
  for (unsigned I = 0; I != Fixed; ++I) {
    if (I)
      Sig += ", ";
    Sig += getTypeName(FTy->getParamType(I));
  }
  if (VarArgDef) {
    if (Fixed)
      Sig += ", ";
    Sig += "void*";
  } else if (FTy->isVarArg() && CB) {
    // A vararg call site must name the trailing argument types: the CLR needs
    // them to know how many bytes the callee pops.
    if (CB->arg_size() > Fixed) {
      Sig += ", ..., ";
      for (unsigned I = Fixed, E = CB->arg_size(); I != E; ++I) {
        if (I != Fixed)
          Sig += ", ";
        Sig += PortableVarArgs
                   ? getPortableVarArgTypeName(CB->getArgOperand(I)->getType())
                   : getVarArgTypeName(CB->getArgOperand(I)->getType());
      }
    }
  }
  Sig += ")";
  return Sig;
}

bool MSILWriter::runOnModule(Module &M) {
  Mod = &M;
  DL = &M.getDataLayout();
  IL = std::make_unique<IntrinsicLowering>(*DL);
  resolveLibcOptions();
  if (LibraryMode && ExternClassName.empty())
    fatalError("-msil-library needs -msil-class-name so "
                       "MSILInitialize() is reachable from managed code");

  // The portable library is linked first so the functions it brings in go
  // through the same lowering and emission as user code. Linking before
  // collectTypes also means the types it introduces get their classes.
  //
  // The front-end wrappers go first: while `printf` is still a definition in
  // this module, LinkOnlyNeeded sees no reference to resolve and skips the
  // library's copy, so the program would keep the CRT wrapper.
  if (EmitPortable) {
    dropStdioWrappers();
    linkPortableLibc();
  }
  // The C++ runtime goes in after the C library so its malloc/free/abort/mem*
  // references are satisfied by declarations the C link already resolved, and
  // before collectTypes so the functions it adds are lowered like user code.
  if (EmitCxxRt)
    linkPortableCxxRt();

  // An alias is just another name for its target; resolve it away so an alias
  // of a function is not mistaken for a variable.
  for (GlobalAlias &A : M.aliases())
    A.replaceAllUsesWith(A.getAliasee());

  shortenLongNames(M);

  // The link added functions after the target machine's prep passes ran, so
  // the constructs that need lowering for IL are still there: switches,
  // allocas, and unwinds. Run that subset again over the definitions the link
  // brought in. User code has already been through it, so re-running would
  // only cost time -- but the bookkeeping of which is which is not worth it
  // for a module-sized pass on a handful of functions.
  lowerLinkedFunctions();

  for (Function &F : M)
    if (!F.isDeclaration()) {
      matchCallsToDefinitions(F);
      promoteWideIntegers(F);
      lowerIntrinsics(F);
      splitOddWidthAccesses(F);
    }

  collectTypes();
  UsesCxxRuntime = usesCxxRuntime();

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
  if (UsesCxxRuntime)
    out() << ".assembly extern '" << CxxRuntimeAssembly << "' {}\n";
  // Whether any body needs Vector<T> is only known once they are all written,
  // so settle that before emitting the header.
  UsesVector = vectorNeeded(M);
  if (UsesVector)
    out() << ".assembly extern System.Numerics.Vectors { .ver 9:0:0:0\n"
             "  .publickeytoken = (B0 3F 5F 7F 11 D5 0A 3A ) }\n";
  out() << ".assembly " << AssemblyName << " {}\n\n";

  // Class declarations must precede the globals that name them as templates.
  printAggregateDeclarations();
  collectPInvokes();
  printExternals();
  if (EmitLibc || EmitPortable)
    printBuiltinLibc();
  if (UsesLibcSeam)
    printLibcSeam();
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
  if (!LibraryMode)
    printArgHelper();
  printInitializer();
  if (LibraryMode)
    printLibraryInitializer();

  if (Wrap)
    out() << "}\n";
  // The lock lives on <Module> so a class-wrapped module can still reach it:
  // a field's accessibility is checked against the declaring type, and a
  // private <Module> field is not visible from inside a class.
  printAtomicLock();
  printModuleStartup();
  return false;
}
