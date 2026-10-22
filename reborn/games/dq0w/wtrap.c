/*
** wtrap.c - (dq0w) traps (DUNGEON.md: traps, pits and other nasty surprises), and searches: set off as their record
** says (a plate trodden, a wire walked through: the world's walk stops on them; a door opened: wdoor.c's), each
** judged by the referee: a pit fallen into (HC_FALL, its depth; a spiked one's spikes, HC_STRIKE) and climbed out of
** (the world's climb); gas resisted (P_RESIST_POISON) or choked on, senseless a while; an alarm a noise the denizens
** hear (E_NOISE); the rest dodged (P_DODGE_HAZARD) or struck by (HC_STRIKE: their aspect, dice and place, trap_harm's,
** their power more).  Noticed as the character steps on one (P_NOTICE_SIGHT, a smaller chance than a search's: it
** stops short), found by a search (P_SEARCH, as hidden doors are; magic's only by magical means); one known of is gone
** round (the world's walks), its door left shut, and disarmed (P_DISARM; magic only by magical means; a pit not at
** all).  Their states kept (TRAP_STATE), a pit's square T_PIT once it's given way.
**
** All of it swapped part 8's (../dq0/swap.inc: dq0w.cfg's SW8), traps () called through its stub (bank.s's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"

#pragma code-name ("SW8CODE")
#pragma rodata-name ("SW8RODATA")
#pragma local-strings (on)

#define W_TRAPS
#define W_HARM
#define W_SPRING
#include "workpart.h"

/* A trap stepped onto (W_STEP: tq's square; from fx, fy): noticed in time (a magical one never), or set off; the
** referee's chance by its hiding (a hazard underfoot, P_NOTICE_SIGHT's b 1: a passing glance's, or a search's as the
** character searches as it goes) */
static void step (void)
{
    if (!(wk.tq.state & TS_MAGIC) && G_OK (test (P_NOTICE_SIGHT, 1, -(signed char) wk.tq.hide))) {
        trap_state (wk.tq.state & ~TS_HIDDEN);
        trap_told (TO_NOTICED);
        return;
    }
    spring (1);
    if (!wk.climb) {                            /* (On it; or fallen in, to climb out to where it came from) */
        wk.fx = wk.tx;
        wk.fy = wk.ty;
    }
}

/* The trap tq (tx, ty) disarmed, a minute's work a try, in two checks (the user's): its mechanism made out first
** (P_ASSESS: never deadly; a critical success makes the disarm easier), then the disarm (P_DISARM: a critical failure
** sets it off).  Each a long task (HOUSE.md's), begun (wk.plan: its test not made yet, nothing told), its test made
** and its outcome told at its minute's end; magic's means asked as it's begun (none: told at once) */
static void disarm (void)
{
    unsigned char o = TO_PIT;
    signed char g;

    if (wk.tq.kind > TK_SPIKED) {
        if (wk.tq.state & TS_MAGIC) {
            test (P_DISARM, HIDE_MAGICAL, 0);   /* (By magic only: the referee's means, or none) */
            o = wres ? TO_CANT : TO_NOT_YET;
            if (!wres && wk.plan) {
                timed (LT_DISARM, 600);
                return;
            }
        } else if (wk.plan) {
            timed (wk.tq.state & TS_ASSESSED ? LT_DISARM : LT_STUDY, 600);
            return;
        } else if (!(wk.tq.state & TS_ASSESSED)) {
            g = test (P_ASSESS, 0, -(signed char) (wk.tq.power >> 1));
            o = G_OK (g) ? TO_ASSESSED : TO_PUZZLED;
            if (o == TO_ASSESSED) {
                trap_state (wk.tq.state | TS_ASSESSED | (g == G_CS ? TS_KEEN : 0));
            }
        } else {
            g = test (P_DISARM, HIDE_MECHANICAL, -(signed char) (wk.tq.power >> 1) + (wk.tq.state & TS_KEEN ? 2 : 0));
            o = G_OK (g) ? TO_DISARMED : g == G_CF ? TO_FUMBLED : TO_NOT_YET;
        }
    }
    trap_told (o);
    if (o == TO_DISARMED) {
        trap_state (wk.tq.state | TS_DISARMED);
    } else if (o == TO_FUMBLED) {
        wk.tq.trigger = 3;                      /* (Its words: no trigger's) */
        spring (0);
    }
}

/* A trap the character knows of in its reach (the eight squares round it, its own): into tq, 1; or 0 */
static unsigned char near_trap (void)
{
    signed char i, j;
    unsigned x, y;

    for (j = -1; j <= 1; ++j) {
        for (i = -1; i <= 1; ++i) {
            x = wk.fx + i;
            y = wk.fy + j;
            if (sq (x, y) && (trap_on (x, y, 0) || trap_on (x, y, 1)) && !(wk.tq.state & TS_HIDDEN)) {
                return 1;
            }
        }
    }
    return 0;
}

