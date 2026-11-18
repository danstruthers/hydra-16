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
**
** Only what it's found (the user's, 2026-11-17: one body rule, GAME-CRITIQUE.md's E4): a denizen's own take sees what
** it senses alone, by light or its darkvision, a line clear between, or underfoot, nothing hidden; what's hidden is
** found only by a search of its own (TW_SEARCH: a minute's, its reach, by its sheet, where it sees or feels the place);
** and its reach into a thing respects its compartments as the character's does (the take rule's: a closed one opened, a
** locked one unlocked by its own key or left, a hidden one not found left).
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
#include "line.h"

#pragma local-strings (on)
#pragma code-name ("SW81CODE")
#pragma rodata-name ("SW81RODATA")

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
unsigned char __fastcall__ throw_at (const char* name);    /* (Part 88's: things.c, every actor's; 0 body bd's, tk_x the thing) */
extern unsigned tk_x, tk_y;
unsigned char __fastcall__ ln_seen (unsigned char sn, unsigned x, unsigned y, unsigned tx, unsigned ty,
    unsigned char mode);                        /* (line.s's: the line's, this part's own) */

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
    struct item* p;

    if (I_ROB_ERRAND (*m) == FE_DROP) {
        while ((n = held (0)) != 0) {
            let_go (n);
        }
    } else if (I_ROB_ERRAND (*m) == FE_PASS) {  /* (Food handed to a fellow beside it: none held, nothing, no time) */
        if ((n = held (1)) != 0) {
            hub_map (0);
            memcpy (&q, &HUB->actor[I_ROB_FELLOW (*m) - 1], sizeof q);
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
        p = item_at (I_ROB_THING (*m));
        k = p->kind;
        n = p->where == IW_DEN && p->owner == (unsigned char) bd.did && p->hide == bd.did >> 8;
        hub_map (0);
        if (n) {
            tk_x = I_ROB_THING (*m);
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
** hands full (two things held), IR_FULL; in a thing, reached into by its compartments' rules (wfight.c's den_pry: a
** hidden one the character hasn't found none of its, a closed one opened, a locked one unlocked by its own key, else
** left: IR_NOT_HERE, one it can't get to; the user's, 2026-11-17); else IR_OK.  The square it lies on in tk_x, tk_y */
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
        (unsigned) (tk_y - bd.y + 1) > 2 || ((tk_x != bd.x || tk_y != bd.y) && TILE (*at (bd.sn, tk_x, tk_y)) == T_PIT)) {
        k = IR_NOT_HERE;                        /* (Down in a pit, from its edge: the character's own rule, H10) */
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
            k = in_hands (bd.did) >= (h ? 2 : 1) ? IR_FULL : item_at (n)->where != IW_ITEM ||
                den_pry (n, PR_KNOWN | PR_QUIET | PR_TIMED) ? IR_OK : IR_NOT_HERE;
        }
    }
    hub_map (0);
    return k;
}

/* ---- Only what it's found (the user's, 2026-11-17) */
static unsigned char want, dark, cleared;       /* (The take's: its wants, its darkvision, got's) */

/* The thing ip (items ()'s) at body bd's own home, its lair's own or its takings: 1 (left there) */
static unsigned char home_near (void)
{
    return ip->sn == bd.hsn && (unsigned) (ip->x - bd.hx + 1) <= 2 && (unsigned) (ip->y - bd.hy + 1) <= 2;
}

/* Square x, y, d squares off body bd (the more of across and down, its four at most), as it senses it: underfoot, felt
** where it stands; else lit (a room's light, the character's: unlit_at's) or within its darkvision, a line clear
** between (the eye's, line.h's: no rock, wall, shut door), as its eyes stand (its position's): 1 sensed */
static unsigned char __fastcall__ sensed (unsigned x, unsigned y, unsigned char d)
{
    if (!d) {
        return 1;
    }
    if (d > dark && unlit_at (bd.sn, x, y)) {
        return 0;
    }
    ln_eye = LN_EYES (bd.pos);
    return !ln_seen (bd.sn, bd.x, bd.y, x, y, LN_SIGHT);
}

/* Kind k as body bd wants it (want's): a weapon in hand, its having none (the catalogue's weapons held in a hand, not
** a lot counted); food; coins and jewels: its craft (AP_*) plus 1, or 0 */
static unsigned char __fastcall__ wanted (unsigned char k)
{
    unsigned char c, f;

    hub_map (KIND_BANK);
    c = CATALOGUE[k].craft;
    f = CATALOGUE[k].flags;
    return ((want & TW_ARMS) && c == AP_WEAPON && (f & K_HAND) && !(CATALOGUE[k].form & KF_COUNT)) ||
        ((want & TW_FOOD) && !strcmp (CATALOGUE[k].tool, "food")) ||
        ((want & TW_LOOT) && (k == KI_PENNIES || k == KI_CROWNS || k == KI_FARTHINGS || c == AP_JEWEL)) ? c + 1 : 0;
}

