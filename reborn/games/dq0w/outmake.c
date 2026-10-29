/*
** outmake.c - (dq0w, swapped part 58: ../dq0/swap.inc, bank.s's stubs) the outfits made: the things part 57 drew (the
** list in the kinds' bank, outfit.h's) made in the item store, each where its outfit says: worn, in a hand (the main
** one, the other, both: a hand that's full, the pack), hung from the belt (its slots; in its carrier, the thing before
** it, a sword in its scabbard), in the pouch on the belt, in the pack (its room, the weight it bears: what won't go in
** its pouch in the pack; what won't go in the pack not made); a light lit, a waterskin full (its pints of water in it).
** Then each made its own (s_make_own's way: its number, its material by its make, its durability whole, a light's life,
** what's stoppered shut; no die of the world's spent: its make the draw's).  The character's, or a denizen's (its
** things filed at its lair's home, IW_DEN, its number theirs: I_KIT's, 2026-10-27), and its belongings (2026-10-27,
** the outfits' phases 2 and 3): at its bed, lying on a square of its lair's home room (its own, the list's first such
** thing's), hidden there (a stash: found by searching), on that room's floor (its lair's stores: a square of their
** own), its faction's hoard there (a container its robbers' takings go into: owner OW_HOARD | the faction), worn
** beneath its clothes (hidden: found by searching its body); a key made locking the last thing with a lid made before
** it, and fitting it alone (its number in the key's size and thick: HOUSE.md's keys that fit); a thing's material the
** list's if it says one (a gold ring); its pouch and belt found among its things if it has them already (its purse
** drawn as it's first met).  It calls no other part (its own data in its image: the main RAM's room).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "outfit.h"

#pragma code-name ("SW58CODE")
#pragma rodata-name ("SW58RODATA")
#pragma bss-name ("SW58RODATA")                 /* (Its own data: zeros each time it's copied in) */
#pragma local-strings (on)
#pragma static-locals (on)

static unsigned pack, belt, pouch;              /* The worn pack, the belt, the pouch on it */
static unsigned char hands, mainh;              /* The hands holding something (IH_*), the main one */
static unsigned char den;                       /* (A denizen's things: ost's did, filed at its lair's home, IW_DEN) */
static unsigned made[NOFIT];                    /* Each entry's thing made (0 none) */
static struct kind kk;                          /* A kind's record, copied out */
static struct ofit e;
static const unsigned char tough[NMATS] = MAT_TOUGH;
static unsigned box;                            /* The last thing with a lid made (a key locks it) */
static unsigned sqx[2], sqy[2];                 /* Its bed's square and its stores' (a denizen's: PL_BED's, PL_HOME's) */
static unsigned char sqset;                     /*   chosen (a bit each) */

/* The draw's die, its own (part 59's way: the same stream, on from where the draw left it): 0 ... n - 1 */
static unsigned char __fastcall__ die (unsigned char n)
{
    ost.rs = ost.rs * 25173U + 13849U;
    return n ? (ost.rs >> 8) % n : 0;
}

/* Spot k's square (0 its bed's, 1 its stores'), chosen the first time: a square of open floor in its lair's home room
** (ost's bx0 ... : the home square, none found in twelve tries) */
static void __fastcall__ spot (unsigned char k)
{
    unsigned char t, *q;
    unsigned x, y;

    if (sqset & (k + 1)) {                      /* (k 0 or 1: its bit) */
        return;
    }
    sqset |= k + 1;
    for (t = 0; t < 12; ++t) {
        x = ost.bx0 + die (ost.bw);
        y = ost.by0 + die (ost.bh);
        if ((q = at (ost.hsn, x, y)) != 0 && TILE (*q) == T_FLOOR && (x != ost.hx || y != ost.hy)) {
            break;
        }
    }
    if (t == 12) {
        x = ost.hx;
        y = ost.hy;
    }
    sqx[k] = x;                                 /* (Its squares stored once: the part's room, 2026-10-28) */
    sqy[k] = y;
}

/* Kind k's record copied out (kk) */
static void __fastcall__ kind_get (unsigned char k)
{
    hub_map (KIND_BANK);
    memcpy (&kk, CATALOGUE + (k < NKINDS ? k : 0), sizeof kk);
}

/* A thing's bulk and weight (kk its kind's record), q of a lot */
static unsigned char tb, tw;
static void __fastcall__ measure (unsigned char q)
{
    tb = kk.form & KF_TENS ? (kk.bulk * q + 9) / 10 : kk.bulk;
    tw = kk.form & KF_TENS ? LOT_WT (kk.form, kk.weight * q) : kk.form & KF_COUNT ? kk.weight * q : kk.weight;
}

