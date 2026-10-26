/* printf family, ported from musl src/stdio/vfprintf.c.
 *
 * Differences from musl, all forced by the environment:
 *
 *  - There is no FILE. Output goes either to stdout (through __msil_write,
 *    which the backend emits as IL over System.IO.Stream) or to a caller
 *    buffer for the sprintf family.
 *  - There is no locale, so character classes are fixed ASCII, and the wide
 *    conversions (%ls/%lc) treat their argument as narrow. A UTF-8 string's
 *    bytes pass through unchanged either way.
 *  - musl decides float rounding with a long-double probe that needs more
 *    precision than the double has. The CLR has no extended precision, so
 *    the decision is made exactly on the base-1e9 chunks instead: same answer,
 *    no wider type.
 *  - Positional arguments (%n$), %m and errno are not implemented: the first
 *    needs a second scan of the format, the other two need a runtime error
 *    variable this libc has no meaning for.
 *
 * The digit-generation, padding and flag logic follows musl line for line, so
 * output matches a real C library for the conversions that are supported.
 */
#include "libc.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

/* Flags, indexed by the character's offset from space, as in musl. */
#define ALT_FORM (1u << ('#' - ' '))
#define ZERO_PAD (1u << ('0' - ' '))
#define LEFT_ADJ (1u << ('-' - ' '))
#define PAD_POS (1u << (' ' - ' '))
#define MARK_POS (1u << ('+' - ' '))
#define GROUPED (1u << ('\'' - ' '))
#define FLAGMASK (ALT_FORM | ZERO_PAD | LEFT_ADJ | PAD_POS | MARK_POS | GROUPED)

/* The argument kinds the state machine can produce. */
enum {
  BARE,
  LPRE,
  LLPRE,
  HPRE,
  HHPRE,
  BIGLPRE,
  ZTPRE,
  JPRE,
  STOP,
  PTR,
  INT,
  UINT,
  ULLONG,
  LONG,
  ULONG,
  SHORT,
  USHORT,
  CHAR,
  UCHAR,
  LLONG,
  SIZET,
  IMAX,
  UMAX,
  PDIFF,
  UIPTR,
  DBL,
  LDBL,
  NOARG
};

#define S(x) [(x) - 'A']

static const unsigned char states[][('z' - 'A' + 1)] = {
    {/* 0: bare types */
     S('d') = INT, S('i') = INT, S('o') = UINT, S('u') = UINT,
     S('x') = UINT, S('X') = UINT, S('e') = DBL, S('f') = DBL,
     S('g') = DBL, S('a') = DBL, S('E') = DBL, S('F') = DBL,
     S('G') = DBL, S('A') = DBL, S('c') = INT, S('C') = UINT,
     S('s') = PTR, S('S') = PTR, S('p') = UIPTR, S('n') = PTR,
     S('l') = LPRE, S('h') = HPRE, S('L') = BIGLPRE, S('z') = ZTPRE,
     S('j') = JPRE, S('t') = ZTPRE},
    {/* 1: l-prefixed */
     S('d') = LONG, S('i') = LONG, S('o') = ULONG, S('u') = ULONG,
     S('x') = ULONG, S('X') = ULONG, S('e') = DBL, S('f') = DBL,
     S('g') = DBL, S('a') = DBL, S('E') = DBL, S('F') = DBL,
     S('G') = DBL, S('A') = DBL, S('c') = UINT, S('s') = PTR,
     S('n') = PTR, S('l') = LLPRE},
    {/* 2: ll-prefixed */
     S('d') = LLONG, S('i') = LLONG, S('o') = ULLONG, S('u') = ULLONG,
     S('x') = ULLONG, S('X') = ULLONG, S('n') = PTR},
    {/* 3: h-prefixed */
     S('d') = SHORT, S('i') = SHORT, S('o') = USHORT, S('u') = USHORT,
     S('x') = USHORT, S('X') = USHORT, S('n') = PTR, S('h') = HHPRE},
    {/* 4: hh-prefixed */
     S('d') = CHAR, S('i') = CHAR, S('o') = UCHAR, S('u') = UCHAR,
     S('x') = UCHAR, S('X') = UCHAR, S('n') = PTR},
    {/* 5: L-prefixed */
     S('e') = LDBL, S('f') = LDBL, S('g') = LDBL, S('a') = LDBL,
     S('E') = LDBL, S('F') = LDBL, S('G') = LDBL, S('A') = LDBL,
     S('n') = PTR},
    {/* 6: z- or t-prefixed */
     S('d') = PDIFF, S('i') = PDIFF, S('o') = SIZET, S('u') = SIZET,
     S('x') = SIZET, S('X') = SIZET, S('n') = PTR},
    {/* 7: j-prefixed */
     S('d') = IMAX, S('i') = IMAX, S('o') = UMAX, S('u') = UMAX,
     S('x') = UMAX, S('X') = UMAX, S('n') = PTR},
};

