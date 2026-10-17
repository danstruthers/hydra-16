/*
** view.c - (dq0w) what the character sees (DUNGEON.md), from where it stands and the way it faces (one of the
** compass's eight: the half of the world that way): the squares in front of it and to its sides (none behind, but
** for the ones it's next to), that a line from its eyes (five feet above its floor) reaches without a wall or rock
** in the way, nor a floor between that rises above the line; within its sight's range; and lit, by the light layer
** (light.c: a room's own light, a lit thing lying about, a torch put down) or by the light it carries.  Each is
** marked T_VIS (seen now: the perception record's) and T_SEEN (remembered: the map).  A wall or a door beside a
** room's floor that's seen is seen too (a line along a wall the character stands by would run through it).  The
** range is the referee's (its summary's sight, 0-9, with whatever the character's state does to it); blinded, it
** sees nothing.  In the dark its night vision carries a light further (the light's reach, times it), and its
** ultravision sees without light to its range (the summary's, both).
**
** It's worked out in a grid of its own (the hub's LOOK_BANK): the squares round the character copied in a row at a
** time, a chunk's run of them at once (what each is, its floor, the light layer's light on it, a shut door); the
** lines cast there (cast.s, in assembly); then the squares seen marked in the chunks (the last look's unmarked) once
** they're all known.  (The light layer works its lamps' lines out in the same grid, before the look's: grid ()'s
** lmode, the chunks the cache has alone.)
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "light.h"

#pragma code-name ("SW1CODE")                   /* (Part 1's, all of it: swapped, bank.s's stubs, once a look) */
#pragma rodata-name ("SW1RODATA")

#define EYE         5                           /* The eyes, feet above the floor */
#define GRID        ((unsigned char*) 0x8000)   /* The grid (cast.s's): a row 128 bytes, a square two (its flags, its
                                                **   floor) */
#define G_OPAQUE    0x01                        /* (cast.s's flags: the eye doesn't see past it, nothing to see on it, */
#define G_SKIP      0x02                        /*   a room's own light reaches it) */
#define G_LIT       0x04

extern unsigned char g_r, g_m, g_hx, g_hy, g_bright, g_nl, g_li[NLAMPS], g_lj[NLAMPS], g_lr[NLAMPS];
extern unsigned char g_lzl[NLAMPS], g_lzh[NLAMPS];
extern unsigned g_r2, g_l2, g_u2;
extern int g_ez;
extern unsigned char* run_p, * run_g;
extern signed char* run_q;
void __fastcall__ cast_row (unsigned char j);
void cast_own (void);
unsigned char __fastcall__ run_in (unsigned char n);
void __fastcall__ run_out (unsigned char n);
void __fastcall__ run_clear (unsigned char n);
extern unsigned char piece[256];                /* (things.c's buffer in main RAM: a row of the grid) */
extern unsigned char* run_l;                    /* (cast.s's light_in: the light layer's squares into a run's) */
extern unsigned char run_lo, run_lb;
void __fastcall__ light_in (unsigned char n);
extern unsigned char at_bank;                   /* (chunk.c's: the chunk at () found last, its slot's bank and place) */
extern struct chunk* at_chunk;

struct lamp lamp[NLAMPS];
unsigned char nlamps;

static unsigned char vs;                        /* The stratum */
static unsigned bx0, by0, bx1, by1;             /* The squares the last view looked at (bx1 0: none) */
static unsigned gx0, gx1;                       /* (The grid's: the view's) */
static unsigned char lmode;                     /* (grid's: for the light layer, its lamps' lines) */
static unsigned char lbk = 0xFF;                /* (run's: the chunk its light's grid is of, the cache's slot: its */
static struct chunk* lck;                       /*   bank and place, as at () found it; the grid) */
static unsigned char* lgp;

void give_way (void);                           /* (dq0p.c's: the look giving way to the UI's command in hand) */
extern unsigned char gw;


