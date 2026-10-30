/*
** missile.c - (overlay 7, heal.c's too, the referee's: called through bank.s's stubs) a missile's attack, HarnMaster's (Combat 15-17:
** the missile sequence, slice 4.7's): an RS_ATTACK with RS_MISSILE (rs.h's), its sides made by fight.c's o_fight (the
** character's, a creature's), its strike and its words fight.c's (through their stubs: strike, effect).
**
** The missile (the request's c, the catalogue's kind: what it's shot with, or the thing thrown) by the book's Missile
** Data Table (Combat 16: each weapon's impact at its four ranges, its skill; the range's band the world's, b: short,
** medium, long, extreme, the attacker's EML 0, 20, 40 or 80 less); a thing thrown that isn't made to be thrown by
** Throwing, its impact its own as a weapon's (the gear's, a light thing's the stick's, a hefty one's the club's: the
** book's), all of it at the short and medium ranges, half at the long and extreme.  Its aspect the book's (Combat 17):
** an arrow's and a bolt's point, a stone's blunt, a thing thrown by the d6 (the taburi's, the shorkana's, the spear's,
** any other's: of the aspects it has).  Arrows, bolts and stones are high-velocity, the rest low.
**
** The attacker's EML the less for the range, an aim high or low (ten), the target's size (Combat 16's table: a man's
** none, a giant rat's fifteen less, a bear's ten more; one prone a two-foot target), and the medium range under a
** ceiling (the book's restricted missile ranges: underground the world allows no long or extreme shot, the medium
** fifteen less, the house's middle of the book's ten to twenty: HOUSE.md's, for review); the circumstances' steps (the
** world's: through a narrow opening).  The defence (Combat 15): one unaware of it (the world's MS_UNAWARE: a denizen
** that hasn't sensed the shooter; the character not seeing it), senseless or in shock ignores it; else a block with a
** shield (at its full block), a low-velocity one's with a weapon at half (a bare arm's at half its Unarmed: struck by it,
** the book's H2), none against a high-velocity one; a dodge (a low-velocity one's at full, a high-velocity one's at
** half); the character's by its stance (D_BLOCK, D_DODGE; D_NONE none; else the better), a creature's the better.  The
** missile attack table's result (harn.h's harn_missile_result): wild (the attacker's fumble roll: failed, what it shot
** with let fall), a miss, a block (on the shield or the weapon, no harm; a bare arm struck), a strike of n dice (the
** missile's impact added, the armour at the location off it: fight.c's strike).  Behind cover (MS_COVER: a slit's or a
** window's stone up to its head, the world's), a strike below the neck strikes the stone (Combat 17's cover).  A
** stray's (MS_STRAY, the world's: a miss that struck one beside its mark, the book's 20%) a strike of one die, no test.
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")

#include <stddef.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* The missile weapons (Combat 16's Missile Data Table): a kind, its skill, its impact at each range, its aspect (MA_*:
** a high-velocity one's | MA_HV) */
#define MA_POINT        0                       /* (Its aspect: the point's, an arrow's or a bolt's; */
#define MA_BLUNT        1                       /*   the blunt, a stone's; */
#define MA_TABURI       2                       /*   by the d6, Combat 17's Random Aspect: a taburi's, */
#define MA_SHORKANA     3                       /*   a shorkana's, */
#define MA_SPEAR        4                       /*   a javelin's or a spear's, */
#define MA_OTHER        5                       /*   any other thing thrown) */
#define MA_HV           0x80
static const struct mw {
    unsigned char   kind, sk;
    unsigned char   imp[4];
    unsigned char   asp;
} mws[] = {
    { KI_SHORTBOW, HARN_BOW, { 6, 5, 4, 3 }, MA_POINT | MA_HV },
    { KI_LONGBOW, HARN_BOW, { 8, 7, 6, 5 }, MA_POINT | MA_HV },
    { KI_HARTBOW, HARN_BOW, { 9, 8, 7, 6 }, MA_POINT | MA_HV },
    { KI_CROSSBOW, HARN_BOW, { 8, 7, 6, 5 }, MA_POINT | MA_HV },
    { KI_SLING, HARN_SLG, { 4, 3, 2, 2 }, MA_BLUNT | MA_HV },
    { KI_TABURI, HARN_DAG, { 4, 3, 2, 2 }, MA_TABURI },
    { KI_SHORKANA, HARN_AXE, { 5, 4, 3, 3 }, MA_SHORKANA },
    { KI_SPEAR, HARN_SPR, { 8, 7, 6, 5 }, MA_SPEAR },
};
#define NMW             (sizeof mws / sizeof mws[0])

