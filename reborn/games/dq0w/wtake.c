/*
** wtake.c - (dq0w) the take rule, every actor's (Phase 3C's one path, 2026-11-02: DUNGEON.md's one path for every
** actor), and a denizen's own take: its mind's I_TAKE (a its wants, TW_*), the first thing lying in its reach it wants
** taken up by the rule (wfetch.c's errands by it too, an ally's FE_TAKE), held, told; none, the nearest it sees told its
** mind (the hub's dig_t).  All of it swapped part 81's (../dq0/swap.inc: dq0w.cfg's SW81), called through its stubs
** (bank.s's: part 80's den_more, part 72's den_fetch).
**
** And the things' rest, every actor's by the character's rules (2026-11-04: HOUSE.md's denizens handling things): what it
** holds let fall (I_ROB's FE_DROP: the character's let fall, where it stands, no time, a shaft's head takes it), the food
** it holds handed to a fellow beside it (FE_PASS: the taker's own hands, as the take rule's; two seconds), a thing thrown
** at the character (FE_THROW: things.c's s_throw_at, the character's own: its reach, its bands, the referee's Throwing by
** its sheet; two seconds, as the character's), and what it holds eaten as it's hungry (I_TAKE's TW_FOOD: five minutes,
** as the character's meal; its mind told, the hub's dig_t, TW_ATE).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "outfit.h"
#include "wbody.h"

#pragma local-strings (on)
#pragma code-name ("SW81CODE")
#pragma rodata-name ("SW81RODATA")

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
unsigned char __fastcall__ throw_at (const char* name);    /* (Part 88's: things.c, every actor's; 0 body bd's, tk_x the thing) */
extern unsigned tk_x, tk_y;

#define FETCH_HANDS     200                     /* (Quarter pounds: the most a folk denizen lifts in its hands, 50 lb;
                                                **   one without hands, in its mouth, 3 lb: HOUSE.md's fetching) */
#define FETCH_MOUTH     12

/* Thing n actor a's own (a copy, or bd), held (filed at its lair's home, as its things are: items.h's IW_DEN) */
static void __fastcall__ hold (unsigned n, const struct actor* a)
{
    struct item* p = item_at (n);

    p->where = IW_DEN;
    p->sn = a->hsn;
    p->x = a->hx;
    p->y = a->hy;
    p->owner = a->did;
    p->hide = a->did >> 8;
    p->flags = (p->flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_HELD;
    hub_map (0);
}

/* What actor did holds, counted (its hands) */
static unsigned char __fastcall__ in_hands (unsigned did)
{
    unsigned c = 0;
    unsigned char k = 0;

    while (did && (c = items (c, IN_DEN, 0)) != 0) {
        if (ip->owner == (unsigned char) did && ip->hide == did >> 8 && (ip->flags & I_HELD)) {
            ++k;
        }
    }
    hub_map (0);
    return k;
}

/* Told as it's seen (its square in the character's sight: the limbs' news, its kind's) */
static void __fastcall__ news (unsigned w)
{
    unsigned char* p = at (bd.sn, bd.x, bd.y);

    if (p && (*p & T_VIS)) {
        limb_news (w, bk + 1);
    }
    hub_map (0);
}

/* The first thing body bd holds that's food; or (food 0) that's neither a weapon nor a light: its number, or 0 */
static unsigned __fastcall__ held (unsigned char food)
{
    unsigned i = 0;
    unsigned char k, y;

    while (bd.did && (i = items (i, IN_DEN, 0)) != 0) {
        if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_HELD)) {
            k = ip->kind;
            hub_map (KIND_BANK);
            y = !strcmp (CATALOGUE[k].tool, "food");
            if (food ? y : CATALOGUE[k].craft != AP_WEAPON && !CATALOGUE[k].light) {
                hub_map (0);
                return i;
            }
        }
    }
    hub_map (0);
    return 0;
}

