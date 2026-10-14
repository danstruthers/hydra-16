/*
** built.c - (dq0m) a purpose-built chunk grown (DUNGEON.md), in the window: rooms round its links first (a stairway's
** or a ladder's foot under its head), then the rest at random where they fit in the chunk and the stratum's bounds,
** a wall round each and a square of rock between; each room's floor one to three zones (a plane each, from the
** stratum's ground there: an elevation, maybe a slope; a ceiling: its kind, its height at the middle and at the
** walls), steps where two zones meet too steeply to walk; sorted from the west, each room joined to the next by a
** passage (a door in each one's wall facing the other, a dog-leg between that gives up rather than cut through a
** room), its floor running evenly from one door's elevation to the other's, steps where too steep; a few more for
** loops; each way across the chunk's edges joined to the nearest room; everything reached from everything, walking,
** or the chunk grown again (and at the last, a chunk of passages alone, meeting in its middle).  Some rooms are lit,
** fewer the deeper the stratum (a stand-in for the theme's say).
*/

#include <string.h>
#include <dice.h>
#include "../dq0w/world.h"

static struct chunk* L;
static const struct edges* E;

#define PMAX    96                              /* A passage's squares, at most */
static unsigned char px[PMAX], py[PMAX];
static signed char pz[PMAX];
static unsigned char pn;

/* The stratum's ground at a square of the chunk: its corners', evenly between */
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

/* ---- Rooms */

/* A room's floor (x, y, w, h) fits: its walls in the bounds, and the rectangle round them (a square more) all rock;
** off an edge that has a way across (the edge's squares are the passages') */
static unsigned char __fastcall__ fits (unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i, j, x0, y0, x1, y1;

    if (x < (E->way[DIR_W] != NO_WAY ? 2 : E->x0 + 1) || y < (E->way[DIR_N] != NO_WAY ? 2 : E->y0 + 1) ||
        x + w > (E->way[DIR_E] != NO_WAY ? CW - 2 : E->x1) || y + h > (E->way[DIR_S] != NO_WAY ? CW - 2 : E->y1)) {
        return 0;
    }
    x0 = x < E->x0 + 2 ? E->x0 : x - 2;
    y0 = y < E->y0 + 2 ? E->y0 : y - 2;
    x1 = x + w + 1 > E->x1 ? E->x1 : x + w + 1;
    y1 = y + h + 1 > E->y1 ? E->y1 : y + h + 1;
    for (j = y0; j <= y1; ++j) {
        for (i = x0; i <= x1; ++i) {
            if (L->tile[j][i] != T_ROCK) {
                return 0;
            }
        }
    }
    return 1;
}

static void __fastcall__ put_room (unsigned char x, unsigned char y, unsigned char w, unsigned char h)
{
    unsigned char i, j;
    struct room* r = &L->room[L->nrooms++];

    r->x = x;
    r->y = y;
    r->w = w;
    r->h = h;
    r->flags = random (8) + strata[L->sn].n < 5 ? R_LIT : 0;
    r->kind = random (8);
    for (j = y - 1; j <= y + h; ++j) {
        for (i = x - 1; i <= x + w; ++i) {
            L->tile[j][i] = j < y || j == y + h || i < x || i == x + w ? T_WALL : T_FLOOR;
        }
    }
}

/* A room round a link's square (lx, ly): 1, or 0 (no room for one) */
static unsigned char __fastcall__ landing_room (unsigned char lx, unsigned char ly)
{
    unsigned char tries, w, h, x, y;

    for (tries = 0; tries < 40; ++tries) {
        w = 2 + random (tries < 20 ? 7 : 2);
        h = 2 + random (tries < 20 ? 3 : 1);
        x = lx - random (w);
        y = ly - random (h);
        if (lx >= x && ly >= y && fits (x, y, w, h)) {
            put_room (x, y, w, h);
            return 1;
        }
    }
    return 0;
}

