/*
** wrope.c - (dq0w) rope and grapples (the user's A16, 2026-11-03: "Queue rope rules"; HOUSE.md's rope and grapples, for
** review; the book's alone the climb's help, Skills 8's good climbing gear): a rope tied off and let down a drop.  A rope
** in place is two records of the item store, filed with their chunks, kept as anything lying there is: the rope itself
** (KI_TIED, "tied rope": fixed where it stands, so nobody takes it up) on the top's square; a KI_HUNG, "hanging rope", at
** the foot, so it's seen and used from there; on a shaft open to the sky (no top on any stratum) the rope itself a KI_HUNG
** at the foot.  Each one's fuel RK_* | the way to its other end's square times 16 | RK_SLIP | the feet of its hold above the
** foot times 256, its owner the feet of its end above the foot, its hide the kind it's tied to.  A drop found, the rope
** put in place, a grapple thrown up: wrope3.c, part 95's.  Untied, and looked at: wrope2.c, part 98's.  The climb itself:
** dq0w.c's part 97.
**
** All of it swapped part 96's (../dq0/swap.inc: dq0w.cfg's SW96), through things.c's s_hide: a request of the things,
** IO_HIDE's shape, IO_ROPE in its to (the UI's words.c: its steps, its timed work, its reply as a thing hidden's are).
*/

#pragma local-strings (on)
#pragma code-name ("SW96CODE")
#pragma rodata-name ("SW96RODATA")

#include "wrope.h"

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
unsigned __fastcall__ at_hand (const char* name);   /* (Part 4's) */
unsigned __fastcall__ carried_named (const char* name);
void __fastcall__ make_own (unsigned i);        /* (Part 13's) */

#define ROPE_HEAVY      120                     /* (Quarter pounds: 30 lb, a thing too heavy to come away with a climber) */
#define ROPE_RUN        5                       /* (Feet of it gone from what it's tied to, beside, to the edge) */
#define TIE_T           300                     /* (Tenths: tied and let down) */
#define DRIVE_T         600                     /* (A spike driven first) */
#define R_A             (*(unsigned*) (iq.into + 0))    /* (The thing it's tied to, the spike, the hammer: the other's name's */
#define R_SPIKE         (*(unsigned*) (iq.into + 2))    /*   room, past its use) */
#define R_HAMMER        (*(unsigned*) (iq.into + 4))

/* Thing i: does it hold a rope?  Lying there, fixed where it stands (a ring, a post, a spike driven, an altar, a well) or too
** heavy to come away */
static unsigned char __fastcall__ rp_holds (unsigned i)
{
    unsigned char k = item_at (i)->kind;

    if (item_at (i)->where != IW_SQUARE || k == KI_TIED || k == KI_HUNG || k == KI_SCONCE) {
        return 0;
    }
    hub_map (KIND_BANK);
    return (CATALOGUE[k].keeps & KF_FIXED) || weight_of (i) >= ROPE_HEAVY;
}

/* The nearest thing that holds one, on the character's square or beside it: its number, or 0 */
static unsigned rp_anchor (void)
{
    unsigned i = 0;

    while ((i = items (i, IN_SQUARE, 0)) != 0) {
        if (ip->sn == R_TSN && (unsigned) (ip->x - R_FX + 1) <= 2 && (unsigned) (ip->y - R_FY + 1) <= 2 && rp_holds (i)) {
            return i;
        }
    }
    return 0;
}

