/*
** limbs.c - (overlay 5, the referee's: called through bank.s's stubs; beside part 2's image) a body's health, limbs,
** conditions and getting up, any ruleset's words for them (the character's, or a creature's: its sheet's, slice
** 4.1b's): gear.c's once, moved here 2026-10-27 (overlay 3's room for the denizens' gear: the outfits').
*/

#pragma code-name ("OV5CODE")
#pragma rodata-name ("OV5RODATA")

#include <stddef.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* Its health in words any ruleset can say (the game's: by its injuries' levels against its Endurance; down, in
** shock or senseless): the main RAM's room, slice 4.1b's */
unsigned char __fastcall__ o_health_of (const harn_char* h)
{
    unsigned char ip = harn_get_ip (h), e = harn_end (h);

    return h->state == HARN_DEAD ? H_DEAD : h->state != HARN_CONSCIOUS ? H_DOWN : !ip ? H_FINE : 3 * ip < e ? H_HURT :
        3 * ip < 2 * e ? H_BADLY : H_CRITICAL;
}

/* Its limbs (rs.h's LB_*, slice 4.1b's), as its sheet has them (the character's, or a creature's): its main hand (the
** medical traits', Character 9: left-handed, ambidextrous; an arm missing or deformed, its off arm the one, no
** two-handed use), each limb made useless (a grievous wound, till it heals to M1: Combat 14); and each one's hurt, its
** injuries' levels (lhurt: the left arm's, the right's, the left leg's, the right's; a leg missing or deformed, the
** medical trait's, five levels, a limp at half its move) */
unsigned char __fastcall__ o_limbs_of (const harn_char* h)
{
    unsigned char k, l = 0, j, v = 0;
    const harn_injury* p;

    memset (lhurt, 0, sizeof lhurt);
    for (k = 0; h == &pc && k < HARN_MEDMAX; ++k) {
        j = pc.med[k];
        l |= j == HARN_LEFTHANDED ? LB_LEFT : j == HARN_AMBIDEXTROUS ? LB_AMBI : 0;
        v |= j == HARN_MED_ARM;
        if (j == HARN_MED_LEG) {
            lhurt[2] = 5;                       /* (A leg missing or deformed, Character 9: half its move, a limp of five
                                                **   levels, the house's; the left, its choice) */
        }
    }
    if (h == &pc && v) {
        l |= l & LB_LEFT ? LB_ARM_R : LB_ARM_L;
    }
    for (k = 0; k < h->ninj; ++k) {
        p = &h->inj[k];
        if (p->sev >= HARN_SERIOUS) {
            l |= LB_HURT;                       /* (Seriously or grievously wounded: getting up, the book's Rise, a
                                                **   test) */
        }
        if ((j = harn_is_limb (p->loc)) != 0) {
            j = (j - 1) * 2 + ((p->loc & HARN_RIGHT) != 0);
            v = lhurt[j] + p->il;
            lhurt[j] = v < p->il ? 255 : v;
            if (p->flags & (HARN_IF_LIMB | HARN_IF_AMPUTATED)) {
                l |= LB_ARM_L << j;             /* (Useless, or gone: an amputation's never heals whole) */
            }
        }
    }
    return l;
}

/* Its conditions (rs.h's C_*), as its sheet has them (the character's, or a creature's: its body cplan's, BF_*):
** its state (senseless, in shock, dead), bleeders, limbs made useless and how they move it now (the book's, Combat 14:
** on two legs, one useless, it hops or walks with a staff, its arms enough it crawls, none: C_CRAWLS, else C_STUCK; on
** four, one useless, at half its move, C_LAME, two, C_CRAWLS, more, C_STUCK); tired, or tiring */
unsigned __fastcall__ o_cond_of (const harn_char* h)
{
    static const unsigned state[4] = { 0, C_UNCONSCIOUS, C_STUNNED, C_DEAD };
    unsigned c = state[h->state & 3] | (h->fl || (h == &pc && tired) ? C_FATIGUED : 0);   /* (Tired, or tiring) */
    unsigned char k, l = o_limbs_of (h), arms, legs, b = h == &pc ? BF_BIPED : cplan;

    for (k = 0; k < h->ninj; ++k) {
        if (h->inj[k].flags & HARN_IF_BLEEDER) {
            c |= C_BLEEDING;
        }
    }
    arms = !!(l & LB_ARM_L) + !!(l & LB_ARM_R);
    legs = !!(l & LB_LEG_L) + !!(l & LB_LEG_R);
    c |= (arms ? C_ARM_USELESS : 0) | (legs ? C_LEG_USELESS : 0);
    if (b & BF_BIPED) {
        c |= !legs ? 0 : legs == 1 || !arms ? C_CRAWLS : C_STUCK;
    } else if (b & BF_QUAD) {
        k = arms + legs;                        /* (Its forelegs the strike table's arms) */
        c |= !k ? 0 : k == 1 ? C_LAME : k == 2 ? C_CRAWLS : C_STUCK;
    }
    return c;
}

/* Getting up, prone (the book's Rise, Combat 8: a round; the house's, every actor's, HOUSE.md's prone): its grade
** (rs.h's G_*): never with no leg to stand on (on two legs, both useless; on four, two or more: G_CF); seriously or
** grievously wounded, or with no good arm (the user's, 2026-10-22), an Agility test (x5; no good arm four steps the
** harder); else always (G_MS) */
signed char __fastcall__ o_rise_of (const harn_char* h)
{
    unsigned char l = o_limbs_of (h), legs, arms;

    legs = !!(l & LB_LEG_L) + !!(l & LB_LEG_R);
    arms = !!(l & LB_ARM_L) + !!(l & LB_ARM_R);
    if (h != &pc && (cplan & BF_QUAD)) {
        legs += arms;                           /* (Its forelegs the arms: two lame of four, it can't, as on two) */
        arms = 0;
    }
    if (legs >= 2) {
        return G_CF;
    }
    if (!(l & LB_HURT) && arms < 2) {
        return G_MS;
    }
    return grade (harn_test_attr (h, HARN_AGL, 5, arms < 2 ? 0 : -4 * STEP_PCT));
}

