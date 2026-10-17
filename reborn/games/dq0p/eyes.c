/*
** eyes.c - (dq0p) the eyes (DUNGEON.md: perception as layers): each actor's adapted to the light where it is, the
** senses' to keep (the character's, each denizen's), as time goes: quickly to a brighter light, dazzled a while by
** one much brighter than they're used to, slowly (minutes) to the dark; and what they make of the light layer far
** off.  A light seen isn't one that lights: its source in view, a glow, is seen far further than it lights, the
** further the darker the eyes are used to (a source of brightness b, 2 r + 1 for a light of reach r, the light
** layer's, at d squares, is seen if d <= 16 b / (4 + a), a the eyes' level, GLOW_MAX at most), and only against a
** darker place than itself (the light where the eyes are).  Seeing it is a line from the eyes to it, clear of rock,
** walls and shut doors.  The rates are the kinds' (kinds.txt's, for the denizens; the character's here, a man's).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "light.h"
#include "eyes.h"

#pragma static-locals (on)

#define GLOW_MAX        31                      /* Squares a light far off is seen at, at most */
#define PC_DARK         19                      /* The character's eyes: tenths to a 16th of a level darker (a torch's
                                                **   9 to the dark's 0 in some 4.5 minutes), */
#define DAZZLE          6                       /*   dazzled by a light this many levels brighter than they're used to,
                                                **   for DAZZLE_T tenths a level */
#define DAZZLE_T        5

struct eyes E[1 + NACTORS];                     /* The character's (0), each actor's (its entry plus 1) */
struct glows G;

/* Eyes e as time's gone (to now) in light lv (0-15): adapted (rate: tenths to a 16th of a level darker) */
void __fastcall__ adapt (struct eyes* e, unsigned char lv, unsigned char rate)
{
    static unsigned long now, d;
    static unsigned char t;

    now = hub_now ();
    t = lv << 4;
    if (!e->t || t >= e->a) {
        if (e->t && t - e->a >= DAZZLE << 4) {
            e->dz = now + (unsigned) ((t - e->a) >> 4) * DAZZLE_T;     /* (Dazzled: the light much brighter) */
        }
        e->a = t;
        e->t = now;
        return;
    }
    d = (now - e->t) / rate;                    /* (Darker: a 16th of a level each rate tenths, the rest kept) */
    e->t += d * rate;
    e->a = d >= e->a - t ? t : e->a - (unsigned char) d;
}

/* How far eyes e see a light of brightness b far off (the caller's to say if they're dazzled: no hub_now here, the
** window a chunk's as a look goes) */
unsigned char __fastcall__ glow_reach (const struct eyes* e, unsigned char b)
{
    static unsigned r;

    r = 16 * b / (4 + (e->a >> 4));
    return r > GLOW_MAX ? GLOW_MAX : r;
}

/* A clear line from x, y on stratum sn, dx, dy across (dist squares, the most of them), between: no rock, wall nor
** shut door on it (the squares a step at a time, dx i / dist toward 0: worked out a step at a time, without dividing;
** mk 0: a chunk not made is in the way, none made: the cache asked once a chunk) */
unsigned char __fastcall__ clear_line (unsigned char sn, unsigned x, unsigned y, int dx, int dy, unsigned dist,
    unsigned char mk)
{
    static unsigned char* p;
    static struct portal* q;
    static unsigned i, sx, sy, ax, ay, qx, qy, rx, ry, kx, ky, ox, oy;
    static unsigned char ok;

    ax = dx < 0 ? -dx : dx;
    ay = dy < 0 ? -dy : dy;
    qx = qy = rx = ry = 0;
    kx = 0xFFFF;
    for (i = 1; i < dist; ++i) {
        for (rx += ax; rx >= dist; rx -= dist) {   /* (|dx| i / dist, its rest rx) */
            ++qx;
        }
        for (ry += ay; ry >= dist; ry -= dist) {
            ++qy;
        }
        sx = dx < 0 ? x - qx : x + qx;
        sy = dy < 0 ? y - qy : y + qy;
        if (!mk) {
            ox = (sx - strata[sn].x0) >> CW_BITS;   /* (Its chunk in the cache: asked once a chunk) */
            oy = (sy - strata[sn].y0) >> CW_BITS;
            if (ox != kx || oy != ky) {
                kx = ox;
                ky = oy;
                ok = cached (sn, sx, sy);
            }
            if (!ok) {
                return 0;
            }
        }
        if (!(p = at (sn, sx, sy)) || !T_CLEAR (*p) ||
            (TILE (*p) == T_DOOR && (q = portal_at (cx_, cy_)) != 0 && !(q->state & PS_OPEN))) {
            return 0;
        }
    }
    return 1;
}

