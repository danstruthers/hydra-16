/*
** rsmeta.h - Dingus Quest Zero's rulesets as the programs between dungeons see them (ENGINE-CRITIQUE.md's section 8,
** 2026-11-11: the meta programs talk to "the ruleset" through this, not through a referee's own headers).  Three parts:
**
**   The rulesets' table (rulesets[]): each ruleset's referee (dq0 -r NAME's program, game.dat's referee) and its account
**   of the time between dungeons (lib's acct folder): the engine's account (games/dq0/acct) linked with that ruleset's
**   hooks, below.  dq0life runs the one the save's ruleset names.
**
**   The ruleset's part of the account (acct_*): what the engine's account asks of the ruleset between two dungeons (its
**   sheet, its healing, its practice, its living and wages); each ruleset's own code (HarnMaster's games/dq0harn/acct, the
**   null ruleset's games/dq0null/acct), linked into its account (DECISIONS.md's "dq0acct split").
**
**   The character's sheet for the scholar's house (dq0schol, and any meta program that reads a skill): the masteries
**   it asks of a ruleset's sheet by what they're for (MK_*), the base a master's teaching opens a tongue or a script
**   at, the sheet's lines read, the tongues and scripts a sheet holds.  A ruleset gives it as one file a meta program
**   compiles in (HarnMaster's ../dq0harn/meta/meta.c, the null's ../dq0null/meta/meta.c), so the program names no
**   ruleset's own skills or headers.
**
**   The town's and the scholar's house's rules (meta_test, meta_hired, meta_honest): a test's dice and its grade, a
**   hired reader's skill and honesty.  The town (games/dq0/town/town.c) and the scholar's house (schol.c) are the
**   engine's, each linked once per ruleset with these, as the account is (DECISIONS.md's "The town and the scholar's
**   house: the ruleset's hooks"): HarnMaster's dq0town and dq0schol (../dq0harn/meta), the null's dq0ntown and dq0nschol
**   (../dq0null/meta).  meta_test is a file of its own (test.c), so the town links it alone.
*/

#ifndef _RSMETA_H
#define _RSMETA_H

/* ---- The rulesets */

struct rsmeta {
    const char*     referee;                    /* Its referee's program (game.dat's referee: "dq0harn") */
    const char*     accounts;                   /* Its account of the time between, in lib's folder ("/../acct/dq0acct");
                                                **   0 none (the time between isn't told, the game goes on as it did) */
};
#define RULESETS        { { "dq0harn", "/../acct/dq0acct" }, { "dq0null", "/../acct/dq0nacct" } }

/* ---- The ruleset's part of the account (games/dq0/acct/acct.h: what of the account's a hook may use, and which may put
** in an overlay of the ruleset's own).  Called in this order: acct_sheet_read; acct_wounds, then acct_day for each day
** till acct_mended or the healing's most, then for the road, the lull, the town's days and the days of work; acct_tool's
** needs before the money's matters (acct_habit, acct_wage), the things left after; then acct_rested, acct_first (or with
** the town, before it), acct_worked, acct_practice, acct_sheet_write.  Dead of it (acct_wounds or acct_day 0), acct_death
** and acct_sheet_write. */

struct between;
unsigned char acct_sheet_read (void);           /* The character's sheet (META_SHEET) read: 1, or 0 none (nothing to tell) */
void acct_sheet_write (void);                   /* And written back, as the time between left it */
unsigned char acct_wounds (void);               /* Its wounds as it comes out: what's done at once (a physician paid from
                                                **   funds into fee, the account's): 1 alive */
unsigned char acct_mended (void);               /* Nothing left that a day's rest would better */
unsigned char acct_day (void);                  /* A day at rest (the account counts it): 1 alive */
const char* acct_death (void);                  /* What it died of, in words (" The infection kills you.") */
void acct_rested (void);                        /* Rested, going on into the next dungeon (its fatigue clear) */
const char* acct_calling (void);                /* Its calling, in words ("" none) */
unsigned char acct_wage (void);                 /* A month's wage at its calling, pence (the work's: half a master's) */
unsigned char acct_habit (void);                /* A day's living by its station, pence (1, 2, 3 or 8: the plan's lodgings) */
unsigned acct_birthday (void);                  /* Its birthday, its month << 8 | its day (life.h's LIFE_AGE's) */
unsigned char acct_tool (unsigned char needs);  /* A thing carried, told in ans (0-7 its tool's key, 8 its kind, 9 its
                                                **   flags; kind 0 begins): what it can practise with (needs 0); or (1)
                                                **   1 if its calling or its practice needs it (never sold unasked) */
void acct_first (void);                         /* The words of its wounds: what was done, what came of them */
void acct_worked (void);                        /* What the days of work taught it, after the work's words (work_days) */
void acct_practice (void);                      /* The months' practice (passed, less town_taught): asked, rolled, told */
void __fastcall__ acct_town (struct between* t);    /* Its knacks for the town: t's rhet (the market's haggling) and intr
                                                **   (the inn's listening), 0-100 */
unsigned char __fastcall__ acct_learnt (const char* what);  /* What a master opened (the record's learnt: "*delvic=81" a
                                                **   script, "sindarin=11" a tongue) put on its sheet: 1, or 0 no room */

/* ---- The sheet, for the meta programs */

enum { MK_LANGUAGE, MK_SCRIPT, MK_FOLKLORE, MK_RECKON, MK_ASTROLOGY, MK_HERALDRY, NMK };   /* (What a mastery's for:
                                                **   its own tongue, its native script, lore, reckoning, the stars,
                                                **   heraldry) */
#define META_TONGUES    8                       /* (The tongues and scripts a sheet holds past its own, at most: the
                                                **   referee's store's; one with more refused, loudly, never cut short) */
#define META_SHEET      "pc.hm"                 /* (The character's file in the save's folder) */

void __fastcall__ meta_line (char* l, void __fastcall__ (*own) (char* l));   /* A line of the sheet read: what the
                                                **   ruleset keeps of it (its attributes, the masteries above), any
                                                **   other line given to own */
unsigned char __fastcall__ meta_ml (unsigned char mk);      /* A mastery (0-100 and on; 0 not opened) */
unsigned char __fastcall__ meta_taught (unsigned char mk);  /* What a master's month opens a tongue (MK_LANGUAGE) or a
                                                **   script (MK_SCRIPT) at, by the character's own aptitude; 0 opened with
                                                **   no mastery (a ruleset with none: its sheet's line the name alone) */

/* ---- The town's and the scholar's house's rules */

signed char __fastcall__ meta_test (int chance);    /* A test at chance (a knack in the town's record, a mastery, a hired
                                                **   reader's skill, its steps added: 0-100 and past either end): its grade,
                                                **   rs.h's G_* (G_BADLY, G_FAIL, G_PASS, G_WELL), by the ruleset's dice */
unsigned char __fastcall__ meta_hired (unsigned char sage); /* A hired reader's skill, a chance for meta_test: a scribe's
                                                **   (sage 0; a sage asked one of the other skills too), a sage's of a dead
                                                **   tongue (1); its dice the ruleset's */
enum { MH_LIES, MH_DEAR, MH_SELLS, MH_HONEST }; /* (A hired reader's honesty, worst first: tells a wrong reading as
                                                **   confidently; asks half again; sells a copy on; none of it.  Each worse
                                                **   one does the better ones' too) */
unsigned char __fastcall__ meta_honest (unsigned char set); /* A hired reader's honesty (MH_*): set a figure a test's
                                                **   scene gave (the rc variable dq0sg's, in the ruleset's own scale), 0xFF
                                                **   none: the ruleset's dice */

#endif
