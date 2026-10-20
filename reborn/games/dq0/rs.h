/*
** rs.h - Dingus Quest Zero's rulesets: what the game asks a ruleset, and in what terms it answers, the same whatever
** the ruleset (HarnMaster, Castles & Crusades, ...).  The game never names a skill, a class or a die: it asks for a
** purpose (notice a sound, climb, resist poison), gives the circumstances as steps (each 5%, for the better or the
** worse: the scales below), and gets back a grade, conditions, effects and words.  A ruleset is a program of its own,
** the referee (dq0harn, dq0null, dq0cc ...: lib/dq0/dq0NAME, dq0 -r NAME's), that answers through the hub (hub.h: its
** request and reply areas), and a character generator (hmchar ...) whose files it reads, the referee's RS_HELLO
** naming it (RULESETS.md: what a ruleset answers, and what the game assumes of it).
**
** Converting a character between rulesets goes through a neutral character (struct rs_neutral): each ruleset can
** make one of its own characters (RS_EXPORT) and make its own from one (RS_IMPORT).
*/

#ifndef _RS_H
#define _RS_H

/* ---- The scales and the units: one each, the game's, for every difficulty, size, weight and distance it keeps, asks
** a ruleset about or reads from its data (a data file's written in them, or made them by its compiler, once; the
** game's code converts only to word them, a square's distance told in feet, a weight in pounds)
**   A difficulty, and every circumstance of a test: steps (a request's steps, + easier, - harder; a record's
**     difficulty, the steps worse a test of it goes), each 5 points of a d100 test (STEP_PCT: HarnMaster's; a d20's
**     1).  As a guide: 1 or 2 a little harder (haste, a simple lock), 4 hard (an iron door to force), 6 to 8 very
**     hard (a sound lock, a thing hidden well, a window's bars), 10 or more all but beyond most (a thing hidden
**     cunningly)
**   A size: pints (a thing's bulk, a container's room)
**   A weight: quarter pounds (WT_LB a pound's: a data file's pounds made quarters by its compiler)
**   A distance: feet (a height, a depth, a fall, a reach, a door's width), kept in whole feet but where a field says
**     otherwise: across the map, in its squares (SQ_FT feet each: a light's reach, a creature's sight and hearing,
**     how far off a thing's made out; a data file's feet made squares by its compiler), a slope in quarter feet a
**     square, a thickness in eighths of an inch (TH_IN an inch's) */
#define STEP_PCT        5
#define WT_LB           4
#define SQ_FT           5
#define TH_IN           8

/* ---- Grades: how a test went (HarnMaster's levels one for one; a d20's: a natural 1 CF, a miss MF, a hit MS, a
** natural 20 or a hit by 10 or more CS) */
#define G_CF            (-2)
#define G_MF            (-1)
#define G_MS            1
#define G_CS            2
#define G_OK(g)         ((signed char) (g) > 0)

/* ---- Purposes: what a test is for (the ruleset picks its skill, attribute or save).  A notice (P_NOTICE_*) is the
** worse for the actor's state as the ruleset has it, and for its pace while it moves (the hub's pc_going: the
** referee's steps); P_NOTICE_SIGHT's b 1 a hidden hazard underfoot (a trap stepped onto), its steps the hiding's own:
** the referee's chance, a passing glance's or (searching as it goes) a search's */
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
    P_ASSESS,                                   /* A mechanism made out (a trap's, before it's disarmed: never
                                                **   deadly, however it goes) */
    P_DECEIVE,                                  /* Guile: a creature talked round by a tale, a bluff (the persuading
                                                **   is P_PERSUADE's, a threat P_INTIMIDATE's, an animal P_ANIMALS's;
                                                **   any of them in words: the text the tongue they're said in, the
                                                **   reply's result 2 if the character doesn't speak it) */
    P_THROW,                                    /* A thing thrown at a mark (its range band in the steps: Throwing,
                                                **   the book's missile ranges) */
    P_THEME,                                    /* A theme's own, P_THEME + n (named in its data) */
};

