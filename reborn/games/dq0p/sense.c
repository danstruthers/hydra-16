/*
** sense.c - (dq0p) the senses answering for an actor (DUNGEON.md: perception as layers; the hub's dsense, the
** denizens asking for their minds, as one decides): whether it senses the character, or another actor (the hub's
** dsense_whom: ONE-PATH.md's stage 1, one rule for every looker and every target, the user's 2026-11-17), its senses
** kept in the hub's sens for every task to read.  Its eyes first, adapted to
** the light where it stands (the light layer's, and the character's own light if it's within its reach), as time's
** gone since they were last (eyes.c's adapt, at its kind's rate).  Seen: the character lit (its own light, or the
** light layer's where it stands) within the kind's sight, else within its darkvision, the less as its eyes are still
** used to more light than is there; a line clear between (the eye's, line.inc's: the character's own look's to
** it, from the character's eyes, so it sees the character just when the character would see it, the light aside; no
** rock, wall, shut door nor closed shutters, no floor above it; bars and glass seen through; the chunks made for it
** as they were for the denizens' own); unlit, low (crouched, kneeling, sitting: three quarters as
** far; prone, half: HOUSE.md's body position).  Heard (slices 4.5, 4.6: the sound layer, sound.c's): the character
** busy, by how loud it is (the world's pc_noise: its steps by its pace, its armour, its body's position, its Stealth
** moving quietly), less the background where the actor is (sound.c's snd_mask), within its loudness times the kind's
** hearing over two squares (HOUSE.md's), the more downwind (the draught, a little), the way the sound goes (round
** corners, a shut door muffling it, a room's wall each square four: the sound layer's).  Asked of a noise of
** the character's (dsense's noise: the world's E_NOISE, a dig's, a battering's), heard or not, nothing else.  Its light
** seen far off: carried, in line, within the kind's eyes' reach of it (eyes.c's glow_reach).  Smelt (slice 4.1; the
** air, slice 4.5's): within a third of its nose's keenness in still air (squares: a hound's three), downwind the
** further, upwind the less or not at all (by the draught's strength: sound.c's snd_wind), the way the air goes (the
** sound layer's, no shut door on it; past it, a line the air passes: line.inc's).  A far one (beyond all of them) is
** answered coarsely, no line cast: how far it may go with the answer standing (slack).  The character hidden (the hub's
** pc_hid: HOUSE.md's hiding oneself): still, no sound of it; in sight's reach, SN_LOOK, not seen: its mind's to test the
** look (dq0d's part hiding); smelt all the same.  Another actor the target: its body and its light (the hub's sens),
** its steps heard while it's busy along the straight line, the air's (no rock nor shut door on it: the user's,
** 2026-11-17; exact, the sound layer's way, whenever the character is at either end), smelt the same.  Seen only
** before its eyes, as the character's look is (none behind it but beside it: REL_BEHIND), its face the way its mind's
** turned it (gw2, 2026-11-19: the audit's S1 and S14).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "light.h"
#include "eyes.h"
#include "../dq0w/line.h"

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

#pragma code-name (push, "SW_den_senses_CODE")  /* (With the layer's readers: once a denizen's turn, the main
                                                **   RAM's room) */
#pragma rodata-name (push, "SW_den_senses_RODATA")

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? dx : dy;
}

/* (dens.c's, a darkvision's look's alone: the main RAM's room) The body copied (sact) low (crouched, kneeling,
** sitting; prone) and unlit, seen only by the character's darkvision: past three quarters of its reach (prone,
** half), not seen, as every looker's rule has it (sense.c's: ONE-PATH.md's 2.2, the audit's S6) */
unsigned char s_low_far (void)
{
    static unsigned d, e;
    static unsigned char m;

    if (!sact.pos) {
        return 0;
    }
    d = sact.x - HUB->pc_x;
    e = sact.y - HUB->pc_y;
    d = (int) d < 0 ? -d : d;
    e = (int) e < 0 ? -e : e;
    d = d > e ? d : e;
    m = HUB->pcsum.darkvision;
    e = HUB->pc_light * (HUB->pcsum.nightvision ? HUB->pcsum.nightvision : 1);  /* (Its own light's reach, the look's) */
    if ((e && d <= e) || light_at (sact.sn, sact.x, sact.y)) {
        return 0;
    }
    hub_map (0);
    return d > (sact.pos == POS_PRONE ? m >> 1 : m - (m >> 2));
}

