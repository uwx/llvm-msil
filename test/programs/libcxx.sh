#!/usr/bin/env bash
# Runs the portable-C++-runtime program on the CLR and compares it with a
# native build of the same source.
#
#   ./libcxx.sh <llvm-msil>
#
# The native build is the reference: the point of the runtime is to behave like
# a real C++ implementation, so any disagreement is a bug in the runtime.
set -eu

MSIL=${1:?usage: libcxx.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANGXX=${CLANGXX:-I:/Downloads/llvm-project-18.1.7/bin/clang++.exe}
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cp "$HERE/libcxx.cpp" "$TMP/libcxx.cpp"

# The native build is the reference. -fno-exceptions/-fno-rtti because the
# runtime is written for exactly that configuration, and -fno-builtin so a call
# to printf cannot be turned into something else.
"$CLANGXX" -std=c++17 -fno-exceptions -fno-rtti -O1 -w -fno-builtin \
  -o "$TMP/native$EXE" "$TMP/libcxx.cpp"
"$TMP/native$EXE" | tr -d '\r' > "$TMP/want.txt"

fail=0
for O in 0 1 2; do
  "$CLANGXX" $MSIL_CFLAGS -std=c++17 -fno-exceptions -fno-rtti -fno-builtin \
    -S -emit-llvm -O$O -w -o "$TMP/libcxx$O.ll" "$TMP/libcxx.cpp"
  if ! "$MSIL" "$TMP/libcxx$O.ll" -o "$TMP/libcxx$O.il" \
       -msil-libc=builtin -msil-libcxx=builtin $MSIL_CRT_ARGS; then
    echo "libcxx: -O$O translation failed" >&2
    fail=1
    continue
  fi
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/libcxx$O$EXE" "$TMP/libcxx$O.il"
  write_runtimeconfig "$TMP/libcxx$O"
  if ! (cd "$TMP" && run_msil "libcxx$O$EXE") 2>"$TMP/err$O.txt" | tr -d '\r' \
       > "$TMP/got$O.txt"; then
    echo "libcxx: -O$O crashed" >&2
    head -20 "$TMP/err$O.txt" >&2
    fail=1
    continue
  fi
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "libcxx: -O$O differs from the native build" >&2
    fail=1
  fi
done

exit $fail
