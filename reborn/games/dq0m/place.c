/*
** place.c - (dq0m) the maker's dice, keyed, and its placers (ENGINE-CRITIQUE.md's §5, step 8): what places a
** chunk's fixtures and what they're made of rolls here, every overlay and swapped part alike.
**
** The dice: a number of the maker's is keyed by what it decides (a purpose: DP_*), the chunk's place and where in it
** (a room, a wall, a portal's square), never by what was rolled before it; dkey (purpose, where) sets them, dr (n)
** rolls (0 to n - 1), as many rolls after a key as its decision wants.  So a decision's numbers never move when
** another's are added or taken away, nor as the order of the work changes.  Each purpose's key is the one its placer
** always had (the doors' their own, pass.c's; the fixtures' fixtures.c's; the sconces' a copy of the portals' make's):
** the same dungeons, every seed's.  Those keys are the chunk's place alone (no stratum's seed: two strata's chunks at
** the same place have the same fixtures' numbers, as they always had); a purpose from DP_SEEDED on has its stratum's
** seed in its key too.  And the growers' streams (dice.h's: a layout's rolls, one after another, the order its work's
** done in): their seeds made here (rstream: the stratum's seed and the chunk's place, each stream's own way).
**
** The placers: what places a chunk's fixtures and things, a row each (placers[]: where each may go, how often by the
** style's odds and the rooms' uses, what it makes), in its stage of the chunk's making (fixtures.c's and stock.c's,
** and sconce.c's); place (stage) goes over the stage's rows, each over its sites (a room, a room's wall, a door), the
** site's key the dice's, its odds rolled, and a row's put (its hook) called for each site it's for.  Rows of a stage
** that follow one another over the same sites are placed together, site by site (a room's mark, then its well).
**
** (The rolls, dr, the main RAM's; the rest overlay 9's, called through bank.s's stubs, as the hooks are: the row a
** hook's called for copied to the main RAM, pr, for it to read, its own overlay in the window and not this one.)
*/

#include <stddef.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

/* ---- The dice */

static unsigned long dh;

unsigned char __fastcall__ dr (unsigned char n)
{
    dh ^= dh << 13;
    dh ^= dh >> 17;
    dh ^= dh << 5;
    return (unsigned char) ((dh >> 8) % n);
}

struct placer pr;                               /* (The row a hook's called for: PR) */
const struct placer* PR = &pr;

#pragma code-name ("OV9CODE")
#pragma rodata-name ("OV9RODATA")

static const unsigned char dk_a[DP_SEEDED] = { 0, 1, 2, 3, 4, 5, 6, 7, 6 };  /* (Each purpose's key: its number in */
static const unsigned char dk_c[DP_SEEDED] = { 1, 7, 7, 7, 7, 7, 7, 7, 7 };  /*   it, and what's added) */

void __fastcall__ o_dkey (unsigned char p, unsigned b)
{
    unsigned long k = (unsigned long) L->cx << 20 ^ (unsigned long) L->cy << 8 ^ b;

    if (p < DP_SEEDED) {
        dh = (k ^ (unsigned long) dk_a[p] << 28) * 2654435761UL + dk_c[p];
    } else {
        dh = (k ^ (unsigned long) p << 24 ^ STRATUM (L->sn).seed) * 2654435761UL + 7;
    }
}

/* A stream's seed (dice.h's rseed's): the chunk L's grower's (RS_GROW), its stock's (RS_STOCK) */
unsigned long __fastcall__ o_rstream (unsigned char p)
{
    unsigned long k = (unsigned long) L->cx << 16 | L->cy;

    return p == RS_STOCK ? STRATUM (L->sn).seed ^ k * 69069UL ^ 0x5EED : STRATUM (L->sn).seed ^ k * 40503UL;
}

/* ---- The placers */

/* Room r one of a fortified stratum's defences (uses.c's ufort): its gatehouse, its guardrooms */
unsigned char __fastcall__ o_defended (unsigned char r)
{
    unsigned char u = L->room[r].kind;

    return ufort && (u == U_GATEHOUSE || u == U_GUARDROOM);
}

