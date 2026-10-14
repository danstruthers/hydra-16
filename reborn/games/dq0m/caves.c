/*
** caves.c - (dq0m, overlay 3) a natural chunk grown (DUNGEON.md: the natural style), in the main RAM: tunnels from
** each way across the chunk's edges (and each link's square) wandering toward its middle, one square wide or three;
** chambers, irregular, one at the middle and some along the way (each an area: a room's record, its bounds, one
** zone, its floor from the stratum's ground, a rough high ceiling; now and then a glow); the floors the ground's,
** with an unevenness that fades to nothing at the chunk's edges, so the next chunk's tunnels meet them; the rock
** beside what's open, the caves' walls.  (Water's shaping, and age's, are the next steps'.)
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

#include <string.h>
#include <dice.h>
#include "../dq0w/world.h"
#include "grow.h"

static signed char bump[5][5];                  /* The unevenness: a lattice 8 squares apart, 0 at the edges */

/* The stratum's ground at a square of the chunk (its corners', evenly between), and its unevenness */
static int __fastcall__ floor_at (unsigned char x, unsigned char y)
{
    long top = (long) E->ground[0] * (CW - 1 - x) + (long) E->ground[1] * x;
    long bottom = (long) E->ground[2] * (CW - 1 - x) + (long) E->ground[3] * x;
    int g = (int) ((top * (CW - 1 - y) + bottom * y) / ((CW - 1) * (CW - 1)));
    unsigned char i = x >> 3, j = y >> 3, fx = x & 7, fy = y & 7;
    int a = bump[j][i] * (8 - fx) + bump[j][i + 1] * fx, b = bump[j + 1][i] * (8 - fx) + bump[j + 1][i + 1] * fx;

    return g + (a * (8 - fy) + b * fy) / 64;
}

static unsigned char __fastcall__ inside (unsigned char x, unsigned char y)
{
    return x > E->x0 && x < E->x1 && y > E->y0 && y < E->y1;
}

static void __fastcall__ open (unsigned char x, unsigned char y, unsigned char t)
{
    if (x <= E->x1 && y <= E->y1 && (L->tile[y][x] == T_ROCK || (t == T_FLOOR && L->tile[y][x] == T_CORR))) {
        L->tile[y][x] = t;
    }
}

/* A tunnel from (x, y) to (tx, ty), wandering: most steps toward it, some aside; wide: three squares across */
static void __fastcall__ tunnel (unsigned char x, unsigned char y, unsigned char tx, unsigned char ty,
    unsigned char wide)
{
    unsigned char d, n, nx, ny;

    open (x, y, T_CORR);
    for (n = 0; (x != tx || y != ty) && n < 200; ++n) {
        if (random (10) < 7) {                  /* (Toward it, along the farther way) */
            d = (x > tx ? x - tx : tx - x) >= (y > ty ? y - ty : ty - y) ? (x < tx ? DIR_E : DIR_W) :
                (y < ty ? DIR_S : DIR_N);
        } else {
            d = random (4);
        }
        nx = x + dir_x[d];
        ny = y + dir_y[d];
        if (!inside (nx, ny) && (nx != tx || ny != ty)) {
            continue;
        }
        x = nx;
        y = ny;
        open (x, y, T_CORR);
        if (wide) {
            open (x + dir_y[d], y + dir_x[d], T_CORR);
            open (x - dir_y[d], y - dir_x[d], T_CORR);
        }
    }
}

/* A chamber round (cx, cy), radius about r: its squares opened, a room's record of it */
static void __fastcall__ chamber (unsigned char cx, unsigned char cy, unsigned char r)
{
    struct room* p;
    struct zone* z;
    unsigned char x, y, x0 = 0xFF, y0 = 0xFF, x1 = 0, y1 = 0;
    int dx, dy;

    for (y = cy > r + 1 ? cy - r - 1 : 0; y <= cy + r + 1 && y < CW; ++y) {
        for (x = cx > r + 2 ? cx - r - 2 : 0; x <= cx + r + 2 && x < CW; ++x) {
            dx = (int) x - cx;
            dy = ((int) y - cy) * 3 / 2;        /* (Wider than deep, as caves go) */
            if (inside (x, y) && dx * dx + dy * dy <= (int) (r * r) + (int) random (r + 2) - 1) {
                open (x, y, T_FLOOR);
                x0 = x < x0 ? x : x0;
                y0 = y < y0 ? y : y0;
                x1 = x > x1 ? x : x1;
                y1 = y > y1 ? y : y1;
            }
        }
    }
    if (x0 == 0xFF || L->nrooms >= NROOMS || L->nzones >= NZONES) {
        return;
    }
    p = &L->room[L->nrooms];
    p->x = x0;
    p->y = y0;
    p->w = x1 - x0 + 1;
    p->h = y1 - y0 + 1;
    p->flags = random (10) < SP->glow ? R_LIT : 0;    /* (A glow: the style's say) */
    p->kind = 6;                                /* (A cave) */
    p->z0 = L->nzones;
    p->nz = 1;
    z = &L->zone[L->nzones++];
    z->x = x0;
    z->y = y0;
    z->w = p->w;
    z->h = p->h;
    z->room = L->nrooms++;
    z->z = floor_at (cx, cy);
    z->gx = z->gy = 0;
    z->ceil = C_ROUGH;
    z->cmid = 10 + random (30);
    z->cwall = z->cmid - random (4);
}

