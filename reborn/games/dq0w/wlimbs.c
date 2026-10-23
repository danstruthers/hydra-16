/*
** wlimbs.c - (dq0w) the hands and limbs (slice 4.1b; the user's, 2026-10-21: "if you have a limb injury, it should
** affect that limbs usefulness ... the HM fumble rules should be followed for hand and arm wounds and dropping anything
** held in the hand on that side"): what a hand lets go of, fumbled (the referee's X_DROP: the book's, Skills 21, onto a
** neighbouring square or its own), told; what's to hand told the referee after (RS_EQUIP); a harm's effects (a fall's,
** a trap's: what it dropped, a leg's stumble laying it prone).
**
** And the denizens' limbs (part 51's): a creature's weapon let fall (the referee's X_DROP, a creature's: on the floor,
** seen, taken up again in its reach, its turn), its legs at a move's start (stuck, prone).
**
** All of it swapped part 49's and part 51's (../dq0/swap.inc: dq0w.cfg's SW49, SW51), called through their stubs
** (bank.s's).
*/

#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

#pragma local-strings (on)
#pragma code-name ("SW49CODE")
#pragma rodata-name ("SW49RODATA")

unsigned __fastcall__ weight_in (unsigned where, unsigned x);  /* (Part 10's, things.c's) */

static struct msg lm;                           /* (An event told) */
static struct item* lp;

/* The thing at hand (ip) on the character, however deep (lp, what holds it at the last) */
static unsigned char __fastcall__ on_me (void)
{
    for (lp = ip; lp->where == IW_ITEM; lp = item_at (lp->x)) {
    }
    return lp->where == IW_ACTOR && !lp->x;
}

