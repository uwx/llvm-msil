; RUN: llvm-msil %s -o - | FileCheck %s

; llvm.frexp and llvm.modf return a pair that the C functions hand back through
; an out pointer. A float goes through the double function, which is exact.

target triple = "x86_64-unknown-linux-gnu"

declare { double, i32 } @llvm.frexp.f64.i32(double)
declare { float, float } @llvm.modf.f32(float)

; CHECK-LABEL: 'split_exp'
; CHECK: call float64 'frexp'(float64, void*)
define i32 @split_exp(double %x) {
  %r = call { double, i32 } @llvm.frexp.f64.i32(double %x)
  %e = extractvalue { double, i32 } %r, 1
  ret i32 %e
}

; CHECK-LABEL: 'split_int'
; CHECK: conv.r8
; CHECK: call float64 'modf'(float64, void*)
; CHECK: conv.r4
define float @split_int(float %x) {
  %r = call { float, float } @llvm.modf.f32(float %x)
  %whole = extractvalue { float, float } %r, 1
  ret float %whole
}
