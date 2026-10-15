/*
** harnfall.c - harn.h's falling (Skills 8): a fall's impact by its height, the ground it lands on, a landing made on
** the feet (Acrobatics or Dodge, in a clear fall of more than 15 feet), and a hold grabbed on the way down (DEX
** times 5: the fall slowed, or stopped; an arm wrenched).  The impact's a blunt strike: harn_wound makes it an
** injury, at a location the caller rolls (harn_strike_location: low if it landed on its feet).
*/

#include <harn.h>

/* By the ground (HARN_GROUND ...): d6s more (or fewer, and so taken away) */
static const signed char ground_dice[6] = { 0, -3, -2, -1, 1, 2 };

/* By the landing's level (CF, MF, MS, CS): Acrobatics's d6s, and Dodge's */
static const signed char acro_dice[4] = { 1, 0, -2, -3 };
static const signed char dodge_dice[4] = { 2, 1, -1, -2 };

/* By the grab's level: d6s the fall's slowed by (a CF: none, and an arm wrenched) */
static const unsigned char grab_dice[4] = { 0, 0, 2, 3 };

/* n d6s: added, or taken away (n under 0) */
static int __fastcall__ sdice (signed char n)
{
    return n < 0 ? -dice (-n, 6) : dice (n, 6);
}

unsigned char __fastcall__ harn_fall_dice (unsigned feet)
{
    return feet <= 5 ? 0 : feet > 2540 ? 255 : (feet + 9) / 10;
}

int __fastcall__ harn_fall (harn_char* c, unsigned feet, unsigned char ground, unsigned char flags,
                            unsigned char* how)
{
    unsigned char n = harn_fall_dice (feet), r;
    int impact;

    *how = 0;
    if (!n) {
        return 0;
    }
    impact = dice (n, 6) + sdice (ground_dice[ground < 6 ? ground : 0]);
    if ((flags & HARN_FALL_CLEAR) && feet > 15) {   /* (On its feet, or its head: the better of the two skills) */
        if (c->ml[HARN_ACR] && harn_eml (c, HARN_ACR) >= harn_eml (c, HARN_DGE)) {
            r = harn_test_skill (c, HARN_ACR, 0);
            impact += sdice (acro_dice[r]);
        } else {
            r = harn_test_skill (c, HARN_DGE, 0);
            impact += sdice (dodge_dice[r]);
        }
        if (HARN_SUCCESS (r)) {
            *how |= HARN_FALL_FEET;
        }
    }
    if (flags & HARN_FALL_HOLDS) {              /* (A hold grabbed: slowed, or stopped if it takes it all) */
        r = harn_test_attr (c, HARN_DEX, 5, 0);
        if (r == HARN_CF) {
            *how |= HARN_FALL_WRENCHED;
            harn_wound (c, HARN_SHOULDER | (random (2) ? HARN_RIGHT : 0), HARN_BLUNT, 1);
        } else if (grab_dice[r]) {
            impact -= dice (grab_dice[r], 6);
            if (impact < 0) {
                *how |= HARN_FALL_CAUGHT;
                return 0;
            }
        }
    }
    return impact < 0 ? 0 : impact;
}
