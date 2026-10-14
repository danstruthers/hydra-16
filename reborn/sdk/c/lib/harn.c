/*
** harn.c - harn.h's characters as they're played: danlang's lib/harn.dl's names (the attributes', species', sexes',
** months', sunsigns', frames'), its skills (harn.skills) and a skill's base, opening mastery and development, the
** volatiles, the load and the penalties, endurance and size.  harngen.c makes a character.
*/

#include <harn.h>

static const char* const attrs[HARN_NATTRS] = {
    "Species", "Sex", "Birthday", "Sunsign", "Height (inches)", "Frame", "Optimal Weight (pounds)",
    "Current Weight (pounds)", "Comeliness", "Strength", "Stamina", "Dexterity", "Agility", "Speed", "Eyesight",
    "Hearing", "Smell/Taste", "Touch", "Voice", "Intelligence", "Aura", "Will", "Morality", "Piety", "Deity",
    "Endurance"
};

const char* __fastcall__ harn_attr_name (unsigned char a)
{
    return a < HARN_NATTRS ? attrs[a] : "";
}

unsigned char __fastcall__ harn_attr_group (unsigned char a)
{
    if (a <= HARN_SS) {
        return HARN_BIR;
    }
    if (a == HARN_HT || a == HARN_OWT || a == HARN_FR || a == HARN_CML) {
        return HARN_APP;
    }
    if (a >= HARN_STR && a <= HARN_VOI) {
        return HARN_PHY;
    }
    if (a >= HARN_INT && a <= HARN_MOR) {
        return HARN_PER;
    }
    if (a == HARN_DTY || a == HARN_PTY) {
        return HARN_REL;
    }
    return HARN_NONE;                           /* (:wt and :end, as harn.dl has them) */
}

static const char* const species[] = { "Human", "Sindarin", "Khuzdul", "Other" };
static const char* const sexes[] = { "Female", "Male", "Neuter", "Asexual", "Hermaphrodite", "Not-applicable" };
static const char* const sexualities[] = { "Heterosexual", "Homosexual", "Bi-sexual", "Not-applicable" };
static const char* const months[] = {
    "Nuzyael", "Peonu", "Kelen", "Nolus", "Larane", "Agrazhar", "Azura", "Halane", "Savor", "Ilvin", "Navek",
    "Morgat"
};
static const char* const signs[] = {
    "Ulandus", "Aralius", "Feniri", "Ahnu", "Angberelius", "Nadai", "Hirin", "Tarael", "Tai", "Skorus", "Masura",
    "Lado"
};
static const char* const frames[] = { "Scant", "Light", "Medium", "Heavy", "Massive" };

const char* __fastcall__ harn_species_name (unsigned char sp)
{
    return species[sp <= HARN_X ? sp : HARN_X];
}

const char* __fastcall__ harn_sex_name (unsigned char sx)
{
    return sexes[sx <= HARN_SX_X ? sx : HARN_SX_X];
}

const char* __fastcall__ harn_sexuality_name (unsigned char s)
{
    return sexualities[s <= HARN_NA ? s : HARN_NA];
}

const char* __fastcall__ harn_month_name (unsigned char m)
{
    return m >= 1 && m <= 12 ? months[m - 1] : "";
}

const char* __fastcall__ harn_sign_name (unsigned char s)
{
    return s < 12 ? signs[s] : "";
}

const char* __fastcall__ harn_frame_name (unsigned char fr)
{
    return fr <= HARN_FR_V ? frames[fr] : "";
}

/* A sunsign's signs: the sign alone, then each cusp's two (harn.sunsign-name's order: Ulandus-Lado Cusp, Ulandus,
** Ulandus-Aralius Cusp, Aralius-Ulandus Cusp, Aralius ... Lado, Lado-Ulandus Cusp).  So the sunsign before sign
** s's (2 + 3s, from Ulandus at 1) is the cusp s and s - 1, the one after it s and s + 1 */
unsigned char __fastcall__ harn_sunsign_signs (unsigned char ss)
{
    unsigned char s;

    if (ss == HARN_ULA_LAD) {
        return (HARN_S_ULA << 4) | HARN_S_LAD;
    }
    if (ss == HARN_LAD_ULA) {
        return (HARN_S_LAD << 4) | HARN_S_ULA;
    }
    s = (ss - 1) / 3;                           /* ULA 1, ULA_ARA 2, ARA_ULA 3, ARA 4 ...: (ss - 1) / 3 the sign, */
    switch ((ss - 1) % 3) {                     /*   its remainder which of its three */
    case 0:
        return (s << 4) | s;                    /* The sign alone */
    case 1:
        return (s << 4) | (s + 1);              /* Its cusp with the next */
    default:
        return ((s + 1) << 4) | s;              /* The next's cusp with it */
    }
}