/* Thing i (its kind k, its craft c plus 1: wanted's) taken up by body bd by the take rule, held (its own: a weapon told
** the referee, its hand's), told (E_BEAT WB_PICKS: "The bandit snatches up its sword."): 1; or 0 (cleared 1: a weapon
** wanted, its hands full of other things, put down as the character's busy hands' are, and still not taken) */
static unsigned char __fastcall__ got (unsigned i, unsigned char k, unsigned char c)
{
    unsigned char f;
    unsigned s;

    if ((cleared = (f = s_take_rule (i)) == IR_FULL && c == AP_WEAPON + 1) != 0 && (s = held (0)) != 0) {
        let_go (s);
        f = s_take_rule (i);
    }
    if (f != IR_OK) {
        return 0;
    }
    bd.ready += 20;                             /* (Two seconds: stooped, taken up) */
    if (c == AP_WEAPON + 1) {
        if (i != (bd.dropped & ~DR_OWN) || !bd.arm) {   /* (A weapon: in its hand as its own let fall is */
            bd.dropped = i | DR_OWN;            /*   taken up again, part 60's den_took; that one, */
            bd.arm = k;                         /*   as it is) */
        }
        den_took ();
    } else {
        hold (i, &bd);
        news (WB_PICKS | (unsigned) k << 8);
        den_saw = 1;
    }
    hub_map (0);
    ++HUB->den_seq;
    return 1;
}

/* Body bd's own take (its mind's I_TAKE, Phase 3C's one path: a its wants, TW_*): the first thing lying within its
** reach it wants (wanted's), by the take rule, got (held, told); none, the nearest one lying within four squares told
** its mind (the hub's dig_t, TW_SEEN | its offsets: it goes to it, and asks again).  Only what it senses (the user's,
** 2026-11-17): nothing hidden, a thing's square sensed (sensed's: by light or its darkvision, I_TAKE's b, a line clear
** between; or underfoot).  Then its own search, if it's asked (TW_SEARCH: a robber where it last sensed the character),
** nothing in its reach taken: a minute's (the character's search about it), each hidden thing within its reach that it
** wants (not hidden by magic) tested by its sheet (P_SEARCH, as the character's: HIDE_MECHANICAL, den_test's), its
** hiding the steps worse, by touch where it doesn't see the place two more (the character's search's own steps),
** found, got; the rest left as they were, hidden */
void __fastcall__ s_den_take (const struct msg* m)
{
    struct item* p;
    unsigned i, seen = 0;
    unsigned char k, f, c, d, best = 5;
    int dx, dy;

    if (m->kind == I_ROB) {
        rest (m);                               /* (The things' rest: above) */
        return;
    }
    want = I_TAKE_WANT (*m);
    dark = I_TAKE_DARK (*m);
    if ((want & TW_FOOD) && (i = held (1)) != 0) {   /* (Hungry, with food in hand: eaten, five minutes as the character's
                                                **   meal; a lot one less; its mind told) */
        k = (p = item_at (i))->kind;
        if (p->qty > 1) {
            --p->qty;
        } else {
            p->kind = 0;
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
            home_near () || (ip->flags & I_HIDDEN)) {
            continue;                           /* (Out of its sight's four squares; at its own home, its lair's own
                                                **   or its takings, left there; hidden: its search's alone) */
        }
        if (!(c = wanted (k = ip->kind))) {
            continue;
        }
        d = dx < 0 ? -dx : dx;
        f = dy < 0 ? -dy : dy;
        d = d > f ? d : f;
        if (!sensed (bd.x + dx, bd.y + dy, d)) {
            continue;                           /* (Unseen: in the dark, beyond a wall, a shut door) */
        }
        if (d > 1) {
            if (d < best) {                     /* (Out of its reach: the nearest, told its mind) */
                best = d;
                seen = TW_SEEN | (dx + 8) | (dy + 8) << 4;
            }
            continue;
        }
        if (got (i, k, c)) {
            return;
        }
        if (cleared) {
            seen = 0;
        }
    }
    if (want & TW_SEARCH) {                     /* (Its own search: a minute's, its reach) */
        bd.ready += 600;
        for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {
            if (ip->sn != bd.sn || (ip->flags & (I_HIDDEN | I_MAGIC)) != I_HIDDEN ||
                (unsigned) ((dx = ip->x - bd.x) + 1) > 2 || (unsigned) ((dy = ip->y - bd.y) + 1) > 2 || home_near ()) {
                continue;                       /* (Out of its reach; its own home's, its takings' stash, left) */
            }
            f = ip->hide;
            if (!(c = wanted (k = ip->kind))) {
                continue;
            }
            if (den_test (P_SEARCH, -(signed char) f - (!dark && unlit_at (bd.sn, bd.x + dx, bd.y + dy) ? 2 : 0)) >=
                G_PASS && got (i, k, c)) {
                seen = 0;
            }
        }
    }
    hub_map (0);
    if (bd.doing != DO_DIG) {
        HUB->dig_t[bk] = seen;                  /* (Nothing done, no time: what it saw, or none, told its mind; a
                                                **   tunneller's dig_t its spell's) */
    }
}
