/*
** main.c - Dingus Quest Zero: the UI's task, its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file] [-d folder] [-t theme] [-k things] [-r rules] [-T [-T]]: -d keeps the game in a
** folder (on any card), and carries on the game kept there if there is one; without it nothing's kept. the seed makes
** the same game each time (the dice: dice.h's rseed), and its region's history and theme (themegen's: theme.h), but
** a seed under 65,536 a stand-in theme's (the tests'); -t a theme written by hand, the history made to fit it; the
** pace is r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (the ruleset's: hmchar's).  Without one, the
** ruleset's character generator (its referee names it: hmchar) is run to make one (in this window, the screen as it
** is outside the game while it runs), into /ram/dq0.hm; a ruleset with none makes its own.  The rules: the ruleset's
** name (RULESETS.md: harn, HarnMaster's, if none's given; null, flat odds), its referee libdir/dq0NAME (the save's: a
** game carried on keeps its own).  The tests' own: -C, -a, -T, and -I (no engage initiative test, nor a grope's test
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
#include "rs.h"
#include "level.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

unsigned long now;                              /* The game's time, as the world's clock last said */
unsigned char pc_face;                          /* (The character's facing, copied for the overlays) */
unsigned char pace = 0xFF, paused;
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

static unsigned char asking;                    /* (A long task's soft question put, the answer awaited) */
const char* cl;                                 /* (The command's line, s_command's: chores' words of it; help.c's) */
unsigned char harm_told;                        /* (The states a harm's words told in this answer: intro.c's) */
unsigned char autogo;                           /* (dq0 -a: every soft question answered yes, unasked: the tests') */
static unsigned char noinit;                    /* (dq0 -I: no engage initiative test, no grope's: the tests') */
static unsigned char asked;                     /* (A sound's question put in this command's long task) */
static unsigned bar_done;                       /* (The long task's part done as the bar was last drawn) */
static unsigned char busy = 1;                  /* The character's doing something: no command till it's free (nor
                                                **   till the world's started, and says so: E_READY) */
static unsigned char held;                      /* (A line typed as it's busy, held till it's free; */
static unsigned char took;                      /*   and the fight it was in stopped for it, once) */
static unsigned place;                          /* Where this task is in the world's events */
static int task_[NTASKS];                       /* Each task's number, as it was started (0: not) */
static unsigned char test_start;
static unsigned char small;                     /* (dq0 -C: the caches small, the tests') */
static const char* kit;                         /* (dq0 -k: things more, as the game starts: a tool, ...) */
static const char* tfile;                       /* (dq0 -t: a theme written by hand) */
static unsigned char made;                      /* (The theme to read: 0xFF the game's own, themegen's; theme.h) */
const char* savedir;                            /* (dq0 -d: the save's folder; task.c's meta too) */
unsigned char __fastcall__ things (unsigned char op, const char* name);   /* (way.c's too, part 6's) */
static void to_hand (void);
static void __fastcall__ dig_used (unsigned char n);
static char dug_with[16];                       /* (The tool a dig took, named) */
static unsigned char finds;                     /* (Things a search turned up, told as it ends: the hub's text) */
struct ireq judged;                             /* (A thing looked at closely: what it is, for the appraiser: way.c's,
                                                **   part 6's, examine) */
struct portal viewed;                           /* (A portal looked at closely: the world's record of it) */
static char pending[16];                        /* A thing to take up as the character's free (a portal's part lifted
                                                **   out; one the world's walk found fixed: E_TAKEN's) */
static char taking[16];                         /* (The thing taken up last named) */
static char lifting[16];                        /* (A portal's part being lifted out, to take up after) */
static unsigned char barw, rebar;               /* (A bar to hand to bar a door with, its width; and the door barred:
                                                **   the bar to set in its brackets, as the character's free) */
unsigned char strike;                           /* (What it strikes with: the parser's, SK_*, sent with each blow) */
static unsigned char pc_gait, pc_watch;         /* (The character's file's pace and watchfulness: the referee's) */

struct pc P;
struct quest Q;                                 /* The theme's puzzle, as it stands (theme.c's; the save's) */
char tname[32];
static volatile unsigned char quit;

static void interrupted (int sig)
{
    (void) sig;
    quit = 1;
}

/* ---- Where its files are: the directory it was started from (rc's $prog), if its lib/dq0 is there; else the
** card's /sd/0/games (dq0 found through /bin, bound there) */

char gamedir[40], libdir[48];

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

static void find_dirs (void)
{
    char pv[sizeof gamedir + 1];
    const char* prog = env ("prog", pv, sizeof pv);
    char* slash;
    int fd;

    if (prog && strlen (prog) < sizeof gamedir) {
        strcpy (gamedir, prog);
        if ((slash = strrchr (gamedir, '/')) != 0) {
            *slash = 0;
            strcpy (libdir, gamedir);
            strcat (libdir, "/lib/dq0");
            strcat (libdir, "/dq0.ovl");
            fd = open (libdir, O_RDONLY);
            libdir[strlen (libdir) - 8] = 0;
            if (fd >= 0) {
                close (fd);
                return;
            }
        }
    }
    strcpy (gamedir, "/sd/0/games");
    strcpy (libdir, "/sd/0/games/lib/dq0");
}

/* ---- The overlays: libdir/dq0.ovl, an 8K bank each (dq0.cfg's OV1 ...; the file cut after the last one used:
** make.js), read into banks of the game's own */

