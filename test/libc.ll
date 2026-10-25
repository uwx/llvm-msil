; RUN: llvm-msil %s -o - | FileCheck %s --check-prefix=DEFAULT
; RUN: llvm-msil %s -o - -msil-libc=builtin | FileCheck %s --check-prefix=BUILTIN
; RUN: llvm-msil %s -o - -msil-libc=builtin -msil-alloc=hooks:My.Hooks \
; RUN:   | FileCheck %s --check-prefix=HOOK
; RUN: llvm-msil %s -o - -msil-libc=user_libc | FileCheck %s --check-prefix=USERDEF
; RUN: llvm-msil %s -o - -msil-libc=user_libc:Their.Libc \
; RUN:   | FileCheck %s --check-prefix=USER

; External C functions pinvoke the Windows CRT by default. -msil-libc rebinds
; them to managed code instead: 'builtin' emits a platform-independent
; implementation into the module, anything else names an assembly (and
; optionally a type) the caller supplies.
;
; Emission order is: the forwarders that keep each C symbol's name, then the
; externals that could not be rebound, then the module body.

declare ptr @malloc(i64)
declare void @free(ptr)
declare i64 @strlen(ptr)
declare i32 @printf(ptr, ...)

; DEFAULT-NOT: .assembly extern System.Private.CoreLib
; DEFAULT: pinvokeimpl("{{[^"]*}}" as "malloc" cdecl)
; DEFAULT: pinvokeimpl("{{[^"]*}}" as "strlen" cdecl)
; DEFAULT-NOT: $MSIL_Libc

; The forwarders keep the C symbol's name and call into the emitted class.
; BUILTIN: .method public static hidebysig void* 'malloc'(int64)
; BUILTIN: call void* $MSIL_Libc::malloc(int64)
; BUILTIN: .method public static hidebysig void 'free'(void*)
; BUILTIN: call void $MSIL_Libc::free(void*)
; BUILTIN: .method public static hidebysig int64 'strlen'(void*)
; BUILTIN: call int64 $MSIL_Libc::strlen(void*)
; The built-in does not implement printf, so that one still pinvokes.
; BUILTIN: pinvokeimpl("{{[^"]*}}" as "printf" cdecl)
; The implementation itself.
; BUILTIN: .assembly extern System.Private.CoreLib
; BUILTIN: .class public auto ansi abstract sealed beforefieldinit '$MSIL_Libc'
; BUILTIN: .method public static hidebysig void* 'malloc'(int64)
; BUILTIN: NativeMemory::Alloc(native uint)
; BUILTIN: .method public static hidebysig void 'free'(void*)
; BUILTIN: NativeMemory::Free(void*)
; BUILTIN: .method public static hidebysig int64 'strlen'(void*)
; Only what the module uses is emitted.
; BUILTIN-NOT: 'strcat'

; The allocator hook reroutes the four allocating calls and nothing else.
; HOOK: call void* [hooks]My.Hooks::malloc(int64)
; HOOK: call void [hooks]My.Hooks::free(void*)
; HOOK: .method public static hidebysig int64 'strlen'(void*)
; HOOK-NOT: NativeMemory::Alloc

; A user-supplied libc is called by name, and the type defaults to MSIL.Libc.
; USERDEF: .assembly extern 'user_libc'
; USERDEF: [user_libc]MSIL.Libc::malloc(int64)
; USER: .assembly extern 'user_libc'
; USER: [user_libc]Their.Libc::malloc(int64)
; USER: [user_libc]Their.Libc::free(void*)
; USER: [user_libc]Their.Libc::strlen(void*)

define i32 @main() {
  %p = call ptr @malloc(i64 8)
  %n = call i64 @strlen(ptr %p)
  %f = call i32 (ptr, ...) @printf(ptr %p, i32 1)
  call void @free(ptr %p)
  %r = trunc i64 %n to i32
  %s = add i32 %r, %f
  ret i32 %s
}
