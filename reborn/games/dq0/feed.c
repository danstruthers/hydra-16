/*
** feed.c - (dq0, swapped part 16: swap.inc) eating and drinking, the UI's part (slice 4.9: HOUSE.md's water, hunger and
** thirst): "eat", "drink", the world's things asked (IO_USE, IO_FIND), the referee told (rf/want.c's)
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"
#include "rs.h"
#include "actions.h"

#pragma code-name ("SW_feed_CODE")
#pragma rodata-name ("SW_feed_RODATA")
#pragma local-strings (on)

/* ---- Eating and drinking (slice 4.9: HOUSE.md's water, hunger and thirst, every ruleset's): "eat" a meal (five minutes,
** a third of a day's ration: one used up as a day's begun, the things' IO_USE by its key, "food"); "drink" its fill
** from the water it stands in or a well within reach (the things' IO_FIND), else a pint at a time from its waterskin
** (IO_USE, "water": half a minute each), as many as it wants; the referee told (RS_TIME, w1 0: x[14] the meals, | 0x80 a
** ration begun, x[15] the pints), its hunger and thirst its own (rf/want.c's); not hungry, not thirsty, refused */
/* The thing named for eating or drinking (cmd.obj's word) one that is: food, a meal; water, a waterskin, the well; or one
** to hand whose kind's key is food, or water.  1 so, or none named; else 0, refused by its name ("You can't eat the
** pot.", "You have no apples.": dq0sweep2's, 2026-11-07, the rule that a command's answer names its thing) */
static unsigned char __fastcall__ fed_on (unsigned char eat)
{
    static const char* const ok[] = { "food", "meal", "meals", "water", "waterskin", "well" };
    char key[8];
    const char* w = cmd.obj.word;
    unsigned char k;

    if (!*w) {
        return 1;
    }
    for (k = eat ? 0 : 3; k < (eat ? 3 : 6); ++k) {
        if (!strcmp (w, ok[k])) {
            return 1;
        }
    }
    if (othings (IO_FIND, w, "", 0) != IR_OK) {
        say_no (A_HAVE_NO, w);
        return 0;
    }
    hub_mem (KIND_BANK, key, CATALOGUE[(unsigned char) othings_x].tool, sizeof key);   /* (Its kind: othings', the window put back) */
    if (!strcmp (key, eat ? "food" : "water")) {
        return 1;
    }
    unknown (U_CANT);
    return 0;
}

static void __fastcall__ feed_do (unsigned char eat)
{
    struct req* r;
    unsigned w;
    unsigned char m = 0, p = 0, n, f = 0;

    if (!strcmp (cmd.obj.word, "antidote") || !strcmp (cmd.obj.word, "remedy")) {   /* (A remedy taken, 2026-11-02:
                                                **   HOUSE.md's remedies; the referee's to judge, RT_TAKEN) */
        if (othings (IO_USE, cmd.obj.word[0] == 'a' ? "cure" : "remedy", "", 0) != IR_OK) {   /* (Its key: a tool key's seven letters) */
            say_no (A_HAVE_NO, cmd.obj.word);
            return;
        }
        sayf ("\nYou drink the %s.", cmd.obj.word);
        m = 0;
        p = RT_TAKEN | (cmd.obj.word[0] == 'a' ? 1 : 2);
        w = 50;
        goto told;
    }
    if (eat && fasting) {                       /* (A vow of fasting, hself.c's: a meal refused till its day's out, or
                                                **   it's broken; hunger counting on, the referee's: 2026-11-10) */
        hub_map (0);
        if (fasting > HUB->now) {
            w = (unsigned) ((fasting - HUB->now + 35999UL) / 36000UL);
            sayf ("\nYou're fasting, with %u hour%s of your vow to go: you leave the food be. (\"break my fast\" ends the vow.)",
                w, w == 1 ? "" : "s");
            return;
        }
        fasting = 0;
        msg ("Your day's fast is over.");
    }
    if (!fed_on (eat)) {
        return;
    }
    hub_map (0);
    w = eat ? HUB->pc_hunger : HUB->pc_thirst;
    if (w < (eat ? 120 : 60)) {
        if (cmd.obj.word[0]) {                  /* (Named: its answer names it) */
            sayf (eat ? "\nYou're not hungry. You leave the %s be." :
                "\nYou're not thirsty. You leave the %s be.", cmd.obj.word);
        } else {
            msg (eat ? "You're not hungry." : "You're not thirsty.");
        }
        return;
    }
    if (eat) {
        if (!HUB->pc_meals) {
            if (tool_out (1)) {
                return;                         /* (Got out first: the command again as it's up; 2026-11-04) */
            }
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
            if (tool_out (2)) {
                return;
            }
            while (p < n && othings (IO_USE, "water", "", 0) == IR_OK) {
                ++p;
                f |= othings_x ? 0x80 : 0;      /* (Filled from still water: the flux's risk, as drunk there,
                                                **   2026-11-03) */
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
    RS_TIME_MINUTES (*r) = RS_TIME_BLEEDERS (*r) = 0;
    RS_TIME_FIGHTING (*r) = RS_TIME_DROWNED (*r) = 0;
    RS_TIME_MEAL (*r) = m;
    RS_TIME_DRINK (*r) = p | f;
    ask_b (RS_TIME, 0, 0)->state = REQ_FREE;
    if (p & RT_TAKEN) {
        body_words ();                          /* (The remedy's work told at once: the referee's news) */
    }
    intend2 (I_WAIT, w, I_WAIT_W2 (0, 0, 1));
}

void __fastcall__ s_feed (unsigned char eat)
{
    unsigned b = hub_here ();                   /* (Called from overlay 2: its window put back) */

    feed_do (eat);
    hub_back (b);
}
