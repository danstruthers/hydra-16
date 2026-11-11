/*
** rsmeta.h - Dingus Quest Zero's rulesets as the programs between dungeons see them (ENGINE-CRITIQUE.md's section 8,
** 2026-11-11: the meta programs talk to "the ruleset" through this, not through a referee's own headers).  Two parts:
**
**   The rulesets' table (rulesets[]): each ruleset's referee (dq0 -r NAME's program, game.dat's referee) and the
**   program of its own that accounts for the time between dungeons (its healing, its skills' growth, the town: lib's
**   acct folder), or none (the time between isn't told, the game goes on as it did).  dq0life runs the one the save's
**   ruleset names.
**
**   The character's sheet for the scholar's house (dq0schol, and any meta program that reads a skill): the masteries
**   it asks of a ruleset's sheet by what they're for (MK_*), the base a master's teaching opens a tongue or a script
**   at, the sheet's lines read, the tongues and scripts a sheet holds.  A ruleset gives it as one file a meta program
**   compiles in (HarnMaster's ../dq0harn/meta/meta.c), so the program names no ruleset's own skills or headers.
*/

#ifndef _RSMETA_H
#define _RSMETA_H

/* ---- The rulesets */

struct rsmeta {
    const char*     referee;                    /* Its referee's program (game.dat's referee: "dq0harn") */
    const char*     accounts;                   /* Its program for the time between, in lib's folder ("/../acct/dq0acct");
                                                **   0 none */
};
#define RULESETS        { { "dq0harn", "/../acct/dq0acct" }, { "dq0null", 0 } }

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
                                                **   script (MK_SCRIPT) at, by the character's own aptitude */

#endif
