/*
** wempty.c - (dq0w) a thing's things put out, and a thing put down on a square within reach (P4 and P5, the user's,
** 2026-11-18, DECISIONS.md's "The merged branches' questions": "empty X" puts its things down here, or on Y when
** named, a liquid poured away, a take's moment a handful; a thing put on a ledge, in a corner, the centre, a side or a
** passage, walked there and put down on that square), every actor's by the same code (DECISIONS.md's one path): the
** things' IO_EMPTY and IO_PLACE (things.c's s_serve), a denizen's I_ROB FE_EMPTY and FE_PLACE (wbody.c's: no mind posts
** them yet).  All of it swapped part 111's (../dq0/swap.inc: dq0w.cfg's SW111), through its stub, handout.  Its locals
** the part's own (static, in its image): what lasts across a call to another part (pick's, bulk_of's, sky_ask's) is in
** the request's copy, iq, the main RAM's, as the drag's is (wdrag.c)
*/

#pragma local-strings (on)

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

extern struct ireq iq;                          /* (things.c's: the request's copy) */
void __fastcall__ put (const char* s);          /* (tlist.s's: its words after the text) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;
unsigned __fastcall__ pick (const char* name, unsigned char how);   /* (Part 4's) */

#pragma code-name ("SW111CODE")
#pragma rodata-name ("SW111RODATA")
#pragma bss-name ("SW111RODATA")
#pragma static-locals (on)

/* A kind's name (the catalogue's) */
static const char* __fastcall__ name (unsigned char k)
{
    static char b[14];

    hub_map (KIND_BANK);
    return strcpy (b, CATALOGUE[k].name);
}

#define TAKE_T          20                      /* (Tenths: a thing taken out, a take's moment, wtake.c's got) */
#define PLACE_T         10                      /* (A thing set down where it's to lie: a moment) */
#define HW_ANY          3                       /* (things.c's pick: carried, or lying about) */
#define HW_CARRIED      1
#define E_N             iq.item                 /* (The thing, */
#define E_Y             iq.value                /*   the other, */
#define E_I             iq.dur                  /*   a thing in it, */
#define E_B             iq.dur_max              /*   the room they want) */
unsigned char __fastcall__ sky_ask (unsigned w);    /* (Part 89's: a thing let fall on a shaft's head) */
unsigned __fastcall__ bulk_of (unsigned i);     /* (Part 10's) */
unsigned __fastcall__ bulk_in (unsigned x);
unsigned __fastcall__ shut_in (unsigned n);     /* (Part 106's: the closed thing it's in) */
void __fastcall__ reach_from (unsigned n);      /* (Part 56's: its reach owed, got out first) */
extern unsigned rtime;
extern unsigned char rsteps;

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

