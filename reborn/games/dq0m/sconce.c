/*
** sconce.c - (dq0m) a chunk's rooms' sconces (DUNGEON.md: light kept; slice 4.1), as it's first stocked (make.c's,
** after stock.c's: their things listed in the main RAM, sk, after the rest): a room built with them (the style's odds of
** a room lit by its own light, once, now its sconces': R_SCONCE, built.c's), and every room its keepers keep lit (its
** sconce's square within the range of a keeping whose keepers live: the denizens' places kept, the hub's KEPT_AT), but
** a cave's glowing chamber (its light nobody's), has one, on a square of its floor beside a wall: a kept room's with its
** torch lit and tended (the room lit by it, R_LIT, kept so by the world after: ROOM_LIT, dq0w/wkept.c), another's with
** a cold one, part burnt (the lights'), or none.  Each room's own dice (fixtures.c's, a copy of them), so nothing else
** moves for them.
**
** (Swapped part 3's, the maker's: ../dq0/swap.inc, dq0m.cfg's SW3, overlay 6's bank; called through bank.s's stub.)
*/

#pragma code-name ("SW3CODE")
#pragma rodata-name ("SW3RODATA")

#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

/* The fixtures' dice (fixtures.c's, a copy of its own): from the chunk's place, what's being decided (a), and
** where (b) */
static unsigned long dh;
static void __fastcall__ dseed (unsigned char a, unsigned b)
{
    dh = ((unsigned long) L->cx << 20 ^ (unsigned long) L->cy << 8 ^ (unsigned long) a << 28 ^ b) * 2654435761UL + 7;
}

static unsigned char __fastcall__ dr (unsigned char n)
{
    dh ^= dh << 13;
    dh ^= dh >> 17;
    dh ^= dh << 5;
    return (unsigned char) ((dh >> 8) % n);
}

/* Square x, y of stratum sn kept lit: within the range of a keeping whose keepers live (the square's no further
** than their range either way from their lair's chunk's middle), the hub's places kept, each copied out */
static unsigned char __fastcall__ kept (unsigned char sn, unsigned x, unsigned y)
{
    static struct keep q;
    static unsigned char n, i;
    int dx, dy;

    hub_get (DEN_BANK, &KEPT_AT->n, &n, 1);
    for (i = 0; i < n; ++i) {
        hub_get (DEN_BANK, &KEPT_AT->at[i], &q, sizeof q);
        dx = (int) (x - STRATUM (sn).x0 - ((unsigned) q.cx << CW_BITS)) - CW / 2;
        dy = (int) (y - STRATUM (sn).y0 - ((unsigned) q.cy << CW_BITS)) - CW / 2;
        if (q.sn == sn && q.live && (dx < 0 ? -dx : dx) <= q.range && (dy < 0 ? -dy : dy) <= q.range) {
            return 1;
        }
    }
    return 0;
}

/* Chunk c's rooms' sconces listed (its first square x0, y0 in the frame): a room built with sconces (R_SCONCE), or
** kept lit (its sconce's square within a keeping's range), but a glowing one (a cave's: its light nobody's), has one,
** on a square of its floor beside a wall; a kept room's with its torch lit and tended, the room lit by it (R_LIT);
** else with a cold one, part burnt (the lights'), or (one in four) none: burnt away, or taken, long ago */
void __fastcall__ s_sconces (struct chunk* c, unsigned x0, unsigned y0)
{
    struct room* r;
    unsigned char i, n, d, lx, ly, k;

    L = c;
    for (i = 0, r = c->room; i < c->nrooms; ++i, ++r) {
        if (r->flags & R_LIT) {
            continue;
        }
        dseed (6, i);
        for (n = 0; n < 8; ++n) {
            d = dr (4);
            lx = d == DIR_W ? r->x : d == DIR_E ? r->x + r->w - 1 : r->x + dr (r->w);
            ly = d == DIR_N ? r->y : d == DIR_S ? r->y + r->h - 1 : r->y + dr (r->h);
            if (lx && ly && lx + 1 < CW && ly + 1 < CW && TILE (c->tile[ly][lx]) == T_FLOOR &&
                TILE (c->tile[ly + dir_y[d]][lx + dir_x[d]]) == T_WALL) {
                break;                          /* (A square of its floor, the wall beside it) */
            }
        }
        if (n == 8 || (!(k = kept (c->sn, x0 + lx, y0 + ly)) && !(r->flags & R_SCONCE)) ||
            !put_down (KI_SCONCE, c->sn, x0 + lx, y0 + ly, 1, ST_SHOWN)) {
            continue;
        }
        if (k) {
            if (put_down (KI_TORCH, c->sn, x0 + lx, y0 + ly, 1, ST_LIT)) {
                r->flags |= R_LIT;              /* (Its keepers' light: the world's ROOM_LIT keeps it, as it's put
                                                **   down) */
            }
        } else if (dr (4)) {
            put_down (KI_TORCH, c->sn, x0 + lx, y0 + ly, 1, ST_IN);
        }
    }
}
