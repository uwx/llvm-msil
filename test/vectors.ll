; RUN: llvm-msil %s -o - | FileCheck %s

; IL has no SIMD: a vector is a byte blob and every operation expands into a
; per-lane loop. These are the shapes the vectorizer produces at -O2, which
; used to be emitted as scalar instructions on a 16-byte value type.

define i32 @vec_ops(<4 x i32> %a, <4 x i32> %b, <4 x i32> %c) {
; CHECK-LABEL: .method static {{.*}} 'vec_ops'
  %sum = add <4 x i32> %a, %b
; A vector select branches per lane, not once on the whole blob.
; CHECK: ldind.i1
; CHECK-NEXT: brfalse
; CHECK: ldind.i4
; CHECK-NEXT: stind.i4
  %cmp = icmp slt <4 x i32> %sum, %c
  %sel = select <4 x i1> %cmp, <4 x i32> %sum, <4 x i32> %a
; Folds become an extract/combine chain.
; CHECK: add
  %r = call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %sel)
  ret i32 %r
}

; A GEP may take a vector index, producing one pointer per lane.
@tbl = internal global [8 x i32] zeroinitializer

define i32 @vec_gep(<4 x i64> %idx) {
; CHECK-LABEL: .method static {{.*}} 'vec_gep'
  %p = getelementptr i32, ptr @tbl, <4 x i64> %idx
  %e = extractelement <4 x ptr> %p, i64 2
  %v = load i32, ptr %e
  ret i32 %v
}

declare i32 @llvm.vector.reduce.add.v4i32(<4 x i32>)
