/*
** lseek.c - lseek() on the Hydra's IO layer: IO_SEEK sets an fd's offset.  SEEK_CUR takes the offset now from
** the task's fd table (IO_FD_TABLE, in the task system page: the IO layer keeps each fd's offset there), and
** SEEK_END the file's size from its stat record (IO_STAT).
*/

#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#define IO_FD_TABLE     0x7DA0          /* 8 bytes an fd ... */
#define IO_FD_OFS       4               /*   its offset (4 bytes) */
#define IO_MAX_FDS      12

int __fastcall__ _hy_seek (int fd, long offset);
long __fastcall__ _hy_size (int fd);

off_t __fastcall__ lseek (int fd, off_t offset, int whence)
{
    long size;

    if ((unsigned) fd >= IO_MAX_FDS) {
        _directerrno (EBADF);
        return -1;
    }
    switch (whence) {
    case SEEK_SET:
        break;
    case SEEK_CUR:
        offset += *(long*) (IO_FD_TABLE + fd * 8 + IO_FD_OFS);
        break;
    case SEEK_END:
        size = _hy_size (fd);
        if (size < 0) {
            return -1;
        }
        offset += size;
        break;
    default:
        _directerrno (EINVAL);
        return -1;
    }
    if (offset < 0) {
        _directerrno (EINVAL);
        return -1;
    }
    if (_hy_seek (fd, offset) < 0) {
        return -1;
    }
    return offset;
}
