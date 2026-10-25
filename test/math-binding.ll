; RUN: llvm-msil %s -o - | FileCheck %s --check-prefix=MATH
; RUN: llvm-msil %s -o - --msil-math=0 | FileCheck %s --check-prefix=CRT

; libm calls bind to System.Math/System.MathF rather than pinvoking a CRT
; that may not export them. The externals are all emitted before any body, so
; the checks for them come first.

declare double @llvm.sqrt.f64(double)
declare double @llvm.sin.f64(double)
declare double @llvm.floor.f64(double)
declare double @llvm.ceil.f64(double)
declare double @llvm.trunc.f64(double)
declare double @llvm.round.f64(double)
declare double @llvm.roundeven.f64(double)
declare double @llvm.rint.f64(double)
declare double @llvm.sinh.f64(double)
declare double @llvm.copysign.f64(double, double)
declare double @llvm.pow.f64(double, double)
declare double @llvm.ldexp.f64.i32(double, i32)
declare double @llvm.powi.f64.i32(double, i32)
declare float  @llvm.sqrt.f32(float)
declare double @llvm.minnum.f64(double, double)
declare double @llvm.maxnum.f64(double, double)

; The System.Runtime reference accompanies the first Math call.
; MATH-DAG: .assembly extern System.Runtime
; MATH-DAG: System.Math::Sqrt(float64)
; MATH-DAG: System.MathF::Sqrt(float32)
; MATH-DAG: System.Math::Sin(float64)
; MATH-DAG: System.Math::Floor(float64)
; MATH-DAG: System.Math::Ceiling(float64)
; MATH-DAG: System.Math::Truncate(float64)
; MATH-DAG: System.Math::Sinh(float64)
; MATH-DAG: System.Math::CopySign(float64, float64)
; MATH-DAG: System.Math::Pow(float64, float64)
; MATH-DAG: System.Math::ScaleB(float64, int32)
; MATH-DAG: System.Math::Round(float64)
; C's round() rounds half away from zero; Math.Round rounds half to even, so
; the forwarder passes the mode argument.
; MATH-DAG: System.Math::Round(float64, valuetype [System.Runtime]System.MidpointRounding)

; The num forms are fmin/fmax, which have the required NaN behaviour
; (Math.Min/Max propagate NaN, so they are deliberately not used here).
; MATH-DAG: pinvokeimpl({{.*}}"fmin"
; MATH-DAG: pinvokeimpl({{.*}}"fmax"

; With the binding off, every one of them is a pinvoke again and no Math
; reference is emitted.
; CRT-NOT: System.Runtime
; CRT-DAG: pinvokeimpl({{.*}}"sqrt"
; CRT-DAG: pinvokeimpl({{.*}}"pow"
; ldexp lowers to a scalbn call in both modes.
; CRT-DAG: pinvokeimpl({{.*}}"scalbn"

define double @f(double %x, double %y, i32 %n) {
  %a = call double @llvm.sqrt.f64(double %x)
  %b = call double @llvm.sin.f64(double %a)
  %c = call double @llvm.floor.f64(double %b)
  %d = call double @llvm.ceil.f64(double %c)
  %e = call double @llvm.trunc.f64(double %d)
  %f1 = call double @llvm.round.f64(double %e)
  %g = call double @llvm.roundeven.f64(double %f1)
  %h = call double @llvm.rint.f64(double %g)
  %i = call double @llvm.sinh.f64(double %h)
  %j = call double @llvm.copysign.f64(double %i, double %y)
  %k = call double @llvm.pow.f64(double %j, double %y)
  %l = call double @llvm.ldexp.f64.i32(double %k, i32 %n)
  %m = call double @llvm.powi.f64.i32(double %l, i32 %n)
  %n1 = call double @llvm.minnum.f64(double %m, double %y)
  %o = call double @llvm.maxnum.f64(double %n1, double %y)
  ret double %o
}

define float @g(float %x) {
  %a = call float @llvm.sqrt.f32(float %x)
  ret float %a
}
