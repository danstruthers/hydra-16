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
** in the dark, a walk while it listens; a square's time that pace's step (the referee's), the time moved at it, the
** time at strenuous work (a climb's rounds, a dig's spells, a battering's blows: GA_WORK) and the time stood still
** the referee's to tire the character by and rest it (RS_EXERT).  Searching as it goes, it
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
** to and taken up, I_TAKE; the game begun, the character's state set, the UI's WQ_START and WQ_SET; world.dat); part
** 27, the lights burning down (a game minute at a time: slice 4.1's).
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
#include "trace.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

/* ---- The swapped parts' stubs (bank.s's) */

void __fastcall__ start_dig (unsigned char d);  /* Part 1 */
void dig_on (void);
void __fastcall__ climb (unsigned char d);      /* Part 2 */
void climb_on (void);
unsigned __fastcall__ fall (unsigned feet, unsigned char how);
void begin (void);                              /* Part 3 */
void __fastcall__ grow_below (unsigned char n);
void __fastcall__ go (unsigned char h);         /* Part 9 */
void __fastcall__ go_to (unsigned x, unsigned y);
void __fastcall__ step_to (unsigned char h, unsigned char n, unsigned char keep);
void __fastcall__ door_work (unsigned char what);   /* Part 12 */
unsigned char batter_on (void);
void step_on (void);                            /* Part 23 */
unsigned char task_news (void);
void __fastcall__ task_word (const struct msg* m);
void __fastcall__ path_ways (unsigned char tx, unsigned char ty);
void __fastcall__ close_go (const struct msg* m);
unsigned char close_on (void);
unsigned char __fastcall__ zone_step (unsigned ox, unsigned oy, unsigned x, unsigned y);
void __fastcall__ fight_go (const struct msg* m);
unsigned char __fastcall__ fight_on (unsigned char idle);
void __fastcall__ disengage_go (const struct msg* m);    /* Part 26 */
unsigned char flee_on (void);                   /* Part 26 */
unsigned char toward_middle (void);            /* Part 3 */
void __fastcall__ across (unsigned char h, unsigned char r);   /* Part 33 */
void __fastcall__ intent_more (struct msg* m);  /* Part 37 */
unsigned char tripped (void);                   /* Part 43 */
unsigned char __fastcall__ dark_end (unsigned char how);    /* Part 44 */
struct work wk;
extern char _SWAP_START__[];
unsigned char den_done;                         /* (wbody.h's) */
unsigned char den_saw;
unsigned char noinit;                           /* (dq0 -I: no engage initiative test, nor a grope's; the UI's
                                                **   WQ_START, wgame.c's) */

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
extern unsigned char pc_hit;
unsigned char cl_hold;                          /* (A climb struck or hurt part way: its next test to keep its grip) */
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
    if (kind == E_HARM && !who) {
        pc_hit = 1;                             /* (A wound: its long task stopped, hard) */
    }
}

/* What the character sees, changed (E_VIEW's why: V_*), told the UI */
static void __fastcall__ view (unsigned w)
{
    event (E_VIEW, 0, w);
}

/* ---- A move's way, a square at a time (HOUSE.md's long tasks, slice 4.0): each square's heading and its time from
** the start kept as it's worked out (path_add, path_ways: in the ways' bank, after their queue), and walked as the
** clock goes (stepping): the character on each square in its turn, its long task's part done, its words as it goes
** (E_BEAT), what it meets on the way (a denizen in it, a blow, something new seen) */
#define PATH_MAX        250
static unsigned char npath, pnext, pwalk0, pon; /* (Its squares; the next to step onto; where a walk's passage begins;
                                                **   a walk on it under way) */
static unsigned long pt0, pbase, pdue;          /* (When it set out; the time a part of it begins, path_add's; when
                                                **   its next square comes, stepping's) */
static void __fastcall__ path_add (unsigned char h, unsigned long t);
static unsigned pspent;                         /* (A walk's tenths the referee's tired the character by so far) */
static unsigned char pfoe, paim, pstrike, pfd;  /* (A blow's close-in, I_CLOSE's: its target plus 1, 0 none; the
                                                **   blow's aim, what it strikes with; how far off it was as the walk
                                                **   last set out) */

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

/* The denizens the character sees now, by the look's marks (as the senses' dens.c decides): a bit an actor */
static unsigned char seen_now (void)
{
    struct actor* a;
    unsigned char k, s = 0, sn;
    unsigned char* p;
    unsigned x, y;

    for (k = 0; k < NACTORS; ++k) {
        hub_map (0);
        a = &HUB->actor[k];
        if (!a->state || (sn = a->sn) != HUB->pc_stratum) {
            continue;
        }
        x = a->x;
        y = a->y;
        if ((p = at (sn, x, y)) != 0 && (*p & T_VIS)) {
            s |= 1 << k;
        }
    }
    hub_map (0);
    return s;
}

/* The character's long task begun (HOUSE.md's long tasks: the hub's pc_task, the UI's bar): its kind, its length in
** its own units; what disturbs it from now on (a blow, a wound: pc_hit; a denizen not seen till now: pseen) */
unsigned char pc_hit;                           /* (A blow at the character, or a wound: wfight.c's too) */
unsigned char fwant;                            /* (Fighting on's, part 24's: an enemy may engage the character, looked
                                                **   at as it's free: | 1 a blow at it, wfight.c's, or its walk stopped
                                                **   in a zone, looked at at once; 2 a command taking over a fight) */
unsigned char flast;                            /* (The last to strike at the character, plus 1: wfight.c's) */
static unsigned char fheld;                     /* (The enemies the player stopped fighting, a bit each: part 24's) */
static unsigned char pseen;                     /* (The denizens seen as it began, and since) */
static unsigned char pc_tired;                  /* (Strenuous work done tired it a level more: asked, the main loop's) */
static void __fastcall__ task (unsigned char kind, unsigned len)
{
    pseen = seen_now ();
    HUB->pc_task.kind = kind;
    HUB->pc_task.asking = 0;
    HUB->pc_task.len = len;
    HUB->pc_task.done = 0;
    HUB->pc_task.foe = 0;
    pc_hit = 0;
    pt0 = HUB->now;
}

/* A walk set out on, its way the path's: the character busy for t tenths, its long task a walk of the path's
** squares, stepped as the clock goes (stepping) */
static unsigned char pquiet, plast, pfull, pprev; /* (pprev: the square before, stepped from, in a passage) */
static unsigned char proom;                     /* (The squares of a room walked, as it goes: beat's, a room crossed) */
unsigned char bold_;                            /* (A pace said outright in the dark: wgame.c's WS_PACE; gone as a move
                                                **   begins in the light: paced's) */
static unsigned char feel_, rush_;              /* (The move's, paced's: in the dark; its gait faster than feeling its
                                                **   way there) */
#define IDLE            (bold_ ? GOING_BOLD : 0)    /* (The hub's pc_going, not moving: its pace said outright in the
                                                **   dark, the referees' words of it) */
static unsigned char pfeel, prush;              /* (The walk under way's: in the dark, feel_; its gait faster than
                                                **   feeling its way there, rush_) */
unsigned char pthing, pthk, pcut;               /* (Part 43's: a faster walk's thing in the way, its square's place in
                                                **   the path plus 1, and its kind; a cautious one's, stopped short of
                                                **   it, its kind plus 1) */
unsigned pthi;                                  /*   (its item) */
unsigned long dt_;                              /* (The walk's time, dark_way's to cut) */
unsigned char __fastcall__ dark_way (void);     /* (Part 43's) */
static void __fastcall__ set_off (unsigned long t)
{
    pfeel = feel_;
    prush = rush_;
    pthing = pcut = 0;
    if (feel_) {
        dt_ = t;
        if (dark_way ()) {
            return;                             /* (Its foot on a thing at once, told: no walk) */
        }
        t = dt_;
    }
    busy (t);
    task (LT_WALK, npath);
    pnext = 0;
    pquiet = 0;
    pfull = npath;
    pon = 1;
    pprev = proom = 0;
    pdue = 0;                                   /* (Its first square looked for at once) */
    pspent = 0;
    plast = HUB->pc_face;
}

/* ---- The denizens in the character's way: the actors' squares on its stratum, as a move's worked out (dens_where)
** and made; none is walked onto, nor walked through */

static unsigned char nden, dk[NACTORS], blocked;    /* (blocked: the actor that stopped it, plus 1; or 0) */
static unsigned dsx[NACTORS], dsy[NACTORS];
static unsigned rx0, ry0;                       /* (The room's chunk's first square, in the frame: the ways') */

static void dens_where (void)
{
    struct actor* q;
    unsigned char k;

    hub_map (0);
    for (k = nden = 0, q = HUB->actor; k < NACTORS; ++k, ++q) {
        if (q->state && q->sn == HUB->pc_stratum && k + 1 != pfoe) {   /* (Not a close-in's target: walked up to) */
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
** walk's twice a walk's, as the dark's always was, a creep's three times).  In the dark (the user's, 2026-10-22:
** HOUSE.md's walking in the dark) it walks by what it knows (feel_: the walk's way, go's doors), cautiously, feeling
** ahead, unless a faster pace was said outright there (bold_: the UI's WS_PACE, GA_SAID; rush_ that pace, a walk or
** faster, its dangers part 43's) */
static unsigned char pace_, mul_ = 1, searching;
static unsigned char lim_ = 250;                /* (A walk's squares at most: a small move's, I_STEP's, fewer) */
static unsigned char pst = 10;
static unsigned char runs;                      /* (A flight's: at a run, whatever the player's pace: part 24's) */

static unsigned char dark (void);

static void paced (void)
{
    unsigned char dk = dark (), g, w;

    hub_map (0);
    g = runs ? GA_RUN : HUB->gait;
    w = HUB->watch;
    searching = w & WA_TRAPS;
    if (!dk) {
        bold_ = 0;
    }
    if (((dk && !bold_) || (w & (WA_TRAPS | WA_MAP))) && g > GA_SLOW) {
        g = GA_SLOW;
    } else if ((w & WA_LISTEN) && g > GA_WALK) {
        g = GA_WALK;
    }
    feel_ = dk;
    rush_ = dk && g > GA_SLOW ? g : 0;
    pace_ = g;
    mul_ = g < GA_WALK ? GA_WALK + 1 - g : 1;
    w = HUB->pcsum.gstep[g < GA_WALK ? GA_WALK : g];
    pst = w ? w : 10;
    HUB->pc_going = g + 1 | (dk ? GOING_DARK : 0);
}

/* A square's time to walk into, tenths of a second: the pace's step (the referee's, its summary's gstep), more uphill
** (half again a foot of rise), a little more steeply downhill (a quarter a foot past the first), more on steps (half
** again); a slower pace's (a slow walk's: feeling the way in the dark) times as many */
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

/* Square x, y known to the character: seen (now, or before: T_SEEN, the map remembered), or felt, walked in the dark
** (the senses' felt: T_SEEN too) */
static unsigned char __fastcall__ known_at (unsigned x, unsigned y)
{
    unsigned char* p = at (sn_, x, y);

    return p && (*p & (T_SEEN | T_VIS));
}

/* A way off to the side of the passage at (mx, my), heading d, that the character knows of: 1 (a fork it knows) */
static unsigned char __fastcall__ side_known (unsigned char d)
{
    unsigned char e = (d + 1) & 3, k;

    for (k = 0; k < 2; ++k, e ^= 2) {
        if (step_ok (sn_, mx, my, e) && !breadth (sn_, mx, my, e) && known_at (mx + dir_x[e], my + dir_y[e])) {
            return 1;
        }
    }
    return 0;
}

/* From (mx, my), heading d, to the next place: the time it takes (mx, my where it ends, mface its heading; walked
** its squares: the senses remember them felt, the look's walk).  In the dark (feel_), by what the character knows:
** straight on the way said, stopped by a way off it it knows of; round a bend only one it knows (else it meets the
** wall ahead: told as it gets there, part 43's), never toward an opening it hasn't seen or felt (the senses' felt
** goes the same way: the look's wd | 0x80) */
static unsigned char walked;
static unsigned long __fastcall__ walk (unsigned char d)
{
    unsigned long t = 0;
    unsigned char n = 0, inside, cross, h0 = d, nd;
    unsigned char* p;
    int z;

    pwalk0 = npath;
    way_room = 0xFF;
    cross = room_of (sn_, mx, my) == 0xFF && ways_from (sn_, mx, my, d) > 1 ? 3 : 0;   /* (Set out from a fork:
                                                **   three squares the way said, a wide one crossed, not each of its
                                                **   squares a fork again) */
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
        path_add (H_OF (d), pbase + t);
        inside = TILE (*p) != T_DOOR && room_of (sn_, mx, my) != 0xFF;
        if (inside) {                           /* (Through a door, into a room) */
            break;
        }
        if (TILE (*at (sn_, mx, my)) == T_DOOR) {
            continue;
        }
        if (feel_) {                            /* (In the dark: on as it goes, by what it knows) */
            if (step_ok (sn_, mx, my, d)) {
                if (side_known (d)) {
                    break;
                }
                continue;
            }
            if (ways_from (sn_, mx, my, d) == 1 && known_at (mx + dir_x[way_on], my + dir_y[way_on])) {
                d = way_on;
                continue;
            }
            break;
        }
        if (cross && (--cross, step_ok (sn_, mx, my, h0))) {
            d = h0;
            continue;
        }
        if ((nd = ways_from (sn_, mx, my, d)) == 1) {
            d = way_on;                         /* (On: its one way, a wide passage's breadth none) */
        } else if (nd || !step_ok (sn_, mx, my, d)) {
            break;                              /* (A fork, or the passage's end: a wide one's last squares on to
                                                **   it) */
        }
    }
    mface = d;
    walked = n;
    return t;
}

static void __fastcall__ no_way (unsigned why)
{
    event (E_NOWAY, 0, why);
    HUB->pc_ready = HUB->now;
    HUB->pc_going = IDLE;
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
** one of its doors or openings, by steps that can be walked; the time it takes.  The ways from a square of the room
** to all its squares (its floor and its walls, as big as the chunk lets it be: no fixed room's size) worked out at
** once (ways: as the first of them is looked up, part 21's ways_go, a search of the squares a step at a time, the
** first way to each found kept, each square's time from the start and the first trap on its way, in the world's own
** bank: struct wcell), then each way the caller wants to know looked up there (way) */

struct wcell {
    unsigned char   from;                       /* The way it was come into from the one before (4: the start; 0xFF
                                                **   none: not reached) */
    unsigned        t;                          /* Its time from the start (tenths, at most 0xFFFE) */
    unsigned        trap;                       /* The first square on its way with a trap that's set (that would
                                                **   be set off: its place in the chunk, y times 32 and x; 0xFFFF
                                                **   none) */
};
#define WAYS_AT         ((struct wcell*) 0x8000)    /* (The ways' bank, the world's own: ovbank[OVBANKS + 1]: the
                                                **   chunk's squares' cells, then the search's queue) */
#define WAYS_Q          ((unsigned*) (WAYS_AT + CW * CW))
typedef char ways_fit[CW * CW * (sizeof (struct wcell) + 2) <= 0x2000 ? 1 : -1];
static unsigned char wox, woy, wow, woh;        /* (The ways' room: its walls' first square, its size with them) */
extern unsigned char at_bank;                   /* (chunk.c's: at ()'s chunk's bank, C's) */
static unsigned char last_d;                    /* (way's: the way into its square) */
void __fastcall__ ways_go (unsigned char r, unsigned char fx, unsigned char fy);    /* (Part 21's) */
unsigned char __fastcall__ ways_near (unsigned char* tx, unsigned char* ty);
static unsigned char go_near;                   /* (I_GOTO's a: the nearest square the way reaches, if it can't reach
                                                **   the one said: a place in the room, the UI's) */
static unsigned char wr, wfx, wfy, wneed;       /* (The ways wanted: worked out as the first is looked up) */

/* ---- The path (above: a move's way, a square at a time) kept in the ways' bank, after their queue: three bytes a
** square, its heading and its time from the start (the chunk in the window again after) */
#define PATH_AT         ((unsigned char*) (WAYS_Q + CW * CW))
typedef char path_fits[CW * CW * (sizeof (struct wcell) + 2) + 3 * PATH_MAX <= 0x2000 ? 1 : -1];

static void __fastcall__ path_add (unsigned char h, unsigned long t)
{
    unsigned char* p;

    if (npath < PATH_MAX) {
        if (t > 0xFFFE) {
            t = 0xFFFE;
        }
        hy_bank (ovbank[OVBANKS + 1]);
        p = PATH_AT + 3 * npath++;
        p[0] = h;
        p[1] = (unsigned char) t;
        p[2] = (unsigned char) (t >> 8);
        hub_map (at_bank);
    }
}


static void __fastcall__ ways (unsigned char r, unsigned char fx, unsigned char fy)
{
    wr = r;
    wfx = fx;
    wfy = fy;
    wneed = 1;
}

/* The way worked out (ways) to (tx, ty) of the room's floor and walls (chunk squares): its time, or 0xFFFFFFFF (no
** way); cut 1 if a trap's on it (the first on it: tx_, ty_ its square, fx_, fy_ the one before, cut_t the time to
** it), last_d the way into its square; the chunk back in the window after */
static unsigned long __fastcall__ way (unsigned char tx, unsigned char ty)
{
    struct wcell* w = &WAYS_AT[(unsigned) ty * CW + tx];
    unsigned long t = 0xFFFFFFFFUL;
    unsigned q;

    if (wneed) {                                /* (Worked out as the first is wanted: none, if none's wanted) */
        wneed = 0;
        ways_go (wr, wfx, wfy);
    }
    cut = 0;
    if ((unsigned char) (tx - wox) < wow && (unsigned char) (ty - woy) < woh) {
        hy_bank (ovbank[OVBANKS + 1]);
        if ((last_d = w->from) != 0xFF) {
            t = w->t;
            if ((q = w->trap) != 0xFFFF) {
                cut = 1;
                cut_t = WAYS_AT[q].t;
                tx_ = q & (CW - 1);
                ty_ = q >> CW_BITS;
                q = WAYS_AT[q].from;
                fx_ = tx_ - dir_x[q];
                fy_ = ty_ - dir_y[q];
            }
        }
        hub_map (at_bank);
    }
    return t;
}

/* The character in the dark: its square unseen (the room's own light, its own, a lamp lying near would show it), or
** the character blind (the referee's summary, C_BLINDED: the one place a blindness, however it comes, takes it) */
static unsigned char dark (void)
{
    unsigned char d = !(*at (sn_, mx, my) & T_VIS);

    hub_map (0);
    return d || (HUB->pcsum.cond & C_BLINDED) != 0;
}

/* ---- Feeling the way in the dark (HOUSE.md's walking in the dark): what's on heading h from (mx, my) that stops a
** walk there, as the character's hands (or its foot, its staff) find it: FA_* (FA_NONE, the way on open), the chunk in
** the window after (ahead); told (E_BEAT's WB_FELT, the hub's header in the window after: "Your hands meet a wall."),
** and the walk's stopped there, facing it (felt_stop: 0 if there was nothing to tell, or it isn't dark); no way on in a
** passage, said either way (no_exit: go's).  Part 45's (the main RAM's room), but a square it knows and a thing found
** by touch, the main RAM's */
#define UNKNOWN(t)      unknown_ (t)            /* (In the dark, a square it doesn't know: a door or a passage's mouth it
                                                **   neither sees nor has seen) */
static unsigned char __fastcall__ unknown_ (unsigned char t)
{
    return feel_ && !(t & (T_SEEN | T_VIS));
}

unsigned char __fastcall__ ahead (unsigned char h);     /* (Part 45's stubs) */
unsigned char __fastcall__ felt_stop (unsigned char h);
void __fastcall__ no_exit (unsigned char h);

#pragma code-name (push, "SW45CODE")
#pragma rodata-name (push, "SW45RODATA")

unsigned char __fastcall__ s_ahead (unsigned char h)
{
    unsigned char* p = at (sn_, mx, my);
    unsigned char s, t;
    struct portal* q;
    int dz;

    if (!p) {
        return FA_WALL;
    }
    s = TILE (*p) == T_STEPS;
    dz = C->elev[cy_][cx_];
    if (!(p = at (sn_, mx + head_x[h], my + head_y[h]))) {
        return FA_WALL;
    }
    if ((t = TILE (*p)) == T_PIT) {
        return FA_PIT;
    }
    if (t == T_DOOR) {
        return !(q = portal_at (cx_, cy_)) || (q->state & PS_OPEN) ? FA_NONE : q->state & PS_HIDDEN ? FA_WALL :
            q->kind == PK_PORTCULLIS ? FA_BARS : FA_DOOR;
    }
    if (!T_PASS (t)) {
        return FA_WALL;
    }
    dz = C->elev[cy_][cx_] - dz;
    s = s || t == T_STEPS ? STAIR_MAX : STEP_MAX;
    return dz > s ? FA_RISE : -dz > s ? FA_DROP : FA_NONE;
}

unsigned char __fastcall__ s_felt_stop (unsigned char h)
{
    unsigned char k = feel_ ? s_ahead (h) : 0;

    hub_map (0);
    if (k) {
        event (E_BEAT, 0, WB_FELT | k << 8);
        HUB->pc_face = h;
        HUB->pc_ready = HUB->now;
        HUB->pc_going = IDLE;
    }
    return k;
}

void __fastcall__ s_no_exit (unsigned char h)
{
    if ((h & 1) || !s_felt_stop (h)) {
        hub_map (0);
        no_way (N_NO_EXIT);
    }
}

/* Square x, y of stratum sn dark to a denizen (wbody.c's: its kind with no darkvision feels its way there): not seen
** lit by the character, nor in a room's own light (the chunk in the window after) */
unsigned char __fastcall__ s_unlit_at (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char* p = at (sn, x, y), r;

    return p && !(*p & T_VIS) && ((r = room_of (sn, x, y)) == 0xFF || !(C->room[r].flags & R_LIT));
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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
        hub_map (0);
        if (HUB->pc_task.kind && !pon && !HUB->pc_task.asking && task_news ()) {
            return;                             /* (Something new seen as it digs or waits: told, then asked) */
        }
        view (V_STIR);
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
    if (HUB->ireq.kind == IO_PUZZLE ? puzzle () : things_req ()) {
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
        things_req ();
        hub_reply (&HUB->ireq);
    }
}

/* The senses asked to look about (the denizens it sees with it), and waited for, the things it saw made their own;
** the first time there, the reply's (LF_ARRIVE: by the senses' memory of the room) */
static unsigned npx, npy;
static unsigned char nps;
static unsigned char __fastcall__ looked (unsigned char first)
{
    hub_map (0);
    if (HUB->pc_x != npx || HUB->pc_y != npy || HUB->pc_stratum != nps) {
        npx = HUB->pc_x;
        npy = HUB->pc_y;
        nps = HUB->pc_stratum;
        it_near ();
    }
    HUB->look.first = first;
    hub_post (&HUB->look, T_SENSES);
    while (!hub_wait_t (&HUB->look, 4)) {      /* (Asleep meanwhile, a request rung waking it: the senses' stock,
                                                **   the UI's things; else four ticks, the clock on from the ticks
                                                **   gone: a tick's, a fifth of the CPU a look took from it) */
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
** (the hub's header in the window as it ends); moved, the things round it in the item store first (its paging's, as
** it comes into a chunk: ipage.c) */
static void __fastcall__ see (unsigned char first)
{
    looked (first);
    sn_ = HUB->pc_stratum;
}

/* Across room r on heading h (no door that way): as far as its floor can be walked, a ledge stopping it (told); a
** small move's (I_STEP's) no more than lim_ squares, and not on along a passage (part 33's: the main RAM's room, the
** traces', slice 4.1; part 9's walks calling it, once a move) */
#pragma code-name (push, "SW33CODE")
#pragma rodata-name (push, "SW33RODATA")

void __fastcall__ s_across (unsigned char h, unsigned char r)
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
                pbase = t;
                t += walk (h >> 1);
                pbase = 0;
                move.face = H_OF (mface);
                hub_map (0);
                if (!n && blocked && mx == move.x && my == move.y) {
                    move.on = 0;
                    den_way ();
                    return;
                }
                set_off (t);
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
            path_add (h, cut_t);
            on_way ();
            return;
        }
        t += (unsigned long) cost (dz, st) * ((h & 1) ? 7 : 5) / 5;
        mx = nx;
        my = ny;
        ++n;
        path_add (h, t);
    }
    hub_map (0);
    if (!n) {
        if (!blocked && feel_ && felt_stop (h)) {
            return;                             /* (In the dark: what its hands meet there, told) */
        }
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
    set_off (t);
    bound_for (mx, my);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

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
    npath = 0;
    r = room_of (sn_, mx, my);                  /* (0xFF: a passage, the ways about it, ways_go's) */
    if (x - (mx - cx_) >= CW || y - (my - cy_) >= CW) {
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
    if (t == T_DOOR || t == T_WINDOW || (r != 0xFF && t == T_CORR)) {  /* (Up to it: the room's square before it; in a
                                                **   passage the passage's, on the character's side) */
        for (d = 0; d < 4; ++d) {
            p = &C->tile[ty - dir_y[d]][tx - dir_x[d]];
            if (r != 0xFF ? T_INSIDE (*p) : TILE (*p) == T_CORR &&
                abs ((int) (tx - dir_x[d]) - fx) + abs ((int) (ty - dir_y[d]) - fy) < abs ((int) tx - fx) + abs ((int) ty - fy)) {
                tx -= dir_x[d];
                ty -= dir_y[d];
                break;
            }
        }
    }
    ways (r, fx, fy);
    if ((tt = way (tx, ty)) == 0xFFFFFFFFUL && go_near && ways_near (&tx, &ty)) {
        tt = way (tx, ty);                      /* (A place it can't stand on itself: the nearest it can) */
    }
    if (tt == 0xFFFFFFFFUL) {
        d = nden;                               /* (A way, but for the denizens: one's in the way) */
        nden = 0;
        ways (r, fx, fy);
        tt = way (tx, ty);
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
        path_ways (tx_, ty_);
        on_way ();                              /* (A trap on its way) */
        return;
    }
    path_ways (tx, ty);
    hub_map (0);
    move.on = 1;
    move.how = A_WENT_TO;
    move.sn = sn_;
    move.x = mx - fx + tx;
    move.y = my - fy + ty;
    move.face = tx == x - (mx - fx) && ty == y - (my - fy) ? HUB->pc_face : bearing ((int) x - move.x, (int) y - move.y);
    move.ledge = 0;
    set_off (tt);
    bound_for (move.x, move.y);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Climbing a ledge (the referee's P_CLIMB: feet of progress a test, a test a round, the game's choice, HOUSE.md;
** a hard climb, rough stone: 20 feet on a critical success, 10 on a success, 5 back on a marginal failure,
** and on a critical failure a fall, the referee's harm); or jumping down one */

#define ROUND_T         HUB->round              /* Tenths: a round, the ruleset's (RS_HELLO's: the hub's header in
                                                **   the window) */

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* A fall of the character's (the referee's harm, rs.h's HC_FALL): feet, onto the floor where it lands (the maker's
** material for it: earth, flags or masonry hard, stone broken rock), how (HF_*); the UI told (E_HARM).  The tenths
** it's out for (knocked senseless), or 0xFFFF: a hold grabbed, no fall (its stub's: part 43's too, a fall in the
** dark) */
unsigned __fastcall__ s_fall (unsigned feet, unsigned char how)
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
    r->b = m == MAT_EARTH || m == MAT_SOIL ? GR_FLOOR : m == MAT_STONE ? GR_ROCKY : GR_HARD;
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
    task (LT_CLIMB, cl.need);
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
    if (cl_hold) {                              /* (Struck or hurt part way: a test to keep its grip, HOUSE.md's) */
        cl_hold = 0;
        if (g == G_MS || g == G_CS) {
            event (E_CLIMB, 0, K_HELD << 8 | (cl.need - cl.done > 255 ? 255 : cl.need - cl.done));
            climb_round ();
            return;
        }
        g = G_CF;                               /* (Its grip lost: a fall, from as high as it got) */
    }
    if (g == G_CF && !(cl.up ? cl.done : cl.need - cl.done)) {
        g = G_MF;                               /* (Not off the ground yet: nothing to fall) */
    }
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
    t = g != G_MF ? K_ON : cl.done ? K_BACK : K_NOHOLD;   /* (A slip: back, if it's off the ground) */
    cl.done = g == G_CS ? cl.done + 20 : g == G_MS ? cl.done + 10 : cl.done > 5 ? cl.done - 5 : 0;
    HUB->pc_task.done = cl.done < cl.need ? cl.done : cl.need;
    if (cl.done < cl.need) {
        event (E_CLIMB, 0, t << 8 | (cl.need - cl.done > 255 ? 255 : cl.need - cl.done));
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
    npath = 0;
    if ((r = room_of (sn_, mx, my)) != 0xFF) {
        rx0 = ox = mx - cx_;                    /* (The chunk's first square, in the frame) */
        ry0 = oy = my - cy_;
        wx = cx_;
        wy = cy_;
        ways (r, wx, wy);
        for (k = 0; k < 4; ++k) {               /* (The doors that way: the quickest to walk to) */
            for (d = 0; wall_tile (&C->room[r], k, d); ++d) {
                if (TILE (C->tile[wall_y][wall_x]) != T_DOOR || (tgx ? TARGET (ox + wall_x, oy + wall_y) : (h & 1) ?
                    bearing ((int) wall_x - wx, (int) wall_y - wy) != h : k != (h >> 1)) ||
                    (portal_at (wall_x, wall_y) && (portal_at (wall_x, wall_y)->state & PS_HIDDEN)) ||
                    UNKNOWN (C->tile[wall_y][wall_x])) {
                    continue;                   /* (In the dark, only a door it knows of) */
                }
                if ((t = way (wall_x, wall_y)) < bt) {
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
                        (int) by - wy) != h) || UNKNOWN (C->tile[by][bx]) ||
                        (t = way (bx, by)) >= bt) {
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
            if (way (k, d), cut) {
                path_ways (tx_, ty_);
                on_way ();                      /* (A trap on its way to it) */
                return;
            }
            path_ways (k - dir_x[best], d - dir_y[best]);
            mx = ox + k - dir_x[best];          /* (From the room's square before it, walked on) */
            my = oy + d - dir_y[best];
            d = best;
            move.x = mx;
            move.y = my;
            pbase = bt;
            t = bt + walk (d);
            pbase = 0;
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
        if (way (bx, by), cut) {
            path_ways (tx_, ty_);
            on_way ();
            return;
        }
        path_ways (bx, by);
        mx = ox + bx;
        my = oy + by;
        d = best;
        move.x = mx;
        move.y = my;
        pbase = bt;
        t = bt + walk (d);
        pbase = 0;
    } else if ((h & 1) || !step_ok (sn_, mx, my, d)) {
        no_exit (h);                            /* (In the dark: what its hands meet, told) */
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
    set_off (t);
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
    npath = 0;
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
** (the maker's wrec.grown) written into the strata's, the world's; then its populations waited for (the hub's header
** in the window after) */
static void __fastcall__ planned (unsigned char n)
{
    ask_maker (M_PLAN, n, 0, 0);
    hub_map (WORLD_BANK);
    memcpy (&WREC->strata[SLOT (n)], &WREC->grown, sizeof WREC->grown);
    sync_strata ();
    kept_wait (n);                              /* (Its populations, wkept.c's) */
}

static unsigned char vast;                      /* (dq0 -T -T -T -T's: the strata below vast) */

/* A stand-in for the theme's say: a stratum's bounds round the squares it must hold (the links down to it), its
** size picked from a few classes: a cellar (a few squares), a modest one (a few hundred feet), a great one (a mile
** or two) */
static void __fastcall__ bounds (struct stratum* s, unsigned lx0, unsigned ly0, unsigned lx1, unsigned ly1)
{
    unsigned char k = random (20);
    unsigned w = k < 5 ? 4 + random (5) : k < 12 ? 60 + random (140) : 400 + random (1600), h;

    h = k < 5 ? 4 + random (5) : k < 12 ? 60 + random (140) : 400 + random (1600);
    if (vast) {
        w = h = 27 * RW;                        /* (dq0 -T -T -T -T: the stratum below the top 27 regions each way,
                                                **   its plan more than PLAN_MAX: paged, the maker's) */
    }
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

/* Stratum m's slot in the hub's strata's records (world.h's MAXSTRATA: m & 7) made way for: the stratum there, if
** it's another's and made, put out (the chunks the maker's stocked marked first; its things' chunks out of the item
** store, it_flush; its record to its file, st_out: stSS.dat), the slot free: 1, or 0 (no save folder: it stays) */
static unsigned char __fastcall__ slot_for (unsigned char m)
{
    struct stratum* s = &STRATUM (m);
    unsigned char o = s->n;

    if (!s->made || o == m) {
        return 1;
    }
    hub_map (0);
    if (!HUB->savedir[0]) {
        return 0;
    }
    stocks ();
    it_flush (o);
    if (!st_out (o)) {
        return 0;
    }
    hub_map (WORLD_BANK);
    WREC->strata[SLOT (m)].made = 0;
    sync_strata ();
    return 1;
}

/* Stratum m in its slot (the character or a shaft come to it): its record read back from its file if it's been put
** out (the one there put out for it: slot_for; the maker told, its plan grown again, M_SLOT): 1, or 0 (not made yet,
** the slot free for it: the caller grows it; or no room, no folder) */
unsigned char __fastcall__ s_stratum_in (unsigned char m)
{
    if (STRATUM (m).made && STRATUM (m).n == m) {
        return 1;
    }
    if (!slot_for (m) || !st_in (m)) {
        return 0;
    }
    sync_strata ();
    ask_maker (M_SLOT, m, 0, 0);
    return 1;
}

/* Stratum n + 1 begun, under stratum n's links down: its record (in the hub: its bounds round them, a link up at
** each, in its slot: the stratum there put out first), then its plan the maker's */
void __fastcall__ s_grow_below (unsigned char n)
{
    struct stratum* s = &STRATUM (n);
    struct stratum* b;
    struct link* l;
    struct link* k;
    unsigned lx0 = 0xFFFF, ly0 = 0xFFFF, lx1 = 0, ly1 = 0, x, y;
    unsigned char i, st;

    if (!slot_for (n + 1)) {
        return;
    }
    hub_map (WORLD_BANK);                       /* (The strata's records the world bank's: it in the window) */
    b = &WREC->strata[SLOT (n + 1)];
    rseed (s->seed + 977);
    memset (b, 0, sizeof *b);
    b->n = n + 1;
    b->seed = s->seed * 69069UL + 1;
    i = random (2);                             /* (The theme's say: one of the stratum's styles) */
    hub_map (THEME_BANK);
    st = THEME->style[n + 1 < TSTRATA ? n + 1 : TSTRATA - 1][i];   /* (Past the theme's strata, its deepest's) */
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
    ways (r, cx_, cy_);
    for (k = 0; k < C->nopen; ++k) {
        kind = C->open[k].kind;
        if ((d == DIR_UP) != (kind == O_STAIRS_UP || kind == O_LADDER_UP || kind == O_WAY_OUT || kind == O_SHAFT_UP)) {
            continue;
        }
        if ((u = way (C->open[k].x, C->open[k].y)) < bt) {
            bt = u;
            best = C->open[k].link;
            bk = k;
        }
        at (sn_, mx, my);                       /* (Its chunk in the window again) */
    }
    if (best != 0xFF) {
        way (C->open[bk].x, C->open[bk].y);     /* (Its way again: a trap on it?) */
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
    lk = &STRATUM (sn_).link[l];
    if (lk->kind == O_WAY_OUT) {                /* (The way out: shut, till the theme's mission opens it; a far exit,
                                                **   one below the top, the goal's, open: theme.h's G_THROUGH) */
        if (HUB->way_open || sn_) {
            event (E_LEFT, 0, 0);
        } else {
            no_way (N_SHUT);
        }
        return;
    }
    if (!stratum_in (lk->to)) {                 /* (Its record in its slot: from its file, or grown now) */
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
    feet = abs (STRATUM (sn_).datum - STRATUM (lk->to).datum);
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

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

/* The heading from (mx, my) to the middle of its room (as the character comes into a stratum: it looks in; part 3's,
** the main RAM's room) */
unsigned char s_toward_middle (void)
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

#pragma code-name (pop)
#pragma rodata-name (pop)


/* Time spent (RS_EXERT: the referee's fatigue, by the pace; GA_WORK, at strenuous work; GA_REST, standing still), t
** tenths: wres 1 if it tired the character a level more */
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
    unsigned char first, how = move.how, dw = pon && pfeel;
    unsigned t = went;

    move.on = 0;
    if (how == A_DIGGING || how == A_CLIMBING) {    /* (A spell of digging done, a round of climbing: its test, then
                                                **   its time the referee's, strenuous work, HOUSE.md's tiring; a
                                                **   fatigue level more asked of if the next is begun) */
        if (how == A_DIGGING) {
            dig_on ();
        } else {
            climb_on ();
        }
        spend (GA_WORK, t);
        pc_tired = wres && move.on;
        return;
    }
    spend (pace_, went > pspent ? went - pspent : 0);    /* (As it begins, or carries on: none; a walk's, what's
                                                **   left of it) */
    pspent = 0;
    sn_ = move.sn;
    mx = move.x;
    my = move.y;
    dens_where ();
    if (pon) {                                  /* (A walk, stepped: where it's got to, stopped short if a denizen's
                                                **   come into its way; a walk's passage squares the senses', felt:
                                                **   the look's walk) */
        pon = 0;
        hub_map (0);
        mx = HUB->pc_x;
        my = HUB->pc_y;
        walked = pnext > pwalk0 ? pnext - pwalk0 : 0;
        if (pnext < pfull) {
            move.face = HUB->pc_face;           /* (Stopped short: facing as it went) */
        }
        if (move.den) {
            blocked = den_on (mx + head_x[move.face], my + head_y[move.face]);
        }
        npath = 0;
    } else if (how <= A_START) {                /* (Down, up, begun: looking into the room) */
        move.face = toward_middle ();
    }
    hub_map (0);
    if (how == A_WALKED) {
        HUB->look.wx = move.x;
        HUB->look.wy = move.y;
        HUB->look.wd = move.dir | (pfeel ? 0x80 : 0);  /* (In the dark: the senses' felt as it went) */
        HUB->look.wn = walked;
    }
    HUB->pc_stratum = sn_;
    HUB->pc_x = mx;
    HUB->pc_y = my;
    HUB->pc_face = move.face;
    bound_for (mx, my);
    if (dw) {
        how = dark_end (how);                   /* (In the dark: what it met told, or blundered into, part 44's; a room
                                                **   crossed told as a move alone, unseen) */
    }
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
        if (work (PA_SEARCH, NO_SQ, NO_SQ, dw ? SF_NEAR | SF_TOUCH : 0)) {    /* (In the dark: by touch, within
                                                **   reach) */
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
    HUB->pc_going = IDLE;
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
        kept_wait (move.sn);                    /* (The places kept read back, the denizens') */
        return;
    }
    store_begin (0);                            /* (A new game's generation: the folder's files of another not read) */
    seed = HUB->seed;
    hub_map (WORLD_BANK);                       /* (The strata's records the world bank's: it in the window) */
    s = &WREC->strata[SLOT (0)];
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
    move.x = STRATUM (0).link[0].x;
    move.y = STRATUM (0).link[0].y;
    hub_map (0);
    if (HUB->test_start && STRATUM (0).nlinks > 1) {  /* (dq0 -T: at the first way down, for the tests) */
        move.x = STRATUM (0).link[1].x;
        move.y = STRATUM (0).link[1].y;
    }
    if (HUB->test_start == 2) {                 /* (dq0 -T -T: on the theme's first piece on the top stratum, put down
                                                **   as its chunk is made) */
        hub_map (THEME_BANK);
        for (k = NPIECES, p = THEME_PLACE; k && p->sn; --k, ++p) {
        }
        if (k) {
            at (0, (p->cx << CW_BITS) + STRATUM (0).x0, (p->cy << CW_BITS) + STRATUM (0).y0);
            hub_map (THEME_BANK);
            if (p->x) {
                move.x = p->x;
                move.y = p->y;
            }
        }
        hub_map (0);
    }
    vast = HUB->test_start == 4;
    if (HUB->test_start == 3) {                 /* (dq0 -T -T -T: four strata grown below the top, one under
                                                **   another, then the character at the foot of the second's way up:
                                                **   with dq0 -C's two slots, the deepest two in them, the second's
                                                **   record read back into its slot, the fourth's put out) */
        for (k = 0; k < 3 && STRATUM (k).made && STRATUM (k).n == k; ++k) {
            s_grow_below (k);
        }
        s_stratum_in (1);
        move.sn = 1;
        move.x = STRATUM (1).link[0].x;
        move.y = STRATUM (1).link[0].y;
        hub_map (0);
    }
}

/* The world's start (main's, dir its lib folder: the item store's catalogue there), once it's attached: the things
** asked as the game starts (the theme's kinds, the kit ...) till the UI's begun it, then the dungeon begun, the
** character arrived, and the UI told (part 3's: the main RAM's room) */
static void things (void);
void __fastcall__ s_start (const char* dir)
{
    unsigned char k;

    chunks_init (ASK_WORLD);
    items_begin (dir);                          /* (The item store: the catalogue, a game carried on its things) */
    hy_bank (ovbank[OVBANKS]);
    strcpy (TR_DIR, dir);                       /* (The traces' kinds' words, the lib folder's: wtrace.c's) */
    for (k = 0; hub_map (0), !HUB->quit && k != IO_BEGIN; ) {
        hy_sem_wait (HUB->sem[T_WORLD], 4);
        hub_map (0);
        if (hub_posted (&HUB->ireq)) {
            k = HUB->ireq.kind;
            things ();
        }
        if (hub_posted (&HUB->wreq)) {
            answer ();                          /* (The game begun: WQ_START, its pace, a game carried on) */
        }
    }
    hub_map (0);
    stept = HUB->pcsum.step ? HUB->pcsum.step : 10;
    s_begin ();
    arrive ();                                  /* (Its look the first: the things round it in the item store first, its
                                                **   paging's) */
    event (E_READY, 0, 0);                      /* (The UI waits for it: its commands, the world started) */
    last = hy_ticks ();
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW22CODE")            /* (Part 22's, sfile.c's: its bank's room) */
#pragma rodata-name (push, "SW22RODATA")

/* A look one way (I_LOOK's: HOUSE.md's looking one way): the senses' look about, and twice as far in the quarter
** ahead, their own record of it too (LK_WAY), t tenths; told the UI, E_VIEW V_LOOKED */
void __fastcall__ s_look_way (unsigned t)
{
    busy (t);
    HUB->look.kind = LK_WAY;
    see (0);
    hub_map (0);
    HUB->look.kind = LK_LOOK;
    view (V_LOOKED);
}
void __fastcall__ look_way (unsigned t);        /* (Its stub, bank.s's) */

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Digging (DUNGEON.md: changing the dungeon): a wall or the rock beside the character, or a shaft down through
** its floor; a spell of work at a time (ten minutes), the referee's test of it (P_DIG: the house's tools) the feet
** it makes (a critical success three, a success two, a marginal failure one, a critical failure none), loud (a noise
** each spell, for the denizens); through, the square's a passage, or the shaft a way down to the stratum below (a
** link, both its ends: the stratum below grown if no one has been there; or solid rock beneath, if the stratum
** below doesn't reach under it).  The feet dug kept as it goes (a change record, DUG: HOUSE.md's what stopping
** costs, a hole half dug stays half dug), so a dig stopped and begun again on its square, the same way, goes on */

#define SPELL           6000                    /* Tenths: ten minutes' work */

static struct {
    unsigned char   d;                          /* DIR_N ... DIR_W, DIR_DOWN */
    unsigned char   mat;                        /* What it's into (rs.h's MAT_*: the referee judges the tools) */
    unsigned char   top;                        /*   for its first feet (a shaft: the floor's; the rock's below) */
    unsigned        need, done;                 /* Half-feet (P_DIG's: bare hands half a foot a spell) */
    unsigned        x, y;                       /* The square dug */
} dig;
void __fastcall__ dig_kept (unsigned char begin);   /* (Part 19's: the hole's record, DUG) */

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
    s = &WREC->strata[SLOT (sn)];
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
        if (sn_ == 0xFE || sn_ + 1 >= MAXSTRATA && (hub_map (0), !HUB->savedir[0]) ||
            room_of (sn_, mx, my) == 0xFF) {    /* (Past the hub's strata's slots, none to put out without a folder) */
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
        dig.top = dig.mat == MAT_EARTH || dig.mat == MAT_SOIL ? 4 : dig.mat == MAT_STONE ? 0 : 2;  /* (Earth or soil
                                                **   two feet, flags or masonry one) */
        if (!stratum_in (sn_ + 1)) {
            grow_below (sn_);                   /* (Grown now: the shaft goes down to its floors, if it reaches under) */
            mx = dig.x;
            my = dig.y;
        }
        dig.need = 2 * abs (STRATUM (sn_).datum - STRATUM (sn_ + 1).datum);
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
        dig.need = t == T_WALL ? 4 : 10;        /* (A wall, two feet; a square of rock, five) */
        dig.mat = C->mat[t == T_WALL && k != 0xFF ? k : NROOMS][MP_WALL];   /* (Its room's walls, or the rock) */
        dig.top = 0xFF;
    }
    dig_kept (1);                               /* (Told, begun: on from where it was left, a hole begun before) */
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
        event (E_DIG, 0, D_NO_TOOL << 8 | (dig.done < dig.top ? dig.mat : MAT_STONE) + 1);   /* (What it's into) */
        return;
    }
    dig.done += made;
    hub_map (0);
    HUB->pc_task.done = dig.done;
    noise (6);
    if (dig.done < dig.need) {
        dig_kept (0);                           /* (The feet dug kept: stopped, it goes on from here) */
        event (E_DIG, 0, (g == G_CS || g == G_MS ? D_GOOD : g == G_MF ? D_POOR : D_SLIP) << 8 |
            (dig.need - dig.done > 510 ? 255 : (dig.need - dig.done + 1) / 2));
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
    stratum_in (sn_ + 1);
    b = &STRATUM (sn_ + 1);
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

#pragma code-name (push, "SW19CODE")           /* (Part 19's: the room of its bank and its area) */
#pragma rodata-name (push, "SW19RODATA")

extern struct change rec;                       /* (store.c's: the record written, or found) */
unsigned char store_get (void);                 /* (Part 19's, wfile.c's) */

/* The hole's record (DUG, a change record: its half-feet dug, the way it's dug), kept as a spell's done (begin 0); or
** (begin 1) looked up as the dig begins, on from where it was left if it's a hole begun before and dug the same way;
** and told (E_DIG: D_AGAIN and the feet to go, or D_BEGUN and its feet), its long task begun, its bar the part done
** (the hub's header in the window after) */
void __fastcall__ s_dig_kept (unsigned char begin)
{
    rec.sn = sn_;
    rec.x = dig.x;
    rec.y = dig.y;
    rec.tile = dig.done < 255 ? dig.done : 255;
    rec.elev = dig.d;
    rec.link = DUG;
    if (!begin) {
        store_put ();
        return;
    }
    if (store_get () && rec.elev == dig.d && rec.tile < dig.need) {
        dig.done = rec.tile;
    }
    hub_map (0);
    event (E_DIG, 0, dig.done ? D_AGAIN << 8 | (dig.need - dig.done > 510 ? 255 : (dig.need - dig.done + 1) / 2) :
        D_BEGUN << 8 | (dig.need > 510 ? 255 : dig.need / 2));     /* (Told in feet: to go, or of it) */
    task (LT_DIG, dig.need);
    HUB->pc_task.done = dig.done;
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
    ways (r, wx, wy);
    for (k = 0; k < 4; ++k) {                   /* (Its room's doors that way: the quickest to walk up to) */
        for (d = 0; wall_tile (&C->room[r], k, d); ++d) {
            if (!WANTED (C->tile[wall_y][wall_x]) || (tgx ? TARGET (ox + wall_x, oy + wall_y) :
                bearing ((int) wall_x - wx, (int) wall_y - wy) != face && k != (face >> 1)) ||
                (portal_at (wall_x, wall_y) && (portal_at (wall_x, wall_y)->state & PS_HIDDEN))) {
                continue;
            }
            if ((t = way (wall_x - dir_x[k], wall_y - dir_y[k])) < bt) {
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
    way (bx - dir_x[bd], by - dir_y[bd]);   /* (Its way again: a trap on it?) */
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
    if (x == NO_SQ || what == W_STEP || what == PA_SEARCH || what == PA_DISARM || what == PA_CHECK) {
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
    if (wk.open) {
        revealed ();                            /* (An opening told, with what the look found beyond it: part 12's) */
    }
    if (t) {
        event (E_ARRIVE, 0, A_CLIMBED);         /* (Through a window: somewhere new) */
    }
}

/* A search's end (the long task's: its time done, or stopped): the walls about it searched, the nearest first as far
** as its time went (HOUSE.md: a search cut short covers the part done, wtrap.c's), told (E_PORTAL: the UI's things'
** search after it, as far); 0, done */
static unsigned char srch;
unsigned char srch_form;                        /* (Its form, SF_*, | SF_TOUCH, | the wall's heading times 16: the
                                                **   things' search's too, things.c's) */
static unsigned char searched (void)
{
    srch = 0;
    here ();
    work (PA_SEARCH, NO_SQ, NO_SQ, srch_form);
    wk.tt = 0;                                  /* (Its minute spent already, the long task's) */
    worked (0);
    return 0;
}

/* A door (or a portcullis, a window, a slit) the character faces worked, walked up to first; or about it: a search,
** a trap in reach disarmed (else the door ahead's) (part 12's) */
#pragma code-name (push, "SW12CODE")
#pragma rodata-name (push, "SW12RODATA")

void __fastcall__ s_door_work (unsigned char what)
{
    unsigned long t;

    here ();
    if (what == PA_SEARCH) {                    /* (A long task: a minute's, what it finds told as it ends; a search for
                                                **   a thing, till it's found, I_PORTAL's w1: its form the UI's, by touch
                                                **   in the dark) */
        srch_form = want_ | (dark () ? SF_TOUCH : 0);
        if ((srch_form & (7 | SF_TOUCH)) == (SF_WALL | SF_TOUCH) && (unsigned char) (ahead (srch_form >> 4) - FA_WALL) >
            FA_BARS - FA_WALL) {
            hub_map (0);
            event (E_BEAT, 0, WB_FELT);         /* (By touch, no wall within reach that way: told, no search) */
            HUB->pc_ready = HUB->now;
            return;
        }
        t = tgx ? tgx : 600;
        task (LT_SEARCH, t);
        busy (t);
        srch = 1;
        return;
    }
    if (what == PA_DISARM && !work (what, NO_SQ, NO_SQ, 0)) {
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
    if (wk.what == W_BLOW) {
        task (LT_BATTER, BLOWS);                /* (Battering: a long task, a blow at a time as it's free) */
    }
}

/* An opening the character's made told (wk's open, its PO_*: a door opened, unbarred, forced, broken down; a window's
** shutters opened, burst, smashed; a portcullis raised), with what the look after it shows beyond it (the user's,
** 2026-10-22: "You open the door, revealing a dark corridor ..."; the look one way's words, dq0p's way_seen, the
** model): from the portal's square on, the way from the character through it, along the squares the eye sees past
** (no rock, no wall, no shut door), straight on through the dark, round a bend only where it sees the square, as far
** as a look one way reaches; how far it sees (the last square it sees now, T_VIS); the first square of a room but the
** character's own ends it, the room's kind and its light noted.  E_PORTAL, the opening's | PO_SHOWS: a what (WB_*) |
** 4 lit, b the room's kind, w2 the squares seen (part 41's, its own: the main RAM's room) */
#pragma code-name (push, "SW41CODE")
#pragma rodata-name (push, "SW41RODATA")
#pragma static-locals (push, on)
void s_revealed (void)
{
    struct portal* o;
    struct room* q;
    unsigned char* p;
    unsigned x = px_, y = py_, rx, ry;
    unsigned char d = pd_, n = 0, k, w, r, what = WB_DARK, dist = 0, kind = 0, lit = 0;

    hub_map (0);
    rx = HUB->pc_x;                             /* (The character's room: its floor's first square) */
    ry = HUB->pc_y;
    if ((r = room_of (sn_, rx, ry)) != 0xFF) {
        q = &C->room[r];
        rx = rx - cx_ + q->x;
        ry = ry - cy_ + q->y;
    } else {
        rx = ry = 0xFFFF;
    }
    while (n < 31) {
        for (k = 0, w = 0xFF; k < 4; ++k) {     /* (Straight on, else the first way seen that isn't back) */
            if (k != ((d + 2) & 3) && (w == 0xFF || k == d) && (p = at (sn_, x + dir_x[k], y + dir_y[k])) != 0 &&
                T_CLEAR (*p) && !((TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) && (o = portal_at (cx_, cy_)) != 0 &&
                !(o->state & PS_OPEN) && o->kind != PK_PORTCULLIS) && (k == d || (*p & T_VIS))) {
                w = k;
            }
        }
        if (w == 0xFF) {
            break;
        }
        x += dir_x[d = w];
        y += dir_y[d];
        ++n;
        if (!(*at (sn_, x, y) & T_VIS)) {
            continue;                           /* (Dark: on, as the eye goes) */
        }
        dist = n;
        what = WB_PASSAGE;
        if ((r = room_of (sn_, x, y)) != 0xFF) {
            q = &C->room[r];
            if (x - cx_ + q->x != rx || y - cy_ + q->y != ry) {    /* (Not the character's own) */
                what = WB_ROOM;
                kind = q->kind;
                lit = (q->flags & R_LIT) != 0;
                break;
            }
        }
    }
    hub_map (0);
    wk.m.kind = E_PORTAL;
    wk.m.who = 0;
    wk.m.a = what | lit << 2;
    wk.m.b = kind;
    wk.m.w1 = wk.open | wk.pq.kind << 8 | PO_SHOWS;
    wk.m.w2 = dist;
    wk.m.t = HUB->now;
    hub_event (&wk.m);
    wk.open = 0;
}
#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A battering's next blow as the character's free (the main loop's, while its long task's LT_BATTER: part 6's
** blows), the character busy its time; or, done, what came of the blows told and seen, the long task over, and the
** character busy for the time a trap it set off took: 2 while it's busy, 0 once it's free */
unsigned char s_batter_on (void)
{
    wk.tt = 0;
    blows ();
    hub_map (0);
    if (wk.what != W_BLOW && wk.what != W_GONE) {
        HUB->pc_task.kind = LT_NONE;
        see (0);
        if (wk.open) {
            revealed ();                        /* (Broken down: told with what it shows beyond, part 41's) */
        }
    } else {                                    /* (A blow struck: strenuous work, HOUSE.md's tiring; a fatigue level
                                                **   more asked of if more are to come) */
        spend (GA_WORK, BLOW_T);
        pc_tired = wres && wk.what == W_BLOW;
    }
    if (!wk.tt) {
        return 0;
    }
    busy (wk.tt);
    return 2;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The walk worked out last stops at the trap on its way (tx_, ty_, from fx_, fy_: its chunk's, C's, made the
** frame's), to be set off as it gets there (or noticed in time: wtrap.c's) */
static void on_way (void)
{
    unsigned ox = STRATUM (sn_).x0 + C->cx * CW, oy = STRATUM (sn_).y0 + C->cy * CW;

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
    set_off (cut_t);
}

/* On the trap on its way (A_STOPPED): set off (wtrap.c's) */
static void stopped (void)
{
    mx = fx_;
    my = fy_;
    work (W_STEP, tx_, ty_, 0);
    worked (0);
}


/* The character's long task as the clock goes: its part done, if it's a wait; its work, if there's any (part 23's:
** a walk's next square come, a blow or a wound to see to) */
static void stepping (void)
{
    hub_map (0);
    if (!HUB->pc_task.kind || HUB->pc_task.asking) {
        return;
    }
    if (HUB->pc_task.kind == LT_WAIT || HUB->pc_task.kind == LT_SEARCH) {
        HUB->pc_task.done = (unsigned) (HUB->now - pt0);
    }
    if (pc_hit || (pon && pnext < npath && HUB->now >= pdue)) {
        step_on ();
    }
}

#pragma code-name (push, "SW23CODE")
#pragma rodata-name (push, "SW23RODATA")

/* ---- The long task as it goes (HOUSE.md's long tasks): a walk stepped a square at a time as the clock goes, what
** it meets told; any task stopped, hard (a blow or a wound: all wounds, the user's; a hostile in sight within 30
** feet), or put to the player, soft (something else new seen: "Keep walking?"), or stopped as the player asks */
static unsigned long pask;                      /* (When the question was put) */
static unsigned prem;                           /* (The tenths it had left then) */

static void __fastcall__ halt (unsigned char why)
{
    hub_map (0);
    npath = pnext;                              /* (A walk: its way's end where it's got to) */
    move.den = blocked;
    went = (unsigned) (HUB->now - pt0);
    HUB->pc_ready = HUB->now;
    HUB->pc_task.asking = 0;
    if (why != 0xFF) {
        take_n = 0;                             /* (Stopped, hard or by the player: what it walked for dropped, a
                                                **   thing to take up, a blow's close-in) */
        pfoe = 0;
        event (E_TASK, 0, why | HUB->pc_task.kind << 8);
        HUB->pc_task.kind = LT_NONE;            /* (Its task over, told: a blow after it no second stop) */
    }
}

unsigned char pc_cut;                           /* (work.h's) */
static void __fastcall__ stop (unsigned char why)
{
    pc_cut = 1;
    if (pon) {
        halt (why);
        return;
    }
    hub_map (0);
    if (move.on) {                              /* (Work in place given up, a spell of digging: the part of it done
                                                **   strenuous work all the same, the referee's; free_at its end) */
        spend (GA_WORK, went - ((unsigned) free_at - (unsigned) HUB->now));
    }
    move.on = 0;                                /* (A spell of digging given up, or a wait cut short) */
    if (HUB->pc_task.kind != LT_FIGHT) {
        HUB->pc_ready = HUB->now;               /* (A fight's blow struck: its round out first) */
    }
    HUB->pc_task.asking = 0;
    pc_hit = 0;                                 /* (A blow taken as it was asked: no second stop after the answer's) */
    event (E_TASK, 0, why | HUB->pc_task.kind << 8);
}

static void __fastcall__ ask (unsigned char why)
{
    hub_map (0);
    HUB->pc_task.asking = 1;
    pask = HUB->now;
    prem = HUB->pc_ready > HUB->now ? (unsigned) (HUB->pc_ready - HUB->now) : 0;
    HUB->pc_ready = HUB->now;                   /* (The clock still at the turns pace till it's answered) */
    event (E_TASK, 0, why | HUB->pc_task.kind << 8);
}

/* Something new seen as it goes: a hostile within 30 feet stops it (6 squares), anything else asks; 1 if so */
static unsigned char news (void)
{
    unsigned char n = seen_now (), k, nigh = 0;
    struct actor* a;
    unsigned dx, dy;

    n &= ~pseen;
    pseen |= n;
    if (!n || !HUB->pc_task.kind || HUB->pc_task.kind == LT_FIGHT) {
        return 0;                               /* (A fight's not stopped by one come into sight, nor asked) */
    }
    for (k = 0, a = HUB->actor; k < NACTORS; ++k, ++a) {
        if (n & 1 << k) {
            dx = a->x > HUB->pc_x ? a->x - HUB->pc_x : HUB->pc_x - a->x;
            dy = a->y > HUB->pc_y ? a->y - HUB->pc_y : HUB->pc_y - a->y;
            nigh |= a->attitude == AT_HOSTILE && dx <= 6 && dy <= 6;
        }
    }
    view (V_STIR);                              /* (What it sees now, worded first) */
    if (nigh) {
        stop (TE_NEAR);
    } else {
        ask (TE_ASK_SEEN);
    }
    return 1;
}

/* The character's passing journalled as it leaves its square heading h (the traces': trod.inc's; the way it went from
** there, a tracker's; a flame it carries, its smoke) */
#include "trod.inc"

static void __fastcall__ pc_trod (unsigned char h)
{
    struct tstep s;

    hub_map (0);
    s.kind = 0;
    s.sn = HUB->pc_stratum;
    s.head = h | (HUB->pc_light ? TS_LIT : 0);
    s.x = HUB->pc_x;
    s.y = HUB->pc_y;
    trod (&s);
}

/* A walk's beat as it comes onto square x, y heading h (in a passage: E_BEAT): it turns, goes through a doorway,
** rises or falls, or a quiet stretch (6 squares, 30 feet); 1 if one's told */
static signed char pz;
static unsigned char __fastcall__ beat (unsigned x, unsigned y, unsigned char h)
{
    unsigned char b = 0xFF, s = WS_AHEAD, t;
    unsigned char* p;
    signed char z, dz;

    p = at (sn_, x, y);
    t = p ? TILE (*p) : T_CORR;
    z = elev_at (sn_, x, y);
    dz = z - pz;
    pz = z;
    if (room_of (sn_, x, y) == 0xFF) {
        if (proom >= 2) {                       /* (A room crossed, as the walk leaves it, to a door or a passage:
                                                **   told as it goes, the user's) */
            hub_map (0);
            event (E_BEAT, 0, WB_CROSS);
        }
        proom = 0;
        if (h != plast && pprev) {         /* (A turn between two squares of the passage walked) */
            b = WB_TURN;
            s = ((h - plast) & 7) < 4 ? WS_RIGHT : WS_LEFT;
        } else if (t == T_DOOR) {
            b = WB_DOOR;
        } else if (dz >= 2 || dz <= -2 || t == T_STEPS) {
            b = dz > 0 ? WB_UP : WB_DOWN;
        } else if (++pquiet >= 6) {
            b = WB_QUIET;
        }
        pprev = 1;
    } else {
        pprev = 0;
        ++proom;
    }
    plast = h;
    hub_map (0);
    if (b == 0xFF) {
        return 0;
    }
    pquiet = 0;
    event (E_BEAT, 0, b | s << 8);
    return 1;
}

/* The character's long task as the clock goes (stepping's, as there's work for it): a walk's squares stepped as
** their times come, a look every third square or at a beat (what's new seen); a task in place (a dig, a climb, a
** wait) disturbed hard at once */
void s_step_on (void)
{
    static unsigned char h, b, e;
    static unsigned t, x, y, ox, oy;
    unsigned char* p;

    hub_map (0);
    if (!HUB->pc_task.kind || HUB->pc_task.asking) {
        return;
    }
    if (!pon) {                                 /* (In place) */
        if (pc_hit) {
            pc_hit = 0;
            if (move.on && move.how == A_CLIMBING) {
                cl_hold = 1;                    /* (Its grip tested at once: the round's end now) */
                HUB->pc_ready = HUB->now;
            } else if (HUB->pc_task.kind != LT_FIGHT && (HUB->pc_task.kind != LT_WALK || npath > pnext)) {
                stop (TE_HIT);                  /* (A fight's blows at it no stop; nor a walk's stopped already,
                                                **   come into a zone: told once) */
            }
        }
        return;
    }
    if (pc_hit) {
        pc_hit = 0;                             /* (Struck as it closes with its target, beside it: no stop, the
                                                **   blow back as it arrives) */
        hub_map (0);
        halt (pfoe && zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y) == 0xFF ? 0xFF : TE_HIT);
        return;
    }
    dens_where ();
    while (pnext < npath) {
        hy_bank (ovbank[OVBANKS + 1]);
        p = PATH_AT + 3 * pnext;
        h = p[0];
        t = p[1] | p[2] << 8;
        hub_map (0);
        if (HUB->now < pt0 + t) {
            pdue = pt0 + t;                     /* (When the next comes: stepping looks for it) */
            break;
        }
        x = HUB->pc_x + head_x[h];
        y = HUB->pc_y + head_y[h];
        if ((blocked = den_on (x, y)) != 0) {
            HUB->pc_face = h;
            halt (0xFF);                        /* (A denizen come into its way: stopped short, told as it arrives) */
            return;
        }
        if (pthing == pnext + 1 && tripped ()) {
            HUB->pc_face = h;                   /* (Over a thing in its way in the dark, fallen: a round to rise, part
                                                **   43's) */
            halt (0xFF);
            HUB->pc_ready = HUB->now + ROUND_T;
            return;
        }
        if (!pnext) {
            pz = elev_at (sn_, HUB->pc_x, HUB->pc_y);
        }
        pc_trod (h);                            /* (The square it leaves journalled: the traces', the way it went
                                                **   from it) */
        ox = HUB->pc_x;
        oy = HUB->pc_y;
        HUB->pc_x = x;
        HUB->pc_y = y;
        HUB->pc_face = h;
        HUB->pc_task.done = ++pnext;
        if ((e = zone_step (ox, oy, x, y)) != 0) {    /* (HOUSE.md's engagement, part 24's) */
            if (e != 0xFF) {
                evb = e;                        /* (Come into an enemy's zone, or a square on in one: stopped; a
                                                **   hostile one fought, part 24's fighting on) */
                fwant |= 1;
            }
            halt (e == 0xFF ? 0xFF : TE_ENGAGED);   /* (0xFF: a close-in beside its target, the blow as it arrives) */
            engaged (0, e);                     /* (Newly engaged by its walk: the engage initiative test, part 25's) */
            return;
        }
        b = beat (x, y, h);
        if (pnext < npath && (b || pnext % 3 == 0)) {
            see (0);                            /* (Looking about as it goes: what's come into sight) */
            if (news ()) {
                return;
            }
            hub_map (0);
            t = (unsigned) (HUB->now - pt0);    /* (Its exertion so far the referee's: a fatigue level more, asked) */
            spend (pace_, t - pspent);
            pspent = t;
            if (wres) {
                ask (TE_ASK_TIRED);
                return;
            }
        }
    }
}

/* Something new seen as a task in place goes (a dig, a wait: the denizens' bodies changed, stirred's) */
unsigned char s_task_news (void)
{
    return news ();
}

/* A soft question answered: the task on as it was */
static void go_on (void)
{
    HUB->pc_task.asking = 0;
    pt0 += HUB->now - pask;
    HUB->pc_ready = HUB->now + prem;
}

/* The player's stop (I_STOP: a its kind named, or none) and a soft question's answer (I_GO_ON) */
void __fastcall__ s_task_word (const struct msg* m)
{
    unsigned char k;

    hub_map (0);
    k = HUB->pc_task.kind;
    if (m->kind == I_ASK) {                     /* (A sound the UI heard: asked, if there's a task going; a fight only
                                                **   its own, badly hurt, part 24's) */
        if (k && !HUB->pc_task.asking && (k != LT_FIGHT || m->w1 == TE_ASK_HURT)) {
            ask ((unsigned char) m->w1);
        }
        return;
    }
    if (m->kind == I_GO_ON) {
        if (HUB->pc_task.asking) {
            go_on ();
            event (E_TASK, 0, TE_ON | k << 8);
        }
        return;
    }
    if (!k) {
        if (!m->w1) {
            event (E_TASK, 0, TE_IDLE);         /* (The player's stop, nothing to stop; a command's, nothing said) */
        }
    } else if (m->a && m->a != k) {
        evb = m->a;                             /* (The one named: its b) */
        event (E_TASK, 0, TE_NOT | k << 8);
    } else if (k == LT_CLIMB) {
        event (E_TASK, 0, TE_CANT | k << 8);   /* (Part way up: no stopping; asked, "no", it goes on all the same) */
        if (HUB->pc_task.asking) {
            go_on ();
        }
    } else {
        if (k == LT_FIGHT) {                    /* (A command taking over: fought on after it, if an enemy engages
                                                **   it still; the player's stop: those engaging it now left be.  The
                                                **   round's blow posted, the turns pace's, struck first: told first) */
            if (HUB->pc_foe) {
                pc_blow ();
            }
            if (m->w1) {
                fwant = 2;
            } else {
                fheld = 0xFF;
            }
        }
        stop (TE_STOPPED);
    }
}

/* The character's light gone out (part 27's: its fuel burnt, b its kind, the UI's to word), and it's in the dark:
** its long task stopped hard with it (HOUSE.md's long tasks: the light going out), told as one (TE_OUT, its kind);
** but a climb, part way up, and a fight go on, the light's going told alone */
void __fastcall__ s_task_out (unsigned char b)
{
    unsigned char k;

    hub_map (0);
    k = HUB->pc_task.kind;
    evb = b;
    if (!k || k == LT_CLIMB || k == LT_FIGHT) {
        event (E_TASK, 0, TE_OUT);
    } else {
        stop (TE_OUT);
    }
}

/* The way across the room to its chunk's square tx, ty (the ways as worked out last), its squares added to the path
** from where it began, in order */
void __fastcall__ s_path_ways (unsigned char tx, unsigned char ty)
{
    static unsigned i;
    static unsigned char n, k, d;
    struct wcell* w;
    unsigned char* p;

    hy_bank (ovbank[OVBANKS + 1]);
    for (n = 0, i = (unsigned) ty * CW + tx; (d = WAYS_AT[i].from) < 4 && n < PATH_MAX; ++n) {
        i -= dir_x[d] + (int) dir_y[d] * CW;
    }
    if (npath + n > PATH_MAX) {
        n = PATH_MAX - npath;
    }
    npath = k = npath + n;
    for (i = (unsigned) ty * CW + tx; n && (d = (w = &WAYS_AT[i])->from) < 4; --n) {
        p = PATH_AT + 3 * --k;
        p[0] = H_OF (d);
        p[1] = (unsigned char) w->t;
        p[2] = (unsigned char) (w->t >> 8);
        i -= dir_x[d] + (int) dir_y[d] * CW;
    }
    hub_map (at_bank);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Parts 43 and 44: the walk in the dark (the user's, 2026-10-22: HOUSE.md's walking in the dark), their own: its
** way gone over as it sets out, a thing stumbled over as it goes (43's), what it meets where it ends, felt, or at a
** faster pace blundered into (44's) */
#pragma code-name (push, "SW43CODE")
#pragma rodata-name (push, "SW43RODATA")
#pragma static-locals (push, on)

#define NBIG            6                       /* (The things in the way looked for, at most) */
#define BIG_BULK        20                      /* (A thing in the way, felt or stumbled over: its bulk, pints, at
                                                **   least, a chair's, a backpack's; */
#define HEAVY_BULK      60                      /*   a heavy one, the book's heavy obstruction, a table's, a chest's) */

/* Thing i known to the character: found by touch (as found things are) */
static void __fastcall__ known (unsigned i)
{
    item_at (i)->flags |= I_KNOWN;
    hub_map (0);
}
static unsigned bigx[NBIG], bigy[NBIG], bigi[NBIG];
static unsigned char bigk[NBIG], bigb[NBIG], nbig, pheavy;


/* The walk worked out, in the dark (set_off's: feel_), its way gone over from where it sets out: the first thing on it
** big enough to be in the way (BIG_BULK) that the character doesn't know of (lying on a square: the item store's); a
** cautious walk stopped short of it, its foot finding it there (told as it gets there: pcut), a faster one to stumble
** over it as it comes to it (pthing: tripped's).  Told as it sets out ("You feel your way north.", "You run north into
** the dark.": a walk's, not a small move's).  1 if the thing's right before it, told (no walk) */
unsigned char s_dark_way (void)
{
    struct item* p;
    unsigned x, y, i;
    unsigned char n, j, h = 0, h0 = move.face, b, sn = sn_;   /* (h0: the way it goes, as it'll face at its end: not
                                                **   its first step's, a door's way round the room) */

    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    for (i = nbig = 0; nbig < NBIG && (i = items (i, IN_SQUARE, 0)) != 0; ) {
        p = ip;
        if (p->sn != sn || (p->flags & I_KNOWN) || p->x - x + npath > 2 * npath || p->y - y + npath > 2 * npath) {
            continue;                           /* (Not on its way's ground, or known: gone round, as in the light) */
        }
        bigx[nbig] = p->x;
        bigy[nbig] = p->y;
        j = p->kind;
        hub_map (CAT_BANK);
        if ((b = CATALOGUE[j].bulk) >= BIG_BULK) {
            bigk[nbig] = j;
            bigb[nbig] = b;
            bigi[nbig++] = i;
        }
    }
    for (n = 0; nbig && n < npath; ++n) {
        hy_bank (ovbank[OVBANKS + 1]);          /* (The path, after the ways' queue: its headings) */
        h = PATH_AT[3 * n];
        x += head_x[h];
        y += head_y[h];
        for (j = 0; j < nbig && (bigx[j] != x || bigy[j] != y); ++j) {
        }
        if (j < nbig) {
            break;
        }
    }
    if (nbig && n < npath) {                    /* (Square n of its way: the thing's, j) */
        if (!rush_) {                           /* (Cautious: stopped short of it, its foot on it) */
            pcut = bigk[j] + 1;
            pthi = bigi[j];
            if (!n) {
                known (pthi);
                event (E_BEAT, 0, WB_THING | bigk[j] << 8);
                move.on = pcut = 0;
                HUB->pc_face = h;
                HUB->pc_ready = HUB->now;
                HUB->pc_going = IDLE;
                return 1;
            }
            npath = n;
            hy_bank (ovbank[OVBANKS + 1]);
            dt_ = PATH_AT[3 * n - 2] | (unsigned) PATH_AT[3 * n - 1] << 8;   /* (To the square before it) */
            mx = x - head_x[h];                 /* (Where it ends, for its bound_for: a walk's move.x, y its passage's
                                                **   start, the senses' felt) */
            my = y - head_y[h];
            move.face = h;
            move.ledge = 0;
            move.den = 0;
            if (move.how == A_STOPPED) {
                move.how = A_STEPPED;           /* (A trap past it not come to) */
            }
        } else {
            pthing = n + 1;                     /* (Faster: stumbled over, as it comes to it) */
            pthk = bigk[j];
            pthi = bigi[j];
            pheavy = bigb[j] >= HEAVY_BULK;
        }
    }
    hub_map (0);
    if (lim_ == 250) {
        event (E_BEAT, 0, WB_FEEL | (unsigned) (h0 | rush_ << 4) << 8);
    }
    return 0;
}

/* Over a thing in its way at a faster pace in the dark (step_on's, as it comes to it: pthing): the book's stumble roll
** (the referee's P_STUMBLE: steps the darkness's 4 worse, the thing's 2, a heavy one's 4, a run's 2, a sprint's 4);
** failed, fallen over it (the walk stopped short of it, a round to rise: step_on's), else stumbled and on.  Told, and
** the thing known (its foot found it).  1 if it fell */
unsigned char s_tripped (void)
{
    signed char g;

    known (pthi);
    wsteps = -(4 + (pheavy ? 4 : 2) + (prush >= GA_SPRINT ? 4 : prush >= GA_RUN ? 2 : 0));
    g = test_w (P_STUMBLE, 0);
    hub_map (0);
    event (E_BEAT, 0, (G_OK (g) ? WB_STUMBLE : WB_TRIP) | pthk << 8);
    pthing = 0;
    return !G_OK (g);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW44CODE")
#pragma rodata-name (push, "SW44RODATA")

/* (Its own copy: the main RAM's room) */
static void __fastcall__ known44 (unsigned i)
{
    item_at (i)->flags |= I_KNOWN;
    hub_map (0);
}

/* A blunt blow from no one (rs.h's HC_STRIKE: b its aspect, | its place (Z_*) times 32; its dice, more), the UI told
** (E_HARM): the tenths it's out for */
static unsigned __fastcall__ struck (unsigned char b, unsigned char c, unsigned char w1)
{
    struct req* r;
    unsigned out;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_HARM;
    r->actor = 0;
    r->other = 0;
    r->a = HC_STRIKE;
    r->b = b & 3;
    r->c = c;
    r->w1 = w1;
    r->w2 = b >> 5;
    r->text[0] = 0;
    r = hub_ask (SLOT_WORLD);
    out = r->w2;
    event (E_HARM, 0, HC_STRIKE | r->b << 8 | (out ? E_OUT : 0));
    r->state = REQ_FREE;
    return out;
}

/* The walk in the dark ended (arrive's, how its A_*, the character where it got to: mx, my): if it went its whole way
** and met something there (a walk on, across or a small move; a walk to a place or a thing went where it meant to),
** what: cautious, felt and told ("Your hands meet a wall.", "Your foot finds something on the floor: a chest.", "The
** floor falls away before you."); faster, blundered into: a wall, a door, bars or a rise run into (a blunt blow to the
** head, a jog's a die, a run's two, a sprint's three; a walk's none; a door's or bars' a thump the denizens hear), an
** edge (a drop, a pit's) stopped short at by a test of the character's dodging (a walk's 4 steps the easier, a jog's
** none, a run's 4 the harder, a sprint's 8), or gone over: a fall (a drop's height, a pit's depth, and a spiked pit's
** spikes), and a pit climbed out of */
static const signed char edge_[4] = { 4, 0, -4, -8 };
unsigned char __fastcall__ s_dark_end (unsigned char how)
{
    struct trap* q;
    unsigned char k, d = move.face, n, feet;
    unsigned x, y, out = 0;
    int z;

    hub_map (0);
    if (how == A_CROSSED) {
        how = A_STEPPED;                        /* (Across a room it doesn't see: no "You cross the room.") */
    }
    if (pnext < pfull) {
        pcut = 0;
        return how;                             /* (Stopped short: a denizen, a blow, the player, a stumble: told) */
    }
    if (pcut) {
        known44 (pthi);
        event (E_BEAT, 0, WB_THING | (pcut - 1) << 8);
        pcut = 0;
        return how;
    }
    if ((how != A_WALKED && how != A_STEPPED) || !(k = ahead (d))) {
        hub_map (0);
        return how;
    }
    move.ledge = 0;                             /* (Its ledge felt: no E_LEDGE, its height unseen) */
    hub_map (0);
    if (!prush) {
        event (E_BEAT, 0, WB_FELT | k << 8);
        return how;
    }
    if (k != FA_DROP && k != FA_PIT) {          /* (Run into it) */
        event (E_BEAT, 0, WB_HIT | k << 8 | prush << 12);
        if (k == FA_DOOR || k == FA_BARS) {
            wk.m.kind = E_NOISE;                /* (A thump: the denizens hear it; the work's event, its room) */
            wk.m.who = 0;
            wk.m.a = 2 + prush;
            wk.m.b = NZ_BLOW;
            wk.m.w1 = HUB->pc_x;
            wk.m.w2 = HUB->pc_y;
            wk.m.t = HUB->now;
            hub_event (&wk.m);
            wk.m.b = 0;
        }
        if (prush > GA_WALK) {
            busy (stept + struck (AS_BLUNT | Z_HIGH << 5, prush - GA_WALK, 0));
        }
        return how;
    }
    wsteps = edge_[(prush - GA_WALK) & 3];      /* (At the edge: stopped short, or over it) */
    if (G_OK (test_w (P_DODGE_HAZARD, 0))) {
        hub_map (0);
        event (E_BEAT, 0, WB_SHORT | k << 8);
        return how;
    }
    hub_map (0);
    event (E_BEAT, 0, WB_OFF | k << 8);
    x = mx + head_x[d];
    y = my + head_y[d];
    z = elev_at (sn_, mx, my) - elev_at (sn_, x, y);
    feet = z > 120 ? 120 : z;
    q = 0;
    if (k == FA_PIT) {                          /* (A pit's depth: its record's) */
        at (sn_, x, y);
        for (n = C->ntraps, q = C->trap; n && (q->x != cx_ || q->y != cy_); --n, ++q) {
        }
        feet = n && q->power ? q->power : 10;
        k = n ? q->kind : TK_PIT;
    }
    hub_map (0);
    HUB->pc_x = x;
    HUB->pc_y = y;
    bound_for (x, y);
    out = fall (feet, 0);
    if (q) {                                    /* (In a pit: its spikes, and climbed out of to where it came from) */
        if (k == TK_SPIKED) {
            out += struck (AS_POINT | Z_LOW << 5, 1 + feet / 10, feet / 5);
        }
        hub_map (0);
        cl.up = 1;
        cl.need = feet;
        cl.done = 0;
        cl.x = mx;
        cl.y = my;
        event (E_CLIMB, 0, K_BEGUN << 8 | feet);
        climb_round ();
        if (out) {
            busy (out + ROUND_T);
        }
        return how;
    }
    busy (stept + out);
    mx = x;
    my = y;
    return how;
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 24: engagement, and a blow's close-in */
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")

/* ---- Engagement (HOUSE.md's, the book's engagement zones): an enemy on its feet and conscious (a hostile or
** unfriendly denizen: one that strikes at a character beside it) has a zone, its square and the eight round it, but
** not round a corner (a diagonal with solid squares on both sides); one lying down only its own square, one senseless
** none.  The denizens move a square at a time and a hostile one stops beside the character to strike, so the rule
** holds for them as they are; the character's walks are stopped by it (step_on) */

/* Square ax, ay beside square x, y, not round a corner (the chunk in the window after) */
static unsigned char __fastcall__ beside (unsigned x, unsigned y, unsigned ax, unsigned ay)
{
    static unsigned char* p;
    int dx = ax - x, dy = ay - y;

    if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || (!dx && !dy)) {
        return 0;
    }
    if (dx && dy && (!(p = at (sn_, x + dx, y)) || !T_CLEAR (*p)) && (!(p = at (sn_, x, y + dy)) || !T_CLEAR (*p))) {
        return 0;
    }
    return 1;
}

/* The enemy whose zone square x, y is in: it plus 1, or 0 (the hub in the window after) */
static unsigned char __fastcall__ engager (unsigned x, unsigned y)
{
    static unsigned char k;
    struct actor* a;
    unsigned ax, ay;

    for (k = 0; k < NACTORS; ++k) {
        hub_map (0);
        a = &HUB->actor[k];
        if (!a->state || a->sn != HUB->pc_stratum || a->pos == POS_PRONE || a->doing == DO_DOWN ||
            a->attitude > AT_UNFRIENDLY) {
            continue;
        }
        ax = a->x;
        ay = a->y;
        if (beside (x, y, ax, ay)) {
            hub_map (0);
            return k + 1;
        }
    }
    hub_map (0);
    return 0;
}

/* The close-in's target beside square x, y (on its feet or not: it's walked up to) */
static unsigned char __fastcall__ near_foe (unsigned x, unsigned y)
{
    struct actor* a;
    unsigned ax, ay;

    hub_map (0);
    a = &HUB->actor[(pfoe - 1) & 7];
    if (!a->state || a->sn != HUB->pc_stratum) {
        return 0;
    }
    ax = a->x;
    ay = a->y;
    return beside (x, y, ax, ay);
}

/* A walk's square stepped onto (x, y, from ox, oy: step_on's): 0xFF beside a close-in's target; the enemy whose zone
** it's come into plus 1, | TE_STILL if it was in it already; 0 neither */
unsigned char __fastcall__ s_zone_step (unsigned ox, unsigned oy, unsigned x, unsigned y)
{
    unsigned char e;

    if (pfoe && near_foe (x, y)) {
        return 0xFF;
    }
    if ((e = engager (x, y)) != 0 && e == engager (ox, oy)) {
        e |= TE_STILL;
    }
    return e;
}

unsigned char s_close_on (void);
static void __fastcall__ fight_begin (unsigned char k);
static void __fastcall__ fight_blow (unsigned char k);

/* A blow's close-in begun (I_CLOSE: walked up to its target, then struck; engaged already, refused, the user's) */
void __fastcall__ s_close_go (const struct msg* m)
{
    unsigned char e;

    hub_map (0);
    if ((e = engager (HUB->pc_x, HUB->pc_y)) != 0 && e != m->a) {
        evb = e | TE_STILL;
        event (E_TASK, 0, TE_ENGAGED);
        return;
    }
    pfoe = m->a;
    paim = m->b;
    pstrike = m->x[1];
    pfd = 0xFF;
    s_close_on ();
}

/* The close-in as the character's free (its walk done, or begun): beside its target, the blow struck (I_ATTACK's: a
** round); nearer than it was as it last set out, on again (it's moved); else out of reach still (told, TE_FAR).  2 if
** it's walking or striking, 0 if it's done */
unsigned char s_close_on (void)
{
    static unsigned x, y, px, py, dx, dy;
    static unsigned char k, leg;
    struct actor* a;

    hub_map (0);
    k = (pfoe - 1) & 7;
    a = &HUB->actor[k];
    px = HUB->pc_x;
    py = HUB->pc_y;
    if (a->state && a->sn == HUB->pc_stratum) {
        x = a->x;
        y = a->y;
        if (beside (px, py, x, y)) {
            hub_map (0);
            pfoe = 0;
            HUB->pc_strike = pstrike;
            HUB->pc_aim = paim;
            fheld = 0;
            fight_begin (++k | TE_QUIET);       /* (The walk over: the blow's round, and fighting on after it) */
            fight_blow (k);
            return 2;
        }
        dx = x > px ? x - px : px - x;
        dy = y > py ? y - py : py - y;
        if (dy > dx) {
            dx = dy;
        }
        if (dx < pfd) {
            leg = pfd != 0xFF;                  /* (A later leg: its target moved on) */
            pfd = dx > 0xFE ? 0xFE : (unsigned char) dx;
            go_near = 1;                        /* (The nearest square it can reach, if the way can't reach its
                                                **   target's: out of reach still, TE_FAR, as it gets there) */
            go_to (x, y);                       /* (No way at all: told, the walk's words) */
            go_near = 0;
            if (!move.on) {
                pfoe = 0;
            } else if (leg) {
                move.how = A_WALKED;            /* (Walked on: "You go over to it." told of its first leg alone) */
            }
            return move.on ? 2 : 0;
        }
    }
    pfoe = 0;
    evb = k + 1;
    event (E_TASK, 0, TE_FAR);
    return 0;
}

/* ---- Fighting on (HOUSE.md's, the user's, 2026-10-20: "once engaged in combat, you should continue to attack your
** target as you can, without actively having to say "attack" each time"): a long task of its own (LT_FIGHT, the
** hub's pc_task: its foe the target plus 1, its done the blows struck), begun as the character attacks (I_ATTACK, a
** close-in come beside its target) and as a hostile enemy engages it (one striking at it, or one whose zone stops
** its walk: fwant, looked at as it's free; not one that isn't hostile, a wary rat keeping its distance; not while
** it's senseless, but as it comes round, if it's engaged still).  As it's free, a blow at its target with what it
** holds, while the target's within reach and able; its target beaten (dead, senseless, out of reach), the next
** enemy engaging it, turned to (fight_foe: the one that struck at it last, else the first); none, the task over,
** told ("No foe is left beside you.": a senseless one left lying, "attack the rat" finishes it).  A command that
** takes time takes over (the UI's I_STOP, w1 1: its round out, then the command; fought on after it if an enemy
** engages it still), the player's stop ends it (the enemies engaging it then left be, fheld, till they've left it or
** it attacks); the free commands are the UI's, and an attack as it fights its target changed (fight_go).  Badly hurt
** as it fights (its health's band worse: rs.h's H_BADLY, H_CRITICAL), asked ("You're badly hurt.  Keep fighting?"),
** the blows going on at the real paces (a pause in a real-time fight would only cost it), the clock still at the
** turns pace till it's answered.  Tiring as it fights isn't decided: none yet.
**
** The forms to come (the user's, 2026-10-20) the same task: a missile weapon's, its target within range, seen, a
** clear line (fight_reach's), its next missile taken before each shot, none left the task over (fight_on's, by the
** blow), the next foe one in range (fight_foe's), an enemy engaging the character ending it, the melee's taking over
** (fight_on's, as it begins one); a setting of the player's widening where it begins ("autoengage with sword at close
** range": the zone and a square more, closed with; a missile weapon's bands), fight_foe's; an ally's, by its actor */
static unsigned char fhurt;                     /* (Its health's band as it began, or as it was last asked of) */
static unsigned char fsaid;                     /* (The next blow the player's: at its target whatever its state, if
                                                **   it's within reach, "attack the rat": one senseless finished) */

/* Its target k (the actor plus 1) within reach (beside the character, round no corner; a missile weapon's, to
** come: within range, seen, a clear line): 2, and able (on its stratum, conscious); 1, senseless; 0 neither (the hub
** in the window after) */
static unsigned char __fastcall__ fight_reach (unsigned char k)
{
    struct actor* a;
    unsigned ax, ay;
    unsigned char d;

    hub_map (0);
    a = &HUB->actor[(k - 1) & 7];
    if (!k || !a->state || a->sn != HUB->pc_stratum) {
        return 0;
    }
    d = a->doing == DO_DOWN ? 1 : 2;
    ax = a->x;
    ay = a->y;
    if (!beside (HUB->pc_x, HUB->pc_y, ax, ay)) {
        d = 0;
    }
    hub_map (0);
    return d;
}

/* The foe it turns to, or begins on: a hostile enemy engaging the character (in its zone: on its feet, conscious,
** beside it round no corner; sensing it, not fleeing), not one the player's stopped fighting (fheld's, its bit let go
** once it isn't one); the one that struck at it last, else the first.  It plus 1, or 0 (the hub in the window after).
** (A setting of the player's, to come, widens it: a hostile within its range) */
static unsigned char fight_foe (void)
{
    static unsigned char k, f, b;
    struct actor* a;
    unsigned ax, ay;

    for (k = f = 0; k < NACTORS; ++k) {
        hub_map (0);
        a = &HUB->actor[k];
        b = 1 << k;
        if (a->state && a->sn == HUB->pc_stratum && a->attitude == AT_HOSTILE && a->pos != POS_PRONE &&
            a->doing != DO_ROAM && a->doing < DO_FLEE) {
            ax = a->x;
            ay = a->y;
            if (beside (HUB->pc_x, HUB->pc_y, ax, ay)) {
                if (!(fheld & b) && (!f || k + 1 == flast)) {
                    f = k + 1;
                }
                continue;
            }
        }
        fheld &= ~b;
    }
    hub_map (0);
    return f;
}

/* Fighting on begun at k (the actor plus 1, | TE_QUIET: begun by the player's blow, not worded): the long task, its
** target, told (the UI busy till it's done, a line it holds taking it over: TE_FIGHT); the hub in the window */
static void __fastcall__ fight_begin (unsigned char k)
{
    task (LT_FIGHT, 0);
    HUB->pc_task.foe = k & 0x0F;
    fhurt = HUB->pcsum.health;
    fsaid = 0;
    pc_cut = 0;
    evb = k;
    event (E_TASK, 0, TE_FIGHT | LT_FIGHT << 8);
}

/* Its blow at k: the character busy its round, the blow posted (the hub's pc_foe) and struck, at the real paces at
** once, at the turns pace as the clock moves on (turns_run's); its record's (the hub in the window) */
static unsigned char ftired;                    /* (A level more tired by its fighting: asked, fight_on's) */

static void __fastcall__ fight_blow (unsigned char k)
{
    busy (HUB->round);
    spend (GA_WORK, HUB->round);                /* (A round of fighting strenuous work: the user's, 2026-10-21, the
                                                **   book's second variant, a level each five minutes of it) */
    ftired |= wres;
    hub_map (0);
    HUB->pc_task.foe = k;
    ++HUB->pc_task.done;
    HUB->pc_foe = k;
    if (HUB->tpr) {
        pc_blow ();
    }
}

/* The player's blow (I_ATTACK: a the one struck at, b the aim, x[1] what it strikes with): while it fights, its target
** changed, struck at as the character's next free; else struck now, a round of its time, and fought on after it */
void __fastcall__ s_fight_go (const struct msg* m)
{
    hub_map (0);
    HUB->pc_strike = m->x[1];
    HUB->pc_aim = m->b;
    fheld = 0;
    if (HUB->pc_task.kind == LT_FIGHT && !pc_cut) {
        HUB->pc_task.foe = m->a;
        fsaid = 1;
        return;
    }
    fight_begin (m->a | TE_QUIET);
    fight_blow (m->a);
}

#define FW_RUN          0x80                    /* (fwant's: a flight's run on, as it's free: disengage_go's) */
static unsigned char fleeh;                     /* (Its way) */

/* Fighting on as the character's free (the main loop's, while its long task's LT_FIGHT, or as an enemy may engage it,
** fwant; idle 1, it wasn't busy: the UI at its prompt): fighting, a blow at its target if it's within reach and able
** (or the player's own, at one within reach), else at the next foe, turned to; none, the fight over, told.  Not
** fighting, a hostile engaging it begins it, told (after a command that took a fight over, fwant 2, turned to).
** 2 if it's striking (or asking), 0 if it's done (E_READY), 3 if it was idle and still is */
unsigned char __fastcall__ s_fight_on (unsigned char idle)
{
    struct msg q;                               /* (Not static: a part's statics the main RAM's) */
    static unsigned char k, h;

    hub_map (0);
    if (HUB->pc_task.kind == LT_FIGHT) {
        if (pc_cut) {
            HUB->pc_task.foe = 0;               /* (Stopped, the player's or a command's: its round out, told as it
                                                **   stopped) */
            return 0;
        }
        if (HUB->pcsum.cond & C_STUNNED) {      /* (In shock, come round: in no state to fight, the referee's; the
                                                **   fight over, told) */
            evb = 1;
            event (E_TASK, 0, TE_NO_FOE | LT_FIGHT << 8);
            HUB->pc_task.foe = 0;
            return 0;
        }
        h = fight_reach (k = HUB->pc_task.foe);
        if (h != 2 && (!h || !fsaid)) {
            if (!(k = fight_foe ())) {          /* (Its target beaten: the next, or none: told, but after a blow alone,
                                                **   its words enough, "You finish the giant rat where it lies.") */
                if (HUB->pc_task.done > 1) {
                    event (E_TASK, 0, TE_NO_FOE | LT_FIGHT << 8);
                }
                HUB->pc_task.foe = 0;
                fwant = 0;
                return 0;
            }
            evb = k | TE_STILL;
            event (E_TASK, 0, TE_FIGHT | LT_FIGHT << 8);
        }
        fsaid = 0;
        if ((h = HUB->pcsum.health) > fhurt && h >= H_BADLY && h <= H_CRITICAL) {
            fhurt = h;                          /* (Badly hurt as it fights: asked) */
            evb = h;
            if (!HUB->tpr) {
                q.kind = I_ASK;                 /* (The clock still at the turns pace till it's answered: then the
                                                **   blow) */
                q.w1 = TE_ASK_HURT;
                task_word (&q);
                return 2;
            }
            event (E_TASK, 0, TE_ASK_HURT | LT_FIGHT << 8);     /* (The blows on as it asks) */
        }
        if (ftired) {                           /* (A level more tired: asked as a hurt's is, the blows on as it
            ftired = 0;                         **   asks; the clock still at the turns pace) */
            if (!HUB->tpr) {
                q.kind = I_ASK;
                q.w1 = TE_ASK_TIRED;
                task_word (&q);
                return 2;
            }
            event (E_TASK, 0, TE_ASK_TIRED | LT_FIGHT << 8);
        }
        fight_blow (k);                         /* (A missile weapon's, to come: its next missile taken first) */
        return 2;
    }
    if (fwant & FW_RUN) {                       /* (A flight disengaged, free of them: a run on that way, part 26's) */
        fwant &= ~FW_RUN;
        if (!engager (HUB->pc_x, HUB->pc_y) && flee_on ()) {
            return 2;
        }
    }
    if (!((h = fwant) & 1) && idle) {
        return 3;
    }
    fwant = 0;
    if (!(k = fight_foe ()) || (HUB->pcsum.cond & C_STUNNED)) {
        return idle ? 3 : 0;                    /* (None; or it's in shock, in no state to fight) */
    }
    fight_begin (h & 2 ? k | TE_STILL : k);    /* (A hostile engaging it: fought back, at the middle, with what it
                                                **   holds; after a command that took a fight over, turned to) */
    HUB->pc_aim = Z_MID;
    HUB->pc_strike = SK_BEST;
    fight_blow (k);
    return 2;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 25: the engage initiative test, and a grope's */
#pragma code-name (push, "SW25CODE")
#pragma rodata-name (push, "SW25RODATA")

/* ---- The engage initiative test (HOUSE.md's, the user's, 2026-10-20; the book's optional rule): as one engages
** another (eng the one whose move did it, def the other: 0 the character, else an actor's entry plus 1, the actor
** beside the character, on its feet), both test their quickness (the referee's RS_CONTEST, P_INITIATIVE each), and a
** defender that does better gains a tactical advantage, an action more at once: it strikes first (a blow before the
** engager's, told so: FW_FIRST), or an actor that would rather, one not hostile, disengages (a square back out of
** reach, wbody.c's: told, TE_AWAY; cornered, it strikes).  The character's blow first stops its long task, as a blow
** at it does.  Its callers': a walk come into an enemy's zone, a close-in come beside its target (step_on's), an enemy
** come up beside the character (wbody.c's: def 0); none under dq0 -I (the tests'), nor while the character climbs.
** The walk's (eng 0) def is zone_step's: an enemy plus 1 (| TE_STILL, engaged with it already: none), 0xFF the
** close-in's target.  1 if the defender struck first, 2 if it backed away, 0 neither */
unsigned char __fastcall__ s_engaged (unsigned char eng, unsigned char def)
{
    struct req* r;
    struct actor* a;
    unsigned char k, f, z;

    if (def == 0xFF) {
        def = pfoe;
    }
    if (def & TE_STILL) {
        return 0;
    }
    k = ((eng ? eng : def) - 1) & 7;
    hub_map (0);
    a = &HUB->actor[k];
    if (noinit || (move.on && move.how == A_CLIMBING) || !a->state || a->sn != HUB->pc_stratum ||
        a->pos == POS_PRONE || a->doing == DO_DOWN || (unsigned) (a->x - HUB->pc_x + 1) > 2 ||
        (unsigned) (a->y - HUB->pc_y + 1) > 2) {
        return 0;
    }
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_CONTEST;
    r->actor = eng;
    r->other = def;
    r->a = r->b = P_INITIATIVE;
    r->steps = 0;
    r->c = 0;
    r = hub_ask (SLOT_WORLD);
    f = r->result;
    r->state = REQ_FREE;
    if (f != 2) {
        return 0;                               /* (The engager as quick, or quicker: as it was) */
    }
    if (def) {
        if ((f = den_first (k, HUB->actor[k].attitude > AT_HOSTILE)) == 2) {
            pfoe = 0;                           /* (Backed away: a close-in on it given up, the player's to say
                                                **   what next) */
            evb = def;
            event (E_TASK, 0, TE_AWAY);
        }
        pc_hit = 0;                             /* (Its walk stopped already: a blow at it first no stop more) */
        return f;
    }
    f = HUB->pc_foe;                            /* (The character's blow first, at the middle: one it had posted
                                                **   kept) */
    z = HUB->pc_aim;
    HUB->pc_foe = eng;
    HUB->pc_aim = Z_MID | FW_FIRST;
    pc_blow ();
    hub_map (0);
    HUB->pc_foe = f;
    HUB->pc_aim = z;
    pc_hit = 1;
    fwant |= 1;                                 /* (And fought on, a hostile one: part 24's) */
    return 1;
}

/* ---- A grope (HOUSE.md's, the user's, 2026-10-20; the book's: a thing handled in a fight): a thing of kind k taken
** up, drawn or put away while the character's engaged (part 24's zone) is a test of its hands (the referee's
** P_GROPE); 1 if it failed, its words in the hub's text ("You fumble for the sword.": the round wasted, the caller's),
** 0 if it went, or the character isn't engaged (dq0 -I: never tested); the hub in the window after */
unsigned char __fastcall__ s_fumbled (unsigned char k)
{
    char w[16];
    struct req* r;
    signed char g;

    hub_map (0);
    if (noinit || !zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y)) {
        return 0;
    }
    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_GROPE;
    r->b = 0;
    r->steps = 0;
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    r->state = REQ_FREE;
    if (G_OK (g)) {
        return 0;
    }
    hub_map (CAT_BANK);
    strcpy (w, CATALOGUE[k].name);
    hub_map (UI_BANK);
    strcpy (UIREC->things_text, "You fumble for the ");
    strcat (UIREC->things_text, w);
    strcat (UIREC->things_text, ".");
    hub_map (0);
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 26: disengaging */
#pragma code-name (push, "SW26CODE")
#pragma rodata-name (push, "SW26RODATA")

/* ---- Disengaging (HOUSE.md's, the user's, 2026-10-21: the book's, Combat 7): an engaged character moves a square,
** then, if it's no longer engaged, a half-move more (its walk's squares a round), straight on (step_to's: a wall, a
** ledge, a denizen stops it; another enemy's zone, step_on's), with no blow that round; the only way an engaged one
** moves.  The way the player's, else away from the foe engaging it (the nearest way open to that: straight away, half
** left or right, then aside; a passage's four alone).  A flight the same, then, free of them, a run on that way
** (fight_on's, FW_RUN); not engaged, a run from the nearest foe near.  Engaged still after it, fought on (fwant's) */

/* The nearest foe within a dozen squares of the character (hostile or unfriendly, on its feet), plus 1; or 0 */
static unsigned char near_foe12 (void)
{
    unsigned char k, f, d, best;                /* (Not static: a part's statics the main RAM's) */
    struct actor* a;
    unsigned dx, dy;

    for (k = f = 0, best = 13; k < NACTORS; ++k) {
        hub_map (0);
        a = &HUB->actor[k];
        if (!a->state || a->sn != HUB->pc_stratum || a->pos == POS_PRONE || a->doing == DO_DOWN ||
            a->attitude > AT_UNFRIENDLY) {
            continue;
        }
        dx = a->x > HUB->pc_x ? a->x - HUB->pc_x : HUB->pc_x - a->x;
        dy = a->y > HUB->pc_y ? a->y - HUB->pc_y : HUB->pc_y - a->y;
        d = dx > dy ? (dx > 12 ? 13 : (unsigned char) dx) : (dy > 12 ? 13 : (unsigned char) dy);
        if (d < best) {
            best = d;
            f = k + 1;
        }
    }
    hub_map (0);
    return f;
}

/* The square beside the character on heading h open to step onto (its floor passable, no denizen on it; in a
** passage, the four ways alone): 1 (dens_where's first) */
static unsigned char __fastcall__ open_to (unsigned char h)
{
    unsigned x, y;
    unsigned char* p;
    unsigned char sn;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x + head_x[h];
    y = HUB->pc_y + head_y[h];
    return ((h & 1) == 0 || room_of (sn, HUB->pc_x, HUB->pc_y) != 0xFF) && (p = at (sn, x, y)) != 0 && T_PASS (*p) &&
        !den_on (x, y);
}

/* A flight's run on, free of the foes after its disengage (fight_on's): the way on open, a run that way, 1 */
unsigned char s_flee_on (void)
{
    dens_where ();
    if (!open_to (fleeh)) {
        return 0;                               /* (Cornered: the disengage enough) */
    }
    fwant = 0;
    runs = 1;
    step_to (fleeh, 250, 0);
    runs = 0;
    hub_map (0);
    return move.on;
}

/* I_DISENGAGE (w1 the way, H_*, 0xFF away; w2 1 a flight): begun, told (E_TASK TE_BREAK), or none (TE_FREE) */
void __fastcall__ s_disengage_go (const struct msg* m)
{
    static const unsigned char off[5] = { 0, 1, 7, 2, 6 };
    unsigned char e, h, k, n, f;
    unsigned x, y;

    dens_where ();
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    f = m->w2 != 0;
    e = zone_step (x, y, x, y);                 /* (The enemy engaging it, part 24's: a close-in's target beside it,
                                                **   that one) */
    if (!(e = e == 0xFF ? pfoe : e & 0x3F)) {
        if (!f || !(e = near_foe12 ())) {
            evb = f;                            /* (Not engaged; or a flight with none near to flee) */
            event (E_TASK, 0, TE_FREE);
            return;
        }
        f = 2;                                  /* (A run from one not engaging it) */
    }
    h = m->w1;
    if (h > H_NW) {                             /* (Away from it: the nearest way open) */
        hub_map (0);
        h = bearing ((int) x - HUB->actor[(e - 1) & 7].x, (int) y - HUB->actor[(e - 1) & 7].y);
        for (k = 0; k < sizeof off && !open_to (n = (h + off[k]) & 7); ++k) {
        }
        if (k == sizeof off) {
            evb = 2;                            /* (No way back open) */
            event (E_TASK, 0, TE_FREE);
            return;
        }
        h = n;
    }
    hub_map (0);
    n = HUB->pcsum.gstep[GA_WALK];
    n = f == 2 ? 250 : 1 + HUB->round / (n ? n : 10);    /* (A square, a half-move more: the walk's squares a round) */
    evb = e;
    event (E_TASK, 0, TE_BREAK | (unsigned) f << 8);
    fleeh = h;
    fwant |= f == 1 ? FW_RUN | 1 : 1;           /* (Free of them, a flight's run on: fight_on's; engaged still,
                                                **   fought on) */
    runs = f == 2;
    step_to (h, n, !f);                         /* (Backing away facing the foe; a flight turned to run) */
    runs = 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

static unsigned char pc_waited;

/* An intent carried out, the UI's: a step, a walk, a turn, a climb, a wait, the quit (each answered at once: the
** main RAM's); the rest part 37's (intent_more) */
static void __fastcall__ intent (struct msg* m)
{
    if (m->kind == I_STOP || m->kind == I_GO_ON || m->kind == I_ASK) {
        task_word (m);                          /* (The long task's: no rest begun) */
        return;
    }
    if ((HUB->pcsum.cond & C_FATIGUED) && HUB->now > free_at) {
        spend (GA_REST, HUB->now - free_at);    /* (Stood still since it was last free: rest, the referee's) */
        hub_map (0);
    }
    free_at = HUB->now;
    switch (m->kind) {
    case I_WAIT:
        pc_waited = 1;                             /* (Its eyes looked to as it ends: the senses') */
        busy (m->w1 ? m->w1 : HUB->round);
        if (!m->w2 && m->w1 > HUB->round) {
            task (LT_WAIT, m->w1);              /* (A wait longer than a round: a long task) */
        }
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
        view (V_FACING);                        /* (Its first words told at once, the look after: V_TURNED's) */
        see (0);
        view (V_TURNED);
        break;
    case I_GOTO:
        go_near = m->a;
        go_to (m->w1, m->w2);
        go_near = 0;
        break;
    case I_STEP:
        step_to (m->w1 & 7, m->w2, (m->w1 & STEP_KEEP) != 0);
        break;
    case I_QUIT:
        HUB->quit = 1;
        break;
    default:
        intent_more (m);                        /* (The rest: part 37's) */
        break;
    }
}

/* ==== Part 37: the rest of the UI's intents carried out (intent's; its own part, the main RAM's room: slice 4.1's) */
#pragma code-name (push, "SW37CODE")
#pragma rodata-name (push, "SW37RODATA")

void __fastcall__ s_intent_more (struct msg* m)
{
    switch (m->kind) {
    case I_DIG:
        start_dig (m->a);
        break;
    case I_TAKE:
        take_go (m);                            /* (Walked over to it, then taken up: wgame.c) */
        break;
    case I_CLOSE:
        close_go (m);                           /* (Walked up to it, then struck: part 23's) */
        break;
    case I_PORTAL:
        HUB->pc_strike = m->x[1];               /* (What it strikes with: the parser's) */
        want_ = m->b;
        tgx = m->w1;
        tgy = m->w2;
        barw_ = m->x[0];
        door_work (m->a);
        break;
    case I_LIGHT:
        burn_ask = 1;                           /* (Its light's minutes left, the hub's, at once: part 27's) */
        busy (m->a ? 3 * stept : stept);
        see (0);
        view (m->a ? V_LIT : V_DARK);
        break;
    case I_LOOK:
        look_way (m->w1);                       /* (A look one way: part 3's) */
        break;
    case I_ATTACK:                              /* (A round's blow, struck: at once; at the turns pace, as the clock
                                                **   moves on, the denizens' slice's first: turns_run; then fought on,
                                                **   part 24's) */
        fight_go (m);
        break;
    case I_DISENGAGE:
        disengage_go (m);                       /* (The book's disengage: part 26's) */
        break;
    case I_TRACE:
        tr_flush (0);                           /* (The traces read, the journal taken in first, then the time it
                                                **   takes: parts 28's, 29's) */
        busy (traced (m));
        break;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* At the turns pace, the character busy: the clock moved on to when it's free, TURN_SLICE tenths at a time, the
** denizens given their turn at each (the hub's den_go, rung: they act, and say when they've done, I_DONE), and at the
** same clock after work of its own (an event told since their last: the character arrived, its work done, free
** again), so they act through all the time what it does takes, and the game's the same whatever the tasks' timing;
** waited for each time (their intents carried out as they come, the world asleep on its doorbell between), for as long
** as they take: thirty seconds at most, by the clock, not by its wakes (each intent rings it: a turn of many
** denizens woke it a thousand times, and a turn cut short at a count made the game turn on the tasks' timing); one
** that stuck is left to catch up as it can, late, till the character's next command.  The character's blow struck as the clock
** moves on, before it (its round's: the slice's first, seen by the denizens as they see the slice).  1 if it moved it
** on or gave them a turn */
#define TURN_WAIT       (30 * HY_TICK_HZ)       /* (The longest a turn's waited for: a stuck task's, not a slow one's) */

static unsigned go_eq;

#define BEHIND          (den_done != HUB->den_go)

static void turn (void)
{
    unsigned t0;

    go_eq = HUB->eq_head;
    ++HUB->den_go;
    hub_ring (T_DENIZENS);
    for (t0 = hy_ticks (); !late && BEHIND && (unsigned) (hy_ticks () - t0) < TURN_WAIT; ) {
        hub_sleep (1);                          /* (Waited for here, a tick at a time at most) */
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
    stepping ();                                /* (Its walk on, square by square, before the denizens' turn) */
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
    if ((fd = open ((char*) b, O_RDONLY)) < 0 || (n = hy_banks_alloc (OVBANKS + 4)) < 0) {
        return -1;
    }
    for (k = 0, first = n; k < OVBANKS + 4; ++k) {
        ovbank[k] = first + k;
    }
    for (k = off = 0; (n = read (fd, b, 512)) > 0; ) {   /* (The file the build made: OVBANKS banks at most, 8K
                                                **   each, read 512 bytes at a time; first + k, not ovbank[k]: cc65's
                                                **   optimizer dropped hy_bank's index here, hydra.h's) */
        hy_bank (first + k);
        memcpy ((void*) (0x8000 + off), b, n);
        if ((off += n) >= 0x2000) {
            off = 0;
            ++k;
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
    memset (&HUB->pc_task, 0, sizeof HUB->pc_task);     /* (No long task yet) */
    start (argv[3]);                            /* (The things asked as the game starts, the dungeon begun: part 3's) */
    while (hub_map (0), !HUB->quit) {         /* (The hub at $8000 again, each time round) */
        while (hy_sem_try (HUB->sem[T_WORLD]) == 0) {
        }
        stocks ();                              /* (The chunks the maker stocked: their things put down, marked so) */
        if (tr_n >= TR_FLUSH) {
            tr_flush (0);                       /* (The passings journalled taken into the traces' store: part 28's) */
        }
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
        stepping ();                            /* (The long task as the clock goes: a walk a square at a time) */
        if (den_saw && !move.on) {
            den_saw = 0;                        /* (A body fallen: looked at again, the things too) */
            see (0);
        }
        if (HUB->den_seq != dseq) {
            dseq = HUB->den_seq;
            stirred ();                         /* (The denizens' bodies' changes: what it sees of them) */
            hub_map (0);
        }
        if (HUB->round && (burn_ask || HUB->now >= burn_due)) {
            burns_now ();                       /* (The lights burnt down, a game minute's: part 27's; once the game's
                                                **   begun, WQ_START's, its clock a game carried on's) */
            hub_map (0);
        }
        ++HUB->beat[T_WORLD];
        if (turns_run ()) {
            continue;                           /* (The turns pace: the denizens catching up) */
        }
        if (move.on && HUB->now >= HUB->pc_ready && !HUB->pc_task.asking) {
            arrive ();
        }
        if ((was_busy || (fwant & 1)) && !move.on && HUB->now >= HUB->pc_ready && !HUB->pc_task.asking) {
            if (pc_waited) {
                pc_waited = 0;
                ask_look (LK_EYES);             /* (A wait's end: its eyes, as time's gone, the senses') */
            }
            took = take_n ? take_up () : pfoe ? close_on () : srch ? searched () : 0;  /* (A thing walked over to: taken up, or walked on
                                                **   to; a blow's close-in: struck, or walked on) */
            if (!took && HUB->pc_task.kind == LT_BATTER) {
                took = batter_on ();            /* (A battering's next blow, or what came of it told) */
            }
            if (!took && (HUB->pc_task.kind == LT_FIGHT || fwant)) {
                took = fight_on (!was_busy);    /* (Fighting on's next blow, or begun: part 24's; 3, idle still) */
            }
            if (took == 1) {
                see (0);
            } else if (!took) {
                HUB->pc_task.kind = LT_NONE;    /* (Its long task over) */
                event (E_READY, 0, 0);          /* (The character free again: the UI takes its next line) */
            }
            was_busy = took == 2;
        }
        if (pc_tired) {                         /* (Tired a level more by strenuous work: asked, soft, as it goes on,
                                                **   "You're tiring.  Keep digging?", part 23's) */
            pc_tired = 0;
            m.kind = I_ASK;
            m.w1 = TE_ASK_TIRED;
            task_word (&m);
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

/* ==== Part 21: the ways across a room worked out (way's: see the way across a room, above) */
#pragma code-name (push, "SW21CODE")
#pragma rodata-name (push, "SW21RODATA")

/* The step from chunk square x, y the way d, if it can be walked (into the room's walls and floor: wox ...): its
** time, or 0xFFFF (it can't: rock, a wall, a ledge too high, a denizen on it, a trap known of); wtrap 1 if there's
** one that's set on the square stepped onto (the chunk in the window) */
static unsigned char wtrap;
static unsigned __fastcall__ step_by (unsigned char x, unsigned char y, unsigned char d)
{
    unsigned char nx = x + dir_x[d], ny = y + dir_y[d], t = C->tile[y][x], u;
    struct trap* q;
    int dz;

    if (nx >= CW || ny >= CW || (unsigned char) (nx - wox) >= wow || (unsigned char) (ny - woy) >= woh) {
        return 0xFFFF;
    }
    u = C->tile[ny][nx];
    q = trap_at (nx, ny);
    if (!T_PASS (u) || den_on (rx0 + nx, ry0 + ny) || (q && !(q->state & TS_HIDDEN)) || (wr == 0xFF && TILE (u) == T_DOOR)) {
        return 0xFFFF;                          /* (Nor onto a denizen, nor a trap it knows of: it goes round) */
    }
    wtrap = q != 0;
    dz = C->elev[ny][nx] - C->elev[y][x];
    if ((dz < 0 ? -dz : dz) > (TILE (t) == T_STEPS || TILE (u) == T_STEPS ? STAIR_MAX : STEP_MAX)) {
        return 0xFFFF;
    }
    return cost (dz, TILE (u) == T_STEPS);
}

/* The ways from (fx, fy) to every square of room r's floor and walls it can walk to (a search a step at a time, in the
** four ways' order: the first way to each found kept, as the one way's search was), in the ways' bank; the chunk in
** the window after */
void __fastcall__ s_ways_go (unsigned char r, unsigned char fx, unsigned char fy)
{
    static unsigned c[4];
    static unsigned char tr[4];
    struct room* p;
    struct wcell* w;
    unsigned head = 0, tail = 0, i, n, t, q;
    unsigned char x, y, d, b = ovbank[OVBANKS + 1];

    if (r == 0xFF) {                            /* (In a passage: the squares within 12 of it, the chunk's: the box kept
                                                **   in the chunk, its corner never wrapped below it) */
        wox = fx > 12 ? fx - 12 : 0;
        woy = fy > 12 ? fy - 12 : 0;
        wow = wox + 25 > CW ? CW - wox : 25;
        woh = woy + 25 > CW ? CW - woy : 25;
    } else {
        p = &C->room[r];
        wox = p->x - 1;
        woy = p->y - 1;
        wow = p->w + 2;
        woh = p->h + 2;
    }
    hy_bank (b);
    for (y = 0; y < woh; ++y) {                 /* (None reached yet) */
        for (x = 0; x < wow; ++x) {
            if ((unsigned char) (wox + x) < CW && (unsigned char) (woy + y) < CW) {
                WAYS_AT[(unsigned) (woy + y) * CW + wox + x].from = 0xFF;
            }
        }
    }
    i = (unsigned) fy * CW + fx;
    w = &WAYS_AT[i];
    w->from = 4;
    w->t = 0;
    w->trap = 0xFFFF;
    WAYS_Q[tail++] = i;
    while (head < tail) {
        i = WAYS_Q[head++];
        t = WAYS_AT[i].t;
        q = WAYS_AT[i].trap;
        x = i & (CW - 1);
        y = i >> CW_BITS;
        hub_map (at_bank);
        for (d = 0; d < 4; ++d) {
            c[d] = step_by (x, y, d);
            tr[d] = wtrap;
        }
        hy_bank (b);
        for (d = 0; d < 4; ++d) {
            if (c[d] == 0xFFFF) {
                continue;
            }
            n = i + dir_x[d] + (int) dir_y[d] * CW;
            w = &WAYS_AT[n];
            if (w->from != 0xFF) {
                continue;
            }
            w->from = d;
            w->t = t + c[d] < t || t + c[d] > 0xFFFE ? 0xFFFE : t + c[d];
            w->trap = q != 0xFFFF ? q : tr[d] ? n : 0xFFFF;
            WAYS_Q[tail++] = n;
        }
    }
    hub_map (at_bank);
}

/* The square the ways reach nearest chunk square *tx, *ty (go_to's: a place the character can't stand on itself, a
** cave's rock about its middle): 1, it in *tx, *ty; 0 none (the chunk in the window after) */
unsigned char __fastcall__ s_ways_near (unsigned char* tx, unsigned char* ty)
{
    unsigned char x, y, qx, qy, bx = 0xFF, by = 0, d, dy, best = 0xFF;

    hy_bank (ovbank[OVBANKS + 1]);
    for (y = 0; y < woh; ++y) {
        qy = woy + y;
        for (x = 0; x < wow; ++x) {
            qx = wox + x;
            if (qx >= CW || qy >= CW || WAYS_AT[(unsigned) qy * CW + qx].from == 0xFF) {
                continue;
            }
            d = qx > *tx ? qx - *tx : *tx - qx;
            dy = qy > *ty ? qy - *ty : *ty - qy;
            if (dy > d) {
                d = dy;
            }
            if (d < best) {
                best = d;
                bx = qx;
                by = qy;
            }
        }
    }
    hub_map (at_bank);
    if (bx == 0xFF) {
        return 0;
    }
    *tx = bx;
    *ty = by;
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 27: the lights burning (slice 4.1's: DUNGEON.md's light, HOUSE.md's burning) */
#pragma code-name (push, "SW27CODE")
#pragma rodata-name (push, "SW27RODATA")

/* Every light lit burns down as the clock goes, wherever it is (carried, put down, set in a sconce: but not one its
** keepers tend, I_TENDED, nor one that never burns down, a glow, the theme's), a game minute at a time: the store's
** few lit records gone through once a minute (burn_due, the main loop's), the minutes since the last taken off each
** one's fuel (a chunk's things paged out caught up as they're read in: ipage.c's).  So no lighting, putting out or
** filling has any work for it, nor the record any room (a minute's pass of the store, in assembly, and this part's
** swap: off every hot path).  In its last ten minutes it's burning low, told if it's the character's; at none it goes
** out (a candle burnt away, gone; a lantern dry, a torch burnt out, kept to be renewed), told if it's the
** character's, or one it sees (E_TASK's TE_LOW, TE_OUT: b its kind, | 0x80 not the character's), the light layer
** about one lying there out of date (E_LIGHT: the senses'); the character's light made again and looked about in (its
** only light gone, in the dark: its long task stopped hard, part 23's task_out); and the minutes left of the light it
** carries in the hub (pc_fuel: the status line's) */
void __fastcall__ task_out (unsigned char b);   /* (Part 23's) */
unsigned long burn_due;
unsigned char burn_ask;                         /* (The minutes left made again at once: a light's change) */
static unsigned char burn_on;                   /* (The first minute begun) */

/* The character's light's minutes left (f, its brightest's: 0xFFFF it never burns down), in words for the status
** line, the hub's pc_left ("torch 40m", "lantern 5h59m"; "" none: the UI's main RAM's room, as the referee's
** words, pcstatus, are its) */
static void __fastcall__ left_words (unsigned f)
{
    char* e = HUB->pc_left;

    *e = 0;
    if (!HUB->pc_light || f == 0xFFFF) {
        return;
    }
    strncpy (e, HUB->pc_lamp, 12);
    e[12] = 0;
    e += strlen (e);
    *e++ = ' ';
    if (f >= 60) {
        utoa (f / 60, e, 10);
        strcat (e, f % 60 < 10 ? "h0" : "h");
        e += strlen (e);
    }
    utoa (f % 60, e, 10);
    strcat (e, "m");
}

void s_burns_now (void)
{
    struct msg lm;                              /* (On the C stack: the main RAM's room) */
    struct item q;
    const struct burn* b;
    struct item* p;
    unsigned char* sq;
    unsigned i, j, d, left, bf, wx, wy;
    unsigned char mine, r, best, out, seen, ww, ws, low = 0, more = 0;

    hub_map (0);
    burn_ask = 0;
    if (!burn_on) {
        burn_on = 1;                            /* (The first: a minute from now, the game begun or carried on) */
        burn_due = HUB->now + 600;
    }
    for (d = 0; HUB->now >= burn_due; ++d) {    /* (The minutes gone, a minute at a time: no long division; none, a
                                                **   light's change, I_LIGHT's, the minutes left made again) */
        burn_due += 600;
    }
    best = out = seen = 0;
    bf = 0xFFFF;
    for (i = 0; (i = items (i, IN_FLAGS | IN_SQUARE | IN_ACTOR | IN_ITEM | IN_PORTAL, I_LIT)) != 0; ) {
        memcpy (&q, ip, sizeof q);
        for (j = i; (p = item_at (j))->where == IW_ITEM; j = p->x) {   /* (Up to whoever has it, or where it lies) */
        }
        ww = p->where;                          /* (Copied: the window moves) */
        ws = p->sn;
        wx = p->x;
        wy = p->y;
        mine = ww == IW_ACTOR && !wx;
        hub_map (CAT_BANK);
        r = mine ? CATALOGUE[q.kind].light : 0;
        if (!q.id || (q.flags & I_TENDED) || !(b = burn_of (q.kind))) {
            more |= mine;                       /* (A light of its own lit that lasts: no other's burning low told) */
            if (r > best) {                     /* (The brightest it carries: never burning down) */
                best = r;
                bf = 0xFFFF;
            }
            continue;
        }
        left = q.fuel > d ? q.fuel - d : 0;
        item_at (i)->fuel = left;
        if (r > best) {
            best = r;
            bf = left;
        }
        if (left) {
            if (mine && left > BURN_LOW) {
                more = 1;                       /* (Another of its lights lit with more left) */
            } else if (mine && q.fuel > BURN_LOW) {
                low = q.kind;                   /* (Burning low now: told below, if it's the last of them) */
            }
            continue;
        }
        burn_ask = 1;                           /* (Its minutes left made again, the next time round) */
        if (!b->fuels) {
            item_at (i)->kind = 0;              /* (Burnt away: a candle gone) */
        } else {
            item_at (i)->flags &= ~I_LIT;       /* (A lantern dry, a torch burnt out: to be renewed) */
        }
        if (mine) {
            out = q.kind;                       /* (The character's: its light made again, below) */
            continue;
        }
        if (ww != IW_SQUARE && ww != IW_PORTAL) {
            continue;                           /* (An actor's: the denizens' lights, to come) */
        }
        lm.kind = E_LIGHT;                      /* (One lying there: the light layer about it out of date) */
        lm.a = ws;
        lm.w1 = wx;
        lm.w2 = wy;
        hub_map (0);
        lm.t = HUB->now;
        hub_event (&lm);
        if (lm.a == HUB->pc_stratum && (sq = at (lm.a, lm.w1, lm.w2)) != 0 && (*sq & T_VIS)) {
            hub_map (0);
            evb = q.kind | 0x80;                /* ("The torch gutters and goes out.": one it sees) */
            event (E_TASK, 0, TE_OUT);
            seen = 1;
        }
    }
    if (low && !more) {
        hub_map (0);
        evb = low;
        event (E_TASK, 0, TE_LOW);              /* ("Your torch is burning low.": the last lit it carries; one of two,
                                                **   once the other is too, or out) */
    }
    if (d) {
        sconce_lights (0xFF);                   /* (The rooms' lights by their sconces, a minute's: one burnt out, its
                                                **   keepers gone: wkept.c's) */
    }
    if (out || seen) {
        if (out) {
            light_now ();                       /* (Its light made again: another lit still, or none) */
            hub_map (0);
            HUB->pc_left[0] = 0;                /* (Its words too, the next time round) */
        }
        see (0);                                /* (Looked about in the new light) */
        hub_map (0);
        if (out && (sq = at (HUB->pc_stratum, HUB->pc_x, HUB->pc_y)) != 0 && !(*sq & T_VIS)) {
            task_out (out);                     /* (In the dark: its long task stopped, part 23's) */
        } else if (out) {
            hub_map (0);
            evb = out;
            event (E_TASK, 0, TE_OUT);          /* ("Your torch gutters and goes out.": another lit still, or a
                                                **   room's own light) */
        }
        view (V_DARK);
    } else {
        hub_map (0);
        HUB->pc_fuel = bf;
        left_words (bf);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
