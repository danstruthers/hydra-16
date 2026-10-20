/*
** hub.h - Dingus Quest Zero's tasks' meeting place: a shared segment (hy_seg_create) the UI makes and the others
** attach to (its number an argument of theirs), its bank 0 the header and the queues between the tasks, its other
** banks the world's state.  Every task keeps it at $8000 (hub_map), but for the moments an overlay's or a bank of
** its own is there (bank.s's stubs and copies put it back).
**
** The tasks (each a program): the UI (dq0: the screen, the keys, the words), the world (dq0w: the clock, the
** dungeon, the rules of the place), the denizens (dq0d), the referee (a program a ruleset: dq0harn ...: every rules
** question), the sound (dq0s), the maker (dq0m), the senses (dq0p), the command parser (dq0c: assembly, a line's
** verdict).  Each queue has one task that writes it and one that reads it, so none needs a lock: an entry is written
** whole, then the index that shows it.  A task waiting for its queues waits on its doorbell, a counting semaphore its
** writers release.
**
** The clock is the world's: now, in tenths of a second of game time; a round of the ruleset's (round tenths) takes
** tpr real ticks (200 a second) at the pace the player chose.
*/

#ifndef _HUB_H
#define _HUB_H

#include <stddef.h>
#include "rs.h"
#include "level.h"
#include "../dq0w/world.h"
#include "items.h"
#include "theme.h"

#define HUB_MAGIC       0x5144                  /* "DQ" */
#define HUB_VERSION     3                       /* (2: the command parser's role and request, dq0c; 3: its meaning) */
#define HUB_BANKS       (1 + PLAN_BANKS + CACHE_BANKS + 1 + ITEM_BANKS + 6 + CHANGE_BANKS + 1)   /* Bank 0: the header, the
                                                **   clock, the doorbells, the rings and the requests' slots; then the
                                                **   plans' banks and the chunks' (world.h); the catalogue's, and the
                                                **   items' (items.h); the denizens' events; the senses' look
                                                **   (LOOK_BANK); the theme (THEME_BANK); the records the tasks share,
                                                **   each bank its keeper's: the senses' (SIGHT_BANK), the UI's
                                                **   (UI_BANK), the world's (WORLD_BANK); the change records' pages
                                                **   (CHANGE_BANK: world.h's, the world's); the commands' text (CMD_BANK:
                                                **   the UI's, the command parser's to read) */
#define CAT_BANK        (1 + PLAN_BANKS + CACHE_BANKS)
#define CATALOGUE       ((struct kind*) 0x8000)
#define ITEM_BANK(i)    (CAT_BANK + 1 + (i) / (0x2000 / sizeof (struct item)))
#define ITEM_AT(i)      ((struct item*) 0x8000 + (i) % (0x2000 / sizeof (struct item)))
#define HUB_WINDOW      ((unsigned char*) 0x8000)
#define DEN_BANK        (CAT_BANK + 1 + ITEM_BANKS)     /* The denizens' events (DQ_SIZE of them, a ring: the world's) */
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
                                                **   places (the maker's), and the names given (NAMES_AT) */
#define NAMES_AT        ((struct tname*) 0x9800)    /* The names the character's given (the UI's: struct tname, TNAMES
                                                **   of them), in the theme bank after its pieces' places */
#define SND_REC         32
#define SND_WORDS       20
#define SND_KINDS       32
#define SND_WORLD       16
#define DTEXT           160                     /* (The denizens' referee slot's text: hub.h's dslot) */
#define NFIGHTS         8                       /* The fights' words (the world's, after the sounds: a ring of them,
                                                **   an E_FIGHT's x[3] which; the UI words what it reads) */
#define FIGHT_AT        ((struct fight*) (SND_AT + (2 * SND_KINDS + SND_WORLD) * SND_REC))
struct fight {
    char            att[12], def[12];           /* The attacker's name and the defender's ("": the character) */
    unsigned        out;                        /* The tenths one knocked senseless is out for */
    unsigned char   which;                      /* Whose a fumble or a stumble was (RS_ATTACK's a) */
    char            text[DTEXT - 27];           /* The referee's words (RS_ATTACK's four lines) */
};

/* A name the character's given (the UI's, dq0/names.c: DUNGEON.md's names), in the theme bank (NAMES_AT): a thing
** ("name my sword Linda": by its id, struct item's, never another's), a room ("call this room the Mausoleum": by
** its floor), an ally (the denizens' task's own name, "name the rat Grub", recorded here too, so no other name takes
** it).  Kept in the save (game.dat's end).  Any task may read it (the theme bank in the window): a room's name the
** way to it (the UI's room_named finds one in a line's words) */
#define TNAMES          48
#define NAME_LEN        20
struct tname {
    unsigned char   what;                       /* NM_* (NM_NONE: a free entry) */
    unsigned char   sn;                         /* A room's: its stratum, */
    unsigned        x, y;                       /*   its floor's northwest square (the frame's: struct sight's rx,
                                                **   ry), a square in it; a thing's: x its id */
    unsigned char   w, h;                       /*   its floor, in squares (struct sight's rw, rh) */
    char            kind[14];                   /* What it is: a thing's kind's name ("sword"), an ally's ("giant
                                                **   rat") */
    char            name[NAME_LEN];             /* The name as it's said ("Linda", "the Mausoleum"): its words
                                                **   capitalised; matched without its "the", the case aside */
};
enum { NM_NONE, NM_ITEM, NM_ROOM, NM_DEN };
typedef char names_fit[TNAMES * sizeof (struct tname) <= 0x800 ? 1 : -1];  /* (The theme bank's last 2K) */

/* The records the tasks share, out of bank 0 (it keeps the header, the clock, the doorbells, the rings and the
** requests' slots), each in a bank of its own at $8000: a reader copies out what it needs (hub_get's, or its own
** hub_mem ...: the bank in the window, the bytes copied, the header back), so no pointer into one outlives the next
** hub_map; a writer puts it back the same way, or writes it in place with the bank in the window and nothing else
** read meanwhile */
#define SIGHT_BANK      (THEME_BANK + 1)        /* The senses': what the character perceives (struct sight: the
                                                **   senses' after each look about, the denizens it sees the world's) */
#define SIGHT           ((struct sight*) HUB_WINDOW)
#define UI_BANK         (SIGHT_BANK + 1)        /* The UI's (struct uirec): the map, the things' words */
#define UIREC           ((struct uirec*) HUB_WINDOW)
#define WORLD_BANK      (UI_BANK + 1)           /* The world's (struct wrec): the strata's records, the chunks' cache's
                                                **   table */
#define WREC            ((struct wrec*) HUB_WINDOW)
#define CHANGE_BANK(p)  (WORLD_BANK + 1 + (p) / 32)    /* The change records' page p's bank (world.h's CHANGE_AT: the
                                                **   world writes them, under the hub's change_lock) */
