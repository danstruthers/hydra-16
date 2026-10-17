/*
** light.c - (dq0p) the light layer (DUNGEON.md: perception as layers): how bright each square is, worked out by the
** senses from the world's lights (its emitters: a lit room's own light, R_LIT for now; the lit things lying about,
** the item store's), a grid a chunk, half a byte a square (0 dark ... 15), in a bank of the senses' own: their cache,
** always rebuildable, never another task's.  A chunk's grid is worked out as it's first wanted (a look's squares, an
** actor's), and again only as something changes near it: a light lying about lit, put out, put down or taken up
** (things.c's lamps, each look, against the last), a door opened or shut, a square dug (the world's E_LIGHT, from
** its change records), the stratum left.  None of it on a clock.  A room's light fills its floor and its walls
** (L_ROOM); a lamp of reach r (its kind's light) lights the squares within r each way it has a clear line to (from
** two feet above its floor: cast.s's lamps, in the look's grid round it), 2 (r - d) + 1 at d squares off.  The
** character's own light is folded into its look (view.c), as it carries it.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "light.h"

#pragma static-locals (on)                      /* (Its locals static: the code the less) */

#define LSLOTS          16                      /* Chunks' grids kept (a bank's: 512 bytes each) */
#define GRID            ((unsigned char*) 0x8000)   /* (The look's grid, LOOK_BANK's: cast.s's) */
#define G_SKIP          0x02
#define G_LAMP          0x20
#define LREACH          8                       /* (A light's reach, at most: a change within it, near a chunk) */

extern unsigned char g_m, g_nl, g_li[NLAMPS], g_lj[NLAMPS], g_lr[NLAMPS], g_lzl[NLAMPS], g_lzh[NLAMPS];
void __fastcall__ cast_lamp (unsigned char w);
void __fastcall__ s_grid (unsigned char sn, unsigned gx, unsigned gy, unsigned char w, unsigned char lm);
extern unsigned char piece[256];
extern unsigned char run_lb;                    /* (cast.s's light_in: the senses' bank) */

static unsigned char lbank;                     /* The senses' own bank */
static struct lslot {                           /* Each grid's: */
    unsigned char   ok;                         /*   1 worked out (2: but a lamp whose chunk wasn't made: this look's) */
    unsigned char   sn;
    unsigned        x0, y0;                     /*   its chunk's first square, the frame's */
    unsigned        age;                        /*   when it was last wanted */
    unsigned char*  g;                          /*   its place in the bank */
} L[LSLOTS];
static unsigned lclock;
static unsigned char lsn_ = 0xFF;               /* (The stratum the lamps are of) */
struct lsrc ls[NLS];                            /* The lit things lying about (light.h's), as the last look found them */
unsigned char nls;
static struct lsrc ln[NLS];                     /*   and as this one finds them */
static unsigned char nln;
static unsigned place;                          /* (Its place in the world's events: E_LIGHT) */
unsigned char lgen;                             /* (light.h's) */

static unsigned char k;
static struct lslot* q;

void light_init (void)
{
    run_lb = lbank = hy_banks_alloc (1);
    for (k = 0, q = L; k < LSLOTS; ++k, ++q) {
        q->g = (unsigned char*) 0x8000 + ((unsigned) k << 9);
    }
    hub_map (0);
    place = HUB->eq_head;
}

static void all_stale (void)
{
    for (k = LSLOTS, q = L; k; --k, ++q) {
        q->ok = 0;
    }
}

/* The grids near square x, y of stratum sn (r squares more each way) to be worked out again */
static void __fastcall__ stale (unsigned char sn, unsigned x, unsigned y, unsigned char r)
{
    static unsigned lim;

    lim = CW + 2 * r;
    x += r;
    y += r;
    for (k = LSLOTS, q = L; k; --k, ++q) {
        if (q->ok && q->sn == sn && x - q->x0 < lim && y - q->y0 < lim) {
            q->ok = 0;
        }
    }
}

/* The world's events since the last: a light or an obstruction changed (E_LIGHT), the grids near it out of date */
void light_events (void)
{
    static struct msg* m;

    hub_map (0);
    if (HUB->eq_head - place > EQ_SIZE) {       /* (Too far behind: every grid's out of date) */
        all_stale ();
        place = HUB->eq_head;
    }
    for (; place != HUB->eq_head; ++place) {    /* (hub.c's hub_next, its reading alone: the senses send none) */
        m = &HUB->eq[place % EQ_SIZE];
        if (m->kind == E_LIGHT) {
            ++lgen;
            stale (m->a, m->w1, m->w2, LREACH);
        }
    }
}

