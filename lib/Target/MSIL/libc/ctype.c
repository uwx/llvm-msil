/* Character classification, ported from musl src/ctype/*.c.
 *
 * musl's C locale is a fixed table encoded as arithmetic on the bit pattern,
 * which is exactly what a freestanding implementation wants: no locale
 * object, no table to initialize, and the same answers as musl.
 */
#include "libc.h"

int isalpha(int c) {
  return ((unsigned)c | 32) - 'a' < 26u;
}

int isdigit(int c) {
  return (unsigned)c - '0' < 10u;
}

int isalnum(int c) {
  return isalpha(c) || isdigit(c);
}

int isspace(int c) {
  /* ' ' and \t..\r, the five whitespace characters. */
  return c == ' ' || (unsigned)c - '\t' < 5u;
}

int isblank(int c) {
  return c == ' ' || c == '\t';
}

int iscntrl(int c) {
  return (unsigned)c < 0x20u || c == 0x7f;
}

int isprint(int c) {
  return (unsigned)c - 0x20u < 0x5fu;
}

int isgraph(int c) {
  return (unsigned)c - 0x21u < 0x5eu;
}

int ispunct(int c) {
  /* Printable, not alphanumeric and not a space. */
  return isgraph(c) && !isalnum(c);
}

int isupper(int c) {
  return (unsigned)c - 'A' < 26u;
}

int islower(int c) {
  return (unsigned)c - 'a' < 26u;
}

int isxdigit(int c) {
  return isdigit(c) || ((unsigned)c | 32) - 'a' < 6u;
}

int isascii(int c) {
  return !(c & ~0x7f);
}

int toascii(int c) {
  return c & 0x7f;
}

int tolower(int c) {
  if (isupper(c))
    return c | 32;
  return c;
}

int toupper(int c) {
  if (islower(c))
    return c & ~32;
  return c;
}