#define OOB(x) ((unsigned)(x) - 'A' > 'z' - 'A')

union arg {
  uintmax_t i;
  double f;
  void *p;
};

/* One destination: a caller buffer, or stdout when dst is null. */
struct out {
  char *dst;
  size_t cap;
  size_t len;
};

static void emit(struct out *o, const char *s, size_t n) {
  if (!n)
    return;
  if (o->dst) {
    /* At most cap-1 bytes are stored, but len keeps counting what would
     * have been written. */
    if (o->len < o->cap) {
      size_t room = o->cap - 1 - o->len;
      size_t k = MIN(n, room);
      memcpy(o->dst + o->len, s, k);
    }
  } else {
    __msil_write(s, n);
  }
  o->len += n;
}

static void pad(struct out *o, char c, int w, int l, unsigned fl) {
  char buf[256];
  if ((fl & (LEFT_ADJ | ZERO_PAD)) || l >= w)
    return;
  l = w - l;
  memset(buf, c, (size_t)MIN(l, (int)sizeof buf));
  for (; l >= (int)sizeof buf; l -= (int)sizeof buf)
    emit(o, buf, sizeof buf);
  emit(o, buf, (size_t)l);
}

static const char xdigits[16] = "0123456789ABCDEF";

static char *fmt_x(uintmax_t x, char *s, int lower) {
  for (; x; x >>= 4)
    *--s = (char)(xdigits[x & 15] | (unsigned)lower);
  return s;
}

static char *fmt_o(uintmax_t x, char *s) {
  for (; x; x >>= 3)
    *--s = (char)('0' + (x & 7));
  return s;
}

static char *fmt_u(uintmax_t x, char *s) {
  unsigned long y;
  for (; x > ULONG_MAX; x /= 10)
    *--s = (char)('0' + x % 10);
  for (y = (unsigned long)x; y >= 10; y /= 10)
    *--s = (char)('0' + y % 10);
  if (y)
    *--s = (char)('0' + y);
  return s;
}

/* Bit access to a double. A memcpy through a pointer would be turned into a
 * float-to-integer conversion once inlined at -O2: LLVM sees a load of a
 * pointer that only ever held this double and folds the copy away, which for a
 * subnormal is not a bit copy. bit_cast is an intrinsic and stays one. */
static uint64_t double_bits(double d) { return __builtin_bit_cast(uint64_t, d); }

static double bits_double(uint64_t u) { return __builtin_bit_cast(double, u); }

/* ---- Floating point conversions ---------------------------------------- */

/* A double is held in base-1e9 chunks, most significant first (a) to one past
 * the least (z), with the radix point after chunk r. 144 chunks cover both
 * the 1024-bit integer part and the 1074 bits of fraction a subnormal needs.
 * The layout, the 29-bit scale and the rounding rules are musl's. */
#define BIG_CHUNKS 144

