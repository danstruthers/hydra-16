/*
** harn.h - HârnMaster (third edition, 3.5) characters and their rules, in C: the C SDK's version of danlang's
** lib/harn.dl (which follows the same book), with dice.h's dice.  harn.dl's names, harn.NAME as harn_NAME (a - as
** _), for what it has; the rest by the book's own names.
**
** A character (harn_char) is its attributes, by number (HARN_SP ... HARN_PTY, in the book's order of generation:
** harn_attr_name has each one's name), its birth and family (culture, social class, sibling rank, parent,
** estrangement, clanhead), its appearance's colours, its medical traits and mental disorders, its skills' mastery
** levels (ml, by skill: HARN_ACR ...; 0 not opened), and its volatiles: fatigue, bloodloss, development points, its
** equipment (its load), its injuries, whether it's conscious.  harn_create makes one at random, as the book's
** Character article does (harn_template: a template that fixes any part of it).
**
** Skills (harn_skills: the book's Skill Data table): each one's three attributes, its sunsign bonuses, its opening
** multiplier (OML = SB times it), whether it's automatic, which penalty it's subject to.  SB (harn_get_sb) is the
** three attributes' average, rounded, and the best bonus of the character's sunsign (a cusp has two signs); SI is
** ML / 10.  A program's own skills go after the book's (harn_own_skills).
**
** Tests (Skills 5, 21-22): harn_test (d100 against an EML: CF, MF, MS, CS), harn_eml (ML less five times the
** penalty the skill is subject to: universal or physical), attribute tests, the shock, stumble and fumble rolls,
** development (d100 and SB over ML: ML + 1).
**
** Combat (Combat 1-18, the Combat Tables): the melee and missile attack tables, the strike location table (aiming
** high, mid, low), the injury table, armour (its materials' protection against blunt, edge, point and fire, its
** pieces' coverage), the weapons, and harn_wound: an effective impact on a body location made an injury (the kill
** roll, the shock roll, bleeding, a fumble or stumble roll).  Healing (Physician 2-4): treatment (a healing rate),
** the five-day healing roll, shock recovery, bloodloss.
**
** Left out: occupations and their skills, contacts, aging, psionics, magic, religion's rituals, mounted combat.
*/

#ifndef _HARN_H
#define _HARN_H

#include <dice.h>

typedef struct harn_char harn_char;

#define HARN_ANY        0xFFFF                  /* A template's attribute that's rolled */
#define HARN_ANYB       0xFF                    /*   and a template's byte field */

/* ---- Attributes (Character 2-12, in the order they're generated) */

enum {
    HARN_SP, HARN_SX, HARN_BD, HARN_SS,                                 /* Birth */
    HARN_HT, HARN_FR, HARN_OWT, HARN_WT, HARN_CML,                      /* Appearance */
    HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML, HARN_VOI,     /* Physical */
    HARN_INT, HARN_AUR, HARN_WIL, HARN_MOR,                             /* Personality */
    HARN_DTY, HARN_PTY,                                                 /* Religion */
    HARN_END,                                                           /* Endurance (derived: STR, STA, WIL) */
    HARN_NATTRS
};
/* HARN_BD is (month << 8) | day; HARN_SS a sunsign (below); HARN_OWT the optimum weight, HARN_WT the weight now;
** HARN_DTY a deity (HARN_AGRIK ..., or HARN_NODEITY); HARN_END isn't kept (harn_end: its value) */

const char* __fastcall__ harn_attr_name (unsigned char a);     /* "Species" ... "Endurance" */
enum { HARN_A_BIR, HARN_A_APP, HARN_A_PHY, HARN_A_PER, HARN_A_REL, HARN_A_NONE };
unsigned char __fastcall__ harn_attr_group (unsigned char a);

