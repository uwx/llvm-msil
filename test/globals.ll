; RUN: llvm-msil %s -o - | FileCheck %s

; Each initialized global gets a read-only template blob plus a writable
; pointer, because a `.data`-backed field is read-only at run time.

@i = global i32 7
@arr = global [3 x i16] [i16 1, i16 2, i16 300]

; CHECK: .field static private valuetype {{.*}} 'i$tpl'
; CHECK: .data 'i$tpl$data' = { int32 (7) }
; CHECK: .field static {{.*}} void* 'i'

; CHECK: .data 'arr$tpl$data' = { int16 (1), int16 (2), int16 (300) }

define i32 @main() {
; CHECK-LABEL: 'main'
; CHECK: ldsfld void* 'i'
; CHECK: ldind.i4
  %v = load i32, ptr @i
  ret i32 %v
}

; Storage is allocated and filled from the template at startup.
; CHECK-LABEL: '$MSIL_Init'
; CHECK: call void* '$MSIL_Alloc'(int64)
; CHECK: cpblk
