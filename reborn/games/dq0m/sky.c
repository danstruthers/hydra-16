/*
** sky.c - (dq0m, overlay 9) what shows the sky (the user's, 2026-10-31 and 2026-11-03, "All three together": DECISIONS.md's;
** HOUSE.md's what shows the sky, for review): the shafts open to the sky in a stratum's links, as its plan's grown; a
** chunk's skylights and windows on the surface, as it's made.  By the seed alone, on dice of their own, after the
** plan's and the links', so nothing else moves for them; a chunk made again is the same, byte for byte (dq0pure).
**
** A shaft: the top stratum has one game in two, on a square of its halls in a chunk with no other way between strata
** in it, a link up to the surface (to 0xFF, LD_SKY); one in two of them goes on down (the link O_SHAFT_DOWN), to the
** stratum below, and on from there the same, through three strata at most, if the stratum has a way down of its own
** (so a stratum below it is grown, its bounds round those ways: the world's grow_below passes a shaft over, so the strata
** below are as they'd be without it).  Its foot, a link up (LD_SKY) as the stratum below's plan is grown here, if its
** square lies inside the bounds, off their edges, and in the stratum's halls; if not, it's choked a little way down
** (the world's s_climb says so).  Two of a stratum's links kept free for the shafts dug.
**
** A chunk's: in the top stratum's hewn rooms (a natural cave's none), one room in six a skylight, over the middle of its
** floor (T_UP as a way up's square is, so nothing's stocked there; O_SKYLIGHT, no link; the map's mark its own); one
** room in six a window on the surface (PP_SKY), in a wall with solid rock two squares deep behind it, a light well up
** to the day, its parts by its dice, and for those who look for the sky an O_WINDOW on the floor square beside it, its
** sill the portal's number.  None past NOPEN - 1 openings: one kept for a shaft dug.  A window anywhere else looks on a
** passage (fixtures.c's), and shows no sky.
**
** A test's scene (the environment's dq0sky, rc's): every room both, and a shaft always, beside the stratum's first
** way down, going on down.
**
** Its locals in the grower's stack's room (sy: idle now, built.c's), the code the less.
*/

#pragma code-name ("OV9CODE")
#pragma rodata-name ("OV9RODATA")
#pragma local-strings (on)

#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0w/world.h"
#include "grow.h"

extern unsigned char sy[];
struct sk {
    struct hy_regs  ar;
    char            val[4];
    struct stratum* s;
    struct link*    l;
    struct room*    p;
    struct portal*  q;
    struct opening* o;
    unsigned        x, y, h;
    unsigned char   i, t, f, down, d, k, lx, ly, ox, oy, len, b, sd;
};
#define Y   (*(struct sk*) sy)

/* The environment's dq0sky said */
static unsigned char forced (void)
{
    Y.ar.a = 0xFF;
    Y.ar.r[0] = (unsigned) "dq0sky";
    Y.ar.r[1] = (unsigned) Y.val;
    Y.ar.r[2] = 3;
    Y.ar.r[3] = 0;
    return !hy_call (HY_ENV_GET, &Y.ar);
}

/* Its dice: a little generator, from Y.h */
static unsigned char skd (void)
{
    Y.h ^= Y.h << 7;
    Y.h ^= Y.h >> 9;
    Y.h ^= Y.h << 8;
    return (unsigned char) (Y.h >> 4);
}

/* A link at Y.x, Y.y of Y.s */
static void __fastcall__ sky_link (unsigned char kind, unsigned char to)
{
    Y.l = &Y.s->link[Y.s->nlinks++];
    Y.l->kind = kind;
    Y.l->to = to;
    Y.l->x = Y.x;
    Y.l->y = Y.y;
    Y.l->dug = LD_SKY;
}

static void __fastcall__ sky_links (unsigned char n)
{
    Y.s = &STRATUM (n);
    Y.f = forced ();
    Y.x = 0;
    Y.down = 0xFF;
    for (Y.i = 0, Y.l = Y.s->link; Y.i < Y.s->nlinks; ++Y.i, ++Y.l) {
        if (Y.down == 0xFF && Y.l->to == n + 1) {
            Y.down = Y.i;                       /* (A way down of its own) */
        }
    }
    Y.h = ((unsigned) Y.s->seed ^ (unsigned) (Y.s->seed >> 16) ^ 0x5B17) | 1;
    skd ();
    skd ();
    if (n) {
        if (STRATUM (n - 1).n != n - 1 || !STRATUM (n - 1).made) {
            return;
        }
        for (Y.i = STRATUM (n - 1).nlinks, Y.l = STRATUM (n - 1).link; Y.i; --Y.i, ++Y.l) {
            if (Y.l->kind == O_SHAFT_DOWN && Y.l->dug == LD_SKY && Y.l->x - Y.s->x0 - 2 < Y.s->w - 4 &&
                Y.l->y - Y.s->y0 - 2 < Y.s->h - 4 &&
                (Y.f || rg (n, (Y.l->x - Y.s->x0) >> 8, (Y.l->y - Y.s->y0) >> 8)->kind == RG_HALLS)) {
                Y.x = Y.l->x;
                Y.y = Y.l->y;
            }
        }
    } else if (Y.f) {
        if (Y.down != 0xFF) {
            Y.x = Y.s->link[Y.down].x + 1;
            Y.y = Y.s->link[Y.down].y;
        }
    } else if (skd () & 1) {
        for (Y.t = 0; Y.t < 30 && !Y.x; ++Y.t) {
            Y.x = 2 + (skd () * 3U) % (Y.s->w - 4);
            Y.x = Y.s->x0 + INSET (Y.x);
            Y.y = 2 + (skd () * 3U) % (Y.s->h - 4);
            Y.y = Y.s->y0 + INSET (Y.y);
            if (rg (n, (Y.x - Y.s->x0) >> 8, (Y.y - Y.s->y0) >> 8)->kind != RG_HALLS) {
                Y.x = 0;
                continue;
            }
            for (Y.i = Y.s->nlinks, Y.l = Y.s->link; Y.i; --Y.i, ++Y.l) {
                if (((Y.l->x - Y.s->x0) ^ (Y.x - Y.s->x0)) < CW && ((Y.l->y - Y.s->y0) ^ (Y.y - Y.s->y0)) < CW) {
                    Y.x = 0;                    /* (A chunk of its own) */
                }
            }
        }
    }
    if (!Y.x || Y.s->nlinks > NLINKS - 4) {
        return;
    }
    sky_link (O_SHAFT_UP, n ? n - 1 : 0xFF);
    if (n < SKY_DEEP && Y.down != 0xFF && (Y.f || (skd () & 1))) {
        sky_link (O_SHAFT_DOWN, n + 1);
    }
}

