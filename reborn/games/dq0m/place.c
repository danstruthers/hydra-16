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
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

/* ---- The dice */

static unsigned long dh;

unsigned char __fastcall__ dr (unsigned char n)
{
    unsigned r;

    ahead_check ();                             /* (A checkpoint of a chunk made ahead, each throw: the growers' busiest
                                                **   work between their own; nothing but the chunk's own state, and the
                                                **   dice's, keyed again before they're next thrown, under way here) */
    dh ^= dh << 13;
    dh ^= dh >> 17;
    dh ^= dh << 5;
    r = (unsigned char) (dh >> 24) % n;         /* ((dh >> 8) % n, a byte at a time: no 32-bit division, the dice the
                                                **   most of a chunk's time, 2026-11-17) */
    r = ((r << 8) | (unsigned char) (dh >> 16)) % n;
    return ((r << 8) | (unsigned char) (dh >> 8)) % n;
}

/* ---- The numbers of their own: what the caves, the sky, the plan, the uses and a mixed stratum decided by a hash of
** their own (none of the dice's streams), each a purpose (DH_*) whose formula is the one it always had, so the
** dungeons are as they were: a * m1 + b * m2 + c * m3, plus the stratum's seed's part (its row's: none; its low 16
** bits; their two halves xor'd; one of its bytes), that xor'd with sx; the sum xor'd with x; then mixed (h ^= h >> 7,
** h *= 0x2F1B: its high byte), or the sum itself (DH_MIX not set) */
struct dhrow {
    unsigned        m1, m2, m3, sx, x;
    unsigned char   sd;                         /* SD_*, and DH_MIX */
};
enum { SD_NONE, SD_LO, SD_FOLD, SD_B0, SD_B1, SD_B2, SD_B3 };
#define DH_MIX          0x80
static const struct dhrow dhrows[DH_N] = {
    { 40503U, 2654, 0, 0x3C5D, 0, SD_FOLD | DH_MIX },  /* DH_CAVERN (caves.c's great caverns: a region's, its
                                                **   wander, its pillars, a cave's sloping tunnel) */
    { 57, 23, 101, 0, 0x2C6B, DH_MIX },         /* DH_AIR (caves.c's bad air: the chunk's place and stratum) */
    { 31, 17, 101, 0, 0x5A5A, DH_MIX },         /* DH_WET (caves.c's water and rubble: the chunk's place, k) */
    { 31, 17, 101, 0, 0x5B17, DH_MIX },         /* DH_SKY (sky.c's skylights and windows: the chunk's place, k and
                                                **   the top stratum's seed's low byte, a byte) */
    { 40503U, 0, 0, 0x5A17, 0, SD_LO | DH_MIX },  /* DH_PWATER (plan.c's streams: k) */
    { 5, 3, 0, 0, 0, SD_B1 },                   /* DH_WELLS (plan.c's water under a region: its place) */
    { 7, 13, 0, 0, 0, SD_LO },                  /* DH_SLOPE (plan.c's long slopes: the stairway's square) */
    { 7, 13, 0, 0, 0, SD_B0 },                  /* DH_RING (uses.c's: a chunk the next ring's) */
    { 1, 1, 1, 0, 0, SD_NONE },                 /* DH_VAULT (uses.c's great hall's pointed vault: room c) */
    { 5, 3, 7, 0, 0, SD_NONE },                 /* DH_ROT (uses.c's: what a room has lying about gone to dust) */
    { 3, 5, 11, 0, 0, SD_NONE },                /* DH_ROTU (uses.c's: the older layer's mark gone to dust) */
    { 73, 151, 0, 0, 0, SD_B1 },                /* DH_MIXED (make.c's: a mixed stratum's chunk left natural) */
    { 0, 0, 0, 0, 0, SD_B3 },                   /* DH_DIAMOND (caves.c's: a stratum's passages on the diagonal) */
};

