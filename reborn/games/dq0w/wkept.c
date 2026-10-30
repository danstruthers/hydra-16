/*
** wkept.c - (dq0w) the rooms' lights kept by their sconces (DUNGEON.md: light kept; slice 4.1): a room with a sconce is
** lit (R_LIT, the light layer's own light of a room) as a light in its sconce is lit, by whoever lit it, and dark as
** it's put out, taken away or burnt away; a change record of it (ROOM_LIT, on the sconce's square: store.c's), so a
** chunk made again has it, and told (E_LIGHT: the senses' light layer).  A keepers' torch in one (I_TENDED: never
** burning down) is tended only while its keepers live (the denizens' places kept, the hub's KEPT_AT): theirs all
** dead, it burns down from then (the lights' burning).  Seen to after a thing's handled within reach (things.c's
** serve), each game minute for the chunks round the character (dq0w.c's burning, burns_now: a torch burnt out, its
** keepers gone), and as a chunk's first stocked (store.c's stocked: the maker's kept rooms lit, R_LIT, a record
** each).  Each sconce's hide keeps its room's light as the world last made it (SC_LIT), so a pass changes only what's
** changed.  And a stratum's populations waited for as its plan's grown, so its chunks are stocked by them from the
** first.
**
** All of it swapped part 34's (../dq0/swap.inc: dq0w.cfg's SW34, its image in overlay 11's bank, the blows' filling
** overlay 2's), called through its stubs (bank.s's), its locals the C stack's (not the main RAM's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "wbody.h"

#pragma code-name ("SW34CODE")
#pragma rodata-name ("SW34RODATA")
#pragma local-strings (on)

extern struct change rec;                       /* (store.c's: the record written) */

/* Stratum n's populations waited for, the denizens' as its plan's grown (the hub's places kept, KEPT_AT), so its
** chunks are stocked by their keepers' lights from the first (dq0w.c's planned; and a game carried on, the places kept
** read back): their intents carried out meanwhile (they may be waiting on the world's), for as long as they take, not
** by the clock: a stratum's chunks stocked before its keepers are placed have their sconces dark, so the game went by
** how fast the denizens populated it (dq0robbed, 2026-10-30: ten seconds at most, stratum 0's populations then none
** or a third placed, by the code's layout).  Only a task that's stuck is left: thirty seconds with neither a keeping
** added nor a beat of its.  That at the turns pace alone (the tests' and replays': the same game whatever the code's
** speed); at the real paces not waited for (the game's start as quick as it was, the user's ten seconds): owed
** (kowed, the lights' minute's to look at), its stocked chunks' kept rooms lit as its populations are done
** (kept_late).  One owed at a time: another stratum's waited for first (the hub's header in the window after) */
extern unsigned char kowed;                     /* (dq0w.c's: a stratum's populations owed, its number plus 1) */
void __fastcall__ s_kept_wait (unsigned char n)
{
    unsigned t0 = hy_ticks ();
    unsigned char d, k, b, kn = 0xFF, kb = 0;

    hub_map (0);
    if ((HUB->pace & 3) != 3 && (!kowed || kowed == n + 1)) {
        kowed = n + 1;                          /* (A real pace: owed, kept_late's as they're done) */
        return;
    }
    for (;;) {
        hub_map (DEN_BANK);
        d = KEPT_DONE (KEPT_AT, n);
        k = KEPT_AT->n;
        hub_map (0);
        b = HUB->beat[T_DENIZENS];
        if (k != kn || b != kb) {
            kn = k;                             /* (On its way: the wait begun again) */
            kb = b;
            t0 = hy_ticks ();
        }
        if (d || HUB->quit || (unsigned) (hy_ticks () - t0) > 30 * HY_TICK_HZ) {
            return;
        }
        hub_sleep (1);
        if (DEN_INTENTS ()) {
            den_intents ();
        }
    }
}

