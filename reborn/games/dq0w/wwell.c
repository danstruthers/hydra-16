/*
** wwell.c - (dq0w) a well's shaft (the user's, 2026-11-06, DECISIONS.md's climbing down a well: "A real shaft"; HOUSE.md's
** a well's shaft, for review): a well's curb (KI_WELL, the maker's: dq0m/uses.c) is the head of a narrow shaft down to
** water, its foot on the stratum below under it.  A shaft as any other: a link of the stratum's both ends (world.h's
** LD_WELL, as a dug one's: its head no change of its square, the curb's there; its foot a T_UP, the change records'),
** made as it's first wanted (climbed down, a rope let down it, a thing put in it: the stratum below grown round it if
** no one's been there), from the stratum's seed and its square alone, so the same seed and the same commands make the
** same well.  Its foot in the rock a sump of its water, over a man's head (one well in four a cistern of nine squares
** about it); one in two of those and one in three of the rest with a way on, a passage up out of the water to the halls
** nearest along one of the four ways, the water standing at their floor; in the halls (a room's floor, a passage's) a
** pool two feet deep where it comes out in the roof; anything else under it (a wall, a stair), choked.  The climb is any
** shaft's (dq0w.c's parts 89 and 97: the rope's help, a slip, a fall), a fall into the water at its foot (the book's
** landing in water), the words its own (here, and the UI's E_ARRIVE O_WELL).
**
** All of it swapped part 103's (../dq0/swap.inc: dq0w.cfg's SW103), in dq0w.rop's last overlay (things.c's ropes_in: the
** caller reads it in first), through its stub, well (WL_*).  What it keeps across the stubs it calls (change, its
** records; stratum_in, grow_below, it_need) is in the things' request's copy (things.c's iq), the fields a rope's drop
** uses (wrope.h's R_*: the same meanings, the curb's square and its strata), the main RAM's at no cost.
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "../dq0/rs.h"
#include "world.h"
#include "work.h"
#include "things.h"

#pragma local-strings (on)
#pragma code-name ("SW103CODE")
#pragma rodata-name ("SW103RODATA")
#pragma bss-name ("SW103RODATA")               /* (Its statics its own, good only within a call that calls no stub) */

void __fastcall__ grow_below (unsigned char n);     /* (Part 3's) */
void __fastcall__ put (const char* s);          /* (tlist.s's: the things' words, after those so far) */
unsigned __fastcall__ at_hand (const char* name);   /* (Part 4's) */
unsigned __fastcall__ seen_named (const char* name);
extern struct ireq iq;                          /* (things.c's) */

#define W_X             iq.x                    /* (The curb's square: wrope.h's R_TX, R_TY) */
#define W_Y             iq.y
#define W_PX            iq.dur                  /* (A square of its foot: R_FX, R_FY) */
#define W_PY            iq.dur_max
#define W_SN            iq.craft                /* (Its stratum: R_TSN; the one below, R_FSN) */
#define W_B             iq.quality
#define W_K             iq.mat                  /* (Its link, the link up below) */
#define W_U             iq.plural
#define W_H             iq.size                 /* (A way on's heading: R_HOW after; its first square, R_HEAD) */
#define W_E             iq.thick

#define WELL_DEEP       6                       /* (Feet of water at its foot in the rock, at least: over a man's head,
                                                **   the swimmer's SWIM_FT; and up to seven more, by its seed) */
#define WELL_POOL       2                       /* (Its pool's, come out in the roof of the halls below) */
#define WELL_RAMP       2                       /* (A way on's rise a square, out of the water: dq0w.c's STEP_MAX) */
#define WELL_REACH      12                      /* (Squares a way on is looked for along each way) */

/* Its numbers by the stratum below's seed and the curb's square (k: which) */
static unsigned char __fastcall__ wh (unsigned char k)
{
    static unsigned v;

    v = (unsigned) STRATUM (W_B).seed ^ W_X * 73 ^ W_Y * 151 ^ k * 2531;
    v = v * 31421 + 6927;
    v ^= v >> 7;
    return (unsigned char) (v * 25173 >> 8);
}

/* The well within reach of the character (on its square or beside it): its item (its square W_X, W_Y; its stratum W_SN,
** the one below W_B), or 0 */
static unsigned near_well (void)
{
    static unsigned i, px, py;

    hub_map (0);
    W_B = (W_SN = HUB->pc_stratum) + 1;
    px = HUB->pc_x;
    py = HUB->pc_y;
    i = 0;
    while ((i = items (i, IN_SQUARE, 0)) != 0) {
        if (ip->kind == KI_WELL && ip->sn == W_SN && ip->x - px + 1 <= 2 && ip->y - py + 1 <= 2) {
            W_X = ip->x;
            W_Y = ip->y;
            break;
        }
    }
    hub_map (0);
    return i;
}

