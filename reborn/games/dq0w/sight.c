/*
** sight.c - (dq0w) what the character perceives, made into the hub's record of it (level.h's struct sight: the
** UI words it, and never reads the dungeon), after each look about (view.c's squares seen now).  (The map is the
** maker's: dq0m/map.c.)
*/

#include <string.h>
#include "../dq0/hub.h"
#include "world.h"
#include "sight.h"

#define EYE         5

static struct sight S;
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

static signed char __fastcall__ fz (int dz)
{
    return dz > 127 ? 127 : dz < -127 ? -127 : dz;
}

/* A room's, in the chunk in the window: its zones but z (the character's), seen now: where, how high, and how one
** gets there (steps in the room, a slope between, or a ledge) */
static void __fastcall__ zones_seen (struct room* rp, unsigned char mine, unsigned char face, int myz, unsigned char lx,
    unsigned char ly)
{
    unsigned char k, i, j, steps = 0, seen, mx, my;
    struct zone* z;
    int dz, worst;

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
                if (i + 1 < CW && zone_index (i + 1, j) == mine) {
                    worst = worst > abs_ (C->elev[j][i + 1] - C->elev[j][i]) ? worst :
                        abs_ (C->elev[j][i + 1] - C->elev[j][i]);
                }
                if (i && zone_index (i - 1, j) == mine) {
                    worst = worst > abs_ (C->elev[j][i - 1] - C->elev[j][i]) ? worst :
                        abs_ (C->elev[j][i - 1] - C->elev[j][i]);
                }
                if (j + 1 < CW && zone_index (i, j + 1) == mine) {
                    worst = worst > abs_ (C->elev[j + 1][i] - C->elev[j][i]) ? worst :
                        abs_ (C->elev[j + 1][i] - C->elev[j][i]);
                }
                if (j && zone_index (i, j - 1) == mine) {
                    worst = worst > abs_ (C->elev[j - 1][i] - C->elev[j][i]) ? worst :
                        abs_ (C->elev[j - 1][i] - C->elev[j][i]);
                }
            }
        }
        S.zone[S.nzones].rel = REL8 (bearing ((int) mx - lx, (int) my - ly), face);
        S.zone[S.nzones].dz = fz (dz);
        S.zone[S.nzones++].how = worst <= STEP_MAX ? Z_SLOPE : steps ? Z_STEPS : Z_LEDGE;
    }
}

void __fastcall__ perceive (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char first)
{
    struct stratum* s = &strata[sn];
    struct room rp;
    struct zone* z;
    unsigned char* p = at (sn, x, y);
    unsigned char r, k, i, j, lx = cx_, ly = cy_, whole, d;
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
    S.first = first;
    S.ceil = C_UNSEEN;
    if ((r = room_of (sn, x, y)) != 0xFF) {
        rp = C->room[r];
        S.where = W_ROOM;
        S.kind = rp.kind;
        S.lit = (rp.flags & R_LIT) != 0;
        whole = 1;
        for (j = rp.y; j < rp.y + rp.h; ++j) {
            for (i = rp.x; i < rp.x + rp.w; ++i) {
                if (!(C->tile[j][i] & T_SEEN)) {
                    whole = 0;
                }
            }
        }
        area = rp.w * rp.h;
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
                if (TILE (C->tile[wall_y][wall_x]) == T_DOOR && (C->tile[wall_y][wall_x] & T_VIS)) {
                    S.door[S.ndoors].rel = REL8 (bearing ((int) wall_x - lx, (int) wall_y - ly), face);
                    S.door[S.ndoors++].dz = fz (C->elev[wall_y][wall_x] - myz);
                }
            }
        }
        for (k = 0; k < C->nopen && S.nopen < SEE_OPEN; ++k) {     /* (Its stairways, ladders, shafts) */
            i = C->open[k].x;
            j = C->open[k].y;
            if (i >= rp.x && i < rp.x + rp.w && j >= rp.y && j < rp.y + rp.h && (C->tile[j][i] & T_VIS)) {
                S.open[S.nopen].rel = i == lx && j == ly ? 0xFF : REL8 (bearing ((int) i - lx, (int) j - ly), face);
                S.open[S.nopen].kind = C->open[k].kind;
                S.open[S.nopen++].dz = fz (C->elev[j][i] - myz);
            }
        }
    } else {
        S.where = W_PASSAGE;
        for (d = 0; d < 4; ++d) {               /* (The ways it goes on, felt in the dark too) */
            if (step_ok (sn, x, y, d) && (p = at (sn, x + dir_x[d], y + dir_y[d])) != 0) {
                dz = C->elev[cy_][cx_] - myz;
                S.way[S.nways].rel = REL8 (H_OF (d), face);
                S.way[S.nways].dz = fz (dz);
                S.way[S.nways++].steps = TILE (*p) == T_STEPS;
            }
        }
    }
    hub_map (0);
    memcpy (&HUB->sight, &S, sizeof S);
}
