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
static unsigned char shut_ok (void);
static unsigned char lastk = 0xFF, lsn;         /* The slot at () found last, and what it holds */
static unsigned lcx, lcy;
static unsigned char lbank;                     /* (Its bank, its chunk in the window) */
static struct chunk* lchunk;
static unsigned char fsn = 0xFF;                /* (at ()'s quick way: the stratum it found last, its bounds, */
static unsigned fx0, fy0, fw, fh;               /*   the first square of the chunk, in the stratum's own squares, */
static unsigned fox, foy, fdx, fdy;             /*   and a square's place from them) */
static unsigned char* ftile;

void __fastcall__ chunks_init (unsigned char who)
{
    me = who;
    sync_strata ();
}

void sync_strata (void)
{
    fsn = 0xFF;                                 /* (at ()'s quick way: the bounds found again) */
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
        fsn = 0xFF;
        memcpy (strata, HUB->strata, sizeof strata);
    }
}

/* The slot holding chunk (cx, cy) of stratum sn, or 0xFF: the hub's header in the window */
static unsigned char __fastcall__ find (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k;
    struct cslot* s = HUB->cslot;

    hub_map (0);
    for (k = 0; k < NCSLOTS; ++k, ++s) {
        if (s->used && s->sn == sn && s->cx == cx && s->cy == cy) {
            s->stamp = ++HUB->stamp;
            HUB->inuse[me] = k;
            return k;
        }
    }
    return 0xFF;
}

/* The square in the chunk at () found last, in its stratum (the quick way: no multiplying, no shifting, nothing on
** the C stack but the arguments; a look about asks for thousands): its chunk in the window; or NULL, another chunk */
static unsigned char* __fastcall__ again (unsigned char sn, unsigned x, unsigned y)
{
    if (sn != fsn || (fdx = x - fx0) >= fw || (fdx -= fox) >= CW || (fdy = y - fy0) >= fh || (fdy -= foy) >= CW) {
        return 0;
    }
    cx_ = fdx;
    cy_ = fdy;
    hub_map (lbank);
    C = lchunk;
    fdy <<= CW_BITS;
    return ftile + fdy + cx_;
}

unsigned char* __fastcall__ at (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s;
    unsigned ox, oy, cx, cy;
    unsigned char k;
    unsigned char* p;

    if ((p = again (sn, x, y)) != 0) {
        return p;
    }
    s = &strata[sn];
    ox = x - s->x0;
    oy = y - s->y0;
    if (ox >= s->w || oy >= s->h) {
        return 0;
    }
    cx = ox >> CW_BITS;
    cy = oy >> CW_BITS;
    cx_ = ox & (CW - 1);
    cy_ = oy & (CW - 1);
    if (lastk == 0xFF || lsn != sn || lcx != cx || lcy != cy) {
        if ((k = find (sn, cx, cy)) == 0xFF) {
            ask_maker (M_CHUNK, sn, cx, cy);
            k = find (sn, cx, cy);
        }
        lastk = k;
        lsn = sn;
        lcx = cx;
        lcy = cy;
        lbank = CACHE_BANK (k);                 /* (Worked out once a slot: cc65 divides slowly) */
        lchunk = CHUNK_AT (k);
    }
    hub_map (lbank);
    C = lchunk;
    fsn = sn;                                   /* (The quick way, the next time: this chunk) */
    fx0 = s->x0;
    fy0 = s->y0;
    fw = s->w;
    fh = s->h;
    fox = cx << CW_BITS;
    foy = cy << CW_BITS;
    ftile = &C->tile[0][0];
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
    if (!T_PASS (u) || (TILE (u) == T_DOOR && !shut_ok ())) {
        return 0;
    }
    dz = C->elev[cy_][cx_] - z;
    if (dz < 0) {
        dz = -dz;
    }
    return dz <= (TILE (t) == T_STEPS || TILE (u) == T_STEPS ? STAIR_MAX : STEP_MAX);
}

struct portal* __fastcall__ portal_at (unsigned char x, unsigned char y)
{
    struct portal* p = C->portal;
    unsigned char i;

    for (i = C->nportals; i; --i, ++p) {
        if (p->x == x && p->y == y) {
            return p;
        }
    }
    return 0;
}

/* The door at at ()'s square open (or none on it): it can be walked through */
static unsigned char shut_ok (void)
{
    struct portal* p = portal_at (cx_, cy_);

    return !p || (p->state & (PS_OPEN | PS_HIDDEN)) == PS_OPEN;
}

unsigned char __fastcall__ room_of (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char r;
    struct room* p;

    if (!at (sn, x, y)) {
        return 0xFF;
    }
    for (r = 0, p = C->room; r < C->nrooms; ++r, ++p) {
        if (cx_ >= p->x && cx_ < p->x + p->w && cy_ >= p->y && cy_ < p->y + p->h &&
            TILE (C->tile[cy_][cx_]) != T_CORR && TILE (C->tile[cy_][cx_]) != T_WALL) {
            return r;                           /* (A cave's bounds may take in a tunnel's squares, or its walls) */
        }
    }
    return 0xFF;
}

#ifndef CHUNK_NO_ZONES                          /* (A task that never asks: its room) */
unsigned char __fastcall__ zone_of (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char z;
    struct zone* p;

    if (!at (sn, x, y)) {
        return 0xFF;
    }
    for (z = 0, p = C->zone; z < C->nzones; ++z, ++p) {
        if (cx_ >= p->x && cx_ < p->x + p->w && cy_ >= p->y && cy_ < p->y + p->h) {
            return z;
        }
    }
    return 0xFF;
}
#endif

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
