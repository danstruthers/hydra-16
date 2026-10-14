/*
** dq0w.c - Dingus Quest Zero's world: a task of its own, started by the UI (dq0w SEG ROLE: the hub's segment and its
** role there, hub.h).  It owns the game's clock, takes the actors' intents and carries them out, and tells what
** happened as events.  It makes the dungeon's levels (gen.c) as the character first comes to each, and moves the
** character through them: a move takes its time, and the character's there when the time's up.  What it sees is
** worked out as it gets there, turns, or its light changes (view.c): the UI tells that.
**
** The clock (main.c's once): each real tick (200 a second) is round tenths of game time over tpr ticks (the pace:
** a round in tpr ticks); paused (Escape) or held (the UI asking something), it stands.  At the turns pace (tpr 0)
** it moves only as the character acts: straight to when the character's free again.
*/

#include <stdlib.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/level.h"
#include "gen.h"
#include "view.h"

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

#define TILE_TIME       10                      /* Tenths of a second to walk a tile (5 feet), looking about (in
                                                **   the dark, feeling the way: twice that) */
#define TORCH           4                       /* A torch's light: 20 feet */

static struct {
    unsigned char   on;                         /* A move under way */
    unsigned char   how;                        /* A_* */
    unsigned char   dir;                        /* (A_WALKED's) */
    unsigned char   face;                       /* The way it'll face */
    unsigned char   level, x, y;                /* Where it ends */
} move;

static unsigned char lv, mx, my, mface;         /* (The character's level, and a walk's tile and heading) */
static struct level* L;

static void __fastcall__ level_in (unsigned char n)
{
    hub_map (LEVEL_BANK (n));
    L = LEVEL (n);
}

/* From (mx, my), heading d, to the next place: the tiles walked (mx, my where it ends); see: the tiles seen */
static unsigned char __fastcall__ walk (unsigned char d, unsigned char see)
{
    unsigned char n = 0, t, w, k, nd;

    while (n < 200) {
        t = L->tile[my + dir_y[d]][mx + dir_x[d]];
        if (!T_PASS (t)) {
            break;
        }
        mx += dir_x[d];
        my += dir_y[d];
        ++n;
        if (see) {
            L->tile[my][mx] |= T_SEEN;
        }
        if (T_INSIDE (t)) {                     /* (Through a door, into a room) */
            break;
        }
        if (TILE (t) == T_DOOR) {
            continue;
        }
        w = ways (L, mx, my) & ~(1 << ((d + 2) & 3));
        for (k = 0, nd = 0; k < 4; ++k) {
            if (w & (1 << k)) {
                ++nd;
                d = k;
            }
        }
        if (nd != 1) {                          /* (A fork, or the passage's end) */
            break;
        }
    }
    mface = d;
    return n;
}

static void __fastcall__ no_way (unsigned char why)
{
    event (E_NOWAY, 0, why);
    HUB->pc_ready = HUB->now;
}

static unsigned char __fastcall__ dist (unsigned char x, unsigned char y)
{
    return (x > mx ? x - mx : mx - x) + (y > my ? y - my : my - y);
}

/* The character's tile in the dark (the room's own light, or its own, would show it) */
static unsigned char dark (void)
{
    return !(L->tile[my][mx] & T_VIS);
}

/* What the character sees, made again (the hub's header in the window: as it ends, too) */
static void see (void)
{
    unsigned char face = HUB->pc_face, light = HUB->pc_light, sight = HUB->pcsum.sight;

    if (HUB->pcsum.cond & C_BLINDED) {
        sight = 0xFF;
    }
    lv = HUB->pc_level;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    level_in (lv);
    view (L, mx, my, face, light, sight);
    hub_map (0);
}

/* The character starts out north, east ... (d): its time, and where it'll be */
static void __fastcall__ go (unsigned char d)
{
    unsigned char r, n, t, slow, sx, sy;

    lv = HUB->pc_level;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    level_in (lv);
    slow = dark ();
    if ((r = room_at (L, mx, my)) != 0xFF) {
        if (!doors_on (L, r, d)) {
            hub_map (0);
            no_way (N_NO_EXIT);
            return;
        }
        t = dist (door_x, door_y);
        mx = door_x;
        my = door_y;
        sx = mx;
        sy = my;
        n = walk (d, 0);
    } else if (!(ways (L, mx, my) & (1 << d))) {
        hub_map (0);
        no_way (N_NO_EXIT);
        return;
    } else {
        t = 0;
        sx = mx;
        sy = my;
        n = walk (d, 0);
    }
    hub_map (0);
    move.on = 1;
    move.how = A_WALKED;
    move.dir = d;
    move.level = lv;
    move.x = sx;                                /* (Walked again from there as it ends: the tiles seen) */
    move.y = sy;
    move.face = mface;
    busy ((unsigned) (t + n) * (slow ? 2 * TILE_TIME : TILE_TIME));
}

