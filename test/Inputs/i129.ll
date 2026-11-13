; An integer wider than 128 bits. The widest .NET integer is Int128, and the
; i65..i127 promotion is built on it, so there is no representation at all.
target triple = "x86_64-unknown-linux-gnu"

define i129 @f(i129 %v) {
  ret i129 %v
}
