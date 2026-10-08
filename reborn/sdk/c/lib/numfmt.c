/*
** numfmt.c - num_format and num_vformat (num.h): the numbers library's FORMAT, its arguments' table made from C's:
** a number (a num_t*) for each placeholder the format string has, as the library finds them ({} and {base}; {{ and
** }} a brace; a { with no } after it, itself), FORMAT_MAX at most.
*/

#include <stdarg.h>
#include <num.h>

#define FORMAT_MAX      16

int __fastcall__ _num_format (char* dst, unsigned room, const char* fmt, const unsigned char* args);   /* num.s */

static unsigned char args[FORMAT_MAX * 3 + 1];

int num_format (char* dst, unsigned room, const char* fmt, ...)
{
    va_list ap;
    int n;

    va_start (ap, fmt);
    n = num_vformat (dst, room, fmt, ap);
    va_end (ap);
    return n;
}

int __fastcall__ num_vformat (char* dst, unsigned room, const char* fmt, va_list ap)
{
    const char* p = fmt;
    const char* q;
    unsigned char* t = args;
    unsigned char n = 0;

    while (*p) {
        if (*p == '{') {
            if (p[1] == '{') {
                p += 2;
                continue;
            }
            for (q = p + 1; *q && *q != '}'; ++q) {
            }
            if (*q) {
                if (n == FORMAT_MAX) {
                    num_error = NE_FORMAT;
                    return -1;
                }
                t[0] = NFMT_NUMBER;
                *(const num_t**) (t + 1) = va_arg (ap, const num_t*);
                t += 3;
                ++n;
                p = q + 1;
                continue;
            }
        } else if (*p == '}' && p[1] == '}') {
            p += 2;
            continue;
        }
        ++p;
    }
    *t = NFMT_END;
    return _num_format (dst, room, fmt, args);
}