/* A search about the character (a minute): the referee's test (P_SEARCH) against each hidden door and each
** hidden trap within a few squares that it sees (a trap harder the better it's hidden), the nearest first as far as
** its time went, each at the full chance (HOUSE.md: a search cut short covers the part done); a door found is a door
** like any other from then on, a trap known (told as it's found, and its reach, the UI's things' search the same); or
** (link 1) where the world's character stops as it searches as it goes, quietly and in no more time, all of them */
#define SEARCH_R        3
static const unsigned char sq_[] = { 9, 4, 1, 0, 1, 4, 9 };    /* (A square's distance off, squared, by its offset) */

static void search (void)
{
    unsigned i, j;
    unsigned char found = 0, k, r, a, b;
    unsigned char* p;

    hub_map (0);                                /* (Its reach, r: a square searched if its distance off, squared, is
                                                **   less; 19 all of them, the corners' 18; one for each 32 tenths of
                                                **   its minute done, the hub's long task's, about as many squares as
                                                **   that part of the 49, the nearest first) */
    i = (HUB->pc_task.done + 31) >> 5;
    r = wk.link || i > 19 ? 19 : i;
    for (b = 0; b <= 2 * SEARCH_R; ++b) {
        j = wk.fy - SEARCH_R + b;
        for (a = 0; a <= 2 * SEARCH_R; ++a) {
            i = wk.fx - SEARCH_R + a;
            if (sq_[a] + sq_[b] >= r || (p = sq (i, j)) == 0 || !(*p & T_VIS)) {
                continue;                       /* (Beyond its reach, or unseen) */
            }
            if ((trap_on (i, j, 0) || trap_on (i, j, 1)) && (wk.tq.state & TS_HIDDEN)) {
                if (test (P_SEARCH, wk.tq.state & TS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL,
                    -(signed char) wk.tq.hide) > 0 && !wres) {
                    trap_state (wk.tq.state & ~TS_HIDDEN);
                    trap_told (TO_FOUND);
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
            if (test (P_SEARCH, wk.pq.state & PS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL, 0) > 0 && !wres) {
                wk.px = i;
                wk.py = j;
                door_state (wk.pq.state & ~PS_HIDDEN);
                found |= 1;
            }
        }
    }
    if (wk.link) {                              /* (Searching as it goes, the world's: quietly, in no more time; res 1
                                                **   if it found something) */
        if (!(wk.res = found)) {
            return;
        }
    } else {
        wk.tt = 600;
    }
    told (E_PORTAL, found & 1 ? PO_FOUND : found ? PO_FOUND_TRAP : PO_NOTHING, r);
}

/* A trap stepped onto (W_STEP: at px, py); a search; a trap disarmed: the door's at px, py, or one in reach (else res
** 1: none there) */
void s_traps (void)
{
    unsigned char armed = 0;

    switch (wk.what) {
    case W_STEP:
        if (sq (wk.px, wk.py) && trap_on (wk.px, wk.py, 0)) {
            step ();
        }
        break;
    case PA_SEARCH:
        search ();
        break;
    case PA_CHECK:                              /* (The door ahead gone over for traps, alone: the search's test of its
                                                **   trap, at its hiding, 10 seconds, 2026-10-22) */
        wk.tt += 100;
        if (!sq (wk.px, wk.py) || !trap_on (wk.px, wk.py, 1)) {
            trap_told (TO_CLEAN);
        } else if (!(wk.tq.state & TS_HIDDEN)) {
            trap_told (TO_KNOWN);
        } else if (!(wk.tq.state & TS_MAGIC) && test (P_SEARCH, HIDE_MECHANICAL, -(signed char) wk.tq.hide) > 0 && !wres) {
            trap_state (wk.tq.state & ~TS_HIDDEN);
            trap_told (TO_FOUND);
        } else {
            trap_told (TO_CLEAN);               /* (Missed: told as if there were none) */
        }
        break;
    default:                                    /* (PA_DISARM) */
        if (wk.px != NO_SQ) {
            sq (wk.px, wk.py);
            if (trap_on (wk.px, wk.py, 1)) {
                armed = 1 | (wk.tq.state & TS_HIDDEN);
            }
            if (armed == 1) {
                disarm ();
            } else {
                trap_told (TO_NONE);
            }
        } else if (near_trap ()) {
            disarm ();
        } else {
            wk.res = 1;
        }
        break;
    }
}
