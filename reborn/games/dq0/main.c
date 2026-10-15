/*
** main.c - Dingus Quest Zero: the UI's task, its start and its loop.
**
** dq0 [-s seed] [-p pace] [-c file] [-d folder] [-k tool] [-T]: -d keeps the game in a folder (on any card), and
** carries on the game kept there if there is one; without it nothing's kept. the seed makes the same game each time (the dice: dice.h's rseed); the pace is
** r, n, m (relaxed, normal, menacing: asked at the start if it isn't given) or t (turns: the world moves only as
** the character acts, for tests); the file a character made already (hmchar's).  Without one, hmchar is run to make
** one (in this window, the screen as it is outside the game while it runs), into /ram/dq0.hm.
**
** The game is tasks that meet in the hub (hub.h): this one, the UI, makes it and starts the others (the world,
** libdir/dq0w; the referee, libdir/dq0harn: the rules, asked in rs.h's terms; the maker, libdir/dq0m: the
** dungeon grown, as the world asks).  The loop: each key goes to the line being typed (term.c); a line entered is the
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
unsigned char pc_face;                          /* (The character's facing, copied for the parser's overlay) */
unsigned char pace = 0xFF, paused;
const char* const pacename[4] = { "relaxed", "normal", "menacing", "turns" };

/* Real ticks (200 a second) a combat round takes, by pace: 5, 3 and 2 real seconds, whatever the ruleset's round
** (the turns pace: 0) */
static const unsigned tpr[4] = { 1000, 600, 400, 0 };

static unsigned char busy = 1;                  /* The character's doing something: no command till it's free (nor
                                                **   till the world's started, and says so: E_READY) */
static unsigned place;                          /* Where this task is in the world's events */
static int world_task = -1, referee_task = -1, maker_task = -1, senses_task = -1;
static unsigned char test_start;
static const char* kit;                         /* (dq0 -k: a tool, equipped as the game starts) */
static const char* savedir;                     /* (dq0 -d: the save's folder) */
static unsigned char __fastcall__ things (unsigned char op, const char* name);
static void to_hand (void);
static void dig_used (void);
static char dug_with[16];                       /* (The tool a dig took, named) */
static unsigned char finds;                     /* (Things a search turned up, told as it ends: the hub's text) */
static struct ireq judged;                      /* (A thing looked at closely: what it is, for the appraiser) */

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

/* ---- The save (DUNGEON.md): game.dat, the UI's (the game's state, the strata's records), beside the things' items.dat
** (dq0p's), the referee's pc.hm
** and the maker's changes.dat and seen.dat */

#define SAVE_MAGIC  0x5144                      /* "DQ" */
#define SAVE_VER    1
static struct save {
    unsigned        magic;
    unsigned char   version, pace;
    unsigned long   seed, now;
    unsigned char   stratum, face, light;
    unsigned        x, y;
    struct pc       pc;
} sv;
static unsigned char resumed;

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
    return n == sizeof sv && sv.magic == SAVE_MAGIC && sv.version == SAVE_VER ? 0 : -1;
}