/* Thing n let fall where body bd stands (the character's let fall, things.c's put_down_here: no time, a shaft's head takes
** it), told */
static void __fastcall__ let_go (unsigned n)
{
    struct item* p;
    unsigned char k;

    if (!it_need (bd.sn, (bd.x - STRATUM (bd.sn).x0) >> CW_BITS, (bd.y - STRATUM (bd.sn).y0) >> CW_BITS)) {
        hub_map (0);
        return;
    }
    p = item_at (n);
    k = p->kind;
    p->where = IW_SQUARE;
    p->sn = bd.sn;
    p->x = bd.x;
    p->y = bd.y;
    p->owner = p->hide = 0;
    p->flags = (p->flags & ~(I_HELD | I_WORN)) | I_KNOWN;
    sky_ask (n);
    hub_map (0);
    news (WB_DROP | (unsigned) k << 8);
    ++HUB->den_seq;
}

/* The things' rest: its mind's I_ROB, a FE_DROP, FE_PASS, FE_THROW */
static void __fastcall__ rest (const struct msg* m)
{
    unsigned n;
    unsigned char k;
    struct actor q;

    if (m->a == FE_DROP) {
        while ((n = held (0)) != 0) {
            let_go (n);
        }
    } else if (m->a == FE_PASS) {               /* (Food handed to a fellow beside it: none held, nothing, no time) */
        if ((n = held (1)) != 0) {
            hub_map (0);
            memcpy (&q, &HUB->actor[m->b - 1], sizeof q);
            if (q.state && q.sn == bd.sn && (unsigned) (q.x - bd.x + 1) <= 2 && (unsigned) (q.y - bd.y + 1) <= 2 &&
                in_hands (q.did) < (q.body == BD_TWO ? 2 : 1)) {
                k = item_at (n)->kind;
                hold (n, &q);
                bd.ready += 20;
                news (WB_HANDS | (unsigned) k << 8);
                ++HUB->den_seq;
            }
        }
    } else {                                    /* (FE_THROW: its own, in hand or not) */
        n = item_at (m->t)->where == IW_DEN && item_at (m->t)->owner == (unsigned char) bd.did && item_at (m->t)->hide == bd.did >> 8 ? m->t : 0;
        k = item_at (m->t)->kind;
        hub_map (0);
        if (n) {
            tk_x = n;
            bd.ready += 20;
            news ((throw_at (0) == TH_SHORT ? WB_FELL : WB_THREW) | (unsigned) k << 8);
            ++HUB->den_seq;
        }
    }
}

/* The take rule, every actor's (Phase 3C's one path, DUNGEON.md's: the character's own, things.c's get, the same
** tests, its pack and hands after): thing n taken up by body bd: lying on a square (or on what lies there) its own or
** beside it, IR_NOT_HERE else; a portal's part, IR_FIXED; fixed where it stands (a theme's altar), IR_NO; and a
** denizen's own: more than it lifts (in its hands, a folk's, 50 lb; in its mouth, 3 lb: HOUSE.md's), IR_HEAVY; its
** hands full (two things held), IR_FULL; else IR_OK.  The square it lies on in tk_x, tk_y */
unsigned tk_x, tk_y;
unsigned char __fastcall__ s_take_rule (unsigned n)
{
    struct item* p;
    unsigned c, w;
    unsigned char k, h;

    for (c = n; (p = item_at (c))->where == IW_ITEM; c = p->x) {
    }
    tk_x = p->x;
    tk_y = p->y;
    k = item_at (n)->kind;
    if (!k || p->where != IW_SQUARE || p->sn != bd.sn || (unsigned) (tk_x - bd.x + 1) > 2 ||
        (unsigned) (tk_y - bd.y + 1) > 2) {
        k = IR_NOT_HERE;
    } else if (item_at (n)->where == IW_PORTAL) {
        k = IR_FIXED;
    } else if (hub_map (KIND_BANK), CATALOGUE[k].keeps & KF_FIXED) {
        k = IR_NO;
    } else {
        w = weight_of (n);
        h = bd.body == BD_TWO;                  /* (Hands: folk's; else its mouth) */
        if (w > (h ? FETCH_HANDS : FETCH_MOUTH)) {
            k = IR_HEAVY;
        } else {
            k = in_hands (bd.did) >= (h ? 2 : 1) ? IR_FULL : IR_OK;
        }
    }
    hub_map (0);
    return k;
}

