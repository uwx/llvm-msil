#!/usr/bin/env bash
# Development aid: runs libc/tests/host_test.c three ways and diffs the output.
#
#   ./compare.sh [clang]
#
#   1. against the host C library        (the reference)
#   2. against the ported C sources       (a portability bug)
#   3. through llvm-msil on the CLR       (a translation bug)
#
# Differences between 1 and 2 mean the C port is wrong; differences between 2
# and 3 mean the backend translated it wrongly. The end-to-end test in
# test/programs/libc.sh is the gate; this is for locating a failure it reports.
#
# Not part of the build: it needs a clang, a timeout-free machine, and it
# links the port over the CRT's own definitions, which is why the sources are
# compiled with tests/rename.h.
set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
LIBC=$(cd "$HERE/.." && pwd)
CLANG=${1:-${CLANG:-clang}}
MSIL=${MSIL:-$LIBC/../../../../build/tools/llvm-msil/llvm-msil.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# 1. the reference
"$CLANG" -O1 -w -DHOST_CRT -o "$TMP/ref.exe" "$HERE/host_test.c" "$HERE/hostshim.c"
"$TMP/ref.exe" | tr -d '\r' > "$TMP/ref.txt"

# 2. the ported sources, natively. rename.h prefixes every public name so the
#    import library's definitions do not collide with the port's.
cat > "$TMP/entry.c" <<'EOF'
int test_main(void);
int main(void) { return test_main(); }
EOF
for f in ctype string stdlib stdio; do
  "$CLANG" -O1 -w -include "$HERE/rename.h" -c -o "$TMP/$f.obj" "$LIBC/$f.c"
done
"$CLANG" -O1 -w -include "$HERE/rename.h" -Dmain=test_main \
         -c -o "$TMP/test.obj" "$HERE/host_test.c"
"$CLANG" -O1 -w -c -o "$TMP/entry.obj" "$TMP/entry.c"
"$CLANG" -O1 -w -c -o "$TMP/shim.obj" "$HERE/hostshim.c"
"$CLANG" -O1 -w -o "$TMP/port.exe" "$TMP/test.obj" "$TMP/entry.obj" \
         "$TMP/shim.obj" "$TMP/ctype.obj" "$TMP/string.obj" \
         "$TMP/stdlib.obj" "$TMP/stdio.obj"
"$TMP/port.exe" | tr -d '\r' > "$TMP/port.txt"

# 3. through the backend
"$CLANG" -S -emit-llvm -O1 -w -o "$TMP/test.ll" "$HERE/host_test.c"
"$MSIL" "$TMP/test.ll" -o "$TMP/test.il" -msil-libc=builtin
"$ILASM" -exe -quiet -output:"$TMP/test.exe" "$TMP/test.il"
printf '%s\n' '{"runtimeOptions":{"tfm":"net9.0","framework":{"name":"Microsoft.NETCore.App","version":"9.0.0"}}}' \
  > "$TMP/test.runtimeconfig.json"
(cd "$TMP" && dotnet test.exe) | tr -d '\r' > "$TMP/msil.txt"

rc=0
if ! diff -u "$TMP/ref.txt" "$TMP/port.txt"; then
  echo "compare: the C port disagrees with the host C library" >&2
  rc=1
fi
if ! diff -u "$TMP/ref.txt" "$TMP/msil.txt"; then
  echo "compare: the MSIL build disagrees with the host C library" >&2
  rc=1
fi
[ $rc -eq 0 ] && echo "compare: all three agree"
exit $rc
