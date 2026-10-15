/*
** hyspawn.c - hydra.h's calls that start programs and wait for them (on hy_call): SPAWN, EXEC, WAIT; apart from
** hydra.c's, for they write stdio's buffers out first (so a program that only, say, takes semaphores doesn't take in
** stdio).  A failed call returns -1 (errno and _oserror set: hy_call).
*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <hydra.h>

static struct hy_regs r;

/* SPAWN's and EXEC's path and arguments into r (argv: a list, argv[0], the name, left out), stdio's buffers out
** first.  OUT: -1 if they're too long (errno EINVAL), else 0 */
static int __fastcall__ hy_args (const char* path, char* const* argv)
{
    static char args[HY_ARGS_MAX];
    unsigned n = 0, k;

    if (argv && *argv) {
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
    return 0;
}

int __fastcall__ hy_spawn (const char* path, char* const* argv, unsigned char flags)
{
    if (hy_args (path, argv)) {
        return -1;
    }
    r.a = flags & ~HY_SPAWN_FDMAP;                      /* (Its fds: this one's 0, 1 and 2) */
    return hy_call (HY_SPAWN, &r) ? -1 : r.a;
}

int __fastcall__ hy_exec (const char* path, char* const* argv)
{
    if (hy_args (path, argv)) {
        return -1;
    }
    hy_call (HY_EXEC, &r);                              /* (Back only if it failed) */
    return -1;
}

int __fastcall__ hy_wait (int task, char* msg)
{
    r.a = task < 0 ? 0xFF : task;
    r.r[0] = (unsigned) msg;
    return hy_call (HY_WAIT, &r) ? -1 : r.x;
}
