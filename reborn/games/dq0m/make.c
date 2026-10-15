/*
** make.c - (dq0m) a chunk made (DUNGEON.md), into the hub's cache (world.h), for whoever asked: the same chunk
** whoever it's for (the stratum's seed's and its place's).  The slot it goes in: one empty, or the one least lately
** found, but never one a task is reading (the hub's inuse) nor one beside the character's chunk.
**
** A chunk's edges (the ways across them, the ground at its corners) are the plan's, worked out by plan.c's
** chunk_edges (overlay 4); the chunk grown by its style's grower (built.c, caves.c: overlays 1 and 3) in the main
** RAM, and given its materials and its fixtures (fixtures.c), then copied into the cache.
**
** What's changed in a chunk (changes.c's records: a square dug, a link made) is laid over it each time it's made,
** and what the character's seen of it is kept as the cache makes way for it, and laid over it again.
*/

#include <string.h>
#include <dice.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

static struct chunk cb;                         /* (A chunk grown in the main RAM: the grower's an overlay) */

/* The styles' proportions (a stand-in for the theme's data): purpose-built (rooms and doors, most closed); natural
** (caves: no doors, unless sentient hands have been at them: the mixed style's); mines (galleries, few doors,
** timbered: built's, for now); mixed (built into natural) */
static const struct style styles[4] = {
    /* lit zones z3 slope  ceilings: flat barrel dome cathedral sloped rough   doors open lock bar  hid mag  wood ib
    ** chambers wide glow */
    { 5, 4, 1, 1, { 6, 4, 3, 2, 2, 3 }, 20, 8, 4, 2, 1, 1, 6, 3, 3, 1, 1,      /* built */
      6, 3, 2, 6, { KI_POTSHERDS, KI_BONES, KI_SKULL, KI_CANDLE, KI_BROKEN_SWORD, KI_SACK, KI_ROPE, KI_PENNIES },
      { KI_PENNIES, KI_KEY, KI_LOCKPICKS, KI_POUCH } },
    { 1, 0, 0, 1, { 0, 0, 0, 0, 0, 20 }, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1,      /* natural */
      4, 2, 1, 4, { KI_BONES, KI_BONES, KI_SKULL, KI_POTSHERDS, KI_BROKEN_SWORD, KI_PENNIES, KI_ROPE, KI_BONES },
      { KI_PENNIES, KI_PENNIES, KI_POUCH, KI_KEY } },
    { 2, 2, 0, 1, { 12, 4, 0, 0, 2, 2 }, 8, 12, 2, 2, 1, 0, 9, 1, 2, 1, 0,     /* mine */
      5, 2, 2, 4, { KI_PICK, KI_SHOVEL, KI_ROPE, KI_CANDLE, KI_LANTERN, KI_SACK, KI_BONES, KI_POTSHERDS },
      { KI_PENNIES, KI_CHISEL, KI_POUCH, KI_KEY } },
    { 4, 3, 1, 1, { 4, 3, 2, 1, 2, 8 }, 10, 8, 3, 2, 1, 1, 6, 3, 3, 1, 1,      /* mixed */
      6, 3, 2, 6, { KI_POTSHERDS, KI_BONES, KI_SKULL, KI_CANDLE, KI_BROKEN_SWORD, KI_SACK, KI_ROPE, KI_PENNIES },
      { KI_PENNIES, KI_KEY, KI_LOCKPICKS, KI_POUCH } },
};
const struct style* SP;

/* A hub's bank's bytes copied to the main RAM (hub_get), or from it (hub_put), or set (hub_set), the window put back
** as it was: the hub reached from an overlay's code (which the hub's bank in the window would put out) */
void __fastcall__ hub_get (unsigned char bank, const void* from, void* to, unsigned n)
{
    unsigned char b = getbank ();

    hub_map (bank);
    memcpy (to, from, n);
    setbank (b);
}

void __fastcall__ hub_put (unsigned char bank, void* to, const void* from, unsigned n)
{
    unsigned char b = getbank ();

    hub_map (bank);
    memcpy (to, from, n);
    setbank (b);
}

void __fastcall__ hub_set (unsigned char bank, void* to, unsigned char c, unsigned n)
{
    unsigned char b = getbank ();

    hub_map (bank);
    memset (to, c, n);
    setbank (b);
}

/* The slot a new chunk goes in: an empty one, or the least lately found that no task is reading and isn't beside
** the character's chunk (the hub's header in the window) */
static unsigned char pick (void)
{
    unsigned char k, best = 0xFF, i, busy;
    struct cslot* c;
    struct stratum* s = &strata[HUB->pc_stratum];
    unsigned pcx = (HUB->pc_x - s->x0) / CW, pcy = (HUB->pc_y - s->y0) / CW;

    for (k = 0; k < NCSLOTS; ++k) {
        c = &HUB->cslot[k];
        if (!c->used) {
            return k;
        }
        for (i = 0, busy = 0; i < NASK; ++i) {
            busy |= HUB->inuse[i] == k;
        }
        if (busy || (c->sn == HUB->pc_stratum && c->cx + 1 >= pcx && c->cx <= pcx + 1 && c->cy + 1 >= pcy &&
            c->cy <= pcy + 1)) {
            continue;
        }
        if (best == 0xFF || c->stamp < HUB->cslot[best].stamp) {
            best = k;
        }
    }
    return best == 0xFF ? 0 : best;
}

unsigned char __fastcall__ make_chunk (unsigned char sn, unsigned cx, unsigned cy)
{
    static struct edges e;
    struct stratum* s = &strata[sn];
    unsigned x = s->x0 + cx * CW, y = s->y0 + cy * CW;
    unsigned char k;
    struct chunk* c;
    struct cslot* sl;

    chunk_edges (sn, cx, cy, &e);
    hub_map (0);
    k = pick ();
    if (HUB->cslot[k].used) {                   /* (What's been seen of the one made way for, kept) */
        seen_keep (k);
        hub_map (0);
    }
    sl = &HUB->cslot[k];
    sl->used = 1;
    sl->sn = sn;
    sl->cx = cx;
    sl->cy = cy;
    sl->stamp = ++HUB->stamp;
    c = &cb;
    memset (c, 0, sizeof *c);
    c->sn = sn;
    c->cx = cx;
    c->cy = cy;
    SP = &styles[s->style & 3];
    if (e.x1 != 0xFF) {                         /* (By its stratum's style) */
        if (s->style == ST_NATURAL) {
            grow_caves (c, &e, s->seed ^ ((unsigned long) cx << 16 | cy) * 40503UL);
        } else {
            grow_built (c, &e, s->seed ^ ((unsigned long) cx << 16 | cy) * 40503UL);
        }
    }
    if (!changes_over (c, x, y) && e.x1 != 0xFF) {
        stock (c, x, y);                        /* (Its things put down, the first time it's made) */
        change_keep (sn, x, y, 0, 0, STOCKED);
    }
    seen_over (c);
    hub_map (CACHE_BANK (k));
    memcpy (CHUNK_AT (k), c, sizeof *c);
    hub_map (0);
    return k;
}