#define CMD_BANK        (WORLD_BANK + 1 + CHANGE_BANKS)    /* The commands' text (the hub's cq: a line, or the world's
                                                **   words, at $8000: the UI writes it, dq0c reads it) */

/* The tasks, and so their doorbells */
enum { T_UI, T_WORLD, T_DENIZENS, T_REFEREE, T_SOUND, T_MAKER, T_SENSES, T_COMMANDS, NTASKS };

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
    I_GO,                                       /* a: a heading (level.h's H_N ... H_NW); w1, w2: the door or the
                                                **   passage's mouth that way meant (a square it sees: 0, any) */
    I_CLIMB,                                    /* a: DIR_UP or DIR_DOWN (the stairway or ladder in the room);
                                                **   | I_JUMP, down: jumped (a ledge, a shaft: a fall) */
    I_TURN,                                     /* a: the heading to face (H_N ... H_NW) */
    I_LIGHT,                                    /* a: 1 a light of the character's lit, 0 put out (its light the
                                                **   things' already, pc_light: the world takes the time, looks) */
    I_DIG,                                      /* a: a direction (DIR_N ... DIR_W, DIR_DOWN) */
    I_PORTAL,                                   /* a: what's done to the door it faces (PA_*); b: which (PW_*: a
                                                **   door or a portcullis, a window or a slit, either); w1, w2: the
                                                **   one meant (its square: 0, the one it faces); x[0]: PA_BAR's bar
                                                **   to hand, its width (the things': 0, none); x[1] what it strikes
                                                **   a thing with (SK_*: the hub's pc_strike, the world's to set) */
    I_GOTO,                                     /* w1, w2: a square in its room it sees, walked up to */
    I_STEP,                                     /* w1: a heading (H_N ... H_NW), | STEP_KEEP its facing kept; w2:
                                                **   squares */
    I_QUIT,                                     /* The game's end (the UI's: the hub's quit, the world's to set; every
                                                **   task ends, the world first) */
    I_ATTACK,                                   /* a: the actor struck at (its entry plus 1), b: the aim (Z_*): a
                                                **   round's (the world's: the character busy for it, the blow posted
                                                **   in the hub's pc_foe, and struck: the referee asked, E_FIGHT); x[1]
                                                **   what it strikes with (SK_*: the hub's pc_strike) */
    I_ENTER,                                    /* (The denizens' own, below) A body come into the world's keeping */
    I_LEAVE,                                    /*   gone from it (no longer an actor) */
    I_ROUSE,                                    /*   its wait cut short */
    I_DONE,                                     /*   the denizens' turn done (the turns pace's: the hub's den_go) */
    I_TAKE,                                     /* A thing taken up, walked over to first (the UI's, the thing out of
                                                **   reach: IO_GET's IR_NOT_HERE): w1, w2 the square it lies on (as
                                                **   I_GOTO's), t the thing (its item: the reply's); the world walks
                                                **   there (again, as long as the walk gets it nearer) and takes it up,
                                                **   E_TAKEN */
    NINTENTS
};
/* The denizens' intents (their ring, the hub's di: the minds', for their actors' bodies, the world's to carry out
** by the character's rules): who the actor's entry plus 1; x[0] what it's doing now (DO_*: the world keeps it, and
** tells as one notices the character or loses it), x[1] the heading it faces (0xFF: either, as it is).  I_STEP: a
** square (w1 the heading, H_*, | STEP_COARSE: no time, nothing heard, a coarse move's; w2 1), a what it's heard as
** (DE_MOVED, DE_NOISE), b how loud; I_GOTO: a dash, to w1 w2 (two squares off at most: past the character), a and b
** as I_STEP's; I_CLIMB: come along with the character up or down, to w1 w2 beside it, x[2] its stratum; I_WAIT: w1
** tenths; I_ATTACK: a the one struck at (0 the character), b the aim (Z_*); I_ENTER: w1 w2 its square, x[2] its
** stratum, a its step, b 1 if it's bleeding, t when it's ready; I_LEAVE; I_ROUSE: free w1 tenths from now (w2 1:
** whenever it was free; 0: if it was later); I_DONE (who 0): w1 the turn it's done (the hub's den_go: the turns
** pace's, the world going on once it's in), t the clock then */
#define STEP_COARSE     0x200

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
    E_FIGHT,                                    /* (The denizens' too, their queue's) A blow struck: who the attacker
                                                **   (0 the character, else the actor plus 1), a the defender (the
                                                **   same), b how loud, w1 w2 the attacker's square, x[0] its stratum,
                                                **   x[1] how it came out (O_*; FO_GONE: the one struck at out of
                                                **   reach), x[2] what it left them in (FF_*), x[3] its words (the
                                                **   ring's: FIGHT_AT) */
    E_LIGHT,                                    /* A light or an obstruction changed (the senses' light layer out of
                                                **   date near it: DUNGEON.md's perception as layers): a door opened
                                                **   or shut, a square dug (the world's change records), a its
                                                **   stratum, w1 w2 its square */
    E_TAKEN,                                    /* who's walked over to a thing and taken it up (I_TAKE's): w1 how it
                                                **   went (items.h's IR_*: IR_OK taken, its time and its light the UI's
                                                **   to ask, as for any thing handled, no E_READY till then; IR_FIXED a
                                                **   portal's part, or a thing fixed where it stands, the UI's to see
                                                **   to), w2 the light it carried before (the hub's pc_light) */
    NEVENTS
};
enum { DE_MOVED, DE_NOISE, DE_NOTICED, DE_LOST };    /* E_DEN's what: a step, a noise of its own (a squeak, a
                                                **   growl), the character sensed, lost (its senses' own) */
enum { SK_BEST, SK_PUNCH, SK_KICK, SK_HEAD };   /* A blow struck with (the hub's pc_strike): the best to hand (the
                                                **   best weapon held at a denizen, the tool that hits hardest at a
                                                **   thing; else the fists), a fist, a foot, the head */
#define FO_GONE         0xFF
#define FO_BLED         0xFE                    /* (E_FIGHT's x[1]: no blow, but the attacker dead of its bleeding) */
#define FF_A_DOWN       0x01                    /* E_FIGHT's x[2]: the attacker knocked senseless, */
#define FF_A_DEAD       0x02                    /*   dead, */
#define FF_D_DOWN       0x04                    /*   the defender senseless, */
#define FF_D_DEAD       0x08                    /*   dead, */
#define FF_FLEES        0x10                    /*   a denizen in it fleeing now, */
#define FF_PRONE        0x20                    /*   one fallen (a stumble), */
#define FF_STOPPED      0x40                    /*   a strike stopped by armour (rs.h's AF_STOPPED, AF_STOPPED_D), */
#define FF_FUMBLE       0x80                    /*   a weapon fumbled */
#define I_JUMP          0x80
#define STEP_KEEP       0x100                   /* (I_STEP's: backing up, a step aside) */
#define E_OUT           0x4000                  /* (E_HARM's: knocked senseless a while) */
#define E_CAUGHT        0x8000                  /*   (and a fall stopped, a hold grabbed) */
#define E_STOPPED       0x8000                  /*   (an HC_STRUCK's: the blows stopped, the striker too hurt) */
#define O_FELL          8                       /* (E_ARRIVE's link kind, or'd: fallen down it, jumped) */

