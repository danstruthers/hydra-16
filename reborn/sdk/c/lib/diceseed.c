/*
** diceseed.c - (dice.h's) the generator's seed from the clock and the ticks, for a first number before any rseed
** (xorshift.s's): a file of its own, so a program that takes only random numbers doesn't link the dice too.
*/

#include <dice.h>
#include <hydra.h>

/* (xorshift.s, its first number with no seed: the clock (seconds) and the ticks, as HyForth's lib random seeds) */
void _rseed_clock (void)
{
    rseed (hy_time () ^ hy_ticks ());
}
