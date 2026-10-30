/*
** hyfore.c - hy_taskfore: a task (this one, or one it started) put in the foreground or taken out (TASKFORE: the
** scheduler picks it first, sched.s).  A file of its own, so only a program that asks for it takes it in.
*/

#include <hydra.h>

int __fastcall__ hy_taskfore (unsigned char task, unsigned char on)
{
    static struct hy_regs r;

    r.a = task;
    r.x = on;
    return hy_call (HY_TASKFORE, &r) ? -1 : 0;
}
