; RUN: llvm-msil %s -o - | FileCheck %s

; Intrinsics that stock IntrinsicLowering does not know: the C99 libm
; functions, the IEEE min/max family, and a handful of integer and
; classification intrinsics. Each has to become a call or an expansion rather
; than the fatal "Code generator does not support intrinsic function".

declare double @llvm.rint.f64(double)
declare float @llvm.rint.f32(float)
declare double @llvm.sinh.f64(double)
declare double @llvm.tanh.f64(double)
declare double @llvm.atan.f64(double)
declare i64 @llvm.llrint.f64(double)
declare double @llvm.exp10.f64(double)

; These live in ucrtbase.dll, not msvcrt.dll; the C99 spelling is not exported
; from the classic CRT at all.
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "rint" cdecl)
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "rintf" cdecl)
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "sinh" cdecl)
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "tanh" cdecl)
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "atan" cdecl)
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "llrint" cdecl)
; CHECK-DAG: pinvokeimpl("ucrtbase.dll" as "exp10" cdecl)
define double @libm(double %x) {
  %a = call double @llvm.rint.f64(double %x)
  %b = call double @llvm.sinh.f64(double %a)
  %c = call double @llvm.tanh.f64(double %b)
  %d = call double @llvm.atan.f64(double %c)
  %e = call double @llvm.exp10.f64(double %d)
  ret double %e
}

define i64 @libm_i64(double %x) {
  %r = call i64 @llvm.llrint.f64(double %x)
  ret i64 %r
}

define float @libm_f32(float %x) {
  %r = call float @llvm.rint.f32(float %x)
  ret float %r
}

declare double @llvm.minnum.f64(double, double)
declare double @llvm.maxnum.f64(double, double)
declare double @llvm.minimum.f64(double, double)
declare double @llvm.maximum.f64(double, double)

; The "num" forms are exactly fmin/fmax.
; CHECK-LABEL: 'minnum'
; CHECK: call {{.*}}'fmin'
; CHECK-LABEL: 'maxnum'
; CHECK: call {{.*}}'fmax'
define double @minnum(double %a, double %b) {
  %r = call double @llvm.minnum.f64(double %a, double %b)
  ret double %r
}
define double @maxnum(double %a, double %b) {
  %r = call double @llvm.maxnum.f64(double %a, double %b)
  ret double %r
}

; The propagating forms are a compare and two selects, with an unordered test
; picking the NaN operand.
; CHECK-LABEL: 'minimum'
; CHECK: fcmp olt
; CHECK: fcmp uno
; CHECK: select
define double @minimum(double %a, double %b) {
  %r = call double @llvm.minimum.f64(double %a, double %b)
  ret double %r
}
define double @maximum(double %a, double %b) {
  %r = call double @llvm.maximum.f64(double %a, double %b)
  ret double %r
}

declare double @llvm.powi.f64.i32(double, i32)
declare double @llvm.ldexp.f64.i32(double, i32)
declare i32 @llvm.fptosi.sat.i32.f64(double)
declare i32 @llvm.scmp.i32.i32(i32, i32)
declare i32 @llvm.ptrmask.p0.i64(ptr, i64)

; powi is square-and-multiply plus a reciprocal for a negative exponent.
; CHECK-LABEL: 'powi'
; CHECK: mul
; CHECK: div
; CHECK: select
define double @powi(double %b, i32 %e) {
  %r = call double @llvm.powi.f64.i32(double %b, i32 %e)
  ret double %r
}

; ldexp is the libm call, which takes an int exponent like the intrinsic.
; CHECK-LABEL: 'ldexp'
; CHECK: call {{.*}}'ldexp'
define double @ldexp(double %x, i32 %e) {
  %r = call double @llvm.ldexp.f64.i32(double %x, i32 %e)
  ret double %r
}

; The saturating float-to-int conversion clamps before converting, so the
; conversion itself never sees an out-of-range value.
; CHECK-LABEL: 'fptosi_sat'
; CHECK: fcmp
; CHECK: conv.i4
; CHECK: select
define i32 @fptosi_sat(double %x) {
  %r = call i32 @llvm.fptosi.sat.i32.f64(double %x)
  ret i32 %r
}

; scmp returns the sign of the comparison, -1/0/1.
; CHECK-LABEL: 'scmp'
; CHECK-DAG: ldc.i4	-1
; CHECK-DAG: ldc.i4	1
define i32 @scmp(i32 %a, i32 %b) {
  %r = call i32 @llvm.scmp.i32.i32(i32 %a, i32 %b)
  ret i32 %r
}

; A pointer's bits are its address, so ptrmask masks the integer.
; CHECK-LABEL: 'pmask'
; CHECK: conv.u8
; CHECK: and
; CHECK: conv.u
define ptr @pmask(ptr %p, i64 %m) {
  %r = call i32 @llvm.ptrmask.p0.i64(ptr %p, i64 %m)
  ret ptr %p
}

declare double @llvm.canonicalize.f64(double)
declare void @llvm.assume(i1)
declare void @llvm.donothing()

; A NaN or infinity constant has to be emitted as a bit pattern: the only
; other spelling ilasm takes, "0.0/0.0", is not a constant expression it
; accepts, and its hex form loads as zero.
; CHECK-LABEL: 'nanconst'
; CHECK: ldc.r8 (00 00 00 00 00 00 f8 7f)
; CHECK: ldc.r8 (00 00 00 00 00 00 f0 7f)
define double @nanconst() {
  ret double 0x7FF8000000000000
}
define double @infconst() {
  ret double 0x7FF0000000000000
}

define double @keep(double %x) {
  call void @llvm.assume(i1 true)
  call void @llvm.donothing()
  %c = call double @llvm.canonicalize.f64(double %x)
  ret double %c
}
