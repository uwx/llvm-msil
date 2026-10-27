; RUN: not --crash llvm-msil %s -o - 2>&1 | FileCheck %s --check-prefix=DEFAULT
; RUN: llvm-msil %s -o - -msil-fp80=double | FileCheck %s --check-prefix=FP80
; RUN: not --crash llvm-msil %S/Inputs/fp80-pinvoke.ll -o - -msil-fp80=double \
; RUN:   2>&1 | FileCheck %s --check-prefix=PINVOKE
; RUN: not --crash llvm-msil %S/Inputs/fp80-vararg.ll -o - -msil-fp80=double \
; RUN:   2>&1 | FileCheck %s --check-prefix=VARARG

; x86_fp80 is `long double` on the x86 ELF target, and the CLR has no 80-bit
; float, so the writer rejects it by default. -msil-fp80=double maps the value
; to binary64, which is lossy (53 mantissa bits instead of 64, 11 exponent bits
; instead of 15) and therefore opt-in.
;
; The IR below is what clang 22 emits for an unoptimized `long double` program.

target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; The member keeps its 16-byte slot and the offset the data layout gave it, so
; struct layout, `sizeof` and GEP strides still say what the native ABI says.
; Only the type named in the slot changes.
%struct.P = type { i32, x86_fp80, i8 }

; DEFAULT: LLVM ERROR: MSIL: unsupported type x86_fp80

; FP80-LABEL: .class value explicit ansi sealed 'struct.P'
; FP80: .pack 1
; FP80: .size 48
; FP80: .field [0] public int32 'f0'
; FP80: .field [16] public float64 'f1'
; FP80: .field [32] public int8 'f2'

; A global's .data template holds the narrowed binary64 bits in the low half of
; the 16-byte slot and pads the rest, so the field and the blob agree on where
; the next member starts. 3.0 is 0x4008000000000000.
@g = global x86_fp80 0xK4000C000000000000000, align 16

; FP80: .data 'g$tpl$data' = { int64 (4613937818241073152), int8 (0) [8] }

define x86_fp80 @scale(x86_fp80 %0) {
; FP80-LABEL: .method static {{.*}} float64 'scale'(float64
; The 80-bit constant is rounded to binary64 when it is emitted, not handed to
; ilasm as an 80-bit literal it has no type for.
; FP80: ldc.r8 2.0E+0
; FP80: add
  %2 = alloca x86_fp80, align 16
  store x86_fp80 %0, ptr %2, align 16
  %3 = load x86_fp80, ptr %2, align 16
  %4 = fadd x86_fp80 %3, 0xK40008000000000000000
  ret x86_fp80 %4
}

; A struct member is reached with ldfld of the mapped type; a load of the
; mapped type uses the r8 suffix; and fptosi skips the float conversion it
; would need for a type the CLR knows, because both slots are already float64.
define i32 @use(ptr %0) {
; FP80-LABEL: .method static {{.*}} int32 'use'
; FP80: ldfld float64 'struct.P'::'f1'
; FP80: ldind.r8
; FP80-NOT: conv.r4
; FP80: conv.i4
  %2 = alloca ptr, align 8
  store ptr %0, ptr %2, align 8
  %3 = load ptr, ptr %2, align 8
  %4 = getelementptr inbounds nuw %struct.P, ptr %3, i32 0, i32 1
  %5 = load x86_fp80, ptr %4, align 16
  %6 = load x86_fp80, ptr @g, align 16
  %7 = fadd x86_fp80 %5, %6
  %8 = call x86_fp80 @scale(x86_fp80 %7)
  %9 = fptosi x86_fp80 %8 to i32
  ret i32 %9
}

; The member after the long double still lands at its declared offset. This is
; the check that would catch a narrowed slot shrinking the struct: 32 is where
; the native ABI puts `c` after a 16-byte `long double`, and `getelementptr`
; and `ldfld` both have to agree with it.
define i32 @after(ptr %0) {
; FP80-LABEL: .method static {{.*}} int32 'after'
; FP80: ldfld float64 'struct.P'::'f1'
; FP80: ldc.i4	32
; FP80: add
; FP80: ldfld int8 'struct.P'::'f2'
  %2 = getelementptr inbounds nuw %struct.P, ptr %0, i32 0, i32 1
  %3 = load x86_fp80, ptr %2, align 16
  %4 = getelementptr inbounds nuw %struct.P, ptr %0, i32 0, i32 2
  %5 = load i8, ptr %4, align 16
  %6 = zext i8 %5 to i32
  ret i32 %6
}

; A long-double libm call the front end leaves as a declaration is a pinvoke
; whose name would still say `frexpl` while the narrowed signature says
; `float64`. glibc's takes an x87 80-bit value, so that would return a wrong
; number rather than fail; the writer refuses instead. See Inputs/.
;
; PINVOKE: LLVM ERROR: MSIL: external 'frexpl' takes or returns x86_fp80
; VARARG: LLVM ERROR: MSIL: external 'printf' is called with an x86_fp80
