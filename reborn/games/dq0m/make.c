/*
** make.c - (dq0m) a chunk made (DUNGEON.md), into the hub's cache (world.h), for whoever asked: the same chunk
** whoever it's for (the stratum's seed's and its place's).  The slot it goes in: one empty, or the one least lately
** found, but never one a task is reading (the hub's inuse) nor one beside the character's chunk.
**
** A chunk's edges (the ways across them, the ground at its corners) are the plan's, worked out by plan.c's
** chunk_edges (overlay 4); the chunk grown by its style's grower (built.c, caves.c: overlays 1 and 3) in the main
** RAM, and given its materials and its fixtures (fixtures.c), then copied into the cache.
**
** What's changed in a chunk (the change records, the world's: a square dug, a link made, a door opened) is laid
** over it each time it's made (changes.c), and what the character's seen of it is kept as the cache makes way for
** it, and laid over it again.  A chunk stocked as it's made (its things listed: stock.c, the list the world bank's
** stock beside its slot) has its slot CS_FRESH, for the world to put them down and mark it stocked among the change
** records; the maker never makes way for it till it has.
*/

#include <string.h>
#include <dice.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

static struct chunk cb;                         /* (A chunk grown in the main RAM: the grower's an overlay) */
static unsigned stocks;                         /* (The chunks stocked, counted: each list's seq) */

struct style sty;                               /* (The chunk's stratum's: uses.c copies it from its styles) */
const struct style* SP;
const unsigned char t_kinds[16] = { 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1 };  /* (world.h's T_PASS: the world's slant.s's, the same) */

/* A hub's bank's bytes copied to the main RAM (hub_get), or from it (hub_put), or set (hub_set), the window put back
** as it was: the hub reached from an overlay's code (which the hub's bank in the window would put out) */
void __fastcall__ hub_get (unsigned char bank, const void* from, void* to, unsigned n)
{
    unsigned b = hub_here ();

    hub_map (bank);
    memcpy (to, from, n);
    hub_back (b);
}

void __fastcall__ hub_put (unsigned char bank, void* to, const void* from, unsigned n)
{
    unsigned b = hub_here ();

    hub_map (bank);
    memcpy (to, from, n);
    hub_back (b);
}

void __fastcall__ hub_set (unsigned char bank, void* to, unsigned char c, unsigned n)
{
    unsigned b = hub_here ();

    hub_map (bank);
    memset (to, c, n);
    hub_back (b);
}

/* The slot a new chunk goes in: an empty one, or the least lately found that no task is reading, isn't beside the
** character's chunk, and isn't one stocked that the world hasn't marked yet (CS_FRESH) (the hub's header in the
** window; the world bank after, the cache's table its) */
static unsigned char pick (void)
{
    unsigned char k, best = 0xFF, i, busy, psn = HUB->pc_stratum;
    struct cslot* c;
    struct stratum* s = &STRATUM (psn);
    unsigned pcx = (HUB->pc_x - s->x0) / CW, pcy = (HUB->pc_y - s->y0) / CW;
    unsigned char lim = HUB->small ? 12 : NCSLOTS;     /* (dq0 -C: twelve slots, the tests' cache made way in soon) */

    hub_map (WORLD_BANK);
    for (k = 0; k < lim; ++k) {
        c = &WREC->cslot[k];
        if (!c->used) {
            return k;
        }
        for (i = 0, busy = 0; i < NASK; ++i) {
            busy |= WREC->inuse[i] == k;
        }
        if (busy || c->used == CS_FRESH || (c->sn == psn && c->cx + 1 >= pcx && c->cx <= pcx + 1 && c->cy + 1 >= pcy &&
            c->cy <= pcy + 1)) {
            continue;
        }
        if (best == 0xFF || c->stamp < WREC->cslot[best].stamp) {
            best = k;
        }
    }
    return best == 0xFF ? 0 : best;
}

