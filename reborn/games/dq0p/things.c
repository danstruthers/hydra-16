/*
** things.c - (dq0p) the things the character perceives (DUNGEON.md: items; items.h): the item store's records read
** (the world's alone to write: games/dq0w/things.c), never written: the things the character sees lying about, named
** in what it perceives (sight.c's), and the lit things lying about (view.c's lamps).  The world makes the things a
** look saw their own after it (the senses' record's); a chunk the maker's stocked the world hasn't yet, the world's
** asked to put its things down before they're looked for (the hub's stock).  A kind's record copied out of the
** catalogue's bank (the world's, the hub's) as it's wanted.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "light.h"
#include "things.h"

#define NK      NKINDS                          /* (The catalogue's, and the theme's after them: theme.h) */

unsigned char piece[256];                       /* (A look's rows (view.c): a buffer in main RAM) */

/* A thing's record, copied out */
static struct item rec;

/* (item_at and items are items.s's, in assembly: its store's layout as here) */
typedef char store_fits[sizeof (struct item) == 25 && ITEM_BANK (0) == 14 && ITEM_BANKS == 2 ? 1 : -1];

/* A kind's record, copied out of the catalogue's bank (into one of two, in turn): the catalogue's bank left in the
** window */
static const struct kind* __fastcall__ kind_of (unsigned char k)
{
    static struct kind kb[2];
    static unsigned char next;
    struct kind* r = &kb[next++ & 1];

    hub_map (CAT_BANK);
    memcpy (r, CATALOGUE + (k < NK ? k : 0), sizeof *r);
    return r;
}

/* Item i's record copied out (into q) */
static void __fastcall__ fetch (struct item* q, unsigned i)
{
    struct item* p = item_at (i);

    if (!p->kind) {                             /* (A free record: only its kind, none, the rest unread; a look
                                                **   about goes through every record, and most are free) */
        q->kind = 0;
        return;
    }
    memcpy (q, p, sizeof *q);
}

/* Where a thing lies about: on a square, or on a surface that is (a table's): the record on the square (its own, or
** the surface's, copied), or 0 */
static const struct item* __fastcall__ lying (const struct item* q)
{
    static struct item s;

    if (q->where == IW_SQUARE || q->where == IW_PORTAL) {
        return q;                               /* (A portal's part as a thing lying there) */
    }
    if (q->where != IW_ITEM) {
        return 0;
    }
    fetch (&s, q->x);
    return s.where == IW_SQUARE && (kind_of (s.kind)->keeps & KF_ON) ? &s : 0;
}

/* The records lying on squares near the character (stratum near_sn, 20 squares each way of near_x, near_y), as the
** look's lamps found them (things_lamps's pass of the store), in order, for things_seen's (near_ok: the list whole,
** the store as it was: the world waits for the look, and writes the store meanwhile only as a chunk's stocked) */
#define NNEAR           24
static unsigned nearv[NNEAR], near_x, near_y;
static unsigned char nnear, near_ok, near_sn, near_r, near_w;  /* (near_r, near_w: things_seen's reach, a look one
                                                **   way's further, its whole width) */

/* The chunks the maker's stocked that the world hasn't put the things of down yet (their slots CS_FRESH): the world
** asked to, and waited for (it's waiting for the look meanwhile) */
void things_stocked (void)
{
    unsigned char k;
    struct cslot* c;

    hub_map (WORLD_BANK);
    for (k = NCSLOTS, c = WREC->cslot; k && c->used != CS_FRESH; --k, ++c) {
    }
    hub_map (0);
    if (k) {
        hub_call (&HUB->stock, T_WORLD);
        HUB->stock.state = REQ_FREE;
        near_ok = 0;                            /* (Things put down: the store gone through again) */
    }
}

/* ---- The things the character sees (sight.c's): on the squares it sees now, not hidden, named, where they lie; a
** surface's things after it, seen with it (hid 2: on the one before), and how far off each is made out (hid's high
** four bits: the kind's, squares past a speck, for the UI's words at a distance).  Each entry made in se (struct
** sight's item's layout: its way and square the square's), then copied in */
static struct { char name[14]; unsigned char qty, plural, rel, hid; unsigned n, x, y; } se;
typedef char se_fits[sizeof se == sizeof ((struct sight*) 0)->item[0] ? 1 : -1];

