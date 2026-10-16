/*
** caves.c - (dq0m, overlay 3) a natural chunk grown (DUNGEON.md: the natural style), in the main RAM, irregular as
** water and time leave rock (the user, 2026-10-16: "not straight tunnels, not rectangular rooms"): tunnels from each
** way across the chunk's edges (and each link's square) winding toward its middle, running on a while and bending,
** wiggling, now narrower and now wider, with pockets in their sides and blind forks off them; chambers, ragged, one
** at the middle and some along the way, each grown by a wanderer from its middle within a box of its own (kept clear
** of the other chambers', so each square's chamber is the one whose bounds it's in), widening and narrowing as the
** wanderer went (each an area: a room's record, the bounds of its floor, one zone, a rough high ceiling; now and then
** a glow); the floors the ground's, with an unevenness that fades to nothing at the chunk's edges, so the next
** chunk's tunnels meet them, and a chamber's floor a hollow (or a hump), smoothly; the rock beside what's open, the
** caves' walls.  (Water's shaping, and age's, are the next steps'.)
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

#include <string.h>
#include <dice.h>
#include "../dq0w/world.h"
#include "grow.h"

static signed char bump[5][5];                  /* The unevenness: a lattice 8 squares apart, 0 at the edges */
static unsigned char bx0, by0, bx1, by1;        /* A chamber's box (chamber ()'s) */

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

/* Within the stratum, off the chunk's edges (only the ways cross them) */
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

/* A tunnel's square opened beside (x, y), off the edges */
static void __fastcall__ open_in (unsigned char x, unsigned char y)
{
    if (inside (x, y)) {
        open (x, y, T_CORR);
    }
}

/* The way from (x, y) toward (tx, ty): along the farther, else the nearer, so long as it stays off the edges (or is
** the end) */
static unsigned char tx, ty;
static unsigned char __fastcall__ toward (unsigned char x, unsigned char y)
{
    unsigned char h = x < tx ? DIR_E : DIR_W, v = y < ty ? DIR_S : DIR_N, d;

    d = x == tx || (y != ty && (y > ty ? y - ty : ty - y) > (x > tx ? x - tx : tx - x)) ? v : h;
    if (!inside (x + dir_x[d], y + dir_y[d]) && (x + dir_x[d] != tx || y + dir_y[d] != ty)) {
        d = d == h ? v : h;
    }
    return d;
}

/* A blind way off a tunnel (a fork that ends, a pocket in its side): n squares from (x, y), setting off d, wandering */
static void __fastcall__ spur (unsigned char x, unsigned char y, unsigned char d, unsigned char n)
{
    for (; n; --n) {
        if (!random (4)) {
            d = (d + 1 + (random (2) << 1)) & 3;
        }
        x += dir_x[d];
        y += dir_y[d];
        if (!inside (x, y)) {
            return;
        }
        open (x, y, T_CORR);
    }
}

/* A tunnel from (x, y) to (tx, ty), winding: on the way it's heading a while, then bending (a bend toward the end
** more often than away), some steps aside; its width wandering (narrow: one square, now and then two; wide: two or
** three), a pocket in its side or a blind fork now and then; and straight on to the end if it's wandered too long */
static void __fastcall__ tunnel (unsigned char x, unsigned char y, unsigned char wide)
{
    unsigned char d, n, b, w = wide;

    open (x, y, T_CORR);
    d = toward (x, y);
    for (n = 0; (x != tx || y != ty) && n < 120; ++n) {
        b = random (16);
        if (b < 4 || (b < 11 && (dir_x[d] * ((int) tx - x) + dir_y[d] * ((int) ty - y)) < 0)) {
            d = toward (x, y);                  /* (Toward the end; and back from heading away, mostly) */
        } else if (b < 8) {
            d = (d + 1 + ((b & 1) << 1)) & 3;   /* (A bend) */
        }
        if (!inside (x + dir_x[d], y + dir_y[d]) && (x + dir_x[d] != tx || y + dir_y[d] != ty)) {
            d = toward (x, y);
            continue;
        }
        x += dir_x[d];
        y += dir_y[d];
        open (x, y, T_CORR);
        if (w) {                                /* (Its width: a side, or both) */
            open_in (x + dir_y[d], y + dir_x[d]);
            if (w > 1) {
                open_in (x - dir_y[d], y - dir_x[d]);
            }
        }
        if (!random (6)) {
            w = wide ? 1 + random (2) : random (4) == 3;
        }
        if (!random (20)) {                     /* (A pocket, or a blind fork) */
            spur (x, y, (d + 1 + (random (2) << 1)) & 3, random (3) ? 1 + random (3) : 5 + random (8));
        }
    }
    while (x != tx || y != ty) {
        d = toward (x, y);
        x += dir_x[d];
        y += dir_y[d];
        open (x, y, T_CORR);
    }
}

/* A chamber's square opened, within its box */
static void __fastcall__ carve (unsigned char x, unsigned char y)
{
    if (x >= bx0 && x <= bx1 && y >= by0 && y <= by1) {
        open (x, y, T_FLOOR);
    }
}