/* The squares from x (n of them, in one column of chunks: the stratum's chunks are counted from its x0) at row y,
** into piece (two bytes a square): what each is, and its floor (run_in); a shut door or shutters (but a
** portcullis's); the light layer's light (light.c's grid of the chunk), but for its own lines (lmode: a chunk the
** cache hasn't made yet as nothing, none made for them); outside the stratum, nothing */
static void __fastcall__ run (unsigned x, unsigned y, unsigned char n)
{
    static unsigned char k, a, doors;
    static unsigned char* p;
    static struct portal* d;

    if ((lmode && !cached (vs, x, y)) || !(run_p = p = at (vs, x, y))) {
        for (k = 0; k < n; ++k) {
            piece[2 * k] = G_OPAQUE | G_SKIP;
            piece[2 * k + 1] = 0;
        }
        return;
    }
    run_q = &C->elev[cy_][cx_];
    run_g = piece;
    doors = run_in (n);
    for (k = 0; doors && k < n; ++k) {
        a = TILE (p[k]);
        if ((a == T_DOOR || a == T_WINDOW) && (d = portal_at (cx_ + k, cy_)) != 0 && !(d->state & PS_OPEN) &&
            d->kind != PK_PORTCULLIS) {
            piece[2 * k] |= G_OPAQUE;
        }
    }
    if (lmode) {
        return;
    }
    if (at_bank != lbk || at_chunk != lck) {    /* (Its chunk's grid: found once a chunk, each look) */
        lbk = at_bank;
        lck = at_chunk;
        lgp = light_grid (vs, x, y, 0);
    }
    if (lgp) {
        run_l = lgp + (cy_ << 4) + (cx_ >> 1);
        run_lo = cx_ & 1;
        light_in (n);
    }
}

/* The squares a walk went (the world's: from x, y its first way d, n squares; the look's walk), felt in the dark,
** remembered (T_SEEN): as dq0w.c's walk goes them, along a passage, on at its bends, through a door, into a room */
void __fastcall__ s_felt (unsigned char sn, unsigned x, unsigned y, unsigned char d, unsigned char n)
{
    unsigned char* p;
    unsigned char k, w;

    while (n--) {
        x += dir_x[d];
        y += dir_y[d];
        if (!(p = at (sn, x, y))) {
            return;
        }
        *p |= T_SEEN;
        if (!n || TILE (*p) == T_DOOR) {
            continue;
        }
        for (k = 0, w = d; k < 4; ++k) {        /* (On: the one way that isn't back, the walk's) */
            if (k != ((d + 2) & 3) && step_ok (sn, x, y, k)) {
                w = k;
            }
        }
        d = w;
    }
}

/* The squares in the chunk's column from x, to x1 at most: how many */
static unsigned char __fastcall__ band (unsigned x, unsigned x1)
{
    unsigned char n = CW - ((x - strata[vs].x0) & (CW - 1));

    return n > x1 - x + 1 ? x1 - x + 1 : n;
}

/* The grid made (LOOK_BANK's, cast.s's) of stratum sn's squares from gx, gy, w each way, a column of chunks at a
** time (lm: the light layer's, for its lamps' lines) */
void __fastcall__ s_grid (unsigned char sn, unsigned gx, unsigned gy, unsigned char w, unsigned char lm)
{
    static unsigned xx, yy, c, g1;
    static unsigned char j, n;

    vs = sn;
    lmode = lm;
    g1 = gx + w - 1;
    for (xx = gx, c = 0; xx <= g1; xx += n, c += 2 * n) {
        give_way ();
        n = band (xx, g1);
        for (j = 0, yy = gy; j < w; ++j, ++yy) {
            run (xx, yy, n);
            hub_map (LOOK_BANK);
            memcpy (GRID + ((unsigned) j << 7) + c, piece, 2 * n);
        }
    }
}

