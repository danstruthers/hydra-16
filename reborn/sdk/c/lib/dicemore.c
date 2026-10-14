/*
** dicemore.c - dice.h's sumrolls, special-die and open-ended dice: danlang's lib/dice.dl's
*/

#include <dice.h>

int __fastcall__ sumrolls (const struct dice_spec* d, unsigned char count)
{
    int sum = 0;

    for (; count--; ++d) {
        sum += dice_plus (d->n, d->sides, d->mod);
    }
    return sum;
}

int __fastcall__ special_die (unsigned sides, const struct dice_case* cases, unsigned char count)
{
    unsigned face = die (sides);

    for (; count--; ++cases) {
        if (cases->face == face) {
            return cases->value;
        }
    }
    return 0;
}

/* (dice.dl's oe-die+: a roll of the highest face is that many, and another roll; the modifier added once) */
int __fastcall__ oe_die_plus (unsigned sides, int mod)
{
    int r;

    while ((r = die (sides)) == (int) sides) {
        mod += sides;
    }
    return r + mod;
}

int __fastcall__ oe_die (unsigned sides)
{
    return oe_die_plus (sides, 0);
}

int __fastcall__ oe_dice_plus (unsigned char n, unsigned sides, int mod)
{
    while (n--) {
        mod += oe_die (sides);
    }
    return mod;
}

int __fastcall__ oe_dice (unsigned char n, unsigned sides)
{
    return oe_dice_plus (n, sides, 0);
}
