; RUN: llvm-msil %s -o - | FileCheck %s

; clang coerces a 3- or 6-byte struct to i24/i48 at -O0, and _BitInt makes any
; width. The CLR has no 3/5/6/7-byte access, so memory goes in power-of-two
; pieces that only touch the value's own bytes, and signatures use the slot
; the alloc size implies.

target datalayout = "e-m:e-p:64:64-i64:64-i128:128-n8:16:32:64-S128"

; CHECK-LABEL: int32 'load24'(void*
; CHECK: ldind.i2
; CHECK: ldind.i1
; CHECK: ldc.i4 16
; CHECK-NEXT: conv.i4
; CHECK-NEXT: shl
; CHECK-NOT: ldind.i4
define i24 @load24(ptr %p) {
  %v = load i24, ptr %p, align 4
  ret i24 %v
}

; CHECK-LABEL: void 'store48'(void*
; CHECK: stind.i4
; CHECK: ldc.i8 32
; CHECK-NEXT: conv.i4
; CHECK: shr.un
; CHECK: stind.i2
; CHECK-NOT: stind.i8
define void @store48(ptr %p, i48 %v) {
  store i48 %v, ptr %p, align 8
  ret void
}

; CHECK-LABEL: 'store56'
; CHECK: stind.i4
; CHECK: stind.i2
; CHECK: stind.i1
define void @store56(ptr %p, i56 %v) {
  store i56 %v, ptr %p, align 8
  ret void
}

; CHECK-LABEL: int8 'narrow3'(int8
define i3 @narrow3(i3 %x) {
  ret i3 %x
}

; CHECK-LABEL: int16 'narrow13'(int16
define i13 @narrow13(i13 %x) {
  ret i13 %x
}

; CHECK-LABEL: int32 'narrow28'(int32
define i28 @narrow28(i28 %x) {
  ret i28 %x
}