/* Species, sex (Character 2) */
enum { HARN_HUM, HARN_SIN, HARN_KHU, HARN_X };
enum { HARN_F, HARN_M };
const char* __fastcall__ harn_species_name (unsigned char sp);  /* "Human", "Sindarin", "Khuzdul", "Other" */
const char* __fastcall__ harn_sex_name (unsigned char sx);
unsigned char harn_rand_species (void);
unsigned char __fastcall__ harn_rand_sex (unsigned char sp);

/* The months (1-12: Nuzyael ... Morgat) and the sunsigns (Character 3): the twelve signs, and a sign's first two and
** last two days its cusps with the signs before and after it (the sign itself named first: HARN_TAI_SKO is born in
** Tai's last days).  A sunsign is 3 times its sign, plus 0 (its first days), 1 (the sign alone) or 2 (its last) */
const char* __fastcall__ harn_month_name (unsigned char m);
unsigned char harn_rand_month (void);
unsigned harn_rand_birthday (void);             /* (month << 8) | day (1-30) */
enum { HARN_S_ULA, HARN_S_ARA, HARN_S_FEN, HARN_S_AHN, HARN_S_ANG, HARN_S_NAD, HARN_S_HIR, HARN_S_TAR, HARN_S_TAI,
       HARN_S_SKO, HARN_S_MAS, HARN_S_LAD };
enum {
    HARN_ULA_LAD, HARN_ULA, HARN_ULA_ARA, HARN_ARA_ULA, HARN_ARA, HARN_ARA_FEN, HARN_FEN_ARA, HARN_FEN,
    HARN_FEN_AHN, HARN_AHN_FEN, HARN_AHN, HARN_AHN_ANG, HARN_ANG_AHN, HARN_ANG, HARN_ANG_NAD, HARN_NAD_ANG,
    HARN_NAD, HARN_NAD_HIR, HARN_HIR_NAD, HARN_HIR, HARN_HIR_TAR, HARN_TAR_HIR, HARN_TAR, HARN_TAR_TAI,
    HARN_TAI_TAR, HARN_TAI, HARN_TAI_SKO, HARN_SKO_TAI, HARN_SKO, HARN_SKO_MAS, HARN_MAS_SKO, HARN_MAS,
    HARN_MAS_LAD, HARN_LAD_MAS, HARN_LAD, HARN_LAD_ULA,
    HARN_NSUNSIGNS
};
const char* __fastcall__ harn_sign_name (unsigned char s);      /* "Ulandus" ... "Masara", "Lado" */
const char* __fastcall__ harn_sunsign_name (unsigned char ss);  /* "Tai-Skorus Cusp", "Tai" ... */
unsigned char __fastcall__ harn_sunsign_of (unsigned char m, unsigned char d);
unsigned char __fastcall__ harn_sunsign_signs (unsigned char ss);   /* Its signs: (its own << 4) | the other's */

/* Culture and social class (Character 3), family (Character 4-5) */
enum { HARN_TRIBAL, HARN_VIKING, HARN_FEUDAL, HARN_IMPERIAL, HARN_KHUZAN, HARN_SINDARIN_C };
enum { HARN_SLAVE, HARN_SERF, HARN_UNGUILDED, HARN_GUILDED, HARN_NOBLE };
enum { HARN_OFFSPRING, HARN_FOSTERED, HARN_ADOPTED, HARN_BASTARD, HARN_ORPHAN };
enum { HARN_OUTCAST, HARN_UNPOPULAR, HARN_AVERAGE, HARN_POPULAR, HARN_FAVORITE };
enum { HARN_DISTANT, HARN_AUNT_UNCLE, HARN_COUSIN, HARN_PARENT_CH };
const char* __fastcall__ harn_culture_name (unsigned char c);
const char* __fastcall__ harn_class_name (unsigned char c);
const char* __fastcall__ harn_parent_name (unsigned char p);
const char* __fastcall__ harn_estrangement_name (unsigned char e);
const char* __fastcall__ harn_clanhead_name (unsigned char c);
unsigned char __fastcall__ harn_rand_class (unsigned char culture);