/* "Ulandus-Lado Cusp", "Ulandus" ... (harn.sunsign-name's): made from the signs' names */
const char* __fastcall__ harn_sunsign_name (unsigned char ss)
{
    static char b[32];
    unsigned char s = harn_sunsign_signs (ss), i = 0;
    const char* p;

    if (ss >= HARN_NSUNSIGNS) {
        return "";
    }
    for (p = signs[s >> 4]; *p; ) {
        b[i++] = *p++;
    }
    if ((s >> 4) != (s & 15)) {
        b[i++] = '-';
        for (p = signs[s & 15]; *p; ) {
            b[i++] = *p++;
        }
        for (p = " Cusp"; *p; ) {
            b[i++] = *p++;
        }
    }
    b[i] = 0;
    return b;
}

/* ---- Skills: harn.skills, each one's attributes, sunsign bonuses, opening multiplier, whether it's automatic,
** specialties */

#define B(s, n)     ((HARN_S_ ## s << 4) | (n))

const harn_skill harn_skills[HARN_NSKILLS] = {
    { "acr", "Acrobatics",  { HARN_STR, HARN_AGL, HARN_AGL }, { B (NAD, 2), B (HIR, 1) }, 1, 0, "tumbling, trapeze, balancing" },
    { "cli", "Climbing",    { HARN_STR, HARN_DEX, HARN_AGL }, { B (ULA, 2), B (ARA, 2) }, 4, 1, "" },
    { "cnd", "Condition",   { HARN_STR, HARN_STA, HARN_WIL }, { B (ULA, 1), B (LAD, 1) }, 5, 1, "" },
    { "ddg", "Dodge",       { HARN_AGL, HARN_AGL, HARN_SPD }, { B (HIR, 1), B (TAR, 1), B (TAI, 1) }, 5, 1, "" },
    { "ini", "Initiative",  { HARN_AGL, HARN_SPD, HARN_WIL }, { B (AHN, 1), B (ANG, 1), B (NAD, 1) }, 5, 1, "" },
    { "dan", "Dancing",     { HARN_DEX, HARN_AGL, HARN_AGL }, { B (TAR, 2), B (HIR, 1), B (TAI, 1) }, 2, 0, "Different dance styles or dances" },
    { "jmp", "Jumping",     { HARN_STR, HARN_AGL, HARN_SPD }, { B (NAD, 2), B (HIR, 2) }, 4, 1, "high, long" },
    { "leg", "Legerdemain", { HARN_DEX, HARN_TCH, HARN_TCH }, { B (SKO, 2), B (TAI, 2), B (TAR, 2) }, 1, 0, "purse-cutting, prestidigitation, pocket-picking" },
    { "mob", "Mobility",    { HARN_AGL, HARN_SPD, HARN_SPD }, { B (HIR, 1), B (TAR, 1) }, 5, 1, "" },
    { "sea", "Seamanship",  { HARN_STR, HARN_DEX, HARN_AGL }, { B (LAD, 3), B (SKO, 1), B (MAS, 1) }, 1, 0, "" },
    { "ski", "Skiing",      { HARN_STR, HARN_DEX, HARN_AGL }, { B (LAD, 1), B (SKO, 1), B (MAS, 2) }, 1, 0, "" },
    { "stl", "Stealth",     { HARN_AGL, HARN_TCH, HARN_WIL }, { B (HIR, 2), B (TAR, 2), B (TAI, 2) }, 3, 1, "" },
    { "swi", "Swimming",    { HARN_STR, HARN_STA, HARN_AGL }, { B (SKO, 1), B (MAS, 3), B (LAD, 3) }, 1, 0, "freestyle, breast, back, butterfly, diving" },
    { "thr", "Throwing",    { HARN_STR, HARN_DEX, HARN_EYE }, { B (HIR, 2), B (TAR, 1), B (NAD, 1) }, 4, 1, "" },
    { "una", "Unarmed",     { HARN_STR, HARN_DEX, HARN_AGL }, { B (MAS, 2), B (LAD, 2), B (ULA, 2), B (AHN, 1), B (NAD, 1) }, 3, 1, "Martial arts styles" },

    { "rid", "Riding",      { HARN_DEX, HARN_AGL, HARN_WIL }, { B (ULA, 1), B (ARA, 1) }, 1, 0, "horse, pony, dragon" },
    { "shd", "Shields",     { HARN_STR, HARN_DEX, HARN_AGL }, { B (LAD, 3), B (SKO, 1), B (MAS, 1) }, 3, 0, "tower, kite, knight, round, buckler" },
    { "kni", "Knives",      { HARN_DEX, HARN_TCH, HARN_EYE }, { B (ANG, 2), B (NAD, 2) }, 3, 0, "dagger, knife, taburi, keltan" },
    { "ssw", "Shortswords", { HARN_STR, HARN_DEX, HARN_DEX }, { B (ANG, 3), B (NAD, 1), B (AHN, 1) }, 3, 0, "longknife, shortsword, mankar, falchion" },
    { "lsw", "Longswords",  { HARN_STR, HARN_STR, HARN_DEX }, { B (ANG, 3), B (NAD, 1), B (AHN, 1) }, 3, 0, "broadsword, mang, estoc, bastard-sword, battlesword" },
    { "clu", "Clubs",       { HARN_STR, HARN_DEX, HARN_DEX }, { B (ULA, 1), B (ARA, 1) }, 4, 0, "club, mace, morningstar, maul" },
    { "axe", "Axes",        { HARN_STR, HARN_DEX, HARN_DEX }, { B (AHN, 1), B (FEN, 1), B (ANG, 1) }, 2, 0, "sickle, shorkana, hatchet, handaxe" },
    { "bax", "Battleaxes",  { HARN_STR, HARN_STR, HARN_DEX }, { B (AHN, 1), B (FEN, 1), B (ANG, 1), B (NAD, 1) }, 2, 0, "battleaxe, warhammer" },
    { "fla", "Flails",      { HARN_DEX, HARN_DEX, HARN_TCH }, { B (HIR, 1), B (TAR, 1), B (NAD, 1) }, 1, 0, "nachakas, grainflail, ball-and-chain, warflail" },
    { "whi", "Whips",       { HARN_DEX, HARN_DEX, HARN_TCH }, { B (HIR, 1), B (NAD, 1) }, 1, 0, "whip, cat-o-nine-tails, isagra" },
    { "net", "Nets",        { HARN_DEX, HARN_TCH, HARN_TCH }, { B (MAS, 1), B (SKO, 1), B (LAD, 1) }, 1, 0, "net, blanket" },
    { "sta", "Staves",      { HARN_STR, HARN_DEX, HARN_EYE }, { B (ARA, 1), B (FEN, 1), B (ULA, 1) }, 2, 0, "javelin, staff, spear, trident, lance" },
    { "pol", "Polearms",    { HARN_STR, HARN_DEX, HARN_DEX }, { B (ANG, 1), B (ARA, 1) }, 2, 0, "glaive, bill, poleaxe, falcastra, pike" },

    { "bow", "Bows",        { HARN_STR, HARN_DEX, HARN_EYE }, { B (HIR, 1), B (TAR, 1), B (NAD, 1) }, 2, 0, "longbow, shortbow" },
    { "blo", "Blowgun",     { HARN_STA, HARN_EYE, HARN_TCH }, { B (HIR, 2), B (TAR, 1), B (NAD, 1) }, 4, 0, "" },
    { "slg", "Slings",      { HARN_DEX, HARN_DEX, HARN_TCH }, { B (HIR, 1), B (TAR, 1), B (NAD, 1) }, 1, 0, "sling, staffsling" },
    { "xbo", "Crossbow",    { HARN_DEX, HARN_EYE, HARN_TCH }, { B (HIR, 1), B (ULA, 1), B (FEN, 1) }, 3, 0, "" },

    { "act", "Acting",      { HARN_AGL, HARN_VOI, HARN_INT }, { B (TAR, 1), B (TAI, 1) }, 1, 0, "theater, politics" },
    { "awa", "Awareness",   { HARN_EYE, HARN_HRG, HARN_SMT }, { B (HIR, 2), B (TAR, 2) }, 4, 0, "" },
    { "ntr", "Intrigue",    { HARN_INT, HARN_AUR, HARN_WIL }, { B (TAI, 1), B (TAR, 1), B (SKO, 1) }, 3, 1, "" },
    { "lov", "Lovecraft",   { HARN_CML, HARN_TCH, HARN_VOI }, { B (MAS, 1), B (ANG, 1) }, 3, 0, "" },
    { "mus", "Musiciain",   { HARN_DEX, HARN_HRG, HARN_TCH }, { B (MAS, 1), B (ANG, 1) }, 1, 0, "Each instrument or classes, i.e. stringed, wind, etc." },
    { "ora", "Oratory",     { HARN_CML, HARN_VOI, HARN_INT }, { B (TAR, 1) }, 2, 1, "" },
    { "rhe", "Rhetoric",    { HARN_VOI, HARN_INT, HARN_WIL }, { B (TAI, 1), B (TAR, 1), B (SKO, 1) }, 3, 1, "" },
    { "sng", "Singing",     { HARN_HRG, HARN_VOI, HARN_VOI }, { B (MAS, 1) }, 3, 1, "opera" },
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

/* The average of three attributes, rounded half up (harn.attr-avg: trunc (0.5 + avg)) */
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
    static const unsigned char upto[9] = { 65, 75, 90, 110, 135, 165, 200, 240, 285 };
    unsigned wt = c->att[HARN_WT];
    unsigned char k;

    for (k = 0; k < 9 && wt > upto[k]; ++k) {
    }
    return k + 1;
}

