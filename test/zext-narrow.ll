; RUN: llvm-msil %s -o - | FileCheck %s

; A narrow argument is sign-extended onto the int32 stack. zext must mask it
; back to the declared width, or 0xf0 becomes 0xfffffff0.
define i32 @z8_32(i8 %x) {
  %r = zext i8 %x to i32
  ret i32 %r
}

define i64 @z8_64(i8 %x) {
  %r = zext i8 %x to i64
  ret i64 %r
}

define i32 @z16_32(i16 %x) {
  %r = zext i16 %x to i32
  ret i32 %r
}

; CHECK: .method {{.*}} 'z8_32'
; CHECK: ldarg {{.*}}
; CHECK-NEXT: conv.u1
; CHECK: .method {{.*}} 'z8_64'
; CHECK: ldarg {{.*}}
; CHECK-NEXT: conv.u1
; CHECK-NEXT: conv.u8
; CHECK: .method {{.*}} 'z16_32'
; CHECK: ldarg {{.*}}
; CHECK-NEXT: conv.u2
