/* Conformance harness. Prints a fixed battery of results, and test/libc.sh
 * compares three builds of it: against the host C library, against the ported
 * sources natively, and through llvm-msil on the CLR. Any disagreement is a
 * compatibility bug, located by which pair differs.
 *
 * Built with -D HOST_CRT it uses the real C library, so it also compiles with
 * the host's own headers; that is why the includes are conditional.
 */
#ifdef HOST_CRT
#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// MSVC has no <strings.h>; these two are declared here. StrCaseCmp is the
// CRT spelling, so the comparison is what the C library actually does.
int _stricmp(const char *, const char *);
int _strnicmp(const char *, const char *, size_t);
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include "../libc.h"
#endif

/* The test writes through the seam function rather than through printf, so
 * its output does not depend on the library under test. */
extern void __msil_write(const void *p, size_t n);
/* The native build force-includes tests/rename.h, which prefixes every public
 * name on both sides of the comparison. That keeps the import library's
 * definitions from colliding with the port's own, so the test links whichever
 * it was asked to. */

static char buf[512];

static int intcmp(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return x < y ? -1 : x > y;
}

static void show(const char *tag, const char *s) {
  __msil_write(tag, strlen(tag));
  __msil_write("=[", 2);
  __msil_write(s, strlen(s));
  __msil_write("]\n", 2);
}

static void pint(const char *tag, long v) {
  snprintf(buf, sizeof buf, "%ld", v);
  show(tag, buf);
}

static void hexline(const unsigned char *p, int n) {
  static const char *h = "0123456789abcdef";
  char out[128];
  int i;
  for (i = 0; i < n * 2; i++)
    out[i] = h[(p[i / 2] >> (i & 1 ? 0 : 4)) & 15];
  out[i] = 0;
  show("hex", out);
}

