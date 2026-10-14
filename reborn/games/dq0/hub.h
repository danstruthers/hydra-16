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

#define HUB_MAGIC       0x5144                  /* "DQ" */
#define HUB_VERSION     1
#define HUB_BANKS       3                       /* Bank 0: the header and queues; bank 1 on: the dungeon's levels
                                                **   (level.h: four a bank) */
#define HUB_WINDOW      ((unsigned char*) 0x8000)

/* The tasks, and so their doorbells */
enum { T_UI, T_WORLD, T_DENIZENS, T_REFEREE, T_SOUND, NTASKS };

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
    I_NONE, I_WAIT,                             /* w1: tenths (0: a round) */
    I_GO,                                       /* a: a heading (level.h's H_N ... H_NW) */
    I_CLIMB,                                    /* a: DIR_UP or DIR_DOWN (the stairs in the room) */
    I_TURN,                                     /* a: the heading to face (H_N ... H_NW) */
    I_LIGHT,                                    /* a: 1 the character's light lit, 0 put out */
    I_QUIT,
    NINTENTS
};

/* Events: what's happened, in the world's terms (the UI words them; the denizens and the sound hear them) */
enum {
    E_NONE, E_TIME,                             /* Time passed for who: w1 tenths */
    E_READY,                                    /* who can act again */
    E_BYE,                                      /* The world's ending */
    E_ARRIVE,                                   /* who's somewhere new: w1 how (A_*), and 256 the first time there */
    E_NOWAY,                                    /* who can't: w1 why (N_*) */
    E_VIEW,                                     /* What who sees has changed: w1 why (V_*) */
    NEVENTS
};

enum { A_WALKED, A_DOWN, A_UP, A_START };   /* E_ARRIVE's how */
enum { N_NO_EXIT, N_SHUT, N_NO_STAIRS };      /* E_NOWAY's why: no way that way, the way out shut, no stairs */
enum { V_TURNED, V_LIT, V_DARK };               /* E_VIEW's why */

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
    unsigned long   seed;
    char            pcstatus[16];               /* The character's state, the referee's words ("UP 3") */
    volatile unsigned char pc_level, pc_x, pc_y;    /* Where the character is (the world's), */
    volatile unsigned char pc_face;             /*   the way it faces (a heading: H_N ... H_NW), */
    volatile unsigned char pc_light;            /*   the light it carries: how far it reaches, in tiles (0: none) */
    struct rs_summary pcsum;                    /* The character's summary (the referee's: its senses ...) */
    /* The UI's intents to the world */
    volatile unsigned char iq_head, iq_tail;    /* Written at head (the UI), read at tail (the world) */
    struct msg      iq[IQ_SIZE];
    /* The world's events, to everyone: each reader keeps its own place */
    volatile unsigned eq_head;                  /* Events written so far (the world's) */
    struct msg      eq[EQ_SIZE];
    /* The referee's slots */
    struct req      slot[NSLOTS];
};

#define HUB     ((struct hub*) HUB_WINDOW)

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
int __fastcall__ hub_spawn (const char* path, unsigned char t, const char* more);   /* A task of the game's */
struct req* __fastcall__ hub_ask (unsigned char slot);  /* A slot's request posted, its reply waited for: the slot
                                                        **   (set REQ_FREE when its reply's taken) */

#endif
