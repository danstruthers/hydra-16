/*
** screen.c - the terminal (an ANSI one: the console's; its size $COLUMNS x $LINES, or 80 x 24): a title line, the
** text's rows, a message line, and two lines naming the commonest keys.  What's on the screen is kept (the shadows),
** and a row is written only where it changed: the cursor's row alone as it's typed in, every row below it as lines
** come and go, the rows that come in as the screen scrolls a few lines (the terminal scrolls the text's rows itself:
** they're its scrolling region).  A tab goes to the next stop of 8; a control character shows as ^ and a letter, a
** byte past 127 as a dot; a line past the screen's right edge has a > there; the block from the mark is reversed.
** The output goes in one write at the end; keys come raw through conio (cgetc).
**   The sequences used (for a screen console to know): CUP, EL, ED 2, SGR 7 and 27, DECSTBM, and LF and RI at the
** scrolling region's edges.
*/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <conio.h>
#include "edit.h"

#define OBUF        255                         /* The output's bytes, at most, before they go */
#define ESC         27
#define MAXW        160                         /* The widest screen used (a wider one: its left 160 columns) */
#define HELP        "/lib/edit/help"            /* The keys, for ^G */

unsigned char W, H, TH, helpshown = 1;
extern unsigned char ndocs, cur;                /* (edit.c's: the files open, and the one shown) */

static unsigned char vbank;                     /* The shadows' bank: */
#define shadow      BANK                        /*   the text's rows as they are on the screen (bit 7: reversed), */
#define tsh         (BANK + BLK - 2 * MAXW)     /*   the title's ... */
#define msh         (BANK + BLK - MAXW)         /*   and the message line's */
static unsigned char rb[MAXW];                  /* A row being made */
static unsigned char dlo, dhi;                  /* The text's rows to be made again: from dlo to dhi */
static char msgt[81];
static unsigned char obuf[OBUF];
static unsigned char on;
static lpos ms, me, ip;                         /* The block from the mark (reversed), and where a row's bytes are */

/* ---- Output */

static void o_flush (void)
{
    if (on) {
        write (1, obuf, on);
        on = 0;
    }
}

static void o_ch (unsigned char c)
{
    if (on == OBUF) {
        o_flush ();
    }
    obuf[on++] = c;
}

static void o_str (const char* s)
{
    while (*s) {
        o_ch (*s++);
    }
}

static void o_num (unsigned n)
{
    char b[6];

    o_str (utoa (n, b, 10));
}

static void o_csi (void)
{
    o_ch (ESC);
    o_ch ('[');
}

/* The cursor to row r, column c (from 0) */
static void o_at (unsigned char r, unsigned char c)
{
    o_csi ();
    o_num (r + 1);
    o_ch (';');
    o_num (c + 1);
    o_ch ('H');
}

static void o_rev (unsigned char rev)
{
    o_csi ();
    if (!rev) {
        o_ch ('2');
    }
    o_str ("7m");
}

/* Screen row srow made nb, where it differs from sh (its shadow, in vbank: made the same) */
static void putrow (unsigned char srow, const unsigned char* nb, unsigned char* sh)
{
    unsigned char a, e, z, i, c, rev = 0;

    setbank (vbank);
    for (a = 0; a < W && sh[a] == nb[a]; ++a) {
    }
    if (a == W) {
        return;
    }
    for (e = W; sh[e - 1] == nb[e - 1]; --e) {
    }
    for (z = W; z > a && nb[z - 1] == ' '; --z) {       /* (Blanks to its end: cleared) */
    }
    o_at (srow, a);
    for (i = a; i < (z < e ? z : e); ++i) {
        c = nb[i];
        if ((c & 0x80) != rev) {
            rev = c & 0x80;
            o_rev (rev);
        }
        o_ch (c & 0x7F);
    }
    if (rev) {
        o_rev (0);
    }
    if (z < e) {
        o_csi ();
        o_ch ('K');
    }
    memcpy (sh + a, nb + a, W - a);
}

