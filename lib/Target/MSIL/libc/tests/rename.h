/* Force-included into the native build of the ported sources and the test, so
 * every public name gets a `msil_` prefix. Without it the import library's
 * definitions of the same names collide at link time, and whichever copy the
 * linker picked would make the comparison meaningless. The MSIL build does
 * not use this file: there the port is the only definition.
 */
#define abs msil_abs
#define labs msil_labs
#define llabs msil_llabs
#define div msil_div
#define ldiv msil_ldiv
#define lldiv msil_lldiv
#define atoi msil_atoi
#define atol msil_atol
#define atoll msil_atoll
#define strtol msil_strtol
#define strtoul msil_strtoul
#define strtoll msil_strtoll
#define strtoull msil_strtoull
#define qsort msil_qsort
#define bsearch msil_bsearch
#define rand msil_rand
#define srand msil_srand

#define isalnum msil_isalnum
#define isalpha msil_isalpha
#define isascii msil_isascii
#define isblank msil_isblank
#define iscntrl msil_iscntrl
#define isdigit msil_isdigit
#define isgraph msil_isgraph
#define islower msil_islower
#define isprint msil_isprint
#define ispunct msil_ispunct
#define isspace msil_isspace
#define isupper msil_isupper
#define isxdigit msil_isxdigit
#define toascii msil_toascii
#define tolower msil_tolower
#define toupper msil_toupper

#define memchr msil_memchr
#define memrchr msil_memrchr
#define strnlen msil_strnlen
#define strncat msil_strncat
#define strspn msil_strspn
#define strcspn msil_strcspn
#define strpbrk msil_strpbrk
#define strtok msil_strtok
#define strtok_r msil_strtok_r
#define strcoll msil_strcoll
#define strxfrm msil_strxfrm
#define strcasecmp msil_strcasecmp
#define strncasecmp msil_strncasecmp

#define printf msil_printf
#define vprintf msil_vprintf
#define sprintf msil_sprintf
#define snprintf msil_snprintf
#define vsprintf msil_vsprintf
#define vsnprintf msil_vsnprintf
#define puts msil_puts
#define putchar msil_putchar
