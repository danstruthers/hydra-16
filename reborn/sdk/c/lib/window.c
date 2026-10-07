/*
** window.c - the window's chrome (W4): its title (/dev/label), its status line and any line of its wctl (/dev/wctl),
** each one write of a line (a wctl line's 63 bytes at most: the console's).  0, or -1 (errno).
*/

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