/* What's to hand told the referee at once (RS_EQUIP 2, as the UI's to_hand tells it a thing handled): the keys of the
** tools the character still carries, however deep, each after its mark (held: its hand's, < the left, > the right, *
** both; + worn; a space neither) and with its quality (things.c's tools); their weight, its load (weight_in's): a
** robbery's, a thing let go of */
void s_equip_now (void)
{
    static const char hmark[4] = "><>*";
    static char wb[12];
    static unsigned i;
    static unsigned char kind, f, n, h, hh, mh;
    struct req* r;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->text[0] = 0;
    mh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    for (hh = 0, i = 0; (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
        kind = ip->kind;
        f = ip->flags;
        n = ip->quality;
        h = ip->sn & IH_BOTH;
        if (!on_me ()) {
            continue;
        }
        if ((f & I_HELD) && lp == ip) {
            hh |= h ? h : mh;                   /* (The hands that hold something) */
        }
        hub_map (CAT_BANK);                     /* (Its key after its mark, its quality after it) */
        memcpy (wb + 1, CATALOGUE[kind].tool, sizeof CATALOGUE[0].tool);
        hub_map (0);
        if (wb[1] && strlen (r->text) < REQ_TEXT - sizeof wb) {
            wb[0] = f & I_HELD ? hmark[h] : f & I_WORN ? '+' : ' ';
            h = strlen (wb);
            wb[h] = '0' + n;
            wb[h + 1] = 0;
            strcat (r->text, wb);
        }
    }
    i = weight_in (IW_ACTOR, 0);
    hub_map (0);
    r->kind = RS_EQUIP;
    r->actor = 0;
    r->a = 2;
    r->w1 = i;
    r->w2 = hh;
    hub_ask (SLOT_WORLD)->state = REQ_FREE;
}

/* The limbs' news (E_BEAT's w, WB_* and more; b whose, 0 the character, else the actor's entry plus 1), in the
** denizens' ring (a fight's: after its blows' words) */
void __fastcall__ s_limb_news (unsigned w, unsigned char b)
{
    hub_map (0);
    lm.kind = E_BEAT;
    lm.who = 0;
    lm.b = b;
    lm.w1 = w;
    lm.t = HUB->now;
    hub_devent (&lm);
}

/* What the character holds in hands h (rs.h's DH_*: the referee's X_DROP's) let go of, fumbled: each on the floor where
** the book's die puts it (Skills 21: a d10, 1-6 the square that way, north, northeast, southeast, south, southwest,
** northwest, if it can lie there; 7-10 its own), told (E_BEAT's WB_DROP, its kind; in the denizens' ring, ring 1, after
** a blow's words, else the world's: a harm's, after its words); the referee told what's to hand, its light anew, the
** world to look about again (it seen lying there) */
void __fastcall__ s_drop_held (unsigned char h, unsigned char ring)
{
    static const unsigned char way[6] = { H_N, H_NE, H_SE, H_S, H_SW, H_NW };
    static unsigned i, x, y;
    static unsigned char k, d, n, sn, mh;
    unsigned char* q;
    struct item* p;

    hub_map (0);
    mh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    sn = HUB->pc_stratum;
    for (n = 0, i = 0; (i = items (i, IN_X | IN_ACTOR, 0)) != 0; ) {
        if (!(ip->flags & I_HELD) || !((ip->sn & IH_BOTH ? ip->sn & IH_BOTH : mh) & h)) {
            continue;
        }
        k = ip->kind;
        hub_map (0);
        x = HUB->pc_x;
        y = HUB->pc_y;
        if ((d = dice (1, 10)) <= 6) {          /* (Beside it, that way, if it's open floor) */
            d = way[d - 1];
            q = at (sn, x + head_x[d], y + head_y[d]);
            if (q && (TILE (*q) == T_FLOOR || TILE (*q) == T_CORR || TILE (*q) == T_STEPS)) {
                x += head_x[d];
                y += head_y[d];
            }
        }
        p = item_at (i);
        p->where = IW_SQUARE;
        p->sn = sn;
        p->x = x;
        p->y = y;
        p->flags &= ~(I_HELD | I_WORN);
        hub_map (0);
        if (HUB->pc_strike == (SK_THING | k)) {
            HUB->pc_strike = SK_BEST;           /* (What struck let go of: what's in its hands now strikes, the fight's
                                                **   next blow) */
        }
        lm.kind = E_BEAT;
        lm.who = 0;
        lm.b = 0;                               /* (The character's: else the actor's plus 1) */
        lm.w1 = WB_DROP | (unsigned) k << 8;
        lm.t = HUB->now;
        if (ring) {
            hub_devent (&lm);
        } else {
            hub_event (&lm);
        }
        ++n;
    }
    if (n) {
        light_now ();                           /* (A light let go of, still lit where it lies) */
        s_equip_now ();
        den_saw = 1;                            /* (The world to look about again: it seen lying there) */
    }
    hub_map (0);
}

/* A harm's effects (the referee's RS_HARM reply, the world's slot, as it's answered: a fall's, a trap's), the
** character's: what its hands let go of (X_DROP, an arm's fumble), fallen prone (a leg's stumble: an X_WOUND's
** C_PRONE), the world's body's position (the hub's pc_pos) */
void s_harm_fx (void)
{
    static const struct rs_effect* e;
    static unsigned char k, h, pr;

    hub_map (0);
    e = (const struct rs_effect*) HUB->slot[SLOT_WORLD].x;
    for (h = pr = k = 0; k < RS_EFFECTS && e->kind; ++k, ++e) {
        if (e->target) {
            continue;
        }
        if (e->kind == X_DROP) {
            h |= e->x[0];
        }
        pr |= (e->cond & C_PRONE) != 0;
    }
    if (pr) {
        HUB->pc_pos = POS_PRONE;
        lm.kind = E_BEAT;                       /* ("You fall.") */
        lm.who = lm.b = 0;
        lm.w1 = WB_FALLEN;
        lm.t = HUB->now;
        hub_event (&lm);
    }
    if (h) {
        s_drop_held (h, 0);
    }
}

/* ==== Part 51: the denizens' limbs (slice 4.1b; the brief's: "The same rules for every actor with hands and legs: a
** denizen's fumble drops its weapon (the world's floor, seen and taken up), a useless leg hops or crawls as the
** character's does (a quadruped on three good legs at half its move, the book's), a prone denizen gets up (its round).
** A creature's own parts (bites, claws) are not dropped."): body bd's (wbody.h's) weapon let fall, its word the
** referee's, its kind the catalogue's of that name, where the book's die puts it (as the character's: Skills 21),
** kept as the body's to take up again in its reach, its turn (the referee's RS_REARM, O_REARM); its legs at a move's
** start.  Its own part, the main RAM's room */
#pragma code-name (push, "SW51CODE")
#pragma rodata-name (push, "SW51RODATA")

static char dword[8];                           /* (Its weapon let fall, in a word: the referee's) */

/* Its weapon's word: line n of a fight's reply's text t (the hub's: 0 the attacker's weapon, 1 the defender's) */
void __fastcall__ s_den_word (const char* t, unsigned char n)
{
    unsigned char k;

    hub_map (0);
    if (n && (t = strchr (t, '\n')) != 0) {
        ++t;
    }
    for (k = 0; t && *t && *t != '\n' && k < sizeof dword - 1; ++k) {
        dword[k] = *t++;
    }
    dword[k] = 0;
}

/* Its weapon on the floor (dword's kind: none of that name, nothing), beside it or where it stands (a d10: 1-6 the
** square that way, if it's open floor), kept as its (bd's arm, dropped), told (E_BEAT's WB_DROP, its kind, its b the
** body's: the denizens' ring, after the blow's words), the world to look about again */
void s_den_drop (void)
{
    static const unsigned char way[6] = { H_N, H_NE, H_SE, H_S, H_SW, H_NW };
    unsigned char k, d;
    unsigned x = bd.x, y = bd.y;
    unsigned char* q;

    hub_map (CAT_BANK);
    for (k = 1; k < NKINDS && strcmp (CATALOGUE[k].name, dword); ++k) {
    }
    hub_map (0);
    if (k >= NKINDS) {
        return;
    }
    if ((d = dice (1, 10)) <= 6) {
        d = way[d - 1];
        q = at (bd.sn, x + head_x[d], y + head_y[d]);
        if (q && (TILE (*q) == T_FLOOR || TILE (*q) == T_CORR || TILE (*q) == T_STEPS)) {
            x += head_x[d];
            y += head_y[d];
        }
    }
    if ((bd.dropped = lay_body (k, bd.sn, x, y)) != 0) {
        bd.arm = k;
        limb_news (WB_DROP | (unsigned) k << 8, bk + 1);
        den_saw = 1;
    }
}

/* Its weapon let fall still lying in its reach (its square or beside it): 1, to take it up (its turn); gone (taken,
** moved off), forgotten */
unsigned char s_den_reach (void)
{
    struct item* p = item_at (bd.dropped);

    if (p->kind == bd.arm && p->where == IW_SQUARE && p->sn == bd.sn && (unsigned) (p->x - bd.x + 1) <= 2 &&
        (unsigned) (p->y - bd.y + 1) <= 2) {
        hub_map (0);
        return 1;
    }
    hub_map (0);
    bd.arm = 0;
    return 0;
}

/* Its weapon taken up again (its turn: the referee's O_REARM): gone from the floor (its record free), told (WB_PICKS,
** its kind), the world to look about again */
void s_den_took (void)
{
    item_at (bd.dropped)->kind = 0;
    hub_map (0);
    limb_news (WB_PICKS | (unsigned) bd.arm << 8, bk + 1);
    bd.arm = 0;
    den_saw = 1;
}

/* Its legs at a move's start (a step, a dash): stuck (its move C_STUCK: no leg to stand on, nor arms enough), no move;
** prone, up first, its round (the book's Rise: told, WB_RISEN), no move yet, unless it crawls (C_CRAWLS: its legs
** won't bear it as they should, it goes on on its belly).  1 if there's no move (the caller's moment more) */
unsigned char s_den_legs (void)
{
    if (bd.move & 4) {
        return 1;
    }
    if (bd.pos == POS_PRONE && !(bd.move & 2)) {
        hub_map (0);
        bd.pos = POS_STANDING;
        bd.ready += HUB->round;
        limb_news (WB_RISEN, bk + 1);
        return 1;
    }
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
