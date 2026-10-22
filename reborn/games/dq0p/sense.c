/*
** sense.c - (dq0p) the senses answering for an actor (DUNGEON.md: perception as layers; the hub's dsense, the
** denizens asking for their minds, as one decides): whether it senses the character.  Its eyes first, adapted to
** the light where it stands (the light layer's, and the character's own light if it's within its reach), as time's
** gone since they were last (eyes.c's adapt, at its kind's rate).  Seen: the character lit (its own light, or the
** light layer's where it stands) within the kind's sight, else within its darkvision, the less as its eyes are still
** used to more light than is there; a line clear between (no rock, wall nor shut door: eyes.c's clear_line, the
** chunks made for it as they were for the denizens' own).  Heard: within the kind's hearing as it works (busy: the
** sounds' layer later, slice 4.5).  Its light seen far off: carried, in line, within the kind's eyes' reach of it
** (eyes.c's glow_reach).  Smelt (slice 4.1): in still air, within a third of its nose's keenness (squares: a hound's
** three), along a line the air passes (clear_line's: no rock, wall nor shut door); the one place the air layer will
** carry it further downwind, less upwind (slice 4.5).  A far one (beyond all of them) is answered coarsely, no line
** cast: how far it may go with the answer standing (slack).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "light.h"
#include "eyes.h"

#pragma static-locals (on)

static struct actor a;

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? dx : dy;
}

void sense_actor (void)
{
    static unsigned char k, sight, dark, hear, eyes, nose, pcs, own, lv, lit, res, m, ex;
    static unsigned pcx, pcy, dist;
    static int dx, dy;
    static struct eyes* e;

    hub_map (0);
    k = HUB->dsense.who;
    sight = HUB->dsense.sight;
    dark = HUB->dsense.dark;
    hear = HUB->dsense.hear;
    eyes = HUB->dsense.eyes;
    nose = HUB->dsense.nose;                    /* (Its smell's reach in still air, squares) */
    memcpy (&a, &HUB->actor[k], sizeof a);
    pcs = HUB->pc_stratum;
    pcx = HUB->pc_x;
    pcy = HUB->pc_y;
    own = HUB->pc_light;
    res = 0;
    if (a.sn != pcs || k >= NACTORS) {
        HUB->dsense.res = 0;
        HUB->dsense.slack = 0xFF;               /* (Another stratum: till the character moves) */
        return;
    }
    if (HUB->pc_ready > hub_now ()) {
        res = SN_HEARD;                         /* (Busy: heard within its hearing) */
    }
    dx = pcx - a.x;
    dy = pcy - a.y;
    dist = apart (dx, dy);
    if (dist > hear) {
        res = 0;
    }
    lv = cached (a.sn, a.x, a.y) ? light_at (a.sn, a.x, a.y) : 0;   /* Its eyes, adapted to the light it stands in
                                                **   (none made for it: the maker's order the denizens' own) */
    if (own && dist <= own && 2 * (own - dist) + 1 > lv) {
        lv = 2 * (own - dist) + 1;
    }
    e = &E[k + 1];
    adapt (e, lv, eyes);
    lit = own || light_at (pcs, pcx, pcy);      /* The character lit: its own light, or where it stands */
    m = lit ? sight : dark;
    if (!lit && (ex = (e->a >> 4) - lv) < 16) {
        m = m * 4 / (4 + ex);                   /* (Darkvision the less, its eyes used to more light) */
    }
    if (!res && (dist <= m || dist <= nose) && clear_line (a.sn, a.x, a.y, dx, dy, dist, 1)) {
        res = dist <= m ? SN_SEEN : SN_SMELT;
    }
    hub_map (0);
    HUB->dsense.res = res;
    m = lit ? sight : dark;                     /* (Far beyond all of them: the coarse answer, no line cast) */
    if (hear > m) {
        m = hear;
    }
    if (nose > m) {
        m = nose;
    }
    HUB->dsense.slack = dist > m + 1 ? (dist - m - 1 > 0xFF ? 0xFF : dist - m - 1) : 0;
}
