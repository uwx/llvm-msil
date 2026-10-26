#!/usr/bin/env bash
# Regenerates lib/Target/MSIL/libc.ll from lib/Target/MSIL/libc/*.c.
#
#   ./scripts/update-libc-ir.sh [clang] [llvm-link]
#
# The build compiles the IR itself when it can find a clang that targets
# Windows; this script is for the cases it cannot (an in-tree build without
# clang, or a machine with no such clang), and for checking in an updated copy.
# Run it after changing anything under libc/.
set -eu

HERE=$(cd "$(dirname "$0")/.." && pwd)
LIBC="$HERE/libc"
OUT="$HERE/libc.ll"

CLANG=${1:-${CLANG:-}}
LLVM_LINK=${2:-${LLVM_LINK:-}}

if [ -z "$CLANG" ]; then
  CLANG=$(command -v clang || true)
fi
if [ -z "$LLVM_LINK" ]; then
  LLVM_LINK=$(command -v llvm-link || true)
fi
if [ -z "$CLANG" ] || [ -z "$LLVM_LINK" ]; then
  echo "update-libc-ir: need clang and llvm-link (pass them as arguments)" >&2
  exit 1
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

for f in ctype string stdlib stdio; do
  "$CLANG" -target x86_64-pc-windows-msvc -ffreestanding -nostdinc \
           -fno-builtin -O2 -fno-vectorize -fno-slp-vectorize \
           -S -emit-llvm -w -fno-discard-value-names \
           -o "$TMP/$f.ll" "$LIBC/$f.c"
done
"$LLVM_LINK" -S "$TMP/ctype.ll" "$TMP/string.ll" "$TMP/stdlib.ll" \
             "$TMP/stdio.ll" -o "$OUT"

echo "wrote $OUT"
