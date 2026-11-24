/*
** wstep.c - (dq0w) the step and its hazards, every actor's (the one-path plan's stage 2, ONE-PATH.md's 2.1 and 2.7;
** the user's, 2026-11-17: "the player and any agent's interaction with the world are handled the same, in all cases";
** the act audit's G1, G3, B9): a body's step onto a trap, the character's rule (wtrap.c's step and spring): a hidden one
** noticed in time (the referee's P_NOTICE_SIGHT, a passing glance's, its hiding the steps worse: it stops short, the trap
** known to its faction, as a search's find is: the user's, 2026-11-23) or set off (a pit fallen into, its spikes; a deadfall, a blade, darts
** dodged or struck by; gas resisted or choked on, senseless a while; an alarm's bell); a door's trap set off as it opens
** it (wdoor.c's, by the same parts); the world's harm to a body (the referee's RS_HARM, its actor: a fall's, a trap's,
** its wound a blow's, knocked senseless as a blow's, its rolls a round each: the user's K14), told as the fights' words
** are (E_FIGHT, FO_HURT: its wound, its fall senseless or dead, the minds' to take to heart); one in a pit climbs out a
** round at a time (the character's pit's climb: P_CLIMB, its depth, a slip back to its floor, a bad one a fall).
** A known trap is gone round, the denizens' walks' as the character's (dq0d's wet_ok): known to its faction (its own,
** as its lair's placed: own_traps; another's found), the user's, 2026-11-23.  A winged body over a floor's traps, pits
** and spilt oil (hz.s); its harm's wound tests its courage as a blow's (den_harm); its cry a noise (DE_CRY).
**
** Swapped part 120's (../dq0/swap.inc: dq0w.cfg's SW120), through its stubs: den_trap (dq0w.c's den_hz, the step's
** cheap test: a pit, a chunk with traps); part 121's den_harm (workpart.h's harm, an actor's); part 122's trap_beat
** (dq0w.c's work_told, an actor's).  Its locals on the C stack: a call to another part copies this one in again after it.
*/

#pragma local-strings (on)

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "work.h"
#include "wbody.h"

#pragma code-name ("SW_step_CODE")
#pragma rodata-name ("SW_step_RODATA")
#pragma bss-name ("SW_step_RODATA")
#pragma static-locals (off)

extern unsigned char fring;                     /* (wfight.c's: the fights' ring's next) */
extern unsigned char bclimb[NACTORS];           /* (wbody.c's: each body's climb out of its pit so far, feet) */
#define LOUD_HURT       5                       /* (A cry of pain: a fight's din, wfight.c's LOUD_FIGHT (a word's 3 till
                                                **   2026-11-23, when it was told at any distance; heard now as any noise
                                                **   is, a word's never carried past the background: step2's, the house's) */
#define CHOKE_T         300                     /* (A gas trap's: senseless half a minute a point of its power, wtrap.c's) */

/* C's trap on square x, y of body bd's stratum that's set and set off underfoot (not a door's): into q, 1; or 0 */
static unsigned char __fastcall__ trap_here (unsigned x, unsigned y, struct trap* q)
{
    struct trap* t;
    unsigned char n;

    if (!at (bd.sn, x, y)) {
        return 0;
    }
    for (t = C->trap, n = C->ntraps; n; --n, ++t) {
        if (t->x == cx_ && t->y == cy_ && TS_SET (t->state) && t->trigger != TT_DOOR) {
            *q = *t;
            return 1;
        }
    }
    return 0;
}

/* The referee's test of body bd's (its actor), p, b, its steps: its grade */
static signed char __fastcall__ den_test (unsigned char p, unsigned char b, signed char s)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = bk + 1;
    RS_TEST_PURPOSE (*r) = p;
    RS_TEST_HOW (*r) = b;
    RS_TEST_STEPS (*r) = s;
    r = hub_ask (SLOT_WORLD);
    g = RS_TEST_GRADE (*r);
    r->state = REQ_FREE;
    return g;
}

#pragma code-name (push, "SW_harm_CODE")          /* (Part 121's: part 120's room) */
#pragma rodata-name (push, "SW_harm_RODATA")
#pragma bss-name (push, "SW_harm_RODATA")

