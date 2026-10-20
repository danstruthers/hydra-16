/*
** plan.c - (dq0m) a stratum grown whole, in outline (DUNGEON.md), as anyone first comes to it: its plan, a grid of
** regions (8 by 8 chunks each) in the hub's plans' banks, each region's kind, the lie of the rock at its corner,
** and the ways across its edges; and its links down to the stratum below.  Purpose-built strata so far: the regions halls, a few solid rock; the ways a tree
** that reaches every hall (a random walk's: the first step into each region is a way), and a few more for loops;
** the rock's lie a walk too, each region near its neighbours' (a built stratum's level).  The regions none of the
** halls reach become rock.
** (The populations, the water and the features come with their steps.)
**
** And a chunk's edges, from the plan (chunk_edges: make.c's): the ways across them, the ground at its corners.
** Where an edge is a region's, the plan's way (a square along the region's edge; the chunk whose stretch of edge
** it's in has it); inside a region, the region's own tree of ways between its 64 chunks, worked out from the seed
** whenever it's needed (a random walk's, as the plan's are), kept in a bank of the maker's own (a cache of them, so
** a chunk on a region's edge doesn't work its neighbours' out again and again).  An edge the stratum's bounds cut
** short has its way as near as it fits, the same for both chunks it joins.
**
** (Overlay 4, the maker's: its code in a bank of its own, so the plan's banks are reached a region at a time, copied
** out and back (rg, rp: hub_get's and hub_put's), and the cache by bread and bwrite.)
*/

#pragma code-name ("OV4CODE")
#pragma rodata-name ("OV4RODATA")

#include <string.h>
#include <dice.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

static struct region R;                         /* (A region copied out of its plan: rg's, put back by rp) */
static struct region* ra;
static unsigned char rb, rsn, rbig;
static unsigned ri;

/* (A plan of more regions than PLAN_MAX, with a save folder: paged to its file, changes.c's pl_get, pl_put: move 2.8's
** no fixed size) */
#define BIG(s)          ((unsigned) (s)->rw * (s)->rh > PLAN_MAX)

/* (The two in the main RAM: overlay 4 full, the pieces' places overlay 5's) */
#pragma code-name (push, "CODE")
#pragma rodata-name (push, "RODATA")

static struct region* __fastcall__ rg (unsigned char sn, unsigned char rx, unsigned char ry)
{
    ri = (unsigned) ry * STRATUM (sn).rw + rx;
    rsn = sn;
    if ((rbig = BIG (&STRATUM (sn))) != 0) {
        pl_get (sn, ri, &R);
    } else {
        rb = PLAN_BANK (sn);
        ra = PLAN_AT (sn) + ri;
        hub_get (rb, ra, &R, sizeof R);
    }
    return &R;
}

