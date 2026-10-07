/*
** isatty.c - isatty(): is fd the console (/dev/cons)?  From the task's fd table (IO_FD_TABLE, in the task system
** page: an fd's device, $FF when it's closed, and its flags, IO_FDF_CONS for the console).
*/

#include <hydra.h>

#define IO_FD_TABLE     0x7DA0
#define IO_MAX_FDS      12
#define IO_FD_CLOSED    0xFF
#define IO_FDF_CONS     0x01

int __fastcall__ isatty (int fd)
{
    unsigned char* e = (unsigned char*) IO_FD_TABLE + fd * 8;

    if ((unsigned) fd >= IO_MAX_FDS || e[0] == IO_FD_CLOSED) {
        return 0;
    }
    return e[3] & IO_FDF_CONS;
}
