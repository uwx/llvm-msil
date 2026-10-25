; RUN: llvm-msil %s -o - | FileCheck %s

; i128 maps to System.Int128, a .NET value type with no IL arithmetic: every
; operation is a static method call. i33..i64 must stay int64.

; The type lives in System.Runtime, which is referenced only by modules that
; use it, and only with the real identity resolves (see NOTES-platform.md).

@w = internal global i128 5

define i64 @narrower(i64 %a) {
; CHECK-LABEL: .method static {{.*}} 'narrower'(int64
; CHECK: add
  %r = add i64 %a, 1
  ret i64 %r
}

define i128 @arith(i128 %a, i128 %b) {
; CHECK-LABEL: .method static {{.*}} 'arith'(valuetype [System.Runtime]System.Int128
  %add = add i128 %a, %b
; CHECK: call {{.*}}System.Int128::op_Addition
  %sub = sub i128 %add, %b
; CHECK: call {{.*}}System.Int128::op_Subtraction
  %mul = mul i128 %sub, %b
; CHECK: call {{.*}}System.Int128::op_Multiply
  %div = sdiv i128 %mul, %b
; CHECK: call {{.*}}System.Int128::op_Division
  %rem = srem i128 %div, %b
; CHECK: call {{.*}}System.Int128::op_Modulus
; Unsigned division has no Int128 method; it reinterprets as UInt128.
  %udiv = udiv i128 %rem, %b
; CHECK: call {{.*}}System.UInt128 {{.*}}System.Int128::op_Explicit
; CHECK: call {{.*}}System.UInt128::op_Division
; CHECK: call valuetype [System.Runtime]System.Int128 {{.*}}System.UInt128::op_Explicit
  %and = and i128 %udiv, %b
; CHECK: call {{.*}}System.Int128::op_BitwiseAnd
  %or = or i128 %and, %b
; CHECK: call {{.*}}System.Int128::op_BitwiseOr
  %xor = xor i128 %or, %b
; CHECK: call {{.*}}System.Int128::op_ExclusiveOr
  ret i128 %xor
}

define i128 @shifts(i128 %a, i128 %n) {
; CHECK-LABEL: .method static {{.*}} 'shifts'
  %l = shl i128 %a, %n
; The count is i128, so it has to reach int64 before conv.i4.
; CHECK: call int64 {{.*}}System.Int128::op_Explicit
; CHECK-NEXT: conv.i4
; CHECK-NEXT: call {{.*}}System.Int128::op_LeftShift({{.*}}, int32)
  %r = lshr i128 %l, %n
; CHECK: call {{.*}}System.Int128::op_UnsignedRightShift
  %s = ashr i128 %r, %n
; CHECK: call {{.*}}System.Int128::op_RightShift
  ret i128 %s
}

define i1 @cmp(i128 %a, i128 %b) {
; CHECK-LABEL: .method static {{.*}} 'cmp'
  %eq = icmp eq i128 %a, %b
; CHECK: call bool {{.*}}System.Int128::op_Equality
  %lt = icmp slt i128 %a, %b
; CHECK: call bool {{.*}}System.Int128::op_LessThan(valuetype [System.Runtime]System.Int128
; An unsigned predicate compares the UInt128 conversion.
  %ugt = icmp ugt i128 %a, %b
; CHECK: call valuetype [System.Runtime]System.UInt128 {{.*}}System.Int128::op_Explicit
; CHECK: call bool {{.*}}System.UInt128::op_GreaterThan
  %sle = icmp sle i128 %a, %b
; CHECK: op_GreaterThan
; CHECK-NEXT: ldc.i4.0
; CHECK-NEXT: ceq
  %r = and i1 %eq, %lt
  %r2 = and i1 %r, %ugt
  %r3 = and i1 %r2, %sle
  ret i1 %r3
}

define i128 @casts(i128 %a, i64 %n, double %d, i32 %s) {
; CHECK-LABEL: .method static {{.*}} 'casts'
  %t = trunc i128 %a to i32
; CHECK: call int64 {{.*}}System.Int128::op_Explicit
; CHECK-NEXT: conv.i4
  %z = zext i32 %s to i128
; CHECK: conv.u8
; CHECK-NEXT: call {{.*}}System.Int128::op_Implicit(uint64)
  %e = sext i32 %s to i128
; CHECK: conv.i8
; CHECK-NEXT: call {{.*}}System.Int128::op_Implicit(int64)
  %f = sitofp i128 %a to double
; CHECK: call float64 {{.*}}System.Int128::op_Explicit
  %g = fptoui double %d to i128
; CHECK: call {{.*}}System.UInt128::op_Explicit(float64)
  %h = uitofp i128 %a to float
; CHECK: call float64 {{.*}}System.UInt128::op_Explicit
; CHECK-NEXT: conv.r4
  %i = fptosi double %d to i128
; CHECK: call {{.*}}System.Int128::op_Explicit(float64)
  %from64 = zext i64 %n to i128
; CHECK: call {{.*}}System.Int128::op_Implicit(uint64)
  ret i128 %i
}

define i128 @memory(i128 %v) {
; CHECK-LABEL: .method static {{.*}} 'memory'
; Value types move with ldobj/stobj, not ldind/stind.
  %a = load i128, ptr @w
; CHECK: ldobj{{.*}}System.Int128
  store i128 %a, ptr @w
; CHECK: stobj{{.*}}System.Int128
  ret i128 %a
}

define i128 @constants() {
; CHECK-LABEL: .method static {{.*}} 'constants'
; A small constant goes through op_Implicit; there is no ldc for a value type.
; CHECK: ldc.i8 5
; CHECK-NEXT: call valuetype [System.Runtime]System.Int128 {{.*}}System.Int128::op_Implicit(int64)
  ret i128 5
}

define i128 @wide_constant() {
; CHECK-LABEL: .method static {{.*}} 'wide_constant'
; A value that does not fit int64 is built with the .ctor(upper, lower).
; CHECK: ldc.i8 1
; CHECK-NEXT: ldc.i8 5
; CHECK-NEXT: newobj instance void {{.*}}System.Int128::.ctor(uint64, uint64)
  ret i128 18446744073709551621
}

