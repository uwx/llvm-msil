#!/usr/bin/env bash
# C11 atomic fetch-and-op, exchange and compare-exchange on 8 to 64 bit
# integers, compared against a native build at every optimization level.
#
#   ./atomics.sh <llvm-msil>
set -eu

MSIL=${1:?usage: atomics.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

"$CLANG" -O1 -w -o "$TMP/native$EXE" "$HERE/atomics.c"
"$TMP/native$EXE" | tr -d "\r" > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  "$CLANG" $MSIL_CFLAGS -S -emit-llvm -O$O -w -o "$TMP/a$O.ll" \
    "$HERE/atomics.c"
  "$MSIL" "$TMP/a$O.ll" -o "$TMP/a$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/a$O$EXE" "$TMP/a$O.il"
  write_runtimeconfig "$TMP/a$O"
  (cd "$TMP" && run_msil "a$O$EXE") 2>&1 | tr -d "\r" > "$TMP/got$O.txt" || true
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "atomics: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
