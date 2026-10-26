/*
** wblow.c - (dq0w) portals broken by blows (break: the user's, 2026-10-15): a door battered down, a portcullis's bars
** broken through, a window's shutters, its glass, its bars (DUNGEON.md: portals).  Each blow an attack the part
** ignores (the referee's RS_ATTACK at RS_THING: the user's, 2026-10-16, "model it after unarmed combat"), with what
** the character strikes with (the hub's pc_strike: the thing named or in its hands, the UI's, 2026-10-22; a fist, a
** foot, its head), its impact less the material's resistance to it (the less, the worse the part's state) taken off its body
** (body.h's, by its material, size, thickness and state: fixtures.c's as it made it), the harm kept (PORTAL_HARM); a
** door broken down open for good (PS_WRECKED), a portcullis a way through for good, jammed.  A fist, a foot or a head
** that strikes it is hurt by a blow gone wrong (the referee's: more by a sturdy door than a rotten one, less with
** gauntlets, boots, a helm), the character stopping when it's hurt enough.  A door's bar, burst, falls where it was;
** broken glass is left in shards at the character's feet; a door's trap set off as it gives way.  A long task
** (HOUSE.md's: a battering, LT_BATTER, its bar the blows struck): begun by work (), then a blow at a time as the
** character's free (dq0w.c's batter_on), the harm kept as each lands, what came of them told as they end; stopped,
** hard or by the player (pc_cut), the harm done so far stays.  Told as it goes (the user's, 2026-10-22: its first
** words dq0w.c's, as it's begun): the first blow that marks it, the one that leaves it giving, every fourth else
** (E_BEAT); a bare part's first blow that would hurt it stopped short of it, asked (RS_SHORT, the referee's: "Keep at
** it?"), once a door.  A soft thing named that strikes (dq0w.c's door_work: the house's, rshouse.h's) one attempt, no
** harm, a blow's time, told (PO_SOFT), and no long task (the user's, 2026-10-22).
**
** All of it swapped part 6's (../dq0/swap.inc: dq0w.cfg's SW6), blows () called through its stub (bank.s's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"

#pragma code-name ("SW6CODE")
#pragma rodata-name ("SW6RODATA")
#pragma local-strings (on)

#define W_TRAPS
#define W_HARM
#define W_PARTS
#define W_FRONT
#define W_SPRING
#include "workpart.h"
#include "body.h"

/* The next blow at the portal's part (its material mat, PM_* or 4 glass; its body), if the battering goes on: a dozen
** at most (BLOWS: the hub's pc_task done, those struck), four that make no mark at all given up (its harm as it
** began, was), none once the striker's hurt enough or the long task's stopped; a few seconds each (tt); an attack
** the part ignores, the material's resistance to each aspect its armour (the less, the worse the part's state); the
** harm past it kept as it lands (pq.harm's part, its mask hm, its shift hs: a window's shutters' and its bars' a
** nibble each, a door's the byte), W_GONE if it gives way.  A part of the striker hurt by a blow gone wrong (the
** referee's say, its armour the referee's) is kept (hb, hw), the worst.  1 if one was struck */
static const unsigned char resist[5][4] = {     /* Against blows, edges, points, fire (items.h's DM_*) */
    { 4, 3, 4, 1 }, { 5, 5, 5, 3 }, { 6, 7, 7, 7 }, { 6, 8, 9, 9 }, { 0, 5, 3, 6 }
};

static unsigned char __fastcall__ batter (unsigned char mat, unsigned char body)
{
    struct req* r;
    unsigned char n, h = wk.pq.harm >> wk.hs & wk.hm, i, k;

    hub_map (0);
    n = (unsigned char) HUB->pc_task.done;
    if (n >= BLOWS || pc_cut || (wk.hw & E_STOPPED) || (n >= 4 && h == wk.was)) {
        return 0;
    }
    HUB->pc_task.done = n + 1;
    wk.tt = BLOW_T;
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_ATTACK;
    r->actor = 0;
    r->other = RS_THING;
    r->c = n;
    r->w2 = wk.qs == wk.sn + 1 && wk.qx == wk.px && wk.qy == wk.py ? 0 : RS_SHORT;  /* (A bare part's hurt stopped
                                                **   short of, but at the door it was warned at) */
    for (k = 0; k < 4; ++k) {
        i = resist[mat][k];
        r->x[k] = i - (i && wk.pq.qual <= 3 ? (wk.pq.qual <= 1 && i > 1 ? 2 : 1) : 0);
    }
    r = hub_ask (SLOT_WORLD);
    i = r->w1;
    wk.tt += r->w2;
    wk.hb = r->c > wk.hb ? r->c : wk.hb;
    wk.hw |= (r->w2 ? E_OUT : 0) | (r->result & 1 ? E_STOPPED : 0);
    if (r->result & 4) {
        wk.ask = 1;                             /* (Stopped short: asked, the door warned at kept) */
        wk.qs = wk.sn + 1;
        wk.qx = wk.px;
        wk.qy = wk.py;
    }
    r->state = REQ_FREE;
    if (r->w2) {
        bearings_off (BO_OUT);                  /* (Knocked senseless by its own blow's hurt: its bearings astray as
                                                **   it comes round, part 52's, slice 4.2's) */
    }
    if (i) {
        if ((unsigned) h + i >= body) {
            wk.what = W_GONE;
            i = 0;
        } else {
            i += h;
        }
        if (i != h) {
            wk.pq.harm = (wk.pq.harm & ~(wk.hm << wk.hs)) | i << wk.hs;
            keep (wk.px, wk.py, wk.pq.harm, 0, PORTAL_HARM);
        }
    }
    return 1;
}