static void rooms (void)
{
    unsigned char tries, w, h, x, y;

    for (tries = 0; tries < 60 && L->nrooms < NROOMS; ++tries) {
        w = 3 + random (8);
        h = 2 + random (4);
        x = 2 + random (CW - 4 - w);
        y = 2 + random (CW - 4 - h);
        if (fits (x, y, w, h)) {
            put_room (x, y, w, h);
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

/* ---- Zones */

/* Zone z's ceiling: a kind, its height at the middle and at the walls (feet above the floor) */
static void __fastcall__ ceiling (struct zone* z)
{
    unsigned char k = random (20);

    z->ceil = k < 6 ? C_FLAT : k < 10 ? C_BARREL : k < 13 ? C_DOME : k < 15 ? C_CATHEDRAL : k < 17 ? C_SLOPED : C_ROUGH;
    if (z->ceil == C_DOME && (z->w < 3 || z->h < 3)) {
        z->ceil = C_BARREL;
    }
    z->cmid = 8 + random (z->ceil == C_CATHEDRAL ? 30 : 16);
    switch (z->ceil) {
    case C_FLAT:
        z->cwall = z->cmid;
        break;
    case C_ROUGH:
        z->cwall = z->cmid - random (4);
        break;
    default:                                    /* (A vault, a dome, a slope: lower at the walls, or at one wall) */
        z->cwall = 7 + random (z->cmid - 6);
        break;
    }
}

/* Squares s0 to s1 along row (across) or column k made steps, evenly from the floor just before s0 to just after
** s1 */
static void __fastcall__ steps (unsigned char across, unsigned char k, unsigned char s0, unsigned char s1)
{
    int za = across ? L->elev[k][s0 - 1] : L->elev[s0 - 1][k];
    int zb = across ? L->elev[k][s1 + 1] : L->elev[s1 + 1][k];
    int n = s1 - s0 + 2;
    unsigned char s;

    for (s = s0; s <= s1; ++s) {
        if (across) {
            L->elev[k][s] = za + (zb - za) * (s - s0 + 1) / n;
            L->tile[k][s] = T_STEPS;
        } else {
            L->elev[s][k] = za + (zb - za) * (s - s0 + 1) / n;
            L->tile[s][k] = T_STEPS;
        }
    }
}

/* Room r's zones: one to three, side by side along its length, each its plane and its ceiling; steps where two
** meet too steeply */
static void __fastcall__ zones (unsigned char r)
{
    struct room* p = &L->room[r];
    struct zone* z;
    unsigned char across = p->w >= p->h, len = across ? p->w : p->h, k = 1, i, x, y, at, part, m, side;
    int zz, dz, worst;

    if (len >= 6 && random (10) < 4) {
        k = len >= 9 && !random (3) ? 3 : 2;
    }
    if (L->nzones + k > NZONES) {
        k = 1;
    }
    p->z0 = L->nzones;
    p->nz = k;
    part = len / k;
    side = across ? p->h : p->w;
    zz = gz (p->x + p->w / 2, p->y + p->h / 2) + (int) random (7) - 3;
    for (i = 0; i < k; ++i) {
        z = &L->zone[L->nzones++];
        z->room = r;
        at = i * part;
        z->x = across ? p->x + at : p->x;
        z->y = across ? p->y : p->y + at;
        z->w = across ? (i + 1 == k ? p->w - at : part) : p->w;
        z->h = across ? p->h : (i + 1 == k ? p->h - at : part);
        if (i) {
            zz += (random (2) ? 1 : -1) * (int) (3 + random (8));
        }
        z->z = clampz (zz);
        z->gx = z->gy = 0;
        if (!random (4)) {                      /* (A floor that slopes evenly: up to a foot and a half a square) */
            if (random (2)) {
                z->gx = (random (2) ? 1 : -1) * (signed char) (1 + random (6));
            } else {
                z->gy = (random (2) ? 1 : -1) * (signed char) (1 + random (6));
            }
        }
        ceiling (z);
        for (y = z->y; y < z->y + z->h; ++y) {
            for (x = z->x; x < z->x + z->w; ++x) {
                L->elev[y][x] = clampz (z->z + (z->gx * (x - z->x) + z->gy * (y - z->y)) / 4);
            }
        }
    }
    /* Where zone i - 1 meets zone i: a slope if every row can be walked, else steps up a row (the rest a ledge) */
    for (i = 1; i < k; ++i) {
        at = (across ? p->x : p->y) + i * part;    /* (Zone i's first column, or row) */
        worst = 0;
        for (m = 0; m < side; ++m) {
            dz = across ? L->elev[p->y + m][at] - L->elev[p->y + m][at - 1] :
                L->elev[at][p->x + m] - L->elev[at - 1][p->x + m];
            if (dz < 0) {
                dz = -dz;
            }
            if (dz > worst) {
                worst = dz;
            }
        }
        if (worst > STEP_MAX && part >= 3) {
            m = random (side);
            steps (across, (across ? p->y : p->x) + m, at - (worst > 12 ? 2 : 1), at + (worst > 12 ? 1 : 0));
        }
    }
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

static void __fastcall__ door (unsigned char x, unsigned char y, int z)
{
    L->tile[y][x] = T_DOOR;
    L->elev[y][x] = z;
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
static unsigned char __fastcall__ join (unsigned char a, unsigned char b)
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
static unsigned char mx, my;
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

/* ---- Everything reached from everything, walking (a stack of squares) */

#define SMAX    128
static unsigned char sx[SMAX], sy[SMAX];

static unsigned char __fastcall__ can (unsigned char x, unsigned char y, unsigned char d)
{
    unsigned char nx = x + dir_x[d], ny = y + dir_y[d], t = L->tile[y][x], u;
    int dz;

    if (nx >= CW || ny >= CW) {
        return 0;
    }
    u = L->tile[ny][nx];
    if (!T_PASS (u)) {
        return 0;
    }
    dz = L->elev[ny][nx] - L->elev[y][x];
    if (dz < 0) {
        dz = -dz;
    }
    return dz <= (TILE (t) == T_STEPS || TILE (u) == T_STEPS ? STAIR_MAX : STEP_MAX);
}

static unsigned char __fastcall__ reached (unsigned char x, unsigned char y)
{
    return (L->tile[y][x] & T_MARK) != 0;
}

static unsigned char connected (void)
{
    unsigned sn = 0;
    unsigned char x, y, d, nx, ny, i, ok = 1;

    if (L->nrooms) {
        sx[0] = L->room[0].x;
        sy[0] = L->room[0].y;
    } else {
        sx[0] = mx;
        sy[0] = my;
    }
    L->tile[sy[0]][sx[0]] |= T_MARK;
    sn = 1;
    while (sn) {
        --sn;
        x = sx[sn];
        y = sy[sn];
        for (d = 0; d < 4; ++d) {
            nx = x + dir_x[d];
            ny = y + dir_y[d];
            if (can (x, y, d) && !reached (nx, ny)) {
                L->tile[ny][nx] |= T_MARK;
                if (sn < SMAX) {
                    sx[sn] = nx;
                    sy[sn] = ny;
                    ++sn;
                } else {
                    ok = 0;                     /* (Too big to follow: grown again) */
                }
            }
        }
    }
    for (i = 0; i < L->nzones; ++i) {
        if (!reached (L->zone[i].x, L->zone[i].y)) {
            ok = 0;
        }
    }
    if ((E->way[DIR_N] != NO_WAY && !reached (E->way[DIR_N], 0)) ||
        (E->way[DIR_S] != NO_WAY && !reached (E->way[DIR_S], CW - 1)) ||
        (E->way[DIR_W] != NO_WAY && !reached (0, E->way[DIR_W])) ||
        (E->way[DIR_E] != NO_WAY && !reached (CW - 1, E->way[DIR_E]))) {
        ok = 0;
    }
    for (i = 0; i < E->nland; ++i) {
        if (!reached (E->lx[i], E->ly[i])) {
            ok = 0;
        }
    }
    for (y = 0; y < CW; ++y) {
        for (x = 0; x < CW; ++x) {
            L->tile[y][x] &= ~T_MARK;
        }
    }
    return ok;
}

/* Every way across the edges joined */
static unsigned char crossings (void)
{
    return (E->way[DIR_N] == NO_WAY || cross (DIR_N, E->way[DIR_N], 0)) &&
        (E->way[DIR_S] == NO_WAY || cross (DIR_S, E->way[DIR_S], CW - 1)) &&
        (E->way[DIR_W] == NO_WAY || cross (DIR_W, 0, E->way[DIR_W])) &&
        (E->way[DIR_E] == NO_WAY || cross (DIR_E, CW - 1, E->way[DIR_E]));
}

static void clear (void)
{
    memset (&L->nrooms, 0, sizeof *L - ((unsigned char*) &L->nrooms - (unsigned char*) L));
}

void __fastcall__ grow_built (struct chunk* c, const struct edges* e, unsigned long seed)
{
    struct stratum* s = &strata[c->sn];
    unsigned char i, r, tries, ok, x, y, k;

    L = c;
    E = e;
    rseed (seed);
    mx = (e->x0 + e->x1) / 2;                   /* (The middle: where passages meet with no room to meet in) */
    my = (e->y0 + e->y1) / 2;
    for (tries = 0; tries < 24; ++tries) {
        clear ();
        ok = 1;
        for (i = 0; i < e->nland; ++i) {        /* (The rooms its links come out in) */
            if (TILE (L->tile[e->ly[i]][e->lx[i]]) != T_FLOOR && !landing_room (e->lx[i], e->ly[i])) {
                ok = 0;
            }
        }
        if (!ok) {
            continue;
        }
        if (tries < 20) {                       /* (At the last, no more rooms than the links') */
            rooms ();
        }
        sort ();
        for (r = 0; r < L->nrooms; ++r) {
            zones (r);
        }
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
        if (crossings () && connected ()) {
            break;
        }
    }
    /* The links' squares: stairways and ladders, up and down (the way out, at the top) */
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
