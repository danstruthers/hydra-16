/*
** hytime.c - hydra.h's calls of the clock (on hy_call): the time, sleeping, yielding (apart from hydra.c's: a
** program takes in only what it calls)
*/

#include <hydra.h>

extern struct hy_regs _hy_r;                           /* (hyregs.c's: the calls' registers, shared) */

unsigned long hy_time (void)
{
    hy_call (HY_TIME, &_hy_r);
    return (unsigned long) _hy_r.r[0] | ((unsigned long) _hy_r.r[1] << 16);
}

int __fastcall__ hy_sleep_ticks (unsigned ticks)
{
    _hy_r.a = ticks;
    _hy_r.x = ticks >> 8;
    return hy_call (HY_SLEEP, &_hy_r) ? -1 : 0;
}

int __fastcall__ hy_sleep_until (unsigned tick)
{
    _hy_r.a = tick;
    _hy_r.x = tick >> 8;
    return hy_call (HY_SLEEP_UNTIL, &_hy_r) ? -1 : 0;
}

void hy_yield (void)
{
    hy_call (HY_YIELD, &_hy_r);
}
