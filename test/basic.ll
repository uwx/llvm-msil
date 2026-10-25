; RUN: llvm-msil %s -o - | FileCheck %s

; A function, a global with a writable template, and an entry point.

@g = internal global i32 42

; CHECK: .assembly extern mscorlib
; CHECK: .assembly MSIL
; CHECK: .field static private valuetype {{.*}} 'g$tpl'
; CHECK: .data 'g$tpl$data' = { int32 (42) }

define i32 @add(i32 %a, i32 %b) {
; CHECK-LABEL: .method static {{.*}} 'add'
; CHECK: ldarg
; CHECK: ldarg
; CHECK: add
; CHECK: ret
  %r = add i32 %a, %b
  ret i32 %r
}

define i32 @main() {
; CHECK-LABEL: .method static {{.*}} 'main'
; CHECK: call
; CHECK: ret
  %v = load i32, ptr @g
  %r = call i32 @add(i32 %v, i32 1)
  ret i32 %r
}

; CHECK-LABEL: '$MSIL_Startup'
; CHECK: .entrypoint
; CHECK: call void '$MSIL_Init'()