static void __fastcall__ seen_one (struct sight* s, unsigned i, unsigned char on)
{
    const struct kind* k;

    fetch (&rec, i);
    k = kind_of (rec.kind);
    strcpy (se.name, k->name);
    se.plural = (k->flags & K_PLURAL) != 0;
    se.qty = rec.qty;
    on = on ? 2 : (rec.flags & I_HIDDEN) != 0;
    se.hid = (rec.flags & I_LIT ? on | 4 : on) | k->seen << 4;
    se.n = i;
    memcpy (&s->item[s->nitems++], &se, sizeof se);
}

void __fastcall__ things_seen (struct sight* s, unsigned char sn, unsigned x, unsigned y, unsigned char face)
{
    static struct item q;
    static unsigned char k, listed;
    static struct item* r;
    static unsigned qx, qy;
    unsigned i = 0, j;
    unsigned char* p;

    s->nitems = 0;
    listed = near_ok && near_sn == sn && near_x == x && near_y == y;    /* (The look's list of those near, or the
                                                **   store gone through) */
    k = 0;
    while (s->nitems < SEE_ITEMS && (i = listed ? (k < nnear ? nearv[k++] : 0) : items (i, IN_SQUARE, 0)) != 0) {
        r = item_at (i);                        /* (Looked at in place, copied out only if it's seen) */
        if (r->sn != sn || (r->flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN || r->x - x + near_r > near_w ||
            r->y - y + near_r > near_w) {
            continue;
        }
        qx = r->x;
        qy = r->y;
        if (!(p = at (sn, qx, qy)) || !(*p & T_VIS)) {
            continue;
        }
        fetch (&q, i);
        se.rel = q.x == x && q.y == y ? 0xFF : REL8 (bearing ((int) (q.x - x), (int) (q.y - y)), face);
        se.x = q.x;
        se.y = q.y;
        seen_one (s, i, 0);
        if (kind_of (q.kind)->keeps & KF_ON) {
            for (j = 0; s->nitems < SEE_ITEMS && (j = items (j, IN_X | IN_ITEM, i)) != 0; ) {
                seen_one (s, j, 1);
            }
        }
    }
}

/* The lit things lying about (on squares within LAMP_FAR of (x, y), on surfaces there), the light layer's lamps
** (light.c's: their squares, their reach); and those within LAMP_R, view.c's (the chunks round them reached); and
** in the same pass of the store, the records lying on squares near it, for things_seen (nearv) */
#define LAMP_R          28
#pragma code-name (push, "SW1CODE")             /* (Part 1's: once a look, bank.s's stub) */
#pragma rodata-name (push, "SW1RODATA")
void give_way (void);                           /* (dq0p.c's: the look giving way to the UI's command in hand) */
void __fastcall__ s_things_lamps (unsigned char sn, unsigned x, unsigned y)
{
    const struct item* s;
    struct lamp* l = lamp;
    unsigned i = 0;
    unsigned char r;

    give_way ();
    nlamps = 0;
    light_lamps_begin ();
    nnear = 0;
    near_ok = !v_way;                           /* (A look one way's further: the store gone through, not the list of
                                                **   those near) */
    near_r = v_way ? WAY_R : 20;
    near_w = 2 * near_r;
    near_sn = sn;
    near_x = x;
    near_y = y;
    while ((i = items (i, IN_SQUARE | IN_ITEM | IN_PORTAL, 0)) != 0) {
        if (ip->where == IW_SQUARE && ip->sn == sn && ip->x - x + 20 <= 40 && ip->y - y + 20 <= 40) {
            if (nnear < NNEAR) {                /* (One near: things_seen's) */
                nearv[nnear++] = i;
            } else {
                near_ok = 0;
            }
        }
        if (!(ip->flags & I_LIT)) {
            continue;
        }
        fetch (&rec, i);
        if (!(s = lying (&rec)) || s->sn != sn || s->x - x + LAMP_FAR > 2 * LAMP_FAR ||
            s->y - y + LAMP_FAR > 2 * LAMP_FAR) {
            continue;
        }
        r = kind_of (rec.kind)->light;
        light_lamp (s->x, s->y, r);
        if (nlamps < NLAMPS && s->x - x + LAMP_R <= 2 * LAMP_R && s->y - y + LAMP_R <= 2 * LAMP_R) {
            l->x = s->x;
            l->y = s->y;
            l->r = r;
            ++l;
            ++nlamps;
        }
    }
    light_lamps_end (sn);
}
#pragma code-name (pop)
#pragma rodata-name (pop)
