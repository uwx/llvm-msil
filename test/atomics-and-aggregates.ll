; RUN: llvm-msil %s -o - | FileCheck %s

; The CLR has no atomic instructions. Atomics lower to an ordinary
; read/modify/write, which is correct for the single-threaded assemblies this
; backend produces. The destination address must stay on the stack exactly
; once: an extra copy is left over on the evaluation stack and the CLR rejects
; the method at JIT time.

@ctr = internal global i32 10

define i32 @atomic_rmw() {
; CHECK-LABEL: .method static {{.*}} 'atomic_rmw'
; CHECK: ldsfld
; CHECK-NEXT: ldind.i4
; CHECK-NEXT: stloc
  %old = atomicrmw add ptr @ctr, i32 5 seq_cst, align 4
; CHECK: stind.i4
; CHECK-NEXT: ldloc
  ret i32 %old
}

; A fence has no observable effect without concurrency.
define void @barrier() {
; CHECK-LABEL: .method static {{.*}} 'barrier'
  fence seq_cst
  ret void
}

; cmpxchg yields { old, success }, built into a two-field struct.
define i1 @cmp_xchg(i32 %want, i32 %new) {
; CHECK-LABEL: .method static {{.*}} 'cmp_xchg'
  %r = cmpxchg ptr @ctr, i32 %want, i32 %new seq_cst seq_cst, align 4
; CHECK: extractvalue
  %ok = extractvalue { i32, i1 } %r, 1
  ret i1 %ok
}

; A constant aggregate is materialized field by field in a scratch local.
define { i8, i1 } @const_pair() {
; CHECK-LABEL: .method static {{.*}} 'const_pair'
; CHECK: initobj
; CHECK: stind.i1
  ret { i8, i1 } { i8 42, i1 false }
}

; A zero-sized global still needs a unique address, so it gets an
; allocation even though there is nothing to copy into it.
@empty = internal global [0 x i64] zeroinitializer

define ptr @empty_addr() {
; CHECK-LABEL: .method static {{.*}} 'empty_addr'
  ret ptr @empty
}
