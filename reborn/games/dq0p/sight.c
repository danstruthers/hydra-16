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
#include "view.h"

#pragma code-name ("SW2CODE")                   /* (Part 2's, all of it: swapped, bank.s's stubs, once a look) */
#pragma rodata-name ("SW2RODATA")
#pragma local-strings (on)
#pragma static-locals (on)                      /* (Its locals static: quicker, the code the less; none of it called
                                                **   in itself) */

#define EYE         5

static struct sight S;
extern unsigned char here_known;                /* (dq0p.c's) */
const struct sight* sight_rec = &S;             /* (A look one way's own, way.c's, made from it: the main RAM's) */
unsigned char eye;                              /* (The character's sight, 0-9: dq0p.c's look sets it) */
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
#pragma data-name (push, "SW2RODATA")           /* (The look's own, set as it begins: in the part, no main RAM) */
static unsigned char mx_[SEE_DOORS] = { 0 }, my_[SEE_DOORS] = { 0 };     /* (The mouths' squares, as they're counted) */
static unsigned char e_ = 0;                    /* (The next door[]: those seen, ndoors of them, then those remembered) */
static unsigned char sun_ = 0;                  /* (The daylight's reach now: the hub's tb.sun) */
static unsigned char kn_ = 0;                   /* (The rel's KNOWN_REL, as it's those remembered) */
static unsigned char some_ = 0;                 /* (A door or an opening only remembered, this look's first pass) */
static unsigned char mine_ = 0;                 /* (The door's bar's side the character's: its wall's, or its passage's) */
static int fzz = 0;                             /* (door_add's: its height, the chunk's first square in the frame, its */
static unsigned fox = 0, foy = 0;               /*   square, its facing) */
static unsigned char flx = 0, fly = 0, ffc = 0;
static struct sdoor* dp = 0;                    /* (door[e_]) */
#pragma data-name (pop)

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
    for (k = 0; k < e_; ++k) {
        if ((S.door[k].rel & MOUTH) && mx_[k] + 1 >= i && mx_[k] <= i + 1 && my_[k] + 1 >= j && my_[k] <= j + 1) {
            mx_[k] = i;                         /* (One mouth: its squares follow on) */
            my_[k] = j;
            return 0;
        }
    }
    mx_[e_] = i;
    my_[e_] = j;
    return 1;
}

