/*
** make.c - (dq0m) a chunk made (DUNGEON.md), into the hub's cache (world.h), for whoever asked: the same chunk
** whoever it's for (the stratum's seed's and its place's).  The slot it goes in: one empty, or the one least lately
** found, but never one a task is reading (the hub's inuse) nor one beside the character's chunk.
**
** A chunk's edges (the ways across them, the ground at its corners) are the plan's, worked out by plan.c's
** chunk_edges (overlay 4); the chunk grown by its style's grower (built.c, caves.c: overlays 1 and 3) in the main
** RAM, and given its materials and its fixtures (fixtures.c), then copied into the cache.
**
** What's changed in a chunk (the change records, the world's: a square dug, a link made, a door opened) is laid
** over it each time it's made (changes.c), and what the character's seen of it is kept as the cache makes way for
** it, and laid over it again.  A chunk stocked as it's made (its things listed: stock.c, the list the world bank's
** stock beside its slot) has its slot CS_FRESH, for the world to put them down and mark it stocked among the change
** records; the maker never makes way for it till it has.
**
** And the chunks made ahead of the actors' eyes (the user's, 2026-11-17), in slots free to the readers: grown as the
** maker has nothing asked of it (dq0m.c's choice), finished as a reader first asks for one (below).
*/

#include <setjmp.h>
#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"
#include "../dq0/rp.h"

struct chunk cb;                                /* (A chunk grown in the main RAM: the grower's an overlay) */
static unsigned stocks;                         /* (The chunks stocked, counted: each list's seq) */

struct style sty;                               /* (The chunk's stratum's: uses.c copies it from its styles) */
const struct style* SP;
const unsigned char t_kinds[16] = { 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1 };  /* (world.h's T_PASS: the world's slant.s's, the same) */

/* A hub's bank's bytes copied to the main RAM (hub_get), or from it (hub_put), or set (hub_set), the window put back
** as it was: the hub reached from an overlay's code (which the hub's bank in the window would put out) */
void __fastcall__ hub_get (unsigned char bank, const void* from, void* to, unsigned n)
{
    unsigned b = hub_here ();

    hub_map (bank);
    memcpy (to, from, n);
    hub_back (b);
}

void __fastcall__ hub_put (unsigned char bank, void* to, const void* from, unsigned n)
{
    unsigned b = hub_here ();

    hub_map (bank);
    memcpy (to, from, n);
    hub_back (b);
}

void __fastcall__ hub_set (unsigned char bank, void* to, unsigned char c, unsigned n)
{
    unsigned b = hub_here ();

    hub_map (bank);
    memset (to, c, n);
    hub_back (b);
}

/* Slot k a task's reading (the hub's inuse), or one stocked that the world hasn't marked yet (CS_FRESH): not made way
** for (the record store's question, as pick asks it: the world bank in the window) */
static unsigned char __fastcall__ busy (unsigned char k)
{
    unsigned char i, b = WREC->cslot[k].used == CS_FRESH;

    for (i = 0; i < NASK; ++i) {
        b |= WREC->inuse[i] == k;
    }
    return b;
}

/* The slot a new chunk goes in: an empty one, or the least lately found (its stamp the oldest, round as the stamps
** wrap, as the other stores' are: the user's, 2026-11-12) that no task is reading, isn't beside the character's
** chunk, and isn't one stocked that the world hasn't marked yet (CS_FRESH): the record store's (the hub's header in
** the window; the world bank after, the cache's table its) */
static struct rpt cpk = { RP_OLDEST, NCSLOTS, sizeof (struct cslot), 0x3E, WREC->cslot, offsetof (struct cslot, stamp),
    RTF_FREE | RTF_WIDE, 0, { 0 }, 0, 0, 0, 1, 1, 2, 4, 0, 0, 0, busy };
static unsigned char pick (void)
{
    unsigned char best;
    struct stratum* s = &STRATUM (cpk.vsn = HUB->pc_stratum);

    cpk.vx = (HUB->pc_x - s->x0) / CW;
    cpk.vy = (HUB->pc_y - s->y0) / CW;
    cpk.n = HUB->small ? 12 : NCSLOTS;          /* (dq0 -C: twelve slots, the tests' cache made way in soon) */
    hub_map (WORLD_BANK);
    return (best = rp_x (&cpk)) == 0xFF ? 0 : best;
}

/* The chunk's style (grow.h's cstyle): its stratum's; but a mixed stratum's (DUNGEON.md's rock and old places,
** slice 4.10's: caves that sentient hands have been at) a third of its chunks left as they were found, grown as caves
** by the natural style, the rest hewn; a chunk with a way between strata in it (the way in, a way down) hewn, where
** the builders' work began.  (dq0 -M, the tests': every built stratum mixed so) */
unsigned char cstyle;

