/*
** view.c - (dq0w) what the character sees (DUNGEON.md), from where it stands and the way it faces (one of the
** compass's eight: the half of the world that way): the squares in front of it and to its sides (none behind, but
** for the ones it's next to), that a line from its eyes (five feet above its floor) reaches without a wall or rock
** in the way, nor a floor between that rises above the line; within its sight's range; and lit, by a room's own
** light or by the light it carries.  Each is marked T_VIS (seen now: the perception record's) and T_SEEN
** (remembered: the map).  A wall or a door beside a room's floor that's seen is seen too (a line along a wall the
** character stands by would run through it).  The range is the referee's (its summary's sight, 0-9, with whatever
** the character's state does to it); blinded, it sees nothing.  In the dark its night vision carries a light further
** (the light's reach, times it), and its ultravision sees without light to its range (the summary's, both).
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
        if (!(p = at (vs, cx, cy)) || !T_CLEAR (*p) || C->elev[cy_][cx_] > ez + (tz - ez) * k / n) {
            return 0;
        }
    }
}

/* A room's floor beside (x, y), seen now (the eight round it) */
static unsigned char __fastcall__ beside_floor (unsigned x, unsigned y)
{
    signed char i, j;
    unsigned char* p;

    for (j = -1; j <= 1; ++j) {
        for (i = -1; i <= 1; ++i) {
            if ((p = at (vs, x + i, y + j)) && (*p & T_VIS) && T_INSIDE (*p)) {
                return 1;
            }
        }
    }
    return 0;
}

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    int dx, dy, tz;
    unsigned i, j, d2, r2, l2, u2 = dark * dark;
    unsigned char range, t;
    unsigned char* p;

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
    if ((p = at (sn, x, y)) != 0) {
        *p |= lit (x, y) || light || dark ? T_VIS | T_SEEN : T_SEEN;
    }
    for (j = by0; j <= by1; ++j) {
        for (i = bx0; i <= bx1; ++i) {
            if (!(p = at (sn, i, j)) || TILE (*p) == T_ROCK) {
                continue;
            }
            tz = C->elev[cy_][cx_];
            dx = (int) (i - x);
            dy = (int) (j - y);
            d2 = dx * dx + dy * dy;
            if (!d2 || d2 > r2) {
                continue;
            }
            if (dx * head_x[face] + dy * head_y[face] < 0 && d2 > 2) {
                continue;                       /* (Behind: but for what's next to it) */
            }
            if (d2 > l2 && d2 > u2 && !lit (i, j)) {
                continue;                       /* (Dark: beyond its light, and its ultravision) */
            }
            if (clear (i, j, tz) && (p = at (sn, i, j)) != 0) {
                *p |= T_VIS | T_SEEN;
            }
        }
    }
    for (j = by0; j <= by1; ++j) {
        for (i = bx0; i <= bx1; ++i) {
            if (!(p = at (sn, i, j))) {
                continue;
            }
            t = TILE (*p);
            if ((t == T_WALL || t == T_DOOR || t == T_WINDOW) && !(*p & T_VIS) && beside_floor (i, j) &&
                (p = at (sn, i, j)) != 0) {
                *p |= T_VIS | T_SEEN;
            }
        }
    }
}