/* ---- Columns */

/* The columns byte c takes at column col */
static unsigned char cw (unsigned char c, unsigned col)
{
    return c == '\t' ? TAB - col % TAB : c < 32 || c == 127 ? 2 : 1;
}

unsigned colof (void)
{
    unsigned k = 0, col = 0;
    int c;

    it_set (cpos);
    while ((c = it_prev ()) >= 0 && c != LF) {
        ++k;
    }
    if (c == LF) {
        it_next ();
    }
    while (k--) {
        col += cw (it_next (), col);
    }
    return col;
}

void tocol (unsigned want)
{
    unsigned col = 0;
    unsigned char w;
    int c;

    t_bol ();
    while ((c = t_get ()) >= 0 && c != LF) {
        w = cw (c, col);
        if (col + w > want) {
            break;
        }
        col += w;
        t_next ();
    }
}

/* ---- Lines */

/* The start of the line n lines above from's */
static lpos lineup (lpos from, unsigned n)
{
    int c;

    it_set (from);
    ++n;
    while ((c = it_prev ()) >= 0) {
        if (c == LF && !--n) {
            it_next ();
            break;
        }
    }
    return it_pos ();
}

/* The iterator past n LFs (or at the end) */
static void skip (unsigned n)
{
    int c;

    while (n) {
        if ((c = it_next ()) < 0) {
            return;
        }
        if (c == LF) {
            --n;
        }
    }
}

/* ---- What's to be drawn again */

void s_dirty (void)
{
    unsigned r = cline - D.topline;

    if (cline < D.topline || r >= TH) {
        r = 0;
    }
    if (r < dlo) {
        dlo = r;
    }
    dhi = TH;
}

void s_dirtyrow (void)
{
    unsigned r = cline - D.topline;

    if (cline < D.topline || r >= TH) {
        s_dirty ();
        return;
    }
    if (r < dlo) {
        dlo = r;
    }
    if (r + 1 > dhi) {
        dhi = r + 1;
    }
}

void s_dirtyall (void)
{
    dlo = 0;
    dhi = TH;
}

void s_settop (unsigned line)
{
    D.top = lineup (cpos, cline - line);
    D.topline = line;
    s_dirtyall ();
}

/* ---- The screen */

static const char* const keys[] = {
    "^G", "Help", "^O", "Save", "^R", "Open", "^W", "Find", "^K", "Cut", "^U", "Paste", "^X", "Exit",
    "M-A", "Mark", "M-6", "Copy", "M-R", "Replace", "M-U", "Undo", "M-E", "Redo", "M-G", "Line", "M-.", "File"
};

/* A help line: 7 keys from keys[k] */
static void helpline (unsigned char srow, unsigned char k)
{
    unsigned char i, col = 0;

    o_at (srow, 0);
    for (i = 0; i < 7 && col + 11 <= W; ++i, k += 2) {
        o_rev (1);
        o_str (keys[k]);
        o_rev (0);
        o_ch (' ');
        o_str (keys[k + 1]);
        col += strlen (keys[k]) + 1 + strlen (keys[k + 1]);
        for (; i < 6 && col % 11; ++col) {
            o_ch (' ');
        }
    }
}

unsigned char s_init (void)
{
    int b = hy_banks_alloc (1);

    if (b < 0) {
        return 1;
    }
    vbank = b;
    screensize (&W, &H);
    if (W > MAXW) {
        W = MAXW;
    }
    if (H < 8) {
        H = 8;
    }
    if ((H - 2) * W > BLK - 2 * MAXW) {
        H = (BLK - 2 * MAXW) / W + 2;
    }
    return 0;
}

void s_done (void)
{
    o_csi ();
    o_ch ('r');
    o_csi ();
    o_str ("2J");
    o_at (0, 0);
    o_flush ();
}

