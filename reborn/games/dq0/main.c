/*
** main.c - Dingus Quest Zero: the UI's task, its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file] [-d folder] [-t theme] [-k things] [-r rules] [-T [-T]]: -d keeps the game in a
** folder (on any card), and carries on the game kept there if there is one; without it nothing's kept (the folder: life.h's,
** the record of the character's life, its sheet and its things, each dungeon in a folder of its own). the seed makes
** the same game each time (the dice: dice.h's rseed), and its region's history and theme (themegen's: theme.h), but
** a seed under 65,536 a stand-in theme's (the tests'); -t a theme written by hand, the history made to fit it; the
** pace is r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (the ruleset's: hmchar's).  Without one, the
** ruleset's character generator (its referee names it: hmchar) is run to make one (in this window, the screen as it
** is outside the game while it runs), into /ram/dq0.hm; a ruleset with none makes its own.  The rules: the ruleset's
** name (RULESETS.md: harn, HarnMaster's, if none's given; null, flat odds), its referee libdir/dq0NAME (the save's: a
** game carried on keeps its own).  The tests' own: -C, -a, -T, -K (the kit as it was before the outfits, the denizens
** with none of theirs: their dice as they were), and -I (no engage initiative test, nor a grope's test
** while engaged: HOUSE.md's, the user's way to keep the tests' fights as they were).
**
** The game is tasks that meet in the hub (hub.h): this one, the UI, makes it and starts the others (the world,
** libdir/dq0w; the referee, libdir/dq0harn: the rules, asked in rs.h's terms; the maker, libdir/dq0m: the
** dungeon grown, as the world asks; the senses, libdir/dq0p; the denizens, libdir/dq0d; the sound, libdir/dq0s).
** The loop: each key goes to the line being typed (term.c); a line entered is the character's next command, made an
** intent for the world when the character's free (the world says so: E_READY); the world's events are worded into
** the story.  Escape pauses the game and goes on again.
**
** Its start and its end (part 1) and the commands (part 2) are swapped parts (swap.inc: run in the main RAM's top,
** one at a time, copied in as they're called), between #pragma code-name (push, "SWnCODE") and its pop: each called
** from outside its part only through its stub (bank.s's SWFN: begin, finish, save_game; command, take).  A line
** typed goes first to pace.c's (part 4: the pace, the watchfulness, the state, small moves).
*/

#include <fcntl.h>
#include <stddef.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "../dq0w/hub0.h"
#include "rs.h"
#include "level.h"
#include "life.h"
#include "cmds.h"                               /* (The command handlers' shared calls, by meaning) */
#include "dq0said.h"                            /* (The phrase table's own outcomes: the build's, said.txt's) */

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

unsigned long now;                              /* The game's time, as the world's clock last said */
unsigned char pc_face;                          /* (The character's facing, copied for the overlays) */
unsigned char pace = 0xFF, paused;
static unsigned char kseq = 1;                 /* (The hub's ui_keys as a line's typed: odd, each line's own) */
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

static unsigned char asking;                    /* (A long task's soft question put, the answer awaited) */
const char* cl;                                 /* (The command's line, s_command's: its handlers' words of it; nameis.s's) */
unsigned char harm_told;                        /* (The states a harm's words told in this answer: intro.c's) */
unsigned char out_told;                         /* (Knocked senseless, told: no line taken till it comes round, the
                                                **   world's pc_out; out_line's) */
unsigned char autogo;                           /* (dq0 -a: every soft question answered yes, unasked: the tests') */
unsigned char what_hi;                          /* (The thing's kind's top bit, the long task's what its other seven: task.c's noun) */
unsigned char take_all;                         /* ("Take all" under way, its last thing too: the pack first, resolve.c's) */
unsigned char take_more;                        /* ("Take all": things still to come after this one, resolve.c's) */
unsigned char nosteps;                   /* (dq0 -J: the reach's steps none, the tests') */
static unsigned char noinit;                    /* (dq0 -I: no engage initiative test, no grope's: the tests') */
static unsigned char nobear;                    /* (dq0 -N: its bearings never tested: the tests', slice 4.2's) */
static unsigned char asked;                     /* (A sound's question put in this command's long task) */
static unsigned bar_done;                       /* (The long task's part done as the bar was last drawn) */
unsigned char busy = 1;                         /* The character's doing something: no command till it's free (nor
                                                **   till the world's started, and says so: E_READY) */
unsigned char nap = 40;                         /* (main's nap with nothing doing, ticks: o_dots's, a fiftieth once it's started) */
unsigned char starting;                         /* The game's starting: its progress shown (dots: hub.c's waits,
                                                **   main's loop), till the character's first ready; 2 once its
                                                **   first look's begun (words.c's): no dots after it */
unsigned char held;                             /* (A line typed as it's busy, held till it's free; */
static unsigned char took;                      /*   and the fight it was in stopped for it, once) */
static unsigned place;                          /* Where this task is in the world's events */
static int task_[NTASKS];                       /* Each task's number, as it was started (0: not) */
static unsigned char test_start;
static unsigned char clock_on;                  /* (dq0 -m: the minutes gone on the status line, for the tests and debugging) */
static unsigned char rolls;                     /* (dq0 -R n: the dice's offset, the tests', 0-252) */
static unsigned char small;                     /* (dq0 -C: the caches small, the tests') */
static unsigned char gear;                      /* (dq0 -K: the kit as it was, no outfits drawn, the tests') */
static const char* kit;                         /* (dq0 -k: things more, as the game starts: a tool, ...) */
static const char* tfile;                       /* (dq0 -t: a theme written by hand) */
unsigned char hard;                             /* (Hard mode: dq0 -H's, the save's pace's top bit) */
const char* savedir;                            /* (dq0 -d: the save's folder; task.c's meta too) */
unsigned char __fastcall__ things (unsigned char op, const char* name);   /* (way.c's too, part 6's) */
void gear_look (void);                          /* (Part 11: a denizen's things made out) */
void __fastcall__ came (const struct msg* m);    /* (Part 6's: come into a stratum, told and kept) */
void __fastcall__ news (const struct msg* m);    /* (Part 11's: the rarer news, worded; a thing hidden; the hands' and
                                                **   limbs': slice 4.1b's) */
void hand_said (void);
void limbs_cmd (void);
void __fastcall__ pose_words (unsigned w);      /* (Part 14's: the body's position, worded) */
void __fastcall__ noise_told (const struct msg* m, unsigned char rel);  /* (Part 14's: a denizen's sound, told) */
unsigned char hold_first (void);
unsigned char errand (void);                    /* (Part 15's: an ally's errand, a gift, slice 4.8's) */
unsigned char __fastcall__ hide (const char* w);   /* (Part 18's) */
unsigned char missiles (void);                  /* (Part 13's: the missiles, slice 4.7's) */
void __fastcall__ dig_used (unsigned bk);        /* (Part 14's) */
char dug_with[16];                              /* (The tool a dig took, named) */
unsigned char finds;                            /* (Things a search turned up, told as it ends: the hub's text;
                                                **   search.c's too) */
struct ireq judged;                             /* (A thing looked at closely: what it is, for the appraiser: way.c's,
                                                **   part 6's, examine) */
struct portal viewed;                           /* (A portal looked at closely: the world's record of it) */
char pending[16];                        /* A thing to take up as the character's free (a portal's part lifted
                                                **   out; one the world's walk found fixed: E_TAKEN's) */
static char taking[16];                         /* (The thing taken up last named) */
static char lifting[16];                        /* (A portal's part being lifted out, to take up after) */
unsigned char barw;                             /* (A bar to hand to bar a door with, its width: hplaces.c's portal) */
static unsigned char rebar;                     /* (The door barred: the bar to set in its brackets, as the character's
                                                **   free) */
unsigned char strike;                           /* (What it strikes with: the parser's, SK_*, sent with each blow) */
unsigned char shand;                            /* (The hand that holds it, if it's said: items.h's IH_*, sent with each
                                                **   blow; the hand a thing's to go in, a hold's: slice 4.1b's) */
char nil[1];                                    /* ("", in the main RAM: an overlay's name for othings) */
char hold_w[16];                                /* (A blow's thing to take in hand first, its target found: resolve.c's,
                                                **   part 11's hold_first) */
static unsigned char pc_gait, pc_watch;         /* (The character's file's pace and watchfulness: the referee's) */

struct pc P;
struct quest Q;                                 /* The theme's puzzle, as it stands (theme.c's; the save's) */
char tname[32];
volatile unsigned char quit;

static void interrupted (int sig)
{
    (void) sig;
    quit = 1;
}

/* ---- Where its files are: the directory it was started from (rc's $prog), if its lib/dq0 is there; else the
** card's /sd/0/games (dq0 found through /bin, bound there) */

char gamedir[40], libdir[48];

/* The ruleset's own words (its referee's, RS_HELLO's: rs.h; 2026-11-12, ENGINE-CRITIQUE.md's section 8): its words for
** the state C_STUNNED is (HarnMaster's "in shock"), its word for its own stance, D_OWN ("counter": "" none), and that
** stance told ("You will strike back first.") */
char rs_stun[12], rs_own[10], rs_owns[32];

/* A variable of the environment (the kernel's ENV_GET: this task's) into buf, size bytes at most with its 0 (a list's
** words a space between, as getenv gives them): buf, or NULL (it isn't set).  The C library's getenv not wanted: its
** 256 bytes' buffer, the main RAM's */
char* __fastcall__ env (const char* name, char* buf, unsigned char size)
{
    struct hy_regs r;
    unsigned char n, i;

    r.a = 0xFF;
    r.r[0] = (unsigned) name;
    r.r[1] = (unsigned) buf;
    r.r[2] = --size;
    r.r[3] = 0;
    if (hy_call (HY_ENV_GET, &r)) {
        return 0;
    }
    n = r.x || r.a > size ? size : r.a;
    if (n && !buf[n - 1]) {
        --n;
    }
    for (i = 0; i < n; ++i) {
        if (!buf[i]) {
            buf[i] = ' ';
        }
    }
    buf[n] = 0;
    return buf;
}

/* The game's folder (prog's), and its lib/dq0, if its overlays are there; else the card's: the overlays' file opened
** (its fd: ovl_load's, opened once, 2026-10-30) */
static int find_dirs (void)
{
    char pv[sizeof gamedir + 1];
    const char* prog = env ("prog", pv, sizeof pv);
    char* slash;
    unsigned char k;
    int fd;

    for (k = 0; k < 2; ++k) {
        if (k || !prog || strlen (prog) >= sizeof gamedir || (slash = strrchr (strcpy (gamedir, prog), '/')) == 0) {
            strcpy (gamedir, "/sd/0/games");
        } else {
            *slash = 0;
        }
        strcpy (libdir, gamedir);
        strcat (libdir, "/lib/dq0/dq0.ovl");
        fd = open (libdir, O_RDONLY);
        libdir[strlen (libdir) - 8] = 0;
        if (fd >= 0) {
            break;
        }
    }
    return fd;
}

/* ---- The overlays: libdir/dq0.ovl (fd, find_dirs'), an 8K bank each (dq0.cfg's OV1 ...; the file cut after the
** last one used: make.js), read into banks of the game's own.  Each bank's bytes in the file as make.js packs them
** (ovlpack.js): packed, a bank on its own (LZ4's block), read and unpacked in place by the unpack library
** (modules/unpack: 2026-10-31), the linker's fill ($FF) put back after its bytes, so each bank's as ld65 made it; all
** of it the game's library's (modules/dq0lib's OVL_LOAD, dq0lib.inc's ovl_load: 2026-11-09), the file closed, the
** window's bank put back.  The game's start's first words before it (the screen not made yet: 2026-10-30, the start's
** progress shown), and a dot halfway (the overlays read in two runs, a dot after the first twelve banks, bank.s's
** OVL_DOT: the screen's silence under its 36M cycles, 2026-11-06, the designators' overlay 23) */
static void __fastcall__ ovl_in (int fd)
{
    write (1, "Dingus Quest Zero is starting ...", 33);
    if (fd < 0 || ovl_load (fd)) {
        hy_exits ("dq0: no overlays (lib/dq0/dq0.ovl, the dq0lib and unpack libraries), or no RAM banks");
    }
}

/* ---- The character: the ruleset's (hmchar's), its file read by the referee */

