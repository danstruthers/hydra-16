/*
** ipage.c - (dq0w) the item store paged (DUNGEON.md: no fixed size; things.c's store): its records in the hub's item
** banks a cache, of the things the character carries (never paged out) and those of the chunks that are in it (a
** chunk's things: those lying on its squares, and what's in them or on them), each chunk's all or none; the rest in
** the save folder, by region (itSXXYY.dat, a version of it each time it's written: vfile.inc's, three slots, the
** newest read as the next is written), each chunk's things a group of their own there (things.h's struct igroup: its
** place in the region, its count; its things' records, a thing in another its container's place in the group,
** renumbered as it's read in), so no count of things limits the dungeon, only the card.  The chunks in the store are
** in a table in the world's own bank (things.h's struct ipg): the 25 round the character's (two each way) always
** (it_near, as it comes into a chunk: dq0w.c's see), and a chunk's as its things are put down (it_need: stocked, a
** denizen's remains); one goes out (written to its region's file, a new version, its records freed) as a free
** record's wanted and there's none (it_room), or a place in the table, the one least lately wanted that isn't round
** the character.  Without a folder, a full store keeps no more.  The store's kept with the game, its table with it
** (wfile.c's keep: items.dat).
**
** Every reader looks for things lying about only near the character (a look's, a search's, a thing named: twenty
** squares at most), so what's out is never wanted; the things a reader holds by number (the senses' record's, the
** UI's request's, a thing walked over to) are near it, and keep their numbers.  Swapped part 17's (the table) and
** 20's (the files: ../dq0/swap.inc, bank.s's stubs).
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)

extern char path[56];                           /* (store.c's) */
extern struct change buf[8];                    /* (store.c's: 64 bytes) */

/* ==== Part 17: the table */
#pragma code-name (push, "SW17CODE")
#pragma rodata-name (push, "SW17RODATA")
#pragma bss-name (push, "SW17RODATA")           /* (Its own data, what's wanted only while it runs: the main RAM's room) */

#pragma bss-name (push, "BSS")
static unsigned char esn;                       /* (The chunk at hand: the main RAM's, wanted after a part's called) */
static unsigned ecx, ecy;
#pragma bss-name (pop)
static unsigned char fr;                        /* (A free entry, find's) */
static unsigned pcx, pcy;                       /* (The character's chunk, and those round it kept: two each way;
                                                **   dq0 -C's, none) */
static unsigned char pcs, rad;

/* The world's own bank in the window */
static void own (void)
{
    hy_bank (ovbank[OVBANKS]);
}

/* The character's chunk (pcs, pcx, pcy) */
static void pc_chunk (void)
{
    hub_map (0);
    pcs = HUB->pc_stratum;
    pcx = (HUB->pc_x - STRATUM (pcs).x0) >> CW_BITS;
    pcy = (HUB->pc_y - STRATUM (pcs).y0) >> CW_BITS;
    rad = HUB->small ? 0 : 2;
}

/* The table's entry for the chunk at hand (0xFF: none), and a free one (fr: 0xFF none); the world's own bank in the
** window after */
static unsigned char find (void)
{
    struct ires* r;
    unsigned char k, f = 0xFF;

    own ();
    for (k = 0, fr = 0xFF, r = IPG->r; k < NIRES; ++k, ++r) {
        if (!r->used) {
            if (fr == 0xFF) {
                fr = k;
            }
        } else if (f == 0xFF && r->sn == esn && r->cx == ecx && r->cy == ecy) {
            f = k;
        }
    }
    return f;
}

/* Region rx, ry of stratum sn's place among those known to have no file of things (nk: 0xFF, not one); the world's own
** bank in the window after */
static unsigned char nk;
static void __fastcall__ none_of (unsigned char sn, unsigned char rx, unsigned char ry)
{
    unsigned char k;

    own ();
    for (k = 0, nk = 0xFF; k < NNONE; ++k) {
        if (IPG->none[k].sn == sn + 1 && IPG->none[k].rx == rx && IPG->none[k].ry == ry) {
            nk = k;
        }
    }
}