void s_all (void)
{
    TH = H - 2 - (helpshown ? 2 : 0);
    o_csi ();
    o_str ("0m");
    o_csi ();
    o_str ("2J");
    o_csi ();                                           /* The text's rows: the scrolling region */
    o_num (2);
    o_ch (';');
    o_num (TH + 1);
    o_ch ('r');
    setbank (vbank);
    memset (shadow, ' ', TH * W);
    memset (tsh, ' ', W);
    memset (msh, ' ', W);
    if (helpshown) {
        helpline (TH + 2, 0);
        helpline (TH + 3, 14);
    }
    s_dirtyall ();
}

/* The screen's top line, so the cursor's line is on it: a few lines away, the terminal scrolls; further, the
** cursor's line goes in the middle.  (A deletion may have left the top in a line: its start) */
static void fixtop (void)
{
    unsigned l = cline, t = D.topline, k;
    unsigned char n;

    if (D.top) {
        it_set (D.top);
        if (it_prev () != LF) {
            D.top = lineup (D.top, 0);
        }
    }
    if (l >= t && l < t + TH) {
        return;
    }
    if (l < t && t - l < TH / 2) {                      /* Down a few: RI at the region's top */
        k = t - l;
        D.top = lineup (D.top, k);
        D.topline = l;
        o_at (1, 0);
        for (n = k; n; --n) {
            o_ch (ESC);
            o_ch ('M');
        }
        setbank (vbank);
        memmove (shadow + k * W, shadow, (TH - k) * W);
        memset (shadow, ' ', k * W);
        dhi = dlo < dhi ? (dhi + k > TH ? TH : dhi + k) : k;
        dlo = 0;
    } else if (l >= t + TH && l - (t + TH - 1) < TH / 2) {  /* Up a few: LF at its bottom */
        k = l - (t + TH - 1);
        it_set (D.top);
        skip (k);
        D.top = it_pos ();
        D.topline += k;
        o_at (TH, 0);
        for (n = k; n; --n) {
            o_ch (LF);
        }
        setbank (vbank);
        memmove (shadow, shadow + k * W, (TH - k) * W);
        memset (shadow + (TH - k) * W, ' ', k * W);
        dlo = dlo < dhi ? (dlo > k ? dlo - k : 0) : TH - k;
        if (dlo > TH - k) {
            dlo = TH - k;
        }
        dhi = TH;
    } else {
        k = l < TH / 2 ? l : TH / 2;
        s_settop (l - k);
    }
}

/* A row made in rb: the line at the iterator, its bytes from D.left on; the iterator left at the next line's
** start (or the end) */
static void compose (void)
{
    unsigned col = 0, end = D.left + W;
    unsigned char w, g, rev = 0;
    int c;

    memset (rb, ' ', W);
    while ((c = it_next ()) >= 0 && c != LF) {
        w = cw (c, col);
        g = c == '\t' ? ' ' : w == 2 ? '^' : c > 127 ? '.' : c;
        if (D.marked) {
            rev = ip >= ms && ip < me ? 0x80 : 0;
            ++ip;
        }
        for (; w; --w, ++col) {
            if (col >= D.left && col < end) {
                rb[col - D.left] = g | rev;
            }
            if (g == '^') {
                g = c ^ 64;
            }
        }
    }
    if (c == LF) {
        ++ip;
    }
    if (col > end) {
        rb[W - 1] = '>';
    }
}

