/*
** dq0w.c - Dingus Quest Zero's world: a task of its own, started by the UI (dq0w SEG ROLE: the hub's segment and its
** role there, hub.h).  It owns the game's clock, takes the actors' intents and carries them out, and tells what
** happened as events.  It keeps the dungeon (world.h: strata grown whole in outline as anyone first comes to them,
** plan.c; made a chunk at a time as anyone comes near, chunk.c and built.c), and moves the character through it: a
** move takes its time (the referee's walking rate, the slopes), and the character's there when the time's up.  What
** it sees is worked out as it gets there, turns, or its light changes (view.c), and what it perceives recorded in
** the hub for the UI to word (sight.c).  It carries out digging, a spell of work at a time (the referee's tests), and
** asks the maker to keep what it changes.
**
** The clock (main.c's once): each real tick (200 a second) is round tenths of game time over tpr ticks (the pace:
** a round in tpr ticks); paused (Escape) or held (the UI asking something), it stands.  At the turns pace (tpr 0)
** it moves only as the character acts: straight to when the character's free again.
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "view.h"
#include "sight.h"

static unsigned last;                           /* The tick count at the last look */
static unsigned long acc;                       /* Real ticks times round, not yet game time */

/* The clock moved on by d tenths, written so that a reader sees it whole (hub_now) */
static void __fastcall__ advance (unsigned long d)
{
    ++HUB->tick;
    HUB->now += d;
    ++HUB->tick;
}

static void __fastcall__ event (unsigned char kind, unsigned char who, unsigned w1)
{
    static struct msg m;

    m.kind = kind;
    m.who = who;
    m.w1 = w1;
    m.t = HUB->now;
    hub_event (&m);
}

/* The character busy for t tenths: free again at pc_ready (the turns pace: the clock there at once) */
static void __fastcall__ busy (unsigned t)
{
    HUB->pc_ready = HUB->now + t;
    if (!HUB->tpr) {
        advance (t);
    }
}

/* ---- The character's moves: worked out as they start (where to, how long), and made when they're done */

#define TORCH           4                       /* A torch's light: 20 feet */

static struct {
    unsigned char   on;                         /* A move under way */
    unsigned char   how;                        /* A_* */
    unsigned char   dir;                        /* (A_WALKED's: the passage's way out of the room, or on) */
    unsigned char   face;                       /* The heading it'll face */
    unsigned char   sn;                         /* Where its walk begins (A_WALKED's), or where it ends */
    unsigned        x, y;
} move;

static unsigned char sn_, mface;                /* (The character's stratum; a walk's heading as it ends) */
static unsigned char stept = 10;                /* (The referee's step, tenths a square: from the hub as a move
                                                **   starts, for the chunks are in the window as it's walked) */
static unsigned mx, my;                         /* (A walk's square) */

/* A square's time to walk into, tenths of a second: the referee's step (HarnMaster's: walking, half its Move a
** round), more uphill (half again a foot of rise), a little more steeply downhill (a quarter a foot past the
** first), more on steps (half again), twice in the dark (feeling the way) */
static unsigned __fastcall__ cost (int dz, unsigned char steps, unsigned char dark)
{
    unsigned base = stept, c = base;

    if (dz > 0) {
        c += base * dz / 2;
    } else if (dz < -1) {
        c += base * (-dz - 1) / 4;
    }
    if (steps) {
        c += base / 2;
    }
    return dark ? 2 * c : c;
}

/* From (mx, my), heading d, to the next place: the time it takes (mx, my where it ends, mface its heading); see:
** the squares walked seen (felt, in the dark) */
static unsigned long __fastcall__ walk (unsigned char d, unsigned char see, unsigned char dark)
{
    unsigned long t = 0;
    unsigned char n = 0, w, k, nd, inside;
    unsigned char* p;
    int z;

    while (n < 250) {
        if (!step_ok (sn_, mx, my, d)) {
            break;
        }
        z = elev_at (sn_, mx, my);
        mx += dir_x[d];
        my += dir_y[d];
        ++n;
        p = at (sn_, mx, my);
        t += cost (C->elev[cy_][cx_] - z, TILE (*p) == T_STEPS, dark);
        if (see) {
            *p |= T_SEEN;
        }
        inside = TILE (*p) != T_DOOR && room_of (sn_, mx, my) != 0xFF;
        if (inside) {                           /* (Through a door, into a room) */
            break;
        }
        if (TILE (*at (sn_, mx, my)) == T_DOOR) {
            continue;
        }
        for (k = 0, nd = 0, w = 0; k < 4; ++k) {
            if (k != ((d + 2) & 3) && step_ok (sn_, mx, my, k)) {
                ++nd;
                w = k;
            }
        }
        if (nd != 1) {                          /* (A fork, or the passage's end) */
            break;
        }
        d = w;
    }
    mface = d;
    return t;
}

