#!/usr/bin/env bash
# Runs a C vararg program and checks its output.
#
#   ./vararg.sh <llvm-msil>
#
# A vararg *definition* is not a CLR `vararg` method: CoreCLR on Linux refuses
# one, and refuses any method that names one. The backend passes the trailing
# arguments in a buffer instead, so this covers the shapes that buffer has to
# carry -- integers, doubles, pointers, a mix -- and the C side reading them
# back with va_arg on both hosts.
set -eu

MSIL=${1:?usage: vararg.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/t.c" <<'SRC'
#include <stdarg.h>
#include <stdio.h>

/* Every integer kind, to check the widening and the sign restoration a narrow
 * argument needs. */
__attribute__((noinline)) static long long isum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  long long t = 0;
  for (int i = 0; i < n; i++)
    t += va_arg(ap, long long);
  va_end(ap);
  return t;
}

/* int and char go through the default promotions; the callee reads them back
 * as int. A negative char is the case that catches a missing sign extension. */
__attribute__((noinline)) static int narrow(int n, ...) {
  va_list ap;
  va_start(ap, n);
  int t = 0;
  for (int i = 0; i < n; i++)
    t += va_arg(ap, int);
  va_end(ap);
  return t;
}

/* A double's slot holds its raw bits, so this is where a conversion instead of
 * a reinterpretation would show. */
__attribute__((noinline)) static double dsum(int n, ...) {
  va_list ap;
  va_start(ap, n);
  double t = 0;
  for (int i = 0; i < n; i++)
    t += va_arg(ap, double);
  va_end(ap);
  return t;
}

/* Pointers cross as addresses. */
__attribute__((noinline)) static const char *pick(int n, ...) {
  va_list ap;
  va_start(ap, n);
  const char *s = 0;
  for (int i = 0; i < n; i++) {
    const char *c = va_arg(ap, const char *);
    if (i == 1)
      s = c;
  }
  va_end(ap);
  return s;
}

/* A vararg function that itself calls a vararg function: the buffer has to
 * survive a second call, and va_copy has to be independent. */
__attribute__((noinline)) static int forward(int n, ...) {
  va_list ap, cp;
  va_start(ap, n);
  va_copy(cp, ap);
  int a = narrow(n, va_arg(ap, int), va_arg(ap, int));
  int b = narrow(n, va_arg(cp, int), va_arg(cp, int));
  va_end(cp);
  va_end(ap);
  return a * 1000 + b;
}

/* Zero trailing arguments still has to be a valid call: the buffer local is
 * never a zero-length array. */
__attribute__((noinline)) static int none(int n, ...) {
  va_list ap;
  va_start(ap, n);
  va_end(ap);
  return n;
}

int main(void) {
  /* printf is the CRT's here (no -msil-libc), so its own vararg call path is
   * covered by the rest of the gate. */
  printf("ints %lld\n", isum(4, 1LL, -2LL, 3000000000LL, -4000000000LL));
  printf("narrow %d\n", narrow(3, (char)-5, (short)-300, 7));
  printf("doubles %f\n", dsum(3, 1.5, -2.25, 100.0));
  printf("ptr %s\n", pick(3, "a", "b", "c"));
  printf("mixed %d\n", forward(2, 10, 20));
  printf("none %d\n", none(7));
  return 0;
}
SRC

"$CLANG" $MSIL_CFLAGS -S -emit-llvm -O1 -w -fno-builtin -o "$TMP/t.ll" "$TMP/t.c"
"$MSIL" "$TMP/t.ll" -o "$TMP/t.il" $MSIL_CRT_ARGS
"$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/t$EXE" "$TMP/t.il"
write_runtimeconfig "$TMP/t"
# The program writes CRLF on Windows; compare line content only.
(cd "$TMP" && run_msil "t$EXE") | tr -d "\r" > "$TMP/got.txt"

cat > "$TMP/want.txt" <<'EXPECT'
ints -1000000001
narrow -298
doubles 99.250000
ptr b
mixed 30030
none 7
EXPECT

if ! diff -u "$TMP/want.txt" "$TMP/got.txt"; then
  echo "vararg: output mismatch" >&2
  exit 1
fi
