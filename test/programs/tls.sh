#!/usr/bin/env bash
# Runs a thread-local program on two real threads and compares it with a
# native build of the same source.
#
#   ./tls.sh <llvm-msil>
#
# The point is that each thread sees its own copy: the workers start from the
# initializer while main holds a different value, so a shared global shows up
# immediately. Thread creation is a Win32 pinvoke, so a machine without
# CreateThread reports skipped rather than failing; the lit test in test/tls.ll
# still covers the emitted IL everywhere.
set -eu

MSIL=${1:?usage: tls.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

uname_s=$(uname -s 2>/dev/null || echo unknown)
case "$uname_s" in
  MINGW*|MSYS*|CYGWIN*) ;;
  *) echo "tls: skipped (thread creation is implemented for Windows only)";
     exit 0 ;;
esac

cp "$HERE/tls.c" "$TMP/tls.c"

# Native build: the reference output.
"$CLANG" -O1 -w -o "$TMP/native.exe" "$TMP/tls.c"
(cd "$TMP" && ./native.exe | tr -d '\r' > "$TMP/want.txt")

# CreateThread and friends live in kernel32, which the CRT name heuristic
# cannot know about, so they are mapped explicitly.
LIBMAP=(-msil-lib-map 'CreateThread=Kernel32.dll'
        -msil-lib-map 'WaitForSingleObject=Kernel32.dll'
        -msil-lib-map 'CloseHandle=Kernel32.dll')

fail=0
for O in 0 1 2 3; do
  "$CLANG" -S -emit-llvm -O$O -w -o "$TMP/tls$O.ll" "$TMP/tls.c"
  "$MSIL" "$TMP/tls$O.ll" -o "$TMP/tls$O.il" -msil-libc=builtin "${LIBMAP[@]}"
  "$ILASM" -exe -quiet -output:"$TMP/tls$O.exe" "$TMP/tls$O.il"
  printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
    > "$TMP/tls$O.runtimeconfig.json"
  if ! (cd "$TMP" && dotnet "tls$O.exe") 2>"$TMP/err$O.txt" | tr -d '\r' \
       > "$TMP/got$O.txt"; then
    echo "tls: -O$O crashed" >&2
    head -20 "$TMP/err$O.txt" >&2
    fail=1
    continue
  fi
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "tls: -O$O differs from the native build" >&2
    fail=1
  fi
done

exit $fail
