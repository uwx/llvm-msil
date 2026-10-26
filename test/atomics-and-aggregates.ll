; RUN: llvm-msil %s -o - | FileCheck %s

; Atomics lower to System.Threading.Interlocked, which takes a managed `T&`
; that an unmanaged void* satisfies directly. `volatile.` supplies the ordering
; for plain accesses, and a standalone fence is Interlocked.MemoryBarrier.

@ctr = internal global i32 10
@narrow = internal global i16 0
@fp = internal global float 0.0

; The forms Interlocked covers go straight to a call, and the result is the
; old value.
define i32 @atomic_rmw() {
; CHECK-LABEL: .method static {{.*}} 'atomic_rmw'
; CHECK: ldsfld void* 'ctr'
; CHECK-NEXT: ldc.i4 5
; CHECK-NEXT: call int32 [mscorlib]System.Threading.Interlocked::Add(int32&, int32)
; CHECK-NOT: ldind
  %old = atomicrmw add ptr @ctr, i32 5 seq_cst, align 4
  ret i32 %old
}

; sub is an add of the two's complement: Interlocked has no Subtract.
define i32 @atomic_sub() {
; CHECK-LABEL: .method static {{.*}} 'atomic_sub'
; CHECK: ldc.i4 3
; CHECK-NEXT: neg
; CHECK-NEXT: call int32 [mscorlib]System.Threading.Interlocked::Add(int32&, int32)
  %old = atomicrmw sub ptr @ctr, i32 3 seq_cst, align 4
  ret i32 %old
}

; Exchange for a plain exchange.
define i32 @atomic_xchg() {
; CHECK-LABEL: .method static {{.*}} 'atomic_xchg'
; CHECK: call int32 [mscorlib]System.Threading.Interlocked::Exchange(int32&, int32)
  %old = atomicrmw xchg ptr @ctr, i32 7 seq_cst, align 4
  ret i32 %old
}

; And/Or/Xor have no .NET Framework overload, so they are a CompareExchange
; loop. The loop re-reads with `volatile.` and retries until the exchange that
; was attempted is the one that happened.
define i32 @atomic_xor() {
; CHECK-LABEL: .method static {{.*}} 'atomic_xor'
; CHECK: IL_rmw
; CHECK: volatile.
; CHECK-NEXT: ldind.i4
; CHECK: xor
; CHECK: call int32 [mscorlib]System.Threading.Interlocked::CompareExchange(int32&, int32, int32)
; CHECK: bne.un IL_rmw
  %old = atomicrmw xor ptr @ctr, i32 2 seq_cst, align 4
  ret i32 %old
}

; A fence is a full barrier on its own.
define void @barrier() {
; CHECK-LABEL: .method static {{.*}} 'barrier'
; CHECK: call void [mscorlib]System.Threading.Interlocked::MemoryBarrier()
  fence seq_cst
  ret void
}

; cmpxchg is a single CompareExchange, and success is re-derived from the
; returned old value.
define i1 @cmp_xchg(i32 %want, i32 %new) {
; CHECK-LABEL: .method static {{.*}} 'cmp_xchg'
; CHECK: call int32 [mscorlib]System.Threading.Interlocked::CompareExchange(int32&, int32, int32)
; CHECK: ceq
  %r = cmpxchg ptr @ctr, i32 %want, i32 %new seq_cst seq_cst, align 4
  %ok = extractvalue { i32, i1 } %r, 1
  ret i1 %ok
}

; A narrow atomic has no overload, so it runs under the shared lock. The lock
; object is emitted once, in a module initializer, because a private field of
; <Module> is not reachable from a class-wrapped module.
define i16 @narrow_rmw() {
; CHECK-LABEL: .method static {{.*}} 'narrow_rmw'
; CHECK: ldsfld object '$MSIL::atomicLock'
; CHECK: call void [mscorlib]System.Threading.Monitor::Enter(object)
; CHECK: .try
; CHECK: leave
; CHECK: finally
; CHECK: call void [mscorlib]System.Threading.Monitor::Exit(object)
; CHECK: endfinally
  %old = atomicrmw add ptr @narrow, i16 1 seq_cst, align 2
  ret i16 %old
}