struct edges me;                                /* (The chunk's edges: uses.c's US_STYLE sets the tests' flags in them) */

/* The chunk (its edges in me) grown in the main RAM (cb, the chunk L): its squares, its rooms and their uses, its
** materials, its fixtures, its sky, its water and its air: the seed's alone (the stratum's, its plan's, the theme's
** style), nothing of the world's as it is; what its plan of uses left for its stock in us (uses.c's).  (Overlay 10's,
** the main RAM's room: grow () its stub, bank.s's) */
#pragma code-name (push, "OV10CODE")
#pragma rodata-name (push, "OV10RODATA")
void __fastcall__ o_grow (unsigned char sn, unsigned cx, unsigned cy)
{
    struct stratum* s = &STRATUM (sn);
    struct chunk* c;
    unsigned char u;

    L = c = &cb;                                /* (The chunk L: place.c's dice's and streams' keys) */
    memset (c, 0, sizeof *c);
    c->sn = sn;
    c->cx = cx;
    c->cy = cy;
    SP = &sty;
    hub_get (0, &HUB->gear, &u, 1);
    cstyle = (s->style == ST_MIXED || (u & GE_MIXED && s->style == ST_BUILT)) && !me.nland &&
        (unsigned char) dhash (DH_MIXED, s->seed, cx, cy, 0) % 3 == 0 ? ST_NATURAL : s->style;        /* (A third, by the stratum's seed and the chunk's place) */
    uses (US_STYLE, cstyle | u << 2);          /* (The style, and the tests' flags: no water, no rubble, the passages
                                                **   on the diagonal) */
    ahead_check ();                             /* (Between its parts, made ahead: each some 100,000 cycles and more) */
    if (me.x1 != 0xFF) {                         /* (By its stratum's style, or its part's) */
        if (cstyle == ST_NATURAL) {
            grow_caves (c, &me, rstream (RS_GROW));
        } else {
            grow_built (c, &me, rstream (RS_GROW));
        }
        ahead_check ();
        fixtures (c);                           /* (Its materials, its windows, its traps ...) */
        ahead_check ();
        sky (c, 0);                             /* (Its skylights and windows on the surface: sky.c's) */
    }
    ahead_check ();
    wet_chunk (c, &me);                          /* (Its water: slice 4.9's; then age's rubble, slice 4.10's) */
    ahead_check ();
    air_pocket (c, &me);                         /* (Its air: 2026-10-31) */
}
#pragma code-name (pop)
#pragma rodata-name (pop)
void __fastcall__ grow (unsigned char sn, unsigned cx, unsigned cy);

/* The chunk grown (cb) finished by the world as it is, into slot k: what's changed in it laid over it (the change
** records); its things listed the first time it's made (stock.c's, sconce.c's: the world puts them down and marks it
** so, STOCKED, as it finds its slot CS_FRESH, in the order they were stocked); its squares by what shows the sky
** marked; what's been seen of it laid over it (the hub's header in the window after) */
static void __fastcall__ finish (unsigned char k)
{
    struct stratum* s = &STRATUM (cb.sn);
    unsigned x = s->x0 + cb.cx * CW, y = s->y0 + cb.cy * CW;
    unsigned char u = 1;

    L = &cb;
    if (!changes_over (&cb, x, y) && me.x1 != 0xFF) {
        stock (&cb, x, y);                      /* (Its things listed) */
        sconces (&cb, x, y);                    /* (Its rooms' sconces, their torches lit where they're kept) */
        sk.seq = ++stocks;
        hub_put (WORLD_BANK, &WREC->stock[k], &sk, sizeof sk - sizeof sk.e + sk.n * sizeof sk.e[0]);
        u = CS_FRESH;
    }
    sky (&cb, 1);                               /* (The squares by what shows the sky marked: sky.c's) */
    seen_over (&cb);
    hub_map (CACHE_BANK (k));
    memcpy (CHUNK_AT (k), &cb, sizeof cb);
    hub_map (WORLD_BANK);
    WREC->cslot[k].used = u;
    hub_map (0);
}

/* ---- The chunks made ahead (the user's, 2026-11-17: the maker builds the map's pieces ahead of the character's eyes,
** in the background, so a look or a walk finds them made; the cost the cache's room): each grown (grow) into a slot
** that's free to the readers (its used 0: none finds it, so none places a lair by it, stocks it or sees into it), what
** its plan of uses left for its stock (us) kept in its kept (idle till it's finished: struct ustate's size); as a
** reader first asks for it (make_chunk), finished there and then (finish: the change records, its things, what's been
** seen of it, as the world is at that moment, the moment a chunk not made ahead is made) into the slot it would have
** been made in: the same chunk, byte for byte, and the same cache.  Which, and where, ahead.c's; when, dq0m.c's (as
** nobody waits on the maker, giving way to a command in hand), each held as it gives way and given up as a request
** comes for another (ahead_check, the growers' checkpoints) */
typedef char us_fits[sizeof (struct ustate) == sizeof (struct ckept) ? 1 : -1];