/* A record made free, or a place in the table: the chunk least lately wanted that isn't round the character (nor the
** chunk at hand) put out (part 20's it_out), its entry free: 1, or 0 (none to put out, or no folder) */
#pragma bss-name (push, "BSS")                  /* (Its locals the main RAM's: it calls a part, which puts this one
                                                **   out and in again, its own data as its image has it) */
static unsigned char make_room (void)
{
    struct ires* r;
    unsigned char k, b = 0xFF;
    unsigned s, y;

    pc_chunk ();
    own ();
    for (k = 0, r = IPG->r; k < NIRES; ++k, ++r) {
        if (r->used && (r->sn != pcs || r->cx - pcx + rad > 2 * rad || r->cy - pcy + rad > 2 * rad) &&
            (r->sn != esn || r->cx != ecx || r->cy != ecy) && (b == 0xFF || r->stamp - s > 0x8000U)) {
            b = k;
            s = r->stamp;
        }
    }
    if (b != 0xFF) {
        r = &IPG->r[b];
        k = r->sn;
        s = r->cx;
        y = r->cy;
        if (it_out (k, s, y)) {
            own ();
            IPG->r[b].used = 0;
            none_of (k, s >> 3, y >> 3);
            if (nk != 0xFF) {
                IPG->none[nk].sn = 0;           /* (It has a file now) */
            }
        } else {
            b = 0xFF;
        }
    }
    hub_map (0);
    return b != 0xFF;
}

unsigned char s_it_room (void)
{
    return make_room ();                        /* (Not the chunk it_need wants last: its things being read in, or
                                                **   put down, stocked) */
}

/* Chunk cx, cy of stratum sn's things in the store: read in if they aren't (part 20's it_in; a place in the table
** first: one put out, if it must), and it wanted now: 1, or 0 if they can't be */
unsigned char __fastcall__ s_it_need (unsigned char sn, unsigned cx, unsigned cy)
{
    struct ires* r;
    unsigned char k;

    esn = sn;
    ecx = cx;
    ecy = cy;
    if ((k = find ()) == 0xFF) {
        if (fr == 0xFF && (!make_room () || (find (), fr == 0xFF))) {
            hub_map (0);
            return 0;
        }
        hub_map (0);
        k = HUB->savedir[0];
        none_of (sn, cx >> 3, cy >> 3);
        if (k && nk == 0xFF) {                  /* (Its region's file read, if it may have one: a folder) */
            if (!(k = it_in (sn, cx, cy))) {
                hub_map (0);
                return 0;
            }
            if (k == 2) {
                own ();
                k = IPG->nnext++ & (NNONE - 1);
                IPG->none[k].sn = sn + 1;     /* (Its stratum plus 1: 0, the entry free) */
                IPG->none[k].rx = cx >> 3;
                IPG->none[k].ry = cy >> 3;
            }
        }
        find ();
        r = &IPG->r[k = fr];
        r->used = 1;
        r->sn = sn;
        r->cx = cx;
        r->cy = cy;
    }
    IPG->r[k].stamp = ++IPG->stamp;
    hub_map (0);
    return 1;
}

/* Stratum sn's chunks' things all put out of the store (its slot in the strata's records made way for another's:
** in_chunk's square's chunk wants its record), their entries free */
void __fastcall__ s_it_flush (unsigned char sn)
{
    unsigned char k;
    unsigned cx, cy;

    for (k = 0; k < NIRES; ++k) {
        own ();
        if (IPG->r[k].used && IPG->r[k].sn == sn) {
            cx = IPG->r[k].cx;
            cy = IPG->r[k].cy;
            if (it_out (sn, cx, cy)) {
                own ();
                IPG->r[k].used = 0;
            }
        }
    }
    hub_map (0);
}

