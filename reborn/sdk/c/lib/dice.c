/*
** dice.c - dice.h's dice: danlang's lib/dice.dl (die+, die, dice+, dice), on xorshift.s's numbers; and the
** generator's seed from the clock and the ticks, for a first number before any rseed.  dicemore.c has sumrolls,
** special-die and the open-ended dice, dicebest.c roll, best-of-rolls, worst-of-rolls and the D&D attribute rolls
** (each a file of its own, so a program links only what it uses).
*/

#include <dice.h>
#include <hydra.h>

/* (xorshift.s, its first number with no seed: the clock (seconds) and the ticks, as HyForth's lib random seeds) */
void _rseed_clock (void)
{
    rseed (hy_time () ^ hy_ticks ());
}

int __fastcall__ die_plus (unsigned sides, int mod)
{
    return 1 + mod + (int) random (sides);
}

int __fastcall__ die (unsigned sides)
{
    return 1 + (int) random (sides);
}

int __fastcall__ dice_plus (unsigned char n, unsigned sides, int mod)
{
    while (n--) {
        mod += die (sides);
    }
    return mod;
}

int __fastcall__ dice (unsigned char n, unsigned sides)
{
    return dice_plus (n, sides, 0);
}
