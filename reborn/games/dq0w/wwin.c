/*
** wwin.c - (dq0w) portcullises and windows worked (DUNGEON.md: portals; their blows wblow.c's, a window's shutters
** wdoor.c's).  A portcullis is never opened: raised by its winch (on its bar's side: a few rounds' cranking, loud) or
** heaved up from the far side (P_LIFT, hard), and lowered by the winch only (dropped at once); down, its bars bent
** apart (P_FORCE, harder than a window's), and it's a way through for good, jammed (PS_WRECKED).  A window's bars bent
** (P_FORCE, hard, loud); its glass worked out whole, its bent bars lifted out (items, IW_PORTAL: set down at the
** character's feet); an open window climbed through (its sill a climb, P_CLIMB: a fall on a critical failure).  A
** slit's only seen and shot through.  What passes them is the senses' (dq0p's view).
**
** All of it swapped part 7's (../dq0/swap.inc: dq0w.cfg's SW7), portals () called through its stub (bank.s's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"

#pragma code-name ("SW7CODE")
#pragma rodata-name ("SW7RODATA")
#pragma local-strings (on)

#define W_HARM
#define W_PARTS
#define W_FRONT
#include "workpart.h"

#define SILL            3                       /* A window's sill, feet above the floor */

static unsigned char __fastcall__ portcullis (unsigned char what)
{
    unsigned char up = what == PA_RAISE, mine = wk.pq.bar == ((wk.pd + 2) & 3);

    if (wk.pq.state & PS_WRECKED) {
        return PO_JAMMED;                       /* (Its bars forced apart: it won't move, but it's a way through) */
    }
    if (what == PA_BEND) {                      /* (Its bars, down: bent apart, P_FORCE, the thicker the harder) */
        if (wk.pq.state & PS_OPEN) {
            return PO_IS_UP;
        }
        noise (7);
        wk.tt += 100;
        if (!G_OK (test (P_FORCE, PM_IRON, -6 - (signed char) wk.pq.thick))) {
            return PO_BARS_HOLD;
        }
        door_state (PS_WRECKED | PS_OPEN);
        return PO_BENT;
    }
    if (!up && what != PA_LOWER) {
        return what == PA_THROUGH ? PO_NOT_WINDOW : PO_NOT_PORTC;
    }
    if (up == (wk.pq.state & PS_OPEN)) {
        return up ? PO_IS_UP : PO_IS_DOWN;
    }
    if (mine) {                                 /* (Its winch: cranked up, or loosed) */
        wk.tt += up ? 300 : 20;
        noise (up ? 5 : 8);
    } else if (!up) {
        return PO_NO_WINCH;
    } else {
        wk.tt += 100;                           /* (By hand, from the far side: a round's heaving) */
        if (!G_OK (test (P_LIFT, 0, -6))) {
            return PO_TOO_HEAVY;
        }
    }
    door_state (wk.pq.state ^ PS_OPEN);
    return !up ? PO_LOWERED : mine ? PO_RAISED : PO_HEAVED;
}

/* A window's (its shutters' work the doors', its blows the blows': not here) */
static unsigned char __fastcall__ window (unsigned char what)
{
    unsigned char shut = (wk.pq.parts & PP_SHUTTERS) && !(wk.pq.state & PS_OPEN);
    signed char g;

    if (what == PA_FORCE) {                     /* (Its bars bent: the world sends only a barred window's here) */
        wk.tt += 100;
        noise (7);
        if (!G_OK (test (P_FORCE, PM_IRON, -6))) {
            return PO_BARS_HOLD;
        }
        wk.pq.parts &= ~PP_BARS;
        door_state (wk.pq.state);
        return PO_BENT;
    }
    if (what == PA_LIFT) {                      /* (Its glass worked out whole; its bars, once they're bent) */
        if (shut) {
            return PO_IN_WAY;
        }
        if (wk.pq.parts & PP_GLASS) {
            wk.tt += 300;
            wk.pq.parts &= ~PP_GLASS;
            door_state (wk.pq.state);
            part_moved (KI_PANE, wk.px, wk.py, KI_PANE, AT_PC);
            return PO_PANE_OUT;
        }
        return wk.pq.parts & PP_BARS ? PO_SET_FAST : part_moved (KI_BARS, wk.px, wk.py, KI_BARS, AT_PC) ?
            PO_BARS_OUT : PO_NO_BAR;
    }
    if (what != PA_THROUGH) {
        return PO_NOT_PORTC;
    }
    if (wk.pq.kind == PK_SLIT) {
        return PO_SLIT;
    }
    if (shut || (wk.pq.parts & (PP_GLASS | PP_BARS))) {
        return PO_IN_WAY;
    }
    wk.tt += 100;
    g = test (P_CLIMB, 0, 0);
    wk.through = G_OK (g);
    wk.res = wk.through << 1;                   /* (Somewhere new: the world tells it as arrived) */
    return wk.through ? PO_CLIMBED : g == G_CF ? PO_SLIPPED : PO_NO_CLIMB;
}

void s_portals (void)
{
    unsigned char out, what = wk.what;

    front ();
    out = wk.pq.kind == PK_PORTCULLIS ? portcullis (what == PA_OPEN || what == PA_FORCE ? PA_RAISE : what == PA_CLOSE ?
        PA_LOWER : what) : window (what);
    tail (out);
    if (wk.through) {                           /* (On the far side) */
        wk.fx = wk.px + dir_x[wk.pd];
        wk.fy = wk.py + dir_y[wk.pd];
    } else if (out == PO_SLIPPED) {
        harm (HC_FALL, strata[wk.sn].style == ST_BUILT ? GR_HARD : GR_ROCKY, HF_CLEAR, SILL);
    }
}
