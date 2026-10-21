/*
** wkept.c - (dq0w) the rooms' lights kept by their sconces (DUNGEON.md: light kept; slice 4.1): a room with a sconce is
** lit (R_LIT, the light layer's own light of a room) as a light in its sconce is lit, by whoever lit it, and dark as
** it's put out, taken away or burnt away; a change record of it (ROOM_LIT, on the sconce's square: store.c's), so a
** chunk made again has it, and told (E_LIGHT: the senses' light layer).  A keepers' torch in one (I_TENDED: never
** burning down) is tended only while its keepers live (the denizens' places kept, the hub's KEPT_AT): theirs all
** dead, it burns down from then (the lights' burning).  Seen to after a thing's handled within reach (things.c's
** serve), each game minute for the chunks round the character (wbody.c's den_intents), and as a chunk's first stocked
** (store.c's stocked: the maker's kept rooms lit, R_LIT, a record each).  And a stratum's populations waited for as its
** plan's grown, so its chunks are stocked by them from the first.  Each sconce's hide keeps its room's light as
** the world last made it (SC_LIT), so a pass changes only what's changed.
**
** All of it swapped part 27's (../dq0/swap.inc: dq0w.cfg's SW27, overlay 9's bank), called through its stubs
** (bank.s's), its locals the C stack's (not the main RAM's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "wbody.h"

#pragma code-name ("SW27CODE")
#pragma rodata-name ("SW27RODATA")
#pragma local-strings (on)

extern struct change rec;                       /* (store.c's: the record written) */

/* Stratum n's populations waited for, the denizens' as its plan's grown (the hub's places kept, KEPT_AT), so its
** chunks are stocked by their keepers' lights from the first (dq0w.c's planned; and a game carried on, the places kept
** read back): their intents carried out meanwhile (they may be waiting on the world's), ten seconds at most (the hub's
** header in the window after) */
void __fastcall__ s_kept_wait (unsigned char n)
{
    unsigned t0 = hy_ticks ();
    unsigned char d;

    for (;;) {
        hub_map (DEN_BANK);
        d = KEPT_DONE (KEPT_AT, n);
        hub_map (0);
        if (d || HUB->quit || (unsigned) (hy_ticks () - t0) > 2000) {
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
** its keepers gone), or within reach of it (a thing handled: a sconce's torch taken, put in it, lit, put out); 1 if
** one changed (a look's to see it) */
unsigned char __fastcall__ s_sconce_lights (unsigned char reach)
{
    hub_map (0);
    return reach == 0xFF ? lights (0xFF, 0, 0, 0, 0) : lights (HUB->pc_stratum, HUB->pc_x - reach, HUB->pc_y - reach,
        2 * reach + 1, 0);
}


/* The kept rooms' lights of the chunk the maker's just stocked (its first square x0, y0 of stratum sn: store.c's, as
** its things are put down): each lit sconce's room's light kept, a record of it (the maker's R_LIT made so) */
void __fastcall__ s_sconce_chunk (unsigned char sn, unsigned x0, unsigned y0)
{
    lights (sn, x0, y0, CW, 1);
}
