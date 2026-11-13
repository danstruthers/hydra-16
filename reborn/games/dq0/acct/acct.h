/*
** acct.h - the account of the time between two dungeons, the engine's (acct.c, quest.c, town.c): what its main RAM and its
** overlays share, and what a ruleset's part of it (rsmeta.h's acct_* hooks) may use.  The account is one source linked once
** for each ruleset, with that ruleset's hooks (games/dq0acct: HarnMaster's, games/dq0harn/acct; games/dq0nacct: the null
** ruleset's, games/dq0null/acct), each link a program of its own in lib/acct, the rulesets' table's (rsmeta.h's RULESETS).
**
** Its banks: acct_b the kinds' (dq0w.cat's first part) and what's carried, at the window between calls; after it the
** overlays, one file (lib/acct/NAME.ovl, 8K each, the last as long as it is): the account's own two (ACCT_OV_QUEST: what
** solving brings, the standing, the rumours; ACCT_OV_TOWN: the town, the date, the console's width), then the ruleset's
** (ACCT_OV_RS on: as many as its link says, ACCT_NOV the whole count).  An overlay runs at $8000 in place of the kinds'
** bank, so none of its code may look at a kind.  A hook the account calls from its main RAM may put in its own overlays,
** and gives the kinds' bank back (ACCT_CATS) before it returns; one it calls from an overlay of its own (acct_habit,
** acct_town, acct_learnt) is the ruleset's main RAM's and puts in no bank.
*/

#ifndef _ACCT_H
#define _ACCT_H

#include "../rsmeta.h"
#include "town.h"

#define HEAL_MAX        90                      /* (The healing's days at most: the plan's) */
#define WORK_MAX        60                      /* (The days it'll work for its needs at most: the house's) */
#define PENNY           4UL                     /* (Farthings) */

#define ACCT_OV_QUEST   1                       /* (The overlays, by their bank after acct_b) */
#define ACCT_OV_TOWN    2
#define ACCT_OV_RS      3
extern unsigned char acct_b;                    /* (The kinds' bank) */
#define ACCT_CATS()     hy_bank (acct_b)

/* acct.c */
extern unsigned char askit;                     /* (The player asked: not the tests' dq0auto) */
extern unsigned char width;                     /* (The console's, for the words: town.c's width_read) */
void __fastcall__ say (const char* s);
void __fastcall__ sayn (unsigned long v);
void __fastcall__ amount (unsigned long f);
void __fastcall__ ask (char* b, unsigned char n);
void say_flush (void);                          /* (The words so far out on the screen) */
extern char ans[64];                            /* (A line typed: in the main RAM; the things told to the ruleset's acct_tool) */
extern const char* lib;
extern const char* top;
extern char path[96];
extern unsigned passed;                         /* (The days gone by) */
extern unsigned long funds, fee, earned;        /* (What it could pay with, the physician's fee, what the work earned: farthings) */
extern unsigned work_days;
extern unsigned long cash;                      /* (The money: farthings) */
extern unsigned char dead;                      /* (Dead of the time between: the ruleset's days said so) */
extern char town[16];                           /* (Where it comes out to, its inn) */
extern char inn[16];
extern unsigned char rate;                      /* (A day's living, pence: the money's matters') */
extern unsigned long living;                    /*   and all of it, farthings */
extern unsigned town_days;                      /* (The days spent in the town, its errands': dq0town's, between.dat's) */
extern unsigned char town_seen;                 /*   and what the player saw to there itself (TS_*) */
extern unsigned char town_taught;               /*   and the months it spent with a master (the scholar's house's, the plan's step
                                                **   12): the month's practice forfeit for each */
extern char town_study[10];                     /*   and the script it studied in the library: practised first ("" none) */
extern char ob[200];                            /* (The words' buffer, empty between them: town.c's dq0town's arguments) */
const char* __fastcall__ in_ (const char* folder, const char* name);   /* folder/name, in path (../meta/meta.c's) */
void __fastcall__ fail (const char* why);       /* "NAME: why" said, exit 1 (../meta/meta.c's) */

/* quest.c */
extern unsigned rw_pence;                       /* What solving brings: the pennies, */
extern signed char rw_stand;                    /*   the standing, */
extern char rw_thing[16];                       /*   a thing (a kind's name: "" none) */
void o_reward (void);                           /* What the dungeon just left brings (its theme's reward line, if solved) */
void o_reward_say (void);                       /*   told */
void o_standing (void);                         /* The standing in words (the account's last sentence's end) */
void o_rumours (void);                          /* The rumours told, one followed (quest.txt); none heard, Enter to go on */

/* town.c */
void width_read (void);                         /* The console's width read (../meta/meta.c's, in the overlay) */
extern unsigned long o_t;                       /* (o_date's date) */
void o_town (void);                             /* The town's name and its inn's (ans[0]: the inn's die) */
void o_date (void);                             /* Date o_t in words */
void o_lodging (void);                          /* The days in town, where it lodged, what it came to */
unsigned char o_town_go (void);                 /* The town (dq0town, run): 1 it ran, town_days and town_seen set */
void o_set_out (void);                          /* The road taken from the town, the date on by it; Enter to go on */
extern unsigned named[24];                      /* (The ids of what the player's named: the dungeon's game.dat's) */
extern unsigned char nnamed;
void __fastcall__ names_read (unsigned n);      /* The names given in dungeon n (a thing's: its id) */
void life_write (void);                         /* The record of the life written (L) */
unsigned char lull (void);                      /* The lull's days */
void o_workdays (void);                             /* The days of work told, what they earned */
extern struct life L;

#endif
