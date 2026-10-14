/*
** harn.c - harn.h's characters as they're played: the names (attributes, species, months, sunsigns, cultures,
** classes, family, appearance, medical traits, disorders, deities), the Skill Data table (Skills 3-4) and a skill's
** base, opening mastery and development (Skills 6), endurance, size, move, the load and the penalties.  harngen.c
** makes a character; harntest.c tests one; harncmbt.c fights; harnheal.c heals.
*/

#include <harn.h>

unsigned char __fastcall__ harn_attr_group (unsigned char a)
{
    return a <= HARN_SS ? HARN_A_BIR : a <= HARN_CML ? HARN_A_APP : a <= HARN_VOI ? HARN_A_PHY : a <= HARN_MOR ? HARN_A_PER :
        a <= HARN_PTY ? HARN_A_REL : HARN_A_NONE;
}

/* A sunsign's signs: its own, then (a cusp) the one before (its first days) or after (its last) */
unsigned char __fastcall__ harn_sunsign_signs (unsigned char ss)
{
    unsigned char s = ss / 3;

    switch (ss % 3) {
    case 0:
        return (s << 4) | (s ? s - 1 : 11);
    case 1:
        return (s << 4) | s;
    }
    return (s << 4) | (s < 11 ? s + 1 : 0);
}

/* ---- The Skill Data table (Skills 3-4) */

#define B(s, n)     ((HARN_S_ ## s << 4) | (n))
#define S3(a, b, c) { HARN_ ## a, HARN_ ## b, HARN_ ## c }
#define PHY         HARN_PHYSICAL
#define COM         HARN_COMMUNICATION
#define REL         HARN_RELIGION
#define CMB         HARN_COMBAT
#define CRA         HARN_CRAFT

