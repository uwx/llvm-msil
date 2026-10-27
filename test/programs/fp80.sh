#!/usr/bin/env bash
# Runs a `long double` program through -msil-fp80=double and compares it with a
# native build of the same source.
#
#   ./fp80.sh <llvm-msil>
#
# `long double` is x86_fp80 on the x86 ELF target only; MSVC's is a plain
# double, so on Windows the flag changes nothing and there is nothing to test.
# The lit test in test/fp80.ll covers the emitted IL on every host.
set -eu

MSIL=${1:?usage: fp80.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

if [ "$MSIL_HOST" = windows ]; then
  echo "fp80: skipped (long double is x86_fp80 only on the ELF target)"
  exit 0
fi

cat > "$TMP/t.c" <<'SRC'
/* -msil-fp80=double maps x86_fp80 to binary64, which has 53 mantissa bits and
   an 11-bit exponent against the 80-bit type's 64 and 15. Every constant and
   every intermediate below is exactly representable in binary64 and stays well
   inside its range, so the native 80-bit build and the narrowed build must
   agree. A value needing the extra precision would round differently, which is
   the loss the flag accepts and the reason it is opt-in. */
#include <math.h>

int putchar(int);

long double g = 1.5L;
long double arr[3] = { 1.0L, 2.0L, 4.0L };

struct S { int tag; long double v; char c; };

/* Out of line, so the argument and return cross the fp80 signature. */
__attribute__((noinline)) static long double add(long double a, long double b) {
  return a + b;
}

/* putchar rather than printf: the portable printf is a managed vararg method,
   which CoreCLR rejects on Linux, and this script only runs there. */
static void putint(int n) {
  if (n < 0) { putchar('-'); n = -n; }
  if (n >= 10) putint(n / 10);
  putchar('0' + n % 10);
}

static void line(const char *label, int n) {
  while (*label) putchar(*label++);
  putchar(' ');
  putint(n);
  putchar('\n');
}

int main(void) {
  long double x = 2.25L;
  struct S s;
  s.tag = 7;
  s.v = 3.5L;
  s.c = 'z';

  long double y = add(x, s.v);
  line("add", (int)y);
  line("neg", (int)add(-y, 1.0L));

  /* The member sits at the offset the data layout chose, 16 bytes into the
     struct, so reading it back proves the narrowed slot kept its size. */
  s.v = s.v * 2.0L;
  line("member", (int)s.v);
  line("tag", s.tag + s.c);

  line("array", (int)(arr[0] + arr[1] + arr[2]));
  line("global", (int)(g + 0.5L));

  /* A narrowing conversion and a comparison. */
  line("narrow", (int)(double)(x * 3.0L));
  line("cmp", y > 5.0L);

  /* A loop, so the value lives across blocks rather than in one expression. */
  long double big = 1.0L;
  for (int i = 0; i < 10; i++) big *= 2.0L;
  line("loop", (int)big);

  /* The library functions the front end turns into f80 intrinsics. These
     reach the backend as llvm.fabs.f80 and friends, which bind to
     System.Math under the flag. */
  line("abs", (int)__builtin_fabsl(-9.5L));
  line("floor", (int)__builtin_floorl(2.75L));
  line("sqrt", (int)__builtin_sqrtl(16.0L));
  line("pow", (int)__builtin_powl(3.0L, 2.0L));

  /* frexpl is not an intrinsic: it stays a declaration, and under the flag it
     would be a pinvoke whose narrowed float64 signature does not match
     glibc's x87 one. The backend rejects that at emit time, so this program
     must never reach it -- the check is in test/fp80.ll. Calling frexp on the
     already-narrowed value is the supported route. */
  int e = 0;
  double m = frexp((double)x, &e);
  line("frexp", (int)(m * 100) + e);
  return 0;
}
SRC

# Native build: the reference. The 80-bit result is what the flag narrows.
# sqrtl lives in libm, which clang does not link by default.
"$CLANG" -O1 -w -o "$TMP/native" "$TMP/t.c" -lm
"$TMP/native" | tr -d '\r' > "$TMP/want.txt"

# The bugs this catches are usually level-specific, so every level is tried;
# an f80->binary64 conversion happens at all of them.
fail=0
for O in 0 1 2 3; do
  "$CLANG" $MSIL_CFLAGS -S -emit-llvm -O$O -w -o "$TMP/t$O.ll" "$TMP/t.c"
  "$MSIL" "$TMP/t$O.ll" -o "$TMP/t$O.il" -msil-fp80=double \
    -msil-libc=builtin $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/t$O$EXE" "$TMP/t$O.il"
  write_runtimeconfig "$TMP/t$O"
  if ! (cd "$TMP" && run_msil "t$O$EXE") 2>"$TMP/err$O.txt" | tr -d '\r' \
       > "$TMP/got$O.txt"; then
    echo "fp80: -O$O crashed" >&2
    head -20 "$TMP/err$O.txt" >&2
    fail=1
    continue
  fi
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "fp80: -O$O differs from the native build" >&2
    fail=1
  fi
done

exit $fail