void __fastcall__ o_grow_caves (struct chunk* c, const struct edges* e, unsigned long seed)
{
    struct stratum* s = &strata[c->sn];
    unsigned char i, j, k, x, y, n;

    L = c;
    E = e;
    rseed (seed);
    for (j = 0; j < 5; ++j) {
        for (i = 0; i < 5; ++i) {
            bump[j][i] = !i || !j || i == 4 || j == 4 ? 0 : (signed char) random (9) - 4;
        }
    }
    mx = e->x0 + 1 + random (e->x1 > e->x0 + 2 ? e->x1 - e->x0 - 1 : 1);   /* (The middle: where the tunnels meet) */
    my = e->y0 + 1 + random (e->y1 > e->y0 + 2 ? e->y1 - e->y0 - 1 : 1);
    chamber (mx, my, 2 + random (4));
    for (i = 0; i < e->nland; ++i) {            /* (The links: each in a chamber, a tunnel to the middle) */
        chamber (e->lx[i], e->ly[i], 2 + random (2));
        tunnel (e->lx[i], e->ly[i], mx, my, random (2));
    }
    if (e->way[DIR_N] != NO_WAY) {
        tunnel (e->way[DIR_N], 0, mx, my, random (3) < SP->wide);
    }
    if (e->way[DIR_S] != NO_WAY) {
        tunnel (e->way[DIR_S], CW - 1, mx, my, random (3) < SP->wide);
    }
    if (e->way[DIR_W] != NO_WAY) {
        tunnel (0, e->way[DIR_W], mx, my, random (3) < SP->wide);
    }
    if (e->way[DIR_E] != NO_WAY) {
        tunnel (CW - 1, e->way[DIR_E], mx, my, random (3) < SP->wide);
    }
    for (k = random (SP->chambers ? SP->chambers : 1); k; --k) {   /* (Chambers more, along the tunnels) */
        for (n = 0; n < 40; ++n) {
            x = 2 + random (CW - 4);
            y = 2 + random (CW - 4);
            if (inside (x, y) && L->tile[y][x] == T_CORR) {
                chamber (x, y, 2 + random (3));
                break;
            }
        }
    }
    /* The floors, and the walls: the rock beside what's open */
    for (y = 0; y < CW; ++y) {
        for (x = 0; x < CW; ++x) {
            if (L->tile[y][x] != T_ROCK) {
                L->elev[y][x] = floor_at (x, y);
            }
        }
    }
    for (y = 0; y < CW; ++y) {
        for (x = 0; x < CW; ++x) {
            if (L->tile[y][x] != T_ROCK) {
                continue;
            }
            for (j = y ? y - 1 : 0, n = 0; j <= y + 1 && j < CW && !n; ++j) {
                for (i = x ? x - 1 : 0; i <= x + 1 && i < CW; ++i) {
                    if (TILE (L->tile[j][i]) == T_FLOOR || TILE (L->tile[j][i]) == T_CORR) {
                        n = 1;
                        break;
                    }
                }
            }
            if (n) {
                L->tile[y][x] = T_WALL;
            }
        }
    }
    /* The links' squares: stairways and ladders, up and down */
    for (i = 0; i < e->nland && L->nopen < NOPEN; ++i) {
        x = e->lx[i];
        y = e->ly[i];
        k = s->link[e->ln[i]].kind;
        L->tile[y][x] = k == O_STAIRS_DOWN || k == O_LADDER_DOWN || k == O_SHAFT_DOWN ? T_DOWN : T_UP;
        L->open[L->nopen].kind = k;
        L->open[L->nopen].x = x;
        L->open[L->nopen].y = y;
        L->open[L->nopen].sill = 0;
        L->open[L->nopen++].link = e->ln[i];
    }
}