/* Combat 17's Random Aspect, by the d6: the rolls that strike blunt, then those that strike with the edge (the rest
** the point), by MA_TABURI ... MA_OTHER */
static const unsigned char rblunt[4] = { 2, 2, 1, 2 };
static const unsigned char redge[4] = { 0, 4, 0, 2 };

/* Combat 16's Target Size Modifiers, by the bestiary's size (1 tiny ... 4 a man ... 6 very large) */
static const signed char tsize[7] = { 0, -20, -15, -10, 0, 10, 15 };
static const signed char band_ml[4] = { 0, -20, -40, -80 };

/* The missile attack table (the book's Combat tables, harn.h's harn_missile_result: here, the main RAM's room): by
** the defence (a block, a dodge), the attacker's level (CF, MF, MS, CS), the defender's; the Ignore's by the attacker's */
#define WLD_            HARN_R_WILD
#define MS__            HARN_R_MISS
#define MH_(n)          (HARN_R_AH | (n))
static const unsigned char mtab[2][4][4] = {
    { { WLD_, WLD_, WLD_, WLD_ }, { MS__, MS__, MS__, MS__ }, { MH_ (2), MH_ (1), HARN_R_BLOCK, HARN_R_BLOCK },
      { MH_ (3), MH_ (2), MH_ (1), HARN_R_BLOCK } },
    { { WLD_, WLD_, WLD_, WLD_ }, { MS__, MS__, MS__, MS__ }, { MH_ (2), MH_ (1), MS__, MS__ },
      { MH_ (3), MH_ (2), MH_ (1), MS__ } },
};
static const unsigned char mign[4] = { WLD_, MS__, MH_ (2), MH_ (3) };

static int __fastcall__ eml (unsigned char sk)
{
    return pc.ml[sk] ? harn_eml (&pc, sk) : harn_get_oml (&pc, sk) - 5 * harn_get_pp (&pc);
}

