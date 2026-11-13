; RUN: llvm-msil %s -o - -msil-libc=builtin \
; RUN:   -msil-lib-map='*=libc.so.6' | FileCheck %s

; The ELF half of -msil-libc=builtin. The library is compiled twice, once per
; ABI, and linkPortableLibc picks by the module's triple; portable-libc.ll
; covers the Windows copy and this covers the other. They differ in more than
; sizeof(long): `strtol` returns long, so its return type is int64 here and
; int32 there, and the copy a program links has to match the declaration its
; own front end emitted.
;
; This is the only coverage the ELF copy has in CI, and it is here because it
; was missing: the ELF copy was built position-independent, so it contained
; llvm.load.relative from its switch tables, which the backend cannot lower.
; Translation aborted and only the program test noticed (on Linux only). A
; translation this file performs is the check that the copy stays lowerable.

target triple = "x86_64-unknown-linux-gnu"

declare i32 @printf(ptr, ...)
declare i32 @puts(ptr)
declare i64 @strtol(ptr, ptr, i32)
declare i32 @toupper(i32)
declare i64 @strlen(ptr)

@fmt = private constant [4 x i8] c"%d\0A\00"

define i32 @main() {
  %p = call i32 (ptr, ...) @printf(ptr @fmt, i32 42)
  %q = call i32 @puts(ptr @fmt)
  %s = call i64 @strtol(ptr @fmt, ptr null, i32 10)
  %u = call i32 @toupper(i32 97)
  %l = call i64 @strlen(ptr @fmt)
  ret i32 %p
}

; The definitions carry the C names and the SysV long.
; CHECK-DAG: .method static public hidebysig int32 'printf'(void* 'l0', void* '$va_args') cil managed
; CHECK-DAG: .method static public hidebysig int32 'puts'(void* 'l0') cil managed
; CHECK-DAG: .method static public hidebysig int64 'strtol'(void* 'l0', void* 'l1', int32 'l2') cil managed
; CHECK-DAG: .method static public hidebysig int32 'toupper'(int32 'l0') cil managed

; And the calls reach them at that width rather than through a pinvoke.
; CHECK-DAG: call int32 'printf'(void*, void*)
; CHECK-DAG: call int32 'puts'(void*)
; CHECK-DAG: call int64 'strtol'(void*, void*, int32)

; The seam is the same on both ABIs except for what a va_list is, and the
; accessors take its address either way.
; CHECK-DAG: .method public static {{.*}}int64 '__msil_va_i64'(void*)
; CHECK-DAG: .method public static {{.*}}void* '__msil_va_ptr'(void*)

; Nothing from the library is left as a pinvoke.
; CHECK-NOT: pinvokeimpl("{{[^"]*}}" as "printf"
; CHECK-NOT: pinvokeimpl("{{[^"]*}}" as "strtol"
