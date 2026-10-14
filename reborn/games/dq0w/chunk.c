/*
** chunk.c - (the world's, the denizens') the dungeon's squares reached (world.h): a square's chunk found in the
** hub's cache and put in the window, or, if it isn't there yet, asked of the maker (games/dq0m), which makes it
** (the same chunk whoever asks: the seed's) while this task waits.  The slot found last is this task's in use, which
** the maker never makes way for; the rest go least lately found first.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"

struct stratum strata[MAXSTRATA];
struct chunk* C;
unsigned char cx_, cy_;

static unsigned char me;                        /* ASK_WORLD ... */
static unsigned char lastk = 0xFF, lsn;         /* The slot at () found last, and what it holds */
static unsigned lcx, lcy;

void __fastcall__ chunks_init (unsigned char who)
{
    me = who;
    sync_strata ();
}

void sync_strata (void)
{
    hub_map (0);
    memcpy (strata, HUB->strata, sizeof strata);
}

void __fastcall__ ask_maker (unsigned char kind, unsigned char sn, unsigned cx, unsigned cy)
{
    struct mreq* q;

    hub_map (0);
    q = &HUB->mreq[me];
    q->kind = kind;
    q->sn = sn;
    q->cx = cx;
    q->cy = cy;
    q->state = REQ_POSTED;
    hub_ring (T_MAKER);
    while (q->state != REQ_DONE) {
        hy_yield ();
        hub_map (0);
    }
    q->state = REQ_FREE;
    if (kind != M_CHUNK) {
        memcpy (strata, HUB->strata, sizeof strata);
    }
}

/* The slot holding chunk (cx, cy) of stratum sn, or 0xFF: the hub's header in the window */
static unsigned char __fastcall__ find (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k;
    struct cslot* s;

    hub_map (0);
    for (k = 0; k < NCSLOTS; ++k) {
        s = &HUB->cslot[k];
        if (s->used && s->sn == sn && s->cx == cx && s->cy == cy) {
            s->stamp = ++HUB->stamp;
            HUB->inuse[me] = k;
            return k;
        }
    }
    return 0xFF;
}

unsigned char* __fastcall__ at (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &strata[sn];
    unsigned ox = x - s->x0, oy = y - s->y0, cx, cy;
    unsigned char k;

    if (ox >= s->w || oy >= s->h) {
        return 0;
    }
    cx = ox / CW;
    cy = oy / CW;
    cx_ = ox % CW;
    cy_ = oy % CW;
    if (lastk == 0xFF || lsn != sn || lcx != cx || lcy != cy) {
        if ((k = find (sn, cx, cy)) == 0xFF) {
            ask_maker (M_CHUNK, sn, cx, cy);
            k = find (sn, cx, cy);
        }
        lastk = k;
        lsn = sn;
        lcx = cx;
        lcy = cy;
    }
    hub_map (CACHE_BANK (lastk));
    C = CHUNK_AT (lastk);
    return &C->tile[cy_][cx_];
}

signed char __fastcall__ elev_at (unsigned char sn, unsigned x, unsigned y)
{
    return at (sn, x, y) ? C->elev[cy_][cx_] : 0;
}

/* From (x, y) to the square beside it d: its kind can be walked, and the rise or fall isn't more than a step's
** (more on steps) */
unsigned char __fastcall__ step_ok (unsigned char sn, unsigned x, unsigned y, unsigned char d)
{
    unsigned char* p = at (sn, x, y);
    unsigned char t, u;
    int z, dz;

    if (!p) {
        return 0;
    }
    t = *p;
    z = C->elev[cy_][cx_];
    if (!(p = at (sn, x + dir_x[d], y + dir_y[d]))) {
        return 0;
    }
    u = *p;
    if (!T_PASS (u)) {
        return 0;
    }
    dz = C->elev[cy_][cx_] - z;
    if (dz < 0) {
        dz = -dz;
    }
    return dz <= (TILE (t) == T_STEPS || TILE (u) == T_STEPS ? STAIR_MAX : STEP_MAX);
}

unsigned char __fastcall__ room_of (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char r;
    struct room* p;

    if (!at (sn, x, y)) {
        return 0xFF;
    }
    for (r = 0; r < C->nrooms; ++r) {
        p = &C->room[r];
        if (cx_ >= p->x && cx_ < p->x + p->w && cy_ >= p->y && cy_ < p->y + p->h &&
            TILE (C->tile[cy_][cx_]) != T_CORR && TILE (C->tile[cy_][cx_]) != T_WALL) {
            return r;                           /* (A cave's bounds may take in a tunnel's squares, or its walls) */
        }
    }
    return 0xFF;
}

unsigned char __fastcall__ zone_of (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char z;
    struct zone* p;

    if (!at (sn, x, y)) {
        return 0xFF;
    }
    for (z = 0; z < C->nzones; ++z) {
        p = &C->zone[z];
        if (cx_ >= p->x && cx_ < p->x + p->w && cy_ >= p->y && cy_ < p->y + p->h) {
            return z;
        }
    }
    return 0xFF;
}

unsigned char wall_x, wall_y;

unsigned char __fastcall__ wall_tile (struct room* p, unsigned char d, unsigned char k)
{
    if (d == DIR_N || d == DIR_S) {
        if (k >= p->w) {
            return 0;
        }
        wall_x = p->x + k;
        wall_y = d == DIR_N ? p->y - 1 : p->y + p->h;
    } else {
        if (k >= p->h) {
            return 0;
        }
        wall_y = p->y + k;
        wall_x = d == DIR_W ? p->x - 1 : p->x + p->w;
    }
    return 1;
}
