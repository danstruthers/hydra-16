/*
** ahead.c - (dq0m) the chunks made ahead (the user's, 2026-11-17: the maker builds the map's pieces ahead of the
** character's eyes, in the background, so a look or a walk finds them made; the cost the cache's room): which to make
** (those the actors could see or reach next), where each goes (a slot the readers see as free), and the cache's slots
** that hold them.  The making's make.c's (make_ahead, finished by make_chunk as a reader first asks for one), the
** time's dq0m.c's (as nobody waits on the maker, giving way to a command in hand).
**
** Which: by one rule for every actor (ONE-PATH.md), every body in the dungeon (the hub's bodies: the character's,
** actor 0's, and each denizen's), each by its own reach: its chunk, and those round it whose nearest square is within
** its eyes' reach and the steps it may take meanwhile (AH_STEPS), or, in the quarter it faces, within twice its eyes'
** (a look one way's, dq0p's WAY_R at most) and those steps; the nearest first.  Its eyes' reach the senses' look's (4
** squares and 2 more for each point of its sight, up to 9: dq0p's view); a denizen's sight isn't in the hub, so the
** most a kind's (9).
**
** Where: the cache's room free to the readers, the last free slot (make_chunk's pick takes the first); else one whose
** chunk made ahead no actor wants now.  A chunk a reader has is never made way for by one: the cache's readers' chunks
** stay as they'd be without the making ahead (travel's rooms remembered, the lit rooms seen far off, the seen maps'
** saving, all as before); one made ahead is made way for as a reader's chunk wants its slot (moved to another free
** one, if there is one).
**
** Overlay 10's: the hub's banks reached through the main RAM's hub_get and hub_put (the window an overlay's), its tables
** the main RAM's (the cache's table's copy and the actors' in the stock list's room, idle but as a chunk's finished).
*/

#pragma code-name ("OV10CODE")
#pragma rodata-name ("OV10RODATA")

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

#define AH_STEPS        8
#define AH_WAY          31                      /* (dq0p's WAY_R: a look one way's reach, at most) */

/* The chunks made ahead, in the maker's own banks (the user's, 2026-11-18: room of its own, so a full cache doesn't stop
** the making ahead and no chunk a reader has is made way for): each own slot's key (its chunk's place), and its state
** (AH_MADE, its style, AH_ROCK); what its plan of uses left (us) kept in its chunk's kept, till it's finished (make.c's);
** each as it changes into the hub's world record (WREC->ahead: the senses' to read) */
struct akey ah_k[NAHEAD];
unsigned char ah_f[NAHEAD];
unsigned char ahb, nah;                         /* (Its own banks' first, and their slots: three a bank) */
unsigned char ah_ver;                           /* (The cache's chunks changed, counted: the choice looked at again) */

/* The cache's table and its readers' slots, as last copied (look_cache's); the actors as last looked at (actors's): in
** the stock list's room (stock.c's sk, idle but as a chunk's finished, make.c's finish: the main RAM's) */
struct act {
    unsigned char   sn, face, r, f;             /* (Its stratum, its facing; its reach, and its reach ahead) */
    unsigned        x, y;
};
struct aroom {
    struct cslot    cs[NCSLOTS];
    unsigned char   inu[NASK];
    struct act      an[1 + NACTORS];
};
typedef char aroom_fits[sizeof (struct aroom) <= sizeof (struct stock) ? 1 : -1];
#define AR              ((struct aroom*) &sk)
#define cs              AR->cs
#define inu             AR->inu
#define an              AR->an
static unsigned char nan, seen_sum, seen_ver;   /* (As the choice last found none: the actors' sum, the cache's count) */
static unsigned char pc0;                       /* (The character's body in the dungeon: an[0] its) */
static unsigned char held;                      /* (The choice given way: each's, dq0m.c's ahead_held) */
unsigned char ahead_held (void);
void __fastcall__ hub_set (unsigned char bank, void* to, unsigned char c, unsigned n);   /* (make.c's) */
unsigned char nsn;                              /* (The next to make: ahead_next's) */
unsigned ncx, ncy;

static void look_cache (void)
{
    hub_get (WORLD_BANK, WREC->cslot, cs, sizeof cs);
    hub_get (WORLD_BANK, WREC->inuse, inu, sizeof inu);
}

/* Own slot k's state into the hub's record of them */
static struct cslot pk;
static void __fastcall__ publish (unsigned char k)
{
    pk.used = ah_f[k];
    pk.sn = ah_k[k].sn;
    pk.cx = ah_k[k].cx;
    pk.cy = ah_k[k].cy;
    hub_put (WORLD_BANK, &WREC->ahead[k], &pk, sizeof pk);
}

