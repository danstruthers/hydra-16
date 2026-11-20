/*
** wpour.c - (dq0w) a thing's things put out, its liquid poured (P4, the user's, 2026-11-18: "empty X" puts its things
** down here, or on Y when named, a liquid poured away, a take's moment a handful; H11, 2026-11-19: "pour the water into
** the flask", "empty the waterskin into the flask", the liquid into the other, as far as there's room), every actor's by
** the same code (DECISIONS.md's one path): the things' IO_EMPTY, a denizen's I_ROB FE_EMPTY (wempty.c's s_handout's:
** no mind posts it yet).  Swapped part 112's (dq0w.cfg's SW112: part 111's room), through its stub, pour.  Its
** locals the part's own (static, in its image): what lasts across a call to another part (pick's, bulk_of's,
** bulk_in's) is in the request's copy, iq, the main RAM's, as the drag's is (wdrag.c)
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
unsigned char pour_said (void);                 /* (Part 113's: wpour2.c, the act and its words) */

#pragma code-name ("SW112CODE")
#pragma rodata-name ("SW112RODATA")
#pragma bss-name ("SW112RODATA")
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

/* A free record (1 on), or 0 none */
static unsigned free_rec (void)
{
    static unsigned i;

    for (i = 1; i < ilim && item_at (i)->kind; ++i) {
    }
    hub_map (0);
    return i < ilim ? i : 0;
}

