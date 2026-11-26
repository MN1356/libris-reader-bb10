#ifndef ESPEAK_WCTYPE_SHIM_H
#define ESPEAK_WCTYPE_SHIM_H
/* QNX libc wide ctype does not handle non-ASCII in the "C" locale: use UCD tables. */
#include <wctype.h>
#include "ucd/ucd.h"
#undef iswalpha
#undef iswalnum
#undef iswdigit
#undef iswspace
#undef iswupper
#undef iswlower
#undef iswpunct
#undef towlower
#undef towupper
#define iswalpha(c)  ucd_isalpha((codepoint_t)(c))
#define iswalnum(c)  ucd_isalnum((codepoint_t)(c))
#define iswdigit(c)  ucd_isdigit((codepoint_t)(c))
#define iswspace(c)  ucd_isspace((codepoint_t)(c))
#define iswupper(c)  ucd_isupper((codepoint_t)(c))
#define iswlower(c)  ucd_islower((codepoint_t)(c))
#define iswpunct(c)  ucd_ispunct((codepoint_t)(c))
#define towlower(c)  ucd_tolower((codepoint_t)(c))
#define towupper(c)  ucd_toupper((codepoint_t)(c))
#endif
