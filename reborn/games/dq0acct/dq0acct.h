/*
** dq0acct.h - (dq0acct) what its main RAM and its overlays share (dq0acct.c: the sheet, what's carried, the money, the words;
** body.c, overlay 1: the referee's code, the days at rest; skill.c, overlay 2: the wages, the occupation's skills, practice;
** sheet.c, overlay 3: the sheet; quest.c, overlay 4: what solving brings, the standing, the rumours, the plan's step 7; town.c,
** overlay 5, a file of its own: the town, the plan's step 11).  An overlay runs at $8000 in place of the kinds' bank, so none of
** its code may look at a kind.
*/

#ifndef _DQ0ACCT_H
#define _DQ0ACCT_H

#include "town.h"

#define HEAL_MAX        90                      /* (The healing's days at most: the plan's) */
#define WEAPON_MAX      70                      /* (No weapon past this by practice: the book's, Skills 18) */
#define PHYS_ML         60                      /* (A town physician's: the house's) */
#define MASTER_ML       80                      /* (The master it works for: the book's bonus below that: the house's) */
#define WORK_MAX        60                      /* (The days it'll work for its needs at most: the house's) */
#define PENNY           4UL                     /* (Farthings) */

/* dq0acct.c */
extern harn_char pc;
extern char calling[20];
extern unsigned char askit;
extern unsigned char width;                     /* (The console's, for the words: skill.c's o_width) */
void __fastcall__ say (const char* s);
void __fastcall__ sayn (unsigned long v);
void __fastcall__ amount (unsigned long f);
void __fastcall__ ask (char* b, unsigned char n);
void say_flush (void);                          /* (The words so far out on the screen) */
extern char ans[64];                            /* (A line typed: in the main RAM) */
extern const char* lib;
extern const char* top;
extern char who[20];
extern char path[96];
extern unsigned char used[HARN_NSKILLS];        /* (Its growth's lines: what it's used since its last night's rest) */
extern unsigned passed;                         /* (The days gone by) */
extern unsigned long funds, fee, earned;        /* (What it could pay with, the physician's fee, what the work earned: farthings) */
extern unsigned work_days;
extern unsigned long cash;                      /* (The money: farthings) */
extern char town[16];                           /* (Where it comes out to, its inn) */
extern char inn[16];
extern unsigned char rate;                      /* (A day's living, pence: the money's matters') */
extern unsigned long living;                    /*   and all of it, farthings */
extern unsigned town_days;                      /* (The days spent in the town, its errands': dq0town's, town.dat's) */
extern unsigned char town_seen;                 /*   and what the player saw to there itself (TS_*) */
extern char ob[200];                            /* (The words' buffer, empty between them: town.c's dq0town's arguments) */
#define NTONGUES        8                       /* (Its tongues and scripts past its own: the plan for the game between dungeons, step 8) */
extern char tongue[NTONGUES][24];               /* (Its sheet's "language NAME=ML" and "script NAME[=ML]" lines, as they are: sheet.c's;
                                                **   practised, skill.c's) */
extern unsigned char ntongues;

/* Each injury's: the wound it was as the account began, in was (0xFF one come since) */
struct wwas {
    unsigned char   loc, aspect, sev, il, by, fever;
};
extern struct wwas was[HARN_INJMAX];
extern unsigned char wid[HARN_INJMAX], nwas;
extern unsigned char tended[HARN_INJMAX], ntended, selfnd, unpaid;

/* body.c */
void o_note (void);                             /* The wounds as they are now */
unsigned char o_day (void);                     /* A day at rest: 1 alive */
unsigned char o_bind (void);                    /* Its bleeding bound: 1 alive */
unsigned char o_mended (void);                  /* Nothing worse than minor left, no infection, shock, toxin or disease */
void o_treat (void);                            /* The physician */

/* skill.c */
void o_width (void);                            /* The console's width read */
unsigned char o_wage (void);                    /* A month's pence */
void o_tool (void);                             /* A thing it has (ans: its tool's key, 8 its kind, 9 its flags; kind 0 begins): what it can practise with */
void o_work (void);                             /* The work's words, its bonus practice */
void o_practice (void);                         /* The question, the rolls, their words */

/* quest.c */
extern unsigned rw_pence;                       /* What solving brings: the pennies, */
extern signed char rw_stand;                    /*   the standing, */
extern char rw_thing[16];                       /*   a thing (a kind's name: "" none) */
void o_reward (void);                           /* What the dungeon just left brings (its theme's reward line, if solved) */
void o_reward_say (void);                       /*   told */
void o_standing (void);                         /* The standing in words (the account's last sentence's end) */
void o_rumours (void);                          /* The rumours told, one followed (quest.txt); none heard, Enter to go on */

/* sheet.c */
unsigned char o_sheet_read (void);             /* The sheet read: 1, or 0 none */
void o_sheet_write (void);                      /* And written */

/* town.c (overlay 5, its own file: the plan's step 11) */
extern unsigned long o_t;                       /* (o_date's date) */
void o_town (void);                             /* The town's name and its inn's (ans[0]: the inn's die) */
void o_date (void);                             /* Date o_t in words */
void o_lodging (void);                          /* The days in town, where it lodged, what it came to */
unsigned char o_town_go (void);                 /* The town (dq0town, run): 1 it ran, town_days and town_seen set */
void o_first (void);                            /* The physician's and the wounds' words */
void o_set_out (void);                          /* The road taken from the town, the date on by it; Enter to go on */

/* skill.c's too */
unsigned char o_needs (void);                   /* A thing (as o_tool's) its calling or its practice needs: never sold unasked */

#endif
