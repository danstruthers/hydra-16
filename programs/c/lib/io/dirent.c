/*
** dirent.c - a directory's entries (dirent.h: opendir, readdir, closedir, rewinddir, telldir, seekdir), as Plan 9's
** dirread: the directory opened with IO_MODE_STAT, whose reads give a stat record (IO_STAT_SIZE bytes) an entry.
** cc65's struct dirent for this target is only d_name, and a readdir's entry has the whole name there (up to 31
** characters); hy_dirstat gives the rest (its size, mode, time) as a struct stat.  HydraFS directories.
*/

#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <hydra.h>

#define IO_MODE_READ    0x01
#define IO_MODE_STAT    0x04
#define NAME_MAX        31

int __fastcall__ _hy_open (const char* name, unsigned char mode);
void __fastcall__ _hy_fromrec (const unsigned char* rec, struct stat* st);

struct DIR {
    int             fd;
    unsigned char   rec[HY_STAT_SIZE];  /* The last entry's stat record */
    struct dirent   ent;                /* The last entry: d_name ... */
    char            name[NAME_MAX];     /*   with room for the rest of it */
};

DIR* __fastcall__ opendir (const char* name)
{
    DIR* dir = malloc (sizeof (DIR));

    if (dir == 0) {
        _directerrno (ENOMEM);
        return 0;
    }
    dir->fd = _hy_open (name, IO_MODE_READ | IO_MODE_STAT);
    if (dir->fd < 0) {
        free (dir);
        return 0;
    }
    return dir;
}

struct dirent* __fastcall__ readdir (DIR* dir)
{
    if (read (dir->fd, dir->rec, HY_STAT_SIZE) != HY_STAT_SIZE) {
        return 0;                       /* (The end, or an error: errno says) */
    }
    memcpy (dir->ent.d_name, dir->rec, NAME_MAX);
    dir->ent.d_name[NAME_MAX] = 0;      /* (In name[]: there's room) */
    return &dir->ent;
}

int __fastcall__ closedir (DIR* dir)
{
    int r = close (dir->fd);

    free (dir);
    return r;
}

long __fastcall__ telldir (DIR* dir)
{
    return lseek (dir->fd, 0, SEEK_CUR);
}

void __fastcall__ seekdir (DIR* dir, long offs)
{
    lseek (dir->fd, offs, SEEK_SET);
}

void __fastcall__ rewinddir (DIR* dir)
{
    lseek (dir->fd, 0, SEEK_SET);
}

int __fastcall__ hy_dirstat (DIR* dir, struct stat* st)
{
    _hy_fromrec (dir->rec, st);
    return 0;
}
