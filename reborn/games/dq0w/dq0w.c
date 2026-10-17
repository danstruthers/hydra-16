/*
** dq0w.c - Dingus Quest Zero's world: a task of its own, started by the UI (dq0w SEG ROLE LIBDIR: the hub's segment,
** its role there, hub.h, and the game's lib folder, for its overlays' file).  It owns the game's clock, takes the
** actors' intents and carries them out, and tells what happened as events.  It keeps the dungeon (world.h: strata grown
** whole in outline as anyone first comes to them, plan.c; made a chunk at a time as anyone comes near, chunk.c and
** built.c), and moves the character through it: a move takes its time (the referee's walking rate, the slopes), and the
** character's there when the time's up.  What it sees is worked out as it gets there, turns, or its light changes, and
** what it perceives recorded in the hub for the UI to word, by the senses (games/dq0p, asked).  It carries out digging,
** a spell of work at a time (the referee's tests), and keeps what it changes: the change records (store.c, world.h's),
** its alone to write, which the maker lays over a chunk it makes again.  It works the portals and the traps (work.h:
** doors, portcullises, windows, a trap set off, disarmed, a search; the referee asked), and answers the UI's
** requests of it (the hub's wreq: the save, a close look at a portal).  It keeps the item store (things.c: the
** things, their places and states, its alone to write), answering the UI's requests of the things (the hub's ireq:
** what's carried, a thing taken up, put down, handled), putting down a chunk's things as the maker listed them
** stocking it.  The denizens near the character are minds (their task, games/dq0d) and bodies, the bodies the world's
** (the hub's actors, wbody.c, wfight.c): the denizens say what each means to do (their intents, the hub's di) and
** the world does it by the character's rules, as it does the character's, and tells what they do (the denizens'
** events); it keeps the character out of their squares, and looks again at those it sees as they change (den_seq).
** Every blow is the world's to strike, the character's at one (I_ATTACK: a round of its time, the hub's pc_foe) and
** theirs at it (the referee asked, in the world's slot).
**
** The pace (DUNGEON.md: moving): the player's (the hub's gait), held to a slow walk while it searches as it goes or
** in the dark, a walk while it listens; a square's time that pace's step (the referee's), the time moved at it and
** the time stood still the referee's to tire the character by and rest it (RS_EXERT).  Searching as it goes, it
** searches about each place it stops (a search's, quietly, in no more time); a trap underfoot noticed, by the
** referee's chance (a search's).  A small move (I_STEP): a square at a time, by the heading, as far
** as it's told or its pace takes it.
**
** The clock (main.c's once): each real tick (200 a second) is round tenths of game time over tpr ticks (the pace:
** a round in tpr ticks); paused (Escape) or held (the UI asking something), it stands.  At the turns pace (tpr 0)
** it moves only as the character acts: on to when the character's free again, a slice at a time as the denizens
** keep up (turns_run).
**
** Its code that's seldom called is in swapped parts (../dq0/swap.inc: run in the main RAM's top, a part at a time,
** copied there from the overlays' file's banks, LIBDIR/dq0w.ovl, as it's called; bank.s's stubs): part 1, digging;
** part 2, climbing; part 3, the dungeon begun and the strata below grown; part 4, the change records' rarer work
** (store.c); the work: part 5 doors (wdoor.c), 6 blows (wblow.c), 7 portcullises and windows (wwin.c), 8 traps and
** searches (wtrap.c), a command's work all in one of them; part 9, the walks (go, go_to, step_to: I_GO, I_GOTO,
** I_STEP, each a command's one part); the item store's, parts 10, 11 and 13 (things.c); part 12, a door's work begun
** (door_work: the door walked up to, its part's work asked); part 14, the denizens' bodies (wbody.c), part 15 the
** blows (wfight.c); part 16, the game's own rules (wgame.c: the theme's puzzle judged, IO_PUZZLE; a thing walked over
** to and taken up, I_TAKE).
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "wbody.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

/* ---- The swapped parts' stubs (bank.s's) */

void __fastcall__ start_dig (unsigned char d);  /* Part 1 */
void dig_on (void);
void __fastcall__ climb (unsigned char d);      /* Part 2 */
void climb_on (void);
void begin (void);                              /* Part 3 */
void __fastcall__ grow_below (unsigned char n);
void __fastcall__ go (unsigned char h);         /* Part 9 */
void __fastcall__ go_to (unsigned x, unsigned y);
void __fastcall__ step_to (unsigned char h, unsigned char n, unsigned char keep);
void __fastcall__ door_work (unsigned char what);   /* Part 12 */
#define OVBANKS         6                       /* (dq0w.cfg's OV1 ...: the parts' images) */
extern unsigned char ovbank[OVBANKS];
struct work wk;
extern char _SWAP_START__[];
unsigned char den_done;                         /* (wbody.h's) */
unsigned char den_saw;

static unsigned last;                           /* The tick count at the last look */
static unsigned acc;                            /* Real ticks times round, not yet game time */

/* The clock moved on by d tenths, written so that a reader sees it whole (hub_now) */
static void __fastcall__ advance (unsigned long d)
{
    ++HUB->tick;
    HUB->now += d;
    ++HUB->tick;
}

static unsigned char evb;                       /* (The next event's b: then 0) */
static void __fastcall__ event (unsigned char kind, unsigned char who, unsigned w1)
{
    static struct msg m;

    m.kind = kind;
    m.who = who;
    m.b = evb;
    evb = 0;
    m.w1 = w1;
    m.t = HUB->now;
    hub_event (&m);
}

/* The character busy for t tenths: free again at pc_ready (the turns pace: the clock moved on to it a slice at a
** time, turns_run), standing where it is unless a move says where it's going (bound_for) */
static unsigned char late;                      /* (The turns pace's: the denizens' task slow to keep up) */

static unsigned long free_at;                   /* (When it was last free of work: its rest from then, RS_EXERT's) */
static unsigned went;                           /* (The last thing's time: a move's, as it's done) */

static void __fastcall__ busy (unsigned long t)
{
    went = t > 60000UL ? 60000U : (unsigned) t;
    free_at = HUB->pc_ready = HUB->now + went;
    HUB->pc_tx = HUB->pc_x;
    HUB->pc_ty = HUB->pc_y;
    late = 0;
}

static void __fastcall__ bound_for (unsigned x, unsigned y)
{
    HUB->pc_tx = x;
    HUB->pc_ty = y;
}

/* ---- The denizens in the character's way: the actors' squares on its stratum, as a move's worked out (dens_where)
** and made; none is walked onto, nor walked through */

static unsigned char nden, dk[NACTORS], blocked;    /* (blocked: the actor that stopped it, plus 1; or 0) */
static unsigned dsx[NACTORS], dsy[NACTORS];
static unsigned rx0, ry0;                       /* (The room's chunk's first square, in the frame: can ()'s) */

static void dens_where (void)
{
    struct actor* q;
    unsigned char k;

    hub_map (0);
    for (k = nden = 0, q = HUB->actor; k < NACTORS; ++k, ++q) {
        if (q->state && q->sn == HUB->pc_stratum) {
            dk[nden] = k;
            dsx[nden] = q->x;
            dsy[nden++] = q->y;
        }
    }
    blocked = 0;
}

/* The actor on square x, y (dens_where's), plus 1; or 0 */
static unsigned char __fastcall__ den_on (unsigned x, unsigned y)
{
    unsigned char i;

    for (i = 0; i < nden; ++i) {
        if (dsx[i] == x && dsy[i] == y) {
            return dk[i] + 1;
        }
    }
    return 0;
}

/* ---- The character's moves: worked out as they start (where to, how long), and made when they're done */

static struct {
    unsigned char   on;                         /* A move under way */
    unsigned char   how;                        /* A_* */
    unsigned char   dir;                        /* (A_WALKED's: the passage's way out of the room, or on) */
    unsigned char   face;                       /* The heading it'll face */
    unsigned char   sn;                         /* Where its walk begins (A_WALKED's), or where it ends */
    unsigned        x, y;
    signed char     ledge;                      /* (A_CROSSED's: the ledge that stopped it, or 0) */
    unsigned char   den;                        /* (The denizen that stopped it, dens_where's blocked; or 0) */
} move;

static unsigned char sn_, mface;                /* (The character's stratum; a walk's heading as it ends) */
static unsigned char stept = 10;                /* (The referee's step, tenths a square: from the hub as a move
                                                **   starts, for the chunks are in the window as it's walked) */
static unsigned mx, my;                         /* (A walk's square) */

/* Where the character is, as a move or a piece of work starts: its stratum, its square, its step */
static void here (void)
{
    sn_ = HUB->pc_stratum;
    mx = HUB->pc_x;
    my = HUB->pc_y;
    stept = HUB->pcsum.step ? HUB->pcsum.step : 10;
}