/* The maker's own banks for them, as it starts: as many as it gets, up to AH_BANKS (none: nothing made ahead) */
void o_ahead_init (void)
{
    unsigned char n;
    int b = -1;

    for (n = AH_BANKS; n && (b = hy_banks_alloc (n)) < 0; --n) {
    }
    ahb = b;
    nah = n * 3;
    hub_set (WORLD_BANK, WREC->ahead, 0, sizeof WREC->ahead + sizeof WREC->growing);
}

/* The own slot chunk sn, cx, cy was made ahead in, or 0xFF */
unsigned char __fastcall__ o_ahead_at (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k;

    for (k = 0; k < nah; ++k) {
        if ((ah_f[k] & AH_MADE) && ah_k[k].sn == sn && ah_k[k].cx == cx && ah_k[k].cy == cy) {
            return k;
        }
    }
    return 0xFF;
}

/* The slot chunk sn, cx, cy is in, a reader's (the cache's table copied), or 0xFF */
unsigned char __fastcall__ o_ahead_found (unsigned char sn, unsigned cx, unsigned cy)
{
    unsigned char k;

    look_cache ();
    for (k = 0; k < NCSLOTS; ++k) {
        if (cs[k].used && cs[k].sn == sn && cs[k].cx == cx && cs[k].cy == cy) {
            return k;
        }
    }
    return 0xFF;
}

/* Slot k's chunk made ahead let go */
void __fastcall__ o_ahead_drop (unsigned char k)
{
    if (ah_f[k]) {
        ah_f[k] = 0;
        ++ah_ver;
        publish (k);
    }
}

/* Every one let go (a stratum's plan grown, or read back: dq0m.c's serve) */
void o_ahead_none (void)
{
    memset (ah_f, 0, sizeof ah_f);
    ++ah_ver;
    hub_set (WORLD_BANK, WREC->ahead, 0, sizeof WREC->ahead);
}

/* Own slot j's chunk made ahead taken to be finished (make.c's make_chunk) into the cache's slot k: its bytes into cb;
** its rock (its edges' x1), its style (sty again), what its plan of uses left back (from its kept, then nothing there);
** its own slot free */
void __fastcall__ o_ahead_take (unsigned char j, unsigned char k)
{
    unsigned char g;

    (void) k;
    own_copy (j, 0);
    me.x1 = ah_f[j] & AH_ROCK ? 0xFF : 0;
    cstyle = ah_f[j] & AH_STYLE;
    hub_get (0, &HUB->gear, &g, 1);
    uses (US_STYLE, cstyle | g << 2);
    memcpy (&us, &cb.kept, sizeof us);
    memset (&cb.kept, 0, sizeof cb.kept);
    SP = &sty;
    o_ahead_drop (j);
}

/* The chunk grown ahead (cb: nsn, ncx, ncy) into own slot k (ahead_room's), what its plan left in its kept: 1 */
unsigned char __fastcall__ o_ahead_keep (unsigned char k)
{
    memcpy (&cb.kept, &us, sizeof us);
    own_copy (k, 1);
    ah_k[k].sn = nsn;
    ah_k[k].cx = ncx;
    ah_k[k].cy = ncy;
    ah_f[k] = AH_MADE | (me.x1 == 0xFF ? AH_ROCK : 0) | cstyle;
    ++ah_ver;
    publish (k);
    return 1;
}

/* ---- Which */

static void __fastcall__ reach (struct act* p, unsigned char sight)
{
    unsigned char e = 4 + 2 * (sight > 9 ? 9 : sight);

    p->r = e + AH_STEPS;
    p->f = (2 * e < AH_WAY ? 2 * e : AH_WAY) + AH_STEPS;
}

