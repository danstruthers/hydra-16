/*
** harntest.c - harn.h's tests (Skills 5, 21-22): d100 against an effective mastery level and its four levels of
** success; a skill's EML (ML less five times the penalty it's subject to); skill against skill; an attribute tested
** as a skill (times 3 to 7) or with 3d6; the shock, stumble and fumble rolls.
*/

#include <harn.h>

/* (harn.dl's roll-3d6: here, as the tests use it, so a program that tests doesn't bring harngen.c with it) */
int roll_3d6 (void)
{
    return dice (3, 6);
}

static unsigned char last;                      /* The last test's d100 */

/* d100 at or under the EML (kept from 5 to 95) succeeds; a roll ending in 0 or 5 is critical */
unsigned char __fastcall__ harn_test (int eml)
{
    unsigned char r = last = d100 ();

    if (eml < 5) {
        eml = 5;
    } else if (eml > 95) {
        eml = 95;
    }
    if (r <= eml) {
        return r % 5 ? HARN_MS : HARN_CS;
    }
    return r % 5 ? HARN_MF : HARN_CF;
}

unsigned char __fastcall__ harn_last_roll (void)
{
    return last;
}

/* Physical and combat skills: physical penalty; the rest: universal (Skills 6) */
int __fastcall__ harn_eml (const harn_char* c, unsigned char sk)
{
    const harn_skill* s = harn_get_skill_info (sk);
    unsigned char g = s ? s->group : HARN_CRAFT;

    return (int) c->ml[sk] - 5 * (g == HARN_PHYSICAL || g == HARN_COMBAT ? harn_get_pp (c) : harn_get_up (c));
}

unsigned char __fastcall__ harn_test_skill (const harn_char* c, unsigned char sk, int mod)
{
    return harn_test (harn_eml (c, sk) + mod);
}

unsigned char __fastcall__ harn_versus (unsigned char a, unsigned char b)
{
    return a > b ? 1 : b > a ? 2 : 0;
}

/* An attribute's penalty: physical attributes PP, the rest UP */
static unsigned char __fastcall__ attr_penalty (const harn_char* c, unsigned char a)
{
    return harn_attr_group (a) == HARN_A_PHY || a == HARN_END ? harn_get_pp (c) : harn_get_up (c);
}

unsigned char __fastcall__ harn_test_attr (const harn_char* c, unsigned char a, unsigned char times, int mod)
{
    return harn_test ((int) harn_get_attr (c, a) * times - 5 * attr_penalty (c, a) + mod);
}

unsigned char __fastcall__ harn_roll_attr (const harn_char* c, unsigned char a, int mod)
{
    return roll_3d6 () + attr_penalty (c, a) + mod <= (int) harn_get_attr (c, a);
}

/* 1d6 a point of universal penalty: over Endurance, the character's down (Skills 22) */
unsigned char __fastcall__ harn_shock_roll (const harn_char* c)
{
    return dice (harn_get_up (c), 6) > harn_end (c);
}

unsigned char __fastcall__ harn_stumble_roll (const harn_char* c, int mod)
{
    return roll_3d6 () + harn_get_pp (c) + mod > (int) c->att[HARN_AGL];
}

unsigned char __fastcall__ harn_fumble_roll (const harn_char* c, int mod)
{
    return roll_3d6 () + harn_get_pp (c) + mod > (int) c->att[HARN_DEX];
}
