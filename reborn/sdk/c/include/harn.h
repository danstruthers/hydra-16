/*
** harn.h - HârnMaster characters: danlang's lib/harn.dl in C (the same functions, harn.NAME as harn_NAME, a - as
** _), with dice.h's dice.
**
** A character (harn_char) has harn.dl's attributes, by number (HARN_SP ... HARN_END, in harn.attributes's order:
** harn_attr_name has each one's name); its skills' mastery levels (ml, by skill: HARN_ACR ...; 0 not opened); and
** its volatiles (development points, fatigue, blood loss, its equipment, its injuries).  harn_create makes one at
** random, as harn.create does: its species, sex and birthday (and so its sunsign), its height, frame and weight,
** each attribute 3d6 with its species', frame's and weight's modifiers, its speed from its agility, its piety 5d6,
** and its automatic skills opened.  A template fixes any of them (HARN_ANY: rolled).
**
** A skill (harn_skill: harn.skills's) is its three attributes, its sunsign bonuses (a sign and a bonus each), its
** opening multiplier, whether it's automatic, and its specialties.  Its base (harn_get_sb) is its three attributes'
** average, rounded, and the best bonus of its signs the character's sunsign has (a cusp has two); its opening
** mastery (harn_get_oml) the base times the multiplier.  harn_apply_dr spends a development point on a skill: d100
** and its index (ML / 10) over its ML raise it by 1.
**
** A program can have skills of its own after harn.dl's (HARN_NSKILLS on, to HARN_MAXSKILLS): harn_own_skills gives
** their table, and they work as harn.dl's do.
**
** Where harn.dl is unfinished, this is too, and says so: the medical and family attributes are none (0), the
** equipment table is empty (harn_equipment: a program has its own), a character's current weight is its optimal
** weight as it's made (harn.dl's create has no :wt).  harn.dl's get-fp is 0; here it's the fatigue volatile (0 till
** a program sets it).
*/

#ifndef _HARN_H
#define _HARN_H

#include <dice.h>

#define HARN_ANY        0xFFFF                  /* A template's attribute (or field) that's rolled */

/* ---- The attributes (harn.attributes, in its order) */

enum {
    HARN_SP, HARN_SX, HARN_BD, HARN_SS, HARN_HT, HARN_FR, HARN_OWT, HARN_WT, HARN_CML, HARN_STR, HARN_STA,
    HARN_DEX, HARN_AGL, HARN_SPD, HARN_EYE, HARN_HRG, HARN_SMT, HARN_TCH, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL,
    HARN_MOR, HARN_PTY, HARN_DTY, HARN_END,
    HARN_NATTRS
};
/* HARN_BD is the month and the day, (month << 8) | day; HARN_DTY is 0 (harn_char's dty has its name); HARN_END
** isn't kept (harn_end: its value) */

const char* __fastcall__ harn_attr_name (unsigned char a);     /* "Species" ... "Endurance" */

/* The groups (harn.attr-group) */
enum { HARN_BIR, HARN_APP, HARN_PHY, HARN_PER, HARN_REL, HARN_NONE };
unsigned char __fastcall__ harn_attr_group (unsigned char a);

/* Species, sex, sexuality */
enum { HARN_HUM, HARN_SIN, HARN_KHU, HARN_X };
enum { HARN_F, HARN_M, HARN_N, HARN_A, HARN_H, HARN_SX_X };
enum { HARN_HE, HARN_HO, HARN_BI, HARN_NA };
const char* __fastcall__ harn_species_name (unsigned char sp);  /* "Human", "Sindarin", "Khuzdul", "Other" */
const char* __fastcall__ harn_sex_name (unsigned char sx);
const char* __fastcall__ harn_sexuality_name (unsigned char s);
unsigned char harn_rand_species (void);
unsigned char __fastcall__ harn_rand_sex (unsigned char sp);

