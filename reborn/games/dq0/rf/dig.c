/*
** dig.c - (the house's rules, ../rshouse.h's) the tools that dig: their table, the one to hand that suits best, and a
** dig's tool and rate for every actor (the one-path plan's stage 2, digging, 2026-11-23)
*/

#include <string.h>
#include "../hub.h"
#include "../rs.h"
#include "../referee.h"
#include "../rshouse.h"

const struct digger digger[NDIGGERS] = {        /* Earth, stone, masonry, flags, soil */
    { "hands", { 0, 0, 0, 0, 1 }, { 0, 0, 0, 0, 0 } }, { "shovel", RS_DIG_SHOVEL, { 0, 0, 0, 0, 0 } },
    { "pick", RS_DIG_PICK, { 0, 1, 1, 1, 0 } },
    { "chisel", { 0, 1, 1, 2, 0 }, { 0, 1, 1, 1, 0 } },   /* (Driven with a hammer, in both hands: the world's to
                                                **   say, as a dig's begun, 2026-11-02) */
    { "mattock", RS_DIG_SHOVEL, { 0, 0, 0, 0, 0 } }, { "spade", RS_DIG_SHOVEL, { 0, 0, 0, 0, 0 } },    /* (As a shovel, 2026-11-03) */
    { "blade", { 1, 0, 0, 0, 2 }, { 40, 0, 0, 0, 40 } }, { "dagger", { 1, 0, 0, 0, 2 }, { 40, 0, 0, 0, 40 } },
    { "knife", { 1, 0, 0, 0, 2 }, { 40, 0, 0, 0, 40 } },
    { "claws", { 2, 0, 0, 0, 4 }, { 0, 0, 0, 0, 0 } },    /* (A burrower's own, kinds.txt's ways d: two thirds of a
                                                **   shovel's in soil and earth, no rock, shale too hard for them,
                                                **   the user's, 2026-11-03: "Soft rock only"; the world's own copy
                                                **   before, dq0w/wdig.c's) */
};

#pragma static-locals (push, on)                /* (The referees' room: their locals in their BSS, not on the stack) */

unsigned char __fastcall__ house_dig_tool (unsigned char m, const char* name, const char* list)
{
    unsigned char k, t, best, r, br;
    const char* key;

    best = 0;
    br = *name ? 0 : digger[0].rate[m];
    for (k = 0; k < NTOOLS; ++k) {
        key = list ? list + (k << 3) : tools[k];    /* (An actor's: its keys in the tools' own form, eight bytes each,
                                                **   the first empty one the last) */
        if (!*key) {
            if (list) {
                break;
            }
            continue;
        }
        for (t = 1; t < NDIGGERS; ++t) {
            if (strcmp (key, digger[t].key) || (*name && strcmp (name, key))) {
                continue;
            }
            r = digger[t].rate[m];
            if (r > br || (r && r == br && digger[t].wear[m] < digger[best].wear[m]) || *name) {
                best = t;
                br = r;
            }
        }
    }
    return best;
}

unsigned char __fastcall__ house_dig (struct req* r)
{
    unsigned char m, t, k, b;
    char* x;
    const struct digger* d;

    x = r->text;
    m = RS_TEST_MAT (*r) > MAT_SOIL ? MAT_STONE : RS_TEST_MAT (*r);
    b = r->actor;                               /* (An actor's tools, listed: none named) */
    t = house_dig_tool (m, b ? "" : x, b ? x : 0);
    d = digger + t;
    k = d->rate[m];
    x[DIG_BETTER] = 0;
    if (!b && *x) {
        if (!t) {
            k = 0;                              /* (Named, and no digger) */
        }
        b = house_dig_tool (m, "", 0);
        if (digger[b].rate[m] > k) {
            strcpy (x + DIG_BETTER, digger[b].key);
        }
    }
    strcpy (x, d->key);
    RS_TEST_WEAR (*r) = k ? d->wear[m] : 0;
    RS_TEST_RATE (*r) = k;
    RS_TEST_CANT (*r) = !k;
    return k;
}

#pragma static-locals (pop)
