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
** rect, in assembly); the lines cast there (cast.s, in assembly); then the squares seen marked in the chunks
** (the last look's unmarked first, as the look begins) once they're all known: only the squares each row's marks
** reach (cast.s keeps them: g_lo, g_hi; its mark_rows puts them in), the last look's and this one's.  The grid's kept from one look to the next,
** while nothing it holds has changed (the light layer's lgen: a light or a square changed near it, a lamp's lines
** worked out in it): a turn where it stands looks in the same grid, its marks taken out; a step or a walk moves it,
** only the squares come into it copied in.  (The light layer works its lamps' lines out in the same grid, before the
** look's: grid ()'s lmode, the chunks the cache has alone.)
**
** A look one way (HOUSE.md's looking one way: dq0p.c's v_way) is the same look, reaching twice as far (WAY_R squares
** at most, the grid's) in the quarter ahead of the character (within 45 degrees of the way it faces: cast.s's), as far
** as the light allows: a glance's reach every way else.
**
** And what the character keeps of it (DUNGEON.md, the map kept): each chunk it sees any of, its memory (struct
** chunk's kept) made fresh after the view (part 4's: kept); mapping as it goes on paper, what it sees drawn as the
** marks go in (T_DRAWN: cast.s's run_out, run_d).
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
#define VMAX        (2 * WAY_R + 1)             /* (The view's squares each way, at most: a look one way's; a glance's
                                                **   2 (4 + 2 * 9) + 1) */

extern unsigned char g_r, g_m, g_hx, g_hy, g_bright, g_lmax, g_nl, g_li[NLAMPS], g_lj[NLAMPS], g_lr[NLAMPS];
extern unsigned char g_lzl[NLAMPS], g_lzh[NLAMPS], g_lo[VMAX], g_hi[VMAX];
extern unsigned g_r2, g_l2, g_u2, g_o2;
extern int g_ez;
extern unsigned char* run_g;
void __fastcall__ cast_row (unsigned char j);
void cast_own (void);
void __fastcall__ grid_clr (unsigned char n);
extern unsigned char v_sn, v_look, v_lmode, r_i0, r_j0, r_wi, r_wj, lbk, bw, blo[VMAX], bhi[VMAX], sh_w;
extern unsigned v_x0, v_y0, v_h, r_x, r_y, bx0, by0;
extern signed char sh_dx, sh_dy;
void rect (void);                               /* (cast.s's: the grid made, moved, its marks put in the chunks) */
void grid_shift (void);
void __fastcall__ mark_rows (unsigned char unmark);
unsigned char __fastcall__ band (unsigned x, unsigned x1);
unsigned char run_d;                            /* (cast.s's run_out's: T_DRAWN, a square seen now drawn on the map
                                                **   as the marks go in, mapping on paper; or 0) */

struct lamp lamp[NLAMPS];
unsigned char nlamps;

static unsigned gx_, gy_;                       /* The grid the view made last (gw_ 0: none, or not the view's now): */
static unsigned char gw_, gsn_, glg_;           /*   from gx_, gy_, gw_ each way, of stratum gsn_, the light layer's */
static unsigned gsx0, gsy0, gsw, gsh;           /*   lgen then, the stratum's bounds */

void give_way (void);                           /* (dq0p.c's: the look giving way to the UI's command in hand) */


/* The squares a walk went (the world's: from x, y its first way d, n squares; the look's walk), felt in the dark,
** remembered (T_SEEN): as dq0w.c's walk goes them, along a passage (a wide one's breadth beside it no way on), on at
** its bends, through a door, into a room (part 2's: its breadth, chunk.c's, there); a walk in the dark (d | 0x80) as it
** goes there, by what the character knew: straight on, round a bend only where the way ahead's closed.  A walk in
** the light, mapping as it goes on paper, drawn too (the view's run_d: the look's view first, dq0p.c's) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")
void __fastcall__ s_felt (unsigned char sn, unsigned x, unsigned y, unsigned char d, unsigned char n)
{
    unsigned char* p;
    unsigned char h0 = d, cross, m = d & 0x80 ? T_SEEN : T_SEEN | run_d;    /* (h0: in the dark, | 0x80) */

    d &= 3;
    way_room = 0xFF;
    cross = !(h0 & 0x80) && room_of (sn, x, y) == 0xFF && ways_from (sn, x, y, d) > 1 ? 3 : 0;   /* (As the walk does:
                                                **   dq0w.c's) */
    while (n--) {
        x += dir_x[d];
        y += dir_y[d];
        if (!(p = at (sn, x, y))) {
            return;
        }
        *p |= m;
        if (!n || TILE (*p) == T_DOOR || ((h0 & 0x80) && step_ok (sn, x, y, d))) {
            continue;
        }
        if (cross && (--cross, step_ok (sn, x, y, h0))) {
            d = h0;                             /* (A fork it set out from crossed the way said) */
        } else if (ways_from (sn, x, y, d) == 1) {
            d = way_on;                         /* (On: the one way that isn't back, the walk's) */
        }
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* Stratum sn the view's (and the grid's: v_sn), its bounds and the grid's bank for cast.s */
static void __fastcall__ stratum (unsigned char sn)
{
    static struct stratum* s;

    s = &STRATUM (sn);
    v_sn = sn;
    v_x0 = s->x0;
    v_y0 = s->y0;
    v_h = s->h;
    v_look = LOOK_BANK;
}

/* The grid made (LOOK_BANK's, cast.s's) of stratum sn's squares from gx, gy, w each way (lm: the light layer's, for
** its lamps' lines: the view's grid not kept) */
void __fastcall__ s_grid (unsigned char sn, unsigned gx, unsigned gy, unsigned char w, unsigned char lm)
{
    stratum (sn);
    v_lmode = lm;
    if (lm) {
        ++lgen;
    }
    r_x = gx;
    r_y = gy;
    r_i0 = r_j0 = 0;
    r_wi = r_wj = w;
    rect ();
}

/* The view's grid of stratum v_sn from gx, gy, w each way: the last one kept if nothing in it has changed (its
** stratum, its size, the light layer's lgen, the stratum's bounds), the last view's marks taken out of it (blo, bhi),
** moved by the steps since (the squares come into it made), else made afresh */
static void __fastcall__ view_grid (unsigned gx, unsigned gy, unsigned char w)
{
    static struct stratum* s;
    static int dx, dy;
    static unsigned char j, ax, ay;

    s = &STRATUM (v_sn);
    v_lmode = 0;
    dx = gx - gx_;
    dy = gy - gy_;
    r_x = gx;
    r_y = gy;
    r_i0 = r_j0 = 0;
    r_wi = r_wj = w;
    if (gw_ == w && gsn_ == v_sn && glg_ == lgen && gsx0 == s->x0 && gsy0 == s->y0 && gsw == s->w && gsh == s->h &&
        dx > -w && dx < w && dy > -w && dy < w) {
        hub_map (LOOK_BANK);                    /* (The last view's marks out: its squares the grid's) */
        for (j = 0; j < bw; ++j) {
            if (blo[j] != 0xFF) {
                run_g = GRID + ((unsigned) j << 7) + 2 * blo[j];
                grid_clr (bhi[j] - blo[j] + 1);
            }
        }
        if (dx || dy) {                         /* (Moved: the squares it keeps, then those come in) */
            sh_dx = dx;
            sh_dy = dy;
            sh_w = w;
            grid_shift ();
            ax = dx < 0 ? -dx : dx;
            ay = dy < 0 ? -dy : dy;
            j = 0;
            if (ay) {
                if (dy > 0) {
                    r_y = gy + w - ay;
                    r_j0 = w - ay;
                } else {
                    j = ay;
                }
                r_wj = ay;
                rect ();
            }
            if (ax) {
                if (dx > 0) {
                    r_x = gx + w - ax;
                    r_i0 = w - ax;
                }
                r_y = gy + j;
                r_j0 = j;
                r_wi = ax;
                r_wj = w - ay;
                rect ();
            }
        }
    } else {
        rect ();
    }
    gx_ = gx;
    gy_ = gy;
    gw_ = w;
    gsn_ = v_sn;
    glg_ = lgen;
    gsx0 = s->x0;
    gsy0 = s->y0;
    gsw = s->w;
    gsh = s->h;
}

/* ==== Part 4 (its own, small: a look's, after the view's, dq0p.c's) */
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* The character's memory of this look's squares (the last view's: bx0, by0, bw, blo, bhi), each chunk with any of
** them in it: a passing more if it's come to see it again (KEPT_PASS or more since it last did: 255 at most); how
** well it keeps it made fresh (the summary's mapmind, mapping as it goes, half that not, a quarter of mapmind more for
** each of its passings' points, TR_MORE's; 100 at most; never less than it remembers of it still: a tenth lost each
** KEPT_FADE); and its squares drawn on its map as the marks went in (run_d's), the chunk's drawn squares another
** map's wiped but those (seen now), how true they're drawn kept (the referee's grade, the north believed in then) */
void s_kept (void)
{
    unsigned now, age, map, u, u0;              /* (On the C stack: the main RAM's room) */
    unsigned char q, q4, i0, i1, j, j0, j1, f, left, dsk;
    signed char dq;
    struct ckept* k;
    unsigned char* p;

    hub_map (0);
    now = KEPT_CLOCK;
    q4 = HUB->pcsum.mapmind;
    q = HUB->watch & WA_MAP ? q4 : q4 >> 1;
    q4 >>= 2;
    u0 = KEPT_UNIT (HUB->pcsum.mapfade);
    map = HUB->pc_map;
    dq = HUB->pc_drawn;
    dsk = HUB->pc_skew;
    for (j0 = 0; j0 < bw; j0 = j1) {            /* (Each chunk's rows of the view, and its columns) */
        j1 = j0 + CW - ((by0 + j0 - v_y0) & (CW - 1));
        j1 = j1 < bw ? j1 : bw;
        for (i0 = 0; i0 < bw; i0 = i1) {
            i1 = i0 + CW - ((bx0 + i0 - v_x0) & (CW - 1));
            i1 = i1 < bw ? i1 : bw;
            for (j = j0; j < j1 && (blo[j] >= i1 || bhi[j] < i0); ++j) {
            }
            if (j == j1 || !at (v_sn, bx0 + i0, by0 + j)) {
                continue;                       /* (None of its squares seen now) */
            }
            k = &C->kept;
            age = now - k->m;
            if ((!k->q || age >= KEPT_PASS) && k->n < 255) {
                ++k->n;
            }
            f = q;                              /* (Fresh: its passings' points, TR_MORE's, a quarter of mapmind
                                                **   each, and its fading the slower, twice for each) */
            u = u0;
            if (k->n >= 4) {
                f += q4;
                u <<= 1;
            }
            if (k->n >= 12) {
                f += q4;
                u <<= 1;
            }
            f = f < 100 ? f : 100;
            for (left = k->q; age >= u && left; age -= u) {
                left = left > 10 ? left - 10 : 0;   /* (What it remembers of it still: a tenth lost each fade) */
            }
            k->q = left > f ? left : f;
            k->m = now;
            if (run_d && k->map != map) {
                k->map = map;                   /* (First drawn on this map: how true, by the north believed in) */
                k->dq = dq;
                k->dsk = dsk;
                for (p = &C->tile[0][0], age = 0; age < CW * CW; ++age, ++p) {
                    if (!(*p & T_VIS)) {
                        *p &= ~T_DRAWN;
                    }
                }
            }
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

void __fastcall__ s_view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    static unsigned char range, own, j, k, w, m;
    static unsigned yy, gy0, d, xx, gx0, gx1;
    static int z;
    static struct lamp* l;

    own = at (sn, x, y) != 0;
    z = (own ? C->elev[cy_][cx_] : 0) + EYE;
    run_d = 0;
    mark_rows (1);                              /* The last view's seen now unmarked (its rows' marks): this one's are
                                                **   marked after */
    stratum (sn);
    if (sight == 0xFF) {                        /* (Blinded: the grid's marks the last view's, not kept) */
        bw = gw_ = 0;
        return;
    }
    range = 4 + 2 * (sight > 9 ? 9 : sight);
    g_o2 = range * range;                       /* (A glance's reach; a look one way's twice it, in the quarter ahead) */
    if (v_way) {
        range = 2 * range < WAY_R ? 2 * range : WAY_R;
    }
    g_r = range;
    g_r2 = range * range;
    g_l2 = light * light;
    g_u2 = dark * dark;
    g_hx = head_x[face];
    g_hy = head_y[face];
    g_ez = z;
    g_bright = light || dark;
    g_lmax = light > dark ? light : dark;
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
    for (yy = gy0; yy < gy0 + w; yy += CW - ((yy - v_y0) & (CW - 1))) {
        for (xx = gx0; xx <= gx1; xx += band (xx, gx1)) {
            at (sn, xx, yy);
        }
    }
    w = 2 * range + 1;                          /* The view's squares: their chunks' light (the layer's, worked out
                                                **   if it isn't yet), then the grid */
    gx0 = x - range;
    gx1 = gx0 + w - 1;
    gy0 = y - range;
    for (yy = gy0; yy < gy0 + w; yy += CW - ((yy - v_y0) & (CW - 1))) {
        for (xx = gx0; xx <= gx1; xx += band (xx, gx1)) {
            give_way ();
            light_grid (sn, xx, yy, 1);
        }
    }
    lbk = 0xFF;
    stratum (sn);                               /* (The light layer's lamps' lines may have had the grid) */
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
    run_d = own && (GRID[((unsigned) range << 7) + (range << 1)] & T_VIS) ? T_DRAWN : 0;     /* (Its own square
                                                **   lit: light to draw by) */
    hub_map (0);
    if (!(HUB->watch & WA_MAP) || !(HUB->pc_chart & CH_DRAW) || !HUB->pc_map) {
        run_d = 0;                              /* (Not mapping as it goes on paper: nothing drawn) */
    }
    mark_rows (0);
}