/* The view's squares (bx0, by0)-(bx1, by1) of stratum vs, a column of chunks at a time (at ()'s quick way, the
** chunk it found last, for all of them but a couple): the grid's marks put in the chunks, or (unmark) seen now no
** longer */
static void __fastcall__ mark (unsigned char unmark)
{
    static unsigned x, y, c;
    static unsigned char n, m;

    for (x = bx0, c = 2 * g_m; x <= bx1; x += n, c += 2 * n) {
        n = band (x, bx1);
        for (y = by0, m = g_m; y <= by1; ++y, ++m) {
            if (!unmark) {
                hub_map (LOOK_BANK);
                memcpy (piece, GRID + ((unsigned) m << 7) + c, 2 * n);
            }
            if ((run_p = at (vs, x, y)) != 0) {
                run_g = piece;
                if (unmark) {
                    run_clear (n);
                } else {
                    run_out (n);
                }
            }
        }
    }
}

void __fastcall__ s_view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    static unsigned char range, own, j, k, w, m;
    static unsigned yy, gy0, d, xx;
    static int z;
    static struct lamp* l;

    gw = 0;
    own = at (sn, x, y) != 0;
    z = (own ? C->elev[cy_][cx_] : 0) + EYE;
    if (bx1) {                                  /* The last view's seen now unmarked: this one's are marked after */
        mark (1);
    }
    vs = sn;
    bx1 = 0;
    if (sight == 0xFF) {                        /* (Blinded) */
        return;
    }
    range = 4 + 2 * (sight > 9 ? 9 : sight);
    g_r = range;
    g_r2 = range * range;
    g_l2 = light * light;
    g_u2 = dark * dark;
    g_hx = head_x[face];
    g_hy = head_y[face];
    g_ez = z;
    g_bright = light || dark;
    for (k = 0, m = 0, l = lamp; k < nlamps; ++k, ++l) {    /* The lamps whose reach comes into the view: its chunks
                                                **   reached that much more each way (31 squares at most), as they
                                                **   always were, in rows first: any the maker must make made in the
                                                **   same order, the lamps' chunks with them, for the light layer */
        d = range + l->r;
        if (l->x - x + d <= 2 * d && l->y - y + d <= 2 * d && l->r > m) {
            m = l->r < 31 - range ? l->r : 31 - range;
        }
    }
    w = 2 * (range + m) + 1;
    gx0 = x - range - m;                        /* (The strata are far from the frame's edges: no wrapping) */
    gx1 = gx0 + w - 1;
    gy0 = y - range - m;
    for (yy = gy0; yy < gy0 + w; yy += CW - ((yy - strata[sn].y0) & (CW - 1))) {
        for (xx = gx0; xx <= gx1; xx += band (xx, gx1)) {
            at (sn, xx, yy);
        }
    }
    w = 2 * range + 1;                          /* The view's squares: their chunks' light (the layer's, worked out
                                                **   if it isn't yet), then the grid */
    gx0 = x - range;
    gx1 = gx0 + w - 1;
    gy0 = y - range;
    for (yy = gy0; yy < gy0 + w; yy += CW - ((yy - strata[sn].y0) & (CW - 1))) {
        for (xx = gx0; xx <= gx1; xx += band (xx, gx1)) {
            light_grid (sn, xx, yy, 1);
        }
    }
    lbk = 0xFF;
    s_grid (sn, gx0, gy0, w, 0);
    g_m = g_nl = 0;
    if (own) {
        cast_own ();                            /* (Its own square: seen if anything lights it, else remembered) */
    }
    for (j = 0, w = 2 * range; j <= w; ++j) {  /* The lines cast, a row at a time (giving way to the UI's command
                                                **   between) */
        give_way ();
        hub_map (LOOK_BANK);
        cast_row (j);
    }
    bx0 = x - range;                            /* Seen: marked in the chunks */
    by0 = y - range;
    bx1 = x + range;
    by1 = y + range;
    mark (0);
}