/* Appearance (Character 6) */
enum { HARN_FR_S, HARN_FR_L, HARN_FR_M, HARN_FR_H, HARN_FR_V };
unsigned char __fastcall__ harn_rand_height (unsigned char sp, unsigned char sx);   /* Inches */
const char* __fastcall__ harn_frame_name (unsigned char fr);    /* "Scant" ... "Massive" */
unsigned char __fastcall__ harn_frame_of (int n);
unsigned char __fastcall__ harn_rand_frame (unsigned char sp, unsigned char sx);
unsigned __fastcall__ harn_weight (unsigned char ht, unsigned char fr);    /* The optimum weight, pounds */
const char* __fastcall__ harn_comeliness_name (unsigned char cml);         /* "Ugly" ... "Handsome" */
enum { HARN_FAIR, HARN_MEDIUM_C, HARN_DARK };
enum { HARN_BROWN, HARN_BLACK, HARN_RED, HARN_SILVER, HARN_BLOND };
enum { HARN_HAZEL, HARN_GRAY, HARN_VIOLET, HARN_GREEN, HARN_BLUE };
const char* __fastcall__ harn_complexion_name (unsigned char c);
const char* __fastcall__ harn_hair_name (unsigned char h);
const char* __fastcall__ harn_eye_name (unsigned char e);

/* An attribute's modifiers (species and sex, frame, weight), and an attribute rolled: 3d6 and them, at least 1 */
int __fastcall__ harn_species_attr_mod (unsigned char sp, unsigned char sx, unsigned char a);
int __fastcall__ harn_frame_attr_mod (unsigned char fr, unsigned char a);
int __fastcall__ harn_weight_attr_mod (unsigned weight, unsigned char a);
unsigned char __fastcall__ harn_rand_attr (unsigned char sp, unsigned char sx, unsigned char fr, unsigned weight,
                                           unsigned char a);
int roll_3d6 (void);

/* Medical traits (Character 9) and mental disorders (Character 11) */
enum {
    HARN_MED_NONE, HARN_ADDICTION, HARN_ALBINISM, HARN_ALLERGY, HARN_AMBIDEXTROUS, HARN_MED_ARM, HARN_BIRTHMARK,
    HARN_DWARFISM, HARN_EPILEPSY, HARN_MED_EAR, HARN_MED_EYE, HARN_GIGANTISM, HARN_HEMOPHILIA, HARN_HIRSUTISM,
    HARN_MED_LEG, HARN_LEFTHANDED, HARN_LEPROSY, HARN_LYCANTHROPY, HARN_MONOCHROMASIA, HARN_OBESITY, HARN_PARASITES,
    HARN_POXMARKS, HARN_SCARS, HARN_STERILE,
    HARN_NMEDICAL
};
const char* __fastcall__ harn_medical_name (unsigned char m);
#define HARN_PSYCHE_NONE    0                   /* (A disorder: 1 ... HARN_NPSYCHE - 1) */
#define HARN_NPSYCHE        39
const char* __fastcall__ harn_psyche_name (unsigned char p);
enum { HARN_MILD, HARN_MODERATE, HARN_SEVERE };
enum { HARN_HETERO, HARN_BI, HARN_HOMO };
const char* __fastcall__ harn_sexuality_name (unsigned char s);
/* (Optional rules, not in harn_create: a character's mental disorders, its sexuality) */
void __fastcall__ harn_rand_psyche (harn_char* c);
unsigned char harn_rand_sexuality (void);

/* Deities (Character 12) */
enum { HARN_AGRIK, HARN_HALEA, HARN_ILVIR, HARN_LARANI, HARN_MORGATH, HARN_NAVEH, HARN_PEONI, HARN_SARAJIN,
       HARN_SAVEKNOR, HARN_SIEM, HARN_NODEITY };
const char* __fastcall__ harn_deity_name (unsigned char d);

/* ---- Skills (Skills 3-4: the Skill Data table, in its order) */

