/*
** rshouse.h - Dingus Quest Zero's house rules that every referee applies alike (HOUSE.md's: the game's own, not a
** ruleset's), so a second ruleset gives the same game where the rules aren't its own.  Linked functions (ENGINE-
** CRITIQUE.md's section 8, 2026-11-11: once static code in headers switched by #defines, this one, rswant.h and
** rsair.h), each defined once in the referee skeleton's folder (rf/dig.c, rf/notice.c, rf/state.c, rf/bdice.c,
** rf/want.c, rf/air.c), each referee compiling the ones it uses where its room is (a file of its own that #includes it,
** its #pragmas first: dq0harn's in its overlays and parts, dq0null's in its main RAM: ../dq0harn/h*.c,
** ../dq0null/house.c); their words through the skeleton's put and putn, the tools to hand the skeleton's
** (referee.h's).  And what's an expression, not a function (what a thing does swung, the world's too; moving quietly's
** and an appraisal's own dice, about a test), macros.  Include it after hub.h, rs.h and referee.h.
*/

#ifndef _RSHOUSE_H
#define _RSHOUSE_H

/* ---- The tools that dig (HOUSE.md: digging; rs.h's P_DIG; rf/dig.c): each one's key (the things' catalogue's), its
** half-feet a spell of work, and the wear a spell does it, by what it's into (rs.h's MAT_*: hard-packed earth, stone,
** masonry, a floor's flags, soft soil); bare hands the first.  The user's, 2026-10-20: earth is dug at the tool's rate,
** no test (bare hands half a foot of soft soil, nothing of hard-packed earth; every tool half as far in hard-packed
** earth as in soil); anything harder only with a pick, a Mining test each spell: its rate at a marginal success,
** twice at a critical, half at a marginal failure, none at a critical failure */
#define RS_DIG_SHOVEL   { 3, 0, 0, 0, 6 }       /* (A shovel's rates and a pick's, the table's: every actor's, the
                                                **   referee's alone since the one-path plan's digging, 2026-11-23) */
#define RS_DIG_PICK     { 2, 4, 2, 6, 4 }
struct digger { char key[8]; unsigned char rate[5], wear[5]; };     /* (Earth, stone, masonry, flags, soil) */
extern const struct digger digger[];
#define NDIGGERS        10                      /* (A burrower's claws the last, 2026-11-23) */
#define DIG_TESTED(m)   ((m) != MAT_EARTH && (m) != MAT_SOIL)   /* (Earth dug at the tool's rate; the rest a Mining test) */

/* The tool to hand that suits the material best (bare hands, digger's 0, if none does better; of two as good, the
** one it wears less), or the one named (name's key: the user's, 2026-11-03: a lesser tool named is used): its digger's
** number (0 too if what's named is no digger).  What's to hand: the character's (list 0: tools, RS_EQUIP's), or an
** actor's, list its keys in the same form (eight bytes each, NTOOLS at most, the first empty one the last: the world's,
** its things' and its claws; the one-path plan's digging) */
unsigned char __fastcall__ house_dig_tool (unsigned char m, const char* name, const char* list);

/* A dig's tool and its rate, every actor's (RS_TEST's P_DIG, r: its material; the character's text the tool named, ""
** the best to hand; an actor's, r->actor not 0, its tools listed, none named): the reply's text its key (and a better
** one named at DIG_BETTER, the character's), its wear, its rate at a marginal success (RS_TEST_RATE), RS_TEST_CANT 1 if
** nothing bites into it; the rate returned (0 none).  The test and the rate by its grade are the referee's */
unsigned char __fastcall__ house_dig (struct req* r);

