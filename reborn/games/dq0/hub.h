/*
** hub.h - Dingus Quest Zero's tasks' meeting place: a shared segment (hy_seg_create) the UI makes and the others
** attach to (its number an argument of theirs), its bank 0 the header and the queues between the tasks, its other
** banks the world's state.  Every task keeps it at $8000 (hub_map), but for the moments an overlay's or a bank of
** its own is there (bank.s's stubs and copies put it back).
**
** The tasks (each a program): the UI (dq0: the screen, the keys, the words), the world (dq0w: the clock, the
** dungeon, the rules of the place), the denizens (dq0d), the referee (a program a ruleset: dq0harn ...: every rules
** question), the sound (dq0s).  Each queue has one task that writes it and one that reads it, so none needs a lock:
** an entry is written whole, then the index that shows it.  A task waiting for its queues waits on its doorbell, a
** counting semaphore its writers release.
**
** The clock is the world's: now, in tenths of a second of game time; a round of the ruleset's (round tenths) takes
** tpr real ticks (200 a second) at the pace the player chose.
*/

#ifndef _HUB_H
#define _HUB_H

#include "rs.h"
#include "level.h"
#include "../dq0w/world.h"
#include "items.h"
#include "theme.h"

#define HUB_MAGIC       0x5144                  /* "DQ" */
#define HUB_VERSION     1
#define HUB_BANKS       (1 + PLAN_BANKS + CACHE_BANKS + 1 + ITEM_BANKS + 3)  /* Bank 0: the header, the queues, the
                                                **   referee's slots, what the character perceives, the map, the
                                                **   strata's records; then the plans' banks and the chunks' (world.h);
                                                **   the catalogue's, and the items' (items.h); the denizens' events;
                                                **   the senses' look (LOOK_BANK); the theme (THEME_BANK) */
#define CAT_BANK        (1 + PLAN_BANKS + CACHE_BANKS)
#define CATALOGUE       ((struct kind*) 0x8000)
#define ITEM_BANK(i)    (CAT_BANK + 1 + (i) / (0x2000 / sizeof (struct item)))
#define ITEM_AT(i)      ((struct item*) 0x8000 + (i) % (0x2000 / sizeof (struct item)))
#define HUB_WINDOW      ((unsigned char*) 0x8000)
#define DEN_BANK        (CAT_BANK + 1 + ITEM_BANKS)     /* The denizens' events (DQ_SIZE of them, a ring) */
#define DQ_SIZE         64
#define DQ_AT           ((struct msg*) 0x8000)
#define SND_AT          ((char*) 0x8000 + DQ_SIZE * sizeof (struct msg))    /* The sounds (SOUND.md), after them: the
                                                **   sound task's table (dq0s.dat), SND_REC bytes a sound, its words
                                                **   first ("a scurrying", 0-ended; "": none): each kind's two (the
                                                **   denizens' task's kinds by number: its step, its noise), then the
                                                **   world's; the UI clears it, the sound task puts it there */
