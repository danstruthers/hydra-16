/*
** wdoor.c - (dq0w) doors worked (DUNGEON.md: portals), and a window's shutters as a door is: opened (unbarred, if
** barred from this side), closed, a lock picked (the referee's P_PICKLOCK, by the tools to hand: a minute a try, harder
** the more complex the lock), forced (Strength against the door's material: its lock or bar burst; loud; a rotten
** door forced more easily, a new one less); a door's bar lifted out (only from its side; or as the door's unbarred to
** open it) and set down at the character's feet, or set in its brackets again (bar the door: shut, from its side, a
** bar to hand that spans it: the things' to put back, an IW_PORTAL part again).  A door's trap (one it doesn't know
** of) set off as it opens.  The world walked the character up to it (dq0w.c's door_ahead) and takes what came of it
** from wk.  Picking and forcing timed work (HOUSE.md's long tasks): begun (wk.plan: the means asked, the time and the
** long task's kind given, nothing told), then done at its time's end (dq0w.c's work_done: the test made then).
**
** All of it swapped part 5's (../dq0/swap.inc: dq0w.cfg's SW5), doors () called through its stub (bank.s's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "work.h"
#include "wbody.h"

#pragma code-name ("SW5CODE")
#pragma rodata-name ("SW5RODATA")
#pragma local-strings (on)

#define W_TRAPS
#define W_HARM
#define W_PARTS
#define W_FRONT
#define W_SPRING
#include "workpart.h"

extern unsigned char burn_ask;                  /* (things.h's) */
unsigned door_key (void);                       /* (Part 80's, wport.c: a key that fits it, the actor's) */