/* The months (1-12: Nuzyael ... Morgat) and the sunsigns: the twelve signs and their cusps (harn.sunsign-name's
** order); a sunsign's signs (a sign alone: both the same) */
const char* __fastcall__ harn_month_name (unsigned char m);
unsigned char harn_rand_month (void);
unsigned harn_rand_birthday (void);             /* (month << 8) | day (1-30) */
enum {
    HARN_ULA_LAD, HARN_ULA, HARN_ULA_ARA, HARN_ARA_ULA, HARN_ARA, HARN_ARA_FEN, HARN_FEN_ARA, HARN_FEN,
    HARN_FEN_AHN, HARN_AHN_FEN, HARN_AHN, HARN_AHN_ANG, HARN_ANG_AHN, HARN_ANG, HARN_ANG_NAD, HARN_NAD_ANG,
    HARN_NAD, HARN_NAD_HIR, HARN_HIR_NAD, HARN_HIR, HARN_HIR_TAR, HARN_TAR_HIR, HARN_TAR, HARN_TAR_TAI,
    HARN_TAI_TAR, HARN_TAI, HARN_TAI_SKO, HARN_SKO_TAI, HARN_SKO, HARN_SKO_MAS, HARN_MAS_SKO, HARN_MAS,
    HARN_MAS_LAD, HARN_LAD_MAS, HARN_LAD, HARN_LAD_ULA,
    HARN_NSUNSIGNS
};
/* The twelve signs themselves (a skill's bonuses name them) */
enum { HARN_S_ULA, HARN_S_ARA, HARN_S_FEN, HARN_S_AHN, HARN_S_ANG, HARN_S_NAD, HARN_S_HIR, HARN_S_TAR, HARN_S_TAI,
       HARN_S_SKO, HARN_S_MAS, HARN_S_LAD };
const char* __fastcall__ harn_sunsign_name (unsigned char ss);  /* "Ulandus-Lado Cusp" ... */
unsigned char __fastcall__ harn_sunsign_of (unsigned char m, unsigned char d);
unsigned char __fastcall__ harn_sunsign_signs (unsigned char ss);   /* Its signs: (first << 4) | second */
const char* __fastcall__ harn_sign_name (unsigned char s);      /* "Ulandus" ... */

/* Height, frame, weight */
enum { HARN_FR_S, HARN_FR_L, HARN_FR_M, HARN_FR_H, HARN_FR_V };
unsigned char __fastcall__ harn_rand_height (unsigned char sp, unsigned char sx);   /* Inches */
const char* __fastcall__ harn_frame_name (unsigned char fr);    /* "Scant" ... "Massive" */
unsigned char __fastcall__ harn_frame_of (int n);
unsigned char __fastcall__ harn_rand_frame (unsigned char sp, unsigned char sx);
unsigned __fastcall__ harn_weight (unsigned char ht, unsigned char fr);    /* Pounds */

/* An attribute's modifiers, and one rolled (3d6 and them, at least 1) */
int __fastcall__ harn_species_attr_mod (unsigned char sp, unsigned char sx, unsigned char a);
int __fastcall__ harn_frame_attr_mod (unsigned char fr, unsigned char a);
int __fastcall__ harn_weight_attr_mod (unsigned weight, unsigned char a);
int __fastcall__ harn_medical_attr_mod (unsigned char med, unsigned char a);
int __fastcall__ harn_attr_mod (unsigned char sp, unsigned char sx, unsigned char fr, unsigned weight,
                                unsigned char med, unsigned char a);
unsigned char __fastcall__ harn_rand_attr (unsigned char sp, unsigned char sx, unsigned char fr, unsigned weight,
                                           unsigned char med, unsigned char a);
int roll_3d6 (void);

/* ---- Skills (harn.skills, in its order) */

enum {
    HARN_ACR, HARN_CLI, HARN_CND, HARN_DDG, HARN_INI, HARN_DAN, HARN_JMP, HARN_LEG, HARN_MOB, HARN_SEA, HARN_SKI,
    HARN_STL, HARN_SWI, HARN_THR, HARN_UNA,
    HARN_RID, HARN_SHD, HARN_KNI, HARN_SSW, HARN_LSW, HARN_CLU, HARN_AXE, HARN_BAX, HARN_FLA, HARN_WHI, HARN_NET,
    HARN_STA_SK, HARN_POL,
    HARN_BOW, HARN_BLO, HARN_SLG, HARN_XBO,
    HARN_ACT, HARN_AWA, HARN_NTR, HARN_LOV, HARN_MUS, HARN_ORA, HARN_RHE, HARN_SNG,
    HARN_NSKILLS
};
/* (Staves is HARN_STA_SK: HARN_STA is Stamina) */
#define HARN_MAXSKILLS  48                      /* harn.dl's, and a program's own after them */

typedef struct harn_skill {
    const char*     key;                        /* harn.dl's: "acr" ... */
    const char*     name;                       /* "Acrobatics" ... */
    unsigned char   att[3];                     /* The attributes its base is the average of */
    unsigned char   ssb[5];                     /* Its sunsign bonuses: (sign << 4) | bonus each (0: none) */
    unsigned char   oml;                        /* Its opening multiplier */
    unsigned char   automatic;                  /* Every character has it opened */
    const char*     sp;                         /* Its specialties, a comma and a space between ("": none) */
} harn_skill;

