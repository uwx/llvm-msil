; RUN: llvm-msil %s -o - | FileCheck %s

; A `constant` global is never stored through, so its read-only `.data` image
; can be its storage: $MSIL_Init only points the address field at it, with no
; heap copy. `ldsflda` still yields a distinct address per global, so pointer
; comparisons stay correct, and a store through one faults at run time rather
; than silently corrupting the image.
;
; ilasm packs `.data` blobs back to back, so an image only keeps its alignment
; by luck. $MSIL_PlaceImage hands back the image when it is aligned and an
; aligned heap copy when it is not.

@c = constant i32 7
@arr = constant [3 x i16] [i16 1, i16 2, i16 300]
@bytes = constant [3 x i8] c"ab\00", align 1
@w = global i32 9
@big = global [4 x i32] [i32 1, i32 2, i32 3, i32 4], align 64

; CHECK: .data 'c$tpl$data' = { int32 (7) }
; CHECK: .data 'arr$tpl$data' = { int16 (1), int16 (2), int16 (300) }

define i32 @main() {
; CHECK-LABEL: 'main'
; CHECK: ldsfld void* 'c'
; CHECK-NEXT: ldind.i4
  %v = load i32, ptr @c
  ret i32 %v
}

; CHECK-LABEL: '$MSIL_AllocAligned'(int64 size, int64 align)
; CHECK: ldc.i8 16
; CHECK-NEXT: bgt.un IL_slack
; CHECK-LABEL: '$MSIL_PlaceImage'(void* image, int64 size, int64 align)

; The constants are pointed at their images, with no allocation in between...
; CHECK-LABEL: '$MSIL_Init'
; CHECK: // c
; CHECK-NEXT: ldsflda valuetype {{.*}} 'c$tpl'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: ldc.i8 4
; CHECK-NEXT: ldc.i8 4
; CHECK-NEXT: call void* '$MSIL_PlaceImage'(void*, int64, int64)
; CHECK-NEXT: stsfld void* 'c'
; CHECK: // arr
; CHECK-NEXT: ldsflda valuetype {{.*}} 'arr$tpl'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: ldc.i8 6
; CHECK-NEXT: ldc.i8 2
; CHECK-NEXT: call void* '$MSIL_PlaceImage'(void*, int64, int64)
; CHECK-NEXT: stsfld void* 'arr'
; ...and a byte-aligned one needs no check at all.
; CHECK: // bytes
; CHECK-NEXT: ldsflda valuetype {{.*}} 'bytes$tpl'
; CHECK-NEXT: conv.u8
; CHECK-NEXT: stsfld void* 'bytes'
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
; CHECK: // big
; CHECK-NEXT: ldc.i8 16
; CHECK-NEXT: ldc.i8 64
; CHECK-NEXT: call void* '$MSIL_AllocAligned'(int64, int64)
; CHECK-NEXT: stsfld void* 'big'
