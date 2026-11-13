; A vector of 128-bit lanes, which has no IL type name.
;
; It is a parameter and return rather than an extract/insert, because that is
; the shape that used to reach the type namer and produce an empty name: the
; per-instruction checks could not see it.
target triple = "x86_64-unknown-linux-gnu"

define <2 x i128> @f(<2 x i128> %v) {
  ret <2 x i128> %v
}
