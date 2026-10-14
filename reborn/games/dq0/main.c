/*
** main.c - Dingus Quest Zero: the UI's task, its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file]: the seed makes the same game each time (the dice: dice.h's rseed); the pace is
** r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (hmchar's).  Without one, hmchar is run to make
** one (in this window, the screen as it is outside the game while it runs), into /ram/dq0.hm.
**
** The game is tasks that meet in the hub (hub.h): this one, the UI, makes it and starts the others (the world,
** LIBDIR/dq0w, so far).  The loop: each key goes to the line being typed (term.c); a line entered is the
** character's next command, made an intent for the world when the character's free (the world says so: E_READY);
** the world's events are worded into the story.  Escape pauses the game and goes on again.
*/

#include <conio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"

unsigned long now;                              /* The game's time, as the world's clock last said */
unsigned char pace = 0xFF, paused;
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

/* Real ticks (200 a second) a combat round takes, by pace: 5, 3 and 2 real seconds, whatever the ruleset's round
** (the turns pace: 0) */
static const unsigned tpr[4] = { 1000, 600, 400, 0 };

static unsigned char busy;                      /* The character's doing something: no command till it's free */
static unsigned place;                          /* Where this task is in the world's events */
static int world_task = -1;
static volatile unsigned char quit;

static void interrupted (int sig)
{
    (void) sig;
    quit = 1;
}

/* ---- The overlays: LIBDIR/dq0.ovl, an 8K bank each (dq0.cfg's OV1 ...; the file cut after the last one used:
** make.js), read into banks of the game's own */

static void ovl_load (void)
{
    static unsigned char b[512];
    int fd = open (LIBDIR "/dq0.ovl", O_RDONLY), first, n;
    unsigned char k;
    unsigned off;

    if (fd < 0 || (first = hy_banks_alloc (4)) < 0) {
        hy_exits ("dq0: " LIBDIR "/dq0.ovl: not found, or no RAM banks");
    }
    for (k = 0; k < 4; ++k) {
        ovbank[k] = first + k;
        for (off = 0; off < 0x2000; off += sizeof b) {
            n = read (fd, b, sizeof b);
            if (n <= 0) {
                break;
            }
            bwrite (first + k, (void*) (0x8000 + off), b, n);
        }
    }
    close (fd);
}

/* ---- The character: hmchar's, its file read (intro.c's load) */

/* A line written to the window's consctl: on an fd kept open (raw mode lasts while the consctl that set it is) */
static void __fastcall__ consctl (const char* what)
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
static void __fastcall__ character (const char* file, unsigned long seed)
{
    static char s[12];
    static char* argv[6];
    int task;

    if (!file) {
        file = "/ram/dq0.hm";
        t_end ();
        consctl ("rawoff");
        argv[0] = "hmchar";
        argv[1] = "-o";
        argv[2] = (char*) file;
        argv[3] = "-s";
        argv[4] = ultoa (seed, s, 10);
        argv[5] = 0;
        task = hy_spawn ("/bin/hmchar", argv, 0);
        if (task < 0 || hy_wait (task, 0)) {
            hy_exits ("dq0: hmchar didn't make a character");
        }
        consctl ("rawon");
        t_init ();
    }
    if (load (file) < 0) {
        t_end ();
        hy_exits ("dq0: no character in its file");
    }
}

/* ---- The hub, and the world started */

static void start_world (unsigned long seed)
{
    if (hub_create (HUB_BANKS)) {
        t_end ();
        hy_exits ("dq0: no shared segment, or no semaphores, for the game's tasks");
    }
    HUB->round = ROUND;                         /* (The ruleset's: HarnMaster's 10 seconds, till the referee says) */
    HUB->tpr = tpr[pace];
    HUB->pace = pace;
    HUB->seed = seed;
    if ((world_task = hub_spawn (LIBDIR "/dq0w", T_WORLD, 0)) < 0) {
        t_end ();
        hy_exits ("dq0: " LIBDIR "/dq0w: can't start it");
    }
}

/* The game's end: the world told to stop, and waited for */
static void stop_world (void)
{
    if (world_task >= 0) {
        HUB->quit = 1;
        hub_ring (T_WORLD);
        hy_wait (world_task, 0);
    }
}

/* ---- The status line */

