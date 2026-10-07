/*
** open.c - open() on the Hydra's OPEN, and CREATE for O_CREAT when there's no such file yet.  O_RDONLY, O_WRONLY
** and O_RDWR are the kernel's O_READ, O_WRITE and O_RDWR; O_TRUNC empties the file as it's opened; O_APPEND starts
** at its end; O_EXCL with O_CREAT fails if it's there.  A name is relative to the task's current directory unless
** it starts with / (or is a # name: a device's own).
*/

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <hydra.h>

int __fastcall__ _hy_open (const char* name, unsigned char mode);
int __fastcall__ _hy_create (const char* name, int mode, unsigned char bits);

int __cdecl__ open (const char* name, int flags, ...)
{
    unsigned char mode = (flags & O_RDWR) - 1;          /* (cc65's 1, 2, 3: the kernel's 0, 1, 2) */
    int fd;

    if ((flags & O_RDWR) == 0) {
        return _directerrno (EINVAL);
    }
    if ((flags & O_TRUNC) && (flags & O_WRONLY)) {
        mode |= HY_O_TRUNC;
    }
    fd = _hy_open (name, mode);
    if (fd < 0) {
        if (_oserror != HY_E_NOENT || !(flags & O_CREAT)) {
            return -1;
        }
        fd = _hy_create (name, mode, 0);
        if (fd < 0) {
            return -1;
        }
    } else if ((flags & (O_CREAT | O_EXCL)) == (O_CREAT | O_EXCL)) {
        close (fd);
        return _directerrno (EEXIST);
    }
    if (flags & O_APPEND) {
        lseek (fd, 0, SEEK_END);
    }
    return fd;
}
