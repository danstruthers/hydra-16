/*
** plan.c - (dq0m) a stratum grown whole, in outline (DUNGEON.md), as anyone first comes to it: its plan, a grid of
** regions (8 by 8 chunks each) in the hub's plans' banks, each region's kind, the lie of the rock at its corner,
** and the ways across its edges; and its links down to the stratum below.  Purpose-built strata so far: the regions halls, a few solid rock; the ways a tree
** that reaches every hall (a random walk's: the first step into each region is a way), and a few more for loops;
** the rock's lie a walk too, each region near its neighbours'.  The regions none of the halls reach become rock.
** (The populations, the water and the features come with their steps.)
*/

#include <string.h>
#include <dice.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"

struct region* __fastcall__ region (unsigned char sn, unsigned char rx, unsigned char ry)
{
    hub_map (PLAN_BANK (sn));
    return PLAN_AT (sn) + (unsigned) ry * strata[sn].rw + rx;
}

/* A region's corner's lie (feet), the plan's edge's for a corner past it */
static int __fastcall__ corner (unsigned char sn, unsigned char rx, unsigned char ry)
{
    struct stratum* s = &strata[sn];

    return 2 * region (sn, rx < s->rw ? rx : s->rw - 1, ry < s->rh ? ry : s->rh - 1)->z;
}

int __fastcall__ ground (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &strata[sn];
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

void __fastcall__ plan_stratum (unsigned char n)
{
    struct stratum* s = &strata[n];
    struct region* r;
    struct region* q;
    unsigned char rx, ry, i, k, nx, ny, d;
    unsigned todo, steps;
    int z;

    s->rw = (s->w + RW - 1) / RW;
    s->rh = (s->h + RW - 1) / RW;
    while ((unsigned) s->rw * s->rh > PLAN_MAX) {     /* (A plan as big as its bank's half holds) */
        if (s->rw > s->rh) {
            --s->rw;
            s->w = s->rw * RW;
        } else {
            --s->rh;
            s->h = s->rh * RW;
        }
    }
    rseed (s->seed);
    hub_map (PLAN_BANK (n));
    memset (PLAN_AT (n), 0, 0x1000);
    /* Kinds, and the rock's lie: each region near the average of those north and west of it */
    for (ry = 0; ry < s->rh; ++ry) {
        for (rx = 0; rx < s->rw; ++rx) {
            z = 0;
            k = 0;
            if (rx) {
                z += region (n, rx - 1, ry)->z;
                ++k;
            }
            if (ry) {
                z += region (n, rx, ry - 1)->z;
                ++k;
            }
            z = (k ? z / k : 0) + (int) random (9) - 4;
            r = region (n, rx, ry);
            r->z = z > 50 ? 50 : z < -50 ? -50 : z;
            r->kind = s->rw * s->rh > 1 && !random (6) ? RG_ROCK : RG_HALLS;
            r->n = r->w = NO_WAY;
        }
    }
    for (i = 0; i < s->nlinks; ++i) {           /* (A link's region is halls) */
        region (n, (s->link[i].x - s->x0) >> 8, (s->link[i].y - s->y0) >> 8)->kind = RG_HALLS;
    }
    /* The halls the first link's region reaches (repeated sweeps), the rest rock */
    rx = s->nlinks ? (s->link[0].x - s->x0) >> 8 : 0;
    ry = s->nlinks ? (s->link[0].y - s->y0) >> 8 : 0;
    region (n, rx, ry)->flags |= VISITED;
    do {
        k = 0;
        for (ny = 0; ny < s->rh; ++ny) {
            for (nx = 0; nx < s->rw; ++nx) {
                r = region (n, nx, ny);
                if (r->kind != RG_HALLS || (r->flags & VISITED)) {
                    continue;
                }
                for (d = 0; d < 4; ++d) {
                    if ((unsigned char) (nx + dir_x[d]) < s->rw && (unsigned char) (ny + dir_y[d]) < s->rh &&
                        (region (n, nx + dir_x[d], ny + dir_y[d])->flags & VISITED)) {
                        region (n, nx, ny)->flags |= VISITED;
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
            r = region (n, nx, ny);
            if (!(r->flags & VISITED)) {
                r->kind = RG_ROCK;
            } else {
                r->flags &= ~VISITED;
                ++todo;
            }
        }
    }
    /* The ways: a random walk over the halls, its first step into each a way (a tree that reaches them all) */
    region (n, rx, ry)->flags |= VISITED;
    for (steps = 0, --todo; todo && steps < 60000U; ++steps) {
        d = random (4);
        nx = rx + dir_x[d];
        ny = ry + dir_y[d];
        if (nx >= s->rw || ny >= s->rh || region (n, nx, ny)->kind != RG_HALLS) {
            continue;
        }
        q = region (n, nx, ny);
        if (!(q->flags & VISITED)) {
            q->flags |= VISITED;
            --todo;
            /* (The edge between: the southern or eastern region's north or west) */
            if (d == DIR_N) {
                region (n, rx, ry)->n = way_at (edge_len ((unsigned) rx * RW, s->w));
            } else if (d == DIR_S) {
                region (n, nx, ny)->n = way_at (edge_len ((unsigned) nx * RW, s->w));
            } else if (d == DIR_W) {
                region (n, rx, ry)->w = way_at (edge_len ((unsigned) ry * RW, s->h));
            } else {
                region (n, nx, ny)->w = way_at (edge_len ((unsigned) ny * RW, s->h));
            }
        }
        rx = nx;
        ry = ny;
    }
    /* A few more, for loops */
    for (ny = 0; ny < s->rh; ++ny) {
        for (nx = 0; nx < s->rw; ++nx) {
            r = region (n, nx, ny);
            r->flags &= ~VISITED;
            if (r->kind != RG_HALLS) {
                continue;
            }
            if (ny && r->n == NO_WAY && region (n, nx, ny - 1)->kind == RG_HALLS && !random (5)) {
                region (n, nx, ny)->n = way_at (edge_len ((unsigned) nx * RW, s->w));
            }
            if (nx && region (n, nx, ny)->w == NO_WAY && region (n, nx - 1, ny)->kind == RG_HALLS && !random (5)) {
                region (n, nx, ny)->w = way_at (edge_len ((unsigned) ny * RW, s->h));
            }
        }
    }
    s->made = 1;
}

/* ---- The strata: the one below a link, grown when anyone first takes it */

/* Stratum n's own links down (one or two, in its halls; none from the deepest), to stratum n + 1 */
void __fastcall__ links_down (unsigned char n)
{
    struct stratum* s = &strata[n];
    unsigned char k, tries;
    struct link* l;

    if (n + 1 >= MAXSTRATA) {
        return;
    }
    for (k = 1 + random (2); k && s->nlinks < NLINKS; --k) {
        for (tries = 0; tries < 30; ++tries) {
            l = &s->link[s->nlinks];
            l->x = s->x0 + 2 + random (s->w - 4);
            l->y = s->y0 + 2 + random (s->h - 4);
            if (region (n, (l->x - s->x0) >> 8, (l->y - s->y0) >> 8)->kind == RG_HALLS) {
                l->kind = random (10) < 7 ? O_STAIRS_DOWN : O_LADDER_DOWN;
                l->to = n + 1;
                ++s->nlinks;
                break;
            }
        }
    }
}
