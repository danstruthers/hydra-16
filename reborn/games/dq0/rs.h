/*
** rs.h - Dingus Quest Zero's rulesets: what the game asks a ruleset, and in what terms it answers, the same whatever
** the ruleset (HarnMaster, Castles & Crusades, ...).  The game never names a skill, a class or a die: it asks for a
** purpose (notice a sound, climb, resist poison), gives the circumstances as steps (each a nudge of about 5%, for the
** better or the worse), and gets back a grade, conditions, effects and words.  A ruleset is a program of its own,
** the referee (dq0harn, dq0cc ...), that answers through the hub (hub.h: its request and reply areas), and a
** character generator (hmchar ...) whose files it reads; its manifest (lib/dq0/rules/NAME) says where they are.
**
** Converting a character between rulesets goes through a neutral character (struct rs_neutral): each ruleset can
** make one of its own characters (RS_EXPORT) and make its own from one (RS_IMPORT).
*/

#ifndef _RS_H
#define _RS_H

/* ---- Grades: how a test went (HarnMaster's levels one for one; a d20's: a natural 1 CF, a miss MF, a hit MS, a
** natural 20 or a hit by 10 or more CS) */
#define G_CF            (-2)
#define G_MF            (-1)
#define G_MS            1
#define G_CS            2
#define G_OK(g)         ((signed char) (g) > 0)

/* ---- Purposes: what a test is for (the ruleset picks its skill, attribute or save) */
enum {
    P_NOTICE_SIGHT, P_NOTICE_SOUND, P_NOTICE_SMELL, P_SEARCH,
    P_STEALTH, P_CLIMB, P_JUMP, P_SWIM, P_BALANCE, P_FORCE, P_LIFT,
    P_PICKLOCK, P_DISARM,
    P_ENDURE, P_RESIST_POISON, P_RESIST_MIND, P_DODGE_HAZARD,
    P_FIRST_AID, P_LORE, P_TRACK, P_PERSUADE, P_INTIMIDATE, P_ANIMALS,
    P_DIG,                                      /* Digging (b: the material, MAT_*): judged by the tool to hand that
                                                **   suits it best (the referee's choice: its key in the reply's
                                                **   text, the wear the spell did it in w2); the reply's w1 the feet a
                                                **   spell of work makes, its result 1 if nothing it has can dig that */
    P_APPRAISE,                                 /* A thing judged (b: the craft that knows it, AP_*): its make, its
                                                **   worth; the grade how near the judgement comes */
    P_CONCEAL,                                  /* Concealment: a thing hidden, or oneself (in shadows ...): the
                                                **   grade sets a thing's difficulty to find, or a person's against
                                                **   a search */
    P_THEME,                                    /* A theme's own, P_THEME + n (named in its data) */
};

/* How a hidden thing's hidden (P_SEARCH's b): found by searching, or only by magical means (the referee's: a spell,
** a power, an ability; its result 1 if the searcher has none) */
enum { HIDE_MECHANICAL, HIDE_MAGICAL };

/* The crafts that know things (P_APPRAISE's b): a warrior knows weapons and armour, not books of magic */
enum { AP_WEAPON, AP_ARMOUR, AP_METAL, AP_WOOD, AP_HIDE, AP_CLOTH, AP_POTTERY, AP_JEWEL, AP_BOOK, AP_FOOD, AP_BONE,
    NAP };

/* What a dig is into */
enum { MAT_EARTH, MAT_STONE, MAT_MASONRY };

/* ---- Conditions (bits): what the world must know of an actor's state */
#define C_PRONE         0x0001
#define C_UNCONSCIOUS   0x0002
#define C_DEAD          0x0004
#define C_STUNNED       0x0008                  /* (HarnMaster's shock) */
#define C_BLEEDING      0x0010
#define C_FATIGUED      0x0020
#define C_ARM_USELESS   0x0040
#define C_LEG_USELESS   0x0080
#define C_BLINDED       0x0100
#define C_ASLEEP        0x0200
#define C_POISONED      0x0400
#define C_FRIGHTENED    0x0800