/* Body bd's own take (its mind's I_TAKE, Phase 3C's one path: a its wants, TW_*): the first thing lying within its
** reach it wants (a weapon in hand, its having none: the catalogue's weapons held in a hand, not a lot counted; food;
** coins and jewels), by the take rule, held (its own: a weapon told the referee, its hand's), told (E_BEAT WB_PICKS:
** "The bandit snatches up its sword."); none, the nearest one lying within four squares told its mind (the hub's
** dig_t, TW_SEEN | its offsets: it goes to it, and asks again) */
void __fastcall__ s_den_take (const struct msg* m)
{
    unsigned i, seen = 0;
    unsigned char want, k, f, c, d, best = 5;
    int dx, dy;

    if (m->kind == I_ROB) {
        rest (m);                               /* (The things' rest: above) */
        return;
    }
    want = m->a;
    if ((want & TW_FOOD) && (i = held (1)) != 0) {   /* (Hungry, with food in hand: eaten, five minutes as the character's
                                                **   meal; a lot one less; its mind told) */
        k = item_at (i)->kind;
        if (item_at (i)->qty > 1) {
            --item_at (i)->qty;
        } else {
            item_at (i)->kind = 0;
        }
        hub_map (0);
        bd.ready += 3000;
        news (WB_EATS | (unsigned) k << 8);
        HUB->dig_t[bk] = TW_ATE;
        ++HUB->den_seq;
        return;
    }

    for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {
        if (ip->sn != bd.sn || (unsigned) ((dx = ip->x - bd.x) + 4) > 8 || (unsigned) ((dy = ip->y - bd.y) + 4) > 8 ||
            (ip->sn == bd.hsn && (unsigned) (ip->x - bd.hx + 1) <= 2 && (unsigned) (ip->y - bd.hy + 1) <= 2)) {
            continue;                           /* (Out of its sight's four squares; at its own home, its lair's own
                                                **   or its takings, left there) */
        }
        k = ip->kind;
        hub_map (KIND_BANK);
        c = CATALOGUE[k].craft;
        f = CATALOGUE[k].flags;
        if (!((want & TW_ARMS) && c == AP_WEAPON && (f & K_HAND) && !(CATALOGUE[k].form & KF_COUNT)) &&
            !((want & TW_FOOD) && !strcmp (CATALOGUE[k].tool, "food")) &&
            !((want & TW_LOOT) && (k == KI_PENNIES || k == KI_CROWNS || k == KI_FARTHINGS || c == AP_JEWEL))) {
            continue;
        }
        d = dx < 0 ? -dx : dx;
        f = dy < 0 ? -dy : dy;
        d = d > f ? d : f;
        if (d > 1) {
            if (d < best) {                     /* (Out of its reach: the nearest, told its mind) */
                best = d;
                seen = TW_SEEN | (dx + 8) | (dy + 8) << 4;
            }
            continue;
        }
        if ((f = s_take_rule (i)) == IR_FULL && c == AP_WEAPON) {
            if ((seen = held (0)) != 0) {       /* (Wanting a weapon, its hands full of other things: put down, as the
                                                **   character's busy hands', then taken up) */
                let_go (seen);
                f = s_take_rule (i);
            }
            seen = 0;
        }
        if (f != IR_OK) {
            continue;
        }
        bd.ready += 20;                         /* (Two seconds: stooped, taken up) */
        if (c == AP_WEAPON) {
            if (i != (bd.dropped & ~DR_OWN) || !bd.arm) {   /* (A weapon: in its hand as its own let fall is */
                bd.dropped = i | DR_OWN;        /*   taken up again, part 60's den_took; that one, */
                bd.arm = k;                     /*   as it is) */
            }
            den_took ();
        } else {
            hold (i, &bd);
            news (WB_PICKS | (unsigned) k << 8);
            den_saw = 1;
        }
        hub_map (0);
        ++HUB->den_seq;
        return;
    }
    hub_map (0);
    if (bd.doing != DO_DIG) {
        HUB->dig_t[bk] = seen;                  /* (Nothing done, no time: what it saw, or none, told its mind; a
                                                **   tunneller's dig_t its spell's) */
    }
}
