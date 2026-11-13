#!/usr/bin/env bash
# Exercises -msil-pointers=i64: pointers kept in int64 slots in the IL, with a
# conv.u before every dereference. The program is pointer-heavy on purpose --
# allocations, structs with pointer members, pointer arithmetic, function
# pointers, void* round trips and a linked list -- and its output is compared
# against a native build of the same source.
#
#   ./ptr64.sh <llvm-msil>
set -eu

MSIL=${1:?usage: ptr64.sh <llvm-msil>}
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/../platform.sh"
CLANG=${CLANG:-I:/Downloads/llvm-project-18.1.7/bin/clang.exe}
ILASM=${ILASM:-/tmp/tools/ilasm-core.exe}
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

cat > "$TMP/p.c" <<'SRC'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct Node {
  int value;
  struct Node *next;
  const char *name;
};

static int triple(int x) { return x * 3; }
static int negate(int x) { return -x; }

typedef int (*op_fn)(int);

static int apply(op_fn f, int x) { return f(x); }

static int cmp_int(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return x < y ? -1 : x > y;
}

int main(void) {
  /* malloc/free and pointer round trips through void*. */
  int *p = malloc(10 * sizeof *p);
  for (int i = 0; i < 10; i++)
    p[i] = i * i;
  void *v = p;
  int *q = (int *)v;
  printf("heap %d %d %d\n", q[0], q[9], (int)(&q[3] - p));

  /* Pointer arithmetic over an array through a second alias. */
  int *walk = p + 2;
  printf("walk %d %d\n", *walk, *(walk + 4));

  /* A struct with pointer members, addressed through a heap pointer. */
  struct Node *n = calloc(3, sizeof *n);
  for (int i = 0; i < 3; i++) {
    n[i].value = (i + 1) * 10;
    n[i].next = i < 2 ? &n[i + 1] : 0;
    n[i].name = i == 0 ? "alpha" : i == 1 ? "beta" : "gamma";
  }
  for (struct Node *it = n; it; it = it->next)
    printf("node %d %s\n", it->value, it->name);
  printf("null %d\n", n[2].next == 0);

  /* Function pointers, including through a variable and a parameter. */
  op_fn f = triple;
  printf("fn %d %d %d\n", f(7), apply(f, 4), apply(negate, 9));
  op_fn table[2] = {triple, negate};
  printf("table %d %d\n", table[0](2), table[1](2));
  printf("fnptr null %d\n", (void *)0 == 0);

  /* Pointer difference and comparison. */
  printf("cmp %d %d\n", p < q, p == q);
  printf("pdiff %d\n", (int)(&p[8] - &p[1]));

  /* A callback through qsort that the CLR must call back into. */
  int a[6] = {4, 1, 6, 3, 5, 2};
  qsort(a, 6, sizeof a[0], cmp_int);
  printf("sorted");
  for (int i = 0; i < 6; i++)
    printf(" %d", a[i]);
  printf("\n");

  /* strdup returns a pointer, and free takes one. */
  char *s = strdup("portable");
  printf("strdup %s %d\n", s, (int)strlen(s));
  free(s);

  /* intptr_t/uintptr_t round trips. */
  int local = 1234;
  intptr_t addr = (intptr_t)&local;
  int *back = (int *)addr;
  printf("intptr %d %d\n", *back, addr != 0);

  free(n);
  free(p);
  return 0;
}
SRC

# Native reference.
"$CLANG" -O1 -w -fno-builtin -o "$TMP/native$EXE" "$TMP/p.c"
"$TMP/native$EXE" | tr -d '\r' > "$TMP/want.txt"

# The IR is built for an LP64 triple: -msil-pointers=i64 keeps the layout LP64
# and only changes the IL value representation, so the wrong triple would make
# the addresses and the struct layout disagree.
fail=0
for O in 0 2; do
  "$CLANG" $MSIL_CFLAGS -S -emit-llvm -O$O -w -fno-builtin \
    -o "$TMP/p$O.ll" "$TMP/p.c"
  if ! "$MSIL" "$TMP/p$O.ll" -o "$TMP/p$O.il" -msil-pointers=i64 \
       -msil-libc=builtin $MSIL_CRT_ARGS; then
    echo "ptr64: -O$O translation failed" >&2
    fail=1
    continue
  fi
  "$ILASM" $MSIL_ILASM_OPTS -output:"$TMP/p$O$EXE" "$TMP/p$O.il"
  write_runtimeconfig "$TMP/p$O"
  if ! (cd "$TMP" && run_msil "p$O$EXE") 2>"$TMP/err$O.txt" | tr -d '\r' \
       > "$TMP/got$O.txt"; then
    echo "ptr64: -O$O crashed" >&2
    head -20 "$TMP/err$O.txt" >&2
    fail=1
    continue
  fi
  if ! diff -u "$TMP/want.txt" "$TMP/got$O.txt"; then
    echo "ptr64: -O$O differs from the native build" >&2
    fail=1
  fi
done

exit $fail