enum {
    /* Physical */
    HARN_ACR, HARN_CLI, HARN_CND, HARN_DAN, HARN_JMP, HARN_LEG, HARN_SKI, HARN_STL, HARN_SWI, HARN_THR,
    /* Communication */
    HARN_ACT, HARN_AWA, HARN_NTR, HARN_LOV, HARN_MEN, HARN_MUS, HARN_ORA, HARN_RHE, HARN_SNG, HARN_LAN, HARN_SCR,
    /* Religion: Ritual, a skill a deity */
    HARN_RAGR, HARN_RHAL, HARN_RILV, HARN_RLAR, HARN_RMOR, HARN_RNAV, HARN_RPEO, HARN_RSAR, HARN_RSAV, HARN_RSIE,
    /* Combat (and Dodge, the book's quasi-skill: AGL times 5) */
    HARN_INI, HARN_UNA, HARN_RID, HARN_AXE, HARN_BLO, HARN_BOW, HARN_CLU, HARN_DAG, HARN_FLA, HARN_NET, HARN_POL,
    HARN_SHD, HARN_SLG, HARN_SPR, HARN_SWD, HARN_WHP, HARN_DGE,
    /* Crafts and lore */
    HARN_AGR, HARN_ALC, HARN_ANI, HARN_AST, HARN_BRE, HARN_CER, HARN_COO, HARN_DRA, HARN_EMB, HARN_ENG, HARN_FIS,
    HARN_FLE, HARN_FOL, HARN_FOR, HARN_GLA, HARN_HRL, HARN_HRB, HARN_HID, HARN_HUN, HARN_INK, HARN_JEW, HARN_LAW,
    HARN_LCK, HARN_LOR, HARN_MSN, HARN_MAT, HARN_MET, HARN_MIL, HARN_MIN, HARN_PRF, HARN_PHY, HARN_PIL, HARN_RUN,
    HARN_SEA, HARN_SHW, HARN_SUR, HARN_TRT, HARN_TEX, HARN_TIM, HARN_TRK, HARN_WPN, HARN_WEA, HARN_WOO,
    HARN_NSKILLS
};
#define HARN_MAXSKILLS  100                     /* The book's, and a program's own after them */

/* A skill's group, and so its penalty: physical and combat skills physical penalty, the rest universal */
enum { HARN_PHYSICAL, HARN_COMMUNICATION, HARN_RELIGION, HARN_COMBAT, HARN_CRAFT };
#define HARN_OML_SCRIPT     0xFE                /* (An OML that's 70 + SB: a native script) */
#define HARN_OML_LANGUAGE   0xFD                /* (A native tongue: 50-80 + SB, by social class) */

typedef struct harn_skill {
    const char*     name;                       /* "Acrobatics" ... */
    unsigned char   att[3];                     /* The attributes its base is the average of */
    unsigned char   ssb[4];                     /* Its sunsign bonuses: (sign << 4) | bonus each (0: none) */
    unsigned char   oml;                        /* Its opening multiplier (or HARN_OML_*) */
    unsigned char   automatic;                  /* Every character has it opened */
    unsigned char   group;                      /* HARN_PHYSICAL ... */
    unsigned char   months;                     /* A craft's months of teaching to open (0: on first use) */
} harn_skill;

extern const harn_skill harn_skills[HARN_NSKILLS];
void __fastcall__ harn_own_skills (const harn_skill* own, unsigned char n);    /* Skills HARN_NSKILLS on */
const harn_skill* __fastcall__ harn_get_skill_info (unsigned char sk);         /* (NULL: none) */
#define harn_skill_name(sk)     (harn_get_skill_info (sk)->name)
const char* __fastcall__ harn_skill_key (unsigned char sk);     /* harn.dl's key: "acr" ... ("": a program's own) */
const char* __fastcall__ harn_skill_sp (unsigned char sk);      /* Its specialties, a comma and a space between */

/* ---- The character */

#define HARN_EQMAX      16
#define HARN_INJMAX     12
#define HARN_MEDMAX     4
#define HARN_PSYMAX     3