/* Tied to the thing named, or what's nearest that holds, and let down: "tie the rope", "tie the rope to the ring", "let down
** the rope"; its work a timed one (the steps of the reach first: a tool, the hands routine's).  "Tie the grapple to the
** rope": one thing, a round's, with no drop asked */
static unsigned char __fastcall__ rp_tie (unsigned char done)
{
    unsigned char k;
    char b[14];

    R_N = *iq.name ? carried_named (iq.name) : rp_carried (KI_ROPE);
    handled = R_N;                              /* (Never put away to free a hand for its hammer: the hands' rule's, the
                                                **   home first, 2026-11-17) */
    k = R_N ? item_at (R_N)->kind : 0;
    if (k == KI_GRAPPLE) {                      /* (The other way about: the grapple to the rope) */
        if (!(R_N = rp_carried (KI_ROPE))) {
            return rp_no (SA_NO_ROPE_TIE);
        }
        strcpy (iq.into, "grapple");
    } else if (k != KI_ROPE) {
        return rp_no (R_N ? SA_TIED_OFF : SA_CARRY_NO_ROPE);
    }
    if (*iq.into) {
        R_A = at_hand (iq.into);
        R_SPIKE = R_HAMMER = 0;
        k = R_A ? item_at (R_A)->kind : 0;
        if (k == KI_GRAPPLE && on_pc (R_A)) {
            handled = R_A;                      /* (The grapple tied on never put away to free a hand for the rope) */
            if (reach_step (R_N, RS_TOOL)) {
                return IR_NO;
            }
            rope3 (RA_JOIN | R_A);
            rp_new ();
            rp_say (SA_TIE_GRAPPLE, "");
            return rp_done (100);
        }
        if (k == KI_SPIKE && on_pc (R_A)) {
            R_SPIKE = R_A;
            R_A = 0;
        } else if (!R_A || !rp_holds (R_A) || !in_reach (R_A)) {
            return rp_no (SA_NO_HOLD_ROPE);
        }
    } else {
        R_SPIKE = R_HAMMER = 0;
        hub_map (0);
        R_TSN = HUB->pc_stratum;
        R_FX = HUB->pc_x;
        R_FY = HUB->pc_y;
        if (!(R_A = rp_anchor ())) {            /* (Else its grapple, set over the edge; else a spike driven) */
            if (!rp_in (R_N, KI_GRAPPLE) && !rp_carried (KI_GRAPPLE) && !(R_SPIKE = rp_carried (KI_SPIKE))) {
                return rp_no (SA_NOTHING_HOLDS);
            }
        }
    }
    if (!rope3 (RA_SCAN)) {
        return rp_no (SA_NO_DROP);
    }
    if (rp_on (R_TSN, R_TX, R_TY)) {
        return rp_no (SA_TIED_HERE);
    }
    if (R_SPIKE && !(R_HAMMER = rp_carried (KI_HAMMER))) {
        return rp_no (SA_NO_HAMMER);
    }
    if (!done) {
        if (reach_step (R_N, RS_TOOL) || (R_HAMMER && reach_step (R_HAMMER, RS_TOOL))) {
            return IR_NO;
        }
        if (R_HAMMER) {
            rs_using = 1;                       /* (The hammer to put away after it, the rope in hand before it or not) */
            rs_tool = R_HAMMER;
        }
        hub_map (0);
        HUB->ireq.item = KI_ROPE;
        HUB->ireq.qty = LT_TIE;
        HUB->ireq.x = R_SPIKE ? DRIVE_T : TIE_T;
        return IR_OK;
    }
    R_RUN = 0;
    R_AK = KI_GRAPPLE;
    R_SLIP = 0;
    if (R_SPIKE) {                              /* (Driven where it stands, for good: a loud minute's work) */
        make_own (R_SPIKE);
        rope3 (RA_GONE | R_SPIKE);
        ip = item_at (R_SPIKE);
        ip->flags &= ~(I_HELD | I_WORN);
        ip->where = IW_SQUARE;
        ip->kind = R_AK = KI_DRIVEN;
        ip->sn = R_TSN;
        ip->x = R_TX;
        ip->y = R_TY;
        rp_noise (7);
    } else if (R_A) {
        ip = item_at (R_A);
        R_AK = ip->kind;
        R_RUN = ip->x != R_TX || ip->y != R_TY ? ROPE_RUN : 0;
    } else if (!rp_in (R_N, KI_GRAPPLE) && (R_A = rp_carried (KI_GRAPPLE)) != 0) {
        rope3 (RA_JOIN | R_A);
    }
    rope3 (RA_HANG);
    rp_new ();
    rp_name (R_AK, b);                          /* (Its words a row, the kind tied to its name: SA.tie_edge ...) */
    rp_say ((R_HOW == RK_SHAFT ? SA_TIE_SHAFT : SA_TIE_EDGE) + (R_END != 0), b);
    return rp_done (0);
}

/* (Its stub's: a request of the things') */
unsigned char s_rope (void)
{
    switch (iq.to & RP_OP) {
    case RP_UNTIE:
        return rope2 (0);
    case RP_THROW:
        return rope3 (0);
    }
    return rp_tie (iq.to & IO_DONE);
}
