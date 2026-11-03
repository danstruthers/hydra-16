/*
** wwater.c - (dq0w, swapped part 74: ../dq0/swap.inc) the water (slice 4.9: DUNGEON.md's water, HOUSE.md's water,
** hunger and thirst): how deep the water is where the character stands (the chunk's, the maker's: world.h's wet bits
** and its rooms' levels), for its wading and swimming (dq0w.c's part 73, s_wade); a waterskin filled where there's water
** (the water it stands in, or a well within reach: the things' IO_FILL, part 27's fill first asking here).  Eating and
** drinking are the UI's (the things' IO_USE: a day's ration, a pint, by their keys) and the referee's (hunger and
** thirst, rswant.h's).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "world.h"
#include "things.h"

unsigned __fastcall__ at_hand (const char* name);   /* (Part 4's stub: to hand, carried or within reach) */
void __fastcall__ put (const char* s);          /* (tlist.s's: the hub's text, the words put after what's there) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;

#pragma code-name ("SW74CODE")
#pragma rodata-name ("SW74RODATA")
#pragma local-strings (on)

#pragma code-name (push, "SW75CODE")           /* (Part 75's: the water's depth, wading's) */
#pragma rodata-name (push, "SW75RODATA")
static const unsigned char bit[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

/* The water at the character's square (the chunk's: world.h's): its depth in feet (0 none), | WET_RUNS running water;
** well, 1 if there's a well beside it (else none) */
unsigned char __fastcall__ s_water_at (unsigned char well)
{
    unsigned char r, sn;
    unsigned x, y, i = 0;
    signed char l, e;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    while (well && (i = items (i, IN_SQUARE, 0)) != 0) {
        if (ip->kind == KI_WELL && ip->sn == sn && ip->x - x + 1 <= 2 && ip->y - y + 1 <= 2) {
            return 1;                           /* (A well beside it: to drink from, fill from) */
        }
    }
    if (!at (sn, x, y) || !(C->wet[cy_][cx_ >> 3] & bit[cx_ & 7])) {
        return 0;
    }
    e = C->elev[cy_][cx_];
    r = room_of (sn, x, y);
    l = C->wlev[r = r == 0xFF ? NROOMS : r];
    if (C->wruns & bit[r]) {
        return (unsigned char) l | WET_RUNS;
    }
    return l > e ? l - e : 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A waterskin filled (the things' IO_FILL, name: "" the first carried): in the water, or by a well within reach; its
** words in the UI bank's things_text, its time in the request's x (a minute); IR_NONE not a waterskin (a light's) */
static unsigned char __fastcall__ wsaid (const char* s, unsigned t)
{
    put_bank = UI_BANK;
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    put (s);
    hub_map (0);
    HUB->ireq.x = t;
    HUB->ireq.qty = 0;
    return t ? IR_OK : IR_NO;
}

static unsigned char fill_skin (void)
{
    char name[16];                              /* (On the C stack: the main RAM's room) */
    struct item* p;
    unsigned sk, i;
    unsigned char cap;

    hub_map (0);
    memcpy (name, HUB->ireq.name, sizeof name);
    if (!(sk = at_hand (*name ? name : "waterskin")) || item_at (sk)->kind != KI_WATERSKIN) {
        return IR_NONE;
    }
    hub_map (KIND_BANK);
    cap = CATALOGUE[KI_WATERSKIN].cap_bulk;
    if (items (0, IN_ITEM | IN_X, sk) && ip->kind == KI_WATER && ip->qty >= cap) {
        return wsaid ("Your waterskin is full already.", 0);    /* (Why it seemed to start empty: the user's,
                                                **   2026-11-03) */
    }
    hub_map (0);
    if (!HUB->pc_wet && !water_at (1)) {
        return wsaid ("There's no water here.", 0);
    }
    if (items (0, IN_ITEM | IN_X, sk) && ip->kind == KI_WATER) {
        p = ip;
    } else {
        for (i = 1; i < ilim && item_at (i)->kind; ++i) {     /* (Its water drunk: a record for it again) */
        }
        if (i >= ilim) {
            return wsaid ("There's no water here.", 0);
        }
        if (i >= ihigh) {
            ihigh = i + 1;
        }
        p = item_at (i);
        memset (p, 0, sizeof *p);
        p->kind = KI_WATER;
        p->where = IW_ITEM;
        p->x = sk;
    }
    p->qty = cap;
    return wsaid ("You fill your waterskin.", 600);
}

/* The part's way in (its stub, water): k W_FILL, a waterskin filled */
unsigned __fastcall__ s_water (unsigned char k)
{
    return k == W_FILL ? fill_skin () : IR_NONE;
}