typedef struct harn_equip {                     /* An item of equipment: what its weight in the load needs */
    const char*     key;
    const char*     name;
    unsigned        wt;                         /* Its weight, tenths of a pound */
    unsigned char   sk;                         /* The skill it's used with (0xFF: none) */
    unsigned        cost;                       /* Pence */
} harn_equip;

typedef struct harn_injury {
    unsigned char   loc;                        /* Where (HARN_SKULL ...; HARN_RIGHT on a limb's right side) */
    unsigned char   aspect;                     /* HARN_BLUNT ... */
    unsigned char   sev;                        /* HARN_MINOR, HARN_SERIOUS, HARN_GRIEVOUS (as it was taken) */
    unsigned char   il;                         /* Its injury level now */
    unsigned char   hr;                         /* Its healing rate (0: not treated yet) */
    unsigned char   flags;                      /* HARN_IF_* */
} harn_injury;
#define HARN_IF_BLEEDER     0x01                /* A grievous wound: a bloodloss point a minute till it's treated */
#define HARN_IF_INFECTED    0x02
#define HARN_IF_TREATED     0x04
#define HARN_IF_LIMB        0x08                /* Its limb unusable (a grievous limb wound) till it heals to M1 */
#define HARN_IF_AMPUTATED   0x10
#define HARN_IF_IMPAIR      0x20                /* Its healing leaves an attribute 1d3 less (Physician 3's *) */
#define HARN_IF_EE          0x40                /* Its treatment's EE: healed in a day */

enum { HARN_CONSCIOUS, HARN_UNCONSCIOUS, HARN_SHOCK, HARN_DEAD };

struct harn_char {
    unsigned        att[HARN_NATTRS];           /* By HARN_SP ...: harn_get_attr */
    unsigned char   culture, social;            /* HARN_FEUDAL ..., HARN_SERF ... */
    unsigned char   sibrank, famsize;           /* Its place among its parents' children, and their number */
    unsigned char   parent, estrange, clanhead; /* HARN_OFFSPRING ..., HARN_AVERAGE ..., HARN_COUSIN ... */
    unsigned char   complexion, hair, eyes;
    unsigned char   med[HARN_MEDMAX];           /* Its medical traits (HARN_MED_NONE: an empty place) */
    unsigned char   psy[HARN_PSYMAX];           /* Its mental disorders (HARN_PSYCHE_NONE: none), */
    unsigned char   psysev[HARN_PSYMAX];        /*   each one's severity */
    unsigned char   sexuality;
    unsigned char   ml[HARN_MAXSKILLS];         /* Each skill's mastery level (0: not opened) */
    /* The volatiles */
    unsigned char   fl;                         /* Fatigue levels */
    unsigned char   bl;                         /* Bloodloss points */
    unsigned char   dp;                         /* Development points (harn.dl's) */
    unsigned char   state;                      /* HARN_CONSCIOUS ... */
    unsigned char   shockhr;                    /* In shock: its healing rate (Physician 4) */
    const harn_equip* eq[HARN_EQMAX];           /* What it carries and wears, */
    unsigned char   neq;                        /*   how many */
    harn_injury     inj[HARN_INJMAX];           /* Its injuries, */
    unsigned char   ninj;                       /*   how many */
};

void __fastcall__ harn_template (harn_char* t);                 /* A template: everything HARN_ANY (rolled) */
void __fastcall__ harn_create (harn_char* c, const harn_char* template);   /* (NULL: all rolled; culture feudal) */
unsigned __fastcall__ harn_get_attr (const harn_char* c, unsigned char a);  /* (HARN_END: harn_end's) */
unsigned char __fastcall__ harn_attr_avg (const harn_char* c, unsigned char a1, unsigned char a2, unsigned char a3);
unsigned char __fastcall__ harn_end (const harn_char* c);       /* Endurance: STR, STA, WIL's average */
unsigned char __fastcall__ harn_size (const harn_char* c);      /* 1-10, by its weight (Character 6) */
unsigned char __fastcall__ harn_move (const harn_char* c);      /* AGL less PP, at least 1 (Combat 6) */

