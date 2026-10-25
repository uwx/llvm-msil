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

; Every member becomes ldfld/stfld, against the GEP's base pointer: the field
; is named rather than reached by adding an offset to a raw pointer, and ldfld
; supplies the offset itself.
define i32 @access(ptr %p) {
; CHECK-LABEL: .method static {{.*}} 'access'
; CHECK: stfld int32 'struct.P'::'f0'
; CHECK: ldfld int32 'struct.P'::'f0'
; CHECK: stfld float64 'struct.P'::'f1'
; CHECK: ldfld int8 'struct.P'::'f2'
  %f0 = getelementptr inbounds %struct.P, ptr %p, i32 0, i32 0
  store i32 7, ptr %f0
  %v0 = load i32, ptr %f0
  %f1 = getelementptr inbounds %struct.P, ptr %p, i32 0, i32 1
  store double 1.5, ptr %f1
  %f2 = getelementptr inbounds %struct.P, ptr %p, i32 0, i32 2
  %v2 = load i8, ptr %f2
  %e = sext i8 %v2 to i32
  %s = add i32 %v0, %e
  ret i32 %s
}

; An access narrower than the field it lands in is a different value with a
; different stack type, so it must not use ldfld/stfld: ldfld of struct.Q would
; push a 64-bit struct where an i32 is expected, which the JIT rejects with
; InvalidProgramException. The pointer path is used instead.
%struct.Q = type { { i32, i32 }, i32 }

define i32 @partial(ptr %p) {
; CHECK-LABEL: .method static {{.*}} 'partial'
; CHECK-NOT: ldfld valuetype 'struct.Q'::'f0'
; CHECK: ldind.i4
; CHECK-NOT: stfld valuetype 'struct.Q'::'f0'
; CHECK: stind.i4
  %f0 = getelementptr inbounds %struct.Q, ptr %p, i32 0, i32 0
  store i32 5, ptr %f0
  %v = load i32, ptr %f0
  %f1 = getelementptr inbounds %struct.Q, ptr %p, i32 0, i32 1
  %w = load i32, ptr %f1
  %s = add i32 %v, %w
  ret i32 %s
}
