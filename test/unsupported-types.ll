; RUN: not llvm-msil %s -o - 2>&1 | FileCheck %s
; RUN: not llvm-msil %S/Inputs/i128-vector.ll -o - 2>&1 \
; RUN:   | FileCheck %s --check-prefix=VECTOR
; RUN: not llvm-msil %S/Inputs/i129.ll -o - 2>&1 \
; RUN:   | FileCheck %s --check-prefix=WIDE

; Constructs the emitter rejects, each with one message that names the type.
;
; `not` is enough -- no --crash -- because a rejection is an ordinary
; diagnostic that exits 1. report_fatal_error's abort path, which raises a
; modal dialog on Windows and prints a compiler backtrace rather than one of
; the program, is replaced by the driver's own handler
; (tools/llvm-msil/llvm-msil.cpp).
;
; A module stops at the first rejection, so each shape is its own file under
; Inputs/: three RUN lines over one module would only ever test the first.

target triple = "x86_64-unknown-linux-gnu"

; CHECK: error: an integer wider than 128 bits has no IL type: i129

; Without the check the value would land in the int64 slot and be silently
; truncated, which is worse than refusing it.
define i129 @f(i129 %v) {
  ret i129 %v
}

; VECTOR: error: a vector of 128-bit integers has no IL type: <2 x i128>

; WIDE: error: an integer wider than 128 bits has no IL type: i129
