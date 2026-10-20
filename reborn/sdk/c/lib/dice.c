/*
** dice.c - dice.h's dice: danlang's lib/dice.dl (die+, die, dice+, dice), on xorshift.s's numbers, and the tests'
** hook (dice_hook: every die rolled by a program's own function).  diceseed.c has the generator's seed from the clock
** and the ticks (a program that only takes random numbers links that, not these), dicemore.c sumrolls, special-die and
** the open-ended dice, dicebest.c roll, best-of-rolls, worst-of-rolls and the D&D attribute rolls (each a file of its
** own, so a program links only what it uses).
*/

#include <dice.h>

int __fastcall__ (*dice_hook) (unsigned sides);

int __fastcall__ die_plus (unsigned sides, int mod)
{
    return die (sides) + mod;
}

int __fastcall__ die (unsigned sides)
{
    if (dice_hook) {
        return dice_hook (sides);
    }
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
