/*
** sight.c - (dq0w) what the character perceives, made into the hub's record of it (level.h's struct sight: the
** UI words it, and never reads the dungeon), after each look about (view.c's squares seen now).  (The map is the
** maker's: dq0m/map.c.)
*/

#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "sight.h"
#include "things.h"
#include "eyes.h"

#pragma code-name ("SW2CODE")                   /* (Part 2's, all of it: swapped, bank.s's stubs, once a look) */
#pragma rodata-name ("SW2RODATA")
#pragma local-strings (on)
#pragma static-locals (on)                      /* (Its locals static: quicker, the code the less; none of it called
                                                **   in itself) */

#define EYE         5

static struct sight S;
unsigned char eye;                              /* (The character's sight, 0-9: dq0p.c's look sets it) */
static unsigned char mx_[SEE_DOORS], my_[SEE_DOORS];     /* (The mouths' squares, as they're counted) */
static unsigned char seq;

static int __fastcall__ abs_ (int v)
{
    return v < 0 ? -v : v;
}

/* The zone of the chunk in the window that square (x, y) of it is in, or 0xFF */
static unsigned char __fastcall__ zone_index (unsigned char x, unsigned char y)
{
    unsigned char k;
    struct zone* z;

    for (k = 0; k < C->nzones; ++k) {
        z = &C->zone[k];
        if (x >= z->x && x < z->x + z->w && y >= z->y && y < z->y + z->h) {
            return k;
        }
    }
    return 0xFF;
}

/* A passage's square (i, j) of the chunk in the window, at a room's edge: a mouth, unless one's been counted
** beside it (a wide tunnel's three squares one mouth) */
static unsigned char __fastcall__ mouth (unsigned char i, unsigned char j)
{
    unsigned char k, d, f = 0;

    for (d = 0; d < 4; ++d) {                   /* (Beside a room's floor) */
        if (i + dir_x[d] < CW && j + dir_y[d] < CW && T_INSIDE (C->tile[j + dir_y[d]][i + dir_x[d]])) {
            f = 1;
        }
    }
    if (!f) {
        return 0;
    }
    for (k = 0; k < S.ndoors; ++k) {
        if ((S.door[k].rel & MOUTH) && mx_[k] + 1 >= i && mx_[k] <= i + 1 && my_[k] + 1 >= j && my_[k] <= j + 1) {
            mx_[k] = i;                         /* (One mouth: its squares follow on) */
            my_[k] = j;
            return 0;
        }
    }
    mx_[S.ndoors] = i;
    my_[S.ndoors] = j;
    return 1;
}

/* A seen door's portal (q: or NULL, a doorway): its kind and parts (a window's; a door's or a portcullis's, its
** features, as they're seen from afar: two leaves ...) */
static void __fastcall__ portal_seen (const struct portal* q)
{
    S.door[S.ndoors].kind = q ? q->kind : PK_DOOR;
    S.door[S.ndoors].parts = !q ? 0 : q->kind == PK_WINDOW ? q->parts : q->feat;
}

/* The traps the character knows of (found, or never hidden: an open pit) on the squares of its chunk it sees now */
static void __fastcall__ traps_seen (unsigned char sn, unsigned x, unsigned y, unsigned char face)
{
    struct trap* t;
    unsigned char k;

    at (sn, x, y);
    for (k = 0, t = C->trap; k < C->ntraps && S.ntraps < SEE_TRAPS; ++k, ++t) {
        if (!(t->state & TS_HIDDEN) && (C->tile[t->y][t->x] & T_VIS)) {
            S.trap[S.ntraps].rel = t->x == cx_ && t->y == cy_ ? 0xFF : REL8 (bearing ((int) t->x - cx_,
                (int) t->y - cy_), face);
            S.trap[S.ntraps].kind = t->kind;
            S.trap[S.ntraps].trigger = t->trigger;
            S.trap[S.ntraps++].state = t->state;
        }
    }
}

static signed char __fastcall__ fz (int dz)
{
    return dz > 127 ? 127 : dz < -127 ? -127 : dz;
}

