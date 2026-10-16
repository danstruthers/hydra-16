/*
** hysemw.c - hy_sem_wait: a semaphore taken, waiting for it a while at most (SEM_WAIT: the ticks .X).  A file of its
** own, so only a program that waits so takes it in.
*/

#include <hydra.h>

int __fastcall__ hy_sem_wait (unsigned char sem, unsigned char ticks)
{
    static struct hy_regs r;

    r.a = sem;
    r.x = ticks;
    return hy_call (HY_SEM_WAIT, &r) ? -1 : 0;
}
