/*
** stat.c - stat() and fstat(): a file's stat record (IO_STAT: the server's; HydraFS gives the size, the mode bits
** and the time of the last change) as a struct stat.  st_mode: S_IREAD, S_IWRITE (not read-only), S_IFDIR (a
** directory: hydra.h); st_size; st_mtim (and st_ctim, st_atim: the same); st_ino (its id on the card), st_dev
** (the card).  Devices (/dev/...) give zeros.
*/

#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>

#define IO_ST_MODE      32
#define IO_ST_CARD      33
#define IO_ST_QID       36
#define IO_ST_SIZE      40
#define IO_ST_STAMP     44
#define HFS_M_DIR       0x80
#define HFS_M_RO        0x01
#define UNIX_2000       946684800UL

int __fastcall__ _hy_statrec (int fd, unsigned char* rec);

/* A stat record (IO_STAT_SIZE bytes, from IO_STAT or a directory's read) as a struct stat */
void __fastcall__ _hy_fromrec (const unsigned char* rec, struct stat* st)
{
    unsigned char mode = rec[IO_ST_MODE];
    unsigned long stamp = *(const unsigned long*) (rec + IO_ST_STAMP);

    memset (st, 0, sizeof *st);
    st->st_dev = rec[IO_ST_CARD];
    st->st_ino = *(const unsigned*) (rec + IO_ST_QID);
    st->st_mode = S_IREAD | ((mode & HFS_M_RO) ? 0 : S_IWRITE) | ((mode & HFS_M_DIR) ? S_IFDIR : 0);
    st->st_nlink = 1;
    st->st_size = *(const long*) (rec + IO_ST_SIZE);
    if (stamp) {
        st->st_mtim.tv_sec = stamp + UNIX_2000;
    }
    st->st_ctim = st->st_mtim;
    st->st_atim = st->st_mtim;
}

int __fastcall__ fstat (int fd, struct stat* st)
{
    unsigned char rec[HY_STAT_SIZE];

    if (_hy_statrec (fd, rec) < 0) {
        return -1;
    }
    _hy_fromrec (rec, st);
    return 0;
}

int __fastcall__ stat (const char* name, struct stat* st)
{
    int fd = open (name, O_RDONLY);
    int r;

    if (fd < 0) {
        return -1;
    }
    r = fstat (fd, st);
    close (fd);
    return r;
}