/* The things in container c: how many, their bulk and weight (those directly in it) */
static unsigned char cnt;
static unsigned cbulk, cweight;
static void __fastcall__ contents (unsigned c)
{
    unsigned i = 0;

    cnt = 0;
    cbulk = cweight = 0;
    while ((i = items (i, IN_X | IN_ITEM, c)) != 0) {
        kind_get (ip->kind);
        measure (item_at (i)->qty);
        ++cnt;
        cbulk += tb;
        cweight += tw;
    }
}

/* Thing k, q of it, fits in container c (its room, its weight: a carrier its family's alone) */
static unsigned char __fastcall__ fits_in (unsigned c, unsigned char k, unsigned char q)
{
    unsigned char b, w, fam;

    kind_get (k);
    measure (q);
    b = tb;
    w = tw;
    fam = kk.slots;
    contents (c);
    kind_get (item_at (c)->kind);
    return (!kk.slots || kk.slots == fam) && cbulk + b <= kk.cap_bulk && cweight + w <= kk.cap_weight;
}

/* A thing made (kind k, q of it, its make g): where it's meant (pl; c the thing it goes in, PL_IN's), else in the pack,
** else not made: its number, or 0 */
static unsigned __fastcall__ make (unsigned char k, unsigned char q, unsigned char g, unsigned char pl, unsigned c)
{
    struct item* p;
    unsigned i, x = 0;
    unsigned char w = den ? IW_DEN : IW_ACTOR, f = den ? 0 : I_KNOWN, h = 0, s = 0;

    if (pl >= PL_BED && pl <= PL_HOARD) {     /* (A denizen's belongings where it sleeps: its square first, before
                                                **   the store's in the window) */
        if (!den) {
            pl = PL_PACK;
        } else {
            spot (s = pl >= PL_HOME);
            w = IW_SQUARE;
        }
    } else if (pl == PL_UNDER) {
        pl = PL_WORN;
        f |= den ? I_HIDDEN : 0;              /* (Beneath its clothes: found by searching its body) */
    }
    kind_get (k);
    if (w == IW_SQUARE) {
    } else if (pl == PL_HAND || pl == PL_OFF || pl == PL_BOTH) {
        h = pl == PL_BOTH || (kk.flags & K_TWO) ? IH_BOTH : pl == PL_HAND ? mainh : mainh ^ IH_BOTH;
        if (!(hands & h)) {
            f |= I_HELD;
        } else {
            h = 0;
            pl = PL_PACK;                       /* (Its hand full: the pack) */
        }
    } else if (pl == PL_WORN) {
        f |= I_WORN;
    } else if (pl == PL_BELT) {
        pl = PL_PACK;
        if (belt && (kk.flags & K_HANG)) {
            contents (belt);
            kind_get (item_at (belt)->kind);
            if (cnt < kk.slots) {
                w = IW_ITEM;
                x = belt;
                pl = PL_BELT;
            }
        }
    } else if (pl == PL_POUCH && pouch && fits_in (pouch, k, q)) {
        w = IW_ITEM;
        x = pouch;
    } else if (pl == PL_IN && c) {
        w = IW_ITEM;
        x = c;
    } else {
        pl = PL_PACK;
    }
    if (pl == PL_PACK) {
        if (!pack || !fits_in (pack, k, q)) {
            return 0;
        }
        w = IW_ITEM;
        x = pack;
    }
    for (i = 1; i < ilim && item_at (i)->kind; ++i) {
    }
    if (i >= ilim) {
        return 0;                               /* (The store full) */
    }
    if (i >= ihigh) {
        ihigh = i + 1;
    }
    p = item_at (i);
    memset (p, 0, sizeof *p);
    p->kind = k;
    p->where = w;
    p->x = x;
    p->qty = q;
    p->flags = f;
    p->sn = h;
    p->quality = g;
    if (w == IW_SQUARE) {                       /* (Where it sleeps, its lair's: its faction's, a hoard marked so; a
                                                **   stash hidden, a step the worse to find) */
        p->x = sqx[s];
        p->y = sqy[s];
        p->sn = ost.hsn;
        p->owner = pl == PL_HOARD ? OW_HOARD | ost.fac : ost.fac;
        if (pl == PL_STASH) {
            p->flags = I_HIDDEN;
            p->hide = 1;
        }
    }
    if (w == IW_DEN) {                          /* (A denizen's: filed at its lair's home, its number its owner and
                                                **   hide, its hand by its kind) */
        p->x = ost.hx;
        p->y = ost.hy;
        p->sn = ost.hsn;
        p->owner = ost.did;
        p->hide = ost.did >> 8;
    }
    hands |= h;
    if (k == KI_BACKPACK && pl == PL_WORN && !pack) {
        pack = i;
    } else if (k == KI_BELT && pl == PL_WORN && !belt) {
        belt = i;
    } else if (k == KI_POUCH && pl == PL_BELT && !pouch) {
        pouch = i;
    }
    return i;
}

