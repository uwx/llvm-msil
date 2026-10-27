; RUN: llvm-msil %s -o - | FileCheck %s

; Narrow integers live in 32-bit IL locals, which normalize on store but not
; on load. Every signed use therefore has to restore the sign first, or a
; negative i8/i16 reads as a large positive number.

@sm = internal global i16 -25
@sc = internal global i8 -7

define i32 @signed_uses() {
; CHECK-LABEL: .method static {{.*}} 'signed_uses'
  %v = load i16, ptr @sm
; sitofp must sign-extend the zero-extended local before converting.
; CHECK: conv.i2
; CHECK-NEXT: conv.r4
  %f = sitofp i16 %v to float
; A conversion back out restores the narrow width on the way.
; CHECK: conv.i4
  %fi = fptosi float %f to i32
; sdiv, srem and ashr likewise restore the sign of their operand first; the
; result is then normalized back to the declared width on store.
; CHECK: conv.i2
; CHECK: div
; CHECK-NEXT: conv.u2
  %q = sdiv i16 %v, 5
; CHECK: conv.i2
; CHECK: rem
; CHECK-NEXT: conv.u2
  %r = srem i16 %v, 5
; CHECK: conv.i2
; CHECK: shr
; CHECK-NEXT: conv.u2
  %s = ashr i16 %v, 2
  %q2 = sext i16 %q to i32
  %r2 = sext i16 %r to i32
  %s2 = sext i16 %s to i32
  %a = add i32 %q2, %r2
  %b = add i32 %a, %s2
  %c = add i32 %b, %fi
  ret i32 %c
}

; An i1 is held as 0 or 1, not as its sign pattern, so conv.i1 would leave a
; true value as 1 instead of -1. `sext i1 %x` is IR for negation, and that is
; what has to be emitted.
define i32 @sext_i1(i1 %c) {
; CHECK-LABEL: .method static {{.*}} 'sext_i1'
; CHECK: neg
  %v = sext i1 %c to i32
  ret i32 %v
}

; An i1 is one bit wide, and no conv opcode produces that. `conv.u1` keeps the
; low eight, so `trunc i64 %j to i1` on row 2 leaves 2, which tests true; the
; bit has to be masked out. stb_image's PNG decoder hit this in
; `cur = filter_buf + (j & 1)*width`, where every even row then read the other
; row's filter buffer and only the first two rows decoded correctly.
define i1 @trunc_to_i1(i64 %x) {
; CHECK-LABEL: .method static {{.*}} 'trunc_to_i1'
; CHECK: conv.u8
; CHECK-NEXT: ldc.i4.1
; CHECK-NEXT: and
; CHECK-NOT: conv.u1
  %v = trunc i64 %x to i1
  ret i1 %v
}

; A bitcast between integer and float is a reinterpretation. Storing into a
; local of the *destination* type would convert instead, so the bits go
; through memory of the source type.
define i32 @bitcast_pun(float %x) {
; CHECK-LABEL: .method static {{.*}} 'bitcast_pun'
; CHECK: stloc
; CHECK-NEXT: ldloca
; CHECK-NEXT: conv.u8
; CHECK-NEXT: ldind.i4
  %b = bitcast float %x to i32
  ret i32 %b
}
