/*
** hyseg.c - hydra.h's calls of shared segments (on hy_call): made, detached, mapped (apart from hydra.c's: a
** program takes in only what it calls)
*/

#include <hydra.h>

extern struct hy_regs _hy_r;                           /* (hyregs.c's: the calls' registers, shared) */

int __fastcall__ hy_seg_create (unsigned char banks)
{
    _hy_r.a = banks;
    return hy_call (HY_SEG_CREATE, &_hy_r) ? -1 : _hy_r.a;
}

int __fastcall__ hy_seg_detach (unsigned char seg)
{
    _hy_r.a = seg;
    return hy_call (HY_SEG_DETACH, &_hy_r) ? -1 : 0;
}

unsigned char* __fastcall__ hy_seg_map (unsigned char seg, unsigned char bank)
{
    _hy_r.a = seg;
    _hy_r.x = bank;
    if (hy_call (HY_SEG_MAP, &_hy_r)) {
        return 0;
    }
    *(volatile unsigned char*) 0xFFF1 = _hy_r.a;            /* U, the shared macro-page (the kernel keeps it a task's) */
    hy_bank (_hy_r.x);                                      /* Then the bank */
    return HY_BANK_WINDOW;
}
