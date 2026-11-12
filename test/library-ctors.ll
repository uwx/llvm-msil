; RUN: llvm-msil %s -o - -msil-library -msil-class-name=L | FileCheck %s

; llvm.global_ctors functions run from MSILInitialize, which is the only
; startup hook a library has.
declare i32 @putchar(i32)

define internal void @ctor() {
  %a = call i32 @putchar(i32 65)
  ret void
}

@llvm.global_ctors = appending global [1 x { i32, ptr, ptr }] [
  { i32, ptr, ptr } { i32 65535, ptr @ctor, ptr null }
]

define i32 @add(i32 %a, i32 %b) {
  %r = add i32 %a, %b
  ret i32 %r
}

; CHECK: .method public static void 'MSILInitialize'()
; CHECK: .maxstack
; CHECK-NEXT: ldsfld bool 'L'::'$MSIL_Initialized'
; CHECK-NEXT: brtrue
; CHECK: call {{.*}}'L'::'ctor'()
; CHECK-NOT: .entrypoint
