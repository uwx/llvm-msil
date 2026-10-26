/* Native test shim: runs the same libc sources under the host compiler so
 * their output can be compared against the real C library.
 *
 * The seam functions below stand in for the IL the backend emits. The native
 * host ABI passes a vararg the same way the CLR seam models it (one 8-byte
 * slot, doubles in an SSE slot), so `va_arg` with the C type the format says
 * reads the same argument the CLR accessor would.
 */
#include "../libc.h"

/* Write to the process's stdout. Declared here rather than including
 * <stdio.h>, so this file still builds with -nostdinc next to the others.
 * Windows spells it _write; the POSIX hosts spell it write. */
#ifdef _WIN32
int _write(int fd, const void *buf, unsigned n);
#define HOST_WRITE _write
typedef unsigned host_unsigned;
#else
long write(int fd, const void *buf, unsigned long n);
#define HOST_WRITE write
typedef unsigned long host_unsigned;
#endif

void __msil_write(const void *p, size_t n) {
  const char *s = p;
  unsigned long done = 0;
  while (done < n) {
    host_unsigned chunk =
        (n - done) > 0x7fffffffu ? 0x7fffffffu : (host_unsigned)(n - done);
    long w = HOST_WRITE(1, s + done, chunk);
    if (w <= 0)
      return;
    done += (unsigned long)w;
  }
}

#define va_arg(ap, t) __builtin_va_arg(ap, t)

int64_t __msil_va_i64(void *ap) {
  /* The MSIL backend widens every integer vararg to int64, so the native
   * stand-in has to read it the same way. C's default promotions make this
   * exact for int; a long long was already 64 bits. */
  va_list *p = (va_list *)ap;
  return va_arg(*p, long long);
}

double __msil_va_f64(void *ap) {
  va_list *p = (va_list *)ap;
  return va_arg(*p, double);
}

void *__msil_va_ptr(void *ap) {
  va_list *p = (va_list *)ap;
  return va_arg(*p, void *);
}

/* Host stand-ins for the functions the writer emits as IL. */
void *memcpy(void *d, const void *s, size_t n) {
  char *dp = d;
  const char *sp = s;
  while (n--)
    *dp++ = *sp++;
  return d;
}

void *memmove(void *d, const void *s, size_t n) {
  char *dp = d;
  const char *sp = s;
  if (dp < sp) {
    while (n--)
      *dp++ = *sp++;
  } else {
    while (n--)
      dp[n] = sp[n];
  }
  return d;
}

void *memset(void *d, int c, size_t n) {
  char *dp = d;
  while (n--)
    *dp++ = (char)c;
  return d;
}

static size_t slen(const char *s) {
  size_t n = 0;
  while (s[n])
    n++;
  return n;
}

size_t strlen(const char *s) {
  return slen(s);
}

char *strcpy(char *d, const char *s) {
  char *r = d;
  while ((*d++ = *s++))
    ;
  return r;
}

char *strncpy(char *d, const char *s, size_t n) {
  char *r = d;
  while (n && (*d++ = *s++, n--))
    ;
  while (n--)
    *d++ = 0;
  return r;
}
