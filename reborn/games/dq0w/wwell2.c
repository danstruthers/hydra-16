/*
** wwell2.c - (dq0w) a thing let fall into a well, or down any shaft or hole within reach (the user's, 2026-11-06: "A real
** shaft"; HOUSE.md's a well's shaft): put on the head's square, then down to its foot, its landing heard, a splash in its
** water.  Swapped part well_drop's (dq0w.rop's overlay 32, as part well is: the things' serve reads it in first), through its
** stub, well_put; the well's own work part well's, through its stub (well: WL_NEAR, WL_SHAFT, WL_FOOT), what it keeps in
** the things' request's copy (iq), wwell.c's W_*.
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "work.h"
#include "things.h"

#pragma local-strings (on)
#pragma code-name ("SW_well_drop_CODE")
#pragma rodata-name ("SW_well_drop_RODATA")
#pragma bss-name ("SW_well_drop_RODATA")

unsigned __fastcall__ at_hand (const char* name);
unsigned __fastcall__ seen_named (const char* name);
extern struct ireq iq;

#define W_X             iq.x                    /* (wwell.c's) */
#define W_Y             iq.y
#define W_SN            iq.craft
#define W_B             iq.quality

/* A shaft going down within reach of the character (on its square or beside it: a shaft open to the sky going on down,
** a dug one, a well's): its link, its head W_X, W_Y, its strata W_SN, W_B; or 0xFF */
static unsigned char shaft_near (void)
{
    static const struct stratum* s;
    static const struct link* l;
    static unsigned px, py;
    static unsigned char k;

    hub_map (0);
    s = &STRATUM (W_SN = HUB->pc_stratum);
    px = HUB->pc_x;
    py = HUB->pc_y;
    for (k = 0, l = s->link; k < s->nlinks; ++k, ++l) {
        if (l->kind == O_SHAFT_DOWN && l->x - px + 1 <= 2 && l->y - py + 1 <= 2) {
            W_X = l->x;
            W_Y = l->y;
            W_B = l->to;
            return k;
        }
    }
    return 0xFF;
}

/* An open pit within reach of the character (its own square, or one of the eight round it: T_PIT, given way, in plain
** sight; "drop the candle stubs in the pit", a hole's kind said, the UI's "shaft"): its square W_X, W_Y on W_SN, 1; or 0 */
static unsigned char pit_near (void)
{
    static unsigned px, py;
    static unsigned char k, r;
    static const unsigned char* p;

    hub_map (0);
    W_SN = HUB->pc_stratum;
    px = HUB->pc_x;
    py = HUB->pc_y;
    for (k = r = 0; k < 9 && !r; ++k) {
        W_X = k ? px + head_x[k - 1] : px;
        W_Y = k ? py + head_y[k - 1] : py;
        r = (p = at (W_SN, W_X, W_Y)) != 0 && TILE (*p) == T_PIT;
    }
    hub_map (0);
    return r;
}

static char b[120];

/* None to let thing n fall down (0: none of the name to hand), its words in the things' text (said.txt's SA rows: "There's
** no well or shaft here for the candle stubs to fall down."; "You have no such thing."): IR_NONE.  (Its words none
** before 2026-11-17: the UI said the things' text as it was, the play test's "drop the candle stubs in the pit", its
** inventory before it, cut) */
static unsigned char __fastcall__ none_for (unsigned n)
{
    unsigned char k = n ? item_at (n)->kind : 0;

    b[0] = n ? SAY_AT2 : SAY_AT;
    b[1] = n ? SA_NO_FALL : SA_NO_THING | 0x80;
    b[2] = 0;
    if (n) {
        hub_map (KIND_BANK);
        strcpy (b + 2, CATALOGUE[k].name);
    }
    strcat (b, "\2\2");
    hub_map (UI_BANK);
    strcpy (UIREC->things_text, b);
    hub_map (0);
    return IR_NONE;
}

/* The thing named (the things' request's) into the well within reach, or down a shaft (the request's into: "well", or
** "shaft", a shaft's or a hole's kind said, the UI's): on the head's square, then down to its foot (a well choked, it lodges
** where it is), told in the things' words, the sound of its landing by the foot's water: IR_OK; its reach's step first,
** IR_NO; a well seen but out of reach, IR_NOT_HERE (where it is: the UI walks over to it, and asks again); none, IR_NONE,
** why in the things' words.  A hole's with no shaft within reach, an open pit's within reach: it falls into the pit, on
** its floor (the user's, 2026-11-17: "drop the candle stubs in the pit", verbs do real actions) */
unsigned char s_well_put (void)
{
    struct item* q;
    const struct stratum* s;
    unsigned n;
    unsigned char k, w, wet = 0, pit = 0;
    static struct msg m;

    if ((w = well (WL_NEAR)) == 0 && (*iq.into != 's' || shaft_near () == 0xFF) && (*iq.into != 's' || !(pit = pit_near ()))) {
        if (*iq.into == 's' || (n = seen_named ("well")) == 0) {
            return none_for (at_hand (iq.name));
        }
        q = item_at (n);
        W_X = q->x;
        W_Y = q->y;
        hub_map (0);
        HUB->ireq.x = W_X;
        HUB->ireq.y = W_Y;
        HUB->ireq.item = n;
        return IR_NOT_HERE;
    }
    if ((n = at_hand (iq.name)) == 0) {
        return none_for (0);
    }
    if (reach_step (n, 0)) {
        return IR_NO;                           /* (A step of the reach first: got out of the pack ...) */
    }
    if (!pit && (w ? well (WL_SHAFT) != 0xFF : well (WL_FOOT))) {
        wet = WET_BIT (C, cx_, cy_) != 0;       /* (Its foot, in the window: part well's footed) */
        s = &STRATUM (W_B);
        k = it_need (W_B, (W_X - s->x0) >> CW_BITS, (W_Y - s->y0) >> CW_BITS);
    } else {
        k = 0;                                  /* (A pit's: its floor's square, this stratum's) */
    }
    q = item_at (n);
    q->where = IW_SQUARE;
    q->sn = k ? W_B : W_SN;
    q->x = W_X;
    q->y = W_Y;
    q->flags &= ~(I_HELD | I_WORN);
    k = q->kind;
    b[0] = SAY_AT2;                             /* (Its sentence coded, said.txt's SA rows: the well's, the shaft's; it
                                                **   lodges, a splash, it lands; the user's, 2026-11-16) */
    b[1] = (pit ? SA_PIT_FALL : SA_WELL_LODGES + (w ? 0 : 3) + (item_at (n)->sn == W_SN ? 0 : wet ? 1 : 2)) | 0x80;
    hub_map (KIND_BANK);
    strcpy (b + 2, CATALOGUE[k].name);
    strcat (b, "\2\2");
    hub_map (UI_BANK);                          /* (Told as the traces' words are, E_VIEW's V_TRACED: after a reach's step too) */
    strcpy (UIREC->trtext, b);
    hub_map (0);
    m.kind = E_VIEW;
    E_VIEW_WHY (m) = V_TRACED;
    m.t = HUB->now;
    hub_event (&m);
    HUB->ireq.x = 0;                            /* (No time: let fall, as a thing put down) */
    return IR_OK;
}

