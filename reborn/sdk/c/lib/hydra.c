/*
** hydra.c - hydra.h's calls of the Hydra's own (on hy_call): the tick, sleeping, tasks and exit statuses,
** semaphores, the namespace, RAM banks, error texts.  A failed call returns -1 (errno and _oserror set: hy_call).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hydra.h>

extern const char* _hy_exitmsg;                         /* crt0.s: the message _exit gives EXITS */

static struct hy_regs r;

/* ---- The clock, the tick */

unsigned long hy_time (void)
{
    hy_call (HY_TIME, &r);
    return (unsigned long) r.r[0] | ((unsigned long) r.r[1] << 16);
}

unsigned hy_ticks (void)
{
    hy_call (HY_TICKS, &r);
    return r.a | (r.x << 8);
}

int __fastcall__ hy_sleep_ticks (unsigned ticks)
{
    r.a = ticks;
    r.x = ticks >> 8;
    return hy_call (HY_SLEEP, &r) ? -1 : 0;
}

void hy_yield (void)
{
    hy_call (HY_YIELD, &r);
}

/* ---- Tasks */

unsigned char hy_task (void)
{
    hy_call (HY_GETPID, &r);
    return r.a;
}

int __fastcall__ hy_spawn (const char* path, char* const* argv, unsigned char flags)
{
    static char args[HY_ARGS_MAX];
    unsigned n = 0, k;

    if (argv && *argv) {                                /* Its arguments: a list (argv[0], the name, left out) */
        while (*++argv) {
            k = strlen (*argv) + 1;
            if (n + k + 1 > HY_ARGS_MAX) {
                return _directerrno (EINVAL);
            }
            memcpy (args + n, *argv, k);
            n += k;
        }
    }
    args[n] = 0;
    fflush (0);                                         /* (What's waiting in stdio's buffers: out first) */
    r.r[0] = (unsigned) path;
    r.r[1] = (unsigned) args;
    r.a = flags & ~HY_SPAWN_FDMAP;                      /* (Its fds: this one's 0, 1 and 2) */
    return hy_call (HY_SPAWN, &r) ? -1 : r.a;
}

int __fastcall__ hy_wait (int task, char* msg)
{
    r.a = task < 0 ? 0xFF : task;
    r.r[0] = (unsigned) msg;
    return hy_call (HY_WAIT, &r) ? -1 : r.x;
}

void __fastcall__ hy_exits (const char* msg)
{
    if (msg == 0 || *msg == 0) {
        exit (0);
    }
    _hy_exitmsg = msg;
    exit (1);
}

int __fastcall__ hy_note (int task, unsigned char note)
{
    r.a = task;
    r.x = note;
    return hy_call (HY_NOTE, &r) ? -1 : 0;
}

unsigned char hy_parent (void)
{
    hy_call (HY_GETPPID, &r);
    return r.a;
}

/* ---- Semaphores */

int __fastcall__ hy_sem_new (unsigned char count, unsigned char flags)
{
    r.a = count;
    r.x = flags;
    return hy_call (HY_SEM_NEW, &r) ? -1 : r.a;
}

static int __fastcall__ sem_call (unsigned call, unsigned char sem)
{
    r.a = sem;
    return hy_call (call, &r) ? -1 : 0;
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

/* ---- The namespace */

int __fastcall__ hy_bind (const char* new, const char* old, unsigned char flags)
{
    r.r[0] = (unsigned) new;
    r.r[1] = (unsigned) old;
    r.a = flags;
    return hy_call (HY_BIND, &r) ? -1 : 0;
}

int __fastcall__ hy_mount (char dev, const char* spec, const char* old, unsigned char flags)
{
    r.x = dev;
    r.r[0] = (unsigned) spec;
    r.r[1] = (unsigned) old;
    r.a = flags;
    return hy_call (HY_MOUNT, &r) ? -1 : 0;
}

int __fastcall__ hy_unmount (const char* new, const char* old)
{
    r.r[0] = (unsigned) new;
    r.r[1] = (unsigned) old;
    return hy_call (HY_UNMOUNT, &r) ? -1 : 0;
}

/* ---- RAM banks */

unsigned char hy_banks (void)
{
    hy_call (HY_BANKS, &r);
    return r.a;
}

int __fastcall__ hy_banks_alloc (unsigned char n)
{
    r.a = n;
    return hy_call (HY_BANKS_ALLOC, &r) ? -1 : r.a;
}

int __fastcall__ hy_banks_free (unsigned char first, unsigned char n)
{
    r.a = first;
    r.x = n;
    return hy_call (HY_BANKS_FREE, &r) ? -1 : 0;
}

/* ---- Errors */

const char* __fastcall__ hy_errstr (unsigned char code)
{
    return _stroserror (code);
}
