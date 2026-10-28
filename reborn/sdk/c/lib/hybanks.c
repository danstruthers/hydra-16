/*
** hybanks.c - hydra.h's calls of RAM banks (on hy_call): how many free, banks given back (apart from hydra.c's: a
** program takes in only what it calls)
*/

#include <hydra.h>

extern struct hy_regs _hy_r;                           /* (hyregs.c's: the calls' registers, shared) */

unsigned char hy_banks (void)
{
    hy_call (HY_BANKS, &_hy_r);
    return _hy_r.a;
}

int __fastcall__ hy_banks_free (unsigned char first, unsigned char n)
{
    _hy_r.a = first;
    _hy_r.x = n;
    return hy_call (HY_BANKS_FREE, &_hy_r) ? -1 : 0;
}
