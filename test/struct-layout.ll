; RUN: llvm-msil %s -o - | FileCheck %s

; A struct becomes an explicit-layout value class whose members are declared at
; the offsets the LLVM data layout picked, so the layout lives in one place
; instead of being re-derived by pointer arithmetic at every access.

%struct.P = type { i32, double, i8 }

; The field offsets match what the data layout chose (i32 at 0, the double
; aligned to 8, the i8 at 16, 24 bytes total with tail padding).
; CHECK-LABEL: .class value explicit ansi sealed 'struct.P'
; CHECK: .pack 1
; CHECK: .size 24
; CHECK: .field [0] public int32 'f0'
; CHECK: .field [8] public float64 'f1'
; CHECK: .field [16] public int8 'f2'

@g = global %struct.P zeroinitializer

; Only offset-0 members become ldfld/stfld: ldfld names the field rather than
; adding to the pointer, so using it for a later member would read the wrong
; bytes. Everything else keeps the pointer arithmetic.
define i32 @access(ptr %p) {
; CHECK-LABEL: .method static {{.*}} 'access'
; CHECK: stfld int32 'struct.P'::'f0'
; CHECK: ldfld int32 'struct.P'::'f0'
  %f0 = getelementptr inbounds %struct.P, ptr %p, i32 0, i32 0
  store i32 7, ptr %f0
  %v = load i32, ptr %f0
  ret i32 %v
}