/* ---- What a thing does swung (HOUSE.md's fights and portals, the user's, 2026-10-22: "I think it should try and use
** whatever you request, but if it is something soft ... it should reply with something like 'You attempt to batter the
** door with the noodle, but it seems ineffectual'"): a thing named to strike with (or the one in its hands) that isn't
** a weapon of the ruleset's, by its kind's data (items.h's struct kind, the catalogue's): soft (IM_SOFT) if it's made of
** something that gives (cloth, leather, wax, tallow, food and the like) or only breaks (glass, pottery), if it's a heap
** or a lot (sand, pennies, arrows), anything worn (armour, clothes: not made to swing; worn and not held: a shield or a
** bow slung isn't worn to swing, it's held, 2026-10-27), under half a pound, or lighter than its bulk (hollow, loose: a
** basket, a pack, straw): one attempt, told, no harm; else light (IM_LIGHT, under two and a half pounds: the book's
** stick, HarnMaster's referee) or hefty (IM_HEFTY: its club).  The ruleset's own weapons by their keys first, the
** referee's.  An expression of k (a const struct kind*): the world's parts and the referee each their own use of it */
enum { IM_SOFT, IM_LIGHT, IM_HEFTY };
#define IM_HARD         ((1u << MT_WOOD) | (1u << MT_IRON) | (1u << MT_STEEL) | (1u << MT_BRONZE) | (1u << MT_BONE) | \
                         (1u << MT_STONE) | (1u << MT_SILVER))
#define HOUSE_SWING(k)  (!(IM_HARD & 1u << (k)->mat) || ((k)->form & (KF_SMALL | KF_LIQUID | KF_GAS | KF_COUNT)) || \
                         ((k)->flags & (K_WEAR | K_HAND)) == K_WEAR || (k)->weight < 2 || (k)->weight < (k)->bulk ? \
                         IM_SOFT : (k)->weight < 10 ? IM_LIGHT : IM_HEFTY)

/* ---- What the character notices, by its going and its watchfulness (HOUSE.md: noticing; DUNGEON.md: moving; rf/
** notice.c): its pace as it moves (a creep or a slow walk a step better, but not by feel in the dark; a jog two worse,
** a run four, a sprint six); listening, a sound two better.  A hazard underfoot (P_NOTICE_SIGHT's b 1, its steps the
** hiding's) by a search's chance while it searches as it goes, else a passing glance's: twice its hiding's steps, and
** two more.  The steps more for request r (go: the hub's pc_going; watch: its watch, WA_*); the ruleset's own (the
** character's state ...) the referee's to add */
signed char __fastcall__ house_notice (const struct req* r, unsigned char go, unsigned char watch);

/* ---- The character's state in words (the status command's, RS_DESCRIBE b 3; rf/state.c): its pace and what it's
** held to (the world's: a slow walk at most while it searches or maps as it goes, or feels its way in the dark; a walk
** while it listens), "Pace: a jog, held to a slow walk while you search as you go", the pace it's held to returned (the
** referee's words after it: its feet a round); its watchfulness, ".\nWatching: ..."; its light, "\nLight: ..." (the
** referee's condition before it: light its reach in squares, lamp what gives it, dark its square unlit) */
#define HOUSE_SHOCK     0x80                    /* (house_pace's watch |: the ruleset's, in shock; on one good leg) */
#define HOUSE_LEG       0x40
unsigned char __fastcall__ house_pace (unsigned char g, unsigned char watch, unsigned char dark);
void __fastcall__ house_watching (unsigned char watch);
void __fastcall__ house_light (unsigned char light, const char* lamp, unsigned char dark);

/* ---- The bearings' own dice (rs.h's P_BEARING; HOUSE.md's bearings underground; rf/bdice.c): the generator's state
** (dice.h's numbers: xorshift.s's xs_seed) swapped for the bearings' own about their test, before it and after, so the
** game's other tests roll as they did whether its walks bend or not; the bearings' state the skeleton's bseed, begun at
** the first from the generator's as it is then (its own seed's: the game's), scrambled */
void bdice (void);