static void __fastcall__ no_way (unsigned char why)
{
    event (E_NOWAY, 0, why);
    HUB->pc_ready = HUB->now;
}

/* ---- The way across a room, round its ledges: the squares of its chunk (in the window) from the room's floor to
** one of its doors or openings, by steps that can be walked; the time it takes */

#define BW      14
#define BH      9
static unsigned char bq[BW * BH], bfrom[BH][BW];

static unsigned char __fastcall__ can (unsigned char x, unsigned char y, unsigned char d)
{
    unsigned char nx = x + dir_x[d], ny = y + dir_y[d], t = C->tile[y][x], u;
    int dz;

    if (nx >= CW || ny >= CW) {
        return 0;
    }
    u = C->tile[ny][nx];
    if (!T_PASS (u)) {
        return 0;
    }
    dz = C->elev[ny][nx] - C->elev[y][x];
    if (dz < 0) {
        dz = -dz;
    }
    return dz <= (TILE (t) == T_STEPS || TILE (u) == T_STEPS ? STAIR_MAX : STEP_MAX);
}

/* From (fx, fy) to (tx, ty) within room r's floor and walls (chunk squares): its time, or 0xFFFFFFFF (no way) */
static unsigned long __fastcall__ cross_room (unsigned char r, unsigned char fx, unsigned char fy, unsigned char tx,
    unsigned char ty, unsigned char dark)
{
    struct room* p = &C->room[r];
    unsigned char ox = p->x - 1, oy = p->y - 1, w = p->w + 2, h = p->h + 2, head = 0, tail = 0, x, y, d, nx, ny;
    unsigned long t = 0;

    if (w > BW || h > BH) {
        return 0xFFFFFFFFUL;
    }
    memset (bfrom, 0xFF, sizeof bfrom);
    bfrom[fy - oy][fx - ox] = 4;
    bq[tail++] = (fy - oy) * BW + (fx - ox);
    while (head < tail) {
        x = bq[head] % BW;
        y = bq[head++] / BW;
        if (x + ox == tx && y + oy == ty) {
            break;
        }
        for (d = 0; d < 4; ++d) {
            nx = x + dir_x[d];
            ny = y + dir_y[d];
            if (nx < w && ny < h && bfrom[ny][nx] == 0xFF && can (x + ox, y + oy, d)) {
                bfrom[ny][nx] = d;
                bq[tail++] = ny * BW + nx;
            }
        }
    }
    x = tx - ox;
    y = ty - oy;
    if (x >= w || y >= h || bfrom[y][x] == 0xFF) {
        return 0xFFFFFFFFUL;
    }
    while ((d = bfrom[y][x]) != 4) {           /* (Back along the way, its steps' times) */
        nx = x - dir_x[d];
        ny = y - dir_y[d];
        t += cost (C->elev[y + oy][x + ox] - C->elev[ny + oy][nx + ox], TILE (C->tile[y + oy][x + ox]) == T_STEPS,
            dark);
        x = nx;
        y = ny;
    }
    return t;
}

/* The character's square in the dark (the room's own light, or its own, would show it) */
static unsigned char dark (void)
{
    return !(*at (sn_, mx, my) & T_VIS);
}

/* What the character sees, made again, and what it perceives recorded (the hub's header in the window: as it ends
** too) */
static void __fastcall__ see (unsigned char first)
{
    unsigned char face = HUB->pc_face, light = HUB->pc_light, sight = HUB->pcsum.sight;
    unsigned char dark = HUB->pcsum.darkvision / 5;   /* (Ultravision's reach, in squares) */
    unsigned x = HUB->pc_x, y = HUB->pc_y;

    light *= HUB->pcsum.nightvision ? HUB->pcsum.nightvision : 1;  /* (Night vision: a light carries further) */
    if (HUB->pcsum.cond & C_BLINDED) {
        sight = 0xFF;
    }
    sn_ = HUB->pc_stratum;
    view (sn_, x, y, face, light, sight, dark);
    perceive (sn_, x, y, face, light, first);
}