static void rp (void)
{
    if (rbig) {
        pl_put (rsn, ri, &R);
    } else {
        hub_put (rb, ra, &R, sizeof R);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* A region's corner's lie (feet), the plan's edge's for a corner past it */
static int __fastcall__ corner (unsigned char sn, unsigned char rx, unsigned char ry)
{
    struct stratum* s = &STRATUM (sn);

    return 2 * rg (sn, rx < s->rw ? rx : s->rw - 1, ry < s->rh ? ry : s->rh - 1)->z;
}

/* The lie of the rock at a square (feet above the datum: the regions' corners, between them evenly) */
static int __fastcall__ ground (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &STRATUM (sn);
    unsigned ox = x - s->x0, oy = y - s->y0;
    unsigned char rx = ox >> 8, ry = oy >> 8, fx = ox & 0xFF, fy = oy & 0xFF;
    long a = corner (sn, rx, ry), b = corner (sn, rx + 1, ry), c = corner (sn, rx, ry + 1), d = corner (sn, rx + 1, ry + 1);
    long top = a * (256 - fx) + b * fx, bottom = c * (256 - fx) + d * fx;

    return (int) ((top * (256 - fy) + bottom * fy) >> 16);
}

/* The squares along a region's edge that are in the stratum (its last regions may be short) */
static unsigned char __fastcall__ edge_len (unsigned start, unsigned total)
{
    return total - start >= RW ? RW - 1 : total - start - 1;
}

/* A way across an edge: a square along it, never at a chunk's corner */
static unsigned char __fastcall__ way_at (unsigned char len)
{
    unsigned char chunks = ((unsigned) len + 1) / CW, at;

    if (len < 4) {
        return 1;
    }
    at = random (chunks ? chunks : 1) * CW;
    return at + 2 + random (at + CW - 2 <= len ? CW - 4 : len - at - 2);
}

#define VISITED     0x01

void __fastcall__ o_plan_stratum (unsigned char n)
{
    struct stratum* s = &STRATUM (n);
    struct region* r;
    unsigned char rx, ry, i, k, nx, ny, d;
    unsigned todo, steps;
    int z;

    s->rw = (s->w + RW - 1) / RW;
    s->rh = (s->h + RW - 1) / RW;
    hub_get (THEME_BANK, &THEME->sage[n < TSTRATA ? n : TSTRATA - 1], &k, 1);
    if (k) {
        s->age = k;                             /* (Its age its history's: its oldest layer's era's, theme.h) */
    }
    hub_get (0, HUB->savedir, &k, 1);
    while (!k && (unsigned) s->rw * s->rh > PLAN_MAX) {  /* (Without a save folder, a plan as big as its bank's
                                                **   half holds; with one, any: its file's) */
        if (s->rw > s->rh) {
            --s->rw;
            s->w = s->rw * RW;
        } else {
            --s->rh;
            s->h = s->rh * RW;
        }
    }
    rseed (s->seed);
    if (BIG (s)) {
        pl_new (n);
    } else {
        hub_set (PLAN_BANK (n), PLAN_AT (n), 0, 0x1000);
    }
    /* Kinds, and the rock's lie: each region near the average of those north and west of it */
    for (ry = 0; ry < s->rh; ++ry) {
        for (rx = 0; rx < s->rw; ++rx) {
            z = 0;
            k = 0;
            if (rx) {
                z += rg (n, rx - 1, ry)->z;
                ++k;
            }
            if (ry) {
                z += rg (n, rx, ry - 1)->z;
                ++k;
            }
            z = (k ? z / k : 0) + (int) random (9) - 4;
            r = rg (n, rx, ry);
            r->z = s->style == ST_BUILT ? 0 : z > 50 ? 50 : z < -50 ? -50 : z;   /* (A built stratum's level: its
                                                **   floors flat, DUNGEON.md's built to a plan) */
            r->kind = s->rw * s->rh > 1 && !random (6) ? RG_ROCK : RG_HALLS;
            r->n = r->w = NO_WAY;
            rp ();
        }
    }
    for (i = 0; i < s->nlinks; ++i) {           /* (A link's region is halls) */
        rg (n, (s->link[i].x - s->x0) >> 8, (s->link[i].y - s->y0) >> 8)->kind = RG_HALLS;
        rp ();
    }
    /* The halls the first link's region reaches (repeated sweeps), the rest rock */
    rx = s->nlinks ? (s->link[0].x - s->x0) >> 8 : 0;
    ry = s->nlinks ? (s->link[0].y - s->y0) >> 8 : 0;
    rg (n, rx, ry)->flags |= VISITED;
    rp ();
    do {
        k = 0;
        for (ny = 0; ny < s->rh; ++ny) {
            for (nx = 0; nx < s->rw; ++nx) {
                r = rg (n, nx, ny);
                if (r->kind != RG_HALLS || (r->flags & VISITED)) {
                    continue;
                }
                for (d = 0; d < 4; ++d) {
                    if ((unsigned char) (nx + dir_x[d]) < s->rw && (unsigned char) (ny + dir_y[d]) < s->rh &&
                        (rg (n, nx + dir_x[d], ny + dir_y[d])->flags & VISITED)) {
                        rg (n, nx, ny)->flags |= VISITED;
                        rp ();
                        k = 1;
                        break;
                    }
                }
            }
        }
    } while (k);
    todo = 0;
    for (ny = 0; ny < s->rh; ++ny) {
        for (nx = 0; nx < s->rw; ++nx) {
            r = rg (n, nx, ny);
            if (!(r->flags & VISITED)) {
                r->kind = RG_ROCK;
            } else {
                r->flags &= ~VISITED;
                ++todo;
            }
            rp ();
        }
    }
    /* The ways: a random walk over the halls, its first step into each a way (a tree that reaches them all) */
    rg (n, rx, ry)->flags |= VISITED;
    rp ();
    for (steps = 0, --todo; todo && steps < 60000U; ++steps) {
        d = random (4);
        nx = rx + dir_x[d];
        ny = ry + dir_y[d];
        if (nx >= s->rw || ny >= s->rh || rg (n, nx, ny)->kind != RG_HALLS) {
            continue;
        }
        if (!(R.flags & VISITED)) {
            R.flags |= VISITED;
            rp ();
            --todo;
            /* (The edge between: the southern or eastern region's north or west) */
            if (d == DIR_N) {
                k = way_at (edge_len ((unsigned) rx * RW, s->w));
                rg (n, rx, ry)->n = k;
            } else if (d == DIR_S) {
                k = way_at (edge_len ((unsigned) nx * RW, s->w));
                rg (n, nx, ny)->n = k;
            } else if (d == DIR_W) {
                k = way_at (edge_len ((unsigned) ry * RW, s->h));
                rg (n, rx, ry)->w = k;
            } else {
                k = way_at (edge_len ((unsigned) ny * RW, s->h));
                rg (n, nx, ny)->w = k;
            }
            rp ();
        }
        rx = nx;
        ry = ny;
    }
    /* A few more, for loops */
    for (ny = 0; ny < s->rh; ++ny) {
        for (nx = 0; nx < s->rw; ++nx) {
            r = rg (n, nx, ny);
            r->flags &= ~VISITED;
            rp ();
            if (r->kind != RG_HALLS) {
                continue;
            }
            if (ny && r->n == NO_WAY && rg (n, nx, ny - 1)->kind == RG_HALLS && !random (5)) {
                k = way_at (edge_len ((unsigned) nx * RW, s->w));
                rg (n, nx, ny)->n = k;
                rp ();
            }
            if (nx && rg (n, nx, ny)->w == NO_WAY && rg (n, nx - 1, ny)->kind == RG_HALLS && !random (5)) {
                k = way_at (edge_len ((unsigned) ny * RW, s->h));
                rg (n, nx, ny)->w = k;
                rp ();
            }
        }
    }
    s->made = 1;
}

/* ---- The strata: the one below a link, grown when anyone first takes it */

/* Stratum n's own links down (one or two, in its halls; none from the deepest), to stratum n + 1 */
void __fastcall__ o_links_down (unsigned char n)
{
    static unsigned char g[2];
    struct stratum* s = &STRATUM (n);
    unsigned char k, tries;
    unsigned o, x, y, d, best;
    struct link* l;

    hub_get (THEME_BANK, &THEME->goal, g, 2);   /* (A far exit's stratum, the goal's: a way out there, open, as far
                                                **   from the way in as it's found, a stair up to the daylight) */
    if (g[0] == G_THROUGH && g[1] == n && s->nlinks < NLINKS) {
        l = &s->link[s->nlinks];
        for (best = tries = 0; tries < 30; ++tries) {
            o = 2 + random (s->w - 4);
            x = s->x0 + INSET (o);
            o = 2 + random (s->h - 4);
            y = s->y0 + INSET (o);
            if (rg (n, (x - s->x0) >> 8, (y - s->y0) >> 8)->kind == RG_HALLS) {
                o = x > s->link[0].x ? x - s->link[0].x : s->link[0].x - x;
                d = y > s->link[0].y ? y - s->link[0].y : s->link[0].y - y;
                if ((d = (d > o ? d : o) + 1) > best) {
                    best = d;
                    l->x = x;
                    l->y = y;
                }
            }
        }
        if (best) {
            l->kind = O_WAY_OUT;
            l->to = 0xFF;
            l->dug = 0;
            ++s->nlinks;
        }
    }
    hub_get (0, HUB->savedir, &k, 1);
    if (n == 0xFE || n + 1 >= MAXSTRATA && !k) {
        return;                                 /* (The deepest: the hub's slots all wanted, without a save folder for
                                                **   those made way for, or the stratum's number a byte's) */
    }
    for (k = 1 + random (2); k && s->nlinks < NLINKS; --k) {
        for (tries = 0; tries < 30; ++tries) {
            l = &s->link[s->nlinks];
            o = 2 + random (s->w - 4);          /* (Off its chunk's edges, room for its rooms: world.h's INSET) */
            l->x = s->x0 + INSET (o);
            o = 2 + random (s->h - 4);
            l->y = s->y0 + INSET (o);
            if (rg (n, (l->x - s->x0) >> 8, (l->y - s->y0) >> 8)->kind == RG_HALLS) {
                l->kind = random (10) < 7 ? O_STAIRS_DOWN : O_LADDER_DOWN;
                l->to = n + 1;
                ++s->nlinks;
                break;
            }
        }
    }
}

/* ---- The theme's puzzle's pieces (theme.h): each one's stratum picked from the game's seed (the top stratum's: the
** same whichever stratum's grown first), and those in stratum n given their places as its plan is grown (its links
** down too): a chunk of a region of its halls, from the stratum's seed, so the same each time it's grown (a game
** carried on); in the hub's theme bank, where the stock finds them as it first makes a chunk (stock.c).  A piece
** whose stratum isn't grown yet has none.  (The places' own dice: the plan's and the links' are done) */
#pragma code-name (push, "OV5CODE")            /* (Overlay 5's: 4 full) */
#pragma rodata-name (push, "OV5RODATA")

/* Chunk (cx, cy) of stratum n is the era's works, or has them under it (its layer's, or the older one's) */
static unsigned char __fastcall__ era_here (unsigned char n, unsigned cx, unsigned cy, unsigned char era)
{
    static struct tlayer y[TLAYERS];
    unsigned char g = layer_at (n, cx, cy);

    hub_get (THEME_BANK, THEME->layer[n < TSTRATA ? n : TSTRATA - 1], y, sizeof y);
    return y[g & SG_LAYER].era == era || ((g & SG_HASUNDER) && y[(g & SG_UNDER) >> 4].era == era);
}

void __fastcall__ o_place_pieces (unsigned char n)
{
    static struct tpiece tp;
    static struct tplace pl;
    struct stratum* s = &STRATUM (n);
    unsigned char p, np, tries, rx, ry, sn[NPIECES], era;
    unsigned kw, kh;

    hub_get (THEME_BANK, &THEME->npieces, &np, 1);
    rseed (STRATUM (0).seed ^ 0x50494543UL);
    for (p = 0; p < np && p < NPIECES; ++p) {
        hub_get (THEME_BANK, &THEME->piece[p], &tp, sizeof tp);
        sn[p] = tp.d0 + random (tp.d1 - tp.d0 + 1);
    }
    for (p = 0; p < np && p < NPIECES; ++p) {
        if (sn[p] != n) {
            continue;
        }
        rseed (s->seed ^ (0x51554553UL + p * 7919UL));
        memset (&pl, 0, sizeof pl);
        pl.sn = n;
        if (s->nlinks) {                        /* (At worst, the chunk of its first link: halls) */
            pl.cx = (s->link[0].x - s->x0) / CW;
            pl.cy = (s->link[0].y - s->y0) / CW;
        }
        hub_get (THEME_BANK, &THEME->pera[p], &era, 1);
        for (tries = 0; tries < 40; ++tries) {
            rx = random (s->rw);
            ry = random (s->rh);
            if (rg (n, rx, ry)->kind == RG_HALLS) {
                kw = (s->w - (unsigned) rx * RW + CW - 1) / CW;      /* (Its chunks in the stratum, each way) */
                kh = (s->h - (unsigned) ry * RW + CW - 1) / CW;
                pl.cx = (unsigned) rx * RCH + random (kw > RCH ? RCH : kw);
                pl.cy = (unsigned) ry * RCH + random (kh > RCH ? RCH : kh);
                if (!era || tries >= 30 || era_here (n, pl.cx, pl.cy, era)) {
                    break;                      /* (A piece of an era's among that era's works: HISTORY.md) */
                }
            }
        }
        hub_put (THEME_BANK, &THEME_PLACE[p], &pl, sizeof pl);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- A region's own ways between its chunks: (the last asked for's) each chunk's north and west edges', kept in
** the cache's bank (detail_bank: changes.c's), a place for each region by its number */

#define DCACHE      (0x2000 / sizeof (struct detail))
static struct detail {
    unsigned char   used, sn, rx, ry;
    unsigned char   n[RCH][RCH], w[RCH][RCH];
} D;

static void __fastcall__ detail (unsigned char sn, unsigned char rx, unsigned char ry)
{
    struct stratum* s = &STRATUM (sn);
    struct detail* at = (struct detail*) 0x8000 + ((unsigned) rx * 31 + (unsigned) ry * 17 + sn * 7) % DCACHE;
    unsigned char ix, iy, nx, ny, d, kw, kh, seen[RCH];
    unsigned todo, steps;

    if (D.used && D.sn == sn && D.rx == rx && D.ry == ry) {
        return;
    }
    bread (detail_bank, at, &D, sizeof D);      /* (Worked out before?) */
    if (D.used && D.sn == sn && D.rx == rx && D.ry == ry) {
        return;
    }
    D.used = 1;
    D.sn = sn;
    D.rx = rx;
    D.ry = ry;
    memset (D.n, NO_WAY, sizeof D.n);
    memset (D.w, NO_WAY, sizeof D.w);
    if (rg (sn, rx, ry)->kind == RG_ROCK) {
        bwrite (detail_bank, at, &D, sizeof D);
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
                D.n[ix][iy] = 2 + random (CW - 4);
            } else if (d == DIR_S) {
                D.n[nx][ny] = 2 + random (CW - 4);
            } else if (d == DIR_W) {
                D.w[ix][iy] = 2 + random (CW - 4);
            } else {
                D.w[nx][ny] = 2 + random (CW - 4);
            }
        }
        ix = nx;
        iy = ny;
    }
    for (iy = 0; iy < kh; ++iy) {                /* (A few more, for loops) */
        for (ix = 0; ix < kw; ++ix) {
            if (iy && D.n[ix][iy] == NO_WAY && !random (4)) {
                D.n[ix][iy] = 2 + random (CW - 4);
            }
            if (ix && D.w[ix][iy] == NO_WAY && !random (4)) {
                D.w[ix][iy] = 2 + random (CW - 4);
            }
        }
    }
    bwrite (detail_bank, at, &D, sizeof D);
}

