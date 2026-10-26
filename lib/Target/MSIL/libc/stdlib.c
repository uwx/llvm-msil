/* Conversions and small numeric helpers, ported from musl.
 *
 * strtol/strtoul follow musl src/internal/intscan.c rather than the simpler
 * loop a hand-written version would use: the semantics that matter for
 * compatibility are the exact set of characters consumed (so `end` lands
 * where the C library would leave it) and the saturation on overflow.
 * errno is not available here, so overflow saturates silently.
 */
#include "libc.h"

typedef unsigned long long ullong;

/* -1 for a non-digit, otherwise the value in bases up to 36. */
static int digitval(int c) {
  if (c >= '0' && c <= '9')
    return c - '0';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z')
    return c - 'A' + 10;
  return -1;
}

/* The shared scanner. Returns the magnitude before sign; `*endp` receives
 * the first unconsumed character (or the original string if nothing was
 * converted). */
static ullong scan(const char *s, char **endp, int base, ullong lim,
                   int *negative) {
  const char *p = s;
  int neg = 0;
  int c;
  ullong y;

  if (base != 0 && (base < 2 || base > 36)) {
    if (endp)
      *endp = (char *)s;
    *negative = 0;
    return 0;
  }
  while (isspace((unsigned char)*p))
    p++;
  if (*p == '+' || *p == '-') {
    neg = (*p == '-');
    p++;
  }
  c = (unsigned char)*p;
  if ((base == 0 || base == 16) && c == '0') {
    p++;
    c = (unsigned char)*p;
    if ((c | 32) == 'x') {
      p++;
      c = (unsigned char)*p;
      if (digitval(c) >= 16) {
        /* "0x" with no digits is just the leading 0: consume one, not two. */
        p--;
        if (endp)
          *endp = (char *)p;
        *negative = 0;
        return 0;
      }
      base = 16;
    } else if (base == 0) {
      base = 8;
    }
  } else if (base == 0) {
    base = 10;
  }
  if (digitval(c) < 0 || digitval(c) >= base) {
    if (endp)
      *endp = (char *)s;
    *negative = 0;
    return 0;
  }

  y = 0;
  for (; digitval(c) >= 0 && digitval(c) < base; c = (unsigned char)*p) {
    p++;
    /* Saturating accumulate: once the multiply would wrap, keep consuming
     * digits but stop accumulating, and clamp at the end. */
    if (y > (lim - (ullong)digitval(c)) / (ullong)base) {
      y = lim;
      while (digitval((unsigned char)*p) >= 0 &&
             digitval((unsigned char)*p) < base)
        p++;
      break;
    }
    y = y * (ullong)base + (ullong)digitval(c);
  }
  if (endp)
    *endp = (char *)(p == s ? s : p);
  *negative = neg;
  return y;
}

long long strtoll(const char *s, char **endp, int base) {
  int neg;
  ullong y = scan(s, endp, base, (ullong)LLONG_MAX, &neg);
  return neg ? -(long long)y : (long long)y;
}

unsigned long long strtoull(const char *s, char **endp, int base) {
  int neg;
  ullong y = scan(s, endp, base, ULLONG_MAX, &neg);
  /* C says a negative value wraps modulo 2^64, and the magnitude was already
   * saturated at ULLONG_MAX, so the negate is the wrap. */
  return neg ? (ullong)0 - y : y;
}

long strtol(const char *s, char **endp, int base) {
  int neg;
  ullong y = scan(s, endp, base, (ullong)LONG_MAX, &neg);
  return neg ? -(long)y : (long)y;
}

unsigned long strtoul(const char *s, char **endp, int base) {
  int neg;
  ullong y = scan(s, endp, base, (ullong)ULONG_MAX, &neg);
  return neg ? (unsigned long)(0 - y) : (unsigned long)y;
}

int atoi(const char *s) {
  return (int)strtol(s, 0, 10);
}

long atol(const char *s) {
  return strtol(s, 0, 10);
}

long long atoll(const char *s) {
  return strtoll(s, 0, 10);
}

int abs(int a) {
  /* Not -a: negating INT_MIN is UB, and the unsigned round trip is what the
   * two's complement answer means. */
  return a > 0 ? a : (int)(0u - (unsigned)a);
}

long labs(long a) {
  return a > 0 ? a : (long)(0ul - (unsigned long)a);
}

long long llabs(long long a) {
  return a > 0 ? a : (long long)(0ull - (unsigned long long)a);
}

div_t div(int num, int den) {
  div_t r;
  r.quot = num / den;
  r.rem = num % den;
  return r;
}

ldiv_t ldiv(long num, long den) {
  ldiv_t r;
  r.quot = num / den;
  r.rem = num % den;
  return r;
}

lldiv_t lldiv(long long num, long long den) {
  lldiv_t r;
  r.quot = num / den;
  r.rem = num % den;
  return r;
}

/* Insertion sort: stable and obviously correct, which matters more here than
 * the asymptotic advantage. Elements are moved in width-sized chunks through
 * a byte buffer, so this works for any element type. */
static void isort(unsigned char *base, size_t n, size_t width,
                  int (*cmp)(const void *, const void *)) {
  unsigned char tmp[64];
  size_t i, j, k;
  for (i = 1; i < n; i++) {
    for (j = i; j > 0; j--) {
      unsigned char *a = base + (j - 1) * width;
      unsigned char *b = base + j * width;
      if (cmp(a, b) <= 0)
        break;
      for (k = 0; k < width; k += sizeof tmp) {
        size_t m = width - k < sizeof tmp ? width - k : sizeof tmp;
        memcpy(tmp, a + k, m);
        memcpy(a + k, b + k, m);
        memcpy(b + k, tmp, m);
      }
    }
  }
}

void qsort(void *base, size_t n, size_t width,
           int (*cmp)(const void *, const void *)) {
  if (width && n > 1)
    isort((unsigned char *)base, n, width, cmp);
}

void *bsearch(const void *key, const void *base, size_t nel, size_t width,
              int (*cmp)(const void *, const void *)) {
  while (nel > 0) {
    const void *try = (const char *)base + width * (nel / 2);
    int sign = cmp(key, try);
    if (sign < 0) {
      nel /= 2;
    } else if (sign > 0) {
      base = (const char *)try + width;
      nel -= nel / 2 + 1;
    } else {
      return (void *)try;
    }
  }
  return 0;
}

/* The simple LCG from C11's example. musl uses the same generator shape, and
 * every seed gives the documented sequence, which is what the standard
 * actually requires. */
static unsigned long long rng_state = 1;

int rand(void) {
  rng_state = rng_state * 6364136223846793005ull + 1;
  return (int)((rng_state >> 33) & 0x7fffffff);
}

void srand(unsigned seed) {
  rng_state = seed;
}
