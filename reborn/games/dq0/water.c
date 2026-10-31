/*
** water.c - (overlay 6) the water's news, hunger's and thirst's, worded (slice 4.9: HOUSE.md's water, hunger and
** thirst): the world's E_BEAT WB_WATER ... WB_WANT, the character's (main.c's limb_words, part 11's)
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include "hub.h"
#include "dq0.h"

/* The water's news, hunger's and thirst's (an E_BEAT's, WB_WATER ... WB_WANT: slice 4.9's, limb_words'), the character's, worded */
void __fastcall__ o_water_words (unsigned w)
{
    static const char* const into[4] = { "You wade out of the water.", "You wade into the water, a foot or two deep.",
        "You wade in up to your waist.", "You're out of your depth: you swim." };
    static const char* const thirst[4] = { "You're getting thirsty.", "You're very thirsty.",
        "You're parched.", "You're dying of thirst." };
    static const char* const hunger[4] = { "You're getting hungry.", "You're very hungry.", "You're starving.",
        "You're wasting away with hunger." };
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
    case WB_WANT:
        if (WANT_THIRST (b)) {
            msg (thirst[(WANT_THIRST (b) > 4 ? 4 : WANT_THIRST (b)) - 1]);
        }
        if (WANT_HUNGER (b)) {
            msg (hunger[(WANT_HUNGER (b) > 4 ? 4 : WANT_HUNGER (b)) - 1]);
        }
        break;
    }
}
