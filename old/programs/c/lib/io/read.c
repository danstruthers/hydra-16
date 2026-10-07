/*
** read.c - read(): the IO layer's read (fileio.s: _hy_read), and stdin from the console as a terminal gives it to a
** C program on Unix (a tty's ICRNL): a terminal's Enter is CR, and C's lines end with LF, so fd 0's CRs, when fd 0 is
** the console, are LFs.  The console echoed the CR; an LF follows it, so the next line starts below.  (Not while
** conio has the console raw: its keys are its own.  Other fds, files and pipes: as they are.)
*/

#include <unistd.h>

#define IO_FD_FLAGS     0x7DA3          /* The task's fd table (IO_FD_TABLE, 8 bytes an fd): its flags ... */
#define IO_FDF_CONS     0x01            /*   the console (/dev/cons) */

int __fastcall__ _hy_read (int fd, void* buf, unsigned count);
int __fastcall__ _hy_write (int fd, const void* buf, unsigned count);

unsigned char _hy_rawcons;              /* conio.c: the console's raw (no echo) */

int __fastcall__ read (int fd, void* buf, unsigned count)
{
    int n = _hy_read (fd, buf, count);
    char* p = buf;
    int i;

    if (n > 0 && fd == 0 && (*(unsigned char*) IO_FD_FLAGS & IO_FDF_CONS)) {
        for (i = 0; i < n; ++i) {
            if (p[i] == '\r') {
                p[i] = '\n';
                if (!_hy_rawcons) {
                    _hy_write (0, "\n", 1);
                }
            }
        }
    }
    return n;
}
