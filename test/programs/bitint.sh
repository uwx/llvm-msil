#!/usr/bin/env bash
# _BitInt(65) and _BitInt(100) arithmetic, which reaches the writer as i65 and
# i100, compared against a native build at every optimization level.
#
#   ./bitint.sh <llvm-msil>
set -eu

MSIL=${1:?usage: bitint.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# 128-bit division is __divti3 and friends; libgcc has them on Linux, the
# MSVC environment does not
RTLIB=
[ "$MSIL_HOST" = windows ] && RTLIB=--rtlib=compiler-rt
"$CLANG" -std=c23 -O1 -w $RTLIB -o "$TMP/native$EXE" "$HERE/bitint.c"
"$TMP/native$EXE" | tr -d "\r" > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  "$CLANG" $MSIL_CFLAGS -std=c23 -S -emit-llvm -O$O -w -o "$TMP/b$O.ll" \
    "$HERE/bitint.c"
  "$MSIL" "$TMP/b$O.ll" -o "$TMP/b$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/b$O$EXE" "$TMP/b$O.il"
  write_runtimeconfig "$TMP/b$O"
  (cd "$TMP" && run_msil "b$O$EXE") 2>&1 | tr -d "\r" > "$TMP/got$O.txt" || true
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "bitint: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
