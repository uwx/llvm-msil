/* Exercises the portable C library through llvm-msil.
 *
 * Every result is printed through printf, so this also tests printf: it is
 * both the subject and the reporting channel. libc.sh compares the output
 * against a native build of the same file, which is the reference behaviour.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* The case-insensitive comparisons are POSIX. MSVC spells them with a leading
 * underscore, so the native reference build needs the map while the MSIL build
 * gets the portable library's own definitions. */
#if defined(_MSC_VER) && !defined(__clang__) || defined(MSIL_LIBC_NATIVE)
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
int _stricmp(const char *, const char *);
int _strnicmp(const char *, const char *, size_t);
#else
int strcasecmp(const char *, const char *);
int strncasecmp(const char *, const char *, size_t);
#endif

static char buf[256];

static void show(const char *tag, const char *s) {
  printf("%-14s [%s]\n", tag, s);
}

static void pint(const char *tag, long v) {
  snprintf(buf, sizeof buf, "%ld", v);
  show(tag, buf);
}

static int intcmp(const void *a, const void *b) {
  int x = *(const int *)a, y = *(const int *)b;
  return x < y ? -1 : x > y;
}

static int strptrcmp(const void *a, const void *b) {
  return strcmp(*(const char *const *)a, *(const char *const *)b);
}

