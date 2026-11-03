/*
** feed.c - (dq0, swapped part 16: swap.inc) eating and drinking, the UI's part (slice 4.9: HOUSE.md's water, hunger and
** thirst): "eat", "drink", the world's things asked (IO_USE, IO_FIND), the referee told (rswant.h's)
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"
#include "rs.h"
#include "actions.h"

#pragma code-name ("SW16CODE")
#pragma rodata-name ("SW16RODATA")
#pragma local-strings (on)

/* ---- Eating and drinking (slice 4.9: HOUSE.md's water, hunger and thirst, every ruleset's): "eat" a meal (five minutes,
** a third of a day's ration: one used up as a day's begun, the things' IO_USE by its key, "food"); "drink" its fill
** from the water it stands in or a well within reach (the things' IO_FIND), else a pint at a time from its waterskin
** (IO_USE, "water": half a minute each), as many as it wants; the referee told (RS_TIME, w1 0: x[14] the meals, | 0x80 a
** ration begun, x[15] the pints), its hunger and thirst its own (rswant.h's); not hungry, not thirsty, refused */
static void __fastcall__ feed_do (unsigned char eat)
{
    struct req* r;
    unsigned w;
    unsigned char m = 0, p = 0, n, f = 0;

    if (!strcmp (cmd.obj.word, "antidote") || !strcmp (cmd.obj.word, "remedy")) {   /* (A remedy taken, 2026-11-02:
                                                **   HOUSE.md's remedies; the referee's to judge, RT_TAKEN) */
        if (othings (IO_USE, cmd.obj.word, "", 0) != IR_OK) {
            sayf ("\nYou have no %s.", cmd.obj.word);
            return;
        }
        sayf ("\nYou drink the %s.", cmd.obj.word);
        m = 0;
        p = RT_TAKEN | (cmd.obj.word[0] == 'a' ? 1 : 2);
        w = 50;
        goto told;
    }
    hub_map (0);
    w = eat ? HUB->pc_hunger : HUB->pc_thirst;
    if (w < (eat ? 120 : 60)) {
        msg (eat ? "You're not hungry." : "You're not thirsty.");
        return;
    }
    if (eat) {
        if (!HUB->pc_meals) {
            if (othings (IO_USE, "food", "", 0) != IR_OK) {
                msg ("You have nothing to eat.");
                return;
            }
            m = 0x80;
        }
        ++m;
        msg ("You eat a meal.");
        w = 3000;
    } else {
        n = (unsigned char) ((w + 359) / 360);
        if (strcmp (cmd.obj.word, "waterskin") && (HUB->pc_wet || othings (IO_FIND, "well", "", 0) == IR_OK)) {
            p = n;
            hub_map (0);
            f = (unsigned char) (HUB->pc_wet - 1) < 0x7F ? 0x80 : 0;     /* (Still water where it stands: foul,
                                                **   the referee's flux, 2026-10-31) */
            msg ("You drink your fill.");
        } else {
            while (p < n && othings (IO_USE, "water", "", 0) == IR_OK) {
                ++p;
            }
            msg (p ? "You drink from your waterskin." : "You have nothing to drink.");
            if (!p) {
                return;
            }
        }
        w = p * 300;
    }
told:
    hub_map (0);
    r = &HUB->slot[SLOT_UI];
    r->w1 = r->w2 = 0;
    r->x[0] = r->x[2] = 0;
    r->x[14] = m;
    r->x[15] = p | f;
    ask_b (RS_TIME, 0, 0)->state = REQ_FREE;
    if (p & RT_TAKEN) {
        body_words ();                          /* (The remedy's work told at once: the referee's news) */
    }
    intend2 (I_WAIT, w, 1);
}

void __fastcall__ s_feed (unsigned char eat)
{
    unsigned b = hub_here ();                   /* (Called from overlay 2: its window put back) */

    feed_do (eat);
    hub_back (b);
}
