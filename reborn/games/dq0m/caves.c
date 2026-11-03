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
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
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
** the end); always nearer (the tunnel's last stretch straight on to its end) */
#define tx  mx                                  /* (The tunnels' end: the chunk's middle, built.c's mx, my) */
#define ty  my
static unsigned char __fastcall__ toward (unsigned char x, unsigned char y)
{
    unsigned char h = x < tx ? DIR_E : DIR_W, v = y < ty ? DIR_S : DIR_N, d;

    d = x == tx || (y != ty && (y > ty ? y - ty : ty - y) > (x > tx ? x - tx : tx - x)) ? v : h;
    if (!inside (x + dir_x[d], y + dir_y[d]) && (x + dir_x[d] != tx || y + dir_y[d] != ty) && x != tx && y != ty) {
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
        if (bx0 > bx1 || by0 > by1 || bx1 >= CW || by1 >= CW) {
            return;                             /* (The stratum's bounds too near: no room inside them) */
        }
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
    x = cx < bx0 ? bx0 : cx > bx1 ? bx1 : cx; /* (Its middle in its box: a link's square by the chunk's edge) */
    y = cy < by0 ? by0 : cy > by1 ? by1 : cy;
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
    if (x0 == 0xFF) {                           /* (Nothing opened) */
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

/* ---- A great cavern (slice 4.10's rest, 2026-11-02: DUNGEON.md's caverns across many chunks, HOUSE.md's great
** caverns, for review): a natural stratum's region (one in three, by its own numbers: a hash of the stratum's seed and
** the region's place, none of the grower's dice; none where a way between strata comes into the region, so the strata's
** ways in stay as they were, but dq0 -V, the tests', every region one, its middle as near its first way's square as the
** region lets it) holds a cavern in a box of its own: its middle and its half-breadths (24 to 63 squares east and west,
** 20 to 55 north and south; 10 squares or more off the region's edges, so no two meet), its floor where the ellipse's
** reach (the squared distance, 64 at its edge) is under 44 and a wander (0-15, a lattice 8 squares apart, evenly
** between) more; a pillar now and then (a lattice point's two by two squares, 4 off the wander's, one in six, well in);
** its floor a hollow (2 to 8 feet at its middle, nothing at its edge, the tunnels in its reach too); its roof 40 to 99
** feet up.  Each chunk it touches grows its part of the one room (struct room's R_CAVERN: the part's bounds) from the
** seed and the square alone, so the parts meet at the edges, the walls too (a square at the edge beside the next
** chunk's floor); the chunk's middle, where its tunnels meet, the part's square nearest its own */
#pragma code-name (push, "OV8CODE")            /* (Overlay 8's, its own: 3 full) */
#pragma rodata-name (push, "OV8RODATA")
struct cav {
    unsigned        seed;
    int             x, y;                       /* Its middle, in the chunk's squares (off it as it may be) */
    unsigned char   hw, hh, deep, roof;
    unsigned char   nz[7][7];                   /* The wander's lattice, from 8 squares before the chunk's corner */
    int             dx, dy;                     /* (cav_at's own) */
    unsigned        ax, ay, r, px, py;
    unsigned char   gx, gy, fx, fy, n, f;
};
struct cw {                                     /* (cavern's and cav_part's own) */
    struct stratum* s;
    struct link*    l;
    struct room*    p;
    struct zone*    z;
    unsigned        ox, oy, ew, eh, lx, ly;
    int             a, b;
    unsigned char   rx, ry, i, j, x, y, c, x0, y0, x1, y1, d, best;
};
extern unsigned char sx[], sy[];                /* (built.c's stack: idle as a cave's grown, its room the cavern's) */
#define V   (*(struct cav*) sx)
#define W   (*(struct cw*) sy)

static unsigned char __fastcall__ ch (unsigned a, unsigned b)
{
    unsigned h = a * 40503U + b * 2654U + V.seed;

    h ^= h >> 7;
    h *= 0x2F1B;
    return (unsigned char) (h >> 8);
}

/* Chunk L's region's great cavern: 1 if it has one that reaches the chunk (or a square past its edges), V its
** numbers */
static unsigned char cv_find (void)
{
    W.s = &STRATUM (L->sn);
    if (W.s->style != ST_NATURAL) {
        return 0;
    }
    W.rx = L->cx >> 3;
    W.ry = L->cy >> 3;
    W.ox = (unsigned) W.rx << 8;
    W.oy = (unsigned) W.ry << 8;
    hub_get (0, &HUB->gear, &V.f, 1);
    V.f &= GE_CAVERN;
    V.seed = (unsigned) W.s->seed ^ (unsigned) (W.s->seed >> 16) ^ 0x3C5D;
    W.lx = 0xFFFF;
    for (W.i = W.s->nlinks, W.l = W.s->link; W.i; --W.i, ++W.l) {  /* (A way between strata into the region: a dug
                                                **   one the records', none) */
        if (!W.l->dug && (unsigned char) ((W.l->x - W.s->x0) >> 8) == W.rx &&
            (unsigned char) ((W.l->y - W.s->y0) >> 8) == W.ry) {
            if (!V.f) {
                return 0;
            }
            if (W.lx == 0xFFFF) {
                W.lx = W.l->x - W.s->x0;
                W.ly = W.l->y - W.s->y0;
            }
        }
    }
    if (!V.f && ch (W.rx, W.ry) % 3) {
        return 0;
    }
    W.ew = W.s->w - W.ox;
    W.ew = W.ew > RW ? RW : W.ew;
    W.eh = W.s->h - W.oy;
    W.eh = W.eh > RW ? RW : W.eh;
    if (W.ew < 60 || W.eh < 60) {
        return 0;                               /* (No room for one) */
    }
    V.hw = 24 + ch (W.rx + 1, W.ry) % 40;
    V.hh = 20 + ch (W.rx + 2, W.ry) % 36;
    if (V.hw > (W.ew - 20) >> 1) {
        V.hw = (W.ew - 20) >> 1;
    }
    if (V.hh > (W.eh - 20) >> 1) {
        V.hh = (W.eh - 20) >> 1;
    }
    W.a = W.ox + 10 + V.hw;                     /* (Its middle's least, and how many past it) */
    W.b = W.ew - 19 - 2 * V.hw;
    W.a = W.lx == 0xFFFF ? W.a + ch (W.rx + 3, W.ry) % W.b : W.lx < W.a ? W.a : W.lx >= W.a + W.b ? W.a + W.b - 1 :
        W.lx;
    V.x = W.a - (int) ((unsigned) L->cx << CW_BITS);
    W.a = W.oy + 10 + V.hh;
    W.b = W.eh - 19 - 2 * V.hh;
    W.a = W.lx == 0xFFFF ? W.a + ch (W.rx + 4, W.ry) % W.b : W.ly < W.a ? W.a : W.ly >= W.a + W.b ? W.a + W.b - 1 :
        W.ly;
    V.y = W.a - (int) ((unsigned) L->cy << CW_BITS);
    if (V.x + V.hw < -1 || V.x - V.hw > CW || V.y + V.hh < -1 || V.y - V.hh > CW) {
        return 0;                               /* (Not this chunk's) */
    }
    V.deep = 2 + ch (W.rx + 5, W.ry) % 7;
    V.roof = 40 + ch (W.rx + 6, W.ry) % 60;
    for (W.j = 0; W.j < 7; ++W.j) {
        for (W.i = 0; W.i < 7; ++W.i) {
            V.nz[W.j][W.i] = ch ((L->cx << 2) - 1 + W.i, ((L->cy << 2) - 1 + W.j) ^ 0x5A5A) & 15;
        }
    }
    return 1;
}

/* Square (i, j) of the chunk (one past its edges too) in the cavern's reach: its reach (the ellipse's: 64 at its
** edge, 127 past it), and 0x80 if it's the cavern's floor */
static unsigned char __fastcall__ cav_at (signed char i, signed char j)
{
    V.dx = i - V.x;
    V.dy = j - V.y;
    V.dx = V.dx < 0 ? -V.dx : V.dx;
    V.dy = V.dy < 0 ? -V.dy : V.dy;
    if (V.dx > 2 * V.hw || V.dy > 2 * V.hh || (V.ax = ((unsigned) V.dx << 6) / V.hw) > 127 ||
        (V.ay = ((unsigned) V.dy << 6) / V.hh) > 127 || (V.r = (V.ax * V.ax + V.ay * V.ay) >> 6) >= 64) {
        return 127;
    }
    V.gx = i + 8;
    V.gy = j + 8;
    V.fx = V.gx & 7;
    V.fy = V.gy & 7;
    V.gx >>= 3;
    V.gy >>= 3;
    V.n = (unsigned) ((V.nz[V.gy][V.gx] * (8 - V.fx) + V.nz[V.gy][V.gx + 1] * V.fx) * (8 - V.fy) +
        (V.nz[V.gy + 1][V.gx] * (8 - V.fx) + V.nz[V.gy + 1][V.gx + 1] * V.fx) * V.fy) >> 6;
    V.px = ((unsigned) L->cx << CW_BITS) + i;
    V.py = ((unsigned) L->cy << CW_BITS) + j;
    if (V.r < 36 && (V.px & 6) == 4 && (V.py & 6) == 4 && ch (V.px >> 3, (V.py >> 3) ^ 0x2D2D) % 6 == 0) {
        return V.r;                             /* (A pillar) */
    }
    return V.r < 44 + V.n ? V.r | 0x80 : V.r;
}

/* The cavern's part in chunk L: its floor, its hollow (the elevations under it, floor_at's to add), its room; the
** chunk's middle (mx, my) its square nearest the chunk's */
static void cav_part (void)
{
    W.x0 = W.y0 = W.best = 0xFF;
    W.x1 = W.y1 = 0;
    for (W.y = 0; W.y < CW; ++W.y) {
        for (W.x = 0; W.x < CW; ++W.x) {
            if (((W.c = cav_at (W.x, W.y)) & 0x7F) < 64) {
                L->elev[W.y][W.x] = -(signed char) ((unsigned) V.deep * (64 - (W.c & 0x7F)) >> 6);
                if (W.c & 0x80) {
                    L->tile[W.y][W.x] = T_FLOOR;
                    W.x0 = W.x < W.x0 ? W.x : W.x0;
                    W.y0 = W.y < W.y0 ? W.y : W.y0;
                    W.x1 = W.x > W.x1 ? W.x : W.x1;
                    W.y1 = W.y > W.y1 ? W.y : W.y1;
                    if ((W.d = (W.x > CW / 2 ? W.x - CW / 2 : CW / 2 - W.x) + (W.y > CW / 2 ? W.y - CW / 2 :
                        CW / 2 - W.y)) < W.best) {
                        W.best = W.d;
                        mx = W.x;
                        my = W.y;
                    }
                }
            }
        }
    }
    for (W.i = 0; W.i < E->nland && W.x0 != 0xFF; ++W.i) {    /* (A way between strata in its reach on its floor,
                                                **   dq0 -V's: its stairs not left in a passage) */
        W.x = E->lx[W.i];
        W.y = E->ly[W.i];
        if ((cav_at (W.x, W.y) & 0x7F) < 64) {
            L->tile[W.y][W.x] = T_FLOOR;
            W.x0 = W.x < W.x0 ? W.x : W.x0;
            W.y0 = W.y < W.y0 ? W.y : W.y0;
            W.x1 = W.x > W.x1 ? W.x : W.x1;
            W.y1 = W.y > W.y1 ? W.y : W.y1;
        }
    }
    if (W.x0 == 0xFF) {
        return;
    }
    W.p = &L->room[L->nrooms];
    W.p->x = W.x0;
    W.p->y = W.y0;
    W.p->w = W.x1 - W.x0 + 1;
    W.p->h = W.y1 - W.y0 + 1;
    W.p->flags = R_CAVERN;
    W.p->kind = U_CAVE;
    W.p->z0 = L->nzones;
    W.p->nz = 1;
    W.z = &L->zone[L->nzones++];
    W.z->x = W.x0;
    W.z->y = W.y0;
    W.z->w = W.p->w;
    W.z->h = W.p->h;
    W.z->room = L->nrooms++;
    W.z->gx = W.z->gy = 0;                      /* (Its floor, z, the grower's: floor_at's, overlay 3's) */
    W.z->ceil = C_ROUGH;
    W.z->cmid = V.roof;
    W.z->cwall = V.roof >> 1;
}


/* ---- Rooms at 45 degrees (the user's "queue up 1-3", 2026-10-31: ways off the compass, step 3's rest; HOUSE.md's
** rooms on the diagonal, for review): a built stratum whose passages are on the diagonal (a fifth of them, by the
** stratum's seed: pass.c's leg) has its rooms' walls on the diagonal too, so the complex lies at 45 degrees to the
** compass; dq0 -D, the tests', every built stratum's.  Each room four squares or more each way, not a link's, cut to
** the diamond its box holds (twice a square's offset from the box's middle, east-west and north-south, adding to its
** longer side, made even, at most: a diamond in a square box, a long room's corners cut), the squares cut its walls; a
** door's, a mouth's or a window's square in its old walls kept joined to the floor by the squares straight in from
** it, and a room's steps kept, so it's walked as it was.  No dice: the layout as it was grown */
static unsigned char __fastcall__ cut (const struct room* p, unsigned char i, unsigned char j)
{
    W.a = 2 * (i - p->x) - (p->w - 1);
    W.b = 2 * (j - p->y) - (p->h - 1);
    W.y0 = (W.a < 0 ? -W.a : W.a) + (W.b < 0 ? -W.b : W.b);
    return W.y0 > (unsigned char) ((p->w > p->h ? p->w : p->h) & 0xFE);
}

static void diamonds (void)
{
    for (W.c = 0, W.p = L->room; W.c < L->nrooms; ++W.c, ++W.p) {
        if (W.p->w < 4 || W.p->h < 4) {
            continue;
        }
        for (W.i = 0; W.i < E->nland && (E->lx[W.i] < W.p->x || E->lx[W.i] >= W.p->x + W.p->w ||
            E->ly[W.i] < W.p->y || E->ly[W.i] >= W.p->y + W.p->h); ++W.i) {
        }
        if (W.i < E->nland) {
            continue;                           /* (A link's room: as it was) */
        }
        for (W.y = W.p->y; W.y < W.p->y + W.p->h; ++W.y) {
            for (W.x = W.p->x; W.x < W.p->x + W.p->w; ++W.x) {
                if (TILE (L->tile[W.y][W.x]) == T_FLOOR && cut (W.p, W.x, W.y)) {
                    L->tile[W.y][W.x] = T_WALL | T_MARK;    /* (Marked: cut, maybe kept) */
                }
            }
        }
        for (W.d = 0; W.d < 4; ++W.d) {         /* (Its old walls' openings, joined to the floor straight in) */
            for (W.j = 0; W.j < (W.d & 1 ? W.p->h : W.p->w); ++W.j) {
                W.x = W.d & 1 ? (W.d == DIR_E ? W.p->x + W.p->w - 1 : W.p->x) : W.p->x + W.j;
                W.y = W.d & 1 ? W.p->y + W.j : (W.d == DIR_S ? W.p->y + W.p->h - 1 : W.p->y);
                W.x0 = TILE (L->tile[W.y + dir_y[W.d]][W.x + dir_x[W.d]]);
                if (W.x0 != T_DOOR && W.x0 != T_CORR && W.x0 != T_WINDOW && W.x0 != T_STEPS) {
                    continue;
                }
                while ((unsigned char) (W.x - W.p->x) < W.p->w && (unsigned char) (W.y - W.p->y) < W.p->h &&
                    (L->tile[W.y][W.x] & T_MARK)) {
                    L->tile[W.y][W.x] = T_FLOOR;
                    W.x -= dir_x[W.d];
                    W.y -= dir_y[W.d];
                }
            }
        }
        for (W.y = W.p->y; W.y < W.p->y + W.p->h; ++W.y) {
            for (W.x = W.p->x; W.x < W.p->x + W.p->w; ++W.x) {
                L->tile[W.y][W.x] &= ~T_MARK;
            }
        }
    }
}

/* ---- A way that climbs within a stratum (slice 4.10's rest, 2026-11-02: HOUSE.md's inclines, for review): one chunk
** in eight of a natural stratum's, or a mine's, with no way between strata in it (dq0 -P, the tests': every chunk that
** has room), by its own numbers (none of the grower's dice): off a passage's square, a straight way through the rock:
** a sloping tunnel, a foot a square up or down for 10 to 16 squares; in a mine a raise, two feet a square for 8 to 12;
** the rock either side of it left whole; ending through a wall in a small chamber (5 squares by 4) on its own level, a
** room of the chunk's with a rough ceiling.  Walked, not climbed: no step of it more than a walk's */
#define INC_DX  (dir_x[W.d])
#define INC_DY  (dir_y[W.d])

/* Square (W.x, W.y) + a along the way + b across it, all rock for the way's or the chamber's room: 1 */
static unsigned char __fastcall__ inc_rock (signed char a, signed char b)
{
    W.x0 = W.x + a * INC_DX + b * INC_DY;
    W.y0 = W.y + a * INC_DY + b * INC_DX;
    if ((unsigned char) (W.x0 - 2) > CW - 5 || (unsigned char) (W.y0 - 2) > CW - 5) {
        return 0;
    }
    for (W.j = 0; W.j < L->nrooms; ++W.j) {
        W.p = &L->room[W.j];
        if (W.x0 + 1 >= W.p->x && W.x0 <= W.p->x + W.p->w && W.y0 + 1 >= W.p->y && W.y0 <= W.p->y + W.p->h) {
            return 0;
        }
    }
    return L->tile[W.y0][W.x0] == T_ROCK;
}

static void incline (void)
{
    W.s = &STRATUM (L->sn);
    V.seed = (unsigned) W.s->seed ^ (unsigned) (W.s->seed >> 16) ^ 0x3C5D;
    hub_get (0, &HUB->gear, &V.f, 1);
    if ((!(V.f & GE_RAMP) && (E->nland || ch (L->cx + 0x77, L->cy ^ 0x1234) & 7)) || L->nrooms >= NROOMS ||
        L->nzones >= NZONES) {
        return;
    }
    for (V.gx = 0; V.gx < 48; ++V.gx) {         /* (Tries: a passage's square, a way off it) */
        V.n = ch (L->cx + V.gx, L->cy + 0x4141);
        W.x = 3 + V.n % (CW - 6);
        W.y = 3 + ch (L->cx + V.gx, L->cy + 0x4242) % (CW - 6);
        W.d = V.n >> 6;
        if (L->tile[W.y][W.x] != T_CORR) {
            continue;
        }
        W.c = cstyle == ST_NATURAL ? 10 + (V.n & 7) % 7 : 8 + (V.n & 7) % 5;   /* (Its length) */
        for (W.i = 1; W.i <= W.c + 1 && inc_rock (W.i, 0) && inc_rock (W.i, 1) && inc_rock (W.i, -1); ++W.i) {
        }
        if (W.i <= W.c + 1) {
            continue;
        }
        for (V.gy = W.c + 1; V.gy <= W.c + 6; ++V.gy) {     /* (The chamber's room, its wall round it) */
            for (V.fx = 0; V.fx < 7 && inc_rock (V.gy, (signed char) V.fx - 3); ++V.fx) {
            }
            if (V.fx < 7) {
                break;
            }
        }
        if (V.gy <= W.c + 6) {
            continue;
        }
        /* It: its floor from the square it leaves, up or down by the hash, its square at the wall a passage's */
        W.a = L->elev[W.y][W.x];
        W.b = (cstyle == ST_NATURAL ? 1 : 2) * (V.n & 8 ? 1 : -1);
        if (W.a + W.b * W.c > 110 || W.a + W.b * W.c < -110) {
            continue;
        }
        for (W.i = 1; W.i <= W.c + 1; ++W.i) {
            inc_rock (W.i, 0);
            L->tile[W.y0][W.x0] = T_CORR;
            L->elev[W.y0][W.x0] = W.a + W.b * (W.i > W.c ? W.c : W.i);
        }
        W.a += W.b * W.c;
        inc_rock (W.c + 2, -2);                 /* (The chamber's corners: its bounds) */
        V.dx = W.x0;
        V.dy = W.y0;
        inc_rock (W.c + 5, 2);
        W.x1 = W.x0;
        W.y1 = W.y0;
        W.x0 = V.dx < W.x1 ? V.dx : W.x1;
        W.y0 = V.dy < W.y1 ? V.dy : W.y1;
        W.x1 = V.dx < W.x1 ? W.x1 : V.dx;
        W.y1 = V.dy < W.y1 ? W.y1 : V.dy;
        W.p = &L->room[L->nrooms];
        W.p->x = W.x0;
        W.p->y = W.y0;
        W.p->w = W.x1 - W.x0 + 1;
        W.p->h = W.y1 - W.y0 + 1;
        W.p->flags = 0;
        W.p->kind = cstyle == ST_NATURAL ? U_CAVE : U_CHAMBER;
        W.p->z0 = L->nzones;
        W.p->nz = 1;
        W.z = &L->zone[L->nzones++];
        W.z->x = W.p->x;
        W.z->y = W.p->y;
        W.z->w = W.p->w;
        W.z->h = W.p->h;
        W.z->room = L->nrooms++;
        W.z->z = W.a;
        W.z->gx = W.z->gy = 0;
        W.z->ceil = C_ROUGH;
        W.z->cmid = 8 + (V.n & 7);
        W.z->cwall = W.z->cmid - 1;
        for (V.gy = W.y0 - 1; V.gy <= W.y1 + 1; ++V.gy) {
            for (V.gx = W.x0 - 1; V.gx <= W.x1 + 1; ++V.gx) {
                if (V.gx >= W.x0 && V.gx <= W.x1 && V.gy >= W.y0 && V.gy <= W.y1) {
                    L->tile[V.gy][V.gx] = T_FLOOR;
                    L->elev[V.gy][V.gx] = W.a;
                } else if (L->tile[V.gy][V.gx] == T_ROCK) {
                    L->tile[V.gy][V.gx] = T_WALL;
                }
            }
        }
        return;
    }
}

/* The great cavern of chunk L's region (the cave grower's, overlay 3's: its stub, bank.s's): m 0, its part grown, 1 if
** it has one (the room and zone 0); m 1, as its walls are made, those at the chunk's edges beside the next chunk's
** part (V as m 0 left it); m 2, a built chunk's rooms at 45 degrees, if its stratum's complex lies so, and a mine's
** raise (the built grower's, overlay 1's); m 3, a cave's sloping tunnel */
unsigned char __fastcall__ o_cavern (unsigned char m)
{
    if (m == 2) {
        hub_get (0, &HUB->gear, &V.f, 1);
        if ((V.f & GE_DIAMOND) || !(((const unsigned char*) &STRATUM (L->sn).seed)[3] % 5)) {
            diamonds ();                        /* (Its passages' strata, pass.c's leg's: a fifth, by the seed) */
        }
        if (cstyle == ST_MINE) {
            incline ();                         /* (A raise) */
        }
        return 0;
    }
    if (m == 3) {
        incline ();                             /* (A sloping tunnel) */
        return 0;
    }
    if (!m) {
        if (!cv_find ()) {
            return 0;
        }
        cav_part ();
        return 1 + L->nrooms;                   /* (1: none in the chunk, only beside it; 2: its part) */
    }
    for (W.y = 0; W.y < CW; ++W.y) {
        for (W.x = 0; W.x < CW; W.x += W.y && W.y < CW - 1 ? CW - 1 : 1) {    /* (The edges' squares) */
            for (W.i = 0; W.i < 9 && L->tile[W.y][W.x] == T_ROCK; ++W.i) {
                if (cav_at (W.x - 1 + W.i % 3, W.y - 1 + W.i / 3) & 0x80) {
                    L->tile[W.y][W.x] = T_WALL;
                }
            }
        }
    }
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "OV1CODE")            /* (Overlay 1's: 3's room, the air's) */
#pragma rodata-name (push, "OV1RODATA")
static const int o8[8] = { -CW, 1 - CW, 1, CW + 1, CW, CW - 1, -1, -CW - 1 };    /* (The eight ways, as offsets in
                                                **   the chunk: N, NE ... NW) */
/* A tunnel's zigzag made the diagonal it is (the user's, 2026-10-31: tunnels off the compass): a tunnel's square in no
** chamber, its two ways on a quarter apart (the rest beside it rock, and the two squares across its corners), the
** zigzag going on past it (a square across its other corners a tunnel's, or cut), the rise between them a step's: the
** corner the diagonal cuts (T_CUT), the squares either side walked between (chunk.c's slant).  A bend alone stays a
** bend.  No dice: the layout as it was grown */
#pragma static-locals (push, on)
void o_slants (void)
{
    static unsigned char x, y, k, a, m, t, n[8];
    static unsigned i;
    static struct room* p;

    for (i = CW + 1; i < CW * (CW - 1) - 1; ++i) {
        x = i & (CW - 1);
        y = i >> CW_BITS;
        if (!x || x == CW - 1 || TILE (L->tile[0][i]) != T_CORR) {
            continue;
        }
        for (k = m = 0; k < 8; ++k) {
            n[k] = t = TILE (L->tile[0][i + o8[k]]);
            if (!(k & 1)) {
                if (t == T_CORR) {
                    m |= 1 << (k >> 1);
                } else if (t != T_ROCK) {
                    m = 0xF0;
                }
            }
        }
        a = m == 3 ? 0 : m == 6 ? 2 : m == 12 ? 4 : m == 9 ? 6 : 0xFF;   /* (Its two ways on a quarter apart: the
                                                **   first's heading, the second's two on) */
        if (a == 0xFF || n[a + 1] != T_ROCK || n[(a + 5) & 7] != T_ROCK ||
            ((t = n[(a + 3) & 7]) != T_CORR && t != T_CUT && (t = n[(a + 7) & 7]) != T_CORR && t != T_CUT) ||
            (unsigned char) (L->elev[0][i + o8[a]] - L->elev[0][i + o8[(a + 2) & 7]] + STEP_MAX) > 2 * STEP_MAX) {
            continue;
        }
        for (k = L->nrooms, p = L->room; k; --k, ++p) {
            if (x + 1 >= p->x && x <= p->x + p->w && y + 1 >= p->y && y <= p->y + p->h) {
                break;
            }
        }
        if (!k) {
            L->tile[0][i] = T_CUT;
        }
    }
}
#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
void slants (void);                             /* (Overlay 1's, its stub) */

void __fastcall__ o_grow_caves (struct chunk* c, const struct edges* e, unsigned long seed)
{
    struct stratum* s = &STRATUM (c->sn);
    unsigned char i, j, k, x, y, n, cv;

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
    if ((cv = cavern (0)) > 1) {               /* (A great cavern's part: first, the chambers kept clear of it; the
                                                **   middle in it) */
        L->zone[0].z = floor_at (mx, my);
    }
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
            L->elev[y][x] = L->tile[y][x] != T_ROCK ? floor_at (x, y) + L->elev[y][x] : 0;   /* (A cavern's hollow
                                                **   in it: cav_part's) */
        }
    }
    for (i = 0; i < L->nrooms; ++i) {
        if (!(L->room[i].flags & R_CAVERN)) {
            hollow (&L->room[i]);
        }
    }
    cavern (3);                                 /* (A sloping tunnel, its floors laid: overlay 8's) */
    slants ();
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
    if (cv) {
        cavern (1);                             /* (A great cavern's walls at the edges, beside the next chunk's part) */
    }
    /* The links' squares: stairways and ladders, up and down */
    for (i = 0; i < e->nland && L->nopen < NOPEN; ++i) {
        x = e->lx[i];
        y = e->ly[i];
        k = s->link[e->ln[i]].kind;
        L->tile[y][x] = k == O_STAIRS_DOWN || k == O_LADDER_DOWN || k == O_SHAFT_DOWN || k == O_RAMP_DOWN ?
            T_DOWN : T_UP;
        L->open[L->nopen].kind = k;
        L->open[L->nopen].x = x;
        L->open[L->nopen].y = y;
        L->open[L->nopen].sill = 0;
        L->open[L->nopen++].link = e->ln[i];
    }
}

/* ---- The water (slice 4.9: DUNGEON.md's water), as a chunk's grown (make.c's, after its grower and its fixtures: any
** style's), from its region's (the plan's RW_*, its edges' water), by its own numbers (a hash of its place: the
** growers' dice as they were): a cistern full (5 to 8 feet over its floor's lowest), a cave's chambers in a lake's
** region flooded (4 to 11 feet), in a stream's pooled in their hollows (a foot or two), where there's water under it
** one in three (a foot), but a chamber with a way between strata in it, kept dry (the house's: the stairs' landing
** never under water); a stream's region's passages running with it, a foot deep.  A square of a room's floor under
** its surface wet (the bits), a passage's square of a stream's */

/* Its air (2026-10-31: DUNGEON.md's bad air, HOUSE.md's for review: overlay 3's, make_chunk's after its water), from the
** chunk's place alone (no dice of the maker's: the layouts as they were): a pocket in one of its rooms, round its
** middle, its reach the room's half and a square: firedamp where the rock's poor (shale at its middle: the coal
** measures' gas) one chunk in four; blackdamp (stale air pooled where nothing stirs it) 100 feet down or more, one
** chunk in eight, 200 feet one in four; strength 1, 2 from 200 feet, a step more one time in four; none where there's
** a way between strata in the chunk (the air moves there).  A test's scene (the environment's dq0air, rc's, read
** once: "0" no bad air; "KS" every chunk a pocket of kind K, rs.h's AQ_*, strength S, from end to end) */
static unsigned char tair = 0xFF;               /* (The environment's: 0x80 | kind times 16 | strength; 0 none said) */

void __fastcall__ o_air_pocket (struct chunk* c, const struct edges* e)
{
#define AR  (*(struct hy_regs*) sy)            /* (The built grower's stack's room, idle: built.c's) */
#define b   ((char*) sy + sizeof (struct hy_regs))
    unsigned h = (c->cx * 57 + c->cy * 23 + c->sn * 101) ^ 0x2C6B, s;
    unsigned char k;
    struct room* p;

    if (tair == 0xFF) {
        tair = 0;
        AR.a = 0xFF;
        AR.r[0] = (unsigned) "dq0air";
        AR.r[1] = (unsigned) b;
        AR.r[2] = 3;
        AR.r[3] = 0;                            /* (From the value's start: the scratch room isn't cleared) */
        if (!hy_call (HY_ENV_GET, &AR)) {
            tair = 0x80 | (b[0] & 3) << 4 | (b[1] & 3);
        }
    }
    h ^= h >> 7;
    h *= 0x2F1B;
    h >>= 8;
    if (!c->nrooms || tair == 0x80) {
        return;
    }
    s = e->depth >= 200 ? 2 : 1;
    if (tair) {
        c->air = tair & 0x3F;                   /* (A test's: its first room) */
        h = 0;
    } else {
        if (e->nland) {
            return;
        } else if (e->poor == 1 && !(h & 3)) {
            k = AQ_FIRE;
        } else if (e->depth >= 100 && (h >> 2 & 7) < s) {
            k = AQ_BLACK;
        } else {
            return;
        }
        c->air = k << 4 | (s + !(h & 0x60));
    }
    p = &c->room[(unsigned char) h % c->nrooms];
    c->ax = p->x + (p->w >> 1);
    c->ay = p->y + (p->h >> 1);
    c->ar = tair ? 100 : ((p->w > p->h ? p->w : p->h) >> 1) + 1;     /* (A test's: the whole chunk) */
}
#undef AR
#undef b

#pragma code-name (push, "OV6CODE")           /* (Overlay 6's: 3 full) */
#pragma rodata-name (push, "OV6RODATA")

static unsigned char __fastcall__ wh (unsigned char k)
{
    unsigned h = (L->cx * 31 + L->cy * 17 + k * 101) ^ 0x5A5A;

    h ^= h >> 7;
    h *= 0x2F1B;
    return (unsigned char) (h >> 8);
}

/* Square x, y of the chunk (L) a passage's that may fall in: a passage's, every square beside it a passage's or the
** rock's (no room's, no doorway's: they stay as they were made), in no room's bounds (a cave's may take in a tunnel),
** not on the chunk's edge */
static const signed char nb[4] = { -CW, 1, CW, -1 };
static unsigned char __fastcall__ fallen_ok (unsigned char x, unsigned char y)
{
    unsigned char d, t;
    unsigned char* q;
    struct room* p;

    if ((unsigned char) (x - 1) >= CW - 2 || (unsigned char) (y - 1) >= CW - 2 || TILE (*(q = &L->tile[y][x])) != T_CORR) {
        return 0;
    }
    for (d = 0; d < 4; ++d) {
        if ((t = TILE (q[nb[d]])) != T_CORR && t != T_ROCK && t != T_WALL && t != T_RUBBLE) {
            return 0;
        }
    }
    for (d = L->nrooms, p = L->room; d; --d, ++p) {
        if (x + 1 >= p->x && x <= p->x + p->w && y + 1 >= p->y && y <= p->y + p->h) {
            return 0;
        }
    }
    return 1;
}

/* Age's collapse (HOUSE.md's rock's kinds and hardness, slice 4.10's): an old stratum's passages partly fallen in,
** a heap of rubble a square and some beside it, by its own dice (the chunk's place: the layout as it was); six tries
** a chunk, each falling in two centuries old or more, in 20 a chance of its age in centuries (ten at most), twice
** that where the rock is poor (shale at the chunk's middle: its edges' poor, plan.c's) */
static void __fastcall__ rubble (unsigned char poor)
{
    unsigned char i, d, x, y, n = STRATUM (L->sn).age;
    unsigned char* q;

    if (n < 2 || poor == 0xFF) {                /* (Young; or dq0 -U's, the tests': none) */
        return;
    }
    n = (n > 10 ? 10 : n) << poor;
    for (i = 0; i < 6; ++i) {
        x = wh (40 + i) & (CW - 1);
        y = wh (50 + i) & (CW - 1);
        if (!fallen_ok (x, y) || wh (60 + i) % 20 >= n) {
            continue;
        }
        *(q = &L->tile[y][x]) += T_RUBBLE - T_CORR;
        for (d = 0; d < 4; ++d) {               /* (Some beside it too) */
            if (wh (70 + i * 4 + d) & 1 && fallen_ok (x + (d == 1) - (d == 3), y + (d == 2) - (d == 0))) {
                q[nb[d]] += T_RUBBLE - T_CORR;
            }
        }
    }
}

void __fastcall__ o_wet_chunk (struct chunk* c, const struct edges* e)
{
    unsigned char r, x, y, k, d, w = e->water;
    signed char lo;
    struct room* p;

    L = c;
    if (w == 0xFF) {                            /* (dq0 -W, the tests': no water; the rubble still) */
        rubble (e->poor);
        return;
    }
    memset (c->wlev, WL_NONE, sizeof c->wlev);
    for (r = 0; r < c->nrooms && w; ++r) {
        p = &c->room[r];
        k = wh (r);
        d = p->kind == U_CISTERN ? 5 + (k & 3) : p->kind != U_CAVE || (p->flags & R_CAVERN) ? 0 : w & RW_LAKE ? 4 + (k & 7) : w & RW_STREAM ?
            1 + (k & 1) : k % 3 == 0;
        for (x = 0; x < e->nland && !(e->lx[x] >= p->x && e->lx[x] < p->x + p->w && e->ly[x] >= p->y &&
            e->ly[x] < p->y + p->h); ++x) {
        }
        if (!d || x < e->nland) {
            continue;
        }
        lo = 120;
        for (y = p->y; y < p->y + p->h; ++y) {
            for (x = p->x; x < p->x + p->w; ++x) {
                if (TILE (c->tile[y][x]) == T_FLOOR && c->elev[y][x] < lo) {
                    lo = c->elev[y][x];
                }
            }
        }
        c->wlev[r] = lo + d;                    /* (The rock within a hundred feet of the datum or so: no more) */
        for (y = p->y; y < p->y + p->h; ++y) {
            for (x = p->x; x < p->x + p->w; ++x) {
                if (TILE (c->tile[y][x]) == T_FLOOR && c->elev[y][x] < c->wlev[r]) {
                    c->wet[y][x >> 3] |= 1 << (x & 7);
                }
            }
        }
        c->wany = 1;
    }
    if (w & RW_STREAM) {
        c->wany = 1;
        c->wlev[NROOMS] = 1;
        c->wruns = 1 << NROOMS;
        for (y = 0; y < CW; ++y) {
            for (x = 0; x < CW; ++x) {
                if (TILE (c->tile[y][x]) == T_CORR) {
                    c->wet[y][x >> 3] |= 1 << (x & 7);
                }
            }
        }
    }
    rubble (e->poor);                        /* (Age's collapse: slice 4.10's, wet or dry) */
}

#pragma code-name (pop)
#pragma rodata-name (pop)
