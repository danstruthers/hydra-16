/*
** undo.c - each file's undo log, and the cut buffer (edit.h).  The log is a bank of records, a change each: its
** kind (U_INS: bytes put in; U_DEL, U_RUB: bytes taken out, forward or rubbed out backwards), its place (3 bytes),
** its count (2), the bytes, then its count again (so the log can be walked back from its top).  Undoing the top
** record does its opposite and moves the top down past it; redoing does it again and moves the top up; a new change
** drops what was undone.  While typing, deleting or rubbing out goes on (D.ugroup), each grows the last record, so
** it's undone whole; a paste is one record too.  A full log drops its oldest records; a change bigger than the log
** can't be undone (the log is emptied, and the change said to be so).  The cut buffer is banks of its own, shared
** by every file.
*/

#include <string.h>
#include "edit.h"

#define U_INS       1                           /* A record's kind (and U_DEL, U_RUB) */
#define HDR         6                           /* A record's kind, place and count ... */
#define TRL         2                           /*   and its count again */
#define CUT_MAX     16                          /* The cut buffer's banks, at most (128K) */

unsigned char u_off;
static unsigned char rec;                       /* u_delstart's record being filled ... */
static unsigned dat;                            /*   where its next byte goes */
static unsigned char hdr[HDR];

/* The log's bytes: n at off into s, and from s */
static void uget (unsigned off, void* s, unsigned n)
{
    setbank (D.ubank);
    memcpy (s, BANK + off, n);
}

static void uput (unsigned off, const void* s, unsigned n)
{
    setbank (D.ubank);
    memcpy (BANK + off, s, n);
}

/* The record at the top: its start, and its header in hdr */
static unsigned top (void)
{
    unsigned n, at;

    uget (D.utop - TRL, &n, 2);
    at = D.utop - TRL - n - HDR;
    uget (at, hdr, HDR);
    return at;
}

static lpos hpos (void)
{
    return hdr[1] | (unsigned) hdr[2] << 8 | (lpos) hdr[3] << 16;
}

static unsigned hcount (void)
{
    return hdr[4] | hdr[5] << 8;
}

unsigned char u_new (void)
{
    int b = hy_banks_alloc (1);

    D.ubank = b < 0 ? 0xFF : b;
    u_clear ();
    return b < 0;
}

void u_clear (void)
{
    D.utop = D.uend = 0;
    D.ugroup = 0;
}

/* Room for n more bytes at the top (what can be redone dropped, then the oldest records).  1: none (the log too
** small: emptied) */
static unsigned char room (lpos n)
{
    unsigned k;

    D.uend = D.utop;
    if (n > BLK) {
        u_clear ();
        s_msg ("(too big to be undone)");
        return 1;
    }
    while (BLK - D.utop < n) {
        uget (4, &k, 2);
        k += HDR + TRL;
        setbank (D.ubank);
        memmove (BANK, BANK + k, D.utop - k);
        D.utop -= k;
    }
    D.uend = D.utop;
    return 0;
}

/* A new record at the top: its header (kind, place, count) and trailer; its bytes' place is dat */
static void newrec (unsigned char kind, lpos p, unsigned n)
{
    hdr[0] = kind;
    hdr[1] = p;
    hdr[2] = p >> 8;
    hdr[3] = p >> 16;
    hdr[4] = n;
    hdr[5] = n >> 8;
    uput (D.utop, hdr, HDR);
    dat = D.utop + HDR;
    D.utop = D.uend = dat + n + TRL;
    uput (D.utop - TRL, hdr + 4, TRL);
}

/* The top record grown by n bytes (its count in its header and trailer) */
static void grow (unsigned at, unsigned n)
{
    n += hcount ();
    hdr[4] = n;
    hdr[5] = n >> 8;
    uput (at + 4, hdr + 4, 2);
    D.utop = D.uend = at + HDR + n + TRL;
    uput (D.utop - TRL, hdr + 4, TRL);
}

void u_ins (lpos p, const unsigned char* s, unsigned n)
{
    unsigned at;

    if (u_off || D.ubank == 0xFF) {
        return;
    }
    if ((D.ugroup == U_TYPE || D.ugroup == U_PASTE) && D.utop && D.utop == D.uend && !room (n) && D.utop) {
        at = top ();                                    /* Typing on, or a paste's next part: the record grown */
        if (hdr[0] == U_INS && hpos () + hcount () == p) {
            dat = D.utop - TRL;
            grow (at, n);
            uput (dat, s, n);
            if (D.ugroup == U_TYPE && (n != 1 || *s == LF)) {
                D.ugroup = 0;                           /* (A line typed is a record) */
            }
            return;
        }
    }
    if (room (n + HDR + TRL)) {
        return;
    }
    newrec (U_INS, p, n);
    uput (dat, s, n);
    if (D.ugroup == U_PASTE0) {
        D.ugroup = U_PASTE;                             /* (A paste's next parts join this record) */
    } else if (D.ugroup != U_PASTE) {
        D.ugroup = n == 1 && *s != LF ? U_TYPE : 0;
    }
}

