/*
** shots.c - (overlay 15) the missiles' words (slice 4.7's: E_TASK's TE_NO_SHOT, TE_LOADED, TE_GATHERED, TE_READIED, the
** world's dq0w/wshoot.c's): why there's no shot, a launcher loaded or taken up, the missiles gathered (task.c's
** o_task_words calls it: overlay 3's room)
*/

#pragma code-name ("OV15CODE")
#pragma rodata-name ("OV15RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"

static const char* const noshot[SH_CANT] = {   /* (TE_NO_SHOT's, by SH_*) */
    "You can't shoot now.", "You hold nothing to shoot with.", "You have nothing left to shoot.",
    "It's out of range.", "You have no clear shot at it.", "Someone's in the way of your shot.",
    "Your mark is gone from sight.", "Your hands are full: put something down first."
};

/* E_TASK's w (TE_*, a kind times 256: the launcher's, the missile's), b named (why, SH_*; how many; already) */
void __fastcall__ o_shot_words (unsigned w, unsigned char named)
{
    char mk[sizeof CATALOGUE[0].name];
    unsigned char p = (unsigned char) w, k = w >> 8;

    if (p == TE_NO_SHOT) {
        msg (noshot[named < SH_CANT ? named : 0]);
        return;
    }
    hub_mem (KIND_BANK, mk, CATALOGUE[k].name, sizeof mk);
    if (p == TE_GATHERED) {
        if (!named) {
            msg ("You find nothing to gather.");
            return;
        }
        if (named == 1 && mk[strlen (mk) - 1] == 's') {
            mk[strlen (mk) - 1] = 0;            /* ("1 arrow") */
        }
        sayf ("\nYou gather up %u %s.", named, mk);
    } else if (p == TE_READIED) {
        sayf ("\nYou take up your %s.", mk);
    } else if (named) {
        sayf ("\nYour %s is loaded already.", mk);
    } else {
        sayf (k == KI_CROSSBOW ? "\nYou span the %s and set a bolt in it." : k == KI_SLING ?
            "\nYou set a stone in your %s." : "\nYou nock an arrow to your %s.", mk);
    }
}
