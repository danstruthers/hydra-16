/*
** vera.c - vera.h: the Vera X through its driver's files (vid: /dev/vid's ctl, draw, vram, pal, sprites, frame,
** mouse), and its registers for a program that's claimed it.  ctl, draw, frame and vram are opened the first time
** they're wanted and stay open (a claim lasts while ctl is open: it ends with the program, or vera_release).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>
#include <vera.h>

int __fastcall__ _hy_open (const char* name, unsigned char mode);

static int ctlfd = -1, drawfd = -1, framefd = -1, vramfd = -1, mousefd = -1;
static char line[64];
static unsigned char k;
static char rec[49];

/* A file of /dev/vid's, open (*fd: -1, opened now).  OUT: its fd, or -1 */
static int dev (int* fd, const char* name, unsigned char mode)
{
    if (*fd < 0) {
        *fd = _hy_open (name, mode);
    }
    return *fd;
}

/* line, then a space and n, at k */
static void num (int n)
{
    line[k++] = ' ';
    itoa (n, line + k, 10);
    k = strlen (line);
}

/* line's word and the numbers (count of them) to /dev/vid/draw */
static int draw (const char* word, unsigned char count, int a, int b, int c, int d)
{
    if (dev (&drawfd, "/dev/vid/draw", HY_O_WRITE) < 0) {
        return -1;
    }
    strcpy (line, word);
    k = strlen (line);
    if (count > 0) num (a);
    if (count > 1) num (b);
    if (count > 2) num (c);
    if (count > 3) num (d);
    return write (drawfd, line, k) < 0 ? -1 : 0;
}

int __fastcall__ vera_ctl (const char* command)
{
    if (dev (&ctlfd, "/dev/vid/ctl", HY_O_WRITE) < 0) {
        return -1;
    }
    return write (ctlfd, command, strlen (command)) < 0 ? -1 : 0;
}

int __fastcall__ vera_claim (unsigned char all)
{
    return vera_ctl (all ? "claim all" : "claim");
}

int vera_release (void)
{
    return vera_ctl ("release");
}

int vera_wait_frame (void)
{
    if (dev (&framefd, "/dev/vid/frame", HY_O_READ) < 0) {
        return -1;
    }
    return read (framefd, line, sizeof line) < 0 ? -1 : 0;
}

void __fastcall__ vpoke (unsigned char data, unsigned long addr)
{
    VERA.control = 0;
    VERA.address = (unsigned) addr;
    VERA.address_hi = (unsigned char) (addr >> 16) & 1;
    VERA.data0 = data;
}

unsigned char __fastcall__ vpeek (unsigned long addr)
{
    VERA.control = 0;
    VERA.address = (unsigned) addr;
    VERA.address_hi = (unsigned char) (addr >> 16) & 1;
    return VERA.data0;
}

/* /dev/vid/vram at addr.  OUT: its fd, or -1 */
static int vram (unsigned long addr)
{
    if (dev (&vramfd, "/dev/vid/vram", HY_O_RDWR) < 0 || lseek (vramfd, (long) addr, SEEK_SET) < 0) {
        return -1;
    }
    return vramfd;
}

int __fastcall__ vera_write (unsigned long addr, const void* buf, unsigned n)
{
    return vram (addr) < 0 || write (vramfd, buf, n) < 0 ? -1 : 0;
}

int __fastcall__ vera_read (unsigned long addr, void* buf, unsigned n)
{
    return vram (addr) < 0 || read (vramfd, buf, n) < 0 ? -1 : 0;
}

int vera_load (const char* path, unsigned long addr)
{
    static unsigned char buf[256];
    int fd, n;

    if ((fd = open (path, O_RDONLY)) < 0) {
        return -1;
    }
    while ((n = read (fd, buf, sizeof buf)) > 0) {
        if (vera_write (addr, buf, n) < 0) {
            n = -1;
            break;
        }
        addr += n;
    }
    close (fd);
    return n < 0 ? -1 : 0;
}

int __fastcall__ vid_bitmap (unsigned width, unsigned char depth)
{
    if (width == 0) {
        return vera_ctl ("bitmap off");
    }
    strcpy (line, "bitmap");
    k = 6;
    num (width);
    num (depth);
    return vera_ctl (line);
}

