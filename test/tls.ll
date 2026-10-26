; RUN: llvm-msil %s -o - | FileCheck %s --check-prefix=DEFAULT
; RUN: llvm-msil %s -o - -msil-class-name=My.App | FileCheck %s --check-prefix=CLASS

; A thread-local global keeps the template blob and the `void*` indirection an
; ordinary global has, but the pointer lives in a [ThreadStatic] slot, so each
; thread gets its own storage. $MSIL_Init skips such a global; its accessor
; allocates and copies on the owning thread's first touch instead.

@t = thread_local global i32 7
@tp = thread_local global ptr @s
@s = global [4 x i8] c"abc\00"
@tc = thread_local constant i32 9
@te = thread_local global [0 x i8] zeroinitializer

; The pointer field carries the attribute; the template does not.
; DEFAULT: .data 't$tpl$data' = { int32 (7) }
; DEFAULT: .field static {{.*}} void* 't'
; DEFAULT-NEXT: .custom instance void [mscorlib]System.ThreadStaticAttribute::.ctor()
; A pointer-valued initializer is a relocation, not bytes: the template is zero
; and the accessor patches the address in, per thread.
; DEFAULT: .data 'tp$tpl$data' = { int64 (0) }
; DEFAULT: .field static {{.*}} void* 'tp'
; DEFAULT-NEXT: .custom instance void [mscorlib]System.ThreadStaticAttribute::.ctor()
; The ordinary global is still a plain field with no attribute.
; DEFAULT: .data 's$tpl$data' = { int8 (97), int8 (98), int8 (99), int8 (0)
; DEFAULT: .field static {{.*}} void* 's'
; DEFAULT-NOT: .custom instance void [mscorlib]System.ThreadStaticAttribute::.ctor()
; A constant thread-local cannot be stored through on any thread, but it still
; needs a per-thread slot: taking its address is what the attribute covers.
; DEFAULT: .data 'tc$tpl$data' = { int32 (9) }
; DEFAULT: .field static {{.*}} void* 'tc'
; DEFAULT-NEXT: .custom instance void [mscorlib]System.ThreadStaticAttribute::.ctor()

define i32 @main() {
; DEFAULT-LABEL: 'main'
; DEFAULT: call void* 't$tls'()
; DEFAULT: ldind.i4
; An access through the intrinsic lowers to the same reference.
; DEFAULT: call void* 't$tls'()
; DEFAULT: ldind.i4
; DEFAULT: call void* 'tp$tls'()
; DEFAULT: ldind.i
; DEFAULT: call void* 'tc$tls'()
; DEFAULT: ldind.i4
  %a = load i32, ptr @t
  %p = call ptr @llvm.threadlocal.address.p0(ptr @t)
  %b = load i32, ptr %p
  %q = call ptr @llvm.threadlocal.address.p0(ptr @tp)
  %r = load ptr, ptr %q
  %d = add i32 %a, %b
  %c = load i32, ptr @tc
  %s2 = add i32 %d, %c
  %s3 = ptrtoint ptr %r to i32
  %s4 = add i32 %s2, %s3
  ret i32 %s4
}

; $MSIL_Init must not touch a thread-local slot, which belongs to whichever
; thread happens to run startup.
; DEFAULT-LABEL: .method {{.*}}'$MSIL_Init'()
; DEFAULT: // s
; DEFAULT: call void* '$MSIL_Alloc'(int64)
; DEFAULT: stsfld void* 's'
; DEFAULT-NOT: ldsfld void* 't'
; DEFAULT-NOT: ldsfld void* 'tp'
; DEFAULT-NOT: ldsfld void* 'tc'
; DEFAULT: ret

; The accessor returns this thread's slot, allocating and copying the template
; only on the first call for that thread.
; DEFAULT-LABEL: 't$tls'() cil managed
; DEFAULT: ldsfld void* 't'
; DEFAULT: brfalse IL_tls_init
; DEFAULT: ldsfld void* 't'
; DEFAULT: ret
; DEFAULT: IL_tls_init:
; DEFAULT: ldc.i8 4
; DEFAULT: call void* '$MSIL_Alloc'(int64)
; DEFAULT: ldsflda valuetype {{.*}} 't$tpl'
; DEFAULT: conv.u8
; DEFAULT: ldc.i4 4
; DEFAULT: cpblk
; DEFAULT: stsfld void* 't'
; DEFAULT: ret

; A pointer initializer is patched into the thread's own copy, so the accessor
; names the target rather than leaving a zero behind.
; DEFAULT-LABEL: 'tp$tls'() cil managed
; DEFAULT: ldsflda valuetype {{.*}} 'tp$tpl'
; DEFAULT: cpblk
; DEFAULT: ldsfld void* 's'
; DEFAULT: stind.i

; A constant thread-local pins every thread's slot at the one image, so it
; allocates nothing.
; DEFAULT-LABEL: 'tc$tls'() cil managed
; DEFAULT: ldsflda valuetype {{.*}} 'tc$tpl'
; DEFAULT: conv.u8
; DEFAULT-NOT: call void* '$MSIL_Alloc'(int64)
; DEFAULT: stsfld void* 'tc'

; A zero-sized thread-local still gets its own allocation, but there are no
; bytes to copy: the accessor must not push a cpblk destination and then skip
; the cpblk, which leaves the stack one item deep and the JIT rejects it.
; DEFAULT-LABEL: 'te$tls'() cil managed
; DEFAULT: ldc.i8 1
; DEFAULT: call void* '$MSIL_Alloc'(int64)
; DEFAULT-NOT: cpblk
; DEFAULT: stsfld void* 'te'

; With -msil-class-name the accessor moves into the class with every other
; helper, so call sites have to name it. The slot itself stays a module-level
; field, like every other global's pointer, and is visible to the class.
; CLASS: .field static {{.*}} void* 't'
; CLASS-NEXT: .custom instance void [mscorlib]System.ThreadStaticAttribute::.ctor()
; CLASS: .class public auto ansi beforefieldinit 'My.App'
; CLASS: call void* 'My.App'::'t$tls'()
; CLASS: .method static assembly void* 't$tls'() cil managed
; CLASS: .method static assembly void* 'tp$tls'() cil managed
; CLASS: .method static assembly void* 'tc$tls'() cil managed

declare ptr @llvm.threadlocal.address.p0(ptr)