/* The way across chunk (cx, cy)'s north edge (west: across its west), or NO_WAY */
static unsigned char __fastcall__ edge_way (unsigned char sn, unsigned cx, unsigned cy, unsigned char west)
{
    struct stratum* s = &STRATUM (sn);
    unsigned char rx = cx / RCH, ry = cy / RCH, ix = cx % RCH, iy = cy % RCH, p;
    unsigned len;

    if (cx >= (s->w + CW - 1) / CW || cy >= (s->h + CW - 1) / CW || (west ? !cx : !cy)) {
        return NO_WAY;
    }
    if (west ? !ix : !iy) {                     /* (A region's edge: the plan's) */
        p = west ? rg (sn, rx, ry)->w : rg (sn, rx, ry)->n;
        if (p == NO_WAY || p / CW != (west ? iy : ix)) {
            return NO_WAY;
        }
        p %= CW;
    } else {
        detail (sn, rx, ry);
        p = west ? D.w[ix][iy] : D.n[ix][iy];
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

/* Chunk (cx, cy)'s edges: the ways across them, the ground at its corners, its squares in the bounds, the links in
** it (x1 and y1 0xFF: solid rock, nothing to grow) */
void __fastcall__ o_chunk_edges (unsigned char sn, unsigned cx, unsigned cy, struct edges* e)
{
    struct stratum* s = &STRATUM (sn);
    unsigned x = s->x0 + cx * CW, y = s->y0 + cy * CW, w;
    unsigned char i;

    e->way[DIR_N] = edge_way (sn, cx, cy, 0);
    e->way[DIR_W] = edge_way (sn, cx, cy, 1);
    e->way[DIR_S] = edge_way (sn, cx, cy + 1, 0);
    e->way[DIR_E] = edge_way (sn, cx + 1, cy, 1);
    e->ground[0] = ground (sn, x, y);
    e->ground[1] = ground (sn, x + CW - 1, y);
    e->ground[2] = ground (sn, x, y + CW - 1);
    e->ground[3] = ground (sn, x + CW - 1, y + CW - 1);
    w = s->w - cx * CW;
    e->x0 = 0;
    e->x1 = w >= CW ? CW - 1 : w - 1;
    w = s->h - cy * CW;
    e->y0 = 0;
    e->y1 = w >= CW ? CW - 1 : w - 1;
    e->nland = 0;
    for (i = 0; i < s->nlinks && e->nland < 4; ++i) {
        if (!s->link[i].dug && s->link[i].x - x < CW && s->link[i].y - y < CW) {   /* (A dug one's the records') */
            e->lx[e->nland] = s->link[i].x - x;
            e->ly[e->nland] = s->link[i].y - y;
            e->ln[e->nland++] = i;
        }
    }
    if (rg (sn, cx / RCH, cy / RCH)->kind == RG_ROCK && !e->nland) {
        e->x1 = e->y1 = 0xFF;                   /* (Solid rock: nothing grown) */
    }
}
