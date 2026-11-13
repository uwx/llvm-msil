#!/usr/bin/env bash
# Exceptions together with the portable C++ runtime, compared against a native
# build at every optimization level.
#
#   ./cxx_eh_runtime.sh <llvm-msil>
#
# cxx_eh.sh covers unwinding on its own and libcxx.sh covers the runtime on its
# own; this is the pair, which is the configuration a real C++ program is in.
# The runtime is what supplies the local-static guards and the exit-time
# destructor table, so the program is translated with -msil-libcxx.
set -eu

MSIL=${1:?usage: cxx_eh_runtime.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
CLANGXX=${CLANGXX:-$(dirname "$CLANG")/$(basename "$CLANG" | sed 's/clang/clang++/')}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
RUNTIME_PROJECT="$HERE/../../runtime/MSIL.Runtime/MSIL.Runtime.csproj"

if ! dotnet --list-sdks 2>/dev/null | grep -q .; then
  echo "cxx_eh_runtime: skipped (no .NET SDK to build MSIL.Runtime)"
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

"$CLANGXX" -O1 -w -o "$TMP/native$EXE" "$HERE/cxx_eh_runtime.cpp"
"$TMP/native$EXE" | tr -d "\r" > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  # Bitcode, not text: see the same note in cxx_eh.sh -- clang 18 prints the
  # multi-inheritance vtable GEPs in an inrange spelling LLVM 22 does not read.
  "$CLANGXX" -target $TRIPLE $MSIL_CFLAGS -c -emit-llvm -O$O -w \
    -o "$TMP/eh$O.bc" "$HERE/cxx_eh_runtime.cpp"
  if ! "$MSIL" "$TMP/eh$O.bc" -o "$TMP/eh$O.il" \
       -msil-libc=builtin -msil-libcxx=builtin $MSIL_CRT_ARGS; then
    echo "cxx_eh_runtime: -O$O translation failed" >&2
    fail=1
    continue
  fi
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/eh$O$EXE" "$TMP/eh$O.il"
  write_runtimeconfig "$TMP/eh$O"
  if ! (cd "$TMP" && run_msil "eh$O$EXE") 2>"$TMP/err$O.txt" | tr -d "\r" \
       > "$TMP/got$O.txt"; then
    echo "cxx_eh_runtime: -O$O crashed" >&2
    head -20 "$TMP/err$O.txt" >&2
    fail=1
    continue
  fi
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "cxx_eh_runtime: -O$O differs from native" >&2
    fail=1
  fi
done
exit $fail
