/* Minimal freestanding environment for the MSIL built-in libc.
 *
 * This file is compiled with -ffreestanding -nostdinc so the generated IR
 * depends on nothing but the C source in this directory: the checked-in
 * libc.ll must be reproducible from any clang without the host's headers.
 *
 * The subset implemented here is portable C. Anything that needs a syscall,
 * a file, a locale or an OS interface lives outside it and keeps its pinvoke.
 */
#ifndef MSIL_LIBC_H
#define MSIL_LIBC_H

typedef __SIZE_TYPE__ size_t;
typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __INT8_TYPE__ int8_t;
typedef __UINT8_TYPE__ uint8_t;
typedef __INT16_TYPE__ int16_t;
typedef __UINT16_TYPE__ uint16_t;
typedef __INT32_TYPE__ int32_t;
typedef __UINT32_TYPE__ uint32_t;
typedef __INT64_TYPE__ int64_t;
typedef __UINT64_TYPE__ uint64_t;
typedef __INTPTR_TYPE__ intptr_t;
typedef __UINTPTR_TYPE__ uintptr_t;
typedef __INTMAX_TYPE__ intmax_t;
typedef __UINTMAX_TYPE__ uintmax_t;

#define NULL ((void *)0)
#define INT_MAX __INT_MAX__
#define INTMAX_MAX __INTMAX_MAX__
#define INTMAX_MIN (-__INTMAX_MAX__ - 1)
#define INT_MIN (-__INT_MAX__ - 1)
#define LONG_MAX __LONG_MAX__
#define LONG_MIN (-__LONG_MAX__ - 1)
#define LLONG_MAX __LONG_LONG_MAX__
#define LLONG_MIN (-__LONG_LONG_MAX__ - 1)
#define ULLONG_MAX (__LONG_LONG_MAX__ * 2ULL + 1ULL)
#define ULONG_MAX (__LONG_MAX__ * 2UL + 1UL)
#define SIZE_MAX (__SIZE_TYPE__)-1
#define CHAR_BIT 8
#define SCHAR_MAX 127
#define UCHAR_MAX 255
#define SHRT_MAX 32767
#define USHRT_MAX 65535

/* Absolute value macros that avoid the signed-overflow UB of `-x`. */
#define MSIL_ABS(x) ((x) < 0 ? -(x) : (x))

/* A vararg definition is not a CLR `vararg` method: the backend passes the
 * trailing arguments in a buffer and lowers llvm.va_start to point the
 * va_list at it, so the accessors below read 8-byte slots through that
 * pointer (see printLibcSeam). */
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_va_start(ap, last)
#define va_end(ap) __builtin_va_end(ap)
#define va_copy(d, s) __builtin_va_copy(d, s)

/* Seam into the CLR, emitted as IL by the backend. Each accessor takes the
 * address of the va_list, not its value, because reading an argument
 * advances the cursor the caller owns.
 *
 * A vararg crosses as int64 or void*, never as its C type: the backend
 * widens every integer argument and reinterprets a double's bits as int64,
 * so the callee can read any argument back as one of those two. */
extern int64_t __msil_va_i64(void *ap);
extern double __msil_va_f64(void *ap);
extern void *__msil_va_ptr(void *ap);
extern void __msil_write(const void *p, size_t n);

/* Already implemented in IL behind these same C names, so they are only
 * declared here. Calling memcpy/strlen by name keeps the -msil-alloc hook
 * and the IL byte loops in effect for the C code too. malloc is the IL
 * allocator (NativeMemory), available under -msil-libc=builtin; strndup
 * below is the one C function here that allocates. */
extern void *malloc(size_t n);
extern void free(void *p);
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);
extern int memcmp(const void *a, const void *b, size_t n);
extern size_t strlen(const char *s);
extern int strcmp(const char *a, const char *b);
extern int strncmp(const char *a, const char *b, size_t n);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t n);
extern char *strcat(char *dst, const char *src);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *h, const char *n);
extern char *strdup(const char *s);

/* ctype.c */
int isalnum(int c);
int isalpha(int c);
int isascii(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int toascii(int c);
int tolower(int c);
int toupper(int c);

/* string.c */
size_t strnlen(const char *s, size_t n);
char *strncat(char *dst, const char *src, size_t n);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char *strpbrk(const char *s, const char *accept);
char *strtok(char *s, const char *sep);
char *strtok_r(char *s, const char *sep, char **p);
int strcasecmp(const char *a, const char *b);
int strncasecmp(const char *a, const char *b, size_t n);
void *memchr(const void *s, int c, size_t n);
void *memrchr(const void *s, int c, size_t n);
int strcoll(const char *a, const char *b);
size_t strxfrm(char *dst, const char *src, size_t n);

/* string.c, GNU/BSD/POSIX additions. */
void *memccpy(void *dst, const void *src, int c, size_t n);
void *mempcpy(void *dst, const void *src, size_t n);
char *stpcpy(char *dst, const char *src);
char *stpncpy(char *dst, const char *src, size_t n);
char *strsep(char **sp, const char *sep);
char *strcasestr(const char *haystack, const char *needle);
char *strndup(const char *s, size_t n);
char *strchrnul(const char *s, int c);
void explicit_bzero(void *dst, size_t n);

/* stdlib.c */
int atoi(const char *s);
long atol(const char *s);
long long atoll(const char *s);
long strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
long long strtoll(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
intmax_t strtoimax(const char *s, char **end, int base);
uintmax_t strtoumax(const char *s, char **end, int base);
double strtod(const char *s, char **end);
float strtof(const char *s, char **end);
int abs(int x);
long labs(long x);
long long llabs(long long x);
typedef struct { int quot, rem; } div_t;
typedef struct { long quot, rem; } ldiv_t;
typedef struct { long long quot, rem; } lldiv_t;
typedef struct { intmax_t quot, rem; } imaxdiv_t;
div_t div(int num, int den);
ldiv_t ldiv(long num, long den);
lldiv_t lldiv(long long num, long long den);
imaxdiv_t imaxdiv(intmax_t num, intmax_t den);
intmax_t imaxabs(intmax_t x);
int ffs(int x);
int ffsl(long x);
int ffsll(long long x);
void qsort(void *base, size_t n, size_t width,
           int (*cmp)(const void *, const void *));
void *bsearch(const void *key, const void *base, size_t n, size_t width,
              int (*cmp)(const void *, const void *));
int rand(void);
void srand(unsigned seed);

/* stdio.c */
int printf(const char *fmt, ...);
int vprintf(const char *fmt, va_list ap);
int sprintf(char *dst, const char *fmt, ...);
int snprintf(char *dst, size_t n, const char *fmt, ...);
int vsprintf(char *dst, const char *fmt, va_list ap);
int vsnprintf(char *dst, size_t n, const char *fmt, va_list ap);
int puts(const char *s);
int putchar(int c);

/* Output without a FILE: stdout is the CLR console, so a `void *` stands in
 * for the FILE* a program would pass. */
int fputc(int c, void *stream);
int fputs(const char *s, void *stream);
size_t fwrite(const void *p, size_t sz, size_t n, void *stream);

/* stdio.c: fixed English messages, no locale. */
char *strerror(int errnum);

#endif