/* Square x, y of stratum sn kept lit: within the range of a keeping whose keepers live (no further than their range
** either way from their lair's chunk's middle: the hub's places kept, the denizens'); the hub's header in the window
** after */
static unsigned char __fastcall__ kept (unsigned char sn, unsigned x, unsigned y)
{
    struct keep* q;
    unsigned char k;
    int dx, dy;

    hub_map (DEN_BANK);
    for (k = KEPT_AT->n, q = KEPT_AT->at; k; --k, ++q) {
        dx = (int) (x - STRATUM (sn).x0 - ((unsigned) q->cx << CW_BITS)) - CW / 2;
        dy = (int) (y - STRATUM (sn).y0 - ((unsigned) q->cy << CW_BITS)) - CW / 2;
        if (q->sn == sn && q->live && (dx < 0 ? -dx : dx) <= q->range && (dy < 0 ? -dy : dy) <= q->range) {
            break;
        }
    }
    hub_map (0);
    return k != 0;
}

/* The room square x, y of stratum sn is in lit by its sconce (lit R_LIT), or not (0): kept, made in its chunk (the
** cache's), and told (E_LIGHT, the senses'); the hub's header in the window after */
static void __fastcall__ room_lit (unsigned char sn, unsigned x, unsigned y, unsigned char lit)
{
    struct msg m;
    unsigned char r;

    rec.sn = sn;
    rec.x = x;
    rec.y = y;
    rec.tile = lit;
    rec.elev = 0;
    rec.link = ROOM_LIT;
    store_put ();
    if ((r = room_of (sn, x, y)) != 0xFF) {
        C->room[r].flags = (C->room[r].flags & ~R_LIT) | lit;
    }
    hub_map (0);
    memset (&m, 0, sizeof m);
    m.kind = E_LIGHT;
    m.a = sn;
    m.w1 = x;
    m.w2 = y;
    m.t = HUB->now;
    hub_event (&m);
}

/* The sconces in the item store on stratum sn, w squares each way from x0, y0 (sn 0xFF: all of them, the chunks round
** the character), their chunks in the cache (else seen to as they are again): each one's room lit if a light in it
** is lit, else not, as it's changed since the world last made it (its hide's SC_LIT; force: a lit one's kept
** whatever, a chunk first stocked); a keepers' torch in one its keepers no longer keep (all dead) tended no longer,
** burning down from then.  Two passes of the store: the lit and the tended in things (each sconce's marked, SC_NOW),
** then the sconces.  1 if a room's light changed */
static unsigned char __fastcall__ lights (unsigned char sn, unsigned x0, unsigned y0, unsigned char w,
    unsigned char force)
{
    struct item* p;
    struct cslot* c;
    unsigned i, x, y;
    unsigned char s, f, h, k, ch = 0;

    for (i = 0; (i = items (i, IN_ITEM | IN_FLAGS, I_LIT | I_TENDED)) != 0; ) {
        f = ip->flags;
        if ((p = item_at (ip->x))->kind != KI_SCONCE) {
            continue;
        }
        if (f & I_LIT) {
            p->hide |= SC_NOW;
        }
        if ((f & I_TENDED) && !kept (p->sn, p->x, p->y)) {
            item_at (i)->flags &= ~I_TENDED;    /* (Its keepers gone: it burns down from now) */
        }
    }
    for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {
        if (ip->kind != KI_SCONCE) {
            continue;
        }
        h = ip->hide;
        ip->hide = h & SC_LIT;                  /* (The pass's mark taken off) */
        s = ip->sn;
        x = ip->x;
        y = ip->y;
        if ((sn != 0xFF && (s != sn || x - x0 >= w || y - y0 >= w)) || (!(h & SC_NOW) == !(h & SC_LIT) &&
            !(force && (h & SC_NOW)))) {
            continue;
        }
        hub_map (WORLD_BANK);
        for (k = NCSLOTS, c = WREC->cslot; k && !(c->used && c->sn == s && c->cx == (x - STRATUM (s).x0) >> CW_BITS &&
            c->cy == (y - STRATUM (s).y0) >> CW_BITS); --k, ++c) {
        }
        if (k) {                                /* (Else as it is, seen to with its chunk in the cache again) */
            room_lit (s, x, y, h & SC_NOW ? R_LIT : 0);
            item_at (i)->hide = h & SC_NOW ? SC_LIT : 0;
            ch = 1;
        }
    }
    hub_map (0);
    return ch;
}