/* Body bd's harm (RS_HARM, its actor: the world's slot's request made, as workpart.h's harm, the character's): its wound
** told (E_FIGHT's FO_HURT: its words the referee's, the ring's third line; its fall senseless, its death), its legs as
** it leaves them, fallen prone, what it held let fall, bleeding; senseless till its time out (its shock rolls a round
** each as its time comes, the user's K14: a blow's way); dead, its remains where it lies (wfight.c's died) */
static const char what_hurt[8][9] = { "fall\n\n", "fall\n\n", "spikes\n\n", "stones\n\n", "blade\n\n", "darts\n\n",
    "gas\n\n", "bell\n\n" };                    /* (A trap's strike, by its kind, TK_*: the fights' words' weapon) */

void s_den_harm (void)
{
    struct req* r;
    struct fight* f;
    const struct rs_effect* e;
    char t[sizeof ((struct fight*) 0)->text];
    unsigned char i, ff = 0, cd = 0, prone = 0, band, af;
    unsigned out;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->actor = bk + 1;
    r->w2 |= HUB->folk[bk] ? RS_THEY : 0;       /* (A person's "their") */
    RS_HARM_HEART (*r) = bd.heart;              /* (Its courage tested as it's wounded, as a blow's: its heart, shy; */
    RS_HARM_SHY (*r) = bd.timid;                /*   the user's, 2026-11-23) */
    r = hub_ask (SLOT_WORLD);
    out = RS_HARM_OUT (*r);
    band = RS_HARM_BAND (*r);
    af = RS_HARM_AF (*r);
    ff = RS_HARM_LEFT (*r) & FF_FLEES;
    for (i = 0, e = (const struct rs_effect*) RS_HARM_EFFECTS (*r); i < RS_EFFECTS && e->kind; ++i, ++e) {
        if (e->kind == X_WOUND) {
            bd.move = e->cond >> 12 & 7;        /* (How its legs move it now, C_MOVE's) */
        } else if (e->kind == X_DEATH) {
            ff |= FF_A_DEAD;
        } else if (e->kind == X_DROP) {
            cd = 1 + e->x[1];                   /* (Its weapon let fall: part 60's) */
        } else if (e->kind == X_COND_ON && (e->cond & C_PRONE)) {
            prone = 1;
        }
    }
    strcpy (t, r->a == HC_FALL ? "fall\n\n" : r->a == HC_FIRE ? "fire\n\n" : what_hurt[wk.tq.kind & 7]);   /* (What hurt
                                                **   it, the ring's first line: a trap's, an actor's work's) */
    i = strlen (t);
    strncpy (t + i, r->text, sizeof t - 1 - i);
    t[sizeof t - 1] = 0;
    RS_HARM_BAND (*r) = 0;                      /* (The character's dart poison none of its: workpart.h's spring) */
    r->state = REQ_FREE;
    if (af & AF_BLEEDING) {
        bleeds |= 1 << bk;
    }
    if (out) {
        bd.doing = DO_DOWN;                     /* (Senseless: it lies there till it comes round, a blow's way) */
        bd.ready = HUB->now + out;
        ff |= FF_A_DOWN;
        prone = 1;
    } else if (ff & FF_FLEES) {
        bd.doing = DO_FLEE;                     /* (Broken: it flees, a blow's way, wfight.c's) */
    } else if (af & AF_CAUTIOUS) {
        bd.ready += HUB->round;                 /* (Shaken: it holds back a round) */
    }
    if (prone) {
        bd.pos = POS_PRONE;
    }
    if (band || ff) {                           /* (Its wound told, as a blow's: the fights' ring, E_FIGHT) */
        hub_map (DEN_BANK);
        f = FIGHT_AT + fring;
        memset (f, 0, sizeof *f);
        f->out = out;
        memcpy (f->att, bd.name, sizeof f->att);
        memcpy (f->text, t, sizeof t);
        hub_map (0);
        bev.kind = E_FIGHT;
        bev.who = bk + 1;
        E_FIGHT_DEF (bev) = 0;
        E_FIGHT_LOUD (bev) = LOUD_HURT;
        E_FIGHT_SX (bev) = bd.x;
        E_FIGHT_SY (bev) = bd.y;
        E_FIGHT_SN (bev) = bd.sn;
        E_FIGHT_OUT (bev) = FO_HURT;
        E_FIGHT_FF (bev) = ff;
        E_FIGHT_WORDS (bev) = fring;
        bev.t = HUB->now;
        hub_devent (&bev);
        fring = (fring + 1) % NFIGHTS;
        den_told (ff & FF_A_DEAD ? DE_CRYCUT : DE_CRY, LOUD_HURT, bd.x, bd.y);    /* (Its cry, told only where it's
                                                **   heard, as any noise: the senses' hearing, the UI's; seen, its
                                                **   wound's words alone, the user's, 2026-11-23) */
    }
    if (prone && !(ff & ~FF_FLEES)) {
        limb_news (WB_FALLEN, (bk + 1) | 0x80);    /* (Fallen: seen, or heard near, as any fall) */
    }
    if (cd && !(ff & FF_A_DEAD)) {
        look_more (0, LM_ROPES);
        den_drop (cd - 1);                      /* (Its weapon on the floor, told: part 60's) */
    }
    if (ff & FF_A_DEAD) {                       /* (Dead: its remains where it lies, its things on them) */
        bd.state = 0;
        bleeds &= ~(1 << bk);
        if (GRIP_OF (grip) == bk + 1) {
            grip = 0;
        }
        den_strip (lay_body (bd.remains, bd.sn, bd.x, bd.y));
        den_saw = 1;
    }
    hub_map (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)

#pragma code-name (push, "SW_trap_told_CODE")          /* (Part 122's: part 120's room) */
#pragma rodata-name (push, "SW_trap_told_RODATA")
#pragma bss-name (push, "SW_trap_told_RODATA")

/* A trap's outcome for body bd (the world's work for an actor, wk.who: dq0w.c's work_told's, E_TRAP's w1, wtrap.c's
** trap_told): told if it's seen (E_BEAT's WB_TRAP: the limbs' news; a pit's fall heard near), but one noticed (the
** character not told, as a search's find); choked on its gas, senseless half a minute a point of its power, prone; an
** alarm's bell rung at its square (E_NOISE, as loud as any: the denizens hear it) */
void __fastcall__ s_trap_beat (unsigned char kind, unsigned w1)
{
    struct msg m;
    unsigned char o = w1 >> 8, k = w1 & 7;

    if (kind != E_TRAP || o == TO_NOTICED || o == TO_FOUND) {
        return;                                 /* (Its noise, an alarm's: told as its trap is, below) */
    }
    if (o == TO_CHOKED) {
        bd.doing = DO_DOWN;
        bd.pos = POS_PRONE;
        hub_map (0);
        bd.ready = HUB->now + (unsigned long) wk.tq.power * CHOKE_T;
    }
    if (k == TK_ALARM) {
        hub_map (0);
        m.kind = E_NOISE;
        m.who = 0;
        E_NOISE_LOUD (m) = 9;
        E_NOISE_WHAT (m) = NZ_WORK;
        E_NOISE_SX (m) = wk.tx;
        E_NOISE_SY (m) = wk.ty;
        m.t = HUB->now;
        hub_event (&m);
    }
    if (o == TO_OPENED) {
        return;                                 /* (A pit's cover fallen in at a door: no one on it) */
    }
    limb_news (WB_TRAP | (unsigned) o << 8 | (unsigned) k << 12, (bk + 1) | (k <= TK_SPIKED || o == TO_CHOKED ? 0x80 : 0));
}

/* A lair of folk placed (I_KIT's KT_TRAPS, the minds' s_place; wlimbs.c's den_other): its faction (b) knows the traps
** set underfoot in its chunk from now on, its own (their knowers, TRAP_STATE's: the user's, 2026-11-23, "each faction
** knows its own traps"; a beast's lair's, the dead's, no one's) */
void __fastcall__ s_own_traps (const struct msg* m)
{
    struct trap* t;
    unsigned char i, kb = TRAP_KB (I_KIT_FACTION (*m)), sn = I_KIT_SN (*m);
    unsigned x = I_KIT_X (*m), y = I_KIT_Y (*m);

    for (i = 0; at (sn, x, y) && i < C->ntraps; ++i) {    /* (The chunk in the window again each time: change's leaves
                                                **   the hub there) */
        t = &C->trap[i];
        if (t->trigger != TT_DOOR && !(t->know & kb)) {
            change (sn, x - cx_ + t->x, y - cy_ + t->y, t->state, kb, TRAP_STATE);
        }
    }
    hub_map (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)

/* Body bd's step its way d (from its square bd.x, bd.y; on it, DT_SPRING), the step's hazards (dq0w.c's den_hz, as
** there's a pit or a trap):
** DT_PIT, it's in a pit, a round's climb out (the character's: P_CLIMB, its depth its trap's power, 10 feet none
** known; a critical success 20 feet, a success 10, a failure 5 back, a slip off the wall back to its floor; a critical
** failure off it a fall, from as high as it got): 1 if it's in it still; DT_NOTICE, a hidden trap on x, y noticed in
** time (its time a refused step's): 1, it stops short; DT_SPRING, on its square, the trap there set off, by wtrap.c's step (its
** work, wk.who its actor, the character's kept round it) */
unsigned char __fastcall__ s_den_trap (unsigned char op, unsigned char d)
{
    struct work keep;
    struct req* r;
    struct trap q;
    struct trap* t;
    unsigned char n, need;
    unsigned x = bd.x, y = bd.y;
    signed char g;

    if (op == DT_PIT) {
        at (bd.sn, bd.x, bd.y);
        for (need = 10, t = C->trap, n = C->ntraps; n; --n, ++t) {
            if (t->x == cx_ && t->y == cy_ && t->power) {
                need = t->power;
            }
        }
        d = bclimb[bk];                         /* (Its way of no more use: the climb so far) */
        g = den_test (P_CLIMB, 0, 0);
        hub_map (0);
        bd.ready += HUB->round;
        if (g == G_BADLY && d) {
            bclimb[bk] = 0;                     /* (Its grip lost: a fall back to its floor, from as high as it got) */
            hub_map (0);
            r = &HUB->slot[SLOT_WORLD];
            r->kind = RS_HARM;
            r->other = 0;
            r->a = HC_FALL;
            r->b = GR_FLOOR;
            r->c = HF_CLEAR | HF_HOLDS;
            r->w1 = d;
            r->w2 = 0;
            r->text[0] = 0;
            den_harm ();
            return 1;
        }
        if (g < G_PASS && d) {
            bclimb[bk] = 0;                     /* (A slip: back on its floor, the climb begun again) */
            return 1;
        }
        d = g == G_WELL ? d + 20 : g == G_PASS ? d + 10 : 0;
        if (d < need) {
            bclimb[bk] = d;
            return 1;
        }
        bclimb[bk] = 0;                         /* (Out: its step onto the edge, its own time) */
        return 0;
    }
    if (op == DT_NOTICE) {
        x += head_x[d];
        y += head_y[d];
    }
    if (!trap_here (x, y, &q)) {
        return 0;
    }
    if (op == DT_NOTICE) {
        n = TRAP_KB (bd.faction);
        if (TRAP_KNOWN (&q, n) || (q.state & TS_MAGIC) || !G_OK (den_test (P_NOTICE_SIGHT, 1, -(signed char) q.hide))) {
            return 0;
        }
        change (bd.sn, x, y, q.state, n, TRAP_STATE);   /* (Known from now on to its faction, the user's, 2026-11-23:
                                                **   another's trap known once found; the character's own find its
                                                **   TS_HIDDEN) */
        bd.ready += 10;
        return 1;
    }
    memcpy (&keep, &wk, sizeof wk);
    memset (&wk, 0, offsetof (struct work, pq));
    wk.who = bk + 1;
    wk.sn = bd.sn;
    wk.what = W_STEP;
    wk.px = wk.fx = x;
    wk.py = wk.fy = y;
    bclimb[bk] = 0;
    traps ();                                   /* (Set off: wtrap.c's step, the character's own, by its sheet) */
    memcpy (&wk, &keep, sizeof wk);
    hub_map (0);
    return 0;
}
