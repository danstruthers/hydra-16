/*
** view.c - (dq0w) what the character sees (DUNGEON.md), from where it stands and the way it faces (one of the
** compass's eight: the half of the world that way): the squares in front of it and to its sides (none behind, but
** for the ones it's next to), that a line from its eyes (five feet above its floor) reaches without a wall or rock
** in the way, nor a floor between that rises above the line; within its sight's range; and lit, by a room's own
** light, by the light it carries, or by a lit thing lying about (a torch put down: things.c's lamps).  Each is
** marked T_VIS (seen now: the perception record's) and T_SEEN (remembered: the map).  A wall or a door beside a
** room's floor that's seen is seen too (a line along a wall the character stands by would run through it).  The
** range is the referee's (its summary's sight, 0-9, with whatever the character's state does to it); blinded, it
** sees nothing.  In the dark its night vision carries a light further (the light's reach, times it), and its
** ultravision sees without light to its range (the summary's, both).
*/

#include <stdlib.h>
#include "../dq0w/world.h"
#include "view.h"

#define EYE         5                           /* The eyes, feet above the floor */

static unsigned char vs;                        /* The stratum */
static unsigned ex, ey;                         /* The eyes' square, */
static int ez;                                  /*   and their height */
static unsigned bx0, by0, bx1, by1;             /* The squares the last view looked at */

/* A room's own light reaching (x, y): its floor and its walls (a room's walls are in its chunk) */
static unsigned char __fastcall__ lit (unsigned x, unsigned y)
{
    unsigned char r;
    struct room* p;

    if (!at (vs, x, y)) {
        return 0;
    }
    for (r = 0; r < C->nrooms; ++r) {
        p = &C->room[r];
        if ((p->flags & R_LIT) && cx_ + 1 >= p->x && cx_ <= p->x + p->w && cy_ + 1 >= p->y && cy_ <= p->y + p->h) {
            return 1;
        }
    }
    return 0;
}

/* A line from the eyes to (x, y) with nothing in the way: no wall or rock, no floor above the line (the square
** itself may be a wall) */
static unsigned char __fastcall__ clear (unsigned x, unsigned y, int tz)
{
    int dx = abs ((int) (x - ex)), dy = -abs ((int) (y - ey)), sx = (int) (x - ex) > 0 ? 1 : -1;
    int sy = (int) (y - ey) > 0 ? 1 : -1, e = dx + dy, e2, n = dx > -dy ? dx : -dy, k = 0;
    unsigned cx = ex, cy = ey;
    unsigned char* p;
    struct portal* q;

    for (;;) {
        e2 = 2 * e;
        if (e2 >= dy) {
            e += dy;
            cx += sx;
        }
        if (e2 <= dx) {
            e += dx;
            cy += sy;
        }
        ++k;
        if (cx == x && cy == y) {
            return 1;
        }
        if (!(p = at (vs, cx, cy)) || !T_CLEAR (*p) || C->elev[cy_][cx_] > ez + (tz - ez) * k / n ||
            ((TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) && (q = portal_at (cx_, cy_)) != 0 &&
            !(q->state & PS_OPEN) && q->kind != PK_PORTCULLIS)) {
            return 0;                           /* (Rock, a wall, a floor above the line, a closed door or shutters) */
        }
    }
}

/* A lit thing lying about (things.c's lamps) lighting (x, y), its floor tz: within its reach, a clear line to it from
** the light (a couple of feet above its floor) */
struct lamp lamp[NLAMPS];
unsigned char nlamps;

static unsigned char __fastcall__ lamplit (unsigned x, unsigned y, int tz)
{
    static struct lamp* l;
    static unsigned sx, sy;
    static int sz;
    static unsigned d, w;
    static unsigned char k, r;

    r = 0;
    for (k = 0, l = lamp; k < nlamps && !r; ++k, ++l) {
        d = l->r;
        w = d + d;
        if (x - l->x + d <= w && y - l->y + d <= w) {   /* (Within its reach: a square of it, near enough) */
            sx = ex;
            sy = ey;
            sz = ez;
            ex = l->x;
            ey = l->y;
            ez = elev_at (vs, ex, ey) + 2;
            r = (x == ex && y == ey) || clear (x, y, tz);
            ex = sx;
            ey = sy;
            ez = sz;
        }
    }
    return r;
}

