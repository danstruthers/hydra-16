/*
** acts.h - (dq0w) the act table's row read, and the one senders by who (GAME-CRITIQUE.md's E6, ONE-PATH.md's 2.1: the
** one-path plan's stage 1, 2026-11-19).  The rows: games/dq0w/acts.txt, made by acts.js into the paged ROM's library
** (modules/dq0lib/table.s: AC_ROW, no main RAM) and obj/gen/dq0acts.h (AC_*, struct act).  A part reads a row into its
** own block (act_get: an XCALL through rp_x, some 150 cycles: an act's, never a loop's), its time by the actor's step
** (ACT_T).  The senders, every actor's by who (0 the character, else its actor's entry plus 1):
**   act_noise (who, ac, x, y)    the act's noise at square x, y: the character's E_NOISE (the row's loudness and
**                                NZ_*), an actor's E_DEN (its DE_*: told seen or heard, wport.c's den_told): its row's
**                                time; part 69's
**   act_test (who, p, steps)     the referee's test (RS_TEST, its actor who; the character's in SLOT_UI, the things'
**                                channel the UI waits on, an actor's in SLOT_WORLD): its grade, or -100 the tool it
**                                wants none (lockpicks): part 69's (wfight.c's s_den_test and things.c's tested before,
**                                each one side's).  The world's door and dig work test by ask_w, its actor wk.who
**                                (workpart.h's test): one sender already
** An act's time is its row's for every actor (ACT_T); spent by the character's clock (dq0w.c's busy, a work's wk.tt; a
** thing's handling's, the UI's wait) or an actor's (bd.ready): built/2026-11-19-act1.md's open question.
*/

#ifndef _ACTS_H
#define _ACTS_H

#include "dq0acts.h"

#define AC_ROW_E        0x66                    /* (The library's entry: dq0lib.s's $A066) */
struct actq {
    unsigned char   op, id;                     /* AC_ROW_E; the act */
    struct act      r;                          /* Its row, AC_ROW's */
};
unsigned __fastcall__ rp_x (void* a);           /* (dq0lib.inc's: an XCALL of a block's entry) */

#define act_get(q, n)   ((q).op = AC_ROW_E, (q).id = (n), rp_x (&(q)))
#define ACT_T(r, st)    ((r).time & ACT_STEP ? ((r).time & ~ACT_STEP) * (st) : (r).time)

unsigned __fastcall__ act_noise (unsigned char who, unsigned char ac, unsigned x, unsigned y);
signed char __fastcall__ act_test (unsigned char who, unsigned char p, signed char steps);

/* ---- Search and hide, every actor's (the one-path plan's stage 2, 2026-11-21): the rules both sides read */
#define HIDE_OF(g)      ((g) == G_WELL ? 12 : (g) == G_PASS ? 6 : (g) == G_FAIL ? 2 : 0)  /* (A thing concealed (the
                                                **   conceal row's test): its hiding's steps by the grade g (a variable:
                                                **   it's read thrice), the user's: 12, 6, 2, at a critical failure 0,
                                                **   dust and corners) */
#define FIND_STEPS(h)   (1 - (signed char) (h)) /* (One hiding found by a search (P_SEARCH): its cover's steps worse, one
                                                **   easier; h its hiding, 1 + the cover's steps: ST_HID's, pc_hid's) */
void __fastcall__ den_search (unsigned char dark);  /* (Part 94's: wsearch.c) Body bd's search about it, the character's
                                                **   own (the search row's): its darkvision dark */

#endif