/* ---- Health: how hurt an actor is, in words any ruleset can say */
enum { H_FINE, H_HURT, H_BADLY, H_CRITICAL, H_DOWN, H_DEAD };

/* ---- How an attack came out, for the words */
enum { O_MISS, O_DODGED, O_BLOCKED, O_HIT, O_BOTH_HIT, O_COUNTER_HIT, O_FUMBLE, O_STUMBLE, O_ADVANTAGE, O_CRIT };

/* ---- Defences: the stance the player sets (the ruleset decides what each means; auto: its best) */
enum { D_AUTO, D_BLOCK, D_DODGE, D_COUNTER, D_NONE };

/* An effect of a reply: what happened to whom (up to RS_EFFECTS a reply) */
#define RS_EFFECTS      4
struct rs_effect {
    unsigned char   target;                     /* An actor */
    unsigned char   kind;                       /* X_* */
    unsigned char   band;                       /* A wound's severity, 1 slight ... 4 mortal */
    unsigned char   phrase;                     /* Its words: a line of the reply's text (0: none) */
    unsigned        cond;                       /* Conditions set (X_COND_ON) or cleared (X_COND_OFF) */
    unsigned char   x[2];
};
enum { X_NONE, X_WOUND, X_HEAL, X_COND_ON, X_COND_OFF, X_DEATH, X_DROP, X_EXTRA_ACTION, X_IMPROVED };
/* (An X_WOUND's cond: the conditions it leaves the actor in; its x[0] where it is, BP_*, and x[1] its aspect, AS_*) */
enum { BP_SKULL, BP_FACE, BP_NECK, BP_SHOULDER, BP_UPPER_ARM, BP_ELBOW, BP_FOREARM, BP_HAND, BP_CHEST, BP_BELLY,
    BP_GROIN, BP_HIP, BP_THIGH, BP_KNEE, BP_CALF, BP_FOOT };
#define BP_RIGHT        0x80                    /* (A limb's right side) */

/* ---- Harm (RS_HARM): its cause (the request's a), and the rest by cause:
**   HC_FALL     w1 its height in feet; b the ground it lands on (GR_*); c HF_* (a clear fall; holds to grab)
**   HC_STRIKE   a trap's (a blade, darts, spikes, a deadfall), or any blow with no one behind it: b its aspect
**               (AS_*), c its dice (d6s), w1 impact more, w2 where it strikes (Z_*)
**   HC_FIRE     a burn: c its dice, w1 more, w2 where
**   HC_BLOW     a blow the actor strikes at the other, with what it has in hand (its key in the text: "blade",
**               "pick" ...; "hands"): b its aspect (AS_BEST: what it does best), c its dice (0: one).  At a thing
**               (other RS_THING: a door, a portcullis's bars, a chest) only the blow's impact is worked out, the
**               reply's w1: what the thing's material and toughness take from it is the world's
** The reply: its effects (x: up to RS_EFFECTS, X_NONE after the last), b the worst wound's band (0: unhurt), w2 the
** tenths the actor's out for (knocked senseless), result 1 a fall stopped (a hold grabbed); its words in the text,
** a line (and RS_DESCRIBE b 2's: the last harm's) */
enum { HC_FALL, HC_STRIKE, HC_FIRE, HC_BLOW };
enum { AS_BLUNT, AS_EDGE, AS_POINT, AS_FIRE, AS_BEST };     /* (items.h's DM_* in the same order) */
enum { Z_MID, Z_HIGH, Z_LOW };
enum { GR_FLOOR, GR_DEEP_WATER, GR_SHALLOW_WATER, GR_SOFT, GR_HARD, GR_ROCKY };    /* Earth; water; mud; paving;
                                                **   broken rock */
