/*
** wtake.c - (dq0w) the take rule, every actor's (Phase 3C's one path, 2026-11-02: DUNGEON.md's one path for every
** actor), and a denizen's own take: its mind's I_TAKE (a its wants, TW_*), the first thing lying in its reach it wants
** taken up by the rule (wfetch.c's errands by it too, an ally's FE_TAKE), held, told; none, the nearest it sees told its
** mind (the hub's dig_t).  All of it swapped part 81's (../dq0/swap.inc: dq0w.cfg's SW81), called through its stubs
** (bank.s's: part 80's den_more, part 72's den_fetch).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "outfit.h"
#include "wbody.h"

#pragma local-strings (on)
#pragma code-name ("SW81CODE")
#pragma rodata-name ("SW81RODATA")

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */

#define FETCH_HANDS     200                     /* (Quarter pounds: the most a folk denizen lifts in its hands, 50 lb;
                                                **   one without hands, in its mouth, 3 lb: HOUSE.md's fetching) */
#define FETCH_MOUTH     12

/* Thing n body bd's own, held (filed at its lair's home, as its things are: items.h's IW_DEN) */
static void __fastcall__ hold (unsigned n)
{
    struct item* p = item_at (n);

    p->where = IW_DEN;
    p->sn = bd.hsn;
    p->x = bd.hx;
    p->y = bd.hy;
    p->owner = bd.did;
    p->hide = bd.did >> 8;
    p->flags = (p->flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_HELD;
    hub_map (0);
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
            for (k = 0, c = 0; bd.did && (c = items (c, IN_DEN, 0)) != 0; ) {
                if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_HELD)) {
                    ++k;                        /* (Its hands: what it holds already) */
                }
            }
            k = k >= (h ? 2 : 1) ? IR_FULL : IR_OK;
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
static void fit (void);
void __fastcall__ s_den_take (const struct msg* m)
{
    unsigned i, seen = 0;
    unsigned char want, k, f, c, d, best = 5;
    int dx, dy;

    if (!m) {
        fit ();                                 /* (The character's outfit's, s_outfit's: below) */
        return;
    }
    want = m->a;

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
        if (s_take_rule (i) != IR_OK) {
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
            hold (i);
            limb_news (WB_PICKS | (unsigned) k << 8, bk + 1);
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


/* An outfit fits the character's strength (HOUSE.md's an outfit that fits; ost.kept what it bears, quarter pounds): the list's
** things left out, the heaviest first (a lot's one at a time: two spare torches), till it weighs that at most or nothing's left
** to leave: never what it wears for clothes and shoes, its pack, its belt and what hangs from it (a waterskin, full), a
** container with something in it, its light lit, its fire-maker, its food.  Not refunded: left at home */
#pragma bss-name (push, "SW81RODATA")           /* (Its own data, wanted only while it runs: not the main RAM's) */
static void fit (void)
{
    static const unsigned char keep[] = { KI_CLOTHES, KI_SANDALS, KI_SHOES, KI_TINDERBOX, KI_RATIONS };
    register struct ofit* e;
    register const struct kind* k;
    static unsigned t, w, bd;
    static unsigned char n, j, i, best;

    hub_map (KIND_BANK);
    n = OF_N;
    for (;;) {
        best = 0xFF;
        bd = 0;
        k = CATALOGUE + KI_PENNIES;             /* (Its money, to come, s_kit's: never left out) */
        t = LOT_WT (k->form, k->weight * (ost.budget >> 2));
        for (j = 0, e = OF_LIST; j < n; ++j, ++e) {
            k = CATALOGUE + e->kind;
            w = k->weight;
            i = e->qty;
            if (k->form & KF_TENS) {
                w = LOT_WT (k->form, w * i);
                i = 1;
            } else if ((k->flags & K_PLURAL) && !(k->form & KF_COUNT)) {
                i = 1;
            }
            t += w * i;
            if (k->craft == AP_WEAPON) {
                t += w >> 2;                    /* (At most: a strong character's heavy version, s_kit's) */
            }
            if (e->flags & TF_FULL) {
                t += 20;                        /* (A waterskin's four pints, five pounds) */
            }
            if (w <= bd || (e->flags & TF_LIT) || ((k->flags & (K_CONTAINER | K_BELT)) && (e->place == PL_WORN ||
                e->place == PL_BELT))) {
                continue;
            }
            for (i = 0; i < sizeof keep && keep[i] != e->kind; ++i) {
            }
            if (i < sizeof keep) {
                continue;
            }
            for (i = 0; i < n && OF_LIST[i].in != j + 1; ++i) {
            }
            if (i == n) {
                bd = w;
                best = j;
            }
        }
        if (t <= ost.kept || best == 0xFF) {
            break;
        }
        e = OF_LIST + best;
        k = CATALOGUE + e->kind;
        if (e->qty > 1 && !(k->form & KF_TENS) && (!(k->flags & K_PLURAL) || (k->form & KF_COUNT))) {
            --e->qty;
            continue;
        }
        --n;
        memcpy (e, e + 1, (n - best) * sizeof *e);   /* (Upward, a byte at a time: the list's own) */
        for (j = 0, e = OF_LIST; j < n; ++j, ++e) {
            if (e->in > best + 1) {
                --e->in;
            }
        }
    }
    OF_N = n;
    hub_map (0);
}
#pragma bss-name (pop)
