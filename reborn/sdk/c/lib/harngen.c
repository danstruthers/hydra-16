/*
** harngen.c - harn.h's characters made: danlang's lib/harn.dl's random species, sex, birthday, height and frame,
** its weight, an attribute's modifiers and one rolled, the sunsign of a birthday, and harn.create.
*/

#include <string.h>
#include <harn.h>

int roll_3d6 (void)
{
    return dice (3, 6);
}

/* ---- An attribute's modifiers */

/* (harn.species-attr-mod: a human male's are none; Other's none) */
int __fastcall__ harn_species_attr_mod (unsigned char sp, unsigned char sx, unsigned char a)
{
    switch (sp) {
    case HARN_HUM:
        if (sx != HARN_F) {
            return 0;
        }
        return a == HARN_FR ? -3 : a == HARN_AUR ? 2 : a == HARN_SMT ? 1 : 0;
    case HARN_SIN:
        switch (a) {
        case HARN_FR:
            return -2;
        case HARN_STR: case HARN_STA:
            return 1;
        case HARN_DEX: case HARN_AGL: case HARN_EYE: case HARN_HRG: case HARN_SMT: case HARN_TCH: case HARN_VOI:
            return 2;
        case HARN_MOR:
            return 3;
        case HARN_AUR:
            return 4;
        }
        return 0;
    case HARN_KHU:
        switch (a) {
        case HARN_AUR:
            return -2;
        case HARN_DEX: case HARN_EYE: case HARN_TCH:
            return 1;
        case HARN_STA: case HARN_HRG: case HARN_SMT:
            return 2;
        case HARN_FR: case HARN_WIL:
            return 3;
        case HARN_STR:
            return 4;
        }
        return 0;
    }
    return 0;
}

int __fastcall__ harn_frame_attr_mod (unsigned char fr, unsigned char a)
{
    return a == HARN_AGL ? 2 - (int) fr : 0;    /* Scant 2, light 1, medium 0, heavy -1, massive -2 */
}

int __fastcall__ harn_weight_attr_mod (unsigned weight, unsigned char a)
{
    static const unsigned upto[16] = { 20, 55, 85, 110, 130, 145, 155, 170, 190, 215, 245, 280, 320, 370, 430, 500 };
    unsigned char k;

    if (a != HARN_STR) {
        return 0;
    }
    for (k = 0; k < 16 && weight > upto[k]; ++k) {
    }
    return (int) k - 6;                         /* 20 and under -6 ... 155 and under 0 ... over 500 10 */
}

int __fastcall__ harn_medical_attr_mod (unsigned char med, unsigned char a)
{
    (void) med;
    (void) a;
    return 0;                                   /* (harn.dl's: none yet) */
}

int __fastcall__ harn_attr_mod (unsigned char sp, unsigned char sx, unsigned char fr, unsigned weight,
                                unsigned char med, unsigned char a)
{
    return harn_species_attr_mod (sp, sx, a) + harn_frame_attr_mod (fr, a) + harn_weight_attr_mod (weight, a) +
        harn_medical_attr_mod (med, a);
}

unsigned char __fastcall__ harn_rand_attr (unsigned char sp, unsigned char sx, unsigned char fr, unsigned weight,
                                           unsigned char med, unsigned char a)
{
    int v = roll_3d6 () + harn_attr_mod (sp, sx, fr, weight, med, a);

    return v < 1 ? 1 : v;
}

/* ---- Species, sex, the birthday */

unsigned char harn_rand_species (void)
{
    unsigned char r = d100 ();

    return r <= 98 ? HARN_HUM : r <= 99 ? HARN_SIN : HARN_KHU;
}

unsigned char __fastcall__ harn_rand_sex (unsigned char sp)
{
    unsigned char r = d100 ();

    switch (sp) {
    case HARN_HUM:
        return r <= 48 ? HARN_M : HARN_F;
    case HARN_SIN:
        return r <= 45 ? HARN_M : HARN_F;
    case HARN_KHU:
        return r <= 75 ? HARN_M : HARN_F;
    }
    return r <= 50 ? HARN_M : HARN_F;
}

unsigned char harn_rand_month (void)
{
    return d12 ();
}

unsigned harn_rand_birthday (void)
{
    unsigned char m = harn_rand_month ();

    return (m << 8) | d30 ();
}