/* The pace a move goes at (from where it stands: here ()'s), the hub's pc_going from now till it's there and has
** looked about: the player's, held to a slow walk while it searches or maps as it goes or feels its way (dark ()), a
** walk while it listens; its step (pst: a slower pace's a walk's, its time mul_ times as much, the referee's: a slow
** walk's twice a walk's, as the dark's always was, a creep's three times) */
static unsigned char pace_, mul_ = 1, searching;
static unsigned char lim_ = 250;                /* (A walk's squares at most: a small move's, I_STEP's, fewer) */
static unsigned char pst = 10;

static unsigned char dark (void);

static void paced (void)
{
    unsigned char dk = dark (), g, w;

    hub_map (0);
    g = HUB->gait;
    w = HUB->watch;
    searching = w & WA_TRAPS;
    if ((dk || (w & (WA_TRAPS | WA_MAP))) && g > GA_SLOW) {
        g = GA_SLOW;
    } else if ((w & WA_LISTEN) && g > GA_WALK) {
        g = GA_WALK;
    }
    pace_ = g;
    mul_ = g < GA_WALK ? GA_WALK + 1 - g : 1;
    w = HUB->pcsum.gstep[g < GA_WALK ? GA_WALK : g];
    pst = w ? w : 10;
    HUB->pc_going = g + 1 | (dk ? GOING_DARK : 0);
}

/* A square's time to walk into, tenths of a second: the pace's step (the referee's: HarnMaster's walking, half its
** Move a round, and the rest), more uphill (half again a foot of rise), a little more steeply downhill (a quarter a
** foot past the first), more on steps (half again); a slower pace's (a slow walk's: feeling the way in the dark)
** times as many */
static unsigned __fastcall__ cost (int dz, unsigned char steps)
{
    unsigned base = pst, c = base;

    if (dz > 0) {
        c += base * dz / 2;
    } else if (dz < -1) {
        c += base * (-dz - 1) / 4;
    }
    if (steps) {
        c += base / 2;
    }
    return c * mul_;
}

