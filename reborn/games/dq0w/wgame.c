/*
** wgame.c - (dq0w) the game's own rules, the world's (Phase 2: one authority on state): the theme's puzzle judged
** (theme.h: the UI's command of it, items.h's IO_PUZZLE, the things found to hand and the rule for their kinds
** looked for, done as it says: the things made one, put, the way out opened, the hub's way_open; the UI words it), and
** a thing walked over to and taken up (I_TAKE: the walk the world's go_to, again as long as it gets nearer; then the
** thing taken up as the item store's IO_GET takes it, E_TAKEN told).  And the character's state and the game's (move
** 2.7): the game begun (the UI's WQ_START: the ruleset's round, the pace and its real ticks a round, the character's
** pace and watchfulness), the character's pace and watchfulness, its stance, the game paused (WQ_SET), and the world's
** part of the save (world.dat, hub.h's struct wsave: the clock, the character's place, its pace, its watchfulness and
** its body's position, its bearings, the puzzle's, the strata's records), written with the game (WQ_SAVE) and read
** back as it's carried on.
**
** All of it swapped part 16's (../dq0/swap.inc: dq0w.cfg's SW16), called through its stubs (bank.s's).
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"

extern unsigned char pmode;                     /* (dq0w.c's: the legs' way said) */
extern unsigned char nobear;                    /* (dq0w.c's: dq0 -N, the tests', its bearings never tested) */
#include "work.h"

#pragma local-strings (on)

unsigned take_n;                                /* (things.h's: I_TAKE's thing, its item; 0 none) */
static unsigned take_x, take_y;                 /*   its square, */
static unsigned take_fx, take_fy;               /*   and where the walk to it began */
static unsigned char take_only;                 /*   1: walked over to alone (I_TAKE's a: the UI's command again) */
static unsigned char done_;                     /* (The rules done, a bit each: the save's, the UI's quest) */
extern unsigned char bold_;                     /* (dq0w.c's: a pace said outright, for the dark) */
static char an[16], bn[16];                     /* (The things named) */

#pragma code-name ("SW16CODE")
#pragma rodata-name ("SW16RODATA")

/* ---- The theme's puzzle */

/* The thing named that's to hand (the item store's IO_FIND, in the request being answered: the UI's, which waits):
** its kind, the theme's (0 ...), or 0xFE (one of the catalogue's); 0xFF if there's none (the things' words why in
** things_text) */
static unsigned char __fastcall__ found (const char* name)
{
    unsigned char k;

    hub_map (0);
    HUB->ireq.kind = IO_FIND;
    strcpy (HUB->ireq.name, name);
    serve ();
    hub_map (0);
    if (HUB->ireq.result != IR_OK) {
        return 0xFF;
    }
    k = HUB->ireq.x;
    return (unsigned char) (k - KI_THEME) < NTKINDS ? k - KI_THEME : 0xFE;
}

/* The rule (verb, a, b) among the theme's: its number, or 0xFF (the theme's bank in the window) */
static unsigned char __fastcall__ rule (unsigned char verb, unsigned char a, unsigned char b)
{
    unsigned char r;
    const struct trule* p = THEME->rule;

    hub_map (THEME_BANK);
    for (r = 0; r < THEME->nrules && r < NRULES; ++r, ++p) {
        if (p->verb == verb && p->a == a && (b == 0xFF || p->b == b)) {
            return r;
        }
    }
    return 0xFF;
}

/* The way out within reach: the character on the top stratum, a square or two from it (its first link's) */
static unsigned char near_out (void)
{
    hub_map (0);
    return !HUB->pc_stratum && STRATUM (0).link[0].x - HUB->pc_x + 2 <= 4 && STRATUM (0).link[0].y - HUB->pc_y + 2 <= 4;
}

