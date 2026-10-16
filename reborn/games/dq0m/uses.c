/*
** uses.c - (dq0m) a built stratum's plan of uses (DUNGEON.md: built to a plan): its purpose the theme's (a keep, a
** temple, a manor's cellars, a crypt, a mine's works, plain halls; fortified or not), each chunk's role by its ring
** from the way in (a gate, a garrison, a domestic suite, the lord's rooms ...), its rooms given their uses along the
** chain the grower joins them in (the hall by its kitchen, the kitchen by its pantry, the stores beyond), a well
** where there's water; their floors flat (a built stratum's), a hall's dais; their ceilings by their size and use
** (a great hall's high and vaulted, a chapel's pointed, the small rooms' and the passages' one height, the
** builders'); and what their use left lying about, as the stock asks (a kitchen's pots and bones, a chapel's candle
** stubs), the stratum's age rotting what rots.  No dice of the grower's: its layout never moves for them.
**
** And the styles' proportions (struct style: a stand-in for the theme's data), the chunk's stratum's copied into
** make.c's sty (SP) as each chunk is made.
**
** (Overlay 6, the maker's: uses () called through bank.s's stub, by built.c (US_PLAN, US_FLOOR), stock.c (US_THING,
** US_SIGN) and make.c (US_STYLE); its data and BSS in the main RAM, as every overlay's are.)
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")

#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

unsigned char ufort, uwater;                    /* (The chunk's stratum fortified, the chunk's water: US_PLAN's) */

/* The styles' proportions (a stand-in for the theme's data): purpose-built (rooms and doors, most closed); natural
** (caves: no doors, unless sentient hands have been at them: the mixed style's); mines (galleries, few doors,
** timbered: built's, for now); mixed (built into natural) */
static const struct style styles[4] = {
    /* lit zones z3 slope  ceilings: flat barrel dome cathedral sloped rough   doors open lock bar  hid mag  wood ib
    ** chambers wide glow  things hidden h_lo h_span  kinds  hide_kinds
    ** earth: floors passages  masonry: walls ceilings  windows slits glass shutters bars  portcullises  traps pits
    ** hidden magic  door traps */
    { 5, 4, 1, 1, { 6, 4, 3, 2, 2, 3 }, 20, 8, 4, 2, 1, 1, 6, 3, 3, 1, 1,      /* built */
      6, 3, 2, 6, { KI_POTSHERDS, KI_BONES, KI_SKULL, KI_CANDLE, KI_BROKEN_SWORD, KI_SACK, KI_ROPE, KI_PENNIES },
      { KI_PENNIES, KI_KEY, KI_LOCKPICKS, KI_POUCH },
      6, 8, 20, 14, 6, 6, 2, 2, 2, 2, 3, 1, 8, 1, 2 },
    { 1, 0, 0, 1, { 0, 0, 0, 0, 0, 20 }, 0, 0, 0, 0, 0, 0, 0, 0, 3, 1, 1,      /* natural */
      4, 2, 1, 4, { KI_BONES, KI_BONES, KI_SKULL, KI_POTSHERDS, KI_BROKEN_SWORD, KI_PENNIES, KI_ROPE, KI_BONES },
      { KI_PENNIES, KI_PENNIES, KI_POUCH, KI_KEY },
      8, 6, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 0, 0, 0 },
    { 2, 2, 0, 1, { 12, 4, 0, 0, 2, 2 }, 8, 12, 2, 2, 1, 0, 9, 1, 2, 1, 0,     /* mine */
      5, 2, 2, 4, { KI_PICK, KI_SHOVEL, KI_ROPE, KI_CANDLE, KI_LANTERN, KI_SACK, KI_BONES, KI_POTSHERDS },
      { KI_PENNIES, KI_CHISEL, KI_POUCH, KI_KEY },
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 3, 6, 0, 1 },
    { 4, 3, 1, 1, { 4, 3, 2, 1, 2, 8 }, 10, 8, 3, 2, 1, 1, 6, 3, 3, 1, 1,      /* mixed */
      6, 3, 2, 6, { KI_POTSHERDS, KI_BONES, KI_SKULL, KI_CANDLE, KI_BROKEN_SWORD, KI_SACK, KI_ROPE, KI_PENNIES },
      { KI_PENNIES, KI_KEY, KI_LOCKPICKS, KI_POUCH },
      6, 8, 12, 8, 3, 6, 2, 2, 2, 1, 3, 2, 8, 1, 2 },
};