unsigned char __fastcall__ harn_get_ssb (const harn_char* c, unsigned char sk);    /* Its sunsign bonus */
unsigned char __fastcall__ harn_get_sb (const harn_char* c, unsigned char sk);     /* Its skill base */
unsigned char __fastcall__ harn_get_oml (const harn_char* c, unsigned char sk);    /* Its opening mastery */
#define harn_get_ml(c, sk)      ((c)->ml[sk])
#define harn_get_si(c, sk)      ((c)->ml[sk] / 10)                  /* Its skill index */
#define harn_put_ml(c, sk, v)   ((c)->ml[sk] = (v))
void __fastcall__ harn_open_skill (harn_char* c, unsigned char sk);    /* ML: its OML (if it isn't open) */

/* Development (Skills 6): d100 and the skill's SB over its ML raise it by 1 (it can't pass 100 + SB).  harn.dl's
** development points: harn_apply_dr spends one on a roll */
enum { HARN_DR_IMPROVED, HARN_DR_NOT_IMPROVED, HARN_DR_MAXED, HARN_DR_NO_DP };
unsigned char __fastcall__ harn_develop (harn_char* c, unsigned char sk);  /* A development roll: HARN_DR_* */
unsigned char __fastcall__ harn_apply_dr (harn_char* c, unsigned char sk); /* The same for a point */
#define harn_get_dp(c)          ((c)->dp)
#define harn_put_dp(c, n)       ((c)->dp = (n))
#define harn_inc_dp(c, i)       ((c)->dp += (i))
#define harn_dec_dp(c, i)       ((c)->dp -= (i))

/* Penalties (Skills 6): IP the injuries' levels, FP the fatigue levels, UP their sum; EP the load over endurance;
** PP UP and EP */
extern const harn_equip harn_equipment[1];      /* (harn.dl's: one empty entry; a program has its own) */
unsigned __fastcall__ harn_get_load (const harn_char* c);       /* Pounds (rounded down) */
unsigned char __fastcall__ harn_get_ip (const harn_char* c);
unsigned char __fastcall__ harn_get_fp (const harn_char* c);
unsigned char __fastcall__ harn_get_up (const harn_char* c);
unsigned char __fastcall__ harn_get_ep (const harn_char* c);
unsigned char __fastcall__ harn_get_pp (const harn_char* c);

/* ---- Tests (Skills 5, 21-22) */

#define HARN_CF     0                           /* Critical failure */
#define HARN_MF     1                           /* Marginal failure */
#define HARN_MS     2                           /* Marginal success */
#define HARN_CS     3                           /* Critical success */
#define HARN_SUCCESS(r)     ((r) >= HARN_MS)
unsigned char __fastcall__ harn_test (int eml);                 /* d100 against eml (5-95): HARN_CF ... */
unsigned char __fastcall__ harn_last_roll (void);               /* The d100 the last test rolled */
int __fastcall__ harn_eml (const harn_char* c, unsigned char sk);   /* ML less 5 times UP or PP (by its group) */
unsigned char __fastcall__ harn_test_skill (const harn_char* c, unsigned char sk, int mod);    /* With mod */
unsigned char __fastcall__ harn_versus (unsigned char a, unsigned char b);  /* Skill against skill: 1 a won, 2 b,
                                                                            **   0 neither (harn_last_roll's not
                                                                            **   kept: a program breaks a tie) */
unsigned char __fastcall__ harn_test_attr (const harn_char* c, unsigned char a, unsigned char times, int mod);
                                                /* An attribute times 3-7 on d100 (less 5 a penalty point) */
unsigned char __fastcall__ harn_roll_attr (const harn_char* c, unsigned char a, int mod);
                                                /* 3d6, the penalty and mod added, at most the attribute: 1 */