/* The game kept: the referee's sheet, the maker's records, and the game's state (the hub's header in the window) */
static void save_game (void)
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
    HUB->test_start = test_start;
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
    strcpy (path, libdir);
    strcat (path, "/dq0m");
    if ((maker_task = hub_spawn (path, T_MAKER, libdir)) < 0) {
        t_end ();
        hy_exits ("dq0: its maker (lib/dq0/dq0m) can't be started");
    }
    strcpy (path, libdir);
    strcat (path, "/dq0p");
    if ((senses_task = hub_spawn (path, T_SENSES, libdir)) < 0) {
        t_end ();
        hy_exits ("dq0: its senses (lib/dq0/dq0p) can't be started");
    }
    if (!resumed) {                             /* (The character's kit: a new game's, -k's thing in hand too) */
        things (IO_KIT, kit ? kit : "");
    }
    to_hand ();
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
    if (maker_task >= 0) {
        hub_ring (T_MAKER);
        hy_wait (maker_task, 0);
    }
    if (senses_task >= 0) {
        hub_ring (T_SENSES);
        hy_wait (senses_task, 0);
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
    unsigned char k;

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
                    k = m.w1 >> 12;             /* (The link's kind: a ladder, or a stairway) */
                    msg ((m.w1 & 0xFF) == A_DOWN ? (k == O_LADDER_DOWN ? "You climb down the ladder." :
                        k == O_SHAFT_DOWN ? "You climb down the shaft." : "You go down the stairway, a long flight.") :
                        k == O_LADDER_UP ? "You climb the ladder." : k == O_SHAFT_UP ? "You climb up the shaft." :
                        "You climb the stairway.");
                    sayf (" You come into the %s.", stratum_name[HUB->sight.sname & 7]);
                    save_game ();               /* (Kept as it comes into a stratum) */
                }
                if ((m.w1 & 0xFF) == A_CROSSED) {
                    msg ("You cross the room.");
                } else if ((m.w1 & 0xFF) == A_WENT_TO) {
                    msg ("You go over to it.");
                }
                look (m.w1 >> 8 & 1);
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
                     m.w1 == N_NO_STAIRS ? "There's no way up or down here." : m.w1 == N_NO_DIG ?
                     "There's nothing there to dig (a wall, or the rock; a shaft from a room's floor)." :
                     "You can't go that way.");
            }
            break;
        case E_DIG:
            if (m.who == 0) {
                k = m.w1 >> 8;
                if (k == D_BEGUN) {
                    dug_with[0] = 0;
                } else if (k != D_NO_TOOL) {
                    dig_used ();
                }
                dig_words (k, m.w1 & 0xFF);
            }
            break;
        case E_LEDGE:
            if (m.who == 0) {
                k = (signed char) m.w1 > 0 ? (signed char) m.w1 : -(signed char) m.w1;
                sayf ((signed char) m.w1 > 0 ? "\nThe floor rises %u feet at a ledge ahead: climb it (u) to go on." :
                      "\nThe floor falls %u feet at a ledge ahead: climb down it (d) to go on.", k);
            }
            break;
        case E_CLIMB:
            if (m.who == 0) {
                climb_words (m.w1 >> 8, m.w1 & 0xFF);
            }
            break;
        case E_PORTAL:
            if (m.who == 0) {
                if (!finds || m.w1 != PO_NOTHING) {
                    portal_words (m.w1);
                }
                if (finds && (m.w1 == PO_FOUND || m.w1 == PO_NOTHING)) {
                    msg (HUB->things_text);     /* (What its search turned up besides) */
                    finds = 0;
                }
            }
            break;
        }
    }
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
        hy_yield ();
        hub_map (0);
    }
    q->state = REQ_FREE;
    return q->result;
}

/* What's to hand, told the referee (the keys of the tools it carries): a skill used takes the tool that suits */
static void to_hand (void)
{
    things (IO_TOOLS, "");
    strcpy (HUB->slot[SLOT_UI].text, HUB->things_text);
    referee (RS_EQUIP, 2)->state = REQ_FREE;
}

/* The tool a dig's spell took (the referee's choice): worn by it, and named as it's first taken up */
char dig_with[16];
static void dig_used (void)
{
    HUB->ireq.to = HUB->dig_wear;
    things (IO_WEAR, HUB->dig_tool);
    if (strcmp (dug_with, HUB->ireq.name)) {
        strcpy (dug_with, HUB->ireq.name);
        strcpy (dig_with, dug_with);
    }
}

static char pending[16];                        /* A thing to take up, walked over to first */

