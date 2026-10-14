/*
** changes.c - (dq0m) what's changed in the dungeon, kept (DUNGEON.md: changing the dungeon): each change a record (a
** stratum, a square, what it became: its kind, its floor, the link it is), in banks of the maker's own, laid over a
** chunk each time it's made again; and the squares the character has seen, each chunk's as a map of bits, kept as
** the cache makes way for the chunk and laid over it when it's made again.  (The save, on the card, is the next
** step's: these are what it will write.)
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

#define CH_BANKS        2
#define CH_PER_BANK     (0x2000 / sizeof (struct change))
#define SEEN_BANKS      2
#define SEEN_PER_BANK   (0x2000 / sizeof (struct seen))

struct change {
    unsigned char   sn;
    unsigned        x, y;
    unsigned char   tile;                       /* Its kind (but the seen bit) */
    signed char     elev;
    unsigned char   link;                       /* The stratum's link it is (an opening), or 0xFF */
};

struct seen {
    unsigned char   used, sn;
    unsigned        cx, cy;
    unsigned char   rooms;                      /* Its rooms visited (bits) */
    unsigned char   bits[CW * CW / 8];
};

static unsigned char chb, seb;                  /* The first banks */
static unsigned nch;                            /* Changes kept */
static unsigned char nseen, nexts;              /* Seen maps kept, the next made way for */

void changes_init (void)
{
    int b = hy_banks_alloc (CH_BANKS + SEEN_BANKS);

    chb = b;
    seb = b + CH_BANKS;
    for (nexts = 0; nexts < SEEN_BANKS; ++nexts) {
        setbank (seb + nexts);
        memset ((void*) 0x8000, 0, 0x2000);
    }
    nexts = 0;
}

static struct change* __fastcall__ change_at (unsigned i)
{
    setbank (chb + i / CH_PER_BANK);
    return (struct change*) 0x8000 + i % CH_PER_BANK;
}

static struct seen* __fastcall__ seen_at (unsigned char i)
{
    setbank (seb + i / SEEN_PER_BANK);
    return (struct seen*) 0x8000 + i % SEEN_PER_BANK;
}

/* A change kept (the window moved) */
void __fastcall__ change_keep (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link)
{
    struct change* c;

    if (nch >= CH_BANKS * CH_PER_BANK) {
        return;                                 /* (Full: the save's, the next step's, will hold more) */
    }
    c = change_at (nch++);
    c->sn = sn;
    c->x = x;
    c->y = y;
    c->tile = tile;
    c->elev = elev;
    c->link = link;
}

/* The change on a chunk (in the main RAM, at x, y in the frame): its square, and (a link) its opening */
static void __fastcall__ change_put (struct chunk* k, unsigned char lx, unsigned char ly, unsigned char tile,
    signed char elev, unsigned char link)
{
    k->tile[ly][lx] = (k->tile[ly][lx] & T_SEEN) | tile;
    k->elev[ly][lx] = elev;
    if (link != 0xFF && k->nopen < NOPEN) {
        k->open[k->nopen].kind = strata[k->sn].link[link].kind;
        k->open[k->nopen].x = lx;
        k->open[k->nopen].y = ly;
        k->open[k->nopen].sill = 0;
        k->open[k->nopen++].link = link;
    }
}

/* The changes laid over a chunk just made (in the main RAM; its first square x, y in the frame) */
void __fastcall__ changes_over (struct chunk* k, unsigned x, unsigned y)
{
    static struct change c;
    unsigned i;
    unsigned char b = getbank ();

    for (i = 0; i < nch; ++i) {
        memcpy (&c, change_at (i), sizeof c);
        if (c.sn == k->sn && c.x - x < CW && c.y - y < CW) {
            change_put (k, c.x - x, c.y - y, c.tile, c.elev, c.link);
        }
    }
    setbank (b);
}

/* A change on a chunk in the cache (slot s: the hub's bank in the window after) */
void __fastcall__ change_cached (unsigned char s, unsigned char lx, unsigned char ly, unsigned char tile,
    signed char elev, unsigned char link)
{
    static struct chunk* k;

    hub_map (CACHE_BANK (s));
    k = CHUNK_AT (s);
    change_put (k, lx, ly, tile, elev, link);
}

/* ---- The squares seen: a chunk's kept as the cache makes way for it (slot s), laid over it when it's made again */

void __fastcall__ seen_keep (unsigned char s)
{
    static struct seen r;
    struct chunk* k;
    unsigned char i, j, n;

    hub_map (CACHE_BANK (s));
    k = CHUNK_AT (s);
    memset (&r, 0, sizeof r);
    r.used = 1;
    r.sn = k->sn;
    r.cx = k->cx;
    r.cy = k->cy;
    for (j = 0, n = 0; j < CW; ++j) {
        for (i = 0; i < CW; ++i) {
            if (k->tile[j][i] & T_SEEN) {
                r.bits[(j * CW + i) >> 3] |= 1 << (i & 7);
                n = 1;
            }
        }
    }
    for (i = 0; i < k->nrooms; ++i) {
        if (k->room[i].flags & R_VISITED) {
            r.rooms |= 1 << i;
        }
    }
    if (!n) {
        return;                                 /* (Nothing seen: nothing to keep) */
    }
    for (i = 0; i < SEEN_BANKS * SEEN_PER_BANK; ++i) {   /* (Its own map again, if it has one) */
        if (seen_at (i)->used && seen_at (i)->sn == r.sn && seen_at (i)->cx == r.cx && seen_at (i)->cy == r.cy) {
            break;
        }
    }
    if (i == SEEN_BANKS * SEEN_PER_BANK) {
        i = nexts;                              /* (Else the next, round: the oldest made way for) */
        nexts = (nexts + 1) % (SEEN_BANKS * SEEN_PER_BANK);
    }
    memcpy (seen_at (i), &r, sizeof r);
}

void __fastcall__ seen_over (struct chunk* k)
{
    static struct seen r;
    unsigned char i, j, m;

    for (i = 0; i < SEEN_BANKS * SEEN_PER_BANK; ++i) {
        memcpy (&r, seen_at (i), sizeof r);
        if (r.used && r.sn == k->sn && r.cx == k->cx && r.cy == k->cy) {
            for (j = 0; j < CW * CW / 8; ++j) {
                if (r.bits[j]) {
                    for (m = 0; m < 8; ++m) {
                        if (r.bits[j] & (1 << m)) {
                            k->tile[(j * 8 + m) / CW][(j * 8 + m) % CW] |= T_SEEN;
                        }
                    }
                }
            }
            for (j = 0; j < k->nrooms; ++j) {
                if (r.rooms & (1 << j)) {
                    k->room[j].flags |= R_VISITED;
                }
            }
            break;
        }
    }
}