/* The rooms' lights by their sconces': the chunks round the character (reach 0xFF: each minute, a torch burnt out,
** its keepers gone); the chunk the maker's just stocked (SL_STOCKED: its first square rec's, as its things are put
** down, store.c's: each lit one's room's light kept, a record of it, the maker's R_LIT made so); or those within reach
** of the character (a thing handled: a sconce's torch taken, put in it, lit, put out).  1 if one changed (a look's to
** see it) */
unsigned char __fastcall__ s_sconce_lights (unsigned char reach)
{
    hub_map (0);
    return reach == 0xFF ? lights (0xFF, 0, 0, 0, 0) : reach == SL_STOCKED ? lights (rec.sn, rec.x, rec.y, CW, 1) :
        lights (HUB->pc_stratum, HUB->pc_x - reach, HUB->pc_y - reach, 2 * reach + 1, 0);
}

/* ==== Part 71's (its image in overlay 21's bank: part 34's had no room): a stratum's populations done after chunks
** of it were stocked (the real paces': s_kept_wait's, owed), each chunk in the cache stocked meanwhile, its rooms the
** maker would have kept lit had its keepers been placed (sconce.c's squares, by its dice, a copy; a room lit by its
** own light not), lit now (lit_late), one at a time; its locals the C stack's (a part it calls copies its image
** in again as it returns: none kept in its image) (the hub's header in the window after) */
#pragma code-name (push, "SW71CODE")
#pragma rodata-name (push, "SW71RODATA")
/* (Part 34's kept and room_lit, copies: no stubs for them, the main RAM's room) */
/* Square x, y of stratum sn kept lit: within the range of a keeping whose keepers live (no further than their range
** either way from their lair's chunk's middle: the hub's places kept, the denizens'); the hub's header in the window
** after */
static unsigned char __fastcall__ kept71 (unsigned char sn, unsigned x, unsigned y)
{
    struct keep* q;
    unsigned char k;
    int dx, dy;

    hub_map (DEN_BANK);
    for (k = KEPT_AT->n, q = KEPT_AT->at; k; --k, ++q) {
        dx = (int) (x - STRATUM (sn).x0 - ((unsigned) q->cx << CW_BITS)) - CW / 2;
        dy = (int) (y - STRATUM (sn).y0 - ((unsigned) q->cy << CW_BITS)) - CW / 2;
        if (q->sn == sn && q->live && (dx < 0 ? -dx : dx) <= q->range && (dy < 0 ? -dy : dy) <= q->range) {
            break;
        }
    }
    hub_map (0);
    return k != 0;
}

/* The room square x, y of stratum sn is in lit by its sconce (lit R_LIT), or not (0): kept, made in its chunk (the
** cache's), and told (E_LIGHT, the senses'); the hub's header in the window after */
static void __fastcall__ room_lit71 (unsigned char sn, unsigned x, unsigned y, unsigned char lit)
{
    struct msg m;
    unsigned char r;

    rec.sn = sn;
    rec.x = x;
    rec.y = y;
    rec.tile = lit;
    rec.elev = 0;
    rec.link = ROOM_LIT;
    store_put ();
    if ((r = room_of (sn, x, y)) != 0xFF) {
        C->room[r].flags = (C->room[r].flags & ~R_LIT) | lit;
    }
    hub_map (0);
    memset (&m, 0, sizeof m);
    m.kind = E_LIGHT;
    m.a = sn;
    m.w1 = x;
    m.w2 = y;
    m.t = HUB->now;
    hub_event (&m);
}

extern unsigned long xs_seed;                   /* (xorshift.s's state: the maker's dice's xorshift too) */
unsigned long rand32 (void);

/* The maker's dice's next (sconce.c's dr: (h >> 8) % n), by the library's generator, its state the maker's, and the
** remainder a byte at a time (no 32-bit shifts or division linked into the main RAM for it) */
static unsigned char __fastcall__ dr71 (unsigned char n)
{
    unsigned char r;

    rand32 ();
    r = ((unsigned char*) &xs_seed)[3] % n;
    r = ((unsigned) r << 8 | ((unsigned char*) &xs_seed)[2]) % n;
    return ((unsigned) r << 8 | ((unsigned char*) &xs_seed)[1]) % n;
}

