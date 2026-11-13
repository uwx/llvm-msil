; RUN: llvm-msil %s -o - | FileCheck %s

; the ordered/unordered NaN checks reload both lanes, a vector compare has to
; push them again rather than assume they are still on the stack

target triple = "x86_64-pc-windows-msvc"

; CHECK-LABEL: 'ogt'(
; CHECK: cgt
; CHECK-NOT: ceq
; CHECK: ldind.r4
; CHECK-NEXT: ldloca
; CHECK: ldind.r4
; CHECK-NEXT: ceq
; CHECK-NEXT: and
; CHECK: ldind.r4
; CHECK-NEXT: ldloca
; CHECK: ldind.r4
; CHECK-NEXT: ceq
; CHECK-NEXT: and
; CHECK-NEXT: stind.i1
define <2 x i1> @ogt(<2 x float> %a, <2 x float> %b) {
  %r = fcmp ogt <2 x float> %a, %b
  ret <2 x i1> %r
}

; CHECK-LABEL: 'uno'(
; CHECK: ldind.r4
; CHECK-NEXT: ldloca
; CHECK: ldind.r4
; CHECK-NEXT: ceq
define <2 x i1> @uno(<2 x float> %a, <2 x float> %b) {
  %r = fcmp uno <2 x float> %a, %b
  ret <2 x i1> %r
}
