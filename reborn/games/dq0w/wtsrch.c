/*
** wtsrch.c - (dq0w) a search about the character or an actor (wtrap.c's till 2026-11-23: part 8's room, the
** one-path plan's step): the referee's test against each hidden door and trap within its reach, each found known (a
** door, a trap the character finds, for every one; a trap underfoot an actor finds, to its faction: the user's,
** 2026-11-23, "another's known only once found").  Swapped part 123's, tsearch called through its stub (the
** character's work, dq0w.c's; an actor's, wsearch.c's); its own copy of the work's helpers (workpart.h).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "work.h"

#pragma code-name ("SW123CODE")
#pragma rodata-name ("SW123RODATA")
#pragma local-strings (on)

#define W_TRAPS
#include "workpart.h"

/* A search about the character (a minute): the referee's test (P_SEARCH) against each hidden door and each
** hidden trap within a few squares that it sees (a trap harder the better it's hidden), the nearest first as far as
** its time went, each at the full chance (HOUSE.md: a search cut short covers the part done); a door found is a door
** like any other from then on, a trap known (told as it's found, and its reach, the UI's things' search the same); or
** (link 1) where the world's character stops as it searches as it goes, quietly and in no more time, all of them.  By
** its form (pd: PA_SEARCH's, SF_*, 2026-10-22, HOUSE.md's searching): nearby, or on the floor nearby, within reach (the
** eight squares round it and its own: a reach of 3); a wall's, that side's walls and doors (its heading pd's high bits:
** a door's trap, a hidden door; 2 steps the easier, the whole search on it); the floor's, its traps (no doors); for a
** thing or on one, none (the things' alone).  By touch (SF_TOUCH: in the dark), what's within reach whether it's seen
** or not, and 2 steps the harder (the eye not to help the hands).  Every actor's (the one-path plan's stage 2,
** 2026-11-21): an actor's (wk.who, wsearch.c's den_search) by the same tests (ask_w's, its sheet), its whole minute
** (its time spent at once, its own), the squares it sees its own (WK_SIGHT: by its eyes, not the character's), told
** nothing (wsearch.c's to tell; wk.res what it found); what it finds found for every one, as the character's finds are */
#define SEARCH_R        3
static const unsigned char sq_[] = { 9, 4, 1, 0, 1, 4, 9 };    /* (A square's distance off, squared, by its offset) */

extern unsigned char srch_p;                    /* (dq0w.c's: P_SEARCH, or a knock's P_NOTICE_SOUND, work's, 2026-11-10) */
void s_tsearch (void)
{
    unsigned i, j;
    unsigned char found = 0, k, r, a, b, f, touch, side, s;
    unsigned char* p;
    signed char m;

    f = wk.pd & 7;
    touch = wk.pd & SF_TOUCH;
    side = wk.pd >> 5 & 3;                      /* (A wall's: its heading's quarter, north, east, south, west) */
    m = touch ? -2 : f == SF_WALL ? 2 : 0;

    hub_map (0);                                /* (Its reach, r: a square searched if its distance off, squared, is
                                                **   less; 19 all of them, the corners' 18; one for each 32 tenths of
                                                **   its minute done, the hub's long task's, about as many squares as
                                                **   that part of the 49, the nearest first) */
    i = (HUB->pc_task.done + 31) >> 5;
    r = wk.link || wk.who || i > 19 ? 19 : i;
    if ((touch || f == SF_NEAR || f == SF_FLOOR) && r > 3) {
        r = 3;                                  /* (Within reach: its own square and the eight round it) */
    }
    for (b = 0; f < SF_FOR && b <= 2 * SEARCH_R; ++b) {
        j = wk.fy - SEARCH_R + b;
        for (a = 0; a <= 2 * SEARCH_R; ++a) {
            i = wk.fx - SEARCH_R + a;
            s = side & 1 ? a : b;               /* (A wall's: past the character that way) */
            if (sq_[a] + sq_[b] >= r || (p = sq (i, j)) == 0 || (!touch && !(wk.who ? WK_SIGHT[b] >> a & 1 :
                *p & T_VIS)) || (f == SF_WALL && (T_INSIDE (*p) || TILE (*p) == T_CORR ||
                (side == 1 || side == 2 ? s <= SEARCH_R : s >= SEARCH_R)))) {
                continue;                       /* (Beyond its reach, or unseen; a wall's: not that side's wall) */
            }
            if (((f != SF_WALL && trap_on (i, j, 0)) || (f != SF_FLOOR && trap_on (i, j, 1))) &&
                ((k = wk.who && wk.tq.trigger != TT_DOOR ? wk.armed : 0) ? wk.tq.hide && !(wk.tq.know & k) :
                wk.tq.state & TS_HIDDEN)) {             /* (An actor's floor trap hidden from its faction (its knowers'
                                                **   bit, wsearch.c's wk.armed): TRAP_KNOWN's) */
                if (test (srch_p, wk.tq.state & TS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL,
                    m - (signed char) wk.tq.hide) > 0 && !wres) {
                    keep (wk.tx, wk.ty, k ? wk.tq.state : wk.tq.state & ~TS_HIDDEN, k, TRAP_STATE);   /* (An actor's
                                                **   floor trap found its faction's to know, the user's, 2026-11-23;
                                                **   a door's found for every one; the character's its own, TS_HIDDEN) */
                    if (!wk.who) {              /* (By touch: its bit 7, the fingers' words; an actor's untold) */
                        told (E_TRAP, TO_FOUND << 8 | wk.tq.kind | wk.tq.trigger << 4 | touch << 4, 0);
                    }
                    found |= 2;
                }
                p = sq (i, j);
            }
            if (TILE (*p) != T_DOOR || f == SF_FLOOR) {
                continue;
            }
            for (k = 0; k < C->nportals && (C->portal[k].x != cx_ || C->portal[k].y != cy_); ++k) {
            }
            if (k == C->nportals || !(C->portal[k].state & PS_HIDDEN)) {
                continue;
            }
            wk.pq = C->portal[k];
            if (test (srch_p, wk.pq.state & PS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL, m) > 0 && !wres) {
                wk.px = i;
                wk.py = j;
                door_state (wk.pq.state & ~PS_HIDDEN);
                found |= 1;
            }
        }
    }
    if (wk.link | wk.who) {                     /* (Searching as it goes, the world's: quietly, in no more time; res 1
                                                **   if it found something; an actor's, untold) */
        if (!(wk.res = found) || wk.who) {
            return;
        }
    } else {
        wk.tt = 600;
    }
    wk.m.b = touch != 0;                        /* (By touch: the hands' words, fixt.c's) */
    told (E_PORTAL, (found & 1 ? PO_FOUND : found ? PO_FOUND_TRAP : PO_NOTHING) << 8, r);
    wk.m.b = 0;
}
