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
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# printf and snprintf are C code in the portable library, so they are managed
# vararg methods. CoreCLR on Linux rejects the vararg calling convention
# outright (InvalidProgramException), so the library cannot run there at all.
# Every integer-only printf still works on Windows.
if [ "$MSIL_HOST" != windows ]; then
  echo "libc: skipped (the portable printf is a managed vararg method)"
  exit 0
fi

cp "$HERE/libc.c" "$TMP/libc.c"

# Native build: the reference result. -fno-builtin so the compiler cannot turn
# a test of printf into something else.
# The portable library is compiled for the Windows ABI, where long is 32 bits,
# but the reference build uses the host's long. The two cases that compare
# against LONG_MAX ask "did it saturate", not "to what", so both sides agree.
"$CLANG" -O1 -w -fno-builtin -DMSIL_LIBC_NATIVE -o "$TMP/native$EXE" "$TMP/libc.c"
"$TMP/native$EXE" | tr -d '\r' > "$TMP/want.txt"

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
  "$CLANG" $MSIL_CFLAGS -S -emit-llvm -O$O -w -fno-builtin \
    -o "$TMP/libc$O.ll" "$TMP/libc.c"
  "$MSIL" "$TMP/libc$O.ll" -o "$TMP/libc$O.il" -msil-libc=builtin $MSIL_CRT_ARGS
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/libc$O$EXE" "$TMP/libc$O.il"
  write_runtimeconfig "$TMP/libc$O"
  if ! (cd "$TMP" && run_msil "libc$O$EXE") 2>"$TMP/err$O.txt" | tr -d '\r' \
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
