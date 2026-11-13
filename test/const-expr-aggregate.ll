; RUN: llvm-msil %s -o - | FileCheck %s

; A member function pointer is { i64 ptrtoint (ptr @fn to i64), i64 adj }, and
; a constant of that type built in a function body has a cast expression as a
; leaf, not just plain integers.

%mfp = type { i64, i64 }

define internal i32 @target(ptr %this, i32 %x) {
  ret i32 %x
}

; CHECK-LABEL: 'make'()
; CHECK: initobj
; CHECK: ldftn int32 'target'(void*, int32)
; CHECK-NEXT: conv.u8
; CHECK-NEXT: stind.i8
define %mfp @make() {
  ret %mfp { i64 ptrtoint (ptr @target to i64), i64 0 }
}
