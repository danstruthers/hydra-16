/*
** write.c - write(): the IO layer's write (fileio.s: _hy_write), and output to the console as a terminal wants it
** (a tty's ONLCR): C's lines end with LF, and a terminal moves to the next line's start on CR LF, so an LF written
** to the console goes out as CR LF.  Files and pipes get the bytes as they are.
*/

#include <unistd.h>

#define IO_FD_TABLE     0x7DA0          /* The task's fds (8 bytes each) ... */
#define IO_FD_FLAGS     3               /*   their flags ... */
#define IO_FDF_CONS     0x01            /*   the console (/dev/cons) */
#define IO_MAX_FDS      12

int __fastcall__ _hy_write (int fd, const void* buf, unsigned count);

int __fastcall__ write (int fd, const void* buf, unsigned count)
{
    static char out[64];
    const char* p = buf;
    unsigned i, n = 0;

    if ((unsigned) fd >= IO_MAX_FDS || !(((unsigned char*) IO_FD_TABLE)[fd * 8 + IO_FD_FLAGS] & IO_FDF_CONS)) {
        return _hy_write (fd, buf, count);
    }
    for (i = 0; i < count; ++i) {                       /* (Gathered, so a line is one request) */
        if (p[i] == '\n') {
            out[n++] = '\r';
        }
        out[n++] = p[i];
        if (n >= sizeof out - 1) {
            if (_hy_write (fd, out, n) < 0) {
                return -1;
            }
            n = 0;
        }
    }
    if (n && _hy_write (fd, out, n) < 0) {
        return -1;
    }
    return count;
}
