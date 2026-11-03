/*
** wroom.c - (dq0w) a room searched whole's tests at a stop (dq0w.c's wroom.inc; HOUSE.md's a room searched whole): the
** referee's P_SEARCH against each hidden trap in the block about wk.fx, wk.fy (the square and the eight round it), and
** each hidden door, and each door's hidden trap, in the walls beside those squares; each at the steps a search's are
** (wtrap.c's search, whose form this is; by touch, in the dark, 2 steps the harder).  Told as found, as a search's are;
** quiet if nothing is.  Swapped part 83's: its own copy of the work's helpers (workpart.h).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"

#pragma code-name ("SW83CODE")
#pragma rodata-name ("SW83RODATA")
#pragma local-strings (on)

#define W_TRAPS
#include "workpart.h"

/* What it found: 1 a door, 2 a trap, each a bit (a door's a door like any other from then on, a trap known) */
unsigned char __fastcall__ s_room_tests (unsigned char touch)
{
    static const unsigned char sq_[5] = { 4, 1, 0, 1, 4 };
    unsigned char found = 0, a, b, k, d, *p;
    signed char m = touch ? -2 : 0;
    unsigned i, j;

    for (b = 0; b < 5; ++b) {
        j = wk.fy - 2 + b;
        for (a = 0; a < 5; ++a) {
            i = wk.fx - 2 + a;
            if ((d = sq_[a] + sq_[b]) > 5 || (p = sq (i, j)) == 0) {
                continue;                       /* (Its corners out of reach) */
            }
            if (((d < 3 && trap_on (i, j, 0)) || trap_on (i, j, 1)) && (wk.tq.state & TS_HIDDEN)) {
                if (test (P_SEARCH, wk.tq.state & TS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL,
                    m - (signed char) wk.tq.hide) > 0 && !wres) {
                    trap_state (wk.tq.state & ~TS_HIDDEN);
                    told (E_TRAP, TO_FOUND << 8 | wk.tq.kind | wk.tq.trigger << 4 | (touch ? 0x80 : 0), 0);  /* (By touch:
                                                **   its bit 7, the fingers' words) */
                    found |= 2;
                }
                p = sq (i, j);
            }
            if (TILE (*p) != T_DOOR) {
                continue;
            }
            for (k = 0; k < C->nportals && (C->portal[k].x != cx_ || C->portal[k].y != cy_); ++k) {
            }
            if (k == C->nportals || !(C->portal[k].state & PS_HIDDEN)) {
                continue;
            }
            wk.pq = C->portal[k];
            if (test (P_SEARCH, wk.pq.state & PS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL, m) > 0 && !wres) {
                wk.px = i;
                wk.py = j;
                door_state (wk.pq.state & ~PS_HIDDEN);
                found |= 1;
            }
        }
    }
    if (found) {
        wk.m.b = touch != 0;                    /* (By touch: the hands' words, fixt.c's) */
        told (E_PORTAL, found & 1 ? PO_FOUND : PO_FOUND_TRAP, 0);
        wk.m.b = 0;
    }
    return found;
}
