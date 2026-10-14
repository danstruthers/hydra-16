/*
** harngen.c - harn.h's characters made, as the book's Character article makes one (Character 2-12): species, sex,
** birthdate and sunsign; culture and social class; sibling rank, family size, parent, estrangement, clanhead;
** height, frame, weight, comeliness, complexion, hair and eyes; the physical attributes with their modifiers; medical
** traits; the personality attributes; deity and piety; and the automatic skills opened.  Mental disorders and
** sexuality, optional rules, are harn_rand_psyche's and harn_rand_sexuality's.
*/

#include <string.h>
#include <stddef.h>
#include <harn.h>

int roll_3d6 (void)
{
    return dice (3, 6);
}

/* A value from a d100 table: the first of n upper bounds the roll doesn't pass */
static unsigned char __fastcall__ d100_table (const unsigned char* upto, unsigned char n, int roll)
{
    unsigned char k;

    for (k = 0; k + 1 < n && roll > upto[k]; ++k) {
    }
    return k;
}

/* ---- Birth (Character 2-3) */

unsigned char harn_rand_species (void)
{
    unsigned char r = d100 ();

    return r <= 97 ? HARN_HUM : r == 98 ? HARN_SIN : r == 99 ? HARN_KHU : HARN_X;
}

unsigned char __fastcall__ harn_rand_sex (unsigned char sp)
{
    unsigned char r = d100 ();

    return r <= (sp == HARN_SIN ? 45 : sp == HARN_KHU ? 75 : 48) ? HARN_M : HARN_F;
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

/* Each sign's first day, as a day of the year (0: 1st Nuzyael ... 359: 30th Morgat): Ulandus from the 4th of
** Nuzyael ... Lado from the 2nd of Morgat, to the 3rd of Nuzyael (Character 3) */
static const unsigned start[12] = { 3, 33, 62, 93, 124, 156, 185, 214, 243, 272, 302, 331 };

unsigned char __fastcall__ harn_sunsign_of (unsigned char m, unsigned char d)
{
    unsigned day = (m - 1) * 30 + (d - 1), from, len;
    unsigned char s = 11;

    while (s && day < start[s]) {               /* (Before Ulandus's start: still Lado's) */
        --s;
    }
    if (!s && day < start[0]) {
        s = 11;
    }
    from = start[s];
    len = (s < 11 ? start[s + 1] : 360 + start[0]) - from;
    day = (day + 360 - from) % 360;             /* The day of its sign */
    return s * 3 + (day < 2 ? 0 : day >= len - 2 ? 2 : 1);
}

/* ---- Culture, class, family (Character 3-5) */

unsigned char __fastcall__ harn_rand_class (unsigned char culture)
{
    /* Each culture's upper bounds for slave, serf, unguilded, guilded (noble the rest).  (Viking: the book has
    ** unguilded 81-93 and guilded 86-98; here unguilded 81-85.  Feudal slaves are Rethem's: elsewhere serfs) */
    static const unsigned char upto[6][4] = {
        { 10, 10, 99, 99 }, { 15, 80, 85, 98 }, { 15, 70, 93, 98 }, { 25, 25, 90, 98 }, { 0, 0, 15, 98 },
        { 0, 0, 70, 99 }
    };

    return d100_table (upto[culture < 6 ? culture : HARN_FEUDAL], 5, d100 ());
}

static unsigned char rand_parent (void)
{
    static const unsigned char upto[5] = { 50, 70, 75, 90, 100 };

    return d100_table (upto, 5, d100 ());
}

static unsigned char __fastcall__ rand_estrangement (unsigned char eldest)
{
    static const unsigned char upto[5] = { 10, 40, 60, 95, 100 };
    int r = d100 () + (eldest ? 5 : 0);

    return d100_table (upto, 5, r > 100 ? 100 : r);
}

static unsigned char rand_clanhead (void)
{
    static const unsigned char upto[4] = { 50, 75, 85, 100 };

    return d100_table (upto, 4, d100 ());
}

/* ---- Appearance (Character 6) */

unsigned char __fastcall__ harn_rand_height (unsigned char sp, unsigned char sx)
{
    unsigned char base = sp == HARN_SIN ? (sx == HARN_F ? 50 : 51) : sp == HARN_KHU ? 40 : (sx == HARN_F ? 52 : 54);

    return base + dice (4, 6);
}

unsigned char __fastcall__ harn_frame_of (int n)
{
    return n <= 5 ? HARN_FR_S : n <= 8 ? HARN_FR_L : n <= 12 ? HARN_FR_M : n <= 15 ? HARN_FR_H : HARN_FR_V;
}

unsigned char __fastcall__ harn_rand_frame (unsigned char sp, unsigned char sx)
{
    return harn_frame_of (roll_3d6 () + harn_species_attr_mod (sp, sx, HARN_FR));
}

/* The Height/Weight table: 75 pounds at 40", 2 more an inch to 51", 3 from 52", 4 from 64", 3 at 70", 5 from 71"
** (to 89": 255); then the frame's -20% ... +20%, rounded */
unsigned __fastcall__ harn_weight (unsigned char ht, unsigned char fr)
{
    unsigned w;

    if (ht < 40) {
        ht = 40;
    } else if (ht > 89) {
        ht = 89;
    }
    w = ht <= 51 ? 75 + 2 * (ht - 40) : ht <= 63 ? 97 + 3 * (ht - 51) : ht <= 69 ? 133 + 4 * (ht - 63) :
        ht == 70 ? 160 : 160 + 5 * (ht - 70);
    return ((unsigned long) w * (8 + fr) + 5) / 10;
}

static unsigned char __fastcall__ rand_complexion (unsigned char sp)
{
    static const unsigned char upto[3][3] = { { 27, 74, 100 }, { 97, 99, 100 }, { 90, 100, 100 } };

    return d100_table (upto[sp == HARN_SIN ? 1 : sp == HARN_KHU ? 2 : 0], 3, d100 ());
}

/* Hair and eyes: humans +25 for a fair complexion, -25 for a dark one */
static int __fastcall__ colour_roll (unsigned char sp, unsigned char complexion)
{
    int r = d100 ();

    if (sp == HARN_HUM || sp == HARN_X) {
        r += complexion == HARN_FAIR ? 25 : complexion == HARN_DARK ? -25 : 0;
    }
    return r < 1 ? 1 : r > 100 ? 100 : r;
}

static unsigned char __fastcall__ rand_hair (unsigned char sp, unsigned char complexion)
{
    static const unsigned char upto[3][5] = { { 40, 55, 65, 70, 100 }, { 25, 48, 50, 60, 100 }, { 40, 45, 70, 90, 100 } };

    return d100_table (upto[sp == HARN_SIN ? 1 : sp == HARN_KHU ? 2 : 0], 5, colour_roll (sp, complexion));
}

static unsigned char __fastcall__ rand_eyes (unsigned char sp, unsigned char complexion)
{
    static const unsigned char upto[3][5] = { { 40, 55, 56, 70, 100 }, { 15, 65, 75, 80, 100 }, { 50, 69, 70, 85, 100 } };

    return d100_table (upto[sp == HARN_SIN ? 1 : sp == HARN_KHU ? 2 : 0], 5, colour_roll (sp, complexion));
}

/* ---- The attributes' modifiers (Character 6-10) */

int __fastcall__ harn_species_attr_mod (unsigned char sp, unsigned char sx, unsigned char a)
{
    switch (sp) {
    case HARN_HUM:
        return sx != HARN_F ? 0 : a == HARN_FR ? -3 : a == HARN_AUR ? 2 : 0;
    case HARN_SIN:
        switch (a) {
        case HARN_FR:
            return -2;
        case HARN_STR: case HARN_STA:
            return 1;
        case HARN_CML: case HARN_DEX: case HARN_AGL: case HARN_EYE: case HARN_HRG: case HARN_VOI:
            return 2;
        case HARN_SML: case HARN_MOR:
            return 3;
        case HARN_AUR:
            return 4;
        }
        return 0;
    case HARN_KHU:
        switch (a) {
        case HARN_AUR:
            return -2;
        case HARN_DEX: case HARN_EYE:
            return 1;
        case HARN_HRG: case HARN_SML:
            return 2;
        case HARN_FR: case HARN_WIL:
            return 3;
        case HARN_STR: case HARN_STA:
            return 4;
        }
        return 0;
    }
    return 0;
}

int __fastcall__ harn_frame_attr_mod (unsigned char fr, unsigned char a)
{
    return a == HARN_AGL ? 2 - (int) fr : 0;    /* Scant +2, light +1, medium 0, heavy -1, massive -2 */
}

/* Strength's by weight: 56-85 -4 ... 146-155 0 ... 216-245 +4 (the table's ends past it) */
int __fastcall__ harn_weight_attr_mod (unsigned weight, unsigned char a)
{
    static const unsigned char upto[8] = { 85, 110, 130, 145, 155, 170, 190, 215 };
    unsigned char k;

    if (a != HARN_STR) {
        return 0;
    }
    for (k = 0; k < 8 && weight > upto[k]; ++k) {
    }
    return (int) k - 4;
}

unsigned char __fastcall__ harn_rand_attr (unsigned char sp, unsigned char sx, unsigned char fr, unsigned weight,
                                           unsigned char a)
{
    int v = roll_3d6 () + harn_species_attr_mod (sp, sx, a) + harn_frame_attr_mod (fr, a) +
        harn_weight_attr_mod (weight, a);

    return v < 1 ? 1 : v;
}

/* ---- Medical traits (Character 9): each trait's upper bound for a male's roll, then a female's */

static const unsigned char med_m[HARN_NMEDICAL] = {
    0, 8, 9, 14, 17, 20, 24, 25, 27, 28, 30, 31, 32, 33, 35, 39, 41, 42, 44, 47, 57, 60, 63, 65
};
static const unsigned char med_f[HARN_NMEDICAL] = {
    0, 5, 6, 10, 14, 15, 19, 20, 22, 23, 25, 26, 27, 29, 30, 40, 42, 43, 45, 48, 58, 61, 63, 65
};

/* An attribute less n, at least 1 */
static unsigned __fastcall__ lower (unsigned v, unsigned n)
{
    return v > n ? v - n : 1;
}

/* The trait's effects on the attributes, as far as the book's numbers go (dwarfism's and gigantism's on height
** aren't made: the height's already taken) */
static void __fastcall__ med_effect (harn_char* c, unsigned char m)
{
    unsigned* a = c->att;

    switch (m) {
    case HARN_AMBIDEXTROUS:
        a[HARN_DEX] += d3 ();
        break;
    case HARN_MED_ARM:
        a[HARN_DEX] = lower (a[HARN_DEX], d3 ());
        break;
    case HARN_BIRTHMARK: case HARN_HIRSUTISM: case HARN_POXMARKS: case HARN_SCARS:
        a[HARN_CML] = lower (a[HARN_CML], d3 ());
        break;
    case HARN_EPILEPSY:
        a[HARN_INT] += d3 ();
        break;
    case HARN_MED_EAR:
        a[HARN_HRG] = lower (a[HARN_HRG], d3 ());
        break;
    case HARN_MED_EYE: case HARN_MONOCHROMASIA:
        a[HARN_EYE] = lower (a[HARN_EYE], d3 ());
        break;
    case HARN_MED_LEG:
        a[HARN_AGL] = lower (a[HARN_AGL], a[HARN_AGL] / 2);
        break;
    case HARN_LEFTHANDED:
        a[HARN_DEX] += die (2);
        break;
    case HARN_LEPROSY:
        a[HARN_CML] = lower (a[HARN_CML], dice (2, 6));
        break;
    case HARN_OBESITY:
        a[HARN_WT] = a[HARN_WT] * (150 + random (51)) / 100;
        a[HARN_AGL] = lower (a[HARN_AGL], d3 ());
        break;
    }
}

/* Roll on the table: a trait, multiple (two more rolls), or none */
static void __fastcall__ rand_medical (harn_char* c, unsigned char depth)
{
    const unsigned char* t = c->att[HARN_SX] == HARN_F ? med_f : med_m;
    unsigned char r = d100 (), m, k;

    if (r > 70) {
        return;
    }
    if (r > 65) {                               /* Multiple traits: roll twice more */
        if (depth < 3) {
            rand_medical (c, depth + 1);
            rand_medical (c, depth + 1);
        }
        return;
    }
    for (m = 1; m < HARN_NMEDICAL - 1 && r > t[m]; ++m) {
    }
    for (k = 0; k < HARN_MEDMAX; ++k) {
        if (c->med[k] == m) {
            return;                             /* (Once each) */
        }
        if (c->med[k] == HARN_MED_NONE) {
            c->med[k] = m;
            med_effect (c, m);
            return;
        }
    }
}

/* ---- Mental disorders and sexuality (Character 11: optional) */

static const unsigned char psy_upto[HARN_NPSYCHE] = {
    0, 3, 5, 6, 8, 9, 11, 14, 15, 17, 19, 22, 24, 25, 26, 28, 31, 32, 35, 36, 37, 38, 40, 43, 45, 46, 47, 48, 49, 51,
    54, 56, 57, 59, 60, 61, 63, 64, 65
};

static void __fastcall__ rand_disorder (harn_char* c, unsigned char depth)
{
    unsigned char r = d100 (), p, k, s;

    if (r > 70) {
        return;
    }
    if (r > 65) {
        if (depth < 3) {
            rand_disorder (c, depth + 1);
            rand_disorder (c, depth + 1);
        }
        return;
    }
    for (p = 1; p < HARN_NPSYCHE - 1 && r > psy_upto[p]; ++p) {
    }
    s = d100 ();
    for (k = 0; k < HARN_PSYMAX; ++k) {
        if (c->psy[k] == HARN_PSYCHE_NONE) {
            c->psy[k] = p;
            c->psysev[k] = s <= 60 ? HARN_MILD : s <= 90 ? HARN_MODERATE : HARN_SEVERE;
            return;
        }
    }
}

void __fastcall__ harn_rand_psyche (harn_char* c)
{
    rand_disorder (c, 0);
}

unsigned char harn_rand_sexuality (void)
{
    unsigned char r = d100 ();

    return r <= 90 ? HARN_HETERO : r <= 96 ? HARN_BI : HARN_HOMO;
}

/* ---- harn_create */

void __fastcall__ harn_template (harn_char* t)
{
    memset (t, 0xFF, sizeof *t);
}

/* The template's attribute a, if it has one, else v */
static unsigned __fastcall__ fixed (const harn_char* t, unsigned char a, unsigned v)
{
    return t && t->att[a] != HARN_ANY ? t->att[a] : v;
}

/* The template's byte field at offset off (in harn_char), if it has one, else v */
static unsigned char __fastcall__ fixedb (const harn_char* t, unsigned char off, unsigned char v)
{
    return t && ((const unsigned char*) t)[off] != HARN_ANYB ? ((const unsigned char*) t)[off] : v;
}
#define FIXB(f, v)  fixedb (t, offsetof (harn_char, f), (v))

void __fastcall__ harn_create (harn_char* c, const harn_char* t)
{
    static const unsigned char physical[] = { HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML,
                                              HARN_VOI };
    static const unsigned char sibs[6] = { 25, 50, 70, 85, 95, 100 };
    unsigned char sp, sx, fr, k, a, ht;
    unsigned bd, wt;
    int r;

    memset (c, 0, sizeof *c);
    /* Birth */
    c->att[HARN_SP] = sp = fixed (t, HARN_SP, harn_rand_species ());
    c->att[HARN_SX] = sx = fixed (t, HARN_SX, harn_rand_sex (sp));
    c->att[HARN_BD] = bd = fixed (t, HARN_BD, harn_rand_birthday ());
    c->att[HARN_SS] = harn_sunsign_of (bd >> 8, bd & 0xFF);
    c->culture = FIXB (culture, sp == HARN_SIN ? HARN_SINDARIN_C : sp == HARN_KHU ? HARN_KHUZAN : HARN_FEUDAL);
    c->social = FIXB (social, harn_rand_class (c->culture));
    /* Family: sibling rank (Sindarin and Khuzdul less 20), family size, parent, estrangement, clanhead */
    r = d100 () - (sp == HARN_SIN || sp == HARN_KHU ? 20 : 0);
    c->sibrank = FIXB (sibrank, 1 + d100_table (sibs, 6, r < 1 ? 1 : r));
    c->famsize = FIXB (famsize, c->sibrank + d6 () - 1);
    c->parent = FIXB (parent, rand_parent ());
    c->estrange = FIXB (estrange, rand_estrangement (c->sibrank == 1));
    c->clanhead = FIXB (clanhead, rand_clanhead ());
    /* Appearance */
    c->att[HARN_HT] = ht = fixed (t, HARN_HT, harn_rand_height (sp, sx));
    c->att[HARN_FR] = fr = fixed (t, HARN_FR, harn_rand_frame (sp, sx));
    c->att[HARN_OWT] = c->att[HARN_WT] = wt = fixed (t, HARN_OWT, harn_weight (ht, fr));
    c->att[HARN_CML] = fixed (t, HARN_CML, harn_rand_attr (sp, sx, fr, wt, HARN_CML));
    c->complexion = FIXB (complexion, rand_complexion (sp));
    c->hair = FIXB (hair, rand_hair (sp, c->complexion));
    c->eyes = FIXB (eyes, rand_eyes (sp, c->complexion));
    /* Physical */
    for (k = 0; k < sizeof physical; ++k) {
        a = physical[k];
        c->att[a] = fixed (t, a, harn_rand_attr (sp, sx, fr, wt, a));
    }
    /* Medical: a template's traits, or rolled (with their effects) */
    if (t && t->med[0] != HARN_ANYB) {
        for (k = 0; k < HARN_MEDMAX; ++k) {
            c->med[k] = t->med[k] == HARN_ANYB ? HARN_MED_NONE : t->med[k];
        }
    } else {
        rand_medical (c, 0);
    }
    /* Personality, religion */
    c->att[HARN_INT] = fixed (t, HARN_INT, harn_rand_attr (sp, sx, fr, wt, HARN_INT));
    c->att[HARN_AUR] = fixed (t, HARN_AUR, harn_rand_attr (sp, sx, fr, wt, HARN_AUR));
    c->att[HARN_WIL] = fixed (t, HARN_WIL, harn_rand_attr (sp, sx, fr, wt, HARN_WIL));
    c->att[HARN_MOR] = fixed (t, HARN_MOR, harn_rand_attr (sp, sx, fr, wt, HARN_MOR));
    c->att[HARN_DTY] = fixed (t, HARN_DTY, HARN_NODEITY);
    c->att[HARN_PTY] = fixed (t, HARN_PTY, dice (5, 6));
    c->sexuality = HARN_HETERO;
    /* The automatic skills, at their OMLs */
    for (k = 0; k < HARN_NSKILLS; ++k) {
        if (harn_skills[k].automatic) {
            harn_open_skill (c, k);
        }
    }
    if (c->att[HARN_DTY] != HARN_NODEITY) {     /* Ritual: automatic for the religious (Skills 12) */
        harn_open_skill (c, HARN_RAGR + c->att[HARN_DTY]);
    }
}