/* The best bonus of the skill's signs that the character's sunsign has (harn.get-ssb) */
unsigned char __fastcall__ harn_get_ssb (const harn_char* c, unsigned char sk)
{
    const harn_skill* s = harn_get_skill_info (sk);
    unsigned char ss = harn_sunsign_signs (c->att[HARN_SS]), k, b, best = 0;

    for (k = 0; k < 5; ++k) {
        b = s->ssb[k];
        if (b && ((b >> 4) == (ss >> 4) || (b >> 4) == (ss & 15)) && (b & 15) > best) {
            best = b & 15;
        }
    }
    return best;
}

unsigned char __fastcall__ harn_get_sb (const harn_char* c, unsigned char sk)
{
    const harn_skill* s = harn_get_skill_info (sk);

    return harn_attr_avg (c, s->att[0], s->att[1], s->att[2]) + harn_get_ssb (c, sk);
}

unsigned __fastcall__ harn_get_oml (const harn_char* c, unsigned char sk)
{
    return harn_get_skill_info (sk)->oml * harn_get_sb (c, sk);
}

void __fastcall__ harn_open_skill (harn_char* c, unsigned char sk)
{
    c->ml[sk] = harn_get_oml (c, sk);
}

unsigned char __fastcall__ harn_apply_dr (harn_char* c, unsigned char sk)
{
    unsigned char ml = c->ml[sk], si = ml / 10;

    if (!c->dp) {
        return HARN_DR_NO_DP;
    }
    if (100 + si <= ml) {
        return HARN_DR_MAXED;
    }
    --c->dp;
    if (si + d100 () > ml) {
        c->ml[sk] = ml + 1;
        return HARN_DR_IMPROVED;
    }
    return HARN_DR_NOT_IMPROVED;
}

/* ---- Equipment and the penalties */

const harn_equip harn_equipment[1] = { { 0, 0, 0, 0xFF, 0 } };     /* (harn.dl's: one, made with nothing) */

unsigned __fastcall__ harn_get_load (const harn_char* c)
{
    unsigned load = 0;
    unsigned char k;

    for (k = 0; k < c->neq; ++k) {
        load += c->eq[k]->wt;
    }
    return load;
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
    return c->fp;
}

unsigned char __fastcall__ harn_get_up (const harn_char* c)
{
    return harn_get_fp (c) + harn_get_ip (c);
}

unsigned char __fastcall__ harn_get_ep (const harn_char* c)
{
    return harn_get_load (c) / harn_end (c);
}

unsigned char __fastcall__ harn_get_pp (const harn_char* c)
{
    return harn_get_up (c) + harn_get_ep (c);
}
