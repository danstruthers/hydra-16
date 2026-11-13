/*
** price.h - (the town, ./town.c, and the scholar's house, ./schol.c) a trade's local price: the ruleset's base (../rsmeta.h's
** meta_price) as the town's own sellers ask it (the user's, 2026-11-13: "Town prices should be determined by local
** availability, loosely based on what the ruleset dictates"; DECISIONS.md's "Town prices, local").  A town has as many of
** each trade as its size gives (a big town, town.txt's kind's, 4 to 6; another 1 to 3); each its own price, its hundredths of
** the base drawn round 100, five either way for each of them (one: 95-105; six: 70-130), so the more there are to choose from
** the wider the prices go; and which of them is at hand each day (the town's days so far: another day, maybe another).  All of
** it by the life's seed and the dungeon's number alone, its own numbers (never the dice's: the same life, the same sellers,
** and every roll of the town's the same as it was).  Included once, where these are: L (the life), tr (the time between's
** record: its days), big.
*/

enum { PT_TRADER = 1, PT_SMITH, PT_SCRIBE, PT_SAGE, PT_MASTER };    /* (The trades) */

static unsigned char pr_n, pr_at;               /* (How many of the last trade asked, and which of them was at hand) */

/* The town's own numbers: k (a trade's and its seller's, or its day's) mixed with the life's seed and the dungeon's */
static unsigned __fastcall__ pr_mix (unsigned k)
{
    unsigned char r;

    k ^= (unsigned) L.h.seed ^ (unsigned) (L.h.seed >> 16) ^ L.h.dungeon * 40503U;
    for (r = 0; r < 2; ++r) {                   /* (Its high bits into its low, and multiplied: twice) */
        k ^= k >> 8;
        k *= 0x6F4B;
    }
    return k ^ k >> 8;
}

/* Trade t's price today, its hundredths of the base: the seller's at hand (pr_at, of pr_n) */
static unsigned char __fastcall__ local (unsigned char t)
{
    unsigned char s;
    unsigned w = (unsigned) t << 12;

    pr_n = (big ? 4 : 1) + pr_mix (0) % 3;
    s = 5 * pr_n;
    pr_at = pr_mix (w | 0x800 | tr.days) % pr_n;
    return 100 - s + pr_mix (w | pr_at) % (2 * s + 1);
}
