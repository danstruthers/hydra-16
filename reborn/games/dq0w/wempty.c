/*
** wempty.c - (dq0w) a thing's things put out, and a thing put down on a square within reach (P4 and P5, the user's,
** 2026-11-18, DECISIONS.md's "The merged branches' questions": "empty X" puts its things down here, or on Y when
** named, a liquid poured away, a take's moment a handful; a thing put on a ledge, in a corner, the centre, a side or a
** passage, walked there and put down on that square), every actor's by the same code (DECISIONS.md's one path): the
** things' IO_EMPTY and IO_PLACE (things.c's s_serve), a denizen's I_ROB FE_EMPTY and FE_PLACE (wbody.c's: no mind posts
** them yet).  Swapped part 111's (../dq0/swap.inc: dq0w.cfg's SW111), through its stub, handout; the empty itself
** part 112's (wpour.c: its liquid poured into another, H11, 2026-11-19).  Its locals
** the part's own (static, in its image): what lasts across a call to another part (pick's, bulk_of's, sky_ask's) is in
** the request's copy, iq, the main RAM's, as the drag's is (wdrag.c)
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
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;
unsigned __fastcall__ pick (const char* name, unsigned char how);   /* (Part 4's) */
unsigned char pour (void);                      /* (Part 112's: wpour.c, IO_EMPTY, every actor's) */
unsigned char __fastcall__ den_things (const struct msg* m);   /* (Part 114's: wthings.c, a denizen's things) */

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
#define E_B             iq.dur_max              /*   the room they want, */
#define E_L             iq.y                    /*   its liquid, */
#define E_C             iq.x                    /*   what's in the other now) */
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

/* The things' words begun (the UI bank's things_text); a denizen's, none (put's room nil: its words aren't the
** character's to read) */
static char nob;
static void text_on (void)
{
    if (iq.to & TO_DEN) {
        tp = tpe = &nob;
        return;
    }
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

/* The things' IO_EMPTY and IO_PLACE (things.c's s_serve: m 0); a denizen's FE_EMPTY, FE_PLACE (wbody.c's: its intent
** m, I_ROB:fetch's fields), above.  A denizen's in a request copy of its own: the character's, iq, kept on the C stack
** and put back after, so a denizen's turn that comes in the middle of the character's request (a stratum grown as a
** thing falls down a shaft, its populations waited for: kept_wait's den_intents) leaves it as it was */
#pragma static-locals (push, off)
unsigned char __fastcall__ s_handout (const struct msg* m)
{
    struct ireq k;
    unsigned char r;

    if (!m) {
        return iq.kind == IO_EMPTY ? pour () : place ();
    }
    if (I_ROB_ERRAND (*m) > FE_PLACE) {
        return den_things (m);                  /* (Its things handled by the character's rules, the act table's rows:
                                                **   wthings.c's, part 114's, the one-path plan's stage 2, 2026-11-23) */
    }
    memcpy (&k, &iq, sizeof iq);
    iq.kind = I_ROB_ERRAND (*m) == FE_EMPTY ? IO_EMPTY : IO_PLACE;
    iq.to = TO_DEN;
    iq.item = I_ROB_THING (*m);
    iq.x = I_ROB_X (*m);
    iq.y = I_ROB_Y (*m);
    r = iq.kind == IO_EMPTY ? pour () : place ();
    memcpy (&iq, &k, sizeof iq);
    return r;
}
#pragma static-locals (pop)