/* ---- The plan: a chunk's role by its ring from the way in, and the suite of uses a role is */

enum { R_GATE, R_GARRISON, R_DOMESTIC, R_LORD, R_SHRINE, R_CLOISTER, R_STORES, R_CELLS, R_TOMBS, R_WORKS };
static const unsigned char rings[6][5] = {      /* A purpose's (theme.h's P_*) roles, ring 0 to 4 and on */
    { R_DOMESTIC, R_STORES, R_DOMESTIC, R_LORD, R_CELLS },             /* halls */
    { R_GATE, R_GARRISON, R_DOMESTIC, R_DOMESTIC, R_LORD },            /* a keep */
    { R_SHRINE, R_CLOISTER, R_DOMESTIC, R_CLOISTER, R_SHRINE },        /* a temple */
    { R_STORES, R_STORES, R_DOMESTIC, R_STORES, R_CELLS },             /* a manor's cellars */
    { R_SHRINE, R_TOMBS, R_TOMBS, R_TOMBS, R_TOMBS },                  /* a crypt */
    { R_WORKS, R_WORKS, R_GARRISON, R_WORKS, R_STORES },               /* a mine's works */
};
static const unsigned char suites[10][11] = {   /* A role's main room, then the rooms east of it along the chain, then
                                                **   west of it */
    { U_GATEHOUSE, U_GUARDROOM, U_BARRACKS, U_ARMOURY, U_STORE, U_CELL,
      U_GUARDROOM, U_BARRACKS, U_STORE, U_KITCHEN, U_PANTRY },
    { U_BARRACKS, U_ARMOURY, U_STORE, U_CELL, U_CELL, U_STORE,
      U_GUARDROOM, U_KITCHEN, U_PANTRY, U_STORE, U_CHAMBER },
    { U_HALL, U_KITCHEN, U_PANTRY, U_STORE, U_STORE, U_CHAMBER,
      U_QUARTERS, U_QUARTERS, U_CHAMBER, U_STORE, U_CELL },
    { U_SOLAR, U_CHAPEL, U_VAULT, U_QUARTERS, U_STORE, U_CHAMBER,
      U_QUARTERS, U_HALL, U_KITCHEN, U_PANTRY, U_STORE },
    { U_HALL, U_CHAPEL, U_STORE, U_VAULT, U_QUARTERS, U_CELL,
      U_CHAPEL, U_GALLERY, U_CRYPT, U_CHAMBER, U_STORE },
    { U_GALLERY, U_CELL, U_CELL, U_CELL, U_QUARTERS, U_STORE,
      U_HALL, U_KITCHEN, U_PANTRY, U_STORE, U_WELL },
    { U_STORE, U_PANTRY, U_STORE, U_CISTERN, U_WELL, U_STORE,
      U_STORE, U_KITCHEN, U_CELL, U_QUARTERS, U_CHAMBER },
    { U_GUARDROOM, U_CELL, U_CELL, U_CELL, U_CELL, U_STORE,
      U_CELL, U_CELL, U_CELL, U_CHAMBER, U_STORE },
    { U_CRYPT, U_GALLERY, U_CRYPT, U_VAULT, U_CRYPT, U_CHAPEL,
      U_CRYPT, U_GALLERY, U_CRYPT, U_STORE, U_CHAMBER },
    { U_GALLERY, U_WORKSHOP, U_STORE, U_WORKSHOP, U_STORE, U_WELL,
      U_BARRACKS, U_KITCHEN, U_STORE, U_CHAMBER, U_CELL },
};

/* The stratum's way in: its first way up (the top one's way out; the foot of the way down from above), or 0xFF */
static unsigned char __fastcall__ way_in (const struct stratum* s)
{
    unsigned char i, k;

    for (i = 0; i < s->nlinks; ++i) {
        k = s->link[i].kind;
        if (k == O_WAY_OUT || k == O_STAIRS_UP || k == O_LADDER_UP || k == O_SHAFT_UP) {
            return i;
        }
    }
    return 0xFF;
}

