/*
** main.c - Dingus Quest Zero: the UI's task, its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file]: the seed makes the same game each time (the dice: dice.h's rseed); the pace is
** r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (hmchar's).  Without one, hmchar is run to make
** one (in this window, the screen as it is outside the game while it runs), into /ram/dq0.hm.
**
** The game is tasks that meet in the hub (hub.h): this one, the UI, makes it and starts the others (the world,
** libdir/dq0w, and the referee, libdir/dq0harn: the rules, asked in rs.h's terms).  The loop: each key goes to the line being typed (term.c); a line entered is the
** character's next command, made an intent for the world when the character's free (the world says so: E_READY);
** the world's events are worded into the story.  Escape pauses the game and goes on again.
*/

#include <conio.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"
#include "rs.h"
#include "level.h"

unsigned long now;                              /* The game's time, as the world's clock last said */
unsigned char pace = 0xFF, paused;
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

/* Real ticks (200 a second) a combat round takes, by pace: 5, 3 and 2 real seconds, whatever the ruleset's round
** (the turns pace: 0) */
static const unsigned tpr[4] = { 1000, 600, 400, 0 };

static unsigned char busy = 1;                  /* The character's doing something: no command till it's free (nor
                                                **   till the world's started, and says so: E_READY) */
static unsigned place;                          /* Where this task is in the world's events */
static int world_task = -1, referee_task = -1;
struct pc P;
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
    static unsigned char b[512];
    static char path[56];
    int fd, first, n;
    unsigned char k;
    unsigned off;

    strcpy (path, libdir);
    strcat (path, "/dq0.ovl");
    if ((fd = open (path, O_RDONLY)) < 0 || (first = hy_banks_alloc (4)) < 0) {
        hy_exits ("dq0: its overlays (lib/dq0/dq0.ovl) not found, or no RAM banks");
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

/* ---- The character: hmchar's, its file read by the referee */

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
static char pcfile[64];

static void __fastcall__ character (const char* file, unsigned long seed)
{
    static char s[12], s2[48];
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
        strcpy (s2, gamedir);
        strcat (s2, "/hmchar");
        task = hy_spawn (s2, argv, 0);
        if (task < 0 || hy_wait (task, 0)) {
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

/* The character's sheet, the referee's words */
static void sheet (void)
{
    struct req* r = referee (RS_DESCRIBE, 0);

    blank ();
    say (r->text);
    blank ();
    r->state = REQ_FREE;
}

/* ---- The hub; the referee started, the character loaded, and the world started */

static void start_world (unsigned long seed)
{
    static char path[56];
    struct req* r;

    if (hub_create (HUB_BANKS)) {
        t_end ();
        hy_exits ("dq0: no shared segment, or no semaphores, for the game's tasks");
    }
    HUB->round = ROUND;                         /* (The ruleset's: HarnMaster's 10 seconds, till the referee says) */
    HUB->tpr = tpr[pace];
    HUB->pace = pace;
    HUB->seed = seed;
    strcpy (path, libdir);
    strcat (path, "/dq0harn");
    if ((referee_task = hub_spawn (path, T_REFEREE, 0)) < 0) {
        t_end ();
        hy_exits ("dq0: its rules (lib/dq0/dq0harn) can't be started");
    }
    r = referee (RS_HELLO, 0);
    HUB->round = r->w1;                         /* (The ruleset's round, in tenths) */
    r->state = REQ_FREE;
    load_pc ();                                 /* (Its senses the referee's, before the world looks through them) */
    strcpy (path, libdir);
    strcat (path, "/dq0w");
    if ((world_task = hub_spawn (path, T_WORLD, 0)) < 0) {
        t_end ();
        hy_exits ("dq0: its world (lib/dq0/dq0w) can't be started");
    }
}

/* The game's end: the world and the referee told to stop, and waited for */
static void stop_world (void)
{
    HUB->quit = 1;
    if (world_task >= 0) {
        hub_ring (T_WORLD);
        hy_wait (world_task, 0);
    }
    if (referee_task >= 0) {
        hub_ring (T_REFEREE);
        hy_wait (referee_task, 0);
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
    if (world_task >= 0) {
        strcat (right, HUB->pcstatus);          /* (The referee's words: "UP 0" ...) */
        strcat (right, "  facing ");
        strcat (right, head_abbr[HUB->pc_face & 7]);
        strcat (right, "  ");
    }
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
                t_prompt (1);
            }
            break;
        case E_BYE:
            quit = 1;
            break;
        case E_ARRIVE:
            if (m.who == 0) {
                if ((m.w1 & 0xFF) == A_DOWN || (m.w1 & 0xFF) == A_UP) {
                    msg ((m.w1 & 0xFF) == A_DOWN ? "You go down the stairs." : "You climb the stairs.");
                }
                look (m.w1 >> 8);
            }
            break;
        case E_VIEW:
            if (m.who == 0) {
                if (m.w1 == V_TURNED) {
                    sayf ("\nYou turn to face %s.", head_name[HUB->pc_face & 7]);
                } else {
                    msg (m.w1 == V_LIT ? "You light your torch." : "You put out your torch.");
                }
                look (0);
            }
            break;
        case E_NOWAY:
            if (m.who == 0) {
                msg (m.w1 == N_SHUT ? "The way out is shut fast. Whatever opens it is somewhere below." :
                     m.w1 == N_NO_STAIRS ? "There are no stairs that way here." : "You can't go that way.");
            }
            break;
        }
    }
}

/* ---- The commands (the parser comes later: a word, and the rest) */

static void __fastcall__ intend (unsigned char kind, unsigned char a)
{
    static struct msg m;

    m.kind = kind;
    m.who = 0;
    m.a = a;
    m.w1 = 0;
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
}

/* A word that's a way: a heading of the compass's eight (north or n ... northwest or nw: H_N ...), or one as the
** character faces (forward, ahead or f; right; back or b; left), made the compass's; up (u) or down (d): GO_UP,
** GO_DOWN; or 0xFF */
#define GO_UP       8
#define GO_DOWN     9
static unsigned char __fastcall__ direction (const char* w)
{
    static const char* const relw[8] = { "forward", "ahead", "f", "right", "back", "b", "left", "" };
    static const unsigned char reloff[8] = { 0, 0, 0, 2, 4, 4, 6, 0 };
    unsigned char h;
    char ab[3];

    for (h = 0; h < 8; ++h) {
        ab[0] = head_abbr[h][0] + 'a' - 'A';
        ab[1] = head_abbr[h][1] ? head_abbr[h][1] + 'a' - 'A' : 0;
        ab[2] = 0;
        if (!strcmp (w, ab) || !strcmp (w, head_name[h])) {
            return h;
        }
    }
    if (!strcmp (w, "u") || !strcmp (w, "up")) {
        return GO_UP;
    }
    if (!strcmp (w, "d") || !strcmp (w, "down")) {
        return GO_DOWN;
    }
    for (h = 0; h < 7; ++h) {
        if (!strcmp (w, relw[h])) {
            return (HUB->pc_face + reloff[h]) & 7;
        }
    }
    return 0xFF;
}

/* The next word of a command's (made lower case), *at moved past it */
static char* __fastcall__ next_word (char** at)
{
    char* w;
    char* s = *at;

    while (*s == ' ') {
        ++s;
    }
    for (w = s; *s && *s != ' '; ++s) {
        if (*s >= 'A' && *s <= 'Z') {
            *s += 'a' - 'A';
        }
    }
    if (*s) {
        *s++ = 0;
    }
    *at = s;
    return w;
}

/* The map, and beside it the referee's short lines (the attributes as they are now) */
static void show_map (void)
{
    struct req* r = ask_b (RS_DESCRIBE, 0, 1);

    map (r->text);                              /* (Its lines read where they are, in the hub's header, before the
                                                **   level's put in the window) */
    r->state = REQ_FREE;
}

/* turn left or right (a quarter turn), half left or half right (an eighth), around (or back), or north ...;
** face north ... (or left ...): the same */
static void __fastcall__ turn (char* rest)
{
    char* how = next_word (&rest);
    unsigned char h = HUB->pc_face;

    if (!strcmp (how, "half")) {
        how = next_word (&rest);
        h = !strcmp (how, "left") ? (h + 7) & 7 : !strcmp (how, "right") ? (h + 1) & 7 : 0xFF;
    } else {
        h = !strcmp (how, "around") ? (h + 4) & 7 : direction (how);
    }
    if (h > H_NW) {
        msg ("Which way? (left, right, half left, half right, around, or north, northeast ... northwest)");
        return;
    }
    if (h == HUB->pc_face) {
        sayf ("\nYou face %s already.", head_name[h]);
        return;
    }
    intend (I_TURN, h);
}

static void __fastcall__ command (char* l)
{
    char* w = l;
    char* rest;
    unsigned char d;

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
    if (!strcmp (w, "go") || !strcmp (w, "walk")) {
        w = next_word (&rest);
        if (!*w) {
            msg ("Go which way?");
            return;
        }
    }
    if ((d = direction (w)) != 0xFF) {
        if (d >= GO_UP) {
            intend (I_CLIMB, d == GO_UP ? DIR_UP : DIR_DOWN);
        } else {
            intend (I_GO, d);
        }
    } else if (!strcmp (w, "quit") || !strcmp (w, "q")) {
        quit = 1;
    } else if (!strcmp (w, "turn") || !strcmp (w, "face")) {
        turn (rest);
    } else if (!strcmp (w, "light")) {
        if (HUB->pc_light) {
            msg ("Your torch is lit already.");
        } else {
            intend (I_LIGHT, 1);
        }
    } else if (!strcmp (w, "douse") || !strcmp (w, "extinguish")) {
        if (!HUB->pc_light) {
            msg ("Your torch isn't lit.");
        } else {
            intend (I_LIGHT, 0);
        }
    } else if (!strcmp (w, "look") || !strcmp (w, "l")) {
        look (0);
    } else if (!strcmp (w, "map") || !strcmp (w, "m")) {
        show_map ();
    } else if (!strcmp (w, "wait") || !strcmp (w, "z")) {
        intend (I_WAIT, 0);
    } else if (!strcmp (w, "sheet") || !strcmp (w, "skills")) {
        sheet ();
    } else if (!strcmp (w, "pause")) {
        HUB->paused = paused = 1;
        msg ("The game is paused: press Escape to go on.");
        status ();
    } else if (!strcmp (w, "help") || !strcmp (w, "?")) {
        msg ("Commands: north, northeast, east ... northwest (n, ne, e, se, s, sw, w, nw, or go north ...); forward, "
             "back, left, right (f, b: as you face); up and down (u, d: the stairs); turn left, right, half left, half "
             "right or around; face north ...; light and douse (your torch); look (l), map (m), sheet (the character), "
             "wait (z), pause (or Escape), quit (q).");
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
    find_dirs ();
    ovl_load ();
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
