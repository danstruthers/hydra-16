/*
** stock.c - (dq0m) a chunk's ordinary things put down, once, as it's first made (DUNGEON.md: items): in its rooms
** (a cave's chambers), by its stratum's style's say (how often, which kinds, how many concealed, and how well); the
** things' records taken from the hub's item store under its lock (the things task, dq0p, keeps them after).  A
** chunk stocked is marked among the change records (STOCKED), so a chunk made again isn't stocked twice.
**
** (stock () is overlay 3's, the maker's, called through bank.s's stub; put_down the main code's, that maps the
** hub's banks, the window put back as it was after.)
*/

#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

/* A thing put down on a square (concealed: hide, its difficulty 0-9; else 0xFF): its record's number, or 0 (the
** window as it was) */
static unsigned __fastcall__ put_down (unsigned char kind, unsigned char sn, unsigned x, unsigned y, unsigned char qty,
    unsigned char hide)
{
    struct item* p;
    unsigned i;
    unsigned char b = getbank ();

    hub_map (0);
    hy_sem_acquire (HUB->item_lock);
    for (i = 1; i < NITEMS; ++i) {
        hub_map (ITEM_BANK (i));
        p = ITEM_AT (i);
        if (!p->kind) {
            memset (p, 0, sizeof *p);
            p->kind = kind;
            p->where = IW_SQUARE;
            p->sn = sn;
            p->x = x;
            p->y = y;
            p->qty = qty;
            if (hide != 0xFF) {
                p->flags = I_HIDDEN;
                p->hide = hide;
            }
            break;
        }
    }
    hub_map (0);
    hy_sem_release (HUB->item_lock);
    setbank (b);
    return i < NITEMS ? i : 0;
}

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

/* The chunk (in the main RAM; its first square x0, y0 in the frame) stocked: two tries a room, each a thing at the
** style's odds, at a square of its floor */
void __fastcall__ o_stock (const struct chunk* c, unsigned x0, unsigned y0)
{
    const struct room* r;
    unsigned char i, n, lx, ly, k, hidden;

    rseed (strata[c->sn].seed ^ ((unsigned long) c->cx << 16 | c->cy) * 69069UL ^ 0x5EED);
    for (i = 0; i < c->nrooms; ++i) {
        r = &c->room[i];
        for (n = 0; n < 2; ++n) {
            if (random (20) >= SP->things) {
                continue;
            }
            lx = r->x + random (r->w);
            ly = r->y + random (r->h);
            if (lx >= CW || ly >= CW || TILE (c->tile[ly][lx]) != T_FLOOR) {
                continue;
            }
            hidden = random (20) < SP->hidden;
            k = hidden ? SP->hide_kinds[random (4)] : SP->kinds[random (8)];
            put_down (k, c->sn, x0 + lx, y0 + ly, k == KI_PENNIES ? 2 + random (20) : 1,
                hidden ? SP->h_lo + random (SP->h_span) : 0xFF);
        }
    }
}