/* The hub's ireq, IO_PUZZLE's, answered (but its state, the caller's): its verb's rule for the things named looked
** for, by their kinds, and done (its result PZ_*, x the rule); 1 if what lies about has changed (a thing put) */
unsigned char s_puzzle (void)
{
    static unsigned char v, a, b, r, res, changed, on, k;

    hub_map (0);
    v = HUB->ireq.to;
    strcpy (an, HUB->ireq.name);
    strcpy (bn, HUB->ireq.into);
    hub_map (UI_BANK);
    UIREC->things_text[0] = 0;                  /* (No words of the things' yet) */
    res = PZ_NO;
    r = 0xFF;
    changed = 0;
    b = 0xFF;
    if (v == TR_OUT && !near_out ()) {
        res = PZ_FAR;
    } else if ((a = found (an)) != 0xFF && (v == TR_READ || v == TR_OUT || (b = found (bn)) != 0xFF)) {
        r = rule (v, a, b);
        if (r == 0xFF && v == TR_COMBINE && (r = rule (v, b, a)) != 0xFF) {
            hub_map (0);                        /* (The one that becomes the whole the rule's first) */
            strcpy (bn, an);
            strcpy (an, HUB->ireq.into);
        }
        hub_map (THEME_BANK);
        if (r == 0xFF) {
            res = PZ_NONE;
        } else if (v == TR_READ || v == TR_OUT) {
            res = PZ_DONE;
        } else {                                /* (The things made one, or put: the item store's) */
            on = v == TR_PUT && (THEME->kind[b].keeps & KF_ON);
            k = THEME->rule[r].c;
            hub_map (0);
            HUB->ireq.kind = v == TR_COMBINE ? IO_COMBINE : IO_PUT;
            strcpy (HUB->ireq.name, an);
            strcpy (HUB->ireq.into, bn);
            HUB->ireq.to = v == TR_COMBINE ? KI_THEME + k : on;
            changed = serve ();
            hub_map (0);
            res = HUB->ireq.result == IR_OK ? PZ_DONE : PZ_NO;
        }
    }
    if (res == PZ_DONE) {
        done_ |= 1 << r;
        hub_map (THEME_BANK);
        k = THEME->rule[r].effect;
        hub_map (0);
        if (k == TE_OPEN) {
            HUB->way_open = 1;                  /* (The way out opened: the mission's, E_LEFT as it goes up) */
        }
    }
    hub_map (0);
    HUB->ireq.result = res;
    HUB->ireq.x = r;
    if (res == PZ_FAR) {                        /* (Where the way out is: the UI walks over to it first) */
        HUB->ireq.x = STRATUM (0).link[0].x;
        HUB->ireq.y = STRATUM (0).link[0].y;
        HUB->ireq.item = 0;
    }
    return changed;
}

/* ---- A thing walked over to and taken up (I_TAKE's, as the walk's done and the character's free): 0 if it wasn't
** (E_TAKEN told: none, out of reach still, a portal's part or a thing fixed where it stands, its hands full; E_READY
** after), 1 if it was (told: the UI's to ask the time it took, its light, as for any thing handled), 2 if it's walking
** on to it (nearer than it was: a denizen moved out of its way, the walk cut short) */
unsigned char s_take_up (void)
{
    unsigned n = take_n;
    unsigned char r, was, k, w;
    const struct item* p;

    if (take_only) {                            /* (Walked over to alone: within reach of its square, or as near as
                                                **   it gets; the UI's command again, as the character's free) */
        hub_map (0);
        if (((unsigned) (take_x - HUB->pc_x + 1) > 2 || (unsigned) (take_y - HUB->pc_y + 1) > 2) &&
            (HUB->pc_x != take_fx || HUB->pc_y != take_fy)) {
            take_fx = HUB->pc_x;
            take_fy = HUB->pc_y;
            go_to (take_x, take_y);
            return 2;
        }
        take_n = 0;
        return 0;
    }
    hub_map (0);
    was = HUB->pc_light;
    p = item_at (n);
    k = p->kind;
    w = p->where;
    if (!k) {
        r = IR_NONE;
    } else if (!in_reach (n)) {
        hub_map (0);
        if (HUB->pc_x != take_fx || HUB->pc_y != take_fy) {
            take_fx = HUB->pc_x;                /* (Nearer: on again) */
            take_fy = HUB->pc_y;
            go_to (take_x, take_y);
            return 2;
        }
        r = IR_NOT_HERE;
    } else {
        if ((r = take_rest ()) == IR_NONE) {    /* (A count, taken up as it gets there: the request as it was, part
                                                **   4's: "take 4 rations", the user's) */
            hub_map (KIND_BANK);
            r = w == IW_PORTAL || (CATALOGUE[k].keeps & KF_FIXED) ? IR_FIXED : !noinit && fumbled (k) ? IR_FUMBLED :
                carry (n, 0) ? IR_OK : IR_FULL; /* (Engaged, a grope failed: the round wasted, its words) */
        }
        if (r == IR_OK) {
            reach_done (0);                     /* (Its reach owed, things.c's: the UI's wait for it after) */
        }
        light_now ();                           /* (Its light: what it holds lit) */
    }
    take_n = 0;
    hub_map (0);
    wk.m.kind = E_TAKEN;                        /* (Told: the work's event record) */
    wk.m.a = 0;
    wk.m.w1 = r;
    wk.m.w2 = was;
    wk.m.t = HUB->now;
    hub_event (&wk.m);
    return r == IR_OK || r == IR_FUMBLED;      /* (A fumble's round the UI's to wait, as a thing taken's time) */
}

/* I_TAKE's walk begun (its intent: the square the thing lies on, its item) */
void __fastcall__ s_take_go (const struct msg* m)
{
    take_only = m->a;
    take_n = m->t ? (unsigned) m->t : 1;        /* (Walked over to alone: its square's, perhaps no thing's) */
    take_x = m->w1;
    take_y = m->w2;
    hub_map (0);
    take_fx = HUB->pc_x;
    take_fy = HUB->pc_y;
    go_to (take_x, take_y);
}

/* ---- The character's state, and the game's (move 2.7: the character's state home): the UI's requests of them (the
** hub's wreq: WQ_START, as the game's begun; WQ_SET, as the player says), and the world's part of the save (world.dat:
** hub.h's struct wsave, then the strata's records) */