/* The lights the character sees far off (beyond range, the look's), into G: the lit things lying about (the
** light layer's lamps), the lit rooms in chunks the cache has; ahead or beside it, a clear line to each (the brightest
** two kept) */
static unsigned char sn_, face_, range_, e_;
static unsigned x_, y_;

static void __fastcall__ source (unsigned sx, unsigned sy, unsigned char b)
{
    static int dx, dy;
    static unsigned d;
    static unsigned char r, k, v, sw[3];

    dx = sx - x_;
    dy = sy - y_;
    d = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    if (d <= range_ || b <= e_ || d > (r = glow_reach (&E[0], b)) || head_x[face_] * dx + head_y[face_] * dy < 0 ||
        !clear_line (sn_, x_, y_, dx, dy, d, 0)) {
        return;
    }
    v = b - e_;                                 /* (How it stands out: the brightest kept) */
    k = G.nglows < SEE_GLOWS ? G.nglows++ : G.glow[1].dist > d ? 1 : 0xFF;
    if (k == 0xFF) {
        return;
    }
    G.glow[k].rel = REL8 (bearing (dx, dy), face_);
    G.glow[k].dist = d;
    G.glow[k].how = v >= 4 && 4 * d <= 3 * r ? GL_LIGHT : GL_GLOW;
    if (k && G.glow[0].dist > d) {               /* (The nearer first) */
        memcpy (sw, &G.glow[0], 3);
        memcpy (&G.glow[0], &G.glow[1], 3);
        memcpy (&G.glow[1], sw, 3);
    }
}

void give_way (void);                           /* (dq0p.c's: the look giving way to the UI's command in hand) */
void __fastcall__ glows (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char range,
    unsigned char light)
{
    static unsigned char k, n, a, lx, ly;
    static struct lsrc* l;
    static struct cslot* s;
    static struct room* r;
    static unsigned cx, cy, ox, oy;
    static struct { unsigned x, y; } rs[8];
    static struct room rm[NROOMS];              /* (A chunk's rooms, copied out: the lines from them go elsewhere) */
    static unsigned char nr;

    give_way ();
    e_ = light_at (sn, x, y);                   /* (The light where the eyes are: the layer's, its own) */
    if (light && 2 * light + 1 > e_) {
        e_ = 2 * light + 1;
    }
    adapt (&E[0], e_, PC_DARK);
    memset (&G, 0, sizeof G);
    G.dazzled = hub_now () < E[0].dz;
    if (range == 0xFF || G.dazzled) {
        return;                                 /* (Blinded, or dazzled: none) */
    }
    sn_ = sn;
    x_ = x;
    y_ = y;
    face_ = face;
    range_ = range;
    for (k = nls, l = ls; k; --k, ++l) {
        source (l->x, l->y, 2 * l->r + 1);
    }
    cx = x - strata[sn].x0 - GLOW_MAX;          /* (The lit rooms in the chunks round it the cache has: each one's
                                                **   lit square nearest the eyes) */
    cy = y - strata[sn].y0 - GLOW_MAX;
    hub_map (WORLD_BANK);
    for (k = NCSLOTS, s = WREC->cslot, n = 0; k && n < 8; --k, ++s) {
        if (s->used && s->sn == sn && (s->cx << CW_BITS) + CW - 1 - cx < CW + 2 * GLOW_MAX &&
            (s->cy << CW_BITS) + CW - 1 - cy < CW + 2 * GLOW_MAX) {
            rs[n].x = strata[sn].x0 + (s->cx << CW_BITS);
            rs[n++].y = strata[sn].y0 + (s->cy << CW_BITS);
        }
    }
    while (n--) {
        at (sn, rs[n].x, rs[n].y);
        nr = C->nrooms;
        memcpy (rm, C->room, nr * sizeof (struct room));
        ox = x - rs[n].x;
        oy = y - rs[n].y;
        for (a = nr, r = rm; a; --a, ++r) {
            if (r->flags & R_LIT) {
                lx = (int) ox < (int) r->x - 1 ? r->x - 1 : (int) ox > r->x + r->w ? r->x + r->w : ox;
                ly = (int) oy < (int) r->y - 1 ? r->y - 1 : (int) oy > r->y + r->h ? r->y + r->h : oy;
                source (rs[n].x + lx, rs[n].y + ly, L_ROOM);
            }
        }
    }
    hub_map (0);
}