; A narrow cmpxchg runs under the same lock. Both sides of the comparison are
; zero-extended, so a negative comparand matches the value the load produced.
define i1 @narrow_cmpxchg(i16 %want) {
; CHECK-LABEL: .method static {{.*}} 'narrow_cmpxchg'
; CHECK: ldsfld object '$MSIL::atomicLock'
; CHECK: conv.u2
; CHECK: bne.un
  %r = cmpxchg ptr @narrow, i16 %want, i16 5 seq_cst seq_cst, align 2
  %ok = extractvalue { i16, i1 } %r, 1
  ret i1 %ok
}

; A floating exchange uses the float overload directly.
define float @fp_xchg() {
; CHECK-LABEL: .method static {{.*}} 'fp_xchg'
; CHECK: call float32 [mscorlib]System.Threading.Interlocked::Exchange(float32&, float32)
  %old = atomicrmw xchg ptr @fp, float 1.5 seq_cst, align 4
  ret float %old
}

; An atomic load or store is an access with the `volatile.` prefix, which is
; the CLR's acquire/release access. Plain accesses must not grow one.
define float @fp_roundtrip() {
; CHECK-LABEL: .method static {{.*}} 'fp_roundtrip'
; CHECK: ldsfld void* 'fp'
; CHECK-NEXT: volatile.
; CHECK-NEXT: ldind.r4
; CHECK: volatile.
; CHECK-NEXT: stind.r4
  %v = load atomic float, ptr @fp seq_cst, align 4
  store atomic float %v, ptr @fp seq_cst, align 4
  ret float %v
}

define float @fp_plain() {
; CHECK-LABEL: .method static {{.*}} 'fp_plain'
; CHECK: ldsfld void* 'fp'
; CHECK-NEXT: ldind.r4
  %v = load float, ptr @fp
  ret float %v
}

; An atomic aggregate access has no ldind form, so it is `volatile. ldobj` /
; `volatile. stobj` on the aggregate class. (Not ldind, which has no aggregate
; suffix and used to abort.)
@vec = internal global <4 x i32> zeroinitializer

define <4 x i32> @atomic_vec() {
; CHECK-LABEL: .method static {{.*}} 'atomic_vec'
; CHECK: volatile.
; CHECK-NEXT: ldobj valuetype
; CHECK: volatile.
; CHECK-NEXT: stobj valuetype
  %v = load atomic <4 x i32>, ptr @vec seq_cst, align 16
  store atomic <4 x i32> %v, ptr @vec seq_cst, align 16
  ret <4 x i32> %v
}

; A constant aggregate is materialized field by field in a scratch local.
define { i8, i1 } @const_pair() {
; CHECK-LABEL: .method static {{.*}} 'const_pair'
; CHECK: initobj
; CHECK: stind.i1
  ret { i8, i1 } { i8 42, i1 false }
}

; insertvalue stores the *inserted value*, so the store takes the element's
; type and not the aggregate's. Emitting stobj of the struct there put an i64
; behind a value-type store, which the JIT rejects at run time even though
; ilasm accepts it.
define { i64, i1 } @insert_scalar(i64 %x, i1 %c) {
; CHECK-LABEL: .method static {{.*}} 'insert_scalar'
; CHECK: stind.i8
; CHECK-NOT: stobj valuetype
  %a = insertvalue { i64, i1 } undef, i64 %x, 0
  %b = insertvalue { i64, i1 } %a, i1 %c, 1
  ret { i64, i1 } %b
}

; A zero-sized global still needs a unique address, so it gets an
; allocation even though there is nothing to copy into it.
@empty = internal global [0 x i64] zeroinitializer

define ptr @empty_addr() {
; CHECK-LABEL: .method static {{.*}} 'empty_addr'
  ret ptr @empty
}

; The lock object is emitted once for the whole module, after the functions.
; CHECK-LABEL: .method static void .cctor() cil managed
; CHECK: newobj instance void [mscorlib]System.Object::.ctor()
; CHECK: stsfld object '$MSIL::atomicLock'
