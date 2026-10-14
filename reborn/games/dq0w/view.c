/*
** view.c - (dq0w) what the character sees, from where it stands and the way it faces: the tiles in front of it and
** to its sides (none behind, but for the ones it's next to), that a line from its eyes reaches without a wall or
** rock in the way, within its sight's range, and lit: by the room's own light, or by the light it carries.  Each
** is marked T_VIS (seen now: what the UI tells) and T_SEEN (remembered: the map).  A wall or a door beside a
** room's floor that's seen is seen too (a line along a wall the character stands by would run through it).  The range is the referee's
** (its summary's sight, 0-9, with whatever the character's state does to it); blinded, it sees nothing.
*/

#include <stdlib.h>
#include "../dq0/level.h"
#include "view.h"

static struct level* L;
static unsigned char ex, ey;                    /* The eyes */

/* A room's light reaching (x, y): its floor and its walls */
static unsigned char __fastcall__ lit (unsigned char x, unsigned char y)
{
    unsigned char r;
    struct room* p;

    for (r = 0; r < L->nrooms; ++r) {
        p = &L->room[r];
        if ((p->flags & R_LIT) && x + 1 >= p->x && x <= p->x + p->w && y + 1 >= p->y && y <= p->y + p->h) {
            return 1;
        }
    }
    return 0;
}

/* A line from the eyes to (x, y) with nothing in the way (the tile itself may be a wall) */
static unsigned char __fastcall__ clear (unsigned char x, unsigned char y)
{
    int dx = abs ((int) x - ex), dy = -abs ((int) y - ey), sx = x > ex ? 1 : -1, sy = y > ey ? 1 : -1, e = dx + dy, e2;
    int cx = ex, cy = ey;

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
        if (cx == x && cy == y) {
            return 1;
        }
        if (!T_PASS (L->tile[cy][cx])) {
            return 0;
        }
    }
}

/* A room's floor beside (x, y), seen now (the eight round it: a corner too) */
static unsigned char __fastcall__ beside_floor (unsigned char x, unsigned char y)
{
    unsigned char i, j, t;

    for (j = y ? y - 1 : 0; j <= y + 1 && j < LH; ++j) {
        for (i = x ? x - 1 : 0; i <= x + 1 && i < LW; ++i) {
            t = L->tile[j][i];
            if ((t & T_VIS) && T_INSIDE (t)) {
                return 1;
            }
        }
    }
    return 0;
}

void __fastcall__ view (struct level* l, unsigned char x, unsigned char y, unsigned char face, unsigned char light,
    unsigned char sight)
{
    int dx, dy;
    unsigned char i, j, x0, y0, x1, y1, range, t;
    unsigned d2, r2, l2;

    L = l;
    ex = x;
    ey = y;
    for (j = 0; j < LH; ++j) {
        for (i = 0; i < LW; ++i) {
            L->tile[j][i] &= ~T_VIS;
        }
    }
    if (sight == 0xFF) {                        /* (Blinded) */
        return;
    }
    range = 4 + 2 * (sight > 9 ? 9 : sight);
    r2 = range * range;
    l2 = light * light;
    x0 = x > range ? x - range : 0;
    y0 = y > range ? y - range : 0;
    x1 = x + range < LW ? x + range : LW - 1;
    y1 = y + range < LH ? y + range : LH - 1;
    L->tile[y][x] |= lit (x, y) || light ? T_VIS | T_SEEN : T_SEEN;
    for (j = y0; j <= y1; ++j) {
        for (i = x0; i <= x1; ++i) {
            t = L->tile[j][i];
            if (TILE (t) == T_ROCK) {
                continue;
            }
            dx = (int) i - x;
            dy = (int) j - y;
            d2 = dx * dx + dy * dy;
            if (!d2 || d2 > r2) {
                continue;
            }
            if (dx * dir_x[face] + dy * dir_y[face] < 0 && d2 > 2) {
                continue;                       /* (Behind: but for what's next to it) */
            }
            if (d2 > l2 && !lit (i, j)) {
                continue;                       /* (Dark) */
            }
            if (clear (i, j)) {
                L->tile[j][i] = t | T_VIS | T_SEEN;
            }
        }
    }
    for (j = y0; j <= y1; ++j) {
        for (i = x0; i <= x1; ++i) {
            t = TILE (L->tile[j][i]);
            if ((t == T_WALL || t == T_DOOR) && !(L->tile[j][i] & T_VIS) && beside_floor (i, j)) {
                L->tile[j][i] |= T_VIS | T_SEEN;
            }
        }
    }
}
