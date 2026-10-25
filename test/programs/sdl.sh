#!/usr/bin/env bash
# Builds the SDL2 demo twice -- native and through llvm-msil -- and compares
# what they print. Needs SDL2 headers and SDL2.dll; without them the test is
# skipped, since SDL is not something the project vendors.
#
#   SDL2_DIR=I:/path/to/sdl2 ./sdl.sh <llvm-msil>
#
# SDL2_DIR is a directory holding include/SDL2/... and lib/SDL2.lib; SDL2.dll
# is looked for in SDL2_BIN or next to lib/. The include and lib layout is the
# one the official SDL2 development package uses, so a plain unzip works.
set -eu

MSIL=${1:?usage: sdl.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}

# shortcut: one machine's SDK path; override SDL2_DIR anywhere else.
SDL2_DIR=${SDL2_DIR:-I:/New folder (2)/pcsx2/deps}
SDL2_BIN=${SDL2_BIN:-}
INC="$SDL2_DIR/include/SDL2"
LIB="$SDL2_DIR/lib/SDL2.lib"
DLL=${SDL2_BIN:-$SDL2_DIR/lib/SDL2.dll}

missing() { echo "sdl: skipped ($*)"; exit 0; }
[ -f "$INC/SDL.h" ] || missing "no SDL2 headers at $INC"
[ -f "$LIB" ] || missing "no SDL2.lib at $LIB"
[ -f "$DLL" ] || missing "no SDL2.dll (set SDL2_BIN)"
[ -x "$CLANG" ] || missing "no clang at $CLANG"
[ -x "$ILASM" ] || missing "no ilasm at $ILASM"

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$DLL" "$TMP/"

CFLAGS=(-O1 -w -DSDL_MAIN_HANDLED "-I$INC")
SRC="$HERE/sdl_demo.c"

# The renderer is created for the hidden window, so a display is only needed
# by the driver that backs it; the dummy driver keeps the test headless.
run() { (cd "$TMP" && SDL_VIDEODRIVER=dummy "$@" 2>&1 | tr -d '\r'); }

# Native reference.
"$CLANG" "${CFLAGS[@]}" -o "$TMP/native.exe" "$SRC" "$LIB" || {
  echo "sdl: native build failed"; exit 1; }

# Through the backend. -msil-lib-map is what makes SDL_* bind to SDL2.dll
# instead of the CRT heuristic's default.
"$CLANG" "${CFLAGS[@]}" -S -emit-llvm -o "$TMP/demo.ll" "$SRC"
"$MSIL" "$TMP/demo.ll" -o "$TMP/demo.il" -msil-lib-map 'SDL_*=SDL2.dll' || {
  echo "sdl: llvm-msil failed"; exit 1; }
"$ILASM" -exe -quiet -output:"$TMP/demo.exe" "$TMP/demo.il" || {
  echo "sdl: ilasm failed"; exit 1; }
printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
  > "$TMP/demo.runtimeconfig.json"

# Volatile lines (timing, driver names) are excluded on both sides; what is
# compared is the frame checksum, the event round trip and the dynload probe.
KEEP='^(frames|events|dynamics|version|ok|gl |FAIL)'
run ./native.exe --frames 40 | grep -E "$KEEP" > "$TMP/want.txt"
run dotnet demo.exe --frames 40 | grep -E "$KEEP" > "$TMP/got.txt"

if ! diff -u "$TMP/want.txt" "$TMP/got.txt"; then
  echo "sdl: output mismatch"
  exit 1
fi