int __fastcall__ vid_pen (unsigned char colour)
{
    return draw ("pen", 1, colour, 0, 0, 0);
}

int __fastcall__ vid_plot (int x, int y)
{
    return draw ("plot", 2, x, y, 0, 0);
}

int __fastcall__ vid_line (int x0, int y0, int x1, int y1)
{
    return draw ("line", 4, x0, y0, x1, y1);
}

int __fastcall__ vid_box (int x0, int y0, int x1, int y1)
{
    return draw ("box", 4, x0, y0, x1, y1);
}

int __fastcall__ vid_bar (int x0, int y0, int x1, int y1)
{
    return draw ("bar", 4, x0, y0, x1, y1);
}

int __fastcall__ vid_circle (int x, int y, int r)
{
    return draw ("circle", 3, x, y, r, 0);
}

int __fastcall__ vid_disc (int x, int y, int r)
{
    return draw ("disc", 3, x, y, r, 0);
}

int vid_clear (void)
{
    return draw ("clear", 0, 0, 0, 0, 0);
}

/* text X Y STRING, 40 characters a command at most (a command's 63 bytes) */
int __fastcall__ vid_text (int x, int y, const char* s)
{
    unsigned char n;

    while (*s) {
        if (dev (&drawfd, "/dev/vid/draw", HY_O_WRITE) < 0) {
            return -1;
        }
        strcpy (line, "text");
        k = 4;
        num (x);
        num (y);
        line[k++] = ' ';
        for (n = 0; n < 40 && s[n]; ++n) {
            line[k++] = s[n];
        }
        if (write (drawfd, line, k) < 0) {
            return -1;
        }
        s += n;
        x += n * 8;
    }
    return 0;
}

/* n bytes into a file of /dev/vid's at offset */
static int put (const char* name, unsigned offset, const void* bytes, unsigned n)
{
    int fd, r;

    if ((fd = _hy_open (name, HY_O_WRITE)) < 0) {
        return -1;
    }
    r = lseek (fd, offset, SEEK_SET) < 0 || write (fd, bytes, n) < 0 ? -1 : 0;
    close (fd);
    return r;
}

int vid_palette (unsigned char index, unsigned rgb)
{
    unsigned char e[2];

    e[0] = (unsigned char) rgb;                     /* $GB */
    e[1] = (unsigned char) (rgb >> 8) & 15;         /* $0R */
    return put ("/dev/vid/pal", index * 2, e, 2);
}

int vid_sprite (unsigned char n, const unsigned char* attr)
{
    return put ("/dev/vid/sprites", n * 8, attr, 8);
}

int vid_sprite_at (unsigned char n, int x, int y)
{
    int xy[2];

    xy[0] = x;
    xy[1] = y;
    return put ("/dev/vid/sprites", n * 8 + 2, xy, 4);
}

int __fastcall__ vid_sprite_off (unsigned char n)
{
    static const unsigned char off = 0;

    return put ("/dev/vid/sprites", n * 8 + 6, &off, 1);
}

/* A /mouse record's x, y and buttons (Plan 9's: m, then fields of 11 and a space) */
static void fields (int* x, int* y, unsigned char* buttons)
{
    rec[12] = rec[24] = rec[36] = 0;
    *x = atoi (rec + 1);
    *y = atoi (rec + 13);
    *buttons = (unsigned char) atoi (rec + 25);
}

int vid_mouse (int* x, int* y, unsigned char* buttons)
{
    int fd, n;

    if ((fd = _hy_open ("/dev/vid/mouse", HY_O_READ)) < 0) {
        return -1;
    }
    n = read (fd, rec, sizeof rec);
    close (fd);
    if (n < (int) sizeof rec) {
        return -1;
    }
    fields (x, y, buttons);
    return 0;
}

int vid_mouse_wait (int* x, int* y, unsigned char* buttons)
{
    if (mousefd < 0) {
        if ((mousefd = _hy_open ("/dev/vid/mouse", HY_O_READ)) < 0 || read (mousefd, rec, sizeof rec) < 0) {
            return -1;
        }
    }
    if (read (mousefd, rec, sizeof rec) < (int) sizeof rec) {
        return -1;
    }
    fields (x, y, buttons);
    return 0;
}