/* The chunk's rooms given their uses (the grower's rooms, sorted from the west, each to be joined to the next) */
static void plan (void)
{
    const struct stratum* s = &strata[L->sn];
    const unsigned char* q;
    struct room* p;
    unsigned char purpose, in, ring, a, r, k, u, i;
    unsigned ex, ey, dx, dy, best;

    hub_get (THEME_BANK, &THEME->purpose[L->sn < TSTRATA ? L->sn : TSTRATA - 1], &purpose, 1);
    ufort = purpose & P_FORT;
    in = way_in (s);
    ring = 2;
    if (in != 0xFF) {                           /* (Its ring: chunks from the way in's, the farther way) */
        ex = (s->link[in].x - s->x0) / CW;
        ey = (s->link[in].y - s->y0) / CW;
        dx = L->cx > ex ? L->cx - ex : ex - L->cx;
        dy = L->cy > ey ? L->cy - ey : ey - L->cy;
        dx = dx > dy ? dx : dy;
        ring = dx > 4 ? 4 : dx;
    }
    if (ring && ring < 4 && !((L->cx * 7 + L->cy * 13 + (unsigned char) s->seed) & 3)) {
        ++ring;                                 /* (A few in four the next ring's, for variety) */
    }
    /* The water: a hook for the water's step (the plan's own, to come); for now two regions in three have it */
    uwater = ((L->cx / RCH) * 5 + (L->cy / RCH) * 3 + (unsigned char) (s->seed >> 8)) % 3 != 0;
    k = rings[purpose & 7][ring];
    if (!ring && ufort) {
        k = R_GATE;
    }
    q = suites[k];
    /* The main room: the largest (a gate's, the room the way in comes out in) */
    for (r = a = 0, best = 0; r < L->nrooms; ++r) {
        p = &L->room[r];
        if ((unsigned) p->w * p->h > best) {
            best = (unsigned) p->w * p->h;
            a = r;
        }
    }
    if (k == R_GATE) {
        for (i = 0; i < E->nland; ++i) {
            if (E->ln[i] == in) {
                for (r = 0; r < L->nrooms; ++r) {
                    p = &L->room[r];
                    if (E->lx[i] >= p->x && E->lx[i] < p->x + p->w && E->ly[i] >= p->y && E->ly[i] < p->y + p->h) {
                        a = r;
                    }
                }
            }
        }
    }
    for (r = 0; r < L->nrooms; ++r) {
        u = r == a ? q[0] : r > a ? (r - a <= 5 ? q[r - a] : U_STORE) : (a - r <= 5 ? q[5 + a - r] : U_CHAMBER);
        if ((u == U_WELL || u == U_CISTERN) && !uwater) {
            u = U_STORE;                        /* (No water: no well) */
        }
        L->room[r].use = L->room[r].kind = u;
    }
}

/* ---- Floors and ceilings */

/* A use's ceiling: its kind (0xFF: the style's, as it was grown), its height over the builders' (feet), and a foot
** more for so many squares of its floor (0: none) */
static const unsigned char ceils[NUSES][3] = {
    { 0xFF, 0, 0 }, { C_BARREL, 4, 6 }, { C_FLAT, 0, 0 }, { C_BARREL, 1, 0 },   /* chamber, hall, cell, vault */
    { C_BARREL, 1, 0 }, { C_BARREL, 2, 0 }, { 0xFF, 0, 0 }, { C_FLAT, 0, 0 },   /* crypt, gallery, cave, storeroom */
    { C_BARREL, 4, 12 }, { C_FLAT, 0, 0 }, { C_FLAT, 0, 0 }, { C_FLAT, 0, 0 },  /* kitchen, pantry, barracks, armoury */
    { C_CATHEDRAL, 8, 6 }, { C_FLAT, 0, 0 }, { C_BARREL, 3, 10 }, { C_BARREL, 1, 0 },   /* chapel, quarters, solar, well */
    { C_BARREL, 2, 10 }, { C_BARREL, 6, 0 }, { C_FLAT, 0, 0 }, { C_FLAT, 1, 0 },    /* cistern, gatehouse, guardroom,
                                                **   workshop */
};