/* The character sets out on heading h (north, northeast ...): its time, and where it'll be.  From a room: north,
** east, south or west by a door in that wall (the nearest it can walk to); a diagonal, by the door that lies that
** way; across the room by the way it can walk.  A passage goes only by the four */
static void __fastcall__ go (unsigned char h)
{
    unsigned char r, d = h >> 1, k, best = 0xFF, slow, bx = 0, by = 0, wx, wy;
    unsigned long t, bt = 0xFFFFFFFFUL;
    unsigned ox, oy;

    sn_ = HUB->pc_stratum;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    stept = HUB->pcsum.step ? HUB->pcsum.step : 10;
    slow = dark ();
    if ((r = room_of (sn_, mx, my)) != 0xFF) {
        ox = mx - cx_;                          /* (The chunk's first square, in the frame) */
        oy = my - cy_;
        wx = cx_;
        wy = cy_;
        for (k = 0; k < 4; ++k) {               /* (The doors that way: the quickest to walk to) */
            for (d = 0; wall_tile (&C->room[r], k, d); ++d) {
                if (TILE (C->tile[wall_y][wall_x]) != T_DOOR || ((h & 1) ? bearing ((int) wall_x - wx,
                    (int) wall_y - wy) != h : k != (h >> 1))) {
                    continue;
                }
                if ((t = cross_room (r, wx, wy, wall_x, wall_y, slow)) < bt) {
                    bt = t;
                    best = k;
                    bx = wall_x;
                    by = wall_y;
                }
            }
        }
        if (best == 0xFF) {
            hub_map (0);
            no_way (N_NO_EXIT);
            return;
        }
        mx = ox + bx;
        my = oy + by;
        d = best;
        move.x = mx;
        move.y = my;
        t = bt + walk (d, 0, slow);
    } else if ((h & 1) || !step_ok (sn_, mx, my, d)) {
        hub_map (0);
        no_way (N_NO_EXIT);
        return;
    } else {
        move.x = mx;
        move.y = my;
        t = walk (d, 0, slow);
    }
    hub_map (0);
    move.on = 1;
    move.how = A_WALKED;
    move.dir = d;
    move.sn = sn_;
    move.face = H_OF (mface);
    busy (t > 60000UL ? 60000U : (unsigned) t);
}

/* ---- The strata: the one below a link, begun when anyone first takes it */

/* A stand-in for the theme's say: a stratum's bounds round the squares it must hold (the links down to it), its
** size picked from a few classes: a cellar (a few squares), a modest one (a few hundred feet), a great one (a mile
** or two) */
static void __fastcall__ bounds (struct stratum* s, unsigned lx0, unsigned ly0, unsigned lx1, unsigned ly1)
{
    unsigned char k = random (20);
    unsigned w = k < 5 ? 4 + random (5) : k < 12 ? 60 + random (140) : 400 + random (1600), h;

    h = k < 5 ? 4 + random (5) : k < 12 ? 60 + random (140) : 400 + random (1600);
    if (w < lx1 - lx0 + 8) {
        w = lx1 - lx0 + 8;
    }
    if (h < ly1 - ly0 + 8) {
        h = ly1 - ly0 + 8;
    }
    s->w = w;
    s->h = h;
    s->x0 = lx0 - 3 - random (w - (lx1 - lx0) - 6);
    s->y0 = ly0 - 3 - random (h - (ly1 - ly0) - 6);
}