static const unsigned tprs[4] = { 1000, 600, 400, 0 };  /* Real ticks (200 a second) a round takes, by pace: 5, 3 and 2
                                                **   real seconds, whatever the ruleset's round (the turns pace: 0) */

/* The save folder's world.dat opened (out: to write it), or -1 (no folder) */
static int __fastcall__ world_dat (unsigned char out)
{
    char p[56];

    hub_map (0);
    if (!HUB->savedir[0]) {
        return -1;
    }
    strcpy (p, HUB->savedir);
    strcat (p, "/world.dat");
    return out ? open (p, O_WRONLY | O_CREAT | O_TRUNC, 0644) : open (p, O_RDONLY);
}

/* The world's part of the game kept (WQ_SAVE's, after the change records) */
void s_world_keep (void)
{
    struct wsave w;
    int fd = world_dat (1);

    if (fd < 0) {
        return;
    }
    hub_map (0);
    w.magic = WSAVE_MAGIC;
    w.version = 2;
    w.sn = HUB->pc_stratum;
    w.face = HUB->pc_face;
    w.pos = HUB->pc_pos;
    w.x = HUB->pc_x;
    w.y = HUB->pc_y;
    w.gait = HUB->gait;
    w.watch = HUB->watch;
    w.open = HUB->way_open;
    w.done = done_;
    w.now = HUB->now;
    w.skew = HUB->pc_skew;
    w.lost = HUB->pc_lost;
    write (fd, &w, sizeof w);
    hub_map (WORLD_BANK);                       /* (The strata's records, the world bank's) */
    write (fd, WREC->strata, sizeof WREC->strata);
    hub_map (0);
    close (fd);
}

/* A game carried on: the world's part of its save read back (the clock, the character's place, state and bearings, the
** puzzle's, the strata's records: the world begins from them, s_begin) */
static void world_back (void)
{
    struct wsave w;
    int fd = world_dat (0);

    if (fd < 0) {
        return;
    }
    if (read (fd, &w, sizeof w) == sizeof w && w.magic == WSAVE_MAGIC) {
        hub_map (0);
        __asm__ ("jsr %w", HY_PREEMPT_OFF);
        ++HUB->tick;                            /* (The clock, as advance () writes it) */
        HUB->now = w.now;
        ++HUB->tick;
        __asm__ ("jsr %w", HY_PREEMPT_ON);
        HUB->pc_stratum = w.sn;
        HUB->pc_x = w.x;
        HUB->pc_y = w.y;
        HUB->pc_face = w.face;
        HUB->pc_pos = w.pos;
        HUB->gait = w.gait;
        HUB->watch = w.watch;
        HUB->way_open = w.open;
        HUB->pc_skew = w.skew;                  /* (Its bearings as they were: slice 4.2's) */
        HUB->pc_lost = w.lost;
        done_ = w.done;
        hub_map (WORLD_BANK);
        read (fd, WREC->strata, sizeof WREC->strata);
    }
    hub_map (0);
    close (fd);
}

/* The hub's wreq, WQ_START's or WQ_SET's, answered (but its reply, the caller's): the game begun (the ruleset's round,
** the pace and its real ticks a round, the character's pace and watchfulness; a game carried on, its save's); the
** character's pace and watchfulness set, its stance, the game paused or going on */
void s_pc_asked (void)
{
    unsigned x;

    hub_map (0);
    x = HUB->wreq.x;
    if (HUB->wreq.kind == WQ_START) {
        HUB->round = x;
        HUB->pace = HUB->wreq.y;
        HUB->tpr = tprs[HUB->wreq.y & 3];
        noinit = (HUB->wreq.y & WY_NO_INIT) != 0;   /* (dq0 -I: the tests') */
        nobear = (HUB->wreq.y & WY_NO_BEAR) != 0;   /* (dq0 -N: the tests') */
        HUB->gait = HUB->wreq.a;
        HUB->watch = HUB->wreq.b;
        if (HUB->resume) {
            world_back ();
        }
        return;
    }
    switch (HUB->wreq.a) {
    case WS_PACE:
        if (x != 0xFF) {
            HUB->gait = x & 7;
            pmode = x & (GA_CRAWL | GA_HOP);    /* (The legs' way said: dq0w.c's part 50, slice 4.1b's) */
            bold_ = 0;
            if (x & GA_SAID) {                  /* (Said outright in the dark, or blind: gone at there, its dangers,
                                                **   dq0w.c's paced; said in the light, the dark's walk cautious) */
                x = *at (HUB->pc_stratum, HUB->pc_x, HUB->pc_y) & T_VIS;
                hub_map (0);
                bold_ = !x || (HUB->pcsum.cond & C_BLINDED) != 0;
            }
            HUB->pc_going = bold_ ? GOING_BOLD : 0;
        }
        HUB->watch = HUB->wreq.y;
        if (HUB->watch & WA_MAP) {
            light_now ();                       /* (Mapping as it goes: a map begun on its first sheet, if it has the
                                                **   things to draw one with and no map: things.c's chart_now) */
        }
        break;
    case WS_STANCE:
        HUB->pc_stance = x;
        break;
    case WS_PAUSE:
        HUB->paused = x;
        break;
    }
}
