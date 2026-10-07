/*
** conio.c - cc65's conio for the Hydra's console, an ANSI terminal on the serial port: the screen through ANSI
** sequences (cursor moves, clearing, colours, reverse), and the keys read raw from the window's console (its own
** fds on /dev/cons, and /dev/consctl's rawon while the program runs: no echo, each key as it comes, the terminal's
** cursor and function keys as one code each: hydra.h's CH_*).  The screen's size is $COLUMNS x $LINES (the
** environment), or 80 x 24.  Output goes out as stdout's does (PUTC), so conio and printf keep their order; wherex
** and wherey follow what conio writes (not printf's).
*/

#include <conio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <hydra.h>

void __fastcall__ _hy_putc (char c);                    /* conglue.s: PUTC */
int __fastcall__ _hy_open (const char* name, unsigned char mode);

static unsigned char x, y, width, height, sized;
static unsigned char fg = COLOR_WHITE, bg = COLOR_BLACK, border, rev;
static int kfd = -1, nbfd = -1, ctlfd = -1;             /* The keys: /dev/cons, and a look without waiting */
static int pending = -1;                                /* A key kbhit saw */

/* ---- The screen */

static void size (void)
{
    char* v;

    if (sized) {
        return;
    }
    sized = 1;
    width = 80;
    height = 24;
    if ((v = getenv ("COLUMNS")) != 0 && atoi (v) > 0) {
        width = atoi (v);
    }
    if ((v = getenv ("LINES")) != 0 && atoi (v) > 0) {
        height = atoi (v);
    }
}

static void num (unsigned char n)
{
    if (n >= 100) {
        _hy_putc ('0' + n / 100);
    }
    if (n >= 10) {
        _hy_putc ('0' + n / 10 % 10);
    }
    _hy_putc ('0' + n % 10);
}

static void csi (void)
{
    _hy_putc (27);
    _hy_putc ('[');
}

static void sgr (unsigned char n)
{
    csi ();
    num (n);
    _hy_putc ('m');
}

static void down (void)
{
    if (y < height - 1) {
        ++y;
    }
}

void __fastcall__ _hy_cputc (char c)
{
    size ();
    if (c == '\n') {                                    /* A new line (the console: CR LF) */
        _hy_putc ('\n');
        x = 0;
        down ();
    } else if (c == '\r') {
        _hy_putc ('\r');
        x = 0;
    } else if (c == '\b') {
        _hy_putc ('\b');
        if (x) {
            --x;
        }
    } else {
        _hy_putc (c);
        if (++x >= width) {                             /* (The terminal wraps) */
            x = 0;
            down ();
        }
    }
}

void __fastcall__ gotoxy (unsigned char nx, unsigned char ny)
{
    size ();
    x = nx;
    y = ny;
    csi ();
    num (ny + 1);
    _hy_putc (';');
    num (nx + 1);
    _hy_putc ('H');
}

void __fastcall__ gotox (unsigned char nx)
{
    gotoxy (nx, y);
}

void __fastcall__ gotoy (unsigned char ny)
{
    gotoxy (x, ny);
}

unsigned char wherex (void)
{
    return x;
}

unsigned char wherey (void)
{
    return y;
}

void clrscr (void)
{
    csi ();
    _hy_putc ('2');
    _hy_putc ('J');
    gotoxy (0, 0);
}

/* The screen's size: its width (.A) and height (.X), for conglue.s's screensize */
unsigned _hy_consize (void)
{
    size ();
    return (height << 8) | width;
}

unsigned char __fastcall__ textcolor (unsigned char color)
{
    unsigned char old = fg;

    fg = color & 15;
    sgr (fg < 8 ? 30 + fg : 90 + fg - 8);
    return old;
}

unsigned char __fastcall__ bgcolor (unsigned char color)
{
    unsigned char old = bg;

    bg = color & 15;
    sgr (bg < 8 ? 40 + bg : 100 + bg - 8);
    return old;
}

unsigned char __fastcall__ bordercolor (unsigned char color)
{
    unsigned char old = border;                         /* (A terminal has none: it's only kept) */

    border = color;
    return old;
}

unsigned char __fastcall__ revers (unsigned char onoff)
{
    unsigned char old = rev;

    rev = onoff != 0;
    sgr (rev ? 7 : 27);
    return old;
}

void __fastcall__ cputcxy (unsigned char nx, unsigned char ny, char c)
{
    gotoxy (nx, ny);
    _hy_cputc (c);
}

void __fastcall__ chline (unsigned char length)
{
    while (length--) {
        _hy_cputc ('-');
    }
}

void __fastcall__ chlinexy (unsigned char nx, unsigned char ny, unsigned char length)
{
    gotoxy (nx, ny);
    chline (length);
}

void __fastcall__ cvline (unsigned char length)
{
    unsigned char ox = x;

    while (length--) {
        _hy_cputc ('|');
        gotoxy (ox, y + 1);
    }
}

void __fastcall__ cvlinexy (unsigned char nx, unsigned char ny, unsigned char length)
{
    gotoxy (nx, ny);
    cvline (length);
}

void __fastcall__ cclear (unsigned char length)
{
    while (length--) {
        _hy_cputc (' ');
    }
}

void __fastcall__ cclearxy (unsigned char nx, unsigned char ny, unsigned char length)
{
    gotoxy (nx, ny);
    cclear (length);
}

/* ---- The keys */

/* The console's lines as they were, at the program's end */
static void cooked (void)
{
    if (ctlfd >= 0) {
        write (ctlfd, "rawoff", 6);
    }
}

static void raw (void)
{
    if (kfd >= 0) {
        return;
    }
    ctlfd = _hy_open ("/dev/consctl", HY_O_WRITE);     /* (Kept open: raw till the program ends) */
    if (ctlfd >= 0) {
        write (ctlfd, "rawon", 5);
        atexit (cooked);
    }
    kfd = _hy_open ("/dev/cons", HY_O_READ);
    nbfd = _hy_open ("/dev/cons", HY_O_READ | HY_O_NONBLOCK);
}

unsigned char kbhit (void)
{
    unsigned char c;

    raw ();
    if (pending >= 0) {
        return 1;
    }
    if (read (nbfd, &c, 1) == 1) {
        pending = c;
        return 1;
    }
    return 0;
}

char _hy_cgetc (void)
{
    unsigned char c;

    raw ();
    if (pending >= 0) {
        c = pending;
        pending = -1;
    } else {
        while (read (kfd, &c, 1) != 1) {
            hy_yield ();
        }
    }
    return c == '\r' ? '\n' : c;                        /* (Enter: CH_ENTER) */
}
