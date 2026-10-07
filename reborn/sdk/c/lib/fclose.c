/*
** fclose.c - fclose and freopen: its fd's buffer (hyfile.h) written out and freed, then the fd closed.  (They
** take the place of cc65's.)
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "hyfile.h"

/* f's fd's buffer: what's waiting written, the buffer freed.  0, or -1 */
static int __fastcall__ done (register FILE* f)
{
    register struct hy_fbuf* b = FBUF (f);
    int r = b->mode == FB_WRITE ? _hy_flush (f) : 0;

    if (b->buf && !(b->flags & FB_ONE)) {
        free (b->buf);
    }
    memset (b, 0, sizeof *b);
    return r;
}

int __fastcall__ fclose (register FILE* f)
{
    int r;

    if ((f->f_flags & _FOPEN) == 0) {
        return _directerrno (EINVAL);
    }
    r = done (f);
    f->f_flags = _FCLOSED;
    return close (f->f_fd) < 0 || r ? EOF : 0;
}

FILE* __fastcall__ freopen (const char* name, const char* mode, register FILE* f)
{
    if ((f->f_flags & _FOPEN) == 0) {
        _seterrno (EINVAL);
        return 0;
    }
    done (f);
    if (close (f->f_fd) < 0) {
        return 0;
    }
    return _fopen (name, mode, f);
}