/* The actors as they are now (the hub's bodies, by who: the character's, actor 0's, first), into an; their sum (a
** change seen: a stratum, a facing, a block of sixteen squares each way, half a chunk; a step within one looked at as
** the next change comes: the chooser's cost each step of every denizen's else, a twentieth of the game's time at the
** normal pace) */
static unsigned char actors (void)
{
    static struct { unsigned char state, sn, face; } h;
    struct act* p = an;
    unsigned char k, s;

    pc0 = 0;
    for (k = 0; k <= NACTORS; ++k) {
        hub_get (0, &HUB->bodies[k].state, &h, 3);
        if (h.state & 1) {                      /* (A body in the dungeon) */
            p->sn = h.sn;
            p->face = h.face & 7;
            hub_get (0, &HUB->bodies[k].x, &p->x, 4);
            s = 9;                              /* (A denizen's sight isn't in the hub: the most a kind's) */
            if (!k) {
                hub_get (0, &HUB->pcsum.sight, &s, 1);
                pc0 = 1;
            }
            reach (p++, s);
        }
    }
    nan = p - an;
    for (s = nan, p = an; p < an + nan; ++p) {
        s = (s << 1 | s >> 7) ^ p->sn ^ p->face;
        s = (s << 1 | s >> 7) ^ (unsigned char) (p->x >> 4);
        s = (s << 1 | s >> 7) ^ (unsigned char) (p->y >> 4);
    }
    return s;
}

/* Chunk sn, cx, cy in the cache (the readers') or made ahead (cs the cache's table as last copied) */
static unsigned char __fastcall__ made (unsigned char sn, unsigned cx, unsigned cy)
{
    struct cslot* c = cs;
    unsigned char k;

    for (k = NCSLOTS; k; --k, ++c) {
        if (c->used && c->sn == sn && c->cx == cx && c->cy == cy) {
            return 1;
        }
    }
    return o_ahead_at (sn, cx, cy) != 0xFF;
}

/* Where an actor stands (its stratum made, it in its bounds: 1, else 0): its chunk, acx, acy; each way the squares from it
** to the nearest of the chunks two before its own to two after (adx[i], ady[i]: i 2 its own, 0; 0xFF past the
** stratum's bounds, or before them); and those chunks' way in the quarter it faces (ahx[i] + ahy[j] over 0) */
static unsigned acx, acy;
static unsigned char adx[5], ady[5];
static signed char ahx[5], ahy[5];
static void __fastcall__ side (unsigned char* d, signed char* a, signed char h, unsigned c, unsigned char r, unsigned w)
{
    unsigned char i;

    d[0] = c < 2 ? 0xFF : r + 1 + CW;
    d[1] = c < 1 ? 0xFF : r + 1;
    d[2] = 0;
    d[3] = (c + 1) * CW >= w ? 0xFF : CW - r;
    d[4] = (c + 2) * CW >= w ? 0xFF : 2 * CW - r;
    for (i = 0; i < 5; ++i) {
        a[i] = h > 0 ? i - 2 : h < 0 ? 2 - i : 0;
    }
}

static unsigned char __fastcall__ stands (const struct act* p)
{
    struct stratum* s = &STRATUM (p->sn);
    unsigned ox = p->x - s->x0, oy = p->y - s->y0;

    if (s->n != p->sn || !s->made || ox >= s->w || oy >= s->h) {
        return 0;
    }
    acx = ox / CW;
    acy = oy / CW;
    side (adx, ahx, head_x[p->face], acx, (unsigned char) ox & (CW - 1), s->w);
    side (ady, ahy, head_y[p->face], acy, (unsigned char) oy & (CW - 1), s->h);
    return 1;
}

/* Chunk i - 2, j - 2 from the actor's (stands') within its reach: its way off (its nearest square's, the more of the two
** ways; half that, one in the quarter it faces: what a look one way sees, the sooner), else 0xFF */
static unsigned char __fastcall__ within (const struct act* p, unsigned char i, unsigned char j)
{
    unsigned char d = adx[i] > ady[j] ? adx[i] : ady[j];

    if (d == 0xFF) {
        return 0xFF;
    }
    if ((signed char) (ahx[i] + ahy[j]) > 0) {
        return d <= p->f ? d >> 1 : 0xFF;
    }
    return d <= p->r ? d : 0xFF;
}

/* Actor p one of the denizens before it over again (its stratum, its facing, its square: a lair's, often; their
** reach the same) */
static unsigned char __fastcall__ again (const struct act* p)
{
    const struct act* q;

    for (q = an + 1; q < p; ++q) {
        if (q->sn == p->sn && q->face == p->face && q->x == p->x && q->y == p->y) {
            return 1;
        }
    }
    return 0;
}