static void double_init(double y, uint32_t *big, uint32_t **a_out,
                        uint32_t **r_out, uint32_t **z_out, int *e2_out) {
  uint64_t bits = double_bits(y);
  int biased = (int)((bits >> 52) & 0x7ff);
  uint32_t *a, *r, *z;
  double y0;
  int e2;

  if (y == 0.0) {
    big[0] = 0;
    *a_out = *r_out = big;
    *z_out = big + 1;
    *e2_out = 0;
    return;
  }

  if (biased == 0) {
    /* Subnormal: multiplying by 2^52 is exact and brings the leading bit into
     * the normal range, which is enough to read its exponent. */
    y0 = y * 4503599627370496.0;
    bits = double_bits(y0);
    e2 = (int)((bits >> 52) & 0x7ff) - 1023 - 52;
  } else {
    e2 = biased - 1023;
  }

  /* The mantissa alone, as a value in [1,2): keep the fraction bits and set
   * the exponent field to bias. musl gets the same quantity from frexp. */
  y0 = bits_double((bits & 0x000fffffffffffffull) | (1023ull << 52));

  /* Scale to [2^28,2^29) so the leading chunk holds 29 bits, as musl's
   * `y *= 0x1p28, e2 -= 28` does. */
  y0 *= 268435456.0;
  e2 -= 28;

  /* The array grows leftward for values above one and rightward below it, so
   * leave room on the side it will need. */
  if (e2 < 0) {
    a = r = z = big;
  } else {
    a = r = z = big + BIG_CHUNKS - 64;
  }

  for (;;) {
    uint32_t c = (uint32_t)y0;
    *z++ = c;
    y0 = 1000000000.0 * (y0 - (double)c);
    if (y0 == 0.0)
      break;
  }
  *a_out = a;
  *r_out = r;
  *z_out = z;
  *e2_out = e2;
}

/* Multiply the chunk array by 2^e2, growing at the front. */
static void double_scale(uint32_t **a_io, uint32_t **z_io, int e2) {
  uint32_t *a = *a_io, *z = *z_io;
  while (e2 > 0) {
    uint32_t carry = 0;
    int sh = e2 < 29 ? e2 : 29;
    uint32_t *d;
    for (d = z - 1; d >= a; d--) {
      uint64_t x = ((uint64_t)*d << sh) + carry;
      *d = (uint32_t)(x % 1000000000u);
      carry = (uint32_t)(x / 1000000000u);
    }
    if (carry)
      *--a = carry;
    while (z > a && !z[-1])
      z--;
    e2 -= sh;
  }
  while (e2 < 0) {
    uint32_t carry = 0, *d;
    int sh = -e2 < 9 ? -e2 : 9;
    for (d = a; d < z; d++) {
      uint32_t rm = *d & ((1u << sh) - 1);
      *d = (*d >> sh) + carry;
      carry = (1000000000u >> sh) * rm;
    }
    if (!*a)
      a++;
    if (carry)
      *z++ = carry;
    e2 += sh;
  }
  *a_io = a;
  *z_io = z;
}

