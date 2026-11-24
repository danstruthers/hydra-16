/*
** wpour2.c - (dq0w) the empty done, and its words, the pour's (P4, the user's, 2026-11-18; H11, 2026-11-19): what
** wpour.c's s_pour found may be, done (every actor's), told the character ("You empty the sack into the iron pot. The water goes in too.", "You pour a pint of the water
** into the vial: it's full, and the rest stays in the waterskin."), from the request's copy, iq, as s_pour left it.
** Swapped part 113's (dq0w.cfg's SW113: part 112's room), through its stub, pour_said
*/

#pragma local-strings (on)

#include <string.h>
#include <stdlib.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

extern struct ireq iq;                          /* (things.c's: the request's copy) */
void __fastcall__ put (const char* s);          /* (tlist.s's: its words after the text) */

#pragma code-name ("SW_pour_words_CODE")
#pragma rodata-name ("SW_pour_words_RODATA")
#pragma bss-name ("SW_pour_words_RODATA")
#pragma static-locals (on)

#define TAKE_T          20                      /* (Tenths: a thing taken out, a take's moment, wtake.c's got) */
#define E_N             iq.item                 /* (wpour.c's: the thing, */
#define E_Y             iq.value                /*   the other, */
#define E_L             iq.y                    /*   its liquid) */

/* A denizen's doing seen (its square in the character's sight: the limbs' news, wtake.c's) */
static void __fastcall__ seen_do (unsigned w)
{
    static unsigned char* p;

    p = at (bd.sn, bd.x, bd.y);
    if (p && (*p & T_VIS)) {
        limb_news (w, bk + 1);
    }
    hub_map (0);
}

/* Thing E_N's solid things (its first compartment's: a chest's inside, not its lid's top) put out (none, iq.size's
** liquid alone): into thing E_Y (its first compartment), or (E_Y 0) down on stratum sn's square x, y.  How many in
** iq.qty, the first one's kind in iq.mat */
static void __fastcall__ out_all (unsigned char sn, unsigned x, unsigned y)
{
    static unsigned i;
    static unsigned char k;
    static struct item* q;

    iq.qty = 0;
    for (i = 0; !iq.size && (i = items (i, IN_X | IN_ITEM, E_N)) != 0; ) {
        if (IC_OF (ip->y)) {
            continue;                           /* (Not its inside: its lid's top, a drawer) */
        }
        k = ip->kind;
        hub_map (KIND_BANK);
        if (CATALOGUE[k].form & KF_LIQUID) {
            continue;                           /* (The liquid's: pour_in's) */
        }
        q = item_at (i);
        if (!iq.qty++) {
            iq.mat = k;
        }
        if (E_Y) {
            q->x = E_Y;
            q->y = 0;
        } else {
            q->where = IW_SQUARE;
            q->sn = sn;
            q->x = x;
            q->y = y;
            q->flags = (q->flags & ~(I_HELD | I_WORN)) | I_KNOWN;
        }
    }
    hub_map (0);
}

/* A free record (1 on), or 0 none */
static unsigned free_rec (void)
{
    static unsigned i;

    for (i = 1; i < ilim && item_at (i)->kind; ++i) {
    }
    hub_map (0);
    return i < ilim ? i : 0;
}

/* Liquid E_L (in thing E_N), iq.osel of it (pints, its count), poured into thing E_Y: joined to the same liquid there,
** else moved, or split off when only some goes (the rest stays where it was: what a hand pouring does, the vessel
** full, nothing spilt unasked; the user's H11, 2026-11-19); or (E_Y 0) poured away, all of it: gone.  Still water's
** foulness goes with it (wwater.c's: mixed with clean, still foul) */
static unsigned __fastcall__ pints_of (unsigned f)
{
    return (f + OIL_PINT - 1) / OIL_PINT;
}

