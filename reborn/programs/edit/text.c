/*
** text.c - a file's text in the task's RAM banks (edit.h): a row of blocks, each a bank with a gap in it; the
** blocks, the cursor and the iterator are blocks.s's.  Here: a file read and written, bytes in and out at the
** cursor (a block split when its gap's full; one emptied given back, but the last), lines, and finding.  A change
** keeps the screen's first line (D.top, D.topline) and the mark where they were in the text.
*/

#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "edit.h"

struct doc D;
unsigned char bounce[CHUNK];

#define GS(b)       (gsl[b] | gsh[b] << 8)
#define GE(b)       (gel[b] | geh[b] << 8)

unsigned char t_new (void)
{
    int b = hy_banks_alloc (1);

    memset (&D, 0, sizeof D);
    memset (bstate, 0, BSTATE);
    D.ubank = 0xFF;
    if (b < 0) {
        return 1;
    }
    nblk = 1;
    bkb[0] = b;
    geh[0] = BLK >> 8;
    return 0;
}

void t_free (void)
{
    while (nblk) {
        hy_banks_free (bkb[--nblk], 1);
    }
    if (D.ubank != 0xFF) {
        hy_banks_free (D.ubank, 1);
        D.ubank = 0xFF;
    }
}

int t_load (const char* name)
{
    int fd, n;
    unsigned char b = 0, known = 0;
    unsigned k, at0, m;
    unsigned char* p;

    if ((fd = open (name, O_RDONLY)) < 0) {
        return -1;
    }
    for (;;) {
        setbank (bkb[b]);
        at0 = GS (b);
        if ((n = read (fd, BANK + at0, LOAD_FILL - at0)) < 0) {
            close (fd);
            return -1;
        }
        if (n == 0) {
            break;
        }
        k = at0 + n;
        if (BANK[k - 1] == '\r' && read (fd, BANK + k, 1) == 1) {
            ++k;                                        /* (A CR LF kept together) */
        }
        if (!known && (p = memchr (BANK + at0, LF, k - at0)) != 0) {
            known = 1;                                  /* Its first LF: after a CR, its lines end CR LF */
            D.dos = p > BANK && p[-1] == '\r';
        }
        if (D.dos) {
            k = at0 + uncr (BANK + at0, k - at0);
        }
        m = lfs (BANK + at0, k - at0) + (nll[b] | nlh[b] << 8);
        nll[b] = m;
        nlh[b] = m >> 8;
        gsl[b] = k;
        gsh[b] = k >> 8;
        if (k >= LOAD_FILL) {
            if (newblk (b)) {
                close (fd);
                return 1;
            }
            ++b;
        }
    }
    if (GS (b) == 0 && nblk > 1) {
        delblk (b);
    }
    close (fd);
    return 0;
}

/* n bytes of block b's bank from off to fd (each LF as CR LF, if the file's so).  1: they weren't */
static unsigned char put (int fd, unsigned char b, unsigned off, unsigned n)
{
    unsigned char c;
    unsigned k;
    int w;

    while (n) {
        setbank (bkb[b]);
        if (!D.dos) {
            if ((w = write (fd, BANK + off, n)) <= 0) {
                return 1;
            }
            off += w;
            n -= w;
            continue;
        }
        for (k = 0; n && k < CHUNK - 1; --n) {
            if ((c = BANK[off++]) == LF) {
                bounce[k++] = '\r';
            }
            bounce[k++] = c;
        }
        if (write (fd, bounce, k) != (int) k) {
            return 1;
        }
    }
    return 0;
}

