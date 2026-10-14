/*
** main.c - Dingus Quest Zero: its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file]: the seed makes the same game each time (the dice: dice.h's rseed); the pace is
** r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (hmchar's).  Without one, hmchar is run to make
** one (in this window, the screen as it is outside the game while it runs), into /ram/dq0.hm.
**
** The loop: each key goes to the line being typed (term.c); the real clock's ticks run the game's clock at the
** pace (the world's things happen as its time comes); a line entered is the character's next command, done when the
** character's free (its last action's time over).  Escape pauses the game and goes on again.
*/

#include <conio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"

unsigned long now;                              /* The game's time, tenths of a second */
unsigned char pace = 0xFF, paused;
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

/* Real ticks (200 a second) a tenth of the game's second takes, by pace: a combat round (10 game seconds) in 5,
** 3 and 2 real seconds */
static const unsigned char tpt[3] = { 10, 6, 4 };

static unsigned last;                           /* The tick count at the last look, */
static unsigned acc;                            /*   and the ticks not yet the game's time */
static unsigned long ready;                     /* When the character can act next */
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

/* ---- The world: what happens as its time comes (the dungeon's comes later) */

static void world (void)
{
}

/* The game's clock run on to t, the world's things as their time comes */
static void __fastcall__ run_to (unsigned long t)
{
    while (now < t) {
        ++now;
        world ();
    }
}

/* The real clock's ticks since the last look, made the game's time at the pace */
static void clock_run (void)
{
    unsigned t = hy_ticks (), d = t - last;
    unsigned long was = now / 10;

    last = t;
    if (paused || pace == PACE_TURNS) {
        return;
    }
    acc += d;
    while (acc >= tpt[pace]) {
        acc -= tpt[pace];
        ++now;
        world ();
    }
    if (now / 10 != was) {
        status ();                              /* (The clock on the status line, each game second) */
    }
}

/* An action of the character's, taking time t: done now, the character busy till its time is over (a turn game:
** the world runs on to then at once) */
static void __fastcall__ takes (unsigned t)
{
    ready = now + t;
    if (pace == PACE_TURNS) {
        run_to (ready);
    }
}

/* ---- The commands (the parser comes later: a word, and the rest) */

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
        msg ("Time passes.");
        takes (ROUND);
    } else if (!strcmp (w, "sheet") || !strcmp (w, "skills")) {
        sheet ();
    } else if (!strcmp (w, "pause")) {
        paused = 1;
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
    unsigned long seed = hy_time () ^ hy_ticks ();
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
    status ();
    blank ();
    sayf ("%s, %s, enters the dungeon.", P.name, P.occupation);
    blank ();
    msg ("Type help for the commands.");
    t_done ();
    last = hy_ticks ();
    while (!quit) {
        while (kbhit ()) {
            k = cgetc ();
            if (k == CH_ESC) {
                paused = !paused;
                status ();
            } else if (k == CH_RESIZE) {
                t_init ();
            } else {
                t_key (k);
            }
        }
        clock_run ();
        if (now >= ready && (l = t_line ()) != 0) {
            command (l);
            status ();
        }
        t_done ();
        hy_sleep_ticks (2);
    }
    t_end ();
    return 0;
}