/* One floating conversion: %e %f %g and upper case, plus %a/%A. */
static int fmt_fp(struct out *o, double y, int w, int p, unsigned fl, int t) {
  uint32_t big[BIG_CHUNKS];
  uint32_t *a, *r, *z, *d;
  int e2, e, i, j, k, l;
  char buf[24], *s;
  char ebuf[16], *estr, *ebufend = ebuf + sizeof ebuf;
  const char *prefix = "-0X+0X 0X-0x+0x 0x";
  int pl = 1;

  if (double_bits(y) >> 63) {
    y = -y;
  } else if (fl & MARK_POS) {
    prefix += 3;
  } else if (fl & PAD_POS) {
    prefix += 6;
  } else {
    prefix++;
    pl = 0;
  }

  if (y != y || (y + y == y && y != 0.0)) {
    const char *str = (t & 32) ? "inf" : "INF";
    if (y != y)
      str = (t & 32) ? "nan" : "NAN";
    pad(o, ' ', w, 3 + pl, fl & ~ZERO_PAD);
    emit(o, prefix, (size_t)pl);
    emit(o, str, 3);
    pad(o, ' ', w, 3 + pl, fl ^ LEFT_ADJ);
    return MAX(w, 3 + pl);
  }

  if (p < 0)
    p = 6;

  double_init(y, big, &a, &r, &z, &e2);
  double_scale(&a, &z, e2);

  /* Decimal exponent of the leading digit, as musl computes it: the array
   * position of a relative to the radix point r, times nine, plus the digit
   * count of the leading chunk. When a > r the value is below one and
   * 9*(r-a) is negative, which is what makes the same formula cover both. */
  if (a < z) {
    e = 9 * (int)(r - a);
    for (i = 10; *a >= (uint32_t)i; i *= 10)
      e++;
  } else {
    e = 0;
  }

  /* Rounding. j is the position of the first digit to drop, counted from the
   * leading digit, for the non-fixed forms. */
  j = p - ((t | 32) != 'f') * e - ((t | 32) == 'g' && p);
  if (j < 9 * (int)(z - r - 1)) {
    uint32_t x;
    /* Which chunk holds the last kept digit, and which digit within it. j
     * may be negative, so the division is a floor, not C's truncation. */
    int chunk = j >= 0 ? j / 9 : -((-j + 8) / 9);
    int rem = j - chunk * 9;
    d = r + 1 + chunk;
    /* i is the place value of the first dropped digit: musl's
     * `for (i=10, j++; j<9; i*=10, j++)` with j being the true remainder. */
    for (i = 1, k = 0; k < 8 - rem; k++)
      i *= 10;
    i *= 10;
    x = *d % (uint32_t)i;
    if (x || d + 1 != z) {
      int up;
      if (x > (uint32_t)i / 2) {
        up = 1;
      } else if (x < (uint32_t)i / 2) {
        up = 0;
      } else {
        uint32_t *q;
        int later = 0;
        for (q = d + 1; q < z; q++)
          if (*q) {
            later = 1;
            break;
          }
        if (later)
          up = 1;
        else if (i == 1000000000)
          /* The tie is at a chunk boundary, so the parity of the last kept
           * chunk decides. */
          up = d > a && (d[-1] & 1);
        else
          up = (int)((*d / (uint32_t)i) & 1);
      }
      *d -= x;
      if (up) {
        *d = *d + (uint32_t)i;
        while (*d > 999999999u) {
          *d-- = 0;
          if (d < a)
            *--a = 0;
          (*d)++;
        }
        if (a < z) {
          e = 9 * (int)(r - a);
          for (i = 10; *a >= (uint32_t)i; i *= 10)
            e++;
        } else {
          e = 0;
        }
      }
    }
    if (z > d + 1)
      z = d + 1;
  }
  for (; z > a && !z[-1]; z--)
    ;

  if ((t | 32) == 'g') {
    if (!p)
      p++;
    if (p > e && e >= -4) {
      t--;
      p -= e + 1;
    } else {
      t -= 2;
      p--;
    }
    if (!(fl & ALT_FORM)) {
      /* Trailing zeros in the last chunk are not significant digits. */
      if (z > a && z[-1]) {
        for (i = 10, j = 0; z[-1] % (uint32_t)i == 0; i *= 10, j++)
          ;
      } else {
        j = 9;
      }
      if ((t | 32) == 'f')
        p = MIN(p, MAX(0, 9 * (int)(z - r - 1) - j));
      else
        p = MIN(p, MAX(0, 9 * (int)(z - r - 1) + e - j));
    }
  }

  if (p > INT_MAX - 1 - (p || (fl & ALT_FORM)))
    return -1;
  l = 1 + p + (p || (fl & ALT_FORM));
  if ((t | 32) == 'f') {
    if (e > 0)
      l += e;
  } else {
    estr = fmt_u(e < 0 ? (uintmax_t)-e : (uintmax_t)e, ebufend);
    while (ebufend - estr < 2)
      *--estr = '0';
    *--estr = e < 0 ? '-' : '+';
    *--estr = (char)t;
    l += (int)(ebufend - estr);
  }

  pad(o, ' ', w, pl + l, fl);
  emit(o, prefix, (size_t)pl);
  pad(o, '0', w, pl + l, fl ^ ZERO_PAD);

  if ((t | 32) == 'f') {
    if (a > r)
      a = r;
    for (d = a; d <= r; d++) {
      s = fmt_u(*d, buf + 9);
      if (d != a)
        while (s > buf)
          *--s = '0';
      else if (s == buf + 9)
        *--s = '0';
      emit(o, s, (size_t)(buf + 9 - s));
    }
    if (p || (fl & ALT_FORM))
      emit(o, ".", 1);
    for (; d < z && p > 0; d++, p -= 9) {
      s = fmt_u(*d, buf + 9);
      while (s > buf)
        *--s = '0';
      emit(o, s, (size_t)MIN(9, p));
    }
    pad(o, '0', p + 9, 9, 0);
  } else {
    if (z <= a)
      z = a + 1;
    for (d = a; d < z && p >= 0; d++) {
      s = fmt_u(*d, buf + 9);
      if (s == buf + 9)
        *--s = '0';
      if (d != a)
        while (s > buf)
          *--s = '0';
      else {
        emit(o, s++, 1);
        if (p > 0 || (fl & ALT_FORM))
          emit(o, ".", 1);
      }
      emit(o, s, (size_t)MIN(buf + 9 - s, p));
      p -= (int)(buf + 9 - s);
    }
    pad(o, '0', p + 18, 18, 0);
    emit(o, estr, (size_t)(ebufend - estr));
  }

  pad(o, ' ', w, pl + l, fl ^ LEFT_ADJ);
  return MAX(w, pl + l);
}