const harn_skill harn_skills[HARN_NSKILLS] = {
    { "Acrobatics", S3 (STR, AGL, AGL), { B (NAD, 2), B (HIR, 1) }, 2, 0, PHY, 0 },
    { "Climbing", S3 (STR, DEX, AGL), { B (ULA, 2), B (ARA, 2) }, 4, 1, PHY, 0 },
    { "Condition", S3 (STR, STA, WIL), { B (ULA, 1), B (LAD, 1) }, 5, 1, PHY, 0 },
    { "Dancing", S3 (DEX, AGL, AGL), { B (TAR, 2), B (HIR, 1), B (TAI, 1) }, 2, 0, PHY, 0 },
    { "Jumping", S3 (STR, AGL, AGL), { B (NAD, 2), B (HIR, 2) }, 4, 1, PHY, 0 },
    { "Legerdemain", S3 (DEX, DEX, WIL), { B (SKO, 2), B (TAI, 2), B (TAR, 2) }, 1, 0, PHY, 0 },
    { "Skiing", S3 (STR, DEX, AGL), { B (MAS, 2), B (SKO, 1), B (LAD, 1) }, 1, 0, PHY, 0 },
    { "Stealth", S3 (AGL, HRG, WIL), { B (HIR, 2), B (TAR, 2), B (TAI, 2) }, 3, 1, PHY, 0 },
    { "Swimming", S3 (STA, DEX, AGL), { B (SKO, 1), B (MAS, 3), B (LAD, 3) }, 1, 0, PHY, 0 },
    { "Throwing", S3 (STR, DEX, EYE), { B (HIR, 2), B (TAR, 1), B (NAD, 1) }, 4, 1, PHY, 0 },

    { "Acting", S3 (AGL, VOI, INT), { B (TAR, 1), B (TAI, 1) }, 2, 0, COM, 0 },
    { "Awareness", S3 (EYE, HRG, SML), { B (HIR, 2), B (TAR, 2) }, 4, 1, COM, 0 },
    { "Intrigue", S3 (INT, AUR, WIL), { B (TAI, 1), B (TAR, 1), B (SKO, 1) }, 3, 1, COM, 0 },
    { "Lovecraft", S3 (CML, AGL, VOI), { B (MAS, 1), B (ANG, 1) }, 3, 0, COM, 0 },
    { "Mental Conflict", S3 (AUR, WIL, WIL), { 0 }, 3, 0, COM, 0 },
    { "Musician", S3 (DEX, HRG, HRG), { B (MAS, 1), B (ANG, 1) }, 1, 0, COM, 0 },
    { "Oratory", S3 (CML, VOI, INT), { B (TAR, 1) }, 2, 1, COM, 0 },
    { "Rhetoric", S3 (VOI, INT, WIL), { B (TAI, 1), B (TAR, 1), B (SKO, 1) }, 3, 1, COM, 0 },
    { "Singing", S3 (HRG, VOI, VOI), { B (MAS, 1) }, 3, 1, COM, 0 },
    { "Language", S3 (VOI, INT, WIL), { B (TAI, 1) }, HARN_OML_LANGUAGE, 1, COM, 0 },
    { "Script", S3 (DEX, EYE, INT), { B (TAR, 1), B (TAI, 1) }, HARN_OML_SCRIPT, 0, COM, 1 },

    { "Ritual: Agrik", S3 (VOI, INT, STR), { B (NAD, 2), B (ANG, 1), B (AHN, 1) }, 1, 0, REL, 0 },
    { "Ritual: Halea", S3 (VOI, INT, CML), { B (TAR, 2), B (HIR, 1), B (MAS, 1) }, 1, 0, REL, 0 },
    { "Ritual: Ilvir", S3 (VOI, INT, AUR), { B (SKO, 2), B (TAI, 1), B (ULA, 1) }, 1, 0, REL, 0 },
    { "Ritual: Larani", S3 (VOI, INT, WIL), { B (ANG, 2), B (AHN, 1), B (FEN, 1) }, 1, 0, REL, 0 },
    { "Ritual: Morgath", S3 (VOI, INT, AUR), { B (LAD, 2), B (AHN, 1), B (MAS, 1) }, 1, 0, REL, 0 },
    { "Ritual: Naveh", S3 (VOI, INT, WIL), { B (MAS, 2), B (SKO, 1), B (TAR, 1) }, 1, 0, REL, 0 },
    { "Ritual: Peoni", S3 (VOI, INT, DEX), { B (ARA, 2), B (ANG, 1), B (ULA, 1) }, 1, 0, REL, 0 },
    { "Ritual: Sarajin", S3 (VOI, INT, STR), { B (FEN, 2), B (ARA, 1), B (LAD, 1) }, 1, 0, REL, 0 },
    { "Ritual: Save-K'nor", S3 (VOI, INT, INT), { B (TAI, 2), B (SKO, 1), B (TAR, 1) }, 1, 0, REL, 0 },
    { "Ritual: Siem", S3 (VOI, INT, AUR), { B (HIR, 2), B (FEN, 1), B (ULA, 1) }, 1, 0, REL, 0 },

    { "Initiative", S3 (AGL, WIL, WIL), { 0 }, 4, 1, CMB, 0 },
    { "Unarmed", S3 (STR, DEX, AGL), { B (MAS, 2), B (LAD, 2), B (ULA, 2) }, 4, 1, CMB, 0 },
    { "Riding", S3 (DEX, AGL, WIL), { B (ULA, 1), B (ARA, 1) }, 1, 0, CMB, 0 },
    { "Axe", S3 (STR, STR, DEX), { B (AHN, 1), B (FEN, 1), B (ANG, 1) }, 3, 0, CMB, 0 },
    { "Blowgun", S3 (STA, DEX, EYE), { B (HIR, 2), B (TAR, 1), B (NAD, 1) }, 4, 0, CMB, 0 },
    { "Bow", S3 (STR, DEX, EYE), { B (HIR, 1), B (TAR, 1), B (NAD, 1) }, 2, 0, CMB, 0 },
    { "Club", S3 (STR, STR, DEX), { B (ULA, 1), B (ARA, 1) }, 4, 0, CMB, 0 },
    { "Dagger", S3 (DEX, DEX, EYE), { B (ANG, 2), B (NAD, 2) }, 3, 0, CMB, 0 },
    { "Flail", S3 (DEX, DEX, DEX), { B (HIR, 1), B (TAR, 1), B (NAD, 1) }, 1, 0, CMB, 0 },
    { "Net", S3 (DEX, DEX, EYE), { B (MAS, 1), B (SKO, 1), B (LAD, 1) }, 1, 0, CMB, 0 },
    { "Polearm", S3 (STR, STR, DEX), { B (ANG, 1), B (ARA, 1) }, 2, 0, CMB, 0 },
    { "Shield", S3 (STR, DEX, DEX), { B (ULA, 1), B (LAD, 1), B (MAS, 1) }, 3, 0, CMB, 0 },
    { "Sling", S3 (DEX, DEX, EYE), { B (HIR, 1), B (TAR, 1), B (NAD, 1) }, 1, 0, CMB, 0 },
    { "Spear", S3 (STR, STR, DEX), { B (ARA, 1), B (FEN, 1), B (ULA, 1) }, 3, 0, CMB, 0 },
    { "Sword", S3 (STR, DEX, DEX), { B (ANG, 3), B (AHN, 1), B (NAD, 1) }, 3, 0, CMB, 0 },
    { "Whip", S3 (DEX, DEX, EYE), { B (HIR, 1), B (NAD, 1) }, 1, 0, CMB, 0 },
    { "Dodge", S3 (AGL, AGL, AGL), { 0 }, 5, 1, CMB, 0 },

    { "Agriculture", S3 (STR, STA, WIL), { B (ULA, 2), B (ARA, 2) }, 2, 0, CRA, 1 },
    { "Alchemy", S3 (SML, INT, AUR), { B (SKO, 3), B (TAI, 2), B (MAS, 2) }, 1, 0, CRA, 6 },
    { "Animalcraft", S3 (AGL, VOI, WIL), { B (ULA, 1), B (ARA, 1) }, 1, 0, CRA, 1 },
    { "Astrology", S3 (EYE, INT, AUR), { B (TAR, 1) }, 1, 0, CRA, 3 },
    { "Brewing", S3 (DEX, SML, SML), { B (SKO, 3), B (TAI, 2), B (MAS, 2) }, 2, 0, CRA, 1 },
    { "Ceramics", S3 (DEX, DEX, EYE), { B (ULA, 2), B (ARA, 2) }, 2, 0, CRA, 2 },
    { "Cookery", S3 (DEX, SML, SML), { B (SKO, 1) }, 3, 0, CRA, 0 },
    { "Drawing", S3 (DEX, EYE, EYE), { B (SKO, 1), B (TAI, 1) }, 2, 0, CRA, 1 },
    { "Embalming", S3 (DEX, EYE, SML), { B (SKO, 1), B (ULA, 1) }, 1, 0, CRA, 2 },
    { "Engineering", S3 (DEX, INT, INT), { B (ULA, 2), B (ARA, 2), B (FEN, 1) }, 1, 0, CRA, 3 },
    { "Fishing", S3 (DEX, EYE, WIL), { B (MAS, 2), B (LAD, 2) }, 3, 0, CRA, 0 },
    { "Fletching", S3 (DEX, DEX, EYE), { B (HIR, 2), B (TAR, 1), B (NAD, 1) }, 1, 0, CRA, 1 },
    { "Folklore", S3 (VOI, INT, INT), { B (TAI, 2) }, 3, 0, CRA, 0 },
    { "Foraging", S3 (DEX, SML, INT), { B (ULA, 2), B (ARA, 2) }, 3, 0, CRA, 0 },
    { "Glassworking", S3 (DEX, EYE, WIL), { B (FEN, 2) }, 1, 0, CRA, 3 },
    { "Heraldry", S3 (DEX, EYE, WIL), { B (SKO, 1), B (TAI, 1) }, 1, 0, CRA, 1 },
    { "Herblore", S3 (EYE, SML, INT), { B (ULA, 3), B (ARA, 2) }, 1, 0, CRA, 1 },
    { "Hidework", S3 (DEX, SML, WIL), { B (ULA, 1), B (ARA, 1) }, 2, 0, CRA, 1 },
    { "Hunting", S3 (AGL, SML, INT), { B (ULA, 2), B (ARA, 2) }, 2, 0, CRA, 0 },
    { "Inkcraft", S3 (EYE, SML, INT), { B (SKO, 2), B (TAI, 1) }, 1, 0, CRA, 1 },
    { "Jewelcraft", S3 (DEX, EYE, WIL), { B (FEN, 3), B (TAR, 1), B (ARA, 1) }, 1, 0, CRA, 2 },
    { "Law", S3 (VOI, INT, WIL), { B (TAR, 1), B (TAI, 1) }, 1, 0, CRA, 2 },
    { "Lockcraft", S3 (DEX, EYE, WIL), { B (FEN, 1) }, 1, 0, CRA, 2 },
    { "Lore", S3 (EYE, INT, INT), { B (TAI, 2) }, 2, 0, CRA, 1 },
    { "Masonry", S3 (STR, DEX, INT), { B (ULA, 2), B (ARA, 2) }, 1, 0, CRA, 1 },
    { "Mathematics", S3 (INT, INT, WIL), { B (TAI, 3), B (TAR, 1), B (SKO, 1) }, 1, 0, CRA, 3 },
    { "Metalcraft", S3 (STR, DEX, WIL), { B (FEN, 3), B (AHN, 1), B (ANG, 1) }, 1, 0, CRA, 2 },
    { "Milling", S3 (STR, DEX, SML), { B (ULA, 1) }, 2, 0, CRA, 1 },
    { "Mining", S3 (STR, EYE, INT), { B (ULA, 2), B (ARA, 2), B (FEN, 1) }, 1, 0, CRA, 1 },
    { "Perfumery", S3 (SML, SML, INT), { B (HIR, 1), B (SKO, 1), B (TAR, 1) }, 1, 0, CRA, 1 },
    { "Physician", S3 (DEX, EYE, INT), { B (MAS, 2), B (SKO, 1), B (TAI, 1) }, 1, 0, CRA, 0 },
    { "Piloting", S3 (DEX, EYE, INT), { B (LAD, 3), B (MAS, 1) }, 1, 0, CRA, 6 },
    { "Runecraft", S3 (INT, AUR, AUR), { B (TAI, 2), B (SKO, 1) }, 1, 0, CRA, 6 },
    { "Seamanship", S3 (STR, DEX, AGL), { B (LAD, 3), B (MAS, 1), B (SKO, 1) }, 2, 0, CRA, 1 },
    { "Shipwright", S3 (STR, DEX, INT), { B (LAD, 3), B (MAS, 1) }, 1, 0, CRA, 2 },
    { "Survival", S3 (STR, DEX, INT), { B (ULA, 2), B (ARA, 1) }, 3, 0, CRA, 1 },
    { "Tarotry", S3 (INT, AUR, WIL), { B (TAR, 2), B (TAI, 2), B (SKO, 1), B (HIR, 1) }, 1, 0, CRA, 4 },
    { "Textilecraft", S3 (DEX, DEX, EYE), { B (ULA, 1), B (ARA, 1) }, 2, 0, CRA, 1 },
    { "Timbercraft", S3 (STR, DEX, AGL), { B (ULA, 3), B (ARA, 1) }, 2, 0, CRA, 1 },
    { "Tracking", S3 (EYE, SML, WIL), { B (ULA, 3), B (ARA, 3) }, 2, 0, CRA, 1 },
    { "Weaponcraft", S3 (STR, DEX, WIL), { B (FEN, 3), B (AHN, 1), B (ANG, 1) }, 1, 0, CRA, 4 },
    { "Weatherlore", S3 (INT, EYE, SML), { B (HIR, 1), B (TAR, 1), B (MAS, 1), B (LAD, 1) }, 3, 0, CRA, 1 },
    { "Woodcraft", S3 (DEX, DEX, WIL), { B (ULA, 2), B (ARA, 1), B (LAD, 1) }, 2, 0, CRA, 1 },
};