enum { A_WALKED, A_DOWN, A_UP, A_START, A_DIGGING, A_CROSSED, A_CLIMBING, A_CLIMBED, A_WENT_TO, A_RESUMED,
       A_STOPPED, A_STEPPED };                  /* E_ARRIVE's how (A_DIGGING, A_CLIMBING: the world's own, a spell
                                                **   of work, a round's climbing; A_STOPPED: on its way, at a trap;
                                                **   A_STEPPED: a small move, I_STEP's) */
enum { N_NO_EXIT, N_SHUT, N_NO_STAIRS, N_NO_DIG, N_CLOSED, N_DEN, N_DOOR };    /* E_NOWAY's why: no way that way, the way out
                                                **   shut, no stairs, nothing to dig there, the portcullis that way
                                                **   down, a denizen in the way (the actor times 256) */
enum { V_TURNED, V_LIT, V_DARK, V_STIR, V_FACING };    /* E_VIEW's why (V_STIR: a denizen come into sight, gone
                                                **   from it, or doing something else; V_FACING: a turn begun, its
                                                **   look to come, V_TURNED) */
enum { D_BEGUN, D_GOOD, D_POOR, D_SLIP, D_THROUGH, D_SOLID, D_NO_TOOL };  /* E_DIG's how */
enum { K_BEGUN, K_ON, K_BACK, K_FELL, K_UP, K_DOWN, K_JUMPED, K_NOHOLD };    /* E_CLIMB's how: begun, on, slipped
                                                **   back, fallen, up, down; jumped down (a ledge, a shaft); no hold
                                                **   found yet (at its foot: nothing to slip back, nor to fall) */
enum { PA_OPEN, PA_CLOSE, PA_UNLOCK, PA_FORCE, PA_SEARCH,     /* I_PORTAL's: what's done (a door's opened, ... */
       PA_RAISE, PA_LOWER, PA_BREAK, PA_THROUGH, PA_DISARM,    /*   a portcullis raised, a window's glass broken,
                                                **   climbed through, a trap disarmed: one in reach, or the door's; */
       PA_LIFT,                                 /*   a door's bar lifted out, a window's pane or its bent bars; */
       PA_EXAMINE,                              /*   looked at closely: the UI's own (the hub's wreq, WQ_EXAMINE:
                                                **   its record copied into the hub's pview); */
       PA_BAR, PA_BEND };                       /*   a door barred again (a bar carried, or in reach, set in its
                                                **   brackets); bars bent (a portcullis's; a window's, forced) */
enum { WQ_SAVE, WQ_EXAMINE, WQ_START, WQ_SET }; /* The hub's wreq's kinds */
enum { WS_PACE, WS_STANCE, WS_PAUSE };          /* WQ_SET's what: the character's pace (x, GA_*: 0xFF as it is) and its
                                                **   watchfulness (y, WA_*); its stance (x, rs.h's D_*); the game
                                                **   paused (x 1) or going on (x 0) */
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
       PO_WRECKED, PO_LIFTED, PO_BAR_FAR, PO_NO_BAR, PO_SET_FAST, PO_PANE_OUT, PO_BARS_OUT, PO_BARS_BROKEN,
                                                /*   broken down: it won't shut; a bar lifted out, one on the far
                                                **   side, none to lift; bars set fast; a pane worked out, bent bars
                                                **   lifted out, bars broken out; */
       PO_REBARRED, PO_NO_BRACKETS, PO_IS_BARRED, PO_SHUT_FIRST, PO_BURST, PO_NO_BAR_HELD, PO_BAR_SHORT,
                                                /*   a bar set in its brackets again, none this side, barred already,
                                                **   open, burst open once (it won't bar), no bar to hand, one too
                                                **   short to span it; */
       PO_GAP, PO_JAMMED };                     /*   a portcullis's bars broken through, its bars forced apart (it
                                                **   won't move) */
enum { TO_SPRUNG, TO_DODGED, TO_RESISTED, TO_CHOKED, TO_OPENED, TO_NOTICED, TO_FOUND, TO_DISARMED, TO_NOT_YET,
       TO_FUMBLED, TO_CANT, TO_PIT, TO_NONE, TO_ASSESSED, TO_PUZZLED };  /* E_TRAP's: set off, dodged, its gas resisted or choked on, a pit's
                                                **   cover fallen in (no one on it); noticed in passing, found by a
                                                **   search; disarmed, not yet, set off disarming it, no means to
                                                **   (magic), a pit (gone round, not disarmed), none in reach */

/* The requests between the tasks: one protocol.  Each request is in bank 0, its owner's slot (the referee's slots,
** the maker's mreq, the world's ireq and stock, the senses' look, the denizens' sway and dsave, the maker's mapq),
** and begins with the same header (struct rqh: level.h's RQH).  The asker writes the request, then hub_post: from
** (itself), its seq, state REQ_POSTED, and the owner's doorbell rung; then hub_wait: asleep on its own doorbell
** (never spinning) till the reply's there (a ring for something else, taken meanwhile, rung again once it's there).
** The owner, rung, answers what's posted (hub_posted), then hub_reply: REQ_DONE, and the asker's doorbell rung.  The
** asker takes the reply and sets REQ_FREE.  A task that asks on another's behalf, in its slot, while that one waits
** (the maker in the world's referee slot, the world's things in the UI's) is the one rung */
#define REQ_FREE        0
#define REQ_POSTED      1
#define REQ_DONE        2
struct rqh {
    RQH;
};