/* %a / %A: the exact hexadecimal form. */
/* musl's `%a`, with long double narrowed to double: frexp's result is
 * recovered from the bits, and the rounding probe is a power of two the double
 * can hold exactly. A double has 53 mantissa bits, so 13 hex digits cover it. */
#define DBL_HEX_DIGITS 13

static int fmt_hexfloat(struct out *o, double y, int w, int p, unsigned fl,
                        int t) {
  const char *prefix = "-0X+0X 0X-0x+0x 0x";
  char buf[2 + DBL_HEX_DIGITS], *s;
  char ebuf[16], *estr, *ebufend = ebuf + sizeof ebuf;
  int pl = 1, e2 = 0, l;

  if (double_bits(y) >> 63) {
    y = -y;
  } else if (fl & MARK_POS) {
    prefix += 3;
  } else if (fl & PAD_POS) {
    prefix += 6;
  } else {
    prefix++;
    pl = 0;
  }
  if (t & 32)
    prefix += 9;
  pl += 2;

  if (y != y || (y + y == y && y != 0.0)) {
    const char *str = (t & 32) ? "inf" : "INF";
    if (y != y)
      str = (t & 32) ? "nan" : "NAN";
    pad(o, ' ', w, 3 + pl, fl & ~ZERO_PAD);
    emit(o, prefix, (size_t)pl);
    emit(o, str, 3);
    pad(o, ' ', w, 3 + pl, fl ^ LEFT_ADJ);
    return MAX(w, 3 + pl);
  }

  /* y = frexp(y, &e2) * 2, then e2-- : y becomes the mantissa in [1,2). For a
   * subnormal the leading one has to be found in the fraction bits first. */
  if (y != 0.0) {
    uint64_t bits = double_bits(y);
    uint64_t m = bits & 0x000fffffffffffffull;
    int biased = (int)((bits >> 52) & 0x7ff);
    if (biased) {
      e2 = biased - 1023;
    } else {
      int k = 51;
      while (!(m >> k))
        k--;
      e2 = k - 1074;
      m = (m << (52 - k)) & 0x000fffffffffffffull;
    }
    y = bits_double(m | (1023ull << 52));
  }

  if (p >= 0 && p < DBL_HEX_DIGITS) {
    /* 2^(52-4p) is the half-ULP of the last kept hex digit; adding and
     * subtracting it rounds y to p fraction digits, ties to even. */
    double round = bits_double((uint64_t)(1023 + 52 - 4 * p) << 52);
    if (*prefix == '-') {
      y = -y;
      y -= round;
      y += round;
      y = -y;
    } else {
      y += round;
      y -= round;
    }
  }

  estr = fmt_u(e2 < 0 ? (uintmax_t)-e2 : (uintmax_t)e2, ebufend);
  if (estr == ebufend)
    *--estr = '0';
  *--estr = e2 < 0 ? '-' : '+';
  *--estr = (char)(t + ('p' - 'a'));

  s = buf;
  do {
    int x = (int)y;
    *s++ = (char)(xdigits[x] | (unsigned)(t & 32));
    y = 16 * (y - x);
    if (s - buf == 1 && (y || p > 0 || (fl & ALT_FORM)))
      *s++ = '.';
  } while (y);

  if (p && s - buf - 2 < p)
    l = (p + 2) + (int)(ebufend - estr);
  else
    l = (int)(s - buf) + (int)(ebufend - estr);

  pad(o, ' ', w, pl + l, fl);
  emit(o, prefix, (size_t)pl);
  pad(o, '0', w, pl + l, fl ^ ZERO_PAD);
  emit(o, buf, (size_t)(s - buf));
  pad(o, '0', l - (int)(ebufend - estr) - (int)(s - buf), 0, 0);
  emit(o, estr, (size_t)(ebufend - estr));
  pad(o, ' ', w, pl + l, fl ^ LEFT_ADJ);
  return MAX(w, pl + l);
}

