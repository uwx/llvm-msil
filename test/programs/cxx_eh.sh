#!/usr/bin/env bash
# C++ exceptions through Itanium landing pads and the MSIL.Runtime glue,
# compared against a native build at every optimization level.
#
#   ./cxx_eh.sh <llvm-msil>
set -eu

MSIL=${1:?usage: cxx_eh.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
CLANGXX=${CLANGXX:-$(dirname "$CLANG")/$(basename "$CLANG" | sed 's/clang/clang++/')}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
RUNTIME_PROJECT="$HERE/../../runtime/MSIL.Runtime/MSIL.Runtime.csproj"

if ! dotnet --list-sdks 2>/dev/null | grep -q .; then
  echo "cxx_eh: skipped (no .NET SDK to build MSIL.Runtime)"
  exit 0
fi

# mingw on Windows keeps the CRT imports on msvcrt; both are Itanium EH.
if [ "$MSIL_HOST" = windows ]; then
  TRIPLE=x86_64-w64-windows-gnu
else
  TRIPLE=x86_64-unknown-linux-gnu
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

if ! dotnet build "$RUNTIME_PROJECT" -c Release -nologo -v q -o "$TMP" \
     > "$TMP/runtime-build.log" 2>&1; then
  cat "$TMP/runtime-build.log"
  exit 1
fi

"$CLANGXX" -O1 -w -o "$TMP/native$EXE" "$HERE/cxx_eh.cpp"
"$TMP/native$EXE" | tr -d "\r" > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  # Bitcode, not text: clang 18 prints the vtable GEPs of a class with
  # multiple inheritance as `inrange i32 0, i32 3`, which LLVM 22's parser
  # reads only in the newer `inrange(a, b)` spelling, so the .ll does not
  # parse. Bitcode is forward compatible, so the same module reaches the
  # backend either way.
  "$CLANGXX" -target $TRIPLE $MSIL_CFLAGS -c -emit-llvm -O$O -w \
    -o "$TMP/eh$O.bc" "$HERE/cxx_eh.cpp"
  "$MSIL" "$TMP/eh$O.bc" -o "$TMP/eh$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/eh$O$EXE" "$TMP/eh$O.il"
  write_runtimeconfig "$TMP/eh$O"
  (cd "$TMP" && run_msil "eh$O$EXE") | tr -d "\r" > "$TMP/got$O.txt"
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "cxx_eh: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