extern const harn_skill harn_skills[HARN_NSKILLS];
void __fastcall__ harn_own_skills (const harn_skill* own, unsigned char n);    /* Skills HARN_NSKILLS on */
const harn_skill* __fastcall__ harn_get_skill_info (unsigned char sk);         /* (NULL: none) */
#define harn_skill_name(sk)     (harn_get_skill_info (sk)->name)

/* ---- The character */

#define HARN_EQMAX      16
#define HARN_INJMAX     8

typedef struct harn_equip {                     /* harn.new-equipment's */
    const char*     key;
    const char*     name;
    unsigned        wt;                         /* Its weight (pounds) */
    unsigned char   sk;                         /* The skill it's used with (0xFF: none) */
    unsigned        cost;
} harn_equip;

typedef struct harn_injury {                    /* An injury: harn.dl's get-ip sums their levels */
    unsigned char   loc;                        /* Where (a program's own numbers) */
    unsigned char   il;                         /* Its injury level */
} harn_injury;

typedef struct harn_char {
    unsigned        att[HARN_NATTRS];           /* By HARN_SP ...: harn_get_attr */
    const char*     dty;                        /* Its deity's name ("<None chosen>") */
    unsigned char   med;                        /* Medical (harn.dl: none yet, 0) */
    unsigned char   sr, est, ch;                /* Family: sibling rank, estrangement, clan head (none yet, 0) */
    unsigned char   ml[HARN_MAXSKILLS];         /* Each skill's mastery level (0: not opened) */
    /* The volatiles (harn.valid-volatiles) */
    unsigned char   bl;                         /* Blood loss */
    unsigned char   dp;                         /* Development points */
    unsigned char   fp;                         /* Fatigue */
    const harn_equip* eq[HARN_EQMAX];           /* Its equipment, */
    unsigned char   neq;                        /*   how many */
    harn_injury     inj[HARN_INJMAX];           /* Its injuries, */
    unsigned char   ninj;                       /*   how many */
} harn_char;

void __fastcall__ harn_create (harn_char* c, const harn_char* template);   /* (template NULL: all rolled; its
                                                                            **   att[] HARN_ANY, dty NULL: rolled) */
unsigned __fastcall__ harn_get_attr (const harn_char* c, unsigned char a); /* (HARN_END: harn_end's) */
unsigned char __fastcall__ harn_attr_avg (const harn_char* c, unsigned char a1, unsigned char a2, unsigned char a3);
unsigned char __fastcall__ harn_end (const harn_char* c);       /* Endurance: STR, STA and WIL's average */
unsigned char __fastcall__ harn_size (const harn_char* c);      /* 1-10, by its weight */

unsigned char __fastcall__ harn_get_ssb (const harn_char* c, unsigned char sk);    /* Its sunsign bonus */
unsigned char __fastcall__ harn_get_sb (const harn_char* c, unsigned char sk);     /* Its skill base */
unsigned __fastcall__ harn_get_oml (const harn_char* c, unsigned char sk);         /* Its opening mastery */
#define harn_get_ml(c, sk)      ((c)->ml[sk])
#define harn_get_si(c, sk)      ((c)->ml[sk] / 10)                  /* Its skill index */
#define harn_put_ml(c, sk, v)   ((c)->ml[sk] = (v))
void __fastcall__ harn_open_skill (harn_char* c, unsigned char sk);    /* ML: its OML */

/* Development (harn.apply-dr): a point spent on a skill, and what came of it */
enum { HARN_DR_IMPROVED, HARN_DR_NOT_IMPROVED, HARN_DR_MAXED, HARN_DR_NO_DP };
#define harn_get_dp(c)          ((c)->dp)
#define harn_put_dp(c, n)       ((c)->dp = (n))
#define harn_inc_dp(c, i)       ((c)->dp += (i))
#define harn_dec_dp(c, i)       ((c)->dp -= (i))
unsigned char __fastcall__ harn_apply_dr (harn_char* c, unsigned char sk);  /* HARN_DR_*: improved (ML + 1, a
                                                                            **   point spent), not (spent), can't
                                                                            **   be (ML 100 + SI), no points */

/* Load and penalties: load the equipment's weight; IP the injuries' levels; FP fatigue; UP their sum; EP the load
** over endurance; PP UP and EP */
extern const harn_equip harn_equipment[];       /* (harn.dl's is empty: a program has its own) */
unsigned __fastcall__ harn_get_load (const harn_char* c);
unsigned char __fastcall__ harn_get_ip (const harn_char* c);
unsigned char __fastcall__ harn_get_fp (const harn_char* c);
unsigned char __fastcall__ harn_get_up (const harn_char* c);
unsigned char __fastcall__ harn_get_ep (const harn_char* c);
unsigned char __fastcall__ harn_get_pp (const harn_char* c);

#endif
