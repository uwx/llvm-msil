; RUN: llvm-msil %s -o - | FileCheck %s

; A vector whose bit width is exactly one SIMD register (Vector<T>.Count *
; sizeof(T) == 32 bytes here) runs on System.Numerics.Vector<T>. Every other
; shape keeps the lane-by-lane expansion into a byte blob, which is the only
; thing that can handle the remaining lane counts, the narrow element types
; and the operations Vector<T> does not have.

; === SIMD: <8 x i32> is exactly Vector<int32> ===============================

; The Vector<T> assembly reference is emitted only when a type needs it, and
; the references are printed before any method, so this check comes first.
; CHECK: .assembly extern System.Numerics.Vectors

define <8 x i32> @simd_add(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_add'
; CHECK: .locals init (valuetype [System.Numerics.Vectors]System.Numerics.Vector`1<int32>
; CHECK: call valuetype [System.Numerics.Vectors]System.Numerics.Vector`1<!!0> [System.Numerics.Vectors]System.Numerics.Vector::Add<int32>
  %r = add <8 x i32> %a, %b
  ret <8 x i32> %r
}

define <8 x i32> @simd_mul(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_mul'
; CHECK: Vector::Multiply<int32>
  %r = mul <8 x i32> %a, %b
  ret <8 x i32> %r
}

; An unsigned divide goes through the uint32 spelling.
define <8 x i32> @simd_udiv(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_udiv'
; CHECK: Vector::AsVectorUInt32<int32>
; CHECK: Vector::Divide<uint32>
; CHECK: Vector::AsVectorInt32<uint32>
  %r = udiv <8 x i32> %a, %b
  ret <8 x i32> %r
}

; A shift takes a scalar count, so lane 0 of the count vector is read.
define <8 x i32> @simd_shl(<8 x i32> %a, <8 x i32> %s) {
; CHECK-LABEL: .method static {{.*}} 'simd_shl'
; CHECK: Vector::ShiftLeft(valuetype [System.Numerics.Vectors]System.Numerics.Vector`1<int32>, int32)
  %r = shl <8 x i32> %a, %s
  ret <8 x i32> %r
}

; A compare only reaches the register when its mask is consumed by a select:
; a Vector<T> mask is as wide as the operands, but an i1 vector is a packed
; blob, so a materialised i1 result keeps the lane path.
define <8 x i32> @simd_select(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_select'
; CHECK: Vector::LessThan<int32>
; CHECK: Vector::ConditionalSelect<int32>
  %c = icmp slt <8 x i32> %a, %b
  %r = select <8 x i1> %c, <8 x i32> %a, <8 x i32> %b
  ret <8 x i32> %r
}

; <4 x double> is one register too.
define <4 x double> @simd_dadd(<4 x double> %a, <4 x double> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_dadd'
; CHECK: Vector::Add<float64>
  %r = fadd <4 x double> %a, %b
  ret <4 x double> %r
}

; Loading and storing a whole register is a plain ldobj/stobj of the vector.
define void @simd_mem(ptr %p, ptr %q) {
; CHECK-LABEL: .method static {{.*}} 'simd_mem'
; CHECK: ldobj valuetype [System.Numerics.Vectors]System.Numerics.Vector`1<int32>
; CHECK: Vector::Add<int32>
; CHECK: stobj valuetype [System.Numerics.Vectors]System.Numerics.Vector`1<int32>
  %a = load <8 x i32>, ptr %p
  %b = load <8 x i32>, ptr %p
  %r = add <8 x i32> %a, %b
  store <8 x i32> %r, ptr %q
  ret void
}

; === Scalar fallback ========================================================

; <4 x i32> is half a register, so it stays a byte blob expanded lane by lane.
define <4 x i32> @scalar_add(<4 x i32> %a, <4 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'scalar_add'
; CHECK-NOT: System.Numerics.Vector
; CHECK: ldind.i4
; CHECK: stind.i4
  %r = add <4 x i32> %a, %b
  ret <4 x i32> %r
}

; A register's worth of i8 lanes, but the CLR has no 8-bit SIMD arithmetic.
define <32 x i8> @scalar_i8(<32 x i8> %a, <32 x i8> %b) {
; CHECK-LABEL: .method static {{.*}} 'scalar_i8'
; CHECK-NOT: System.Numerics.Vector
; CHECK: ldind.i1
  %r = add <32 x i8> %a, %b
  ret <32 x i8> %r
}

; An unsigned lane op has to clear the lanes' high bits, because ldind.i4
; leaves a 32-bit lane alone but the old code re-extended every lane as if it
; were 16 bits wide, so `urem` on an i32 lane produced `(lane & 0xffff) % rhs`.
; A 4-byte lane needs no conversion at all; only the 1- and 2-byte ones do.
define <8 x i32> @scalar_urem(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'scalar_urem'
; CHECK-NOT: Vector::Rem
; CHECK: ldind.i4
; CHECK-NOT: conv.u2
; CHECK: rem.un
  %r = urem <8 x i32> %a, %b
  ret <8 x i32> %r
}

; Vector<T> has no remainder, so it keeps the lane path.
define <8 x i32> @scalar_srem(<8 x i32> %a, <8 x i32> %b) {
; CHECK-LABEL: .method static {{.*}} 'scalar_srem'
; CHECK-NOT: System.Numerics.Vector::Rem
; CHECK: rem
  %r = srem <8 x i32> %a, %b
  ret <8 x i32> %r
}

; The bitwise helpers are named for the operation, not for the CIL operator:
; `xor` is Vector::Xor. Vector::ExclusiveOr does not exist, and emitting it
; assembles fine but throws MissingMethodException the first time it runs.
define <4 x i64> @simd_xor(<4 x i64> %a, <4 x i64> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_xor'
; CHECK: Vector::Xor<int64>
; CHECK-NOT: Vector::ExclusiveOr
  %r = xor <4 x i64> %a, %b
  ret <4 x i64> %r
}

define <4 x i64> @simd_andor(<4 x i64> %a, <4 x i64> %b) {
; CHECK-LABEL: .method static {{.*}} 'simd_andor'
; CHECK: Vector::BitwiseAnd<int64>
; CHECK: Vector::BitwiseOr<int64>
  %m = and <4 x i64> %a, %b
  %r = or <4 x i64> %m, %b
  ret <4 x i64> %r
}

; A GEP with a vector index produces one pointer per lane, always scalar.
@tbl = internal global [8 x i32] zeroinitializer

define i32 @vec_gep(<4 x i64> %idx) {
; CHECK-LABEL: .method static {{.*}} 'vec_gep'
  %p = getelementptr i32, ptr @tbl, <4 x i64> %idx
  %e = extractelement <4 x ptr> %p, i64 2
  %v = load i32, ptr %e
  ret i32 %v
}

; Reductions still fold through the lane chain.
define i32 @vec_reduce(<4 x i32> %v) {
; CHECK-LABEL: .method static {{.*}} 'vec_reduce'
; CHECK: add
  %r = call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %v)
  ret i32 %r
}

declare i32 @llvm.vector.reduce.add.v4i32(<4 x i32>)
