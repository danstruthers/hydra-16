/*
** wblow.c - (dq0w) portals broken by blows (break: the user's, 2026-10-15): a door battered down, a portcullis's bars
** broken through, a window's shutters, its glass, its bars (DUNGEON.md: portals).  Each blow an attack the part
** ignores (the referee's RS_ATTACK at RS_THING: the user's, 2026-10-16, "model it after unarmed combat"), with what
** the character strikes with (the hub's pc_strike: the tool to hand that hits a thing hardest, a fist, a foot, its
** head), its impact less the material's resistance to it (the less, the worse the part's state) taken off its body
** (body.h's, by its material, size, thickness and state: fixtures.c's as it made it), the harm kept (PORTAL_HARM); a
** door broken down open for good (PS_WRECKED), a portcullis a way through for good, jammed.  A fist, a foot or a head
** that strikes it is hurt by a blow gone wrong (the referee's: more by a sturdy door than a rotten one, less with
** gauntlets, boots, a helm), the character stopping when it's hurt enough.  A door's bar, burst, falls where it was;
** broken glass is left in shards at the character's feet; a door's trap set off as it gives way.
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

/* Blows at the portal's part (its material mat, PM_* or 4 glass; its body): as many as it takes, or a dozen (four
** that make no mark at all: given up), a few seconds each; each an attack the part ignores, the material's resistance
** to each aspect its armour (the less, the worse the part's state); the harm past it kept (pq.harm's part, its mask
** hm, its shift hs: a window's shutters' and its bars' a nibble each, a door's the byte): 0 if it gives way, else
** how it stands (PO_GIVING, PO_HOLDING, PO_NO_MARK).  A part of the striker hurt by a blow gone wrong (the referee's
** say, its armour the referee's) is kept (hb, hw), and once it's hurt enough the blows stop */
static const unsigned char resist[5][4] = {     /* Against blows, edges, points, fire (items.h's DM_*) */
    { 4, 3, 4, 1 }, { 5, 5, 5, 3 }, { 6, 7, 7, 7 }, { 6, 8, 9, 9 }, { 0, 5, 3, 6 }
};

static unsigned char __fastcall__ batter (unsigned char mat, unsigned char body)
{
    struct req* r;
    unsigned char n, h = wk.pq.harm >> wk.hs & wk.hm, was = h, i, k, gone = 0;

    if (body > wk.hm) {
        body = wk.hm;
    }
    for (n = 0; n < 12 && !gone && !(wk.hw & E_STOPPED) && (n < 4 || h != was); ++n) {
        wk.tt += 30;
        hub_map (0);
        r = &HUB->slot[SLOT_WORLD];
        r->kind = RS_ATTACK;
        r->actor = 0;
        r->other = RS_THING;
        r->c = n;
        for (k = 0; k < 4; ++k) {
            i = resist[mat][k];
            r->x[k] = i - (i && wk.pq.qual <= 3 ? (wk.pq.qual <= 1 && i > 1 ? 2 : 1) : 0);
        }
        r = hub_ask (SLOT_WORLD);
        i = r->w1;
        wk.tt += r->w2;
        wk.hb = r->c > wk.hb ? r->c : wk.hb;
        wk.hw |= (r->w2 ? E_OUT : 0) | (r->result & 1 ? E_STOPPED : 0);
        r->state = REQ_FREE;
        if (!i) {
            continue;
        }
        if ((unsigned) h + i >= body) {
            gone = 1;
            h = 0;
        } else {
            h += i;
        }
    }
    wk.pq.harm = (wk.pq.harm & ~(wk.hm << wk.hs)) | h << wk.hs;
    if (h != was) {
        keep (wk.px, wk.py, wk.pq.harm, 0, PORTAL_HARM);
    }
    return gone ? 0 : h * 2 >= body ? PO_GIVING : h ? PO_HOLDING : PO_NO_MARK;
}

/* A door's blows (battered down: its body gone, it's open for good); a portcullis's bars, down, broken through (a gap
** for good); a window's shutters, closed, else its glass, else its bars */
void s_blows (void)
{
    unsigned char out, part, shut;

    front ();
    shut = (wk.pq.parts & PP_SHUTTERS) && !(wk.pq.state & PS_OPEN);
    wk.hm = 0xFF;
    wk.hs = 0;
    if (wk.pq.kind == PK_DOOR) {
        if (wk.pq.state & PS_OPEN) {
            out = PO_IS_OPEN;
        } else {
            noise (8);
            if ((out = batter (wk.pq.mat, wk.pq.tough)) == 0) {
                out = PO_BROKE_DOWN;
                door_state (PS_WRECKED | PS_BROKEN | PS_OPEN);
                part_moved (KI_BAR, AT_BAR, KI_BAR, AT_BAR);
            }
        }
    } else if (wk.pq.kind == PK_PORTCULLIS) {
        if (wk.pq.state & PS_WRECKED) {
            out = PO_JAMMED;                    /* (Its bars forced apart: it won't move, but it's a way through) */
        } else if (wk.pq.state & PS_OPEN) {
            out = PO_IS_UP;
        } else {
            noise (8);
            if ((out = batter (PM_IRON, wk.pq.tough)) == 0) {
                out = PO_GAP;
                door_state (PS_WRECKED | PS_OPEN);
            }
        }
    } else {                                    /* (A window's: its shutters, closed; else its glass; else its bars) */
        part = shut ? PP_SHUTTERS : wk.pq.parts & PP_GLASS ? PP_GLASS : wk.pq.parts & PP_BARS;
        if (!part) {
            out = PO_NOTHING_THERE;
        } else {
            noise (part == PP_GLASS ? 8 : 7);
            wk.hm = 15;
            wk.hs = part == PP_BARS ? 4 : 0;
            if ((out = batter (part == PP_GLASS ? 4 : part == PP_BARS ? PM_IRON : wk.pq.mat, part == PP_GLASS ? 1 :
                part == PP_BARS ? part_body (PM_IRON, wk.pq.size, TH_IN, wk.pq.qual) >> 1 : wk.pq.tough)) == 0) {
                wk.pq.parts &= ~part;
                door_state (wk.pq.state | PS_OPEN);
                if (part != PP_SHUTTERS) {      /* (Its glass in shards at the character's feet; its bars broken out) */
                    part_moved (part == PP_GLASS ? KI_PANE : KI_BARS, wk.px, wk.py, part == PP_GLASS ? KI_SHARDS :
                        KI_BARS, AT_PC);
                }
                out = part == PP_GLASS ? PO_SHATTERED : part == PP_BARS ? PO_BARS_BROKEN : PO_SMASHED;
            }
        }
    }
    tail (out);
}