/* ---- Argument fetching ------------------------------------------------- */

/* The backend passes a vararg as int32, int64 (also a double's raw bits) or
 * void*. Each kind below maps to the accessor whose CLR type the call site
 * used. */
static void pop_arg(union arg *arg, int type, void *ap) {
  switch (type) {
  case PTR:
    arg->p = __msil_va_ptr(ap);
    return;
  case UIPTR:
    arg->i = (uintmax_t)(uintptr_t)__msil_va_ptr(ap);
    return;
  case INT:
  case UINT:
  case LONG:
  case ULONG:
  case LLONG:
  case ULLONG:
  case SIZET:
  case IMAX:
  case UMAX:
  case PDIFF:
  case CHAR:
  case UCHAR:
  case SHORT:
  case USHORT:
    /* Every integer crosses as int64, whatever its C type: the back end
     * widens each vararg so the read is predictable. The narrow cases below
     * then cut it back to the declared width. */
    arg->i = (uintmax_t)__msil_va_i64(ap);
    break;
  case DBL:
  case LDBL:
    /* MSVC's long double is a double, and both cross as raw int64 bits. */
    arg->f = __msil_va_f64(ap);
    return;
  default:
    arg->i = 0;
    return;
  }
  /* A narrow argument arrived widened to int by the default promotions, so
   * only its low bits are significant. */
  switch (type) {
  case CHAR:
    arg->i = (uintmax_t)(intmax_t)(signed char)arg->i;
    break;
  case UCHAR:
    arg->i = (uintmax_t)(unsigned char)arg->i;
    break;
  case SHORT:
    arg->i = (uintmax_t)(intmax_t)(short)arg->i;
    break;
  case USHORT:
    arg->i = (uintmax_t)(unsigned short)arg->i;
    break;
  case INT:
    arg->i = (uintmax_t)(intmax_t)(int)arg->i;
    break;
  case UINT:
    arg->i = (uintmax_t)(unsigned)arg->i;
    break;
  case LONG:
    arg->i = (uintmax_t)(intmax_t)(long)arg->i;
    break;
  case ULONG:
    arg->i = (uintmax_t)(unsigned long)arg->i;
    break;
  default:
    break;
  }
}

