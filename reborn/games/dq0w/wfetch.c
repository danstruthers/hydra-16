/*
** wfetch.c - (dq0w) an ally's errands and the character's gifts (slice 4.8, 2026-10-30: DENIZENS.md's fetching and
** influence; HOUSE.md's fetching and gifts), the world's part of them (the thing the UI names found, IO_FETCH, is
** things.c's part 10's): what a denizen's mind says its body does with things (I_ROB's a: FE_TAKE, the thing fetched taken up where it lies,
** if it's still there, within its reach, light enough and its hands free; FE_GIVE, handed to the character, into its
** hand or pack, else set down at its feet; FE_GIFT, the character's thing given it, taken, a count of a lot split
** off), each told the UI (E_TASK TE_FETCHED).  What an actor carries for the character is its own while it does (the
** item store's IW_DEN, filed at its lair's home as its things are).
**
** All of it swapped part 69's (../dq0/swap.inc: dq0w.cfg's SW69), called through its stubs (bank.s's).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

#pragma local-strings (on)
#pragma code-name ("SW69CODE")
#pragma rodata-name ("SW69RODATA")

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);   /* (Part 10's) */
void light_now (void);                          /* (Part 10's) */

#define FETCH_HANDS     200                     /* (Quarter pounds: the most a folk denizen fetches in its hands, 50 lb;
                                                **   one without hands, in its mouth, 3 lb: HOUSE.md's fetching) */
#define FETCH_MOUTH     12

#pragma bss-name (push, "SW69RODATA")
static struct item rec;                         /* (A lot's part split off, a gift's count: part 69's image's, no part
                                                **   called as it's used; the main RAM's room) */
#pragma bss-name (pop)

/* Told (E_TASK TE_FETCHED: b the actor plus 1, how times 16; the thing's kind w1's high byte) */
static void __fastcall__ told (unsigned char how, unsigned char kind)
{
    struct msg m;

    hub_map (0);
    m.kind = E_TASK;
    m.who = 0;
    m.b = bk + 1 | how << 4;
    m.w1 = TE_FETCHED | (unsigned) kind << 8;
    m.t = HUB->now;
    hub_event (&m);
    ++HUB->den_seq;
}

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

/* Body bd's errand (its intent, I_ROB with a: FE_TAKE, FE_GIVE, FE_GIFT; t the thing, w1 w2 where it lay) */
void __fastcall__ s_den_fetch (const struct msg* q)
{
    struct item* p;
    unsigned n = q->t, c, x, y, w;
    unsigned char kind, h, how = q->a, k;

    p = item_at (n);
    kind = p->kind;
    if (how == FE_TAKE) {                       /* (Taken up where it lies: still there, within its reach) */
        for (c = n; (p = item_at (c))->where == IW_ITEM; c = p->x) {
        }
        x = p->x;
        y = p->y;
        if (!kind || p->where != IW_SQUARE || p->sn != bd.sn || x != q->w1 || y != q->w2) {
            told (FT_GONE, kind);
            return;
        }
        if ((unsigned) (x - bd.x + 1) > 2 || (unsigned) (y - bd.y + 1) > 2) {
            hub_map (0);
            told (FT_FAR, kind);
            return;
        }
        w = weight_of (n);
        h = bd.body == BD_TWO;                  /* (Hands: folk's; else its mouth) */
        if (w > (h ? FETCH_HANDS : FETCH_MOUTH)) {
            told (FT_HEAVY, kind);
            return;
        }
        for (k = 0, c = 0; bd.did && (c = items (c, IN_DEN, 0)) != 0; ) {
            if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_HELD)) {
                ++k;                            /* (Its hands: what it holds already) */
            }
        }
        hub_map (0);
        if (k >= 2) {
            told (FT_HANDS, kind);
            return;
        }
        hold (n);
        told (FT_TOOK, kind);
        return;
    }
    if (how == FE_GIVE) {                       /* (Handed over, beside the character: its hand or its pack, else at
                                                **   its feet) */
        if (!kind || p->where != IW_DEN) {
            hub_map (0);
            return;
        }
        p->flags &= ~I_HELD;
        if ((unsigned) (bd.x - bpx + 1) > 2 || (unsigned) (bd.y - bpy + 1) > 2 || bd.sn != bps ||
            !carry (n, 1)) {
            p = item_at (n);
            p->where = IW_SQUARE;
            p->sn = bd.sn;
            p->x = bpx;
            p->y = bpy;
            p->owner = p->hide = 0;
            hub_map (0);
            told (FT_FEET, kind);
            return;
        }
        equip_now ();                           /* (What's to hand now, and its load: the referee's) */
        light_now ();
        told (FT_GAVE, kind);
        return;
    }
    if (how == FE_GIFT) {                       /* (The character's thing, given: a count of a lot split off) */
        if (!kind) {
            hub_map (0);
            return;
        }
        if (q->w1 && q->w1 < p->qty) {
            for (c = 1; c < ilim && item_at (c)->kind; ++c) {
            }
            if (c >= ilim) {
                hub_map (0);
                return;
            }
            memcpy (&rec, item_at (n), sizeof rec);
            item_at (n)->qty -= q->w1;
            rec.qty = q->w1;
            rec.flags &= ~I_NAMED;
            rec.id = 0;                         /* (Its own number as it's next looked at: make_own's; its coins'
                                                **   mints the lot's, in the same shares) */
            memcpy (item_at (c), &rec, sizeof rec);
            if (c >= ihigh) {
                ihigh = c + 1;
            }
            n = c;
        }
        hold (n);
        equip_now ();
        light_now ();
        told (FT_GOT, kind);
    }
}
