/*
** bdice.c - (the house's rules, ../rshouse.h's) the bearings' own dice, swapped in and out about their test
*/

#include "../hub.h"
#include "../rs.h"
#include "../referee.h"
#include "../rshouse.h"

void bdice (void)
{
    unsigned long t = xs_seed;

    xs_seed = bseed ? bseed : t ^ 0x42454152UL;
    bseed = t;
}