/* How a hidden thing's hidden (P_SEARCH's b): found by searching, or only by magical means (the referee's: a spell,
** a power, an ability; its result 1 if the searcher has none); and how a trap's set (P_DISARM's b: magic only
** dispelled, the same) */
enum { HIDE_MECHANICAL, HIDE_MAGICAL };

/* The crafts that know things (P_APPRAISE's b): a warrior knows weapons and armour, not books of magic */
enum { AP_WEAPON, AP_ARMOUR, AP_METAL, AP_WOOD, AP_HIDE, AP_CLOTH, AP_POTTERY, AP_JEWEL, AP_BOOK, AP_FOOD, AP_BONE,
    AP_STONE, AP_GLASS, NAP };                  /* (Stone: a mason's; glass: a glazier's) */

/* What a dig is into (and what a fall lands on): a floor's flags (the user: a material of their own) are lifted, not
** cut */
enum { MAT_EARTH, MAT_STONE, MAT_MASONRY, MAT_FLAGS };

/* ---- Paces: how fast an actor goes, the slowest first (a creep, a slow walk, a walk, a jog, a run, a sprint); the
** summary's gstep each one's time a square; RS_EXERT's a the one the time went at (GA_REST: standing still) */
enum { GA_CREEP, GA_SLOW, GA_WALK, GA_JOG, GA_RUN, GA_SPRINT, NGAITS };
#define GA_REST         0xFF

/* ---- Conditions (bits): what the world must know of an actor's state */
#define C_PRONE         0x0001
#define C_UNCONSCIOUS   0x0002
#define C_DEAD          0x0004
#define C_STUNNED       0x0008                  /* (HarnMaster's shock) */
#define C_BLEEDING      0x0010
#define C_FATIGUED      0x0020                  /* (Tired, or tiring: rest owed it, RS_EXERT's) */
#define C_ARM_USELESS   0x0040
#define C_LEG_USELESS   0x0080
#define C_BLINDED       0x0100
#define C_ASLEEP        0x0200
#define C_POISONED      0x0400
#define C_FRIGHTENED    0x0800

/* ---- Health: how hurt an actor is, in words any ruleset can say */
enum { H_FINE, H_HURT, H_BADLY, H_CRITICAL, H_DOWN, H_DEAD };

/* ---- How an attack came out, for the words (O_DISPATCH: a helpless one finished; O_RISE: the attacker, fallen,
** only got to its feet) */
enum { O_MISS, O_DODGED, O_BLOCKED, O_HIT, O_BOTH_HIT, O_COUNTER_HIT, O_FUMBLE, O_STUMBLE, O_ADVANTAGE, O_CRIT,
    O_DISPATCH, O_RISE };

/* ---- Defences: the stance the player sets (the ruleset decides what each means; auto: its best) */
enum { D_AUTO, D_BLOCK, D_DODGE, D_COUNTER, D_NONE };

