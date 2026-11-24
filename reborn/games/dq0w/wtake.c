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
#include "acts.h"

#pragma local-strings (on)
#pragma code-name ("SW_take_rule_CODE")
#pragma rodata-name ("SW_take_rule_RODATA")

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
unsigned char __fastcall__ throw_at (const char* name);    /* (Part 88's: things.c, every actor's; 0 body bd's, tk_x the thing) */
extern unsigned tk_x, tk_y;
unsigned char __fastcall__ den_things (const struct msg* m);   /* (Part 114's: wthings.c, a denizen's things by the
                                                **   character's rules, the act table's rows) */
unsigned __fastcall__ den_room (unsigned n);    /* (Part 114's: room on it, its hands full: its purse, its pack) */
unsigned char __fastcall__ den_got (unsigned i, unsigned char tell);
unsigned __fastcall__ den_tt (unsigned i);      /* (Part 114's: a taking's time) */   /* (Part 114's: a thing taken, its time, carried, told) */
unsigned char __fastcall__ icomp (unsigned i);  /* (things.c's: thing i's compartment in what holds it, crow its row) */
extern struct comp crow;
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

/* The first thing body bd holds that's food; or (food 0) that's neither a weapon nor a light, nor its takings (a
** robber's, I_LOOT: carried home, 2026-11-23): its number, or 0 */
static unsigned __fastcall__ held (unsigned char food)
{
    unsigned i = 0;
    unsigned char k, y;

    while (bd.did && (i = items (i, IN_DEN, 0)) != 0) {
        if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_HELD) &&
            (food || !(ip->flags & I_LOOT))) {
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
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_LOOT)) | I_KNOWN;
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
    struct msg q2;
    struct item* p;

    if (I_ROB_ERRAND (*m) == FE_DROP) {
        while ((n = held (0)) != 0) {
            let_go (n);
        }
    } else if (I_ROB_ERRAND (*m) == FE_PASS) {  /* (Food handed to a fellow beside it: none held, nothing, no time; the
                                                **   one hand of a thing, every actor's, wthings.c's FE_HAND: the
                                                **   receiver's take rule, the give row, 2026-11-23); or (t PN_*) a
                                                **   spare weapon or light it needs, wthings.c's to choose */
        n = I_ROB_THING (*m) ? 0 : held (1);
        if (n || I_ROB_THING (*m)) {
            memcpy (&q2, m, sizeof q2);
            I_ROB_ERRAND (q2) = FE_HAND;
            I_ROB_Y (q2) = I_ROB_THING (*m);
            I_ROB_THING (q2) = n;
            I_ROB_X (q2) = I_ROB_FELLOW (*m);
            den_things (&q2);
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
            k = in_hands (bd.did) >= (h ? 2 : 1) && !den_room (n) ? IR_FULL : item_at (n)->where != IW_ITEM ||
                den_pry (n, PR_KNOWN | PR_QUIET | PR_TIMED) ? IR_OK : IR_NOT_HERE;
        }
    }
    hub_map (0);
    return k;
}

/* ---- Only what it's found (the user's, 2026-11-17) */
static unsigned char want, dark, cleared;       /* (The take's: its wants, its darkvision, got's) */
static unsigned armw;                           /* (The worth of the best armour it wears: TW_ARMOUR's, 2026-11-23) */

/* Whether body bd wears a thing of kind k, or (k 0) the worth of the best armour it wears (armw) */
static unsigned char __fastcall__ worn_k (unsigned char k)
{
    unsigned i = 0;
    unsigned char j;

    armw = k ? armw : 0;
    while ((i = items (i, IN_DEN, 0)) != 0) {
        if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_WORN)) {
            if ((j = ip->kind) == k) {
                hub_map (0);
                return 1;
            }
            hub_map (KIND_BANK);
            if (!k && CATALOGUE[j].craft == AP_ARMOUR && CATALOGUE[j].value > armw) {
                armw = CATALOGUE[j].value;
            }
        }
    }
    hub_map (0);
    return 0;
}

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

/* Thing i, in another, as body bd sees it lying: what it's in open all the way out (no lid down, no tie tied, no
** hidden compartment: it's found only what it's found, the user's, 2026-11-17), lying on a square at the last (not
** another's, not the character's); that record in ip (items ()'s: its square, for the look's tests), else 0.  A thing
** in a container within reach to hand at any depth (the user's H17, 2026-11-21, every actor's) */
static unsigned char __fastcall__ open_out (unsigned i)
{
    unsigned char c;

    for (;;) {
        ip = item_at (i);
        if (ip->where != IW_ITEM) {
            return ip->where == IW_SQUARE;
        }
        c = icomp (i);                          /* (Its compartment, crow its row) */
        if (c && (crow.noun & CC_HIDDEN)) {
            return 0;
        }
        i = item_at (i)->x;
        if (item_at (i)->shut & IS_SHUT_C (c)) {
            return 0;
        }
    }
}