unsigned o_shot (void)
{
    const struct mw* w;                         /* (On the C stack: the main RAM's room) */
    unsigned char band, f, z, def, al, dl, res, n, hv, asp, k, loc, sz;
    int mod, blk = 0, dge = 0, imp;
    unsigned out = 0;

    band = R->b & 3;
    f = R->w2 >> 8;                             /* (Its MS_*, rs.h's, their high byte: MS_ >> 8 below) */
    z = R->a == Z_HIGH ? HARN_HIGH : R->a == Z_LOW ? HARN_LOW : HARN_MID;
    mod = STEP_PCT * (signed char) R->steps;
    k = R->c;
    R->a = R->c = 0;
    if (A.c->state != HARN_CONSCIOUS || (!A.who && pc_out > tnow)) {
        R->result = 0xFF;                       /* (It can't) */
        return 0;
    }
    if (prones & RS_PRONE_A) {                  /* (Fallen: it gets up first, its turn spent, as a blow's) */
        R->a = !G_OK (rise_of (A.c));
        R->result = O_RISE;
        return 0;
    }
    for (w = mws; w < mws + NMW && w->kind != k; ++w) {
    }
    kind_in (k);
    strcpy (A.word, wk.name);                   /* (Its words its kind's name: "shortbow", "dagger") */
    if (w < mws + NMW) {
        imp = w->imp[band];
        asp = w->asp & 0x7F;
        hv = w->asp & MA_HV;
        if (!A.who) {
            A.aml = eml (w->sk);
        }
        if (hv && band == 1) {
            mod -= 15;                          /* (The medium range under a ceiling: the book's restricted ranges,
                                                **   the house's fifteen) */
        }
    } else {                                    /* (A thing thrown: Throwing, its impact its own as a weapon's, half
                                                **   at the long and extreme ranges) */
        asp = MA_OTHER;
        hv = 0;
        imp = -1;
        if (!A.who) {
            A.aml = eml (HARN_THR);
        }
    }
    if (asp >= MA_TABURI) {                     /* (Its aspect by the d6: of those it has, else its best) */
        n = (unsigned char) dice (1, 6);
        asp = n <= rblunt[asp - MA_TABURI] ? HARN_BLUNT : n <= rblunt[asp - MA_TABURI] + redge[asp - MA_TABURI] ?
            HARN_EDGE : HARN_POINT;
        if (imp < 0) {
            if (A.imp[asp] < 0) {
                for (asp = HARN_BLUNT, n = 1; n < 3; ++n) {
                    asp = A.imp[n] > A.imp[asp] ? n : asp;
                }
            }
            imp = A.imp[asp] < 0 ? 0 : band < 2 ? A.imp[asp] : (A.imp[asp] + 1) / 2;
        }
    } else {
        asp = asp == MA_BLUNT ? HARN_BLUNT : HARN_POINT;
    }
    A.imp[0] = A.imp[1] = A.imp[2] = -1;
    A.imp[asp] = imp;
    A.part = 0xFF;
    sz = D.size < 7 ? D.size : 6;
    A.size = sz;                                /* (A missile's strike none the more for its shooter's size) */
    if ((prones & RS_PRONE_D) && sz > 2) {
        sz = 2;                                 /* (Prone: a two-foot target) */
    }
    mod += band_ml[band] + tsize[sz] - (z != HARN_MID ? 10 : 0);
    def = HARN_IGNORE;
    if (!(f & ((MS_UNAWARE | MS_STRAY) >> 8)) && D.c->state == HARN_CONSCIOUS && (D.who || pc_out <= tnow)) {
        dge = hv ? D.dodge / 2 : D.dodge;       /* (Its defences: Combat 15's) */
        if (D.who) {
            blk = D.dslot ? D.dml : hv || D.part != 0xFF ? 0 : D.aml / 2;
            if (D.dslot) {
                strcpy (D.word, dshw);
            }
        } else {
            blk = pc_defence (&D, PD_SHIELD, 0);
            D.bare = D.bare && !blk;
            if (!blk && !hv) {
                blk = D.dml / 2;                /* (A low-velocity one's: half its weapon's block, or its bare arm's) */
            }
        }
        if (blk > 0 && (D.who ? blk >= dge : stance == D_BLOCK || (stance != D_DODGE && blk >= dge))) {
            def = HARN_BLOCK;
        } else if (D.who || stance != D_NONE) {
            def = HARN_DODGE;
        }
        if (!D.who && stance == D_NONE) {
            def = HARN_IGNORE;
        }
    }
    if (f & (MS_STRAY >> 8)) {
        res = HARN_R_AH | 1;                    /* (A stray's: a strike of one die, no test) */
        al = HARN_MS;
    } else {
        al = harn_test (A.aml + mod);
        dl = def == HARN_IGNORE ? 0 : harn_test (def == HARN_BLOCK ? blk : dge);
        res = def == HARN_IGNORE ? mign[al & 3] : mtab[def == HARN_DODGE][al & 3][dl & 3];
    }
    n = HARN_R_DICE (res);
    switch (HARN_R_KIND (res)) {
    case HARN_R_WILD:                           /* (Gone wild: the attacker's fumble roll, what it shot with let fall;
                                                **   a thing thrown gone already) */
        R->result = O_FUMBLE;
        R->a = 1;
        if (w < mws + NMW && w->sk != HARN_DAG && w->sk != HARN_AXE && w->sk != HARN_SPR && harn_fumble_roll (A.c, 0)) {
            out = SHOT_DROP;
        }
        break;
    case HARN_R_BLOCK:
        R->result = O_BLOCKED;
        if (D.who && D.dslot) {
            R->c |= AF_SHIELD;
        } else if (!D.who && D.bare) {          /* (A bare arm's: struck, the book's H2) */
            out = SHOT_HIT (2, 0, asp, HARN_FOREARM);
        }
        break;
    case HARN_R_AH:
        R->result = O_HIT;
        loc = harn_strike_location (z);
        if ((f & (MS_COVER >> 8)) && HARN_LOC (loc) > HARN_NECK) {
            strcpy (line[0], "the stone");      /* (Behind cover: "..., but the stone turns the blow") */
            R->c |= AF_STOPPED;
            break;
        }
        out = SHOT_HIT (n, z, asp, loc);
        break;
    default:
        R->result = def == HARN_DODGE && al >= HARN_MS ? O_DODGED : O_MISS;
        break;
    }
    R->b = def == HARN_BLOCK ? D_BLOCK : def == HARN_DODGE ? D_DODGE : D_NONE;
    return out;
}
