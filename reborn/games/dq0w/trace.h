/*
** trace.h - (dq0w) the traces on the floor (slice 4.1, DUNGEON.md's traces on the floor): what the world's main code
** (dq0w.c), its walk's part (step_on, part 23) and the bodies' (wbody.c, part 14) share with the traces' store
** (wtrace.c): a passing journalled (trod.inc's trod, in the part that makes it: the hub's traces' journal, TJ_AT),
** the journal taken into the store (part 28's tr_flush, the main loop's, as it fills; and the save's, the store kept),
** its chunks' files (part 30's), the traces read (part 29's traced: I_TRACE's), in words (part 31's), and a hunter's
** way along the character's trail (part 32's tr_scent).
*/

#ifndef _TRACE_H
#define _TRACE_H

#define TK_SWEPT        0x7F                    /* (A journalled passing's kind: a chunk kept swept, I_SWEPT's: its
                                                **   square one of the chunk's) */
#define TR_FLUSH        24                      /* (The passings journalled before the store takes them in: the main
                                                **   loop's; the journal's rings hold NJOURNAL, NTRAIL) */
#define TR_DIR          ((char*) 0x9F80)        /* (The game's lib folder, its dq0tr.dat's: in the world's own paging
                                                **   bank, ipage.c's, its last 128 bytes; s_start's, as the store's
                                                **   emptied) */
extern int trfd;                                /* (Its dq0tr.dat, the kinds' words, kept open: wtrword.c's; dq0w.c's) */

extern unsigned char tr_n;                      /* The passings journalled since the store last took them in (trod.inc's
                                                **   trod: each part's own that journals them) */

/* ---- The swapped parts' (bank.s's stubs) */
void __fastcall__ tr_flush (unsigned char keep); /* (Part 28) The journal taken into the store (keep 1: the store
                                                **   kept, the save's, WQ_SAVE: its chunks changed written to the save
                                                **   folder) */
enum { TF_KEEP, TF_OUT, TF_IN };
void __fastcall__ tr_file (unsigned char h, unsigned char what);   /* (Part 30) Directory entry h's chunk written to
                                                **   its region's file (TF_KEEP; TF_OUT, its records freed, the entry
                                                **   too), or read in (TF_IN) */
unsigned __fastcall__ traced (const struct msg* m);     /* (Part 29) The traces about the character read (I_TRACE's:
                                                **   the UI bank's tread): the tenths it took */
unsigned __fastcall__ tr_words (unsigned w);    /* (Part 31) Their words (and E_VIEW V_TRACED): w back */
unsigned char __fastcall__ tr_scent (unsigned char nose);   /* (Part 32) The way the character's scent goes from body
                                                **   bd's square (wbody.h's), by its nose (a hunter's, STEP_SCENT):
                                                **   H_*, or 0xFF none */

#endif