static const harn_skill* own;                   /* A program's own skills (HARN_NSKILLS on), */
static unsigned char nown;                      /*   how many */

void __fastcall__ harn_own_skills (const harn_skill* t, unsigned char n)
{
    own = t;
    nown = n;
}

const harn_skill* __fastcall__ harn_get_skill_info (unsigned char sk)
{
    if (sk < HARN_NSKILLS) {
        return &harn_skills[sk];
    }
    sk -= HARN_NSKILLS;
    return sk < nown ? &own[sk] : 0;
}

/* ---- The character */

unsigned __fastcall__ harn_get_attr (const harn_char* c, unsigned char a)
{
    return a == HARN_END ? harn_end (c) : a < HARN_NATTRS ? c->att[a] : 0;
}

/* The average of three attributes, rounded half up */
unsigned char __fastcall__ harn_attr_avg (const harn_char* c, unsigned char a1, unsigned char a2, unsigned char a3)
{
    return ((c->att[a1] + c->att[a2] + c->att[a3]) * 2 + 3) / 6;
}

unsigned char __fastcall__ harn_end (const harn_char* c)
{
    return harn_attr_avg (c, HARN_STR, HARN_STA, HARN_WIL);
}

unsigned char __fastcall__ harn_size (const harn_char* c)
{
    static const unsigned upto[9] = { 65, 75, 90, 110, 135, 165, 200, 240, 285 };
    unsigned char k;

    for (k = 0; k < 9 && c->att[HARN_WT] > upto[k]; ++k) {
    }
    return k + 1;
}

