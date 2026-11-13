; RUN: llvm-msil %s -o - | FileCheck %s

; The overflow and saturating intrinsics are otherwise checked by computing in
; a type twice as wide. For i128 that would be i256, so these test the result
; itself: a carry or borrow shows in an unsigned compare, a signed wrap in the
; sign bits, and a product by dividing it back.

declare { i128, i1 } @llvm.uadd.with.overflow.i128(i128, i128)
declare { i128, i1 } @llvm.smul.with.overflow.i128(i128, i128)
declare i128 @llvm.sadd.sat.i128(i128, i128)
declare i128 @llvm.usub.sat.i128(i128, i128)

; CHECK-LABEL: 'uadd'
; CHECK: op_Addition
; CHECK: op_LessThan
; CHECK-NOT: int256
define { i128, i1 } @uadd(i128 %a, i128 %b) {
  %r = call { i128, i1 } @llvm.uadd.with.overflow.i128(i128 %a, i128 %b)
  ret { i128, i1 } %r
}

; CHECK-LABEL: 'smul'
; CHECK: op_Multiply
; CHECK: op_Division
define { i128, i1 } @smul(i128 %a, i128 %b) {
  %r = call { i128, i1 } @llvm.smul.with.overflow.i128(i128 %a, i128 %b)
  ret { i128, i1 } %r
}

; CHECK-LABEL: 'ssat'
; CHECK: op_ExclusiveOr
; CHECK: op_ExclusiveOr
; CHECK: op_BitwiseAnd
define i128 @ssat(i128 %a, i128 %b) {
  %r = call i128 @llvm.sadd.sat.i128(i128 %a, i128 %b)
  ret i128 %r
}

; CHECK-LABEL: 'usat'
; CHECK: op_Subtraction
define i128 @usat(i128 %a, i128 %b) {
  %r = call i128 @llvm.usub.sat.i128(i128 %a, i128 %b)
  ret i128 %r
}
