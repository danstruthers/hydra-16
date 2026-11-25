/*
** referee.h - Dingus Quest Zero's referee skeleton (ENGINE-CRITIQUE.md's section 8, option (a), the user's choice,
** 2026-11-11: swappable rules a real goal): what every referee has alike, in one place, and what a ruleset gives it.
** A referee is the skeleton (games/dq0/rf/rf.c: its main loop, the hub's slots and the keyed dice, the words
** into a reply, the things to hand, the creatures' sheets' file; rf/sheets.c the file's opening), the house's rules
** it wants (rshouse.h: linked functions, rf/*.c), and its ruleset's answers (rf_answer and the hooks below): dq0harn
** (HarnMaster's) and dq0null (flat odds).  A referee's folder compiles each shared file through a file of its own that
** #includes it, its #pragmas first where the referee wants its code (the main RAM, an overlay, a swapped part: the
** room's), so the skeleton's code is one source and each referee's place for it its own.
**
** Include it after hub.h and rs.h.
*/

#ifndef _REFEREE_H
#define _REFEREE_H

/* ---- The skeleton's (rf/rf.c) */

extern char* tp;                                /* The words into a reply: where the next goes, */
extern char* tend;                              /*   and the end (the request's text's room: the denizens' DTEXT) */
void __fastcall__ put (const char* s);          /* Words put (s may be in the window, an overlay's: each character
                                                **   taken with s's bank in, put with the hub's) */
void __fastcall__ putn (int v);

#define NTOOLS  20
extern char tools[NTOOLS][8];                   /* What's to hand (RS_EQUIP's): each one's key, */
extern unsigned char tflag[NTOOLS];             /*   T_HELD, T_WORN, its hands, */
extern unsigned char tqual[NTOOLS];             /*   its quality (0-9), and its letters' bits (TQ_*) */
#define TQ_LIGHT 0x10                           /* (tqual's high bits: a weapon's version, light; heavy; none average:
                                                **   D13's, the book's custom weapons, Combat 3) */
#define TQ_HEAVY 0x20
#define TQ_DULLED 0x40                          /* (tqual's top bits: its edge dulled, a point less cutting harm; dull, */
#define TQ_DULL  0x80                           /*   two: the house's, the user's, 2026-11-03) */
#define TQ_HEFT(q)      ((q) & TQ_LIGHT ? 0 : (q) & TQ_HEAVY ? 2 : 1)
#define T_HELD  0x01
#define T_WORN  0x02
#define T_LEFT  0x04                            /* (Held in the left hand: RS_EQUIP's marks, slice 4.1b's) */
#define T_RIGHT 0x08                            /*   the right; both, a thing in both hands */
void __fastcall__ equip_all (const char* keys); /* All that's to hand anew (RS_EQUIP 2's text) */
extern unsigned char tgen;                      /* (What's to hand changed: bumped by equip_all, and by a ruleset's own
                                                **   change to tools; what it keeps by the keys, kept by this) */
unsigned char __fastcall__ has (const char* key);   /* A tool to hand, by its key ("picks") */

extern unsigned long bseed;                     /* The bearings' own dice (rshouse.h's bdice) */

void sheets_in (void);                          /* The creatures' sheets read back, once, a game carried on (the
                                                **   skeleton's, as RS_NEW_ACTOR's and RS_SAVE's are answered) */
int __fastcall__ sheets_open (unsigned char back);  /* (rf/sheets.c's) The save folder's beasts.dat opened, to read
                                                **   (back 1) or written anew (0): its fd, or -1 (none, no save folder) */
unsigned __fastcall__ num (const char* s);      /* (Its arguments' numbers: the game's library's, dq0lib.inc's; or
                                                **   the referee's own) */
void __fastcall__ dice_key (unsigned char s);   /* (The keyed dice: ../dq0/dice.inc's, a referee's .s includes it;
                                                **   slot s's request's key, hashed into the generator's state:
                                                **   0, 1 the hub's slot[], NSLOTS DSLOT) */

/* ---- The ruleset's (its own files) */

unsigned char __fastcall__ rf_start (int argc, char* argv[]);  /* Its start, before the hub's attached (its arguments
                                                **   checked: SEG ROLE ...; its overlays, its banks): 0, or not 0 */
void rf_begin (void);                           /* As its dice are seeded (the game's seed): its own beginning */
void __fastcall__ rf_answer (struct req* r);    /* A request answered (tp and tend at its text, its result 0): RS_SAVE's
                                                **   the character's alone, the skeleton's the creatures' sheets after */
void __fastcall__ rf_sheets (unsigned char back);   /* Its creatures' sheets written to beasts.dat (0), read back (1) */

/* ---- The summary after a reply (the user's T4, 2026-11-22: "the referee's reply carries the blow's own summary fields,
** the rest of the summary recomputed after it").  A reply carries the character's summary's fields its request changed
** that the world reads as the reply comes (rs.h's struct rs_summary: cond, health, limbs, limp, step and gstep; and the
** status line, pcstatus).  The rest of it (the senses, morale, load, power, lead, the map in mind, its traits: what no
** blow, harm, time or exertion changes) a ruleset may leave for after the reply: rf_answer sets rf_later, and the
** skeleton, its reply sent, lets the asker run first (a tick at most, at the real paces; at the turns pace none, the
** tasks' turns as they were), then calls rf_after, always before it answers anything else.  A ruleset that keeps its summary whole in rf_answer never sets rf_later; its rf_after does nothing.
** rf_after rolls no dice (the dice go by each request's key: there's none) */
extern unsigned char rf_later;                  /* (The skeleton's: set by rf_answer, cleared as rf_after's called) */
void rf_after (void);                           /* The ruleset's: its summary's rest, after a reply that left it */

#endif