void s_doors (void)
{
    unsigned char out, what = wk.what, st, mine;
    signed char g;

    front ();
    hub_map (0);
    st = wk.who ? bd.step : HUB->pcsum.step ? HUB->pcsum.step : 10;  /* (An actor's, its own: wport.c's) */
    mine = wk.pq.bar == ((wk.pd + 2) & 3);      /* (The character on its bar's side) */
    if (wk.pq.parts & PP_SKY && wk.pq.kind == PK_WINDOW) {
        burn_ask |= 2;                          /* (A window on the surface's shutters: the sky looked at, as it's
                                                **   worked: part 76's, things.h's TS_ASK) */
    }
    if (what == PA_BEND) {
        what = PA_FORCE;                        /* (Bent: forced) */
    }
    if (what == PA_BAR && wk.pq.kind != PK_DOOR) {
        out = PO_NO_BRACKETS;
    } else if (wk.pq.kind != PK_DOOR && !(wk.pq.parts & PP_SHUTTERS)) {
        out = wk.pq.kind == PK_SLIT ? PO_SLIT : PO_NO_SHUTTERS;     /* (A window's shutters opened, shut ...) */
    } else {
        switch (what) {
        case PA_OPEN:
            wk.tt += 2 * st;
            if (wk.pq.state & PS_OPEN) {
                out = PO_IS_OPEN;
            } else if ((wk.pq.state & PS_BARRED) && !mine) {
                out = PO_BARRED;                /* (Barred from the far side) */
                goto found;
            } else if (wk.pq.state & PS_LOCKED) {
                out = PO_LOCKED;
found:
                if (!wk.who) {                  /* (Known from now on, the look's: "(closed, locked)") */
                    keep (wk.px, wk.py, wk.pq.state, wk.pq.parts | PP_FOUND, PORTAL_STATE);
                }
            } else if (wk.armed == 1) {
                out = PO_TRAPPED;               /* (A trap on it it knows of: left alone) */
            } else {
                out = wk.pq.state & PS_BARRED ? PO_UNBARRED : PO_OPENED;
                door_state ((wk.pq.state & ~PS_BARRED) | PS_OPEN);
                if (out == PO_UNBARRED) {       /* (Its bar lifted out, and set down by it) */
                    part_moved (KI_BAR, AT_BAR, KI_BAR, AT_PC);
                }
            }
            break;
        case PA_LIFT:                           /* (Its bar lifted out, the door left shut) */
            wk.tt += 2 * st;
            out = !(wk.pq.state & PS_BARRED) ? PO_NO_BAR : !mine ? PO_BAR_FAR : PO_LIFTED;
            if (out == PO_LIFTED) {
                door_state (wk.pq.state & ~PS_BARRED);
                part_moved (KI_BAR, AT_BAR, KI_BAR, AT_PC);
            }
            break;
        case PA_BAR:                            /* (A bar set in its brackets again: the door shut, from its side;
                                                **   link the widest bar to hand, its width: the things', which put it
                                                **   in them, IW_PORTAL, as the UI's told) */
            wk.tt += 2 * st;
            out = !(wk.pq.feat & PF_BRACKETS) || !mine ? PO_NO_BRACKETS : wk.pq.state & PS_BARRED ? PO_IS_BARRED :
                wk.pq.state & PS_OPEN ? PO_SHUT_FIRST : wk.pq.state & PS_BROKEN ? PO_BURST : !wk.link ?
                PO_NO_BAR_HELD : wk.link <= (wk.pq.size & 15) ? PO_BAR_SHORT : PO_REBARRED;
            if (out == PO_REBARRED) {
                door_state (wk.pq.state | PS_BARRED);
            }
            break;
        case PA_CLOSE:
            wk.tt += 2 * st;
            if (wk.pq.state & PS_WRECKED) {
                out = PO_WRECKED;
            } else if (!(wk.pq.state & PS_OPEN)) {
                out = PO_IS_SHUT;
            } else {
                out = PO_CLOSED;
                door_state (wk.pq.state & ~PS_OPEN);
            }
            break;
        case PA_UNLOCK:
            if (!(wk.pq.state & PS_LOCKED)) {
                out = PO_NO_LOCK;
                break;
            }
            if (door_key ()) {                  /* (Its key: no test, three seconds, every actor's; the character's by a
                                                **   tool's steps, a timed work: s_door_work's, 2026-11-04) */
                if (!wk.who) {
                    if (wk.plan) {
                        timed (LT_KEY, 30);
                        return;
                    }
                } else {
                    wk.tt += 30;
                }
                out = PO_KEYED;
                door_state (wk.pq.state & ~PS_LOCKED);
                break;
            }
            if (wk.who && !wk.link) {
                out = PO_NO_KEY;                /* (A denizen picks a lock only if its kind can (a robber's: its mind's,
                                                **   wport.c's wk.link), its tools its own: the house's, 2026-11-02) */
                break;
            }
            g = test (P_PICKLOCK, wk.pq.lock, -(signed char) wk.pq.lock);
            if (wres) {
                out = PO_NO_PICKS;
                break;
            }
            if (wk.plan) {                      /* (Begun, the means to hand (the referee's): a minute a try, the
                                                **   test made again at its end, its outcome then; the lockpicks
                                                **   got out first) */
                timed (LT_PICK, 600);           /* (Its reach, the lockpicks got out, the world's steps first:
                                                **   s_door_work's, reach_tool) */
                return;
            }
            if (g > 0) {
                out = PO_PICKED;
                door_state (wk.pq.state & ~PS_LOCKED);
            } else {
                out = PO_NOT_PICKED;
            }
            break;
        case PA_FORCE:
            if (wk.pq.state & PS_OPEN) {
                out = PO_IS_OPEN;
                break;
            }
            if (wk.plan) {                      /* (Begun: ten seconds' straining, loud, its test at their end) */
                noise (7);
                timed (LT_FORCE, 100);
                return;
            }
            g = test (P_FORCE, wk.pq.mat, (wk.pq.mat == PM_WOOD ? 0 : wk.pq.mat == PM_IRONBOUND ? -2 : -4) -
                ((wk.pq.state & (PS_LOCKED | PS_BARRED)) ? 1 : 0) + (wk.pq.qual <= 1 ? 2 : wk.pq.qual <= 3 ? 1 : 0) -
                (wk.pq.qual >= 8));             /* (A rotten door easier, a new one harder) */
            if (g > 0) {
                out = PO_FORCED;                /* (Its lock or bar burst, the door open) */
                door_state ((wk.pq.state & (PS_LOCKED | PS_BARRED) ? PS_BROKEN : 0) | PS_OPEN);
                part_moved (KI_BAR, AT_BAR, KI_BAR, AT_BAR);    /* (Its bar, burst, fallen where it was) */
            } else {
                out = PO_HELD;
            }
            break;
        case PA_LOCK:                           /* (Locked with its key, shut, every actor's: Phase 3C's) */
            out = !wk.pq.lock || (wk.pq.state & PS_BROKEN) ? PO_NO_LOCK : wk.pq.state & PS_LOCKED ? PO_IS_LOCKED :
                wk.pq.state & PS_OPEN ? PO_SHUT_FIRST : door_key () ? PO_KEY_LOCKED : PO_NO_KEY;
            if (wk.who || wk.plan) {
                if (out == PO_KEY_LOCKED && !wk.who) {
                    timed (LT_KEY, 30);         /* (As unlocking: a tool's steps) */
                    return;
                }
                wk.tt += 30;
            }
            if (out == PO_KEY_LOCKED) {
                door_state (wk.pq.state | PS_LOCKED);
            }
            break;
        default:
            out = what == PA_THROUGH ? PO_NOT_WINDOW : PO_NOT_PORTC;
            break;
        }
    }
    tail (out);
}