/* The command parser's request (dq0c, games/dq0c: assembly, its offsets games/dq0c/hub.inc's, held to these below):
** the UI's.  Its text is in CMD_BANK at $8000 (len bytes, a 0 after): a line to match (CQ_MATCH), the world's words
** (CQ_WORLD: three lists, each 0-ended: the words the world names things by, the names given, the deities'), or
** nothing (CQ_NONE: the round trip alone, timed).  The reply's res: 1 the line parses, 0 it doesn't, -1 too long
** or nesting too deep; 0 for the others.  A line's reply has what it means too: its struct pcmd (dq0.h's, PCMD_SIZE
** bytes) in CMD_BANK at $8000 + CQ_PCMD, made as the character faces face (its ways as it faces kept so: CQ_REL | n,
** the UI's to make the compass's as the command's carried out; but one with a way by the compass, made by face) */
enum { CQ_NONE, CQ_MATCH, CQ_WORLD };
#define CQ_WORDS        1024                    /* The world's words' text, at most (dq0c keeps that much) */
#define CQ_PCMD         0x1000                  /* (Past the text) */
#define PCMD_SIZE       41
#define CQ_REL          0x10                    /* A way as the character faces (CQ_REL | 0 ahead, 2 its right ...) */
struct cq {
    RQH;                                        /* (kind: CQ_*) */
    unsigned char   bank;                       /* CMD_BANK (the UI's, as it makes the hub: dq0c finds it here) */
    unsigned        len;                        /* The text's bytes */
    signed char     res;                        /* The reply's */
    unsigned char   face;                       /* A line's: the way the character faces (H_N ... H_NW) */
};