/* Stratum n + 1 begun, under stratum n's links down: its record (in the hub: its bounds round them, a link up at
** each), then its plan the maker's */
static void __fastcall__ grow_below (unsigned char n)
{
    struct stratum* s = &strata[n];
    struct stratum* b;
    unsigned lx0 = 0xFFFF, ly0 = 0xFFFF, lx1 = 0, ly1 = 0;
    unsigned char i;

    hub_map (0);
    b = &HUB->strata[n + 1];
    rseed (s->seed + 977);
    memset (b, 0, sizeof *b);
    b->n = n + 1;
    b->seed = s->seed * 69069UL + 1;
    b->style = ST_BUILT;
    b->age = 1 + random (40);
    b->name = random (8);
    b->datum = s->datum - 30 - random (40);
    for (i = 0; i < s->nlinks; ++i) {
        if (s->link[i].to != n + 1) {
            continue;
        }
        lx0 = s->link[i].x < lx0 ? s->link[i].x : lx0;
        ly0 = s->link[i].y < ly0 ? s->link[i].y : ly0;
        lx1 = s->link[i].x > lx1 ? s->link[i].x : lx1;
        ly1 = s->link[i].y > ly1 ? s->link[i].y : ly1;
        b->link[b->nlinks].kind = s->link[i].kind == O_LADDER_DOWN ? O_LADDER_UP : s->link[i].kind == O_SHAFT_DOWN ?
            O_SHAFT_UP : O_STAIRS_UP;
        b->link[b->nlinks].dug = s->link[i].dug;
        b->link[b->nlinks].to = n;
        b->link[b->nlinks].x = s->link[i].x;
        b->link[b->nlinks++].y = s->link[i].y;
    }
    bounds (b, lx0, ly0, lx1, ly1);
    ask_maker (M_PLAN, n + 1, 0, 0);
}

/* The link in the character's room the way d (up or down): its index in the stratum, the time to walk to it, or
** 0xFF */
static unsigned char __fastcall__ link_here (unsigned char d, unsigned long* t)
{
    unsigned char r, k, kind, best = 0xFF;
    unsigned long bt = 0xFFFFFFFFUL, u;

    if ((r = room_of (sn_, mx, my)) == 0xFF) {
        return 0xFF;
    }
    for (k = 0; k < C->nopen; ++k) {
        kind = C->open[k].kind;
        if ((d == DIR_UP) != (kind == O_STAIRS_UP || kind == O_LADDER_UP || kind == O_WAY_OUT || kind == O_SHAFT_UP)) {
            continue;
        }
        if ((u = cross_room (r, cx_, cy_, C->open[k].x, C->open[k].y, dark () ? 1 : 0)) < bt) {
            bt = u;
            best = C->open[k].link;
        }
        at (sn_, mx, my);                       /* (Its chunk in the window again) */
    }
    *t = bt;
    return best;
}

/* The stairway or ladder in the room, and up or down it */
static void __fastcall__ climb (unsigned char d)
{
    unsigned long t;
    unsigned char l;
    struct link* lk;

    sn_ = HUB->pc_stratum;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    stept = HUB->pcsum.step ? HUB->pcsum.step : 10;
    l = link_here (d, &t);
    hub_map (0);
    if (l == 0xFF || t == 0xFFFFFFFFUL) {
        no_way (N_NO_STAIRS);
        return;
    }
    lk = &strata[sn_].link[l];
    if (lk->kind == O_WAY_OUT) {
        no_way (N_SHUT);                        /* (The way out: the puzzle's, later) */
        return;
    }
    if (!strata[lk->to].made) {
        grow_below (sn_);
    }
    move.on = 1;
    move.how = d == DIR_UP ? A_UP : A_DOWN;
    move.sn = lk->to;
    move.x = lk->x;
    move.y = lk->y;
    move.face = HUB->pc_face;
    move.dir = lk->kind;
    /* (The flight: the drop between the floors, a step's time a foot on a stairway, twice on a ladder) */
    t += (unsigned long) (abs (strata[sn_].datum - strata[lk->to].datum) + 1) * stept *
        (lk->kind == O_LADDER_DOWN || lk->kind == O_LADDER_UP ? 2 : lk->kind == O_SHAFT_DOWN ||
        lk->kind == O_SHAFT_UP ? 3 : 1) / (d == DIR_UP ? 1 : 2);
    hub_map (0);
    busy (t > 60000UL ? 60000U : (unsigned) t);
}

/* The heading from (mx, my) to the middle of its room (as the character comes into a stratum: it looks in) */
static unsigned char toward_middle (void)
{
    unsigned char r = room_of (sn_, mx, my);
    int dx, dy;

    if (r == 0xFF) {
        return H_S;
    }
    dx = (int) (C->room[r].x + C->room[r].w / 2) - cx_;
    dy = (int) (C->room[r].y + C->room[r].h / 2) - cy_;
    return dx || dy ? bearing (dx, dy) : H_S;
}

static void dig_on (void);                     /* (Digging's: below) */

