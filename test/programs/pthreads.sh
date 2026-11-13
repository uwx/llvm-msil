#!/usr/bin/env bash
# pthread_create and pthread_key_create callbacks running through the
# MSIL.Runtime trampoline, compared against a native build.
#
#   ./pthreads.sh <llvm-msil>
set -eu

MSIL=${1:?usage: pthreads.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
CLANGXX=${CLANGXX:-$(dirname "$CLANG")/$(basename "$CLANG" | sed 's/clang/clang++/')}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
RUNTIME_PROJECT="$HERE/../../runtime/MSIL.Runtime/MSIL.Runtime.csproj"

if [ "$MSIL_HOST" = windows ]; then
  echo "pthreads: skipped (needs glibc's pthreads)"
  exit 0
fi
if ! dotnet --list-sdks 2>/dev/null | grep -q .; then
  echo "pthreads: skipped (no .NET SDK to build MSIL.Runtime)"
  exit 0
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

if ! dotnet build "$RUNTIME_PROJECT" -c Release -nologo -v q -o "$TMP" \
     > "$TMP/runtime-build.log" 2>&1; then
  cat "$TMP/runtime-build.log"
  exit 1
fi

"$CLANGXX" -O1 -w -pthread -o "$TMP/native" "$HERE/pthreads.cpp"
"$TMP/native" > "$TMP/want.txt"

fail=0
for O in 0 2; do
  "$CLANGXX" $MSIL_CFLAGS -S -emit-llvm -O$O -w -o "$TMP/pt$O.ll" \
    "$HERE/pthreads.cpp"
  "$MSIL" "$TMP/pt$O.ll" -o "$TMP/pt$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/pt$O$EXE" "$TMP/pt$O.il"
  write_runtimeconfig "$TMP/pt$O"
  (cd "$TMP" && run_msil "pt$O$EXE") > "$TMP/got$O.txt"
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "pthreads: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
