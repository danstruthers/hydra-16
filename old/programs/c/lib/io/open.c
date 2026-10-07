/*
** open.c - open() on the Hydra's IO layer: IO_OPEN, and IO_CREATE for O_CREAT when there's no such file yet.
** O_RDONLY, O_WRONLY and O_RDWR are the IO layer's IO_MODE_READ, _WRITE and _RDWR; O_TRUNC empties the file
** as it's opened (IO_MODE_TRUNC); O_APPEND starts at its end.  A name is relative to the task's current
** directory unless it starts with /.
*/

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

#define IO_MODE_TRUNC       0x08
#define ERR_IO_NOT_FOUND    0x70

int __fastcall__ _hy_open (const char* name, unsigned char mode);
int __fastcall__ _hy_create (const char* name, int mode, unsigned char bits);

int __cdecl__ open (const char* name, int flags, ...)
{
    unsigned char mode = flags & O_RDWR;
    int fd;

    if ((flags & O_TRUNC) && (mode & O_WRONLY)) {
        mode |= IO_MODE_TRUNC;
    }
    fd = _hy_open (name, mode);
    if (fd < 0) {
        if (_oserror != ERR_IO_NOT_FOUND || !(flags & O_CREAT)) {
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
