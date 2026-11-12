/* String functions not already implemented as IL in the writer.
 *
 * Ported from musl src/string/*.c. The ones the writer emits as IL (strlen,
 * strcmp, strncmp, strcpy, strncpy, strcat, strchr, strrchr, strstr, strdup)
 * stay there: IL reaches them by C name, so the C code here uses them too.
 * These are the rest of what a printf-family C program reaches for.
 */
#include "libc.h"

size_t strnlen(const char *s, size_t n) {
  const char *p = s;
  while (n && *p) {
    p++;
    n--;
  }
  return (size_t)(p - s);
}

char *strncat(char *dst, const char *src, size_t n) {
  char *d = dst + strlen(dst);
  while (n && *src) {
    *d++ = *src++;
    n--;
  }
  *d = 0;
  return dst;
}

size_t strspn(const char *s, const char *accept) {
  const char *a = s;
  /* A 256-bit set fits in one word per 64 characters. */
  uint64_t byteset[32 / sizeof(uint64_t)] = {0};
  if (!accept[0])
    return 0;
  if (!accept[1]) {
    for (; *s == *accept; s++)
      ;
    return (size_t)(s - a);
  }
  for (; *accept; accept++)
    byteset[(unsigned char)*accept >> 6] |= 1ull << ((unsigned char)*accept & 63);
  for (; *s; s++)
    if (!(byteset[(unsigned char)*s >> 6] & (1ull << ((unsigned char)*s & 63))))
      break;
  return (size_t)(s - a);
}

size_t strcspn(const char *s, const char *reject) {
  const char *a = s;
  uint64_t byteset[32 / sizeof(uint64_t)] = {0};
  if (!reject[0])
    return strlen(s);
  if (!reject[1]) {
    for (; *s && *s != *reject; s++)
      ;
    return (size_t)(s - a);
  }
  for (; *reject; reject++)
    byteset[(unsigned char)*reject >> 6] |= 1ull << ((unsigned char)*reject & 63);
  for (; *s; s++)
    if (byteset[(unsigned char)*s >> 6] & (1ull << ((unsigned char)*s & 63)))
      break;
  return (size_t)(s - a);
}

char *strpbrk(const char *s, const char *accept) {
  s += strcspn(s, accept);
  return *s ? (char *)s : 0;
}

/* Reentrant tokenizer: each call resumes at *p, so the caller owns the
 * cursor. strtok keeps a static cursor so it is not reentrant, matching C. */
char *strtok_r(char *s, const char *sep, char **p) {
  if (!s && !(s = *p))
    return 0;
  s += strspn(s, sep);
  if (!*s)
    return *p = 0;
  *p = s + strcspn(s, sep);
  if (**p)
    *(*p)++ = 0;
  else
    *p = 0;
  return s;
}

char *strtok(char *s, const char *sep) {
  static char *p;
  return strtok_r(s, sep, &p);
}

int strcasecmp(const char *a, const char *b) {
  const unsigned char *l = (const unsigned char *)a;
  const unsigned char *r = (const unsigned char *)b;
  for (; *l && *r && (*l == *r || tolower(*l) == tolower(*r)); l++, r++)
    ;
  return tolower(*l) - tolower(*r);
}

int strncasecmp(const char *a, const char *b, size_t n) {
  const unsigned char *l = (const unsigned char *)a;
  const unsigned char *r = (const unsigned char *)b;
  if (!n--)
    return 0;
  for (; *l && *r && n && (*l == *r || tolower(*l) == tolower(*r)); l++, r++, n--)
    ;
  return tolower(*l) - tolower(*r);
}

void *memchr(const void *src, int c, size_t n) {
  const unsigned char *s = src;
  unsigned char ch = (unsigned char)c;
  for (; n; n--, s++)
    if (*s == ch)
      return (void *)s;
  return 0;
}

void *memrchr(const void *src, int c, size_t n) {
  const unsigned char *s = src;
  unsigned char ch = (unsigned char)c;
  while (n--)
    if (s[n] == ch)
      return (void *)(s + n);
  return 0;
}

/* No locale support, so collation is byte order, like strcmp. */
int strcoll(const char *a, const char *b) {
  return strcmp(a, b);
}

/* With collation equal to byte order, the transform is a plain copy that
 * still reports the length the destination would need. */
size_t strxfrm(char *dst, const char *src, size_t n) {
  size_t len = strlen(src);
  if (n) {
    size_t k = len < n ? len : n - 1;
    memcpy(dst, src, k);
    dst[k] = 0;
  }
  return len;
}

/* --- POSIX/GNU additions, ported from musl ------------------------------- */

void *memccpy(void *dst, const void *src, int c, size_t n) {
  const unsigned char *s = src;
  unsigned char *d = dst;
  unsigned char ch = (unsigned char)c;
  for (; n; n--, d++, s++) {
    *d = *s;
    if (*s == ch)
      return d + 1;
  }
  return 0;
}

void *mempcpy(void *dst, const void *src, size_t n) {
  return (char *)memcpy(dst, src, n) + n;
}

char *stpcpy(char *dst, const char *src) {
  while ((*dst = *src++))
    dst++;
  return dst;
}

char *stpncpy(char *dst, const char *src, size_t n) {
  for (; n && *src; n--, dst++, src++)
    *dst = *src;
  if (n) {
    /* The terminator was copied as part of the loop's last iteration only if
     * the source ran out exactly at n; otherwise pad the rest. */
    char *end = dst;
    while (n--)
      *dst++ = 0;
    return end;
  }
  return dst;
}

/* Each call returns the token starting at *sp and advances *sp past the
 * separator that ended it; the separator is replaced with a null. */
char *strsep(char **sp, const char *sep) {
  char *s = *sp;
  if (!s)
    return 0;
  char *p = s + strcspn(s, sep);
  if (*p) {
    *p++ = 0;
    *sp = p;
  } else {
    *sp = 0;
  }
  return s;
}

/* Case-insensitive strstr. Without a locale, "case" is ASCII tolower. */
char *strcasestr(const char *h, const char *n) {
  size_t nl = strlen(n);
  if (!nl)
    return (char *)h;
  for (; *h; h++) {
    size_t i = 0;
    while (i < nl && h[i] &&
           tolower((unsigned char)h[i]) == tolower((unsigned char)n[i]))
      i++;
    if (i == nl)
      return (char *)h;
  }
  return 0;
}

/* strndup is strdup with a bounded length: the result is at most n bytes
 * plus a terminator. malloc/memcpy are the IL implementations. */
char *strndup(const char *s, size_t n) {
  size_t len = strnlen(s, n);
  char *d = malloc(len + 1);
  if (!d)
    return 0;
  memcpy(d, s, len);
  d[len] = 0;
  return d;
}

/* Like strchr, but returns the terminator's address rather than null when
 * the character is not found. */
char *strchrnul(const char *s, int c) {
  unsigned char ch = (unsigned char)c;
  for (;; s++)
    if (*s == ch || !*s)
      return (char *)s;
}

/* A volatile store is enough to stop the compiler from removing a scrub of
 * memory that is about to be freed: the CLR never moves managed references
 * here, so a plain byte loop through a volatile pointer is the whole job. */
void explicit_bzero(void *dst, size_t n) {
  volatile unsigned char *p = dst;
  while (n--)
    *p++ = 0;
}