/* The rooms' floors flat (a built stratum's: its ground's level), but a hall's dais (its last zone two feet up, a
** step along its front); and their ceilings by their size and use, the passages' the builders' height */
static void floors (void)
{
    const struct stratum* s = &strata[L->sn];
    struct room* p;
    struct zone* z;
    unsigned char r, i, x, y, h, k, across, std = 8 + ((unsigned char) (s->seed >> 3) & 3), c;
    unsigned area;
    signed char base = E->ground[0] > 120 ? 120 : E->ground[0] < -120 ? -120 : E->ground[0];

    L->pceil = std;
    for (r = 0; r < L->nrooms; ++r) {
        p = &L->room[r];
        area = (unsigned) p->w * p->h;
        across = p->w >= p->h;                  /* (Its zones side by side along its length: zones () in built.c) */
        for (i = p->z0, z = &L->zone[i]; i < p->z0 + p->nz; ++i, ++z) {
            if (s->style == ST_BUILT) {         /* (Flat: one plane, the stratum's level) */
                z->gx = z->gy = 0;
                z->z = base + (p->use == U_HALL && i && i + 1 == p->z0 + p->nz ? 2 : 0);
                for (y = z->y; y < z->y + z->h; ++y) {
                    for (x = z->x; x < z->x + z->w; ++x) {
                        L->tile[y][x] = T_FLOOR;
                        L->elev[y][x] = z->z;
                    }
                }
                if (z->z != base) {             /* (The dais's step, along its front: its first column, or row) */
                    for (k = 0; k < (across ? z->h : z->w); ++k) {
                        x = across ? z->x : z->x + k;
                        y = across ? z->y + k : z->y;
                        L->tile[y][x] = T_STEPS;
                        L->elev[y][x] = base + 1;
                    }
                }
            }
            c = ceils[p->use][0];
            if (c == 0xFF || s->style == ST_MINE) {
                continue;                       /* (The style's, as it was grown: a mine's timbering) */
            }
            h = std + ceils[p->use][1] + (ceils[p->use][2] ? area / ceils[p->use][2] : 0);
            if (c == C_BARREL && p->use == U_HALL && area >= 40 && (L->cx + L->cy + r) % 3) {
                c = C_CATHEDRAL;                /* (A great hall's pointed vault) */
            } else if (c == C_CATHEDRAL && p->w == p->h && z->w >= 3 && z->h >= 3) {
                c = C_DOME;                     /* (A square chapel's dome) */
            }
            z->ceil = c;
            z->cmid = h > 40 ? 40 : h;
            z->cwall = c == C_FLAT ? z->cmid : std;
        }
    }
}

/* ---- What a room's use left lying about (the stock's), the stratum's age rotting what rots */

