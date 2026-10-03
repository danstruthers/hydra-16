/*
** fread.c - fread, from its fd's buffer (hyfile.h), which a READ fills; a big fread reads straight into its own
** memory.  (It takes the place of cc65's, which reads straight in, a READ a call.)
*/

#include <string.h>
#include <errno.h>
#include "hyfile.h"

size_t __fastcall__ fread (void* buf, size_t size, size_t count, register FILE* f)
{
    register struct hy_fbuf* b;
    unsigned char* p = buf;
    unsigned want, got = 0, k;
    int n;

    if ((f->f_flags & (_FOPEN | _FERROR)) != _FOPEN) {
        _seterrno (EINVAL);
        return 0;
    }
    if ((want = size * count) == 0) {
        return 0;
    }
    if (f->f_flags & _FPUSHBACK) {
        f->f_flags &= ~_FPUSHBACK;
        *p = f->f_pushback;
        got = 1;
    }
    b = FBUF (f);
    while (got < want) {
        if (b->mode == FB_READ && b->at < b->n) {       /* What's in the buffer */
            k = b->n - b->at;
            if (k > want - got) {
                k = want - got;
            }
            memcpy (p + got, b->buf + b->at, k);
            b->at += k;
            got += k;
        } else if (f->f_flags & _FEOF) {
            break;
        } else if (want - got >= FB_SIZE && b->mode != FB_WRITE) {
            if ((n = _hy_read (f, p + got, want - got)) <= 0) {
                break;                                  /* (A big read: straight in) */
            }
            got += n;
        } else if (_hy_fill (f) <= 0) {
            break;
        }
    }
    return got / size;
}