int t_save (const char* name)
{
    int fd;
    unsigned char b;

    if ((fd = open (name, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        return -1;
    }
    for (b = 0; b < nblk; ++b) {
        if (put (fd, b, 0, GS (b)) || put (fd, b, GE (b), BLK - GE (b))) {
            close (fd);
            return -1;
        }
    }
    if (close (fd) < 0) {
        return -1;
    }
    D.changed = 0;
    return 0;
}

lpos t_len (void)
{
    it_set (NOWHERE - 1);
    return it_pos ();
}

unsigned t_lines (void)
{
    unsigned n = 1;
    unsigned char b;

    for (b = 0; b < nblk; ++b) {
        n += nll[b] | nlh[b] << 8;
    }
    return n;
}

/* ---- Lines */

void t_gotoline (unsigned n)
{
    unsigned char b;
    unsigned k = 0, m;
    lpos q = 0;

    for (b = 0; b + 1 < nblk && k + (m = nll[b] | nlh[b] << 8) < n; ++b) {
        k += m;
        q += blen (b);
    }
    t_goto (q);
    while (cline < n) {
        if (t_next () < 0) {                            /* (Past the last line: the last's start) */
            t_bol ();
            return;
        }
    }
}

void t_bol (void)
{
    int c;

    while ((c = t_prev ()) >= 0) {
        if (c == LF) {
            t_next ();
            return;
        }
    }
}

void t_eol (void)
{
    int c;

    while ((c = t_get ()) >= 0 && c != LF) {
        t_next ();
    }
}

/* ---- Changes, at the cursor */

unsigned char t_ins (const unsigned char* s, unsigned n)
{
    const unsigned char* s0 = s;
    lpos p0 = cpos;
    unsigned l0 = cline, k;
    unsigned char full = 0;

    while (n) {
        if (GS (cb) == GE (cb) && split ()) {
            full = 1;
            break;
        }
        gapcur ();
        k = ins1 (s, n);
        s += k;
        n -= k;
    }
    if ((k = s - s0) != 0) {                            /* (What went in) */
        D.changed = 1;
        if (D.top > p0) {
            D.top += k;
            D.topline += cline - l0;
        }
        if (D.marked && D.mark > p0) {
            D.mark += k;
        }
        u_ins (p0, s0, k);
    }
    return full;
}

void t_del (lpos n, unsigned char kind, void (*sink) (const unsigned char* s, unsigned n))
{
    lpos p0 = cpos, left = t_len () - cpos;
    unsigned k, l = 0;
    unsigned char b;

    if (n > left) {
        n = left;
    }
    if (!n) {
        return;
    }
    u_delstart (p0, n, kind);
    for (left = n; left; left -= k) {
        if (co >= blen (cb)) {                          /* (Its block's end: the next block's start) */
            ++cb;
            co = 0;
            k = 0;
            continue;
        }
        gapcur ();
        k = del1 (left > CHUNK ? CHUNK : (unsigned) left);
        l += lfs (bounce, k);
        u_delbytes (bounce, k);
        if (sink) {
            sink (bounce, k);
        }
    }
    for (b = nblk; b-- && nblk > 1; ) {                 /* The blocks emptied, given back */
        if (blen (b) == 0) {
            delblk (b);
        }
    }
    t_goto (p0);
    D.changed = 1;
    if (D.top > p0) {
        if (D.top >= p0 + n) {
            D.top -= n;
            D.topline -= l;
        } else {
            D.top = p0;                                 /* (Its start in what went: the screen finds a line's) */
            D.topline = cline;
        }
    }
    if (D.marked && D.mark > p0) {
        D.mark = D.mark >= p0 + n ? D.mark - n : p0;
    }
}

/* ---- Finding */

static unsigned char fold (unsigned char c)
{
    return c >= 'A' && c <= 'Z' ? c | 0x20 : c;
}

/* The rest of pat (n bytes, its first matched) at the iterator?  It's left where it was */
static unsigned char rest (const unsigned char* pat, unsigned char n)
{
    lpos p = it_pos ();
    unsigned char i;
    int c;

    for (i = 1; i < n; ++i) {
        if ((c = it_next ()) < 0 || fold (c) != fold (pat[i])) {
            break;
        }
    }
    it_set (p);
    return i == n;
}

lpos t_find (const unsigned char* pat, unsigned char n, lpos from, signed char dir)
{
    unsigned char f = fold (pat[0]), g = f >= 'a' && f <= 'z' ? f & 0xDF : f;
    int c;

    it_set (from);
    if (dir > 0) {
        while (!fwd1 (f | g << 8)) {
            if (rest (pat, n)) {
                return it_pos () - 1;
            }
        }
    } else {
        it_next ();                                     /* (from itself may be it) */
        while ((c = it_prev ()) >= 0) {
            if (fold (c) == f) {
                it_next ();
                if (rest (pat, n)) {
                    return it_pos () - 1;
                }
                it_prev ();
            }
        }
    }
    return NOWHERE;
}
