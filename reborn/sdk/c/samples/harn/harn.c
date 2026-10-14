/*
** harn.c - a HârnMaster character, made and shown as danlang's tests/HarnTests.dl shows one (harn.h), and some dice
** (dice.h).  harn [seed]: a seed makes the same character and rolls each time.
*/

#include <stdio.h>
#include <stdlib.h>
#include <harn.h>

static const unsigned char shown[] = {
    HARN_HT, HARN_OWT, HARN_CML, HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_SPD, HARN_EYE, HARN_HRG, HARN_SMT,
    HARN_TCH, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL, HARN_MOR, HARN_PTY
};

static int d6f (void)
{
    return d6 ();
}

int main (int argc, char* argv[])
{
    static harn_char h;
    unsigned char k, six[6], four[6][4];
    int r[3];

    if (argc > 1) {
        rseed (strtoul (argv[1], 0, 10));
    }
    harn_create (&h, 0);

    printf ("\n**** Birth Attributes ****\n");
    printf ("%s %s\n", harn_attr_name (HARN_SP), harn_species_name (h.att[HARN_SP]));
    printf ("%s %s\n", harn_attr_name (HARN_SX), harn_sex_name (h.att[HARN_SX]));
    printf ("%s %s %u\n", harn_attr_name (HARN_BD), harn_month_name (h.att[HARN_BD] >> 8), h.att[HARN_BD] & 0xFF);
    printf ("%s %s\n", harn_attr_name (HARN_SS), harn_sunsign_name (h.att[HARN_SS]));
    printf ("%s %s\n", harn_attr_name (HARN_FR), harn_frame_name (h.att[HARN_FR]));

    printf ("\n**** Physical Attributes ****\n");
    for (k = 0; k < sizeof shown; ++k) {
        printf ("%s %u\n", harn_attr_name (shown[k]), h.att[shown[k]]);
    }
    printf ("%s %s\n", harn_attr_name (HARN_DTY), h.dty);
    printf ("%s %u\n", harn_attr_name (HARN_END), harn_end (&h));

    printf ("\n**** Automatic Skills ****\n");
    for (k = 0; k < HARN_NSKILLS; ++k) {
        if (harn_skills[k].automatic) {
            printf ("%s %u\n", harn_skill_name (k), harn_get_oml (&h, k));
        }
    }

    printf ("\n**** Dice ****\n");
    printf ("3d6 %d, d100 %d, d20+2 %d, open-ended d6 %d\n", dice (3, 6), d100 (), d20_plus (2), oe_die (6));
    printf ("the best 3 of 5 d6: %d (", best_of_rolls (r, 3, 5, d6f));
    printf ("%d %d %d)\n", r[0], r[1], r[2]);
    dnd_attr_rolls_detailed (six, four);
    printf ("D&D attributes:");
    for (k = 0; k < 6; ++k) {
        printf (" %u (%u %u %u %u)", six[k], four[k][0], four[k][1], four[k][2], four[k][3]);
    }
    printf ("\n");
    return 0;
}
