; RUN: llvm-msil %s -o - | FileCheck %s

; A vararg call to an external function becomes a concrete pinvoke thunk,
; because ilasm rejects `vararg` pinvokes. The call site names the exact
; trailing argument types.

declare i32 @printf(ptr, ...)

; CHECK-DAG: .method public static hidebysig pinvokeimpl("{{[^"]*}}" as "printf" cdecl) int32 printf${{[0-9]+}}(void*, int32) preservesig {}
; CHECK-DAG: .method public static hidebysig pinvokeimpl("{{[^"]*}}" as "printf" cdecl) int32 printf${{[0-9]+}}(void*, void*) preservesig {}
; CHECK-DAG: pinvokeimpl("{{[^"]*}}" as "__stdio_common_vsprintf" cdecl)

@fmt = private constant [4 x i8] c"%d\0A\00"

define i32 @main() {
; CHECK: call int32 'printf${{[0-9]+}}'(void*, int32)
  %r = call i32 (ptr, ...) @printf(ptr @fmt, i32 42)
; CHECK: call int32 'printf${{[0-9]+}}'(void*, void*)
  %s = call i32 (ptr, ...) @printf(ptr @fmt, ptr @fmt)
  %t = add i32 %r, %s
  ret i32 %t
}

; A vararg definition whose *own name* is a C library entry cannot work when
; it forwards to another printf family entry point: it passes a va_list that
; only exists in the native ABI. Its body is dropped so calls bind to the
; exported function through a pinvoke thunk.
; The forward can be two hops and must not depend on the intermediate being a
; declaration already: snprintf wraps vsnprintf, which is the definition that
; reaches __stdio_common_vsprintf. Matching the callee by name drops the outer
; wrapper even though the inner one is still a definition at that point.
; CHECK-NOT: .method static {{.*}} 'snprintf'(
;
; A wrapper with any other name is NOT a C library entry, so dropping it would
; leave thunks pinvoking a symbol the CRT does not export -- an exception the
; caller's error path then re-enters. SDL's SDL_sscanf forwards to the same
; __stdio_common_* leaf and is exactly this case, so it keeps a body and calls
; resolve locally instead.
; CHECK: .method static {{.*}} vararg {{.*}} 'my_snprintf'(
; CHECK: '$MSIL_Startup'
define i32 @my_snprintf(ptr %b, i64 %n, ptr %f, ...) {
  %ap = alloca ptr
  call void @llvm.va_start(ptr %ap)
  %a = load ptr, ptr %ap
  %r = call i32 (ptr, i64, ptr, ...) @snprintf(ptr %b, i64 %n, ptr %f, ptr %a)
  call void @llvm.va_end(ptr %ap)
  ret i32 %r
}
define i32 @snprintf(ptr %b, i64 %n, ptr %f, ...) {
  %ap = alloca ptr
  call void @llvm.va_start(ptr %ap)
  %a = load ptr, ptr %ap
  %r = call i32 @__stdio_common_vsprintf(i64 0, ptr %b, i64 %n, ptr %f, ptr null, ptr %a)
  call void @llvm.va_end(ptr %ap)
  ret i32 %r
}
declare i32 @__stdio_common_vsprintf(i64, ptr, i64, ptr, ptr, ptr)
declare void @llvm.va_start(ptr)