/* The chunks round the character's (two each way, in its stratum) in the store, as it comes into a chunk (as it's
** moved: the chunk it was in last in the table's) */
void s_it_near (void)
{
    struct stratum* s;
    unsigned w, h;
    signed char x, y;

    pc_chunk ();
    own ();
    if (IPG->lsn == pcs + 1 && IPG->lcx == pcx && IPG->lcy == pcy) {
        hub_map (0);
        return;
    }
    IPG->lsn = pcs + 1;
    IPG->lcx = pcx;
    IPG->lcy = pcy;
    s = &STRATUM (pcs);
    w = (s->w + CW - 1) >> CW_BITS;
    h = (s->h + CW - 1) >> CW_BITS;
    for (y = -rad; y <= rad; ++y) {
        for (x = -rad; x <= rad; ++x) {
            pc_chunk ();
            if (pcx + x < w && pcy + y < h) {
                s_it_need (pcs, pcx + x, pcy + y);
            }
        }
    }
    hub_map (0);
}

#pragma bss-name (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)

/* ==== Part 20: the files */
#pragma code-name (push, "SW20CODE")
#pragma rodata-name (push, "SW20RODATA")
#pragma bss-name (push, "SW20RODATA")

#define VF_STATIC       static
#include "../dq0/vfile.inc"

static struct vfh hd;
static struct igroup g;
static struct item q;
static unsigned char hsn, hat;                  /* (The chunk at hand: its stratum, its place in its region, */
static unsigned hx0, hy0;                       /*   its first square) */
static int in, out;
static unsigned all;

/* A record's (k's) number in the map, set (mput) or got (mget): a group read in (it_in's, from the map's start), or
** written out (it_out's, from its end: a chunk may be put out as another's read in, their records apart) */
static void __fastcall__ mput (unsigned k, unsigned i)
{
    hy_bank (ovbank[OVBANKS]);
    IPG->map[k] = i;
    hub_map (0);
}

static unsigned __fastcall__ mget (unsigned k)
{
    hy_bank (ovbank[OVBANKS]);
    k = IPG->map[k];
    hub_map (0);
    return k;
}
#define OUT(k)          (NITEMS - 1 - (k))

/* The chunk at hand set, its region's file's name in path: 0, no folder */
static unsigned char __fastcall__ at_hand (unsigned char sn, unsigned cx, unsigned cy)
{
    hsn = sn;
    hx0 = STRATUM (sn).x0 + (cx << CW_BITS);
    hy0 = STRATUM (sn).y0 + (cy << CW_BITS);
    hat = (cx & 7) | (cy & 7) << 3;
    return vf_name (path, "it", sn, cx >> 3, cy >> 3);
}

/* Thing i's in the chunk at hand: it lies on one of its squares, or it's in (or on) one that does */
static unsigned char __fastcall__ in_chunk (unsigned i)
{
    struct item* p;

    for (p = item_at (i); p->where == IW_ITEM; p = item_at (p->x)) {
    }
    return (p->where == IW_SQUARE || p->where == IW_PORTAL) && p->sn == hsn && p->x - hx0 < CW && p->y - hy0 < CW;
}

/* The other chunks' groups in the file in, past (out >= 0: copied to out, their count added to all) or to the chunk
** at hand's (its group in g: 1, or 0 none) */
static unsigned char others (void)
{
    unsigned k;

    while (read (in, &g, sizeof g) == sizeof g) {
        if (g.at == hat && out < 0) {
            return 1;
        }
        if (g.at == hat || out < 0) {
            lseek (in, (long) g.n * sizeof (struct item), SEEK_CUR);
            continue;
        }
        write (out, &g, sizeof g);
        for (k = g.n; k; --k) {
            read (in, buf, sizeof (struct item));
            write (out, buf, sizeof (struct item));
        }
        all += g.n;
    }
    return 0;
}

