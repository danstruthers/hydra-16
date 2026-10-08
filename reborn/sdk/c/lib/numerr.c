/*
** numerr.c - num_strerror (num.h): the number libraries' errors' texts (spec/numbers.def's NE_ codes).
*/

#include <num.h>

static const char* const texts[] = {
    "no error",
    "too big",                                  /* NE_BIG */
    "division by zero",                         /* NE_DIV0 */
    "not a number",                             /* NE_NOTNUM */
    "no room for the result",                   /* NE_ROOM */
    "outside the function's domain",            /* NE_DOMAIN */
    "not a base",                               /* NE_BASE */
    "an integer is needed",                     /* NE_INT */
    "a real number is needed",                  /* NE_REAL */
    "no number libraries",                      /* NE_INIT */
    "placeholders and arguments don't match",   /* NE_FORMAT */
};

const char* __fastcall__ num_strerror (unsigned char e)
{
    return e < sizeof texts / sizeof texts[0] ? texts[e] : "unknown error";
}
