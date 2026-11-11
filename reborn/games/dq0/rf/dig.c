/*
** dig.c - (the house's rules, ../rshouse.h's) the tools that dig: their table, and the one to hand that suits best
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
};

unsigned char __fastcall__ house_dig_tool (unsigned char m, const char* name)
{
    unsigned char k, t, best = 0, r, br = *name ? 0 : digger[0].rate[m];

    for (k = 0; k < NTOOLS; ++k) {
        for (t = 1; tools[k][0] && t < NDIGGERS; ++t) {
            if (strcmp (tools[k], digger[t].key) || (*name && strcmp (name, digger[t].key))) {
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