/* The title: the file's name (* if it's changed), the cursor's line and column, and which file of how many */
static void title (void)
{
    static char t[48];
    unsigned char n, m;
    char b[6];

    memset (rb, ' ', W);
    memcpy (rb + 1, "edit", 4);
    n = strlen (D.name);
    if (!n) {
        memcpy (rb + 7, "(new)", 5);
    } else {
        memcpy (rb + 7, D.name, n > W - 40 ? W - 40 : n);
    }
    if (D.changed) {
        rb[(n ? (n > W - 40 ? W - 40 : n) : 5) + 8] = '*';
    }
    strcpy (t, "line ");
    strcat (t, utoa (cline + 1, b, 10));
    strcat (t, "/");
    strcat (t, utoa (t_lines (), b, 10));
    strcat (t, "  col ");
    strcat (t, utoa (colof () + 1, b, 10));
    if (ndocs > 1) {
        strcat (t, "  file ");
        strcat (t, utoa (cur + 1, b, 10));
        strcat (t, "/");
        strcat (t, utoa (ndocs, b, 10));
    }
    m = strlen (t);
    memcpy (rb + W - 1 - m, t, m);
    for (n = 0; n < W; ++n) {
        rb[n] |= 0x80;
    }
    putrow (0, rb, tsh);
}

void s_render (void)
{
    unsigned c, r;

    fixtop ();
    c = colof ();
    if (c < D.left || c >= D.left + W) {                /* (The screen's left edge, so the cursor's on it) */
        D.left = c > W / 2 ? c - W / 2 : 0;
        s_dirtyall ();
    }
    if (D.marked) {
        s_dirtyall ();
        ms = D.mark < cpos ? D.mark : cpos;
        me = D.mark < cpos ? cpos : D.mark;
    }
    if (dlo < dhi) {
        it_set (D.top);
        skip (dlo);
        if (D.marked) {
            ip = it_pos ();
        }
        for (r = dlo; r < dhi; ++r) {
            compose ();
            putrow (r + 1, rb, shadow + r * W);
        }
        dlo = TH;
        dhi = 0;
    }
    title ();
    memset (rb, ' ', W);
    r = strlen (msgt);
    memcpy (rb, msgt, r < W ? r : W);
    putrow (TH + 1, rb, msh);
    o_at (1 + cline - D.topline, c - D.left);
    o_flush ();
}

void s_msg (const char* m)
{
    strncpy (msgt, m, sizeof msgt - 1);
}

void s_msg2 (const char* a, const char* b)
{
    s_msg (a);
    strncat (msgt, b, sizeof msgt - 1 - strlen (msgt));
}

/* The message line: q, reversed, then t; the cursor after them.  (Its shadow: unknown, for the next render) */
static void ask (const char* q, const char* t)
{
    o_at (TH + 1, 0);
    o_rev (1);
    o_str (q);
    o_rev (0);
    o_ch (' ');
    o_str (t);
    o_csi ();
    o_ch ('K');
    o_flush ();
    setbank (vbank);
    memset (msh, 0, W);
}

unsigned char s_prompt (const char* q, char* buf, unsigned char max)
{
    unsigned char n = strlen (buf), k;

    for (;;) {
        ask (q, buf);
        k = cgetc ();
        if (k == '\n') {
            return 1;
        }
        if (k == ESC) {
            return 0;
        }
        if ((k == 8 || k == 127) && n) {
            buf[--n] = 0;
        } else if (k == ('U' & 0x1F)) {
            buf[n = 0] = 0;
        } else if (k >= ' ' && k < 127 && n < max) {
            buf[n++] = k;
            buf[n] = 0;
        }
    }
}

int s_ask (const char* q, const char* keys)
{
    unsigned char k;
    const char* p;

    ask (q, "");
    for (;;) {
        if ((k = cgetc ()) == ESC) {
            return -1;
        }
        if ((p = strchr (keys, k | 0x20)) != 0) {
            return p - keys;
        }
    }
}

/* ^G: the help file, the whole screen (its first H - 1 lines), then a key */
void s_help (void)
{
    int fd, n;

    o_csi ();
    o_ch ('r');
    o_csi ();
    o_str ("2J");
    o_at (0, 0);
    o_flush ();
    if ((fd = open (HELP, O_RDONLY)) < 0) {
        o_str ("edit: no help (" HELP ")");
    } else {
        while ((n = read (fd, bounce, CHUNK)) > 0) {
            write (1, bounce, n);
        }
        close (fd);
    }
    o_flush ();
    cgetc ();
    s_all ();
}