/* A move done: the character there, what it saw seen, and the UI told */
static void arrive (void)
{
    unsigned char r, first = 0, how = move.how;

    move.on = 0;
    if (how == A_DIGGING) {                     /* (A spell of digging done) */
        dig_on ();
        return;
    }
    sn_ = move.sn;
    mx = move.x;
    my = move.y;
    if (how == A_WALKED) {
        walk (move.dir, 1, dark ());
    } else {
        move.face = toward_middle ();
    }
    if ((r = room_of (sn_, mx, my)) != 0xFF) {
        first = !(C->room[r].flags & R_VISITED);
        C->room[r].flags |= R_VISITED;
    }
    hub_map (0);
    HUB->pc_stratum = sn_;
    HUB->pc_x = mx;
    HUB->pc_y = my;
    HUB->pc_face = move.face;
    see (first);
    event (E_ARRIVE, 0, how | first << 8 | (how == A_UP || how == A_DOWN ? (unsigned) move.dir << 12 : 0));
}

/* The dungeon begun: the top stratum's record (a stand-in for the theme's say: a few hundred feet each way), its way
** in; its plan the maker's */
static void begin (void)
{
    struct stratum* s;
    unsigned long seed;

    hub_map (0);
    s = &HUB->strata[0];
    seed = HUB->seed;
    rseed (seed ^ 0x57524C44UL);
    memset (s, 0, sizeof *s);
    s->seed = seed * 2654435761UL + 12345;
    s->style = ST_BUILT;
    s->age = 1 + random (20);
    s->name = random (8);
    s->w = 120 + random (180);
    s->h = 80 + random (120);
    s->x0 = 32768U - s->w / 2;
    s->y0 = 32768U - s->h / 2;
    s->nlinks = 1;
    s->link[0].kind = O_WAY_OUT;
    s->link[0].x = s->x0 + 4 + random (s->w - 8);
    s->link[0].y = s->y0 + 4 + random (s->h - 8);
    ask_maker (M_PLAN, 0, 0, 0);
    move.how = A_START;                         /* (The character at the foot of the way in) */
    move.sn = 0;
    move.x = strata[0].link[0].x;
    move.y = strata[0].link[0].y;
    hub_map (0);
    if (HUB->test_start && strata[0].nlinks > 1) {  /* (dq0 -T: at the first way down, for the tests) */
        move.x = strata[0].link[1].x;
        move.y = strata[0].link[1].y;
    }
}

/* ---- Digging (DUNGEON.md: changing the dungeon): a wall or the rock beside the character, or a shaft down through
** its floor; a spell of work at a time (ten minutes), the referee's test of it (P_DIG: HarnMaster's Mining) the feet
** it makes (a critical success three, a success two, a marginal failure one, a critical failure none), loud (a noise
** each spell, for the denizens); through, the square's a passage, or the shaft a way down to the stratum below (a
** link, both its ends: the stratum below grown if no one has been there; or solid rock beneath, if the stratum
** below doesn't reach under it) */

#define SPELL           6000                    /* Tenths: ten minutes' work */

static struct {
    unsigned char   d;                          /* DIR_N ... DIR_W, DIR_DOWN */
    unsigned char   mat;                        /* What it's into (rs.h's MAT_*: the referee judges the tools) */
    unsigned        need, done;                 /* Feet */
    unsigned        x, y;                       /* The square dug */
} dig;

/* The referee's test of the character's digging (by what it has equipped): its grade, and the feet it made (made
** 0xFF: nothing it has can dig that) */
static unsigned char made;
static signed char test_dig (void)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_DIG;
    r->b = dig.mat;
    r->steps = 0;
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    made = r->result ? 0xFF : r->w1;
    r->state = REQ_FREE;
    return g;
}

static void spell (void)
{
    move.on = 1;
    move.how = A_DIGGING;
    move.sn = sn_;
    move.x = HUB->pc_x;
    move.y = HUB->pc_y;
    move.face = HUB->pc_face;
    busy (SPELL);
}

/* A change asked of the maker (kept, and made in the chunk) */
static void __fastcall__ change (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link)
{
    hub_map (0);
    HUB->mreq[ASK_WORLD].tile = tile;
    HUB->mreq[ASK_WORLD].elev = elev;
    HUB->mreq[ASK_WORLD].link = link;
    ask_maker (M_CHANGE, sn, x, y);
}

