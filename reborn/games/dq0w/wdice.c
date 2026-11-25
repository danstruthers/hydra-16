/*
** wdice.c - (dq0w) the world's keyed dice (phase C, dice2, 2026-11-24): the world's own rolls (where a thing let fall
** lies, a missile's stray, its flight past its mark, whether it's lost) each a function of its key alone, so no roll
** elsewhere moves them: the game's seed and its dice offset (dq0 -R's), the moment (the hub's clock), and what the roll
** is for and about (a and b: its WD_* and the thing, the actor, the squares).  Two multiplies and two of the
** generator's steps between, so keys a few apart roll apart; the dice after it the generator's (dice.h's random), as
** before.  The dungeon's own dice (a stratum's bounds, its record: the maker's and the world's by the stratum's seed)
** aren't these.  The hub's bank 0 left in the window.  Swapped part wdice's, called through its stub wkey from the
** parts that roll (a nested swap: the rolls are few).  And a test's rigged rolls (hub.h's struct rig, an entry of kind
** RIG_WORLD, its a the roll's WD_* purpose's top four bits or RIG_ANY): the rolls after the key (wroll, in place of
** random) are its faces less 1 in turn (modulo the roll's sides), the rest the key's; every time it matches (the
** world keeps no count: the table's the referee's)
*/

#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"

extern unsigned long xs_seed;                   /* (xorshift.s's state) */
unsigned long rand32 (void);

static unsigned char wr_f[4], wr_k;            /* (A rigged roll's faces, the next: the main RAM's) */

/* A roll of n (0 ... n - 1): a rigged face's, else the generator's (the main RAM's: the parts' rolls) */
unsigned __fastcall__ wroll (unsigned n)
{
    return wr_k < 4 && wr_f[wr_k] ? (wr_f[wr_k++] - 1) % n : random (n);
}

#pragma code-name ("SW_wdice_CODE")
#pragma rodata-name ("SW_wdice_RODATA")

void __fastcall__ s_wkey (unsigned a, unsigned b)
{
    const struct rig_e* e;

    hub_map (0);                                /* (The hub's bank left in: the callers') */
    xs_seed = (HUB->now ^ HUB->seed ^ (unsigned long) HUB->dice << 24) * 1103515245UL ^ ((unsigned long) a << 16 | b);
    rand32 ();
    xs_seed = xs_seed * 1103515245UL + 12345UL ^ 0x57444B59UL;
    rand32 ();
    wr_k = 0;                                   /* (A test's rig: the first entry that's the world's for this purpose) */
    memset (wr_f, 0, sizeof wr_f);
    for (e = HUB->rig.e; e < HUB->rig.e + RIG_N && e->kind; ++e) {
        if (e->kind == RIG_WORLD && (e->a == RIG_ANY || e->a == a >> 12)) {
            memcpy (wr_f, e->face, sizeof wr_f);
            break;
        }
    }
}