/* A referee slot: a request (rs.h's RS_*) and its reply, and the reply's words.  Slot 0 is the world's (its
** requests change things), slot 1 the UI's; the denizens' task's its own (dslot: DSLOT, DTEXT of its text) */
#define REQ_TEXT        1024
struct req {
    RQH;                                        /* (kind: RS_*) */
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

/* An actor: a denizen near the character, a mind and a body.  Its mind is the denizens' task's (dq0d: its records
** its own, in its own banks): it decides what the actor means to do, and says so (its intents, the hub's di: hub.h's
** the denizens' intents).  Its body is the world's, as the character's is: the world carries out its intents by the
** character's rules (a square at a time, its time added to ready; a blow, the referee asked), keeps the character
** out of its way, adds it to what the character perceives, tells what it does (the denizens' events), and bumps
** den_seq as it changes what the character may see of them.  At the turns pace the world moves the clock on through
** what the character does a slice at a time (TURN_SLICE tenths), and gives the denizens their turn (the hub's
** den_go, rung) at each, and at the same clock after work of its own (events told: the character arrived, its work
** done); they act only then, and say when they've done (I_DONE), the world carrying out their intents meanwhile and
** going on once it's in: the same game whatever the tasks' timing */
#define NACTORS         8
#define TURN_SLICE      100
struct actor {
    /* Its body (the world's) */
    volatile unsigned char state;               /* 1: a body in the dungeon (from the denizens' I_ENTER till it's
                                                **   gone: I_LEAVE, or dead); 0 none */
    unsigned char   sn, face;
    unsigned long   ready;                      /* When it can act again */
    unsigned char   step;                       /* Tenths of a second to go 5 feet */
    unsigned char   pos;                        /* Its body's position (POS_*: standing, prone ...) */
    unsigned char   gait;                       /* The pace it goes at (rs.h's GA_*: a walk, till their paces come) */
    unsigned        x, y;                       /* (These, its doing and its name as struct sight's den: copied whole) */
    unsigned char   doing;                      /* What it's doing (DO_*: watching, coming, keeping off: it senses the
                                                **   character) */
    /* Its mind (the denizens') */
    char            name[12];                   /* ("": the entry free) */
    unsigned char   den;                        /* Its record (the denizens' task's) */
    unsigned char   attitude;                   /* Its attitude to the character (AT_*, its record's: the denizens'
                                                **   task's; a hostile one fights it) */
    unsigned char   faction;                    /* Its faction (the denizens' task's: kinds.txt's, the theme's) */
    unsigned char   foe;                        /* An actor of a faction hostile to its own, met (near, sensed): it
                                                **   plus 1, or 0 (till they fight, they keep off one another) */
    unsigned char   kind;                       /* Its kind (kinds.txt's, by number: the sounds' table's, SND_AT) */
    unsigned char   remains;                    /* What it leaves dead (items.h's KI_BODY, KI_BONES) */
    unsigned char   timid;                      /* 1: a shy kind (the referee's: shaken, it flees) */
};
enum { AT_HOSTILE, AT_UNFRIENDLY, AT_WARY, AT_INDIFFERENT, AT_FRIENDLY, AT_ALLIED };   /* An attitude, the worst
                                                **   first (DENIZENS.md): attacks on sight; threatens, keeps off;
                                                **   watches, won't help; minds its own business; talks, helps; comes
                                                **   with the character, takes its requests */

/* The character's way with a denizen (the UI asks the denizens' task: the hub's sway, REQ_*, its doorbell; the
** denizens' task finds the one meant, among those the character sees, by its name or its kind's, and asks the
** referee in the UI's slot, lent it while the UI waits): words to sway it (the referee's test, by the kind's mind:
** an animal's by Animalcraft alone; words want a tongue in common), a request to an ally, a name given to one, a
** look at how it seems, a blow (a stand-in for the fights) */
enum { SW_LOOK, SW_TALK, SW_PERSUADE, SW_DECEIVE, SW_THREATEN, SW_CALM, SW_BEFRIEND, SW_ATTACK, SW_ORDER, SW_NAME };
                                                /* (dq0.h's V_TALK ... V_NAME in the same order) */
enum { OR_NONE, OR_FOLLOW, OR_WAIT, OR_HOME, OR_FETCH };   /* An ally's request (SW_ORDER's): follow the character,
                                                **   wait where it is, go back to its own ways, fetch a thing */
enum { SR_OK, SR_NONE, SR_FAR, SR_TONGUE, SR_BEAST, SR_MINDLESS, SR_AS_FAR, SR_NOT_ALLY, SR_CANT };   /* A sway's
                                                **   result: done (a test's: grade, was, now), none such in sight,
                                                **   too far (an animal's out of reach), no tongue in common, an
                                                **   animal (words don't reach it), mindless, as far as that way
                                                **   goes, not an ally, it can't (yet) */
struct sway {
    RQH;                                        /* (kind: SW_*) */
    char            word[16];                   /* The one meant (a name, a kind's word; "": the nearest, or every
                                                **   ally, an order's) */
    unsigned char   prep;                       /* (dq0.h's struct pcmd's, from word on, copied whole) */
    char            word2[16];                  /* A name given; a thing to fetch */
    unsigned char   order;                      /* OR_* (SW_ORDER's) */
    unsigned char   result;                     /* (The reply's:) SR_* */
    signed char     grade;                      /*   the referee's (G_*: 0, no test) */
    unsigned char   was, now;                   /*   its attitude before and after (AT_*) */
    unsigned char   beast;                      /*   its mind: 1 an animal */
    char            name[12];                   /*   its name (an ally's own, capitalised; else its kind's) */
    unsigned        t;                          /*   the tenths it took the character */
};

#define IQ_SIZE         16                      /* The UI's intents */
#define DI_SIZE         8                       /* The denizens' (their actors': hub.h's struct actor) */
#define EQ_SIZE         128                     /* The world's events (each reader its own place) */

struct hub {                                    /* Bank 0, at $8000 */
    unsigned        magic;
    unsigned char   version;
    unsigned char   seg;
    unsigned char   sem[NTASKS];                /* Each task's doorbell */
    unsigned char   task[NTASKS];               /* Each task's number (0: none) */
    unsigned char   beat[NTASKS];               /* Each task's heartbeat: bumped as it works */
    struct cq       cq;                         /* The command parser's request (dq0c's: hub.inc's offsets) */
    /* The clock (the world's: tick, then now, then tick again, so a reader sees it whole: hub_now) */
    volatile unsigned char tick;
    volatile unsigned long now;
    volatile unsigned long pc_ready;            /* When the character can act again */
    unsigned        round;                      /* The ruleset's round, tenths of a second */
    unsigned        tpr;                        /* Real ticks a round, at the pace (0: turns) */
    volatile unsigned char paused, hold, quit;  /* Paused (Escape: the UI's request, WQ_SET), held (a question), ending
                                                **   (the UI's I_QUIT): the world's */
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
                                                **   the world's, from the lit things it holds: its item store's), */
    char            pc_lamp[14];                /*   and the brightest of them, named */
    struct rs_summary pcsum;                    /* The character's summary (the referee's: its senses ...) */
    /* A look about asked of the senses (dq0p: the world asks, as the character's moved, turned, lit a light; kind
    ** LK_*): the squares it sees marked, what it perceives recorded in the senses' bank's sight, the denizens it sees
    ** with it (the world's bodies, the senses deciding which are seen); the senses' memory theirs to keep (the
    ** squares seen, a room come into, R_VISITED: DUNGEON.md's perception as layers) */
    struct {
        RQH;
        unsigned char first;                    /* The first time there (| LF_ARRIVE: come into a room, the senses
                                                **   to say if it's the first time, by their memory; the reply's) */
        unsigned char ch;                       /* (The reply's: 1 a denizen come into sight, gone, doing otherwise) */
        unsigned    wx, wy;                     /* A walk's start (the squares walked felt, remembered: LK_LOOK's), */
        unsigned char wd, wn;                   /*   its first way (DIR_*) and its squares (0: none) */
    }               look;
    /* The map: the UI asks the maker, which draws its window round the character into the UI bank's map (each row a
    ** line, 0-ended) */
    struct rqh      mapq;
    /* The dungeon (world.h): the maker's requests (the strata's records and the cache's table the world bank's) */
    struct mreq     mreq[NASK];
    /* The things (items.h: the item store, the world's alone): a request of them (the UI's: what's carried, a thing
    ** handled ...), their words for the UI (what's carried); and a chunk's things put down (the senses' request, as a
    ** look comes to one the maker's stocked and the world hasn't yet: the world bank's stock) */
    struct ireq     ireq;                       /* (Their words the UI bank's things_text) */
    struct rqh      stock;
    struct portal   pview;                      /* A portal looked at closely (PA_EXAMINE's: the world's record of
                                                **   it, but bar: 1 the character's on its bar's side) */
    /* The change records' lock (a semaphore: the world writes them, the maker reads them: world.h's), and a request
    ** to the world (the UI's: WQ_*): the game kept (WQ_SAVE: the change records, and the world's own, world.dat,
    ** written to the save folder); a portal looked at closely (WQ_EXAMINE: x, y its square on the character's stratum,
    ** its record into pview; res 1 if there's none there); the game begun (WQ_START, before the UI's things: x the
    ** ruleset's round, y the pace, a and b a new game's pace and watchfulness, the character's file's: the world
    ** sets the clock's, and a game carried on, the hub's resume, is the save's world.dat's); the character's state
    ** or the game's set (WQ_SET: a what, WS_*, x and y its values) */
    unsigned char   change_lock;
    struct {
        RQH;
        unsigned    x, y;
        unsigned char res;
        unsigned char a, b;
    }               wreq;
    char            dig_tool[4][8];             /* A dig's spells' tools, the referee's choices (their keys), in turn */
    unsigned char   dig_wear[4];                /*   and the wear each spell did (E_DIG's b, which: the world runs on */
                                                /*   ahead of the UI's reading of its events, the turns pace) */
    /* The UI's intents to the world */
    volatile unsigned char iq_head, iq_tail;    /* Written at head (the UI), read at tail (the world) */
    struct msg      iq[IQ_SIZE];
    /* The world's events, to everyone: each reader keeps its own place */
    volatile unsigned eq_head;                  /* Events written so far (the world's) */
    struct msg      eq[EQ_SIZE];
    /* The referee's slots: the world's, the UI's, and the denizens' task's (a short one: DTEXT of its text) */
    struct req      slot[NSLOTS];
    unsigned char   dslot[sizeof (struct req) - REQ_TEXT + DTEXT];
    /* The denizens near the character (their minds the denizens' task's, their bodies the world's), and a save asked
    ** of the denizens' task (its records written to the save folder's dens.dat) */
    struct actor    actor[NACTORS];
    struct rqh      dsave;
    volatile unsigned char den_seq;             /* Bumped as the world changes what the character may see of the
                                                **   actors */
    volatile unsigned pc_tx, pc_ty;             /* Where the character's move ends (the world's: no denizen goes
                                                **   there, nor onto the character) */
    /* The denizens' events (E_DEN, E_FIGHT: what their bodies do), to everyone: the world writes them in their own
    ** bank (DEN_BANK), each reader keeps its own place, as in the world's; nobody waits for them, and each perceives
    ** them or not by its own senses (DENIZENS.md) */
    volatile unsigned dq_head;                  /* Events written so far (the world's) */
    /* The theme's puzzle (theme.h: the theme in THEME_BANK): the way out opened (the world's, as it judges a command
    ** of the puzzle's by the theme's rules: items.h's IO_PUZZLE) */
    volatile unsigned char way_open;
    struct sway     sway;                       /* The character's way with a denizen (the UI's request) */
    /* The character's pace and watchfulness (DUNGEON.md: moving; the world's, set as the player says, the UI's request
    ** WQ_SET, and kept with the game, world.dat; the referee's to word, RS_DESCRIBE b 3), the pace it's going at now
    ** (the world's: as a move starts, till it's there and has looked about), and its body's position (the world's, as
    ** the referee says it's fallen, or got up: POS_*) */
    unsigned char   gait;                       /* rs.h's GA_* */
    unsigned char   watch;                      /* WA_* */
    unsigned char   pc_pos;                     /* POS_* */
    volatile unsigned char pc_going;            /* The pace it's going at plus 1 (0: not moving), | GOING_DARK: by
                                                **   feel */
    /* A fight: the character's blow, posted by the world (I_ATTACK) as it takes the round for it, and struck (the
    ** referee asked, E_FIGHT told), and the character's stance (rs.h's D_*: the referee's defence for it) */
    volatile unsigned char pc_foe;              /* The actor struck at, plus 1 (0: none posted) */
    unsigned char   pc_aim;                     /* Its aim (Z_*) */
    unsigned char   pc_stance;                  /* (The world's: the UI's request, WQ_SET) */
    unsigned char   pc_strike;                  /* What it strikes with (SK_*: the parser's, as it's told: the world's,
                                                **   from the blow's intent, I_ATTACK's, I_PORTAL's x[1]) */
    /* The denizens' intents to the world (their actors' bodies': hub.h's the denizens' intents), a ring: written at
    ** di_head (the denizens' task: a full one waited on), read at di_tail as each is carried out (the world: it rings
    ** the denizens as it has) */
    volatile unsigned char di_head, di_tail;
    struct msg      di[DI_SIZE];
    volatile unsigned char den_go;              /* The denizens' turn, at the turns pace (the world's: bumped as it
                                                **   gives them one, a slice's or its own work's) */
    /* An actor's senses (DUNGEON.md's perception as layers: the senses answer for every actor; the denizens ask, for
    ** their minds, as one decides): whether it senses the character, seen by the light layer (the character lit,
    ** by its own light or where it stands: the kind's sight, else its darkvision, the less as its eyes are still
    ** used to more light than is there, its eyes' adapting the senses' to keep at the kind's rate; a line clear),
    ** heard (as it works, within the kind's hearing); and how far it may move with the answer standing (a far one's:
    ** the coarse answer, no line cast) */
    struct {
        RQH;
        unsigned char who;                      /* The actor (its entry) */
        unsigned char sight, dark, hear, eyes;  /* Its kind's (kinds.txt's): squares it sees the character lit, and
                                                **   unlit; it hears it; its eyes' adapting to the dark (tenths to a
                                                **   16th of a level darker; 0: at once) */
        unsigned char res;                      /* (The reply's) SN_* */
        unsigned char slack;                    /* (The reply's) squares it may go, the answer standing (0: none) */
    }               dsense;
    volatile unsigned char ui_cmd;              /* The UI's: a line taken and its command in hand (its words not yet
                                                **   all out): the senses' look gives way to it a while (dq0prompt:
                                                **   the UI answering at once, the world's answers its) */
    /* The save folder's region files' versions (world.h's struct vfh: DUNGEON.md's no fixed size), the world's, as it
    ** begins a game and as it keeps one (WQ_SAVE, the save's last step): every file stamped up to vsaved is the last
    ** save's, every one stamped past vbase (this session's floor, the stamps an earlier session may have given at
    ** most) this session's, any other a session's that ended without its save (a crash): never read.  A writer (the
    ** world, the denizens, the maker) stamps a file past vlo, up to vhi (vfile.c's) */
    unsigned long   vsaved, vbase, vlo, vhi;
    unsigned char   small;                      /* dq0 -C (the tests'): the caches small, so a short game pages them */
};
#define SN_SEEN         0x01                    /* (dsense's res: the character seen, */
#define SN_HEARD        0x02                    /*   heard) */
#define WA_TRAPS        0x01                    /* Searching as it goes: traps underfoot, hidden doors beside */
#define WA_LISTEN       0x02                    /* Listening as it goes */
#define WA_MAP          0x04                    /* Mapping as it goes (the map's side the orientation's: its
                                                **   materials to hand) */