/* The chunk's style (grow.h's cstyle): its stratum's; but a mixed stratum's (DUNGEON.md's rock and old places,
** slice 4.10's: caves that sentient hands have been at) a third of its chunks left as they were found, grown as caves
** by the natural style, the rest hewn; a chunk with a way between strata in it (the way in, a way down) hewn, where
** the builders' work began.  (dq0 -M, the tests': every built stratum mixed so) */
unsigned char cstyle;

struct edges me;                                /* (The chunk's edges: uses.c's US_STYLE sets the tests' flags in them) */
unsigned char __fastcall__ make_chunk (unsigned char sn, unsigned cx, unsigned cy)
{
    struct stratum* s = &STRATUM (sn);
    unsigned x = s->x0 + cx * CW, y = s->y0 + cy * CW;
    unsigned char k, u = 1;
    struct chunk* c;
    struct cslot* sl;

    chunk_edges (sn, cx, cy, &me);
    hub_map (0);
    k = pick ();
    if (WREC->cslot[k].used) {                  /* (What's been seen of the one made way for, kept) */
        seen_keep (k);
        hub_map (WORLD_BANK);
    }
    sl = &WREC->cslot[k];
    sl->used = 1;
    sl->sn = sn;
    sl->cx = cx;
    sl->cy = cy;
    sl->stamp = ++WREC->stamp;
    hub_map (0);
    c = &cb;
    memset (c, 0, sizeof *c);
    c->sn = sn;
    c->cx = cx;
    c->cy = cy;
    SP = &sty;
    hub_get (0, &HUB->gear, &u, 1);            /* (u its slot's use after: CS_FRESH, or 1) */
    cstyle = (s->style == ST_MIXED || (u & GE_MIXED && s->style == ST_BUILT)) && !me.nland &&
        (unsigned char) (((const unsigned char*) &s->seed)[1] + (unsigned char) cx * 73 + (unsigned char) cy * 151) %
        3 == 0 ? ST_NATURAL : s->style;        /* (A third, by the stratum's seed and the chunk's place) */
    uses (US_STYLE, cstyle | u << 2);          /* (The style, and the tests' flags: no water, no rubble, the passages
                                                **   on the diagonal) */
    if (me.x1 != 0xFF) {                         /* (By its stratum's style, or its part's) */
        if (cstyle == ST_NATURAL) {
            grow_caves (c, &me, s->seed ^ ((unsigned long) cx << 16 | cy) * 40503UL);
        } else {
            grow_built (c, &me, s->seed ^ ((unsigned long) cx << 16 | cy) * 40503UL);
        }
        fixtures (c);                           /* (Its materials, its windows, its traps ...) */
        sky (c, 0);                             /* (Its skylights and windows on the surface: sky.c's) */
    }
    wet_chunk (c, &me);                          /* (Its water: slice 4.9's; then age's rubble, slice 4.10's) */
    air_pocket (c, &me);                         /* (Its air: 2026-10-31) */
    u = 1;
    if (!changes_over (c, x, y) && me.x1 != 0xFF) {
        stock (c, x, y);                        /* (Its things listed, the first time it's made: the world puts them
                                                **   down and marks it so, STOCKED, as it finds its slot CS_FRESH, in
                                                **   the order they were stocked) */
        sconces (c, x, y);                      /* (Its rooms' sconces, their torches lit where they're kept) */
        sk.seq = ++stocks;
        hub_put (WORLD_BANK, &WREC->stock[k], &sk, sizeof sk - sizeof sk.e + sk.n * sizeof sk.e[0]);
        u = CS_FRESH;
    }
    sky (c, 1);                                 /* (The squares by what shows the sky marked: sky.c's) */
    seen_over (c);
    hub_map (CACHE_BANK (k));
    memcpy (CHUNK_AT (k), c, sizeof *c);
    hub_map (WORLD_BANK);
    WREC->cslot[k].used = u;
    hub_map (0);
    return k;
}
