/*
** wpool.c - (dq0w) lamp oil poured out on the floor (the user's H15, 2026-11-21): it lies there a while as a pool,
** slippery underfoot and burning if a flame reaches it, every actor's by the same code (DECISIONS.md's one path).  A
** pool is made as oil's poured away or emptied out (wpour2.c's, every actor's); each actor that steps into it is asked
** to keep its feet (the referee's P_STUMBLE, act_test's, every actor's: fallen, prone, told as any fall is, the limbs'
** news); a lit thing lying in it, or a naked flame carried by one fallen in it, sets it alight; burning, it burns the
** character in it (a burn, HC_FIRE, the firedamp's harm), then it's gone.  Swapped part 99's (its room; its stub, pool): its
** state the world bank's (wrec's pools, copied in for a call), but the count, the main RAM's.
*/

#pragma local-strings (on)

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "wbody.h"
#include "acts.h"

#define NPOOLS          3
#define POOL_T          60                      /* (Minutes: an hour on the floor, then soaked away and dried) */
#define BURN_T          2                       /* (Minutes: two burning, then burnt away) */
#define SLIP_STEPS      (-4)                    /* (The stumble roll's steps: as a thing in the way in the dark) */

struct pool {
    unsigned char   sn;                         /* Its stratum (0xFF none) */
    unsigned        x, y;                       /* Its square */
    unsigned        till;                       /* Till then, the clock's minutes: dried, or burnt away */
    unsigned char   burns;                      /* Alight */
};
typedef char pool_fits[sizeof (struct pool) * NPOOLS + 2 == sizeof WREC->pools ? 1 : -1];   /* (The world bank's room,
                                                **   and the outfits' mark after them) */
extern unsigned char npools;                    /* (dq0w.c's: how many there are, 0 none, the callers' cheap test) */

void __fastcall__ air_harm (unsigned char a, unsigned char b, unsigned char c);    /* (Part 74's: a harm, the UI told) */

#pragma code-name ("SW99CODE")
#pragma rodata-name ("SW99RODATA")
#pragma bss-name ("SW99RODATA")
#pragma static-locals (on)

static struct pool pools[NPOOLS];               /* (The world bank's, wrec's pools, copied in for a call, and back) */
static struct pool* p;

/* The pool on stratum sn's square x, y, still there (those gone made free first): p, or 0 */
static struct pool* __fastcall__ pool_at (unsigned char sn, unsigned x, unsigned y)
{
    static struct pool* q;
    static unsigned char k;

    hub_map (0);
    for (q = 0, npools &= PC_WOKE, k = 0, p = pools; k < NPOOLS; ++k, ++p) {
        if (p->sn != 0xFF && p->till <= (unsigned) (HUB->now / 600)) {
            p->sn = 0xFF;                       /* (Dried, or burnt away) */
        }
        if (p->sn == 0xFF) {
            continue;
        }
        ++npools;
        if (p->sn == sn && p->x == x && p->y == y) {
            q = p;
        }
    }
    return p = q;
}

/* Lamp oil poured out on stratum sn's square x, y: a pool there (one there already lies the longer; none free, the
** oldest's room) */
static void __fastcall__ pool_add (unsigned char sn, unsigned x, unsigned y)
{
    static struct pool* q;
    static unsigned char k;

    if (!pool_at (sn, x, y)) {
        for (q = p = pools, k = 0; k < NPOOLS; ++k, ++p) {
            if (p->sn == 0xFF) {
                q = p;
                break;
            }
            if (p->till < q->till) {
                q = p;
            }
        }
        p = q;
        p->sn = sn;
        p->x = x;
        p->y = y;
        p->burns = 0;
        ++npools;
    }
    if (!p->burns) {
        p->till = (unsigned) (HUB->now / 600) + POOL_T;
    }
}

/* Pool p set alight: it burns a while, then it's gone; told the character if it's near enough to see it (its stratum,
** within its eyes' reach) */
static void lit (void)
{
    hub_map (0);
    p->burns = 1;
    p->till = (unsigned) (HUB->now / 600) + BURN_T;
    if (p->sn == HUB->pc_stratum && p->x - HUB->pc_x + 8 <= 16 && p->y - HUB->pc_y + 8 <= 16) {
        limb_news (WB_BLAZE, 0);
    }
}

/* Whether a naked flame (a lit thing that isn't hung in a lantern's glass: a torch, a candle) lies on pool p's square, or
** (who: 0 the character, else body bd's actor, its entry plus 1) is carried by who fallen in it */
static unsigned char __fastcall__ flame (unsigned char who)
{
    static unsigned i, j;
    static unsigned char k;
    static struct item* q;

    for (i = 0; (i = items (i, IN_SQUARE | IN_ACTOR | IN_DEN | IN_ITEM | IN_FLAGS, I_LIT)) != 0; ) {
        k = ip->kind;
        hub_map (KIND_BANK);
        if (CATALOGUE[k].flags & K_HANG) {
            continue;                           /* (A lantern's flame behind its glass) */
        }
        for (j = i; (q = item_at (j))->where == IW_ITEM; j = q->x) {
        }
        if (who == 0xFF ? q->where == IW_SQUARE && q->sn == p->sn && q->x == p->x && q->y == p->y : !who ?
            q->where == IW_ACTOR && !q->x : q->where == IW_DEN && q->owner == (unsigned char) bd.did && q->hide ==
            bd.did >> 8) {
            return 1;
        }
    }
    return 0;
}