unsigned char __fastcall__ harn_move (const harn_char* c)
{
    int m = (int) c->att[HARN_AGL] - harn_get_pp (c);

    return m < 1 ? 1 : m;
}

/* The best bonus of the skill's signs that the character's sunsign has (a cusp: either of its two) */
unsigned char __fastcall__ harn_get_ssb (const harn_char* c, unsigned char sk)
{
    const harn_skill* s = harn_get_skill_info (sk);
    unsigned char ss = harn_sunsign_signs (c->att[HARN_SS]), k, b, best = 0;

    for (k = 0; k < 4; ++k) {
        b = s->ssb[k];
        if (b && ((b >> 4) == (ss >> 4) || (b >> 4) == (ss & 15)) && (b & 15) > best) {
            best = b & 15;
        }
    }
    return best;
}

/* SB: the three attributes' average and the sunsign's bonus (Condition's SB is Endurance: Skills 9) */
unsigned char __fastcall__ harn_get_sb (const harn_char* c, unsigned char sk)
{
    const harn_skill* s = harn_get_skill_info (sk);

    if (sk == HARN_CND) {
        return harn_end (c);
    }
    return harn_attr_avg (c, s->att[0], s->att[1], s->att[2]) + harn_get_ssb (c, sk);
}

/* OML: SB times its multiplier; a native script 70 + SB, a native tongue 50-80 + SB by social class (Skills 11) */
unsigned char __fastcall__ harn_get_oml (const harn_char* c, unsigned char sk)
{
    unsigned char m = harn_get_skill_info (sk)->oml, sb = harn_get_sb (c, sk);

    if (m == HARN_OML_SCRIPT) {
        return 70 + sb;
    }
    if (m == HARN_OML_LANGUAGE) {
        return (c->social <= HARN_SERF ? 50 : c->social == HARN_NOBLE ? 70 : 60) + sb;
    }
    return m * sb;
}

