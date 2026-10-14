/*
** rules.c - Dingus Quest Zero's HârnMaster: the C SDK's harn.h (the book's rules), and what the game adds to it:
** its skill tests for the character (EML from harn.h, a development roll earned by use under stress, as the book's
** Skills 6 has the GM award them), and game time in the book's units.
**
** The game's clock counts the book's time, in tenths of a second: a combat round (Combat 7) is 10 seconds, a turn
** 100 tenths.  How fast that runs against the real clock is the pace (main.c): at the normal pace a round takes
** 3 real seconds, the dungeon's time running a little over three times the real one, so a fight moves while it's
** typed; resting and sleeping run it faster still.
*/

#include "dq0.h"

struct pc P;

void rules_init (void)
{
}

int __fastcall__ eml (unsigned char sk)
{
    return harn_eml (&P.h, sk);
}

/* A test of the character's skill with mod; a stress development roll owed for it (Skills 6: "learn at least as
** much from failure as from success") */
unsigned char __fastcall__ ptest (unsigned char sk, int mod)
{
    P.used[sk >> 3] |= 1 << (sk & 7);
    return harn_test_skill (&P.h, sk, mod);
}