/* An opening, no link's */
static void __fastcall__ sky_open (unsigned char kind, unsigned char sill)
{
    Y.o = &L->open[L->nopen++];
    Y.o->kind = kind;
    Y.o->x = Y.ox;
    Y.o->y = Y.oy;
    Y.o->sill = sill;
    Y.o->link = 0xFF;
}

/* A number of its own, by the chunk's place, the game's seed and k */
static unsigned char __fastcall__ wh (unsigned char k)
{
    Y.h = (L->cx * 31 + L->cy * 17 + (unsigned char) (k + Y.sd) * 101) ^ 0x5B17;
    Y.h ^= Y.h >> 7;
    Y.h *= 0x2F1B;
    return (unsigned char) (Y.h >> 8);
}

/* Room Y.i's walls looked along for a place for a window on the surface: one square deep of wall, the floor inside it, two
** of rock outside it */
static void window (void)
{
    Y.b = wh (0xC0 + Y.i);
    for (Y.t = 4, Y.d = Y.b & 3; Y.t; --Y.t, Y.d = (Y.d + 1) & 3) {
        Y.len = Y.d == DIR_N || Y.d == DIR_S ? Y.p->w : Y.p->h;
        for (Y.k = 0; Y.k < Y.len; ++Y.k) {
            Y.lx = Y.d == DIR_N || Y.d == DIR_S ? Y.p->x + Y.k : Y.d == DIR_W ? Y.p->x - 1 : Y.p->x + Y.p->w;
            Y.ly = Y.d == DIR_W || Y.d == DIR_E ? Y.p->y + Y.k : Y.d == DIR_N ? Y.p->y - 1 : Y.p->y + Y.p->h;
            Y.ox = Y.lx - dir_x[Y.d];
            Y.oy = Y.ly - dir_y[Y.d];
            if (Y.lx >= CW || Y.ly >= CW || (unsigned char) (Y.lx + 2 * dir_x[Y.d]) >= CW ||
                (unsigned char) (Y.ly + 2 * dir_y[Y.d]) >= CW || L->tile[Y.ly][Y.lx] != T_WALL ||
                L->tile[Y.oy][Y.ox] != T_FLOOR || L->tile[Y.ly + dir_y[Y.d]][Y.lx + dir_x[Y.d]] != T_ROCK ||
                L->tile[Y.ly + 2 * dir_y[Y.d]][Y.lx + 2 * dir_x[Y.d]] != T_ROCK) {
                continue;
            }
            L->tile[Y.ly][Y.lx] = T_WINDOW;
            sky_open (O_WINDOW, L->nportals);
            Y.q = &L->portal[L->nportals++];
            Y.q->x = Y.lx;
            Y.q->y = Y.ly;
            Y.q->kind = PK_WINDOW;
            Y.q->parts = PP_SKY | (Y.b >> 2 & (PP_GLASS | PP_SHUTTERS | PP_BARS));
            Y.q->state = (Y.q->parts & PP_SHUTTERS) && (Y.b & 0x80) ? 0 : PS_OPEN;
            Y.q->mat = (Y.q->parts & (PP_BARS | PP_SHUTTERS)) == PP_BARS ? PM_IRON : PM_WOOD;
            Y.q->bar = (Y.d + 2) & 3;
            Y.q->size = 2 | 3 << 4;
            Y.q->thick = TH_IN;
            Y.q->qual = 5;
            Y.q->tough = 15;
            return;
        }
    }
}

static void sky_chunk (void)
{
    if (L->sn || cstyle == ST_NATURAL || !L->nrooms) {
        return;
    }
    Y.f = forced ();
    Y.sd = (unsigned char) STRATUM (0).seed;
    for (Y.i = 0, Y.p = L->room; Y.i < L->nrooms; ++Y.i, ++Y.p) {
        if ((Y.f || wh (0x40 + Y.i) % 6 == 0) && L->nopen < NOPEN - 1) {
            Y.ox = Y.p->x + (Y.p->w >> 1);
            Y.oy = Y.p->y + (Y.p->h >> 1);
            if (L->tile[Y.oy][Y.ox] == T_FLOOR) {
                L->tile[Y.oy][Y.ox] = T_UP | T_HOLE;
                sky_open (O_SKYLIGHT, 0);
            }
        }
        if ((Y.f || wh (0x80 + Y.i) % 6 == 0) && L->nopen < NOPEN - 1 && L->nportals < NPORTALS) {
            window ();
        }
    }
}

/* What's asked: chunk c's (made, the chunk in the main RAM, its fixtures in it), or (c 0) stratum n's links */
void __fastcall__ o_sky (struct chunk* c, unsigned char n)
{
    if (c) {
        L = c;
        sky_chunk ();
    } else {
        sky_links (n);
    }
}
