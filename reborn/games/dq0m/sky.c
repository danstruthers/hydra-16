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

/* The environment's dq0sky said: 1 (a test's scene, all of them), 2 ("0": none at all), 0 not said */
static unsigned char forced (void)
{
    Y.ar.a = 0xFF;
    Y.ar.r[0] = (unsigned) "dq0sky";
    Y.ar.r[1] = (unsigned) Y.val;
    Y.ar.r[2] = 3;
    Y.ar.r[3] = 0;
    return hy_call (HY_ENV_GET, &Y.ar) ? 0 : Y.val[0] == '0' ? 2 : 1;
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

/* Y.x, Y.y's chunk has no other way between strata in it (a room's record, a chamber, its own) */
static unsigned char lonely (void)
{
    for (Y.i = Y.s->nlinks, Y.l = Y.s->link; Y.i; --Y.i, ++Y.l) {
        if (((Y.l->x - Y.s->x0) ^ (Y.x - Y.s->x0)) < CW && ((Y.l->y - Y.s->y0) ^ (Y.y - Y.s->y0)) < CW) {
            return 0;
        }
    }
    return 1;
}

static void __fastcall__ sky_links (unsigned char n)
{
    Y.s = &STRATUM (n);
    Y.f = forced ();
    if (Y.f == 2) {
        return;
    }
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
        if (!Y.f && !lonely ()) {
            return;
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
            if (rg (n, (Y.x - Y.s->x0) >> 8, (Y.y - Y.s->y0) >> 8)->kind != RG_HALLS || !lonely ()) {
                Y.x = 0;
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
    if (L->sn > SKY_DEEP) {
        return;
    }
    for (Y.i = L->nopen, Y.o = L->open; Y.i; --Y.i, ++Y.o) {
        if (Y.o->kind == O_SHAFT_UP && STRATUM (L->sn).link[Y.o->link].dug == LD_SKY) {
            Y.o->kind = O_SKYSHAFT;             /* (A shaft open to the sky, as it's told and looked for) */
        }
    }
    if (L->sn || cstyle == ST_NATURAL || !L->nrooms || (Y.f = forced ()) == 2) {
        return;
    }
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

/* The squares under or beside what shows the sky marked T_HOLE (but a way up's: the map's mark's a skylight's), so the world
** knows it's come by one at a glance (part 17's); after the chunk's stocked, nothing of the maker's reading its squares
** bare */
static void sky_mark (void)
{
    if (L->sn > SKY_DEEP) {
        return;
    }
    for (Y.i = L->nopen, Y.o = L->open; Y.i; --Y.i, ++Y.o) {
        if (Y.o->kind == O_SKYLIGHT || Y.o->kind == O_WINDOW || Y.o->kind == O_SKYSHAFT) {
            for (Y.ly = Y.o->y - 1; Y.ly != (unsigned char) (Y.o->y + 2); ++Y.ly) {
                for (Y.lx = Y.o->x - 1; Y.lx != (unsigned char) (Y.o->x + 2); ++Y.lx) {
                    if (Y.lx < CW && Y.ly < CW && (t_kinds[TILE (L->tile[Y.ly][Y.lx])] & 1) &&
                        TILE (L->tile[Y.ly][Y.lx]) != T_UP) {
                        L->tile[Y.ly][Y.lx] |= T_HOLE;
                    }
                }
            }
        }
    }
}

/* What's asked: chunk c's (made, the chunk in the main RAM, its fixtures in it: n 0; stocked, n 1), or (c 0) stratum n's
** links */
/* Its things' list (stock.c's sk, the chunk's first time) gone over, no dice (nothing else moves for them; the same chunk always the
** same): a found key fits a lock in its own chunk, so it needs no other chunk made (the user's A17, "fit something nearby"; HOUSE.md's
** keys that fit): each in turn the chunk's next locked door (its number the door's, DOOR_KEY: a door's own key, wport.c's), else the
** next chest in the list (locked to fit it as it's put down, KEY_BOX: moved before it); no lock in the chunk, no key put down.  And
** what a rope's tied to, at the top of a shaft going down (HOUSE.md's rope and grapples): a built stratum's an iron ring, a mine's a
** stout post */
static void list_over (void)
{
    struct sthing t;
    struct sthing* e;
    const struct portal* p;
    unsigned id, x0, y0;
    unsigned char j, m, b, k;

    x0 = STRATUM (L->sn).x0 + ((unsigned) L->cx << CW_BITS);
    y0 = STRATUM (L->sn).y0 + ((unsigned) L->cy << CW_BITS);
    m = b = 0;
    for (j = 0; j < sk.n; ++j) {
        e = &sk.e[j];
        if (e->kind != KI_KEY) {
            continue;
        }
        for (; m < L->nportals; ++m) {
            p = &L->portal[m];
            if (p->kind == PK_DOOR && (p->state & (PS_LOCKED | PS_WARDED)) == PS_LOCKED && p->lock) {
                break;
            }
        }
        if (m < L->nportals) {
            id = DOOR_KEY (L->sn, x0 + p->x, y0 + p->y);
            e->make = id;
            e->size = id >> 8;
            ++m;
            continue;
        }
        for (; b < sk.n && sk.e[b].kind != KI_CHEST; ++b) {
        }
        if (b < sk.n) {
            if (b > j) {                        /* (The chest first, as the world puts them down) */
                t = *e;
                *e = sk.e[b];
                sk.e[b] = t;
                sk.e[b].make = KEY_BOX;
                sk.e[b].size = 0;
                j = b;
            } else {
                e->make = KEY_BOX;
                e->size = 0;
            }
            ++b;
            continue;
        }
        --sk.n;                                 /* (None: the rest moved up) */
        for (k = j; k < sk.n; ++k) {
            sk.e[k] = sk.e[k + 1];
        }
        --j;
    }
    k = STRATUM (L->sn).style;
    if (k == ST_BUILT || k == ST_MINE) {
        for (j = 0; j < L->nopen; ++j) {
            if (L->open[j].kind == O_SHAFT_DOWN) {
                put_down (k == ST_MINE ? KI_POST : KI_MOOR, L->sn, x0 + L->open[j].x, y0 + L->open[j].y, 1, ST_SHOWN);
            }
        }
    }
}

void __fastcall__ o_sky (struct chunk* c, unsigned char n)
{
    if (c) {
        L = c;
        if (n == 2) {
            list_over ();
        } else if (n) {
            sky_mark ();
        } else {
            sky_chunk ();
        }
    } else {
        sky_links (n);
    }
}
