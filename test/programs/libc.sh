#!/usr/bin/env bash
# Runs the portable-libc program on the CLR and compares it with a native
# build of the same source.
#
#   ./libc.sh <llvm-msil>
#
# The native build is the reference: the point of the port is to behave like a
# real C library, so any disagreement is a bug in the port regardless of which
# side is "more correct".
set -eu

MSIL=${1:?usage: libc.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cp "$HERE/libc.c" "$TMP/libc.c"

# Native build: the reference result. -fno-builtin so the compiler cannot turn
# a test of printf into something else.
"$CLANG" -O1 -w -fno-builtin -DMSIL_LIBC_NATIVE -o "$TMP/native.exe" "$TMP/libc.c"
"$TMP/native.exe" | tr -d '\r' > "$TMP/want.txt"

# %a/%A is the one conversion the C libraries do not agree on: MSVC pads to 13
# fraction digits, musl (and so the port) prints the shortest exact form. That
# is why those two cases are compared against literals instead of the native
# build. musl 1.2.4 on the same values, which is what the port must produce.
cat > "$TMP/hex_want.txt" <<'EOF'
hex floats     [[0x1p+0][0x1p-1][0x0p+0][0X1P+0]]
hex subnormal  [[0x1p-1074][0x1.000p+0][0x1.p+0]]
EOF
# Everything else, from the native build, with the two %a lines taken out.
grep -v '^hex floats\|^hex subnormal' "$TMP/want.txt" > "$TMP/want_nochex.txt"

# Every optimization level: the bugs this catches have been level-specific
# before (a value read narrower than its type at -O2), so one level is not
# enough.
fail=0
for O in 0 1 2 3; do
  "$CLANG" -S -emit-llvm -O$O -w -fno-builtin -o "$TMP/libc$O.ll" "$TMP/libc.c"
  "$MSIL" "$TMP/libc$O.ll" -o "$TMP/libc$O.il" -msil-libc=builtin
  "$ILASM" -exe -quiet -output:"$TMP/libc$O.exe" "$TMP/libc$O.il"
  # dotnet looks for a runtimeconfig named after the exe it is given.
  printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
    > "$TMP/libc$O.runtimeconfig.json"
  if ! (cd "$TMP" && dotnet "libc$O.exe") 2>"$TMP/err$O.txt" | tr -d '\r' \
       > "$TMP/got$O.txt"; then
    echo "libc: -O$O crashed" >&2
    head -20 "$TMP/err$O.txt" >&2
    fail=1
    continue
  fi
  grep -v '^hex floats\|^hex subnormal' "$TMP/got$O.txt" \
    > "$TMP/got_nochex$O.txt"
  if ! diff -u "$TMP/want_nochex.txt" "$TMP/got_nochex$O.txt"; then
    echo "libc: -O$O differs from the native build" >&2
    fail=1
  fi
  grep '^hex floats\|^hex subnormal' "$TMP/got$O.txt" \
    > "$TMP/hex_got$O.txt"
  if ! diff -u "$TMP/hex_want.txt" "$TMP/hex_got$O.txt"; then
    echo "libc: -O$O disagrees with musl on %a" >&2
    fail=1
  fi
done

exit $fail
