/*
** stratagen.c - (a bank program: sdk/c/include/bankprog.h; the maker's and themegen's) a stratum's plan from its
** layers of history (games/dq0/HISTORY.md; D12, the user's: "A temple could sit over the ruin of an older keep, say.
** I just want the things on the same strata to make sense"): which layer's work each chunk of the stratum is now,
** whether a ruin left it broken, and the older layer under it.  The maker's (dq0m/uses.c: a chunk's uses planned by
** its layer's purpose, its stock with the older layer's remnants and a ruin's debris; plan.c: a piece of an era's
** laid among that era's works), and themegen's (its checker: every layer some chunks of a stratum, so its clues and
** its goal can lie among its works).  Pure: the same chunk the same each time, from the stratum's seed.
**
** The oldest built layer is the whole stratum's; each later one, reused, takes a part of it round the way in (those
** who came after came in that way), from a third to two thirds of the way to its far side by the stratum's seed, a
** later one's a part of an earlier's; a ruin befalls the works standing as it comes, a chunk of them by the odds its
** kind has (collapsed one in two, abandoned one in four ...), but what a later people made its own they put right
** (the ruin under it, the older layer's).
*/

#include "../../games/dq0/hub.h"

static unsigned long x;

static unsigned char __fastcall__ roll (unsigned char n)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return (unsigned) (x >> 8) % n;
}

static void __fastcall__ seed (unsigned long s)
{
    x = s ? s : 1;
    roll (2);                                   /* (Its first number passed over: a seed's low bits show in it) */
}

/* A ruin's odds, by how it came (theme.h's EN_*): one chunk in so many (0: none, an era still held) */
static const unsigned char odds[] = { 4, 3, 2, 3, 3, 0 };

unsigned char __fastcall__ layer_of (struct sgq* q)
{
    const struct tlayer* y;
    unsigned char i, cur = 0xFF, under = 0xFF, r = 0;
    unsigned d, dx, dy, edge, reach, lim;

    if (q->nl < 2) {
        return 0;
    }
    dx = q->cx > q->ix ? q->cx - q->ix : q->ix - q->cx;
    dy = q->cy > q->iy ? q->cy - q->iy : q->iy - q->cy;
    d = dx > dy ? dx : dy;                      /* (Its chunks from the way in's) */
    edge = q->ix > q->cw - 1 - q->ix ? q->ix : q->cw - 1 - q->ix;
    if (q->iy > edge) {
        edge = q->iy;
    }
    if (q->ch - 1 - q->iy > edge) {
        edge = q->ch - 1 - q->iy;
    }
    lim = edge;
    for (i = 0, y = q->l; i < q->nl && i < TLAYERS; ++i, ++y) {
        if (cur == 0xFF && y->did != TL_RUINED) {
            cur = i;                            /* (The first built: all of it) */
        } else if (y->did == TL_REUSED && lim) {
            seed (q->seed ^ (0x52455553UL + i));    /* (A part round the way in: the same for every chunk) */
            reach = (edge * (1 + roll (2)) + 2) / 3;
            if (reach >= lim) {
                reach = lim - 1;
            }
            lim = reach;
            if (d <= reach) {
                under = cur;
                cur = i;
            }
        }
    }
    if (cur == 0xFF) {
        return 0;
    }
    for (i = cur + 1, y = q->l + cur + 1; i < q->nl && i < TLAYERS; ++i, ++y) {
        if (y->did == TL_RUINED && y->work < sizeof odds && odds[y->work]) {
            seed (q->seed ^ ((unsigned long) q->cx << 16 | q->cy) * 2654435761UL ^ i);
            if (!roll (odds[y->work])) {
                r = SG_RUINED;                  /* (A ruin after its work: this chunk of it broken) */
            }
        }
    }
    return cur | r | (under != 0xFF ? under << 4 | SG_HASUNDER : 0);
}
