; RUN: llvm-msil %s -o - | FileCheck %s

; A `constant` global is never stored through, so its read-only `.data` image
; can be its storage: $MSIL_Init only points the address field at it, with no
; heap copy. `ldsflda` still yields a distinct address per global, so pointer
; comparisons stay correct, and a store through one faults at run time rather
; than silently corrupting the image.

@c = constant i32 7
@arr = constant [3 x i16] [i16 1, i16 2, i16 300]
@w = global i32 9

; CHECK: .data 'c$tpl$data' = { int32 (7) }
; CHECK: .data 'arr$tpl$data' = { int16 (1), int16 (2), int16 (300) }

define i32 @main() {
; CHECK-LABEL: 'main'
; CHECK: ldsfld void* 'c'
; CHECK-NEXT: ldind.i4
  %v = load i32, ptr @c
  ret i32 %v
}

; The constants are pointed at their images, with no allocation in between...
; CHECK-LABEL: '$MSIL_Init'
; CHECK: // c
; CHECK-NEXT: ldsflda valuetype {{.*}} 'c$tpl'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: stsfld void* 'c'
; CHECK: // arr
; CHECK-NEXT: ldsflda valuetype {{.*}} 'arr$tpl'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: stsfld void* 'arr'
; CHECK-NOT: '$MSIL_Alloc'
; ...while the writable one still gets its own heap copy.
; CHECK: // w
; CHECK: call void* '$MSIL_Alloc'(int64)
; CHECK-NEXT: stsfld void* 'w'
; CHECK-NEXT: ldsfld void* 'w'
; CHECK-NEXT: ldsflda valuetype {{.*}} 'w$tpl'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: ldc.i4 4
; CHECK-NEXT: cpblk