static int getint(const char **sp) {
  const char *s = *sp;
  int i = 0;
  for (; isdigit((unsigned char)*s); s++) {
    if (i > INT_MAX / 10 || *s - '0' > INT_MAX - 10 * i)
      i = -1;
    else
      i = 10 * i + (*s - '0');
  }
  *sp = s;
  return i;
}

/* The shared formatter. \p ap points at the caller's va_list slot, which the
 * accessors advance in place. */
static int printf_core(struct out *o, const char *fmt, void *ap) {
  const char *a, *z, *s = fmt;
  unsigned fl;
  int w, p, xp;
  union arg arg;
  unsigned st, ps;
  int l = 0;
  char buf[24];
  const char *prefix;
  int t, pl;

  for (;;) {
    if (!*s)
      break;

    for (a = s; *s && *s != '%'; s++)
      ;
    for (z = s; s[0] == '%' && s[1] == '%'; z++, s += 2)
      ;
    if (z - a) {
      emit(o, a, (size_t)(z - a));
      l += (int)(z - a);
      continue;
    }

    s++;
    for (fl = 0; (unsigned)*s - ' ' < 32 && (FLAGMASK & (1u << (*s - ' '))); s++)
      fl |= 1u << (*s - ' ');

    if (*s == '*') {
      w = (int)__msil_va_i64(ap);
      s++;
      if (w == INT_MIN)
        return -1;
      if (w < 0)
        fl |= LEFT_ADJ, w = -w;
    } else if ((w = getint(&s)) < 0) {
      return -1;
    }

    if (*s == '.' && s[1] == '*') {
      p = (int)__msil_va_i64(ap);
      s += 2;
      xp = (p >= 0);
    } else if (*s == '.') {
      s++;
      p = getint(&s);
      xp = 1;
    } else {
      p = -1;
      xp = 0;
    }

    st = 0;
    do {
      if (OOB(*s))
        return -1;
      ps = st;
      st = states[st]S(*s++);
    } while (st - 1 < STOP);
    if (!st)
      return -1;

    if (st != NOARG)
      pop_arg(&arg, (int)st, ap);

    z = buf + sizeof buf;
    prefix = "-+   0X0x";
    pl = 0;
    t = s[-1];

    /* %ls/%lc reduce to %s/%c: the argument is treated as narrow. */
    if (ps && (t & 15) == 3)
      t &= ~32;

    if (fl & LEFT_ADJ)
      fl &= ~ZERO_PAD;

    switch (t) {
    case 'n':
      switch (ps) {
      case BARE:
        *(int *)arg.p = l;
        break;
      case LPRE:
        *(long *)arg.p = l;
        break;
      case LLPRE:
        *(long long *)arg.p = l;
        break;
      case HPRE:
        *(unsigned short *)arg.p = (unsigned short)l;
        break;
      case HHPRE:
        *(unsigned char *)arg.p = (unsigned char)l;
        break;
      case ZTPRE:
        *(size_t *)arg.p = (size_t)l;
        break;
      case JPRE:
        *(uintmax_t *)arg.p = (uintmax_t)l;
        break;
      default:
        break;
      }
      continue;
    case 'p':
      p = MAX(p, 2 * (int)sizeof(void *));
      t = 'x';
      fl |= ALT_FORM;
      /* fall through */
    case 'x':
    case 'X':
      a = fmt_x(arg.i, z, t & 32);
      if (arg.i && (fl & ALT_FORM))
        prefix += (t >> 4), pl = 2;
      goto ifmt_tail;
    case 'o':
      a = fmt_o(arg.i, z);
      if ((fl & ALT_FORM) && p < z - a + 1)
        p = (int)(z - a + 1);
      goto ifmt_tail;
    case 'd':
    case 'i':
      pl = 1;
      if (arg.i > (uintmax_t)INTMAX_MAX) {
        arg.i = -arg.i;
      } else if (fl & MARK_POS) {
        prefix++;
      } else if (fl & PAD_POS) {
        prefix += 2;
      } else {
        pl = 0;
      }
      /* fall through */
    case 'u':
      a = fmt_u(arg.i, z);
    ifmt_tail:
      if (xp && p < 0)
        return -1;
      if (xp)
        fl &= ~ZERO_PAD;
      if (!arg.i && !p) {
        a = z;
        break;
      }
      p = MAX(p, (int)(z - a) + !arg.i);
      break;
    case 'c':
      *(char *)(a = z - (p = 1)) = (char)arg.i;
      fl &= ~ZERO_PAD;
      break;
    case 's':
      a = arg.p ? (const char *)arg.p : "(null)";
      z = a + strnlen(a, p < 0 ? (size_t)INT_MAX : (size_t)p);
      if (p < 0 && *z)
        return -1;
      p = (int)(z - a);
      fl &= ~ZERO_PAD;
      break;
    case 'e':
    case 'f':
    case 'g':
    case 'E':
    case 'F':
    case 'G':
    case 'a':
    case 'A':
      if (xp && p < 0)
        return -1;
      l += ((t | 32) == 'a') ? fmt_hexfloat(o, arg.f, w, p, fl, t)
                             : fmt_fp(o, arg.f, w, p, fl, t);
      continue;
    default:
      return -1;
    }

    if (p < z - a)
      p = (int)(z - a);
    if (p > INT_MAX - pl)
      return -1;
    if (w < pl + p)
      w = pl + p;

    pad(o, ' ', w, pl + p, fl);
    emit(o, prefix, (size_t)pl);
    pad(o, '0', w, pl + p, fl ^ ZERO_PAD);
    pad(o, '0', p, (int)(z - a), 0);
    emit(o, a, (size_t)(z - a));
    pad(o, ' ', w, pl + p, fl ^ LEFT_ADJ);

    l += w;
  }
  return l;
}