/* A seen door's portal (q: or NULL, a doorway): its kind and parts (a window's; a door's or a portcullis's, its
** features, as they're seen from afar: two leaves ...), and shut, what's plain of it: its bar, on the side the character's
** on (mine), or found from the far side; its lock, found (PP_FOUND: the world's, as it's tried) */
static void __fastcall__ portal_seen (const struct portal* q)
{
    dp = &S.door[e_];
    dp->kind = PK_DOOR;
    if (!q) {
        dp->state = DOORWAY;
        return;
    }
    if (q->state & (PS_OPEN | PS_WRECKED)) {
        dp->state = q->state & (PS_OPEN | PS_WRECKED);
    } else if (q->parts & PP_FOUND) {
        dp->state = q->state & (PS_BARRED | PS_LOCKED);
    } else if (mine_ && q->bar == mine_ - 1) {
        dp->state = q->state & PS_BARRED;
    }
    dp->kind = q->kind;
    dp->parts = q->kind == PK_WINDOW ? q->parts : q->feat;
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

/* What its hands find on side rel of it, in the dark (HOUSE.md's walking in the dark, 2026-11-02): 0 nothing within reach,
** 1 a wall, 2 a shut door, 3 bars */
#pragma static-locals (push, off)
static unsigned char __fastcall__ touch (unsigned char rel)
{
    unsigned char* p;
    struct portal* q;
    unsigned char h = (ffc + rel) & 7;

    if (!(p = at (S.stratum, S.x + head_x[h], S.y + head_y[h]))) {
        return 1;
    }
    if (TILE (*p) == T_DOOR && (q = portal_at (cx_, cy_)) != 0 && !(q->state & PS_OPEN)) {
        return q->state & PS_HIDDEN ? 1 : q->kind == PK_PORTCULLIS ? 3 : 2;
    }
    return !T_PASS (*p) && TILE (*p) != T_PIT;
}
#pragma static-locals (pop)

/* Tile t as this pass goes: those it sees, then those it only remembers */
static unsigned char __fastcall__ shown (unsigned char t)
{
    if (kn_) {
        return (t & (T_VIS | T_SEEN)) == T_SEEN;
    }
    some_ |= t & T_SEEN;
    return t & T_VIS;
}

/* The door[] e_ (and on to the next) filled in, of the chunk's square (i, j), q its portal: as it's seen or remembered
** (flx, fly the character's square, ffc its facing, fox, foy the chunk's first in the frame, fzz its height) */
static void __fastcall__ door_add (unsigned char i, unsigned char j, const struct portal* q)
{
    portal_seen (q);                            /* (A portcullis, a window, a slit: its kind and parts) */
    dp->rel = REL8 (bearing ((int) i - flx, (int) j - fly), ffc) | kn_;
    dp->x = fox + i;
    dp->y = foy + j;
    dp->dz = fz (C->elev[j][i] - fzz);
    ++e_;
}

#pragma code-name (push, "SW6CODE")             /* (Part 6's since 2026-11-02: part 2's room, a dark look's touch) */
#pragma rodata-name (push, "SW6RODATA")

static int __fastcall__ abs6 (int v)
{
    return v < 0 ? -v : v;
}

static signed char __fastcall__ fz6 (int dz)
{
    return dz > 127 ? 127 : dz < -127 ? -127 : dz;
}

static unsigned char __fastcall__ zone_at6 (unsigned char x, unsigned char y)
{
    struct zone* z = C->zone;
    unsigned char k;

    for (k = 0; k < C->nzones; ++k, ++z) {
        if (x >= z->x && x < z->x + z->w && y >= z->y && y < z->y + z->h) {
            return k;
        }
    }
    return 0xFF;
}

/* A room's, in the chunk in the window: its zones but z (the character's), seen now: where, how high, and how one
** gets there (steps in the room, a slope between, or a ledge) */
void __fastcall__ s_zones_seen (struct room* rp, unsigned char mine, unsigned char face, int myz, unsigned char lx,
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
                    if (ni < CW && nj < CW && zone_at6 (ni, nj) == mine &&
                        (v = abs6 (C->elev[nj][ni] - C->elev[j][i])) > worst) {
                        worst = v;
                    }
                }
            }
        }
        S.zone[S.nzones].rel = REL8 (bearing ((int) mx - lx, (int) my - ly), face);
        S.zone[S.nzones].dz = fz6 (dz);
        S.zone[S.nzones++].how = worst <= STEP_MAX ? Z_SLOPE : steps ? Z_STEPS : Z_LEDGE;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

void __fastcall__ zones_seen (struct room* rp, unsigned char mine, unsigned char face, int myz, unsigned char lx,
    unsigned char ly);                          /* (Its stub: part 6's) */

