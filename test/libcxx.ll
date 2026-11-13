; RUN: llvm-msil %s -o - -msil-libcxx=builtin -msil-libc=builtin \
; RUN:   | FileCheck %s
; RUN: llvm-msil %s -o - -msil-libcxx=builtin -msil-libc=builtin \
; RUN:   | FileCheck %s --check-prefix=START

; -msil-libcxx=builtin links the portable C++ runtime into the module, keeping
; only the symbols the program reaches. The triple decides which copy is used,
; and with it which C++ ABI spells the allocation operators.

target triple = "x86_64-unknown-linux-gnu"

@guard = internal global i64 0

declare i32 @printf(ptr, ...)
declare ptr @_Znwm(i64)
declare void @_ZdlPv(ptr)
declare ptr @malloc(i64)
declare void @free(ptr)
declare i32 @__cxa_guard_acquire(ptr)
declare void @__cxa_guard_release(ptr)

; The module's own functions are emitted first; the calls name the runtime
; symbols, which the link supplied.
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
  ret i32 0
}

declare ptr @_Znam(i64)
declare void @_ZdaPv(ptr)
