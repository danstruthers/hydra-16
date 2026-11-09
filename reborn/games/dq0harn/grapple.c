/*
** grapple.c - (overlay 9, the referee's: called through bank.s's stub) grappling, by HarnMaster's (the book's unarmed
** combat, Combat 8-10: a grapple an unarmed attack, its hold a grip the held one can't fight free of but by its own;
** HOUSE.md's grappling, 2026-11-09, the user's "full HarnMaster grappling": as built, and where it's simpler,
** DECISIONS.md's).  RS_ATTACK's RS_GRAPPLE (fight.c's, its sides made: A the attacker, D the defender), its move the
** request's a (GM_*), the hold as it stands its c (GH_PIN, the rounds it's been choked), each an opposed test, the
** attacker's against the defender's, the attacker winning only by a success better than the defender's level (the
** defender's ties):
**   GM_HOLD   a hold taken: the attacker's Unarmed against the defender's Unarmed or Dodge, the better (O_HOLD, or
**             O_SLIP: it twisted free, or dodged, D_DODGE); a prone defender four steps the easier, as a blow is
**   GM_THROW  from a hold: its Unarmed against the defender's Unarmed or Strength (five times), the better: thrown down
**             (O_THROWN: prone, and a fall's hurt, two dice of blunt impact at a place the middle's strike table
**             gives, its armour off it: its words the third line), the hold let go as it's thrown (the world's)
**   GM_PIN    the same: pinned (O_PINNED: prone, held so)
**   GM_CHOKE  the same, a pinned one two steps the easier: choked (O_CHOKED), and its Endurance tested (three times
**             it), ten less each round it's been choked before: failed, senseless (knocked_out's: the book's shock
**             rolls till it comes round, a minute at least); bones that move have no breath to stop (O_RESISTS)
**   GM_BREAK  the held one's try (the world's: its turn, anything but a wait or a blow, or anything at all pinned):
**             its Unarmed or Strength, the better, against the holder's Unarmed; pinned, four steps the worse (O_FREE,
**             or O_STRUGGLES)
** each a size's difference two steps (the bigger the better); one that can't resist (senseless, in shock, asleep,
** unaware) none, the attacker's success enough.  A creature's Unarmed its own attack's mastery (its sheet's: the
** bestiary has no Unarmed), its injuries' penalty off it.  The character's Unarmed used, for its development roll.
*/

#pragma code-name ("OV9CODE")
#pragma rodata-name ("OV9RODATA")

#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* Side s's Unarmed: a creature's its own attack's (beast_side's, its penalty off it); the character's skill, its
** opening mastery if it hasn't opened it */
static int __fastcall__ una (const struct side* s)
{
    if (s->b) {
        return s->aml;
    }
    return pc.ml[HARN_UNA] ? harn_eml (&pc, HARN_UNA) : harn_get_oml (&pc, HARN_UNA) - 5 * harn_get_pp (&pc);
}

/* Side s's Unarmed or Strength (five times, its penalty off it), the better */
static int __fastcall__ grit (const struct side* s)
{
    int u = una (s), t = 5 * ((int) s->c->att[HARN_STR] - (int) harn_get_pp (s->c));

    return t > u ? t : u;
}

/* An opposed test: the attacker's at a, the defender's at d (d < 0: it can't resist): 1 if the attacker's succeeds,
** and better than the defender's */
static unsigned char __fastcall__ beats (int a, int d)
{
    unsigned char l = harn_test (a);

    return HARN_SUCCESS (l) && (d < 0 || l > harn_test (d));
}

#define CHOKED_T        600                     /* (Choked senseless: a minute at least, in tenths) */

void o_grip (void)
{
    unsigned char m = RS_ATTACK_MOVE (*R), c = RS_ATTACK_HOLD (*R), out;
    unsigned long* o;
    int a, d, mod = STEP_PCT * (signed char) RS_ATTACK_STEPS (*R) + 2 * STEP_PCT * ((int) A.size - (int) D.size);

    RS_ATTACK_WHOSE (*R) = 0;
    RS_ATTACK_AF (*R) = 0;
    RS_ATTACK_DEFENCE (*R) = D_NONE;
    RS_ATTACK_OUT (*R) = 0;
    if (A.c->state != HARN_CONSCIOUS || (!A.who && pc_out > tnow)) {
        RS_ATTACK_OUTCOME (*R) = 0xFF;          /* (It can't) */
        return;
    }
    if (!A.who) {
        A.sk = HARN_UNA;                        /* (Its skill used: fight.c's) */
    }
    a = una (&A);
    d = D.c->state != HARN_CONSCIOUS || (!D.who && pc_out > tnow) || (prones & RS_ASLEEP) ? -1 : una (&D);
    if (m == GM_BREAK) {
        if (c & GH_PIN) {
            mod -= 4 * STEP_PCT;
        }
        out = beats (grit (&A) + mod, d) ? O_FREE : O_STRUGGLES;
    } else if (m == GM_HOLD) {
        if (prones & RS_PRONE_D) {
            mod += 4 * STEP_PCT;
        }
        if (d >= 0 && D.dodge > d) {
            d = D.dodge;
            RS_ATTACK_DEFENCE (*R) = D_DODGE;
        }
        out = beats (a + mod, d) ? O_HOLD : O_SLIP;
    } else {
        if (d >= 0) {
            d = grit (&D);
        }
        if (m == GM_CHOKE && (c & GH_PIN)) {
            mod += 2 * STEP_PCT;
        }
        out = O_RESISTS;
        if (!(m == GM_CHOKE && D.b && (D.b->flags & BF_MINDLESS)) && beats (a + mod, d)) {
            if (m == GM_CHOKE) {
                out = O_CHOKED;
                if (!HARN_SUCCESS (harn_test (3 * (int) harn_end (D.c) - 10 * (int) (c & ~GH_PIN) - 5 * (int) harn_get_pp (D.c)))) {
                    if ((RS_ATTACK_OUT (*R) = knocked_out (D.c, o = D.who ? &bs.out : &pc_out, &RS_ATTACK_AF (*R))) < CHOKED_T) {
                        RS_ATTACK_OUT (*R) = CHOKED_T;  /* (A minute at least, the house's) */
                        *o = tnow + CHOKED_T;
                    }
                    effect (D.who, X_COND_ON, C_UNCONSCIOUS, 0, 0, 0);
                }
            } else {
                out = m == GM_THROW ? O_THROWN : O_PINNED;
                effect (D.who, X_COND_ON, C_PRONE, 0, 0, 0);
                if (m == GM_THROW) {
                    A.imp[0] = 0;               /* (The fall's: blunt, two dice, the sizes' difference a point a die) */
                    A.imp[1] = A.imp[2] = -1;
                    strike (&A, &D, 2, HARN_MID, 0xFF, line[0], AF_STOPPED);
                }
            }
        }
    }
    RS_ATTACK_OUTCOME (*R) = out;
}
