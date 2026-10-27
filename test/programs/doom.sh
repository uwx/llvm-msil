#!/usr/bin/env bash
# Compiles PureDOOM with llvm-msil and compares it against a native build.
#
#   ./doom.sh <llvm-msil>
#
# PureDOOM has no dependencies beyond the C runtime, so a run is fully
# deterministic once the clock is made virtual: both builds must print the
# same framebuffer hashes. The WAD is fetched at the pinned commit and cached.
#
# Every optimization level is checked because the bugs this found were
# level-specific: at -O2 a struct field was read with ldfld of the field's type
# when the IR read a narrower value, and at -O0 the unoptimized snprintf
# wrapper reached a CRT internal through a va_list.
set -eu

MSIL=${1:?usage: doom.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}

DOOM_COMMIT=355cfbd16fac119718879239336ee2ea408886bd
DOOM_URL=https://raw.githubusercontent.com/Daivuk/PureDOOM/$DOOM_COMMIT
CACHE=${MSIL_TEST_CACHE:-/tmp/msil-thirdparty/PureDOOM-$DOOM_COMMIT}

# The sources are not vendored, so a machine without the cache or without
# network skips rather than fails: the gate is about the backend.
if [ ! -f "$CACHE/doom1.wad" ] || [ ! -f "$CACHE/PureDOOM.h" ]; then
  mkdir -p "$CACHE"
  curl -fsSL "$DOOM_URL/PureDOOM.h" -o "$CACHE/PureDOOM.h" &&
    curl -fsSL "$DOOM_URL/doom1.wad" -o "$CACHE/doom1.wad" || {
      echo "doom: skipped (cannot fetch into $CACHE)"
      exit 0
    }
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$CACHE/PureDOOM.h" "$TMP/"
cp "$HERE/doom.c" "$TMP/"

# Native build: the reference result.
"$CLANG" -O1 -w -DWIN32 -I"$TMP" -o "$TMP/doom_native$EXE" "$TMP/doom.c"
"$TMP/doom_native$EXE" "$CACHE" | tr -d '\r' > "$TMP/want.txt"

fail=0
for O in 0 1 2 3; do
  "$CLANG" $MSIL_CFLAGS -S -emit-llvm -O$O -w -DWIN32 -I"$TMP" \
    -o "$TMP/doom$O.ll" "$TMP/doom.c"
  "$MSIL" "$TMP/doom$O.ll" -o "$TMP/doom$O.il" $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/doom$O$EXE" "$TMP/doom$O.il"
  # The backend calls the CRT, so the assembled exe needs a runtimeconfig to be
  # started with `dotnet`; dotnet looks for one named after the file it is given.
  write_runtimeconfig "$TMP/doom$O"
  # A crash at one level must not stop the other levels from being reported.
  (cd "$TMP" && run_msil "doom$O$EXE" "$CACHE") 2>&1 | tr -d '\r' \
    > "$TMP/got$O.txt" || true
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "doom: -O$O output mismatch" >&2
    fail=1
  fi
done
exit $fail
