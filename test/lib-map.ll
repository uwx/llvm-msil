; RUN: llvm-msil %s -o - | FileCheck %s --check-prefix=DEFAULT
; RUN: llvm-msil %s -o - -msil-lib-map 'SDL_*=SDL2.dll' \
; RUN:   | FileCheck %s --check-prefix=PREFIX
; RUN: llvm-msil %s -o - -msil-lib-map 'SDL_Init=SDL2.dll' \
; RUN:   -msil-lib-map 'printf=mycrt.dll' | FileCheck %s --check-prefix=EXACT

; An imported symbol binds to the C runtime by default. -msil-lib-map rebinds
; it to another DLL, matched on the symbol the module references rather than on
; the export name a POSIX spelling might be rewritten to.

declare i32 @SDL_Init(i32)
declare i32 @printf(ptr, ...)
declare i32 @SDL_GetTicks()

; DEFAULT: pinvokeimpl("msvcrt.dll" as "SDL_Init" cdecl) int32 'SDL_Init'(int32)
; DEFAULT: pinvokeimpl("msvcrt.dll" as "printf" cdecl) int32 printf$

; A trailing * matches a prefix, and both spellings of an SDL symbol that
; reaches the module move together.
; PREFIX: pinvokeimpl("SDL2.dll" as "SDL_Init" cdecl) int32 'SDL_Init'(int32)
; PREFIX-DAG: pinvokeimpl("SDL2.dll" as "SDL_GetTicks" cdecl) int32 'SDL_GetTicks'()
; PREFIX: pinvokeimpl("msvcrt.dll" as "printf" cdecl) int32 printf$

; Without a * the pattern is the whole symbol, so printf can be moved on its
; own while SDL_GetTicks stays on the CRT.
; EXACT: pinvokeimpl("SDL2.dll" as "SDL_Init" cdecl) int32 'SDL_Init'(int32)
; EXACT: pinvokeimpl("msvcrt.dll" as "SDL_GetTicks" cdecl) int32 'SDL_GetTicks'()
; EXACT: pinvokeimpl("mycrt.dll" as "printf" cdecl) int32 printf$

define i32 @main() {
  %a = call i32 @SDL_Init(i32 0)
  %b = call i32 @SDL_GetTicks()
  %s = call i32 (ptr, ...) @printf(ptr null, i32 %a)
  %t = add i32 %b, %s
  ret i32 %t
}
