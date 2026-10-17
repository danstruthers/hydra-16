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
** It's worked out in a grid of its own (the hub's LOOK_BANK): the squares round the character copied in, a chunk's
** rows of a column of chunks at a time (what each is, its floor, the light layer's light on it, a shut door: cast.s's
** run_in, light_in, grid_put); the lines cast there (cast.s, in assembly); then the squares seen marked in the chunks
** (the last look's unmarked first, as the look begins) once they're all known: only the squares each row's marks
** reach (cast.s keeps them: g_lo, g_hi), the last look's and this one's.  The grid's kept from one look to the next,
** while nothing it holds has changed (the light layer's lgen: a light or a square changed near it, a lamp's lines
** worked out in it): a turn where it stands looks in the same grid, its marks taken out; a step or a walk moves it,
** only the squares come into it copied in.  (The light layer works its lamps' lines out in the same grid, before the
** look's: grid ()'s lmode, the chunks the cache has alone.)
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
#define G_OPAQUE    0x01                        /* (cast.s's flags: the eye doesn't see past it; a door or a window, */
#define G_DOOR      0x20                        /*   run_in's mark in piece, taken out here) */
#define VMAX        45                          /* (The view's squares each way, at most: 2 (4 + 2 * 9) + 1) */

extern unsigned char g_r, g_m, g_hx, g_hy, g_bright, g_nl, g_li[NLAMPS], g_lj[NLAMPS], g_lr[NLAMPS];
extern unsigned char g_lzl[NLAMPS], g_lzh[NLAMPS], g_lo[VMAX], g_hi[VMAX];
extern unsigned g_r2, g_l2, g_u2;
extern int g_ez;
extern unsigned char* run_p, * run_g;
extern signed char* run_q;
void __fastcall__ cast_row (unsigned char j);
void cast_own (void);
unsigned char __fastcall__ run_in (unsigned char n);
void __fastcall__ run_out (unsigned char n);
void __fastcall__ run_clear (unsigned char n);
void __fastcall__ grid_put (unsigned char n);
void __fastcall__ grid_fill (unsigned char n);
void __fastcall__ grid_clr (unsigned char n);
extern unsigned char piece[256];                /* (things.c's buffer in main RAM: a row of the grid; run_in's flags,
                                                **   then its floors from 128) */
extern unsigned char* run_l;                    /* (cast.s's light_in: the light layer's squares into a run's) */
extern unsigned char run_lo, run_lb;
void __fastcall__ light_in (unsigned char n);
extern unsigned char at_bank;                   /* (chunk.c's: the chunk at () found last, its slot's bank and place) */
extern struct chunk* at_chunk;

struct lamp lamp[NLAMPS];
unsigned char nlamps;

static unsigned char vs;                        /* The stratum */
static unsigned bx0, by0;                       /* The last view's squares: from bx0, by0, bw each way (0: none), */
static unsigned char bw, blo[VMAX], bhi[VMAX];  /*   each row's marks from blo to bhi (0xFF: none) */
static unsigned char lmode;                     /* (grid's: for the light layer, its lamps' lines) */
static unsigned char lbk = 0xFF;                /* (run's: the chunk its light's grid is of, the cache's slot: its */
static struct chunk* lck;                       /*   bank and place, as at () found it; the grid) */
static unsigned char* lgp;
static unsigned gx_, gy_;                       /* The grid the view made last (gw_ 0: none, or not the view's now): */
static unsigned char gw_, gsn_, glg_;           /*   from gx_, gy_, gw_ each way, of stratum gsn_, the light layer's */
static unsigned gsx0, gsy0, gsw, gsh;           /*   lgen then, the stratum's bounds */

void give_way (void);                           /* (dq0p.c's: the look giving way to the UI's command in hand) */


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

/* The grid's rows j0 ... (wj of them), its columns from i0 (wi of them): stratum vs's squares from x, y, a column of
** chunks at a time, a chunk's rows at a time (at () once, the rows CW apart in it): what each is, and its floor
** (run_in); a shut door or shutters (but a portcullis's); the light layer's light (light.c's grid of the chunk), but
** for its own lines (lmode: a chunk the cache hasn't made yet as nothing, none made for them); outside the stratum,
** nothing */
static void __fastcall__ rect (unsigned x, unsigned y, unsigned char i0, unsigned char j0, unsigned char wi,
    unsigned char wj)
{
    static unsigned xx, yy, x1, oy;
    static unsigned char j, j1, n, k, r, a, c;
    static unsigned char* p;
    static struct portal* d;

    x1 = x + wi - 1;
    j1 = j0 + wj;
    for (xx = x, c = 2 * i0; xx <= x1; xx += n, c += 2 * n) {
        give_way ();
        n = band (xx, x1);
        for (j = j0, yy = y; j < j1; j += k, yy += k) {
            oy = yy - strata[vs].y0;            /* (The rows left in its chunk, in the stratum or out of it) */
            k = CW - (oy & (CW - 1));
            if (oy < strata[vs].h && strata[vs].h - oy < k) {
                k = strata[vs].h - oy;
            }
            if (k > j1 - j) {
                k = j1 - j;
            }
            if ((lmode && !cached (vs, xx, yy)) || !(p = at (vs, xx, yy))) {
                hub_map (LOOK_BANK);
                for (r = 0; r < k; ++r) {
                    run_g = GRID + ((unsigned) (j + r) << 7) + c;
                    grid_fill (n);
                }
                continue;
            }
            if (!lmode && (at_bank != lbk || at_chunk != lck)) {    /* (Its chunk's light: found once a chunk, each
                                                **   look) */
                lbk = at_bank;
                lck = at_chunk;
                lgp = light_grid (vs, xx, yy, 0);
            }
            for (r = 0; r < k; ++r, p += CW) {
                hub_map (at_bank);
                run_p = p;
                run_q = (signed char*) p + CW * CW;     /* (Its floors: struct chunk's elev, after its tile) */
                if (run_in (n)) {
                    for (a = 0; a < n; ++a) {
                        if (piece[a] & G_DOOR) {
                            piece[a] &= ~G_DOOR;
                            if ((d = portal_at (cx_ + a, cy_ + r)) != 0 && !(d->state & PS_OPEN) &&
                                d->kind != PK_PORTCULLIS) {
                                piece[a] |= G_OPAQUE;
                            }
                        }
                    }
                }
                if (!lmode && lgp) {
                    run_l = lgp + ((cy_ + r) << 4) + (cx_ >> 1);
                    run_lo = cx_ & 1;
                    light_in (n);
                }
                hub_map (LOOK_BANK);
                run_g = GRID + ((unsigned) (j + r) << 7) + c;
                grid_put (n);
            }
        }
    }
}

/* The grid made (LOOK_BANK's, cast.s's) of stratum sn's squares from gx, gy, w each way (lm: the light layer's, for
** its lamps' lines: the view's grid not kept) */
void __fastcall__ s_grid (unsigned char sn, unsigned gx, unsigned gy, unsigned char w, unsigned char lm)
{
    vs = sn;
    lmode = lm;
    if (lm) {
        ++lgen;
    }
    rect (gx, gy, 0, 0, w, w);
}

/* The view's grid of stratum vs from gx, gy, w each way: the last one kept if nothing in it has changed (its
** stratum, its size, the light layer's lgen, the stratum's bounds), the last view's marks taken out of it (blo, bhi),
** moved by the steps since (the squares come into it made), else made afresh */
static void __fastcall__ view_grid (unsigned gx, unsigned gy, unsigned char w)
{
    static struct stratum* s;
    static int dx, dy;
    static unsigned char j, ax, ay, f, t;

    s = &strata[vs];
    lmode = 0;
    dx = gx - gx_;
    dy = gy - gy_;
    ax = dx < 0 ? -dx : dx;
    ay = dy < 0 ? -dy : dy;
    if (gw_ == w && gsn_ == vs && glg_ == lgen && gsx0 == s->x0 && gsy0 == s->y0 && gsw == s->w && gsh == s->h &&
        (unsigned) dx + w < 2 * w && (unsigned) dy + w < 2 * w) {
        hub_map (LOOK_BANK);                    /* (The last view's marks out: its squares the grid's) */
        for (j = 0; j < bw; ++j) {
            if (blo[j] != 0xFF) {
                run_g = GRID + ((unsigned) j << 7) + 2 * blo[j];
                grid_clr (bhi[j] - blo[j] + 1);
            }
        }
        if (dx || dy) {                         /* (Moved: the rows it keeps, then the squares come in) */
            f = dx > 0 ? 2 * ax : 0;
            t = dx < 0 ? 2 * ax : 0;
            if (dy >= 0) {
                for (j = 0; j + ay < w; ++j) {
                    memmove (GRID + ((unsigned) j << 7) + t, GRID + ((unsigned) (j + ay) << 7) + f, 2 * (w - ax));
                }
                if (ay) {
                    rect (gx, gy + w - ay, 0, w - ay, w, ay);
                }
                j = 0;
            } else {
                for (j = w; j-- > ay; ) {
                    memmove (GRID + ((unsigned) j << 7) + t, GRID + ((unsigned) (j - ay) << 7) + f, 2 * (w - ax));
                }
                rect (gx, gy, 0, 0, w, ay);
                j = ay;
            }
            if (ax) {
                rect (dx > 0 ? gx + w - ax : gx, gy + j, dx > 0 ? w - ax : 0, j, ax, w - ay);
            }
        }
    } else {
        rect (gx, gy, 0, 0, w, w);
    }
    gx_ = gx;
    gy_ = gy;
    gw_ = w;
    gsn_ = vs;
    glg_ = lgen;
    gsx0 = s->x0;
    gsy0 = s->y0;
    gsw = s->w;
    gsh = s->h;
}

/* Row y's squares from x (n of them) of stratum vs, a chunk's at a time: no longer seen now (unmark), or the grid's
** marks put in them (from its column i of its row j) */
static void __fastcall__ marks (unsigned x, unsigned y, unsigned char n, unsigned char i, unsigned char j,
    unsigned char unmark)
{
    static unsigned char k;

    for (; n; n -= k, x += k, i += k) {
        k = band (x, x + n - 1);
        if (!unmark) {
            hub_map (LOOK_BANK);
            memcpy (piece, GRID + ((unsigned) j << 7) + 2 * i, 2 * k);
        }
        if ((run_p = at (vs, x, y)) != 0) {
            run_g = piece;
            if (unmark) {
                run_clear (k);
            } else {
                run_out (k);
            }
        }
    }
}

void __fastcall__ s_view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    static unsigned char range, own, j, k, w, m;
    static unsigned yy, gy0, d, xx, gx0, gx1;
    static int z;
    static struct lamp* l;

    own = at (sn, x, y) != 0;
    z = (own ? C->elev[cy_][cx_] : 0) + EYE;
    for (j = 0; j < bw; ++j) {                  /* The last view's seen now unmarked (its rows' marks): this one's are
                                                **   marked after */
        if (blo[j] != 0xFF) {
            give_way ();
            marks (bx0 + blo[j], by0 + j, bhi[j] - blo[j] + 1, 0, 0, 1);
        }
    }
    vs = sn;
    if (sight == 0xFF) {                        /* (Blinded: the grid's marks the last view's, not kept) */
        bw = gw_ = 0;
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
            give_way ();
            light_grid (sn, xx, yy, 1);
        }
    }
    lbk = 0xFF;
    view_grid (gx0, gy0, w);
    g_m = g_nl = 0;
    memset (g_lo, 0xFF, w);                     /* (Each row's marks, cast.s's as it makes them: none yet) */
    memset (g_hi, 0, w);
    if (own) {
        cast_own ();                            /* (Its own square: seen if anything lights it, else remembered) */
    }
    for (j = 0, k = 2 * range; j <= k; ++j) {  /* The lines cast, a row at a time (giving way to the UI's command
                                                **   between) */
        give_way ();
        hub_map (LOOK_BANK);
        cast_row (j);
    }
    bx0 = gx0;                                  /* Seen: marked in the chunks (each row's marks) */
    by0 = gy0;
    bw = w;
    memcpy (blo, g_lo, w);
    memcpy (bhi, g_hi, w);
    for (j = 0; j < w; ++j) {
        if (blo[j] != 0xFF) {
            give_way ();
            marks (bx0 + blo[j], by0 + j, bhi[j] - blo[j] + 1, blo[j], j, 0);
        }
    }
}
