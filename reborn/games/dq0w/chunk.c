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

/* at ()'s quick way (at.s): the stratum it found last (0xFF: none), its bounds; the chunk, its first square in the
** stratum's own, its bank, its squares */
unsigned char at_sn = 0xFF;
unsigned at_x0, at_y0, at_w, at_h, at_ox, at_oy;
unsigned char at_bank;
struct chunk* at_chunk;
unsigned char* at_tile;

void __fastcall__ chunks_init (unsigned char who)
{
    me = who;
    sync_strata ();
}

void sync_strata (void)
{
    at_sn = 0xFF;                               /* (at ()'s quick way: the bounds found again) */
    hub_map (WORLD_BANK);                       /* (The records the world bank's: copied out, the header back) */
    memcpy (strata, WREC->strata, sizeof strata);
    hub_map (0);
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
    hub_call (q, T_MAKER);                      /* (Asleep till the maker rings with it) */
    q->state = REQ_FREE;
    if (kind != M_CHUNK) {
        sync_strata ();
    }
}

/* The slot holding chunk (cx, cy) of stratum sn, or 0xFF: the world bank in the window (the cache's table) */
static unsigned char __fastcall__ find (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k;
    struct cslot* s = WREC->cslot;

    hub_map (WORLD_BANK);
    for (k = 0; k < NCSLOTS; ++k, ++s) {
        if (s->used && s->sn == sn && s->cx == cx && s->cy == cy) {
            s->stamp = ++WREC->stamp;
            WREC->inuse[me] = k;
            return k;
        }
    }
    return 0xFF;
}

/* at () the long way (at.s: its quick way, the chunk found last, in assembly; a look about asks for thousands): the
** square's chunk found in the hub's cache (or made), put in the window, and the square in it; the quick way's set
** for it */
unsigned char* __fastcall__ at_find (unsigned char sn, unsigned x, unsigned y)
{
    struct stratum* s = &STRATUM (sn);
    unsigned ox = x - s->x0, oy = y - s->y0, cx, cy;
    unsigned char k;

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
        for (lbank = CACHE_BANK (0); k >= 3; k -= 3) {     /* (CACHE_BANK (k) and CHUNK_AT (k) without dividing or
                                                **   multiplying: cc65's slow) */
            ++lbank;
        }
        lchunk = (struct chunk*) (0x8000 + (k == 0 ? 0 : k == 1 ? CHUNK_SIZE : 2 * CHUNK_SIZE));
    }
    hub_map (lbank);
    C = lchunk;
    at_x0 = s->x0;                              /* (The quick way, the next time: this chunk) */
    at_y0 = s->y0;
    at_w = s->w;
    at_h = s->h;
    at_ox = cx << CW_BITS;
    at_oy = cy << CW_BITS;
    at_bank = lbank;
    at_chunk = C;
    at_tile = &C->tile[0][0];
    at_sn = sn;
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

#ifndef CHUNK_NO_ROOMS
/* The step from (x, y) to the square beside it d only its passage's breadth, no way on (the user's, 2026-10-21: a wide
** passage is one passage, a fork only where a way truly branches off): that square and this one going on side by side
** (both a way across d), nothing out of it this one hasn't too, and nothing past it that way (two squares across;
** three, the middle's the same).  (x, y) to d walkable, as step_ok has it */
#ifdef BREADTH_PART2
#pragma code-name (push, "SW2CODE")             /* (The senses': part 2's, a look's, sight.c's and felt's) */
#endif
#pragma static-locals (push, on)
unsigned char __fastcall__ breadth (unsigned char sn, unsigned x, unsigned y, unsigned char d)
{
    unsigned char n, e, a, b, u, v;
    unsigned nx, ny;

    for (n = 0; n < 2; ++n) {
        nx = x + dir_x[d];
        ny = y + dir_y[d];
        e = (d + 1) & 3;
        a = step_ok (sn, x, y, e);
        b = step_ok (sn, x, y, e ^ 2);
        u = step_ok (sn, nx, ny, e);
        v = step_ok (sn, nx, ny, e ^ 2);
        if (!((a && u) || (b && v)) || (u && !a) || (v && !b)) {
            return 0;                           /* (Not alongside, or a way out of it this one hasn't: a way off) */
        }
        if (!step_ok (sn, nx, ny, d)) {
            return 1;                           /* (Its far side: the passage's breadth) */
        }
        x = nx;
        y = ny;
    }
    return 0;                                   /* (Wider than three: a way) */
}

/* The ways on from (x, y) heading d (a walk's: of the four, not back, nor a wide passage's breadth, nor an opening
** into the room the square before opened into, way_room: a passage along a room's side one way into it): how many;
** one of them in way_on; the room it opens into now in way_room */
unsigned char way_on, way_room = 0xFF;
unsigned char __fastcall__ ways_from (unsigned char sn, unsigned x, unsigned y, unsigned char d)
{
    unsigned char k, nd = 0, r, rm = 0xFF;

    for (k = 0; k < 4; ++k) {
        if (k != ((d + 2) & 3) && step_ok (sn, x, y, k) && !breadth (sn, x, y, k)) {
            if ((r = room_of (sn, x + dir_x[k], y + dir_y[k])) != 0xFF && (rm = r) == way_room) {
                continue;
            }
            ++nd;
            way_on = k;
        }
    }
    way_room = rm;
    return nd;
}
#pragma static-locals (pop)
#ifdef BREADTH_PART2
#pragma code-name (pop)
#endif
#endif

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

#ifndef CHUNK_NO_ROOMS                          /* (A task that never asks a square's room, nor a wall's: its room) */
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
#endif

#ifdef CHUNK_ZONES                              /* (Only for a task that asks: none yet, its room the others') */
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

#ifndef CHUNK_NO_ROOMS
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
#endif
