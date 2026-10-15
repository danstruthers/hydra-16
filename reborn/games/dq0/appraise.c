/*
** appraise.c - (overlay 1) a thing looked at closely, in words: the appraiser's judgement of it (DUNGEON.md: items),
** not the thing as it is.  The referee tests the skill that knows that kind of thing (rs.h's P_APPRAISE: a weapon by
** a warrior's craft, a book of magic by none of theirs); the things task gives what the thing truly is (its make,
** its material, its durability, its worth); the judgement's as near to that as the test's grade allows: a critical
** success right, a marginal one near, a marginal failure well off, a critical failure wild (and said as surely).
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include "dq0.h"
#include "hub.h"

/* The materials (items.h's MT_*): their names, and their resistance to blows, edges, points, fire, cold, acid */
static const struct material mats[NMATS] = {
    { "", { 0, 0, 0, 0, 0, 0 } }, { "wood", { 4, 3, 4, 1, 7, 5 } }, { "iron", { 6, 7, 7, 7, 5, 2 } },
    { "steel", { 7, 8, 8, 8, 6, 3 } }, { "bronze", { 6, 6, 6, 7, 6, 4 } }, { "leather", { 5, 3, 3, 2, 6, 3 } },
    { "cloth", { 6, 1, 2, 0, 5, 1 } }, { "bone", { 3, 5, 5, 4, 7, 2 } }, { "clay", { 1, 6, 5, 9, 8, 6 } },
    { "stone", { 6, 8, 9, 9, 9, 4 } }, { "glass", { 0, 5, 3, 6, 4, 9 } }, { "silver", { 4, 5, 5, 6, 6, 5 } },
    { "", { 0, 0, 0, 0, 0, 0 } },
};
static const char* const harm[NDMG] = { "blows", "edges", "points", "fire", "cold", "acid" };
static const char* const make[10] = {
    "a crude", "a crude", "a poor", "a plain", "a sound", "a sound", "a good", "a fine", "a fine", "a master's"
};

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

/* v, moved by as much as pct percent of it, one way or the other */
static unsigned __fastcall__ off (unsigned v, unsigned char pct)
{
    unsigned d = v / 100 * pct + v % 100 * pct / 100;

    d = random (d + 1);
    return random (2) ? (v + d < v ? 0xFFFF : v + d) : v - d;
}

/* What the material stands up to (those of its resistances from lo to hi) */
static unsigned char __fastcall__ resists (const unsigned char* r, unsigned char lo, unsigned char hi, const char* how,
    unsigned char n)
{
    unsigned char d, k = 0;

    for (d = 0; d < NDMG; ++d) {
        if (r[d] >= lo && r[d] <= hi) {
            add (k++ ? ", " : n ? "; " : "");
            if (k == 1) {
                add (how);
            }
            add (harm[d]);
        }
    }
    return n + k;
}

void __fastcall__ o_appraise (const struct ireq* q, signed char g)
{
    signed char qual = q->quality;
    unsigned char mat = q->mat, life;
    unsigned val, dur = q->dur;
    char b[8];

    val = (q->value * (2 + q->quality) + 3) / 6;      /* (Its worth: its kind's, by its make and how worn it is) */
    life = q->dur_max ? (unsigned char) (dur / (q->dur_max / 10 + 1)) : 10;
    val = (val + 1) / 2 + val * life / 20;
    if (q->qty > 1) {
        val *= q->qty;
    }
    switch (g) {
    case G_CS:
        break;
    case G_MS:
        qual += random (3) - 1;
        dur = off (dur, 15);
        val = off (val, 20);
        break;
    case G_MF:
        qual += random (7) - 3;
        dur = off (dur, 50);
        val = off (val, 60);
        break;
    default:                                    /* (Wild, and as sure of it) */
        qual = random (10);
        if (mat >= MT_IRON && mat <= MT_BRONZE) {
            mat = MT_IRON + random (3);
        }
        dur = random (q->dur_max + 1);
        val = off (val, 90) * (random (3) + 1);
        break;
    }
    qual = qual < 0 ? 0 : qual > 9 ? 9 : qual;
    life = q->dur_max ? (unsigned char) (dur / (q->dur_max / 10 + 1)) : 10;
    lb[0] = 0;
    add ("You judge ");
    add (q->qty > 1 || q->plural ? "them" : "it");
    add (q->qty > 1 || q->plural ? " of " : " ");
    add (q->qty > 1 || q->plural ? make[qual] + 2 : make[qual]);
    add (q->qty > 1 || q->plural ? " make" : " ");
    if (!(q->qty > 1 || q->plural)) {
        add (q->name);
    }
    if (*mats[mat].name) {
        if (!strstr (q->name, mats[mat].name)) {    /* (Not "a pane of glass, of glass") */
            add (", of ");
            add (mats[mat].name);
        }
        add (q->qty > 1 || q->plural ? ": they stand up " : ": it stands up ");
        life = resists (mats[mat].resist, 7, 9, "well to ", 0);
        life = resists (mats[mat].resist, 4, 6, "fairly to ", life);
        resists (mats[mat].resist, 0, 3, "poorly to ", life);
        life = q->dur_max ? (unsigned char) (dur / (q->dur_max / 10 + 1)) : 10;
    }
    add (". ");
    if (q->size) {                              /* (Its size and thickness, plain to see: a portal's part's) */
        add (q->plural ? "They're " : "It's ");
        add (utoa (q->size & 15, b, 10));
        if (q->size >> 4) {
            add (" by ");
            add (utoa (q->size >> 4, b, 10));
            add (" feet");
        } else {
            add (" feet long");
        }
        if (q->thick) {
            add (", ");
            add (q->thick >= 8 ? utoa (q->thick >> 3, b, 10) : q->thick >= 4 ? "half an" : q->thick >= 2 ? "a quarter" :
                "an eighth of an");
            add (q->thick >= 16 ? " inches thick" : " inch thick");
        }
        add (". ");
    }
    if (q->qty > 1 || q->plural) {
        add (life >= 10 ? "They're as good as new" : life >= 7 ? "They have most of their life left" : life >= 4 ?
            "They have about half their life left" : life >= 2 ? "They have little life left" : "They're close to breaking");
    } else {
        add (life >= 10 ? "It's as good as new" : life >= 7 ? "It has most of its life left" : life >= 4 ?
            "It has about half its life left" : life >= 2 ? "It has little life left" : "It's close to breaking");
    }
    if (val) {
        add (q->qty > 1 || q->plural ? "; they would fetch about " : "; it would fetch about ");
        add (utoa (val >= 20 ? (val + 2) / 5 * 5 : val, b, 10));
        add (val == 1 ? " penny." : " pennies.");
    } else {
        add (", and worth nothing to anyone.");
    }
}