/* ---- The lamps (things.c's, each look: the lit things lying about), against the last look's: those come or gone
** (lit, put out, put down, taken up), the grids near them out of date */
void light_lamps_begin (void)
{
    nln = 0;
}

void __fastcall__ light_lamp (unsigned x, unsigned y, unsigned char r)
{
    static struct lsrc* l;

    if (nln < NLS) {
        l = &ln[nln++];
        l->x = x;
        l->y = y;
        l->r = r;
    }
}

/* Square x, y of stratum sn in a chunk the cache has (none made for the light: the maker's order the look's) */
unsigned char __fastcall__ cached (unsigned char sn, unsigned x, unsigned y)
{
    static struct cslot* s;
    static struct stratum* t;
    static unsigned cx, cy;

    t = &STRATUM (sn);
    if ((cx = x - t->x0) >= t->w || (cy = y - t->y0) >= t->h) {
        return 0;
    }
    cx >>= CW_BITS;
    cy >>= CW_BITS;
    hub_map (WORLD_BANK);
    for (k = NCSLOTS, s = WREC->cslot; k && (!s->used || s->sn != sn || s->cx != cx || s->cy != cy); --k, ++s) {
    }
    hub_map (0);
    return k != 0;
}

static unsigned char sn_, part_;
static unsigned x0_, y0_;                       /* (Its first square, the frame's) */
static unsigned char* g_;                       /* (Its place in the bank, in the window) */
static void make (void);                        /* (Worked out: the main RAM's, below) */
void __fastcall__ lamp_on (const struct lsrc* l);   /* (Part 1's: a lamp's light on it) */

/* Square i, j of the grid at least lv bright */
static void __fastcall__ lit (unsigned char i, unsigned char j, unsigned char lv)
{
    static unsigned char* p;

    p = g_ + (j << 4) + (i >> 1);
    if (i & 1) {
        if (lv > *p >> 4) {
            *p = (*p & 0x0F) | lv << 4;
        }
    } else if (lv > (*p & 0x0F)) {
        *p = (*p & 0xF0) | lv;
    }
}


/* The grid of the chunk square x, y of stratum sn is in (worked out, if it isn't yet: mk), its place in the bank,
** or 0 (none: outside the stratum; or not worked out yet, mk 0) */
unsigned char* __fastcall__ light_grid (unsigned char sn, unsigned x, unsigned y, unsigned char mk)
{
    static struct stratum* t;
    static struct lslot* f;
    static unsigned ox, oy, old;

    t = &STRATUM (sn);
    if ((ox = x - t->x0) >= t->w || (oy = y - t->y0) >= t->h) {
        return 0;
    }
    x -= ox & (CW - 1);
    y -= oy & (CW - 1);
    for (k = LSLOTS, q = L, old = 0; k; --k, ++q) {
        if (q->ok && q->sn == sn && q->x0 == x && q->y0 == y) {
            q->age = ++lclock;
            return q->g;
        }
        if (!q->ok || lclock - q->age >= old) {     /* (The one least lately wanted, or a free one, for it) */
            old = q->ok ? lclock - q->age : 0xFFFF;
            f = q;
        }
    }
    if (!mk) {
        return 0;
    }
    sn_ = sn;
    x0_ = x;
    y0_ = y;
    g_ = f->g;
    make ();
    f->ok = 1 + part_;
    f->sn = sn;
    f->x0 = x;
    f->y0 = y;
    f->age = ++lclock;
    return f->g;
}

/* How bright square x, y of stratum sn is (its grid worked out if it must be): 0 dark ... 15 */
unsigned char __fastcall__ light_at (unsigned char sn, unsigned x, unsigned y)
{
    static unsigned char* p;
    static unsigned char i, v;

    if (!(p = light_grid (sn, x, y, 1))) {
        return 0;
    }
    i = (x - STRATUM (sn).x0) & (CW - 1);
    hy_bank (lbank);
    v = p[(((y - STRATUM (sn).y0) & (CW - 1)) << 4) + (i >> 1)];
    hub_map (0);
    return i & 1 ? v >> 4 : v & 0x0F;
}


