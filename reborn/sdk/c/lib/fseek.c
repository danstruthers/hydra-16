/*
** fseek.c - fseek and ftell with its fd's buffer (hyfile.h): what's waiting written first, what was read ahead
** given back.  (They take the place of cc65's.)
*/

#include <errno.h>
#include <unistd.h>
#include "hyfile.h"

int __fastcall__ fseek (register FILE* f, long offset, int whence)
{
    register struct hy_fbuf* b = FBUF (f);

    if ((f->f_flags & _FOPEN) == 0) {
        _seterrno (EINVAL);
        return -1;
    }
    if (b->mode == FB_WRITE) {
        if (_hy_flush (f)) {
            return -1;
        }
    } else if (b->mode == FB_READ) {
        if (whence == SEEK_CUR) {                       /* (From where the reader is, not the read-ahead) */
            offset -= b->n - b->at;
        }
        b->n = b->at = 0;
    }
    if ((f->f_flags & _FPUSHBACK) && whence == SEEK_CUR) {
        --offset;
    }
    if (lseek (f->f_fd, offset, whence) < 0) {
        f->f_flags |= _FERROR;
        return -1;
    }
    f->f_flags &= ~(_FEOF | _FPUSHBACK);
    return 0;
}

long __fastcall__ ftell (register FILE* f)
{
    register struct hy_fbuf* b = FBUF (f);
    long pos;

    if ((f->f_flags & _FOPEN) == 0) {
        _seterrno (EINVAL);
        return -1L;
    }
    if ((pos = lseek (f->f_fd, 0L, SEEK_CUR)) < 0) {
        return -1L;
    }
    if (b->mode == FB_READ) {
        pos -= b->n - b->at;
    } else if (b->mode == FB_WRITE) {
        pos += b->n;
    }
    if (f->f_flags & _FPUSHBACK) {
        --pos;
    }
    return pos;
}
