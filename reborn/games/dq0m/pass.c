/*
** pass.c - (dq0m) a purpose-built chunk's passages (built.c's chunk, in the main RAM): rooms joined by passages (a
** door in each one's wall facing the other, a dog-leg between that gives up rather than cut through a room), their
** floors running evenly from one door's elevation to the other's, steps where too steep to walk; each way across the
** chunk's edges joined to the nearest room (or, with none, to the chunk's middle).  (Overlay 2, the maker's: built.c
** calls it through bank.s's stubs.)
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")

#include <dice.h>
#include "../dq0w/world.h"
#include "grow.h"

#define PMAX    96                              /* A passage's squares, at most */
static unsigned char px[PMAX], py[PMAX];
static signed char pz[PMAX];
static unsigned char pn;

/* The stratum's ground at a square of the chunk: its corners', evenly between (built.c has its own) */
static int __fastcall__ gz (unsigned char x, unsigned char y)
{
    long top = (long) E->ground[0] * (CW - 1 - x) + (long) E->ground[1] * x;
    long bottom = (long) E->ground[2] * (CW - 1 - x) + (long) E->ground[3] * x;

    return (int) ((top * (CW - 1 - y) + bottom * y) / ((CW - 1) * (CW - 1)));
}

static signed char __fastcall__ clampz (int z)
{
    return z > 120 ? 120 : z < -120 ? -120 : z;
}

/* ---- Passages */

/* A square of a passage, if it can be dug: 0 if it's in the way (a room's), or out of the bounds */
static unsigned char __fastcall__ dig (unsigned char x, unsigned char y)
{
    unsigned char t;

    if (x < E->x0 || x > E->x1 || y < E->y0 || y > E->y1) {
        return 0;
    }
    t = TILE (L->tile[y][x]);
    if (t != T_ROCK && t != T_CORR && t != T_STEPS) {
        return 0;
    }
    if (t == T_STEPS) {                         /* (A room's steps aren't a passage's) */
        for (t = 0; t < L->nrooms; ++t) {
            if (x >= L->room[t].x && x < L->room[t].x + L->room[t].w && y >= L->room[t].y &&
                y < L->room[t].y + L->room[t].h) {
                return 0;
            }
        }
    }
    if (pn < PMAX) {
        px[pn] = x;
        py[pn] = y;
        ++pn;
    }
    return pn < PMAX;
}