/* A portal's blows: begun (work ()'s, PA_BREAK: the first as the character's free), the next of them struck (W_BLOW:
** its time tt; each heard, a noise for the denizens and a blow's sound, the user's), or what came of them told as
** they're done (wk.what PA_BREAK after): a door battered down (its body gone, it's open for good); a portcullis's
** bars, down, broken through (a gap for good); a window's shutters, closed, else its glass, else its bars; else how
** it stands (PO_GIVING, PO_HOLDING, PO_NO_MARK), unless it was stopped before a blow was struck */
void s_blows (void)
{
    unsigned char out = 0, part = 0, mat, body, h;

    if (wk.what < W_BLOW) {
        front ();
    }
    mat = wk.pq.mat;
    body = wk.pq.tough;
    wk.hm = 0xFF;
    wk.hs = 0;
    if (wk.pq.kind == PK_DOOR) {
        if (wk.pq.state & PS_OPEN) {
            out = PO_IS_OPEN;
        }
    } else if (wk.pq.kind == PK_PORTCULLIS) {
        mat = PM_IRON;
        if (wk.pq.state & PS_WRECKED) {
            out = PO_JAMMED;                    /* (Its bars forced apart: it won't move, but it's a way through) */
        } else if (wk.pq.state & PS_OPEN) {
            out = PO_IS_UP;
        }
    } else {                                    /* (A window's: its shutters, closed; else its glass; else its bars) */
        part = (wk.pq.parts & PP_SHUTTERS) && !(wk.pq.state & PS_OPEN) ? PP_SHUTTERS : wk.pq.parts & PP_GLASS ?
            PP_GLASS : wk.pq.parts & PP_BARS;
        wk.hm = 15;
        if (part == PP_GLASS) {
            mat = 4;
            body = 1;
        } else if (part == PP_BARS) {
            mat = PM_IRON;
            body = part_body (PM_IRON, wk.pq.size, TH_IN, wk.pq.qual) >> 1;
            wk.hs = 4;
        } else if (!part) {
            out = PO_NOTHING_THERE;
        }
    }
    if (body > wk.hm) {
        body = wk.hm;
    }
    h = wk.pq.harm >> wk.hs & wk.hm;
    if (wk.what == W_GONE) {                    /* (Given way at the last blow) */
        if (wk.pq.kind == PK_DOOR) {
            out = PO_BROKE_DOWN;
            door_state (PS_WRECKED | PS_BROKEN | PS_OPEN);
            part_moved (KI_BAR, AT_BAR, KI_BAR, AT_BAR);
        } else if (wk.pq.kind == PK_PORTCULLIS) {
            out = PO_GAP;
            door_state (PS_WRECKED | PS_OPEN);
        } else {
            wk.pq.parts &= ~part;
            door_state (wk.pq.state | PS_OPEN);
            if (part != PP_SHUTTERS) {          /* (Its glass in shards at the character's feet; its bars broken out) */
                part_moved (part == PP_GLASS ? KI_PANE : KI_BARS, wk.px, wk.py, part == PP_GLASS ? KI_SHARDS :
                    KI_BARS, AT_PC);
            }
            out = part == PP_GLASS ? PO_SHATTERED : part == PP_BARS ? PO_BARS_BROKEN : PO_SMASHED;
        }
    } else if (!out) {
        if (wk.what < W_BLOW && wk.link) {      /* (A soft thing named: one attempt, no harm, a blow's time, told; no
                                                **   long task, dq0w.c's door_work: the user's, 2026-10-22) */
            wk.tt = BLOW_T;
            wk.what = PA_BREAK;
            tail (PO_SOFT);
            return;
        }
        if (wk.what < W_BLOW) {                 /* (Begun: the blows as it's free, a long task, dq0w.c's) */
            wk.was = h;
            wk.what = W_BLOW;
            pc_cut = 0;
            return;
        }
        if (batter (mat, body)) {
            wk.m.b = NZ_BLOW;                   /* (Its noise a blow's: the sound task's) */
            noise (part && part != PP_GLASS ? 7 : 8);
            wk.m.b = 0;
            if (wk.what == W_BLOW) {            /* (Told as it goes: the first mark, giving, every fourth blow else) */
                part = wk.pq.harm >> wk.hs & wk.hm;
                out = h == wk.was && part != h ? WB_TELLS : h * 2 < body && part * 2 >= body ? WB_GIVES :
                    !(HUB->pc_task.done & 3) && HUB->pc_task.done < BLOWS && part != wk.was && !wk.ask ? WB_ON : 0xFF;
                if (out != 0xFF) {
                    told (E_BEAT, out, 0);
                }
            }
            return;
        }
        wk.what = PA_BREAK;
        if (!HUB->pc_task.done) {
            return;                             /* (Stopped before a blow was struck: nothing to tell) */
        }
        out = h * 2 >= body ? PO_GIVING : h ? PO_HOLDING : PO_NO_MARK;
    }
    wk.what = PA_BREAK;
    tail (out);
}
