/*
** view.c - (dq0w) what the character sees (DUNGEON.md), from where it stands and the way it faces (one of the
** compass's eight: the half of the world that way): the squares in front of it and to its sides (none behind, but
** for the ones it's next to), that a line from its eyes (five feet above its floor) reaches without a wall or rock
** in the way, nor a floor between that rises above the line; within its sight's range; and lit, by a room's own
** light, by the light it carries, or by a lit thing lying about (a torch put down: things.c's lamps).  Each is
** marked T_VIS (seen now: the perception record's) and T_SEEN (remembered: the map).  A wall or a door beside a
** room's floor that's seen is seen too (a line along a wall the character stands by would run through it).  The
** range is the referee's (its summary's sight, 0-9, with whatever the character's state does to it); blinded, it
** sees nothing.  In the dark its night vision carries a light further (the light's reach, times it), and its
** ultravision sees without light to its range (the summary's, both).
**
** It's worked out in a grid of its own (the hub's LOOK_BANK): the squares round the character (and the lamps' reach
** more, for their lines) copied in a row at a time, a chunk's run of them at once (what each is, its floor, a lit
** room's light on it, a shut door); the lines cast there (cast.s, in assembly); then the squares seen marked in the
** chunks (the last look's unmarked) once they're all known.  Between the rows, the senses answer what the UI asks
** that wants no look (look_pause: dq0p.c's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"

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

struct lamp lamp[NLAMPS];
unsigned char nlamps;

static unsigned char vs;                        /* The stratum */
static unsigned bx0, by0, bx1, by1;             /* The squares the last view looked at (bx1 0: none) */
static unsigned gx0, gx1;                       /* (The grid's: the view's and the lamps' more) */
static struct chunk* lc;                        /* (run's lit rooms: the chunk they're its, 0 each look) */


/* The squares from x (n of them, in one column of chunks: the stratum's chunks are counted from its x0) at row y,
** into piece (two bytes a square): what each is, and its floor (run_in); a lit room's light (its floor and its
** walls); a shut door or shutters (but a portcullis's); outside the stratum, nothing */
static void __fastcall__ run (unsigned x, unsigned y, unsigned char n)
{
    static unsigned char k, a, b, doors, nl, lx;
    static unsigned char ly0[NROOMS], ly1[NROOMS], la[NROOMS], lb[NROOMS];
    static unsigned char* p;
    static struct room* r;
    static struct portal* d;

    if (!(run_p = p = at (vs, x, y))) {
        for (k = 0; k < n; ++k) {
            piece[2 * k] = G_OPAQUE | G_SKIP;
            piece[2 * k + 1] = 0;
        }
        return;
    }
    run_q = &C->elev[cy_][cx_];
    run_g = piece;
    doors = run_in (n);
    if (C != lc || cx_ != lx) {                 /* (The lit rooms' squares in the run, its chunk's: found once) */
        lc = C;
        lx = cx_;
        for (k = C->nrooms, r = C->room, nl = 0; k; --k, ++r) {
            if ((r->flags & R_LIT) && r->x + r->w >= cx_) {
                ly0[nl] = r->y ? r->y - 1 : 0;
                ly1[nl] = r->y + r->h;
                la[nl] = r->x > cx_ + 1 ? r->x - 1 - cx_ : 0;
                lb[nl++] = r->x + r->w - cx_;
            }
        }
    }
    for (k = 0; k < nl; ++k) {
        if (cy_ >= ly0[k] && cy_ <= ly1[k]) {
            for (a = la[k], b = lb[k]; a <= b && a < n; ++a) {
                piece[2 * a] |= G_LIT;
            }
        }
    }
    for (k = 0; doors && k < n; ++k) {
        a = TILE (p[k]);
        if ((a == T_DOOR || a == T_WINDOW) && (d = portal_at (cx_ + k, cy_)) != 0 && !(d->state & PS_OPEN) &&
            d->kind != PK_PORTCULLIS) {
            piece[2 * k] |= G_OPAQUE;
        }
    }
}

/* The squares in the chunk's column from x, to x1 at most: how many */
static unsigned char __fastcall__ band (unsigned x, unsigned x1)
{
    unsigned char n = CW - ((x - strata[vs].x0) & (CW - 1));

    return n > x1 - x + 1 ? x1 - x + 1 : n;
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

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark)
{
    static unsigned char range, own, j, k, w, n;
    static unsigned yy, gy0, d, c, xx;
    static int z;
    static struct lamp* l;

    lc = 0;
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
    g_m = g_nl = 0;                             /* The lamps whose reach comes into the view: the grid that much more
                                                **   each way, for their lines (31 squares each way at most) */
    for (k = 0, l = lamp; k < nlamps; ++k, ++l) {
        d = range + l->r;
        if (l->x - x + d <= 2 * d && l->y - y + d <= 2 * d && l->r > g_m) {
            g_m = l->r < 31 - range ? l->r : 31 - range;
        }
    }
    w = 2 * (range + g_m) + 1;
    gx0 = x - range - g_m;                      /* (The strata are far from the frame's edges: no wrapping) */
    gx1 = gx0 + w - 1;
    gy0 = y - range - g_m;
    for (k = 0, l = lamp; k < nlamps; ++k, ++l) {
        if (l->x - gx0 < w && l->y - gy0 < w) {
            g_li[g_nl] = l->x - gx0;
            g_lj[g_nl] = l->y - gy0;
            g_lr[g_nl] = l->r;
            z = elev_at (sn, l->x, l->y) + 2;
            g_lzl[g_nl] = z;
            g_lzh[g_nl++] = z >> 8;
        }
    }
    for (yy = gy0; yy < gy0 + w; yy += CW - ((yy - strata[sn].y0) & (CW - 1))) {   /* (Its chunks reached in
                                                **   rows first, as they always were: any the maker must make made in
                                                **   the same order) */
        for (xx = gx0; xx <= gx1; xx += band (xx, gx1)) {
            at (sn, xx, yy);
        }
    }
    for (xx = gx0, c = 0; xx <= gx1; xx += n, c += 2 * n) {     /* The grid made, a column of chunks at a time */
        n = band (xx, gx1);
        for (j = 0, yy = gy0; j < w; ++j, ++yy) {
            run (xx, yy, n);
            hub_map (LOOK_BANK);
            memcpy (GRID + ((unsigned) j << 7) + c, piece, 2 * n);
        }
    }
    if (own) {
        cast_own ();                            /* (Its own square: seen if anything lights it, else remembered) */
    }
    for (j = 0, w = 2 * range; j <= w; ++j) {  /* The lines cast, a row at a time, the UI answered between */
        hub_map (LOOK_BANK);
        cast_row (j);
        if ((j & 3) == 3) {
            look_pause ();
        }
    }
    bx0 = x - range;                            /* Seen: marked in the chunks */
    by0 = y - range;
    bx1 = x + range;
    by1 = y + range;
    mark (0);
}
