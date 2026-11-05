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
** look's: grid ()'s lmode, the chunks the cache has alone.)  A chunk not made yet is nothing in the grid, as outside
** the stratum, till a line from the eyes reaches its edge: made then, and the grid made again; a look stopped by a wall
** a few squares off makes nothing beyond it (a chunk made is the longest wait in the game, 5.5M cycles of the maker's:
** a look one way into new ground made every chunk its grid touched, up to nine, behind it as well, 2026-11-05).  A lamp
** in a chunk made shines into one that is, whichever came first (light.c's).
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
unsigned char __fastcall__ eye_edge (unsigned char n);
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
static unsigned gsx0, gsy0, gsw, gsh;           /*   lgen then, the stratum's bounds, the chunks made by then */
static unsigned char gmd_;

void give_way (void);                           /* (dq0p.c's: the look giving way to the UI's command in hand) */


extern unsigned char here_known;                /* (dq0p.c's: come to a square it knew before this look: sight.c's) */

/* The squares a walk went (the world's: from x, y its first way d, n squares; the look's walk), felt in the dark,
** remembered (T_SEEN): as dq0w.c's walk goes them, along a passage (a wide one's breadth beside it no way on), on at
** its bends, through a door, into a room (part 2's: its breadth, chunk.c's, there); a walk in the dark (d | 0x80) as it
** goes there, by what the character knew: straight on, round a bend only where the way ahead's closed.  A walk in
** the light, mapping as it goes on paper, drawn too (the view's run_d: the look's view first, dq0p.c's).  Part 4's
** (part 2's room for a passage's runs, sight.c's, 2026-11-01), with a copy of chunk.c's ways of its own (ways.inc's,
** renamed: the ways part 2's, its look's) */
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")
#pragma static-locals (push, on)
#pragma bss-name (push, "SW4RODATA")
#pragma data-name (push, "SW4RODATA")
#define breadth         breadth4
#define way_ok          way_ok4
#define ways_from       ways_from4
#define way_on          way_on4
#define way_room        way_room4
#include "../dq0w/ways.inc"
#pragma data-name (pop)
#pragma bss-name (pop)
#pragma static-locals (pop)
void __fastcall__ s_felt (unsigned char sn, unsigned x, unsigned y, unsigned char d, unsigned char n)
{
    unsigned char* p;
    unsigned char h0 = d, cross, m = d & 0x80 ? T_SEEN : T_SEEN | run_d;    /* (h0: in the dark, | 0x80) */

    here_known |= 0x80;                         /* (On a walk: sight.c's, no doors looked for along it till it arrives) */
    d &= 7;                                     /* (A heading: the diagonal ways' too) */
    way_room = 0xFF;
    cross = !(h0 & 0x80) && room_of (sn, x, y) == 0xFF && ways_from (sn, x, y, d) > 1 ? 3 : 0;   /* (As the walk does:
                                                **   dq0w.c's) */
    while (n--) {
        x += head_x[d];
        y += head_y[d];
        if (!(p = at (sn, x, y))) {
            return;
        }
        *p |= m;
        if (!n || TILE (*p) == T_DOOR || ((h0 & 0x80) && step8 (sn, x, y, d))) {
            continue;
        }
        if (cross && (--cross, step8 (sn, x, y, h0))) {
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
    static unsigned char j, ax, ay, md;

    hub_map (WORLD_BANK);
    md = WREC->made;                            /* (One made since, by anyone: nothing in the grid no longer so) */
    s = &STRATUM (v_sn);
    v_lmode = 0;
    dx = gx - gx_;
    dy = gy - gy_;
    r_x = gx;
    r_y = gy;
    r_i0 = r_j0 = 0;
    r_wi = r_wj = w;
    if (gw_ == w && gsn_ == v_sn && glg_ == lgen && gmd_ == md && gsx0 == s->x0 && gsy0 == s->y0 && gsw == s->w && gsh == s->h &&
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
    gmd_ = md;
}

/* The chunk not made at the view's squares i, j on (ni, nj of them): 1 if a line from the eyes reaches the edges of it
** that face them (the eyes r, r in the view) */
static unsigned char i_, j_, ni_, nj_, r_;
static unsigned char __fastcall__ reached (void)
{
    hub_map (LOOK_BANK);
    if (i_ > r_ || i_ + ni_ <= r_) {
        r_i0 = i_ > r_ ? i_ : i_ + ni_ - 1;
        r_j0 = j_;
        r_wi = 0;
        if (eye_edge (nj_)) {
            return 1;
        }
    }
    if (j_ > r_ || j_ + nj_ <= r_) {
        r_i0 = i_;
        r_j0 = j_ > r_ ? j_ : j_ + nj_ - 1;
        r_wi = 1;
        return eye_edge (ni_);
    }
    return 0;
}

/* ==== Part 4 (its own, small: a look's, after the view's, dq0p.c's) */
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* What it keeps of chunk C as it comes to know it again (fr: the clock, the fade, how well it keeps what it sees: its
** mapmind, mapping as it goes, half that not, and a quarter of it): a passing more if it's come to see it again
** (KEPT_PASS or more since it last did: 255 at most); how well it keeps it made fresh (fr's q, a quarter of mapmind
** more for each of its passings' points, TR_MORE's; 100 at most; never less than it remembers of it still: a tenth lost
** each KEPT_FADE); forgotten (under KEPT_GONE: HOUSE.md's travel, slice 4.2's), remembered afresh, what it saw of it
** before gone (its seen marks wiped but those it sees now, its rooms no longer visited; its drawing on paper kept);
** the north it believes in now kept with it (travel's: its way back off by how far its belief's gone since) */
extern unsigned char at_bank;                   /* (chunk.c's: at ()'s chunk's bank) */
struct fresh {
    unsigned        now, u0;
    unsigned char   q, q4, sk;
};
static void __fastcall__ fresh (const struct fresh* fr)
{
    struct ckept* k = &C->kept;
    unsigned age = fr->now - k->m, u = fr->u0, i;
    unsigned char f = fr->q, left;
    unsigned char* p;

    if ((!k->q || age >= KEPT_PASS) && k->n < 255) {
        ++k->n;
    }
    if (k->n >= 4) {                            /* (Fresh: its passings' points, TR_MORE's, a quarter of mapmind
                                                **   each, and its fading the slower, twice for each) */
        f += fr->q4;
        u <<= 1;
    }
    if (k->n >= 12) {
        f += fr->q4;
        u <<= 1;
    }
    f = f < 100 ? f : 100;
    for (left = k->q; age >= u && left; age -= u) {
        left = left > 10 ? left - 10 : 0;       /* (What it remembers of it still: a tenth lost each fade) */
    }
    if (k->q && left < KEPT_GONE) {
        for (p = &C->tile[0][0], i = 0; i < CW * CW; ++i, ++p) {
            if (!(*p & T_VIS)) {
                *p &= ~T_SEEN;                  /* (Forgotten: what it saw then gone) */
            }
        }
        for (i = 0; i < C->nrooms; ++i) {
            C->room[i].flags &= ~R_VISITED;
        }
    }
    k->q = left > f ? left : f;
    k->m = fr->now;
    k->sk = fr->sk;                             /* (The north it believes in as it knows it again) */
}

/* The character's memory of this look's squares (the last view's: bx0, by0, bw, blo, bhi), each chunk with any of
** them in it, made fresh (fresh's); and its squares drawn on its map as the marks went in (run_d's), the chunk's drawn
** squares another map's wiped but those (seen now), how true they're drawn kept (the referee's grade, the north
** believed in then).  And a walk's squares felt in the dark (the look's walk, wd | 0x80: its way felt's, s_felt's,
** before it marks them, its bends near enough), each chunk it went through made fresh the same (HOUSE.md's travel: what's felt kept as what's
** seen, slice 4.2's) */
/* A passage's ways' runs (../dq0/run.h's: the user's, 2026-10-31, ways off the compass; HOUSE.md's a passage's
** heading), for what the character perceives (sight.c's, part 2's: its room), as a look begins: from the character's
** square in a passage, each heading it can step (step8's) followed as a walk goes (its one way on, none behind: a wide
** passage's breadth a way, its run's end), through the squares
** seen now (T_VIS), to a door, a room, a fork, its end, or RUN_MAX steps: its run's bearing in sixteenths, by its
** first heading, into the senses' bank (RUNS) */
#include "../dq0/run.h"
typedef char runs_fit[sizeof (struct sight) <= RUN_AT ? 1 : -1];
#pragma bss-name (push, "SW4RODATA")            /* (Its state the part's own: none kept from one call to the next) */
static void runs (void)
{
    static unsigned char r[8], d, n, h, sn, k, w, nh;
    static unsigned x, y, x0, y0;
    static unsigned char* p;

    hub_map (0);
    sn = HUB->pc_stratum;
    x0 = HUB->pc_x;
    y0 = HUB->pc_y;
    if (room_of (sn, x0, y0) != 0xFF) {
        return;                                 /* (A room's: none) */
    }
    for (d = 0; d < 8; ++d) {
        r[d] = d << 1;
        if (!step8 (sn, x0, y0, d)) {
            continue;
        }
        x = x0;
        y = y0;
        h = d;
        for (n = 0; ; ) {
            run_s[n++] = h;
            x += head_x[h];
            y += head_y[h];
            if (n == RUN_MAX || !(p = at (sn, x, y)) || (*p & (T_VIS | 0x0F)) != (T_VIS | T_CORR)) {
                break;
            }
            for (k = w = 0; k < 8; ++k) {       /* (Its ways on, none behind: one, or the run's end) */
                if (!REL_BEHIND (k - h) && step8 (sn, x, y, k)) {
                    ++w;
                    nh = k;
                }
            }
            if (w != 1) {
                break;
            }
            h = nh;
        }
        r[d] = run_of (n);
    }
    hub_map (SIGHT_BANK);
    memcpy (RUNS, r, sizeof r);
    hub_map (0);
}
#pragma bss-name (pop)

static const unsigned char bend[4] = { 1, 7, 2, 6 };
void s_kept (void)
{
    unsigned age, map, x, y;                    /* (On the C stack: the main RAM's room) */
    unsigned char i0, i1, j, j0, j1, dsk, d, n, sn, lcx, lcy, lb;
    signed char dq;
    struct ckept* k;
    unsigned char* p;
    struct fresh fr;

    runs ();                                    /* (A passage's ways' runs, for sight.c's: part 2's room) */
    hub_map (0);
    fr.now = KEPT_CLOCK;
    fr.q4 = HUB->pcsum.mapmind;
    fr.q = HUB->watch & WA_MAP ? fr.q4 : fr.q4 >> 1;
    fr.q4 >>= 2;
    fr.u0 = KEPT_UNIT (HUB->pcsum.mapfade);
    fr.sk = HUB->pc_skew;
    map = HUB->pc_map;
    dq = HUB->pc_drawn;
    dsk = HUB->pc_skew;
    x = HUB->look.wx;
    y = HUB->look.wy;
    d = HUB->look.wd;
    n = HUB->look.wn;
    sn = HUB->pc_stratum;
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
            fresh (&fr);
            k = &C->kept;
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
    if (n && (d & 0x80)) {                      /* (A walk felt in the dark: its way as s_felt goes it) */
        d &= 7;
        lb = 0;
        while (n--) {
            x += head_x[d];
            y += head_y[d];
            if (!(p = at (sn, x, y))) {
                break;
            }
            if (!lb || at_bank != lb || (unsigned char) C->cx != lcx || (unsigned char) C->cy != lcy) {
                lb = at_bank;                   /* (A chunk come into: made fresh, once) */
                lcx = (unsigned char) C->cx;
                lcy = (unsigned char) C->cy;
                fresh (&fr);
            }
            if (TILE (*p) == T_DOOR || step8 (sn, x, y, d)) {
                continue;
            }
            for (i0 = 0; i0 < 4 && !step8 (sn, x, y, (d + bend[i0]) & 7); ++i0) {
            }                                   /* (Round a bend: the first way to the side, an eighth, then a
                                                **   quarter; ways_from's, near enough, the chunks it went through
                                                **   all that's wanted) */
            if (i0 == 4) {
                break;
            }
            d = (d + bend[i0]) & 7;
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

void __fastcall__ s_view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    static unsigned char range, own, j, k, w, m;
    static unsigned yy, gy0, gy1, xx, gx0, gx1;
    static int z;

    hub_map (0);
    k = HUB->pc_pos;                            /* (Its eyes by its body's position: crouched, kneeling, sitting three
                                                **   feet, prone one, HOUSE.md's, slice 4.6's) */
    j = HUB->look.first;
    own = at (sn, x, y) != 0;
    if ((j & LF_ARRIVE) && own) {
        here_known = (C->tile[cy_][cx_] & T_SEEN) != 0;
    }
    z = (own ? C->elev[cy_][cx_] : 0) + (k ? (k == POS_PRONE ? 1 : 3) : EYE);
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
    w = 2 * range + 1;                          /* The view's squares */
    gx0 = x - range;                            /* (The strata are far from the frame's edges: no wrapping) */
    gx1 = gx0 + w - 1;
    gy0 = y - range;
    gy1 = gy0 + w;
    r_ = range;
    g_m = g_nl = 0;
    do {
        for (yy = gy0; yy < gy1; yy += CW - ((yy - v_y0) & (CW - 1))) {
            for (xx = gx0; xx <= gx1; xx += band (xx, gx1)) {
                give_way ();                    /* Their chunks' light (the layer's, worked out if it isn't yet): those */
                if (cached (sn, xx, yy)) {      /*   made */
                    light_grid (sn, xx, yy, 1);
                }
            }
        }
        lbk = 0xFF;
        stratum (sn);                           /* (The light layer's lamps' lines may have had the grid) */
        view_grid (gx0, gy0, w);                /* The grid; then those not made that the eyes reach made, and it all again */
        m = 0;
        for (yy = gy0; yy < gy1; yy += nj_) {
            nj_ = CW - ((yy - v_y0) & (CW - 1));
            if (yy + nj_ > gy1) {
                nj_ = gy1 - yy;
            }
            j_ = yy - gy0;
            for (xx = gx0; xx <= gx1; xx += ni_) {
                ni_ = band (xx, gx1);
                i_ = xx - gx0;
                if (!cached (sn, xx, yy) && reached () && at (sn, xx, yy)) {
                    m = 1;
                }
            }
        }
    } while (m);
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
