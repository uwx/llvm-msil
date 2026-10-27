; A long-double libm declaration the front end did not turn into an intrinsic.
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

declare x86_fp80 @frexpl(x86_fp80, ptr)

define x86_fp80 @f(x86_fp80 %x, ptr %e) {
  %r = call x86_fp80 @frexpl(x86_fp80 %x, ptr %e)
  ret x86_fp80 %r
}