/* ---- An attack (RS_ATTACK): actor strikes at other (0 the character, else an actor's entry plus 1) with what it has
** in hand (the character's: RS_EQUIP's, the best weapon held; else its fists) or its own (a creature's claws, bite
** ...): a its aim (Z_*), b its aspect (AS_*, AS_BEST: what it does best), c the others engaged with the defender
** besides the attacker (outnumbered), steps the circumstances.  The defence is the ruleset's: the character's by its
** stance (the hub's pc_stance, D_*), a creature's by its nature; one senseless is finished.  The reply: result how it
** came out (O_*; 0xFF: none struck), a whose a fumble or a stumble was (1 the attacker's, 2 the defender's, 3
** both's), b the defence (D_*), c AF_*, its effects (x: X_WOUND, X_DEATH, X_COND_ON: C_PRONE, C_UNCONSCIOUS;
** X_DROP: a weapon fumbled), w1 the attacker's health (H_*) and the defender's times 256, w2 the tenths one knocked
** senseless is out for; its words in the text, four lines: the attacker's weapon ("sword", "bite"), the defender's,
** what the attacker's strike did ("a serious cut to its left thigh (S2)", "your ring byrnie": the armour that stopped
** it), and the defender's (a counterstrike's, a block's on a bare arm or a claw).  A creature's blow and the
** character's at one (the world's, its slot: it keeps what the blow did, and puts the words in the fights' ring,
** hub.h's FIGHT_AT): the request's w2 1 if the creature is shy (a shaken one flees), and RS_PRONE_A if the attacker
** lies prone, RS_PRONE_D the defender (their bodies' positions, the world's: one prone is the easier to strike, and gets
** up as its turn, O_RISE, the world's to stand it up; one that falls, a stumble, a leg giving way, an X_COND_ON
** C_PRONE effect, the world's to lay it down); the reply's grade what it left
** them in (hub.h's FF_*: the creature's courage tested if it was wounded and stands, RS_MORALE's: FF_FLEES, or
** AF_CAUTIOUS).  The character strikes with what the
** hub's pc_strike says (SK_*: the best to hand, a fist, a foot, the head: Unarmed's, the book's hand, foot and head).
**
** At a thing (other RS_THING: a door, a portcullis's bars, a window's shutters or glass; the maker's), which ignores it
** (the book's Ignore defence): x[0..3] the thing's resistance to blunt, edge, point and fire (its armour), c the blow's
** number in a run of them; the character's best to hand the tool that hits a thing hardest (not a weapon), else its
** fists.  The reply: w1 the harm the thing takes (the impact past its resistance, its aspect the best against it: b),
** result 2 if a part of the striker struck it (its words in the text, the striker's weapon), and its own hurt if the
** blow went wrong (a part only: the attack's marginal failure a d6, its critical failure 2d6, the book's H2 for a part
** that meets something hard, Combat 12; and half the thing's blunt resistance; less the part's armour, a kick's the
** soles'): HC_STRUCK's, c its worst band, w2 the tenths it's out for, result | 1 if it's hurt enough to stop, its words
** RS_DESCRIBE b 2's */
#define RS_PRONE_A      0x02                    /* (RS_ATTACK's w2: the attacker lies prone, */
#define RS_PRONE_D      0x04                    /*   the defender) */
#define AF_STOPPED      0x01                    /* The attacker's strike stopped by armour (its line the armour's) */
#define AF_STOPPED_D    0x02                    /*   the defender's */
#define AF_TA           0x04                    /* The defender's tactical advantage: an action more, at once */
#define AF_CAUTIOUS     0x08                    /* The creature in it, wounded, holds back a round (its courage) */
#define AF_BLEEDING     0x10                    /*   bleeding (the referee's RS_TIME a minute at a time) */

/* ---- Morale (RS_MORALE): an actor's courage tested (steps: how its fight goes): the grade, and result its state */
enum { M_STEADY, M_CAUTIOUS, M_BROKEN, M_DESPERATE, M_BERSERK };

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
**   HC_STRUCK   the actor's own part hurt by its blow at a thing (an RS_ATTACK at RS_THING's: the referee's own):
**               c the part (BP_HAND, BP_FOOT, BP_SKULL), w1 its impact (its armour off it), w2 the blow's number in a
**               run of them (0: its words begun anew, else added to); a blunt hit on it, and the reply's result 1 if
**               it's hurt badly enough to stop (the referee's say)
** The reply: its effects (x: up to RS_EFFECTS, X_NONE after the last), b the worst wound's band (0: unhurt), w2 the
** tenths the actor's out for (knocked senseless), result 1 a fall stopped (a hold grabbed); its words in the text,
** a line (and RS_DESCRIBE b 2's: the last harm's) */
enum { HC_FALL, HC_STRIKE, HC_FIRE, HC_STRUCK };
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
    unsigned char   step;                       /* Tenths of a second to go a square (walking) */
    unsigned char   gstep[NGAITS];              /*   and at each pace (GA_*) */
    unsigned char   attacks;                    /* Attacks a round */
    unsigned char   sight, hearing, smell;      /* 0 (none) ... 9 (keen) */
    unsigned char   darkvision;                 /* Ultravision: squares it sees with no light at all (0: none) */
    unsigned char   nightvision;                /* Night vision: how far faint light carries for it, times (1: as
                                                **   anyone's) */
    unsigned char   morale;
    unsigned char   load;                       /* Encumbrance: 0 none, 1 a little burdened, 2 heavily, 3 overloaded */
    unsigned char   power;                      /* 0-100: how mighty, the same scale whatever the ruleset */
    unsigned char   northsense;                 /* 1: it always knows which way north is underground (its species':
                                                **   a dwarf's); 0: it can lose its bearings (DUNGEON.md, to come) */
};

