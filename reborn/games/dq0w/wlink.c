/*
** wlink.c - (dq0w) the links between strata crossed and the lights carried, every actor's by the character's rules
** (ONE-PATH.md's 2.1 and 2.7: the one-path plan's stage 2, climbing and links, lights, 2026-11-23; the act audit's A11,
** G-1, G-4, D1; the sense audit's S2, S3).
**
** A flight (a stairway, a ladder, a long slope): body bd's I_CLIMB (its mind's: an ally coming along with the character,
** a foe after it) crosses the flight joining its stratum to the one it means, the nearest within LINK_R squares of it,
** by the character's own rule (acts.h's FLIGHT_T: the drop between the floors, a step's time a foot on a stairway,
** twice on a ladder, halved down; a long slope walked), at its own step, its walk to the flight's foot a step a square;
** it comes out at the flight's other end, or beside it (someone there), and is told there as its step is (the act
** table's flight row: E_DEN DE_MOVED, seen or heard).  Its time all at once on its ready (an actor's work, as its door
** work's).  None within reach: 0, the caller's (an ally put beside the character, as before: a shaft's climb, a rope's,
** a fall, not yet any actor's: the climb's rounds by who, with an actor's fall, step1's).
**
** A light: body bd's I_LIGHT (its mind's: one that wants light, where its square's dark to it; or put out) lights a
** light of its own by the character's rules: one not lit and not spent (a lantern dry, a torch burnt out), the one in a
** hand first, else got out (into its hand, the off hand's: its time the fire-maker's getting out); a flame to light it
** from: a lit light of its own, or one lying lit within its reach (on its square or beside it, a sconce's), at once;
** else a fire-maker of its own (a tinderbox: its kind's tool "tinder", not found empty), its tries the act table's
** kindle row's (five seconds each, the referee's grope, by its sheet, 4 harder in the dark), each try's success one
** lighting off it, up to a minute's; none, it can't (AL_NOFIRE: its mind asks no more till its things change).  Lit,
** told seen (E_BEAT WB_LIGHTS: "The bandit lights its torch."), the hub's alight made again, the light layer near it out
** of date (E_LIGHT), the character's look made again if it's near.  Put out: each lit light it carries, told (WB_DOUSE,
** the character's rest's words), the act table's douse row's time each (an I_LIGHT's; a rest's, at once, as the
** character's rest_dark is).  Its lights burn down as any light does (part burning's burns_now, its own told as they go out).
**
** Swapped part den_lights's; the flight wflight.c's, part flight.
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
#pragma code-name ("SW_den_lights_CODE")
#pragma rodata-name ("SW_den_lights_RODATA")

#define LAMP_NEAR       16                      /* (A lit actor this near the character: its look made again as its light
                                                **   moves, changes) */

extern unsigned lmx[NACTORS], lmy[NACTORS];    /* (dq0w.c's: where each lit one was last told, DL_MOVED's) */
extern unsigned char lms[NACTORS];

static unsigned __fastcall__ apart (int ax, int ay)
{
    ax = ax < 0 ? -ax : ax;
    ay = ay < 0 ? -ay : ay;
    return ax > ay ? ax : ay;
}

/* The light layer near body bd out of date (its light come, gone, moved: E_LIGHT at its square), and the character's
** look made again if it's near (the main loop's, den_saw) */
static void lit_news (void)
{
    struct msg lm;

    lm.kind = E_LIGHT;
    E_LIGHT_SN (lm) = bd.sn;
    E_LIGHT_SX (lm) = bd.x;
    E_LIGHT_SY (lm) = bd.y;
    hub_map (0);
    lm.t = HUB->now;
    hub_event (&lm);
    hub_map (0);
    if (bd.sn == HUB->pc_stratum && apart (bd.x - HUB->pc_x, bd.y - HUB->pc_y) <= LAMP_NEAR) {
        den_saw = 1;
    }
}

