#!/usr/bin/env bash
# Executes a compiled 128-bit integer program and checks its output.
#
#   ./int128-runtime.sh <llvm-msil>
#
# The runtime needs `dotnet prog.exe` plus a runtimeconfig.json, not a bare exe:
# see NOTES-platform.md.
set -eu

MSIL=${1:?usage: int128-runtime.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/t.c" <<'SRC'
#include <stdio.h>
typedef __int128 i128;
typedef unsigned __int128 u128;

i128 g = ((i128)1 << 100) + 12345;
i128 arr[3] = {1, -2, ((i128)1 << 127) - 1};

__attribute__((noinline)) static i128 add3(i128 a, i128 b, i128 c) {
  return a + b + c;
}

struct Pair { i128 x; i128 y; };

int main(void) {
  i128 a = g, b = -7;
  printf("a %d %d\n", (int)(a >> 100), (int)(long)a);
  printf("add %d\n", (int)(add3(a, b, 5) >> 100));
  printf("sub %d\n", (int)((b - a) < 0));
  printf("mul %d\n", (int)((i128)123456789 * (i128)987654321 / 1000000000));
  printf("div %d %d\n", (int)((i128)-100 / 7), (int)((i128)-100 % 7));
  u128 u = (u128)-1;
  printf("udiv %llx\n", (unsigned long long)((u / 3) >> 64));
  printf("urem %llu\n", (unsigned long long)(u % 10));
  printf("shifts %d %d %d\n", (int)(((i128)1 << 77) >> 77),
         (int)((i128)-256 >> 4), (int)((u128)u >> 120));
  printf("bits %d %d %d %d\n", (int)(a & 255), (int)((i128)8 | 1),
         (int)((i128)12 ^ 10), (int)(~b));
  printf("cmp %d %d %d %d\n", a > b, a < b, a >= b, a != b);
  printf("cmpu %d %d %d %d\n", (u128)a > (u128)b, (u128)a < (u128)b,
         (u128)a <= (u128)b, u == (u128)-1);
  printf("fp %d %d\n", (int)((double)((i128)1 << 100) / 1.2676506002282294e30),
         (int)((i128)1500.75));
  short s = -5;
  printf("ext %d %d\n", (int)((u128)(unsigned short)s >> 120),
         (int)((i128)s >> 120));
  printf("trunc %d %d\n", (int)(i128)65537, (int)(a >> 96));
  printf("arr %d %d\n", (int)arr[0], (int)(arr[2] >> 126));
  struct Pair p;
  p.x = a; p.y = b;
  p.y += p.x;
  printf("struct %d\n", (int)(p.y >> 100));
  i128 c = a;
  c <<= 5; c >>= 3; c *= 2; c += 1; c -= 1; c ^= 3; c |= 4; c &= -1;
  printf("assign %d\n", (int)(c >> 102));
  printf("cond %d\n", a > b ? 42 : 0);
  return 0;
}
SRC

"$CLANG" -S -emit-llvm -O1 -w -o "$TMP/t.ll" "$TMP/t.c"
"$MSIL" "$TMP/t.ll" -o "$TMP/t.il"
"$ILASM" -exe -quiet -output:"$TMP/t.exe" "$TMP/t.il"
printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
  > "$TMP/t.runtimeconfig.json"
# The program writes CRLF on Windows; compare line content only.
(cd "$TMP" && dotnet t.exe) | tr -d "\r" > "$TMP/got.txt"

cat > "$TMP/want.txt" <<'EXPECT'
a 1 12345
add 1
sub 1
mul 121932631
div -14 -2
udiv 5555555555555555
urem 5
shifts 1 -16 255
bits 57 9 6 6
cmp 1 0 1 1
cmpu 0 1 1 1
fp 1 1500
ext 0 -1
trunc 65537 16
arr 1 1
struct 1
assign 2
cond 42
EXPECT

if ! diff -u "$TMP/want.txt" "$TMP/got.txt"; then
  echo "int128-runtime: output mismatch" >&2
  exit 1
fi
