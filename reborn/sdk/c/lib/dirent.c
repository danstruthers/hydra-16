/*
** dirent.c - a directory's entries (dirent.h: opendir, readdir, closedir, rewinddir, telldir, seekdir), as Plan 9's
** dirread: a directory read gives a stat record (HY_SR_SIZE bytes) an entry, no text to take apart.  cc65's struct
** dirent for this target is d_name alone, and readdir gives the whole name there (31 characters at most);
** hy_dirstat gives the rest of the entry (its length, mode, time) as a struct stat.
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

#define NAME_MAX        31

void __fastcall__ _hy_fromrec (const unsigned char* rec, struct stat* st);

struct DIR {
    int             fd;
    unsigned char   rec[HY_SR_SIZE];    /* The last entry's stat record */
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
    dir->fd = open (name, O_RDONLY);
    if (dir->fd < 0) {
        free (dir);
        return 0;
    }
    return dir;
}

struct dirent* __fastcall__ readdir (DIR* dir)
{
    if (read (dir->fd, dir->rec, HY_SR_SIZE) != HY_SR_SIZE) {
        return 0;                       /* (The end, or an error: errno says) */
    }
    memcpy (dir->ent.d_name, dir->rec + HY_SR_NAME, NAME_MAX);
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