/* A room's, in the chunk in the window: its zones but z (the character's), seen now: where, how high, and how one
** gets there (steps in the room, a slope between, or a ledge) */
static void __fastcall__ zones_seen (struct room* rp, unsigned char mine, unsigned char face, int myz, unsigned char lx,
    unsigned char ly)
{
    unsigned char k, i, j, steps = 0, seen, mx, my, d, ni, nj;
    struct zone* z;
    int dz, worst, v;

    for (j = rp->y; j < rp->y + rp->h; ++j) {
        for (i = rp->x; i < rp->x + rp->w; ++i) {
            if (TILE (C->tile[j][i]) == T_STEPS) {
                steps = 1;
            }
        }
    }
    for (k = rp->z0; k < rp->z0 + rp->nz && S.nzones < SEE_ZONES; ++k) {
        if (k == mine) {
            continue;
        }
        z = &C->zone[k];
        seen = 0;
        for (j = z->y; j < z->y + z->h && !seen; ++j) {
            for (i = z->x; i < z->x + z->w; ++i) {
                if (C->tile[j][i] & T_VIS) {
                    seen = 1;
                    break;
                }
            }
        }
        if (!seen) {
            continue;
        }
        mx = z->x + z->w / 2;
        my = z->y + z->h / 2;
        dz = C->elev[my][mx] - myz;
        worst = 0;                              /* (The worst step across its edge with the character's zone) */
        for (j = z->y; j < z->y + z->h; ++j) {
            for (i = z->x; i < z->x + z->w; ++i) {
                for (d = 0; d < 4; ++d) {       /* (The four beside it, in the chunk) */
                    ni = i + dir_x[d];
                    nj = j + dir_y[d];
                    if (ni < CW && nj < CW && zone_index (ni, nj) == mine &&
                        (v = abs_ (C->elev[nj][ni] - C->elev[j][i])) > worst) {
                        worst = v;
                    }
                }
            }
        }
        S.zone[S.nzones].rel = REL8 (bearing ((int) mx - lx, (int) my - ly), face);
        S.zone[S.nzones].dz = fz (dz);
        S.zone[S.nzones++].how = worst <= STEP_MAX ? Z_SLOPE : steps ? Z_STEPS : Z_LEDGE;
    }
}

