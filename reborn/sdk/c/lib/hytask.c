/*
** hytask.c - hydra.h's calls of tasks (on hy_call): an exit status given, a note sent, the parent (apart from
** hydra.c's: a program takes in only what it calls)
*/

#include <stdlib.h>
#include <hydra.h>

extern struct hy_regs _hy_r;                           /* (hyregs.c's: the calls' registers, shared) */

extern const char* _hy_exitmsg;                         /* crt0.s: the message _exit gives EXITS */

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
    _hy_r.a = task;
    _hy_r.x = note;
    return hy_call (HY_NOTE, &_hy_r) ? -1 : 0;
}

unsigned char hy_parent (void)
{
    hy_call (HY_GETPPID, &_hy_r);
    return _hy_r.a;
}
