; RUN: llvm-msil %s -o - | FileCheck %s

; A vararg call to an external function becomes a concrete pinvoke thunk,
; because ilasm rejects `vararg` pinvokes. The call site names the exact
; trailing argument types.

declare i32 @printf(ptr, ...)

; CHECK-DAG: .method public static hidebysig pinvokeimpl("{{[^"]*}}" as "printf" cdecl) int32 printf${{[0-9]+}}(void*, int32) preservesig {}
; CHECK-DAG: .method public static hidebysig pinvokeimpl("{{[^"]*}}" as "printf" cdecl) int32 printf${{[0-9]+}}(void*, void*) preservesig {}

@fmt = private constant [4 x i8] c"%d\0A\00"

define i32 @main() {
; CHECK: call int32 'printf${{[0-9]+}}'(void*, int32)
  %r = call i32 (ptr, ...) @printf(ptr @fmt, i32 42)
; CHECK: call int32 'printf${{[0-9]+}}'(void*, void*)
  %s = call i32 (ptr, ...) @printf(ptr @fmt, ptr @fmt)
  %t = add i32 %r, %s
  ret i32 %t
}