/* A kept room's sconce square x, y lit late: its sconce (put there if the maker put none), a torch in it lit and
** tended (the cold one there, or one put in, as the maker's are: stock_down's), the room lit (a record, told) */
static void __fastcall__ lit_late (unsigned char sn, unsigned x, unsigned y)
{
    unsigned i, s = 0;

    for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {
        if (ip->kind == KI_SCONCE && ip->sn == sn && ip->x == x && ip->y == y) {
            s = i;
        }
    }
    if (!s && !(s = lay_body (KI_SCONCE, sn, x, y))) {
        return;                                 /* (The store full) */
    }
    for (i = 0; (i = items (i, IN_X | IN_ITEM, s)) != 0 && ip->kind != KI_TORCH; ) {
    }
    if (!i && (i = lay_body (KI_TORCH, sn, x, y)) != 0) {
        item_at (i)->where = IW_ITEM;
        item_at (i)->x = s;
    }
    if (i) {
        item_at (i)->flags |= I_LIT | I_TENDED;
        room_lit71 (sn, x, y, R_LIT);
        item_at (s)->hide = SC_LIT;
    }
}

void __fastcall__ s_kept_late (unsigned char sn)
{
    unsigned long kept_xs, h;
    unsigned char* b = (unsigned char*) &h;
    struct room* r;
    unsigned x0, y0;
    unsigned char k, q, n, d, lx, ly, nr, rw, rh, rx, ry;

    hub_map (DEN_BANK);
    k = KEPT_DONE (KEPT_AT, sn);
    hub_map (0);
    if (!k) {
        return;                                 /* (Not done yet: looked at again the next minute) */
    }
    kowed = 0;
    for (k = 0; k < NCSLOTS; ++k) {
        hub_map (WORLD_BANK);
        if (WREC->cslot[k].used != 1 || WREC->cslot[k].sn != sn) {
            continue;                           /* (Stocked ones: a fresh one's things not down yet, its own as
                                                **   they come) */
        }
        x0 = STRATUM (sn).x0 + (WREC->cslot[k].cx << CW_BITS);
        y0 = STRATUM (sn).y0 + (WREC->cslot[k].cy << CW_BITS);
        at (sn, x0, y0);
        nr = C->nrooms;
        for (q = 0; q < nr; ++q) {
            at (sn, x0, y0);
            r = &C->room[q];
            if (r->flags & R_LIT) {
                continue;
            }
            rx = r->x;
            ry = r->y;
            rw = r->w;
            rh = r->h;
            b[0] = q;                           /* (sconce.c's dseed (6, q): cx << 20 ^ cy << 8 ^ 6 << 28 ^ q, a byte
                                                **   at a time) */
            b[1] = (unsigned char) C->cy;
            b[2] = (unsigned char) (C->cx << 4) ^ (unsigned char) (C->cy >> 8);
            b[3] = (unsigned char) (C->cx >> 4) ^ 0x60;
            kept_xs = xs_seed;
            xs_seed = h * 2654435761UL + 7;
            for (n = 0; n < 8; ++n) {
                d = dr71 (4);
                lx = d == DIR_W ? rx : d == DIR_E ? rx + rw - 1 : rx + dr71 (rw);
                ly = d == DIR_N ? ry : d == DIR_S ? ry + rh - 1 : ry + dr71 (rh);
                if (lx && ly && lx + 1 < CW && ly + 1 < CW && TILE (C->tile[ly][lx]) == T_FLOOR &&
                    TILE (C->tile[ly + dir_y[d]][lx + dir_x[d]]) == T_WALL) {
                    break;                      /* (A square of its floor, the wall beside it) */
                }
            }
            xs_seed = kept_xs;                  /* (The world's own dice as they were) */
            if (n < 8 && kept71 (sn, x0 + lx, y0 + ly)) {
                lit_late (sn, x0 + lx, y0 + ly);
            }
        }
    }
    hub_map (0);
}
#pragma code-name (pop)
#pragma rodata-name (pop)