#define GOING_DARK      0x80
enum { POS_STANDING, POS_PRONE, POS_CROUCHED, POS_KNEELING, POS_SITTING };   /* A body's position (the hub's pc_pos,
                                                **   an actor's pos: the world's; the referee's to judge by, and to
                                                **   say it's fallen, rs.h's RS_ATTACK; crouched, kneeling and
                                                **   sitting to come) */

enum { LK_LOOK, LK_DENS, LK_EYES };              /* The look's kinds: a look about; the denizens seen looked at again;
                                                **   the eyes only, as time's gone (the lights seen far off: a wait's) */
#define LF_ARRIVE       0x40                    /* (The look's first: arrived, R_VISITED the senses') */

/* The UI's bank's records (UI_BANK) */
struct uirec {
    char            map[MAP_H][MAP_W + 1];      /* The map's rows (the maker's, as the UI asks: mapq) */
    char            things_text[400];           /* The things' words for the UI (what's carried: ireq's reply), and
                                                **   the UI's to the things (dq0 -k's: the things more) */
};

/* The world's bank's records (WORLD_BANK): the dungeon's (world.h) */
struct wrec {
    struct stratum  strata[MAXSTRATA];          /* The strata's records (each task keeps a copy: sync_strata) */
    struct cslot    cslot[NCSLOTS];             /* The chunks' cache's table */
    unsigned        stamp;
    unsigned char   inuse[NASK];                /* The slot each task that reads the chunks is reading (the maker
                                                **   never makes way for it) */
    struct cpage    cpage[NCPAGES];             /* The change records' pages (world.h's: the world's) */
    unsigned        cstamp;                     /*   their regions' clock, */
    unsigned        cgen;                       /*   and the game's generation (a region's file of another game's
                                                **   in the folder isn't read) */
    struct stratum  grown;                      /* A stratum's record as the maker's grown its plan (M_PLAN's: the
                                                **   world writes it into strata) */
    struct stock    stock[NCSLOTS];             /* Each slot's chunk's things, as the maker stocked it (items.h's: its
                                                **   slot CS_FRESH, the world puts them down) */
};

/* The world's part of the save (DUNGEON.md: the save; save version 8 on): world.dat in the save folder, the world's to
** write (WQ_SAVE) and read back (WQ_START, the hub's resume): this record, then its strata's records (struct wrec's
** strata) */
#define WSAVE_MAGIC     0x5744                  /* "DW" */
struct wsave {
    unsigned        magic;
    unsigned char   version;                    /* (1) */
    unsigned char   sn, face, pos;              /* The character's place (its stratum, its facing, its body's
                                                **   position), */
    unsigned        x, y;                       /*   its square, */
    unsigned char   gait, watch;                /*   its pace and watchfulness; */
    unsigned char   open, done;                 /* the theme's puzzle: the way out opened, its rules done (a bit
                                                **   each); */
    unsigned long   now;                        /* the clock */
};