/* The walls, doors and windows round (x, y), a room's floor seen now, seen too (but those outside the view's squares:
** the next view unsees only those): the eight in its chunk found there (round[], from the square up and to the left
** of it); else each by itself */
static const unsigned char round[9] = { 0, 1, 2, CW, CW + 1, CW + 2, 2 * CW, 2 * CW + 1, 2 * CW + 2 };

static void __fastcall__ walls_round (unsigned x, unsigned y)
{
    static unsigned char* p;
    static unsigned char k, t;
    static unsigned nx, ny;

    if ((p = at (vs, x, y)) != 0 && cx_ - 1 < CW - 2 && cy_ - 1 < CW - 2 && x - bx0 - 1 < bx1 - bx0 - 1 &&
        y - by0 - 1 < by1 - by0 - 1) {
        for (p -= CW + 1, k = 0; k < 9; ++k) {
            if ((t = TILE (p[round[k]])) == T_WALL || t == T_DOOR || t == T_WINDOW) {
                p[round[k]] |= T_VIS | T_SEEN;
            }
        }
        return;
    }
    for (k = 0; k < 8; ++k) {
        nx = x + head_x[k];
        ny = y + head_y[k];
        if (nx - bx0 <= bx1 - bx0 && ny - by0 <= by1 - by0 && (p = at (vs, nx, ny)) != 0 &&
            ((t = TILE (*p)) == T_WALL || t == T_DOOR || t == T_WINDOW)) {
            *p |= T_VIS | T_SEEN;
        }
    }
}

/* The squares, 0 to the longest range (4 + 2 times 9): no multiplying, square by square */
static const unsigned sq[23] = {
    0, 1, 4, 9, 16, 25, 36, 49, 64, 81, 100, 121, 144, 169, 196, 225, 256, 289, 324, 361, 400, 441, 484
};

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    int dx, dy, tz;
    unsigned i, j, d2, r2, l2, u2 = dark * dark;
    unsigned char range, t;
    unsigned char* p;
    signed char hx = head_x[face], hy = head_y[face];

    for (j = by0; j <= by1 && bx1; ++j) {       /* (What the last view saw, unseen now) */
        for (i = bx0; i <= bx1; ++i) {
            if ((p = at (vs, i, j)) != 0) {
                *p &= ~T_VIS;
            }
        }
    }
    vs = sn;
    ex = x;
    ey = y;
    ez = elev_at (sn, x, y) + EYE;
    bx1 = 0;
    if (sight == 0xFF) {                        /* (Blinded) */
        return;
    }
    range = 4 + 2 * (sight > 9 ? 9 : sight);
    r2 = range * range;
    l2 = light * light;
    bx0 = x - range;
    by0 = y - range;
    bx1 = x + range;
    by1 = y + range;
    t = lit (x, y) || light || dark || lamplit (x, y, ez - EYE);
    if ((p = at (sn, x, y)) != 0) {
        *p |= t ? T_VIS | T_SEEN : T_SEEN;
        if (t && T_INSIDE (*p)) {
            walls_round (x, y);
        }
    }
    for (j = by0; j <= by1; ++j) {
        for (i = bx0; i <= bx1; ++i) {
            if (!(p = at (sn, i, j)) || TILE (*p) == T_ROCK) {
                continue;
            }
            tz = C->elev[cy_][cx_];
            dx = (int) (i - x);
            dy = (int) (j - y);
            d2 = sq[dx < 0 ? -dx : dx] + sq[dy < 0 ? -dy : dy];
            if (!d2 || d2 > r2) {
                continue;
            }
            if ((hx < 0 ? -dx : hx ? dx : 0) + (hy < 0 ? -dy : hy ? dy : 0) < 0 && d2 > 2) {
                continue;                       /* (Behind: but for what's next to it) */
            }
            if (d2 > l2 && d2 > u2 && !lit (i, j) && !lamplit (i, j, tz)) {
                continue;                       /* (Dark: beyond its light, its ultravision, a light lying about) */
            }
            if (clear (i, j, tz) && (p = at (sn, i, j)) != 0) {
                *p |= T_VIS | T_SEEN;
                if (T_INSIDE (*p)) {
                    walls_round (i, j);         /* (A wall or a door beside a room's floor that's seen) */
                }
            }
        }
    }
}
