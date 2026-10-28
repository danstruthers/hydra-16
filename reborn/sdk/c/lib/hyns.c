/*
** hyns.c - hydra.h's calls of the namespace and files (on hy_call): bind, mount, unmount, an fd's name (apart from
** hydra.c's: a program takes in only what it calls)
*/

#include <hydra.h>

extern struct hy_regs _hy_r;                           /* (hyregs.c's: the calls' registers, shared) */

/* ---- The namespace */

int __fastcall__ hy_bind (const char* new, const char* old, unsigned char flags)
{
    _hy_r.r[0] = (unsigned) new;
    _hy_r.r[1] = (unsigned) old;
    _hy_r.a = flags;
    return hy_call (HY_BIND, &_hy_r) ? -1 : 0;
}

int __fastcall__ hy_mount (char dev, const char* spec, const char* old, unsigned char flags)
{
    _hy_r.x = dev;
    _hy_r.r[0] = (unsigned) spec;
    _hy_r.r[1] = (unsigned) old;
    _hy_r.a = flags;
    return hy_call (HY_MOUNT, &_hy_r) ? -1 : 0;
}

int __fastcall__ hy_unmount (const char* new, const char* old)
{
    _hy_r.r[0] = (unsigned) new;
    _hy_r.r[1] = (unsigned) old;
    return hy_call (HY_UNMOUNT, &_hy_r) ? -1 : 0;
}

/* ---- Files */

int __fastcall__ hy_fd2path (int fd, char* buf)
{
    _hy_r.a = fd;
    _hy_r.r[0] = (unsigned) buf;
    return hy_call (HY_FD2PATH, &_hy_r) ? -1 : 0;
}
