/*
** main.c - Dingus Quest Zero: the UI's task, its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file] [-d folder] [-k things] [-T [-T]]: -d keeps the game in a folder (on any card), and
** carries on the game kept there if there is one; without it nothing's kept. the seed makes the same game each time (the dice: dice.h's rseed); the pace is
** r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (hmchar's).  Without one, hmchar is run to make
** one (in this window, the screen as it is outside the game while it runs), into /ram/dq0.hm.
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
** from outside its part only through its stub (bank.s's SWFN: begin, finish, save_game; command, take).
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
unsigned char pc_face;                          /* (The character's facing, copied for the parser's overlay) */
unsigned char pace = 0xFF, paused;
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

/* Real ticks (200 a second) a combat round takes, by pace: 5, 3 and 2 real seconds, whatever the ruleset's round
** (the turns pace: 0) */
static const unsigned tpr[4] = { 1000, 600, 400, 0 };

static unsigned char busy = 1;                  /* The character's doing something: no command till it's free (nor
                                                **   till the world's started, and says so: E_READY) */
static unsigned place;                          /* Where this task is in the world's events */
static int task_[NTASKS];                       /* Each task's number, as it was started (0: not) */
static unsigned char test_start;
static const char* kit;                         /* (dq0 -k: things more, as the game starts: a tool, ...) */
static const char* savedir;                     /* (dq0 -d: the save's folder) */
static unsigned char __fastcall__ things (unsigned char op, const char* name);
static void to_hand (void);
static void __fastcall__ dig_used (unsigned char n);
static char dug_with[16];                       /* (The tool a dig took, named) */
static unsigned char finds;                     /* (Things a search turned up, told as it ends: the hub's text) */
static struct ireq judged;                      /* (A thing looked at closely: what it is, for the appraiser) */
static struct portal viewed;                    /* (A portal looked at closely: the maker's record of it) */
static char pending[16];                        /* A thing to take up, walked over to first (or lifted out) */
static char lifting[16];                        /* (A portal's part being lifted out, to take up after) */
static unsigned char barw, rebar;               /* (A bar to hand to bar a door with, its width; and the door barred:
                                                **   the bar to set in its brackets, as the character's free) */

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