void __fastcall__ harn_open_skill (harn_char* c, unsigned char sk)
{
    if (!c->ml[sk]) {
        c->ml[sk] = harn_get_oml (c, sk);
    }
}

unsigned char __fastcall__ harn_develop (harn_char* c, unsigned char sk)
{
    unsigned char ml = c->ml[sk], sb = harn_get_sb (c, sk);

    if (ml >= 100 + sb) {
        return HARN_DR_MAXED;
    }
    if (d100 () + sb > ml) {
        c->ml[sk] = ml + 1;
        return HARN_DR_IMPROVED;
    }
    return HARN_DR_NOT_IMPROVED;
}

unsigned char __fastcall__ harn_apply_dr (harn_char* c, unsigned char sk)
{
    unsigned char r;

    if (!c->dp) {
        return HARN_DR_NO_DP;
    }
    if ((r = harn_develop (c, sk)) != HARN_DR_MAXED) {
        --c->dp;
    }
    return r;
}

/* ---- The load and the penalties (Skills 6) */

const harn_equip harn_equipment[1] = { { 0, 0, 0, 0xFF, 0 } };

unsigned __fastcall__ harn_get_load (const harn_char* c)
{
    unsigned long tenths = 0;
    unsigned char k;

    for (k = 0; k < c->neq; ++k) {
        tenths += c->eq[k]->wt;
    }
    return tenths / 10;
}

unsigned char __fastcall__ harn_get_ip (const harn_char* c)
{
    unsigned char ip = 0, k;

    for (k = 0; k < c->ninj; ++k) {
        ip += c->inj[k].il;
    }
    return ip;
}

unsigned char __fastcall__ harn_get_fp (const harn_char* c)
{
    return c->fl;
}

unsigned char __fastcall__ harn_get_up (const harn_char* c)
{
    return harn_get_ip (c) + harn_get_fp (c);
}

/* EP: the load over endurance, rounded (Combat 2: "rounded off to the nearest whole number") */
unsigned char __fastcall__ harn_get_ep (const harn_char* c)
{
    unsigned char e = harn_end (c);

    return (harn_get_load (c) * 2 + e) / (2 * e);
}

unsigned char __fastcall__ harn_get_pp (const harn_char* c)
{
    return harn_get_up (c) + harn_get_ep (c);
}
