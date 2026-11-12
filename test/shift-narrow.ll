; RUN: llvm-msil %s -o - | FileCheck %s

; lshr is a logical shift of the declared width. The IL stack is int32, and
; loading an i8/i16 argument sign-extends it, so the emitter must zero-extend
; before shr.un. Without that, lshr i8 0xf0, 2 produces 0xfc instead of 0x3c.
define i8 @shr8(i8 %x) {
  %r = lshr i8 %x, 2
  ret i8 %r
}

define i16 @shr16(i16 %x) {
  %r = lshr i16 %x, 4
  ret i16 %r
}

; CHECK: .method {{.*}} 'shr8'
; CHECK: ldarg {{.*}}
; CHECK-NEXT: conv.u1
; CHECK-NEXT: ldc.i4 2
; CHECK-NEXT: conv.i4
; CHECK-NEXT: shr.un
; CHECK: .method {{.*}} 'shr16'
; CHECK: ldarg {{.*}}
; CHECK-NEXT: conv.u2
; CHECK-NEXT: ldc.i4 4
; CHECK-NEXT: conv.i4
; CHECK-NEXT: shr.un

; A width with no CLR type of its own (i24 here, which an APInt constant can
; produce) has no conv that isolates its bits: conv.u1 would mask to 8. The
; emitter masks to the declared width instead.
define i8 @shr24(i8 %n) {
  %s = shl i8 %n, 3
  %z = zext i8 %s to i24
  %r = lshr i24 -5567990, %z
  %t = trunc i24 %r to i8
  ret i8 %t
}

; CHECK: .method {{.*}} 'shr24'
; CHECK: ldc.i4 -5567990
; CHECK-NEXT: ldc.i4 16777215
; CHECK-NEXT: and
; CHECK-NEXT: ldloc {{.*}}
; CHECK-NEXT: conv.i4
; CHECK-NEXT: shr.un
