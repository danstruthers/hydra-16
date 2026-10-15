/*
** harncmbt.c - harn.h's combat (Combat 1-18, the Combat Tables): the melee and missile attack tables, the strike
** location table, the injury table, armour's materials and its protection at a location (Combat 5), the best aspect
** of a weapon, and a wound taken (the kill roll, shock, bleeding, amputation, a limb made unusable: Combat 13-14),
** and a fainted character's waking.
** harnarm.c has the armour pieces, harnwpn.c the weapons.
*/

#include <harn.h>

/* ---- The attack tables: by defence, the attacker's level (CF, MF, MS, CS), the defender's */

#define MS_     HARN_R_MISS
#define BLK     HARN_R_BLOCK
#define AH(n)   (HARN_R_AH | (n))
#define DH(n)   (HARN_R_DH | (n))
#define BH(n)   (HARN_R_BH | (n))
#define AF      HARN_R_AF
#define DF      HARN_R_DF
#define BF      HARN_R_BF
#define AS      HARN_R_AS
#define DS      HARN_R_DS
#define BS      HARN_R_BS
#define DTA     HARN_R_DTA
#define WLD     HARN_R_WILD
#define MH(n)   (HARN_R_AH | (n))               /* (A missile strike: the attacker's) */

static const unsigned char melee[3][4][4] = {
    {   /* Block */
        { BF, AF, DTA, DTA }, { DF, BLK, DTA, DTA }, { AH (2), AH (1), BLK, DTA }, { AH (3), AH (2), AH (1), BLK } },
    {   /* Counterstrike */
        { BF, AF, DH (2), DH (3) }, { DF, BLK, DH (1), DH (2) }, { AH (3), AH (2), BH (1), DH (1) },
        { AH (4), AH (3), AH (1), BH (2) } },
    {   /* Dodge */
        { BS, AS, DTA, DTA }, { DS, MS_, MS_, DTA }, { AH (2), AH (1), MS_, MS_ }, { AH (3), AH (2), AH (1), MS_ } },
};
static const unsigned char ignore[4] = { DTA, AH (1), AH (3), AH (4) };    /* (No defender's roll) */

unsigned char __fastcall__ harn_melee_result (unsigned char defence, unsigned char att, unsigned char def)
{
    att &= 3;
    return defence == HARN_IGNORE ? ignore[att] : melee[defence < 3 ? defence : 0][att][def & 3];
}

static const unsigned char missile[2][4][4] = {
    {   /* Block */
        { WLD, WLD, WLD, WLD }, { MS_, MS_, MS_, MS_ }, { MH (2), MH (1), BLK, BLK }, { MH (3), MH (2), MH (1), BLK } },
    {   /* Dodge */
        { WLD, WLD, WLD, WLD }, { MS_, MS_, MS_, MS_ }, { MH (2), MH (1), MS_, MS_ }, { MH (3), MH (2), MH (1), MS_ } },
};
static const unsigned char mignore[4] = { WLD, MS_, MH (2), MH (3) };

unsigned char __fastcall__ harn_missile_result (unsigned char defence, unsigned char att, unsigned char def)
{
    att &= 3;
    if (defence == HARN_IGNORE || defence == HARN_COUNTER) {
        return mignore[att];
    }
    return missile[defence == HARN_DODGE][att][def & 3];
}

/* ---- Strike locations: each location's upper bound for a high, mid and low strike (0: not reachable) */

static const unsigned char where[3][HARN_NLOCATIONS] = {
    { 15, 30, 45, 57, 69, 73, 81, 85, 95, 100, 0, 0, 0, 0, 0, 0 },
    { 5, 10, 15, 27, 33, 35, 39, 43, 60, 70, 74, 80, 88, 90, 96, 100 },
    { 0, 0, 0, 0, 0, 0, 6, 12, 19, 29, 35, 49, 70, 78, 92, 100 },
};
static const char* const locations[HARN_NLOCATIONS] = {
    "skull", "face", "neck", "shoulder", "upper arm", "elbow", "forearm", "hand", "thorax", "abdomen", "groin", "hip",
    "thigh", "knee", "calf", "foot"
};