/* Who writes what (Phase 2: one authority on state).  Each region of the hub has an owner, the one task that writes
** it; another changes it by asking the owner (an intent, a request), never by writing it.  The test runner holds the
** game to it: every write to the hub's banks in a dq0 test, by the task that made it, checked against these lines
** (sim/lib/hubown.js reads them; sim/test.js, HUBOWN=1 for every violation and a summary).  A line, its words apart:
**   own REGION OWNER [+TASK ...] [2.N:TASK,...] (words)
** REGION: hub.FIELD (bank 0's struct hub; FIELD as offsetof takes it, [*] each element), sight.FIELD, uirec.FIELD,
** wrec.FIELD (their banks' records), chunk.FIELD (each slot of the chunks' cache), item.FIELD (each item), and
** bank(B), bank(B,FROM,TO) (bank B's whole window, or its bytes from FROM up to TO), banks(B,N) (N banks from B):
** C's expressions, no spaces.  A line laid over those before it.  OWNER, TASK: ui world denizens referee sound maker
** senses, or each (element i of a [*] the task T_* i's), or none.  +TASK: one that writes it by design (a request's
** asker: the protocol's header and its request; a ring's reader: its own place; a chunk's reader: the cache's
** stamps).  2.N:TASK: one that writes it still, till Phase 2's move 2.N makes it the owner's alone: a write that
** isn't the owner's, a + task's or a known one fails the test, and the moves shrink the known ones to none.  The
** UI's hub_create (the hub's banks cleared, its header made) and a task's own task[] entry aren't counted.
** The moves (the plan's): 2.2 the maker made pure (done: M_WORK and the change records the world's, the plans'
** strata records the world's to write back, from the maker's wrec.grown); 2.3 the item store the world's (done: the
** items' banks, the catalogue, pc_light and pc_lamp; a chunk's things the maker's list, wrec.stock, a body the
** denizens' intent); 2.4 bodies and blows (done: the actors' bodies the world's, the denizens their minds, saying
** what each means to do, their intents; den_seq, pc_foe, every blow struck, the denizens' events and the fights'
** words in DEN_BANK);
** 2.5 the senses for every actor, by layers, and their memory (done: the chunks' tiles' seen and visible marks and
** R_VISITED the senses', the world's changes to a square keeping them; the denizens seen the senses' to decide, in
** sight; each denizen's senses asked of them, dsense; DUNGEON.md's perception as layers); 2.6 game logic out of the UI
** (done: way_open, the theme's puzzle judged by the world, IO_PUZZLE; a thing walked over to and taken up, I_TAKE); 2.7
** the character's state home (done: gait, watch, pc_stance, pc_strike, the UI's requests, WQ_SET, and the blow's
** intents; the body's position the world's, pc_pos and each actor's; the game begun, WQ_START: round, tpr, pace; a
** game carried on the world's to read back, its world.dat: now, the character's place, the strata's records; paused,
** WQ_SET; quit, I_QUIT).  The known ones are none: Phase 2's goal for the writer check.
**
**   own bank(0) none                                           (The header's room: what no line below names)
**   own hub.magic ui
**   own hub.version ui
**   own hub.seg ui
**   own hub.sem ui
**   own hub.task[*] each                                       (Each task's own number, as it attaches)
**   own hub.beat[*] each
**   own hub.cq commands +ui                                    (The command parser's: the UI's request)
**   own hub.tick world                                         (The clock: the world's alone)
**   own hub.now world
**   own hub.pc_ready world
**   own hub.round world                                        (The game begun: the UI's WQ_START)
**   own hub.tpr world
**   own hub.paused world                                       (The UI's pause and quit: its requests, WQ_SET, I_QUIT)
**   own hub.hold world
**   own hub.quit world
**   own hub.pace world
**   own hub.test_start ui                                      (The game's start: dq0's arguments)
**   own hub.resume ui
**   own hub.savedir ui
**   own hub.seed ui
**   own hub.pcstatus referee                                   (The referee's words)
**   own hub.pc_stratum world                                   (The character's body: the world's; a game carried
**                                                                on, its world.dat's)
**   own hub.pc_x world
**   own hub.pc_y world
**   own hub.pc_face world
**   own hub.pc_light world
**   own hub.pc_lamp world
**   own hub.pcsum referee
**   own hub.look senses +world                                 (The world's look asked)
**   own hub.mapq maker +ui
**   own hub.mreq[ASK_WORLD] maker +world
**   own hub.mreq[ASK_DENIZENS] maker +denizens
**   own hub.mreq[ASK_SENSES] maker +senses
**   own hub.mreq[ASK_UI] maker +ui
**   own hub.change_lock ui                                     (The change records' lock: made with the hub)
**   own hub.wreq world +ui                                     (The UI's requests of the world)
**   own hub.ireq world +ui                                     (The UI's requests of the things)
**   own hub.stock world +senses                                (The senses' request: a chunk's things put down)
**   own hub.pview world                                        (A portal looked at closely: the UI's wreq's)
**   own hub.dig_tool world
**   own hub.dig_wear world
**   own hub.iq_head ui                                         (The UI's intents: its ring)
**   own hub.iq ui
**   own hub.iq_tail world                                      (Its reader's place)
**   own hub.eq_head world                                      (The world's events)
**   own hub.eq world
**   own hub.slot[SLOT_WORLD] referee +world
**   own hub.slot[SLOT_UI] referee +ui +world +denizens         (The UI's, lent to those asking for it)
**   own hub.dslot referee +denizens
**   own hub.actor[*] world                                     (A denizen's body: the world's)
**   own hub.actor[*].name denizens                             (Its mind, its record: the denizens')
**   own hub.actor[*].den denizens
**   own hub.actor[*].attitude denizens
**   own hub.actor[*].faction denizens
**   own hub.actor[*].foe denizens
**   own hub.actor[*].kind denizens
**   own hub.actor[*].remains denizens
**   own hub.actor[*].timid denizens
**   own hub.dsave denizens +ui
**   own hub.den_seq world
**   own hub.pc_tx world
**   own hub.pc_ty world
**   own hub.dq_head world                                      (The denizens' events: the world's)
**   own hub.way_open world                                     (The puzzle's: the world's, by the theme's rules)
**   own hub.sway denizens +ui
**   own hub.gait world                                         (The character's state: the world's, WQ_SET)
**   own hub.watch world
**   own hub.pc_pos world                                       (Its body's position: the world's, as the referee says)
**   own hub.pc_going world
**   own hub.pc_foe world
**   own hub.pc_aim world
**   own hub.pc_stance world
**   own hub.pc_strike world                                    (The blow's intent's)
**   own hub.di_head denizens                                   (The denizens' intents: their ring)
**   own hub.di denizens
**   own hub.di_tail world                                      (Its reader's place)
**   own hub.den_go world                                       (The denizens' turn: the turns pace's)
**   own hub.dsense senses +denizens                            (An actor's senses: the denizens' request)
**   own hub.ui_cmd ui                                          (A command in hand: the look and the minds give way)
**   own hub.vsaved world                                       (The region files' versions: the world's, as it
**   own hub.vbase world                                        (begins and keeps a game)
**   own hub.vlo world
**   own hub.vhi world
**   own hub.small ui                                           (dq0 -C)
**   own banks(1,PLAN_BANKS) maker                              (The plans: grown from the seed)
**   own banks(CACHE_BANK(0),CACHE_BANKS) none                  (The chunks' cache: its room past three chunks)
**   own chunk maker                                            (A chunk: made from the seed)
**   own chunk.tile maker +senses +world                        (Its squares: the senses' memory in them, seen and
**                                                                seen now; the world's changes, a square dug, a pit
**                                                                opened, store.c's change, the seen mark kept)
**   own chunk.room[*].flags maker +senses                      (A room's: R_VISITED, the senses' memory, come into)
**   own chunk.portal maker +world                              (Made from the seed; worked by the world, each change
**   own chunk.trap maker +world                                (made in the chunk as it keeps its record: a state,
**   own chunk.elev maker +world                                (a harm, a square dug, an opening: dq0w/store.c)
**   own chunk.nopen maker +world
**   own chunk.open maker +world
**   own bank(CAT_BANK) world                                   (The catalogue: the world's, the theme's kinds)
**   own banks(CAT_BANK+1,ITEM_BANKS) world                     (The items' places and states)
**   own bank(DEN_BANK) world                                   (The denizens' events)
**   own bank(DEN_BANK,SND_AT,FIGHT_AT) sound +ui               (The sounds: the UI clears them)
**   own bank(DEN_BANK,FIGHT_AT,FIGHT_AT+NFIGHTS) world         (The fights' words)
**   own bank(LOOK_BANK) senses
**   own bank(THEME_BANK) ui                                    (The theme, and the names given)
**   own bank(THEME_BANK,THEME_PLACE,NAMES_AT) maker            (The pieces' places)
**   own bank(SIGHT_BANK) senses                                (What the character perceives, the denizens seen too)
**   own bank(UI_BANK) ui
**   own uirec.map maker +ui
**   own uirec.things_text world +ui
**   own bank(WORLD_BANK) maker                                 (The cache's table)
**   own wrec.strata world                                    (The strata's records: the world's dungeon)
**   own wrec.grown maker                                       (A stratum's record as its plan's grown: M_PLAN's)
**   own wrec.stock maker                                       (A chunk's things: the maker's list, the world's)
**   own wrec.cpage world                                       (The change records' pages: the world's, store.c)
**   own wrec.cstamp world
**   own wrec.cgen world
**   own banks(CHANGE_BANK(0),CHANGE_BANKS) world
**   own bank(CMD_BANK) ui                                      (The commands text: the UI writes it, dq0c reads it)
**   own bank(CMD_BANK,HUB_WINDOW+CQ_PCMD,HUB_WINDOW+CQ_PCMD+PCMD_SIZE) commands  (dq0c's reply: a line's meaning)
**   own wrec.cslot[*].used maker +world                        (A chunk stocked, CS_FRESH: the world's mark taken)
**   own wrec.cslot[*].stamp maker +world +denizens +senses +ui     (Each chunk's reader's stamp, as it finds it)
**   own wrec.stamp maker +world +denizens +senses +ui
**   own wrec.inuse[ASK_WORLD] world
**   own wrec.inuse[ASK_DENIZENS] denizens
**   own wrec.inuse[ASK_SENSES] senses
**   own wrec.inuse[ASK_UI] ui
*/

