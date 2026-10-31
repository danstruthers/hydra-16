/*
** water.c - (overlay 4) the water's news, worded (slice 4.9: HOUSE.md's water, hunger and
** thirst): the world's E_BEAT WB_WATER ... WB_DOUSED, the character's (main.c's limb_words, part 11's)
*/

#pragma code-name ("OV4CODE")
#pragma rodata-name ("OV4RODATA")

#include "hub.h"
#include "dq0.h"

/* The water's news (an E_BEAT's, WB_WATER ... WB_DOUSED: slice 4.9's, limb_words'), the character's, worded */
void __fastcall__ o_water_words (unsigned w)
{
    static const char* const into[4] = { "You wade out of the water.", "You wade into the water, a foot or two deep.",
        "You wade in up to your waist.", "You're out of your depth: you swim." };
    unsigned char b = w >> 8;

    switch ((unsigned char) w) {
    case WB_WATER:
        msg (into[b & 3]);
        break;
    case WB_SWIM:
        msg (b & 2 ? "You come up, gasping." : b & 1 ? "You swim strongly." : "You struggle to keep afloat.");
        break;
    case WB_UNDER:
        if (!b) {
            msg ("You go under!");
        }
        break;
    case WB_DOUSED:
        msg ("The water puts out your light.");
        break;
    }
}