/* A line written to the window's consctl: on an fd kept open (raw mode lasts while the consctl that set it is) */
void __fastcall__ consctl (const char* what)
{
    static int fd = -1;

    if (fd < 0) {
        fd = open ("/dev/consctl", O_WRONLY);
    }
    if (fd >= 0) {
        write (fd, what, strlen (what));
    }
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

#pragma code-name (push, "SW7CODE")
#pragma rodata-name (push, "SW7RODATA")

/* A game's own theme (theme.h): a game carried on, its save's (Q.theme 0xFF: its folder's); a new one's (a seed of
** 65,536 or more, or dq0 -t's) its region's history and its theme made by themegen from the game's seed, in the
** save's folder (or /ram/dq0): theme.txt, and its record, theme.dat; a hand-written theme's (dq0 -t) filled out.  The
** theme to read (theme.c's): 0xFF the game's own; else a stand-in's, the save's or the seed's second byte's (a small
** seed's, or themegen can't be run, or it failed).  (Part 7's, part 1 full: its arguments in lb, nothing the game's
** yet; its own spawn, hub_run being part 1's) */
static unsigned char resumed;
static unsigned long seed;
static char spath[56];                          /* (A path in the dungeon's folder, itself sdl bytes of it, with -d: dq0life's,
                                                **   s_life's; save_path's, part 14's save_at's, part 19's; "../" the save's) */
static unsigned char sdl;
unsigned char __fastcall__ s_history (unsigned long seed)
{
    struct hy_regs r;
    char* p = lb;

    if (resumed) {
        return Q.theme;
    }
    if (!tfile && seed < 65536UL) {
        return (unsigned char) (seed >> 8);
    }
    msg ("The place's history is being made ...");
    t_flush ();                                 /* (Told now: themegen's dots after it, -p's) */
    strcpy (p, "-q");
    strcpy (p += 3, "-p");
    strcpy (p += 3, "-s");
    ultoa (seed, p += 3, 10);
    strcpy (p += strlen (p) + 1, "-o");
    if (savedir) {                              /* (The dungeon's folder: a path in it there, cut for now) */
        spath[sdl] = 0;
    }
    strcpy (p += 3, savedir ? spath : "/ram/dq0");
    spath[sdl] = '/';
    strcpy (p += strlen (p) + 1, "-l");
    strcpy (p += 3, libdir);
    p += strlen (p) + 1;
    if (tfile) {
        strcpy (p, "-t");
        strcpy (p += 3, tfile);
        p += strlen (p) + 1;
    }
    *p = 0;
    strcpy (lb + 300, gamedir);
    strcat (lb + 300, "/themegen");
    r.a = 0;
    r.r[0] = (unsigned) (lb + 300);
    r.r[1] = (unsigned) lb;
    if (hy_call (HY_SPAWN, &r)) {
        return (unsigned char) (seed >> 8);
    }
    r.r[0] = 0;                                 /* (Its task in r.a: waited for, its status 0) */
    return hy_call (HY_WAIT, &r) || r.x ? (unsigned char) (seed >> 8) : 0xFF;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The character's file for the referee to read (RS_LOAD_PC's path, put in the UI's slot): file (-c's, or the save's);
** else one the ruleset's character generator makes (maker, the referee's RS_HELLO names it: hmchar ...), run in this
** window (the console cooked for it, the screen whole: on the terminals the game's on), its arguments -o FILE -s SEED;
** else none (maker ""): the referee makes its own */
static void __fastcall__ character (const char* file, unsigned long seed, const char* maker)
{
    static char s[32] = "-o\0/ram/dq0.hm\0-s", s2[48];
    int task;

    if (!*file && *maker) {
        file = s + 3;                           /* (hmchar -o /ram/dq0.hm -s seed) */
        strcpy (s2, gamedir);
        strcat (s2, "/");
        strcat (s2, maker);
        t_away ();
        consctl ("rawoff");
        ultoa (seed, s + 18, 10);
        s[19 + strlen (s + 18)] = 0;
        task = hub_run (s2, s);
        if (task < 0 || hub_join (task)) {
            t_end ();
            hy_exits ("dq0: its character generator made none");
        }
        consctl ("rawon");
        t_init ();
    }
    hub_map (0);
    strcpy (HUB->slot[SLOT_UI].text, file);
}

/* The referee's reply's text: its next line (cut at its \n), or NULL */
static char* __fastcall__ reply_line (char** at)
{
    char* l = *at;
    char* e;

    if (!*l) {
        return 0;
    }
    if ((e = strchr (l, '\n')) != 0) {
        *e++ = 0;
    } else {
        e = l + strlen (l);
    }
    *at = e;
    return l;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The referee asked (the UI's slot): its request's kind, and a (a purpose ...) and b, the slot's text already set */
struct req* __fastcall__ ask_b (unsigned char kind, unsigned char a, unsigned char b)
{
    struct req* r = &HUB->slot[SLOT_UI];

    r->kind = kind;
    r->actor = 0;
    r->a = a;
    r->b = b;
    r->steps = 0;
    return hub_ask (SLOT_UI);
}

#define referee(kind, a)    ask_b ((kind), (a), 0)

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The character's file read by the referee (its path in the slot already: character's): its name and its calling */
static void load_pc (void)
{
    struct req* r;
    char* at;
    char* l;

    r = referee (RS_LOAD_PC, 0);
    if (RS_LOAD_PC_BAD (*r)) {
        at = (char*) (RS_LOAD_PC_BAD (*r) == 2 ? "dq0: too many tongues and scripts in its file" : "dq0: no character in its file");
        t_end ();
        write (2, at, strlen (at));             /* (Said, and its status: refused aloud, never cut short) */
        write (2, "\n", 1);
        hy_exits (at);
    }
    at = r->text;
    if ((l = reply_line (&at)) != 0) {
        strncpy (P.name, l, sizeof P.name - 1);
    }
    if ((l = reply_line (&at)) != 0) {
        strncpy (P.occupation, l, sizeof P.occupation - 1);
    }
    pc_gait = RS_LOAD_PC_GAIT (*r);                             /* (A new game's, the world's to set) */
    pc_watch = RS_LOAD_PC_WATCH (*r);
    r->state = REQ_FREE;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The save (DUNGEON.md): game.dat, the UI's (the game's state, the strata's records), beside the things' items.dat
** (dq0p's), the referee's pc.hm, the world's change records (changes.dat and the regions' rg*.dat: world.h's) and
** the maker's seen.dat */

/* SAVE_VER, life.h's: (2: the items' records with their size, the portals' make; 3, the
                                                **   theme and its puzzle; 4, the denizens' attitudes, the factions',
                                                **   the names given; 5, the creatures' sheets; 6, the names the
                                                **   character's given, after the strata's: names.c's; 7, the change
                                                **   records the world's, by region: its changes.dat, then rg*.dat,
                                                **   a version 6 changes.dat read in as the game's carried on; 8, the
                                                **   world's own, world.dat: the clock, the character's place, state
                                                **   and position, the puzzle's, the strata's records, no longer
                                                **   here, the names after the header: a save of 5 to 7 made into
                                                **   one, old_world, as the game's carried on; 9, no fixed size, the
                                                **   stores paged to the folder: the region files in versions,
                                                **   changes.dat their stamps, items.dat its chunks' table after its
                                                **   banks; the save's last step the world's, so the region files
                                                **   written back since the last are the save's only once it's
                                                **   whole; 10, the old saves' readers gone, the last save's fields
                                                **   the world's now with them: till the release, only a save of this
                                                **   version is carried on; 11, the ruleset's referee: a game carried
                                                **   on with the ruleset it was made with; 12, a thing's concealment
                                                **   in steps, rs.h's scale; 13, each denizen's record its faction:
                                                **   the theme's inhabitants; 14, the items' records their fuel,
                                                **   the region files' groups the minute they were put out: the
                                                **   lights burning, slice 4.1's; the places kept lit after the
                                                **   denizens' dens.dat; 15, the items' records their shut, a
                                                **   thing's lid, its tie, its stopper, its lock; 16, the player's
                                                **   shortcuts after the names: alias.c's; 17, the catalogue's axe, the
                                                **   theme's kinds a number on; the hub's actors' legs and weapons let fall:
                                                **   slice 4.1b's; 18, the seen maps' drawn squares and each chunk's
                                                **   memory, the maker's; the catalogue's paper ... map, the theme's
                                                **   kinds seven on, the map in plain letters: slice 4.2's; 19, the
                                                **   world's world.dat the character's bearings: slice 4.2's; 20, each
                                                **   chunk's memory the north believed in as it was last seen: the
                                                **   maker's seen maps, travel's, slice 4.2's; 21, the catalogue's
                                                **   kinds 128, the theme's from 120, the outfits' kinds; the items'
                                                **   records a denizen's own, IW_DEN: the outfits' gear; 22, its
                                                **   belongings where it sleeps, its record's D_MET, its kinds past
                                                **   128; 23, worth in farthings, a theme's mints (its record's
                                                **   head the longer), a pile of coins' mints in its size, thick and
                                                **   fuel: the money's, 2026-10-28; 24, the denizens' records' serve
                                                **   and contact, D_LOST: morale's bonds, 2026-10-30; 25, their
                                                **   D_RANK and D_PROM: succession; 26, the denizens' head's
                                                **   deeds, a faction's regard: slice 4.8's; 27, world.dat's
                                                **   hunger and thirst: slice 4.9's; 28, the denizens' records' want (ate,
                                                **   drank), the creatures' bodies after their sheets (beasts.dat:
                                                **   their fatigue), 2026-10-31; 29, world.dat's time of day believed,
                                                **   the hub's tb, 2026-10-31; 30, the diagonal ways' cut
                                                **   corners (T_CUT) in the chunks and the change records; 31, the
                                                **   tunnellers: DO_DIG among the doings, slice 4.11's; 32: the same seed's
                                                **   natural chunks not as they were, great caverns and sloping tunnels
                                                **   in them, a dungeon's change records laid on what they were made
                                                **   for, 2026-11-02; 38, the body line's venom numbers, the cave spider's among them, 2026-11-04; 41, the ends' solved in the quest; 42, the outcome (life.h's), the sheet's copy
                                                **   gone; each dungeon in its own folder, life.dat; kit.dat each thing as it is) */
static struct save sv;
static unsigned char resumed;
unsigned char plain;

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

static char* __fastcall__ save_path (const char* name)
{
    strcpy (spath + sdl + 1, name);
    return spath;
}

/* The game's folder (dq0 -d) as it begins: LF_GOING, a game kept in its dungeon's folder, its state read; else lib/dq0/dq0life run (it
** says what's next, dq0life.c's), the seed its: LF_NEXT, a dungeon past the first, its sheet the character's, to go on in (D8's,
** slice 4.4's); LF_NEW, the first of a new life: a new game, a new character.  No folder, LF_NEW */
static unsigned char __fastcall__ life (unsigned char given)
{
    int fd;
    unsigned char n = LF_NEW;

    if (savedir) {
        life_dir ();
        if ((fd = open (save_path ("game.dat"), O_RDONLY)) >= 0) {
            if (read (fd, &sv, sizeof sv) == sizeof sv && sv.magic == SAVE_MAGIC && sv.version == SAVE_VER && !sv.q.done) {
                n = LF_GOING;
            }
            close (fd);
        }
        if (n) {
            between (given);                    /* (dq0life: it begins the next dungeon, or a new life) */
            fd = hub_join (hub_run (lb + 300, lb));
            if (fd == 2) {
                hy_exits (0);                   /* (Dead between the dungeons, by the rolls: told, the game's not to begin) */
            }
            if (fd) {
                hy_exits ("dq0: dq0life failed");
            }
            life_dir ();
            seed = ((struct life_head*) lb)->dseed;
            if (((struct life_head*) lb)->dungeon > 1) {
                n = LF_NEXT;
                hard |= ((struct life_head*) lb)->flags >> 3 & 1;   /* (LF_HARD's: carried on in hard mode, life.h's) */
            }
        }
    }
    return n;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW3CODE")             /* (Part 3's, the map's: part 1's room, slice 4.4's; none of the commands'
                                                **   path the longer for it, part 13's copied in at every command) */
#pragma rodata-name (push, "SW3RODATA")

/* The character's sheet (0), the referee's words; or the map (1), with its side's lines (part 2's room, 2026-11-03) */
void __fastcall__ s_sheet_map (unsigned char what)
{
    struct req* r = ask_b (RS_DESCRIBE, 0, what);

    if (what) {
        map (r->text);                          /* (Its lines read where they are, in the hub's header, before the
                                                **   map's drawn) */
    } else {
        blank ();
        say (r->text);
        blank ();
    }
    r->state = REQ_FREE;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW14CODE")            /* (Part 14's since 2026-11-03: overlay 19's bank's room) */
#pragma rodata-name (push, "SW14RODATA")

static char* __fastcall__ save_at (const char* name)
{
    strcpy (spath + sdl + 1, name);
    return spath;
}

/* The game kept: the referee's sheet, the maker's seen maps, the denizens' records, the world's (its change records,
** its things, its own: its last step, the region files written made the save's), and the game's state (the hub's
** header in the window) */
void s_save_game (void)
{
    struct req* r;
    struct mreq* q;
    int fd;

    if (!savedir) {
        return;
    }
    strcpy (HUB->slot[SLOT_UI].text, save_at ("../pc.hm"));
    r = referee (RS_SAVE, 0);
    r->state = REQ_FREE;
    q = &HUB->mreq[ASK_UI];                     /* (The maker's seen maps: asked, and waited for) */
    q->kind = M_SAVE;
    hub_call (q, T_MAKER);
    q->state = REQ_FREE;
    if (task_[T_DENIZENS] > 0) {                /* (The denizens': dens.dat) */
        hub_call (&HUB->dsave, T_DENIZENS);
        HUB->dsave.state = REQ_FREE;
    }
    if (task_[T_WORLD] > 0) {                   /* (The world's: its change records, its things, items.dat, its
                                                **   own, world.dat; then the save's versions, its last step) */
        HUB->wreq.kind = WQ_SAVE;
        hub_call (&HUB->wreq, T_WORLD);
        HUB->wreq.state = REQ_FREE;
    }
    sv.magic = SAVE_MAGIC;
    sv.version = SAVE_VER;
    sv.pace = pace | hard << 7;                 /* (Hard mode, its top bit: slice 4.4's) */
    sv.seed = HUB->seed;
    sv.outcome = Q.solved ? LO_SOLVED : Q.done == END_LEFT ? LO_LEFT : Q.done ? LO_DIED : LO_NONE;
    sv.q = Q;
    sv.plain = plain;
    if (fasting > HUB->now) {                   /* (A vow of fasting, its time left in FAST_STEPs: life.h's) */
        sv.plain |= (unsigned char) ((fasting - HUB->now + FAST_STEP - 1) / FAST_STEP) << 1;
    }
    if ((fd = open (save_at ("game.dat"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, &sv, sizeof sv);
        names_keep (fd, 0);                     /* (The names given: the theme bank's) */
        alias_keep (fd, 0);                     /* (The shortcuts: the theme bank's too) */
        letters_keep (fd, 0);                   /* (The letters: the UI bank's, letters.c) */
        close (fd);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The save's names given (after its header) */
static void __fastcall__ load_names (void)
{
    int fd = open (save_path ("game.dat"), O_RDONLY);

    if (fd >= 0) {
        read (fd, &sv, sizeof sv);
    }
    keeps_back (fd);                            /* (The names, the shortcuts, the letters: letters.c's, part 22's) */
    if (fd >= 0) {
        close (fd);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The hub; the referee started, the character loaded, the theme read in, and the world started */

/* Bytes copied between one of the hub's banks and the main RAM (to or from in the window), the window put back as it
** was: the hub reached from an overlay's code (theme.c's) */
void __fastcall__ hub_mem (unsigned char bank, void* to, const void* from, unsigned n)
{
    unsigned b = hub_here ();

    (hub_map) (bank);
    memcpy (to, from, n);
    hub_back (b);
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* A task of the game's started, libdir/name in role t (more: its third argument, or none); or the game's over (but
** for the sound's: the game goes on silent) */
static void __fastcall__ start (const char* name, unsigned char t, const char* more)
{
#pragma bss-name (push, "SW1RODATA")              /* (The part's own: no other part called while it's needed; the
                                                **   main RAM's room, 2026-11-11) */
    static char path[56];
#pragma bss-name (pop)

    strcpy (path, libdir);
    strcat (path, "/");
    strcat (path, name);
    if ((task_[t] = hub_spawn (path, t, more)) < 0 && t != T_SOUND) {
        t_end ();
        strcpy (path, "dq0: can't start lib/dq0/");
        strcat (path, name);
        hy_exits (path);
    }
}

/* A reply waited for as the game starts (q posted already), a dot told every second or so meanwhile (o_dots's: the
** start's progress, 2026-10-30: the referee's and the world's start, their programs and overlays read from the card,
** some 10 and 30 s on the board) */
static void __fastcall__ waited (void* q)
{
    while (!hub_wait_t (q, 250)) {
        dots (0);
    }
}

static void world_words (void);

static void __fastcall__ start_world (unsigned long seed, const char* file, unsigned char made)
{
    struct req* r;
    unsigned char k;
    unsigned round;
    int w;
    char* l;
    char* p[4];

    if (hub_create (HUB_BANKS)) {
        t_end ();
        hy_exits ("dq0: no shared segment or semaphores");
    }
    HUB->starting = 1;
    HUB->seed = seed;
    HUB->test_start = test_start;
    HUB->dice = rolls;
    HUB->small = small;
    HUB->gear = gear;
    hub_map (DEN_BANK);                         /* (The sounds' words: none till the sound task's put them there; the
                                                **   places kept lit, the denizens' as they decide its populations) */
    memset (SND_AT, 0, (2 * SND_KINDS + SND_WORLD) * SND_REC);
    memset (KEPT_AT, 0, sizeof (struct kept));
    hub_map (0);
    start ("dq0c", T_COMMANDS, libdir);         /* (Its command parser: its tables read as the others start) */
    start ("dq0s", T_SOUND, libdir);            /* (Its sound: SOUND.md) */
    start (sv.referee, T_REFEREE, libdir);      /* (Its rules, the ruleset's referee: its folder, for its overlay) */
    r = &HUB->slot[SLOT_UI];                    /* (The referee's hello: its first request, the rest of it 0 as the
                                                **   hub's made; waited for with the start's dots) */
    r->kind = RS_HELLO;
    hub_post (r, T_REFEREE);
    waited (r);
    round = RS_HELLO_ROUND (*r);                              /* (The ruleset's round, in tenths: the world's to keep) */
    for (l = r->text, k = 0; k < 4; ++k) {      /* (Its name, then its character generator's (none, ""), then its
                                                **   words, rs.h's: each line cut at its \n) */
        if (l && (l = strchr (l, '\n')) != 0) {
            *l++ = 0;
        }
        p[k] = l ? l : (char*) "";
    }
    strncpy (rs_stun, p[1], sizeof rs_stun - 1);
    strncpy (rs_own, p[2], sizeof rs_own - 1);
    strncpy (rs_owns, p[3], sizeof rs_owns - 1);
    character (file, seed, p[0]);               /* (The character: its file, or the generator's) */
    r->state = REQ_FREE;
    load_pc ();                                 /* (Its senses the referee's, before the world looks through them) */
    if (savedir) {                              /* (The dungeon's folder) */
        spath[sdl] = 0;
        strncpy (HUB->savedir, spath, sizeof HUB->savedir - 1);
        spath[sdl] = '/';
    }
    if (resumed) {                              /* (Carried on: where it was, and when, the strata as they were: the
                                                **   world's, its world.dat) */
        load_names ();
        HUB->resume = 1;
    } else {
        keeps_back (-1);                        /* (No names given yet, no shortcuts, no letters) */
    }
    start ("dq0m", T_MAKER, libdir);            /* (Its maker, its senses, its world: the world's item store first,
                                                **   the things asked as the game starts, till it's begun) */
    start ("dq0p", T_SENSES, libdir);
    start ("dq0w", T_WORLD, libdir);
    w = task_[T_WORLD];
    task_[T_WORLD] = 0;                         /* (Not begun yet: the status line's) */
    HUB->wreq.kind = WQ_START;                  /* (Its clock's round and pace, the character's pace and watchfulness:
                                                **   a game carried on, its save's, the world's) */
    HUB->wreq.x = round;
    HUB->wreq.y = pace | (noinit ? WY_NO_INIT : 0) | (nobear ? WY_NO_BEAR : 0);
    HUB->pc_lump = nosteps;                     /* (PL_NOSTEPS) */
    HUB->wreq.a = pc_gait;
    HUB->wreq.b = pc_watch;
    hub_post (&HUB->wreq, T_WORLD);
    waited (&HUB->wreq);
    HUB->wreq.state = REQ_FREE;
    things (IO_TOOLS, "");                      /* (The world's catalogue read in: the theme's kinds after it) */
    theme_load (made);                          /* (The game's own, 0xFF; a stand-in's, theme.h: history's) */
    things (IO_THEME, "");
    world_words ();
    if (!resumed) {                             /* (The character's kit: a new game's, -k's things too) */
        hub_mem (UI_BANK, UIREC->things_text, kit ? kit : "", kit ? strlen (kit) + 1 : 1);  /* (Its things more,
                                                **   commas between) */
        things (IO_KIT, "");
    }
    to_hand ();
    for (k = 0; task_[T_SOUND] > 0 && !HUB->beat[T_SOUND] && k < 100; ++k) {   /* (The sounds' words in the hub
                                                **   before anything's heard: a second at most) */
        hy_sleep_ticks (2);
        hub_map (0);
    }
    start ("dq0d", T_DENIZENS, libdir);         /* (Its denizens; then its world begun) */
    things (IO_BEGIN, "");
    task_[T_WORLD] = w;
}

/* The command parser told the world's words: the catalogue's kinds' names (the theme's with them), so a word of its
** grammar's is a thing's name too where one's called by it ("drop the pick", "wear the knee boots"; the game's own
** things, "door", "room", it knows); no names given yet; and the world's deities' names, "call on Imma"
** (in the kinds' bank: items.h's GODS_AT, deities.txt's, spaces between) */
static char nm[sizeof CATALOGUE[0].name + 1];
static unsigned n;
static const char* wfrom;

/* Words from the kinds' bank (wfrom there), up to their 0, into the parser's (each part's copy through nm, in the main RAM) */
static void words_in (void)
{
    unsigned char k;

    do {
        hub_map (KIND_BANK);
        memcpy (nm, wfrom, sizeof nm - 1);
        hub_map (CMD_BANK);
        for (k = 0; k < sizeof nm - 1 && nm[k]; ++k) {
        }
        memcpy (HUB_WINDOW + n, nm, k);
        n += k;                                 /* (The last piece's too, 2026-11-04: the theme's went over it, "take the
                                                **   grapple" not understood) */
        wfrom += k;
    } while (k == sizeof nm - 1);
}

static void world_words (void)
{
    unsigned char i, k;

    n = 0;
    wfrom = KIND_WORDS;
    words_in ();                                /* (The catalogue's words, each once, the build's: dq0w/make.js's, in
                                                **   the kinds' bank after them; "kite shield", "tower shield": the
                                                **   parser's lists are words, its room CQ_WORDS) */
    for (i = KI_THEME; i < KI_THEME + NTKINDS; ++i) {   /* (Then the theme's kinds') */
        hub_map (KIND_BANK);
        memcpy (nm, CATALOGUE[i].name, sizeof CATALOGUE[0].name);
        hub_map (CMD_BANK);
        if ((k = strlen (nm)) != 0 && n + k + 2 < CQ_WORDS) {
            memcpy (HUB_WINDOW + n, nm, k);
            HUB_WINDOW[n + k] = ' ';
            n += k + 1;
        }
    }
    HUB_WINDOW[n++] = 0;                        /* (Then the names given: none) */
    HUB_WINDOW[n++] = 0;
    wfrom = GODS_AT;                            /* (Then the deities') */
    words_in ();
    HUB_WINDOW[n] = 0;
    hub_map (0);
    HUB->cq.kind = CQ_WORLD;
    HUB->cq.len = n + 1;
    hub_call (&HUB->cq, T_COMMANDS);
    HUB->cq.state = REQ_FREE;
}

/* The game's end: its tasks told to stop (the world first: its I_QUIT, the hub's quit its to set), and waited for */
static void stop_world (void)
{
    static struct msg m;
    unsigned char t;

    m.kind = I_QUIT;
    hub_send (&m);
    for (t = T_WORLD; t < NTASKS; ++t) {
        if (task_[t] > 0) {
            hub_ring (t);
            hub_join (task_[t]);
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The answer's tasks in the kernel's foreground (TASKFORE: base/kernel/sched.s), at the real paces: the UI for good (FS_UI:
** so a line's Enter is looked for at the next tick, a tick at most behind the background's work; a driver serving its
** calls, the console's, with it), and the world, the command parser, the senses and the referee while a command's in
** hand (FS_CMD: the hub's ui_cmd).  The senses with them: the world waits on their look and their stock (in the
** background, an inventory's answer was 60,000 cycles the slower).  And the maker but while a command's in hand (FS_MAKE:
** asked for a chunk or the map, someone waits on it, the player mostly, a chunk the longest wait in the game: in the
** background 12M cycles, a chunk's 5.5M of its own among the denizens and the rest; asked for nothing, asleep), and the
** senses always (a look's rest after its first word, the sound's pulse after it: over the sooner, not lying under the next
** line's answer; asked for nothing, asleep).  Not at
** the turns pace: there the game waits for the player, and its tasks take their turns as they did (fore_sync's; overlay
** 6's) */
#pragma code-name (push, "OV13CODE")            /* (Overlay 13's and 3's since 2026-11-04, overlay 17's room) */
#pragma rodata-name (push, "OV13RODATA")
void __fastcall__ o_fore_set (unsigned char on)
{
    static const unsigned char fg[] = { T_WORLD, T_COMMANDS, T_REFEREE, T_SENSES, T_MAKER }, fm[] = { FS_CMD, FS_CMD, FS_CMD, FS_SENS, FS_MAKE };
    unsigned char k;

    hy_taskfore (hy_task (), (on & FS_UI) != 0);
    for (k = 0; k < sizeof fg; ++k) {
        if (task_[fg[k]] > 0) {
            hy_taskfore (task_[fg[k]], on & fm[k]);
        }
    }
}

/* The game's start, its progress shown (2026-10-30, the user's: a start of some seconds is fine with its progress
** shown; on the board the start is a minute and more, the card's reading most of it).  k 2: the world's tasks being
** started (s_begin's), told; 3: the dungeon begun, the character's first look to come, told.  0: a dot as the start
** waits on a task's reply (hub.c's hub_wait_t: its doorbell's wait out); 1: main's loop round while the game starts,
** a dot each second or so till its first look begins (starting 2, words.c's o_words: no dot after its words, the
** user's, 2026-11-13); the character ready the first time (its first look told, E_READY), the start over: the
** help's line said, with the prompt after it */
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "OV3CODE")
#pragma rodata-name (push, "OV3RODATA")
void __fastcall__ o_dots (unsigned char k)
{
    static unsigned at;

    if (k >= 2) {
        if (k == 3) {
            blank ();
        }
        msg (k == 2 ? "The dungeon is being made ready ..." : "Nearly ready ...");
    } else if (k && !busy) {
        starting = 0;
        nap = 4;
        blank ();
        tell (T_TYPE_HELP_FOR);
        return;
    } else if (starting == 2 || hy_ticks () - at < 240) {
        return;                                 /* (Its first look begun: no dot after its words) */
    } else {
        say (".");
    }
    at = hy_ticks ();
    t_flush ();
}
#pragma code-name (pop)
#pragma rodata-name (pop)

unsigned char fored;                            /* (The tasks in the foreground: FS_*, fore_sync's) */
static unsigned char iqh, owed, obeat;          /* (The intents' ring as a line's taken; an answer owed, main's;
                                                **   the world's heartbeat as it was sent) */
static unsigned char stirs;                     /* (A denizen's stir held while a line's answered: events', main's) */
static void fore_sync (void)
{
    static unsigned char f;

    if (owed && !busy && HUB->iq_tail == HUB->iq_head && (unsigned char) (HUB->beat[T_WORLD] - obeat) >= 2) {
        HUB->ui_cmd = owed = 0;                 /* (Its intent taken and the world round again since: an answer
                                                **   without news to the character, if any, given) */
    }
    f = HUB->tpr ? FS_UI | FS_SENS | (HUB->ui_cmd ? FS_CMD : FS_MAKE) : 0;
    if (f != fored) {
        fore_set (fored = f);
    }
}

unsigned char cq_st;                            /* The line posted to the command parser: none (0), its reply not
                                                **   taken (CQ_OUT), or taken (CQ_IN: its struct cmd in CMD_BANK) */
#define CQ_OUT      1
#define CQ_IN       2
static signed char cq_res;                      /* (Its verdict, cq_cmd's) */
unsigned char cq_again;                         /* The line changed since it was posted (names_subst's): posted again */
struct cmd cmd;                                 /* The command being carried out (the parser's, cq_cmd's) */
unsigned char reach;                            /* (5: a step of the reach under way, the command kept and asked again as
                                                **   it's up, 6; part 19's step_go.  1: walking over, the command kept (the UI bank's) to carry out
                                                **   again as it's free; 2: carried out again; 0 again as a long task's
                                                **   stopped, task.c's, or a line's command begun; 3: a thing's timed
                                                **   work under way, words.c's, the command kept the same, 4: carried
                                                **   out again, its end; 6: walking over for a command the world carries
                                                **   out as it gets there: a thing taken, a blow) */
unsigned char said_ahead;                       /* (A walk over told as the thing's asked for, walk_ahead's: not told again) */
unsigned char step_k, step_n;                   /* (dq0.h's) */
unsigned char step_aw;                        /* (A step's thing told apart, its AW_*: step_go's, s_reach_said's) */
unsigned char walk_op, worded;                  /* (The things' request it's asked for, IO_*; its first word told this line: walk_ahead's) */
unsigned char placing;                          /* (A place in the room walked to: place_go's, or a walk told as it began,
                                                **   walk_says': its arrival not "over to it") */
unsigned char untarget;                         /* (The target let go at the next command: resolve.c's) */

/* The heading the character believes h is (the hub's pc_skew: its bearings, the world's, slice 4.2's: the north it
** believes in, the compass's words and the status line's), from the true one */
unsigned char __fastcall__ believed (unsigned char h)
{
    return (h + HUB->pc_skew) & 7;
}

/* ---- The status line (overlay 19's, through its stub status: the main RAM's room, 2026-11-11, overlay 27's till the
** outcome events' words came there, the same day; the hub read through
** hub_mem, whatever's in the window as it's called, its left side the name itself, the line drawn by o_t_status, its
** own overlay's) */

static char sright[72];                         /* (Its right side, in the main RAM: bar_text's, overlay 3's, writes
                                                **   it.  Its longest: paused, a fight's words, the state, the facing;
                                                **   dq0 -m, the minutes) */
#pragma code-name (push, "OV19CODE")
#pragma rodata-name (push, "OV19RODATA")
void o_status (void)
{
    static const char* const posw[] = { "", " PRONE", " CROUCHED", " KNEELING", " SITTING", "", "", "" };
    char* right = sright;
    struct { unsigned len, done; } t;
    unsigned char f[3];
    char n[16];

    strcpy (right, paused ? "PAUSED  " : "");
    if (task_[T_WORLD] > 0) {
        hub_mem (0, &t, &HUB->pc_task.len, sizeof t);
        bar_text (right, bar_done = t.done, t.len);   /* (A long task's bar, its part done; a fight's foe; the light's
                                                **   minutes left: task.c's) */
        hub_mem (0, n, HUB->pcstatus, sizeof n);
        strcat (right, n);                      /* (The referee's words: "UP 0" ...; prone, said: slice 4.1b's) */
        hub_mem (0, f, &HUB->pc_pos, 1);
        hub_mem (0, f + 1, &HUB->pc_face, 1);
        hub_mem (0, f + 2, &HUB->pc_lost, 1);
        strcat (right, posw[f[0] & 7]);         /* (Not standing, said: slices 4.1b's, 4.6's) */
        strcat (right, "  facing ");
        hub_mem (0, n, &HUB->pc_skew, 1);
        strcat (right, head_abbr[(f[1] + n[0]) & 7]);   /* (The way it believes it faces: slice 4.2's, believed's) */
        strcat (right, f[2] ? "?  " : "  ");
    }
    if (clock_on) {                             /* (The game's minutes gone, no seconds: for the tests and debugging
                                                **   alone, the user's, 2026-11-03: the player sees what the character
                                                **   believes, not the true hour) */
        strcat (right, utoa ((unsigned) (now / 600), n, 10));
        strcat (right, " min");
    }
    o_t_status (P.name[0] ? P.name : "Dingus Quest Zero", right);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The world's events, worded */

/* ---- The denizens' events (DENIZENS.md: events), as the character hears them: by its hearing (the referee's, 0-9),
** a step or a noise within its loudness times (hearing + 3) / 2 squares, on its stratum, of one on a square it doesn't
** see (one it sees is told as seen: the world's V_STIR), told by the way it lies, in its sound's words (the sound
** task's table, SOUND.md: each square of wall or rock between four squares further, each shut door two, as it's
** played); not the same way again for half a minute, nor any for five seconds.  The rest pass it by.  Listening one
** way (way.c's lsn: "listen at the door"), twice as far that way (HOUSE.md's) */
static unsigned dplace;
unsigned long heard_at;
unsigned char lsn;                              /* (A listen, or a sniff, under way: way.c's) */
unsigned char heard_rel = 0xFF;
char heard_what[20];


static void heard (void)
{
    static struct msg m;
    unsigned char rel;

    while (!quit && hub_dnext (&dplace, &m)) {   /* (Dead of a blow: the end told once, the rest of its round's untold) */
        if (m.kind == E_FIGHT) {                /* (The character's, one at it; or a stray's, between two actors) */
            blow (&m);
            continue;
        }
        if (m.kind == E_BEAT) {
            if (E_BEAT_BEAT (m) == WB_DRAWS || actor_seen (E_BEAT_WHOSE (m))) {
                news (&m);                      /* (A fight's limbs' news, after its blow's words: part 11's, a thing
                                                **   let go of, a fall; another's only if it's seen, not in the dark
                                                **   or out of sight; a weapon drawn to strike, beside it, as its blow
                                                **   is, the outfits' gear) */
            }
            continue;
        }
        if (m.kind == E_DEN && E_DEN_LOUD (m) && !(E_DEN_SEEN (m) && E_DEN_DE (m) < DE_RALLY) && E_DEN_SN (m) == HUB->pc_stratum &&
            (rel = den_heard (&m)) < 8) {       /* (Weighed: caps.c's, the main RAM's room; a rally told seen too: its
                                                **   words, part 14's; a snore, a word on watch, the day's) */
            noise_told (&m, rel);               /* (Told: part 14's, from the main code, the hub in the window after) */
        }
    }
}

static unsigned char events (void)          /* (How many there were) */
{
    static struct msg m;
    unsigned char k, n = 0;

    while (!quit && hub_next (&place, &m)) {
        ++n;
        if (!m.who && (busy | owed) && (m.kind != E_VIEW || E_VIEW_WHY (m) != V_STIR)) {
            HUB->ui_cmd = owed = 0;             /* (The character's news: a command's answer begun, its intent out;
                                                **   not a denizen stirring, the background's, held till after it) */
        }
        switch (m.kind) {
        case E_TIME:
            if (m.who == 0) {
                time_said (E_TIME_TENTHS (m), E_TIME_HOW (m));   /* ("Time passes.", "You wait eight hours.": task.c's) */
            }
            break;
        case E_READY:
            if (m.who == 0) {
                busy = step_k = 0;
                placing = 0;                    /* (Its walk's over, whatever its legs, wherever stopped) */
                asking = 0;                     /* (A fight's question, its fight over as it asked: let go) */
                heard ();                       /* (What came before it, told before the prompt: a blow's words;
                                                **   knocked senseless by one, busy again; dead of one, the
                                                **   game's end: no prompt, however the tasks' timing falls) */
                if (lsn) {
                    look_way (VB_NONE);         /* (A listen's or a sniff's end: way.c's) */
                }
                if (news_due && (task_words (TE_BODY, 0) & 8)) {
                    ending (END_DIED);             /* (Its body's news held as it slept: told as it wakes, slice 4.3's) */
                }
                if (!quit) {
                    t_prompt (!busy);
                }
            }
            break;
        case E_BYE:
            quit = 1;
            break;
        case E_ARRIVE:
            if (m.who == 0) {
                if ((m.w1 & 0xFF) == A_DOWN || (m.w1 & 0xFF) == A_UP) {
                    came (&m);                  /* (Come into a stratum: told and kept, part 6's) */
                }
                k = E_ARRIVE_HOW (m);
                if (k == A_CROSSED) {
                    tell (T_YOU_CROSS_THE);
                } else if (k == A_WENT_TO && !placing && !E_ARRIVE_HALTED (m)) {   /* (Stopped short) */
                    tell (T_YOU_GO_OVER);
                }
                if (HUB->pc_task.kind == LT_TRAVEL) {
                    lb_salt = HUB->pc_x + HUB->pc_y;    /* (A journey's leg: its look's words said of where it's
                                                **   come to, the place told whole, its things and denizens seen
                                                **   before once an answer, lbsaid.s's; a walk's, the room's words
                                                **   once: "The ceiling is flat." not again across it) */
                }
                if (reach) {
                    look (LOOK_NEW);            /* (Walked over for a command: only what's new, its outcome after it,
                                                **   the user's, 2026-11-02) */
                    break;
                }
                                                /* (As a run or a sprint ends, briefly: the pace not held) */
                look ((m.w1 >> 8 & 1) | ((k == A_WALKED || k == A_CROSSED || k == A_WENT_TO || k == A_STEPPED) &&
                    HUB->gait > GA_JOG && !(HUB->watch & (WA_TRAPS | WA_LISTEN | WA_MAP)) && !sight_get ()->dark ? 4 :
                    0));
            }
            break;
        case E_VIEW:
            if (m.who == 0) {
                if (E_VIEW_WHY (m) == V_STIR) {
                    if ((t_next () || inn || HUB->ui_cmd) && (HUB->tpr || !busy)) {
                        stirs = 1;              /* (A line typed or its answer owed, at the real paces: told after its answer, main's,
                                                **   the background's news not in its way, 2026-10-30; or one being
                                                **   typed: told as it's entered and answered, or let go, its echo
                                                **   and its Enter not behind the stir's words.  At the turns pace a line
                                                **   typed or being typed, or just taken, nothing going: its answer's own, so what it says isn't told twice,
                                                **   the stir's words after its echo and its look after them: 2026-11-04) */
                    } else {
                        stir ();                /* (A denizen it sees moved) */
                    }
                    break;
                }
                if ((E_VIEW_WHY (m) & ~1) == V_FACING) {  /* (A turn's first words, as it's begun: out at once, its look
                                                **   after, V_TURNED's; a step's or a walk's, V_SETOFF's: b its way) */
                    turned_says (E_VIEW_WAY (m));
                    t_flush ();
                    break;
                }
                look ((unsigned char) E_VIEW_WHY (m) & (V_LOOKED | V_TRACED));  /* (A look one way's own words: way.c's;
                                                **   the traces read, words.c's) */
            }
            break;
        case E_TASK:                            /* (The long task's: stopped, asked, refused; slice 4.0's) */
            if (m.who == 0) {
                if (E_TASK_TE (m) == TE_BODY) {
                    heard ();                   /* (The blow that brought it told first: a venomous bite's) */
                }
                k = task_words (m.w1, m.b);
                if (k & 2) {
                    intend2 (I_GO_ON, 0, 0);    /* (dq0 -a's: on, unasked) */
                } else if (k & 1) {
                    asking = 1;
                    t_prompt (1);               /* (Its answer's prompt: the character still busy) */
                }                               /* (Other news of it as the question stands, a light burning low, not its
                                                **   answer: the world asking still, "go on" held as a command for the
                                                **   character to be free, no prompt ever again: seed 358's hang) */
                if (k & 8) {
                    ending (END_DIED);             /* (Dead of its wounds, its venom: TE_BODY's, slice 4.3's) */
                }
                if (k & 4) {
                    busy = 1;                   /* (Fighting on begun by the world: busy till it's done, E_READY; a
                                                **   line held taking it over) */
                    t_prompt (0);
                    if (held && !took && reach != 5) {   /* (Not in a step of the reach: as they're done) */
                        took = 1;
                        intend2 (I_STOP, 1, 0);
                    }
                }
            }
            break;
        case E_BEAT:                            /* (A walk's, as it goes; the limbs', part 11's: slice 4.1b's; the walk's
                                                **   too since 2026-11-11, said.txt's BEAT rows, beat_words' before) */
            if (m.who == 0) {
                news (&m);
            }
            break;
        case E_NOWAY:                           /* (A way refused: told at once, the main RAM's) */
            if (m.who != 0) {
                break;
            }
            if (E_NOWAY_WHY (m) == N_DEN) {
                strcpy (lb, HUB->actor[E_NOWAY_ACTOR (m) & 7].name);  /* (A denizen's: its name, for overlay 4) */
                den_way ();
            } else if (m.w1 == N_SHUT) {
                way_words ();                   /* (The theme's: its lock) */
            } else {
                tell (m.w1 == N_NO_STAIRS ? T_THERE_S_NO : m.w1 == N_NO_DIG ? T_THERE_S_NOTHING : m.w1 == N_CLOSED ?
                    T_THE_PORTCULLIS_IS : m.w1 == N_DOOR ? T_DOOR_SHUT : T_YOU_CAN_T);
            }
            break;
        case E_BEARING:                         /* (Its bearings lost, or set right: slice 4.2's) */
        case E_DIG:                             /* (The rarer news, its words part 11's: the main RAM's room, slice
                                                **   4.1b's) */
        case E_LEDGE:
        case E_CLIMB:
        case E_HARM:
        case E_PORTAL:
            if (m.who == 0) {
                news (&m);
            }
            break;
        case E_TRAP:
            if (m.who == 0) {
                untarget |= E_TRAP_TO (m) == TO_SPRUNG;   /* (Set off: the character turns, its target let go) */
                outcome_words (&m);              /* (Its words said.txt's: said.c) */
            }
            break;
        case E_TAKEN:                           /* (A thing walked over to and taken up: the world's, I_TAKE) */
            if (m.who == 0) {
                took_up (E_TAKEN_IR (m) | E_TAKEN_LIT (m) << 8);
            }
            break;
        case E_LEFT:                            /* (Out of the dungeon, solved or not; a take rule done: theme.c's) */
            if (m.who == 0) {
                ending ((unsigned char) E_LEFT_END (m));
            }
            break;
        }
    }
    return n;
}

/* The character come down or up a way between strata (E_ARRIVE's A_DOWN, A_UP: the link's kind in w1's high four bits:
** a ladder, a stairway, a shaft, a long slope, slice 4.10's), told, and the game kept (main's events' till 2026-11-02:
** the main RAM's room; part 6's, its room) */
#pragma code-name (push, "SW6CODE")
#pragma rodata-name (push, "SW6RODATA")
void __fastcall__ s_came (const struct msg* m)
{
    unsigned char k = E_ARRIVE_LINK (*m);

    if (k == O_RAMP_DOWN || k == O_RAMP_UP) {
        msg (k == O_RAMP_DOWN ? "You walk down the long slope." : "You trudge up the long slope.");
    } else if (k == O_WELL || k == O_WELL_FELL) {  /* (A well's shaft: 2026-11-06, the world's wwell.c) */
        msg (k == O_WELL_FELL ? "You come up in the water at the foot of the well." : (m->w1 & 0xFF) == A_DOWN ?
            "You climb down the well, into the water at its foot." : "You climb up out of the well.");
        if (E_ARRIVE_MOUTH (*m)) {
            sayf (" Far above, its mouth shows as a small ring of light.");
        }
    } else {
        tell ((m->w1 & 0xFF) == A_DOWN ? (k == O_LADDER_DOWN ? T_YOU_CLIMB_DOWN :
            k == O_SHAFT_DOWN ? T_YOU_CLIMB_DOWN_2 : k == O_FELL ? T_PICK_YOURSELF_UP : T_YOU_GO_DOWN) :
            k == O_LADDER_UP ? T_YOU_CLIMB_THE : k == O_SHAFT_UP ? T_YOU_CLIMB_UP : T_YOU_CLIMB_THE_2);
    }
    theme_named (TN_STRATUM, sight_get ()->sname);
    sayf (" You come into the %s.", tname);
    save_game ();                               /* (Kept as it comes into a stratum) */
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 11: the rarer news of the world's, worded (events()'s, the character's: a dig, a ledge, a climb, a harm, a
** portal worked; its own part, the main RAM's room: slice 4.1b's) */
#pragma code-name (push, "SW11CODE")
#pragma rodata-name (push, "SW11RODATA")

/* The state a harm leaves the character in (shock, bleeding, a leg or an arm useless: rs.h's C_*, cond its low byte),
** said.txt's HARM.state: each once an answer, then "still" (a second harm's: a door battered, its trap after; task.c's
** state_words before, 2026-11-11) */
static void __fastcall__ state_said (unsigned char cond)
{
    static const unsigned char c_[] = { C_STUNNED, C_BLEEDING, C_LEG_USELESS, C_ARM_USELESS };
    struct msg c;
    unsigned char i;

    c.kind = E_HARM;
    c.who = 0;
    c.w1 = SAID_HARM_STATE;
    said_s[2] = rs_stun;                        /* (The ruleset's words for C_STUNNED's state: "in shock") */
    for (i = 0; i < sizeof c_; ++i) {
        if (cond & c_[i]) {
            said_n = i * 2 + ((harm_told & c_[i]) != 0);
            outcome_words (&c);
        }
    }
    harm_told |= cond & ~1;
}

/* A walk's beat (E_BEAT's WB_QUIET ... WB_SPENT, c a copy), its row's words worked out (said.txt's BEAT rows: the
** dark, a bend's bearing by the north the character believes in, a dark walk's heading, a thing's plural, a swat's
** foe and the thing swung, its arg and its words): 0, nothing to say by the rows (said here, or not at all) */
static unsigned char __fastcall__ beat_arg (struct msg* c)
{
    unsigned char b = c->w1 >> 8, k, i, f;

    hub_mem (SIGHT_BANK, &f, &SIGHT->dark, 1);
    said_n = f;
    switch ((unsigned char) c->w1) {
    case WB_TURN:                               /* (In the dark: as its feet go, no passage's name, no bearing; else a
                                                **   bend's sixteenth, by the north it believes in, plus 1) */
        if (!f) {
            said_n = b & 0x10 ? 1 + ((b + (HUB->pc_skew << 1)) & 15) : 1;
        } else {
            said_n = 0;
        }
        break;
    case WB_CROSS:                              /* (A room crossed to a door, a passage: "You cross the room."; in the
                                                **   dark, no room's told) */
        if (!f) {
            tell (T_YOU_CROSS_THE);
        }
        return 0;
    case WB_SIDES:                              /* (A dark walk, as what its hands find on either side changes: its low
                                                **   four bits now, left then right; its high four as they were) */
        for (k = 0; k < 2; ++k, b >>= 2) {
            f = (b >> 4) & 3;
            i = b & 3;
            if (f != i && (!i || i == 2 || f != 2)) {
                said_n = (!i ? 0 : i == 2 ? 2 : 4) + k;
                outcome_words (c);
            }
        }
        return 0;
    case WB_FEEL:                               /* (The way by the north it believes in: slice 4.2's) */
        if (reach == 2) {                       /* (Its command again, walked over first: told as the walk began) */
            return 0;
        }
        worded = 2;                             /* (Its way told: not "You set off south." after it: facing_words') */
        said_n = (b + HUB->pc_skew) & 7;
        break;
    case WB_THING:
    case WB_TRIP:
    case WB_STUMBLE:
        hub_mem (KIND_BANK, &k, &CATALOGUE[b].flags, 1);
        said_n = (k & K_PLURAL) != 0;
        break;
    case WB_SWAT:                               /* (A blow at a denizen with a soft thing: its name, the thing the hub's
                                                **   pc_strike's) */
        strcpy (lb + 300, HUB->actor[(b - 1) & 7].name);
        said_s[0] = lb + 340;
        strcpy (lb + 340, lb[300] >= 'a' ? "the " : "");
        strcat (lb + 340, lb + 300);
        hub_mem (KIND_BANK, lb + 370, CATALOGUE[SK_KIND (HUB->pc_strike)].name, sizeof CATALOGUE[0].name);
        lb[370 + sizeof CATALOGUE[0].name] = 0;
        said_s[1] = lb + 370;
        break;
    case WB_SPENT:                              /* (A thing used up, its kind: HOUSE.md's things used up; a remedy drunk
                                                **   told as it's drunk, 2026-11-02) */
        return b < KI_ANTIDOTE;
    }
    return 1;
}

void __fastcall__ s_news (const struct msg* m)
{
    unsigned char k;
    struct req* r;
    struct msg c;

    switch (m->kind) {
    case E_BEAT:
        if (E_BEAT_BEAT (*m) == WB_SEARCH) {
            search_said (m);                    /* (A room searched whole's: search.c's) */
            break;
        }
        if (E_BEAT_BEAT (*m) == WB_POSED) {     /* (The body's position as it said, slice 4.6's: part 14's) */
            pose_words (m->w1);
            break;
        }
        c = *m;
        if (E_BEAT_BEAT (*m) < WB_DROP) {       /* (A walk's beat as it goes, in the dark or not: said.txt's BEAT rows,
                                                **   beat_words' and felt_words' before, task.c's, 2026-11-11) */
            if (beat_arg (&c)) {
                outcome_words (&c);
            }
            break;
        }                                 /* (The limbs' news, every actor's, and the water's: said.txt's BEAT
                                                **   rows, limb_words' and water_words' before, 2026-11-11) */
        said_n = BD_TWO;                        /* (The character's body: its rows every actor's, 2026-11-11) */
        if ((k = E_BEAT_WHOSE (*m)) & 0x80 && actor_seen (k) == 2) {
            c.w1 = SAID_BEAT_UNSEEN;            /* (A fall unseen, near: the world's b | 0x80, part 8's, 2026-10-30) */
        } else if (k) {
            k = HUB->actor[(k - 1) & 7].body;   /* (A creature's getting up, failing to, falling, worded by its body:
                                                **   "The cave adder coils itself up again", kinds.txt's) */
            said_n = k < NBODIES ? k : BD_TWO;
        }
        outcome_words (&c);
        break;
    case E_BEARING:                             /* (Its bearings: lost, known, or set right, a room it knows; the sky
                                                **   seen, the time it believes, time.c's) */
        if (E_BEARING_BRG (*m) == BRG_SKY) {
            time_ui (TU_SKY | (E_BEARING_SKY_REFUSAL (*m) ? m->w1 & 0xFF00 : E_BEARING_SKY_SKY (*m) << 8));   /* (A shaft's
                                                **   refusal's, its high byte) */
            break;
        }
        tell (m->w1 == BRG_LOST ? T_LOST_BEARINGS : T_BEARINGS_BACK);
        break;
    case E_DIG:                                 /* (Its words said.txt's, said.c's: the tool it digs with, and the one
                                                **   newly taken up, named once, the UI's) */
        k = E_DIG_HOW (*m);
        said_s[1] = dug_with;
        said_s[2] = dig_with;
        if (k == D_BEGUN || k == D_AGAIN) {
            dug_with[0] = 0;
        } else if (k != D_PORTAL) {
            dig_used (m->b | k << 8);           /* (And the rock it's into told: slice 4.10's; D_NO_TOOL's b what it's
                                                **   into, why told by what it carries, 2026-11-02; a better tool, its
                                                **   name: their words') */
        }
        outcome_words (m);
        if ((k >= D_GOOD && k <= D_SLIP) || k == D_ROOF) {
            dig_with[0] = 0;                    /* (Named: once) */
        }
        break;
    case E_LEDGE:                               /* (A ledge come to: said.txt's own rows, CLIMB.rises and .falls) */
        k = E_LEDGE_RISE (*m) > 0 ? E_LEDGE_RISE (*m) : -E_LEDGE_RISE (*m);
        c.kind = E_CLIMB;
        c.who = 0;
        c.w1 = (E_LEDGE_RISE (*m) > 0 ? SAID_CLIMB_RISES : SAID_CLIMB_FALLS) << 8 | k;
        outcome_words (&c);
        break;
    case E_CLIMB:
        outcome_words (m);
        break;
    case E_HARM:                                /* (The referee's words of its wounds (the row's text), then the row's of
                                                **   it all, said.txt's HARM: harm_words' before, 2026-11-11), its arg 1
                                                **   if it's dead of it; then the state it's left in, task.c's) */
        r = ask_b (RS_DESCRIBE, 0, 2);
        strcpy (lb + 200, r->text);             /* (Under 200: the referee's hurt[]) */
        r->state = REQ_FREE;
        said_s[2] = lb + 200;
        said_n = (HUB->pcsum.cond & C_DEAD) != 0;
        outcome_words (m);
        if (!said_n && (E_HARM_HC (*m) == HC_STRUCK || (E_HARM_BAND (*m) && !(m->w1 & E_CAUGHT)))) {
            state_said ((unsigned char) HUB->pcsum.cond);
        }
        if (HUB->pcsum.cond & C_DEAD) {
            ending (END_DIED);                     /* (The game's end) */
        }
        out_told |= (m->w1 & E_OUT) != 0;       /* (Knocked senseless: no line taken till it comes round, main's) */
        break;
    case E_PORTAL:
        k = E_PORTAL_PO (*m);
        if (finds == 0xFF) {                    /* (A search's end: the things concealed about it too, as far as the
                                                **   walls were searched, the part of it done: the world's reach) */
            HUB->ireq.to = E_PORTAL_SEARCH_REACH (*m);
            finds = things (IO_SEARCH, "");
        }
        if (!finds || k != PO_NOTHING) {
            if (k == PO_SOFT) {                 /* (A blow with a soft thing, the hub's pc_strike's: its name, the
                                                **   thing struck with) */
                hub_mem (KIND_BANK, lb + 300, CATALOGUE[SK_KIND (HUB->pc_strike)].name, sizeof CATALOGUE[0].name);
                lb[300 + sizeof CATALOGUE[0].name] = 0;
                said_s[1] = lb + 300;
            }
            outcome_words (m);               /* (Its words said.txt's: said.c) */
        }
        if (lifting[0] && (k == PO_LIFTED || k == PO_PANE_OUT || k == PO_BARS_OUT)) {
            strcpy (pending, lifting);
        }
        lifting[0] = 0;
        rebar |= k == PO_REBARRED;
        if (finds && (k == PO_FOUND || k == PO_NOTHING || k == PO_FOUND_TRAP)) {
            msg (tt ());                        /* (What its search turned up besides) */
            finds = 0;
        }
        break;
    }
}


/* A thing hidden where the character stands (P_CONCEAL: the referee's test, the things' record), worded by how
** well (its difficulty, in steps); part 10's, the search's (part 11's room, part 2's before it).  No thing said, the
** character itself (HOUSE.md's hiding oneself: the world's I_RISE, POS_HIDE): behind or in a thing said, one within
** reach, walked over to first; in the shadows, a doorway, the world's to say.  And its words as they're told
** (pose_words', E_BEAT WB_POSED's HW_WORDS: w no string, its address past the RAM) */
#pragma code-name (push, "SW18CODE")
#pragma rodata-name (push, "SW18RODATA")
static const char hw_said[] =               /* (By HW_*, one after another: a table's strings would be the main RAM's) */
    "\nYou crouch behind the %s, out of sight.\0" "\nYou get down into the %s, out of sight.\0"
    "\nYou press yourself into the doorway, out of sight.\0" "\nYou crouch among the rubble, out of sight.\0"
    "\nYou keep still in the dark, out of sight.\0" "\nYou come out of hiding.\0"
    "\nThere's no cover here: you're hiding no longer.\0" "\n%s has seen you!\0" "\nYou spot %s, hiding.\0"
    "\n%s leaps out of hiding!\0" "\nYou strike from hiding.\0" "\nYou're hidden already.\0"
    "\nNot with a light on you: it gives you away.\0" "\n%s is watching you: you can't hide from it now.\0"
    "\nThere's nothing here to hide behind, and the light's too good.";
unsigned char __fastcall__ s_hide (const char* w)
{
    unsigned char d, n;
    char t[16], tn[20];
    const char* s;

    if ((unsigned) w <= (unsigned) HD_UP) {     /* (A rope's or a grapple's word; thrown up, at no mark: wrope.c's work, words.c's) */
        n = (unsigned) w;
        w = cmd.obj.word;
        d = strstr (w, "rope") || strstr (w, "grapple") || !*w;
        if (n == (unsigned) HD_ROPE) {
            return d;
        }
        if (!*w || !d || (cmd.dir != GO_UP && cmd.other.x)) {
            return 0;
        }
        cmd.action = VB_TIE_OBJECT;
        cmd.aux = RP_THROW;
        things_work (H_TIE);
        return 1;
    }
    if ((unsigned) w >= HW_WORDS) {
        d = (unsigned) w >> 8 & 0x3F;
        for (s = hw_said, n = d; n; --n) {
            s += strlen (s) + 1;
        }
        hub_mem (0, &n, &HUB->pc_hw, 1);
        if (d <= HW_HEAP) {
            hub_mem (KIND_BANK, t, CATALOGUE[n].name, sizeof CATALOGUE[0].name);
            t[sizeof CATALOGUE[0].name] = 0;
            sayf (s, strstr (t, " of ") ? strstr (t, " of ") + 4 : t);
        } else if (d == HW_FOUND || d == HW_SPOT || d == HW_LEAP || d == HW_SEEN) {
            hub_mem (0, t, HUB->actor[(n - 1) & 7].name, 12);
            t[12] = 0;
            strcpy (tn, t[0] >= 'A' && t[0] <= 'Z' ? "" : d == HW_SPOT ? "a " : "The ");
            strcat (tn, t);
            sayf (s, tn);
        } else {
            sayf (s, "");
        }
        return 0;
    }
    if (!*w || (!cmd.other.word[0] && (strstr (cl, " behind ") || strstr (cl, " in ") || strstr (cl, " under ") ||
        strstr (cl, " among ") || strstr (cl, " beside ") || strstr (cl, " by ")))) {
        if (*w) {
            strcpy (cmd.other.word, w);         /* (Behind the table, in the straw: the parser's the thing, or the other; so
                                                **   kept, walked over to it first) */
            cmd.obj.word[0] = 0;
        }
        w = cmd.other.word;
        if (*w && !strstr ("shadow shadows dark darkness doorway door corner", w) && (HUB->ireq.to = LM_COND, d = things (IO_LOOK | IO_GO, w)) != IR_OK) {
            if (d != IR_GONE) {
                say_no (A_SEE_NO, w);
            }
            return 0;
        }
        intend2 (I_RISE, POS_HIDE | HD_IN, 0);
    } else {
again:  HUB->ireq.to = reach == 4 ? IO_DONE : 0;
        if ((d = things (IO_HIDE | IO_STEPS | IO_GO, w)) == IR_NONE && reach != 4) {
            if ((d = things (IO_GET | IO_STEPS | IO_GO, w)) == IR_OK) {
                *tt () ? msg (lb) : tell (T_TAKEN); /* (One lying about, not carried: taken up first, then hidden, as
                                                **   HOUSE.md's hiding a thing, put down where it stands at its end; out
                                                **   of reach, walked over to first and the hide again: the one path for
                                                **   things, COMMANDS-CRITIQUE.md's §5.2 H) */
                goto again;
            }
            if (d != IR_NONE) {
                if (d != IR_GONE && *tt ()) {
                    msg (lb);                   /* (Not taken: the take's words why; a step of the reach, none) */
                }
                d = IR_NO;
            }
        }
        if (d == IR_OK) {
            if (reach != 4) {
                things_work (H_HIDDEN);         /* (Begun: a minute's work, a timed work, the things' IO_DONE at its
                                                **   end, words.c's) */
                return 0;
            }
            d = HUB->ireq.qty;
            tell (d >= 10 ? T_HID_CUNNINGLY : d >= 4 ? T_HID_WELL : T_HID_POORLY);
            to_hand ();
            if (HUB->ireq.y >= 0xFD) {
                intend2 (I_WAIT, 1, I_WAIT_W2 (0, 0, 1));   /* (The reach's steps to come: the UI waits for them) */
            }
        } else if (d != IR_NO && d != IR_GONE) {    /* (IR_NO: a step of the reach first, part 19's) */
            say_no (A_HAVE_NO, w);
        }
    }
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The hand said in the command's line (cl: "in my left hand", "in my right hand", "in both hands", "in my other hand",
** "in my off hand", "in my main hand"; slice 4.1b's), into shand (items.h's IH_*; IH_OTHER, the other one): a thing
** taken, put, held or worn so made a hold of it, into that hand ("take the torch in my right hand", "put the dagger in
** my other hand"); and the way and the count the parser made of its words no more (dq0c's meaning takes "left" for a
** way, "both" for a count: "attack the rat with the dagger in my left hand" no rat to its left) */
#pragma code-name (push, "SW23CODE")
#pragma rodata-name (push, "SW23RODATA")
void s_hand_said (void)
{
    static const char* const said[6] = { "left hand", "right hand", "both hands", "other hand", "off hand",
        "main hand" };
    const char* p;
    unsigned char k, a = cmd.action, h;

    for (k = 0; k < 6; ++k) {
        for (p = cl; *p && strnicmp (p, said[k], strlen (said[k])); ++p) {
        }
        if (*p) {
            break;
        }
    }
    if (k == 6) {
        return;
    }
    h = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    shand = k == 0 ? IH_LEFT : k == 1 ? IH_RIGHT : k == 2 ? IH_BOTH : k == 3 ? IH_OTHER : k == 4 ? h ^ IH_BOTH : h;
    cmd.dir = 0xFF;
    cmd.how = 0;
    cmd.n = 0;
    if (cmd.prep != PR_ON && (a == VB_PUT_OBJECT || a == VB_RETRIEVE_OBJECT || a == VB_EQUIP_OBJECT ||
        a == VB_HANG_OBJECT || a == VB_HOLD_OBJECT)) {  /* (Not "on my left hand": worn there, worn.c's, 2026-11-10) */
        cmd.action = VB_HOLD_OBJECT;
        cmd.aux = IO_HOLD;
        cmd.prep = 0;
        cmd.other.word[0] = 0;
        cmd.other.sel = 0;
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A blow's thing taken in hand first (resolve.c's hold_w: its target found, the blow to be sent), into the hand said,
** if one was (shand): its words ("You take the bar in both hands."), the referee told what's to hand now (RS_EQUIP: its
** blocks with it from now on), its time; held already, nothing; 0, no hand for it, why not told ("Your hands are full
** ...": not struck with another instead) */
/* A denizen's things as the eyes make them out from here (the world's IO_GEAR: examined, or assessed, cmd's aux),
** after how it seems (deal's): HOUSE.md's three levels, the user's, 2026-10-27 */
#pragma code-name (push, "SW3CODE")             /* (Part 3's since 2026-11-04, overlay 15's room) */
#pragma rodata-name (push, "SW3RODATA")
void s_gear_look (void)
{
    HUB->ireq.to = (cmd.aux ? 2 : 1) | sight_get ()->eye << 4;
    if (things (IO_GEAR, cmd.obj.word) == IR_OK) {
        msg (tt ());
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

unsigned char s_hold_first (void)
{
    unsigned char r, s = HUB->ireq.sel;
    unsigned t;

    HUB->ireq.into[0] = 0;
    HUB->ireq.to = shand;
    if (cmd.other.word[0] && !strcmp (hold_w, cmd.other.word)) {
        HUB->ireq.sel = cmd.other.sel | cmd.other.n << 4;   /* ("With the other sword": the instrument's own selector, not
                                                **   the target's: COMMANDS-CRITIQUE.md's §5.1 disagreement 9) */
    }
    r = things (IO_HOLD | IO_STEPS | IO_GO, hold_w);
    HUB->ireq.sel = s;
    if (HUB->ireq.kind == IO_STEP || r == IR_GONE) {
        return 0;                               /* (A step of the reach first, HOUSE.md's: the blow asked again after it, hold_w
                                                **   still said; 2026-11-04; out of reach, walked over to it first, the
                                                **   one path for things) */
    }
    t = HUB->ireq.x;
    hold_w[0] = 0;
    if (r == IR_OK) {
        msg (tt ());
        to_hand ();
        intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
    } else if (tt (), tt_sa != (SA_HELD | 0x80)) {     /* (Not held already: said.txt's SA.held, its code) */
        msg (lb);
        return 0;
    }
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 13 (slice 4.7's): the missiles (overlay 1's bank, after its own) */
#pragma code-name (push, "SW13CODE")
#pragma rodata-name (push, "SW13RODATA")

/* ---- The missiles (slice 4.7's: DUNGEON.md's fights complete), s_command's: a shot ("shoot the bandit", "fire the
** bow at the rat", "shoot at the rat with my bow"; "shoot", the target), a throw at a denizen ("throw the dagger at
** it": a missile's attack, the world's shot; at anything else its throw as before, s_command's: 0 back), a launcher
** loaded ("nock an arrow", "load the crossbow"; "draw the sword", a thing taken in hand), the missiles gathered
** ("gather the arrows").  A launcher named, or the one carried, taken in hand first if it isn't (hold_first's), sent
** with the shot (I_ATTACK's x[1] SK_OF its kind, b AIM_SHOT, fought on as a blow is: the world's) */
/* A launcher's kind whose name has w (the catalogue's: a sling, the bows, a crossbow), its name in hold_w; 0 none */
static unsigned char __fastcall__ bow_named (const char* w, unsigned char k)
{
    for (; k <= KI_CROSSBOW; ++k) {
        hub_mem (KIND_BANK, hold_w, CATALOGUE[k].name, sizeof CATALOGUE[0].name);
        if (!*w || strstr (hold_w, w)) {
            return k;
        }
    }
    return 0;
}

/* The launcher named (w: a word of its name: its kind; "" any, the world's to find, held or taken up); 0 none (told) */
static unsigned char __fastcall__ launcher (const char* w)
{
    unsigned char k = *w ? bow_named (w, KI_SLING) : KI_SLING;

    if (!k) {
        msg ("You can't shoot with that.");
    }
    return k;
}

/* The denizen thing t names that the character sees, near or far: its entry plus 1; 0 none (why in lb) */
static unsigned char __fastcall__ mark (const struct cthing* t)
{
    static struct cmd c;
    unsigned char d;

    memcpy (&c, &cmd, sizeof c);
    memcpy (&c.obj, t, sizeof c.obj);
    c.dir = 0xFF;                               /* (Any way: its aux none, a shot's, a throw's) */
    d = fight_target (&c);
    return d == FT_THING ? 0 : d & 0x0F;
}

static void __fastcall__ shot (unsigned char d, unsigned char s)
{
    static struct msg m;

    m.kind = I_ATTACK;
    m.who = 0;
    I_ATTACK_FOE (m) = d;
    I_ATTACK_AIM (m) = AIM_SHOT | Z_MID | (cmd.action == VB_THROW_OBJECT && (cmd.manner & MN_HARD) ? AIM_HARD : 0);
    m.w1 = m.w2 = 0;
    I_ATTACK_STRIKE (m) = s;
    I_ATTACK_HAND (m) = 0;                      /* (Its hand the world's to find) */
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
}

void go_thing (void);                           /* (way.c's, this part's) */
unsigned char s_missiles (void)
{
    const struct cthing* t = &cmd.obj;
    const char* w = cmd.other.word;
    unsigned char d, k;

    switch (cmd.action) {
    case VB_MOVE_DIRECTION:                     /* (Gone to a thing: command's move, no stub of its own) */
    case VB_MOVE_TOWARD:
    case VB_ENTER:
    case VB_APPROACH_OBJECT:
        go_thing ();
        return 1;
    case VB_THROW_OBJECT:
        if (hide (HD_UP)) {
            return 1;                           /* (A grapple thrown up, its rope on it: wrope.c's) */
        }
        if (cmd.other.x && cmd.obj.word[0] && (d = mark (&cmd.other)) != 0) {
            if (things (IO_FIND, cmd.obj.word) != IR_OK) {
                msg (tt ());
                return 1;
            }
            k = HUB->ireq.x;
            strcpy (hold_w, cmd.obj.word);
            shand = 0;
            if (hold_first ()) {
                shot (d, SK_OF (k));
            }
            return 1;
        }
        if (cmd.other.x) {                      /* (Not at a denizen: the world's throw, at a thing) */
            HUB->ireq.x = cmd.other.x;
            HUB->ireq.y = cmd.other.y;
            HUB->ireq.to = cmd.manner;
            d = things (IO_THROW | IO_STEPS, cmd.obj.word);
        } else {
            d = cmd.other.word[0] ? IR_NOT_HERE : IR_FULL;   /* (No mark it sees; none said) */
        }
        throw_said (d | HUB->ireq.qty << 8);
        return d == IR_OK ? 2 : 1;
    case VB_SHOOT_OBJECT:
        if (cmd.prep == PR_AT) {                /* ("fire the bow at the rat") */
            t = &cmd.other;
            w = cmd.obj.word;
        }
        if (!(d = mark (t))) {                  /* (None said, none the target: one beside it, else why not) */
            msg (lb);
            return 1;
        }
        if ((k = launcher (w)) != 0) {
            shot (d, SK_OF (k));
        }
        return 1;
    case VB_LOAD_OBJECT:                        /* ("nock an arrow", "load the crossbow"; "draw the sword": in hand) */
        w = cmd.obj.word;
        k = *w ? bow_named (w, KI_SLING) : 0;
        if (!k && *w && !strstr (w, "arrow") && !strstr (w, "bolt") && !strstr (w, "stone")) {
            strcpy (hold_w, w);
            shand = 0;
            hold_first ();
            return 1;
        }
        if (k) {
            shand = k != KI_SLING ? IH_BOTH : 0;   /* (The launcher named: in hand first) */
            if (!hold_first ()) {
                return 1;
            }
        }
        intend2 (I_LOAD, 0, 0);
        return 1;
    case VB_GATHER_OBJECT:                      /* ("gather the arrows": the world's, each walked over to) */
        w = cmd.obj.word;
        intend_a = strstr (w, "arrow") ? FM_ARROW : strstr (w, "bolt") ? FM_BOLT : strstr (w, "stone") ? FM_SHOT : 0;
        reach = 6;                              /* (Walked to, each: its arrivals' words short) */
        msg ("You go over to them.");
        placing = 1;
        intend2 (I_GATHER, 0, 0);
        return 1;
    }
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW11CODE")
#pragma rodata-name (push, "SW11RODATA")

/* The hands' and limbs' commands (s_command's: slice 4.1b's): what's in its hands changed over ("swap hands", "switch
** hands": the things' IO_SWAP, its words, its time); getting up ("get up", "stand", "rise": the world's I_RISE, the
** book's Rise); the body's position ("crouch", "squat", "kneel", "sit", "lie down": I_RISE's position, slice 4.6's) */
void s_limbs_cmd (void)
{
    unsigned t;

    if (cmd.action != VB_SWAP_HANDS) {
        intend2 (I_RISE, cmd.action == VB_SIT ? POS_SITTING : cmd.action == VB_LIE_DOWN ? POS_PRONE :
            cmd.action == VB_STAND ? POS_STANDING : strstr (cl, "kneel") ? POS_KNEELING : POS_CROUCHED, 0);
        return;
    }
    if (cmd.action == VB_SWAP_HANDS) {
        t = things (IO_SWAP, "") == IR_OK ? HUB->ireq.x : 0;
        msg (tt ());
        if (t) {
            to_hand ();                         /* (The referee told its hands anew) */
            intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The commands (the parser comes later: a word, and the rest) */

unsigned char intend_a;

void __fastcall__ intend2 (unsigned char kind, unsigned w1, unsigned w2)
{
    static struct msg m;
    unsigned b = hub_here ();                   /* (An overlay's too: the hub's header put in the window for it, then
                                                **   the overlay back; slice 4.1's, words.c's more) */

    m.kind = kind;
    m.who = 0;
    m.a = intend_a;                             /* (Its a, if it's one that has it: a wait's unit, task.c's) */
    intend_a = 0;
    m.w1 = w1;
    m.w2 = w2;
    hub_map (0);
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
    hub_back (b);
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* An intent sent: the character's busy with it */
static void __fastcall__ sent (struct msg* m)
{
    if (hub_send (m)) {
        busy = 1;
        t_prompt (0);
    }
}

/* A portal (or a trap) worked: what's done (PA_*), to what (the parser's thing: a door, a window, or either) */
void __fastcall__ s_work (unsigned char what, unsigned char obj)
{
    static struct msg m;

    I_PORTAL_SET (m, 0, what, obj == OB_DOOR || obj == OB_PORTC ? PW_DOOR : obj == OB_WINDOW ? PW_WINDOW : PW_ANY,
        tx, ty, barw, strike, shand);           /* (The one meant, if it was found: else 0, the one it faces) */
    sent (&m);
}

void __fastcall__ s_intend (unsigned char kind, unsigned char a)
{
    static struct msg m;

    m.kind = kind;
    m.who = 0;
    m.a = a;
    m.b = intend_a;                             /* (I_ATTACK's aim, a grapple's: blows.c's, 2026-11-09) */
    intend_a = 0;
    m.w1 = m.w2 = 0;
    I_ATTACK_STRIKE (m) = strike;              /* (I_ATTACK's, I_CLOSE's: what it strikes with, the hand it's in) */
    I_ATTACK_HAND (m) = shand;
    if (kind == I_GO) {                         /* (Through the one meant, if it was found) */
        I_GO_X (m) = tx;
        I_GO_Y (m) = ty;
    }
    sent (&m);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* (Part 2's own calls of them straight; part 24's through their stubs; the handlers' files', cmds.h's) */
void __fastcall__ lift (const char* name);      /* (Part 24's: the commands' rarer, part 2's till 2026-11-04, the swap study's) */
#define work    s_work
#define intend  s_intend

/* The character's state, or the game's, set: the world's (its request, WQ_SET, waited for: the window put back) */
void __fastcall__ wset (unsigned char what, unsigned char x, unsigned char y)
{
    unsigned b = hub_here ();

    hub_map (0);
    HUB->wreq.kind = WQ_SET;
    HUB->wreq.a = what;
    HUB->wreq.x = x;
    HUB->wreq.y = y;
    hub_call (&HUB->wreq, T_WORLD);
    HUB->wreq.state = REQ_FREE;
    hub_back (b);
}

/* ---- The things (items.h): a request to the world's item store, waited for: its result.  With IO_GO, every verb's
** one path for a thing (COMMANDS-CRITIQUE.md's §5.2 H): out of reach, walked over to first and the command again
** (reach_go: walk_a's walk); several that differ, which asked (caps.c's which_ask); either, IR_GONE.  The words and
** the time the verb's own.  (The request by its fixed address, its flag a static: the main RAM's room, 2026-11-07) */
#define q   (&HUB->ireq)
unsigned char __fastcall__ things (unsigned char op, const char* name)
{
    static unsigned char go;                    /* (Nothing it calls asks the things again before it's read) */

    go = op & IO_GO;
    op ^= go;
    q->kind = op;
    strncpy (q->name, name, sizeof q->name - 1);
    q->name[sizeof q->name - 1] = 0;
    said_ahead = 0;
    if (op == (IO_GET | IO_STEPS)) {
        HUB->pc_lump = (HUB->pc_lump & ~(PL_MORE | PL_ALL)) | (take_more ? PL_MORE : 0) | (take_all ? PL_ALL : 0);   /* ("Take
                                                **   all": its first and last too, the pack first) */
    }
    if (op & IO_STEPS) {
        walk_op = op;
        walk_ahead (name);                      /* (Its first words told now, the world's answer some 700,000 cycles off
                                                **   and more, its parts' copies and its chooser: the user's D17,
                                                **   2026-11-03; the user's, 2026-11-04: a word as it begins, sway.c's) */
    }
    if (ndes) {
        des_req (name);                         /* (A thing named by its letter, or chosen by the first words: that one,
                                                **   "#" and its id, letters.c's; after them, sway.c's bind) */
    }
    hub_call (q, T_WORLD);                      /* (Asleep till the world rings with the reply) */
    q->state = REQ_FREE;
    if (q->kind == IO_STEP) {
        step_go ();                             /* (A step of the reach begun, HOUSE.md's: part 19's) */
    }
    op = q->result;
    if (go) {
        if (op == IR_WHICH) {
            which_ask ();
            return IR_GONE;
        }
        if (op == IR_NOT_HERE && reach_go ()) {
            return IR_GONE;
        }
    }
    walk_a = 1;                                 /* (Not walked: the next walk's the command kept's again) */
    return op;
}
#undef q

/* The things asked from an overlay (theme.c's): the hub's header put in the window for it, then the overlay back */
unsigned othings_x;
unsigned char __fastcall__ othings (unsigned char op, const char* name, const char* into, unsigned char to)
{
    unsigned b = hub_here ();
    unsigned char r;

    hub_map (0);
    strcpy (HUB->ireq.into, into);
    HUB->ireq.to = to;
    r = things (op, name);
    othings_x = HUB->ireq.x;
    tt ();
    hub_back (b);
    return r;
}

/* What's to hand, told the referee (the keys of the tools it carries: a skill used takes the tool that suits), and
** the load it carries (the things' weight, all told: the referee's encumbrance, in its summary) */
void to_hand (void)
{
    things (IO_TOOLS, "");
    strcpy (HUB->slot[SLOT_UI].text, tt ());
    RS_EQUIP_LOAD (HUB->slot[SLOT_UI]) = HUB->ireq.x;
    RS_EQUIP_HELD (HUB->slot[SLOT_UI]) = HUB->ireq.y;        /* (The hands that hold something: slice 4.1b's) */
    referee (RS_EQUIP, 2)->state = REQ_FREE;
}

/* The tool a dig's spell took (the referee's choice): worn by it, and named as it's first taken up (part 14's, s_news's
** alone, through its stub: the main RAM's room); bk an E_DIG's b, its how (D_*) times 256.  And the rock a spell's
** into (rock.h's RK_*: the b's high four bits, slice 4.10's), told as it's come to (its first spell, or the b's 8: a
** new rock): how it looks and how it goes, its name too after a spell that went well (Mining's eye: D_GOOD), and at
** the first spell that goes well after any that didn't (the user's, 2026-11-03: dug_rock, the rock last named).  A
** better tool told of (D_BETTER), or a dig that won't begin (D_NO_TOOL: bk's low byte what it's into, MAT_* + 1): its
** words' (said.txt's DIG.better, DIG.no_tool) worked out, the tools named (said_s) and why (said_n), by what it
** carries (HOUSE.md's, 2026-11-02; the user's, 2026-11-03: the thing named, no use on it, the one that would be) */
char dig_with[16];
char dig_word[16];                              /* (The thing named to dig with, as said: resolve.c's; the world's dig_kind) */
static unsigned char dug_rock;
#pragma code-name (push, "SW14CODE")
#pragma rodata-name (push, "SW14RODATA")
void __fastcall__ s_dig_used (unsigned bk)
{
    static const char rocks[] = "shale\0flaky\0sandstone\0buff\0limestone\0pale grey\0slate\0"
        "blue-grey\0marble\0white\0granite\0speckled grey\0basalt\0black\0quartz\0white-veined\0ironstone\0"
        "rusty red\0copper ore\0green-stained";
    const char* r;
    unsigned char n = bk & 3, i = (unsigned char) bk >> 4, g = bk >> 8 == D_GOOD;

    if (bk >> 8 == D_NO_TOOL || bk >> 8 == D_BETTER) {
        if (bk >> 8 == D_NO_TOOL) {
            to_hand ();                         /* (What it carries, fresh, the tools' list: its words' why) */
        }
        said_s[1] = dig_word;                   /* (The thing named; the better one to hand) */
        said_s[2] = lb + 160;
        memcpy (lb + 160, HUB->dig_better, 8);
        lb[168] = 0;
        memcpy (lb + 170, HUB->dig_name, 8);
        lb[178] = 0;
        said_n = 0;
        if (bk >> 8 == D_BETTER || (lb[170] && strcmp (lb + 170, "chisel"))) {
            return;                             /* (The thing named, no use on it: the user's, 2026-11-03) */
        }
        hub_mem (UI_BANK, lb + 200, UIREC->things_text, 180);   /* (What it carries) */
        lb[380] = 0;
        n = (unsigned char) bk - 1;             /* (What it's into, MAT_*) */
        n = n != MAT_EARTH && n != MAT_SOIL;    /* (Hard) */
        r = strstr (lb + 200, "shovel") ? "shovel" : strstr (lb + 200, "spade") ? "spade" : strstr (lb + 200, "mattock") ?
            "mattock" : strstr (lb + 200, "blade") ? "blade" : strstr (lb + 200, "dagger") ?
            "dagger" : strstr (lb + 200, "knife") ? "knife" : "";
        if (n && !strstr (lb + 200, "chisel") && strstr (lb + 200, "woodchi")) {
            said_n = 1;                         /* (A wood chisel) */
        } else if (n && strstr (lb + 200, "chisel")) {
            said_n = strstr (lb + 200, "hammer") ? 2 : 3;   /* (A chisel: a hammer, both hands; none) */
        } else if (*r) {
            strcpy (lb + 200, r);               /* (A tool that only scratches it) */
            said_s[2] = lb + 200;
            said_n = 4;
        } else {
            said_n = 5;                         /* (Its hands) */
        }
        return;
    }
    HUB->ireq.to = HUB->dig_wear[n];
    things (IO_WEAR, HUB->dig_tool[n]);
    if (strcmp (dug_with, HUB->ireq.name)) {
        strcpy (dug_with, HUB->ireq.name);
        strcpy (dig_with, strcmp (dug_with, "hands") ? dug_with : "bare hands");   /* (Named once: "your bare hands") */
    }
    if (i && bk >> 8 != D_THROUGH && (dig_with[0] || bk & 8 || (g && dug_rock != i))) {
        if (g) {
            dug_rock = i;
        }
        for (r = rocks; --i; r += strlen (r) + 1) {    /* (Its name, then its looks) */
            r += strlen (r) + 1;
        }
        sayf ("\nYou're into %s rock%s%s.", r + strlen (r) + 1, g ? ": " : "", g ? r : "");
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* A portal's part, fixed in it (a door's bar, a window's pane or its bars: IR_FIXED, its square in the request's x,
** y): the portal at it or beside it faced, and the part lifted out (the maker's: a bar only from its side), then taken
** up (E_PORTAL's) */
#undef work
#undef intend
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")
void __fastcall__ s_lift (const char* name)
{
    struct sight* s = sight_get ();
    unsigned char i;

    for (i = 0; i < s->ndoors; ++i) {
        if (!(s->door[i].rel & MOUTH) && s->door[i].x - HUB->ireq.x + 1 <= 2 && s->door[i].y - HUB->ireq.y + 1 <= 2) {
            strcpy (lifting, name);
            tx = s->door[i].x;                  /* (That one: found by its part's square, faced, the feature path's) */
            ty = s->door[i].y;
            i = s->door[i].kind == PK_WINDOW ? OB_WINDOW : OB_DOOR;
            cmd.obj.lies = bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y);
            feature (0, FW_NONE);
            work (PA_LIFT, i);
            return;
        }
    }
    say_no (A_SEE_NO, name);
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#define work    s_work
#define intend  s_intend

/* A thing taken up, or not (r: IR_*, and the light carried before times 256): told, and its time (the world's
** E_TAKEN's too, a thing it walked over to and took up: a portal's part, or one fixed where it stands, seen to as the
** character's free) */
void __fastcall__ s_took_up (unsigned r)
{
    if ((unsigned char) r == IR_FIXED) {
        strcpy (pending, taking);
        return;
    }
    if ((unsigned char) r == IR_FUMBLED) {      /* (Engaged, a grope failed: the things' words, the round wasted) */
        msg (tt ());
        intend2 (I_WAIT, HUB->round, I_WAIT_W2 (0, 0, 1));
        return;
    }
    if ((unsigned char) r == IR_OK && *tt ()) {
        msg (lb);                               /* (By the count: "You take 4 rations.", the world's) */
    } else if ((unsigned char) r != IR_NOT_HERE) {  /* (Out of reach still: the walk's words told why) */
        if ((unsigned char) r == IR_OK || (unsigned char) r == IR_FULL) {
            tell ((unsigned char) r == IR_OK ? T_TAKEN : T_YOUR_HANDS_ARE);
        } else {
            say_no (A_SEE_NO, taking);
        }
    }
    if ((unsigned char) r == IR_OK) {
        after (r >> 8, 20, IO_GET);
        taken_next (pending);                   /* ("Take all": the next, as it's free) */
    }
}

/* A thing taken up, by its name: the one path for things (things' IO_GO: out of reach, the world walks over to it
** first and takes it up as it gets there, reach_go's walk_a 0, E_TAKEN's; several that differ, which asked); a
** portal's part lifted out first; one fixed where it stands, the things' words */
void __fastcall__ s_take (const char* name)
{
    unsigned char was = HUB->pc_light, r;

    strcpy (taking, name);                      /* (name may be pending, cleared) */
    pending[0] = 0;
    walk_a = 0;
    r = things (IO_GET | IO_STEPS | IO_GO, taking);
    if (r == IR_FIXED) {
        lift (taking);
    } else if (r == IR_NO) {
        msg (tt ());                            /* (A theme's altar, its throne: fixed where it stands) */
    } else if (r != IR_GONE) {
        s_took_up (r | was << 8);
    }
}


/* The map, and beside it the referee's short lines (the attributes as they are now) */
/* A thing it sees, the parser's (its kind, | PICK_TELL to say so if there's none; and the heading it lies, if one
** was said): 1, its square in tx, ty; 0, none; 2, several and none ahead, which asked (overlay 7's pick) */
unsigned tx, ty;
#define find(obj, orel) pick ((obj), (orel))
unsigned char news_due;                         /* (The body's news held as it sleeps: part 12's) */

/* The character's way with a denizen (how: hub.h's SW_*, the command's verb's: commands.verbs' aux; its things, the
** one meant and a name or a thing, and its request): asked of the denizens' task (the hub's sway: it lends the UI's
** slot to the referee meanwhile), waited for, worded (overlay 6: but none found, if SW_QUIET), and the time it took
** the character's.  Its result */
#define SW_QUIET    0x80
#define dealt       (*(struct sway*) (lb + sizeof lb - sizeof (struct sway)))  /* (Its copy at lb's end: the
                                                **   words, a line or two, go at its start) */
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")
unsigned char __fastcall__ s_deal (unsigned char how)
{
    struct sway* s = &HUB->sway;

    if ((s->kind = how & ~SW_QUIET) <= SW_GIVE) {
        s->actor = den_meant;                   /* (The one a capital named: that one, letters.c's) */
    }
    strcpy (s->word, cmd.obj.word);
    s->prep = cmd.prep;
    strcpy (s->word2, cmd.other.word);
    s->order = cmd.order;
    hub_call (s, T_DENIZENS);
    memcpy (&dealt, s, sizeof dealt);
    s->state = REQ_FREE;
    if (!(how & SW_QUIET) || dealt.result != SR_NONE) {
        sway_said (&dealt);
    }
    if (dealt.t) {
        intend2 (I_WAIT, dealt.t, 0);
    }
    return dealt.result;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)

/* A command that takes no time and wants nothing of the character (what it carries, its sheet, the map, the help):
** carried out at once, the character busy or not (on its way somewhere, looking about: the user, "the UI is slow to
** respond"), if it's the next one typed */
static unsigned char __fastcall__ at_once (const char* l)
{
    cq_cmd (l);                                 /* (Its command: the command parser's, the line's) */
    return free_cmd (l);                        /* (1 at once; 0 held; 2 held, its fight stopped for it: task.c's) */
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* A line typed as the character lies senseless, or waiting as it was knocked out (main's, out_told: told so): not
** taken, nor any behind it, its refusal said once (the user's, 2026-11-13: no commands queued through a black-out, to
** be carried out as it comes round; "ignore input or not allow it at all when knocked out").  But the quit (held till
** it comes round, as any line typed as it's busy; the game's end if it dies) and the help (at once).  Come round (the
** world's pc_out), the lines as ever.  Part 2's: the parser's own */
void s_out_line (void)
{
    hub_map (0);
    if (!HUB->pc_out) {
        out_told = 0;
        return;
    }
    if (cq_cmd (t_next ()) > 0 && (cmd.action == VB_META_QUIT || cmd.action == VB_META_HELP)) {
        return;
    }
    while (t_line ()) {
    }
    cq_drop ();
    held = took = 0;
    HUB->ui_cmd = 0;                            /* (No command in hand: the world's jobs not held for it) */
    say_no (A_OUT, 0);                          /* ("You lie senseless.") */
}

/* Nothing: part 2 copied in (main's, while no line's in hand: the next command's part there for it, its copy's 50,000
** cycles not between its Enter and its answer) */
void s_ready (void)
{
}

/* The command parser (dq0c, the hub's cq): each line posted to it as it's taken (cq_post), its work done while the
** UI sees to the line (the names', the pace's), and what it makes of it taken where that's wanted (cq_cmd: waited
** for only if it isn't there yet; the line posted then, if it wasn't), till the line's seen to (cq_drop) */
void __fastcall__ s_cq_post (const char* l)
{
    static unsigned n;

    if (cq_st) {
        return;                                 /* (A held line's: posted as it was typed) */
    }
    n = strlen (l);
    hub_map (CMD_BANK);
    memcpy (HUB_WINDOW, l, n + 1);
    hub_map (0);
    HUB->cq.kind = CQ_MATCH;
    HUB->cq.len = n;
    HUB->cq.face = believed (HUB->pc_face);     /* (Its compass the north it believes in: slice 4.2's) */
    hub_post (&HUB->cq, T_COMMANDS);
    cq_st = CQ_OUT;
}

/* Its reply, waited for if it's out: its verdict (cq_res: 1 the line parses, 0 it doesn't, -1 too long; -2 none, the
** game ending before it came) */
static void cq_wait (void)
{
    if (cq_st == CQ_OUT) {
        hub_wait (&HUB->cq);
        hub_map (0);
        cq_res = HUB->cq.state == REQ_DONE ? HUB->cq.res : -2;
        HUB->cq.state = REQ_FREE;
        cq_st = CQ_IN;
    }
}

/* The command line l makes (the command parser's: the line posted, if it wasn't), into cmd: its ways by the compass
** the north the character believes in (slice 4.2's bearings: the parser's made as it believes it faces), made the
** true ones; its ways as it faces made the compass's as it faces now (dq0c's CQ_REL | n), a half turn an eighth
** ("turn half left").  Its verdict */
signed char __fastcall__ s_cq_cmd (const char* l)
{
    static unsigned char f, d;

    s_cq_post (l);
    cq_wait ();
    if (cq_res == -2) {
        memset (&cmd, 0, sizeof cmd);           /* (VB_NONE: nothing) */
        return -2;
    }
    hub_map (CMD_BANK);
    memcpy (&cmd, HUB_WINDOW + CQ_CMD, sizeof cmd);
    hub_map (0);
    f = HUB->pc_face;
    d = HUB->pc_skew;
    if (cmd.dir <= H_NW) {
        cmd.dir = (cmd.dir - d) & 7;
    } else if (cmd.dir != 0xFF && (cmd.dir & CQ_REL)) {
        cmd.dir = (f + cmd.dir) & 7;
    }
    if (cmd.obj.lies <= H_NW) {
        cmd.obj.lies = (cmd.obj.lies - d) & 7;
    } else if (cmd.obj.lies != 0xFF && (cmd.obj.lies & CQ_REL)) {
        cmd.obj.lies = (f + cmd.obj.lies) & 7;
    }
    if (cmd.action == VB_FACE_DIRECTION && (cmd.flags & CF_HALF) && cmd.dir < GO_UP) {
        d = (cmd.dir - f) & 7;
        cmd.dir = d == 6 ? (f + 7) & 7 : d == 2 ? (f + 1) & 7 : cmd.dir;
    }
    return cq_res;
}

/* The line seen to: its reply waited for, if it's out (the next line posted then) */
void s_cq_drop (void)
{
    cq_wait ();
    cq_st = 0;
}

/* A command, the parser's, carried out: by its action (actions.h's VB_*), the verb's own value its aux (the things'
** request, a door's work, the denizens' sway: commands.verbs'): its handler the dispatch table's (actions.c's verb_row) */
void __fastcall__ s_command (char* l)
{
    unsigned h;
    unsigned char d;
    signed char ok;

    cl = l;
    if (!l) {                                   /* (A command kept, its thing walked over to first: reach_go's; or
                                                **   its timed work done, words.c's) */
        hub_mem (UI_BANK, &cmd, &UIREC->kept, sizeof cmd);
        pc_face = HUB->pc_face;
        tx = 0;
        cl = "";
        goto again;
    }
    if (!busy) {
        reach = 0;                              /* (A line's own, the character free: none kept; one carried out at
                                                **   once as it's busy, the kept left be) */
    }
    worded = 0;
    ok = s_cq_cmd (l);                          /* (The command the parser made: dq0c's, the grammar's verdict with
                                                **   it) */
    pc_face = HUB->pc_face;
    d = choose (&cmd);                          /* (A fixture's choice asked for: this line's, the command again; a
                                                **   thing's, a line made again by letters.c's which_in) */
    tx = 0;
    if (!d && parts_in (PO_AGAIN)) {
        return;                                 /* (A list's later part, a thing alone: "take rope and torch") */
    }
    if (!d && (unsigned char) (cmd.action - VB_MOVE_DIRECTION) < 2 && cmd.how && cmd.how != HOW_ACROSS &&
        (!cmd.obj.kind || cmd.obj.kind == OB_ROOM || cmd.obj.kind == OB_WAY_OUT) && travel_go (l)) {
        return;                                 /* (A place it knows gone to, "go back to the Mausoleum": part 4's) */
    }
    if (!d && (cmd.flags & CF_STRANGE) && *l != '#') {   /* (A verb the game doesn't know: the grammar's, not done
                                                **   yet ("grapple"); or none of its ("bribe") */
        unknown (ok);
        return;
    }
    if ((cmd.obj.word[0] || cmd.n) && (cmd.action == VB_MOVE_TOWARD || cmd.action == VB_MOVE_DIRECTION) &&
        cmd.dir == 0xFF && !cmd.obj.kind && !cmd.how && (cmd.flags & CF_NOVERB)) {   /* (A word alone, or a number, that's no
                                                **   way: "foo", "1") */
        unknown (0);
        return;
    }
    if (!d && !ok) {                            /* (Words the game knows, but not a command the grammar has) */
        sayf ("\nI don't understand that.");
        return;
    }
    if (strstr (l, "hand")) {
        hand_said ();                           /* (A hand said: part 11's, slice 4.1b's) */
    }
    place_said ();                              /* ("From the floor", "on the ground": sway.c's, overlay 14) */
    if (((cmd.flags & CF_UNSAID) && what_ask ()) || resolve () != RV_ON) {
        return;                                 /* (Nothing named, of a thing carried: which, asked by their letters,
                                                **   letters.c's; its things found, its unsaid filled; or a question
                                                **   asked) */
    }
    if (refused ()) {
        return;                                 /* (A thing named its verb would pass over: refused, said back: refuse.c's) */
    }
again:
    HUB->ireq.sel = cmd.obj.sel | cmd.obj.n << 4;  /* (Which of several, how many: the things' requests', the */
    HUB->ireq.osel = cmd.other.sel | cmd.other.n << 4;  /*   world's to choose, slice 4.1's: "the second sack") */
    HUB->ireq.count = cmd.flags & CF_HALF ? CT_HALF : cmd.obj.sel == SL_ALL ? CT_ALL : cmd.n;
    if ((d = puzzle (&cmd)) != Q_NONE) {        /* (The theme's puzzle's: theme.c) */
        if (d >= Q_DONE) {
            intend2 (I_WAIT, 50, I_WAIT_W2 (0, 0, 1));   /* (Its time: the way out opened the world's already) */
        }
        return;
    }
    h = verb_row (cmd.action);                  /* (Its row of the dispatch table: its handler, the kinds it takes, times
                                                **   256; actions.c's, overlay 11, §5.2's step L) */
    d = h;                                      /* (Its group's entry, by the number: cmds.h's, each where its pragma puts
                                                **   it, part 2 all so far) */
    if (d >= H_SELF) {
        self_do (h);                            /* (The body's, the meta commands: hself.c) */
    } else if (d >= H_DENS) {
        dens_do (h);                            /* (The denizens': hdens.c) */
    } else if (d >= H_MOVING) {
        moving_do (h);                          /* (Moving: hmove.c) */
    } else if (d >= H_PLACES) {
        places_do (h);                          /* (The features' and the doors': hplaces.c) */
    } else if (d >= H_WORK) {
        things_work (d);                        /* (The things' timed works: hthings.c, overlay 1) */
    } else if (d) {
        things_do (h);                          /* (The things' verbs: hthings.c) */
    } else {
        unknown (1);                            /* (Those the game doesn't do yet: its verb, as typed) */
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The game's arguments (s_begin's: part 7's, part 1 full; the defaults' words in the main RAM) */
static unsigned long seed;                      /* (Its seed: the clock's, as the task starts, but for -s's) */
static const char* file = "";                   /* (-c's: the character's file) */
static const char* rules = "harn";              /* (The ruleset, if none's given: HarnMaster's) */

#pragma code-name (push, "SW13CODE")           /* (Part 13's since 2026-11-02: part 7's room) */
#pragma rodata-name (push, "SW13RODATA")

#pragma code-name (push, "SW19CODE")
#pragma rodata-name (push, "SW19RODATA")
#pragma local-strings (push, on)

#pragma code-name (push, "SW22CODE")            /* (Part 22's: seldom, overlay 15 full) */
#pragma rodata-name (push, "SW22RODATA")
#pragma static-locals (push, on)
#pragma bss-name (push, "SW22RODATA")           /* (Wanted only while it runs: not the main RAM's) */

/* The save's folder's dungeon, by the record of the life (life.h's: its head left in lb): its folder in spath (sdl), or d1 */
void s_life_dir (void)
{
    static int fd;
    static unsigned n;

    n = 1;
    strcpy (spath, savedir);
    sdl = strlen (spath);
    strcpy (spath + sdl, "/" LIFE_FILE);
    if ((fd = open (spath, O_RDONLY)) >= 0) {
        if (read (fd, lb, sizeof (struct life_head)) == sizeof (struct life_head)) {     /* (Not one of its: dq0life's to find) */
            n = ((struct life_head*) lb)->dungeon;
        }
        close (fd);
    }
    strcpy (spath + sdl, "/d");
    ultoa (n, spath + sdl + 2, 10);
    sdl += strlen (spath + sdl);
    spath[sdl] = '/';
}

#pragma bss-name (pop)
#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW21CODE")            /* (Part 21's: seldom, overlay 15 full) */
#pragma rodata-name (push, "SW21RODATA")
#pragma static-locals (push, on)
#pragma bss-name (push, "SW21RODATA")           /* (Wanted only while it runs: not the main RAM's) */

static char* ap;
static void __fastcall__ arg (const char* s)
{
    strcpy (ap, s);
    ap += strlen (ap) + 1;
}

/* dq0life's path (lb + 300) and arguments (lb), life's, part 1's: its folder, the game's lib folder; given, the seed said (dq0 -s), else the
** clock's its own.  Given 2, as it ends in a death (s_finish's): dq0end's, hard mode's -h, its name and occupation (dead with nothing saved, there's no
** sheet in the folder to say): the life's account */
void __fastcall__ s_between (unsigned char given)
{
    ap = lb;
    arg ("-d");
    arg (savedir);
    if (given == 2) {
        if (hard) {
            arg ("-h");
        }
        arg ("-n");
        arg (P.name);
        arg ("-o");
        arg (P.occupation);
    } else {
        arg ("-l");
        arg (libdir);
    }
    if (given == 1) {
        arg ("-s");
        ultoa (seed, ap, 10);
        arg (ap);
    }
    *ap = 0;
    strcpy (lb + 300, libdir);
    strcat (lb + 300, given == 2 ? "/../acct/dq0end" : "/dq0life");
}

#pragma bss-name (pop)
#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A step of the reach begun by the world (the reply's IO_STEP, HOUSE.md's reach, the user's, 2026-11-02: each its words, its
** time, its grope): its first words, its long task (I_WAIT's kind, its bar, its stop), the command kept and asked
** again as it's up (reach 5: the world's done the step's change at its end, lwork's); qty 0, a grope failed,
** the things' words the caller's, the round wasted */
void s_step_go (void)
{
    unsigned char k, n;
    unsigned t;

    hub_map (UI_BANK);
    memcpy (&UIREC->kept, &cmd, sizeof cmd);
    hub_map (0);
    k = HUB->ireq.qty;
    n = HUB->ireq.item;
    t = HUB->ireq.x;
    what_hi = n & 0x80;
    step_aw = HUB->ireq.plural <= AW_EMPTY ? HUB->ireq.plural : 0;                 /* (Its thing told apart, AW_*: s_reach_said's) */
    took = 0;                                   /* (Its line under way: the next line held may stop a fight in its turn) */
    HUB->pc_lump = (HUB->pc_lump & ~PL_MORE) | (take_more ? PL_MORE : 0);
    if (k) {
        reach = 5;
        if (HUB->ireq.result == IR_NOT_HERE) {  /* ("Take all"'s next is a walk off: the pack on first, the command again
                                                **   after it, told no more on this line: the lump's) */
            HUB->ireq.result = IR_NO;
            HUB->pc_lump |= PL_TOLD;
            hub_map (UI_BANK);
            UIREC->things_text[0] = 0;
            hub_map (0);
        } else if (k == SV_ON || k == SV_PUTAWAY) {    /* (Refused with the pack off: its words first, nothing asked again) */
            reach = 0;
            msg (tt ());
            hub_map (UI_BANK);
            UIREC->things_text[0] = 0;
            hub_map (0);
        }
        task_words (TE_BEGUN | (unsigned) k << 8, n);
    }
    if (HUB->ireq.y & 2) {
        intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));   /* (Engaged: the round's action, a blow no stop of it) */
        return;
    }
    intend2 (I_WAIT, t, I_WAIT_W2 (k & ~SM_ALL, n, 1));   /* (Its long task the verb: SM_* its words' alone) */
    if (k == SV_BLOWDARK) {
        intend2 (I_ASK, TE_ASK_HEARD, 0);       /* (Asked: "no" stops it, the torch still lit) */
    }
}

/* A thing got out for its use (a bandage, first aid's, tend.c's; a meal, a drink, feed.c's: a tool's steps, the things'):
** 1 if a step's begun first, the command asked again as it's up */
unsigned char __fastcall__ s_tool_out (unsigned char which)
{
    static const char* const keys[3] = { "bandage", "food", "water" };   /* (Here: a caller's part isn't in, this one copied over it) */

    return things (IO_USE | IO_STEPS, keys[which]) != IR_OK;
}

/* The reach's words (task.c's: w an E_TASK's, TE_BEGUN with a step's kind; TE_PACKED, TE_FUMBLE, TE_KINDLED; named its
** thing's kind): the step's first words; the pack left off, stopped part way; the world's own step's grope failed; a
** fire-maker's try.  Their words said.txt's TASK rows (TASK.step by its verb, its text the thing with its word: a word
** of SM_*'s or AW_*'s; 2026-11-11, its 25 sentences here before) */
void __fastcall__ s_reach_said (unsigned w, unsigned char named)
{
    static const char* const sm[4] = { "", "new ", "old ", "other " };   /* (By SM_*: "the old torch") */
    static const char* const aw[AW_EMPTY + 1] = { "", "lit ", "unlit ", "spent ", "empty " };
    char t[sizeof CATALOGUE[0].name + 8], u[sizeof CATALOGUE[0].name];
    unsigned char k = w >> 8;
    struct msg c;

    c.kind = E_TASK;
    c.who = 0;
    c.b = named;
    c.w1 = w;
    if ((unsigned char) w == TE_KINDLED) {
        outcome_words (&c);
        return;
    }
    if ((unsigned char) w == TE_IDLE) {
        reach = 0;
        othings (IO_CUT, "", "", 0);
        return;
    }
    if ((unsigned char) w != TE_PACKED) {
        what_hi = named & 0x80;
    }
    if ((unsigned char) w == TE_BEGUN) {
        step_k = k;                             /* (The step under way: a stop's words, task.c's TE_IDLE) */
        step_n = named | 0x80;                  /* (Its kind, task.c's noun: the top bit what_hi's) */
        t[0] = 0;
        if (k & SM_ALL) {
            strcpy (t, sm[k >> 6]);             /* ("You put away the old torch.": the step's own word) */
        } else if (step_aw && (k & ~SM_ALL) >= SV_IN) {   /* (Not "the lit torch ... still burning") */
            strcpy (t, aw[step_aw]);            /* (Told apart from another of its kind at hand: "You put the spent torch in
                                                **   your pack.", the world's apart, the user's playtest, 2026-11-06) */
        }
        hub_mem (KIND_BANK, t + strlen (t), CATALOGUE[named].name, sizeof CATALOGUE[0].name);
        said_s[2] = t;
        if ((k &= ~SM_ALL) >= SV_SCONCE || k == SV_SHEATHE) {   /* (Where it goes, its obj: the world's words for it, "You stow
                                                **   the torch in your backpack.", "You put the sword in the scabbard.": the
                                                **   world's ireq.mat, its kind; one wording, 2026-11-13) */
            hub_mem (0, &k, &HUB->ireq.mat, 1);
            hub_mem (KIND_BANK, u, CATALOGUE[k].name, sizeof CATALOGUE[0].name);
            said_s[0] = u;
            k = w >> 8;
        }
        said_n = (k & ~SM_ALL) - SV_OFF;
        c.w1 = SAID_TASK_STEP;
    }
    step_aw = 0;
    outcome_words (&c);
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A command whose thing is out of reach (the things' IR_NOT_HERE, the puzzle's PZ_FAR: where it is in the hub's ireq,
** its words not said): walked over to it first (I_TAKE's walk alone), the command kept and carried out again as the
** character's free (HOUSE.md's out of reach, walked to first); 1 if so, or if it's the command again and the thing's
** out of reach still (told).  The one walk over (COMMANDS-CRITIQUE.md's §5.2 I), by walk_a, I_TAKE's a: 1, the
** command kept and again; 0, a take's, the world taking it up as it gets there (E_TAKEN: reach 6, its arrival's
** words short); 2, a "go to" (way.c's go_thing), walked on as it gets nearer, nothing after */
static struct msg pm;                           /* (reach_go's, place_go's) */
static unsigned char pm_kind = I_TAKE;          /* (reach_go's walk: feature's I_GOTO, to the door's square) */
unsigned char walk_a = 1;

unsigned char s_reach_go (void)
{
    unsigned b = hub_here ();
    unsigned char r = 1, a = walk_a;

    walk_a = 1;
    if (reach && a == 1) {
        say_no (A_NEARER, 0);
    } else {
        if (a == 1) {
            cmd.obj.lies = 0xFF;                /* (Kept without the side it lay on, said from where it was: it's come to it) */
            hub_map (UI_BANK);
            memcpy (&UIREC->kept, &cmd, sizeof cmd);
        }
        hub_map (0);
        walk_says (a ? 0 : taking);             /* (A take's: its name, "take all"'s next perhaps) */
        pm.kind = pm_kind;
        pm.who = 0;
        I_TAKE_ALONE (pm) = a;                  /* (I_TAKE's, or I_GOTO's to a feature's square: the same) */
        I_TAKE_X (pm) = HUB->ireq.x;
        I_TAKE_Y (pm) = HUB->ireq.y;
        I_TAKE_THING (pm) = HUB->ireq.item;
        if ((r = hub_send (&pm)) != 0) {
            busy = 1;
            if (a < 2) {
                reach = a ? 1 : 6;
            }
            t_prompt (0);
        }
    }
    pm_kind = I_TAKE;
    hub_back (b);
    return r;
}

/* A feature meant (a door, a window, a portcullis, a passage, the stairs, a ladder, a shaft, the way out: the one path
** for them all, each command its own last step, COMMANDS-CRITIQUE.md's §5.2 step J, 2026-11-07): found (obj, its kind
** | PICK_*: choose.c's pick, which asked of several; none, told if PICK_TELL, else the world's to say why; kind 0, the
** way cmd.obj.lies says); walked up to first if it's out of reach, then the command again as it's come (the user's,
** 2026-10-20: "any action that needs its thing within reach walks over first"; I_GOTO, it ends before it): how's FG_NEAR,
** unless it's beside it, by the squares about it (what the UI does from here: a close look); FG_FAR, never (a look one
** way, a move's own walk); else as the world's door work reaches (a door, a window, a portcullis beside it by the four
** ways, or on the walls of the room it stands in: round a bend, diagonal to it, down a passage beyond, walked up to);
** a blow's thing taken in hand (hold_w, hold_first's); faced (the intent, the world's turn: but FG_GO, a move, turning as
** it goes); its first words said (how's low bits, walk_says': a portal's work PA_* + 1, W_*; 0 the thing's own; FW_NONE
** none).  1 to act on it (tx, ty its square, 0 if none found; cmd.obj.lies the way it lies), 2 the same, turned to it
** first; 0 not (none, which, walked up to first, still out of reach, no hand for its thing: told) */
unsigned char __fastcall__ s_feature (unsigned char obj, unsigned char how)
{
    const struct sight* s;
    unsigned char k = obj & 15, d = 1, r = 1;
    int dx, dy;

    if (k) {
        if ((d = pick (obj, cmd.obj.lies)) == 2 || (!d && (obj & PICK_TELL))) {
            return 0;
        }
        if (d) {
            dx = (int) tx - HUB->pc_x;
            dy = (int) ty - HUB->pc_y;
            s = sight_get ();
            if (!(how & FG_FAR) && (how & FG_NEAR ? (unsigned) (dx + 1) > 2 || (unsigned) (dy + 1) > 2 :
                (k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC) && dx * dx + dy * dy > 1 && !(s->where == W_ROOM &&
                tx + 1 >= s->rx && tx <= s->rx + s->rw && ty + 1 >= s->ry && ty <= s->ry + s->rh))) {
                HUB->ireq.x = tx;
                HUB->ireq.y = ty;
                HUB->ireq.item = 0;
                pm_kind = I_GOTO;
                s_reach_go ();
                return 0;
            }
            cmd.obj.lies = bearing (dx, dy);
        }
    }
    if (hold_w[0] && d && !hold_first ()) {
        return 0;                               /* (A blow's thing in hand first, its door seen: no hand for it, why not
                                                **   told) */
    }
    if (!(how & FG_GO) && cmd.obj.lies <= H_NW && cmd.obj.lies != HUB->pc_face) {
        intend_a = cmd.obj.lies;                /* (Faced first: the world's turn, its look) */
        intend2 (I_TURN, 0, 0);
        r = 2;
    }
    if ((how &= FW_NONE) != FW_NONE) {
        walk_says ((const char*) how);          /* (Its first words at once, the world's answer some 2M cycles off) */
    }
    return r;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW7CODE")
#pragma rodata-name (push, "SW7RODATA")

/* A place in the room the character's in (HOUSE.md's going places), by its facing alone: the centre (the middle); a
** side, the far one (the other side, the far end, the wall ahead) unless it says which (near, left, right, the way it
** lies); a corner ("the far left corner"; unsaid, the nearest).  The square there walked to (I_GOTO, a 1: the nearest
** the way reaches, a cave's rock about it); 1 if it's such a place (walked, or told why not), 0 if it isn't one */
unsigned char __fastcall__ s_place_go (const struct cmd* c)
{
    signed char a, b, fa, ra;
    unsigned char k, l, q, fy;
    unsigned x0, x1, y0, y1, px, py, f0, f1, r0, r1, fc, rc;
    struct sight* s;
    const char* w = cl;                         /* (Its words: the line's, "the near side" the parser's how) */

    k = c->obj.kind;
    if (k == OB_ROOM) {                         /* ("The centre of the room": the place, not the room) */
        k = strstr (w, "corner") ? OB_CORNER : strstr (w, "cent") || strstr (w, "middle") ? OB_CENTRE : OB_SIDE;   /* (Its
                                                **   far side: "the far end of the room", "cross the room") */
    }
    if (k != OB_CENTRE && k != OB_SIDE && k != OB_CORNER && k != OB_WALL) {
        return 0;
    }
    s = sight_get ();
    if (s->where != W_ROOM || s->dark) {
        if (k == OB_CORNER || k == OB_WALL) {
            return 0;                           /* (A passage's corner, a wall: as ever) */
        }
        if (s->dark) {
            intend_a = pc_face;                 /* (A place it can't see: straight on) */
            intend2 (I_GO, 0, 0);
            return 1;
        }
        msg ("You aren't in a room.");
        return 1;
    }
    a = b = 2;
    if (k == OB_CENTRE) {
        a = b = 0;
    }
    a = strstr (w, "far") || strstr (w, "other") || strstr (w, "opposite") ? 1 : strstr (w, "near") ? -1 : a;
    b = strstr (w, "left") ? -1 : strstr (w, "right") ? 1 : b;
    l = c->obj.lies != 0xFF ? c->obj.lies : c->dir <= H_NW && a == 2 && b == 2 ? c->dir : 0xFF;
    if (l != 0xFF) {
        l = l & CQ_REL ? l & 7 : (l - pc_face) & 7;    /* (As it faces) */
        a = l == 7 || l <= 1 ? 1 : l >= 3 && l <= 5 ? -1 : a;
        b = l >= 1 && l <= 3 ? 1 : l >= 5 ? -1 : b;
    }
    if (k != OB_CORNER && a == 2 && b == 2) {
        a = 1;                                  /* (A side, a wall: the far one) */
    }
    x0 = s->rx;
    x1 = s->rx + s->rw - 1;
    y0 = s->ry;
    y1 = s->ry + s->rh - 1;
    px = s->x;
    py = s->y;
    q = ((pc_face + 1) & 7) >> 1;               /* (The nearest of the four it faces: its ahead and its right) */
    fy = !(q & 1);                              /* (Ahead along y: facing north or south) */
    fa = q == 1 || q == 2 ? 1 : -1;             /* (Ahead the way x or y grows) */
    ra = q <= 1 ? 1 : -1;                       /* (Its right the way the other grows) */
    f0 = fy ? y0 : x0;
    f1 = fy ? y1 : x1;
    r0 = fy ? x0 : y0;
    r1 = fy ? x1 : y1;
    fc = fy ? py : px;
    rc = fy ? px : py;
    if (k == OB_CORNER) {                       /* (Unsaid: the nearer end each way) */
        if (a == 2) {
            a = (fa > 0 ? f1 - fc <= fc - f0 : fc - f0 <= f1 - fc) ? 1 : -1;
        }
        if (b == 2) {
            b = (ra > 0 ? r1 - rc <= rc - r0 : rc - r0 <= r1 - rc) ? 1 : -1;
        }
    }
    fc = a == 2 ? fc : !a ? f0 + (f1 - f0) / 2 : a * fa > 0 ? f1 : f0;   /* (Not (f0 + f1) / 2: the frame's squares near */
    rc = b == 2 ? rc : !b ? r0 + (r1 - r0) / 2 : b * ra > 0 ? r1 : r0;   /*   $8000, a sum past 16 bits) */
    I_GOTO_X (pm) = fy ? rc : fc;
    I_GOTO_Y (pm) = fy ? fc : rc;
    if (I_GOTO_X (pm) == px && I_GOTO_Y (pm) == py) {
        msg ("You're there already.");
        return 1;
    }
    msg ("You set off across the room.");       /* (Told at once, the user's D17: nothing till it got there, 13M cycles) */
    t_flush ();
    pm.kind = I_GOTO;
    pm.who = 0;
    I_GOTO_NEAR (pm) = 1;
    if (hub_send (&pm)) {
        busy = 1;
        placing = 1;
        t_prompt (0);
    }
    return 1;
}

unsigned char __fastcall__ s_args (int argc, char* argv[])
{
    unsigned char i, given = 0;
    char* l;

    seed = hy_time () ^ hy_ticks ();            /* (The clock's, unless -s says: here, part 7's, not main's: the main
                                                **   RAM's room) */
    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            given = 1;
            for (seed = 0, l = argv[++i]; *l >= '0' && *l <= '9'; ++l) {
                seed = seed * 10 + *l - '0';
            }
        } else if (!strcmp (argv[i], "-d") && i + 1 < argc) {
            savedir = argv[++i];                /* (The save's folder) */
        } else if (!strcmp (argv[i], "-t") && i + 1 < argc) {
            tfile = argv[++i];                  /* (A theme written by hand: themegen's to fill out) */
        } else if (!strcmp (argv[i], "-k") && i + 1 < argc) {
            kit = argv[++i];                    /* (A tool to begin with: till the items layer, the tests') */
        } else if (!strcmp (argv[i], "-C")) {
            small = 1;                          /* (For the tests: the caches small, paged to the save folder soon) */
        } else if (argv[i][0] == '-' && argv[i][1] && !argv[i][2] && (l = strchr (GE_FLAGS, argv[i][1])) != 0) {
            gear |= 1 << (l - GE_FLAGS);        /* (For the tests, hub.h's GE_*: -K the kit as it was before the outfits,
                                                **   the denizens with none of theirs, their dice as they were; -W no
                                                **   water grown in the chunks, the walks' as they were, slice 4.9's;
                                                **   -M every built stratum mixed, slice 4.10's; -U no rubble, the
                                                **   passages as they were made; -A every built stratum's passages on
                                                **   the diagonal, 2026-11-01; -V a great cavern in every region of a
                                                **   natural stratum, -P every stairway down a long slope, -D every
                                                **   built stratum's rooms at 45 degrees, 2026-11-02) */
        } else if (!strcmp (argv[i], "-m")) {
            clock_on = 1;                       /* (For the tests and debugging: the minutes gone on the status line) */
        } else if (!strcmp (argv[i], "-H")) {
            hard = 1;                           /* (Hard mode: D7's, no autosave to go back to; kept in its save) */
        } else if (!strcmp (argv[i], "-a")) {
            autogo = 1;                         /* (For the tests: a long task goes on at every soft question) */
        } else if (!strcmp (argv[i], "-N")) {
            nobear = 1;                         /* (For the tests: its bearings never tested, as if it knew north) */
        } else if (!strcmp (argv[i], "-J")) {
            nosteps |= PL_NOSTEPS;              /* (For the tests: the reach in one lump, untold: their games as they were) */
        } else if (!strcmp (argv[i], "-E")) {
            nosteps |= PL_DUSK;                 /* (For the tests: the game begins at dusk) */
        } else if (!strcmp (argv[i], "-F")) {
            nosteps |= PL_NOFIT;                /* (For the tests: the outfit as it's drawn, all of it, unfitted to its strength) */
        } else if (!strcmp (argv[i], "-I")) {
            noinit = 1;                         /* (For the tests: no engage initiative test, no grope's) */
            nosteps |= PL_NOFACE;               /*   (nor a test on facing it, nor a weapon broken) */
        } else if (!strcmp (argv[i], "-R") && i + 1 < argc) {
            rolls = dnum (argv[++i]);           /* (For the tests: the dice's own offset, 0-252: the same dungeon, other
                                                **   rolls) */
        } else if (!strcmp (argv[i], "-T")) {
            ++test_start;                       /* (For the tests: the character begins at a way down; -T twice, by
                                                **   the theme's first piece; three times, four strata down) */
        } else if (!strcmp (argv[i], "-c") && i + 1 < argc) {
            file = argv[++i];
        } else if (!strcmp (argv[i], "-r") && i + 1 < argc) {
            rules = argv[++i];                  /* (The ruleset, by its name: its referee dq0NAME) */
        } else if (!strcmp (argv[i], "-p") && i + 1 < argc) {
            ++i;
            pace = argv[i][0] == 'r' ? PACE_RELAXED : argv[i][0] == 'm' ? PACE_MENACING :
                argv[i][0] == 't' ? PACE_TURNS : PACE_NORMAL;
        } else {
            hy_exits ("usage: dq0 [-s seed] [-p r|n|m|t] [-c file] [-d dir] [-H] [-t theme] [-r rs] [-T..]");    /* (Not the tests' own: the room's) */
        }
    }
    return given;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The game begun: its arguments, a game kept carried on, the title, the character, the world started */
void __fastcall__ s_begin (int argc, char* argv[])
{
    unsigned char n = life (args (argc, argv)), v = 0;

    if (n == LF_GOING) {                        /* (A game kept there: carried on) */
        resumed = 1;
        seed = sv.seed;
        Q = sv.q;
        plain = sv.plain & 1;
        v = sv.plain >> 1;                      /* (A vow of fasting's time left: the user's, 2026-11-11) */
        hard = sv.pace >> 7;                    /* (Its hard mode its save's: slice 4.4's) */
        if (pace == 0xFF) {
            pace = sv.pace & 0x7F;
        }
        file = save_path ("../pc.hm");
    } else if (n == LF_NEXT) {
        file = save_path ("../pc.hm");          /* (Its character carried on, into a new dungeon: D8's; its kit, the
                                                **   world's kit.dat, its outfit) */
        v = ((struct life_head*) lb)->fast;     /* (A vow of fasting carried in, its end after the date's midnight:
                                                **   what's left of it after the hour it begins, its seed's, as
                                                **   dq0life reckons it) */
        n = (unsigned char) ((unsigned) seed % 24 * 5);
        v = v > n ? v - n : 0;
    }
    if (!resumed) {                             /* (Its ruleset's referee: a game carried on keeps its own) */
        strcpy (sv.referee, "dq0");             /* (strncpy, not strncat: the library's in the main RAM) */
        strncpy (sv.referee + 3, rules, sizeof sv.referee - 4);
        sv.referee[sizeof sv.referee - 1] = 0;
    }
    rseed (seed);
    signal (SIGINT, interrupted);
    t_init ();
    title ();
    t_flush ();                                 /* (Its words out now: the start's progress shown, 2026-10-30) */
    starting = 1;
    n = history (seed);                         /* (The game's own theme, or a stand-in's: which) */
    dots (2);                                   /* (Its tasks started: told, a dot as each's waited for) */
    start_world (seed, file, n);                /* (The character with it: its file, or the ruleset's generator) */
    if (v) {
        fasting = HUB->now + v * FAST_STEP;     /* (The vow kept, on the clock as it's begun) */
    }
    status ();
    story (resumed);                            /* (The theme's) */
    blank ();
    entered (resumed);
    dots (3);                                   /* (Its first look to come: told, main's loop's dots; the help's line
                                                **   with the first prompt, o_dots') */
    t_done ();
}

/* The game's end: kept (as it ends: the game's quit, not lost), the world stopped.  An ending's, the character's sheet
** told as it ends; a death's not kept (its last save, if it's made one, left to go back to: D7's; dq0end says which)
** unless it's hard mode (dq0 -H: the save marked ended, the character gone; slice 4.4's) */
void s_finish (void)
{
    struct req* r;

    if (Q.done) {
        r = referee (RS_DESCRIBE, 0);
        say (r->text);
        blank ();
        r->state = REQ_FREE;
    }
    if (Q.done != END_DIED || hard) {
        save_game ();
    }
    stop_world ();
    t_end ();
    if (Q.done == END_DIED && savedir) {        /* (The life's account: dq0end's; hard mode's, its end recorded) */
        between (2);
        hub_join (hub_run (lb + 300, lb));
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

int main (int argc, char* argv[])
{
    unsigned shown = 0, t;
    unsigned long next = 0;
    unsigned char quick = 0;
    unsigned char k, f;
    char* l;

    ovl_in (find_dirs ());                      /* (The overlays read in, and the swapped parts with them, first) */
    begin (argc, argv);
    while (!quit) {
        hub_map (0);                            /* (The hub at $8000 again: a call may have moved the window) */
        while (t_haskey ()) {
            k = t_getkey ();
            if (k == CH_ESC) {
                wset (WS_PAUSE, paused = !paused, 0);
                status ();
            } else if (k == CH_RESIZE) {
                t_init ();
            } else {
                if (!HUB->ui_keys) {
                    kseq += 2;                  /* (A line's first key: its own, the holds begun again for it) */
                }
                HUB->ui_keys = kseq;            /* (A key: the world's long jobs hold back, ui_first's, its echo and
                                                **   an Enter's words already behind none) */
                t_key (k);
                quick = 25;                     /* (A line being typed: its Enter looked for twice as often, the
                                                **   background given way to it the sooner) */
                if (k == 10) {                  /* (Enter: t_getkey's) */
                    break;                      /* (A line entered: taken up at once, the keys after it next time) */
                }
            }
        }
        if (t_next () && (!busy || !held)) {
            HUB->ui_cmd = 1;                    /* (A line entered: the senses' look and the minds give way at once,
                                                **   not after the events and the status line; its answer first) */
            owed = 0;
            fore_sync ();                       /* (The answer's tasks in the foreground meanwhile) */
            letters_in ((char*) t_next ());     /* (Its letters made its things' names first, as it's posted: letters.c's;
                                                **   not again, cq_st) */
            cq_post (t_next ());                /* (The command parser at work on it at once, while the events and
                                                **   the status line are seen to: not after them, 2026-10-30) */
        }
        HUB->ui_keys = inn ? kseq : t_next () != 0; /* (Keys still on the line typed, or a line entered held for the
                                                **   character, its first word still to come; else its Enter's ui_cmd now, or none) */
        k = events ();
        heard ();
        if (starting) {
            dots (1);                           /* (The game's start: its progress, then the help's line) */
        }
        now = hub_now ();
        if (HUB->ui_cmd) {
        } else if (k || ((now >= next || now < next - 600 || (HUB->pc_task.kind && HUB->pc_task.done != bar_done)) &&
            hy_ticks () - shown >= (inn ? 1000 : 200) && !t_haskey ())) {
            next = (now / 600 + 1) * 600;       /* (The status line as the events change it, and its clock as a
                                                **   game minute goes, a real second apart at most, five while a
                                                **   line's being typed: only what's changed drawn, t_status; the
                                                **   minute's end kept, not divided for each time round; not as
                                                **   events come while a command's in hand: its answer first, nor
                                                **   its clock then, 2026-11-01; nor its clock while keys wait, a
                                                **   line typed ahead, 2026-10-30) */
            shown = hy_ticks ();
            status ();
        }
        if (!busy && !inn && (rebar || pending[0])) {   /* (Not while a line's being typed: a stop typed during "take
                                                **   all" acted on before its next thing, the user's rule of a line typed
                                                **   during a reach, 2026-11-06; dq0reachstop's, 2026-11-07) */
            HUB->ui_giving = 1;                /* (Being given, as a line's command: "take all"'s next thing's two
                                                **   intents falling with the clock moved between them or not, 2026-11-04) */
            when_free ();                       /* (A door barred, a thing lifted out: part 14's) */
            hub_map (0);
            HUB->ui_giving = 0;
            hub_ring (T_WORLD);
        }
        if (!busy && (reach & 1)) {
            ++reach;                            /* (Walked over to its thing, or its timed work's time up: the command
                                                **   kept carried out again, its end; till the next's, s_command's) */
            hub_map (0);
            HUB->ui_giving = 1;                 /* (Being given, as a line's command: ui_giving's) */
            command (0);
            hub_map (0);
            HUB->ui_giving = 0;
            hub_ring (T_WORLD);
        }
        if (out_told && t_next ()) {
            out_line ();                        /* (Senseless: the line not taken, part 2's) */
        }
        if (held & 2) {
            held = 0;                           /* (That line let go, parts.c's: its parse with it) */
            cq_drop ();
        }
        if ((l = (char*) t_next ()) != 0 && (!busy || !held)) {
            letters_in (l);                     /* (Its letters made its things' names first: letters.c's) */
            cq_post (l);                        /* (The command parser at work on it meanwhile) */
            if (!held) {
                ntold = 0;                      /* (Its answer: no look's sentence said yet, no harm's state; a
                                                **   line held for the character, taken again, the same answer:
                                                **   what its task's end told meanwhile not told again) */
                harm_told = 0;
            }
            asked = 0;                          /* (Its long task, if it's one: not asked of a sound yet) */
            hub_map (0);
            HUB->ui_cmd = 1;                    /* (A command in hand: the senses' look gives way meanwhile, from
                                                **   its first look, at_once's, on: a line typed as a look began
                                                **   waited for it, its parsing a third of the CPU) */
            k = 0xFF;
            if (asking) {                       /* (A soft question's answer: yes goes on, no stops it; a command
                                                **   stops it, then is carried out as the character's free) */
                asking = 0;
                if ((k = answer_of (l)) != 0) {
                    t_line ();                  /* (Its line taken: no command) */
                    cq_drop ();
                    HUB->ui_cmd = 0;
                    l = 0;
                }
                intend2 (k == 1 ? I_GO_ON : I_STOP, !k, 0);     /* (A command's stop: a fight fought on after it) */
            }
            if (l && busy && (held = !k || (k = at_once (l)) != 1)) {
                if ((took = k >= 2) != 0) {
                    intend2 (I_STOP, 1, 0);     /* (A fight taken over: its round out, then the command; a line that
                                                **   breaks off, its long task stopped at once, task.c's free_cmd 3) */
                    hub_map (0);
                    if (k == 3 && reach >= 5 && (!HUB->pc_task.kind || HUB->pc_task.kind == LT_FIGHT)) {
                        took = HUB->pc_task.kind;   /* (No fight to stop now: fighting on begun after, stopped for it) */
                        task_words (TE_IDLE, 0);    /* (A reach's step with no long task to stop, engaged a round's
                                                **   action, or a fight between its steps: the reach ends where it
                                                **   stands, the pack's place told, then the line) */
                    }
                }
                HUB->ui_cmd = 0;                /* (Held: the next waits for the character, not parsed again) */
                l = 0;
            }
        } else {
            l = 0;
        }
        if (l) {
            HUB->ui_giving = 1;                 /* (Its command being given: the turns pace waits for it, the
                                                **   world's turns_run) */
            iqh = HUB->iq_head;                 /* (The UI's intents' ring as the line's taken: one sent, its answer
                                                **   owed, the world's news) */
            f = names_in (l = t_line ());       /* (The line's first look, in the overlays' banks: a part's
                                                **   copied in only for a line that may be its; an ordinary
                                                **   command's only part 2, the commands') */
            k = (alias_wait || alias_said (l)) && alias_line (l) ? NL_DONE :    /* (The shortcuts': alias.c's) */
                f & NI_LINE ? names_line (l) : NL_NONE;     /* (A name given or forgotten: the names') */
            if (k == NL_NONE && pacing (l) > (HUB->gait == GA_WALK && !paced) && (cq_cmd (l), pace_cmd (l))) {
                k = NL_DONE;                    /* (The pace's, the state's, a small move's: pace.c's; "walk to the
                                                **   door", walking plainly: nothing to change, three parts' swaps and a
                                                **   word with the world spared, the user's D17) */
            }
            if (k == NL_NONE && !strncmp (l, "around", 6)) {
                around (l);                     /* (What's in sight listed, its capitals; or two letters swapped:
                                                **   caps.c's) */
                k = NL_DONE;
            }
            if (k == NL_NONE && names_n && (f & NI_THING) && (t = names_subst (l)) != 0) {
                to_hand ();                     /* (The things' names in it made theirs, "#" and the id; a weapon
                                                **   named taken in hand to strike with: its time, as after ()'s) */
                intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
            }
            if (cq_again) {
                cq_again = 0;
                cq_drop ();                     /* (Its names made theirs: the line as it is now posted again) */
            }
            if (k != NL_DONE) {
                command (l);                    /* (The character free; or a command that wants nothing of it:
                                                **   held, the next waits for it to be free, not parsed again) */
                if (k == NL_DEN) {
                    names_den ();               /* (A denizen named: its name recorded) */
                }
            }
            cq_drop ();                         /* (The command parser's reply, if the line wasn't a command's: not
                                                **   left there) */
            held = 0;
            quick = 25;                         /* (Its answer looked for twice as often, a quarter second) */
            if (!busy) {
                status ();                      /* (A world's answer to come: the status line after it, events') */
            }
            hub_map (0);
            owed = HUB->iq_head != iqh;
            obeat = HUB->beat[T_WORLD];
            HUB->ui_giving = 0;                 /* (Given: the world on, woken to it) */
            hub_ring (T_WORLD);
            HUB->ui_cmd = busy | owed;          /* (Its answer the world's, not yet said: the look and the minds
                                                **   give way till the character's news comes, events'; an intent
                                                **   sent, the character not busy, its answer owed too: a step that
                                                **   can't be taken, the world's words) */
        } else {
            ready ();                           /* (The commands' part back, if another's in) */
        }
        if (stirs && !inn && !HUB->ui_cmd) {
            stirs = 0;
            t_flush ();                         /* (Its answer out, not waiting on these words) */
            stir ();                            /* (A stir held while the line was typed and answered: told now, its
                                                **   answer begun, ui_cmd's) */
        }
        t_done ();
        HUB->ui_eq = (unsigned char) place;     /* (All it does about the events done: the turns pace on, the world's turns_run) */
        HUB->ui_dq = (unsigned char) dplace;
        fore_sync ();                           /* (In the foreground or out, as a command's in hand or not) */
        hub_nap (t_wrote && t_haskey () ? 0 : quick ? (--quick, 1) : nap);  /* (Keys that came as it said something,
                                                **   a line typed as an answer or a look went out: no nap, taken at
                                                **   once, 2026-10-30.  A fiftieth of a second: the keys looked
                                                **   for, the world's
                                                **   events told; each look at the keys a call of the console's,
                                                **   some 9,000 cycles with its own, a quarter of the CPU at two
                                                **   ticks while nothing happened; a hundredth just after a command,
                                                **   its answer told the sooner; and woken at once by the world's
                                                **   events, its doorbell rung with them; a fifth of a second as the
                                                **   game starts, its look at the keys and the console's a quarter of
                                                **   the CPU otherwise: the start's time, 2026-11-03) */
    }
    finish ();
    return 0;
}

/* ==== Part 14: the body's positions and the background, worded (slices 4.5 and 4.6: overlay 15's bank, the main
** RAM's room) */
#pragma code-name (push, "SW14CODE")
#pragma rodata-name (push, "SW14RODATA")

/* What waited for the character to be free (main's, seldom: part 2's, the main RAM's room, slice 4.2's; part 14's,
** part 2's room, 2026-10-29; part 12's, overlay 3 full, 2026-10-31) */
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW14CODE")            /* (Part 14's since 2026-11-04, overlay 15's room) */
#pragma rodata-name (push, "SW14RODATA")
void s_when_free (void)
{
    if (rebar) {                                /* (A door barred: its bar set in its brackets, on the square the
                                                **   character stands on, beside it; what it carries anew) */
        rebar = 0;
        HUB->ireq.to = 1;
        HUB->ireq.x = HUB->pc_x;
        HUB->ireq.y = HUB->pc_y;
        HUB->ireq.item = HUB->pc_stratum;
        things (IO_BAR, "");
        to_hand ();
    }
    if (pending[0]) {                           /* (Lifted out, or come to: now taken up) */
        strcpy (cmd.obj.word, pending);         /* (Its steps ask this one again, "take all"'s next; a thing of that name) */
        cmd.obj.kind = 0;
        HUB->pc_lump = (HUB->pc_lump & ~PL_MORE) | (take_more ? PL_MORE : 0);
        take (pending);                         /* (Its own to clear: "take all"'s next may be put there as it's done) */
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW14CODE")
#pragma rodata-name (push, "SW14RODATA")

/* A denizen's sound heard, or its doing seen (heard's, its weighing the main code's: m its event, rel which way it is),
** told: seen, said.txt's DEN row (its actor swapped into the row's words: "The dwarf opens the door.", the character's
** own words for it the same row's, @open); heard, in the sound's words, the sound task's if it's put them there, else
** the DEN row's (its arg 1: the words left in lb, said_q, "a door opening"); its long task paused at the first (part
** 14's: seldom, the main RAM's room; main.c's door_words, part 18's, and its own strings before, 2026-11-11) */
void __fastcall__ s_noise_told (const struct msg* m, unsigned char rel)
{
    unsigned char d = E_DEN_DE (*m);

    hub_map (DEN_BANK);                         /* (Its sound's words: the sound task's, if it's put them there) */
    memcpy (heard_what, SND_AT + (E_DEN_DKIND (*m) % SND_KINDS * 2 + (d == DE_NOISE)) * SND_REC, sizeof heard_what);
    hub_map (0);
    heard_what[sizeof heard_what - 1] = 0;
    said_s[0] = 0;                              /* (No thing done to: a piece every actor's says "the door") */
    said_s[2] = heard_what;
    if (d >= DE_RALLY) {                        /* (A successor at its post rallying its fellows, HOUSE.md's morale; a
                                                **   sleeper's snore, those on watch talking, the alarm raised, the
                                                **   day's, 2026-10-31; a pastime heard (dice rattling, singing), or let
                                                **   go as its denizen rises to act, seen, leisure's, tell.c's words:
                                                **   seen, by its name, else heard; a tunneller's, a door worked, Phase
                                                **   3C's, a portcullis, a lock picked, 2026-11-02) */
        if (d == DE_DIGS) {
            return;                             /* (A tunneller's spells after its first: told as it began) */
        }
        if (d >= DE_PASTIME && d < DE_DIG) {
            if (E_DEN_SEEN (*m) ? d < DE_UP : d >= DE_UP) {
                return;                         /* (One seen at its pastime: the look's words say it) */
            }
            tell ((d - DE_PASTIME - PT_DICE + T_PAST_HEARD) | TELL_COPY);   /* (Its words: DEN.any's text) */
        } else {
            said_n = said_q = 1;                /* (What it sounds like: the row's heard words, into lb) */
            outcome_words (m);
            said_q = 0;
            strcpy (heard_what, lb);
        }
        said_n = 0;
        if (E_DEN_SEEN (*m)) {
            if (d == DE_DUG) {                  /* (Its way through seen: where, the heard words' way's) */
                heard_words (0, rel);
                said_s[2] = strcpy (lb + 300, lb + 10 + strlen (heard_what));
            }
            outcome_words (m);
            return;
        }
    }
    heard_words (d | (E_DEN_BETWEEN (*m) ? 0x80 : 0), rel);
    msg (lb);
    if (busy && HUB->pc_task.kind && !asked && !HUB->pc_out) {
        asked = 1;                              /* (The first sound heard as a long task goes: asked, the world's to
                                                **   pause it; not as it lies senseless, 2026-11-13) */
        intend2 (I_ASK, TE_ASK_HEARD, 0);
    }
}


/* The body's position as the character said (E_BEAT WB_POSED's w1: the position times 256, | 0x8000 there already).
** Part 12's since 2026-10-31 (part 14's bank, overlay 3, full) */
#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW12CODE")
#pragma rodata-name (push, "SW12RODATA")
void __fastcall__ s_pose_words (unsigned w)
{
    static const char pdo[5][10] = { "", "lie down", "crouch", "kneel", "sit down" };
    static const char pis[5][10] = { "", "lying", "crouching", "kneeling", "sitting" };
    unsigned char k = (w >> 8) & 7;

    if ((w & HW_WORDS) == HW_WORDS) {
        hide ((const char*) w);                 /* (Hiding oneself's: part 10's) */
        return;
    }

    sayf (w & 0x8000 ? "\nYou are %s already." : "\nYou %s.", w & 0x8000 ? pis[k] : pdo[k]);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW14CODE")
#pragma rodata-name (push, "SW14RODATA")

/* The background and the air, worded (way.c's, slices 4.5's: the senses' sound layer's, the hub's pc_bg, air_h,
** air_s): a listen's end (what 0: heard nothing, what it hears behind it all; 1, heard something, the background
** after it), a sniff's start (2: the draught, from where, or the air still) */
void __fastcall__ s_bg_words (unsigned char what)
{
    static const char* const bgw[] = { "", "water trickling somewhere near", "the wind in a shaft near by",
        "those about you moving" };
    static const char* const airw[] = { "", "faint ", "", "strong " };
    static const char* const relname[8] = {
        "ahead", "ahead to your right", "your right", "behind you to your right", "behind you",
        "behind you to your left", "your left", "ahead to your left" };
    unsigned char b = HUB->pc_bg & 3;

    if (what == 2) {
        if (HUB->air_s) {
            sayf ("\nYou feel a %sdraught from %s.", airw[HUB->air_s & 3], relname[(HUB->air_h + 4 - HUB->pc_face) & 7]);
        } else {
            msg ("The air is still.");
        }
    } else if (!what) {
        sayf (b ? "\nYou hear nothing but %s." : "\nYou hear nothing.", bgw[b]);
    } else if (b) {
        sayf ("\nBehind it all, %s.", bgw[b]);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
