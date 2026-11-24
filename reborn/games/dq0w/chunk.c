/*
** chunk.c - (the world's, the denizens') the dungeon's squares reached (world.h): a square's chunk found in the
** hub's cache and put in the window, or, if it isn't there yet, asked of the maker (games/dq0m), which makes it
** (the same chunk whoever asks: the seed's) while this task waits.  The slot found last is this task's in use, which
** the maker never makes way for; the rest go least lately found first (the record store's: ../dq0/rp.h's RP_FIND,
** RP_OLDEST the maker's).  The lookup itself the game's library's (the paged ROM's AT_FIND, ../dq0/lnzp.h: at.s's
** at () the long way, 2026-11-14; chunk.c's at_find and find before), the task's chunk state at.s's.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "../dq0/lnzp.h"

struct stratum strata[MAXSTRATA];

static unsigned char shut_ok (void);
extern unsigned char at_sn;                     /* (at.s's: at ()'s quick way, the stratum it found last: 0xFF none) */

#ifndef CHUNK_READER
#pragma code-name (push, "SW_dungeon_begin_CODE")             /* (The world's: its start's alone, part 3's, s_start's: the main RAM's
                                                **   room, 2026-10-30) */
#endif
void __fastcall__ chunks_init (unsigned char who)
{
    at_me = who;
    lz_init ();
    sync_strata ();
}
#ifndef CHUNK_READER
#pragma code-name (pop)
#endif

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
    q = &HUB->mreq[at_me];
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

#ifndef CHUNK_NO_ELEV                           /* (A task that never asks a square's height: the minds', its room) */
signed char __fastcall__ elev_at (unsigned char sn, unsigned x, unsigned y)
{
    return at (sn, x, y) ? C->elev[cy_][cx_] : 0;
}
#endif

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

/* (slant.s: slant, a diagonal way past two corners not stood on, one cut; step8, the four's or a diagonal way) */

#ifndef CHUNK_NO_ROOMS
/* The step from (x, y) to the square beside it d only its passage's breadth, no way on (the user's, 2026-10-21: a wide
** passage is one passage, a fork only where a way truly branches off): that square and this one going on side by side
** (both a way across d), nothing out of it this one hasn't too, and nothing past it that way (two squares across;
** three, the middle's the same).  (x, y) to d walkable, as step_ok has it */
#ifdef BREADTH_IN                               /* (A reader's part, its chunk.c's "SW_name_": the senses' perceive, a
                                                **   look's, sight.c's and felt's) */
#pragma code-name (push, BREADTH_IN "CODE")
#endif
#pragma static-locals (push, on)
#include "../dq0w/ways.inc"
#pragma static-locals (pop)
#ifdef BREADTH_IN
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