/* A link added to stratum sn's record (to: the other), a well's, the window as sync_strata leaves it: its index, or 0xFF */
static unsigned char __fastcall__ wl_link (unsigned char sn, unsigned char kind, unsigned char to)
{
    static struct stratum* s;
    static struct link* l;
    static unsigned char k;

    hub_map (WORLD_BANK);
    s = &WREC->strata[SLOT (sn)];
    k = 0xFF;
    if (s->nlinks < NLINKS) {
        l = &s->link[k = s->nlinks++];
        l->kind = kind;
        l->to = to;
        l->x = W_X;
        l->y = W_Y;
        l->dug = LD_WELL;
    }
    sync_strata ();
    return k;
}

/* Square W_PX, W_PY of the foot made (its kind, its floor; its link; under the water, surface s: its water's after it) */
static void __fastcall__ wl_sq (unsigned char t, signed char f, unsigned char u, signed char s)
{
    change (W_B, W_PX, W_PY, t, f, u);
    if (f < s) {
        change (W_B, W_PX, W_PY, (unsigned char) s, 0, WELL_WET);
    }
}

/* Square W_PX, W_PY: m squares from the foot heading W_H; its square's kind, TILE's (0xFF outside the stratum below) */
static unsigned char __fastcall__ wl_at (unsigned char m)
{
    static unsigned char* p;

    W_PX = W_X + head_x[W_H] * m;
    W_PY = W_Y + head_y[W_H] * m;
    return (p = at (W_B, W_PX, W_PY)) != 0 ? TILE (*p) : 0xFF;
}

/* A way on from the foot heading W_H (the four's), its first square W_E squares off (a cistern's edge before it): all rock
** to the halls along it, or to a room's wall with its floor behind it (the wall broken through, as a tunnel dug through
** one is), two squares of it or more past the foot's own (WELL_REACH at most): so many, the halls' floor in *z; or 0 */
static unsigned char __fastcall__ wl_way (signed char* z)
{
    static unsigned char m, t;

    for (m = 1; m < W_E + WELL_REACH; ++m) {
        if ((t = wl_at (m)) == T_WALL && ((t = wl_at (m + 1)) == T_FLOOR || t == T_CORR)) {
            ++m;                                /* (Through a room's wall, or a passage's, into it) */
        }
        if (t == T_FLOOR || t == T_CORR) {
            *z = C->elev[cy_][cx_];
            return m >= W_E + 2 ? m - W_E : 0;
        }
        if (t != T_ROCK) {
            return 0;                           /* (Outside, a door, a stair, walls: not broken into) */
        }
    }
    return 0;
}

/* A test's scene, the environment's dq0well (rc's): its numbers' first, h, as it says ("0" a sump, "1" a cistern, "2" a sump
** with a way on, "3" a cistern with one: wsaid 1); none wsaid, h as it was */
static unsigned char wsaid;
static unsigned char __fastcall__ scene (unsigned char h)
{
    static struct hy_regs ar;
    static char v[4];

    ar.a = 0xFF;
    ar.r[0] = (unsigned) "dq0well";
    ar.r[1] = (unsigned) v;
    ar.r[2] = 3;
    ar.r[3] = 0;
    if ((wsaid = !hy_call (HY_ENV_GET, &ar)) != 0) {
        h = (h & 0xC7) | (v[0] & 1 ? 0x18 : 0) | (v[0] & 2 ? 0x20 : 0);
    }
    return h;
}

/* The well's foot on stratum W_B under the curb (W_U its link up there) */
static void foot (void)
{
    unsigned char h, n, k, d, cis;              /* (On the C stack: change, a stub, copies the part back) */
    signed char s, z, f;

    W_H = 0;
    k = wl_at (0);
    if (k == T_FLOOR || k == T_CORR) {          /* (Into the halls: a pool where it comes out in the roof) */
        s = C->elev[cy_][cx_];
        wl_sq (T_UP, s - WELL_POOL, W_U, s);
        return;
    }
    if (k != T_ROCK) {
        return;                                 /* (Choked) */
    }
    h = scene (wh (0));
    d = WELL_DEEP + (h & 7);
    cis = (h & 0x18) == 0x18;                   /* (One in four a cistern) */
    W_E = cis ? 2 : 1;
    s = k = 0;                                  /* (Its water's surface: the datum, or a way on's floor) */
    if ((cis || wsaid) ? (h & 0x20) != 0 : wh (1) % 3 == 0) {
        for (n = 0; n < 4 && !k; ++n) {         /* (The four ways, from one by its seed) */
            W_H = (h >> 6) + n << 1 & 6;
            if ((k = wl_way (&z)) != 0) {
                s = z;
                if (d > k * WELL_RAMP) {
                    d = k * WELL_RAMP;          /* (A short way on: the water the shallower) */
                }
            }
        }
    }
    h = W_H;
    f = s - d;
    wl_at (0);
    wl_sq (T_UP, f, W_U, s);
    for (n = 0; cis && n < 8; ++n) {            /* (A cistern: the eight about it under the same water) */
        W_H = n;
        if (wl_at (1) == T_ROCK) {
            wl_sq (T_CORR, f, 0xFF, s);
        }
    }
    W_H = h;
    for (n = 0; n < k; ++n) {                   /* (Up out of the water, a rise of WELL_RAMP a square, to the halls' floor) */
        z = f + (n + 1) * WELL_RAMP;
        wl_at (W_E + n);
        wl_sq (T_CORR, z < s ? z : s, 0xFF, s);
    }
}