static void find_dirs (void)
{
    const char* prog = getenv ("prog");
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

/* ---- The character: hmchar's, its file read by the referee */

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

/* hmchar run to make one (the console cooked for it, the screen whole), then its file read */
static char pcfile[64];

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

static void __fastcall__ character (const char* file, unsigned long seed)
{
    static char s[32] = "-o\0/ram/dq0.hm\0-s", s2[48];
    int task;

    if (!file) {
        file = s + 3;                           /* (hmchar -o /ram/dq0.hm -s seed) */
        t_end ();
        consctl ("rawoff");
        ultoa (seed, s + 18, 10);
        s[19 + strlen (s + 18)] = 0;
        strcpy (s2, gamedir);
        strcat (s2, "/hmchar");
        task = hub_run (s2, s);
        if (task < 0 || hub_wait (task)) {
            hy_exits ("dq0: hmchar didn't make a character");
        }
        consctl ("rawon");
        t_init ();
    }
    strcpy (pcfile, file);
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
static struct req* __fastcall__ ask_b (unsigned char kind, unsigned char a, unsigned char b)
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

/* The character's file read by the referee: its name and its calling */
static void load_pc (void)
{
    struct req* r;
    char* at;
    char* l;

    strcpy (HUB->slot[SLOT_UI].text, pcfile);
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
** (dq0p's), the referee's pc.hm
** and the maker's changes.dat and seen.dat */

#define SAVE_MAGIC  0x5144                      /* "DQ" */
#define SAVE_VER    5                           /* (2: the items' records with their size, the portals' make; 3, the
                                                **   theme and its puzzle; 4, the denizens' attitudes, the factions',
                                                **   the names given; 5, the creatures' sheets) */
static struct save {
    unsigned        magic;
    unsigned char   version, pace;
    unsigned long   seed, now;
    unsigned char   stratum, face, light;
    unsigned        x, y;
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

/* The game kept: the referee's sheet, the maker's records, and the game's state (the hub's header in the window) */
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
    q = &HUB->mreq[ASK_UI];                     /* (The maker's records: asked, and waited for) */
    q->kind = M_SAVE;
    q->state = REQ_POSTED;
    hub_ring (T_MAKER);
    while (q->state != REQ_DONE) {
        hy_yield ();
        hub_map (0);
    }
    q->state = REQ_FREE;
    things (IO_SAVE, "");                       /* (The things': items.dat) */
    if (task_[T_DENIZENS] > 0) {                /* (The denizens': dens.dat) */
        HUB->den_state = REQ_POSTED;
        hub_ring (T_DENIZENS);
        while (HUB->den_state != REQ_DONE) {
            hy_yield ();
            hub_map (0);
        }
        HUB->den_state = REQ_FREE;
    }
    sv.magic = SAVE_MAGIC;
    sv.version = SAVE_VER;
    sv.pace = pace;
    sv.seed = HUB->seed;
    sv.now = hub_now ();
    sv.stratum = HUB->pc_stratum;
    sv.x = HUB->pc_x;
    sv.y = HUB->pc_y;
    sv.face = HUB->pc_face;
    sv.light = HUB->pc_light;
    sv.pc = P;
    sv.q = Q;
    if ((fd = open (save_path ("game.dat"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, &sv, sizeof sv);
        write (fd, HUB->strata, sizeof HUB->strata);
        close (fd);
    }
}

/* The saved strata's records into the hub (after the save's header) */
static void __fastcall__ load_strata (void)
{
    int fd = open (save_path ("game.dat"), O_RDONLY);

    if (fd >= 0) {
        read (fd, &sv, sizeof sv);
        read (fd, HUB->strata, sizeof HUB->strata);
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
    unsigned char b = getbank ();

    hub_map (bank);
    memcpy (to, from, n);
    setbank (b);
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

static void start_world (unsigned long seed)
{
    struct req* r;
    unsigned char k;

    if (hub_create (HUB_BANKS)) {
        t_end ();
        hy_exits ("dq0: no shared segment or semaphores for its tasks");
    }
    HUB->round = ROUND;                         /* (The ruleset's: HarnMaster's 10 seconds, till the referee says) */
    HUB->tpr = tpr[pace];
    HUB->pace = pace;
    HUB->seed = seed;
    HUB->test_start = test_start;
    HUB->way_open = Q.open;
    hub_map (DEN_BANK);                         /* (The sounds' words: none till the sound task's put them there) */
    memset (SND_AT, 0, (2 * SND_KINDS + SND_WORLD) * SND_REC);
    hub_map (0);
    start ("dq0s", T_SOUND, libdir);            /* (Its sound: SOUND.md) */
    start ("dq0harn", T_REFEREE, libdir);       /* (Its rules: its folder, for its overlay) */
    r = referee (RS_HELLO, 0);
    HUB->round = r->w1;                         /* (The ruleset's round, in tenths) */
    r->state = REQ_FREE;
    load_pc ();                                 /* (Its senses the referee's, before the world looks through them) */
    if (savedir) {
        strncpy (HUB->savedir, savedir, sizeof HUB->savedir - 1);
    }
    if (resumed) {                              /* (Carried on: where it was, and when, the strata as they were) */
        load_strata ();
        HUB->resume = 1;
        HUB->now = sv.now;
        HUB->pc_stratum = sv.stratum;
        HUB->pc_x = sv.x;
        HUB->pc_y = sv.y;
        HUB->pc_face = sv.face;
        HUB->pc_light = sv.light;
    }
    start ("dq0m", T_MAKER, libdir);            /* (Its maker, its senses) */
    start ("dq0p", T_SENSES, libdir);
    things (IO_TOOLS, "");                      /* (The things' catalogue read in: the theme's kinds after it) */
    theme_load (resumed ? Q.theme : (unsigned char) (seed >> 8));     /* (The seed's second byte: theme.h) */
    if (!resumed) {                             /* (The character's kit: a new game's, -k's things too) */
        strcpy (HUB->things_text, kit ? kit : "");    /* (Its things more, commas between) */
        things (IO_KIT, "");
    }
    to_hand ();
    for (k = 0; task_[T_SOUND] > 0 && !HUB->beat[T_SOUND] && k < 100; ++k) {   /* (The sounds' words in the hub
                                                **   before anything's heard: a second at most) */
        hy_sleep_ticks (2);
        hub_map (0);
    }
    start ("dq0d", T_DENIZENS, libdir);         /* (Its denizens, its world) */
    start ("dq0w", T_WORLD, 0);
}

/* The game's end: its tasks told to stop (the world first), and waited for */
static void stop_world (void)
{
    unsigned char t;

    HUB->quit = 1;
    for (t = T_WORLD; t < NTASKS; ++t) {
        if (task_[t] > 0) {
            hub_ring (t);
            hub_wait (task_[t]);
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The status line */

void status (void)
{
    static char left[48], right[40];
    char n[6];

    strcpy (left, P.name[0] ? P.name : "Dingus Quest Zero");
    strcpy (right, paused ? "PAUSED  " : "");
    if (task_[T_WORLD] > 0) {
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
** played); not the same way again for half a minute, nor any for five seconds.  The rest pass it by */
static unsigned dplace;
static unsigned long heard_at;
static unsigned char heard_rel = 0xFF;
char heard_what[20];

static void __fastcall__ intend2 (unsigned char kind, unsigned w1, unsigned w2);

/* A blow (an E_FIGHT: the character's, or one at it), worded (overlay 6's); the character dead of it, the game's
** end; knocked senseless, it waits it out */
static void __fastcall__ blow (const struct msg* m)
{
    unsigned out = fight_words (m);

    msg (lb);
    if (HUB->pcsum.cond & C_DEAD) {
        quit = 1;
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
    }
}

static unsigned char events (void)          /* (How many there were) */
{
    static struct msg m;
    unsigned char k, n = 0;
    struct req* r;

    while (!quit && hub_next (&place, &m)) {
        ++n;
        switch (m.kind) {
        case E_TIME:
            if (m.who == 0) {
                tell (T_TIME_PASSES);
            }
            break;
        case E_READY:
            if (m.who == 0) {
                busy = 0;
                heard ();                       /* (What came before it, told before the prompt: a blow's words;
                                                **   knocked senseless by one, busy again) */
                t_prompt (!busy);
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
                    theme_named (TN_STRATUM, HUB->sight.sname);
                    sayf (" You come into the %s.", tname);
                    save_game ();               /* (Kept as it comes into a stratum) */
                }
                if ((m.w1 & 0xFF) == A_CROSSED) {
                    tell (T_YOU_CROSS_THE);
                } else if ((m.w1 & 0xFF) == A_WENT_TO) {
                    tell (T_YOU_GO_OVER);
                }
                look (m.w1 >> 8 & 1);
            }
            break;
        case E_VIEW:
            if (m.who == 0) {
                if (m.w1 == V_STIR) {
                    stir ();                    /* (A denizen it sees moved) */
                    break;
                }
                if (m.w1 == V_TURNED) {          /* (A light lit or put out: the things' words, said) */
                    sayf ("\nYou turn to face %s.", head_name[HUB->pc_face & 7]);
                }
                look (0);
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
        case E_DIG:
            if (m.who == 0) {
                k = m.w1 >> 8;
                if (k == D_BEGUN) {
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
                if (!finds || m.w1 != PO_NOTHING) {
                    fixt_words (m.w1);
                }
                k = m.w1;                       /* (A part lifted out: now taken up) */
                if (lifting[0] && (k == PO_LIFTED || k == PO_PANE_OUT || k == PO_BARS_OUT)) {
                    strcpy (pending, lifting);
                }
                lifting[0] = 0;
                rebar |= k == PO_REBARRED;
                if (finds && (m.w1 == PO_FOUND || m.w1 == PO_NOTHING || m.w1 == PO_FOUND_TRAP)) {
                    msg (HUB->things_text);     /* (What its search turned up besides) */
                    finds = 0;
                }
            }
            break;
        case E_TRAP:
            if (m.who == 0) {
                trap_words (m.w1);
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

static void __fastcall__ intend2 (unsigned char kind, unsigned w1, unsigned w2)
{
    static struct msg m;

    m.kind = kind;
    m.who = 0;
    m.a = 0;
    m.w1 = w1;
    m.w2 = w2;
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
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

/* ---- The things (items.h): a request to the things task (dq0p), waited for: its result */
static unsigned char __fastcall__ things (unsigned char op, const char* name)
{
    struct ireq* q = &HUB->ireq;

    q->op = op;
    strncpy (q->name, name, sizeof q->name - 1);
    q->name[sizeof q->name - 1] = 0;
    q->state = REQ_POSTED;
    hub_ring (T_SENSES);
    while (q->state != REQ_DONE) {
        hy_sleep_ticks (1);                     /* (The CPU left to the senses, not spun away) */
        hub_map (0);
    }
    q->state = REQ_FREE;
    return q->result;
}

/* The things asked from an overlay (theme.c's): the hub's header put in the window for it, then the overlay back */
unsigned othings_x;
unsigned char __fastcall__ othings (unsigned char op, const char* name, const char* into, unsigned char to)
{
    unsigned char b = getbank (), r;

    hub_map (0);
    strcpy (HUB->ireq.into, into);
    HUB->ireq.to = to;
    r = things (op, name);
    othings_x = HUB->ireq.x;
    strcpy (lb, HUB->things_text);
    setbank (b);
    return r;
}

/* What's to hand, told the referee (the keys of the tools it carries: a skill used takes the tool that suits), and
** the load it carries (the things' weight, all told: the referee's encumbrance, in its summary) */
static void to_hand (void)
{
    things (IO_TOOLS, "");
    strcpy (HUB->slot[SLOT_UI].text, HUB->things_text);
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
    msg (HUB->things_text);
    if (HUB->ireq.result == IR_OK) {
        after (was, HUB->ireq.x, op);
    }
}

/* A portal's part, fixed in it (a door's bar, a window's pane or its bars: IR_FIXED, its square in the request's x,
** y): the portal at it or beside it faced, and the part lifted out (the maker's: a bar only from its side), then taken
** up (E_PORTAL's) */
static void __fastcall__ lift (const char* name)
{
    struct sight* s = &HUB->sight;
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

/* A thing taken up, by its name (walked over to first, if it's out of reach; a portal's part lifted out first) */
void __fastcall__ s_take (const char* name)
{
    unsigned char was = HUB->pc_light, r;

    if (fixed (name)) {                         /* (A theme's altar, its throne) */
        pending[0] = 0;
        return;
    }
    r = things (IO_GET, name);

    if (r == IR_NOT_HERE) {
        strcpy (pending, name);
        intend2 (I_GOTO, HUB->ireq.x, HUB->ireq.y);
        return;
    }
    pending[0] = 0;
    if (r == IR_FIXED) {
        lift (name);
        return;
    }
    tell (r == IR_OK ? T_TAKEN : r == IR_FULL ? T_YOUR_HANDS_ARE : T_YOU_SEE_NONE);
    if (r == IR_OK) {
        after (was, 20, IO_GET);
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

/* A portal it sees looked at closely (the parser's thing, the way it lies): the maker's record of it (ASK_UI's
** M_WORK, PA_EXAMINE: a doorway has none), judged by the craft that knows its material (the referee's test: wood
** Timbercraft, iron Metalcraft, stone Masonry), in words (overlay 4); a few seconds' looking */
static void __fastcall__ inspect (unsigned char obj, unsigned char orel)
{
    struct mreq* q = &HUB->mreq[ASK_UI];
    struct req* r;
    signed char g;

    if (find (obj | PICK_TELL, orel) != 1) {
        return;
    }
    if (REL_BEHIND (REL8 (g = bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y), HUB->pc_face))) {
        intend (I_TURN, g);                     /* (One behind it, that it knows of: turned to, to see it) */
    }
    q->kind = M_WORK;
    q->tile = PA_EXAMINE;
    q->sn = HUB->pc_stratum;
    q->cx = tx;
    q->cy = ty;
    q->state = REQ_POSTED;
    hub_ring (T_MAKER);
    while (q->state != REQ_DONE) {
        hy_yield ();
        hub_map (0);
    }
    q->state = REQ_FREE;
    if (q->res) {
        msg ("An open doorway: there's no door in it.");
        return;
    }
    memcpy (&viewed, &HUB->pview, sizeof viewed);
    r = ask_b (RS_TEST, P_APPRAISE, viewed.mat == PM_STONE ? AP_STONE : viewed.mat == PM_IRON ? AP_METAL : AP_WOOD);
    g = r->grade;
    r->state = REQ_FREE;
    portal_look (&viewed, g);
    msg (lb);
    intend2 (I_WAIT, 50, 1);
}

/* A thing hidden where the character stands (Concealment: the referee's test, the things' record), worded by how
** well */
static void __fastcall__ hide (const char* w)
{
    unsigned char d;

    if (!*w) {
        tell (T_HIDE_WHAT_HIDING);
    } else if (things (IO_HIDE, w) == IR_OK) {
        d = HUB->ireq.qty;
        tell (d >= 6 ? T_HID_CUNNINGLY : d >= 3 ? T_HID_WELL : T_HID_POORLY);
        to_hand ();
        intend2 (I_WAIT, 600, 1);
    } else {
        tell (T_YOU_CARRY_NONE);
    }
}

/* The character's way with a denizen (how: hub.h's SW_*; the parser's words, the one meant and a name or a thing,
** and its request): asked of the denizens' task (the hub's sway: it lends the UI's slot to the referee meanwhile),
** waited for, worded (overlay 6: but none found, if SW_QUIET), and the time it took the character's.  Its result */
#define SW_QUIET    0x80
typedef char sway_copies[offsetof (struct sway, order) - offsetof (struct sway, word) ==
    offsetof (struct pcmd, order) - offsetof (struct pcmd, word) ? 1 : -1];   /* (The same, word to order) */
#define dealt       (*(struct sway*) (lb + sizeof lb - sizeof (struct sway)))  /* (Its copy at lb's end: the
                                                **   words, a line or two, go at its start) */
static unsigned char __fastcall__ deal (unsigned char how, const struct pcmd* c)
{
    struct sway* s = &HUB->sway;

    s->how = how & ~SW_QUIET;
    memcpy (s->word, c->word, sizeof c->word + 1 + sizeof c->word2 + 1);   /* (word, prep, word2, order) */
    s->state = REQ_POSTED;
    hub_ring (T_DENIZENS);
    while (s->state != REQ_DONE) {
        hy_sleep_ticks (1);
        hub_map (0);
    }
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

/* The commands that want nothing but their verb (unoptimised: cc65 2.19's optimiser takes a branch here for a short
** one that isn't, an assembler's range error) */
#pragma optimize (push, off)
static void __fastcall__ chores (unsigned char verb)
{
    switch (verb) {
    case V_LOOK:
        look (0);
        return;
    case V_MAP:
        show_map ();
        return;
    case V_WAIT:
        intend (I_WAIT, 0);
        return;
    case V_SHEET:
        sheet ();
        return;
    case V_PAUSE:
        HUB->paused = paused = 1;
        tell (T_PAUSED);
        status ();
        return;
    case V_HELP:
        help ();
        return;
    case V_QUIT:
        quit = 1;
        return;
    case V_SAVE:
        if (!savedir) {
            tell (T_NOT_KEPT);
        } else {
            save_game ();
            tell (T_KEPT);
        }
        return;
    }
}
#pragma optimize (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)

/* A command that takes no time and wants nothing of the character (what it carries, its sheet, the map, the help):
** carried out at once, the character busy or not (on its way somewhere, looking about: the user, "the UI is slow to
** respond"), if it's the next one typed */
static unsigned char __fastcall__ at_once (const char* l)
{
    static struct pcmd c;
    static char b[INMAX + 1];

    strcpy (b, l);
    parse (b, &c);
    return c.verb == V_INVENTORY || c.verb == V_SHEET || c.verb == V_MAP || c.verb == V_HELP;
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* A command, the parser's, carried out */
void __fastcall__ s_command (char* l)
{
    static struct pcmd c;
    struct req* r;
    unsigned char d, pa;

    pc_face = HUB->pc_face;
    parse (l, &c);
    choose (&c);                                /* (A choice asked for: this line's, the command again) */
    tx = 0;
    if (c.word[0] && c.verb == V_MOVE && c.dir == 0xFF && !c.obj) {
        sayf ("\nI don't know how to %s.", c.word);
        return;
    }
    if ((d = puzzle (&c)) != Q_NONE) {          /* (The theme's puzzle's: theme.c) */
        if (d >= Q_DONE) {
            HUB->way_open = Q.open;
            intend2 (I_WAIT, 50, 1);
        }
        if (d == Q_RULED) {
            end (END_RULED);
        }
        return;
    }
    switch (c.verb) {
    case V_NONE:
        return;
    case V_MOVE:
    case V_ENTER:
    case V_APPROACH:
        if (c.obj) {                            /* (A thing: up to it, or through it) */
            if (find (c.obj | PICK_TELL, c.orel) != 1) {
                return;                         /* (None, told; or which, asked) */
            }
            if (c.obj == OB_WINDOW && (c.how == HOW_THROUGH || c.verb == V_ENTER)) {
                pa = PA_THROUGH;                /* (Climbed through) */
                goto portal;
            } else if ((c.how == HOW_THROUGH || c.verb == V_ENTER) && (c.obj == OB_DOOR || c.obj == OB_PASSAGE)) {
                intend (I_GO, bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y));
            } else {
                intend2 (I_GOTO, tx, ty);
            }
        } else if (c.dir == GO_UP || c.dir == GO_DOWN) {
            intend (I_CLIMB, c.dir == GO_UP ? DIR_UP : DIR_DOWN);
        } else if (c.dir != 0xFF) {
            intend (I_GO, c.dir);
        } else {
            tell (T_GO_WHICH_WAY);
        }
        return;
    case V_CLIMB:
        if (c.obj == OB_WINDOW || c.how == HOW_THROUGH) {
            c.obj = OB_WINDOW;                  /* (Through a window) */
            pa = PA_THROUGH;
            goto portal;
        }
        intend (I_CLIMB, c.dir == GO_DOWN ? DIR_DOWN : DIR_UP);
        return;
    case V_TURN:
    case V_FACE:
        if (c.dir > H_NW) {
            tell (T_WHICH_WAY_LEFT);
        } else if (c.dir == HUB->pc_face) {
            sayf ("\nYou face %s already.", head_name[c.dir]);
        } else {
            intend (I_TURN, c.dir);
        }
        return;
    case V_OPEN:
    case V_CLOSE:
    case V_UNLOCK:
    case V_FORCE:
    case V_RAISE:
    case V_LOWER:
    case V_BREAK:
    case V_DISARM:
    case V_BAR:
    case V_BEND:
        pa = c.verb == V_OPEN ? PA_OPEN : c.verb == V_CLOSE ? PA_CLOSE : c.verb == V_FORCE ? PA_FORCE :
            c.verb == V_RAISE ? PA_RAISE : c.verb == V_LOWER ? PA_LOWER : c.verb == V_BREAK ? PA_BREAK :
            c.verb == V_DISARM ? PA_DISARM : c.verb == V_BAR ? PA_BAR : c.verb == V_BEND ? PA_BEND : PA_UNLOCK;
        if (pa == PA_RAISE && c.word[0]) {
            pa = PA_LIFT;                       /* ("Lift the bar": a portal's part lifted out) */
        } else if (pa == PA_RAISE || pa == PA_LOWER) {
            c.obj = OB_PORTC;
        } else if (c.obj == OB_WINDOW && !strcmp (c.word, "bars") && !find (OB_WINDOW | PICK_ANY, c.orel)) {
            c.obj = OB_PORTC;                   /* ("Bend the bars": a portcullis's, with no window there) */
        }
        HUB->pc_strike = SK_BEST;               /* (Broken with the best to hand) */
portal:
        if (c.obj && (d = find (c.obj, c.orel)) == 2) {
            return;                             /* (Which, asked) */
        }
        if (c.obj && d) {
            c.orel = bearing ((int) tx - HUB->pc_x, (int) ty - HUB->pc_y);    /* (The one it sees: faced) */
        }
        if (c.orel != 0xFF && c.orel <= H_NW && c.orel != HUB->pc_face) {
            intend (I_TURN, c.orel);            /* (The door that way: faced first) */
        }
        HUB->ireq.to = 0;
        barw = pa == PA_BAR && things (IO_BAR, "") == IR_NO ? HUB->ireq.x : 0;    /* (The widest bar to hand) */
        work (pa, c.obj);
        return;
    case V_EXAMINE:                             /* (A thing looked at closely: judged, by the craft that knows it; a
                                                **   portal, or its part if it's not there; else the room) */
        if (c.word[0] && !c.obj && deal (SW_LOOK | SW_QUIET, &c) != SR_NONE) {
            return;                             /* (A denizen it sees: how it seems) */
        }
        if (!c.word[0] || things (IO_DESCRIBE, c.word) != IR_OK) {
            if (c.obj == OB_DOOR || c.obj == OB_WINDOW || c.obj == OB_PORTC) {
                inspect (c.obj, c.orel);
            } else if (c.word[0]) {
                sayf ("\nYou see no %s here.", c.word);
            } else {
                look (2);                       /* (A close look about: the room's figures) */
            }
            return;
        }
        memcpy (&judged, &HUB->ireq, sizeof judged);
        r = ask_b (RS_TEST, P_APPRAISE, judged.craft);
        d = r->grade;
        r->state = REQ_FREE;
        appraise (&judged, d);
        msg (lb);
        return;
    case V_SEARCH:
        finds = things (IO_SEARCH, "");         /* (Things concealed about it; then the walls) */
        intend (I_PORTAL, PA_SEARCH);
        return;
    case V_DIG:
        d = c.dir;
        if (d == GO_DOWN) {
            intend (I_DIG, DIR_DOWN);
        } else if (d <= H_NW && !(d & 1)) {
            intend (I_DIG, d >> 1);
        } else {
            tell (T_DIG_WHICH_WAY);
        }
        return;
    case V_GET:
        if (c.prep == PR_FROM) {
            handle (IO_TAKE, c.word, c.word2, 0);
        } else if (!c.word[0]) {
            tell (T_TAKE_WHAT);
        } else {
            take (c.word);
        }
        return;
    case V_DROP:
        if (!c.word[0]) {
            tell (T_PUT_DOWN_WHAT);
        } else if (d = HUB->pc_light, things (IO_DROP, c.word) == IR_OK) {
            tell (T_PUT_DOWN);
            after (d, 10, IO_DROP);
        } else {
            tell (T_YOU_CARRY_NONE);
        }
        return;
    case V_HIDE:                                /* (Concealment: a thing; oneself comes with the denizens) */
        hide (c.word);
        return;
    case V_HOLD:
    case V_STOW:
    case V_PUT:
    case V_DON:
    case V_DOFF:
    case V_LIGHT:
    case V_DOUSE:
        handle (c.verb == V_HOLD ? IO_HOLD : c.verb == V_STOW ? IO_STOW : c.verb == V_PUT ? IO_PUT : c.verb == V_DON ?
            IO_DON : c.verb == V_DOFF ? IO_DOFF : c.verb == V_LIGHT ? IO_LIGHT : IO_DOUSE, c.word, c.word2,
            c.prep == PR_ON);
        return;
    case V_INVENTORY:
        things (IO_LIST, "");
        msg (HUB->things_text);
        if ((d = HUB->pcsum.load) != 0) {
            tell (T_BURDENED - 1 + d);          /* (Its load, as the referee has it: its encumbrance) */
        }
        return;
    case V_ATTACK:                              /* (A blow at a denizen beside it, overlay 6's choice; or the stance */
    case V_DEFEND:                              /*   set, the referee's defence for the character) */
        if ((d = fight_target (&c)) == FT_THING) {
            pa = PA_BREAK;                      /* (A blow at a thing: broken by it) */
            goto portal;
        }
        if (d) {
            intend (I_ATTACK, d);
            return;
        }
        msg (lb);
        return;
    case V_JUMP:
        if (c.dir == GO_DOWN) {
            intend (I_CLIMB, DIR_DOWN | I_JUMP);    /* (Down a ledge, or a shaft: a fall) */
        } else {
            tell (T_YOU_JUMP_AND);
        }
        return;
    default:                                    /* (A denizen's: V_TALK ... in SW_TALK's order; the rest chores) */
        if (c.verb >= V_TALK) {
            deal (c.verb - V_TALK + SW_TALK, &c);
        } else {
            chores (c.verb);
        }
        return;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The game begun: its arguments, a game kept carried on, the title, the character, the world started */
static unsigned long seed;                      /* (Its seed: the clock's, as the task starts, but for -s's) */

void __fastcall__ s_begin (int argc, char* argv[])
{
    const char* file = 0;
    unsigned char i;
    char* l;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            for (seed = 0, l = argv[++i]; *l >= '0' && *l <= '9'; ++l) {
                seed = seed * 10 + *l - '0';
            }
        } else if (!strcmp (argv[i], "-d") && i + 1 < argc) {
            savedir = argv[++i];                /* (The save's folder) */
        } else if (!strcmp (argv[i], "-k") && i + 1 < argc) {
            kit = argv[++i];                    /* (A tool to begin with: till the items layer, the tests') */
        } else if (!strcmp (argv[i], "-T")) {
            ++test_start;                       /* (For the tests: the character begins at a way down; -T twice, by
                                                **   the theme's first piece) */
        } else if (!strcmp (argv[i], "-c") && i + 1 < argc) {
            file = argv[++i];
        } else if (!strcmp (argv[i], "-p") && i + 1 < argc) {
            ++i;
            pace = argv[i][0] == 'r' ? PACE_RELAXED : argv[i][0] == 'm' ? PACE_MENACING :
                argv[i][0] == 't' ? PACE_TURNS : PACE_NORMAL;
        } else {
            hy_exits ("usage: dq0 [-s seed] [-p r|n|m|t] [-c file] [-d folder] [-k things] [-T [-T]]");
        }
    }
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
    rseed (seed);
    signal (SIGINT, interrupted);
    t_init ();
    title ();
    character (file, seed);
    start_world (seed);
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
    unsigned shown = 0;
    unsigned long next = 0;
    unsigned char held = 0;
    unsigned char k;
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
                HUB->paused = paused = !paused;
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
        if (k || ((now >= next || now < next - 600) && hy_ticks () - shown >= (inn ? 1000 : 200))) {
            next = (now / 600 + 1) * 600;       /* (The status line as the events change it, and its clock as a
                                                **   game minute goes, a real second apart at most, five while a
                                                **   line's being typed: only what's changed drawn, t_status; the
                                                **   minute's end kept, not divided for each time round) */
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
        if (!busy && pending[0]) {              /* (Walked over: now taken up) */
            take (pending);
            pending[0] = 0;
        }
        if ((l = (char*) t_next ()) != 0 && (!busy || (!held && !(held = !at_once (l))))) {
            command (t_line ());              /* (The character free; or a command that wants nothing of it:
                                                **   held, the next waits for it to be free, not parsed again) */
            held = 0;
            status ();
        }
        t_done ();
        hy_sleep_ticks (2);
    }
    finish ();
    return 0;
}
