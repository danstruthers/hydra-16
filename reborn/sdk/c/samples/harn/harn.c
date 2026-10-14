/*
** harn.c - a HârnMaster character made and shown as a Character Profile has it (harn.h: the book's Character
** article), a blow struck at it (the combat tables, the injury table), and some dice (dice.h).  harn [seed]: a seed
** makes the same character, blow and rolls each time.  (It writes with fputs, not printf: harn.h's tables and
** printf together leave a program too little room for its stack.)
*/

#include <stdio.h>
#include <stdlib.h>
#include <harn.h>

static void __fastcall__ s (const char* t)
{
    fputs (t, stdout);
}

static void __fastcall__ n (int v)
{
    char b[8];

    s (itoa (v, b, 10));
}

static void __fastcall__ named (const char* name, int v, const char* after)
{
    s (name);
    s (" ");
    n (v);
    s (after);
}

static const unsigned char shown[] = {
    HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL,
    HARN_MOR
};

static int d6f (void)
{
    return d6 ();
}

int main (int argc, char* argv[])
{
    static harn_char h;
    static const harn_armour* worn[2];
    const harn_weapon* sword = &harn_weapons[16];       /* (A broadsword) */
    unsigned char k, six[6], four[6][4], loc, res, w;
    int r[3], impact;

    if (argc > 1) {
        rseed (strtoul (argv[1], 0, 10));
    }
    harn_create (&h, 0);

    s (harn_species_name (h.att[HARN_SP]));
    s (" ");
    s (harn_sex_name (h.att[HARN_SX]));
    s (", born ");
    n (h.att[HARN_BD] & 0xFF);
    s (" ");
    s (harn_month_name (h.att[HARN_BD] >> 8));
    s (" (");
    s (harn_sunsign_name (h.att[HARN_SS]));
    s ("); ");
    s (harn_culture_name (h.culture));
    s (" ");
    s (harn_class_name (h.social));
    s ("\n");
    n (h.sibrank);
    s (" of ");
    n (h.famsize);
    s ("; ");
    s (harn_parent_name (h.parent));
    s (", ");
    s (harn_estrangement_name (h.estrange));
    s ("; clanhead: ");
    s (harn_clanhead_name (h.clanhead));
    s ("\n");
    n (h.att[HARN_HT]);
    s ("\", ");
    s (harn_frame_name (h.att[HARN_FR]));
    s (", ");
    n (h.att[HARN_WT]);
    s (" lb (size ");
    n (harn_size (&h));
    s ("); ");
    named (harn_comeliness_name (h.att[HARN_CML]), h.att[HARN_CML], "; ");
    s (harn_complexion_name (h.complexion));
    s (", ");
    s (harn_hair_name (h.hair));
    s (" hair, ");
    s (harn_eye_name (h.eyes));
    s (" eyes\n");
    for (k = 0; k < sizeof shown; ++k) {
        named (harn_attr_name (shown[k]), h.att[shown[k]], k == 7 || k + 1 == sizeof shown ? "\n" : ", ");
    }
    named ("Endurance", harn_end (&h), ", ");
    named ("Move", harn_move (&h), ", ");
    named ("Piety", h.att[HARN_PTY], "; medical:");
    for (k = 0; k < HARN_MEDMAX && h.med[k]; ++k) {
        s (" ");
        s (harn_medical_name (h.med[k]));
    }
    s (h.med[0] ? "\n" : " none\n");

    s ("Automatic skills (SB/ML):");
    for (k = 0; k < HARN_NSKILLS; ++k) {
        if (harn_skills[k].automatic) {
            s (" ");
            named (harn_skill_name (k), harn_get_sb (&h, k), "/");
            n (h.ml[k]);
        }
    }
    s ("\n");

    /* A broadsword strike on it, mail and quilt worn: the attacker's MS against its dodge's MF */
    worn[0] = &harn_armours[39];                /* (A mail hauberk, */
    worn[1] = &harn_armours[10];                /*   a gambeson) */
    res = harn_melee_result (HARN_DODGE, HARN_MS, HARN_MF);
    loc = harn_strike_location (HARN_MID);
    impact = dice (HARN_R_DICE (res), 6) + sword->imp[HARN_EDGE] - harn_armour_at (worn, 2, loc, HARN_EDGE, 1);
    w = harn_wound (&h, loc, HARN_EDGE, impact);
    s ("A broadsword: A*");
    n (HARN_R_DICE (res));
    s (" on the ");
    s (harn_is_limb (loc) ? (loc & HARN_RIGHT ? "right " : "left ") : "");
    s (harn_location_name (loc));
    s (", effective impact ");
    n (impact);
    s (": ");
    if (w & HARN_W_DEAD) {
        s ("killed\n");
    } else if (w) {
        s (harn_severity_name (h.inj[0].sev));
        s (", ");
        n (h.inj[0].il);
        s (h.inj[0].il == 1 ? " level" : " levels");
        s (w & HARN_W_DOWN ? ", down" : "");
        named ("; UP", harn_get_up (&h), "\n");
    } else {
        s ("no injury\n");
    }

    named ("3d6", dice (3, 6), ", ");
    named ("d100", d100 (), ", ");
    named ("d20+2", d20_plus (2), ", ");
    named ("open-ended d6", oe_die (6), "\n");
    named ("the best 3 of 5 d6:", best_of_rolls (r, 3, 5, d6f), " (");
    for (k = 0; k < 3; ++k) {
        n (r[k]);
        s (k < 2 ? " " : ")\n");
    }
    dnd_attr_rolls_detailed (six, four);
    s ("D&D attributes:");
    for (k = 0; k < 6; ++k) {
        s (" ");
        n (six[k]);
        s (" (");
        n (four[k][0]);
        s (" ");
        n (four[k][1]);
        s (" ");
        n (four[k][2]);
        s (" ");
        n (four[k][3]);
        s (")");
    }
    s ("\n");
    return 0;
}