unsigned char __fastcall__ harn_shock_roll (const harn_char* c);    /* 1d6 a point of UP over END: 1 (down) */
unsigned char __fastcall__ harn_stumble_roll (const harn_char* c, int mod);    /* 3d6 + PP + mod over AGL: 1 */
unsigned char __fastcall__ harn_fumble_roll (const harn_char* c, int mod);     /* 3d6 + PP + mod over DEX: 1 */

/* ---- Combat (Combat 1-18) */

/* The melee and missile attack tables: a result (high nibble) and its dice (low nibble) */
enum { HARN_BLOCK, HARN_COUNTER, HARN_DODGE, HARN_IGNORE };     /* Defences */
#define HARN_R_MISS     0x00
#define HARN_R_BLOCK    0x10                    /* Weapons meet (a weapon damage check, if that's in play) */
#define HARN_R_AH       0x20                    /* The attacker strikes, with n d6 */
#define HARN_R_DH       0x30                    /* The defender strikes */
#define HARN_R_BH       0x40                    /* Both strike */
#define HARN_R_AF       0x50                    /* The attacker's fumble roll */
#define HARN_R_DF       0x60
#define HARN_R_BF       0x70
#define HARN_R_AS       0x80                    /* The attacker's stumble roll */
#define HARN_R_DS       0x90
#define HARN_R_BS       0xA0
#define HARN_R_DTA      0xB0                    /* The defender's tactical advantage */
#define HARN_R_WILD     0xC0                    /* A missile gone wild (its fumble roll) */
#define HARN_R_KIND(r)  ((r) & 0xF0)
#define HARN_R_DICE(r)  ((r) & 0x0F)
unsigned char __fastcall__ harn_melee_result (unsigned char defence, unsigned char att, unsigned char def);
unsigned char __fastcall__ harn_missile_result (unsigned char defence, unsigned char att, unsigned char def);

/* Strike locations (the Combat Tables), aiming high, mid or low (high and low: EML - 10) */
enum { HARN_HIGH, HARN_MID, HARN_LOW };
enum {
    HARN_SKULL, HARN_FACE, HARN_NECK, HARN_SHOULDER, HARN_UPPERARM, HARN_ELBOW, HARN_FOREARM, HARN_HAND,
    HARN_THORAX, HARN_ABDOMEN, HARN_GROIN, HARN_HIP, HARN_THIGH, HARN_KNEE, HARN_CALF, HARN_FOOT,
    HARN_NLOCATIONS
};
#define HARN_RIGHT      0x80                    /* A limb's right side (an even roll) */
#define HARN_LOC(l)     ((l) & 0x7F)
unsigned char __fastcall__ harn_strike_location (unsigned char zone);
const char* __fastcall__ harn_location_name (unsigned char loc);   /* "skull" ... (no side) */
unsigned char __fastcall__ harn_is_limb (unsigned char loc);    /* 1 arm (a fumble roll), 2 leg (a stumble) */

/* The injury table: effective impact on a location to an injury (Combat 13-14) */
enum { HARN_BLUNT, HARN_EDGE, HARN_POINT, HARN_FIRE };          /* Aspects */
enum { HARN_MINOR = 1, HARN_SERIOUS, HARN_GRIEVOUS, HARN_KILL };
#define HARN_INJ_SEV(i)     ((i) >> 4 & 7)      /* HARN_MINOR ... HARN_KILL */
#define HARN_INJ_LEVEL(i)   ((i) & 7)           /* Its injury levels (K4: 4) */
#define HARN_INJ_ROLL       0x08                /* u: a fumble or stumble roll */
#define HARN_INJ_AMPUTATE   0x80                /* t: an amputation roll (an edge strike) */
unsigned char __fastcall__ harn_injury_of (unsigned char loc, int impact);     /* 0: none (impact under 1) */
const char* __fastcall__ harn_severity_name (unsigned char sev);    /* "minor" ... */

/* Armour (Combat 4-5): its materials' protection, and pieces that cover locations */
enum { HARN_CLOTH, HARN_QUILT, HARN_LEATHER, HARN_KURBUL, HARN_RING, HARN_MAIL, HARN_SCALE, HARN_PLATE,
       HARN_NMATERIALS };