/* Thing i's way up to whoever has it: body bd's own (worn, held, hung from it, or in what it carries), 1; and in a
** carrier not a belt (its pack, a sack), 2 */
static unsigned char __fastcall__ bd_own (unsigned i)
{
    struct item* p;
    unsigned char in = 0, k;

    for (;;) {
        p = item_at (i);
        if (p->where != IW_ITEM) {
            break;
        }
        i = p->x;
        k = item_at (i)->kind;
        hub_map (KIND_BANK);
        if (!(CATALOGUE[k].flags & K_BELT)) {
            in = 1;
        }
    }
    return p->where == IW_DEN && (p->owner | p->hide << 8) == bd.did ? 1 + in : 0;
}

/* A light spent (a lantern dry, a torch burnt out: its fuel all burnt), thing i of kind k */
static unsigned char __fastcall__ spent (unsigned i, unsigned char k)
{
    struct item* p = item_at (i);

    return p->id && !p->fuel && burn_of (k);
}

/* A flame to light body bd's light n from (not n): a lit light of its own, or one lying lit on its square or beside it
** (in a sconce there too), FL_LIT; else its fire-maker (its kind's tool "tinder", not found empty), its number; 0 none */
#define FL_LIT          0xFFFF
static unsigned __fastcall__ flame_for (unsigned n)
{
    struct item* p;
    unsigned i, j, fm = 0;
    unsigned char k, sh;

    for (i = 0; (i = items (i, IN_DEN | IN_ITEM | IN_SQUARE, 0)) != 0; ) {
        if (i == n) {
            continue;
        }
        k = ip->kind;
        sh = ip->shut;
        if (ip->flags & I_LIT) {
            if (bd_own (i)) {
                return FL_LIT;
            }
            for (j = i; (p = item_at (j))->where == IW_ITEM; j = p->x) {
            }
            if (p->where == IW_SQUARE && p->sn == bd.sn && apart (p->x - bd.x, p->y - bd.y) <= 1) {
                return FL_LIT;
            }
            continue;
        }
        if (!fm && !(sh & IS_SPENT)) {
            hub_map (KIND_BANK);
            if (!strcmp (CATALOGUE[k].tool, "tinder") && bd_own (i)) {
                fm = i;
            }
        }
    }
    return fm;
}

void __fastcall__ s_den_lights (unsigned char op);

/* A fire-maker's try, body bd's: the referee's P_GROPE by its sheet (act_test's, but its own die, RS_ASKER: a creature's
** test by the quiet dice comes out the same each try till the game's dice move, a try at a time nothing else rolling) */
static signed char __fastcall__ kindle_test (signed char steps)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = bk + 1;
    r->other = RS_ASKER;
    RS_TEST_PURPOSE (*r) = P_GROPE;
    RS_TEST_HOW (*r) = 0;
    RS_TEST_STEPS (*r) = steps;
    r = hub_ask (SLOT_WORLD);
    g = RS_TEST_GRADE (*r);
    r->state = REQ_FREE;
    return g;
}