/* Square x, y of stratum sn lit, for every actor alike (the user's, 2026-11-17: a denizen sees by any light, the
** character's torch included; ONE-PATH.md's 2.2, the audit's S2): the light layer's there (a room's own light, a lit
** thing lying, daylight), or a carried light's within its reach with a line clear from it (the character's; an
** actor's, the world's alight, the one-path plan's lights, 2026-11-23); a chunk not made, dark.  The world's ask (the
** hub's look, LK_LIT: its unlit_at that was, what the character saw) */
unsigned char __fastcall__ s_lit_sq (unsigned char sn, unsigned x, unsigned y)
{
    static unsigned char own, k, ap;
    static unsigned d, ax, ay;
    static const struct actor* a;

    if (!cached (sn, x, y)) {
        return 0;
    }
    if (light_at (sn, x, y)) {
        return 1;
    }
    hub_map (0);
    own = HUB->pc_light * (HUB->pcsum.nightvision ? HUB->pcsum.nightvision : 1);
    if (own && HUB->pc_stratum == sn && (d = apart (x - HUB->pc_x, y - HUB->pc_y)) <= own) {
        ln_eye = LN_EYES (HUB->pc_pos);
        if (!line (sn, HUB->pc_x, HUB->pc_y, x, y, LN_SIGHT)) {
            return 1;
        }
    }
    for (k = 0; k < NACTORS; ++k) {             /* (An actor's carried light: the same rule) */
        hub_map (0);
        a = &HUB->actor[k];
        if ((own = HUB->alight[k] & AL_REACH) != 0 && (a->state & 1) && a->sn == sn && apart (x - a->x, y - a->y) <= own) {
            ax = a->x;
            ay = a->y;
            ap = a->pos;
            ln_eye = LN_EYES (ap);
            if (!line (sn, ax, ay, x, y, LN_SIGHT)) {
                return 1;
            }
        }
    }
    return 0;
}

/* The world's ask (the hub's look, LK_LIT: square wx, wy of stratum wd), answered (its ch) */
void s_lit_ask (void)
{
    static unsigned char c;

    hub_map (0);
    c = s_lit_sq (HUB->look.wd, HUB->look.wx, HUB->look.wy);
    hub_map (0);
    HUB->look.ch = c;
    hub_reply (&HUB->look);
}