#define HF_CLEAR        0x01
#define HF_HOLDS        0x02
#define RS_THING        0xFF                    /* (other: not an actor) */

/* An actor's summary: what the world needs often, kept by the referee in the actor's record so as not to ask */
struct rs_summary {
    unsigned        cond;                       /* C_* */
    unsigned char   health;                     /* H_* */
    unsigned char   step;                       /* Tenths of a second to go 5 feet */
    unsigned char   attacks;                    /* Attacks a round */
    unsigned char   sight, hearing, smell;      /* 0 (none) ... 9 (keen) */
    unsigned char   darkvision;                 /* Ultravision: feet it sees with no light at all (0: none) */
    unsigned char   nightvision;                /* Night vision: how far faint light carries for it, times (1: as
                                                **   anyone's) */
    unsigned char   morale;
    unsigned char   load;                       /* Encumbrance: 0 none ... 3 overloaded */
    unsigned char   power;                      /* 0-100: how mighty, the same scale whatever the ruleset */
};

/* ---- The requests (the hub's referee areas carry one, and its reply) */
enum {
    RS_HELLO,                                   /* -> its name, its round (tenths), its standard action times */
    RS_LOAD_PC,                                 /* A character file -> its name, calling, summary */
    RS_NEW_ACTOR,                               /* A creature by key (and its tier, size, kind) -> its summary */
    RS_DROP_ACTOR,
    RS_TEST,                                    /* An actor, a purpose, steps -> a grade */
    RS_TEST_MANY,                               /* The same for several (perception) */
    RS_CONTEST,                                 /* Two actors, two purposes -> who won */
    RS_ATTACK,                                  /* Attacker, defender, weapon, mode, range, steps -> the outcome */
    RS_HARM,                                    /* A cause (fall, fire, poison ...) and its dice -> effects */
    RS_TIME,                                    /* Minutes passed: bleeding, fatigue -> effects */
    RS_REST,                                    /* Hours resting: healing, advancement -> effects, words */
    RS_TREAT,                                   /* A healer, a patient -> effects */
    RS_EQUIP,                                   /* An item taken up (a 1) or put away (a 0): its key in the text
                                                **   ("pick"); or all that's to hand anew (a 2: the keys, a space
                                                **   between): the referee judges tests by it, a skill used taking the
                                                **   tool that suits */
    RS_INITIATIVE,                              /* Actors -> each one's delay as a fight starts */
    RS_MORALE,
    RS_EXPERIENCE,                              /* Something that counts to advancement */
    RS_DESCRIBE,                                /* An actor's sheet, wounds, or a line -> words */
    RS_POWER,                                   /* (Later: spells and psionics, a generic power) */
    RS_EXPORT,                                  /* The character as a neutral one (struct rs_neutral) */
    RS_IMPORT,                                  /* A neutral character made one of this ruleset's */
    RS_SAVE, RS_RESTORE, RS_QUIT,
};

/* A neutral character: what carries over between rulesets.  Attributes on a 3-18 scale (HarnMaster's as they are; a
** d20's: Strength, Dexterity, Constitution, Intelligence, Wisdom, Charisma mapped to the nearest); its abilities as
** purposes, each a chance in percent; its calling as a word the other ruleset maps to its nearest (warrior, rogue,
** hunter, healer, scholar, miner ...) */
struct rs_neutral {
    char            name[16];
    char            calling[12];
    unsigned char   species, female;
    unsigned char   str, sta, dex, agl, per, intl, wil, cha;    /* 3-18 */
    unsigned char   ability[P_THEME];           /* Each purpose's chance, percent (0: untrained) */
    unsigned char   weapon[8];                  /* Weapon families' chances: sword, axe, club, dagger, spear, bow,
                                                **   sling, unarmed */
    unsigned char   level;                      /* Experience: 1 a beginner ... (HarnMaster's from its skills) */
};

#endif
