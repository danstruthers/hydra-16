/*
** stat.c - stat() and fstat(): a file's stat record (STAT, FSTAT: its server's; HydraFS gives the length, the mode
** and the time of the last change) as a struct stat.  st_mode: S_IREAD, S_IWRITE (it has w bits), S_IFDIR (a
** directory: hydra.h); st_size; st_mtim (and st_ctim, st_atim: the same); st_ino (its qid's path, the low 16 bits),
** st_dev (its device letter).  _hy_fromrec makes one from any stat record (dirent.c's hy_dirstat).
*/

#include <sys/stat.h>
#include <string.h>
#include <hydra.h>

#define UNIX_2000       946684800UL     /* 2000-01-01 00:00:00, in seconds since 1970 */

int __fastcall__ _hy_fstat (int fd, unsigned char* rec);
int __fastcall__ _hy_stat (const char* name, unsigned char* rec);

/* A stat record (HY_SR_SIZE bytes) as a struct stat */
void __fastcall__ _hy_fromrec (const unsigned char* rec, struct stat* st)
{
    unsigned long stamp = *(const unsigned long*) (rec + HY_SR_MTIME);

    memset (st, 0, sizeof *st);
    st->st_dev = rec[HY_SR_DEV];
    st->st_ino = *(const unsigned*) (rec + HY_SR_QPATH);
    st->st_mode = S_IREAD | ((rec[HY_SR_MODE] & 0x92) ? S_IWRITE : 0) |
        ((rec[HY_SR_MODE + 1] & HY_DM_DIR) ? S_IFDIR : 0);
    st->st_nlink = 1;
    st->st_size = *(const long*) (rec + HY_SR_LENGTH);
    if (stamp) {
        st->st_mtim.tv_sec = stamp + UNIX_2000;
    }
    st->st_ctim = st->st_mtim;
    st->st_atim = st->st_mtim;
}

int __fastcall__ fstat (int fd, struct stat* st)
{
    unsigned char rec[HY_SR_SIZE];

    if (_hy_fstat (fd, rec) < 0) {
        return -1;
    }
    _hy_fromrec (rec, st);
    return 0;
}

int __fastcall__ stat (const char* name, struct stat* st)
{
    unsigned char rec[HY_SR_SIZE];

    if (_hy_stat (name, rec) < 0) {
        return -1;
    }
    _hy_fromrec (rec, st);
    return 0;
}