/* A thing taken up, by its name (walked over to first, if it's out of reach) */
static void __fastcall__ take (const char* name)
{
    unsigned char r = things (IO_GET, name);

    if (r == IR_NOT_HERE) {
        strcpy (pending, name);
        intend2 (I_GOTO, HUB->ireq.x, HUB->ireq.y);
        return;
    }
    pending[0] = 0;
    msg (r == IR_OK ? "Taken." : r == IR_FULL ? "Your hands are full, and your pack too." : "You see none of those here.");
    if (r == IR_OK) {
        to_hand ();
        intend2 (I_WAIT, 20, 1);
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

/* A thing it sees, the parser's (its kind, and the heading it lies, if one was said): its square, in tx, ty (1),
** or 0 */
static unsigned tx, ty;
static unsigned char __fastcall__ find (unsigned char obj, unsigned char orel)
{
    struct sight* s = &HUB->sight;
    unsigned char i, k, h;

    for (i = 0; i < s->ndoors; ++i) {
        h = (pc_face + (s->door[i].rel & 7)) & 7;
        if ((obj == OB_DOOR ? !(s->door[i].rel & MOUTH) : obj == OB_PASSAGE && (s->door[i].rel & MOUTH)) &&
            (orel == 0xFF || h == orel)) {
            tx = s->door[i].x;
            ty = s->door[i].y;
            return 1;
        }
    }
    for (i = 0; i < s->nopen; ++i) {
        k = s->open[i].kind;
        h = s->open[i].rel == 0xFF ? 0xFF : (pc_face + (s->open[i].rel & 7)) & 7;
        if ((obj == OB_STAIRS && (k == O_STAIRS_UP || k == O_STAIRS_DOWN || k == O_WAY_OUT)) ||
            (obj == OB_LADDER && (k == O_LADDER_UP || k == O_LADDER_DOWN)) ||
            (obj == OB_SHAFT && (k == O_SHAFT_UP || k == O_SHAFT_DOWN)) || (obj == OB_WAY_OUT && k == O_WAY_OUT)) {
            if (orel == 0xFF || h == orel) {
                tx = s->open[i].x;
                ty = s->open[i].y;
                return 1;
            }
        }
    }
    return 0;
}

static const char* const thing[] = { "", "door", "way on", "stairs", "ladder", "shaft", "ledge", "way out", "wall" };

/* A command, the parser's, carried out */
static void __fastcall__ command (char* l)
{
    static struct pcmd c;
    struct req* r;
    unsigned char d;

    pc_face = HUB->pc_face;
    parse (l, &c);
    if (c.word[0] && c.verb == V_MOVE && c.dir == 0xFF && !c.obj) {
        sayf ("\nI don't know how to %s.", c.word);
        return;
    }
    switch (c.verb) {
    case V_NONE:
        return;
    case V_MOVE:
    case V_ENTER:
    case V_APPROACH:
        if (c.obj) {                            /* (A thing: up to it, or through it) */
            if (!find (c.obj, c.orel)) {
                sayf ("\nYou see no %s %s.", thing[c.obj], c.orel == 0xFF ? "here" : "that way");
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
            msg ("Go which way?");
        }
        return;
    case V_CLIMB:
        intend (I_CLIMB, c.dir == GO_DOWN ? DIR_DOWN : DIR_UP);
        return;
    case V_TURN:
    case V_FACE:
        if (c.dir > H_NW) {
            msg ("Which way? (left, right, half left, half right, around, or north, northeast ... northwest)");
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
        if (c.orel != 0xFF && c.orel <= H_NW && c.orel != HUB->pc_face) {
            intend (I_TURN, c.orel);            /* (The door that way: faced first) */
        }
        intend (I_PORTAL, c.verb == V_OPEN ? PA_OPEN : c.verb == V_CLOSE ? PA_CLOSE : c.verb == V_FORCE ? PA_FORCE :
            PA_UNLOCK);
        return;
    case V_SEARCH:
    case V_EXAMINE:
        if (c.verb == V_EXAMINE && c.word[0]) {  /* (A thing looked at closely: judged, by the craft that knows it) */
            if (things (IO_DESCRIBE, c.word) != IR_OK) {
                sayf ("\nYou see no %s here.", c.word);
                return;
            }
            memcpy (&judged, &HUB->ireq, sizeof judged);
            r = ask_b (RS_TEST, P_APPRAISE, judged.craft);
            d = r->grade;
            r->state = REQ_FREE;
            appraise (&judged, d);
            msg (lb);
            return;
        }
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
            msg ("Dig which way? (north, east, south or west: through a wall or the rock; or down, a shaft)");
        }
        return;
    case V_GET:
        if (!c.word[0]) {
            msg ("Take what?");
        } else {
            take (c.word);
        }
        return;
    case V_DROP:
        if (!c.word[0]) {
            msg ("Put down what?");
        } else if (things (IO_DROP, c.word) == IR_OK) {
            msg ("Put down.");
            to_hand ();
            intend2 (I_WAIT, 10, 1);
        } else {
            msg ("You carry none of those.");
        }
        return;
    case V_INVENTORY:
        things (IO_LIST, "");
        msg (HUB->things_text);
        return;
    case V_DON:
    case V_DOFF:
        msg (c.verb == V_DON ? "You have nothing to put on, or take up." : "You have nothing on you could take off.");
        return;
    case V_JUMP:
        msg ("You jump, and come down where you were.");
        return;
    case V_LIGHT:
        if (HUB->pc_light) {
            msg ("Your torch is lit already.");
        } else {
            intend (I_LIGHT, 1);
        }
        return;
    case V_DOUSE:
        if (!HUB->pc_light) {
            msg ("Your torch isn't lit.");
        } else {
            intend (I_LIGHT, 0);
        }
        return;
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
        msg ("The game is paused: press Escape to go on.");
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
            msg ("This game isn't being kept (dq0 -d FOLDER keeps one).");
        } else {
            save_game ();
            msg ("The game is kept.");
        }
        return;
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
        } else if (!strcmp (argv[i], "-d") && i + 1 < argc) {
            savedir = argv[++i];                /* (The save's folder) */
        } else if (!strcmp (argv[i], "-k") && i + 1 < argc) {
            kit = argv[++i];                    /* (A tool to begin with: till the items layer, the tests') */
        } else if (!strcmp (argv[i], "-T")) {
            test_start = 1;                     /* (For the tests: the character begins at a way down) */
        } else if (!strcmp (argv[i], "-c") && i + 1 < argc) {
            file = argv[++i];
        } else if (!strcmp (argv[i], "-p") && i + 1 < argc) {
            ++i;
            pace = argv[i][0] == 'r' ? PACE_RELAXED : argv[i][0] == 'm' ? PACE_MENACING :
                argv[i][0] == 't' ? PACE_TURNS : PACE_NORMAL;
        } else {
            hy_exits ("usage: dq0 [-s seed] [-p r|n|m|t] [-c file] [-d folder] [-k tool] [-T]");
        }
    }
    if (savedir && !load_save ()) {             /* (A game kept there: carried on) */
        resumed = 1;
        seed = sv.seed;
        if (pace == 0xFF) {
            pace = sv.pace;
        }
        file = save_path ("pc.hm");
    } else if (savedir) {
        mkdir (savedir, 0755);                  /* (A new game: its folder) */
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
    sayf (resumed ? "%s, %s, takes up where they left off." : "%s, %s, enters the dungeon.", P.name, P.occupation);
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
        if (!busy && pending[0]) {              /* (Walked over: now taken up) */
            take (pending);
            pending[0] = 0;
        }
        if (!busy && (l = t_line ()) != 0) {
            command (l);
            status ();
        }
        t_done ();
        hy_sleep_ticks (2);
    }
    save_game ();                               /* (Kept as it ends: the game's quit, not lost) */
    stop_world ();
    t_end ();
    return 0;
}
