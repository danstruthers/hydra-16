/*
** wflight.c - (dq0w) a flight between strata crossed by an actor, by the character's rule (the one-path plan's stage 2,
** climbing and links, 2026-11-23: wlink.c's header says how).  Swapped part 114's; its lights wlink.c's, part 115.
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "work.h"
#include "wbody.h"
#include "acts.h"

#pragma local-strings (on)
#pragma code-name ("SW114CODE")
#pragma rodata-name ("SW114RODATA")

#define LINK_R          3                       /* (A flight within reach: its foot this far off at most, as an ally's
                                                **   coming along was, the minds' anear) */

static unsigned __fastcall__ apart (int ax, int ay)
{
    ax = ax < 0 ? -ax : ax;
    ay = ay < 0 ? -ay : ay;
    return ax > ay ? ax : ay;
}

/* Someone on square x, y of stratum sn: the character (or where its move ends), another body but bd */
static unsigned char __fastcall__ held_sq (unsigned char sn, unsigned x, unsigned y)
{
    struct actor* q = HUB->actor;
    unsigned char k;

    hub_map (0);
    if (sn == HUB->pc_stratum && ((x == HUB->pc_x && y == HUB->pc_y) || (x == HUB->pc_tx && y == HUB->pc_ty))) {
        return 1;
    }
    for (k = 0; k < NACTORS; ++k, ++q) {
        if (k != bk && (q->state & 1) && q->sn == sn && q->x == x && q->y == y) {
            return 1;
        }
    }
    return 0;
}

/* Square x, y of stratum sn one to stand on (made, open floor, steps or a flight's foot) and no one's */
static unsigned char __fastcall__ free_sq (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char* p = at (sn, x, y);
    unsigned char t;

    if (!p) {
        hub_map (0);
        return 0;
    }
    t = TILE (*p);
    return (t == T_FLOOR || t == T_CORR || t == T_STEPS || t == T_UP || t == T_DOWN) && !held_sq (sn, x, y);
}

unsigned char __fastcall__ s_den_flight (const struct msg* m)
{
    struct stratum* s;
    struct link* lk;
    unsigned long t;
    unsigned char to = I_CLIMB_SN (*m), i, best = 0xFF, up, h, k;
    unsigned u, bu = 0xFFFF, x, y, feet;

    for (i = 0, s = &STRATUM (bd.sn); i < s->nlinks; ++i) {
        lk = &s->link[i];
        k = lk->kind;
        if (lk->to == to && (k <= O_LADDER_DOWN || k == O_RAMP_UP || k == O_RAMP_DOWN) &&
            (u = apart (lk->x - bd.x, lk->y - bd.y)) <= LINK_R && u < bu) {
            bu = u;                             /* (The flight joining its stratum to the one it means, the nearest) */
            best = i;
        }
    }
    if (best == 0xFF || STRATUM (to).n != to || !STRATUM (to).made) {
        if (bd.attitude == AT_ALLIED && free_sq (to, I_CLIMB_X (*m), I_CLIMB_Y (*m))) {
            bd.sn = to;                         /* (None within reach, an ally: put beside the character as it was,
                                                **   a second's (a shaft's climb, a rope's, a fall: not yet any
                                                **   actor's, the climb's rounds by who) */
            bd.x = I_CLIMB_X (*m);
            bd.y = I_CLIMB_Y (*m);
            bd.ready = HUB->now + 10;
        } else {
            bd.ready += 10;                     /* (None within reach: it waits a moment) */
        }
        hub_map (0);
        return 0;
    }
    if (((bd.move & 4) | bd.pos) && den_legs ()) {
        bd.ready += 10;                         /* (Down, or its legs won't carry it: as its step's) */
        return 1;
    }
    lk = &s->link[best];
    k = lk->kind;
    up = k == O_STAIRS_UP || k == O_LADDER_UP || k == O_RAMP_UP;
    feet = abs (s->datum - STRATUM (to).datum);
    x = lk->x;
    y = lk->y;
    if (!free_sq (to, x, y)) {                  /* (Someone at its other end: out beside it) */
        for (h = 0; h < 8 && !free_sq (to, lk->x + head_x[h], lk->y + head_y[h]); ++h) {
        }
        if (h == 8) {
            hub_map (0);
            bd.ready += 10;                     /* (No room there: it waits a moment) */
            return 1;
        }
        x = lk->x + head_x[h];
        y = lk->y + head_y[h];
    }
    t = (unsigned long) bu * bd.step + FLIGHT_T (k, bd.step, feet, up);   /* (Its walk to its foot, and the flight: the
                                                **   character's own, at its step) */
    hub_map (0);
    if (bd.ready < HUB->now) {
        bd.ready = HUB->now;
    }
    bd.ready += t;
    bd.sn = to;
    bd.x = x;
    bd.y = y;
    act_noise (bk + 1, AC_FLIGHT, x, y);        /* (Come out there: told as a step is, seen or heard) */
    hub_map (0);
    return 1;
}

