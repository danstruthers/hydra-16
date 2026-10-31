/*
** errand.c - (dq0) an ally's errands and the character's gifts, the UI's part (slice 4.8, 2026-10-30: DENIZENS.md's
** fetching and influence): before the denizens are asked (deal's: the hub's sway), the thing found, the world's
** (IO_FETCH): for "tell Grub to fetch the key", one seen lying about (its item and where it lies, the sway's item,
** ix, iy); for "give the bandit 5 pennies" ("give the pennies to the bandit", "offer the orc my dagger"), one the
** character carries (its item, its worth as a local takes it times the count, whether it's food), the one meant the
** command's thing then, the gift its other; for "follow me at 3 squares", the distance (ix).  Swapped part 15's.
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "actions.h"

#pragma code-name ("SW15CODE")
#pragma rodata-name ("SW15RODATA")
#pragma local-strings (on)

/* 1 to ask the denizens (the sway's item, ix, iy filled in), or 0 (told why not) */
unsigned char s_errand (void)
{
    struct ireq* q = &HUB->ireq;
    unsigned char r;
    unsigned n, w, v;
    char t[16];

    hub_map (0);
    HUB->sway.item = HUB->sway.ix = HUB->sway.iy = 0;
    q->sel = 0;
    if (cmd.action == VB_GIVE_OBJECT) {         /* (A gift: the thing the character carries, then the one meant) */
        if (!cmd.other.word[0]) {
            msg ("Give it to whom?");
            return 0;
        }
        q->to = 1;
        if (things (IO_FETCH, cmd.obj.word) != IR_OK) {
            sayf ("\nYou have no %s.", cmd.obj.word[0] ? cmd.obj.word : "such thing");
            return 0;
        }
        hub_map (0);
        n = q->qty ? q->qty : 1;
        if (cmd.n && cmd.n < n) {
            n = cmd.n;                          /* (A count of a lot: split off, the world's) */
            HUB->sway.iy = n << 8;
        }
        v = q->value;
        w = v && n > 0xFFFFU / v ? 0xFFFFU : v * n;
        HUB->sway.item = q->item;
        HUB->sway.ix = w;
        HUB->sway.iy |= q->craft == AP_FOOD;
        strcpy (t, cmd.obj.word);
        strcpy (cmd.obj.word, cmd.other.word);
        strcpy (cmd.other.word, t);
        cmd.obj.kind = 0;
        return 1;
    }
    if (cmd.order == OR_FETCH) {                /* (A thing to fetch: one seen lying about) */
        if (!cmd.other.word[0]) {
            msg ("Fetch what?");
            return 0;
        }
        q->to = 0;
        if ((r = things (IO_FETCH, cmd.other.word)) != IR_OK) {
            sayf (r == IR_NONE ? "\nYou see no %s here." : "\nThe %s can't be fetched from where it is.",
                  cmd.other.word);
            return 0;
        }
        hub_map (0);
        HUB->sway.item = q->item;
        HUB->sway.ix = q->x;
        HUB->sway.iy = q->y;
    } else if (cmd.order == OR_FOLLOW) {
        HUB->sway.ix = cmd.n;                   /* (Its distance: "follow me at 3 squares") */
    }
    return 1;
}
