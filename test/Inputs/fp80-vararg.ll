; A long double crossing a vararg pinvoke boundary, which is printf("%Lf").
target datalayout = "e-m:e-p:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private constant [4 x i8] c"%Lf\00"
declare i32 @printf(ptr, ...)

define i32 @main() {
  %r = call i32 (ptr, ...) @printf(ptr @.str, x86_fp80 0xK3FFF8000000000000000)
  ret i32 %r
}
