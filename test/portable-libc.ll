; RUN: llvm-msil %s -o - -msil-libc=builtin | FileCheck %s

; -msil-libc=builtin links the portable C library (lib/Target/MSIL/libc/*.c)
; into the module, keeping only what the program reaches. These checks are
; about the plumbing: which definitions appear, how a vararg call reaches the
; C code, and what the seam methods look like. Behaviour is covered by
; test/programs/libc.sh, which compares against a native build.
;
; The signatures below are the Windows ABI's, where long is 32 bits. An ELF
; triple selects a copy of the library built with a 64-bit long, so the triple
; is pinned rather than left to the host.
target triple = "x86_64-pc-windows-msvc"

declare i32 @printf(ptr, ...)
declare i32 @puts(ptr)
declare i32 @atoi(ptr)
declare i32 @strtol(ptr, ptr, i32)
; strlen and memcpy stay on the IL subset: it implements them already, so the
; portable library declares them rather than defining them.
declare i64 @strlen(ptr)
declare i32 @toupper(i32)

@fmt = private constant [4 x i8] c"%d\0A\00"

define i32 @main() {
  %p = call i32 (ptr, ...) @printf(ptr @fmt, i32 42)
  %q = call i32 @puts(ptr @fmt)
  %n = call i32 @atoi(ptr @fmt)
  %s = call i32 @strtol(ptr @fmt, ptr null, i32 10)
  %l = call i64 @strlen(ptr @fmt)
  %u = call i32 @toupper(i32 97)
  ret i32 %p
}

; The library's definitions are ordinary module methods under their C names,
; and the calls reach them directly instead of through a pinvoke. printf stays
; vararg: the portable implementation is a real C vararg function.
; CHECK-DAG: .method static public hidebysig vararg int32 'printf'(void* 'l0')
; CHECK-DAG: .method static public hidebysig int32 'puts'(void* 'l0')
; CHECK-DAG: .method static public hidebysig int32 'atoi'(void* 'l0')
; CHECK-DAG: .method static public hidebysig int32 'strtol'(void* 'l0', void* 'l1', int32 'l2')
; CHECK-DAG: .method static public hidebysig int32 'toupper'(int32 'l0')
; A vararg slot is read by the C side as int64, so every integer argument
; widens at the call.
; CHECK-DAG: call vararg int32 'printf'(void*, ..., int64)
; CHECK-DAG: call int32 'puts'(void*)
; CHECK-DAG: call int32 'atoi'(void*)
; CHECK-DAG: call int32 'strtol'(void*, void*, int32)

; The vararg seam: printf reads its arguments through the CLR's ArgIterator,
; because the backend passes each one as int64, float64 or void*, never as a C
; type. The slot holds a pointer to the iterator (va_start stores it), so the
; accessor dereferences once before copying the iterator in and out.
; CHECK-DAG: .method public static hidebysig int64 '__msil_va_i64'(void*)
; CHECK-DAG: .method public static hidebysig float64 '__msil_va_f64'(void*)
; CHECK-DAG: .method public static hidebysig void* '__msil_va_ptr'(void*)
; CHECK-DAG: call{{.}}instance typedref [mscorlib]System.ArgIterator::GetNextArg(valuetype [mscorlib]System.RuntimeTypeHandle)
; CHECK-DAG: stobj{{.}}valuetype [mscorlib]System.ArgIterator
; A pointer argument is read through object and unboxed as UIntPtr, which is
; how a void* boxes.
; CHECK-DAG: TypedReference::ToObject(typedref)
; CHECK-DAG: unbox{{.}}[mscorlib]System.UIntPtr

; Output goes through Console rather than pinvoking _write.
; CHECK-DAG: .method public static hidebysig void '__msil_write'(void*, int64)
; CHECK-DAG: System.Console::Write(string)

; Nothing the library defines is left as a pinvoke anywhere in the output.
; CHECK-NOT: pinvokeimpl("{{[^"]*}}" as "printf"
; CHECK-NOT: pinvokeimpl("{{[^"]*}}" as "puts"
; CHECK-NOT: pinvokeimpl("{{[^"]*}}" as "atoi"
; CHECK-NOT: pinvokeimpl("{{[^"]*}}" as "strtol"

; Only the parts the program reaches are linked: qsort and putchar are not.
; CHECK-NOT: .method static public hidebysig void 'qsort'(
; CHECK-NOT: .method static public hidebysig int32 'putchar'(