unsigned char __fastcall__ harn_is_limb (unsigned char loc)
{
    loc = HARN_LOC (loc);
    return loc >= HARN_SHOULDER && loc <= HARN_HAND ? 1 : loc >= HARN_HIP ? 2 : 0;
}

/* A limb: an odd roll the left, an even one the right */
unsigned char __fastcall__ harn_strike_location (unsigned char zone)
{
    const unsigned char* t = where[zone < 3 ? zone : HARN_MID];
    unsigned char r = d100 (), k;

    for (k = 0; k < HARN_NLOCATIONS - 1 && (!t[k] || r > t[k]); ++k) {
    }
    return harn_is_limb (k) && !(r & 1) ? k | HARN_RIGHT : k;
}

const char* __fastcall__ harn_location_name (unsigned char loc)
{
    loc = HARN_LOC (loc);
    return loc < HARN_NLOCATIONS ? locations[loc] : "";
}

/* ---- The injury table: by location, its injury at effective impact 1+, 5+, 9+, 13+, 17+ */

#define M1      ((HARN_MINOR << 4) | 1)
#define S2      ((HARN_SERIOUS << 4) | 2)
#define S3      ((HARN_SERIOUS << 4) | 3)
#define G4      ((HARN_GRIEVOUS << 4) | 4)
#define G5      ((HARN_GRIEVOUS << 4) | 5)
#define K4      ((HARN_KILL << 4) | 4)
#define K5      ((HARN_KILL << 4) | 5)
#define U       HARN_INJ_ROLL
#define T       HARN_INJ_AMPUTATE

static const unsigned char injuries[HARN_NLOCATIONS][5] = {
    { M1, S2, S3, K4, K5 },                     /* Skull */
    { M1, S2, S3, G4, K5 },                     /* Face */
    { M1, S2, S3, K4, K5 | T },                 /* Neck */
    { U | M1, U | S2, U | S3, U | G4, U | K4 }, /* Shoulder */
    { U | M1, U | M1, U | S2, U | S3, U | G4 | T },     /* Upper arm */
    { U | M1, U | S2, U | S3, U | G4, U | G5 | T },     /* Elbow */
    { U | M1, U | M1, U | S2, U | S3, U | G4 | T },     /* Forearm */
    { U | M1, U | S2, U | S3, U | G4, U | G5 | T },     /* Hand */
    { M1, S2, S3, G4, K5 },                     /* Thorax */
    { M1, S2, S3, K4, K5 },                     /* Abdomen */
    { M1, S2, S3, G4, G5 | T },                 /* Groin */
    { U | M1, U | S2, U | S3, U | G4, U | K4 }, /* Hip */
    { U | M1, U | S2, U | S3, U | G4, U | K4 | T },     /* Thigh */
    { U | M1, U | S2, U | S3, U | G4, U | G5 | T },     /* Knee */
    { U | M1, U | M1, U | S2, U | S3, U | G4 | T },     /* Calf */
    { U | M1, U | S2, U | S3, U | G4, U | G5 | T },     /* Foot */
};

unsigned char __fastcall__ harn_injury_of (unsigned char loc, int impact)
{
    if (impact < 1) {
        return 0;
    }
    return injuries[HARN_LOC (loc) % HARN_NLOCATIONS][impact >= 17 ? 4 : (impact - 1) / 4];
}

const char* __fastcall__ harn_severity_name (unsigned char sev)
{
    static const char* const s[] = { "", "minor", "serious", "grievous", "mortal" };

    return s[sev <= HARN_KILL ? sev : 0];
}

/* ---- Armour (Combat 4-5) */

const unsigned char harn_protection[HARN_NMATERIALS][4] = {
    { 1, 1, 1, 1 }, { 5, 3, 2, 4 }, { 2, 4, 3, 3 }, { 4, 5, 4, 3 },     /* Cloth, quilt, leather, kurbul */
    { 3, 6, 4, 3 }, { 2, 8, 5, 1 }, { 5, 9, 4, 5 }, { 6, 10, 6, 2 },    /* Ring, mail, scale, plate */
};