/* From (mx, my), heading d, to the next place: the time it takes (mx, my where it ends, mface its heading; walked
** its squares: the senses remember them felt, the look's walk) */
static unsigned char walked;
static unsigned long __fastcall__ walk (unsigned char d)
{
    unsigned long t = 0;
    unsigned char n = 0, w, k, nd, inside;
    unsigned char* p;
    int z;

    while (n < lim_) {
        if (!step_ok (sn_, mx, my, d) || (blocked = den_on (mx + dir_x[d], my + dir_y[d])) != 0) {
            break;
        }
        z = elev_at (sn_, mx, my);
        mx += dir_x[d];
        my += dir_y[d];
        ++n;
        p = at (sn_, mx, my);
        t += cost (C->elev[cy_][cx_] - z, TILE (*p) == T_STEPS);
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
    walked = n;
    return t;
}

static void __fastcall__ no_way (unsigned why)
{
    event (E_NOWAY, 0, why);
    HUB->pc_ready = HUB->now;
    HUB->pc_going = 0;
}

/* Stopped by a denizen in its way before it's gone a square (told: the actor's) */
static void den_way (void)
{
    no_way (N_DEN | (unsigned) (blocked - 1) << 8);
}

/* ---- Traps on the way (DUNGEON.md: traps; set off by the world's work, wtrap.c): a walk worked out stops at the
** first it would set off (a plate's, a wire's, on a room's floor), and goes round one the character knows of */

static unsigned tx_, ty_, fx_, fy_;             /* (The trap's square, and the one it's walked onto from) */
static unsigned char cut;                       /* (The way worked out last stops at it, cut_t tenths on) */
static unsigned long cut_t;
static void on_way (void);
static void stopped (void);

/* C's trap on its square x, y that's set, and set off as it's walked on; or NULL */
static struct trap* __fastcall__ trap_at (unsigned char x, unsigned char y)
{
    struct trap* t = C->trap;
    unsigned char n;

    for (n = C->ntraps; n; --n, ++t) {
        if (t->x == x && t->y == y && TS_SET (t->state) && t->trigger != TT_DOOR) {
            return t;
        }
    }
    return 0;
}

/* ---- The way across a room, round its ledges: the squares of its chunk (in the window) from the room's floor to
** one of its doors or openings, by steps that can be walked; the time it takes */

#define BW      14
#define BH      9
static unsigned char bq[BW * BH], bfrom[BH][BW];

static unsigned char __fastcall__ can (unsigned char x, unsigned char y, unsigned char d)
{
    unsigned char nx = x + dir_x[d], ny = y + dir_y[d], t = C->tile[y][x], u;
    struct trap* q;
    int dz;

    if (nx >= CW || ny >= CW) {
        return 0;
    }
    u = C->tile[ny][nx];
    if (!T_PASS (u) || den_on (rx0 + nx, ry0 + ny) || ((q = trap_at (nx, ny)) != 0 && !(q->state & TS_HIDDEN))) {
        return 0;                               /* (Nor onto a denizen, nor a trap it knows of: it goes round) */
    }
    dz = C->elev[ny][nx] - C->elev[y][x];
    if (dz < 0) {
        dz = -dz;
    }
    return dz <= (TILE (t) == T_STEPS || TILE (u) == T_STEPS ? STAIR_MAX : STEP_MAX);
}

/* From (fx, fy) to (tx, ty) within room r's floor and walls (chunk squares): its time, or 0xFFFFFFFF (no way) */
static unsigned char last_d;                    /* (cross_room's way into its last square) */

static unsigned long __fastcall__ cross_room (unsigned char r, unsigned char fx, unsigned char fy, unsigned char tx,
    unsigned char ty)
{
    struct room* p = &C->room[r];
    unsigned char ox = p->x - 1, oy = p->y - 1, w = p->w + 2, h = p->h + 2, head = 0, tail = 0, x, y, d, nx, ny;
    unsigned long t = 0;

    cut = 0;
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
    last_d = bfrom[y][x];                       /* (The way it came into the square: a mouth's way on) */
    while ((d = bfrom[y][x]) != 4) {           /* (Back along the way, its steps' times) */
        nx = x - dir_x[d];
        ny = y - dir_y[d];
        if (trap_at (x + ox, y + oy)) {
            cut = 1;                            /* (A trap it would set off: the first on the way, the last found) */
            cut_t = t;
            tx_ = x + ox;
            ty_ = y + oy;
            fx_ = nx + ox;
            fy_ = ny + oy;
        }
        t += cost (C->elev[y + oy][x + ox] - C->elev[ny + oy][nx + ox], TILE (C->tile[y + oy][x + ox]) == T_STEPS);
        x = nx;
        y = ny;
    }
    cut_t = t - cut_t;                          /* (The time to the trap, from where it set out) */
    return t;
}

/* The character's square in the dark (the room's own light, or its own, would show it) */
static unsigned char dark (void)
{
    return !(*at (sn_, mx, my) & T_VIS);
}

/* The denizens the character sees looked at again (the senses asked: their bodies the world's, which are seen the
** senses' to decide, into what it perceives): 1 if one's come into sight, gone from it, or doing otherwise */
static unsigned char __fastcall__ ask_look (unsigned char kind)
{
    hub_map (0);
    HUB->look.kind = kind;
    hub_call (&HUB->look, T_SENSES);
    HUB->look.state = REQ_FREE;
    HUB->look.kind = LK_LOOK;
    return HUB->look.ch;
}
#define dens_seen()     ask_look (LK_DENS)

/* The denizens the character sees looked at again, as they've changed (their task's den_seq, or a step of one's the
** world made), the UI told if it sees them otherwise (not on its way somewhere: it looks about as it gets there) */
static void stirred (void)
{
    if ((!move.on || move.how == A_DIGGING) && dens_seen ()) {
        event (E_VIEW, 0, V_STIR);
    }
}

/* ---- The things (the item store: things.c): the UI's request of them answered (the chunks stocked first: what it
** asks about there), looked about again if what lies about has changed (as the senses did; the denizens it sees
** with it); while a look's waited for, the senses' request (a chunk's things put down, before they see
** them) and what the UI asks that wants no look (what's carried, the tools, a tool's wear: the hub's header in the
** window after each) */
static void clock_run (void);
static unsigned char __fastcall__ looked (unsigned char first);
static void __fastcall__ see (unsigned char first);
static void things (void)
{
    stocks ();
    if (HUB->ireq.kind == IO_PUZZLE ? puzzle () : serve ()) {
        see (0);                                /* (The puzzle's the world's rules: wgame.c) */
    }
    hub_reply (&HUB->ireq);
}

static void waiting (void)
{
    unsigned char op;

    clock_run ();
    if (hub_posted (&HUB->stock)) {
        stocks ();
        hub_reply (&HUB->stock);
    }
    op = HUB->ireq.kind;
    if (hub_posted (&HUB->ireq) && (op == IO_LIST || op == IO_TOOLS || op == IO_WEAR)) {
        serve ();
        hub_reply (&HUB->ireq);
    }
}

/* The senses asked to look about (the denizens it sees with it), and waited for, the things it saw made their own;
** the first time there, the reply's (LF_ARRIVE: by the senses' memory of the room) */
static unsigned char __fastcall__ looked (unsigned char first)
{
    hub_map (0);
    HUB->look.first = first;
    hub_post (&HUB->look, T_SENSES);
    while (!hub_wait_t (&HUB->look, 1)) {
        waiting ();
    }
    HUB->look.state = REQ_FREE;
    HUB->look.wn = 0;
    first = HUB->look.first;
    owned ();
    return first;
}

/* What the character sees, made again, and what it perceives recorded: the senses asked (dq0p), and waited for (the
** clock going on meanwhile, at the real paces: asleep on the doorbell a tick at a time), the denizens it sees added
** (the hub's header in the window as it ends) */
static void __fastcall__ see (unsigned char first)
{
    looked (first);
    sn_ = HUB->pc_stratum;
}

/* Across room r on heading h (no door that way): as far as its floor can be walked, a ledge stopping it (told); a
** small move's (I_STEP's) no more than lim_ squares, and not on along a passage */

static void __fastcall__ across (unsigned char h, unsigned char r)
{
    unsigned long t = 0;
    unsigned char n = 0, st;
    signed char ledge = 0;
    unsigned char* p;
    struct trap* q;
    unsigned nx, ny;
    int z, dz;

    while (n < lim_) {
        nx = mx + head_x[h];
        ny = my + head_y[h];
        p = at (sn_, mx, my);
        st = TILE (*p) == T_STEPS;
        z = C->elev[cy_][cx_];
        if (room_of (sn_, nx, ny) != r || !(p = at (sn_, nx, ny)) || !T_PASS (*p)) {
            if (lim_ == 250 && !(h & 1) && p && (TILE (*p) == T_CORR || TILE (*p) == T_DOOR) &&
                step_ok (sn_, mx, my, h >> 1)) {
                hub_map (0);                    /* (A passage's mouth: on along it, to the next place) */
                move.on = 1;
                move.how = A_WALKED;
                move.dir = h >> 1;
                move.sn = sn_;
                move.x = mx;
                move.y = my;
                t += walk (h >> 1);
                move.face = H_OF (mface);
                hub_map (0);
                if (!n && blocked && mx == move.x && my == move.y) {
                    move.on = 0;
                    den_way ();
                    return;
                }
                busy (t);
                bound_for (mx, my);
                return;
            }
            break;
        }
        dz = C->elev[cy_][cx_] - z;
        st |= TILE (*p) == T_STEPS;
        if ((dz < 0 ? -dz : dz) > (st ? STAIR_MAX : STEP_MAX)) {
            ledge = dz > 120 ? 120 : dz < -120 ? -120 : dz;
            break;
        }
        if ((blocked = den_on (nx, ny)) != 0) {
            break;
        }
        if ((q = trap_at (cx_, cy_)) != 0) {
            if (!(q->state & TS_HIDDEN)) {
                break;                          /* (A trap it knows of: it stops short of it) */
            }
            cut_t = t + cost (dz, st);
            tx_ = cx_;
            ty_ = cy_;
            fx_ = cx_ - head_x[h];
            fy_ = cy_ - head_y[h];
            on_way ();
            return;
        }
        t += (unsigned long) cost (dz, st) * ((h & 1) ? 7 : 5) / 5;
        mx = nx;
        my = ny;
        ++n;
    }
    hub_map (0);
    if (!n) {
        if (ledge) {
            HUB->pc_face = h;                   /* (It turns to face the ledge) */
            event (E_LEDGE, 0, (unsigned char) ledge);
            HUB->pc_ready = HUB->now;
        } else {
            if (blocked) {
                den_way ();
            } else {
                no_way (N_NO_EXIT);
            }
        }
        return;
    }
    move.on = 1;
    move.how = A_CROSSED;
    move.sn = sn_;
    move.x = mx;
    move.y = my;
    move.face = h;
    move.ledge = ledge;
    move.den = blocked;
    busy (t);
    bound_for (mx, my);
}

#pragma code-name (push, "SW9CODE")
#pragma rodata-name (push, "SW9RODATA")

/* Up to square (x, y) of the character's room (a door's or a passage's mouth: the room's square before it), by the
** way it can walk */
void __fastcall__ s_go_to (unsigned x, unsigned y)
{
    unsigned char r, tx, ty, d, t, fx, fy;
    unsigned long tt;
    unsigned char* p;

    dens_where ();
    here ();
    paced ();
    if ((r = room_of (sn_, mx, my)) == 0xFF || x - (mx - cx_) >= CW || y - (my - cy_) >= CW) {
        hub_map (0);
        no_way (N_NO_EXIT);
        return;
    }
    fx = cx_;
    fy = cy_;
    rx0 = mx - fx;
    ry0 = my - fy;
    tx = x - (mx - cx_);
    ty = y - (my - cy_);
    t = TILE (C->tile[ty][tx]);
    if (t == T_DOOR || t == T_CORR || t == T_WINDOW) {  /* (Up to it: the room's square before it) */
        for (d = 0; d < 4; ++d) {
            p = &C->tile[ty - dir_y[d]][tx - dir_x[d]];
            if (T_INSIDE (*p)) {
                tx -= dir_x[d];
                ty -= dir_y[d];
                break;
            }
        }
    }
    if ((tt = cross_room (r, fx, fy, tx, ty)) == 0xFFFFFFFFUL) {
        d = nden;                               /* (A way, but for the denizens: one's in the way) */
        nden = 0;
        tt = cross_room (r, fx, fy, tx, ty);
        nden = d;
        blocked = d ? dk[0] + 1 : 0;
        hub_map (0);
        if (tt != 0xFFFFFFFFUL && blocked) {
            den_way ();
        } else {
            no_way (N_NO_EXIT);
        }
        return;
    }
    if (cut) {
        on_way ();                              /* (A trap on its way) */
        return;
    }
    hub_map (0);
    move.on = 1;
    move.how = A_WENT_TO;
    move.sn = sn_;
    move.x = mx - fx + tx;
    move.y = my - fy + ty;
    move.face = tx == x - (mx - fx) && ty == y - (my - fy) ? HUB->pc_face : bearing ((int) x - move.x, (int) y - move.y);
    move.ledge = 0;
    busy (tt);
    bound_for (move.x, move.y);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Climbing a ledge (HarnMaster's Climbing, Skills 8: feet of progress a test, a test a round, the game's
** choice; a hard climb, rough stone: 20 feet on a critical success, 10 on a success, 5 back on a marginal failure,
** and on a critical failure a fall, the referee's harm); or jumping down one */

#define ROUND_T         100                     /* Tenths: a round */

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* A fall of the character's (the referee's harm, rs.h's HC_FALL): feet, onto the floor where it lands (the maker's
** material for it: earth, flags or masonry hard, stone broken rock), how (HF_*); the UI told (E_HARM).  The tenths
** it's out for (knocked senseless), or 0xFFFF: a hold grabbed, no fall */
static unsigned __fastcall__ fall (unsigned feet, unsigned char how)
{
    struct req* r;
    unsigned out;
    unsigned char k, m = MAT_STONE;

    hub_map (0);
    if (at (sn_, HUB->pc_x, HUB->pc_y)) {
        k = room_of (sn_, HUB->pc_x, HUB->pc_y);
        m = C->mat[k == 0xFF ? NROOMS : k][MP_FLOOR];
    }
    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_HARM;
    r->actor = 0;
    r->other = 0;
    r->a = HC_FALL;
    r->b = m == MAT_EARTH ? GR_FLOOR : m == MAT_STONE ? GR_ROCKY : GR_HARD;
    r->c = how;
    r->w1 = feet;
    r = hub_ask (SLOT_WORLD);
    out = r->result == 1 ? 0xFFFF : r->w2;
    event (E_HARM, 0, HC_FALL | r->b << 8 | (r->result == 1 ? E_CAUGHT : 0) | (r->w2 ? E_OUT : 0));
    r->state = REQ_FREE;
    return out;
}

static struct {
    unsigned char   up;
    unsigned        need, done;                 /* Feet */
    unsigned        x, y;                       /* The square at its top (up) or its foot (down) */
} cl;

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The referee asked about the character (kind, a, b; its w1 ww1, its steps wsteps, 0 after): the reply's grade (wres
** its result) */
unsigned char wres;
signed char wsteps;
static unsigned ww1;
signed char __fastcall__ ask_w (unsigned char kind, unsigned char a, unsigned char b)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = kind;
    r->actor = 0;
    r->a = a;
    r->b = b;
    r->w1 = ww1;
    r->steps = wsteps;
    wsteps = 0;
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    wres = r->result;
    r->state = REQ_FREE;
    return g;
}