/* The stairs in the room: to them, and up or down */
static void __fastcall__ climb (unsigned char d)
{
    unsigned char r, x, y;

    lv = HUB->pc_level;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    level_in (lv);
    r = room_at (L, mx, my);
    x = d == DIR_UP ? L->upx : L->downx;
    y = d == DIR_UP ? L->upy : L->downy;
    if (r == 0xFF || !x || room_at (L, x, y) != r) {
        hub_map (0);
        no_way (N_NO_STAIRS);
        return;
    }
    r = dist (x, y);
    hub_map (0);
    if (d == DIR_UP && !lv) {
        no_way (N_SHUT);                        /* (The way out: the puzzle's, later) */
        return;
    }
    move.on = 1;
    move.how = d == DIR_UP ? A_UP : A_DOWN;
    move.level = lv;
    move.face = HUB->pc_face;
    busy ((unsigned) r * TILE_TIME + 3 * TILE_TIME);
}

/* Level n, made if it isn't yet */
static void __fastcall__ make (unsigned char n)
{
    unsigned long seed;

    hub_map (0);
    seed = HUB->seed;
    level_in (n);
    if (!L->made) {
        gen_level (L, n, seed);
    }
}

/* The way from (mx, my) to the middle of its room (as the character first comes down the stairs: it looks in) */
static unsigned char toward_middle (void)
{
    unsigned char r = room_at (L, mx, my);
    int dx, dy;

    if (r == 0xFF) {
        return DIR_S;
    }
    dx = (int) (L->room[r].x + L->room[r].w / 2) - mx;
    dy = (int) (L->room[r].y + L->room[r].h / 2) - my;
    if (abs (dx) > abs (dy)) {
        return dx > 0 ? DIR_E : DIR_W;
    }
    return dy < 0 ? DIR_N : DIR_S;
}

/* A move done: the character there, what it saw seen, and the UI told */
static void arrive (void)
{
    unsigned char r, first = 0, how = move.how;

    move.on = 0;
    lv = move.level;
    if (how == A_WALKED) {
        mx = move.x;
        my = move.y;
        level_in (lv);
        walk (move.dir, 1);
    } else {
        lv += how == A_DOWN ? 1 : how == A_UP ? -1 : 0;
        make (lv);
        mx = how == A_UP ? L->downx : L->upx;
        my = how == A_UP ? L->downy : L->upy;
        if (how == A_START) {
            move.face = toward_middle ();
        }
    }
    if ((r = room_at (L, mx, my)) != 0xFF) {
        first = !(L->room[r].flags & R_VISITED);
        L->room[r].flags |= R_VISITED;
    }
    hub_map (0);
    HUB->pc_level = lv;
    HUB->pc_x = mx;
    HUB->pc_y = my;
    HUB->pc_face = move.face;
    see ();
    event (E_ARRIVE, 0, how | first << 8);
}

static void __fastcall__ intent (struct msg* m)
{
    switch (m->kind) {
    case I_WAIT:
        busy (m->w1 ? m->w1 : HUB->round);
        event (E_TIME, 0, m->w1 ? m->w1 : HUB->round);
        break;
    case I_GO:
        go (m->a & 3);
        break;
    case I_CLIMB:
        climb (m->a);
        break;
    case I_TURN:
        HUB->pc_face = m->a & 3;
        busy (TILE_TIME);
        see ();
        event (E_VIEW, 0, V_TURNED);
        break;
    case I_LIGHT:
        HUB->pc_light = m->a ? TORCH : 0;
        busy (m->a ? 3 * TILE_TIME : TILE_TIME);
        see ();
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
    HUB->pc_light = TORCH;                      /* (A torch lit: items come later) */
    move.how = A_START;                         /* (The character at the foot of the stairs it came down) */
    move.level = 0;
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
        if (was_busy && HUB->now >= HUB->pc_ready) {
            event (E_READY, 0, 0);              /* (The character free again: the UI takes its next line) */
            was_busy = 0;
        }
        ++HUB->beat[T_WORLD];
        hy_sleep_ticks (2);
    }
    event (E_BYE, 0, 0);
    return 0;
}
