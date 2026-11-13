extern "C" int printf(const char *, ...);

typedef float v2f __attribute__((vector_size(8)));
typedef int v2i __attribute__((vector_size(8)));
typedef unsigned v2u __attribute__((vector_size(8)));
typedef long long v2l __attribute__((vector_size(16)));
typedef unsigned long long v2ul __attribute__((vector_size(16)));
typedef int v8i __attribute__((vector_size(32)));
typedef unsigned v8u __attribute__((vector_size(32)));
typedef float v8f __attribute__((vector_size(32)));
typedef short v4s __attribute__((vector_size(8)));
typedef bool v8b __attribute__((ext_vector_type(8)));

__attribute__((noinline)) static void scalar(float a, float b) {
  printf("s %d%d%d%d%d%d %d%d%d%d%d%d\n", a < b, a > b, a <= b, a >= b, a == b,
         a != b, !(a < b), !(a > b), !(a <= b), !(a >= b), !(a == b),
         !(a != b));
}

__attribute__((noinline)) static void lanes(v2f a, v2f b) {
  v2i lt = a < b, gt = a > b, le = a <= b, ge = a >= b, eq = a == b,
      ne = a != b;
  for (int i = 0; i < 2; i++)
    printf("v%d %d %d %d %d %d %d\n", i, lt[i], gt[i], le[i], ge[i], eq[i],
           ne[i]);
}

__attribute__((noinline)) static void simd_int(v8i a, v8i b) {
  v8i one = {1, 1, 1, 1, 1, 1, 1, 1}, zero = {};
  v8i lt = a < b ? one : zero, le = a <= b ? one : zero;
  v8i gt = a > b ? one : zero, ge = a >= b ? one : zero;
  v8i eq = a == b ? one : zero, ne = a != b ? one : zero;
  v8u ua = (v8u)a, ub = (v8u)b;
  v8i ult = ua < ub ? one : zero, ule = ua <= ub ? one : zero;
  v8i ugt = ua > ub ? one : zero, uge = ua >= ub ? one : zero;
  for (int i = 0; i < 8; i++)
    printf("i%d %d%d%d%d%d%d %d%d%d%d\n", i, lt[i], le[i], gt[i], ge[i], eq[i],
           ne[i], ult[i], ule[i], ugt[i], uge[i]);
}

__attribute__((noinline)) static void simd_float(v8f a, v8f b) {
  v8f one = {1, 1, 1, 1, 1, 1, 1, 1}, zero = {};
  v8f lt = a < b ? one : zero, le = a <= b ? one : zero;
  v8f gt = a > b ? one : zero, ge = a >= b ? one : zero;
  v8f eq = a == b ? one : zero, ne = a != b ? one : zero;
  for (int i = 0; i < 8; i++)
    printf("f%d %d%d%d%d%d%d\n", i, (int)lt[i], (int)le[i], (int)gt[i],
           (int)ge[i], (int)eq[i], (int)ne[i]);
}

__attribute__((noinline)) static void widen(v2i s, v2u u) {
  v2l sl = __builtin_convertvector(s, v2l);
  v2ul ul = __builtin_convertvector(u, v2ul);
  for (int i = 0; i < 2; i++)
    printf("w%d %lld %llu\n", i, sl[i], ul[i]);
}

__attribute__((noinline)) static void bitcasts(v2i a, long long b, v8i c) {
  long long whole = (long long)a;
  v2i split = (v2i)b;
  v4s halves = (v4s)a;
  v8f floats = (v8f)c;
  printf("b %lld %d %d", whole, split[0], split[1]);
  for (int i = 0; i < 4; i++)
    printf(" %d", halves[i]);
  for (int i = 0; i < 8; i++)
    printf(" %g", floats[i]);
  printf("\n");
}

__attribute__((noinline)) static void masks(v8i a, v8i b) {
  v8b m = __builtin_convertvector(a < b, v8b);
  unsigned char bits = __builtin_bit_cast(unsigned char, m);
  v8b back = __builtin_bit_cast(v8b, (unsigned char)(bits ^ 0x5a));
  printf("m %u", bits);
  for (int i = 0; i < 8; i++)
    printf(" %d", (int)back[i]);
  printf("\n");
}

int main() {
  volatile float vals[] = {1.0f, 2.0f, __builtin_nanf("")};
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++) {
      float a = vals[i], b = vals[j];
      printf("%d,%d: ", i, j);
      scalar(a, b);
      v2f va = {a, b}, vb = {b, a};
      lanes(va, vb);
    }

  volatile int ints[] = {-3, -1, 0, 1, 5, 0x7fffffff, (int)0x80000000, 2};
  v8i a, b;
  for (int i = 0; i < 8; i++) {
    a[i] = ints[i];
    b[i] = ints[(i + 3) % 8];
  }
  simd_int(a, b);
  simd_int(a, a);

  v8f fa, fb;
  for (int i = 0; i < 8; i++) {
    fa[i] = i < 3 ? vals[i] : (float)ints[i];
    fb[i] = i < 3 ? vals[(i + 1) % 3] : (float)ints[i];
  }
  simd_float(fa, fb);

  v2i s = {ints[0], ints[5]};
  v2u u = {(unsigned)ints[0], (unsigned)ints[6]};
  widen(s, u);

  v8i bits;
  for (int i = 0; i < 8; i++)
    bits[i] = 0x3f800000 + i * 0x00100000;
  bitcasts(s, 0x123456789abcdef0ll + ints[0], bits);
  masks(a, b);
  return 0;
}
