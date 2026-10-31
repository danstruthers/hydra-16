/*
** water.c - (overlay 6) the water's news, hunger's and thirst's, worded (slice 4.9: HOUSE.md's water and hunger and
** thirst): the world's E_BEAT WB_WATER ... WB_WANT, the character's (main.c's limb_words, part 11's)
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include "hub.h"
#include "dq0.h"

/* The water's news, hunger's and thirst's (an E_BEAT's, WB_WATER on: slice 4.9's), the character's, worded */
void __fastcall__ o_water_words (unsigned w)
{
    static const char* const into[8] = { "\nYou wade out of the water.", "\nYou wade into the water, a foot or two deep.",
        "\nYou wade in up to your waist.", "\nYou're out of your depth: you swim.", "\nYou step out of the stream.",
        "\nYou wade into the stream; it tugs at your legs.", "\nThe stream runs about your waist, pulling at you.",
        "\nThe stream sweeps you off your feet: you swim." };
    static const char* const thirst[4] = { "\nYou're getting thirsty.", "\nYou're very thirsty.",
        "\nYou're parched.", "\nYou're dying of thirst." };
    static const char* const hunger[4] = { "\nYou're getting hungry.", "\nYou're very hungry.", "\nYou're starving.",
        "\nYou're wasting away with hunger." };
    unsigned char b = w >> 8;

    switch ((unsigned char) w) {
    case WB_WATER:
        msg (into[(b & 3) | (b & 0x80 ? 4 : 0)]);
        break;
    case WB_SWIM:
        msg (b & 2 ? "\nYou come up, gasping." : b & 1 ? "\nYou swim strongly." : "\nYou struggle to keep afloat.");
        break;
    case WB_UNDER:
        if (!b) {
            msg ("\nYou go under!");
        }
        break;
    case WB_DOUSED:
        msg ("\nThe water puts out your light.");
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

