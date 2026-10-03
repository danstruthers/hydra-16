/*
** fwrite.c - fputc, fwrite, fputs and puts, into the fd's buffer (hyfile.h): out when it's full, at an LF if it's
** a console's, or when asked; a big write goes straight out.  (They take the place of cc65's, which write each
** call.)
*/

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "hyfile.h"

int __fastcall__ fputc (int c, register FILE* f)
{
    register struct hy_fbuf* b;

    if ((f->f_flags & (_FOPEN | _FERROR)) != _FOPEN || _hy_towrite (f)) {
        return EOF;
    }
    b = FBUF (f);
    b->buf[b->n++] = c;
    if ((b->n == FBSIZE (b) || (c == '\n' && (b->flags & FB_LINE))) && _hy_flush (f)) {
        return EOF;
    }
    return (unsigned char) c;
}

size_t __fastcall__ fwrite (const void* buf, size_t size, size_t count, register FILE* f)
{
    register struct hy_fbuf* b;
    unsigned want, room;

    if ((f->f_flags & (_FOPEN | _FERROR)) != _FOPEN) {
        _seterrno (EINVAL);
        return 0;
    }
    if ((want = size * count) == 0 || _hy_towrite (f)) {
        return 0;
    }
    b = FBUF (f);
    room = FBSIZE (b);
    if (want > room - b->n) {                        /* (No room: what's waiting out first) */
        if (_hy_flush (f)) {
            return 0;
        }
        if (want >= room) {                          /* (A big write: straight out) */
            if (write (f->f_fd, buf, want) != want) {
                f->f_flags |= _FERROR;
                return 0;
            }
            return count;
        }
    }
    memcpy (b->buf + b->n, buf, want);
    b->n += want;
    if ((b->n == room || ((b->flags & FB_LINE) && memchr (buf, '\n', want))) && _hy_flush (f)) {
        return 0;
    }
    return count;
}

int __fastcall__ fputs (const char* s, FILE* f)
{
    unsigned n = strlen (s);

    return n == 0 || fwrite (s, n, 1, f) == 1 ? 0 : EOF;
}

int __fastcall__ puts (const char* s)
{
    return fputs (s, stdout) == EOF || fputc ('\n', stdout) == EOF ? EOF : 0;
}
