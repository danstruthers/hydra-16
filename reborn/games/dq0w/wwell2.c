/*
** wwell2.c - (dq0w) a thing let fall into a well, or down any shaft or hole within reach (the user's, 2026-11-06: "A real
** shaft"; HOUSE.md's a well's shaft): put on the head's square, then down to its foot, its landing heard, a splash in its
** water.  Swapped part 104's (dq0w.rop's overlay 31, as part 103 is: the things' serve reads it in first), through its
** stub, well_put; the well's own work part 103's, through its stub (well: WL_NEAR, WL_SHAFT, WL_FOOT), what it keeps in
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
#pragma code-name ("SW104CODE")
#pragma rodata-name ("SW104RODATA")
#pragma bss-name ("SW104RODATA")

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

/* The thing named (the things' request's) into the well within reach, or down a shaft (the request's into: "well", or
** "shaft", a shaft's or a hole's kind said, the UI's): on the head's square, then down to its foot (a well choked, it lodges
** where it is), told in the things' words, the sound of its landing by the foot's water: IR_OK; its reach's step first,
** IR_NO; a well seen but out of reach, IR_NOT_HERE (where it is: the UI walks over to it, and asks again); none, IR_NONE */
unsigned char s_well_put (void)
{
    struct item* q;
    const struct stratum* s;
    unsigned n;
    unsigned char k, w, wet = 0;
    static char b[120];
    static struct msg m;

    if ((w = well (WL_NEAR)) == 0 && (*iq.into != 's' || shaft_near () == 0xFF)) {
        if (*iq.into == 's' || (n = seen_named ("well")) == 0) {
            return IR_NONE;
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
        return IR_NONE;
    }
    if (reach_step (n, 0)) {
        return IR_NO;                           /* (A step of the reach first: got out of the pack ...) */
    }
    if (w ? well (WL_SHAFT) != 0xFF : well (WL_FOOT)) {
        wet = WET_BIT (C, cx_, cy_) != 0;       /* (Its foot, in the window: part 103's footed) */
        s = &STRATUM (W_B);
        k = it_need (W_B, (W_X - s->x0) >> CW_BITS, (W_Y - s->y0) >> CW_BITS);
    } else {
        k = 0;
    }
    q = item_at (n);
    q->where = IW_SQUARE;
    q->sn = k ? W_B : W_SN;
    q->x = W_X;
    q->y = W_Y;
    q->flags &= ~(I_HELD | I_WORN);
    k = q->kind;
    strcpy (b, "You let the ");
    hub_map (KIND_BANK);
    strcat (b, CATALOGUE[k].name);
    strcat (b, w ? " fall into the well." : " fall down the shaft.");
    strcat (b, item_at (n)->sn == W_SN ? " It lodges a little way down, where the shaft narrows." : wet ?
        " A moment, and you hear a splash far below." : " A moment, and you hear it land far below.");
    hub_map (UI_BANK);                          /* (Told as the traces' words are, E_VIEW's V_TRACED: after a reach's step too) */
    strcpy (UIREC->trtext, b);
    hub_map (0);
    m.kind = E_VIEW;
    m.w1 = V_TRACED;
    m.t = HUB->now;
    hub_event (&m);
    HUB->ireq.x = 0;                            /* (No time: let fall, as a thing put down) */
    return IR_OK;
}