#define test_w(p, b)    ask_w (RS_TEST, (p), (b))

/* A spell of work where the character stands (how: A_CLIMBING, A_DIGGING), t tenths */
static void __fastcall__ in_place (unsigned char how, unsigned t)
{
    move.on = 1;
    move.how = how;
    move.sn = sn_;
    move.x = HUB->pc_x;
    move.y = HUB->pc_y;
    move.face = HUB->pc_face;
    busy (t);
}

static void climb_round (void)
{
    in_place (A_CLIMBING, ROUND_T);
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* The ledge ahead of the character, up (or down): its climb begun (jump: jumped down it), or 0 (none) */
static unsigned char __fastcall__ start_ledge (unsigned char up, unsigned char face, unsigned char jump)
{
    unsigned char* p;
    unsigned nx, ny;
    int z, dz;

    nx = mx + head_x[face];
    ny = my + head_y[face];
    z = elev_at (sn_, mx, my);
    if (!(p = at (sn_, nx, ny)) || !T_PASS (*p)) {
        return 0;
    }
    dz = C->elev[cy_][cx_] - z;
    if (up ? dz <= STEP_MAX : dz >= -STEP_MAX) {
        return 0;
    }
    cl.up = up;
    cl.need = dz < 0 ? -dz : dz;
    cl.done = 0;
    cl.x = nx;
    cl.y = ny;
    hub_map (0);
    if (jump && !up) {                          /* (Jumped: a fall to its foot) */
        event (E_CLIMB, 0, K_JUMPED << 8 | (cl.need > 255 ? 255 : cl.need));
        HUB->pc_x = nx;
        HUB->pc_y = ny;
        busy (stept + fall (cl.need, HF_CLEAR));
        see (0);
        return 1;
    }
    event (E_CLIMB, 0, K_BEGUN << 8 | (cl.need > 255 ? 255 : cl.need));
    climb_round ();
    return 1;
}

/* A round of climbing done: its test, and on, back, fallen, or there */
void s_climb_on (void)
{
    signed char g;
    unsigned t;

    sn_ = HUB->pc_stratum;
    g = test_w (P_CLIMB, 0);
    hub_map (0);
    if (g == G_CF) {                            /* (A fall: from as high as it got, up; to the foot, down) */
        t = cl.up ? cl.done : cl.need - cl.done;
        event (E_CLIMB, 0, K_FELL << 8 | t);
        if ((t = fall (t, HF_CLEAR | HF_HOLDS)) == 0xFFFF) {
            climb_round ();                     /* (Caught on the way down: the climb goes on) */
            return;
        }
        hub_map (0);
        if (!cl.up) {
            HUB->pc_x = cl.x;
            HUB->pc_y = cl.y;
        }
        busy (t);
        see (0);
        return;
    }
    cl.done = g == G_CS ? cl.done + 20 : g == G_MS ? cl.done + 10 : cl.done > 5 ? cl.done - 5 : 0;
    if (cl.done < cl.need) {
        event (E_CLIMB, 0, (g == G_MF ? K_BACK : K_ON) << 8 | (cl.need - cl.done > 255 ? 255 : cl.need - cl.done));
        climb_round ();
        return;
    }
    HUB->pc_x = cl.x;
    HUB->pc_y = cl.y;
    event (E_CLIMB, 0, (cl.up ? K_UP : K_DOWN) << 8);
    see (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The door or the passage's mouth the UI means (I_GO's and I_PORTAL's w1, w2: its square, the frame's; tgx 0, any:
** the nearest that way, the one faced) */
static unsigned tgx, tgy;
#define TARGET(x, y)    (tgx ? (x) != tgx || (y) != tgy : 0)   /* (Square x, y not the one meant) */

#pragma code-name (push, "SW9CODE")
#pragma rodata-name (push, "SW9RODATA")

/* The character sets out on heading h (north, northeast ...): its time, and where it'll be.  From a room: north,
** east, south or west by a door in that wall (the nearest it can walk to); a diagonal, by the door that lies that
** way; across the room by the way it can walk; the UI's door or mouth, if it means one (tgx).  A passage goes only by
** the four */
void __fastcall__ s_go (unsigned char h)
{
    unsigned char r, d = h >> 1, k, best = 0xFF, bx = 0, by = 0, wx, wy;
    unsigned long t, bt = 0xFFFFFFFFUL;
    unsigned ox, oy;
    struct portal* q;

    dens_where ();
    here ();
    paced ();
    if ((r = room_of (sn_, mx, my)) != 0xFF) {
        rx0 = ox = mx - cx_;                    /* (The chunk's first square, in the frame) */
        ry0 = oy = my - cy_;
        wx = cx_;
        wy = cy_;
        for (k = 0; k < 4; ++k) {               /* (The doors that way: the quickest to walk to) */
            for (d = 0; wall_tile (&C->room[r], k, d); ++d) {
                if (TILE (C->tile[wall_y][wall_x]) != T_DOOR || (tgx ? TARGET (ox + wall_x, oy + wall_y) : (h & 1) ?
                    bearing ((int) wall_x - wx, (int) wall_y - wy) != h : k != (h >> 1)) ||
                    (portal_at (wall_x, wall_y) && (portal_at (wall_x, wall_y)->state & PS_HIDDEN))) {
                    continue;
                }
                if ((t = cross_room (r, wx, wy, wall_x, wall_y)) < bt) {
                    bt = t;
                    best = k;
                    bx = wall_x;
                    by = wall_y;
                }
            }
        }
        if (best == 0xFF) {                     /* (No door that way: a passage's mouth that way, the quickest) */
            for (by = C->room[r].y ? C->room[r].y - 1 : 0; by <= C->room[r].y + C->room[r].h && by < CW; ++by) {
                for (bx = C->room[r].x ? C->room[r].x - 1 : 0; bx <= C->room[r].x + C->room[r].w && bx < CW; ++bx) {
                    if (TILE (C->tile[by][bx]) != T_CORR || (tgx ? TARGET (ox + bx, oy + by) : bearing ((int) bx - wx,
                        (int) by - wy) != h) ||
                        (t = cross_room (r, wx, wy, bx, by)) >= bt) {
                        continue;
                    }
                    bt = t;
                    best = last_d;              /* (The way on from it) */
                    k = bx;
                    d = by;
                }
            }
            if (best == 0xFF) {                 /* (None: across the room) */
                across (h, r);
                return;
            }
            if (cross_room (r, wx, wy, k, d), cut) {
                on_way ();                      /* (A trap on its way to it) */
                return;
            }
            mx = ox + k - dir_x[best];          /* (From the room's square before it, walked on) */
            my = oy + d - dir_y[best];
            d = best;
            move.x = mx;
            move.y = my;
            t = bt + walk (d);
            goto set_out;
        }
        if ((q = portal_at (bx, by)) != 0 && !(q->state & PS_OPEN)) {
            k = q->state;                       /* (Shut: a door it has handled opened again, as it goes; another
                                                **   it must open itself, the user's; a portcullis down, raised) */
            d = q->parts;
            if (q->kind != PK_DOOR || !(d & PP_KNOWN) || (k & (PS_LOCKED | PS_BARRED | PS_WARDED))) {
                k = q->kind == PK_PORTCULLIS ? N_CLOSED : N_DOOR;    /* (Read while its chunk's in the window) */
                hub_map (0);
                no_way (k);
                return;
            }
            change (sn_, ox + bx, oy + by, k | PS_OPEN, d, PORTAL_STATE);
            event (E_PORTAL, 0, PO_OPENED);
            bt += 2 * stept;
        }
        if (cross_room (r, wx, wy, bx, by), cut) {
            on_way ();
            return;
        }
        mx = ox + bx;
        my = oy + by;
        d = best;
        move.x = mx;
        move.y = my;
        t = bt + walk (d);
    } else if ((h & 1) || !step_ok (sn_, mx, my, d)) {
        hub_map (0);
        no_way (N_NO_EXIT);
        return;
    } else {
        move.x = mx;
        move.y = my;
        t = walk (d);
        if (blocked && mx == move.x && my == move.y) {
            hub_map (0);
            den_way ();
            return;
        }
    }
set_out:
    hub_map (0);
    move.on = 1;
    move.how = A_WALKED;
    move.dir = d;
    move.sn = sn_;
    move.face = H_OF (mface);
    busy (t);
    bound_for (mx, my);
}

/* A small move (I_STEP: DUNGEON.md's): n squares on heading h (the UI's: by the words, else by the pace), its facing
** kept if keep (backing up, a step aside); in a room straight
** across its floor (across's rules: its edge, a ledge, a denizen or a trap stops it), in a passage along it (go's,
** walk's: a fork, a door or a denizen stops it) */
void __fastcall__ s_step_to (unsigned char h, unsigned char n, unsigned char keep)
{
    unsigned char r;

    dens_where ();
    here ();
    paced ();
    lim_ = n;
    move.on = 0;
    if ((r = room_of (sn_, mx, my)) != 0xFF) {
        across (h, r);
    } else {
        s_go (h);                               /* (Its own part's) */
    }
    lim_ = 250;
    hub_map (0);
    if (move.on && (move.how == A_CROSSED || move.how == A_WALKED)) {
        move.how = A_STEPPED;
        if (keep) {
            move.face = HUB->pc_face;
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The strata: the one below a link, begun when anyone first takes it */

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

/* Stratum n's plan grown by the maker (its record set in the world bank's strata first), and its record as grown
** (the maker's wrec.grown) written into the strata's, the world's (the hub's header in the window after) */
static void __fastcall__ planned (unsigned char n)
{
    ask_maker (M_PLAN, n, 0, 0);
    hub_map (WORLD_BANK);
    memcpy (&WREC->strata[n], &WREC->grown, sizeof WREC->grown);
    sync_strata ();
}

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
void __fastcall__ s_grow_below (unsigned char n)
{
    struct stratum* s = &strata[n];
    struct stratum* b;
    struct link* l;
    struct link* k;
    unsigned lx0 = 0xFFFF, ly0 = 0xFFFF, lx1 = 0, ly1 = 0, x, y;
    unsigned char i, st;

    hub_map (WORLD_BANK);                       /* (The strata's records the world bank's: it in the window) */
    b = &WREC->strata[n + 1];
    rseed (s->seed + 977);
    memset (b, 0, sizeof *b);
    b->n = n + 1;
    b->seed = s->seed * 69069UL + 1;
    i = random (2);                             /* (The theme's say: one of the stratum's styles) */
    hub_map (THEME_BANK);
    st = THEME->style[n + 1][i];
    hub_map (WORLD_BANK);
    b->style = st;
    b->age = 1 + random (40);
    random (8);                                 /* (Its name the theme's, by its depth: the dice as they were) */
    b->name = n + 1;
    b->datum = s->datum - 30 - random (40);
    for (i = s->nlinks, l = s->link, k = b->link; i; --i, ++l) {   /* (A link up under each link down to it) */
        if (l->to != n + 1) {
            continue;
        }
        x = l->x;
        y = l->y;
        lx0 = x < lx0 ? x : lx0;
        ly0 = y < ly0 ? y : ly0;
        lx1 = x > lx1 ? x : lx1;
        ly1 = y > ly1 ? y : ly1;
        k->kind = l->kind == O_LADDER_DOWN ? O_LADDER_UP : l->kind == O_SHAFT_DOWN ? O_SHAFT_UP : O_STAIRS_UP;
        k->dug = l->dug;
        k->to = n;
        k->x = x;
        k++->y = y;
        ++b->nlinks;
    }
    bounds (b, lx0, ly0, lx1, ly1);
    planned (n + 1);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* The link in the character's room the way d (up or down): its index in the stratum, the time to walk to it, or
** 0xFF; its way walked out again, so cut says if a trap's on it (the other walks' rule: one it knows of gone round) */
static unsigned char __fastcall__ link_here (unsigned char d, unsigned long* t)
{
    unsigned char r, k, kind, best = 0xFF, bk = 0;
    unsigned long bt = 0xFFFFFFFFUL, u;

    cut = 0;
    if ((r = room_of (sn_, mx, my)) == 0xFF) {
        return 0xFF;
    }
    for (k = 0; k < C->nopen; ++k) {
        kind = C->open[k].kind;
        if ((d == DIR_UP) != (kind == O_STAIRS_UP || kind == O_LADDER_UP || kind == O_WAY_OUT || kind == O_SHAFT_UP)) {
            continue;
        }
        if ((u = cross_room (r, cx_, cy_, C->open[k].x, C->open[k].y)) < bt) {
            bt = u;
            best = C->open[k].link;
            bk = k;
        }
        at (sn_, mx, my);                       /* (Its chunk in the window again) */
    }
    if (best != 0xFF) {
        cross_room (r, cx_, cy_, C->open[bk].x, C->open[bk].y);    /* (Its way again: a trap on it?) */
        at (sn_, mx, my);
    }
    *t = bt;
    return best;
}

/* The stairway or ladder in the room, and up or down it (I_JUMP: down a shaft, or a ledge, jumped) */
void __fastcall__ s_climb (unsigned char d)
{
    unsigned long t;
    unsigned char l, face, jump = d & I_JUMP;
    unsigned feet;
    struct link* lk;

    here ();
    face = HUB->pc_face;
    paced ();
    d &= ~I_JUMP;
    if (d == DIR_UP && start_ledge (1, face, 0)) {
        return;                                 /* (A ledge just ahead, faced: climbed before the room's way up, as its
                                                **   words say, u) */
    }
    l = link_here (d, &t);
    if (l == 0xFF || t == 0xFFFFFFFFUL) {
        if (!start_ledge (d == DIR_UP, face, jump)) { /* (No way up or down here: a ledge ahead, climbed) */
            hub_map (0);
            no_way (N_NO_STAIRS);
        }
        return;
    }
    if (cut) {
        on_way ();                              /* (A trap on its way to it: stopped there, as the other walks are) */
        return;
    }
    hub_map (0);
    lk = &strata[sn_].link[l];
    if (lk->kind == O_WAY_OUT) {                /* (The way out: shut, till the theme's mission opens it) */
        if (HUB->way_open) {
            event (E_LEFT, 0, 0);
        } else {
            no_way (N_SHUT);
        }
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
    feet = abs (strata[sn_].datum - strata[lk->to].datum);
    t += (unsigned long) (feet + 1) * stept *
        (lk->kind == O_LADDER_DOWN || lk->kind == O_LADDER_UP ? 2 : lk->kind == O_SHAFT_DOWN ||
        lk->kind == O_SHAFT_UP ? 3 : 1) / (d == DIR_UP ? 1 : 2);
    if (jump && lk->kind == O_SHAFT_DOWN) {     /* (Jumped down the shaft: a fall to its foot, told as fallen) */
        event (E_CLIMB, 0, K_JUMPED << 8 | (feet > 255 ? 255 : feet));
        t = stept + fall (feet, HF_CLEAR);
        move.dir |= O_FELL;
    }
    hub_map (0);
    busy (t);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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


/* Time spent (RS_EXERT: the referee's fatigue, by the pace; GA_REST, standing still), t tenths */
static void __fastcall__ spend (unsigned char a, unsigned long t)
{
    ww1 = t > 60000UL ? 60000U : (unsigned) t;
    ask_w (RS_EXERT, a, 0);
    ww1 = 0;
}

/* Searching as it went (the hub's watch, WA_TRAPS): about where it's stopped, a search (the hidden doors and traps
** within a few squares it sees, each the referee's P_SEARCH: wtrap.c's), quietly (told only if it finds something)
** and in no more time (the slow walk paid for it) */
static unsigned char barw_;
static unsigned char __fastcall__ work (unsigned char what, unsigned x, unsigned y, unsigned char d);


/* A move done: the character there, what it saw seen, and the UI told; the time it took at its pace the referee's */
static void turn (void);

static void arrive (void)
{
    unsigned char first, how = move.how;

    move.on = 0;
    if (how == A_DIGGING) {                     /* (A spell of digging done) */
        dig_on ();
        return;
    }
    if (how == A_CLIMBING) {                    /* (A round of climbing done) */
        climb_on ();
        return;
    }
    spend (pace_, went);                        /* (As it begins, or carries on: none) */
    sn_ = move.sn;
    mx = move.x;
    my = move.y;
    dens_where ();
    if (how == A_WALKED) {
        walk (move.dir);                        /* (Stopped short, if a denizen's come into its way; its squares the
                                                **   senses', felt: the look's walk) */
    } else if ((how == A_CROSSED || how == A_STEPPED) && move.den) {
        blocked = den_on (mx + head_x[move.face], my + head_y[move.face]);
    } else if (how <= A_START) {                /* (Down, up, begun: looking into the room) */
        move.face = toward_middle ();
    }
    hub_map (0);
    if (how == A_WALKED) {
        HUB->look.wx = move.x;
        HUB->look.wy = move.y;
        HUB->look.wd = move.dir;
        HUB->look.wn = walked;
    }
    HUB->pc_stratum = sn_;
    HUB->pc_x = mx;
    HUB->pc_y = my;
    HUB->pc_face = move.face;
    bound_for (mx, my);
    first = looked (LF_ARRIVE);                 /* (What it sees: the denizens there, at the turns pace, given their
                                                **   turn first, the chunks round it made, so it sees them as it gets
                                                **   there; the first time in a room, the senses' memory's) */
    if (!HUB->tpr && !HUB->paused && !HUB->hold) {
        turn ();
    }
    dens_seen ();
    sn_ = HUB->pc_stratum;
    if (searching && (unsigned char) (how - A_DOWN) > A_UP - A_DOWN && how != A_STOPPED) {   /* (Not up or down) */
        barw_ = 1;                              /* (Quietly: the search's link; what it finds seen) */
        if (work (PA_SEARCH, NO_SQ, NO_SQ, 0)) {
            see (first);
        }
        barw_ = 0;
    }
    event (E_ARRIVE, 0, how | first << 8 | (how == A_UP || how == A_DOWN ? (unsigned) move.dir << 12 : 0));
    if ((how == A_CROSSED || how == A_STEPPED) && move.ledge) {
        event (E_LEDGE, 0, (unsigned char) move.ledge);
    }
    if (how == A_STOPPED) {
        stopped ();
    }
    if (blocked) {
        den_way ();
    }
    hub_map (0);
    HUB->pc_going = 0;
}

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

/* The dungeon begun: the top stratum's record (a stand-in for the theme's say: a few hundred feet each way), its way
** in; its plan the maker's */
void s_begin (void)
{
    struct stratum* s;
    unsigned long seed;
    unsigned char k;
    unsigned o;
    struct tplace* p;

    hub_map (0);
    if (HUB->resume) {                          /* (Carried on: the save's strata, the change records' generation,
                                                **   the maker's seen maps back) */
        move.how = A_RESUMED;
        move.sn = HUB->pc_stratum;
        move.x = HUB->pc_x;
        move.y = HUB->pc_y;
        move.face = HUB->pc_face;
        sync_strata ();
        store_begin (1);
        ask_maker (M_RESTORE, 0, 0, 0);
        return;
    }
    store_begin (0);                            /* (A new game's generation: the folder's files of another not read) */
    seed = HUB->seed;
    hub_map (WORLD_BANK);                       /* (The strata's records the world bank's: it in the window) */
    s = &WREC->strata[0];
    rseed (seed ^ 0x57524C44UL);
    memset (s, 0, sizeof *s);
    s->seed = seed * 2654435761UL + 12345;
    hub_map (THEME_BANK);                       /* (The theme's say: its top stratum's first style) */
    k = THEME->style[0][0];
    hub_map (WORLD_BANK);
    s->style = k;
    s->age = 1 + random (20);
    random (8);                                 /* (Its name the theme's first: the dice as they were) */
    s->w = 120 + random (180);
    s->h = 80 + random (120);
    s->x0 = 32768U - s->w / 2;
    s->y0 = 32768U - s->h / 2;
    s->nlinks = 1;
    s->link[0].kind = O_WAY_OUT;
    o = 4 + random (s->w - 8);                  /* (Off its chunk's edges: world.h's INSET) */
    s->link[0].x = s->x0 + INSET (o);
    o = 4 + random (s->h - 8);
    s->link[0].y = s->y0 + INSET (o);
    planned (0);
    move.how = A_START;                         /* (The character at the foot of the way in) */
    move.sn = 0;
    move.x = strata[0].link[0].x;
    move.y = strata[0].link[0].y;
    hub_map (0);
    if (HUB->test_start && strata[0].nlinks > 1) {  /* (dq0 -T: at the first way down, for the tests) */
        move.x = strata[0].link[1].x;
        move.y = strata[0].link[1].y;
    }
    if (HUB->test_start == 2) {                 /* (dq0 -T -T: on the theme's first piece on the top stratum, put down
                                                **   as its chunk is made) */
        hub_map (THEME_BANK);
        for (k = NPIECES, p = THEME_PLACE; k && p->sn; --k, ++p) {
        }
        if (k) {
            at (0, (p->cx << CW_BITS) + strata[0].x0, (p->cy << CW_BITS) + strata[0].y0);
            hub_map (THEME_BANK);
            if (p->x) {
                move.x = p->x;
                move.y = p->y;
            }
        }
        hub_map (0);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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
    unsigned char   top;                        /*   for its first feet (a shaft: the floor's; the rock's below) */
    unsigned        need, done;                 /* Feet */
    unsigned        x, y;                       /* The square dug */
} dig;

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The referee's test of the character's digging (by what it has equipped): its grade, and the feet it made (made
** 0xFF: nothing it has can dig that) */
static unsigned char made;
static unsigned char dign;
static signed char test_dig (void)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_DIG;
    r->b = dig.done < dig.top ? dig.mat : MAT_STONE;     /* (Through the floor: the stratum's rock) */
    r->steps = 0;
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    made = r->result ? 0xFF : r->w1;
    evb = ++dign & 3;                           /* (Its tool and its wear, for the next event: E_DIG's) */
    strncpy (HUB->dig_tool[evb], r->text, sizeof HUB->dig_tool[0] - 1);
    HUB->dig_wear[evb] = r->w2;
    r->state = REQ_FREE;
    return g;
}

/* A noise where the character is: how loud (0-9), for the denizens (the hub's header in the window) */
static void __fastcall__ noise (unsigned char loud)
{
    static struct msg m;

    hub_map (0);
    m.kind = E_NOISE;
    m.who = 0;
    m.a = loud;
    m.w1 = HUB->pc_x;
    m.w2 = HUB->pc_y;
    m.t = HUB->now;
    hub_event (&m);
}

static void spell (void)
{
    in_place (A_DIGGING, SPELL);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* A link added to stratum sn's record (the world bank's: it in the window after), its index */
static unsigned char __fastcall__ add_link (unsigned char sn, unsigned char kind, unsigned char to, unsigned x,
    unsigned y)
{
    struct stratum* s;
    struct link* l;

    hub_map (WORLD_BANK);
    s = &WREC->strata[sn];
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

void __fastcall__ s_start_dig (unsigned char d)
{
    unsigned char* p;
    unsigned char t, k;

    here ();
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
        dig.mat = C->mat[room_of (sn_, dig.x, dig.y)][MP_FLOOR];    /* (The room's floor: the maker's) */
        dig.top = dig.mat == MAT_EARTH ? 2 : dig.mat == MAT_STONE ? 0 : 1;   /* (Earth two feet, flags or masonry one) */
        if (!strata[sn_ + 1].made) {
            grow_below (sn_);                   /* (Grown now: the shaft goes down to its floors, if it reaches under) */
            mx = dig.x;
            my = dig.y;
        }
        dig.need = abs (strata[sn_].datum - strata[sn_ + 1].datum);
    } else {
        dig.x = mx + dir_x[d];
        dig.y = my + dir_y[d];
        if ((k = room_of (sn_, mx, my)) == 0xFF) {  /* (A wall's room: its side's, or the far side's; in its chunk) */
            k = room_of (sn_, dig.x + dir_x[d], dig.y + dir_y[d]);
        }
        p = at (sn_, dig.x, dig.y);
        t = p ? TILE (*p) : T_PIT;
        if (t != T_WALL && t != T_ROCK) {
            hub_map (0);
            no_way (N_NO_DIG);
            return;
        }
        dig.need = t == T_WALL ? 2 : 5;         /* (A wall, two feet; a square of rock, five) */
        dig.mat = C->mat[t == T_WALL && k != 0xFF ? k : NROOMS][MP_WALL];   /* (Its room's walls, or the rock) */
        dig.top = 0xFF;
    }
    hub_map (0);
    event (E_DIG, 0, D_BEGUN << 8 | (dig.need > 255 ? 255 : dig.need));
    spell ();
}

/* A spell of work done: its test, its noise, and on, or through */
void s_dig_on (void)
{
    struct stratum* b;
    struct link* q;
    signed char g;
    unsigned char l, u, k;
    unsigned char* p;
    int z;

    here ();
    g = test_dig ();
    if (made == 0xFF) {                         /* (Nothing it has will bite into it) */
        hub_map (0);
        event (E_DIG, 0, D_NO_TOOL << 8);
        return;
    }
    dig.done += made;
    hub_map (0);
    noise (6);
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
    b = &strata[sn_ + 1];
    if (b->made && (dig.x - b->x0 >= b->w || dig.y - b->y0 >= b->h)) {
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
    if (!b->made) {
        grow_below (sn_);                       /* (Grown round it: a link up under each link down, this one's) */
        for (u = 0xFF, k = 0, q = b->link; k < b->nlinks; ++k, ++q) {
            if (q->x == dig.x && q->y == dig.y) {
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

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Doors (DUNGEON.md: portals), and the other portals (portcullises, windows, arrow slits) and the traps: the one
** the character faces walked up to, and worked (work.h's parts: opened, closed, a lock picked, forced; raised,
** lowered; broken, climbed through; a trap disarmed, a search), its changes kept (store.c's) */

static unsigned px_, py_;                       /* (The door's square, in the frame) */
static unsigned char pd_;                       /* (The way from where the character stands to it) */
static unsigned char want_;                     /* (What's to be worked, I_PORTAL's b: PW_*) */
                                                /* (barw_: a bar to set in a door's brackets, I_PORTAL's x[0]: its width;
                                                **   1, a search's quietly) */
#define WANTED(t)       ((TILE (t) == T_DOOR && want_ != PW_WINDOW) || (TILE (t) == T_WINDOW && want_ != PW_DOOR))

#pragma code-name (push, "SW12CODE")             /* (Part 12's: the main RAM's room) */
#pragma rodata-name (push, "SW12RODATA")

/* The door the character faces, walked up to (its time), its square in px_, py_: the door's record (in the window),
** or NULL */
static struct portal* __fastcall__ door_ahead (unsigned long* tt)
{
    unsigned char r, k, d, face = HUB->pc_face, bx = 0, by = 0, wx, wy, bd = 0;
    unsigned long t, bt = 0xFFFFFFFFUL;
    unsigned ox, oy;
    unsigned char* p;

    here ();
    paced ();
    *tt = 0;
    dens_where ();                              /* (Walked up to round the denizens) */
    cut = 0;
    if ((r = room_of (sn_, mx, my)) == 0xFF) {  /* (A passage: a door beside it, the way it faces first) */
        for (k = 0; k < 4; ++k) {
            d = (k + (face >> 1)) & 3;
            if ((p = at (sn_, mx + dir_x[d], my + dir_y[d])) != 0 && WANTED (*p) &&
                !TARGET (mx + dir_x[d], my + dir_y[d]) &&
                !(portal_at (cx_, cy_) && (portal_at (cx_, cy_)->state & PS_HIDDEN))) {
                px_ = mx + dir_x[d];
                py_ = my + dir_y[d];
                pd_ = d;
                at (sn_, px_, py_);
                return portal_at (cx_, cy_);
            }
        }
        return 0;
    }
    rx0 = ox = mx - cx_;
    ry0 = oy = my - cy_;
    wx = cx_;
    wy = cy_;
    for (k = 0; k < 4; ++k) {                   /* (Its room's doors that way: the quickest to walk up to) */
        for (d = 0; wall_tile (&C->room[r], k, d); ++d) {
            if (!WANTED (C->tile[wall_y][wall_x]) || (tgx ? TARGET (ox + wall_x, oy + wall_y) :
                bearing ((int) wall_x - wx, (int) wall_y - wy) != face && k != (face >> 1)) ||
                (portal_at (wall_x, wall_y) && (portal_at (wall_x, wall_y)->state & PS_HIDDEN))) {
                continue;
            }
            if ((t = cross_room (r, wx, wy, wall_x - dir_x[k], wall_y - dir_y[k])) < bt) {
                bt = t;
                bx = wall_x;
                by = wall_y;
                bd = k;
            }
        }
    }
    if (bt == 0xFFFFFFFFUL) {
        return 0;
    }
    cross_room (r, wx, wy, bx - dir_x[bd], by - dir_y[bd]);   /* (Its way again: a trap on it?) */
    *tt = bt;
    px_ = ox + bx;
    py_ = oy + by;
    pd_ = bd;
    mx = px_ - dir_x[bd];                       /* (Up to it: the room's square before it) */
    my = py_ - dir_y[bd];
    at (sn_, px_, py_);
    return portal_at (cx_, cy_);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* What (PA_*, or W_STEP) worked at x, y (or about the character, NO_SQ) from where the character stands (mx, my), d
** the way from there to it, by the part whose work it is (a command's all one part's: a door's, its shutters', a
** bar's the doors'; any portal's blows the blows'; a portcullis's, a window's the portals'; a trap's, a search's the
** traps'): 1 if there was nothing there to work */
static unsigned char __fastcall__ work (unsigned char what, unsigned x, unsigned y, unsigned char d)
{
    struct portal* p;
    unsigned char k, shut;

    wk.sn = sn_;
    wk.pd = d;
    wk.px = x;
    wk.py = y;
    wk.fx = mx;
    wk.fy = my;
    wk.link = barw_;                            /* (PA_BAR's: the bar to hand, its width; 0 none) */
    wk.tt = 0;
    wk.climb = 0;
    wk.res = 0;
    if (x == NO_SQ || what == W_STEP || what == PA_SEARCH || what == PA_DISARM) {
        wk.what = what;
        traps ();
    } else {
        at (sn_, x, y);
        p = portal_at (cx_, cy_);               /* (The world found it: it's there) */
        k = p->kind;
        shut = (p->parts & PP_SHUTTERS) && !(p->state & PS_OPEN);
        if (what == PA_BEND && k != PK_PORTCULLIS) {
            what = PA_FORCE;                    /* (A window's bars bent, a door: forced) */
        }
        wk.what = what;
        if (what == PA_BREAK || (k != PK_DOOR && k != PK_PORTCULLIS && what == PA_FORCE && !shut &&
            !(p->parts & PP_BARS))) {
            blows ();                           /* (Broken; a window forced, nothing shut or barred: its glass) */
        } else if (k == PK_DOOR || what == PA_BAR || (k != PK_PORTCULLIS && (what <= PA_UNLOCK || (what == PA_FORCE &&
            shut)))) {
            doors ();                           /* (A door's; a window's shutters, as a door's) */
        } else {
            portals ();
        }
    }
    hub_map (0);
    return wk.res;
}

/* What came of it (the work's): the character where it says, busy t more and the time it took; or in a pit,
** climbing out to there */
static void __fastcall__ worked (unsigned long t)
{
    hub_map (0);
    t += wk.tt > 60000UL ? 60000U : (unsigned) wk.tt;
    HUB->pc_x = wk.fx;
    HUB->pc_y = wk.fy;
    if (wk.climb) {
        cl.up = 1;
        cl.need = wk.climb;
        cl.done = 0;
        cl.x = wk.fx;
        cl.y = wk.fy;
        HUB->pc_x = tx_;                        /* (At the pit's foot) */
        HUB->pc_y = ty_;
        event (E_CLIMB, 0, K_BEGUN << 8 | cl.need);
        climb_round ();
        t += t ? ROUND_T : 0;
    }
    if (t) {
        busy (t);
    }
    t = wk.res;
    see (0);
    if (t) {
        event (E_ARRIVE, 0, A_CLIMBED);         /* (Through a window: somewhere new) */
    }
}

/* A door (or a portcullis, a window, a slit) the character faces worked, walked up to first; or about it: a search,
** a trap in reach disarmed (else the door ahead's) (part 12's) */
#pragma code-name (push, "SW12CODE")
#pragma rodata-name (push, "SW12RODATA")

void __fastcall__ s_door_work (unsigned char what)
{
    unsigned long t;

    here ();
    if ((what == PA_SEARCH || what == PA_DISARM) && !work (what, NO_SQ, NO_SQ, 0)) {
        worked (0);
        return;
    }
    if (!door_ahead (&t)) {
        hub_map (0);
        event (what == PA_DISARM ? E_TRAP : E_PORTAL, 0, what == PA_DISARM ? TO_NONE << 8 : want_ == PW_WINDOW ?
            PO_NONE | PK_WINDOW << 8 : PO_NONE);
        HUB->pc_ready = HUB->now;
        return;
    }
    if (cut) {
        on_way ();                              /* (A trap on its way up to it) */
        return;
    }
    work (what, px_, py_, pd_);
    HUB->pc_face = H_OF (pd_);
    worked (t);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The walk worked out last stops at the trap on its way (tx_, ty_, from fx_, fy_: its chunk's, C's, made the
** frame's), to be set off as it gets there (or noticed in time: wtrap.c's) */
static void on_way (void)
{
    unsigned ox = strata[sn_].x0 + C->cx * CW, oy = strata[sn_].y0 + C->cy * CW;

    tx_ += ox;
    ty_ += oy;
    fx_ += ox;
    fy_ += oy;
    hub_map (0);
    move.on = 1;
    move.how = A_STOPPED;
    move.sn = sn_;
    move.x = tx_;
    move.y = ty_;
    move.face = bearing ((int) (tx_ - fx_), (int) (ty_ - fy_));
    busy (cut_t);
}

/* On the trap on its way (A_STOPPED): set off (wtrap.c's) */
static void stopped (void)
{
    mx = fx_;
    my = fy_;
    work (W_STEP, tx_, ty_, 0);
    worked (0);
}

static unsigned char pc_waited;
static void __fastcall__ intent (struct msg* m)
{
    if ((HUB->pcsum.cond & C_FATIGUED) && HUB->now > free_at) {
        spend (GA_REST, HUB->now - free_at);    /* (Stood still since it was last free: rest, the referee's) */
        hub_map (0);
    }
    free_at = HUB->now;
    switch (m->kind) {
    case I_WAIT:
        pc_waited = 1;                             /* (Its eyes looked to as it ends: the senses') */
        busy (m->w1 ? m->w1 : HUB->round);
        free_at = HUB->now;                     /* (Waiting: rest) */
        if (!m->w2) {
            event (E_TIME, 0, m->w1 ? m->w1 : HUB->round);
        }
        break;
    case I_GO:
        tgx = m->w1;
        tgy = m->w2;
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
    case I_GOTO:
        go_to (m->w1, m->w2);
        break;
    case I_TAKE:
        take_go (m);                            /* (Walked over to it, then taken up: wgame.c) */
        break;
    case I_STEP:
        step_to (m->w1 & 7, m->w2, (m->w1 & STEP_KEEP) != 0);
        break;
    case I_PORTAL:
        want_ = m->b;
        tgx = m->w1;
        tgy = m->w2;
        barw_ = m->x[0];
        door_work (m->a);
        break;
    case I_LIGHT:
        busy (m->a ? 3 * stept : stept);
        see (0);
        event (E_VIEW, 0, m->a ? V_LIT : V_DARK);
        break;
    case I_QUIT:
        HUB->quit = 1;
        break;
    case I_ATTACK:                              /* (A round's blow, struck: at once; at the turns pace, as the clock
                                                **   moves on, the denizens' slice's first: turns_run) */
        busy (HUB->round);
        HUB->pc_aim = m->b;
        HUB->pc_foe = m->a;
        if (HUB->tpr) {
            pc_blow ();
        }
        break;
    }
}

/* At the turns pace, the character busy: the clock moved on to when it's free, TURN_SLICE tenths at a time, the
** denizens given their turn at each (the hub's den_go, rung: they act, and say when they've done, I_DONE), and at the
** same clock after work of its own (an event told since their last: the character arrived, its work done, free
** again), so they act through all the time what it does takes, and the game's the same whatever the tasks' timing;
** waited for each time (their intents carried out as they come, the world asleep on its doorbell between; one slow to
** is left to catch up as it can, late, till the character's next command).  The character's blow struck as the clock
** moves on, before it (its round's: the slice's first, seen by the denizens as they see the slice).  1 if it moved it
** on or gave them a turn */
static unsigned waited, go_eq;

#define BEHIND          (den_done != HUB->den_go)

static void turn (void)
{
    go_eq = HUB->eq_head;
    ++HUB->den_go;
    hub_ring (T_DENIZENS);
    for (waited = 0; !late && BEHIND && waited < 1000; ++waited) {
        hub_sleep (1);                          /* (Waited for here, a thousand wakes at most, a tick each at most:
                                                **   five seconds; the rest of the round's no use, and a fight's blows
                                                **   ask the referee a few times over) */
        if (DEN_INTENTS ()) {
            den_intents ();                     /* (Their intents carried out as they come) */
        }
        if (hub_posted (&HUB->ireq)) {
            things ();                          /* (The UI's things meanwhile: what's carried ...) */
        }
    }
    late = BEHIND;
}

static unsigned char turns_run (void)
{
    unsigned d;

    if (HUB->tpr || HUB->paused || HUB->hold) {
        return 0;
    }
    if (HUB->eq_head != go_eq) {
        turn ();                                /* (Its own work told: the denizens' turn at it, at this clock) */
        return 1;
    }
    if ((d = (unsigned) HUB->pc_ready - (unsigned) HUB->now) == 0 || d > 60000U) {
        return 0;                               /* (Free: busy () takes 60,000 tenths at most) */
    }
    if (HUB->pc_foe) {
        pc_blow ();
    }
    hub_map (0);
    advance (d < TURN_SLICE ? d : TURN_SLICE);
    turn ();
    return 1;
}

/* The real clock's ticks since the last look, made game time at the pace */
static void clock_run (void)
{
    unsigned t = hy_ticks (), d = t - last;

    last = t;
    if (HUB->paused || HUB->hold || !HUB->tpr) {
        return;
    }
    while (d) {                                 /* (64 ticks at a time: in 16 bits, cc65's longs slow, and this
                                                **   each time round) */
        t = d > 64 ? 64 : d;
        d -= t;
        acc += t * HUB->round;
        if (acc >= HUB->tpr) {
            advance (acc / HUB->tpr);
            acc %= HUB->tpr;
        }
    }
}

/* The swapped parts' images read into banks of its own (dir/dq0w.ovl: dq0w.cfg's OV1 ..., 8K each): 0, or -1 */
static int __fastcall__ ovl_load (const char* dir)
{
    unsigned char* b = (unsigned char*) _SWAP_START__;  /* (Through the parts' area: none in it yet) */
    unsigned off;
    unsigned char k, first;
    int fd, n;

    strcpy ((char*) b, dir);
    strcat ((char*) b, "/dq0w.ovl");
    if ((fd = open ((char*) b, O_RDONLY)) < 0 || (n = hy_banks_alloc (OVBANKS)) < 0) {
        return -1;
    }
    for (k = 0, first = n; k < OVBANKS; ++k) {
        ovbank[k] = first + k;
    }
    for (k = 0, n = 1; k < OVBANKS && n > 0; ++k) {     /* (A bank at a time: cc65's optimizer can lose an index,
                                                **   ovbank[i]'s or off's in a bank) */
        for (off = 0; off < 0x2000 && (n = read (fd, b, 512)) > 0; off += n) {
            hy_bank (first + k);
            memcpy ((void*) (0x8000 + off), b, n);
        }
    }
    close (fd);
    return 0;
}

int main (int argc, char* argv[])
{
    static struct msg m;
    unsigned char was_busy = 0, dseq = 0, took;

    if (argc < 4 || ovl_load (argv[3]) || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    chunks_init (ASK_WORLD);
    items_begin (argv[3]);                      /* (The item store: the catalogue, a game carried on its things) */
    for (dseq = 0; hub_map (0), !HUB->quit && dseq != IO_BEGIN; ) {
        hy_sem_wait (HUB->sem[T_WORLD], 4);     /* (The things asked as the game starts: the theme's kinds, the kit
                                                **   ..., till the UI's begun it) */
        hub_map (0);
        if (hub_posted (&HUB->ireq)) {
            dseq = HUB->ireq.kind;
            things ();
        }
    }
    dseq = 0;
    hub_map (0);
    stept = HUB->pcsum.step ? HUB->pcsum.step : 10;
    begin ();
    arrive ();
    event (E_READY, 0, 0);                      /* (The UI waits for it: its commands, the world started) */
    last = hy_ticks ();
    while (hub_map (0), !HUB->quit) {         /* (The hub at $8000 again, each time round) */
        while (hy_sem_try (HUB->sem[T_WORLD]) == 0) {
        }
        stocks ();                              /* (The chunks the maker stocked: their things put down, marked so) */
        if (DEN_INTENTS ()) {
            den_intents ();                     /* (The denizens' intents carried out) */
        }
        while (hub_take (&m)) {
            intent (&m);
            was_busy = 1;
        }
        if (hub_posted (&HUB->wreq)) {
            answer ();                          /* (The UI's: the save, a close look) */
        }
        clock_run ();
        if (den_saw && !move.on) {
            den_saw = 0;                        /* (A body fallen: looked at again, the things too) */
            see (0);
        }
        if (HUB->den_seq != dseq) {
            dseq = HUB->den_seq;
            stirred ();                         /* (The denizens' bodies' changes: what it sees of them) */
            hub_map (0);
        }
        ++HUB->beat[T_WORLD];
        if (turns_run ()) {
            continue;                           /* (The turns pace: the denizens catching up) */
        }
        if (move.on && HUB->now >= HUB->pc_ready) {
            arrive ();
        }
        if (was_busy && !move.on && HUB->now >= HUB->pc_ready) {
            if (pc_waited) {
                pc_waited = 0;
                ask_look (LK_EYES);             /* (A wait's end: its eyes, as time's gone, the senses') */
            }
            took = take_n ? take_up () : 0;     /* (A thing walked over to: taken up, or walked on to) */
            if (took == 1) {
                see (0);
            } else if (!took) {
                event (E_READY, 0, 0);          /* (The character free again: the UI takes its next line) */
            }
            was_busy = took == 2;
        }
        if (hub_posted (&HUB->ireq)) {
            things ();                          /* (The UI's: the things; after the clock's moved on for an intent,
                                                **   so the denizens see it moved, as before the things were here) */
        }
        hub_sleep (move.on || was_busy ? 2 : 4);  /* (Nothing under way: asleep on the doorbell, an intent
                                                **   waking it at once; the clock on from the ticks gone) */
    }
    event (E_BYE, 0, 0);
    return 0;
}
