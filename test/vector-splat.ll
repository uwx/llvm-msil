; RUN: llvm-msil %s -o - | FileCheck %s

; newer LLVM hands splats over as vector-typed ConstantInt/ConstantFP, they
; still have to be built lane by lane, not pushed as one scalar

target triple = "x86_64-pc-windows-msvc"

; CHECK: .data 'ints$tpl$data' = { int32 (3), int32 (3), int32 (3), int32 (3) }
@ints = global <4 x i32> splat (i32 3)

; CHECK: .data 'floats$tpl$data' = { int32 (1065353216), int32 (1065353216) }
@floats = global <2 x float> splat (float 1.0)

; CHECK-LABEL: 'scale'(
; CHECK: initobj valuetype '[[VEC:\$agg[0-9]+]]'
; CHECK-NOT: ldc.r8
; CHECK: ldc.r4 2.55E+2
; CHECK-NEXT: stind.r4
; CHECK: ldc.r4 2.55E+2
; CHECK-NEXT: stind.r4
; CHECK: ldc.r4 2.55E+2
; CHECK-NEXT: stind.r4
; CHECK: ldc.r4 2.55E+2
; CHECK-NEXT: stind.r4
; CHECK: div
define <4 x float> @scale(<4 x float> %v) {
  %r = fdiv <4 x float> %v, splat (float 2.550000e+02)
  ret <4 x float> %r
}

; CHECK-LABEL: 'bump'(
; CHECK: ldc.i4 7
; CHECK-NEXT: stind.i4
; CHECK: ldc.i4 7
; CHECK-NEXT: stind.i4
; CHECK: ldc.i4 7
; CHECK-NEXT: stind.i4
; CHECK: ldc.i4 7
; CHECK-NEXT: stind.i4
; CHECK: add
define <4 x i32> @bump(<4 x i32> %v) {
  %r = add <4 x i32> %v, splat (i32 7)
  ret <4 x i32> %r
}