/* The rows, in the order they're placed (a stage's: fixtures.c's, stock.c's, sconce.c's).  A row: its stage, its
** sites, its dice's purpose (DP_NONE none), its odds (a chance in sides of the style's field odds: none if sides is
** 0), what a site must not be (a room's flags, a door's state), the use a room must have (U_ANY any), its qualifiers
** (PQ_*), where in the site it goes (WH_*: its hook's to find), what it makes (0: its hook's choice) and its hook */
#define ODDS(f)     offsetof (struct style, f)
static const struct placer placers[] = {
    /* A room's side: a window or an arrow slit, where a passage runs beside it (a fortified stratum's defences: a slit
    ** wherever one covers a way) */
    { PG_WALLS, SI_WALL, DP_WINDOW, ODDS (windows), 20, 0, U_ANY, PQ_DEFENCE, WH_WALL, PK_WINDOW, put_window },
    /* A door: a portcullis instead (not a secret one) */
    { PG_WALLS, SI_DOOR, DP_PORTC, ODDS (portc), 20, PS_HIDDEN, U_ANY, 0, WH_DOOR, PK_PORTCULLIS, put_portc },
    /* A room: a trap on its floor (a pit, or one set off underfoot), where it can be walked round */
    { PG_TRAPS, SI_ROOM, DP_TRAP, ODDS (traps), 20, 0, U_ANY, 0, WH_FLOOR, 0, put_trap },
    /* A closed door: a trap set off as it's opened */
    { PG_TRAPS, SI_DOOR, DP_DTRAP, ODDS (d_trap), 20, PS_OPEN, U_ANY, 0, WH_DOOR, 0, put_dtrap },
    /* A room: its use's mark, in its middle; a kitchen's well, where the chunk has water, by its west wall (no dice) */
    { PG_STOCK, SI_ROOM, DP_NONE, 0, 0, 0, U_ANY, 0, WH_MIDDLE, 0, put_mark },
    { PG_STOCK, SI_ROOM, DP_NONE, 0, 0, 0, U_KITCHEN, PQ_WATER, WH_WEST, KI_WELL, put_mark },
    /* A room (not one lit by its own light): a sconce, on its floor beside a wall, if it was built with them or it's
    ** kept lit */
    { PG_SCONCES, SI_ROOM, DP_SCONCE, 0, 0, R_LIT, U_ANY, 0, WH_BYWALL, KI_SCONCE, put_sconce },
    { PG_END }
};

/* Row w at its site at (a room; a room's side, its number times 4 and the side; a portal's index), if it's for it */
static const struct placer* w;
static void __fastcall__ site (unsigned char at)
{
    const struct portal* p;
    const struct room* q;
    unsigned char r = w->site == SI_WALL ? at >> 2 : at;
    unsigned key = at;

    if (w->site == SI_DOOR) {
        p = &L->portal[at];
        if (p->kind != PK_DOOR || (p->state & w->clear)) {
            return;
        }
        key = p->x << 5 | p->y;
    } else {
        q = &L->room[r];
        if ((q->flags & w->clear) || (w->use != U_ANY && q->kind != w->use) || ((w->flags & PQ_WATER) && !uwater)) {
            return;
        }
    }
    if (w->purpose != DP_NONE) {
        o_dkey (w->purpose, key);
        if (w->sides && dr (w->sides) >= ((const unsigned char*) SP)[w->odds] && !((w->flags & PQ_DEFENCE) &&
            o_defended (r))) {
            return;
        }
    }
    pr = *w;
    pr.put (at);
}

/* The chunk L's placers of a stage, each over its sites */
void __fastcall__ o_place (unsigned char stage)
{
    const struct placer* a;
    const struct placer* b;
    unsigned char at;

    for (a = placers; a->stage != PG_END; a = b) {
        for (b = a + 1; b->stage == a->stage && b->site == a->site; ++b) {
        }
        if (a->stage != stage) {
            continue;
        }
        for (at = 0; at < (a->site == SI_DOOR ? L->nportals : a->site == SI_WALL ? L->nrooms << 2 : L->nrooms); ++at) {
            for (w = a; w < b; ++w) {
                site (at);
            }
        }
    }
}