/* A chamber round (cx, cy), up to hw squares east and west of it, hh north and south (its box: smaller till it's
** clear of the other chambers' bounds, a square between): a wanderer from its middle, drawn back toward it the
** farther it's gone, opening what it treads and now and then about it; a room's record of what's open */
static void __fastcall__ chamber (unsigned char cx, unsigned char cy, unsigned char hw, unsigned char hh)
{
    struct room* p;
    struct zone* z;
    unsigned char x, y, x0 = 0xFF, y0 = 0xFF, x1 = 0, y1 = 0, n, d, r, ax, ay;

    if (L->nrooms >= NROOMS || L->nzones >= NZONES) {
        return;
    }
    for (;;) {
        bx0 = cx > E->x0 + hw ? cx - hw : E->x0 + 1;
        bx1 = cx + hw < E->x1 ? cx + hw : E->x1 - 1;
        by0 = cy > E->y0 + hh ? cy - hh : E->y0 + 1;
        by1 = cy + hh < E->y1 ? cy + hh : E->y1 - 1;
        for (r = 0, p = L->room; r < L->nrooms; ++r, ++p) {
            if (bx0 <= p->x + p->w && bx1 + 1 >= p->x && by0 <= p->y + p->h && by1 + 1 >= p->y) {
                break;
            }
        }
        if (r == L->nrooms) {
            break;
        }
        if (hw < 2) {                           /* (No room for it) */
            return;
        }
        --hw;
        hh -= hh > 1;
    }
    x = cx;
    y = cy;
    carve (x, y);
    for (n = (bx1 - bx0 + 1) * (by1 - by0 + 1) * 2; n; --n) {
        ax = x > cx ? x - cx : cx - x;
        ay = y > cy ? y - cy : cy - y;
        if (random (hw + hh) < ax + ay) {       /* (Drawn back: along the way it's farther out, for its box) */
            d = ax * hh >= ay * hw ? (x > cx ? DIR_W : DIR_E) : (y > cy ? DIR_N : DIR_S);
        } else {
            d = random (4);
        }
        if (x + dir_x[d] < bx0 || x + dir_x[d] > bx1 || y + dir_y[d] < by0 || y + dir_y[d] > by1) {
            continue;
        }
        x += dir_x[d];
        y += dir_y[d];
        carve (x, y);
        if (!random (3)) {                      /* (And about it) */
            for (d = 0; d < 4; ++d) {
                carve (x + dir_x[d], y + dir_y[d]);
            }
        }
    }
    for (y = by0; y <= by1; ++y) {              /* (Its floor's bounds) */
        for (x = bx0; x <= bx1; ++x) {
            if (L->tile[y][x] == T_FLOOR) {
                x0 = x < x0 ? x : x0;
                y0 = y < y0 ? y : y0;
                x1 = x > x1 ? x : x1;
                y1 = y > y1 ? y : y1;
            }
        }
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

/* A chamber's floor a hollow (or a hump): deepest at its bounds' middle, nothing past them, smoothly (what's open in
** them: its floor, a tunnel through it) */
static void __fastcall__ hollow (const struct room* p)
{
    unsigned char x, y;
    int hw = p->w / 2 + 1, hh = p->h / 2 + 1, dx, dy, deep = (int) random (p->h / 3 + 2) - (random (4) == 0) * 2;

    for (y = p->y; y < p->y + p->h; ++y) {
        for (x = p->x; x < p->x + p->w; ++x) {
            if (L->tile[y][x] != T_ROCK) {
                dx = (int) x - (p->x + p->w / 2);
                dy = (int) y - (p->y + p->h / 2);
                L->elev[y][x] -= deep * (hw * hw - dx * dx) / (hw * hw) * (hh * hh - dy * dy) / (hh * hh);
            }
        }
    }
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
    tx = mx;
    ty = my;
    for (i = 0; i < e->nland; ++i) {            /* (The links: each in a chamber, a tunnel to the middle) */
        chamber (e->lx[i], e->ly[i], 2 + random (2), 1 + random (2));
        tunnel (e->lx[i], e->ly[i], random (2));
    }
    chamber (mx, my, 3 + random (3), 2 + random (2));
    if (e->way[DIR_N] != NO_WAY) {
        tunnel (e->way[DIR_N], 0, random (3) < SP->wide);
    }
    if (e->way[DIR_S] != NO_WAY) {
        tunnel (e->way[DIR_S], CW - 1, random (3) < SP->wide);
    }
    if (e->way[DIR_W] != NO_WAY) {
        tunnel (0, e->way[DIR_W], random (3) < SP->wide);
    }
    if (e->way[DIR_E] != NO_WAY) {
        tunnel (CW - 1, e->way[DIR_E], random (3) < SP->wide);
    }
    for (k = random (SP->chambers ? SP->chambers : 1); k; --k) {   /* (Chambers more, along the tunnels) */
        for (n = 0; n < 40; ++n) {
            x = 2 + random (CW - 4);
            y = 2 + random (CW - 4);
            if (inside (x, y) && L->tile[y][x] == T_CORR) {
                chamber (x, y, 2 + random (4), 1 + random (3));
                break;
            }
        }
    }
    /* The floors, the chambers' hollows, and the walls: the rock beside what's open */
    for (y = 0; y < CW; ++y) {
        for (x = 0; x < CW; ++x) {
            if (L->tile[y][x] != T_ROCK) {
                L->elev[y][x] = floor_at (x, y);
            }
        }
    }
    for (i = 0; i < L->nrooms; ++i) {
        hollow (&L->room[i]);
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