/* ---- The requests (the hub's referee areas carry one, and its reply) */
enum {
    RS_HELLO,                                   /* -> its name, its round (tenths), its standard action times; its
                                                **   text a line its name, a line its character generator (a
                                                **   program in the game's folder, run NAME -o FILE -s SEED; none:
                                                **   "", RS_LOAD_PC's path empty, the referee's own character) */
    RS_LOAD_PC,                                 /* A character file -> its name, calling, summary; a and b the pace
                                                **   and the watchfulness its file says (GA_WALK and 0 if none: the
                                                **   world's to set, a new game's) */
    RS_NEW_ACTOR,                               /* A creature by key (and its tier, size, kind) -> its summary:
                                                **   actor its entry plus 1, the key in the text, w1 its record (the
                                                **   denizens' task's: its sheet's, kept and saved with the game; made
                                                **   the first time, the same one after) */
    RS_DROP_ACTOR,
    RS_TEST,                                    /* An actor, a purpose, steps -> a grade */
    RS_TEST_MANY,                               /* The same for several (perception) */
    RS_CONTEST,                                 /* Two actors, two purposes -> who won */
    RS_ATTACK,                                  /* Attacker, defender, weapon, mode, range, steps -> the outcome */
    RS_HARM,                                    /* A cause (fall, fire, poison ...) and its dice -> effects */
    RS_TIME,                                    /* Minutes passed (w1): bleeding, fatigue -> effects (b its health,
                                                **   result 1 if it died of them) */
    RS_EXERT,                                   /* Time passed (w1 tenths) moving at a pace (a: GA_*), or standing
                                                **   still (GA_REST): fatigue gained, or rested off (its summary
                                                **   anew) */
    RS_REST,                                    /* Hours resting: healing, advancement -> effects, words */
    RS_TREAT,                                   /* A healer, a patient -> effects */
    RS_EQUIP,                                   /* An item taken up (a 1) or put away (a 0): its key in the text
                                                **   ("pick"); or all that's to hand anew (a 2: the keys, each after
                                                **   a mark, * held, + worn, a space neither, and its quality after it,
                                                **   a digit 0-9: " pick4*blade5+ltunic3"): the referee judges tests
                                                **   by it, a skill used taking the tool that suits, a fight by what's
                                                **   held and worn; a 2's w1 the load, all the actor carries (its
                                                **   weight): its encumbrance the referee's, in the summary (its
                                                **   load, its step) and its tests */
    RS_INITIATIVE,                              /* Actors -> each one's delay as a fight starts */
    RS_MORALE,
    RS_EXPERIENCE,                              /* Something that counts to advancement */
    RS_DESCRIBE,                                /* An actor's sheet, wounds, or a line -> words (b 3: its state, the
                                                **   hub's pace and watchfulness its first lines; a 1, those only) */
    RS_POWER,                                   /* (Later: spells and psionics, a generic power) */
    RS_EXPORT,                                  /* The character as a neutral one (struct rs_neutral) */
    RS_IMPORT,                                  /* A neutral character made one of this ruleset's */
    RS_SAVE, RS_RESTORE, RS_QUIT,
    RS_DICE,                                    /* (A test's: the referee's unit tests, tests/ram/t_rules) Its dice
                                                **   fixed: x the faces the next dice show, in turn, to a 0, and b
                                                **   every die's after them (0: the dice as they fall); a 1, its dice
                                                **   seeded anew from w1.  The reply what was rolled since the last:
                                                **   w1 the dice, c the faces forced but not rolled, x the first
                                                **   ones' sides (0 past the last) */
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
