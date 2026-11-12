; RUN: llvm-msil %s -o - -msil-library -msil-class-name=L | FileCheck %s

; No globals means printInitializer emits no $MSIL_Init at all, but a library
; still needs a stable, callable MSILInitialize().
define i32 @add(i32 %a, i32 %b) {
  %r = add i32 %a, %b
  ret i32 %r
}

; CHECK-NOT: '$MSIL_Init'
; CHECK: .method public static void 'MSILInitialize'()
; CHECK: .maxstack
; CHECK-NEXT: ldsfld bool 'L'::'$MSIL_Initialized'
; CHECK-NEXT: brtrue
; CHECK: ret
; CHECK-NOT: .entrypoint