#define LOOK_BANK       (DEN_BANK + 1)  /* The senses' own (dq0p/view.c): the squares round the character as a look
                                                **   about works them out, a grid of them (cast.s's): no other task's */
#define THEME_BANK      (LOOK_BANK + 1)  /* The game's theme (theme.h: the UI's, read in as it starts), and its pieces'
                                                **   places (the maker's) */
#define SND_REC         32
#define SND_WORDS       20
#define SND_KINDS       32
#define SND_WORLD       16

/* The tasks, and so their doorbells */
enum { T_UI, T_WORLD, T_DENIZENS, T_REFEREE, T_SOUND, T_MAKER, T_SENSES, NTASKS };

/* An entry of a queue: an intent (to the world), or an event (from it) */
struct msg {
    unsigned char   kind;                       /* I_* or E_* */
    unsigned char   who;                        /* The actor (0: the character) */
    unsigned char   a, b;                       /* By kind */
    unsigned        w1, w2;
    unsigned long   t;                          /* An event's game time */
    unsigned char   x[4];
};

/* Intents: what an actor means to do (the UI's for the character, the denizens' for theirs) */
enum {
    I_NONE, I_WAIT,                             /* w1: tenths (0: a round); w2: 1, quietly (a thing's handling) */
    I_GO,                                       /* a: a heading (level.h's H_N ... H_NW) */
    I_CLIMB,                                    /* a: DIR_UP or DIR_DOWN (the stairway or ladder in the room);
                                                **   | I_JUMP, down: jumped (a ledge, a shaft: a fall) */
    I_TURN,                                     /* a: the heading to face (H_N ... H_NW) */
    I_LIGHT,                                    /* a: 1 a light of the character's lit, 0 put out (its light the
                                                **   things' already, pc_light: the world takes the time, looks) */
    I_DIG,                                      /* a: a direction (DIR_N ... DIR_W, DIR_DOWN) */
    I_PORTAL,                                   /* a: what's done to the door it faces (PA_*); b: which (PW_*: a
                                                **   door or a portcullis, a window or a slit, either) */
    I_GOTO,                                     /* w1, w2: a square in its room it sees, walked up to */
    I_QUIT,
    NINTENTS
};

/* Events: what's happened, in the world's terms (the UI words them; the denizens and the sound hear them) */
enum {
    E_NONE, E_TIME,                             /* Time passed for who: w1 tenths */
    E_READY,                                    /* who can act again */
    E_BYE,                                      /* The world's ending */
    E_ARRIVE,                                   /* who's somewhere new: w1 how (A_*), 256 the first time there,
                                                **   and (up or down) the link's kind (O_*) times 4096 */
    E_NOWAY,                                    /* who can't: w1 why (N_*) */
    E_VIEW,                                     /* What who sees has changed: w1 why (V_*) */
    E_DIG,                                      /* who's digging: w1 how it goes (D_*) times 256, and the feet to go */
    E_NOISE,                                    /* A noise: a how loud (0-9), w1 w2 its square (the denizens') */
    E_LEDGE,                                    /* who's come to a ledge: w1 its rise (a signed char: a fall less) */
    E_CLIMB,                                    /* who's climbing: w1 how it goes (K_*) times 256, and the feet to go */
    E_PORTAL,                                   /* who's worked a door: w1 what came of it (PO_*), and the
                                                **   portal's kind (PK_*) times 256 */
    E_HARM,                                     /* who's been harmed (the referee's RS_HARM): w1 its cause (HC_*),
                                                **   its worst wound's band times 256 (0: unhurt), E_OUT, E_CAUGHT */
    E_TRAP,                                     /* A trap and who: w1 its kind (TK_*) and its trigger (TT_*) times
                                                **   16, what came of it (TO_*) times 256 */
    E_DEN,                                      /* (The denizens' own, their queue's: DENIZENS.md's events) A
                                                **   denizen's done something: who the actor plus 1, a what (DE_*),
                                                **   b how loud (0 silent ... 9), w1 w2 its square, x[0] its stratum,
                                                **   x[1] its kind (the denizens' task's), x[2] 1 if it's on a square
                                                **   the character sees now (the senses' marks), x[3] what lies
                                                **   between it and the character, of a loud one on its stratum (the
                                                **   squares of wall and rock on the line, 15 at most, and the shut
                                                **   doors times 16: SOUND.md) */
    E_LEFT,                                     /* who's gone up the way out, open (theme.h: the mission's, escaped) */
    NEVENTS
};
enum { DE_MOVED, DE_NOISE, DE_NOTICED, DE_LOST };    /* E_DEN's what: a step, a noise of its own (a squeak, a
                                                **   growl), the character sensed, lost (its senses' own) */
#define I_JUMP          0x80
#define E_OUT           0x4000                  /* (E_HARM's: knocked senseless a while) */
#define E_CAUGHT        0x8000                  /*   (and a fall stopped, a hold grabbed) */
#define E_STOPPED       0x8000                  /*   (an HC_HANDS's: the blows stopped, the hands too hurt) */
#define O_FELL          8                       /* (E_ARRIVE's link kind, or'd: fallen down it, jumped) */

enum { A_WALKED, A_DOWN, A_UP, A_START, A_DIGGING, A_CROSSED, A_CLIMBING, A_CLIMBED, A_WENT_TO, A_RESUMED,
       A_STOPPED };                             /* E_ARRIVE's how (A_DIGGING, A_CLIMBING: the world's own, a spell
                                                **   of work, a round's climbing; A_STOPPED: on its way, at a trap) */
enum { N_NO_EXIT, N_SHUT, N_NO_STAIRS, N_NO_DIG, N_CLOSED, N_DEN, N_DOOR };    /* E_NOWAY's why: no way that way, the way out
                                                **   shut, no stairs, nothing to dig there, the portcullis that way
                                                **   down, a denizen in the way (the actor times 256) */
enum { V_TURNED, V_LIT, V_DARK, V_STIR };       /* E_VIEW's why (V_STIR: a denizen come into sight, gone from it,
                                                **   or doing something else) */
enum { D_BEGUN, D_GOOD, D_POOR, D_SLIP, D_THROUGH, D_SOLID, D_NO_TOOL };  /* E_DIG's how */
enum { K_BEGUN, K_ON, K_BACK, K_FELL, K_UP, K_DOWN, K_JUMPED };    /* E_CLIMB's how: begun, on, slipped back,
                                                **   fallen, up, down; jumped down (a ledge, a shaft) */
enum { PA_OPEN, PA_CLOSE, PA_UNLOCK, PA_FORCE, PA_SEARCH,     /* I_PORTAL's: what's done (a door's opened, ... */
       PA_RAISE, PA_LOWER, PA_BREAK, PA_THROUGH, PA_DISARM,    /*   a portcullis raised, a window's glass broken,
                                                **   climbed through, a trap disarmed: one in reach, or the door's; */
       PA_LIFT,                                 /*   a door's bar lifted out, a window's pane or its bent bars; */
       PA_EXAMINE };                            /*   looked at closely: the UI's own M_WORK, ASK_UI's, cx, cy its
                                                **   square: its record copied into the hub's pview) */
enum { PW_ANY, PW_DOOR, PW_WINDOW };            /* I_PORTAL's b: a door or a portcullis, a window or a slit */
enum { PO_NONE, PO_OPENED, PO_CLOSED, PO_IS_OPEN, PO_IS_SHUT, PO_LOCKED, PO_BARRED, PO_UNBARRED, PO_PICKED,
       PO_NOT_PICKED, PO_NO_PICKS, PO_NO_LOCK, PO_FORCED, PO_HELD, PO_FOUND, PO_NOTHING,   /* E_PORTAL's (a door's, */
       PO_RAISED, PO_HEAVED, PO_LOWERED, PO_IS_UP, PO_IS_DOWN, PO_NO_WINCH, PO_TOO_HEAVY, PO_NOT_PORTC,   /* a
                                                **   portcullis's: by its winch, by hand; */
       PO_NO_SHUTTERS, PO_BENT, PO_BARS_HOLD, PO_SHATTERED, PO_SMASHED, PO_NOTHING_THERE, PO_IN_WAY, PO_SLIT,
       PO_CLIMBED, PO_NO_CLIMB, PO_SLIPPED, PO_NOT_WINDOW,  /*   a window's: its bars bent, its glass broken, its
                                                **   shutters smashed, climbed through; */
       PO_TRAPPED, PO_FOUND_TRAP,               /*   a known trap on a door; a search's traps, told as found; */
       PO_BROKE_DOWN, PO_GIVING, PO_HOLDING, PO_NO_MARK,   /*   blows at it: it gives way, it's giving, it holds
                                                **   (marked), they make no mark; */
       PO_WRECKED, PO_LIFTED, PO_BAR_FAR, PO_NO_BAR, PO_SET_FAST, PO_PANE_OUT, PO_BARS_OUT, PO_BARS_BROKEN };
                                                /*   broken down: it won't shut; a bar lifted out, one on the far
                                                **   side, none to lift; bars set fast; a pane worked out, bent bars
                                                **   lifted out, bars broken out) */
enum { TO_SPRUNG, TO_DODGED, TO_RESISTED, TO_CHOKED, TO_OPENED, TO_NOTICED, TO_FOUND, TO_DISARMED, TO_NOT_YET,
       TO_FUMBLED, TO_CANT, TO_PIT, TO_NONE, TO_ASSESSED, TO_PUZZLED };  /* E_TRAP's: set off, dodged, its gas resisted or choked on, a pit's
                                                **   cover fallen in (no one on it); noticed in passing, found by a
                                                **   search; disarmed, not yet, set off disarming it, no means to
                                                **   (magic), a pit (gone round, not disarmed), none in reach */

/* A referee slot: a request (rs.h's RS_*) and its reply, and the reply's words.  The task that asks writes the
** request, then sets state REQ_POSTED and rings the referee's doorbell; the referee answers, then sets REQ_DONE; the
** task takes the reply and sets REQ_FREE.  Slot 0 is the world's (its requests change things), slot 1 the UI's */
#define REQ_FREE        0
#define REQ_POSTED      1
#define REQ_DONE        2
#define REQ_TEXT        1024
struct req {
    volatile unsigned char state;
    unsigned char   kind;                       /* RS_* */
    unsigned char   actor, other;               /* Who asks about whom */
    signed char     steps;                      /* The circumstances: + better, - worse */
    unsigned char   a, b, c;
    unsigned        w1, w2;
    unsigned char   x[32];                      /* By kind: a reply's effects (rs.h's), a summary ... */
    signed char     grade;                      /* The reply's: G_CF ... G_CS */
    unsigned char   result;
    char            text[REQ_TEXT];             /* A request's path, or a reply's words (lines, a 0 at the end) */
};
enum { SLOT_WORLD, SLOT_UI, NSLOTS };

/* An actor: a denizen near the character, put here by the denizens' task (dq0d: its records are its own, in its
** own banks) for the world to keep the character out of its way and the senses to see.  The denizens' task moves
** its actors itself, by the character's rules (a square at a time, its time added to ready), and bumps den_seq as
** it changes what the character may see of them (moved, what they're doing).  (state, intent, a and result: the
** world's moves of them, once; unused.)  At the turns pace the world moves the clock on through what the character
** does a slice at a time (TURN_SLICE tenths), each once the denizens' task has caught up with it (den_at the
** clock), the world waiting */
#define NACTORS         8
#define TURN_SLICE      100
struct actor {
    volatile unsigned char state;               /* REQ_* */
    unsigned char   den;                        /* Its record (the denizens' task's) */
    unsigned char   sn, face;
    unsigned long   ready;                      /* When it can act again */
    unsigned char   step;                       /* Tenths of a second to go 5 feet */
    unsigned char   intent, a;                  /* I_* (I_GO: a heading, H_N, H_E ...) */
    unsigned char   result;                     /* 0 done, or N_* */
    unsigned        x, y;                       /* (These last as struct sight's den: copied whole) */
    unsigned char   doing;                      /* What it's doing (DO_*: watching, coming, keeping off: it senses the
                                                **   character) */
    char            name[12];                   /* ("": the entry free) */
};

#define IQ_SIZE         16                      /* The UI's intents */
#define EQ_SIZE         128                     /* The world's events (each reader its own place) */

struct hub {                                    /* Bank 0, at $8000 */
    unsigned        magic;
    unsigned char   version;
    unsigned char   seg;
    unsigned char   sem[NTASKS];                /* Each task's doorbell */
    unsigned char   task[NTASKS];               /* Each task's number (0: none) */
    unsigned char   beat[NTASKS];               /* Each task's heartbeat: bumped as it works */
    /* The clock (the world's: tick, then now, then tick again, so a reader sees it whole: hub_now) */
    volatile unsigned char tick;
    volatile unsigned long now;
    volatile unsigned long pc_ready;            /* When the character can act again */
    unsigned        round;                      /* The ruleset's round, tenths of a second */
    unsigned        tpr;                        /* Real ticks a round, at the pace (0: turns) */
    volatile unsigned char paused, hold, quit;  /* The UI's: paused (Escape), held (a question), ending */
    unsigned char   pace;
    unsigned char   test_start;                 /* (dq0 -T: the character begins at a way down, for the tests; 2,
                                                **   -T twice: by the theme's first piece on the top stratum) */
    unsigned char   resume;                     /* A game carried on from its save (the world restores, not begins) */
    char            savedir[40];                /* The save's folder (dq0 -d), or "" (none) */
    unsigned long   seed;
    char            pcstatus[16];               /* The character's state, the referee's words ("UP 3") */
    volatile unsigned char pc_stratum;          /* Where the character is (the world's): its stratum, */
    volatile unsigned pc_x, pc_y;               /*   its square (the dungeon's frame, world.h's), */
    volatile unsigned char pc_face;             /*   the way it faces (a heading: H_N ... H_NW), */
    volatile unsigned char pc_light;            /*   the light it carries: how far it reaches, in tiles (0: none:
                                                **   the things' (dq0p), from the lit things it holds), */
    char            pc_lamp[14];                /*   and the brightest of them, named */
    struct rs_summary pcsum;                    /* The character's summary (the referee's: its senses ...) */
    struct sight    sight;                      /* What it perceives (the senses', after each look about) */
    /* A look about asked of the senses (dq0p: the world asks, as the character's moved, turned, lit a light): the
    ** squares it sees marked, what it perceives recorded in sight */
    volatile unsigned char look_state;          /* REQ_* */
    unsigned char   look_first;                 /* The first time there */
    /* The map: the UI asks (map_state REQ_POSTED, the maker's doorbell), the maker draws its window round the
    ** character into map (each row a line, 0-ended) and says REQ_DONE */
    volatile unsigned char map_state;
    char            map[MAP_H][MAP_W + 1];
    /* The dungeon (world.h): the strata's records, the chunks' cache's table, the maker's requests, and the slot
    ** each task that reads the chunks is reading (the maker never makes way for it) */
    struct stratum  strata[MAXSTRATA];
    struct cslot    cslot[NCSLOTS];
    unsigned        stamp;
    struct mreq     mreq[NASK];
    unsigned char   inuse[NASK];
    /* The things (items.h): the store's lock (a semaphore: the maker and the things both take its records), a
    ** request to the things (dq0p), and their words for the UI (what's carried) */
    unsigned char   item_lock;
    struct ireq     ireq;
    char            things_text[400];
    struct portal   pview;                      /* A portal looked at closely (PA_EXAMINE's: the maker's record of
                                                **   it, but bar: 1 the character's on its bar's side) */
    char            dig_tool[4][8];             /* A dig's spells' tools, the referee's choices (their keys), in turn */
    unsigned char   dig_wear[4];                /*   and the wear each spell did (E_DIG's b, which: the world runs on */
                                                /*   ahead of the UI's reading of its events, the turns pace) */
    /* The UI's intents to the world */
    volatile unsigned char iq_head, iq_tail;    /* Written at head (the UI), read at tail (the world) */
    struct msg      iq[IQ_SIZE];
    /* The world's events, to everyone: each reader keeps its own place */
    volatile unsigned eq_head;                  /* Events written so far (the world's) */
    struct msg      eq[EQ_SIZE];
    /* The referee's slots */
    struct req      slot[NSLOTS];
    /* The denizens near the character (the denizens' task's, moved by the world), and a save asked of the denizens'
    ** task (REQ_*: its records written to the save folder's dens.dat) */
    struct actor    actor[NACTORS];
    volatile unsigned char den_state;
    volatile unsigned long den_at;              /* The clock as the denizens' task last caught up with it */
    volatile unsigned char den_seq;             /* Bumped as it changes what the character may see of its actors */
    volatile unsigned pc_tx, pc_ty;             /* Where the character's move ends (the world's: no denizen goes
                                                **   there, nor onto the character) */
    /* The denizens' events (E_DEN), to everyone: the denizens' task writes them in their own bank (DEN_BANK), each
    ** reader keeps its own place, as in the world's; nobody waits for them, and each perceives them or not by its own
    ** senses (DENIZENS.md) */
    volatile unsigned dq_head;                  /* Events written so far (the denizens' task's) */
    /* The theme's puzzle (theme.h: the theme in THEME_BANK): the way out opened (the UI's, as the mission's done) */
    volatile unsigned char way_open;
};

#define HUB     ((struct hub*) HUB_WINDOW)
typedef char hub_fits[sizeof (struct hub) <= 0x2000 ? 1 : -1];   /* (The header in bank 0) */
typedef char snd_fits[DQ_SIZE * sizeof (struct msg) + (2 * SND_KINDS + SND_WORLD) * SND_REC <= 0x2000 ? 1 : -1];

/* hub.c */
int __fastcall__ hub_create (unsigned char banks);      /* The UI's: the segment, its doorbells: 0, or -1 */
int __fastcall__ hub_attach (unsigned char seg, unsigned char me);    /* A child's: 0, or -1 */
void __fastcall__ hub_map (unsigned char bank);         /* The hub's bank at $8000 */
unsigned long hub_now (void);                           /* The clock, read whole */
void __fastcall__ hub_ring (unsigned char t);           /* Task t's doorbell rung */
unsigned char __fastcall__ hub_send (const struct msg* m);  /* An intent to the world (the UI's): 1 sent, 0 full */
unsigned char __fastcall__ hub_take (struct msg* m);    /* An intent (the world's): 1, or 0 none */
void __fastcall__ hub_event (const struct msg* m);      /* An event (the world's) */
unsigned char __fastcall__ hub_next (unsigned* place, struct msg* m);  /* The next event after place: 1, or 0 */
void __fastcall__ hub_devent (const struct msg* m);     /* A denizens' event (the denizens' task's) */
unsigned char __fastcall__ hub_dnext (unsigned* place, struct msg* m);  /* The next denizens' event: 1, or 0 */
int __fastcall__ hub_spawn (const char* path, unsigned char t, const char* more);   /* A task of the game's */
int __fastcall__ hub_run (const char* path, const char* args);  /* A program (its arguments each 0-ended, then an
                                                        **   empty one): its task, or -1 (hy_spawn without stdio) */
int __fastcall__ hub_wait (unsigned char task);         /* A task waited for: its status, or -1 */
struct req* __fastcall__ hub_ask (unsigned char slot);  /* A slot's request posted, its reply waited for: the slot
                                                        **   (set REQ_FREE when its reply's taken) */

#endif