/* The chunk asked for (a reader's request: the world's, the denizens', the senses', the UI's) made into the cache:
** grown, or, made ahead already, taken from its slot (moved, if another's in the slot it goes in); finished, into the
** slot pick () gives (made way for if it's used: what's been seen of it kept), the slot a chunk not made ahead would
** have had.  But one in the cache already (two readers asking for it at once, the second's request answered as the
** first's is made: made once, not twice, its things not listed twice, 2026-11-17) not made again.  Its slot */
unsigned char __fastcall__ make_chunk (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k, j;
    struct cslot* sl;

    if ((k = ahead_found (sn, cx, cy)) != 0xFF) {
        return k;
    }
    j = ahead_at (sn, cx, cy);

    if (j == 0xFF) {
        chunk_edges (sn, cx, cy, &me);
    }
    hub_map (0);
    k = pick ();
    if (WREC->cslot[k].used) {                  /* (What's been seen of the one made way for, kept) */
        seen_keep (k);
        hub_map (WORLD_BANK);
    }
    sl = &WREC->cslot[k];
    sl->used = 1;
    sl->sn = sn;
    sl->cx = cx;
    sl->cy = cy;
    sl->stamp = ++WREC->stamp;
    hub_map (0);
    if (j == 0xFF) {
        grow (sn, cx, cy);
    } else {
        ahead_take (j, k);                      /* (Made ahead: as it was grown, and what its plan left) */
    }
    finish (k);
    ++ah_ver;
    return k;
}

/* ---- Making one ahead: at the growers' checkpoints (ahead_check), held as it gives way to a command (dq0m.c's
** ahead_wanted), given up as a request comes for anything but it, the stacks it was called with put back (longjmp: the
** overlays' and the parts' call stacks, bank.s's ahead_mark, ahead_back) */
static jmp_buf ajb;
static unsigned char aheading;                  /* (A chunk being made ahead: the checkpoints look) */
unsigned char ahead_wanted (void);              /* (dq0m.c's: a request posted, or a command: 1) */
void ahead_mark (void);                         /* (bank.s's) */
void ahead_back (void);
void fore_again (void);                         /* (dq0m.c's: in the foreground or not, as the requests are now) */

void ahead_check (void)
{
    unsigned w;

    if (!aheading) {
        fore_again ();                          /* (A requested chunk: its asker waited on by the foreground since,
                                                **   a test's scene: even3's) */
        return;
    }
    w = hub_here ();
    if (ahead_wanted ()) {
        aheading = 0;
        longjmp (ajb, 1);
    }
    hub_back (w);
}

/* Chunk sn, cx, cy made ahead: grown, then into the slot ahead_room gives (ahead.c's ahead_keep), what its plan left
** kept with it: 1, or 0 (given up, or no room) */
/* Own slot k's chunk (the maker's own banks: ahead.c's ahb) into cb, or (put) cb into it; the window put back */
void __fastcall__ own_copy (unsigned char k, unsigned char put)
{
    unsigned w = hub_here ();

    hy_bank (ahb + k / 3);
    if (put) {
        memcpy (CHUNK_AT (k), &cb, sizeof cb);
    } else {
        memcpy (&cb, CHUNK_AT (k), sizeof cb);
    }
    hub_back (w);
}

/* The one being grown ahead, in the hub's world record (WREC->growing: none, used 0) */
static struct cslot gk;
static void __fastcall__ growing (unsigned char on)
{
    gk.used = on;
    gk.sn = nsn;
    gk.cx = ncx;
    gk.cy = ncy;
    hub_put (WORLD_BANK, &WREC->growing, &gk, sizeof gk);
}

unsigned char __fastcall__ make_ahead (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k;

    if (setjmp (ajb)) {
        ahead_back ();                          /* (Given up: the calls it was in gone) */
        growing (0);
        hub_map (0);
        return 0;
    }
    ahead_mark ();
    growing (1);
    aheading = 1;
    ahead_check ();                             /* (A line or a request come as it was chosen: given way at once) */
    chunk_edges (sn, cx, cy, &me);
    grow (sn, cx, cy);
    aheading = 0;
    k = (k = ahead_room ()) != 0xFF && ahead_keep (k);
    growing (0);
    return k;
}


