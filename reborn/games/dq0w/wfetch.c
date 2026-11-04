/*
** wfetch.c - (dq0w) an ally's errands and the character's gifts (slice 4.8, 2026-10-30: DENIZENS.md's fetching and
** influence; HOUSE.md's fetching and gifts), the world's part of them (the thing the UI names found, IO_FETCH, is
** things.c's part 10's): what a denizen's mind says its body does with things (I_ROB's a: FE_TAKE, the thing fetched taken up where it lies,
** if it's still there, within its reach, light enough and its hands free; FE_GIVE, handed to the character, into its
** hand or pack, else set down at its feet; FE_GIFT, the character's thing given it, taken, a count of a lot split
** off), each told the UI (E_TASK TE_FETCHED).  What an actor carries for the character is its own while it does (the
** item store's IW_DEN, filed at its lair's home as its things are).
**
** All of it swapped part 72's (../dq0/swap.inc: dq0w.cfg's SW72), called through its stubs (bank.s's).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

#pragma local-strings (on)
#pragma code-name ("SW72CODE")
#pragma rodata-name ("SW72RODATA")

unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
unsigned char __fastcall__ take_rule (unsigned n);  /* (Part 81's, wtake.c: the take rule, tk_x, tk_y) */
extern unsigned tk_x, tk_y;
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);   /* (Part 10's) */
void light_now (void);                          /* (Part 10's) */
unsigned char __fastcall__ on_pc (unsigned i);  /* (Part 10's) */
void __fastcall__ mix_join (unsigned i, unsigned n);   /* (Part 64's: two piles of coins' mints) */

#define FETCH_HANDS     200                     /* (Quarter pounds: the most a folk denizen fetches in its hands, 50 lb;
                                                **   one without hands, in its mouth, 3 lb: HOUSE.md's fetching) */
#define FETCH_MOUTH     12

#pragma bss-name (push, "SW72RODATA")
static struct item rec;                         /* (A lot's part split off, a gift's count: part 72's image's, no part
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

/* A count k of lot n split off (a gift's, a trade's: its own record, the rest left where it was): the part's number,
** or 0 (the store full) */
static unsigned __fastcall__ lot (unsigned n, unsigned k)
{
    unsigned c;

    for (c = 1; c < ilim && item_at (c)->kind; ++c) {
    }
    if (c >= ilim) {
        return 0;
    }
    memcpy (&rec, item_at (n), sizeof rec);
    item_at (n)->qty -= k;
    rec.qty = k;
    rec.flags &= ~I_NAMED;
    rec.id = 0;                                 /* (Its own number as it's next looked at: make_own's; its coins' mints
                                                **   the lot's, in the same shares) */
    memcpy (item_at (c), &rec, sizeof rec);
    if (c >= ihigh) {
        ihigh = c + 1;
    }
    return c;
}

/* Body bd's errand (its intent, I_ROB with a: FE_TAKE, FE_GIVE, FE_GIFT; t the thing, w1 w2 where it lay; a gift's,
** a trade's, w1 a count of a lot split off, FE_GIFT's w2 1 kept, not held: the market's coins and goods, 2026-11-02) */
void __fastcall__ s_den_fetch (const struct msg* q)
{
    struct item* p;
    unsigned n = q->t, c;
    unsigned char kind, how = q->a;

    p = item_at (n);
    kind = p->kind;
    if (how == FE_TAKE) {                       /* (Taken up where it lies: still there, within its reach, by the take
                                                **   rule) */
        c = take_rule (n);
        if (c == IR_NOT_HERE && (!kind || tk_x != q->w1 || tk_y != q->w2 || item_at (n)->kind != kind)) {
            c = 0xFF;                           /* (Gone from where it lay) */
        }
        if (c == IR_OK) {
            hold (n);
        }
        hub_map (0);
        told (c == IR_OK ? FT_TOOK : c == 0xFF ? FT_GONE : c == IR_NOT_HERE ? FT_FAR : c == IR_FULL ? FT_HANDS :
            FT_HEAVY, kind);
        return;
    }
    if (how == FE_GIVE) {                       /* (Handed over, beside the character: its hand or its pack, else at
                                                **   its feet; its own, or in a thing it carries: a trade's, its coins
                                                **   in its pouch, a count of them split off) */
        for (c = n; item_at (c)->where == IW_ITEM; c = item_at (c)->x) {
        }
        p = item_at (c);
        if (!kind || p->where != IW_DEN || (p->owner | p->hide << 8) != bd.did ||
            (q->w1 && q->w1 < item_at (n)->qty && !(n = lot (n, q->w1)))) {
            hub_map (0);
            return;
        }
        p = item_at (n);
        p->flags &= ~I_HELD;
        if ((unsigned) (bd.x - bpx + 1) > 2 || (unsigned) (bd.y - bpy + 1) > 2 || bd.sn != bps ||
            !carry (n, q->w2 != 1)) {   /* (Kept: into its pack first, the market's) */
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
        if (q->w2 == 1 && kind == KI_PENNIES) {   /* (Coins kept: joined to a pile of the character's alike, the
                                                **   market's, 2026-11-02) */
            for (c = 0; (c = items (c, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
                if (c != n && item_at (c)->kind == kind && !(item_at (c)->flags & I_NAMED) && on_pc (c) &&
                    (unsigned) item_at (c)->qty + item_at (n)->qty <= 250) {
                    mix_join (c, n);
                    item_at (c)->qty += item_at (n)->qty;
                    item_at (n)->kind = 0;
                    break;
                }
            }
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
        if (q->w1 && q->w1 < p->qty && !(n = lot (n, q->w1))) {
            hub_map (0);
            return;
        }
        hold (n);
        if (q->w2 == 1) {
            item_at (n)->flags &= ~I_HELD;      /* (Kept, not held: the market's, 2026-11-02) */
        }
        equip_now ();
        light_now ();
        told (FT_GOT, kind);
    }
}