/* Its foot there, a T_UP (the stratum below in its slot): 1, or 0 (choked) */
static unsigned char footed (void)
{
    W_H = 0;
    return stratum_in (W_B) && wl_at (0) == T_UP;
}

/* The shaft of the well at the curb W_X, W_Y on stratum W_SN: its link there (made, the first time: the stratum below grown
** round it if no one's been there, its foot made), or 0xFF: none to climb (the stratum below not under it, no room for a
** link, the deepest, or its foot choked) */
static unsigned char shaft_of (void)
{
    const struct stratum* s = &STRATUM (W_SN);
    const struct link* l;
    unsigned char k;

    for (k = 0, l = s->link; k < s->nlinks; ++k, ++l) {
        if (l->x == W_X && l->y == W_Y && l->dug == LD_WELL) {
            return footed () ? k : 0xFF;
        }
    }
    hub_map (0);
    if (W_SN == 0xFE || (W_B >= MAXSTRATA && !HUB->savedir[0])) {
        return 0xFF;                            /* (Past the hub's strata's slots, none to put out without a folder) */
    }
    s = &STRATUM (W_B);
    if (stratum_in (W_B) && (W_X - s->x0 >= s->w || W_Y - s->y0 >= s->h)) {
        return 0xFF;                            /* (The stratum below doesn't reach under it) */
    }
    if ((W_K = wl_link (W_SN, O_SHAFT_DOWN, W_B)) == 0xFF) {
        return 0xFF;
    }
    W_U = 0xFF;
    if (stratum_in (W_B)) {
        W_U = wl_link (W_B, O_SHAFT_UP, W_SN);
    } else {
        grow_below (W_SN);                      /* (Grown round it: a link up under each link down, this one's) */
        if (stratum_in (W_B)) {
            for (k = s->nlinks, l = s->link; k--; ++l) {
                if (l->x == W_X && l->y == W_Y) {
                    W_U = l - s->link;
                }
            }
        }
    }
    if (W_U != 0xFF) {
        foot ();
    }
    return footed () ? W_K : 0xFF;
}

/* (Its stub's: WL_DOWN the shaft of the well within reach, to climb down: its link, 0xFF none there, 0xFE choked (told);
** WL_ROPE a rope's drop down it, the R_*'s (wrope.h's: 2, or 0 none); WL_NEAR, WL_SHAFT, WL_FOOT part 104's: near_well's,
** shaft_of's, footed's) */
unsigned char __fastcall__ s_well (unsigned w)
{
    static struct msg m;
    unsigned char k;

    switch (w) {                                /* (wwell2.c's, part 104's: a thing let fall down it) */
    case WL_NEAR:
        return near_well () != 0;
    case WL_SHAFT:
        return shaft_of ();
    case WL_FOOT:
        return footed ();
    }
    if (!near_well ()) {
        return w ? 0 : 0xFF;
    }
    k = shaft_of ();
    hub_map (0);                                /* (The hub's header in the window, as its callers want it) */
    if (w) {                                    /* (WL_ROPE: R_TSN, R_FSN its strata, R_TX, R_TY its square, already) */
        W_PX = W_X;                             /* (R_FX, R_FY) */
        W_PY = W_Y;
        W_H = RK_SHAFT;                         /* (R_HOW) */
        W_E = 0;                                /* (R_HEAD) */
        return k == 0xFF ? 0 : 2;
    }
    if (k == 0xFF) {                            /* (Told as the traces' words are, E_VIEW's V_TRACED: the UI bank's trtext) */
        hub_map (UI_BANK);
        strcpy (UIREC->trtext, "Far down, the well's shaft narrows to a crack the water seeps through: there's no climbing down it.");
        hub_map (0);
        m.kind = E_VIEW;
        m.w1 = V_TRACED;
        m.t = HUB->now;
        hub_event (&m);
        return 0xFE;
    }
    return k;
}
