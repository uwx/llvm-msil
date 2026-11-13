#!/usr/bin/env bash
# 16-byte atomic load, store, exchange and compare-exchange, compared against a
# native build. This is the width Interlocked has no overload for.
#
#   ./atomics128.sh <llvm-msil>
#
# -mcx16 is what makes clang emit a real `cmpxchg i128` and `atomicrmw xchg`
# instead of a __atomic_* libcall, so the IR reaches the writer's own 16-byte
# path rather than an unresolved declaration. It is also what a native build
# needs for the same reason, so both sides use it and the comparison is
# like-for-like.
set -eu

MSIL=${1:?usage: atomics128.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

"$CLANG" -mcx16 -O1 -w -fno-pic -o "$TMP/native$EXE" "$HERE/atomics128.c"
"$TMP/native$EXE" | tr -d "\r" > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  "$CLANG" -mcx16 $MSIL_CFLAGS -S -emit-llvm -O$O -w \
    -o "$TMP/a$O.ll" "$HERE/atomics128.c"
  "$MSIL" "$TMP/a$O.ll" -o "$TMP/a$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/a$O$EXE" "$TMP/a$O.il"
  write_runtimeconfig "$TMP/a$O"
  (cd "$TMP" && run_msil "a$O$EXE") 2>&1 | tr -d "\r" > "$TMP/got$O.txt" || true
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "atomics128: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
