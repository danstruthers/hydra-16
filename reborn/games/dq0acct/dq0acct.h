/*
** dq0acct.h - (dq0acct) what its main RAM and its two overlays share (dq0acct.c: the sheet, what's carried, the money, the words;
** body.c, overlay 1: the referee's code, the days at rest; skill.c, overlay 2: the wages, the occupation's skills, practice).  An
** overlay runs at $8000 in place of the kinds' bank, so none of its code may look at a kind.
*/

#ifndef _DQ0ACCT_H
#define _DQ0ACCT_H

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
extern char ans[64];                            /* (A line typed: in the main RAM) */
extern const char* lib;
extern const char* top;
extern char who[20];
extern char path[96];
extern unsigned char used[HARN_NSKILLS];        /* (Its growth's lines: what it's used since its last night's rest) */
extern unsigned passed;                         /* (The days gone by) */
extern unsigned long funds, fee, earned;        /* (What it could pay with, the physician's fee, what the work earned: farthings) */
extern unsigned work_days;
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
void o_first (void);                            /* The physician's and the wounds' words */
unsigned char o_wage (void);                    /* A month's pence */
void o_tool (void);                             /* A thing it has (ans: its tool's key, 8 its kind, 9 its flags; kind 0 begins): what it can practise with */
void o_work (void);                             /* The work's words, its bonus practice */
void o_practice (void);                         /* The question, the rolls, their words */

/* sheet.c */
unsigned char o_sheet_read (void);             /* The sheet read: 1, or 0 none */
void o_sheet_write (void);                      /* And written */

#endif
