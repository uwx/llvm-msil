; RUN: llvm-msil %s -o - | FileCheck %s

; a vector bitcast keeps the bits, so it goes through memory of the source
; type. i1 lanes are a byte each in the blob and a bit each in the integer.

target triple = "x86_64-pc-windows-msvc"

; CHECK-LABEL: 'to_scalar'(
; CHECK: stloc '[[SRC:t[0-9]+]]'
; CHECK-NEXT: ldloca '[[SRC]]'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: ldind.i8
define i64 @to_scalar(<2 x i32> %v) {
  %r = bitcast <2 x i32> %v to i64
  ret i64 %r
}

; CHECK-LABEL: 'reshape'(
; CHECK: ldloca '[[SRC:t[0-9]+]]'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: ldobj valuetype
define <4 x i16> @reshape(<2 x i32> %v) {
  %r = bitcast <2 x i32> %v to <4 x i16>
  ret <4 x i16> %r
}

; CHECK-LABEL: 'pack'(
; CHECK: ldc.i4.0
; CHECK: ldind.u1
; CHECK-NEXT: ldc.i4.1
; CHECK-NEXT: and
; CHECK-NEXT: ldc.i4 0
; CHECK-NEXT: shl
; CHECK-NEXT: or
; CHECK: ldc.i4 7
; CHECK-NEXT: shl
; CHECK-NEXT: or
define i8 @pack(<8 x i1> %m) {
  %r = bitcast <8 x i1> %m to i8
  ret i8 %r
}

; CHECK-LABEL: 'unpack'(
; CHECK: ldc.i4 7
; CHECK-NEXT: shr.un
; CHECK-NEXT: ldc.i4.1
; CHECK-NEXT: and
; CHECK-NEXT: stind.i1
define <8 x i1> @unpack(i8 %b) {
  %r = bitcast i8 %b to <8 x i1>
  ret <8 x i1> %r
}