int main(void) {
  char *end;
  int i;

  /* ctype */
  pint("isalpha A", isalpha('A') != 0);
  pint("isalpha 7", isalpha('7') != 0);
  pint("isdigit 7", isdigit('7') != 0);
  pint("isspace nl", isspace('\n') != 0);
  pint("isspace x", isspace('x') != 0);
  pint("ispunct #", ispunct('#') != 0);
  pint("isxdigit g", isxdigit('g') != 0);
  pint("toupper q", toupper('q'));
  pint("tolower Q", tolower('Q'));

  /* strtol family: the end pointer and the bases are what compatibility
   * hinges on, so each case prints both the value and where it stopped. */
  pint("strtol dec", strtol("  1234abc", &end, 10));
  show("strtol end", end);
  pint("strtol neg", strtol("-42", 0, 10));
  pint("strtol hex", strtol("0xBEEF", 0, 0));
  pint("strtol oct", strtol("0755", 0, 0));
  pint("strtol 0x", strtol("0x", &end, 0));
  show("strtol 0x end", end);
  pint("strtol none", strtol("xyz", &end, 10));
  show("strtol none end", end);
  pint("strtol base2", strtol("1011", 0, 2));
  pint("strtol base36", strtol("zz", 0, 36));
  pint("strtol clamp", strtol("99999999999999999999999", 0, 10) == LONG_MAX);
  pint("atoi", atoi(" -77x"));
  pint("atol", atol("+8"));
  pint("strtoul max", strtoul("18446744073709551616", 0, 10) == ULONG_MAX);
  pint("abs", abs(-5));
  pint("labs", (long)(labs(-1234567890123L) / 1000));
  {
    div_t d = div(-17, 5);
    pint("div quot", d.quot);
    pint("div rem", d.rem);
  }

  /* string */
  pint("strspn", (long)strspn("abcbaz", "abc"));
  pint("strcspn", (long)strcspn("hello world", " "));
  pint("strnlen", (long)strnlen("hello", 3));
  pint("strcasecmp", strcasecmp("abc", "ABD") < 0);
  pint("strncasecmp", strncasecmp("ABCx", "abcY", 3) == 0);
  {
    char t[32];
    strcpy(t, "ab");
    strncat(t, "cdef", 3);
    show("strncat", t);
    strcpy(t, "a,b;c");
    show("strtok", strtok(t, ",;"));
    show("strtok2", strtok(0, ",;"));
  }
  {
    const char *s = "needle in haystack";
    pint("strstr", (long)((const char *)strstr(s, "in") - s));
    pint("memchr", (long)((const char *)memchr(s, 'i', strlen(s)) - s));
    pint("strpbrk", (long)((const char *)strpbrk(s, "vy") - s));
  }

  /* sprintf / snprintf truncation and the return value */
  {
    char small[8];
    int r = snprintf(small, 4, "%s", "abcdefgh");
    pint("snprintf ret", r);
    show("snprintf str", small);
    r = snprintf(small, sizeof small, "%s", "abcdefgh");
    pint("snprintf fit", r);
    show("snprintf fit", small);
  }

  /* integer conversions and flags */
  snprintf(buf, sizeof buf, "[%d][%5d][%-5d][%05d][%+d][% d]", 42, 42, 42, 42,
           42, 42);
  show("int flags", buf);
  snprintf(buf, sizeof buf, "[%x][%X][%#x][%o][%#o][%u]", 255, 255, 255, 8, 8,
           4000000000u);
  show("base flags", buf);
  snprintf(buf, sizeof buf, "[%c][%3c][%-3c]", 'A', 'B', 'C');
  show("char", buf);
  snprintf(buf, sizeof buf, "[%10s][%-10s][%.2s][%5.2s]", "hi", "hi", "abcdef",
           "abcdef");
  show("string width", buf);
  snprintf(buf, sizeof buf, "[%ld][%lld][%zu][%hhx][%hhd]", 123456789L,
           1234567890123LL, (size_t)7, 0x1ff, (signed char)-5);
  show("length mods", buf);
  snprintf(buf, sizeof buf, "[%*d][%-*d][%.*f]", 6, 42, 6, 42, 2, 3.14159);
  show("star", buf);

  /* floating point: every conversion, the rounding cases, and the special
   * values, which is where a port is most likely to disagree. */
  snprintf(buf, sizeof buf, "[%f][%.2f][%10.3f][%-10.3f][%010.3f]", 3.14159,
           3.14159, 2.71828, 2.71828, 2.71828);
  show("f widths", buf);
  snprintf(buf, sizeof buf, "[%f][%f][%.0f][%.0f][%.0f]", 0.0, -0.0, 0.5, 1.5,
           2.5);
  show("f rounding", buf);
  snprintf(buf, sizeof buf, "[%f][%.1f][%.2f][%.10f]", 1.0 / 3, 1.0 / 3,
           1.0 / 3, 1.0 / 3);
  show("f thirds", buf);
  snprintf(buf, sizeof buf, "[%e][%.2e][%E][%15.4e]", 1234.5678, 1234.5678,
           1234.5678, 1234.5678);
  show("e", buf);
  snprintf(buf, sizeof buf, "[%e][%e]", 0.0, -1.5);
  show("e zero", buf);
  snprintf(buf, sizeof buf, "[%g][%g][%g][%g][%g]", 100000.0, 1000000.0, 0.0001,
           0.00001, 123456.0);
  show("g", buf);
  snprintf(buf, sizeof buf, "[%g][%.3g][%#g][%G]", 0.0, 3.14159, 1.0, 0.000123);
  show("g flags", buf);
  snprintf(buf, sizeof buf, "%.3f", 1e20);
  show("f huge", buf);
  snprintf(buf, sizeof buf, "%.10f", 1e-10);
  show("f tiny", buf);
  snprintf(buf, sizeof buf, "[%f][%F][%e]", 1.0 / 0.0, 1.0 / 0.0, -1.0 / 0.0);
  show("inf", buf);
  snprintf(buf, sizeof buf, "[%f][%F][%g]", 0.0 / 0.0, 0.0 / 0.0, 0.0 / 0.0);
  show("nan", buf);

  /* %a is compared against a literal, not against the native build: the C
   * libraries disagree on this conversion, so the native reference is MSVC's
   * answer and the port deliberately keeps musl's. These are musl 1.2.4's
   * outputs, checked on the same values. */
  snprintf(buf, sizeof buf, "[%a][%a][%a][%A]", 1.0, 0.5, 0.0, 1.0);
  show("hex floats", buf);
  snprintf(buf, sizeof buf, "[%a][%.3a][%#.0a]", 4.9406564584124654e-324,
           1.0, 1.0);
  show("hex subnormal", buf);

  /* qsort and bsearch over two element types, which also exercises the
   * callback path from C into a caller-supplied comparison. */
  {
    int a[9] = {5, 3, 9, 1, 7, 2, 8, 0, 6};
    int key = 7;
    qsort(a, 9, sizeof a[0], intcmp);
    for (i = 0; i < 9; i++)
      pint("sorted", a[i]);
    pint("bsearch", *(const int *)bsearch(&key, a, 9, sizeof a[0], intcmp));
    pint("bsearch miss",
         bsearch(&(int){42}, a, 9, sizeof a[0], intcmp) == 0);
  }
  {
    const char *w[5] = {"pear", "apple", "fig", "banana", "cherry"};
    qsort(w, 5, sizeof w[0], strptrcmp);
    for (i = 0; i < 5; i++)
      show("word", w[i]);
  }

  return 0;
}