/* harn.sunsign-of, as harn.dl has it, test for test (so its quirks too: the 1st of Nuzyael is Ulandus-Lado Cusp,
** the 2nd and 3rd Lado-Ulandus) */
unsigned char __fastcall__ harn_sunsign_of (unsigned char m, unsigned char d)
{
    switch (m) {
    case 1:                                     /* Nuzyael */
        return d >= 2 && d <= 3 ? HARN_LAD_ULA : d <= 5 ? HARN_ULA_LAD : HARN_ULA;
    case 2:                                     /* Peonu */
        return d == 1 ? HARN_ULA : d <= 3 ? HARN_ULA_ARA : d <= 5 ? HARN_ARA_ULA : HARN_ARA;
    case 3:                                     /* Kelen */
        return d <= 2 ? HARN_ARA_FEN : d <= 4 ? HARN_FEN_ARA : HARN_FEN;
    case 4:                                     /* Nolus */
        return d == 1 ? HARN_FEN : d <= 3 ? HARN_FEN_AHN : d <= 5 ? HARN_AHN_FEN : HARN_AHN;
    case 5:                                     /* Larane */
        return d <= 2 ? HARN_AHN : d <= 4 ? HARN_AHN_ANG : d <= 6 ? HARN_ANG_AHN : HARN_ANG;
    case 6:                                     /* Agrazhar */
        return d <= 4 ? HARN_ANG : d <= 6 ? HARN_ANG_NAD : d <= 8 ? HARN_NAD_ANG : HARN_NAD;
    case 7:                                     /* Azura */
        return d <= 3 ? HARN_NAD : d <= 5 ? HARN_NAD_HIR : d <= 7 ? HARN_HIR_NAD : HARN_HIR;
    case 8:                                     /* Halane */
        return d <= 2 ? HARN_HIR : d <= 4 ? HARN_HIR_TAR : d <= 6 ? HARN_TAR_HIR : HARN_TAR;
    case 9:                                     /* Savor */
        return d == 1 ? HARN_TAR : d <= 3 ? HARN_TAR_TAI : d <= 5 ? HARN_TAI_TAR : HARN_TAI;
    case 10:                                    /* Ilvin */
        return d <= 2 ? HARN_TAI_SKO : d <= 4 ? HARN_SKO_TAI : HARN_SKO;
    case 11:                                    /* Navek */
        return d <= 2 ? HARN_SKO_MAS : d <= 4 ? HARN_MAS_SKO : d <= 29 ? HARN_MAS : HARN_MAS_LAD;
    }
    return d == 1 ? HARN_MAS_LAD : d <= 3 ? HARN_LAD_MAS : HARN_LAD;     /* Morgat */
}

/* ---- Height, frame, weight */

unsigned char __fastcall__ harn_rand_height (unsigned char sp, unsigned char sx)
{
    unsigned char h = dice (4, 6);

    switch (sp) {
    case HARN_SIN:
        return h + (sx == HARN_F ? 51 : 52);
    case HARN_KHU:
        return h + 40;
    }
    return h + d6 () + (sx == HARN_F ? 48 : 50);  /* Human (and Other: harn.dl has none, so a human's) */
}

unsigned char __fastcall__ harn_frame_of (int n)
{
    return n <= 5 ? HARN_FR_S : n <= 8 ? HARN_FR_L : n <= 12 ? HARN_FR_M : n <= 15 ? HARN_FR_H : HARN_FR_V;
}

unsigned char __fastcall__ harn_rand_frame (unsigned char sp, unsigned char sx)
{
    return harn_frame_of (roll_3d6 () + harn_species_attr_mod (sp, sx, HARN_FR));
}

/* (harn.weight: (1 + the frame's -1/5 ... 1/5) * (75 + 2 (ht - 40) + the inches over 52, 63 and 70), rounded) */
unsigned __fastcall__ harn_weight (unsigned char ht, unsigned char fr)
{
    int base = 75 + 2 * ((int) ht - 40);

    if (ht > 52) {
        base += ht - 52;
    }
    if (ht > 63) {
        base += ht - 63;
    }
    if (ht > 70) {
        base += ht - 70;
    }
    return ((long) base * (8 + fr) + 5) / 10;  /* (8 + fr: 10 times 1 + -1/5 ... 1/5, as tenths) */
}

/* ---- harn.create */

/* The template's attribute a, if it has one, else v */
static unsigned __fastcall__ fixed (const harn_char* t, unsigned char a, unsigned v)
{
    return t && t->att[a] != HARN_ANY ? t->att[a] : v;
}

void __fastcall__ harn_create (harn_char* c, const harn_char* t)
{
    static const unsigned char rolled[] = {
        HARN_CML, HARN_STR, HARN_STA, HARN_DEX, 0xFF, HARN_EYE, HARN_HRG, HARN_SMT, HARN_TCH, HARN_VOI, HARN_INT,
        HARN_AUR, HARN_WIL, HARN_MOR
    };
    unsigned char sp, sx, fr, k, a;
    unsigned bd, wt;
    int spd;

    memset (c, 0, sizeof *c);
    c->att[HARN_SP] = sp = fixed (t, HARN_SP, harn_rand_species ());
    c->att[HARN_SX] = sx = fixed (t, HARN_SX, harn_rand_sex (sp));
    c->att[HARN_BD] = bd = fixed (t, HARN_BD, harn_rand_birthday ());
    c->att[HARN_SS] = harn_sunsign_of (bd >> 8, bd & 0xFF);
    c->att[HARN_HT] = fixed (t, HARN_HT, harn_rand_height (sp, sx));
    c->att[HARN_FR] = fr = fixed (t, HARN_FR, harn_rand_frame (sp, sx));
    c->att[HARN_OWT] = c->att[HARN_WT] = wt = harn_weight (c->att[HARN_HT], fr);
    c->att[HARN_AGL] = fixed (t, HARN_AGL, harn_rand_attr (sp, sx, fr, wt, 0, HARN_AGL));   /* (First: speed's) */
    for (k = 0; k < sizeof rolled; ++k) {
        if ((a = rolled[k]) == 0xFF) {          /* Speed: agility - 5 + d9, at least 1 */
            spd = (int) c->att[HARN_AGL] - 5 + die (9);
            c->att[HARN_SPD] = fixed (t, HARN_SPD, spd < 1 ? 1 : spd);
            continue;
        }
        c->att[a] = fixed (t, a, harn_rand_attr (sp, sx, fr, wt, 0, a));
    }
    c->att[HARN_PTY] = fixed (t, HARN_PTY, dice (5, 6));
    c->dty = t && t->dty ? t->dty : "<None chosen>";
    for (k = 0; k < HARN_NSKILLS; ++k) {        /* The automatic skills opened */
        if (harn_skills[k].automatic) {
            harn_open_skill (c, k);
        }
    }
}
