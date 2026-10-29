/*
** sense.c - (dq0p) the senses answering for an actor (DUNGEON.md: perception as layers; the hub's dsense, the
** denizens asking for their minds, as one decides): whether it senses the character.  Its eyes first, adapted to
** the light where it stands (the light layer's, and the character's own light if it's within its reach), as time's
** gone since they were last (eyes.c's adapt, at its kind's rate).  Seen: the character lit (its own light, or the
** light layer's where it stands) within the kind's sight, else within its darkvision, the less as its eyes are still
** used to more light than is there; a line clear between (no rock, wall nor shut door: eyes.c's clear_line, the
** chunks made for it as they were for the denizens' own); unlit, low (crouched, kneeling, sitting: three quarters as
** far; prone, half: HOUSE.md's body position).  Heard (slices 4.5, 4.6: the sound layer, sound.c's): the character
** busy, by how loud it is (the world's pc_noise: its steps by its pace, its armour, its body's position, its Stealth
** moving quietly), less the background where the actor is (sound.c's snd_mask), within its loudness times the kind's
** hearing over two squares (HOUSE.md's), the more downwind (the draught, a little), the way the sound goes (round
** corners, a shut door muffling it, a room's wall each square four: the sound layer's).  Asked of a noise of
** the character's (dsense's noise: the world's E_NOISE, a dig's, a battering's), heard or not, nothing else.  Its light
** seen far off: carried, in line, within the kind's eyes' reach of it (eyes.c's glow_reach).  Smelt (slice 4.1; the
** air, slice 4.5's): within a third of its nose's keenness in still air (squares: a hound's three), downwind the
** further, upwind the less or not at all (by the draught's strength: sound.c's snd_wind), the way the air goes (the
** sound layer's, no shut door on it; past it, a line the air passes).  A far one (beyond all of them) is answered
** coarsely, no line cast: how far it may go with the answer standing (slack).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "light.h"
#include "eyes.h"

#pragma static-locals (on)

#define SACT            offsetof (struct actor, did)    /* (dens.c's the same) */
#define sact            (*(struct actor*) sactb)
unsigned char sactb[SACT];                      /* (A body, copied out of the hub's header: dens.c's too, never both at
                                                **   once, the main RAM's room; its fields up to its denizen's, the
                                                **   senses' wanting none of those) */

void snd_need (void);                           /* (sound.c's) */
unsigned char __fastcall__ snd_cost (unsigned x, unsigned y);
unsigned char __fastcall__ s_snd_mask (unsigned x, unsigned y, unsigned char k);
signed char __fastcall__ snd_wind (int dx, int dy);

#pragma code-name (push, "SW6CODE")             /* (Part 6's, with the layer's readers: once a denizen's turn, the main
                                                **   RAM's room) */
#pragma rodata-name (push, "SW6RODATA")

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? dx : dy;
}

void s_sense_actor (void)
{
    static unsigned char k, sight, dark, hear, eyes, nose, pcs, own, lv, lit, res, m, ex, noise, c, pos;
    static unsigned pcx, pcy, dist, reach;
    static int dx, dy;
    static signed char w;
    static struct eyes* e;

    hub_map (0);
    k = HUB->dsense.who;
    sight = HUB->dsense.sight;
    dark = HUB->dsense.dark;
    hear = HUB->dsense.hear;
    eyes = HUB->dsense.eyes;
    nose = HUB->dsense.nose;                    /* (Its smell's reach in still air, squares) */
    noise = HUB->dsense.noise;                  /* (A noise of the character's: heard or not, alone) */
    memcpy (&sact, &HUB->actor[k], SACT);
    pcs = HUB->pc_stratum;
    pcx = HUB->pc_x;
    pcy = HUB->pc_y;
    own = HUB->pc_light;
    pos = HUB->pc_pos;
    res = 0;
    HUB->dsense.cost = 0xFF;
    if (sact.sn != pcs || k >= NACTORS) {
        HUB->dsense.res = 0;
        HUB->dsense.slack = 0xFF;               /* (Another stratum: till the character moves) */
        return;
    }
    if (!noise && HUB->pc_ready > hub_now ()) {
        noise = HUB->pc_going ? HUB->pc_noise : HUB->pc_task.kind == LT_SLEEP ? 1 : 2;   /* (Busy: its steps as loud
                                                **   as the world says, the rest as a walk; asleep, softly) */
    }
    snd_need ();                                /* (The sound layer, for the character where it is: sound.c's) */
    dx = sact.x - pcx;                          /* (From the character to it: the way a sound or a smell goes) */
    dy = sact.y - pcy;
    dist = apart (dx, dy);
    w = snd_wind (dx, dy);
    reach = 0;
    if (noise > (m = s_snd_mask (sact.x, sact.y, k + 1))) {
        reach = (noise - m) * hear * (32 + w) / 32; /* (Twice the squares: a loudness's times the hearing's over two,
                                                **   downwind a little further, upwind a little less) */
        if (2 * dist <= reach) {
            c = snd_cost (sact.x, sact.y);
            c = c == SND_NONE ? 0xFF : SND_COST (c);
            hub_map (0);
            HUB->dsense.cost = c;
            if (2 * c <= reach) {
                res = SN_HEARD;
            }
        }
    }
    hub_map (0);
    if (HUB->dsense.noise) {
        HUB->dsense.res = res;                  /* (A noise: heard or not, alone) */
        return;
    }
    lv = cached (sact.sn, sact.x, sact.y) ? light_at (sact.sn, sact.x, sact.y) : 0;   /* Its eyes, adapted to the
                                                **   light it stands in (none made for it: the maker's order the
                                                **   denizens' own) */
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
    if (pos && !own) {
        m = pos == POS_PRONE ? m >> 1 : m - (m >> 2);   /* (Low, no light of its own: the less seen) */
    }
    w = 2 + w < 0 ? 0 : 2 + w;                  /* (The air: its smell carried downwind, held back upwind) */
    nose = nose * w / 2;
    if (!res && dist <= m && clear_line (sact.sn, sact.x, sact.y, -dx, -dy, dist, 1)) {
        res = SN_SEEN;
    } else if (!res && dist <= nose) {
        c = snd_cost (sact.x, sact.y);          /* (Smelt along the way the air goes: no shut door on it; past the
                                                **   sound layer, a line the air passes) */
        if (c != SND_NONE ? !(c & SND_AIR) && SND_COST (c) <= nose :
            dist > SND_R && clear_line (sact.sn, sact.x, sact.y, -dx, -dy, dist, 1)) {
            res = SN_SMELT;
        }
    }
    hub_map (0);
    HUB->dsense.res = res;
    m = lit ? sight : dark;                     /* (Far beyond all of them: the coarse answer, no line cast) */
    reach >>= 1;
    if (reach > m) {
        m = reach > 250 ? 250 : reach;
    }
    if (nose > m) {
        m = nose;
    }
    HUB->dsense.slack = dist > m + 1 ? (dist - m - 1 > 0xFF ? 0xFF : dist - m - 1) : 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
