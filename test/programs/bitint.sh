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

# 128-bit division is __divti3 and friends, which libgcc has on Linux and the
# MSVC environment does not. A release clang+llvm archive ships no
# clang_rt.builtins either, so --rtlib=compiler-rt fails there with LNK1181 on
# a file that was never installed. Try it, and fall back to the default runtime
# if the link cannot find the archive: the point of this test is the *backend*,
# and a host toolchain missing a library is not a backend failure.
"$CLANG" -std=c23 -O1 -w --rtlib=compiler-rt -o "$TMP/native$EXE" "$HERE/bitint.c" \
  >"$TMP/native-rt.log" 2>&1 || {
  if grep -q "LNK1181\|cannot open input file" "$TMP/native-rt.log"; then
    "$CLANG" -std=c23 -O1 -w -o "$TMP/native$EXE" "$HERE/bitint.c" \
      >"$TMP/native.log" 2>&1 || {
      # No 128-bit division helper anywhere: skip rather than fail, and say so.
      if grep -q "unresolved external symbol __udivti3\|undefined reference to .__udivti3" \
           "$TMP/native.log"; then
        echo "bitint: skipped (this clang has no 128-bit division runtime)"
        exit 0
      fi
      cat "$TMP/native.log"
      exit 1
    }
  else
    cat "$TMP/native-rt.log"
    exit 1
  fi
}
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
