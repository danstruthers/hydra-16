/*
** fmisc.c - fflush (a FILE's buffer written out: hyfile.h; NULL: every FILE's), and the rest of cc65's fmisc:
** clearerr, feof, ferror and fileno.  (It takes the place of cc65's, whose fflush does nothing.)
*/

#include <errno.h>
#include "hyfile.h"

int __fastcall__ fflush (register FILE* f)
{
    if (f == 0) {
        _hy_flushall ();
        return 0;
    }
    if ((f->f_flags & _FOPEN) == 0) {
        return _directerrno (EBADF);
    }
    return _hy_flush (f);
}

void __fastcall__ clearerr (register FILE* f)
{
    if (f->f_flags & _FOPEN) {
        f->f_flags &= ~(_FEOF | _FERROR);
    }
}

int __fastcall__ feof (register FILE* f)
{
    return (f->f_flags & _FOPEN) ? f->f_flags & _FEOF : 0;
}

int __fastcall__ ferror (register FILE* f)
{
    return (f->f_flags & _FOPEN) ? f->f_flags & _FERROR : 0;
}

int __fastcall__ fileno (register FILE* f)
{
    if ((f->f_flags & _FOPEN) == 0) {
        return _directerrno (EBADF);
    }
    return f->f_fd;
}
