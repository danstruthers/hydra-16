/*
** tire.c - (dq0harn, overlay 7) a creature's body: its fatigue (2026-10-31, every actor's tiring: HOUSE.md's fights),
** by the character's rules (sheet.c's o_spent: a level 6000 units, strenuous work 2 a tenth, rest a unit a tenth and
** then a level each ten minutes), kept in a bank of the referee's beside the sheets (fb, dq0harn.h's struct bbody, by
** the same number), reckoned at need by the clock, never ticked: each round of a fight charged as strenuous work (the
** book's second variant, a level each five minutes of it), the time since its last round rest.
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")

#include <string.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

struct bbody bb;
#define LEVEL   6000

/* Sheet n's body brought up to now (bb): w tenths of strenuous work ending now (a round of a fight; no more than the
** time since it was last reckoned, so a round's charged once, its own blow and one at it), the time before it rest (its
** exertion worn off first, then a level each ten minutes, its want's never); 1 if a level more */
unsigned char __fastcall__ o_tire (unsigned char n, unsigned w)
{
    unsigned t;
    unsigned char up = 0;

    bread (fb, (struct bbody*) 0x8000 + n, &bb, sizeof bb);
    if (bb.at) {
        t = tnow <= bb.at ? 0 : tnow - bb.at >= 60000U ? 60000U : (unsigned) (tnow - bb.at);    /* (A hundred minutes or
                                                **   more: ten levels rested off, all it has as a rule) */
        if (w > t) {
            w = t;
        }
        t -= w;
        if (t >= bb.ex) {
            for (t -= bb.ex, bb.ex = 0; t >= LEVEL && bb.fl > bb.wfl; t -= LEVEL) {
                --bb.fl;
            }
        } else {
            bb.ex -= t;
        }
    }
    bb.at = tnow;
    for (bb.ex += 2 * w; bb.ex >= LEVEL; bb.ex -= LEVEL) {
        if (bb.fl < 99) {
            ++bb.fl;
            up = 1;
        }
    }
    return up;
}

/* Actor who's round of a fight charged (strenuous work, a round's) and kept: 1 if it's a level more tired */
unsigned char __fastcall__ o_worked (unsigned char who)
{
    unsigned char n = aden[(who - 1) & 7], up = o_tire (n, 100);

    bwrite (fb, (struct bbody*) 0x8000 + n, &bb, sizeof bb);
    return up;
}

/* Sheet n's body anew (k: the hunger and thirst's fatigue levels, the minds'), or (n | 0x100) a new creature's, rested */
void __fastcall__ o_want_fl (unsigned n, unsigned char k)
{
    bread (fb, (struct bbody*) 0x8000 + (unsigned char) n, &bb, sizeof bb);
    if (n & 0x100) {
        memset (&bb, 0, sizeof bb);
    }
    if (k > bb.wfl) {                           /* (The levels more, as the character's want's, want.c's; fewer, rested
                                                **   off) */
        bb.fl += k - bb.wfl;
    }
    bb.wfl = k;
    bwrite (fb, (struct bbody*) 0x8000 + (unsigned char) n, &bb, sizeof bb);
}