unsigned __fastcall__ dhash (unsigned char p, unsigned long s, unsigned a, unsigned b, unsigned c)
{
    const struct dhrow* q = &dhrows[p];
    unsigned char d = q->sd & ~DH_MIX;
    unsigned h;

    h = d == SD_NONE ? 0 : d == SD_LO ? (unsigned) s : d == SD_FOLD ? (unsigned) s ^ (unsigned) (s >> 16) :
        ((const unsigned char*) &s)[d - SD_B0];
    h = (a * q->m1 + b * q->m2 + c * q->m3 + (h ^ q->sx)) ^ q->x;
    if (q->sd & DH_MIX) {
        h ^= h >> 7;
        h *= 0x2F1B;
        h >>= 8;
    }
    return h;
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

/* A room's furniture (the user's, 2026-11-16: desks, cabinets, cases, shelves, their compartments the catalogue's):
** by its use, one of two kinds, or none (each one time in four), on a square of its floor beside a wall (sconce.c's
** way), listed with the stock (put_down's), if the list keeps room for what's to come after it (the theme's pieces,
** the portals' parts, the sconces) */
extern unsigned sx0, sy0;                       /* (stock.c's: the chunk's first square) */
static unsigned char fenv;                      /* (The environment's dq0furn: 0 not read yet; 1 none said; 2 "0",
                                                **   none placed, a test's scene's, as sky.c's dq0sky) */
static struct hy_regs far_;
static char fnm[8], fval[4];                    /* (In the main RAM: the name, its value) */
static const unsigned char furn[NUSES][4] = {
    { 0 }, { KI_CABINET, KI_SHELVES }, { 0 }, { KI_CASE, KI_CASE }, { 0 }, { 0 }, { 0 },   /* (chamber, hall, cell,
                                                **   vault, crypt, gallery, cave) */
    { KI_SHELVES, KI_CABINET }, { KI_SHELVES, KI_CABINET }, { KI_SHELVES, KI_SHELVES }, { 0 },   /* (store, kitchen,
                                                **   pantry, barracks) */
    { KI_SHELVES, KI_CABINET }, { KI_CASE }, { KI_DESK, KI_CASE }, { KI_DESK, KI_CABINET },   /* (armoury, chapel,
                                                **   quarters, solar) */
    { 0 }, { 0 }, { 0 }, { 0 }, { KI_SHELVES, KI_DESK }    /* (well, cistern, gatehouse, guardroom, workshop) */
};
static void __fastcall__ o_put_furn (unsigned char i)
{
    const struct room* r = &L->room[i];
    unsigned char n, d, lx, ly, k;

    if (!fenv) {
        strcpy (fnm, "dq0furn");
        far_.a = 0xFF;
        far_.r[0] = (unsigned) fnm;
        far_.r[1] = (unsigned) fval;
        far_.r[2] = 3;
        far_.r[3] = 0;
        fenv = !hy_call (HY_ENV_GET, &far_) && fval[0] == '0' ? 2 : 1;
    }
    if (fenv == 2 || r->kind >= NUSES || !(k = furn[r->kind][dr (4)]) || sk.n + 12 > NSTOCK) {
        return;
    }
    for (n = 0; n < 8; ++n) {
        d = dr (4);
        lx = d == DIR_W ? r->x : d == DIR_E ? r->x + r->w - 1 : r->x + dr (r->w);
        ly = d == DIR_N ? r->y : d == DIR_S ? r->y + r->h - 1 : r->y + dr (r->h);
        if (lx && ly && lx + 1 < CW && ly + 1 < CW && TILE (L->tile[ly][lx]) == T_FLOOR &&
            TILE (L->tile[ly + dir_y[d]][lx + dir_x[d]]) == T_WALL) {
            put_down (k, L->sn, sx0 + lx, sy0 + ly, 1, ST_SHOWN);
            return;
        }
    }
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
    /* A room whose use has furniture: one piece, one time in two, of the use's two, on a square of its floor beside a
    ** wall (its own dice: the user's, 2026-11-16) */
    { PG_STOCK, SI_ROOM, DP_FURNITURE, 0, 0, 0, U_ANY, 0, WH_BYWALL, 0, o_put_furn },
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
            ahead_check ();
            for (w = a; w < b; ++w) {
                site (at);
            }
        }
    }
}