/* Chunk cx, cy of stratum sn's things written out (its region's file, a new version: every other chunk's group as it
** was, then its own), and their records freed: 1, or 0 (no folder, or no file made) */
unsigned char __fastcall__ s_it_out (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned i, k, m;

    if (!at_hand (sn, cx, cy)) {
        return 0;
    }
    in = vf_open (path, 3, &hd);
    if ((out = vf_make (path, 3)) >= 0) {
        all = 0;
        if (in >= 0) {
            others ();
        }
        for (i = 0, m = 0; (i = items (i, IN_SQUARE | IN_ITEM | IN_PORTAL, 0)) != 0; ) {    /* (Its own things) */
            if (in_chunk (i)) {
                mput (OUT (m), i);
                ++m;
            }
        }
        if (m) {
            g.at = hat;
            g.n = m;
            hub_map (0);
            g.m = CLOCK_256;                    /* (When it's put out: its lights caught up as it's read in) */
            write (out, &g, sizeof g);
            for (k = 0; k < m; ++k) {
                memcpy (&q, item_at (mget (OUT (k))), sizeof q);
                if (q.where == IW_ITEM) {       /* (In another: its container's place in the group) */
                    for (i = 0; i < m && mget (OUT (i)) != q.x; ++i) {
                    }
                    q.x = i;
                }
                write (out, &q, sizeof q);
            }
            all += m;
        }
        vf_done (out, all);
        for (k = 0; k < m; ++k) {
            item_at (mget (OUT (k)))->kind = 0;     /* (Their records free) */
        }
    }
    if (in >= 0) {
        close (in);
    }
    hub_map (0);
    return out >= 0;
}

/* The store's free records (up to its limit, things.c's ilim): how many, and the first after i */
static unsigned frees (void)
{
    unsigned i, n = 0;

    for (i = 1; i < ilim; ++i) {
        n += !item_at (i)->kind;
    }
    return n;
}

static unsigned __fastcall__ free_after (unsigned i)
{
    while (item_at (++i)->kind) {
    }
    return i;
}

/* Chunk cx, cy of stratum sn's things read in (its group in its region's file, if it has one: room made for them first,
** chunks put out, it_room's): 1, or 0 (no room for them all: none read in) */
#pragma bss-name (push, "BSS")                  /* (Its locals the main RAM's: it calls a part, which puts this one
                                                **   out and in again, its own data as its image has it) */
unsigned char __fastcall__ s_it_in (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned k, i, n, gone;
    unsigned char ok = 1;
    int f;

    if (!at_hand (sn, cx, cy) || (in = vf_open (path, 3, &hd)) < 0) {
        return 2;
    }
    out = -1;
    if (others ()) {
        n = g.n;
        hub_map (0);
        gone = CLOCK_256 - g.m;                 /* (The minutes it was out: g's, before another's put out; 25.6
                                                **   seconds a unit, 32 / 75 of a minute; past fourteen hours, all) */
        gone = gone > 2000 ? 0xFFFF : gone * 32 / 75;
        f = in;
        while (frees () < n && it_room ()) {    /* (Another's file read and written meanwhile: its own fds) */
        }
        in = f;
        at_hand (sn, cx, cy);
        if (frees () < n) {
            ok = 0;
        } else {
            for (k = 0, i = 0; k < n; ++k) {
                i = free_after (i);
                read (in, &q, sizeof q);
                if ((q.flags & (I_LIT | I_TENDED)) == I_LIT) {     /* (A light lit, burnt down by the minutes it was
                                                **   out: its last, out quietly, unseen) */
                    q.fuel = q.fuel > gone ? q.fuel - gone : 0;
                    if (!q.fuel && q.id) {
                        q.flags &= ~I_LIT;
                    }
                }
                memcpy (item_at (i), &q, sizeof q);
                if (i >= ihigh) {
                    ihigh = i + 1;              /* (items ()'s end past it) */
                }
                mput (k, i);
            }
            for (k = 0; k < n; ++k) {           /* (In another: its record's number, by its place) */
                i = mget (k);
                if (item_at (i)->where == IW_ITEM) {
                    f = item_at (i)->x;
                    f = mget (f < n ? f : 0);
                    item_at (i)->x = f;
                }
            }
        }
    }
    close (in);
    hub_map (0);
    return ok;
}
#pragma bss-name (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
