/*
** wlimbs.c - (dq0w) the hands and limbs (slice 4.1b; the user's, 2026-10-21: "if you have a limb injury, it should
** affect that limbs usefulness ... the HM fumble rules should be followed for hand and arm wounds and dropping anything
** held in the hand on that side"): what a hand lets go of, fumbled (the referee's X_DROP: the book's, Skills 21, onto a
** neighbouring square or its own), told; what's to hand told the referee after (RS_EQUIP); a harm's effects (a fall's,
** a trap's: what it dropped, a leg's stumble laying it prone).
**
** And the denizens' limbs (part 51's): its weapon's word, its legs at a move's start (stuck, prone); and their things
** (part 60's, the outfits' gear, 2026-10-27): made as a lair's placed, told the referee, a weapon drawn before a blow,
** let fall (the referee's X_DROP, a creature's: on the floor, seen, taken up again in its reach, its turn), on its body
** as it dies.
**
** All of it swapped parts 49's, 51's and 60's (../dq0/swap.inc: dq0w.cfg's SW49, SW51, SW60), called through their
** stubs (bank.s's).
*/

#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "wbody.h"
#include "trace.h"
#include "outfit.h"

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
        hub_map (KIND_BANK);                     /* (Its key after its mark, its quality after it) */
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
        if ((d = 1 + random (10)) <= 6) {       /* (Beside it, that way, if it's open floor: dice (1, 10)'s roll, without
                                                **   dice.c in the main RAM, 2026-10-27) */
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
** C_PRONE), the world's body's position (the hub's pc_pos); a fall, its bearings shaken (part 52's, slice 4.2's) */
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
    bearings_off (0);                           /* (A fall's, its bearings shaken: part 52's, slice 4.2's, the
                                                **   harm's request read before what's let go of is told the referee) */
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

/* ==== Part 60: the denizens' things (the outfits' gear, 2026-10-27), and a weapon let fall and taken up again (part
** 51's once: its room) */
#pragma code-name (push, "SW60CODE")
#pragma rodata-name (push, "SW60RODATA")

/* ---- The denizens' things (the outfits' gear, 2026-10-27: DENIZENS.md's gear; items.h's IW_DEN): made as a lair's
** placed (I_KIT's, the world's draw), filed at its lair's home, the actor's while it is one (its did: the hub's), told
** the referee as it comes near and as they change (RS_EQUIP's, the actor's), a weapon drawn from its belt before it
** strikes, on its body as it dies.  Their data the image's (each of these calls no part after it sets it: zeros each
** time it's copied in) */
#pragma bss-name (push, "SW60RODATA")


static struct kind dk;                          /* (A thing's kind, copied out) */
static unsigned ki;                             /* (Its thing at hand, kit_next's) */

/* Kind k's record copied out (dk) */
static void __fastcall__ dkind (unsigned char k)
{
    hub_map (KIND_BANK);
    memcpy (&dk, CATALOGUE + (k < NKINDS ? k : 0), sizeof dk);
}

/* Body bd's next thing of its own after i (worn, held, hung from it: IW_DEN, its number its did), ip its record: 0 none
** (none of its own: a beast, dq0 -K's) */
static unsigned __fastcall__ kit_next (unsigned i)
{
    if (bd.did) {
        while ((i = items (i, IN_DEN, 0)) != 0) {
            if ((ip->owner | ip->hide << 8) == bd.did) {
                return i;
            }
        }
    }
    return 0;
}

/* A weapon to strike with (dk its kind's: a weapon's craft, held in a hand, not worn: not a shield, not a bow) */
static unsigned char weapon (void)
{
    return dk.craft == AP_WEAPON && (dk.flags & (K_HAND | K_WEAR)) == K_HAND;
}

/* The hand a thing of its own is held in (dk its kind's): two-handed both, a shield or a light the left, else the
** right (items.h's IW_DEN) */
static unsigned char hand (void)
{
    return dk.flags & K_TWO ? IH_BOTH : (dk.flags & K_LIGHT) || (dk.craft == AP_WEAPON && (dk.flags & K_WEAR)) ?
        IH_LEFT : IH_RIGHT;
}

/* Thing i (ip) made body bd's own again: filed at its lair's home (IW_DEN), held if held is 1 */
static void __fastcall__ filed (unsigned i, unsigned char held)
{
    struct item* p = item_at (i);

    p->where = IW_DEN;
    p->sn = bd.hsn;
    p->x = bd.hx;
    p->y = bd.hy;
    p->owner = bd.did;
    p->hide = bd.did >> 8;
    if (held) {
        p->flags |= I_HELD;
    }
    hub_map (0);
}

/* A denizen's things made (I_KIT's, the denizens' ring's, part 14's: its lair placed): drawn and made (part 57's), but
** for dq0 -K (the tests': none of theirs) */
/* Kind k a role in the outfits' index (its entry "#" and its number, outfits.js's: the index read in, the character's
** outfit's, as the game began): 1, or 0 (none, or no index: a kind with no things of its own, part 57 not called) */
static unsigned char __fastcall__ has_role (unsigned char k)
{
    unsigned h = ('#' * 31 + k), n;
    unsigned char* q;

    hub_map (KIND_BANK);
    if ((OF_INDEX[0] | OF_INDEX[1] << 8) != 0x464F) {
        return 0;
    }
    for (n = OF_INDEX[3], q = OF_INDEX + 4; n; --n, q += 5) {
        if ((q[0] | q[1] << 8) == h) {
            return 1;
        }
    }
    return 0;
}

void __fastcall__ s_den_other (const struct msg* m)
{
    hub_map (0);
    if (!(HUB->gear & GE_OLD) && has_role (m->x[1])) {
        ost.did = m->t;
        ost.hx = m->w1;
        ost.hy = m->w2;
        ost.hsn = m->x[2];
        it_need (ost.hsn, (ost.hx - STRATUM (ost.hsn).x0) >> CW_BITS, (ost.hy - STRATUM (ost.hsn).y0) >> CW_BITS);
        den_kit (m->x[1]);
    }
}

/* What body bd holds and wears told the referee (RS_EQUIP 2, the actor's: the keys of its things held and worn, each
** after its mark, < the left hand, > the right, * both, + worn, with its quality after it, as the character's), if it
** has things of its own (else the bestiary's, as before) */
void s_den_equip (void)
{
    static const char hmark[4] = " <>*";
    static char wb[12];
    static unsigned char f, n, h, k;
    struct req* r;

    hub_map (0);
    HUB->slot[SLOT_WORLD].text[0] = 0;
    if (!(ki = kit_next (0))) {               /* (Its first: ip its record, its bank in the window) */
        return;
    }
    for (; ki; ki = kit_next (ki)) {
        f = ip->flags;
        n = ip->quality;
        dkind (ip->kind);
        if (!dk.tool[0] || !(f & (I_HELD | I_WORN))) {
            continue;
        }
        h = hand ();
        memcpy (wb + 1, dk.tool, sizeof dk.tool);
        wb[0] = f & I_HELD ? hmark[h] : '+';
        k = strlen (wb);
        wb[k] = '0' + n;
        wb[k + 1] = 0;
        hub_map (0);
        r = &HUB->slot[SLOT_WORLD];
        if (strlen (r->text) < REQ_TEXT - sizeof wb) {
            strcat (r->text, wb);
        }
    }
    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_EQUIP;
    r->actor = bk + 1;
    r->a = 2;
    r->w1 = r->w2 = 0;
    hub_ask (SLOT_WORLD)->state = REQ_FREE;
}

/* Body bd come into the world's keeping (I_ENTER's, m: part 14's, its body's from its record): on its feet, walking,
** its legs the referee's; its things in the store (its lair's home's chunk read in, if it's out: an actor far from
** home), the referee told them */
void __fastcall__ s_den_enter (const struct msg* m)
{
    bd.state = 1;
    bd.sn = m->x[2];
    bd.x = m->w1;
    bd.y = m->w2;
    bd.ready = m->t;
    bd.step = m->a;
    bd.pos = POS_STANDING;
    bd.gait = GA_WALK;
    bd.move = m->x[3];
    bd.arm = 0;
    bleeds = m->b ? bleeds | 1 << bk : bleeds & ~(1 << bk);
    hub_map (0);
    if (bd.did && !(HUB->gear & GE_OLD)) {      /* (dq0 -K's: none of their own) */
        it_need (bd.hsn, (bd.hx - STRATUM (bd.hsn).x0) >> CW_BITS, (bd.hy - STRATUM (bd.hsn).y0) >> CW_BITS);
        s_den_equip ();
    }
}

/* Body bd about to strike, its hands empty of a weapon (a denizen at its ease keeps its sheathed, or hung from its
** belt, DENIZENS.md's gear): the best on its belt (in a carrier hung there, or hung there itself: the worthiest) drawn
** into its main hand, its round's (the book's grope; the caller's time), told (WB_DRAWS, its kind), the referee told:
** 1; else 0 (a weapon in hand, its own let fall, none on its belt, no things of its own) */
unsigned char s_den_draw (void)
{
    static unsigned i, best, top;
    static struct item* p;
    static unsigned char k;

    if ((bd.arm & 0x80) || !kit_next (0)) {    /* (Its own let fall: it takes that up again, its turn: RS_REARM) */
        return 0;
    }
    for (ki = 0; (ki = kit_next (ki)) != 0; ) {
        if (ip->flags & I_HELD) {
            dkind (ip->kind);
            if (weapon ()) {
                return 0;
            }
        }
    }
    for (best = top = 0, i = 0; (i = items (i, IN_ITEM, 0)) != 0; ) {
        k = ip->kind;
        p = item_at (ip->x);                    /* (What it's in: the belt, or a carrier on it) */
        if (p->where == IW_ITEM) {
            p = item_at (p->x);
        }
        if (p->where != IW_DEN || (p->owner | p->hide << 8) != bd.did || !(p->flags & I_WORN)) {
            continue;
        }
        dkind (k);
        if (weapon () && dk.value >= top) {
            top = dk.value;
            best = i;
        }
    }
    if (!best) {
        return 0;
    }
    k = item_at (best)->kind;
    filed (best, 1);
    limb_news (WB_DRAWS | (unsigned) k << 8, bk + 1);
    s_den_equip ();
    return 1;
}

/* Body bd dead (wfight.c's died: its remains, body, put down where it fell, or 0 none): its things on it (worn, hung
** from it: on its body, what's in them with them), what it held let fall where it lies; its own no longer (the
** body's, the floor's: anyone's to take) */
void __fastcall__ s_den_strip (unsigned body)
{
    static unsigned char f;

    for (ki = 0; (ki = kit_next (ki)) != 0; ) {
        f = ip->flags;
        ip->owner = ip->hide = 0;
        ip->flags = f & ~(I_HELD | I_WORN);
        if ((f & I_HELD) || !body) {
            ip->where = IW_SQUARE;
            ip->sn = bd.sn;
            ip->x = bd.x;
            ip->y = bd.y;
        } else {
            ip->where = IW_ITEM;
            ip->x = body;
        }
    }
    hub_map (0);
}

#pragma bss-name (pop)

/* Its weapon on the floor (dword's kind: none of that name, nothing), beside it or where it stands (a d10: 1-6 the
** square that way, if it's open floor), kept as its (bd's arm, dropped), told (E_BEAT's WB_DROP, its kind, its b the
** body's: the denizens' ring, after the blow's words), the world to look about again.  A weapon of its own in its hand
** (the outfits' gear) that thing (arm | 0x80), the referee told it's gone from its hand */
void s_den_drop (void)
{
    static const unsigned char way[6] = { H_N, H_NE, H_SE, H_S, H_SW, H_NW };
    unsigned char k, d;
    unsigned x = bd.x, y = bd.y, i;
    unsigned char* q;

    for (i = 0; (i = kit_next (i)) != 0; ) {   /* (Its own weapon in hand) */
        if (ip->flags & I_HELD) {
            dkind (ip->kind);
            if (weapon ()) {
                break;
            }
        }
    }
    hub_map (KIND_BANK);
    for (k = 1; k < NKINDS && strcmp (CATALOGUE[k].name, dword); ++k) {
    }
    hub_map (0);
    if (i) {
        k = item_at (i)->kind;
        hub_map (0);
    } else if (k >= NKINDS) {
        return;
    }
    if ((d = 1 + random (10)) <= 6) {           /* (dice (1, 10)'s) */
        d = way[d - 1];
        q = at (bd.sn, x + head_x[d], y + head_y[d]);
        if (q && (TILE (*q) == T_FLOOR || TILE (*q) == T_CORR || TILE (*q) == T_STEPS)) {
            x += head_x[d];
            y += head_y[d];
        }
    }
    if (i) {                                    /* (Its own: that thing on the floor, its chunk's things in the store) */
        it_need (bd.sn, (x - STRATUM (bd.sn).x0) >> CW_BITS, (y - STRATUM (bd.sn).y0) >> CW_BITS);
        ip = item_at (i);
        ip->where = IW_SQUARE;
        ip->sn = bd.sn;
        ip->x = x;
        ip->y = y;
        ip->owner = ip->hide = 0;
        ip->flags &= ~I_HELD;
        hub_map (0);
        bd.dropped = i;
        bd.arm = k | 0x80;
    } else if ((bd.dropped = lay_body (k, bd.sn, x, y)) != 0) {
        bd.arm = k;
    } else {
        return;
    }
    limb_news (WB_DROP | (unsigned) k << 8, bk + 1);
    den_saw = 1;
    if (i) {
        s_den_equip ();
    }
}

/* Its weapon let fall still lying in its reach (its square or beside it): 1, to take it up (its turn); gone (taken,
** moved off), forgotten */
unsigned char s_den_reach (void)
{
    struct item* p = item_at (bd.dropped);

    if (p->kind == (bd.arm & 0x7F) && p->where == IW_SQUARE && p->sn == bd.sn && (unsigned) (p->x - bd.x + 1) <= 2 &&
        (unsigned) (p->y - bd.y + 1) <= 2) {
        hub_map (0);
        return 1;
    }
    hub_map (0);
    bd.arm = 0;
    return 0;
}

/* Its weapon taken up again (its turn: the referee's O_REARM): gone from the floor (its record free; its own, in its
** hand again, the referee told), told (WB_PICKS, its kind), the world to look about again */
void s_den_took (void)
{
    unsigned char k = bd.arm;

    if (k & 0x80) {
        filed (bd.dropped, 1);
    } else {
        item_at (bd.dropped)->kind = 0;
        hub_map (0);
    }
    limb_news (WB_PICKS | (unsigned) (k & 0x7F) << 8, bk + 1);
    bd.arm = 0;
    den_saw = 1;
    if (k & 0x80) {
        s_den_equip ();
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