#define HUB     ((struct hub*) HUB_WINDOW)
#define DSLOT   ((struct req*) HUB->dslot)
#define HUB_FREE        2048                    /* (Bank 0's room kept free, at least) */
typedef char hub_fits[sizeof (struct hub) + HUB_FREE <= 0x2000 ? 1 : -1];   /* (The header in bank 0) */
/* What dq0c (assembly) reads of the hub, where it is (games/dq0c/hub.inc's HUBO_* and CQO_*: its make.js holds them
** to these numbers, and these hold them to the structs) */
#define HUBO_SEM        4
#define HUBO_TASK       12
#define HUBO_BEAT       20
#define HUBO_CQ         28
#define HUBO_QUIT       52
#define CQO_BANK        4
#define CQO_LEN         5
#define CQO_RES         7
#define CQO_FACE        8
typedef char dq0c_fits[offsetof (struct hub, sem) == HUBO_SEM && offsetof (struct hub, task) == HUBO_TASK &&
                       offsetof (struct hub, beat) == HUBO_BEAT && offsetof (struct hub, cq) == HUBO_CQ &&
                       offsetof (struct hub, quit) == HUBO_QUIT && offsetof (struct cq, bank) == CQO_BANK &&
                       offsetof (struct cq, len) == CQO_LEN && offsetof (struct cq, res) == CQO_RES &&
                       offsetof (struct cq, face) == CQO_FACE && NTASKS == 8 ? 1 : -1];
typedef char recs_fit[sizeof (struct sight) <= 0x2000 && sizeof (struct uirec) <= 0x2000 &&
                      sizeof (struct wrec) <= 0x2000 ? 1 : -1];
typedef char snd_fits[(char*) (FIGHT_AT + NFIGHTS) - (char*) 0x8000 <= 0x2000 ? 1 : -1];   /* (DEN_BANK's) */

/* hub.c */
int __fastcall__ hub_create (unsigned char banks);      /* The UI's: the segment, its doorbells: 0, or -1 */
int __fastcall__ hub_attach (unsigned char seg, unsigned char me);    /* A child's: 0, or -1 */
void __fastcall__ hub_map (unsigned char bank);         /* The hub's bank at $8000 */
unsigned hub_here (void);                               /* Where the window is (for hub_back) */
void __fastcall__ hub_back (unsigned w);                /* The window put back where hub_here found it */
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
int __fastcall__ hub_join (unsigned char task);         /* A task waited for: its status, or -1 */
void __fastcall__ hub_post (void* q, unsigned char t);  /* A request (struct rqh first) posted to task t, rung */
unsigned char __fastcall__ hub_wait_t (void* q, unsigned char ticks);   /* Its reply waited for, ticks at most (0:
                                                        **   till it's there): 1 there, 0 not yet (or the game's end) */
#define hub_wait(q)     hub_wait_t ((q), 0)
#define hub_posted(q)   ((q)->state == REQ_POSTED)      /* (The owner's: a request there to answer) */
void __fastcall__ hub_reply (void* q);                  /* A request answered (the owner's): the asker rung */
void __fastcall__ hub_sleep (unsigned char ticks);      /* This task asleep on its doorbell, ticks at most */
void __fastcall__ hub_nap (unsigned char ticks);        /* (The UI's) The same, the rings that came with it taken */
void* __fastcall__ hub_call (void* q, unsigned char t);  /* hub_post, then hub_wait: the request (set REQ_FREE
                                                        **   when its reply's taken) */
#define hub_ask(s)      ((struct req*) hub_call (&HUB->slot[s], T_REFEREE))     /* (The referee asked, slot s) */

#endif