/* Kind k as body bd wants it (want's): a weapon in hand, its having none (the catalogue's weapons held in a hand, not
** a lot counted); food; coins and jewels; armour better than it wears (TW_ARMOUR, the user's, 2026-11-23: a kind it
** doesn't wear, worth more than the best it does, armw): its craft (AP_*) plus 1, or 0 */
static unsigned char __fastcall__ wanted (unsigned char k)
{
    unsigned char c, f, r;

    hub_map (KIND_BANK);
    c = CATALOGUE[k].craft;
    f = CATALOGUE[k].flags;
    r = ((want & TW_ARMS) && c == AP_WEAPON && (f & K_HAND) && !(CATALOGUE[k].form & KF_COUNT)) ||
        ((want & TW_FOOD) && !strcmp (CATALOGUE[k].tool, "food")) ||
        ((want & TW_LOOT) && (k == KI_PENNIES || k == KI_CROWNS || k == KI_FARTHINGS || c == AP_JEWEL));
    if (!r && (want & TW_ARMOUR) && c == AP_ARMOUR && (f & K_WEAR) && CATALOGUE[k].value > armw) {
        r = !worn_k (k);
    }
    return r ? c + 1 : 0;
}

/* Thing i (its kind k, its craft c plus 1: wanted's) taken up by body bd by the take rule, held (its own: a weapon told
** the referee, its hand's), told (E_BEAT WB_PICKS: "The bandit snatches up its sword."): 1; or 0 (cleared 1: a weapon
** wanted, its hands full of other things, put down as the character's busy hands' are, and still not taken) */
static unsigned char __fastcall__ got (unsigned i, unsigned char k, unsigned char c)
{
    unsigned char f, w = c == AP_WEAPON + 1;
    unsigned s;

    if ((cleared = w && in_hands (bd.did) >= (bd.body == BD_TWO ? 2 : 1)) != 0 && (s = held (0)) != 0) {
        let_go (s);                             /* (A weapon wanted in hand, its hands full of other things) */
    }
    if ((f = s_take_rule (i)) != IR_OK || (w && in_hands (bd.did) >= (bd.body == BD_TWO ? 2 : 1))) {
        return 0;
    }
    if (w) {
        bd.ready += den_tt (i);                 /* (Two seconds: stooped, taken up, the take row's; out of a thing the
                                                **   takeout row's and its levels: wthings.c's, every taking's) */
        if (i != (bd.dropped & ~DR_OWN) || !bd.arm) {   /* (A weapon: in its hand as its own let fall is */
            bd.dropped = i | DR_OWN;            /*   taken up again, part 60's den_took; that one, */
            bd.arm = k;                         /*   as it is) */
        }
        den_took ();
    } else {
        den_got (i, 1);                         /* (Its time, the take's or the takeout's and its reach; carried by
                                                **   the character's rule, coins its purse, a hand, else its pack;
                                                **   told: wthings.c's, 2026-11-23) */
        den_saw = 1;
    }
    hub_map (0);
    ++HUB->den_seq;
    if (!w && bd.doing != DO_DIG) {
        HUB->dig_t[bk] = TW_GOT;                /* (Its mind's to see to it: worn, put away, 2026-11-23) */
    }
    return 1;
}

/* Body bd's own take (its mind's I_TAKE, Phase 3C's one path: a its wants, TW_*): the first thing lying within its
** reach it wants (wanted's), by the take rule, got (held, told); none, the nearest one lying within four squares told
** its mind (the hub's dig_t, TW_SEEN | its offsets: it goes to it, and asks again).  Only what it senses (the user's,
** 2026-11-17): nothing hidden, a thing's square sensed (sensed's: by light or its darkvision, I_TAKE's b, a line clear
** between; or underfoot).  Its own search first, if it's asked (TW_SEARCH: a robber where it last sensed the
** character): the character's search about it, wsearch.c's (a minute's, by its sheet, where it sees or feels the
** place: the hidden doors, traps, things and ones hiding in its reach; what it finds uncovered), then its take as ever */
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
    if (want & TW_ARMOUR) {
        worn_k (0);                             /* (The best it wears, armour's: what's better) */
    }
    if (want & TW_SEARCH) {                     /* (Its own search first, a robber's where it lost the character: the
                                                **   character's own, every actor's, wsearch.c's (the one-path plan's
                                                **   stage 2, 2026-11-21): what it finds uncovered, taken as anything
                                                **   lying is, below) */
        den_search (dark);
    }
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

    for (i = 0; (i = items (i, IN_SQUARE | IN_ITEM, 0)) != 0; ) {
        if ((ip->flags & I_HIDDEN) || !(c = wanted (k = ip->kind)) || !open_out (i)) {
            continue;                           /* (Hidden: its search's alone; not wanted; in a thing that's shut, or
                                                **   not lying about: a thing in an open one seen in it, at any depth,
                                                **   the one-path plan's stage 2, 2026-11-23, the user's H17; ip what
                                                **   it's in at the last) */
        }
        if (ip->sn != bd.sn || (unsigned) ((dx = ip->x - bd.x) + 4) > 8 || (unsigned) ((dy = ip->y - bd.y) + 4) > 8 ||
            home_near ()) {
            continue;                           /* (Out of its sight's four squares; at its own home, its lair's own
                                                **   or its takings, left there) */
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
    hub_map (0);
    if (bd.doing != DO_DIG) {
        HUB->dig_t[bk] = seen;                  /* (Nothing done, no time: what it saw, or none, told its mind; a
                                                **   tunneller's dig_t its spell's) */
    }
}
