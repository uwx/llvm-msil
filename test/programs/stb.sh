#!/usr/bin/env bash
# Compiles the stb libraries with llvm-msil and compares against a native build.
#
#   ./stb.sh <llvm-msil>
#
# stb_image (decode + encode round trip), stb_truetype (rasterisation and
# metrics), stb_rect_pack and stb_ds, in one translation unit. The two tests
# only compare stdout and exit status, so they need no fixed expected values.
set -eu

MSIL=${1:?usage: stb.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
# stb_truetype needs a font; any TrueType works as both builds read the same
# file.
FONT=${STB_TEST_FONT:-/c/Windows/Fonts/arial.ttf}
if [ ! -f "$FONT" ]; then
  echo "stb: skipped (no font at $FONT; set STB_TEST_FONT)"
  exit 0
fi

STB_COMMIT=003501f6d7a8668ce8b6d875e34dd97ad0d5d5b5
STB_URL=https://raw.githubusercontent.com/nothings/stb/$STB_COMMIT
CACHE=${MSIL_TEST_CACHE:-/tmp/msil-thirdparty/stb-$STB_COMMIT}

if [ ! -f "$CACHE/data/map_01.png" ] || [ ! -f "$CACHE/stb_image.h" ]; then
  mkdir -p "$CACHE/data"
  fetch() {
    for h in stb_image.h stb_image_write.h stb_truetype.h stb_rect_pack.h stb_ds.h; do
      curl -fsSL "$STB_URL/$h" -o "$CACHE/$h" || return 1
    done
    curl -fsSL "$STB_URL/data/map_01.png" -o "$CACHE/data/map_01.png"
  }
  fetch || {
    echo "stb: skipped (cannot fetch into $CACHE)"
    exit 0
  }
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
cp "$HERE/stb.c" "$TMP/"

# Native build: the reference result.
"$CLANG" -O1 -w -I"$CACHE" -o "$TMP/stb_native.exe" "$TMP/stb.c"
"$TMP/stb_native.exe" "$CACHE" "$FONT" | tr -d '\r' > "$TMP/want.txt"

# All four levels: the bugs this found were level-specific (a struct field read
# narrower than its declared type at -O2, the unoptimized snprintf wrapper at
# -O0), so checking one level would have missed them.
fail=0
for O in 0 1 2 3; do
  "$CLANG" -S -emit-llvm -O$O -w -I"$CACHE" -o "$TMP/stb$O.ll" "$TMP/stb.c"
  "$MSIL" "$TMP/stb$O.ll" -o "$TMP/stb$O.il"
  "$ILASM" -exe -quiet -output:"$TMP/stb$O.exe" "$TMP/stb$O.il"
  # dotnet looks for a runtimeconfig named after the exe it is given.
  printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
    > "$TMP/stb$O.runtimeconfig.json"
  # A crash at one level must not stop the other levels from being reported.
  (cd "$TMP" && dotnet stb$O.exe "$CACHE" "$FONT") 2>&1 | tr -d '\r' > "$TMP/got$O.txt" || true
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "stb: -O$O output mismatch" >&2
    fail=1
  fi
done
exit $fail
