/*
** gen.c - Dingus Quest Zero's levels made (the world's): rooms put down at random where they fit, a wall round
** each and a tile of rock between; sorted from the west, each joined to the next by a passage (a door in each
** room's wall facing the other, and a dog-leg between them, that gives up rather than cut through a room), and a
** few more joins for loops; every room reached from the first, or the level made again; the stairs in two rooms.
** Some rooms are lit, fewer the deeper the level (a stand-in for the theme's say).
** The way NetHack makes its levels, more or less.  A level's own seed, from the game's: the same game, the same
** dungeon, whenever the character first goes down to it.
*/

#include <string.h>
#include <dice.h>
#include "../dq0/level.h"
#include "gen.h"

static struct level* L;

#define PMAX    160                             /* A passage's tiles, at most */
static unsigned char px[PMAX], py[PMAX];
static unsigned char pn;

/* The rectangle round a room's floor (its walls, and a tile more) all rock: it fits */
static unsigned char __fastcall__ fits (unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i, j, x0 = x - 2, y0 = y - 2, x1 = x + w + 1, y1 = y + h + 1;

    if (x < 2) {
        x0 = 0;
    }
    if (y < 2) {
        y0 = 0;
    }
    if (x1 >= LW) {
        x1 = LW - 1;
    }
    if (y1 >= LH) {
        y1 = LH - 1;
    }
    for (j = y0; j <= y1; ++j) {
        for (i = x0; i <= x1; ++i) {
            if (L->tile[j][i] != T_ROCK) {
                return 0;
            }
        }
    }
    return 1;
}

static void rooms (void)
{
    unsigned char tries, w, h, x, y, i, j;
    struct room* r;

    for (tries = 0; tries < 120 && L->nrooms < NROOMS; ++tries) {
        w = 3 + random (9);
        h = 2 + random (4);
        x = 1 + random (LW - 1 - w);
        y = 1 + random (LH - 1 - h);
        if (!fits (x, y, w, h)) {
            continue;
        }
        r = &L->room[L->nrooms++];
        r->x = x;
        r->y = y;
        r->w = w;
        r->h = h;
        r->flags = random (8) + L->depth < 5 ? R_LIT : 0;
        r->kind = random (8);
        for (j = y - 1; j <= y + h; ++j) {
            for (i = x - 1; i <= x + w; ++i) {
                L->tile[j][i] = j < y || j == y + h || i < x || i == x + w ? T_WALL : T_FLOOR;
            }
        }
    }
}

/* The rooms sorted from the west (so that joining each to the next seldom crosses) */
static void sort (void)
{
    static struct room t;
    unsigned char i, j;

    for (i = 1; i < L->nrooms; ++i) {
        t = L->room[i];
        for (j = i; j && L->room[j - 1].x > t.x; --j) {
            L->room[j] = L->room[j - 1];
        }
        L->room[j] = t;
    }
}

/* A tile of a passage, if it can be dug: 0 if it's in the way (a room's) */
static unsigned char __fastcall__ step (unsigned char x, unsigned char y)
{
    unsigned char t = L->tile[y][x];

    if (t != T_ROCK && t != T_CORR) {
        return 0;
    }
    if (pn < PMAX) {
        px[pn] = x;
        py[pn] = y;
        ++pn;
    }
    return pn < PMAX;
}

/* A dog-leg from (x, y) to (tx, ty): along the doors' axis to half way, across, then on (each tile kept in px, py) */
static unsigned char __fastcall__ leg (unsigned char x, unsigned char y, unsigned char tx, unsigned char ty,
    unsigned char across)
{
    unsigned char m;

    pn = 0;
    if (!step (x, y)) {
        return 0;
    }
    if (across) {                               /* (East and west: across first) */
        m = (x + tx) / 2;
        while (x != m) {
            x += x < m ? 1 : -1;
            if (!step (x, y)) {
                return 0;
            }
        }
        while (y != ty) {
            y += y < ty ? 1 : -1;
            if (!step (x, y)) {
                return 0;
            }
        }
    } else {
        m = (y + ty) / 2;
        while (y != m) {
            y += y < m ? 1 : -1;
            if (!step (x, y)) {
                return 0;
            }
        }
        while (x != tx) {
            x += x < tx ? 1 : -1;
            if (!step (x, y)) {
                return 0;
            }
        }
    }
    while (x != tx || y != ty) {
        if (x != tx) {
            x += x < tx ? 1 : -1;
        } else {
            y += y < ty ? 1 : -1;
        }
        if (!step (x, y)) {
            return 0;
        }
    }
    return 1;
}