/* A link added to stratum sn's record (the hub's), its index */
static unsigned char __fastcall__ add_link (unsigned char sn, unsigned char kind, unsigned char to, unsigned x,
    unsigned y)
{
    struct stratum* s;
    struct link* l;

    hub_map (0);
    s = &HUB->strata[sn];
    if (s->nlinks >= NLINKS) {
        return 0xFF;
    }
    l = &s->link[s->nlinks];
    l->kind = kind;
    l->to = to;
    l->x = x;
    l->y = y;
    l->dug = 1;
    return s->nlinks++;
}

static void __fastcall__ start_dig (unsigned char d)
{
    unsigned char* p;
    unsigned char t;

    sn_ = HUB->pc_stratum;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    dig.d = d;
    dig.done = 0;
    if (d == DIR_DOWN) {
        if (sn_ + 1 >= MAXSTRATA || room_of (sn_, mx, my) == 0xFF) {
            hub_map (0);
            no_way (N_NO_DIG);                  /* (A shaft from a room's floor, and not from the deepest) */
            return;
        }
        dig.x = mx;
        dig.y = my;
        if (TILE (*at (sn_, mx, my)) != T_FLOOR) {  /* (On a stairway, a doorway: a plain floor beside it) */
            for (t = 0; t < 8; ++t) {
                if ((p = at (sn_, mx + head_x[t], my + head_y[t])) != 0 && TILE (*p) == T_FLOOR &&
                    room_of (sn_, mx + head_x[t], my + head_y[t]) != 0xFF) {
                    dig.x = mx + head_x[t];
                    dig.y = my + head_y[t];
                    break;
                }
            }
            if (t == 8) {
                hub_map (0);
                no_way (N_NO_DIG);
                return;
            }
            hub_map (0);
            HUB->pc_x = dig.x;                  /* (It steps aside to dig) */
            HUB->pc_y = dig.y;
        }
        if (!strata[sn_ + 1].made) {
            grow_below (sn_);                   /* (Grown now: the shaft goes down to its floors, if it reaches under) */
            mx = dig.x;
            my = dig.y;
        }
        dig.need = abs (strata[sn_].datum - strata[sn_ + 1].datum);
        dig.mat = MAT_STONE;
    } else {
        dig.x = mx + dir_x[d];
        dig.y = my + dir_y[d];
        p = at (sn_, dig.x, dig.y);
        t = p ? TILE (*p) : T_PIT;
        if (t != T_WALL && t != T_ROCK) {
            hub_map (0);
            no_way (N_NO_DIG);
            return;
        }
        dig.need = t == T_WALL ? 2 : 5;         /* (A wall's masonry, two feet; a square of rock, five) */
        dig.mat = t == T_WALL ? MAT_MASONRY : MAT_STONE;
    }
    hub_map (0);
    event (E_DIG, 0, D_BEGUN << 8 | (dig.need > 255 ? 255 : dig.need));
    spell ();
}

