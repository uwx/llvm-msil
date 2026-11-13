// 16-byte atomic accesses: the width Interlocked has no overload for.
//
// The value is a .NET Int128, a value type rather than a primitive, so it
// moves with ldobj/stobj and its arithmetic is the type's own operators. The
// read-modify-write forms take the module's Monitor lock, because neither
// runtime has an Int128 CompareExchange that the JIT accepts.
//
// The output is compared against a native build, so it also covers the native
// side: clang lowers a 16-byte __atomic_* to a libcall, which needs libatomic.

#include <stdio.h>

typedef __int128 i128;

static i128 slot;

// A CompareExchange loop, which is what a lock-free implementation of these
// looks like, expressed through the __atomic builtins the way a C program
// would write it.
__attribute__((noinline)) static i128 fetch_add(i128 v) {
  i128 expected = slot, desired;
  do {
    desired = expected + v;
  } while (!__atomic_compare_exchange(&slot, &expected, &desired, 0,
                                      __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
  return expected;
}

__attribute__((noinline)) static i128 swap(i128 v) {
  return __atomic_exchange_n(&slot, v, __ATOMIC_SEQ_CST);
}

__attribute__((noinline)) static void put(i128 v) {
  __atomic_store_n(&slot, v, __ATOMIC_SEQ_CST);
}

__attribute__((noinline)) static i128 get(void) {
  return __atomic_load_n(&slot, __ATOMIC_SEQ_CST);
}

// Print the 128-bit value as its two halves, so the comparison does not depend
// on whether the printf implementation can format a 128-bit integer.
static void show(const char *what, i128 v) {
  printf("%s %llu %llu\n", what, (unsigned long long)(v >> 64),
         (unsigned long long)v);
}

int main(void) {
  put(((i128)1 << 100) + 7);
  show("start", get());

  i128 old = fetch_add(5);
  show("add-old", old);
  show("add-new", get());

  old = swap(((i128)1 << 100) + 512);
  show("xchg-old", old);
  show("xchg-new", get());

  old = fetch_add(-12);
  show("sub-old", old);
  show("sub-new", get());

  // The compare fails when the comparand is stale, and leaves the slot alone.
  i128 expected = 1, desired = 2;
  int ok = __atomic_compare_exchange(&slot, &expected, &desired, 0,
                                     __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
  printf("failed-cx %d\n", ok);
  show("after-failed-cx", get());

  put(0);
  show("zeroed", get());
  return 0;
}
