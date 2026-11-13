#!/usr/bin/env bash
# Scalar, per-lane and Vector<T> comparisons (NaN included) and vector
# integer widening, compared against a native build at every optimization
# level.
#
#   ./compare.sh <llvm-msil>
set -eu

MSIL=${1:?usage: compare.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
CLANGXX=${CLANGXX:-$(dirname "$CLANG")/$(basename "$CLANG" | sed 's/clang/clang++/')}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

"$CLANGXX" -O1 -w -o "$TMP/native$EXE" "$HERE/compare.cpp"
"$TMP/native$EXE" | tr -d "\r" > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  "$CLANGXX" $MSIL_CFLAGS -S -emit-llvm -O$O -w -o "$TMP/c$O.ll" \
    "$HERE/compare.cpp"
  "$MSIL" "$TMP/c$O.ll" -o "$TMP/c$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/c$O$EXE" "$TMP/c$O.il"
  write_runtimeconfig "$TMP/c$O"
  (cd "$TMP" && run_msil "c$O$EXE") 2>&1 | tr -d "\r" > "$TMP/got$O.txt" || true
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "compare: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