/* ---- A chunk's grid worked out (the main RAM's: once part 3's, swapped in as a grid was wanted, the look's part 1
** put out for it, and in again after, part 3 out for its lamps' part 1 between: a look into new chunks copied 22K) */
static void make (void)
{
    static unsigned char a, b, i, j, n, rx0[NROOMS], ry0[NROOMS], rx1[NROOMS], ry1[NROOMS];
    static struct room* r;
    static struct lsrc* l;

    part_ = 0;
    ++lgen;
    at (sn_, x0_, y0_);                         /* (The rooms lit by their own light: their floors and walls) */
    for (a = C->nrooms, r = C->room, n = 0; a; --a, ++r) {
        if (r->flags & R_LIT) {
            rx0[n] = r->x ? r->x - 1 : 0;
            ry0[n] = r->y ? r->y - 1 : 0;
            rx1[n] = r->x + r->w < CW ? r->x + r->w : CW - 1;
            ry1[n++] = r->y + r->h < CW ? r->y + r->h : CW - 1;
        }
    }
    hy_bank (lbank);
    memset (g_, 0, 512);
    for (b = 0; b < n; ++b) {
        for (j = ry0[b]; j <= ry1[b]; ++j) {
            for (i = rx0[b]; i <= rx1[b]; ++i) {
                lit (i, j, L_ROOM);
            }
        }
    }
    for (b = nls, l = ls; b; --b, ++l) {        /* (The lamps whose reach comes into it) */
        if (l->x + l->r - x0_ < CW + 2 * l->r && l->y + l->r - y0_ < CW + 2 * l->r) {
            lamp_on (l);
        }
    }
    hub_map (0);
}

/* ---- Part 1's (swapped: with the look's grid and its lines, view.c's and cast.s's, the lamps found, things.c's):
** the lamps against the last look's; a lamp's light on a chunk (bank.s's stub) */
#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* Those of list a (n of them) not in list b (m), stale round them */
static void __fastcall__ gone (const struct lsrc* a, unsigned char n, const struct lsrc* b, unsigned char m)
{
    static const struct lsrc* p;
    static unsigned char j;

    for (; n; --n, ++a) {
        for (p = b, j = m; j && (p->x != a->x || p->y != a->y || p->r != a->r); --j, ++p) {
        }
        if (!j) {
            stale (lsn_, a->x, a->y, a->r + 1);
        }
    }
}

void __fastcall__ light_lamps_end (unsigned char sn)
{
    for (k = LSLOTS, q = L; k; --k, ++q) {
        if (q->ok == 2) {                       /* (A grid with a lamp left out, its chunk not made: again) */
            q->ok = 0;
        }
    }
    if (sn != lsn_) {                           /* (Another stratum: its grids worked out afresh) */
        all_stale ();
        lsn_ = sn;
    } else {
        gone (ls, nls, ln, nln);
        gone (ln, nln, ls, nls);
    }
    memcpy (ls, ln, sizeof ls);
    nls = nln;
}

/* ---- A chunk's grid worked out */


/* Lamp l's light on the chunk: its squares round it (r each way) gridded (the cached chunks' alone: view.c's grid,
** lmode), the lines from it cast (cast.s), each square it reaches in the chunk lit, the further the dimmer */
void __fastcall__ s_lamp_on (const struct lsrc* l)
{
    static unsigned char w, i, j, r, d, e;
    static unsigned gx0, gy0;
    static int z;

    if (!cached (sn_, l->x, l->y)) {
        part_ = 1;                              /* (Its chunk not made: its light left out, this look's alone) */
        return;
    }
    r = l->r;
    w = 2 * r + 1;
    gx0 = l->x - r;
    gy0 = l->y - r;
    z = elev_at (sn_, l->x, l->y) + 2;
    s_grid (sn_, gx0, gy0, w, 1);
    g_m = 0;
    g_nl = 1;
    g_li[0] = g_lj[0] = g_lr[0] = r;
    g_lzl[0] = z;
    g_lzh[0] = z >> 8;
    hub_map (LOOK_BANK);
    cast_lamp (w);
    g_nl = 0;
    gx0 -= x0_;                                 /* (The grid's first square, in the chunk's) */
    gy0 -= y0_;
    for (j = 0; j < w; ++j) {
        if (gy0 + j >= CW) {
            continue;
        }
        hub_map (LOOK_BANK);
        memcpy (piece, GRID + ((unsigned) j << 7), 2 * w);
        hy_bank (lbank);
        e = j > r ? j - r : r - j;
        for (i = 0; i < w; ++i) {
            if ((piece[2 * i] & G_LAMP) && gx0 + i < CW) {
                d = i > r ? i - r : r - i;
                lit (gx0 + i, gy0 + j, 2 * (r - (d > e ? d : e)) + 1);
            }
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