/* A dog-leg from (x, y) to (tx, ty), both dug: along the first axis to half way, across, then on */
static unsigned char __fastcall__ leg (unsigned char x, unsigned char y, unsigned char tx, unsigned char ty,
    unsigned char across)
{
    unsigned char m;

    pn = 0;
    if (!dig (x, y)) {
        return 0;
    }
    if (across) {
        m = (x + tx) / 2;
        while (x != m) {
            x += x < m ? 1 : -1;
            if (!dig (x, y)) {
                return 0;
            }
        }
        while (y != ty) {
            y += y < ty ? 1 : -1;
            if (!dig (x, y)) {
                return 0;
            }
        }
    } else {
        m = (y + ty) / 2;
        while (y != m) {
            y += y < m ? 1 : -1;
            if (!dig (x, y)) {
                return 0;
            }
        }
        while (x != tx) {
            x += x < tx ? 1 : -1;
            if (!dig (x, y)) {
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
        if (!dig (x, y)) {
            return 0;
        }
    }
    return 1;
}

/* The passage leg () dug made: its floor evenly from za (its first square's, exactly: a door's floor, or the ground
** at the chunk's edge, where the next chunk's passage meets it) to zb (after its last); where it meets a passage there
** already, that one's; steps where a step's rise is more than a walk's.  0: too steep, or a passage there at
** another height */
static unsigned char __fastcall__ lay (int za, int zb)
{
    unsigned char k, t;
    int dz;

    for (k = 0; k < pn; ++k) {
        pz[k] = clampz (za + (zb - za) * k / pn);
        t = TILE (L->tile[py[k]][px[k]]);
        if (t != T_ROCK) {
            dz = L->elev[py[k]][px[k]] - pz[k];
            if (dz > 2 || dz < -2) {
                return 0;
            }
            pz[k] = L->elev[py[k]][px[k]];
        }
    }
    for (k = 0; k <= pn; ++k) {
        dz = (k < pn ? pz[k] : zb) - (k ? pz[k - 1] : za);
        if (dz > STAIR_MAX || dz < -STAIR_MAX) {
            return 0;
        }
    }
    for (k = 0; k < pn; ++k) {
        t = TILE (L->tile[py[k]][px[k]]);
        L->elev[py[k]][px[k]] = pz[k];
        if (t == T_ROCK) {
            L->tile[py[k]][px[k]] = T_CORR;
        }
        dz = pz[k] - (k ? pz[k - 1] : za);
        if (dz > STEP_MAX || dz < -STEP_MAX) {
            L->tile[py[k]][px[k]] = T_STEPS;
            if (k) {
                L->tile[py[k - 1]][px[k - 1]] = T_STEPS;
            }
        }
    }
    dz = zb - pz[pn - 1];
    if (dz > STEP_MAX || dz < -STEP_MAX) {
        L->tile[py[pn - 1]][px[pn - 1]] = T_STEPS;
    }
    return 1;
}

/* A door's square in room r's wall d, at random along it (never a corner), and the floor's elevation inside it */
static unsigned char dxr, dyr;
static int dzr;
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
    dzr = L->elev[dyr - dir_y[d]][dxr - dir_x[d]];
}

/* A door at (x, y): its square, and its portal (most closed; some locked, some barred from inside the room; wood
** mostly, some iron-bound, a few iron) */
/* A door's own dice: from its square and its chunk's place, not the grower's (so its details never move the layout) */
static unsigned long dh;
static unsigned char __fastcall__ dr (unsigned char n)
{
    dh ^= dh << 13;
    dh ^= dh >> 17;
    dh ^= dh << 5;
    return (unsigned char) ((dh >> 8) % n);
}

/* The door at (x, y) is a way out of a room a link comes out in (a stairway's, a ladder's: the way in's): never hidden,
** so nobody begins shut in */
static unsigned char __fastcall__ by_link (unsigned char x, unsigned char y)
{
    unsigned char d, i, r, nx, ny;
    const struct room* p;

    for (d = 0; d < 4; ++d) {
        nx = x + dir_x[d];
        ny = y + dir_y[d];
        for (r = 0, p = L->room; r < L->nrooms; ++r, ++p) {
            if (nx < p->x || nx >= p->x + p->w || ny < p->y || ny >= p->y + p->h) {
                continue;
            }
            for (i = 0; i < E->nland; ++i) {
                if (E->lx[i] >= p->x && E->lx[i] < p->x + p->w && E->ly[i] >= p->y && E->ly[i] < p->y + p->h) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void __fastcall__ door (unsigned char x, unsigned char y, int z)
{
    struct portal* p;
    unsigned char d, k;

    L->tile[y][x] = T_DOOR;
    L->elev[y][x] = z;
    for (k = 0, p = L->portal; k < L->nportals; ++k, ++p) {
        if (p->x == x && p->y == y) {
            return;                             /* (Its door made already, by another way into the room (door_in
                                                **   chose its square again): one portal a door, or the fixtures put
                                                **   a trap on it twice, the senses saw it twice) */
        }
    }
    if (L->nportals >= NPORTALS) {
        return;
    }
    dh =((unsigned long) L->cx << 20 ^ (unsigned long) L->cy << 8 ^ (unsigned) x << 5 ^ y) * 2654435761UL + 1;
    k = dr (20);
    p = &L->portal[L->nportals++];
    p->x = x;
    p->y = y;
    p->kind = PK_DOOR;
    p->state = k < SP->d_open ? PS_OPEN : 0;    /* (By the style's proportions) */
    p->lock = 0;
    if (k >= 20 - SP->d_locked - SP->d_barred && k < 20 - SP->d_barred) {
        p->lock = 1 + dr (5);
        p->state = PS_LOCKED;
    } else if (k >= 20 - SP->d_barred) {
        p->state = PS_BARRED;
    }
    if (dr (15) < SP->d_hidden && !by_link (x, y)) {    /* (A secret door: closed, like the wall; some by magic) */
        p->state = PS_HIDDEN | (dr (4) < SP->d_magic ? PS_MAGIC : 0);
    }
    k = dr (10);
    p->mat = k < SP->m_wood ? PM_WOOD : k < SP->m_wood + SP->m_ironbound ? PM_IRONBOUND : PM_IRON;
    p->bar = DIR_N;
    for (d = 0; d < 4; ++d) {                   /* (Barred from the room's side) */
        if (x + dir_x[d] < CW && y + dir_y[d] < CW && L->tile[y + dir_y[d]][x + dir_x[d]] == T_FLOOR) {
            p->bar = d;
        }
    }
    if (dr (20) >= SP->doors) {                 /* (No door at all: a doorway, by the style's say) */
        --L->nportals;
    }
}

/* The way room a faces room b (or a square: b 0xFF, the square bx, by) */
static unsigned char __fastcall__ facing (unsigned char a, unsigned char bx, unsigned char by)
{
    struct room* A = &L->room[a];

    if (bx >= A->x + A->w) {
        return by >= A->y && by < A->y + A->h || (bx - (A->x + A->w)) >= (by < A->y ? A->y - by : by - A->y - A->h) ?
            DIR_E : by < A->y ? DIR_N : DIR_S;
    }
    if (bx < A->x) {
        return by >= A->y && by < A->y + A->h || (A->x - bx) >= (by < A->y ? A->y - by : by - A->y - A->h) ?
            DIR_W : by < A->y ? DIR_N : DIR_S;
    }
    return by < A->y ? DIR_N : DIR_S;
}

/* Rooms a and b joined: 1, or 0 */
unsigned char __fastcall__ o_join (unsigned char a, unsigned char b)
{
    struct room* B = &L->room[b];
    unsigned char da, db, ax, ay;
    int za;

    da = facing (a, B->x + B->w / 2, B->y + B->h / 2);
    db = (da + 2) & 3;
    door_in (a, da);
    ax = dxr;
    ay = dyr;
    za = dzr;
    door_in (b, db);
    if (!leg (ax + dir_x[da], ay + dir_y[da], dxr + dir_x[db], dyr + dir_y[db], da == DIR_E || da == DIR_W) ||
        !lay (za, dzr)) {
        return 0;
    }
    door (ax, ay, za);
    door (dxr, dyr, dzr);
    return 1;
}

/* The way across edge d (at its square sx, sy) joined to the nearest room, or (none) to the square mx, my: 1, or 0 */
static unsigned char __fastcall__ cross (unsigned char d, unsigned char sx, unsigned char sy)
{
    unsigned char r, best = 0xFF, dr;
    unsigned dist, bd = 0xFFFF;
    int zs = gz (sx, sy);

    for (r = 0; r < L->nrooms; ++r) {
        dist = (unsigned) (sx > L->room[r].x ? sx - L->room[r].x : L->room[r].x - sx) +
            (sy > L->room[r].y ? sy - L->room[r].y : L->room[r].y - sy);
        if (dist < bd) {
            bd = dist;
            best = r;
        }
    }
    if (best == 0xFF) {
        return leg (sx, sy, mx, my, d == DIR_E || d == DIR_W) && lay (zs, gz (mx, my));
    }
    dr = facing (best, sx, sy);
    door_in (best, dr);
    if (!leg (sx, sy, dxr + dir_x[dr], dyr + dir_y[dr], d == DIR_E || d == DIR_W) || !lay (zs, dzr)) {
        return 0;
    }
    door (dxr, dyr, dzr);
    return 1;
}

/* Every way across the edges joined */
unsigned char o_crossings (void)
{
    return (E->way[DIR_N] == NO_WAY || cross (DIR_N, E->way[DIR_N], 0)) &&
        (E->way[DIR_S] == NO_WAY || cross (DIR_S, E->way[DIR_S], CW - 1)) &&
        (E->way[DIR_W] == NO_WAY || cross (DIR_W, 0, E->way[DIR_W])) &&
        (E->way[DIR_E] == NO_WAY || cross (DIR_E, CW - 1, E->way[DIR_E]));
}