/* Thing i made its own (s_make_own's way: its number, its material by its make, its durability whole, a light's life,
** what's stoppered shut) */
static void __fastcall__ own (unsigned i)
{
    const struct burn* b;
    struct item* p = item_at (i);
    unsigned life, id;
    unsigned char m, g = p->quality, h;

    kind_get (p->kind);
    m = e.mat ? e.mat : kk.mat;                 /* (The list's material, else its kind's: a gold ring) */
    if (m & 0x80) {
        item_at (i)->size = m << 4 | 15;        /* (Old coins: their mint, whole, part 59's; part 63's piles) */
        m = kk.mat;
    }
    if (m == MT_IRON && kk.craft == AP_WEAPON) {
        m = g >= 7 ? MT_STEEL : g <= 1 ? MT_BRONZE : MT_IRON;   /* (A blade's metal by its make) */
    }
    if (m == MT_TALLOW && g >= 6) {
        m = MT_WAX;
    }
    p = item_at (0);
    id = ++p->x;
    p = item_at (i);
    p->id = id;
    p->mat = m;
    p->dur = p->dur_max = tough[m] * (kk.weight + 4) * (5 + g) / 5;
    if (!p->dur) {
        p->dur = p->dur_max = 1;
    }
    p->dur -= p->dur_max / 10 * (e.flags >> 4);    /* (A denizen's condition, part 59's: Treasure 1's) */
    if ((kk.keeps & KF_CLOSES) == KF_STOPPER) {
        p->shut = IS_CLOSED;
    } else if (p->kind == KI_CHEST) {
        p->shut = IS_CLOSED | IS_LOCK;          /* (A chest closed, with a lock: locked if a key's made for it) */
    }
    h = kk.flags;
    if ((b = burn_of (item_at (i)->kind)) != 0) {
        life = b->life * 10;
        if ((h & K_LIGHT) && !b->fuels) {       /* (A candle: its thickness by its number, its wax) */
            h = (unsigned char) (id * 37 + (id >> 3));
            p = item_at (i);
            p->thick = 4 + 2 * (h & 3);
            life = life / 6 * p->thick * (m == MT_WAX ? 3 : 2) / 2 * (15 + g) / 19;
        }
        item_at (i)->fuel = life;
    }
}

/* The list's things made, the character's (who 0), or a denizen's (who 1: ost's did, its lair's home) */
void __fastcall__ s_outmake (unsigned char who)
{
    unsigned char k, n;
    unsigned i, id;
    struct item* p;

    hub_map (0);
    mainh = (den = who) != 0 || !(HUB->pcsum.limbs & LB_LEFT) ? IH_RIGHT : IH_LEFT;
    if (den) {                                  /* (Its belt and the pouch on it, if it has them already: its purse's) */
        for (i = 0; (i = items (i, IN_DEN, 0)) != 0; ) {
            if ((ip->owner | ip->hide << 8) == ost.did && ip->kind == KI_BELT && (ip->flags & I_WORN)) {
                belt = i;
            }
        }
        for (i = 0; belt && (i = items (i, IN_X | IN_ITEM, belt)) != 0; ) {
            if (ip->kind == KI_POUCH) {
                pouch = i;
            }
        }
    }
    if (!den && (HUB->pcsum.limbs & (mainh == IH_LEFT ? LB_ARM_R : LB_ARM_L))) {
        hands |= mainh ^ IH_BOTH;               /* (Its off arm useless: nothing held in it) */
    }
    hub_map (KIND_BANK);
    n = OF_N;
    for (k = 0; k < n; ++k) {
        hub_map (KIND_BANK);
        memcpy (&e, OF_LIST + k, sizeof e);
        made[k] = i = make (e.kind, e.qty, e.quality, e.place, e.in && e.in <= k ? made[e.in - 1] : 0);
        if (!i) {
            continue;
        }
        if (e.flags & TF_LIT) {
            item_at (i)->flags |= I_LIT;
        }
        own (i);
        kind_get (e.kind);
        if ((kk.keeps & KF_CLOSES) == KF_LID) {
            box = i;                            /* (A key made after it locks it) */
        } else if (e.kind == KI_KEY && box) {   /* (Its key: it fits that lock alone, its number in the key's size
                                                **   and thick) */
            id = item_at (box)->id;
            item_at (box)->shut = IS_CLOSED | IS_LOCK | IS_LOCKED;
            p = item_at (i);
            p->size = id;
            p->thick = id >> 8;
            box = 0;
            hub_map (KIND_BANK);
            n = OF_N;
        }
        if (e.flags & TF_FULL) {                /* (A waterskin full: its pints of water in it) */
            kind_get (e.kind);
            if ((i = make (KI_WATER, kk.cap_bulk, 4, PL_IN, i)) != 0) {
                own (i);
            }
        }
    }
    hub_map (0);
}