static void pour_in (void)
{
    static struct item c;
    static unsigned i, mf;
    static struct item* q;

    memcpy (&c, item_at (E_L), sizeof c);
    mf = c.kind != KI_LAMP_OIL ? 0 : iq.osel == c.qty ? c.fuel : iq.osel * OIL_PINT;   /* (Lamp oil's measure that goes:
                                                **   whole pints, the last one's part with it all) */
    if (E_Y) {
        for (i = 0; (i = items (i, IN_X | IN_ITEM, E_Y)) != 0 && (IC_OF (ip->y) || ip->kind != c.kind); ) {
        }
        if (i) {
            q = item_at (i);                    /* (Joined to the same liquid there) */
            q->qty += iq.osel;
            q->wear |= c.wear & 1;
            if (mf) {
                q->fuel += mf;
                q->qty = pints_of (q->fuel);
            }
        } else if (iq.osel == c.qty) {
            q = item_at (E_L);                  /* (All of it: moved) */
            q->x = E_Y;
            q->y = 0;
            hub_map (0);
            return;
        } else if ((i = free_rec ()) != 0) {
            if (i >= ihigh) {
                ihigh = i + 1;
            }
            q = item_at (i);                    /* (Some: split off into it, a record of its own) */
            memcpy (q, &c, sizeof c);
            q->x = E_Y;
            q->y = 0;
            q->qty = iq.osel;
            q->fuel = mf;
            q->id = 0;                          /* (Its own number as it's next looked at: make_own's, wfetch.c's) */
        }
    }
    q = item_at (E_L);
    q->fuel -= mf;
    if (!E_Y || !(q->qty -= iq.osel)) {
        q->kind = 0;                            /* (All gone from it) */
    }
    hub_map (0);
}


/* A kind's name (the catalogue's) */
static const char* __fastcall__ name (unsigned char k)
{
    static char b[14];

    hub_map (KIND_BANK);
    return strcpy (b, CATALOGUE[k].name);
}

/* "a pint of the water", "3 pints of the water" put (n pints of liquid l) */
static void __fastcall__ pints (unsigned char n, const char* l)
{
    static char b[4];

    put (n == 1 ? "a pint" : !n ? "some" : utoa (n, b, 10));
    put (n < 2 ? " of the " : " pints of the ");   /* (Some: a flask filled, by its measure) */
    put (l);
}

/* The act (the solid things put out, the liquid poured: as wpour.c's s_pour found it may be), every actor's; the
** character's words (the UI bank's things_text, begun), its time in the reply's x: the thing emptied's kind iq.craft, the
** other's iq.quality (iq.value the other, 0 none; iq.to a surface), the liquid's iq.plural, how much of it iq.count, how
** much went in iq.osel; the solid things put out iq.qty.  IR_OK */
unsigned char s_pour_said (void)
{
    static char ln[14];
    static unsigned char k, ky, lq;

    hub_map (0);
    if (iq.to & TO_DEN) {
        out_all (bd.sn, bd.x, bd.y);
    } else {
        out_all (HUB->pc_stratum, HUB->pc_x, HUB->pc_y);
    }
    if (E_L) {
        pour_in ();
        if (!E_Y && iq.plural == KI_LAMP_OIL) { /* (Lamp oil poured out: a pool where it stands, the user's H15) */
            time_kept (POOL_ADD + ((iq.to & TO_DEN) != 0));
        }
    }
    if (iq.to & TO_DEN) {
        bd.ready += TAKE_T * (iq.qty + (iq.count != 0));
        ++HUB->den_seq;
        if (iq.qty && !E_Y) {
            seen_do (WB_DROP | (unsigned) iq.mat << 8);
        }
        return IR_OK;                           /* (A denizen's: no words) */
    }
    k = iq.craft;
    ky = iq.quality;
    lq = iq.count;
    strcpy (ln, lq ? name (iq.plural) : "");
    if (iq.qty) {
        put ("You empty the ");
        put (name (k));
        if (iq.value) {
            put (iq.to ? " onto the " : " into the ");
            put (name (ky));
            put (".");
            if (lq && iq.osel == lq) {
                put (" The ");
                put (ln);
                put (" goes in too.");
            } else if (lq) {
                put (" The ");
                put (name (ky));
                put (" takes only ");
                pints (iq.osel, ln);
                put (": the rest stays in the ");
                put (name (k));
                put (".");
            }
        } else {
            put (" out at your feet.");
            if (*ln) {
                put (" The ");
                put (ln);
                put (iq.plural == KI_LAMP_OIL ? " pools on the floor." : " runs away.");
            }
        }
    } else if (iq.value && iq.osel < lq) {
        put ("You pour ");
        pints (iq.osel, ln);
        put (" into the ");
        put (name (ky));
        put (": it's full, and the rest stays in the ");
        put (name (k));
        put (".");
    } else {
        put ("You pour the ");
        put (ln);
        put (iq.value ? " into the " : " out of the ");
        put (name (iq.value ? ky : k));
        put (!iq.value && iq.plural == KI_LAMP_OIL ? ": it pools on the floor." : ".");
    }
    hub_map (0);
    HUB->ireq.x = TAKE_T * (iq.qty + (*ln != 0));   /* (A moment a handful, a take's; the pouring one) */
    return IR_OK;
}
