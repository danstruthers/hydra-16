/*
** hacct.h - HarnMaster's part of the account of the time between two dungeons (../../dq0/rsmeta.h's acct_* hooks, linked into the
** engine's account as dq0acct: games/dq0acct): what its main RAM (hooks.c: the hooks, the character's sheet as the referee's) and
** its overlays share (body.c: the referee's code, the days at rest; skill.c: the wages, the occupation's skills, practice;
** sheet.c and hfile.c: the sheet; words.c: the wounds' words).  Its overlays are the account's ACCT_OV_RS on (acct.h), in that
** order: none of their code may look at a kind.
*/

#ifndef _HACCT_H
#define _HACCT_H

#include "../../dq0/acct/acct.h"

#define WEAPON_MAX      70                      /* (No weapon past this by practice: the book's, Skills 18) */
#define PHYS_ML         60                      /* (A town physician's: the house's) */
#define MASTER_ML       80                      /* (The master it works for: the book's bonus below that: the house's) */

#define OV_BODY         (acct_b + ACCT_OV_RS)   /* (Its overlays' banks) */
#define OV_SKILL        (acct_b + ACCT_OV_RS + 1)
#define OV_SHEET        (acct_b + ACCT_OV_RS + 2)
#define OV_WORDS        (acct_b + ACCT_OV_RS + 3)

/* hooks.c */
extern unsigned char used[HARN_NSKILLS];        /* (Its growth's lines: what it's used since its last night's rest) */
#define NTONGUES        8                       /* (Its tongues and scripts past its own: the plan for the game between dungeons, step 8;
                                                **   the referee's store's, dq0harn.h's TONGUE_LINES, sheet.c's check: more refused, loudly) */
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
extern char dressed[64];                        /* (Its "dressed" line whole, "" none: the sheet's, read and written by
                                                **   body.c's o_dress) */
void __fastcall__ o_dress (unsigned char out);  /* The dressings read from it (0), or written into it (1) */

/* skill.c */
unsigned char o_wage (void);                    /* A month's pence */
void o_tool (void);                             /* A thing it has (ans: its tool's key, 8 its kind, 9 its flags; kind 0 begins): what it can practise with */
unsigned char o_needs (void);                   /* A thing (as o_tool's) its calling or its practice needs: never sold unasked */
void o_work (void);                             /* What the work taught: its bonus practice */
void o_practice (void);                         /* The question, the rolls, their words */

/* sheet.c */
unsigned char o_sheet_read (void);              /* The sheet read: 1, or 0 none */
void o_sheet_write (void);                      /* And written */

/* words.c */
void o_first (void);                            /* The physician's and the wounds' words */

#endif
