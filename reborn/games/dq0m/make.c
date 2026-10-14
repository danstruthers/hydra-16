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
** (What changes in a chunk, what the character has seen, is lost when it's made way for: the records that keep it,
** and the save, are the next step's.)
*/

#include <string.h>
#include <dice.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"

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
        if (s->link[i].x - x < CW && s->link[i].y - y < CW) {
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
    sl = &HUB->cslot[k];
    sl->used = 1;
    sl->sn = sn;
    sl->cx = cx;
    sl->cy = cy;
    sl->stamp = ++HUB->stamp;
    hub_map (CACHE_BANK (k));
    c = CHUNK_AT (k);
    memset (c, 0, sizeof *c);
    c->sn = sn;
    c->cx = cx;
    c->cy = cy;
    if (e.x1 != 0xFF) {
        grow_built (c, &e, s->seed ^ ((unsigned long) cx << 16 | cy) * 40503UL);
    }
    hub_map (0);
    return k;
}