extern const unsigned char harn_protection[HARN_NMATERIALS][4];     /* B, E, P, F */
typedef struct harn_armour {
    const char*     name;                       /* "mail byrnie" ... */
    unsigned char   material;
    unsigned char   side;                       /* HARN_BOTH, or a breastplate's HARN_FRONT, a backplate's HARN_BACK */
    unsigned        cover;                      /* Bit n: location n covered */
    unsigned        wt;                         /* Tenths of a pound */
    unsigned        cost;
} harn_armour;
extern const harn_armour harn_armours[];
extern const unsigned char harn_narmours;
enum { HARN_BOTH, HARN_FRONT, HARN_BACK };
unsigned char __fastcall__ harn_armour_at (const harn_armour* const* worn, unsigned char n, unsigned char loc,
                                          unsigned char aspect, unsigned char front);
                                                /* The layers' protection at loc, summed (front: the strike's on
                                                **   the front, as one in four isn't: Combat 13) */

/* Weapons (Combat 3) */
typedef struct harn_weapon {
    const char*     name;
    unsigned char   sk;                         /* Its skill */
    unsigned char   wt;                         /* Tenths of a pound */
    unsigned char   wq;                         /* Its quality */
    unsigned char   wac, wdc;                   /* Its attack and defence classes */
    unsigned char   hm;                         /* Its one-handed penalty */
    signed char     imp[3];                     /* Blunt, edge, point impact (-1: none) */
    unsigned        cost;
} harn_weapon;
extern const harn_weapon harn_weapons[];
extern const unsigned char harn_nweapons;
unsigned char __fastcall__ harn_best_aspect (const harn_weapon* w);   /* The aspect with the most impact */

/* A wound: an effective impact on a location made an injury, and all it brings: the kill roll (Kn: n d6 over END,
** dead), the shock roll, bleeding (grievous), an amputation roll (grievous edge on a t location), a limb made
** unusable.  Its value: HARN_W_* */
#define HARN_W_INJURED      0x01
#define HARN_W_DEAD         0x02
#define HARN_W_DOWN         0x04                /* The shock roll failed: unconscious */
#define HARN_W_FUMBLE       0x08                /* An arm: a fumble roll (or the item dropped) */
#define HARN_W_STUMBLE      0x10                /* A leg: a stumble roll (or fallen) */
#define HARN_W_AMPUTATED    0x20
#define HARN_W_BLEEDING     0x40
#define HARN_W_LIMB         0x80                /* A limb made unusable: what it held dropped, or fallen */
unsigned char __fastcall__ harn_wound (harn_char* c, unsigned char loc, unsigned char aspect, int impact);
unsigned char __fastcall__ harn_kill_roll (const harn_char* c, unsigned char n);   /* n d6 over END: 1 (dead) */

/* ---- Healing (Physician 2-4) */

unsigned char __fastcall__ harn_treat (harn_char* c, unsigned char i, int eml);   /* A treatment of injury i at
                                                                                **   eml: its healing rate (7: EE,
                                                                                **   healed in a day) */
unsigned char __fastcall__ harn_heal_roll (harn_char* c, unsigned char i, int bonus);  /* Injury i's five-day
                                                                                **   roll: 1 if it healed whole */
unsigned char __fastcall__ harn_shock_recovery (harn_char* c, int bonus);         /* A four-hour roll: state */
unsigned char __fastcall__ harn_wake (harn_char* c);            /* A fainted character's turn: its state */
unsigned char __fastcall__ harn_bleed (harn_char* c, unsigned minutes);   /* Bloodloss from bleeders: state */
void __fastcall__ harn_regen_blood (harn_char* c);              /* The five-day roll: bloodloss less 1 or 2 */
void __fastcall__ harn_rest (harn_char* c, unsigned minutes);   /* A fatigue level gone each ten minutes */

#endif
