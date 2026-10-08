/*
** window.c - the window's chrome (W4): its title (/dev/label), its status line and any line of its wctl (/dev/wctl),
** each one write of a line (a wctl line's 63 bytes at most: the console's).  0, or -1 (errno).  And a window made
** (W5): wctl's new, its number the fid's next read (as Plan 9's clone files answer).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <hydra.h>

int __fastcall__ _hy_open (const char* name, unsigned char mode);

/* The text s after prefix p, and a line end, to the file at path */
static int put (const char* path, const char* p, const char* s)
{
    static char line[64];
    unsigned char n = strlen (p);
    int fd, r;

    strcpy (line, p);
    strncpy (line + n, s, sizeof line - 2 - n);
    line[sizeof line - 2] = 0;
    n = strlen (line);
    line[n++] = '\n';
    if ((fd = _hy_open (path, HY_O_WRITE)) < 0) {
        return -1;
    }
    r = write (fd, line, n);
    close (fd);
    return r < 0 ? -1 : 0;
}

int __fastcall__ hy_wlabel (const char* s)
{
    return put ("/dev/label", "", s);
}

int __fastcall__ hy_wstatus (const char* s)
{
    return put ("/dev/wctl", "status ", s);
}

int __fastcall__ hy_wctl (const char* s)
{
    return put ("/dev/wctl", "", s);
}

int __fastcall__ hy_wnew (unsigned char flags)
{
    static char n[8];
    int fd, r, w = -1;

    if ((fd = _hy_open ("/dev/wctl", HY_O_RDWR)) < 0) {
        return -1;
    }
    if (write (fd, "new group", flags & HY_WGROUP ? 9 : 3) >= 0 && lseek (fd, 0, SEEK_SET) >= 0 &&
        (r = read (fd, n, sizeof n - 1)) > 0) {
        n[r] = 0;
        w = atoi (n);
    }
    close (fd);
    return w;
}