void status (void)
{
    static char left[48], right[40];
    char n[6];
    unsigned long s = now / 10;

    strcpy (left, P.name[0] ? P.name : "Dingus Quest Zero");
    strcpy (right, paused ? "PAUSED  " : "");
    strcat (right, "UP ");
    strcat (right, utoa (harn_get_up (&P.h), n, 10));
    strcat (right, "  ");
    strcat (right, utoa (s / 3600, n, 10));
    strcat (right, s % 3600 / 60 < 10 ? ":0" : ":");
    strcat (right, utoa (s % 3600 / 60, n, 10));
    strcat (right, s % 60 < 10 ? ":0" : ":");
    strcat (right, utoa (s % 60, n, 10));
    t_status (left, right);
}

/* ---- The world's events, worded */

static void events (void)
{
    static struct msg m;

    while (hub_next (&place, &m)) {
        switch (m.kind) {
        case E_TIME:
            if (m.who == 0) {
                msg ("Time passes.");
            }
            break;
        case E_READY:
            if (m.who == 0) {
                busy = 0;
            }
            break;
        case E_BYE:
            quit = 1;
            break;
        }
    }
}

/* ---- The commands (the parser comes later: a word, and the rest) */

static void __fastcall__ intend (unsigned char kind, unsigned w1)
{
    static struct msg m;

    m.kind = kind;
    m.who = 0;
    m.w1 = w1;
    if (hub_send (&m)) {
        busy = 1;
    }
}

static void __fastcall__ command (char* l)
{
    char* w = l;
    char* rest;

    while (*w == ' ') {
        ++w;
    }
    for (rest = w; *rest && *rest != ' '; ++rest) {
        if (*rest >= 'A' && *rest <= 'Z') {
            *rest += 'a' - 'A';
        }
    }
    if (*rest) {
        *rest++ = 0;
    }
    if (!*w) {
        return;
    }
    if (!strcmp (w, "quit") || !strcmp (w, "q")) {
        quit = 1;
    } else if (!strcmp (w, "wait") || !strcmp (w, "z")) {
        intend (I_WAIT, 0);
    } else if (!strcmp (w, "sheet") || !strcmp (w, "skills")) {
        sheet ();
    } else if (!strcmp (w, "pause")) {
        HUB->paused = paused = 1;
        msg ("The game is paused: press Escape to go on.");
        status ();
    } else if (!strcmp (w, "help") || !strcmp (w, "?")) {
        msg ("Commands: sheet (the character), wait (z), pause (or Escape), quit (q).");
    } else {
        sayf ("\nI don't know how to %s.", w);
    }
}

int main (int argc, char* argv[])
{
    unsigned long seed = hy_time () ^ hy_ticks (), was;
    const char* file = 0;
    unsigned char k, i;
    char* l;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            seed = strtoul (argv[++i], 0, 10);
        } else if (!strcmp (argv[i], "-c") && i + 1 < argc) {
            file = argv[++i];
        } else if (!strcmp (argv[i], "-p") && i + 1 < argc) {
            ++i;
            pace = argv[i][0] == 'r' ? PACE_RELAXED : argv[i][0] == 'm' ? PACE_MENACING :
                argv[i][0] == 't' ? PACE_TURNS : PACE_NORMAL;
        } else {
            hy_exits ("usage: dq0 [-s seed] [-p r|n|m|t] [-c file]");
        }
    }
    rseed (seed);
    signal (SIGINT, interrupted);
    ovl_load ();
    rules_init ();
    t_init ();
    title ();
    character (file, seed);
    start_world (seed);
    status ();
    blank ();
    sayf ("%s, %s, enters the dungeon.", P.name, P.occupation);
    blank ();
    msg ("Type help for the commands.");
    t_done ();
    while (!quit) {
        hub_map (0);                            /* (The hub at $8000 again: a call may have moved the window) */
        while (kbhit ()) {
            k = cgetc ();
            if (k == CH_ESC) {
                HUB->paused = paused = !paused;
                status ();
            } else if (k == CH_RESIZE) {
                t_init ();
            } else {
                t_key (k);
            }
        }
        events ();
        was = now / 10;
        now = hub_now ();
        if (now / 10 != was) {
            status ();                          /* (The clock on the status line, each game second) */
        }
        if (!busy && (l = t_line ()) != 0) {
            command (l);
            status ();
        }
        t_done ();
        hy_sleep_ticks (2);
    }
    stop_world ();
    t_end ();
    return 0;
}