/* Thing E_N's things (its first compartment's: a chest's inside, not its lid's top) put out: a liquid poured away (gone);
** the rest into thing E_Y (its first compartment), or (E_Y 0) down on stratum sn's square x, y.  How many put out in
** iq.qty, the first one's kind in iq.mat; how many poured in iq.plural, the first one's kind in iq.craft */
static void __fastcall__ out_all (unsigned char sn, unsigned x, unsigned y)
{
    static unsigned i;
    static unsigned char k;
    static struct item* q;

    iq.qty = iq.plural = 0;
    for (i = 0; (i = items (i, IN_X | IN_ITEM, E_N)) != 0; ) {
        if (IC_OF (ip->y)) {
            continue;                           /* (Not its inside: its lid's top, a drawer) */
        }
        k = ip->kind;
        hub_map (KIND_BANK);
        if (CATALOGUE[k].form & KF_LIQUID) {
            item_at (i)->kind = 0;              /* (Poured away: gone) */
            if (!iq.plural++) {
                iq.craft = k;
            }
            continue;
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

/* Thing n to the character's hand: carried, or lying (or in a thing lying) on its square or one beside it: 1; else 0,
** where it lies in the reply (the UI walks there first) */
static unsigned char __fastcall__ in_hand (unsigned n)
{
    static struct item* p;
    static unsigned char w, sn;
    static unsigned x, y;

    while ((p = item_at (n))->where == IW_ITEM) {
        n = p->x;                               /* (Its outermost holder's place) */
    }
    w = p->where;
    sn = p->sn;
    x = p->x;
    y = p->y;
    hub_map (0);
    if ((w == IW_ACTOR && !x) || (w == IW_SQUARE && sn == HUB->pc_stratum && x - HUB->pc_x + 1 <= 2 &&
        y - HUB->pc_y + 1 <= 2)) {
        return 1;
    }
    HUB->ireq.x = x;
    HUB->ireq.y = y;
    HUB->ireq.item = 0;
    return 0;
}

/* The things' words begun (the UI bank's things_text) */
static void text_on (void)
{
    put_bank = UI_BANK;
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
}

/* "The {kind k}{s}" put: IR_NO */
static unsigned char __fastcall__ the (unsigned char k, const char* s)
{
    put ("The ");
    put (name (k));
    put (s);
    return IR_NO;
}

/* A thing's things put out (IO_EMPTY): the character's (iq.name, carried or within reach; iq.into the other, a surface
** to 1, or none: at its feet), or body bd's (iq.to's TO_DEN: its own thing iq.item, at its feet).  Refused: not a thing
** that holds things, closed (sealed, locked), nothing in it; the other not one that holds things, closed, no room in it
** for them.  A flask's oil poured away as a liquid in a thing is */
static unsigned char empty (void)
{
    static unsigned char k, ky, sh, f, oil, any;
    static struct item* q;

    if (iq.to & TO_DEN) {
        E_Y = 0;
        out_all (bd.sn, bd.x, bd.y);
        bd.ready += TAKE_T * (iq.qty + (iq.plural != 0));
        ++HUB->den_seq;
        if (iq.qty) {
            seen_do (WB_DROP | (unsigned) iq.mat << 8);
        }
        return IR_OK;
    }
    E_Y = 0;
    if (!(E_N = pick (iq.name, HW_ANY))) {
        return IR_NONE;
    }
    if (!in_hand (E_N)) {
        return IR_NOT_HERE;                     /* (Walked over to it first: the UI's) */
    }
    if (*iq.into && (!(E_Y = pick (iq.into, HW_ANY)) || E_Y == E_N)) {
        return IR_NONE;
    }
    if (E_Y) {
        if (!in_hand (E_Y)) {
            return IR_NOT_HERE;
        }
        E_B = bulk_in (E_Y);                    /* (The room they want, the solid ones, and what's in it) */
        for (E_I = 0; (E_I = items (E_I, IN_X | IN_ITEM, E_N)) != 0; ) {
            if (!IC_OF (ip->y)) {
                k = ip->kind;
                hub_map (KIND_BANK);
                if (!(CATALOGUE[k].form & KF_LIQUID)) {
                    E_B += bulk_of (E_I);
                }
            }
        }
    }
    text_on ();
    q = item_at (E_N);
    k = q->kind;
    sh = q->shut;
    oil = k == KI_OIL && q->fuel;
    any = items (0, IN_X | IN_ITEM, E_N) != 0;
    if (E_Y) {
        q = item_at (E_Y);
        ky = q->kind;
        f = q->shut;
    }
    hub_map (KIND_BANK);
    if (!(CATALOGUE[k].flags & K_CONTAINER) && !oil) {
        return the (k, " isn't a thing to empty.");
    }
    if (sh & IS_CLOSED) {
        return the (k, sh & IS_LOCKED ? " is locked." : " is closed.");
    }
    if (!any && !oil) {
        return the (k, " is empty.");
    }
    if (E_Y) {
        hub_map (KIND_BANK);
        if (!(CATALOGUE[ky].flags & K_CONTAINER) && !(CATALOGUE[ky].keeps & KF_ON)) {
            return the (ky, " won't hold them.");
        }
        if (f & IS_CLOSED) {
            return the (ky, " is closed.");
        }
        if (E_B > CATALOGUE[ky].cap_bulk) {
            put (iq.to ? "There's no room for them on the " : "There's no room for them in the ");
            put (name (ky));
            put (".");
            return IR_NO;
        }
    }
    hub_map (0);
    out_all (HUB->pc_stratum, HUB->pc_x, HUB->pc_y);
    if (oil) {
        item_at (E_N)->fuel = 0;                /* (A flask's oil poured away) */
        ++iq.plural;
    }
    if (iq.qty) {
        put ("You empty the ");
        put (name (k));
        if (E_Y) {
            put (iq.to ? " onto the " : " into the ");
            put (name (ky));
            put (".");
        } else {
            put (" out at your feet.");
        }
        if (iq.plural) {
            put (" The ");
            put (oil ? "oil" : name (iq.craft));
            put (" runs away.");
        }
    } else {
        put ("You pour the ");
        put (oil ? "oil" : name (iq.craft));
        put (" out of the ");
        put (name (k));
        put (".");
    }
    hub_map (0);
    HUB->ireq.x = TAKE_T * (iq.qty + (iq.plural != 0));    /* (A moment a handful, a take's) */
    return IR_OK;
}

/* A thing put down on a square within reach (IO_PLACE): the character's (iq.name, carried; iq.x, iq.y the square, the
** UI's; iq.qty the place's words, PW_*: "in the corner"), or body bd's (iq.to's TO_DEN: its own thing iq.item).  Out of
** reach: walked there first (the UI's; a denizen's mind's); a square nothing rests on (a wall, rock): refused */
static unsigned char place (void)
{
    static const char* const pw[] = { " down.", " in the corner.", " in the middle.", " on the far side.",
        " on the near side.", " on the left side.", " on the right side.", " on the ledge.", " in the passage." };   /* (PW_*) */
    static unsigned char sn, k;
    static unsigned char* t;
    static struct item* q;

    hub_map (0);
    if (iq.to & TO_DEN) {
        sn = bd.sn;
        if (iq.x - bd.x + 1 > 2 || iq.y - bd.y + 1 > 2) {
            return IR_NOT_HERE;
        }
    } else {
        if (!(E_N = pick (iq.name, HW_CARRIED))) {
            return IR_NONE;
        }
        hub_map (0);
        if (iq.x - HUB->pc_x + 1 > 2 || iq.y - HUB->pc_y + 1 > 2) {
            HUB->ireq.x = iq.x;                 /* (Walked there first: the UI's) */
            HUB->ireq.y = iq.y;
            HUB->ireq.item = 0;
            return IR_NOT_HERE;
        }
        if ((E_Y = shut_in (E_N)) != 0) {
            text_on ();
            return the (item_at (E_Y)->kind, " is closed.");
        }
        hub_map (0);
        sn = HUB->pc_stratum;
    }
    t = at (sn, iq.x, iq.y);
    k = t ? TILE (*t) : T_ROCK;
    if (k != T_FLOOR && k != T_CORR && k != T_STEPS && k != T_UP && k != T_DOWN && k != T_PIT) {
        if (!(iq.to & TO_DEN)) {
            text_on ();
            put ("There's nowhere there to put it.");
        }
        return IR_NO;
    }
    if (!(iq.to & TO_DEN)) {
        if (!(item_at (E_N)->flags & I_WORN)) {
            reach_from (E_N);                   /* (Got out first: its reach, HOUSE.md's) */
        } else {
            rtime += 50;                        /* (Worn: taken off first, a doff's 5 seconds) */
            ++rsteps;
        }
        hub_map (0);
        sn = HUB->pc_stratum;
    }
    q = item_at (E_N);
    q->where = IW_SQUARE;
    q->sn = sn;
    q->x = iq.x;
    q->y = iq.y;
    q->owner = q->hide = 0;
    q->flags = (q->flags & ~(I_HELD | I_WORN)) | I_KNOWN;
    sky_ask (E_N);                              /* (On a shaft's head: it falls, part 89's) */
    k = item_at (E_N)->kind;
    hub_map (0);
    if (iq.to & TO_DEN) {
        bd.ready += PLACE_T;
        ++HUB->den_seq;
        seen_do (WB_DROP | (unsigned) k << 8);
        return IR_OK;
    }
    text_on ();
    put ("You put the ");
    put (name (k));
    put (pw[iq.qty < sizeof pw / sizeof pw[0] ? iq.qty : 0]);
    hub_map (0);
    HUB->ireq.x = PLACE_T;
    return IR_OK;
}

/* The things' IO_EMPTY and IO_PLACE (things.c's s_serve); a denizen's FE_EMPTY, FE_PLACE (wbody.c's: iq.kind 0, its
** intent at iq.item): above */
unsigned char s_handout (void)
{
    static const struct msg* m;

    if (!iq.kind) {                             /* (A denizen's, body bd's: its intent at iq.item, I_ROB:fetch's fields) */
        m = (const struct msg*) iq.item;
        iq.kind = I_ROB_ERRAND (*m) == FE_EMPTY ? IO_EMPTY : IO_PLACE;
        iq.to = TO_DEN;
        iq.item = I_ROB_THING (*m);
        iq.x = I_ROB_X (*m);
        iq.y = I_ROB_Y (*m);
    }
    return iq.kind == IO_EMPTY ? empty () : place ();
}
