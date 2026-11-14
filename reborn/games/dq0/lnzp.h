/*
** lnzp.h - (every reader of the chunks: the world, the denizens, the senses) C's side of lnzp.inc, the contract with
** the game's library for the squares' code in the paged ROM (modules/dq0lib: ENGINE-CRITIQUE.md's §4, step 2,
** 2026-11-14): the call registers named for C (lr0 ...: an entry's arguments), the call (lib_x: dq0lib.inc's), the
** chunk lookup's calls.  The line's names are line.h's, the way-finder's walk.h's; at.s (games/dq0w) defines them
*/

#ifndef _LNZP_H
#define _LNZP_H

#define AT_FIND         0x4B                    /* The library's entries (lnzp.inc's) */
#define CK_MADE         0x4E
#define CK_AT           0x51
#define LN_LINE         0x54
#define LN_PASS         0x57
#define LN_PORTAL       0x5A
#define WK_ROW          0x5D
#define WK_SEED         0x60
#define WK_SEARCH       0x63

extern unsigned lr0, lr1, lr2;                  /* (The call registers r0-r2: an entry's arguments) */
#pragma zpsym ("lr0")
#pragma zpsym ("lr1")
#pragma zpsym ("lr2")
unsigned __fastcall__ lib_x (unsigned char e);  /* (dq0lib.inc's: the library's entry e called, an XCALL) */
extern unsigned char at_me;                     /* (at.s's: ASK_*, the cache's slot this task has in use) */
void lz_init (void);                            /* (at.s's: the zero page's block made ready, as the task starts) */

/* Chunk cx, cy of stratum sn in the hub's cache (made): 1, else 0 (its stamp left; hub_map (0) after) */
#define ck_made(sn, cx, cy)     (lr0 = (sn), lr1 = (cx), lr2 = (cy), (unsigned char) lib_x (CK_MADE))
/* Square x, y of stratum sn's chunk in the cache: 1, else 0 (outside the stratum too; hub_map (0) after) */
#define ck_at(sn, x, y)         (lr0 = (sn), lr1 = (x), lr2 = (y), (unsigned char) lib_x (CK_AT))

#endif