unsigned char __fastcall__ harn_armour_at (const harn_armour* const* worn, unsigned char n, unsigned char loc,
                                          unsigned char aspect, unsigned char front)
{
    const harn_armour* a;
    unsigned bit = 1u << HARN_LOC (loc);
    unsigned char k, sum = 0;

    for (k = 0; k < n; ++k) {
        a = worn[k];
        if ((a->cover & bit) && (a->side == HARN_BOTH || (a->side == HARN_FRONT) == (front != 0))) {
            sum += harn_protection[a->material][aspect & 3];
        }
    }
    return sum;
}

unsigned char __fastcall__ harn_best_aspect (const harn_weapon* w)
{
    unsigned char a, best = HARN_BLUNT;

    for (a = 1; a < 3; ++a) {
        if (w->imp[a] > w->imp[best]) {
            best = a;
        }
    }
    return best;
}

/* ---- A wound (Combat 13-14) */

unsigned char __fastcall__ harn_kill_roll (const harn_char* c, unsigned char n)
{
    return dice (n, 6) > harn_end (c);
}

unsigned char __fastcall__ harn_wound (harn_char* c, unsigned char loc, unsigned char aspect, int impact)
{
    unsigned char i = harn_injury_of (loc, impact), sev, il, limb, w = 0;
    harn_injury* j;

    if (!i) {
        return 0;
    }
    sev = HARN_INJ_SEV (i);
    il = HARN_INJ_LEVEL (i);
    w = HARN_W_INJURED;
    if (sev == HARN_KILL) {                     /* Kn: n d6 over Endurance is death; else it's Gn */
        if (harn_kill_roll (c, il)) {
            c->state = HARN_DEAD;
            return w | HARN_W_DEAD;
        }
        sev = HARN_GRIEVOUS;
    }
    if (c->ninj < HARN_INJMAX) {
        j = &c->inj[c->ninj++];
    } else {
        j = &c->inj[HARN_INJMAX - 1];           /* (No room: the last injury takes its levels) */
        il += j->il;
    }
    j->loc = loc;
    j->aspect = aspect;
    j->sev = sev;
    j->il = il;
    j->hr = 0;
    j->flags = 0;
    limb = harn_is_limb (loc);
    if (sev == HARN_GRIEVOUS && aspect != HARN_FIRE) {
        j->flags |= HARN_IF_BLEEDER;            /* A bloodloss point a minute till it's treated */
        w |= HARN_W_BLEEDING;
    }
    if (sev == HARN_GRIEVOUS && aspect == HARN_EDGE && (i & HARN_INJ_AMPUTATE) &&
        (unsigned) dice (il, 6) > c->att[HARN_WT] / 10) {
        j->il = 5;                              /* Severed: G5, and an automatic fumble or stumble */
        j->flags |= HARN_IF_AMPUTATED | HARN_IF_LIMB;
        w |= HARN_W_AMPUTATED | HARN_W_LIMB;
    } else if (limb && sev == HARN_GRIEVOUS) {
        j->flags |= HARN_IF_LIMB;               /* Unusable till it heals to M1 */
        w |= HARN_W_LIMB;
    }
    if ((i & HARN_INJ_ROLL) || (w & HARN_W_LIMB)) {
        w |= limb == 1 ? HARN_W_FUMBLE : limb == 2 ? HARN_W_STUMBLE : 0;
    }
    if (c->state == HARN_CONSCIOUS && harn_shock_roll (c)) {
        c->state = HARN_UNCONSCIOUS;
        w |= HARN_W_DOWN;
    }
    return w;
}

/* A fainted character's turn: a shock roll not over Endurance wakes it, then one more decides whether it's in
** shock (Skills 22, Combat 14) */
unsigned char __fastcall__ harn_wake (harn_char* c)
{
    if (c->state == HARN_UNCONSCIOUS && !harn_shock_roll (c)) {
        if (harn_shock_roll (c)) {
            c->state = HARN_SHOCK;
            c->shockhr = 5;
        } else {
            c->state = HARN_CONSCIOUS;
        }
    }
    return c->state;
}
