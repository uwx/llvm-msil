#include <stdatomic.h>
#include <stdio.h>

#define EXERCISE(T, FMT, start, operand)                                       \
  do {                                                                         \
    _Atomic T v = (start);                                                     \
    T a = atomic_fetch_add(&v, (operand));                                     \
    T b = atomic_fetch_sub(&v, (operand) + 1);                                 \
    T c = atomic_fetch_or(&v, (operand));                                      \
    T d = atomic_fetch_and(&v, (T)~(T)1);                                      \
    T e = atomic_fetch_xor(&v, (operand));                                     \
    T f = atomic_exchange(&v, (operand));                                      \
    T expected = (operand);                                                    \
    _Bool g = atomic_compare_exchange_strong(&v, &expected, (start));          \
    T h = atomic_load(&v);                                                     \
    printf(#T " " FMT " " FMT " " FMT " " FMT " " FMT " " FMT " %d " FMT "\n", \
           a, b, c, d, e, f, g, h);                                            \
  } while (0)

__attribute__((noinline)) static void run(int seed) {
  EXERCISE(signed char, "%d", (signed char)(seed + 5), (signed char)3);
  EXERCISE(short, "%d", (short)(seed + 1000), (short)300);
  EXERCISE(int, "%d", seed + 100000, 7);
  EXERCISE(long long, "%lld", seed + 10000000000ll, 70000000000ll);
  EXERCISE(unsigned, "%u", (unsigned)seed + 4000000000u, 5u);
}

int main(void) {
  volatile int seeds[] = {0, 1, -9, 123};
  for (int i = 0; i < 4; i++)
    run(seeds[i]);
  return 0;
}