/* Whether thing n's to hand: the character's (carried, or lying, or in a thing lying, on its square or one beside
** it), or body bd's (iq.to's TO_DEN: its own, or lying beside it): 1; else 0, where it lies in the reply (the UI
** walks there first; a denizen's mind's to bring it there) */
static unsigned char __fastcall__ in_hand (unsigned n)
{
    static struct item* p;
    static unsigned char w, sn, o, h, as;
    static unsigned x, y, ax, ay;

    while ((p = item_at (n))->where == IW_ITEM) {
        n = p->x;                               /* (Its outermost holder's place) */
    }
    w = p->where;
    sn = p->sn;
    x = p->x;
    y = p->y;
    o = p->owner;
    h = p->hide;
    hub_map (0);
    if (iq.to & TO_DEN) {
        if (w == IW_DEN && o == (unsigned char) bd.did && h == bd.did >> 8) {
            return 1;                           /* (Its own: wtake.c's) */
        }
        as = bd.sn;
        ax = bd.x;
        ay = bd.y;
    } else {
        if (w == IW_ACTOR && !x) {
            return 1;
        }
        as = HUB->pc_stratum;
        ax = HUB->pc_x;
        ay = HUB->pc_y;
    }
    if (w == IW_SQUARE && sn == as && x - ax + 1 <= 2 && y - ay + 1 <= 2) {
        return 1;
    }
    if (!(iq.to & TO_DEN)) {
        HUB->ireq.x = x;
        HUB->ireq.y = y;
        HUB->ireq.item = 0;
    }
    return 0;
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

/* The liquid kind word w names, its very name ("water": not the waterskin's, which the chooser takes first), or 0 */
static unsigned char __fastcall__ liquid_named (const char* w)
{
    static unsigned char k;

    hub_map (KIND_BANK);
    for (k = 1; k < NKINDS; ++k) {
        if ((CATALOGUE[k].form & KF_LIQUID) && !strcmp (CATALOGUE[k].name, w)) {
            return k;
        }
    }
    return 0;
}

/* None to hand of the word w (said.txt's SA.no_such: "You have no flask to hand."): IR_NO */
static unsigned char __fastcall__ none (const char* w)
{
    static const char c[] = { SAY_AT, SA_NO_SUCH | 0x80, 0 };

    text_on ();
    put (c);
    put (w);
    put ("\2\2");                               /* (SAY_SEP, no other, SAY_SEP) */
    return IR_NO;
}

/* "The {kind k}{s}" put: IR_NO */
static unsigned char __fastcall__ the (unsigned char k, const char* s)
{
    put ("The ");
    put (name (k));
    put (s);
    return IR_NO;
}

/* A thing's things put out, its liquid poured (IO_EMPTY), every actor's: the character's (iq.name, carried or within
** reach, or the liquid in it named, "pour the water into the flask": that alone; iq.into the other, a surface to 1,
** or none: at its feet), or body bd's (iq.to's TO_DEN: its own thing iq.item, the other iq.x or none, no words).
** Into the other: the solid things, and the liquid as far as there's room (the rest stays: pour_in's).  Refused: not a
** thing that holds things, closed (sealed, locked), nothing in it; the other not one that holds things, closed, no
** room in it for the solid ones; for the liquid, not one that holds a liquid, full, or another liquid in it already (no
** mixing: a vessel holds one liquid, the user's H11, 2026-11-19).  A flask's oil only poured away: a lantern's filled
** from it (IO_FILL) */
static unsigned char empty (void)
{
    static unsigned char k, ky, sh, f, oil, any, lk, yk, room;
    static unsigned lq, cap, yf;
    static const char* ln;
    static char lb[14];
    static struct item* q;

    iq.size = 0;
    if (iq.to & TO_DEN) {
        E_Y = iq.x;
    } else {
        E_Y = 0;
        if (!(E_N = pick (iq.name, HW_ANY))) {
            return none (iq.name);
        }
        if (*iq.into && !(E_Y = pick (iq.into, HW_ANY))) {
            return none (iq.into);
        }
        if ((k = liquid_named (iq.name)) != 0 && (yk = item_at (E_N)->kind) != k && (E_N == E_Y || (!strncmp (name (yk),
            iq.name, lk = strlen (iq.name)) && name (yk)[lk] >= 'a'))) {
            for (E_I = 0; (E_I = items (E_I, IN_ITEM, 0)) != 0; ) {
                if (ip->kind == k && in_hand (E_I)) {
                    E_N = E_I;                  /* ("Pour the water": the water to hand, wherever it's in, the chooser's
                                                **   "waterskin" only a word's start; "the oil", a flask of oil named,
                                                **   the chooser's, as the UI's first words have it: sway.c's ix_pick) */
                    break;
                }
            }
        }
    }
    E_L = 0;
    q = item_at (E_N);
    k = q->kind;
    hub_map (KIND_BANK);
    if (CATALOGUE[k].form & KF_LIQUID) {        /* (The liquid named: its holder's, it alone) */
        q = item_at (E_N);
        if (q->where != IW_ITEM) {
            text_on ();
            return the (k, " isn't in anything to pour from.");
        }
        E_L = E_N;
        E_N = q->x;
        iq.size = 1;
    }
    if (E_Y == E_N) {
        if (!E_L) {
            return IR_NONE;
        }
        text_on ();
        put ("The ");
        put (name (k));
        put (" is in the ");
        put (name (item_at (E_N)->kind));
        put (" already.");
        return IR_NO;
    }
    if (!in_hand (E_N) || (E_Y && !in_hand (E_Y))) {
        return IR_NOT_HERE;                     /* (Walked over to it first: the UI's) */
    }
    E_B = 0;
    if (E_Y) {
        E_C = bulk_in (E_Y);               /* (What's in it now) */
        for (E_I = 0; !iq.size && (E_I = items (E_I, IN_X | IN_ITEM, E_N)) != 0; ) {
            if (!IC_OF (ip->y)) {
                k = ip->kind;
                hub_map (KIND_BANK);
                if (!(CATALOGUE[k].form & KF_LIQUID)) {
                    E_B += bulk_of (E_I);       /* (The room the solid ones want) */
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
    for (E_I = 0; !E_L && (E_I = items (E_I, IN_X | IN_ITEM, E_N)) != 0; ) {
        if (!IC_OF (ip->y)) {
            lk = ip->kind;
            hub_map (KIND_BANK);
            if (CATALOGUE[lk].form & KF_LIQUID) {
                E_L = E_I;                      /* (Its liquid: one, a vessel holds one) */
            }
        }
    }
    lq = 0;
    if (E_L) {
        q = item_at (E_L);
        lk = q->kind;
        lq = q->qty;
    }
    yk = 0;
    if (E_Y) {
        q = item_at (E_Y);
        ky = q->kind;
        f = q->shut;
        yf = q->fuel;
        for (E_I = 0; (E_I = items (E_I, IN_X | IN_ITEM, E_Y)) != 0; ) {
            if (!IC_OF (ip->y)) {
                k = ip->kind;
                hub_map (KIND_BANK);
                if (CATALOGUE[k].form & KF_LIQUID) {
                    yk = k;                     /* (A liquid in it already) */
                }
            }
        }
        k = item_at (E_N)->kind;
    }
    if (oil && E_Y) {
        lk = KI_LAMP_OIL;                       /* (A flask's oil poured into a vessel: lamp oil, a pint at most, the
                                                **   user's H14, 2026-11-19) */
        lq = 1;
    }
    ln = oil ? "oil" : lq ? strcpy (lb, name (lk)) : "";
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
    iq.osel = lq;
    if (E_Y && ky == KI_OIL) {                  /* (Into a flask of oil: its lamp oil, as far as it takes, H14's) */
        if (E_B || !lq || lk != KI_LAMP_OIL) {
            put ("The flask of oil won't hold ");
            put (lq && !E_B ? ln : "them");
            put (".");
            return IR_NO;
        }
        if (f & IS_CLOSED) {
            return the (ky, " is closed.");
        }
        if (yf >= OIL_PINT) {
            return the (ky, " is full.");
        }
        iq.size |= 2;                           /* (By its measure, minutes: wpour2.c's pour_in) */
    } else if (E_Y) {
        hub_map (KIND_BANK);
        if (!(CATALOGUE[ky].flags & K_CONTAINER) && !(CATALOGUE[ky].keeps & KF_ON)) {
            return the (ky, " won't hold them.");
        }
        if (f & IS_CLOSED) {
            return the (ky, " is closed.");
        }
        cap = CATALOGUE[ky].cap_bulk;
        if (E_B + E_C > cap) {
            put (iq.to ? "There's no room for them on the " : "There's no room for them in the ");
            put (name (ky));
            put (".");
            return IR_NO;
        }
        if (lq) {
            room = cap - E_C - E_B > 255 ? 255 : cap - E_C - E_B;
            hub_map (KIND_BANK);
            if (!(CATALOGUE[ky].keeps & KF_LIQUID)) {
                put ("The ");
                put (name (ky));
                put (" won't hold ");
                put (ln);
                put (".");
                return IR_NO;
            }
            if (yk && yk != lk) {
                put ("The ");
                put (name (ky));
                put (" has ");
                put (name (yk));
                put (" in it already.");
                return IR_NO;
            }
            if (!room) {
                return the (ky, " is full.");
            }
            if (room < lq) {
                iq.osel = room;
            }
            if (!yk && (room < lq || oil) && !free_rec ()) {
                return the (ky, " is full.");   /* (No record for the part poured, or the flask's oil: as full,
                                                **   nothing poured) */
            }
        }
    }
    iq.craft = k;                               /* (Done and told by part 113's: pour_said, wpour2.c) */
    iq.quality = ky;
    iq.plural = lk;
    iq.count = lq;
    iq.thick = oil;
    return pour_said ();
}

/* The part's way in (its stub, pour: wempty.c's s_handout's IO_EMPTY, every actor's) */
unsigned char s_pour (void)
{
    return empty ();
}