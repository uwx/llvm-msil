#include <stdio.h>

typedef _BitInt(65) i65;
typedef unsigned _BitInt(65) u65;
typedef _BitInt(100) i100;
typedef unsigned _BitInt(100) u100;

static void show(const char *tag, unsigned __int128 v) {
  printf("%s %016llx%016llx\n", tag, (unsigned long long)(v >> 64),
         (unsigned long long)v);
}

__attribute__((noinline)) static void narrow(unsigned long long a,
                                             long long b, unsigned shift) {
  i65 sa = (i65)a, sb = (i65)b;
  u65 ua = (u65)a, ub = (u65)b;
  i65 max = (i65)(((u65)1 << 64) - 1);
  show("wrap", (unsigned __int128)(i65)(max + (i65)1));
  show("add", (unsigned __int128)(i65)(sa + sb));
  show("sub", (unsigned __int128)(i65)(sb - sa));
  show("mul", (unsigned __int128)(i65)(sa * sb));
  show("shl", (unsigned __int128)(u65)(ua << shift));
  show("lshr", (unsigned __int128)(u65)(ub >> shift));
  show("ashr", (unsigned __int128)(i65)(sb >> shift));
  show("udiv", (unsigned __int128)(u65)(ub / (ua | 1)));
  show("urem", (unsigned __int128)(u65)(ub % (ua | 1)));
  show("sdiv", (unsigned __int128)(i65)(sb / (sa | 1)));
  show("srem", (unsigned __int128)(i65)(sb % (sa | 1)));
  printf("cmp %d %d %d %d\n", sa < sb, ua < ub, sa == (i65)a, ub > ua);

  long long sum;
  int over = __builtin_add_overflow(a, b, &sum);
  printf("mixed %d %lld\n", over, sum);
  unsigned long long usum;
  over = __builtin_add_overflow(b, a, &usum);
  printf("mixedu %d %llu\n", over, usum);
}

__attribute__((noinline)) static void wide(unsigned long long hi,
                                           unsigned long long lo) {
  i100 v = (i100)(((u100)hi << 64) | lo);
  u100 u = (u100)v;
  show("w.add", (unsigned __int128)(i100)(v + v));
  show("w.mul", (unsigned __int128)(i100)(v * (i100)3));
  show("w.lshr", (unsigned __int128)(u100)(u >> 7));
  show("w.ashr", (unsigned __int128)(i100)(v >> 7));
  i100 r;
  int over = __builtin_add_overflow(v, v, &r);
  printf("w.over %d\n", over);
  show("w.r", (unsigned __int128)r);
  u100 ur;
  over = __builtin_sub_overflow(u, u + 1, &ur);
  printf("w.uover %d\n", over);
  show("w.ur", (unsigned __int128)ur);
}

int main(void) {
  volatile unsigned long long a[] = {0, 1, 0x7fffffffffffffffull,
                                     0xffffffffffffffffull,
                                     0x8000000000000000ull, 12345};
  volatile long long b[] = {-1, 5, -0x7fffffffffffffffll - 1, 77, -3};
  for (int i = 0; i < 6; i++)
    for (int j = 0; j < 5; j++) {
      printf("== %d %d\n", i, j);
      narrow(a[i], b[j], (unsigned)(i * 7 + j) % 65);
    }
  volatile unsigned long long hi[] = {0, 7, 0xf, 0x8};
  for (int i = 0; i < 4; i++) {
    printf("== wide %d\n", i);
    wide(hi[i], a[i]);
  }
  return 0;
}