unsigned char __fastcall__ s_perceive (unsigned char sn, unsigned x, unsigned y, unsigned char face,
    unsigned char light, unsigned char first)
{
    struct stratum* s = &STRATUM (sn);
    struct room rp;
    struct zone* z;
    unsigned char* p;
    struct portal* q;
    unsigned char r, k, i, j, lx, ly, whole, d, arrive = first & LF_ARRIVE;
    int myz, dz;
    unsigned area;

    hub_map (0);
    sun_ = HUB->tb.sun;
    p = at (sn, x, y);
    lx = cx_;
    ly = cy_;
    myz = C->elev[ly][lx];
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
    S.eye = v_way ? 2 * eye + 2 : eye;          /* (A look one way: what it makes out twice as far) */
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
        d = 0;                                  /* (A great cavern's walls seen: how many, to 6) */
        for (j = rp.y; j < (unsigned char) (rp.y + rp.h); ++j) {
            for (i = rp.x; i < (unsigned char) (rp.x + rp.w); ++i) {
                if ((C->tile[j][i] & (T_VIS | 0x0F)) == (T_VIS | T_WALL)) {
                    d += d < 6;
                }
                if ((C->tile[j][i] & (T_SEEN | 0x0F)) == T_FLOOR) {   /* (Its floor's: a cave's bounds take in rock) */
                    whole = 0;
                }
            }
        }
        area = rp.w * rp.h;
        S.rw = rp.w;
        S.rh = rp.h;
        S.rx = x - lx + rp.x;
        S.ry = y - ly + rp.y;
        S.size = rp.flags & R_CAVERN ? S_VAST + (d >= 6) : !whole ? S_PART : area <= 8 ? S_SMALL : area >= 30 ? S_LARGE :
            rp.w >= 3 * rp.h || rp.h >= 3 * rp.w ? S_LONG : S_MID;     /* (A great cavern's part: vast, its walls
                                                **   beyond the light or not, S_VASTW, slice 4.10's) */
        if ((k = zone_index (lx, ly)) != 0xFF) {
            z = &C->zone[k];
            if (z->gx) {
                S.up_rel = REL8 (z->gx > 0 ? H_E : H_W, face);
                S.up_ft = (z->gx > 0 ? z->gx : -z->gx) / 2;
            } else if (z->gy) {
                S.up_rel = REL8 (z->gy > 0 ? H_S : H_N, face);
                S.up_ft = (z->gy > 0 ? z->gy : -z->gy) / 2;
            }
            d = 0;                              /* (Daylight falling in, a skylight or a shaft open to the sky of its room's:
                                                **   the ceiling seen by it, 2026-11-04) */
            for (i = 0; sun_ && !S.lit && !S.dark && i < C->nopen; ++i) {
                if (C->open[i].kind >= O_SKYLIGHT && C->open[i].x >= rp.x && C->open[i].x < rp.x + rp.w &&
                    C->open[i].y >= rp.y && C->open[i].y < rp.y + rp.h) {
                    d = 1;
                }
            }
            if (!S.dark && (S.lit || d || (unsigned) light * SQ_FT + EYE >= z->cmid)) {
                S.ceil = z->ceil;
                S.cmid = z->cmid;
                S.cwall = z->cwall;
            }
            if (rp.nz > 1) {
                zones_seen (&rp, k, face, myz, lx, ly);
            }
        }
    }
    some_ = e_ = kn_ = 0;                       /* (The part's own: after part 6's zones_seen, its image copied back) */
    fzz = myz;
    fox = x - lx;
    foy = y - ly;
    flx = lx;
    fly = ly;
    ffc = face;
    if (r != 0xFF) {
        for (;;) {                              /* (Those it sees; then those it remembers, KNOWN_REL: named, the look
                                                **   not telling them, the user's, 2026-11-02) */
            for (d = 0; d < 4; ++d) {           /* (Its doors seen) */
                for (k = 0; wall_tile (&rp, d, k) && e_ < SEE_DOORS; ++k) {
                    i = C->tile[wall_y][wall_x];
                    if ((TILE (i) == T_DOOR || TILE (i) == T_WINDOW) && shown (i) &&
                        !((q = portal_at (wall_x, wall_y)) != 0 && (q->state & PS_HIDDEN))) {
                        mine_ = ((d + 2) & 3) + 1;  /* (Its bar seen, on the room's side) */
                        door_add (wall_x, wall_y, q);
                        mine_ = 0;
                    }
                }
            }
            for (j = rp.y ? rp.y - 1 : 0; j <= rp.y + rp.h && j < CW; ++j) {     /* (Its passages' mouths seen) */
                for (i = rp.x ? rp.x - 1 : 0; i <= rp.x + rp.w && i < CW && e_ < SEE_DOORS; ++i) {
                    d = C->tile[j][i];
                    if (TILE (d) == T_CORR && shown (d) && mouth (i, j)) {
                        door_add (i, j, 0);
                        dp->rel |= MOUTH;
                        dp->state = PS_OPEN;
                    }
                }
            }
            for (k = 0, r = S.nopen; k < C->nopen && r < SEE_OPEN; ++k) {     /* (Its stairways, ladders, shafts) */
                i = C->open[k].x;
                j = C->open[k].y;
                d = C->tile[j][i];
                if (i >= rp.x && i < rp.x + rp.w && j >= rp.y && j < rp.y + rp.h && shown (d) && C->open[k].kind != O_WINDOW) {
                    S.open[r].rel = i == lx && j == ly ? 0xFF : REL8 (bearing ((int) i - lx, (int) j - ly), face) | kn_;
                    d = C->open[k].kind;                /* (A window on the surface: told as a window, the doors' pass;
                                                        **   a skylight, a shaft open to the sky, as they're lit now) */
                    S.open[r].kind = d >= O_SKYLIGHT && sun_ ? d | O_DAYLIT : d;
                    S.open[r].x = x - lx + i;
                    S.open[r].y = y - ly + j;
                    S.open[r++].dz = fz (C->elev[j][i] - myz);
                }
            }
            if (kn_) {
                break;
            }
            S.ndoors = e_;
            S.nopen = r;
            if (!some_) {
                break;                          /* (None to look for: the look the quicker) */
            }
            kn_ = KNOWN_REL;
        }
    } else {
        S.where = W_PASSAGE;
        for (d = 0; d < 4; ++d) {               /* (A door beside it, open or shut) */
            if ((p = at (sn, x + dir_x[d], y + dir_y[d])) != 0 && (TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) &&
                S.ndoors < SEE_DOORS && !((q = portal_at (cx_, cy_)) != 0 && (q->state & PS_HIDDEN))) {
                e_ = S.ndoors++;
                mine_ = ((d + 2) & 3) + 1;
                portal_seen (q);
                mine_ = 0;
                dp->rel = REL8 (H_OF (d), face);
                dp->x = x + dir_x[d];
                dp->y = y + dir_y[d];
                dp->dz = fz (C->elev[cy_][cx_] - myz);
            }
        }
        at (sn, x, y);                          /* (The doors further along it, seen or remembered: named, the look not
                                                **   telling them; but as it goes along a walk, its looks the quicker) */
        e_ = S.ndoors;
        kn_ = KNOWN_REL;
        for (j = cy_ > 5 ? cy_ - 5 : 0; (arrive || !(here_known & 0x80)) && j < (unsigned char) (cy_ + 6) && j < CW; ++j) {
            for (i = cx_ > 5 ? cx_ - 5 : 0; i < (unsigned char) (cx_ + 6) && i < CW && e_ < SEE_DOORS; ++i) {
                d = C->tile[j][i];
                if ((TILE (d) == T_DOOR || TILE (d) == T_WINDOW) && (d & (T_VIS | T_SEEN)) &&
                    abs_ ((int) i - cx_) + abs_ ((int) j - cy_) > 1 && !((q = portal_at (i, j)) != 0 && (q->state & PS_HIDDEN))) {
                    door_add (i, j, q);
                }
            }
        }
        for (d = 0; d < 8 && S.nways < 4; ++d) {    /* (The ways it goes on, felt in the dark too; a wide passage's
                                                **   breadth none: one passage, the user's; a diagonal way too) */
            if (way_ok (sn, x, y, d) && (p = at (sn, x + head_x[d], y + head_y[d])) != 0) {
                dz = C->elev[cy_][cx_] - myz;
                S.way[S.nways].rel = REL8 (d, face);
                S.way[S.nways].dz = fz (dz);
                S.way[S.nways++].steps = TILE (*p);    /* (Its square's kind: steps, rubble) */
            }
        }
    }
    if (S.dark) {                               /* (In the dark, its ceiling's unseen: what its hands find instead, and
                                                **   whether it has been on this square before, not forgotten: its walk's
                                                **   squares felt after the look's, here_known, dq0p.c's) */
        S.cmid = touch (0);
        S.cmid += touch (6) << 2;
        S.cmid += touch (2) << 4;
        S.cwall = here_known & 1;
    }
    here_known = S.dark ? here_known & 1 : 1;   /* (Seen lit here: known, if its light goes out) */
    S.first = first;
    memcpy (&S.nglows, &G, sizeof G);           /* (The lights far off: eyes.c's) */
    traps_seen (sn, x, y, face);
    things_stocked ();                          /* (A chunk the maker's stocked: its things put down, the world's) */
    things_seen (&S, sn, x, y, face);           /* (The things it sees lying about) */
    hub_map (SIGHT_BANK);                       /* (The record the senses' bank's) */
    for (k = 0; k < S.nways; ++k) {             /* (Each way's run's bearing as the character faces, in sixteenths, the
                                                **   high four bits: its shape, view.c's runs; words.c's words) */
        d = (RUNS[(S.way[k].rel + face) & 7] - (face << 1)) << 4;
        S.way[k].rel |= d;
    }
    memcpy (SIGHT, &S, sizeof S);
    hub_map (0);
    return first;
}
