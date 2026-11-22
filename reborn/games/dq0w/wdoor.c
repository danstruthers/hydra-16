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
#include "acts.h"

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
#define door_key()      tool_of (wk.who, KI_KEY, 0)     /* (A key that fits it, the actor's: wport.c's) */
unsigned char __fastcall__ braced (unsigned char sn, unsigned x, unsigned y);   /* (Part 109's, wdrag.c) */

/* A door's outcome's act (PO_*, AC_* in pairs): its noise, every actor's, by the act table's row (act_noise: the
** character's E_NOISE, an actor's E_DEN, at the door; wport.c's own table of an actor's before, the character's own
** quiet sounds here, its opening and shutting none: the one-path plan's stage 1, 2026-11-19).  A key, a bar and a pick
** quiet, heard close by (the user's, 2026-11-17) */
static const unsigned char po_ac[] = {
    PO_OPENED, AC_OPEN, PO_CLOSED, AC_SHUT, PO_UNBARRED, AC_UNBAR, PO_LIFTED, AC_LIFT, PO_REBARRED, AC_BAR, PO_KEYED,
    AC_UNLOCK, PO_KEY_LOCKED, AC_LOCK, PO_PICKED, AC_PICKED, PO_FORCED, AC_BURST, PO_HELD, AC_HELD, PO_SLAMMED, AC_SLAM, 0
};

void s_doors (void)
{
    unsigned char out, what = wk.what, st, mine, i;
    signed char g;

    front ();
    hub_map (0);
    st = wk.who ? bd.step : HUB->pcsum.step ? HUB->pcsum.step : 10;  /* (An actor's, its own: wport.c's) */
    mine = wk.pq.bar == ((wk.pd + 2) & 3);      /* (The character on its bar's side) */
    if (wk.pq.parts & PP_SKY && wk.pq.kind == PK_WINDOW && !wk.who) {
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
            } else if (wk.pq.kind == PK_DOOR && braced (wk.sn, wk.px, wk.py)) {
                out = PO_BLOCKED;               /* (A chest shoved against it, either side: wdrag.c's, the verbs plan's B4) */
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
        case PA_SLAM:                           /* (Slammed: shut, its noise louder, below) */
        case PA_CLOSE:
            wk.tt += 2 * st;
            if (wk.pq.state & PS_WRECKED) {
                out = PO_WRECKED;
            } else if (!(wk.pq.state & PS_OPEN)) {
                out = PO_IS_SHUT;
            } else {
                out = what == PA_SLAM ? PO_SLAMMED : PO_CLOSED;
                door_state (wk.pq.state & ~PS_OPEN);
            }
            break;
        case PA_PICK:                           /* (Picked, no key tried: "pick the lock", the user's, 2026-11-17) */
        case PA_UNLOCK:
            if (!(wk.pq.state & PS_LOCKED)) {
                out = PO_NO_LOCK;
                break;
            }
            if (what == PA_UNLOCK && door_key ()) {   /* (Its key: no test, three seconds, every actor's; the character's
                                                **   by a tool's steps, a timed work: s_door_work's, 2026-11-04) */
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
            if (wk.who && what == PA_UNLOCK) {
                out = PO_NO_KEY;                /* (A denizen's unlocking its key alone: its picking its mind's to ask,
                                                **   PA_PICK (a robber's kind: dq0d.c's door_way)) */
                break;
            }
            if (wk.who && !tool_of (wk.who, KI_LOCKPICKS, 0)) {
                out = PO_NO_PICKS;              /* (Its tools to hand, every actor's (the referee's Skills 23): the
                                                **   character's the referee's to know, its CANT below; an actor's its
                                                **   own things, wport.c's tool_of: the one-path plan's stage 2,
                                                **   closures, 2026-11-21; a robber picked with none before) */
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
                timed (LT_PICK, act_noise (wk.who, AC_PICKING, wk.px, wk.py));   /* (Its scratching at the lock,
                                                **   its time the row's; its reach, the lockpicks got out, the world's
                                                **   steps first: s_door_work's, reach_tool) */
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
                timed (LT_FORCE, act_noise (wk.who, AC_FORCING, wk.px, wk.py));
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
    for (i = 0; po_ac[i] && po_ac[i] != out; i += 2) {
    }
    if (po_ac[i]) {
        i = po_ac[i + 1];
        if (wk.pq.kind != PK_DOOR && i <= AC_SLAM) {
            i = i == AC_OPEN ? AC_WOPEN : AC_WSHUT;     /* (A window's shutters: a door's numbers, told as shutters;
                                                **   slammed, shut) */
        }
        act_noise (wk.who, i, wk.px, wk.py);
    }
    tail (out);
}
