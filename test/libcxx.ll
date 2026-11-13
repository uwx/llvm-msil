; RUN: llvm-msil %s -o - -msil-libcxx=builtin -msil-libc=builtin \
; RUN:   | FileCheck %s
; RUN: llvm-msil %s -o - -msil-libcxx=builtin -msil-libc=builtin \
; RUN:   | FileCheck %s --check-prefix=START

; -msil-libcxx=builtin links the portable C++ runtime into the module, keeping
; only the symbols the program reaches. The triple decides which copy is used,
; and with it which C++ ABI spells the allocation operators.

target triple = "x86_64-unknown-linux-gnu"

@guard = internal global i64 0
; A global destructor makes clang name __dso_handle: a link-time constant it
; passes to __cxa_atexit. It is not evidence that the exception runtime is in
; use, and counting it once dragged MSIL.Runtime.dll into every ELF C++
; program with a global destructor -- which assembles but then dies at run
; time with FileNotFoundException. The CHECK-NOT at the end pins that down.
@__dso_handle = external hidden global i8
@dtor = internal global i32 0

declare i32 @printf(ptr, ...)
declare ptr @_Znwm(i64)
declare void @_ZdlPv(ptr)
declare ptr @malloc(i64)
declare void @free(ptr)
declare i32 @__cxa_guard_acquire(ptr)
declare void @__cxa_guard_release(ptr)
declare i32 @__cxa_atexit(ptr, ptr, ptr)
declare void @destroy(i32)

; The module's own functions are emitted first; the calls name the runtime
; symbols, which the link supplied.
; A global destructor makes clang name __dso_handle, which is a link-time
; constant for __cxa_atexit, not evidence that the exception runtime is in
; use. Counting it once dragged MSIL.Runtime.dll into every ELF C++ program
; with a global destructor, which assembled but then died at run time with
; FileNotFoundException. This is the first check so it covers the header,
; where that assembly reference would appear.
; CHECK-NOT: MSIL.Runtime
; CHECK: call void* '_Znwm'(int64)
; CHECK: call void '_ZdlPv'(void*)
; CHECK: call int32 '__cxa_guard_acquire'(void*)

; Then the runtime's own definitions. On an ELF triple that is the Itanium
; spelling of the operators, with the array forms alongside the scalar ones,
; and the guard functions and teardown hook that come from the same runtime.
; CHECK: .method static public hidebysig void* '_Znwm'(int64 'l0') cil managed
; CHECK: .method static public hidebysig void* '_Znam'(int64 'l0') cil managed
; CHECK: .method static public hidebysig void '_ZdlPv'(void* 'l0') cil managed
; CHECK: .method static public hidebysig void '_ZdaPv'(void* 'l0') cil managed
; CHECK: .method static public hidebysig int32 '__cxa_guard_acquire'(void* 'l0') cil managed
; CHECK: .method static public hidebysig void '__cxa_guard_release'(void* 'l0') cil managed
; CHECK: .method static public hidebysig void '__msil_cxx_finalize'() cil managed

; $MSIL_Startup runs the exit-time teardown after main returns.
; START: '$MSIL_Startup'
; START: call int32 'main'
; START-NEXT: call void __msil_cxx_finalize()
; START-NEXT: conv.i4
; START-NEXT: ret

define i32 @main() {
  %p = call ptr @_Znwm(i64 4)
  call void @_ZdlPv(ptr %p)
  %q = call ptr @_Znam(i64 8)
  call void @_ZdaPv(ptr %q)
  %r = call ptr @malloc(i64 8)
  call void @free(ptr %r)
  call i32 @__cxa_guard_acquire(ptr @guard)
  call void @__cxa_guard_release(ptr @guard)
  %d = call i32 @__cxa_atexit(ptr @destroy, ptr @dtor, ptr @__dso_handle)
  ret i32 0
}

declare ptr @_Znam(i64)
declare void @_ZdaPv(ptr)
