/*
** hydra.c - hydra.h's calls of the Hydra's own (on hy_call) that nearly every program makes: the tick, its task,
** semaphores, RAM banks taken, a shared segment attached.  The rest are in modules of their own (hytime.c, hytask.c,
** hyns.c, hybanks.c, hyseg.c, hyerrstr.c; spawning, exec and waiting hyspawn.c's), so a program takes in only those
** it calls (the game's tasks' room, 2026-10-27).  A failed call returns -1 (errno and _oserror set: hy_call).
*/

#include <hydra.h>

extern struct hy_regs _hy_r;                           /* (hyregs.c's: the calls' registers, shared) */

/* ---- The tick */

unsigned hy_ticks (void)
{
    hy_call (HY_TICKS, &_hy_r);
    return _hy_r.a | (_hy_r.x << 8);
}

/* ---- Tasks */

unsigned char hy_task (void)
{
    hy_call (HY_GETPID, &_hy_r);
    return _hy_r.a;
}

/* ---- Semaphores */

int __fastcall__ hy_sem_new (unsigned char count, unsigned char flags)
{
    _hy_r.a = count;
    _hy_r.x = flags;
    return hy_call (HY_SEM_NEW, &_hy_r) ? -1 : _hy_r.a;
}

static int __fastcall__ sem_call (unsigned call, unsigned char sem)
{
    _hy_r.a = sem;
    return hy_call (call, &_hy_r) ? -1 : 0;
}

int __fastcall__ hy_sem_acquire (unsigned char sem)
{
    return sem_call (HY_SEM_ACQUIRE, sem);
}

int __fastcall__ hy_sem_try (unsigned char sem)
{
    return sem_call (HY_SEM_TRY, sem);
}

int __fastcall__ hy_sem_release (unsigned char sem)
{
    return sem_call (HY_SEM_RELEASE, sem);
}

int __fastcall__ hy_sem_free (unsigned char sem)
{
    return sem_call (HY_SEM_FREE, sem);
}

/* ---- RAM banks, a shared segment */

int __fastcall__ hy_banks_alloc (unsigned char n)
{
    _hy_r.a = n;
    return hy_call (HY_BANKS_ALLOC, &_hy_r) ? -1 : _hy_r.a;
}

int __fastcall__ hy_seg_attach (unsigned char seg)
{
    _hy_r.a = seg;
    return hy_call (HY_SEG_ATTACH, &_hy_r) ? -1 : 0;
}
