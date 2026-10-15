/*
** make.c - (dq0m) a chunk made (DUNGEON.md), into the hub's cache (world.h), for whoever asked: the same chunk
** whoever it's for (the stratum's seed's and its place's).  The slot it goes in: one empty, or the one least lately
** found, but never one a task is reading (the hub's inuse) nor one beside the character's chunk.
**
** A chunk's edges: the ways across them.  Where an edge is a region's, the plan's way (plan.c: a square along the
** region's edge; the chunk whose stretch of edge it's in has it); inside a region, the region's own tree of ways
** between its 64 chunks, worked out from the seed whenever it's needed (a random walk's, as the plan's are), never
** kept but for the last region's.  An edge the stratum's bounds cut short has its way as near as it fits, the same
** for both chunks it joins.
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

/* ---- A region's own ways between its chunks: (the last asked for's) each chunk's north and west edges' */

static unsigned char dsn = 0xFF, drx, dry;
static unsigned char dn[RCH][RCH], dw[RCH][RCH];

static void __fastcall__ detail (unsigned char sn, unsigned char rx, unsigned char ry)
{
    struct stratum* s = &strata[sn];
    unsigned char ix, iy, nx, ny, d, kw, kh, seen[RCH];
    unsigned todo, steps;

    if (dsn == sn && drx == rx && dry == ry) {
        return;
    }
    dsn = sn;
    drx = rx;
    dry = ry;
    memset (dn, NO_WAY, sizeof dn);
    memset (dw, NO_WAY, sizeof dw);
    if (region (sn, rx, ry)->kind == RG_ROCK) {
        return;
    }
    kw = (s->w - (unsigned) rx * RW + CW - 1) / CW;      /* (Its chunks in the stratum, each way) */
    kh = (s->h - (unsigned) ry * RW + CW - 1) / CW;
    kw = kw > RCH ? RCH : kw;
    kh = kh > RCH ? RCH : kh;
    rseed (s->seed ^ ((unsigned long) rx << 16 | ry) * 2654435761UL);
    memset (seen, 0, sizeof seen);
    seen[0] = 1;
    ix = iy = 0;
    for (todo = kw * kh - 1, steps = 0; todo && steps < 20000; ++steps) {
        d = random (4);
        nx = ix + dir_x[d];
        ny = iy + dir_y[d];
        if (nx >= kw || ny >= kh) {
            continue;
        }
        if (!(seen[ny] & (1 << nx))) {
            seen[ny] |= 1 << nx;
            --todo;
            if (d == DIR_N) {
                dn[ix][iy] = 2 + random (CW - 4);
            } else if (d == DIR_S) {
                dn[nx][ny] = 2 + random (CW - 4);
            } else if (d == DIR_W) {
                dw[ix][iy] = 2 + random (CW - 4);
            } else {
                dw[nx][ny] = 2 + random (CW - 4);
            }
        }
        ix = nx;
        iy = ny;
    }
    for (iy = 0; iy < kh; ++iy) {                /* (A few more, for loops) */
        for (ix = 0; ix < kw; ++ix) {
            if (iy && dn[ix][iy] == NO_WAY && !random (4)) {
                dn[ix][iy] = 2 + random (CW - 4);
            }
            if (ix && dw[ix][iy] == NO_WAY && !random (4)) {
                dw[ix][iy] = 2 + random (CW - 4);
            }
        }
    }
}

/* The way across chunk (cx, cy)'s north edge (west: across its west), or NO_WAY */
static unsigned char __fastcall__ edge_way (unsigned char sn, unsigned cx, unsigned cy, unsigned char west)
{
    struct stratum* s = &strata[sn];
    unsigned char rx = cx / RCH, ry = cy / RCH, ix = cx % RCH, iy = cy % RCH, p;
    unsigned len;

    if (cx >= (s->w + CW - 1) / CW || cy >= (s->h + CW - 1) / CW || (west ? !cx : !cy)) {
        return NO_WAY;
    }
    if (west ? !ix : !iy) {                     /* (A region's edge: the plan's) */
        p = west ? region (sn, rx, ry)->w : region (sn, rx, ry)->n;
        if (p == NO_WAY || p / CW != (west ? iy : ix)) {
            return NO_WAY;
        }
        p %= CW;
    } else {
        detail (sn, rx, ry);
        p = west ? dw[ix][iy] : dn[ix][iy];
        if (p == NO_WAY) {
            return NO_WAY;
        }
    }
    len = west ? (s->h - cy * CW >= CW ? CW : s->h - cy * CW) : (s->w - cx * CW >= CW ? CW : s->w - cx * CW);
    if (len < 3) {
        return NO_WAY;
    }
    return p > len - 2 ? len - 2 : p;
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
    unsigned x = s->x0 + cx * CW, y = s->y0 + cy * CW, w;
    unsigned char i, k;
    struct chunk* c;
    struct cslot* sl;

    e.way[DIR_N] = edge_way (sn, cx, cy, 0);
    e.way[DIR_W] = edge_way (sn, cx, cy, 1);
    e.way[DIR_S] = edge_way (sn, cx, cy + 1, 0);
    e.way[DIR_E] = edge_way (sn, cx + 1, cy, 1);
    e.ground[0] = ground (sn, x, y);
    e.ground[1] = ground (sn, x + CW - 1, y);
    e.ground[2] = ground (sn, x, y + CW - 1);
    e.ground[3] = ground (sn, x + CW - 1, y + CW - 1);
    w = s->w - cx * CW;
    e.x0 = 0;
    e.x1 = w >= CW ? CW - 1 : w - 1;
    w = s->h - cy * CW;
    e.y0 = 0;
    e.y1 = w >= CW ? CW - 1 : w - 1;
    e.nland = 0;
    for (i = 0; i < s->nlinks && e.nland < 4; ++i) {
        if (!s->link[i].dug && s->link[i].x - x < CW && s->link[i].y - y < CW) {   /* (A dug one's the records') */
            e.lx[e.nland] = s->link[i].x - x;
            e.ly[e.nland] = s->link[i].y - y;
            e.ln[e.nland++] = i;
        }
    }
    if (region (sn, cx / RCH, cy / RCH)->kind == RG_ROCK && !e.nland) {
        e.x1 = e.y1 = 0xFF;                     /* (Solid rock: nothing grown) */
    }
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