/* A door's tile in room r's wall d, at random along it (never a corner) */
static unsigned char dxr, dyr;
static void __fastcall__ door_in (unsigned char r, unsigned char d)
{
    struct room* p = &L->room[r];

    if (d == DIR_N || d == DIR_S) {
        dxr = p->x + random (p->w);
        dyr = d == DIR_N ? p->y - 1 : p->y + p->h;
    } else {
        dyr = p->y + random (p->h);
        dxr = d == DIR_W ? p->x - 1 : p->x + p->w;
    }
}

/* Rooms a and b joined: 1, or 0 (a room in the way) */
static unsigned char __fastcall__ join (unsigned char a, unsigned char b)
{
    struct room* A = &L->room[a];
    struct room* B = &L->room[b];
    unsigned char da, db, ax, ay, bx, by, k;

    if (B->x > A->x + A->w) {
        da = DIR_E;
    } else if (B->x + B->w < A->x) {
        da = DIR_W;
    } else if (B->y > A->y) {
        da = DIR_S;
    } else {
        da = DIR_N;
    }
    db = (da + 2) & 3;
    door_in (a, da);
    ax = dxr;
    ay = dyr;
    door_in (b, db);
    bx = dxr;
    by = dyr;
    if (!leg (ax + dir_x[da], ay + dir_y[da], bx + dir_x[db], by + dir_y[db], da == DIR_E || da == DIR_W)) {
        return 0;
    }
    for (k = 0; k < pn; ++k) {
        L->tile[py[k]][px[k]] = T_CORR;
    }
    L->tile[ay][ax] = T_DOOR;
    L->tile[by][bx] = T_DOOR;
    return 1;
}

/* Every room reached from the first: the tiles that can be walked, marked from it (a stack of them, in px, py and
** the stack's own) */
#define SMAX    320
static unsigned char sx[SMAX], sy[SMAX];

static unsigned char connected (void)
{
    unsigned sn = 0;
    unsigned char x, y, d, nx, ny, r, ok = 1;

    sx[0] = L->room[0].x;
    sy[0] = L->room[0].y;
    L->tile[sy[0]][sx[0]] |= T_MARK;
    sn = 1;
    while (sn) {
        --sn;
        x = sx[sn];
        y = sy[sn];
        for (d = 0; d < 4; ++d) {
            nx = x + dir_x[d];
            ny = y + dir_y[d];
            if (nx < LW && ny < LH && T_PASS (L->tile[ny][nx]) && !(L->tile[ny][nx] & T_MARK)) {
                L->tile[ny][nx] |= T_MARK;
                if (sn < SMAX) {
                    sx[sn] = nx;
                    sy[sn] = ny;
                    ++sn;
                } else {
                    ok = 0;                     /* (Too big to follow: made again) */
                }
            }
        }
    }
    for (r = 0; r < L->nrooms; ++r) {
        if (!(L->tile[L->room[r].y][L->room[r].x] & T_MARK)) {
            ok = 0;
        }
    }
    for (y = 0; y < LH; ++y) {
        for (x = 0; x < LW; ++x) {
            L->tile[y][x] &= ~T_MARK;
        }
    }
    return ok;
}

/* A tile of room r's floor, at random, that isn't stairs already */
static void __fastcall__ spot (unsigned char r, unsigned char* x, unsigned char* y)
{
    struct room* p = &L->room[r];

    do {
        *x = p->x + random (p->w);
        *y = p->y + random (p->h);
    } while (L->tile[*y][*x] != T_FLOOR);
}

void __fastcall__ gen_level (struct level* l, unsigned char depth, unsigned long seed)
{
    unsigned char i, up, down;

    L = l;
    rseed (seed ^ ((unsigned long) (depth + 1) * 40503UL));
    do {
        memset (L, 0, sizeof *L);
        L->depth = depth;
        rooms ();
        if (L->nrooms < 4) {
            continue;
        }
        sort ();
        for (i = 0; i + 1 < L->nrooms; ++i) {
            if (!join (i, i + 1) && i + 2 < L->nrooms) {
                join (i, i + 2);
            }
        }
        for (i = 0; i + 2 < L->nrooms; i += 2) {
            if (!random (2)) {
                join (i, i + 2);
            }
        }
    } while (L->nrooms < 4 || !connected ());
    up = random (L->nrooms);
    do {
        down = random (L->nrooms);
    } while (down == up);
    spot (up, &L->upx, &L->upy);
    L->tile[L->upy][L->upx] = T_UP;
    if (depth + 1 < NLEVELS) {
        spot (down, &L->downx, &L->downy);
        L->tile[L->downy][L->downx] = T_DOWN;
    }
    L->made = 1;
}