/* ---- Moving quietly's own dice (rs.h's P_STEALTH, the character's and a creature's: HOUSE.md's sound, the air and
** moving quietly): the game's dice as they stand, mixed (QUIET_MIX: a step of another generator, a linear congruential
** one's, so not the next roll's own bits), for the test; the game's put back after (QUIET_IN, QUIET_OUT about it), so
** its other tests roll as they did whether anyone creeps or stalks; nothing kept (the referees' main RAM's room).  Since
** the keyed dice (dice.inc's, dice2, 2026-11-24) every request's dice are its own anyway, the game's state its key's:
** these matter only to a test's own stream (RS_DICE's seed, t_rules) */
extern unsigned long xs_seed;
#define QUIET_MIX(t)    ((t) * 1103515245UL + 12345UL | 1)
#define QUIET_IN        { unsigned long qt_ = xs_seed; xs_seed = QUIET_MIX (qt_);
#define QUIET_OUT       xs_seed = qt_; }
/* (An appraisal's die, the user's, 2026-11-03: "keep the first answer", per thing, till the skill improves: no store: the
** dice for it a hash of the thing (w1, its key: the UI's and the world's), the skill's mastery and the game's seed; the
** game's put back after) */
#define JUDGE_IN(k, ml) { unsigned long qt_ = xs_seed; xs_seed = QUIET_MIX (QUIET_MIX ((unsigned long) (k) << 16 ^ \
                            (unsigned long) (ml) << 8 ^ (unsigned) HUB->seed));
#define JUDGE_OUT       xs_seed = qt_; }

/* ---- Hunger and thirst (slice 4.9: HOUSE.md's water, hunger and thirst; rf/want.c): asked with the bodies' time
** (RS_TIME, the world's: w1 the minutes gone, copied into x[16..17] by the referee before its reply's w1; a what the
** character was about, 2 vigorous work: twice the want) and as it eats and drinks (RS_TIME, the UI's: w1 0, x[14] the
** meals eaten, | 0x80 a day's ration begun, x[15] the pints drunk); the hub's want in x[8..13] (pc_hunger, pc_thirst,
** pc_want, pc_meals: the referee's own, copied in and back by its main code), anew; its levels' rising told (its words
** the ruleset's want_say's, news: grade 2, and 8, the world's long task asked about, soft); the clock's next ask (the
** reply's x[4..7]) the sooner for its next level.  want_of's result: the fatigue levels from want, | WANT_DIES dead of
** it (its words told) */
#define WANT_X          8                       /* (The request's x: the hub's want, from pc_hunger on) */
#define WANT_DIES       0x80
#define WANT_MEAL       480                     /* (A meal: eight hours of hunger off) */
#define WANT_PINT       360                     /* (A pint: six hours of thirst off) */
unsigned char __fastcall__ want_of (struct req* r, unsigned long now);
void __fastcall__ want_say (const char* s);    /* (The ruleset's: words into its news, RS_DESCRIBE b 4's) */

/* ---- The air the character breathes (2026-10-31: HOUSE.md's bad air; rf/air.c): asked with the bodies' time (RS_TIME,
** the world's: a minute at a time while the character's in bad air or its head's not clear, x[20] the air where it
** stands, rs.h's AQ_*, x[21] its flame, FLM_*).  air_of's result: 1 dead of it, 2 news, 4 worse (a hard stop: its long
** task); air_lv the fatigue levels its breath costs it now (HarnMaster's: the book's universal penalty; the null
** ruleset's none); its words the ruleset's air_say's */
extern unsigned air_m;                          /* (Minutes in the blackdamp it's in) */
extern unsigned air_co;                         /* (Whitedamp in its blood: the doses) */
extern unsigned char air_lv;                    /* (Its breath's fatigue levels as last told) */
unsigned char __fastcall__ air_of (unsigned char q, unsigned char f, unsigned m);
void __fastcall__ air_say (const char* s);     /* (The ruleset's: words into its news, RS_DESCRIBE b 4's) */

#endif