/* Each chunk an actor could see or reach next: w called with it and its way off (within's); 1 as w says 1 (stop) */
typedef unsigned char __fastcall__ (*eachf) (const struct act* p, unsigned cx, unsigned cy, unsigned char d);
static unsigned char __fastcall__ each (eachf w)
{
    struct act* p;
    unsigned char i, j, d;

    for (p = an; p < an + nan; ++p) {
        if (ahead_held ()) {                    /* (A line or a command: given way, the choice left) */
            held = 1;
            return 1;
        }
        if ((p == an || !again (p)) && stands (p)) {
            for (j = 0; j < 5; ++j) {
                for (i = 0; i < 5; ++i) {
                    if ((d = within (p, i, j)) != 0xFF && w (p, acx + i - 2, acy + j - 2, d)) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* (each's: the nearest not made, into nsn ...) */
static unsigned char best;
static unsigned char __fastcall__ nearer (const struct act* p, unsigned cx, unsigned cy, unsigned char d)
{
    if (d < best && !made (p->sn, cx, cy)) {
        best = d;
        nsn = p->sn;
        ncx = cx;
        ncy = cy;
    }
    return 0;
}

/* Chunk sn, cx, cy one an actor could see or reach next (the actors as last looked at): each's, that chunk alone */
static unsigned char __fastcall__ wanted (unsigned char sn, unsigned cx, unsigned cy)
{
    struct act* p;
    unsigned i, j;

    for (p = an; p < an + nan; ++p) {
        if (p->sn == sn && stands (p) && (i = cx - acx + 2) < 5 && (j = cy - acy + 2) < 5 &&
            within (p, (unsigned char) i, (unsigned char) j) != 0xFF) {
            return 1;
        }
    }
    return 0;
}

/* The chunk being made ahead (nsn, ncx, ncy) one the character's own eyes want (the actors as last looked at, the
** character's first): 1, else 0 (dq0m.c's ahead_wanted: a finish asked for waits the grow's rest, not thrown away) */
unsigned char o_ahead_mine (void)
{
    return pc0 && an->sn == nsn && stands (an) && ncx - acx + 2 < 5 && ncy - acy + 2 < 5 &&
        within (an, (unsigned char) (ncx - acx + 2), (unsigned char) (ncy - acy + 2)) != 0xFF;
}

/* ---- Where */

/* The own slot a chunk made ahead goes in: a free one, else one whose chunk no actor wants now; or 0xFF (none: every
** one wanted; or no own banks).  The cache's slots never: a chunk a reader has is never made way for by one made
** ahead, and the cache is as it would be without them (what the readers find, the save's seen maps, all as before) */
unsigned char o_ahead_room (void)
{
    unsigned char k;

    for (k = nah; k--; ) {
        if (!ah_f[k]) {
            return k;
        }
    }
    for (k = nah; k--; ) {
        if (!wanted (ah_k[k].sn, ah_k[k].cx, ah_k[k].cy)) {
            return k;
        }
    }
    return 0xFF;
}

/* The chunk being made ahead (nsn, ncx, ncy) still the nearest wanted (the actors looked at again: dq0m.c's, as the
** character's moved): 1, else 0 (give it up, another first) */
unsigned char o_ahead_still (void)
{
    unsigned char sn = nsn;
    unsigned cx = ncx, cy = ncy;

    actors ();
    look_cache ();
    best = 0xFF;
    held = 0;
    each (nearer);
    if (held || best == 0xFF || (nsn == sn && ncx == cx && ncy == cy)) {
        nsn = sn;
        ncx = cx;
        ncy = cy;
        return 1;
    }
    return 0;
}

/* The next chunk to make ahead, if one's wanted and there's room for it: 1 (into nsn, ncx, ncy), else 0 (looked for
** again only as an actor moves or the cache changes); the strata's records copied then (a stratum grown, the cache's
** count's changed: ahead_none's) */
unsigned char o_ahead_next (void)
{
    unsigned char s;

    if (ahead_held ()) {
        return 0;                               /* (A line or a request: given way before the actors' copy) */
    }
    s = actors ();
    if (ahead_held ()) {
        return 0;                               /* (And before the strata's, some 25,000 cycles) */
    }
    if (s == seen_sum && ah_ver == seen_ver) {
        return 0;                               /* (Nothing changed: the wake's few thousand cycles, not the strata's
                                                **   copy too, some 25,000: a twentieth of the game's time asleep) */
    }
    hub_get (WORLD_BANK, WREC->strata, strata, sizeof strata);
    look_cache ();
    best = 0xFF;
    held = 0;
    each (nearer);
    if (held) {
        return 0;                               /* (Given way: chosen again, the sum not kept) */
    }
    if (best != 0xFF && o_ahead_room () != 0xFF) {
        return 1;
    }
    seen_sum = s;
    seen_ver = ah_ver;
    return 0;
}
