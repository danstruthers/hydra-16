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

/* ---- Links and lights, every actor's (the one-path plan's stage 2, 2026-11-23): the rules both sides read */
unsigned long __fastcall__ flight_t (unsigned char k, unsigned char st, unsigned feet, unsigned char up);   /* (dq0w.c's,
                                                **   the main RAM's: FLIGHT_T's one code) */
#define FLIGHT_T(k, st, feet, up)   flight_t ((k), (st), (feet), (up))
                                                /* (A flight's crossing, tenths: link kind k, the actor's step st, the drop
                                                **   between the floors in feet; a step's time a foot on a stairway, twice
                                                **   on a ladder, halved down; a long slope four feet along a foot of the
                                                **   drop, walked, eight tenths of a step a foot down, thirteen up:
                                                **   HOUSE.md's, slice 4.10's; the character's s_climb, an actor's
                                                **   den_flight) */
#define FLIGHT_WORK(st, feet)       ((unsigned) (((feet) + 1) * (st)) >> 1)  /* (A long slope climbed: its strenuous
                                                **   work, the character's GA_WORK; an actor's the one body clock's, to
                                                **   come) */
unsigned char __fastcall__ den_flight (const struct msg* m);    /* (Part 118's: wflight.c) Body bd up or down a flight
                                                **   (I_CLIMB's): 1, or 0 none within reach (the caller's) */
void __fastcall__ den_light (const struct msg* m);  /* (Part 119's: wlink.c) A light of body bd's own lit, put out */
void __fastcall__ den_lights (unsigned char op);    /* (Part 119's: wlink.c) Body bd's lights: DL_SUM its hub's alight
                                                **   made again; DL_MOVED, it stepped, lit (the light layer near it, a
                                                **   look); DL_DOUSE, each it carries put out (lain down to sleep: a rest
                                                **   puts a burning light out first, every actor's); DL_OUT, one burnt out */
#define DL_SUM          0
#define DL_MOVED        1
#define DL_DOUSE        2
#define DL_OUT          3                       /* (One of its lights burnt out: part 27's) */
void __fastcall__ den_lsum (unsigned char nf);   /* (Part 60's: wlimbs.c) Body bd's lights into the hub's alight */
extern unsigned char dlit[NACTORS];             /* (wlink.c's: each actor carrying a light lit, its reach) */

#endif