/* A spell of work done: its test, its noise, and on, or through */
static void dig_on (void)
{
    signed char g;
    unsigned char l, u, k;
    unsigned char* p;
    int z;

    sn_ = HUB->pc_stratum;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    g = test_dig ();
    if (made == 0xFF) {                         /* (Nothing it has will bite into it) */
        hub_map (0);
        event (E_DIG, 0, D_NO_TOOL << 8);
        return;
    }
    dig.done += made;
    hub_map (0);
    {
        static struct msg m;                    /* (The noise of it: loud, and carrying) */

        m.kind = E_NOISE;
        m.who = 0;
        m.a = 6;
        m.w1 = dig.x;
        m.w2 = dig.y;
        m.t = HUB->now;
        hub_event (&m);
    }
    if (dig.done < dig.need) {
        event (E_DIG, 0, (g == G_CS || g == G_MS ? D_GOOD : g == G_MF ? D_POOR : D_SLIP) << 8 |
            (dig.need - dig.done > 255 ? 255 : dig.need - dig.done));
        spell ();
        return;
    }
    if (dig.d != DIR_DOWN) {                    /* (Through: the square a passage, at the character's floor) */
        z = elev_at (sn_, mx, my);
        change (sn_, dig.x, dig.y, T_CORR, z, 0xFF);
        hub_map (0);
        event (E_DIG, 0, D_THROUGH << 8);
        see (0);
        return;
    }
    /* The shaft: a way down, if the stratum below reaches under it */
    if (strata[sn_ + 1].made && (dig.x - strata[sn_ + 1].x0 >= strata[sn_ + 1].w ||
        dig.y - strata[sn_ + 1].y0 >= strata[sn_ + 1].h)) {
        hub_map (0);
        event (E_DIG, 0, D_SOLID << 8);
        return;
    }
    l = add_link (sn_, O_SHAFT_DOWN, sn_ + 1, dig.x, dig.y);
    if (l == 0xFF) {
        hub_map (0);
        event (E_DIG, 0, D_SOLID << 8);
        return;
    }
    sync_strata ();
    z = elev_at (sn_, dig.x, dig.y);
    change (sn_, dig.x, dig.y, T_DOWN, z, l);   /* (Its head) */
    if (!strata[sn_ + 1].made) {
        grow_below (sn_);                       /* (Grown round it: a link up under each link down, this one's) */
        for (u = 0xFF, k = 0; k < strata[sn_ + 1].nlinks; ++k) {
            if (strata[sn_ + 1].link[k].x == dig.x && strata[sn_ + 1].link[k].y == dig.y) {
                u = k;
            }
        }
    } else {
        u = add_link (sn_ + 1, O_SHAFT_UP, sn_, dig.x, dig.y);
        sync_strata ();
    }
    if (u != 0xFF) {                            /* (Its foot: a hole in the ceiling below, into what's there) */
        z = (p = at (sn_ + 1, dig.x, dig.y)) != 0 && T_PASS (*p) ? C->elev[cy_][cx_] : 0;
        change (sn_ + 1, dig.x, dig.y, T_UP, z, u);
    }
    hub_map (0);
    event (E_DIG, 0, D_THROUGH << 8 | 1);
    see (0);
}

static void __fastcall__ intent (struct msg* m)
{
    switch (m->kind) {
    case I_WAIT:
        busy (m->w1 ? m->w1 : HUB->round);
        event (E_TIME, 0, m->w1 ? m->w1 : HUB->round);
        break;
    case I_GO:
        go (m->a & 7);
        break;
    case I_CLIMB:
        climb (m->a);
        break;
    case I_TURN:
        HUB->pc_face = m->a & 7;
        busy (stept);
        see (0);
        event (E_VIEW, 0, V_TURNED);
        break;
    case I_DIG:
        start_dig (m->a);
        break;
    case I_LIGHT:
        HUB->pc_light = m->a ? TORCH : 0;
        busy (m->a ? 3 * stept : stept);
        see (0);
        event (E_VIEW, 0, m->a ? V_LIT : V_DARK);
        break;
    case I_QUIT:
        HUB->quit = 1;
        break;
    }
}

/* The real clock's ticks since the last look, made game time at the pace */
static void clock_run (void)
{
    unsigned t = hy_ticks (), d = t - last;

    last = t;
    if (HUB->paused || HUB->hold || !HUB->tpr) {
        return;
    }
    acc += (unsigned long) d * HUB->round;
    if (acc >= HUB->tpr) {
        advance (acc / HUB->tpr);
        acc %= HUB->tpr;
    }
}

int main (int argc, char* argv[])
{
    static struct msg m;
    unsigned char was_busy = 0;

    if (argc < 3 || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    chunks_init (ASK_WORLD);
    hub_map (0);
    HUB->pc_light = TORCH;                      /* (A torch lit: items come later) */
    stept = HUB->pcsum.step ? HUB->pcsum.step : 10;
    begin ();
    arrive ();
    event (E_READY, 0, 0);                      /* (The UI waits for it: its commands, the world started) */
    last = hy_ticks ();
    while (hub_map (0), !HUB->quit) {         /* (The hub at $8000 again, each time round) */
        while (hy_sem_try (HUB->sem[T_WORLD]) == 0) {
        }
        while (hub_take (&m)) {
            intent (&m);
            was_busy = 1;
        }
        clock_run ();
        if (move.on && HUB->now >= HUB->pc_ready) {
            arrive ();
        }
        if (was_busy && !move.on && HUB->now >= HUB->pc_ready) {
            event (E_READY, 0, 0);              /* (The character free again: the UI takes its next line) */
            was_busy = 0;
        }
        ++HUB->beat[T_WORLD];
        hy_sleep_ticks (2);
    }
    event (E_BYE, 0, 0);
    return 0;
}