int main(void) {
  char *end;

  /* ctype */
  pint("isalpha_A", (isalpha('A')) != 0);
  pint("isalpha_dig", (isalpha('7')) != 0);
  pint("isdigit", (isdigit('7')) != 0);
  pint("isspace_nl", (isspace('\n')) != 0);
  pint("isspace_x", (isspace('x')) != 0);
  pint("ispunct", (ispunct('#')) != 0);
  pint("isxdigit_g", (isxdigit('g')) != 0);
  pint("toupper_q", toupper('q'));
  pint("tolower_Q", tolower('Q'));
  pint("isprint_7f", (isprint(0x7f)) != 0);

  /* strtol family */
  pint("strtol_dec", strtol("  1234abc", &end, 10));
  show("strtol_end", end);
  pint("strtol_neg", strtol("-42", 0, 10));
  pint("strtol_hex", strtol("0xBEEF", 0, 0));
  pint("strtol_oct", strtol("0755", 0, 0));
  pint("strtol_zero", strtol("0x", &end, 0));
  show("strtol_zero_end", end);
  pint("strtol_none", strtol("xyz", &end, 10));
  show("strtol_none_end", end);
  pint("strtol_base2", strtol("1011", 0, 2));
  pint("strtol_base36", strtol("zz", 0, 36));
  pint("strtol_big", strtol("99999999999999999999999", 0, 10) == LONG_MAX);
  pint("atoi", atoi(" -77x"));
  pint("atol_plus", atol("+8"));

  /* unsigned saturate */
  pint("strtoul_max", strtoul("18446744073709551616", 0, 10) == ULONG_MAX);

  /* integer conversions */
  pint("abs_min", abs(-5));
  pint("labs", (long)(labs(-1234567890123L) / 1000));
  {
    div_t d = div(-17, 5);
    pint("div_q", d.quot);
    pint("div_r", d.rem);
  }

  /* string */
  pint("strspn", (long)strspn("abcbaz", "abc"));
  pint("strcspn", (long)strcspn("hello world", " "));
  pint("strnlen", (long)strnlen("hello", 3));
  pint("strcasecmp_lt", strcasecmp("abc", "ABD") < 0);
  pint("strncasecmp_eq", strncasecmp("ABCx", "abcY", 3) == 0);
  {
    char t[32];
    strcpy(t, "ab");
    strncat(t, "cdef", 3);
    show("strncat", t);
  }
  {
    char t[16];
    strcpy(t, "a,b;c");
    show("strtok1", strtok(t, ",;"));
  }

  /* snprintf truncation and return value */
  {
    char small[8];
    int r = snprintf(small, 4, "%s", "abcdefgh");
    pint("snprintf_ret", r);
    show("snprintf_str", small);
  }

  /* integer formatting */
  show("fmt_d", (snprintf(buf, sizeof buf, "[%d][%5d][%-5d][%05d][%+d][% d]",
                          42, 42, 42, 42, 42, 42), buf));
  show("fmt_x", (snprintf(buf, sizeof buf, "[%x][%X][%#x][%o][%#o][%u]", 255,
                          255, 255, 8, 8, 4000000000u), buf));
  show("fmt_c", (snprintf(buf, sizeof buf, "[%c][%3c][%-3c]", 'A', 'B', 'C'),
                 buf));
  show("fmt_s", (snprintf(buf, sizeof buf, "[%10s][%-10s][%.2s][%5.2s]", "hi",
                          "hi", "abcdef", "abcdef"), buf));
  show("fmt_p", (snprintf(buf, sizeof buf, "%p", (void *)0), buf));
  /* %n is not in the shared battery: the MSVC CRT disables it by default
   * and aborts the process rather than writing through the pointer, so there
   * is no reference behaviour to compare against. */

  /* length modifiers */
  show("fmt_len", (snprintf(buf, sizeof buf, "[%ld][%lld][%zu][%hhx][%hhd]",
                            123456789L, 1234567890123LL, (size_t)7, 0x1ff,
                            (signed char)-5), buf));

  /* floating point */
  show("fmt_f", (snprintf(buf, sizeof buf, "[%f][%.2f][%10.3f][%-10.3f][%010.3f]",
                          3.14159, 3.14159, 2.71828, 2.71828, 2.71828), buf));
  show("fmt_f2", (snprintf(buf, sizeof buf, "[%f][%f][%.0f][%.0f][%.0f]", 0.0,
                           -0.0, 0.5, 1.5, 2.5), buf));
  show("fmt_f3", (snprintf(buf, sizeof buf, "[%f][%.1f][%.2f][%.10f]", 1.0 / 3,
                          1.0 / 3, 1.0 / 3, 1.0 / 3), buf));
  show("fmt_e", (snprintf(buf, sizeof buf, "[%e][%.2e][%E][%15.4e]", 1234.5678,
                          1234.5678, 1234.5678, 1234.5678), buf));
  show("fmt_e2", (snprintf(buf, sizeof buf, "[%e][%e]", 0.0, -1.5), buf));
  show("fmt_g", (snprintf(buf, sizeof buf, "[%g][%g][%g][%g][%g]", 100000.0,
                          1000000.0, 0.0001, 0.00001, 123456.0), buf));
  show("fmt_g2", (snprintf(buf, sizeof buf, "[%g][%.3g][%#g][%G]", 0.0, 3.14159,
                           1.0, 0.000123), buf));
  show("fmt_big", (snprintf(buf, sizeof buf, "%.3f", 1e20), buf));
  show("fmt_tiny", (snprintf(buf, sizeof buf, "%.10f", 1e-10), buf));
  show("fmt_inf", (snprintf(buf, sizeof buf, "[%f][%F][%e]", 1.0 / 0.0,
                            1.0 / 0.0, -1.0 / 0.0), buf));
  show("fmt_nan", (snprintf(buf, sizeof buf, "[%f][%F][%g]", 0.0 / 0.0,
                            0.0 / 0.0, 0.0 / 0.0), buf));

  /* Rounding has to carry into the digits already kept, and %g has to strip
   * the zeros that leaves behind. Both decide a different digit if the
   * comparison in fmt_fp is off by one. */
  show("fmt_round", (snprintf(buf, sizeof buf, "[%.6f][%.6f][%.15g][%.3g]",
                              3.14159265358979, 123456789.123456789,
                              1e-5, 3.14159265358979), buf));
  show("fmt_round2", (snprintf(buf, sizeof buf, "[%.0f][%.0f][%g][%g]", 1.5, 2.5,
                               999999.5, 1e23), buf));
  /* The subnormal path reads the exponent back out of the bits after scaling
   * by 2^52, which only stays exact while the value is still a bit pattern. */
  show("fmt_subnormal", (snprintf(buf, sizeof buf, "[%e][%e][%.20e]",
                                  4.9406564584124654e-324,
                                  2.2250738585072014e-308,
                                  2.2250738585072014e-308), buf));

  /* star width and precision take int varargs before the value */
  show("fmt_star", (snprintf(buf, sizeof buf, "[%*d][%-*d][%.*f]", 6, 42, 6, 42,
                             2, 3.14159), buf));

  /* rand's sequence is implementation-defined, so the property to check is
   * determinism: the same seed must reproduce the same run, and the values
   * must stay in range. */
  {
    int r1[8], r2[8], i;
    srand(1);
    for (i = 0; i < 8; i++)
      r1[i] = rand();
    srand(1);
    for (i = 0; i < 8; i++)
      r2[i] = rand();
    pint("rand_reproducible", memcmp(r1, r2, sizeof r1) == 0);
    pint("rand_in_range", (r1[0] >= 0 && r1[0] <= 0x7fffffff && r1[1] >= 0 &&
                           r1[1] <= 0x7fffffff));
    pint("rand_varies", r1[0] != r1[1]);
  }

  /* qsort and bsearch */
  {
    static int a[7] = {5, 3, 9, 1, 7, 2, 8};
    int key = 7;
    int i;
    qsort(a, 7, sizeof a[0], intcmp);
    for (i = 0; i < 7; i++)
      pint("sorted", a[i]);
    pint("bsearch", *(const int *)bsearch(&key, a, 7, sizeof a[0], intcmp));
  }
  return 0;
}