void __fastcall__ s_den_light (const struct msg* m)
{
    struct actq aq;                             /* (The act table's kindle and douse rows) */
    struct item* p;
    unsigned i, n = 0, f;
    unsigned char h = 0, w, dark, k, fl;

    if (!I_LIGHT_LIT (*m)) {                    /* (Put out: each it carries, the douse row's time each) */
        act_get (aq, AC_DOUSE);
        for (i = 0; (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {
            if ((ip->flags & I_LIT) && bd_own (i) == 1) {
                bd.ready += aq.r.time;
            }
        }
        s_den_lights (DL_DOUSE);
        return;
    }
    dark = unlit_at (bd.sn, bd.x, bd.y);
    if (!dark) {
        bd.ready += 10;                         /* (Its square lit: none wanted) */
        return;
    }
    for (i = 0; (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {   /* (A light of its own not lit nor spent: one in a hand
                                                **   first, as the character's light is) */
        k = ip->kind;
        fl = ip->flags;
        hub_map (KIND_BANK);
        if ((fl & I_LIT) || !(CATALOGUE[k].flags & K_LIGHT) || spent (i, k) || !(w = bd_own (i))) {
            continue;
        }
        if (!n || (fl & I_HELD)) {
            n = i;
            h = w;
        }
    }
    if (!n) {
        den_lsum (0);
        bd.ready += 10;
        return;
    }
    if ((f = flame_for (n)) != 0 && f != FL_LIT) {              /* (Its fire-maker: got out, then a try, the character's) */
        act_get (aq, AC_KINDLE);
        bd.ready += item_at (f)->flags & I_HELD ? 0 : 10;
        if (!item_at (f)->fuel) {
            item_at (f)->shut |= IS_SPENT;      /* (Found empty: known from now on) */
            f = 0;
        } else {
            bd.ready += aq.r.time;              /* (A try, the kindle row's: one an ask, its mind's to try again, as the
                                                **   player's; each its own die, not one moment's tries all alike, the
                                                **   referee's quiet dice for an actor's test) */
            if (G_OK (kindle_test (dark ? -4 : 0))) {
                --item_at (f)->fuel;            /* (Caught: one lighting off it) */
                f = FL_LIT;
            }
        }
    }
    if (f != FL_LIT) {
        den_lsum (0);
        if (!f) {
            hub_map (0);
            HUB->alight[bk] |= AL_NOFIRE;       /* (No flame to be had: its mind asks no more till its things change;
                                                **   its tries all failed, it tries again as its mind asks) */
        }
        bd.ready += 10;
        return;
    }
    p = item_at (n);
    if (h == 2) {                               /* (In its pack: got out into its hand, as the character's is) */
        p->where = IW_DEN;
        p->sn = bd.hsn;
        p->x = bd.hx;
        p->y = bd.hy;
        p->owner = bd.did;
        p->hide = bd.did >> 8;
        p->flags |= I_HELD;                     /* (Its hand by its kind: a light the left, wlimbs.c's) */
        bd.ready += 10;
        p = item_at (n);
    }
    p->flags |= I_LIT;
    w = p->kind;
    if (bd.state > 1) {
        bd.state = 1;                           /* (Out of hiding: a light lit, as the character's thing handled is) */
    }
    limb_news (WB_LIGHTS | (unsigned) w << 8, bk + 1);   /* ("The bandit lights its torch.": seen) */
    den_lsum (1);
    lit_news ();
    hub_map (0);
}

void __fastcall__ s_den_lights (unsigned char op)
{
    struct actor* q;
    unsigned i;
    unsigned char k, any = 0;

    if (op == DL_MOVED) {                       /* (Each actor carrying a light lit that's moved since: the light
                                                **   layer near it out of date, the character's look made again if it's
                                                **   near; the main loop's, as their bodies change) */
        for (k = 0; k < NACTORS; ++k) {
            hub_map (0);
            q = &HUB->actor[k];
            if (dlit[k] && (q->x != lmx[k] || q->y != lmy[k] || q->sn != lms[k])) {
                lmx[k] = q->x;
                lmy[k] = q->y;
                lms[k] = q->sn;
                BD_GET (k);
                lit_news ();
            }
        }
        return;
    }
    if (op == DL_DOUSE) {
        for (i = 0; (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {
            if ((ip->flags & I_LIT) && bd_own (i) == 1) {
                k = item_at (i)->kind;
                item_at (i)->flags &= ~I_LIT;
                limb_news (WB_DOUSE | (unsigned) k << 8, bk + 1);   /* ("The bandit puts out its torch.": seen) */
                any = 1;
            }
        }
    }
    den_lsum (op != DL_DOUSE);
    if (any || op == DL_OUT) {
        lit_news ();
    }
    hub_map (0);
}