/* Run the core into \p o, then terminate a caller buffer. */
static int finish(struct out *o, const char *fmt, void *ap) {
  int r = printf_core(o, fmt, ap);
  if (o->dst && o->cap) {
    size_t at = o->len < o->cap - 1 ? o->len : o->cap - 1;
    o->dst[at] = 0;
  }
  return r;
}

int vsnprintf(char *dst, size_t n, const char *fmt, va_list ap) {
  struct out o;
  o.dst = dst;
  o.cap = n;
  o.len = 0;
  /* The accessors advance the cursor, so they take its address. */
  return finish(&o, fmt, &ap);
}

int vsprintf(char *dst, const char *fmt, va_list ap) {
  return vsnprintf(dst, SIZE_MAX, fmt, ap);
}

int vprintf(const char *fmt, va_list ap) {
  struct out o;
  o.dst = 0;
  o.cap = 0;
  o.len = 0;
  return finish(&o, fmt, &ap);
}

int snprintf(char *dst, size_t n, const char *fmt, ...) {
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = vsnprintf(dst, n, fmt, ap);
  va_end(ap);
  return r;
}

int sprintf(char *dst, const char *fmt, ...) {
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = vsprintf(dst, fmt, ap);
  va_end(ap);
  return r;
}

int printf(const char *fmt, ...) {
  va_list ap;
  int r;
  va_start(ap, fmt);
  r = vprintf(fmt, ap);
  va_end(ap);
  return r;
}

int puts(const char *s) {
  size_t n = strlen(s);
  __msil_write(s, n);
  __msil_write("\n", 1);
  return 0;
}

int putchar(int c) {
  char ch = (char)c;
  __msil_write(&ch, 1);
  return c;
}