/* An actor stepped onto its square (who: 0 the character, on its square; else body bd's, its entry plus 1): a pool there
** alight burns the character; else it's slippery, its feet kept or not (the referee's stumble roll, every actor's): fallen,
** prone, told as every fall is, a naked flame it carries setting it alight.  1 if it fell */
static unsigned char __fastcall__ pool_step (unsigned char who)
{
    static unsigned char sn;
    static unsigned x, y;

    hub_map (0);
    if (who) {
        sn = bd.sn;
        x = bd.x;
        y = bd.y;
    } else {
        sn = HUB->pc_stratum;
        x = HUB->pc_x;
        y = HUB->pc_y;
    }
    if (!pool_at (sn, x, y)) {
        return 0;
    }
    if (p->burns) {
        if (!who) {
            air_harm (HC_FIRE, 0, 2);           /* (A burn, 2d6: the burning oil about its feet) */
        }
        return 0;
    }
    if (G_OK (act_test (who, P_STUMBLE, SLIP_STEPS))) {
        return 0;
    }
    hub_map (0);
    if (who) {
        bd.pos = POS_PRONE;
    } else {
        HUB->pc_pos = POS_PRONE;
    }
    limb_news (WB_SLIP, who ? who | 0x80 : 0);  /* (Slipped in the oil and fallen: seen, or heard near, as a fall is) */
    pool_at (sn, x, y);
    if (flame (who)) {
        lit ();
    }
    return 1;
}

/* The pools' minute (the lights' minute, burn_job's): those gone made free; each set alight by a naked flame lying in
** it; the character in one alight burnt */
static void pool_time (void)
{
    static unsigned char k;
    static struct pool* q;

    pool_at (0xFF, 0, 0);
    for (k = 0, q = pools; k < NPOOLS; ++k, ++q) {
        if (q->sn == 0xFF) {
            continue;
        }
        p = q;
        if (!p->burns && flame (0xFF)) {
            lit ();
        }
        hub_map (0);
        if (p->burns && p->sn == HUB->pc_stratum && p->x == HUB->pc_x && p->y == HUB->pc_y) {
            air_harm (HC_FIRE, 0, 2);
        }
    }
    hub_map (0);
}

/* The flasks of oil an outfit made (outmake's: the character's kit, a denizen's things), their numbers past from: a pint
** of lamp oil put in each, as in a vial (the user's H16, 2026-11-21: the flask a vessel, its oil a liquid in it) */
static void __fastcall__ oil_new (unsigned from)
{
    static unsigned i, j;
    static struct item* q;

    for (i = 0; (i = items (i, IN_ACTOR | IN_DEN | IN_ITEM | IN_SQUARE, 0)) != 0; ) {
        if (ip->kind != KI_OIL || ip->id <= from || items (0, IN_X | IN_ITEM, i)) {
            continue;
        }
        for (j = 1; j < ilim && item_at (j)->kind; ++j) {
        }
        if (j >= ilim) {
            break;
        }
        if (j >= ihigh) {
            ihigh = j + 1;
        }
        q = item_at (j);
        memset (q, 0, sizeof *q);
        q->kind = KI_LAMP_OIL;
        q->where = IW_ITEM;
        q->x = i;
        q->qty = 1;
        q->fuel = OIL_PINT;
        q->flags = item_at (i)->flags & I_KNOWN;
    }
}

/* Part 99's way in for them (wtime.c's time_kept, its stub, its codes past any minute: no stub of their own, the main
** RAM's room): POOL_OP, the character stepped onto its square, POOL_OP + 1 body bd (pool_step; the character fallen, prone); POOL_ADD + 0 or
** 1, oil poured out where the character or body bd stands; POOL_TIME, the pools' minute; POOL_MARK, the things'
** numbers so far marked (the world bank's, after the pools), an outfit to be made; POOL_FILL, the flasks of oil made since
** it filled (oil_new).  The pools the world bank's, copied in and back */
void __fastcall__ pool_op (unsigned w)
{
    static unsigned m;

    hub_map (WORLD_BANK);
    if (w >= POOL_MARK) {
        if (w == POOL_MARK) {
            m = item_at (0)->x;
            hub_map (WORLD_BANK);
            memcpy (WREC->pools + sizeof pools, &m, 2);
        } else {
            memcpy (&m, WREC->pools + sizeof pools, 2);
            oil_new (m);
        }
        hub_map (0);
        return;
    }
    memcpy (pools, WREC->pools, sizeof pools);
    if (w == POOL_TIME) {
        pool_time ();
    } else if (w >= POOL_ADD) {
        hub_map (0);
        if (w > POOL_ADD) {
            pool_add (bd.sn, bd.x, bd.y);
        } else {
            pool_add (HUB->pc_stratum, HUB->pc_x, HUB->pc_y);
        }
    } else {
        pool_step (w == POOL_OP ? 0 : bk + 1); /* (POOL_OP + 1: body bd's, its actor) */
    }
    hub_map (WORLD_BANK);
    memcpy (WREC->pools, pools, sizeof pools);
    hub_map (0);
}
