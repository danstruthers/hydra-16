/*
** isatty.c - isatty(): is fd a console (a window's cons, the console driver's: #c, #cN)?  From its stat record:
** its device and its name.
*/

#include <string.h>
#include <hydra.h>

int __fastcall__ _hy_fstat (int fd, unsigned char* rec);

int __fastcall__ isatty (int fd)
{
    unsigned char rec[HY_SR_SIZE];

    if (_hy_fstat (fd, rec) < 0) {
        return 0;
    }
    return rec[HY_SR_DEV] == 'c' && strcmp ((char*) rec + HY_SR_NAME, "cons") == 0;
}