static const unsigned char leavings[NUSES][8] = {
    { 0 }, { KI_TABLE, KI_CHAIR, KI_POTSHERDS, KI_BONES, KI_CANDLE, KI_PENNIES, KI_POTSHERDS, KI_SKULL },     /* hall */
    { KI_STRAW, KI_BONES, KI_SKULL, KI_STRAW, KI_POTSHERDS, KI_BONES, KI_ROPE, KI_STRAW },          /* cells */
    { KI_CHEST, KI_PENNIES, KI_PENNIES, KI_KEY, KI_SACK, KI_CHEST, KI_POUCH, KI_PENNIES },          /* a vault */
    { KI_BONES, KI_SKULL, KI_STUBS, KI_BONES, KI_SKULL, KI_POTSHERDS, KI_PENNIES, KI_BONES },       /* a crypt */
    { KI_POTSHERDS, KI_BONES, KI_CANDLE, KI_STUBS, KI_SKULL, KI_POTSHERDS, KI_ROPE, KI_PENNIES },   /* a gallery */
    { 0 }, { KI_SACK, KI_BASKET, KI_CHEST, KI_ROPE, KI_SACK, KI_POTSHERDS, KI_BASKET, KI_SACK },    /* a storeroom */
    { KI_POT, KI_BONES, KI_POTSHERDS, KI_TABLE, KI_POT, KI_POTSHERDS, KI_BONES, KI_DAGGER },        /* a kitchen */
    { KI_SACK, KI_BASKET, KI_POTSHERDS, KI_RATIONS, KI_SACK, KI_POTSHERDS, KI_BASKET, KI_BONES },   /* a pantry */
    { KI_STRAW, KI_STRAW, KI_BONES, KI_CLOAK, KI_PENNIES, KI_POTSHERDS, KI_CHAIR, KI_BROKEN_SWORD },    /* barracks */
    { KI_BROKEN_SWORD, KI_BROKEN_SWORD, KI_ARROWS, KI_QUIVER, KI_BROKEN_SWORD, KI_ARROWS, KI_CHEST, KI_BROKEN_SWORD },
    { KI_STUBS, KI_STUBS, KI_CANDLE, KI_POTSHERDS, KI_SKULL, KI_STUBS, KI_PENNIES, KI_BONES },      /* a chapel */
    { KI_CHAIR, KI_TABLE, KI_CLOAK, KI_CHEST, KI_POTSHERDS, KI_CANDLE, KI_PENNIES, KI_STRAW },      /* quarters */
    { KI_CHEST, KI_CHAIR, KI_TABLE, KI_CLOAK, KI_PENNIES, KI_CANDLE, KI_KEY, KI_POUCH },            /* the solar */
    { KI_ROPE, KI_POTSHERDS, KI_BASKET, KI_ROPE, KI_BONES, KI_POTSHERDS, KI_SACK, KI_PENNIES },     /* a well room */
    { KI_POTSHERDS, KI_BONES, KI_ROPE, KI_POTSHERDS, KI_PENNIES, KI_BONES, KI_POTSHERDS, KI_SKULL },    /* a cistern */
    { KI_ARROWS, KI_BROKEN_SWORD, KI_BONES, KI_ROPE, KI_POTSHERDS, KI_ARROWS, KI_CHAIR, KI_SKULL },     /* gatehouse */
    { KI_TABLE, KI_CHAIR, KI_POTSHERDS, KI_ARROWS, KI_CANDLE, KI_BROKEN_SWORD, KI_PENNIES, KI_BONES },  /* guardroom */
    { KI_CHISEL, KI_PICK, KI_SHOVEL, KI_ROPE, KI_TABLE, KI_POTSHERDS, KI_LANTERN, KI_SACK },        /* a workshop */
};
static const unsigned char signs[NUSES] = {     /* The mark its use left, always (if the years have spared it) */
    0, KI_TABLE, KI_STRAW, KI_CHEST, KI_BONES, 0, 0, KI_SACK, KI_POT, KI_POTSHERDS, KI_STRAW, KI_BROKEN_SWORD,
    KI_STUBS, KI_CHAIR, KI_CHEST, KI_WELL, 0, 0, KI_TABLE, KI_CHISEL
};

/* A kind rots (wood, cloth, straw, food, leather: the years take it); bones, pots, metal and stone last */
static unsigned char __fastcall__ rots (unsigned char k)
{
    return k == KI_STRAW || k == KI_SACK || k == KI_BASKET || k == KI_TABLE || k == KI_CHAIR || k == KI_CLOAK ||
        k == KI_CANDLE || k == KI_RATIONS || k == KI_ROPE || k == KI_ARROWS || k == KI_QUIVER || k == KI_POUCH;
}

/* What room r (arg's low three bits; its die, 0-7, the rest) has lying about: a kind, 0 nothing (rotted away); or
** (US_SIGN) its use's mark, and (arg's 0x80) a kitchen's well, where there's water */
static unsigned char __fastcall__ thing (unsigned char sign, unsigned char arg)
{
    unsigned char r = arg & 7, u = L->room[r].use, k;

    if (sign) {
        k = arg & 0x80 ? (u == U_KITCHEN && uwater ? KI_WELL : 0) : signs[u];
    } else {
        k = leavings[u][arg >> 3 & 7];
        if (!k) {
            return SP->kinds[arg >> 3 & 7];     /* (A chamber's, a cave's: the style's) */
        }
    }
    if (k && rots (k) && (unsigned char) (L->cx * 5 + L->cy * 3 + arg * 7) % 40 < strata[L->sn].age) {
        k = 0;                                  /* (Gone to dust) */
    }
    return k;
}

unsigned char __fastcall__ o_uses (unsigned char step, unsigned char arg)
{
    switch (step) {
    case US_STYLE:
        memcpy (&sty, &styles[arg & 3], sizeof sty);
        ufort = uwater = 0;
        break;
    case US_PLAN:
        plan ();
        break;
    case US_FLOOR:
        floors ();
        break;
    default:
        return thing (step == US_SIGN, arg);
    }
    return 0;
}