void u_delstart (lpos p, lpos n, unsigned char kind)
{
    unsigned at, k;

    rec = 0;
    if (u_off || D.ubank == 0xFF) {
        return;
    }
    if (n == 1 && D.ugroup == kind && D.utop && D.utop == D.uend && !room (1) && D.utop) {
        at = top ();
        if (hdr[0] == kind && kind == U_DEL && hpos () == p) {
            dat = D.utop - TRL;                         /* Deleting on: its byte after the record's */
            grow (at, 1);
            rec = 1;
            return;
        }
        if (hdr[0] == kind && kind == U_RUB && hpos () == p + 1) {
            k = hcount ();                              /* Rubbing out on: its byte before them */
            setbank (D.ubank);
            memmove (BANK + at + HDR + 1, BANK + at + HDR, k);
            hdr[1] = p;
            hdr[2] = p >> 8;
            hdr[3] = p >> 16;
            uput (at + 1, hdr + 1, 3);
            grow (at, 1);
            dat = at + HDR;
            rec = 1;
            return;
        }
    }
    if (room (n + HDR + TRL)) {
        D.ugroup = 0;
        return;
    }
    newrec (kind, p, (unsigned) n);
    rec = 1;
    D.ugroup = n == 1 ? kind : 0;
}

void u_delbytes (const unsigned char* s, unsigned n)
{
    if (rec) {
        uput (dat, s, n);
        dat += n;
    }
}

/* The record's n bytes from off put in at the cursor */
static void reput (unsigned off, unsigned n)
{
    unsigned k;

    while (n) {
        k = n > CHUNK ? CHUNK : n;
        uget (off, bounce, k);
        t_ins (bounce, k);
        off += k;
        n -= k;
    }
}

unsigned char u_undo (void)
{
    unsigned at, n;
    lpos p;

    if (D.ubank == 0xFF || D.utop == 0) {
        return 0;
    }
    at = top ();
    p = hpos ();
    n = hcount ();
    u_off = 1;
    t_goto (p);
    if (hdr[0] == U_INS) {
        t_del (n, 0, 0);
    } else {
        reput (at + HDR, n);
        if (hdr[0] == U_DEL) {
            t_goto (p);
        }
    }
    u_off = 0;
    D.utop = at;
    D.ugroup = 0;
    return 1;
}

unsigned char u_redo (void)
{
    unsigned at = D.utop, n;
    lpos p;

    if (D.ubank == 0xFF || D.utop == D.uend) {
        return 0;
    }
    uget (at, hdr, HDR);
    p = hpos ();
    n = hcount ();
    u_off = 1;
    t_goto (p);
    if (hdr[0] == U_INS) {
        reput (at + HDR, n);
    } else {
        t_del (n, 0, 0);
    }
    u_off = 0;
    D.utop = at + HDR + n + TRL;
    D.ugroup = 0;
    return 1;
}

/* ---- The cut buffer */

static unsigned char cbank[CUT_MAX], ncb;
static lpos clen;

lpos c_len (void)
{
    return clen;
}

void c_clear (void)
{
    clen = 0;
}

unsigned char c_add (const unsigned char* s, unsigned n)
{
    unsigned char i;
    unsigned off, k;
    int b;

    while (n) {
        i = clen >> 13;
        off = (unsigned) clen & (BLK - 1);
        if (i >= ncb) {
            if (ncb == CUT_MAX || (b = hy_banks_alloc (1)) < 0) {
                return 1;
            }
            cbank[ncb++] = b;
        }
        k = BLK - off;
        if (k > n) {
            k = n;
        }
        setbank (cbank[i]);
        memcpy (BANK + off, s, k);
        clen += k;
        s += k;
        n -= k;
    }
    return 0;
}

unsigned char c_paste (void)
{
    lpos done = 0;
    unsigned off, k;

    while (done < clen) {
        off = (unsigned) done & (BLK - 1);
        k = BLK - off;
        if (k > CHUNK) {
            k = CHUNK;
        }
        if (k > clen - done) {
            k = (unsigned) (clen - done);
        }
        setbank (cbank[(unsigned char) (done >> 13)]);
        memcpy (bounce, BANK + off, k);
        if (t_ins (bounce, k)) {
            return 1;
        }
        done += k;
    }
    return 0;
}