unsigned char __fastcall__ s_perceive (unsigned char sn, unsigned x, unsigned y, unsigned char face,
    unsigned char light, unsigned char first)
{
    struct stratum* s = &STRATUM (sn);
    struct room rp;
    struct zone* z;
    unsigned char* p = at (sn, x, y);
    struct portal* q;
    unsigned char r, k, i, j, lx = cx_, ly = cy_, whole, d, arrive = first & LF_ARRIVE;
    int myz = C->elev[ly][lx], dz;
    unsigned area;

    memset (&S, 0, sizeof S);
    S.seq = ++seq;
    S.stratum = sn;
    S.sname = s->name;
    S.style = s->style;
    S.x = x;
    S.y = y;
    S.z = s->datum + myz;
    S.dark = !(*p & T_VIS);
    S.ceil = C_UNSEEN;
    if (arrive) {
        first = 0;                              /* (Come in: the first time in a room the senses don't remember, and
                                                **   remembered from now on) */
    }
    S.eye = eye;
    if ((r = room_of (sn, x, y)) != 0xFF) {
        rp = C->room[r];
        if (arrive) {
            first = !(rp.flags & R_VISITED);
            C->room[r].flags |= R_VISITED;
        }
        S.where = W_ROOM;
        S.kind = rp.kind;
        S.lit = (rp.flags & R_LIT) != 0;
        whole = 1;
        for (j = rp.y; j < (unsigned char) (rp.y + rp.h); ++j) {
            for (i = rp.x; i < (unsigned char) (rp.x + rp.w); ++i) {
                if ((C->tile[j][i] & (T_SEEN | 0x1F)) == T_FLOOR) {   /* (Its floor's: a cave's bounds take in rock) */
                    whole = 0;
                }
            }
        }
        area = rp.w * rp.h;
        S.rw = rp.w;
        S.rh = rp.h;
        S.rx = x - lx + rp.x;
        S.ry = y - ly + rp.y;
        S.size = !whole ? S_PART : area <= 8 ? S_SMALL : area >= 30 ? S_LARGE : rp.w >= 3 * rp.h ||
            rp.h >= 3 * rp.w ? S_LONG : S_MID;
        if ((k = zone_index (lx, ly)) != 0xFF) {
            z = &C->zone[k];
            if (z->gx) {
                S.up_rel = REL8 (z->gx > 0 ? H_E : H_W, face);
                S.up_ft = (z->gx > 0 ? z->gx : -z->gx) / 2;
            } else if (z->gy) {
                S.up_rel = REL8 (z->gy > 0 ? H_S : H_N, face);
                S.up_ft = (z->gy > 0 ? z->gy : -z->gy) / 2;
            }
            if (!S.dark && (S.lit || (unsigned) light * 5 + EYE >= z->cmid)) {
                S.ceil = z->ceil;
                S.cmid = z->cmid;
                S.cwall = z->cwall;
            }
            zones_seen (&rp, k, face, myz, lx, ly);
        }
        for (d = 0; d < 4; ++d) {               /* (Its doors seen) */
            for (k = 0; wall_tile (&rp, d, k) && S.ndoors < SEE_DOORS; ++k) {
                if ((TILE (C->tile[wall_y][wall_x]) == T_DOOR || TILE (C->tile[wall_y][wall_x]) == T_WINDOW) &&
                    (C->tile[wall_y][wall_x] & T_VIS) &&
                    !((q = portal_at (wall_x, wall_y)) != 0 && (q->state & PS_HIDDEN))) {
                    S.door[S.ndoors].rel = REL8 (bearing ((int) wall_x - lx, (int) wall_y - ly), face);
                    S.door[S.ndoors].state = (q = portal_at (wall_x, wall_y)) != 0 ? q->state & (PS_OPEN | PS_WRECKED) : DOORWAY;
                    portal_seen (q);                /* (A portcullis, a window, a slit: its kind and parts) */
                    S.door[S.ndoors].x = x - lx + wall_x;
                    S.door[S.ndoors].y = y - ly + wall_y;
                    S.door[S.ndoors++].dz = fz (C->elev[wall_y][wall_x] - myz);
                }
            }
        }
        for (j = rp.y ? rp.y - 1 : 0; j <= rp.y + rp.h && j < CW; ++j) {     /* (Its passages' mouths seen) */
            for (i = rp.x ? rp.x - 1 : 0; i <= rp.x + rp.w && i < CW && S.ndoors < SEE_DOORS; ++i) {
                if (TILE (C->tile[j][i]) == T_CORR && (C->tile[j][i] & T_VIS) && mouth (i, j)) {
                    S.door[S.ndoors].rel = REL8 (bearing ((int) i - lx, (int) j - ly), face) | MOUTH;
                    S.door[S.ndoors].state = PS_OPEN;
                    S.door[S.ndoors].x = x - lx + i;
                    S.door[S.ndoors].y = y - ly + j;
                    S.door[S.ndoors++].dz = fz (C->elev[j][i] - myz);
                }
            }
        }
        for (k = 0; k < C->nopen && S.nopen < SEE_OPEN; ++k) {     /* (Its stairways, ladders, shafts) */
            i = C->open[k].x;
            j = C->open[k].y;
            if (i >= rp.x && i < rp.x + rp.w && j >= rp.y && j < rp.y + rp.h && (C->tile[j][i] & T_VIS)) {
                S.open[S.nopen].rel = i == lx && j == ly ? 0xFF : REL8 (bearing ((int) i - lx, (int) j - ly), face);
                S.open[S.nopen].kind = C->open[k].kind;
                S.open[S.nopen].x = x - lx + i;
                S.open[S.nopen].y = y - ly + j;
                S.open[S.nopen++].dz = fz (C->elev[j][i] - myz);
            }
        }
    } else {
        S.where = W_PASSAGE;
        for (d = 0; d < 4; ++d) {               /* (A door beside it, open or shut) */
            if ((p = at (sn, x + dir_x[d], y + dir_y[d])) != 0 && (TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) &&
                S.ndoors < SEE_DOORS && !((q = portal_at (cx_, cy_)) != 0 && (q->state & PS_HIDDEN))) {
                S.door[S.ndoors].rel = REL8 (H_OF (d), face);
                S.door[S.ndoors].state = q ? q->state & (PS_OPEN | PS_WRECKED) : DOORWAY;
                portal_seen (q);
                S.door[S.ndoors].x = x + dir_x[d];
                S.door[S.ndoors].y = y + dir_y[d];
                S.door[S.ndoors++].dz = fz (C->elev[cy_][cx_] - myz);
            }
        }
        for (d = 0; d < 4; ++d) {               /* (The ways it goes on, felt in the dark too) */
            if (step_ok (sn, x, y, d) && (p = at (sn, x + dir_x[d], y + dir_y[d])) != 0) {
                dz = C->elev[cy_][cx_] - myz;
                S.way[S.nways].rel = REL8 (H_OF (d), face);
                S.way[S.nways].dz = fz (dz);
                S.way[S.nways++].steps = TILE (*p) == T_STEPS;
            }
        }
    }
    S.first = first;
    memcpy (&S.nglows, &G, sizeof G);           /* (The lights far off: eyes.c's) */
    traps_seen (sn, x, y, face);
    things_stocked ();                          /* (A chunk the maker's stocked: its things put down, the world's) */
    things_seen (&S, sn, x, y, face);           /* (The things it sees lying about) */
    hub_map (SIGHT_BANK);                       /* (The record the senses' bank's) */
    memcpy (SIGHT, &S, sizeof S);
    hub_map (0);
    return first;
}
