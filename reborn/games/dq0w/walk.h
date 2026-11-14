/* walk.h - (the world's dq0w.c, part 21: the character's ways across a room or about a passage; the denizens'
** dq0d.c, part 18: an actor's way to its mark) one way-finder over one rule for a step (ENGINE-CRITIQUE.md's §4,
** step 2's second part: one source 2026-11-09, walk.inc compiled into each; the paged ROM's 2026-11-14, the game's
** library's modules/dq0lib/walk.s, its contract the zero page's block, ../dq0/lnzp.inc's LZ_CL ..., named here).
**
** The rule (the one chunk.c's step_ok and slant.s's slant go by, a step at a time): a square can be stood on if its
** kind can (world.h's T_PASS: not rock, a wall, a window, a pit, a cut), and a door's only if it's open (or broken
** down) and not hidden; a step to the square beside it rises or falls no more than STEP_MAX feet (STAIR_MAX on steps,
** either end); a diagonal step only a diagonal way's, past two corners neither of which can be stood on (rock, a wall,
** a door, a window, a pit), one of them cut.  Each walker's differences, its rules (wk_rules), and what its reader
** adds as it reads the squares in (a square's door byte, WD_*: WD_NO, a square it won't go onto):
**
**   - WK_HANDS: a shut door (not a hidden one) a way, to be worked as it gets there (opened, a bar lifted, a key or a
**     pick tried, forced; a portcullis heaved: a denizen's door_way; the character's, a door it has handled opened as
**     it goes, across's door_on).  One with no hands (a beast) goes through no shut door.
**   - WK_NODOOR: no door's square a way, open or shut (the character's ways about a passage: it keeps to it, its
**     doors the walk's to stop at).
**   - the denizens': no water deeper than it wades (unless it swims or flies) and no bad air (dq0d's wades, the same
**     its steps go by); a chunk not made, none (never made for a way: read in as rock).
**   - the character's: no square a denizen stands on, nor one with a trap it knows of (a hidden one's on its way: the
**     way's first, WD_TRAP, its cell's WC_TRAP); its times by its pace (wk_reach's: cost's, a diagonal seven feet).
**
** The squares read in a row at a time (wk_row: their kinds' bytes made their cells, by the rule, from their door
** bytes); the search (wk_search) over a grid of CW by CW cells (wk_cl: a chunk's squares, the world's; a field about
** the actor and its mark, the denizens'), a cell a byte, each square's floor beside it (wk_el), from the cells queued
** (wk_seed: the character's square; the mark, or the field's edges toward it; the queue wk_ring, WK_RN squares)
** outward a step at a time, the four ways then the diagonal ways in dir_x's order, the first way to each square found
** kept: its cell then the way it was come into (0-7, dir_x's; WC_SEED a seed), WC_STEPS and WC_CORNER kept.  Within
** the box wk_x0, wk_y0 (wk_w by wk_h: the room's floor and walls, the world's); stopped as it comes to wk_stop (the
** actor's square, the denizens'); each square reached told to wk_reach, if it's set (wk_u from, wk_v reached, wk_e
** the way, wk_dz its floor above u's, wk_v8 its cell before it was reached: the world's times and traps).  The grid,
** the floors and the queue are in the bank in the window as it searches */

#include "../dq0/lnzp.h"

#define WC_NEW          0x80                    /* Not reached (yet, or ever: one that can't be stood on) */
#define WC_OPEN         0x40                    /* It can be stood on, by the walker's rules */
#define WC_CORNER       0x20                    /* A diagonal may pass its corner: not stood on (cornk's), or cut */
#define WC_STEPS        0x10                    /* Steps (STAIR_MAX) */
#define WC_CUT          0x01                    /* (Not reached) its corner cut: a diagonal way's (T_CUT) */
#define WC_TRAP         0x02                    /* (Not reached) a trap on it the walk would set off: the world's */
#define WC_SEED         8                       /* (Reached) where the search began: the character's square, the mark */
#define WC_WAY          0x0F                    /* (Reached) the way it was come into: 0-7, dir_x's; or WC_SEED */
#define WK_HANDS        0x40                    /* (wk_rules) A shut door a way: it works it */
#define WK_NODOOR       0x80                    /*   No door's square a way */
#define WK_RN           512                     /* The queue's ring (two of the search's fronts at most) */
#define WD_NONE         0x80                    /* (A square's door byte: no portal on it; else its PS_HIDDEN, PS_OPEN) */
#define WD_NO           0x40                    /*   (The walker won't go onto it) */
#define WD_TRAP         0x10                    /*   (A hidden trap on it, the world's: WC_TRAP if it's open) */
#define WD_PORTAL(q)    ((q) ? (q)->state & (PS_HIDDEN | PS_OPEN) : WD_NONE)   /* (Portal q's door byte, or none's) */

extern unsigned char* wk_cl;                    /* (The grid: CW * CW cells) */
extern signed char* wk_el;                      /* (Each square's floor) */
extern unsigned* wk_ring;                       /* (The queue: WK_RN squares) */
extern unsigned char wk_rules;
extern unsigned char wk_x0, wk_y0, wk_w, wk_h;  /* (The box searched) */
extern unsigned wk_qh, wk_qt, wk_stop;          /* (The queue's head and tail; the square it stops at, 0xFFFF none) */
extern void (*wk_reach) (void);                 /* (Each square reached, or 0) */
extern unsigned wk_u, wk_v;                     /* (wk_reach's: the square reached from, the square, */
extern unsigned char wk_e, wk_v8;               /*   the way, its cell before, */
extern signed char wk_dz;                       /*   its floor above u's) */
extern unsigned char *wk_rt, *wk_rd, wk_rn;     /* (wk_row's: the kinds' bytes, the doors', how many) */
#pragma zpsym ("wk_cl")
#pragma zpsym ("wk_el")
#pragma zpsym ("wk_ring")
#pragma zpsym ("wk_rules")
#pragma zpsym ("wk_x0")
#pragma zpsym ("wk_y0")
#pragma zpsym ("wk_w")
#pragma zpsym ("wk_h")
#pragma zpsym ("wk_qh")
#pragma zpsym ("wk_qt")
#pragma zpsym ("wk_stop")
#pragma zpsym ("wk_reach")
#pragma zpsym ("wk_u")
#pragma zpsym ("wk_v")
#pragma zpsym ("wk_e")
#pragma zpsym ("wk_v8")
#pragma zpsym ("wk_dz")
#pragma zpsym ("wk_rt")
#pragma zpsym ("wk_rd")
#pragma zpsym ("wk_rn")

#define wk_row(t, d, n) (wk_rt = (t), wk_rd = (d), wk_rn = (n), lib_x (WK_ROW))    /* (n squares' cells, by the rule) */
#define wk_seed(i)      (wk_v = (i), lib_x (WK_SEED))  /* (Square i queued, a seed) */
#define wk_search()     lib_x (WK_SEARCH)       /* (The search, from the squares queued) */