static void ovl_load (void)
{
    static char path[56];
    unsigned char* b = (unsigned char*) _SWAP_START__;  /* (512 bytes of the swapped parts' area: none in it yet) */
    int fd, first, n;
    unsigned char k;
    unsigned off;

    strcpy (path, libdir);
    strcat (path, "/dq0.ovl");
    if ((fd = open (path, O_RDONLY)) < 0 || (first = hy_banks_alloc (sizeof ovbank)) < 0) {
        hy_exits ("dq0: no overlays (lib/dq0/dq0.ovl), or no RAM banks");
    }
    for (k = 0; k < sizeof ovbank; ++k) {
        ovbank[k] = first + k;
        for (off = 0; off < 0x2000; off += 512) {
            n = read (fd, b, 512);
            if (n <= 0) {
                break;
            }
            bwrite (first + k, (void*) (0x8000 + off), b, n);
        }
    }
    close (fd);
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
    strcpy (p, "-q");
    strcpy (p += 3, "-s");
    ultoa (seed, p += 3, 10);
    strcpy (p += strlen (p) + 1, "-o");
    strcpy (p += 3, savedir ? savedir : "/ram/dq0");
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
    if (r->result) {
        t_end ();
        hy_exits ("dq0: no character in its file");
    }
    at = r->text;
    if ((l = reply_line (&at)) != 0) {
        strncpy (P.name, l, sizeof P.name - 1);
    }
    if ((l = reply_line (&at)) != 0) {
        strncpy (P.occupation, l, sizeof P.occupation - 1);
    }
    pc_gait = r->a;                             /* (A new game's, the world's to set) */
    pc_watch = r->b;
    r->state = REQ_FREE;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* The character's sheet, the referee's words */
static void sheet (void)
{
    struct req* r = referee (RS_DESCRIBE, 0);

    blank ();
    say (r->text);
    blank ();
    r->state = REQ_FREE;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The save (DUNGEON.md): game.dat, the UI's (the game's state, the strata's records), beside the things' items.dat
** (dq0p's), the referee's pc.hm, the world's change records (changes.dat and the regions' rg*.dat: world.h's) and
** the maker's seen.dat */

#define SAVE_MAGIC  0x5144                      /* "DQ" */
#define SAVE_VER    15                          /* (2: the items' records with their size, the portals' make; 3, the
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
                                                **   thing's lid, its tie, its stopper, its lock) */
static struct save {
    unsigned        magic;
    unsigned char   version, pace;
    unsigned long   seed;
    char            referee[12];                /* The ruleset's referee (dq0 -r NAME's: dq0NAME) */
    struct pc       pc;
    struct quest    q;                          /* (Its theme, its puzzle, and its ending: a game done isn't carried
                                                **   on, but a new one begun) */
} sv;
static unsigned char resumed;

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

static char* __fastcall__ save_path (const char* name)
{
    static char p[56];

    strcpy (p, savedir);
    strcat (p, "/");
    strcat (p, name);
    return p;
}

/* A game kept in the folder: its state read (0), or -1 (none) */
static int load_save (void)
{
    int fd = open (save_path ("game.dat"), O_RDONLY), n;

    if (fd < 0) {
        return -1;
    }
    n = read (fd, &sv, sizeof sv);
    close (fd);
    return n == sizeof sv && sv.magic == SAVE_MAGIC && sv.version == SAVE_VER && !sv.q.done ? 0 : -1;
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
    strcpy (HUB->slot[SLOT_UI].text, save_path ("pc.hm"));
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
    sv.pace = pace;
    sv.seed = HUB->seed;
    sv.pc = P;
    sv.q = Q;
    if ((fd = open (save_path ("game.dat"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, &sv, sizeof sv);
        names_keep (fd, 0);                     /* (The names given: the theme bank's) */
        close (fd);
    }
}

/* The save's names given (after its header) */
static void __fastcall__ load_names (void)
{
    int fd = open (save_path ("game.dat"), O_RDONLY);

    if (fd >= 0) {
        read (fd, &sv, sizeof sv);
    }
    names_keep (fd, 1);
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

    hub_map (bank);
    memcpy (to, from, n);
    hub_back (b);
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* A task of the game's started, libdir/name in role t (more: its third argument, or none); or the game's over (but
** for the sound's: the game goes on silent) */
static void __fastcall__ start (const char* name, unsigned char t, const char* more)
{
    static char path[56];

    strcpy (path, libdir);
    strcat (path, "/");
    strcat (path, name);
    if ((task_[t] = hub_spawn (path, t, more)) < 0 && t != T_SOUND) {
        t_end ();
        strcpy (path, "dq0: this can't be started: lib/dq0/");
        strcat (path, name);
        hy_exits (path);
    }
}

static void world_words (void);
static char gods[112];                          /* (The world's deities' names, read as the game starts: world_words') */

static void __fastcall__ start_world (unsigned long seed, const char* file)
{
    struct req* r;
    unsigned char k;
    unsigned round;
    int w;
    char* l;

    strcat (libdir, "/dq0g.txt");               /* (Read before any of the game's tasks is begun: the time it takes
                                                **   theirs alike) */
    w = open (libdir, O_RDONLY);
    libdir[strlen (libdir) - 9] = 0;
    k = read (w, gods, sizeof gods - 1);
    gods[k < sizeof gods ? k : 0] = 0;
    close (w);
    if (hub_create (HUB_BANKS)) {
        t_end ();
        hy_exits ("dq0: no shared segment or semaphores for its tasks");
    }
    HUB->seed = seed;
    HUB->test_start = test_start;
    HUB->small = small;
    hub_map (DEN_BANK);                         /* (The sounds' words: none till the sound task's put them there; the
                                                **   places kept lit, the denizens' as they decide its populations) */
    memset (SND_AT, 0, (2 * SND_KINDS + SND_WORLD) * SND_REC);
    memset (KEPT_AT, 0, sizeof (struct kept));
    hub_map (0);
    start ("dq0c", T_COMMANDS, libdir);         /* (Its command parser: its tables read as the others start) */
    start ("dq0s", T_SOUND, libdir);            /* (Its sound: SOUND.md) */
    start (sv.referee, T_REFEREE, libdir);      /* (Its rules, the ruleset's referee: its folder, for its overlay) */
    r = referee (RS_HELLO, 0);
    round = r->w1;                              /* (The ruleset's round, in tenths: the world's to keep) */
    l = strchr (r->text, '\n');                 /* (Its name, then its character generator's: none, "") */
    character (file, seed, l ? l + 1 : "");     /* (The character: its file, or the generator's) */
    r->state = REQ_FREE;
    load_pc ();                                 /* (Its senses the referee's, before the world looks through them) */
    if (savedir) {
        strncpy (HUB->savedir, savedir, sizeof HUB->savedir - 1);
    }
    if (resumed) {                              /* (Carried on: where it was, and when, the strata as they were: the
                                                **   world's, its world.dat) */
        load_names ();
        HUB->resume = 1;
    } else {
        names_keep (-1, 1);                     /* (No names given yet) */
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
    HUB->wreq.y = noinit ? pace | WY_NO_INIT : pace;
    HUB->wreq.a = pc_gait;
    HUB->wreq.b = pc_watch;
    hub_call (&HUB->wreq, T_WORLD);
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
** (libdir/dq0g.txt: the denizens' data's, deities.txt's, spaces between) */
static void world_words (void)
{
    static char nm[sizeof CATALOGUE[0].name + 1];
    static unsigned n;
    unsigned char i, k;

    n = 0;
    for (i = 0; i < NKINDS; ++i) {
        hub_map (CAT_BANK);
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
    if (n + (k = strlen (gods)) < CQ_WORDS) {   /* (Then the deities') */
        memcpy (HUB_WINDOW + n, gods, k);
        n += k;
    }
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

static unsigned char cq_st;                     /* The line posted to the command parser: none (0), its reply not
                                                **   taken (CQ_OUT), or taken (CQ_IN: its struct cmd in CMD_BANK) */
#define CQ_OUT      1
#define CQ_IN       2
static signed char cq_res;                      /* (Its verdict, cq_cmd's) */
unsigned char cq_again;                         /* The line changed since it was posted (names_subst's): posted again */
struct cmd cmd;                                 /* The command being carried out (the parser's, cq_cmd's) */
unsigned char reach;                            /* (1: walking over, the command kept (the UI bank's) to carry out
                                                **   again as it's free; 2: carried out again; 0 again as a long task's
                                                **   stopped, task.c's, or a line's command begun; 3: a thing's timed
                                                **   work under way, words.c's, the command kept the same, 4: carried
                                                **   out again, its end) */
static unsigned char placing;                   /* (A place in the room walked to: place_go's, its arrival not "over
                                                **   to it") */
unsigned char untarget;                         /* (The target let go at the next command: resolve.c's) */

/* ---- The status line */

void status (void)
{
    static char left[48], right[72];            /* (Its right side's longest: paused, a fight's words, the state, the
                                                **   facing, the minutes) */
    char n[6];

    strcpy (left, P.name[0] ? P.name : "Dingus Quest Zero");
    strcpy (right, paused ? "PAUSED  " : "");
    if (task_[T_WORLD] > 0) {
        bar_text (right, bar_done = HUB->pc_task.done, HUB->pc_task.len);   /* (A long task's bar, its part done;
                                                **   a fight's foe; the light's minutes left: task.c's) */
        strcat (right, HUB->pcstatus);          /* (The referee's words: "UP 0" ...) */
        strcat (right, "  facing ");
        strcat (right, head_abbr[HUB->pc_face & 7]);
        strcat (right, "  ");
    }
    strcat (right, utoa ((unsigned) (now / 600), n, 10));    /* (The game's minutes gone, the user's: no seconds) */
    strcat (right, " min");
    t_status (left, right);
}

/* The game's end (theme.h's END_*): its words, the theme's; the save marked so (a game done isn't carried on) */
static void __fastcall__ end (unsigned char how)
{
    ending (how);
    Q.done = how;
    quit = 1;
}

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
static unsigned char heard_rel = 0xFF;
char heard_what[20];


/* A blow (an E_FIGHT: the character's, or one at it), worded (overlay 6's); the character dead of it, the game's
** end; knocked senseless, it waits it out */
static void __fastcall__ blow (const struct msg* m)
{
    unsigned out = fight_words (m);

    msg (lb);
    if (HUB->pcsum.cond & C_DEAD) {
        end (END_DIED);                         /* (The game's end, as harm's is) */
    } else if (out) {
        intend2 (I_WAIT, out, 1);               /* (Out: the time it's senseless) */
    }
}

static void heard (void)
{
    static struct msg m;
    unsigned dx, dy;
    unsigned char rel;

    while (hub_dnext (&dplace, &m)) {
        if (m.kind == E_FIGHT && (!m.who || !m.a)) {
            blow (&m);
            continue;
        }
        if (m.kind != E_DEN || !m.b || m.x[2] || m.x[0] != HUB->pc_stratum) {
            continue;
        }
        dx = m.w1 - HUB->pc_x;
        dy = m.w2 - HUB->pc_y;
        rel = (bearing ((int) dx, (int) dy) - HUB->pc_face) & 7;
        dx = (int) dx < 0 ? -dx : dx;
        dy = (int) dy < 0 ? -dy : dy;
        dx = (dx > dy ? dx : dy) + 4 * (m.x[3] & 15) + 2 * (m.x[3] >> 4);
        if ((lsn & 1) && ((rel + 1) & 7) < 3) {
            dx >>= 1;                           /* (Listened for, that way: twice as far) */
        }
        if (dx * 2 > m.b * (HUB->pcsum.hearing + 3) || m.t < heard_at + (rel == heard_rel ? 300 : 50)) {
            continue;
        }
        heard_at = m.t;
        heard_rel = rel;
        hub_map (DEN_BANK);                     /* (Its sound's words: the sound task's, if it's put them there) */
        memcpy (heard_what, SND_AT + (m.x[1] % SND_KINDS * 2 + (m.a == DE_NOISE)) * SND_REC, sizeof heard_what);
        hub_map (0);
        heard_what[sizeof heard_what - 1] = 0;
        heard_words (m.a | (m.x[3] ? 0x80 : 0), rel);
        msg (lb);
        if (busy && HUB->pc_task.kind && !asked) {
            asked = 1;                          /* (The first sound heard as a long task goes: asked, the world's to
                                                **   pause it) */
            intend2 (I_ASK, TE_ASK_HEARD, 0);
        }
    }
}

static unsigned char events (void)          /* (How many there were) */
{
    static struct msg m;
    unsigned char k, n = 0;
    struct req* r;

    while (!quit && hub_next (&place, &m)) {
        ++n;
        if (!m.who) {
            HUB->ui_cmd = 0;                    /* (The character's news: a command's answer begun) */
        }
        switch (m.kind) {
        case E_TIME:
            if (m.who == 0) {
                tell (T_TIME_PASSES);
            }
            break;
        case E_READY:
            if (m.who == 0) {
                busy = 0;
                asking = 0;                     /* (A fight's question, its fight over as it asked: let go) */
                heard ();                       /* (What came before it, told before the prompt: a blow's words;
                                                **   knocked senseless by one, busy again; dead of one, the
                                                **   game's end: no prompt, however the tasks' timing falls) */
                if (lsn) {
                    look_way (VB_NONE);         /* (A listen's or a sniff's end: way.c's) */
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
                    k = m.w1 >> 12;             /* (The link's kind: a ladder, or a stairway) */
                    tell ((m.w1 & 0xFF) == A_DOWN ? (k == O_LADDER_DOWN ? T_YOU_CLIMB_DOWN :
                        k == O_SHAFT_DOWN ? T_YOU_CLIMB_DOWN_2 : k & O_FELL ? T_PICK_YOURSELF_UP : T_YOU_GO_DOWN) :
                        k == O_LADDER_UP ? T_YOU_CLIMB_THE : k == O_SHAFT_UP ? T_YOU_CLIMB_UP :
                        T_YOU_CLIMB_THE_2);
                    theme_named (TN_STRATUM, sight_get ()->sname);
                    sayf (" You come into the %s.", tname);
                    save_game ();               /* (Kept as it comes into a stratum) */
                }
                if ((m.w1 & 0xFF) == A_CROSSED) {
                    tell (T_YOU_CROSS_THE);
                } else if ((m.w1 & 0xFF) == A_WENT_TO && !placing) {
                    tell (T_YOU_GO_OVER);
                }
                placing = 0;
                k = m.w1 & 0xFF;               /* (As a run or a sprint ends, briefly: the pace not held) */
                look ((m.w1 >> 8 & 1) | ((k == A_WALKED || k == A_CROSSED || k == A_WENT_TO || k == A_STEPPED) &&
                    HUB->gait > GA_JOG && !(HUB->watch & (WA_TRAPS | WA_LISTEN | WA_MAP)) && !sight_get ()->dark ? 4 :
                    0));
            }
            break;
        case E_VIEW:
            if (m.who == 0) {
                if (m.w1 == V_STIR) {
                    stir ();                    /* (A denizen it sees moved) */
                    break;
                }
                if (m.w1 == V_FACING) {         /* (A turn's first words, as it's begun: out at once, its look
                                                **   after, V_TURNED's) */
                    sayf ("\nYou turn to face %s.", head_name[HUB->pc_face & 7]);
                    t_flush ();
                    break;
                }
                look ((unsigned char) m.w1 & (V_LOOKED | V_TRACED));  /* (A look one way's own words: way.c's;
                                                **   the traces read, words.c's) */
            }
            break;
        case E_NOWAY:
            if (m.who == 0 && (m.w1 & 0xFF) == N_DEN) {
                strcpy (lb, HUB->actor[(m.w1 >> 8) & 7].name);  /* (A denizen's: its name, for overlay 4) */
                den_way ();
            } else if (m.who == 0 && m.w1 == N_SHUT) {
                way_words ();                   /* (The theme's: its lock) */
            } else if (m.who == 0) {
                tell (m.w1 == N_SHUT ? T_THE_WAY_OUT :
                     m.w1 == N_NO_STAIRS ? T_THERE_S_NO : m.w1 == N_NO_DIG ?
                     T_THERE_S_NOTHING : m.w1 == N_CLOSED ? T_THE_PORTCULLIS_IS : m.w1 == N_DOOR ? T_DOOR_SHUT :
                     T_YOU_CAN_T);
            }
            break;
        case E_TASK:                            /* (The long task's: stopped, asked, refused; slice 4.0's) */
            if (m.who == 0) {
                k = task_words (m.w1, m.b);
                if (k & 2) {
                    intend2 (I_GO_ON, 0, 0);    /* (dq0 -a's: on, unasked) */
                } else if ((asking = k & 1) != 0) {
                    t_prompt (1);               /* (Its answer's prompt: the character still busy) */
                }
                if (k & 4) {
                    busy = 1;                   /* (Fighting on begun by the world: busy till it's done, E_READY; a
                                                **   line held taking it over) */
                    t_prompt (0);
                    if (held && !took) {
                        took = 1;
                        intend2 (I_STOP, 1, 0);
                    }
                }
            }
            break;
        case E_BEAT:                            /* (A walk's, as it goes) */
            if (m.who == 0) {
                beat_words (m.w1);
            }
            break;
        case E_DIG:
            if (m.who == 0) {
                k = m.w1 >> 8;
                if (k == D_BEGUN || k == D_AGAIN) {
                    dug_with[0] = 0;
                } else if (k != D_NO_TOOL) {
                    dig_used (m.b);
                }
                dig_words (k, m.w1 & 0xFF);
            }
            break;
        case E_LEDGE:
            if (m.who == 0) {
                k = (signed char) m.w1 > 0 ? (signed char) m.w1 : -(signed char) m.w1;
                climb_words ((signed char) m.w1 > 0 ? CW_RISES : CW_FALLS, k);
            }
            break;
        case E_CLIMB:
            if (m.who == 0) {
                climb_words (m.w1 >> 8, m.w1 & 0xFF);
            }
            break;
        case E_HARM:
            if (m.who == 0) {                   /* (The referee's words of its wounds, then the UI's of it all) */
                r = ask_b (RS_DESCRIBE, 0, 2);
                strcpy (lb, r->text);
                r->state = REQ_FREE;
                harm_words (m.w1, HUB->pcsum.cond);
                if (HUB->pcsum.cond & C_DEAD) {
                    end (END_DIED);             /* (The game's end) */
                }
            }
            break;
        case E_PORTAL:
            if (m.who == 0) {
                if (finds == 0xFF) {            /* (A search's end: the things concealed about it too, as far as the
                                                **   walls were searched, the part of it done: the world's reach) */
                    HUB->ireq.to = m.a;
                    finds = things (IO_SEARCH, "");
                }
                if (!finds || m.w1 != PO_NOTHING) {
                    fixt_words (&m);
                }
                k = m.w1;                       /* (A part lifted out: now taken up) */
                if (lifting[0] && (k == PO_LIFTED || k == PO_PANE_OUT || k == PO_BARS_OUT)) {
                    strcpy (pending, lifting);
                }
                lifting[0] = 0;
                rebar |= k == PO_REBARRED;
                if (finds && (m.w1 == PO_FOUND || m.w1 == PO_NOTHING || m.w1 == PO_FOUND_TRAP)) {
                    msg (tt ());                /* (What its search turned up besides) */
                    finds = 0;
                }
            }
            break;
        case E_TRAP:
            if (m.who == 0) {
                trap_words (m.w1);
            }
            break;
        case E_TAKEN:                           /* (A thing walked over to and taken up: the world's, I_TAKE) */
            if (m.who == 0) {
                took_up (m.w1 | m.w2 << 8);
            }
            break;
        case E_LEFT:                            /* (Up the way out, open: the theme's mission's) */
            if (m.who == 0) {
                end (END_ESCAPED);
            }
            break;
        }
    }
    return n;
}

/* ---- The commands (the parser comes later: a word, and the rest) */

void __fastcall__ intend2 (unsigned char kind, unsigned w1, unsigned w2)
{
    static struct msg m;
    unsigned b = hub_here ();                   /* (An overlay's too: the hub's header put in the window for it, then
                                                **   the overlay back; slice 4.1's, words.c's more) */

    m.kind = kind;
    m.who = 0;
    m.a = 0;
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

/* A portal (or a trap) worked: what's done (PA_*), to what (the parser's thing: a door, a window, or either) */
static void __fastcall__ work (unsigned char what, unsigned char obj)
{
    static struct msg m;

    m.kind = I_PORTAL;
    m.who = 0;
    m.a = what;
    m.b = obj == OB_DOOR || obj == OB_PORTC ? PW_DOOR : obj == OB_WINDOW ? PW_WINDOW : PW_ANY;
    m.w1 = tx;                                  /* (The one meant, if it was found: else 0, the one it faces) */
    m.w2 = ty;
    m.x[0] = barw;
    m.x[1] = strike;
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
}

static void __fastcall__ intend (unsigned char kind, unsigned char a)
{
    static struct msg m;

    m.kind = kind;
    m.who = 0;
    m.a = a;
    m.w1 = m.w2 = 0;
    m.x[1] = strike;                            /* (I_ATTACK's: what it strikes with) */
    if (kind == I_GO) {                         /* (Through the one meant, if it was found) */
        m.w1 = tx;
        m.w2 = ty;
    }
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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

/* ---- The things (items.h): a request to the world's item store, waited for: its result */
unsigned char __fastcall__ things (unsigned char op, const char* name)
{
    struct ireq* q = &HUB->ireq;

    q->kind = op;
    strncpy (q->name, name, sizeof q->name - 1);
    q->name[sizeof q->name - 1] = 0;
    hub_call (q, T_WORLD);                      /* (Asleep till the world rings with the reply) */
    q->state = REQ_FREE;
    return q->result;
}

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
static void to_hand (void)
{
    things (IO_TOOLS, "");
    strcpy (HUB->slot[SLOT_UI].text, tt ());
    HUB->slot[SLOT_UI].w1 = HUB->ireq.x;
    referee (RS_EQUIP, 2)->state = REQ_FREE;
}

/* The tool a dig's spell took (the referee's choice): worn by it, and named as it's first taken up */
char dig_with[16];
static void __fastcall__ dig_used (unsigned char n)
{
    HUB->ireq.to = HUB->dig_wear[n];
    things (IO_WEAR, HUB->dig_tool[n]);
    if (strcmp (dug_with, HUB->ireq.name)) {
        strcpy (dug_with, HUB->ireq.name);
        strcpy (dig_with, dug_with);
    }
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* After a thing's handled (taken up, put down ...): its light changed (lit, put out, a torch put down), the world's
** look about in the new light; else the time it took (t tenths), quietly */
static void __fastcall__ after (unsigned char was, unsigned t, unsigned char op)
{
    to_hand ();
    if (op == IO_LIGHT || op == IO_DOUSE || HUB->pc_light != was) {
        intend (I_LIGHT, op == IO_LIGHT || HUB->pc_light > was);
    } else {
        intend2 (I_WAIT, t, 1);
    }
}

/* A thing handled (held, stowed, put in or on another, taken from it, worn, taken off, lit, put out), by its name
** (and the other's: y, on 1 for on it): the things' words of it said */
static void __fastcall__ handle (unsigned char op, const char* x, const char* y, unsigned char on)
{
    unsigned char was = HUB->pc_light;

    strcpy (HUB->ireq.into, y);
    HUB->ireq.to = on;
    things (op, x);
    if (HUB->ireq.result == IR_NOT_HERE && reach_go ()) {
        return;                                 /* (Out of reach: walked over to it first, then again) */
    }
    msg (tt ());
    if (HUB->ireq.result == IR_OK) {
        after (was, HUB->ireq.x, op);
    }
}

/* A portal's part, fixed in it (a door's bar, a window's pane or its bars: IR_FIXED, its square in the request's x,
** y): the portal at it or beside it faced, and the part lifted out (the maker's: a bar only from its side), then taken
** up (E_PORTAL's) */
static void __fastcall__ lift (const char* name)
{
    struct sight* s = sight_get ();
    unsigned char i, d;

    for (i = 0; i < s->ndoors; ++i) {
        if (!(s->door[i].rel & MOUTH) && s->door[i].x - HUB->ireq.x + 1 <= 2 && s->door[i].y - HUB->ireq.y + 1 <= 2) {
            d = bearing ((int) s->door[i].x - HUB->pc_x, (int) s->door[i].y - HUB->pc_y);
            if (d != HUB->pc_face) {
                intend (I_TURN, d);
            }
            strcpy (lifting, name);
            tx = s->door[i].x;                  /* (That one) */
            ty = s->door[i].y;
            work (PA_LIFT, s->door[i].kind == PK_WINDOW ? OB_WINDOW : OB_DOOR);
            return;
        }
    }
    tell (T_YOU_SEE_NONE);
}

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
        intend2 (I_WAIT, HUB->round, 1);
        return;
    }
    if ((unsigned char) r == IR_OK && *tt ()) {
        msg (lb);                               /* (By the count: "You take 4 rations.", the world's) */
    } else if ((unsigned char) r != IR_NOT_HERE) {  /* (Out of reach still: the walk's words told why) */
        tell ((unsigned char) r == IR_OK ? T_TAKEN : (unsigned char) r == IR_FULL ? T_YOUR_HANDS_ARE : T_YOU_SEE_NONE);
    }
    if ((unsigned char) r == IR_OK) {
        after (r >> 8, 20, IO_GET);
        taken_next (pending);                   /* ("Take all": the next, as it's free) */
    }
}

/* A thing taken up, by its name (out of reach, the world walks over to it first, and takes it up: I_TAKE; a portal's
** part lifted out first; one fixed where it stands, the things' words) */
void __fastcall__ s_take (const char* name)
{
    static struct msg m;
    unsigned char was = HUB->pc_light, r = things (IO_GET, name);

    strcpy (taking, name);
    pending[0] = 0;
    if (r == IR_NOT_HERE) {
        m.kind = I_TAKE;
        m.w1 = HUB->ireq.x;
        m.w2 = HUB->ireq.y;
        m.t = HUB->ireq.item;
        if (hub_send (&m)) {
            busy = 1;
            t_prompt (0);
        }
    } else if (r == IR_FIXED) {
        lift (taking);
    } else if (r == IR_NO) {
        msg (tt ());                            /* (A theme's altar, its throne: fixed where it stands) */
    } else {
        s_took_up (r | was << 8);
    }
}


/* The map, and beside it the referee's short lines (the attributes as they are now) */
static void show_map (void)
{
    struct req* r = ask_b (RS_DESCRIBE, 0, 1);

    map (r->text);                              /* (Its lines read where they are, in the hub's header, before the
                                                **   map's drawn) */
    r->state = REQ_FREE;
}

/* A thing it sees, the parser's (its kind, | PICK_TELL to say so if there's none; and the heading it lies, if one
** was said): 1, its square in tx, ty; 0, none; 2, several and none ahead, which asked (overlay 7's pick) */
unsigned tx, ty;
#define find(obj, orel) pick ((obj), (orel))

/* A thing hidden where the character stands (P_CONCEAL: the referee's test, the things' record), worded by how
** well (its difficulty, in steps) */
static void __fastcall__ hide (const char* w)
{
    unsigned char d;

    if (!*w) {
        tell (T_HIDE_WHAT_HIDING);
    } else if ((HUB->ireq.to = reach == 4 ? IO_DONE : 0, things (IO_HIDE, w)) == IR_OK) {
        if (reach != 4) {
            more (VB_HIDE);                     /* (Begun: a minute's work, a timed work, the things' IO_DONE at its
                                                **   end, words.c's) */
            return;
        }
        d = HUB->ireq.qty;
        tell (d >= 10 ? T_HID_CUNNINGLY : d >= 4 ? T_HID_WELL : T_HID_POORLY);
        to_hand ();
    } else {
        tell (T_YOU_CARRY_NONE);
    }
}

/* The character's way with a denizen (how: hub.h's SW_*, the command's verb's: commands.verbs' aux; its things, the
** one meant and a name or a thing, and its request): asked of the denizens' task (the hub's sway: it lends the UI's
** slot to the referee meanwhile), waited for, worded (overlay 6: but none found, if SW_QUIET), and the time it took
** the character's.  Its result */
#define SW_QUIET    0x80
#define dealt       (*(struct sway*) (lb + sizeof lb - sizeof (struct sway)))  /* (Its copy at lb's end: the
                                                **   words, a line or two, go at its start) */
static unsigned char __fastcall__ deal (unsigned char how)
{
    struct sway* s = &HUB->sway;

    s->kind = how & ~SW_QUIET;
    strcpy (s->word, cmd.obj.word);
    s->prep = cmd.prep;
    strcpy (s->word2, cmd.other.word);
    s->order = cmd.order;
    hub_call (s, T_DENIZENS);
    memcpy (&dealt, s, sizeof dealt);
    s->state = REQ_FREE;
    if (!(how & SW_QUIET) || dealt.result != SR_NONE) {
        sway_words (&dealt);
        msg (lb);
    }
    if (dealt.t) {
        intend2 (I_WAIT, dealt.t, 0);
    }
    return dealt.result;
}

/* The commands that want nothing but their action: 1 if it was one of them, carried out */
static unsigned char __fastcall__ chores (unsigned char action)
{
    switch (action) {
    case VB_OBSERVE_DIRECTION:                  /* (Looked, listened, sniffed one way, or at a thing, through it: */
    case VB_OBSERVE_OBJECT:                     /*   way.c's, part 6; */
    case VB_OBSERVE_THROUGH:
    case VB_LISTEN:
    case VB_SMELL:
    case VB_TRACK:                              /*   the traces read, slice 4.1's: the floor looked at, a track) */
        look_way (action);
        break;
    case VB_OBSERVE_LOCALE:
    case VB_OBSERVE_AROUND:
    case VB_FIND_OBJECT:
        look (0);
        break;
    case VB_META_MAP:
        show_map ();
        break;
    case VB_WAIT:                               /* (A round, or as long as it says: "wait 10 minutes"; engaged, */
    case VB_PASS:                               /*   "pass": a round given up, defending) */
        intend2 (I_WAIT, wait_of (cl, cmd.n), 0);
        break;
    case VB_DISENGAGE:                          /* (Away from the foes engaging it, the book's disengage; a flight */
    case VB_FLEE:                               /*   at a run once free: the world's) */
        intend2 (I_DISENGAGE, cmd.dir, action == VB_FLEE);
        break;
    case VB_STOP:                               /* (The long task: "stop", "stop digging") */
        intend (I_STOP, stop_kind (cmd.obj.word));
        break;
    case VB_META_SHEET:
        sheet ();
        break;
    case VB_META_PAUSE:
    case VB_META_SAVE:
        meta (action);                          /* (Paused, or kept: overlay 3's) */
        break;
    case VB_META_HELP:
        help ();
        break;
    case VB_META_QUIT:
        quit = 1;
        break;
    default:
        return more (action);                   /* (The rest: overlay 2's, slice 4.1's on: the light's, the traces') */
    }
    return 1;
}

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
    HUB->cq.face = HUB->pc_face;
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

/* The command line l makes (the command parser's: the line posted, if it wasn't), into cmd: its ways as the
** character faces made the compass's as it faces now (dq0c's CQ_REL | n), a half turn an eighth ("turn half
** left").  Its verdict */
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
    if (cmd.dir != 0xFF && (cmd.dir & CQ_REL)) {
        cmd.dir = (f + cmd.dir) & 7;
    }
    if (cmd.obj.lies != 0xFF && (cmd.obj.lies & CQ_REL)) {
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
** request, a door's work, the denizens' sway: commands.verbs') */
void __fastcall__ s_command (char* l)
{
    struct req* r;
    unsigned char d, pa;
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
    ok = s_cq_cmd (l);                          /* (The command the parser made: dq0c's, the grammar's verdict with
                                                **   it) */
    pc_face = HUB->pc_face;
    d = choose (&cmd);                          /* (A choice asked for: this line's, the command again; a fixture's, */
    d |= answer (d);                            /*   or a thing's) */
    tx = 0;
    if (!d && (cmd.flags & CF_STRANGE) && *l != '#') {   /* (A verb the game doesn't know: the grammar's, not done
                                                **   yet ("grapple"); or none of its ("bribe") */
        unknown (ok);
        return;
    }
    if ((cmd.obj.word[0] || cmd.n) && (cmd.action == VB_MOVE_TOWARD || cmd.action == VB_MOVE_DIRECTION) &&
        cmd.dir == 0xFF && !cmd.obj.kind) {     /* (A word alone, or a number, that's no way: "foo", "1") */
        sayf ("\nI don't know how to %s.", cmd.obj.word[0] ? cmd.obj.word : l);
        return;
    }
    if (!d && !ok) {                            /* (Words the game knows, but not a command the grammar has) */
        sayf ("\nI don't understand that.");
        return;
    }
    if (resolve () != RV_ON) {                  /* (Its things found, its unsaid filled; or a question asked) */
        return;
    }
again:
    HUB->ireq.sel = cmd.obj.sel | cmd.obj.n << 4;  /* (Which of several, how many: the things' requests', the */
    HUB->ireq.osel = cmd.other.sel | cmd.other.n << 4;  /*   world's to choose, slice 4.1's: "the second sack") */
    HUB->ireq.count = cmd.flags & CF_HALF ? CT_HALF : cmd.obj.sel == SL_ALL ? CT_ALL : cmd.n;
    if ((d = puzzle (&cmd)) != Q_NONE) {        /* (The theme's puzzle's: theme.c) */
        if (d >= Q_DONE) {
            intend2 (I_WAIT, 50, 1);            /* (Its time: the way out opened the world's already) */
        }
        if (d == Q_RULED) {
            end (END_RULED);
        }
        return;
    }
    switch (cmd.action) {
    case VB_NONE:
        return;
    case VB_MOVE_DIRECTION:
    case VB_MOVE_TOWARD:
    case VB_ENTER:
    case VB_APPROACH_OBJECT:
        d = cmd.obj.kind;
        if ((d == OB_WALL || (d >= OB_ROOM && d != OB_PILLAR)) && place_go (&cmd)) {
            return;                             /* (A place in the room: the centre, a side, a corner: part 7's) */
        }
        if (cmd.obj.kind && cmd.action != VB_MOVE_DIRECTION) {   /* (A thing: up to it, or through it) */
            if (find (cmd.obj.kind | PICK_TELL, cmd.obj.lies) != 1) {
                return;                         /* (None, told; or which, asked) */
            }
            if (cmd.obj.kind == OB_WINDOW && (cmd.how == HOW_THROUGH || cmd.action == VB_ENTER)) {
                pa = PA_THROUGH;                /* (Climbed through) */
                goto portal;
            } else if ((cmd.how == HOW_THROUGH || cmd.action == VB_ENTER) &&
                (cmd.obj.kind == OB_DOOR || cmd.obj.kind == OB_PASSAGE)) {
                intend (I_GO, bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y));
            } else {
                intend2 (I_GOTO, tx, ty);
            }
        } else if (cmd.dir == GO_UP || cmd.dir == GO_DOWN) {
            intend (I_CLIMB, cmd.dir == GO_UP ? DIR_UP : DIR_DOWN);    /* (Up or down: the stairs, a ladder) */
        } else if (cmd.dir != 0xFF) {
            intend (I_GO, cmd.dir);
        } else {
            tell (T_GO_WHICH_WAY);
        }
        return;
    case VB_CLIMB:
        if (cmd.obj.kind == OB_WINDOW || cmd.how == HOW_THROUGH) {
            cmd.obj.kind = OB_WINDOW;           /* (Through a window) */
            pa = PA_THROUGH;
            goto portal;
        }
        intend (I_CLIMB, cmd.dir == GO_DOWN || (cmd.dir == 0xFF && cmd.obj.lies == GO_DOWN) ? DIR_DOWN : DIR_UP);
                                                /* ("climb down the ladder": the way where the thing lies) */
        return;
    case VB_FACE_DIRECTION:
        if (cmd.dir > H_NW) {
            tell (T_WHICH_WAY_LEFT);
        } else if (cmd.dir == HUB->pc_face) {
            sayf ("\nYou face %s already.", head_name[cmd.dir]);
        } else {
            intend (I_TURN, cmd.dir);
        }
        return;
    case VB_OPEN_OBJECT:                        /* (A door's work: the verb's, PA_*) */
    case VB_CLOSE_OBJECT:
    case VB_UNLOCK_OBJECT:
    case VB_FORCE_OBJECT:
    case VB_RAISE_OBJECT:
    case VB_LOWER_OBJECT:
    case VB_BREAK_OBJECT:
    case VB_DISARM_OBJECT:
    case VB_BAR_OBJECT:
    case VB_BEND_OBJECT:
        if (!cmd.obj.kind && more (cmd.action)) {
            return;                             /* (A thing that closes: a chest, a sack, a flask: overlay 2's) */
        }
        pa = cmd.aux;
        if (pa == PA_RAISE && cmd.obj.word[0]) {
            pa = PA_LIFT;                       /* ("Lift the bar": a portal's part lifted out) */
        } else if (pa == PA_RAISE || pa == PA_LOWER) {
            cmd.obj.kind = OB_PORTC;
        } else if (cmd.obj.kind == OB_WINDOW && !strcmp (cmd.obj.word, "bars") &&
            !find (OB_WINDOW | PICK_ANY, cmd.obj.lies)) {
            cmd.obj.kind = OB_PORTC;            /* ("Bend the bars": a portcullis's, with no window there) */
        }
        strike = SK_BEST;                       /* (Broken with the best to hand) */
portal:
        if (cmd.obj.kind && (d = find (cmd.obj.kind, cmd.obj.lies)) == 2) {
            return;                             /* (Which, asked) */
        }
        if (cmd.obj.kind && d) {
            cmd.obj.lies = bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y);    /* (The one it sees: faced) */
        }
        if (cmd.obj.lies <= H_NW && cmd.obj.lies != HUB->pc_face) {
            intend (I_TURN, cmd.obj.lies);      /* (The door that way: faced first) */
        }
        HUB->ireq.to = 0;
        barw = pa == PA_BAR && things (IO_BAR, "") == IR_NO ? HUB->ireq.x : 0;    /* (The widest bar to hand) */
        work (pa, cmd.obj.kind);
        return;
    case VB_EXAMINE_OBJECT:                     /* (A thing looked at closely: judged, by the craft that knows it; a */
    case VB_EXAMINE_LOCALE:                     /*   portal, or its part if it's not there; else the room) */
        if (cmd.obj.word[0] && !cmd.obj.kind && deal (SW_LOOK | SW_QUIET) != SR_NONE) {
            return;                             /* (A denizen it sees: how it seems) */
        }
        if ((pa = look_way (VB_EXAMINE_OBJECT)) != 0) {
            goto portal;                        /* (A thing, a portal, the room, its aspects: part 6's, way.c; a door
                                                **   gone over for traps, the world's) */
        }
        return;
    case VB_SEARCH:
        finds = 0xFF;                           /* (A search, a long task: what it finds told as it ends, the walls'
                                                **   and the things concealed about it, E_PORTAL's) */
        msg ("Searching about...");
        intend (I_PORTAL, PA_SEARCH);
        return;
    case VB_DIG:
        d = cmd.dir;
        if (d == GO_DOWN) {
            intend (I_DIG, DIR_DOWN);
        } else if (d <= H_NW && !(d & 1)) {
            intend (I_DIG, d >> 1);
        } else {
            tell (T_DIG_WHICH_WAY);
        }
        return;
    case VB_RETRIEVE_OBJECT:
        if (cmd.prep == PR_FROM) {
            handle (IO_TAKE, cmd.obj.word, cmd.other.word, 0);
        } else if (!cmd.obj.word[0]) {
            tell (T_TAKE_WHAT);
        } else {
            take (cmd.obj.word);
        }
        return;
    case VB_DROP_OBJECT:
        if (!cmd.obj.word[0]) {
            tell (T_PUT_DOWN_WHAT);
        } else if (d = HUB->pc_light, (pa = things (IO_DROP, cmd.obj.word)) == IR_OK) {
            if (*tt ()) {
                msg (lb);                       /* (By the count: "You put down two sacks.", the world's) */
            } else {
                tell (T_PUT_DOWN);
            }
            after (d, 10, IO_DROP);
        } else if (pa == IR_NO) {
            msg (tt ());                        /* (Why not: in a thing closed) */
        } else {
            tell (T_YOU_CARRY_NONE);
        }
        return;
    case VB_HIDE:                               /* (P_CONCEAL: a thing; oneself comes with the denizens) */
        hide (cmd.obj.word);
        return;
    case VB_HOLD_OBJECT:                        /* (A thing handled: the things' request the verb's, IO_*) */
    case VB_EQUIP_OBJECT:
    case VB_UNEQUIP_OBJECT:
    case VB_PUT_OBJECT:
    case VB_HANG_OBJECT:
    case VB_LIGHT_OBJECT:
    case VB_EXTINGUISH_OBJECT:
        handle (cmd.aux, cmd.obj.word, cmd.other.word, cmd.prep == PR_ON);
        return;
    case VB_THROW_OBJECT:                       /* (Thrown at the thing said, or the target: the world's throw, its
                                                **   words overlay 11's) */
        d = HUB->pc_light;
        if (cmd.other.x) {
            HUB->ireq.x = cmd.other.x;
            HUB->ireq.y = cmd.other.y;
            HUB->ireq.to = cmd.manner;
            pa = things (IO_THROW, cmd.obj.word);
        } else {
            pa = cmd.other.word[0] ? IR_NOT_HERE : IR_FULL;   /* (No mark it sees; none said) */
        }
        throw_words (pa | HUB->ireq.qty << 8);
        if (pa == IR_OK) {
            after (d, 20, IO_DROP);             /* (Its time; a light thrown, the world's look in the new light) */
        }
        return;
    case VB_META_INVENTORY:
        things (IO_LIST, "");
        msg (tt ());
        if ((d = HUB->pcsum.load) != 0) {
            tell (T_BURDENED - 1 + d);          /* (Its load, as the referee has it: its encumbrance) */
        }
        return;
    case VB_ATTACK_OBJECT:                      /* (A blow at a denizen beside it, overlay 6's choice; or the stance */
    case VB_DEFEND:                             /*   set, the referee's defence for the character) */
        if ((d = fight_target (&cmd)) == FT_THING) {
            pa = PA_BREAK;                      /* (A blow at a thing: broken by it) */
            goto portal;
        }
        if (d) {
            intend (d & FT_CLOSE ? I_CLOSE : I_ATTACK, d & 0x0F);   /* (Out of reach: closed with first) */
            return;
        }
        msg (lb);
        return;
    case VB_JUMP:
        if (cmd.dir == GO_DOWN) {
            intend (I_CLIMB, DIR_DOWN | I_JUMP);    /* (Down a ledge, or a shaft: a fall) */
        } else {
            tell (T_YOU_JUMP_AND);
        }
        return;
    case VB_TELL_OBJECT:                        /* (A denizen swayed, an ally asked, a name given: the denizens' */
    case VB_PERSUADE:                           /*   sway the verb's, SW_*) */
    case VB_DECEIVE:
    case VB_FAST_TALK:
    case VB_THREATEN:
    case VB_INTIMIDATE:
    case VB_CALM:
    case VB_BEFRIEND:
    case VB_ORDER:
    case VB_ALLY_COME:
    case VB_ALLY_STAY:
    case VB_ALLY_DISMISS:
    case VB_NAME_OBJECT:
    case VB_NAME_PLACE:
        deal (cmd.aux);
        return;
    default:                                    /* (The rest that want nothing but their action; and those the game
                                                **   doesn't do yet: its verb, as typed) */
        if (!chores (cmd.action)) {
            unknown (1);
        }
        return;
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* The game's arguments (s_begin's: part 7's, part 1 full; the defaults' words in the main RAM) */
static unsigned long seed;                      /* (Its seed: the clock's, as the task starts, but for -s's) */
static const char* file = "";                   /* (-c's: the character's file) */
static const char* rules = "harn";              /* (The ruleset, if none's given: HarnMaster's) */

#pragma code-name (push, "SW7CODE")
#pragma rodata-name (push, "SW7RODATA")

/* A command whose thing is out of reach (the things' IR_NOT_HERE, the puzzle's PZ_FAR: where it is in the hub's ireq,
** its words not said): walked over to it first (I_TAKE's walk alone), the command kept and carried out again as the
** character's free (HOUSE.md's out of reach, walked to first); 1 if so, or if it's the command again and the thing's
** out of reach still (told) */
static struct msg pm;                           /* (reach_go's, place_go's) */
unsigned char s_reach_go (void)
{
    unsigned b = hub_here ();
    unsigned char r = 1;

    if (reach) {
        msg ("You can't get any nearer to it.");
    } else {
        hub_map (UI_BANK);
        memcpy (&UIREC->kept, &cmd, sizeof cmd);
        hub_map (0);
        pm.kind = I_TAKE;
        pm.who = 0;
        pm.a = 1;
        pm.w1 = HUB->ireq.x;
        pm.w2 = HUB->ireq.y;
        pm.t = HUB->ireq.item;
        if ((r = hub_send (&pm)) != 0) {
            busy = 1;
            reach = 1;
            t_prompt (0);
        }
    }
    hub_back (b);
    return r;
}

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
        k = strstr (w, "corner") ? OB_CORNER : strstr (w, "side") || strstr (w, "end") || strstr (w, "wall") ? OB_SIDE :
            strstr (w, "cent") || strstr (w, "middle") ? OB_CENTRE : OB_ROOM;
    }
    if (k != OB_CENTRE && k != OB_SIDE && k != OB_CORNER && k != OB_WALL) {
        return 0;
    }
    s = sight_get ();
    if (s->where != W_ROOM || s->dark) {
        if (k == OB_CORNER || k == OB_WALL) {
            return 0;                           /* (A passage's corner, a wall: as ever) */
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
    fc = a == 2 ? fc : !a ? (f0 + f1) / 2 : a * fa > 0 ? f1 : f0;
    rc = b == 2 ? rc : !b ? (r0 + r1) / 2 : b * ra > 0 ? r1 : r0;
    pm.w1 = fy ? rc : fc;
    pm.w2 = fy ? fc : rc;
    if (pm.w1 == px && pm.w2 == py) {
        msg ("You're there already.");
        return 1;
    }
    pm.kind = I_GOTO;
    pm.who = 0;
    pm.a = 1;
    if (hub_send (&pm)) {
        busy = 1;
        placing = 1;
        t_prompt (0);
    }
    return 1;
}

void __fastcall__ s_args (int argc, char* argv[])
{
    unsigned char i;
    char* l;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-s") && i + 1 < argc) {
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
        } else if (!strcmp (argv[i], "-a")) {
            autogo = 1;                         /* (For the tests: a long task goes on at every soft question) */
        } else if (!strcmp (argv[i], "-I")) {
            noinit = 1;                         /* (For the tests: no engage initiative test, no grope's) */
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
            hy_exits ("usage: dq0 [-s seed] [-p r|n|m|t] [-c file] [-d folder] [-t theme] [-k things] [-r rules] [-T [-T]] [-C] [-a] [-I]");
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The game begun: its arguments, a game kept carried on, the title, the character, the world started */
void __fastcall__ s_begin (int argc, char* argv[])
{
    args (argc, argv);
    if (savedir && !load_save ()) {             /* (A game kept there: carried on) */
        resumed = 1;
        seed = sv.seed;
        Q = sv.q;
        if (pace == 0xFF) {
            pace = sv.pace;
        }
        file = save_path ("pc.hm");
    } else if (savedir) {
        mkdir (savedir, 0755);                  /* (A new game: its folder) */
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
    made = history (seed);                      /* (The game's own theme, or a stand-in's: which) */
    start_world (seed, file);                   /* (The character with it: its file, or the ruleset's generator) */
    status ();
    story (resumed);                            /* (The theme's) */
    blank ();
    entered (resumed);
    blank ();
    tell (T_TYPE_HELP_FOR);
    t_done ();
}

/* The game's end: kept (as it ends: the game's quit, not lost), the world stopped */
void s_finish (void)
{
    save_game ();
    stop_world ();
    t_end ();
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

    seed = hy_time () ^ hy_ticks ();
    find_dirs ();                               /* (The overlays read in, and the swapped parts with them, first) */
    ovl_load ();
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
                t_key (k);
            }
        }
        k = events ();
        heard ();
        now = hub_now ();
        if ((k && !HUB->ui_cmd) || ((now >= next || now < next - 600 || (HUB->pc_task.kind && HUB->pc_task.done != bar_done)) &&
            hy_ticks () - shown >= (inn ? 1000 : 200))) {
            next = (now / 600 + 1) * 600;       /* (The status line as the events change it, and its clock as a
                                                **   game minute goes, a real second apart at most, five while a
                                                **   line's being typed: only what's changed drawn, t_status; the
                                                **   minute's end kept, not divided for each time round; not as
                                                **   events come while a command's in hand: its answer first) */
            shown = hy_ticks ();
            status ();
        }
        if (!busy && rebar) {                   /* (A door barred: its bar set in its brackets, on the square the
                                                **   character stands on, beside it; what it carries anew) */
            rebar = 0;
            HUB->ireq.to = 1;
            HUB->ireq.x = HUB->pc_x;
            HUB->ireq.y = HUB->pc_y;
            HUB->ireq.item = HUB->pc_stratum;
            things (IO_BAR, "");
            to_hand ();
        }
        if (!busy && pending[0]) {              /* (Lifted out, or come to: now taken up) */
            take (pending);
            pending[0] = 0;
        }
        if (!busy && (reach & 1)) {
            ++reach;                            /* (Walked over to its thing, or its timed work's time up: the command
                                                **   kept carried out again, its end; till the next's, s_command's) */
            command (0);
        }
        if ((l = (char*) t_next ()) != 0 && (!busy || !held)) {
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
                if ((took = k == 2) != 0) {
                    intend2 (I_STOP, 1, 0);     /* (A fight taken over: its round out, then the command) */
                }
                HUB->ui_cmd = 0;                /* (Held: the next waits for the character, not parsed again) */
                l = 0;
            }
        } else {
            l = 0;
        }
        if (l) {
            f = names_in (l = t_line ());       /* (The line's first look, in the overlays' banks: a part's
                                                **   copied in only for a line that may be its; an ordinary
                                                **   command's only part 2, the commands') */
            k = f & NI_LINE ? names_line (l) : NL_NONE;     /* (A name given or forgotten: the names') */
            if (k == NL_NONE && pacing (l) && pace_cmd (l)) {
                k = NL_DONE;                    /* (The pace's, the state's, a small move's: pace.c's) */
            }
            if (k == NL_NONE && names_n && (f & NI_THING) && (t = names_subst (l)) != 0) {
                to_hand ();                     /* (The things' names in it made theirs, "#" and the id; a weapon
                                                **   named taken in hand to strike with: its time, as after ()'s) */
                intend2 (I_WAIT, t, 1);
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
            HUB->ui_cmd = busy;                 /* (Its answer the world's, not yet said: the look and the minds
                                                **   give way till the character's news comes, events') */
        } else {
            ready ();                           /* (The commands' part back, if another's in) */
        }
        t_done ();
        hub_nap (quick ? (--quick, 2) : 4);     /* (A fiftieth of a second: the keys looked for, the world's
                                                **   events told; each look at the keys a call of the console's,
                                                **   some 9,000 cycles with its own, a quarter of the CPU at two
                                                **   ticks while nothing happened; a hundredth just after a command,
                                                **   its answer told the sooner; and woken at once by the world's
                                                **   events, its doorbell rung with them) */
    }
    finish ();
    return 0;
}