#define PC_HID          0x80                    /* (pos's: the target hidden) */
static unsigned char eden[NACTORS];             /* (Each entry's denizen its eyes are, its record: E's) */
void s_sense_actor (void)
{
    static unsigned char k, j, sight, dark, hear, eyes, nose, tsn, own, lv, lit, res, m, ex, noise, c, pos, gr;
    static unsigned tx, ty, dist, reach;
    static int dx, dy;
    static signed char w;
    static struct eyes* e;
    static struct asense* r;
    static struct actor* t;

    hub_map (0);
    k = HUB->dsense.who;
    j = HUB->dsense_whom;                       /* (Its target: 0 the character, else an actor's entry + 1) */
    sight = HUB->dsense.sight;
    dark = HUB->dsense.dark;
    hear = HUB->dsense.hear;
    eyes = HUB->dsense.eyes;
    nose = HUB->dsense.nose;                    /* (Its smell's reach in still air, squares) */
    noise = HUB->dsense.noise;                  /* (A noise of the target's: heard or not, alone) */
    HUB->dsense.cost = 0xFF;
    if (k >= NACTORS || j > NACTORS || j == k + 1) {
        HUB->dsense.res = 0;
        HUB->dsense.slack = 0xFF;
        return;
    }
    memcpy (&sact, &HUB->actor[k], SACT);
    r = &HUB->sens[k + 1];                      /* (Its senses kept, as asked: the hub's, every task's to read) */
    r->sight = sight;
    r->dark = dark;
    r->hear = hear;
    r->eyes = eyes;
    r->nose = nose;
    r->light = HUB->alight[k] & AL_REACH;       /* (Its carried light: the world's alight) */
    r->flags = AS_SET;
    if (!j) {                                   /* (The target: the character) */
        tsn = HUB->pc_stratum;
        tx = HUB->pc_x;
        ty = HUB->pc_y;
        own = HUB->pc_light;
        pos = HUB->pc_pos | (HUB->pc_hid ? PC_HID : 0);
    } else {                                    /* (Or another actor: its body, its light the one it carries) */
        t = &HUB->actor[j - 1];
        tsn = (t->state & 1) ? t->sn : 0xFF;
        tx = t->x;
        ty = t->y;
        own = HUB->alight[j - 1] & AL_REACH;    /* (The world's: the one-path plan's lights, 2026-11-23) */
        pos = t->pos | (t->state > 1 ? PC_HID : 0);
    }
    res = 0;
    if (sact.sn != tsn) {
        HUB->dsense.res = 0;
        HUB->dsense.slack = 0xFF;               /* (Another stratum: till one moves) */
        return;
    }
    if (!noise) {
        if (!j) {
            if (HUB->pc_ready > hub_now ()) {
                c = (HUB->pc_going & 0x3F) - 1; /* (Busy: its steps as loud as the world says, or by its pace, a walk
                                                **   2, a jog 3, a run 4, a sprint 5, its armour a level more; the
                                                **   rest as a walk; asleep, softly: HOUSE.md's) */
                noise = !HUB->pc_going ? (HUB->pc_task.kind == LT_SLEEP ? 1 : pos & PC_HID ? 0 : 2) :
                    HUB->pc_noise != 0xFF ? HUB->pc_noise : (c > GA_WALK ? c : GA_WALK) + HUB->pc_clank;
            }
        } else if (t->ready > hub_now () && !(pos & PC_HID)) {
            noise = 1;                          /* (Another actor busy: its step, as the minds have it) */
        }
    }
    snd_need ();                                /* (The sound layer, for the character where it is: sound.c's) */
    dx = sact.x - tx;                           /* (From the target to it: the way a sound or a smell goes) */
    dy = sact.y - ty;
    dist = apart (dx, dy);
    w = snd_wind (dx, dy);
    reach = 0;
    if (noise > (m = s_snd_mask (sact.x, sact.y, k + 1))) {
        reach = (noise - m) * hear * (32 + w) / 32; /* (Twice the squares: a loudness's times the hearing's over two,
                                                **   downwind a little further, upwind a little less) */
        if (2 * dist <= reach) {
            if (!j) {
                c = snd_cost (sact.x, sact.y);  /* (The character at one end: the way a sound goes, exactly) */
                c = c == SND_NONE ? 0xFF : SND_COST (c);
            } else {                            /* (Between two denizens: along the straight line, the air's, no rock
                                                **   nor shut door on it: the user's, 2026-11-17) */
                ln_eye = LN_EYE;
                c = line (tsn, tx, ty, sact.x, sact.y, LN_AIR) ? 0xFF : (dist > 0xFE ? 0xFE : dist);
            }
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
    if (eden[k] != sact.den) {                  /* (Another denizen in the entry: its eyes its own, as a new one's, not
                                                **   its last one's: GAME-CRITIQUE.md's 3.5, 2026-11-17) */
        eden[k] = sact.den;
        e->t = 0;
    }
    adapt (e, lv, eyes);
    lit = own || (cached (tsn, tx, ty) && light_at (tsn, tx, ty));     /* The target lit: its own light, or where it
                                                **   stands */
    m = lit ? sight : dark;
    if (!lit && (ex = (e->a >> 4) - lv) < 16) {
        m = m * 4 / (4 + ex);                   /* (Darkvision the less, its eyes used to more light) */
    }
    if ((pos & ~PC_HID) && !own) {
        m = (pos & ~PC_HID) == POS_PRONE ? m >> 1 : m - (m >> 2);   /* (Low, no light of its own: the less seen) */
    }
    gr = own && !(pos & PC_HID) ? glow_reach (e, 2 * own + 1) : 0;   /* (Its light seen far off, eyes.c's) */
    w = 2 + w < 0 ? 0 : 2 + w;                  /* (The air: its smell carried downwind, held back upwind) */
    nose = nose * w / 2;
    ln_eye = LN_EYES (pos & ~PC_HID);           /* (The line between them: the target's eyes', as the character's look
                                                **   to it is) */
    if ((!res || (pos & PC_HID)) && (dist <= m || dist <= gr) && (dist <= 1 || !REL_BEHIND (REL8 (bearing (-dx, -dy), sact.face))) &&
        !line (tsn, tx, ty, sact.x, sact.y, LN_SIGHT)) {   /* (Before its eyes, as the character's look is: none behind
                                                **   it but beside it, its face its mind's to turn, gw2's, 2026-11-19) */
        res |= pos & PC_HID ? SN_LOOK : SN_SEEN;
    }
    if (!(res & ~SN_LOOK) && dist <= nose) {
        if (!j) {
            c = snd_cost (sact.x, sact.y);      /* (Smelt along the way the air goes: no shut door on it; past the
                                                **   sound layer, a line the air passes) */
            c = c != SND_NONE ? !(c & SND_AIR) && SND_COST (c) <= nose :
                dist > SND_R && !line (tsn, tx, ty, sact.x, sact.y, LN_AIR);
        } else {
            ln_eye = LN_EYE;
            c = !line (tsn, tx, ty, sact.x, sact.y, LN_AIR);
        }
        if (c) {
            res = SN_SMELT;
        }
    }
    hub_map (0);
    HUB->dsense.res = res;
    m = lit ? sight : dark;                     /* (Far beyond all of them: the coarse answer, no line cast) */
    if (gr > m) {
        m = gr;
    }
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
