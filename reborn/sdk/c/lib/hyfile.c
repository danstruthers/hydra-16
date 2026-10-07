/*
** hyfile.c - the stdio buffers (hyfile.h): each fd's, made as a FILE first uses it, filled by a read and written
** out when full, at an LF (a console's), or when asked; all of them written out as the program ends.
*/

#include <stdlib.h>
#include <unistd.h>
#include "hyfile.h"

struct hy_fbuf _hy_fbuf[HY_FD_MAX];

/* f's buffer, made if it isn't yet: FB_SIZE bytes, or one (stderr's, or no memory).  (The first: the buffers out
** at the program's end) */
static struct hy_fbuf* __fastcall__ buffer (FILE* f)
{
    static unsigned char registered;
    register struct hy_fbuf* b = FBUF (f);

    if (b->buf == 0) {
        if (!registered) {
            registered = 1;
            atexit (_hy_flushall);
        }
        b->flags = isatty (f->f_fd) ? FB_LINE : 0;
        if (f == stderr || (b->buf = malloc (BUFSIZ)) == 0) {
            b->buf = &b->one;
            b->flags |= FB_ONE;
        }
    }
    return b;
}

int __fastcall__ _hy_read (FILE* f, void* p, unsigned n)
{
    register struct hy_fbuf* o = FBUF (stdout);
    int got;

    if ((o->flags & FB_LINE) && o->mode == FB_WRITE && o->n) {     /* (A prompt, before its answer) */
        _hy_flush (stdout);
    }
    got = read (f->f_fd, p, n);
    if (got <= 0) {
        f->f_flags |= got ? _FERROR : _FEOF;
    }
    return got;
}

int __fastcall__ _hy_fill (FILE* f)
{
    register struct hy_fbuf* b = buffer (f);
    int got;

    if (b->mode == FB_WRITE && _hy_flush (f)) {
        return -1;
    }
    b->mode = FB_READ;
    b->n = b->at = 0;
    got = _hy_read (f, b->buf, FBSIZE (b));
    if (got > 0) {
        b->n = got;
    }
    return got;
}

int __fastcall__ _hy_towrite (FILE* f)
{
    register struct hy_fbuf* b = buffer (f);

    if (b->mode != FB_WRITE) {
        if (b->mode == FB_READ) {                       /* (Read ahead: given back, if it can be) */
            _hy_flush (f);
        }
        b->mode = FB_WRITE;
        b->n = 0;
    }
    return 0;
}

int __fastcall__ _hy_flush (FILE* f)
{
    register struct hy_fbuf* b = FBUF (f);
    unsigned char n = b->n;

    if (b->mode == FB_WRITE) {
        b->n = 0;
        if (n && write (f->f_fd, b->buf, n) != n) {
            f->f_flags |= _FERROR;
            return -1;
        }
    } else if (b->mode == FB_READ && b->at < n) {       /* (A pipe's or a console's can't be: kept) */
        if (lseek (f->f_fd, (long) b->at - n, SEEK_CUR) >= 0) {
            b->n = b->at = 0;
        }
    }
    return 0;
}

void _hy_flushall (void)
{
    register FILE* f;

    for (f = _filetab; f < _filetab + FOPEN_MAX; ++f) {
        if ((f->f_flags & _FOPEN) && FBUF (f)->mode == FB_WRITE) {
            _hy_flush (f);
        }
    }
}
