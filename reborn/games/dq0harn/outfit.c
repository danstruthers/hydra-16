/*
** outfit.c - (overlay 1, the referee's: called through bank.s's stubs) the character's outfit's makings (rs.h's
** RS_OUTFIT, the world's as a new game's kit is made: games/dq0/outfits.txt, drawn by the world, dq0w/outfit.c): its
** occupation, its open skills by their mastery (each one's weapons and tools), its family's wealth by its class
** (Character 17: the book's three levels a class, poor, average and rich; hmchar's file's "wealth" line, else rolled
** here from the character's own numbers, as hmchar rolls it: a quarter poor, half average, a quarter rich, an outcast
** or an unpopular child a tenth the poorer, a favourite a tenth the richer, DECISIONS.md's answer 2; a slave's a serf's
** poor, the house's), its culture, class, wealth and sex as the outfits' tags, and a seed for the draw (the character's
** own numbers, so the same character in the same game draws the same outfit).  Read and answered in a copy in the main
** RAM (the hub's not in the window while the overlay is), its words put into the reply's text.
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include <string.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* The family wealth by class (Character 17: pennies), poor, average, rich; a slave's the serf's poor */
static const unsigned purse[5][3] = {
    { 40, 40, 40 }, { 40, 80, 160 }, { 60, 120, 240 }, { 90, 180, 360 }, { 600, 1200, 2400 }
};
static const unsigned char culture_tag[6] = { OT_TRIBAL, OT_VIKING, OT_FEUDAL, OT_IMPERIAL, OT_KHUZAN, OT_SINDARIN };

void __fastcall__ o_outfit_of (struct req* r)
{
    unsigned char k, c = pc.social < 5 ? pc.social : HARN_SERF, lv, top, best, n;
    unsigned h = 0x2545, v;
    int roll;
    unsigned char open[HARN_NSKILLS];

    for (k = 0; k < HARN_NATTRS; ++k) {         /* (Its own numbers: its attributes and its name) */
        h = h * 31 + pc.att[k];
    }
    for (k = 0; name[k]; ++k) {
        h = h * 31 + (unsigned char) name[k];
    }
    if ((v = wealth) != 0) {                    /* (Its file's: the level nearest it) */
        lv = v < purse[c][1] ? 0 : v > purse[c][1] ? 2 : 1;
    } else {                                    /* (None: rolled, by its own numbers, hmchar's way) */
        roll = 1 + (h >> 4) % 100 + (pc.estrange <= HARN_UNPOPULAR ? -10 : pc.estrange == HARN_FAVORITE ? 10 : 0);
        lv = roll <= 25 ? 0 : roll <= 75 ? 1 : 2;
        v = purse[c][lv];
    }
    r->w1 = v;
    r->w2 = h;
    r->a = lv;
    v = 1u << culture_tag[pc.culture < 6 ? pc.culture : HARN_FEUDAL] | 1u << (OT_SLAVE + c) | 1u << (OT_POOR + lv) |
        1u << (pc.att[HARN_SX] == HARN_F ? OT_FEMALE : OT_MALE);
    r->x[0] = v;
    r->x[1] = v >> 8;
    r->result = 0;
    put (calling);                              /* (Its occupation, then its open skills, the highest first) */
    for (k = 0; k < HARN_NSKILLS; ++k) {
        open[k] = pc.ml[k];
    }
    for (n = 0; n < 40; ++n) {
        for (top = 0, best = 0xFF, k = 0; k < HARN_NSKILLS; ++k) {
            if (open[k] > top) {
                top = open[k];
                best = k;
            }
        }
        if (best == 0xFF) {
            break;
        }
        open[best] = 0;
        put ("\n");
        put (harn_get_skill_info (best)->name);
    }
}
