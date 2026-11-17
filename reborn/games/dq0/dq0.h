/*
** dq0.h - Dingus Quest Zero: a text adventure in real time, in random dungeons, on a ruleset's rules (HÃ¢rnMaster's,
** so far: the referee's, games/dq0harn; the game asks it in rs.h's terms, never naming a skill or a die).
**
** The game's parts, each a file:
**   main.c     the start (its arguments, the overlays read in), the loop: the keys, the clock, the commands
**   term.c     the screen: the status line at the top, the story scrolling under it (its words wrapped), the line
**              being typed at the bottom, kept whole while the story moves on; sayf, the story's own printf
**   bank.s     the RAM banks: the window at $8000, bytes between a bank and the main RAM, the overlays' calls
**   look.c     what the character perceives, in words (the world's record of it, in the hub: level.h); the map
**   fixt.c     (overlay 4) the fixtures' words: a portal looked at, a denizen in the way
**   said.c     (overlay 27) the outcome events' words (a dig, a climb, a portal worked, a trap): the one renderer of
**              the phrase table, said.txt (the build's: games/dq0/saidgen.js, obj/gen/dq0said.h)
**   blows.c    (overlay 6) a fight's words: the blows struck, the wounds, a denizen's fall (an E_FIGHT's)
**   intro.c    (overlay 1) the title and the pace (the character is hmchar's: a program of its own,
**              games/hmchar, that main.c runs, and whose file the referee reads)
**   theme.c    (overlay 5) the theme's words (its story, its names, its endings) and its puzzle (theme.h)
**   sway.c     (overlay 6) the character's ways with the denizens, in words (talk, persuade, an ally's requests:
**              the denizens' task's answers), and the commands listed
**   seen.c     (overlay 12) more of what the character perceives, in words: a ceiling at a glance, the things
**              and the denizens it sees, those it hears
**   choose.c   (overlay 7) the thing the player means, of those it perceives: of several, which, asked
**   resolve.c  (overlay 11) the command's things found among those it perceives, its unsaid filled (it, the
**              target, the things near), which of several; a throw told
**   names.c    the names the character gives things, rooms, allies (parts 5 and 6: given, and said)
**   actions.c  (overlay 11) the dispatch table: a row a verb, its handler, its takes, its refusals, its request
**   h*.c       the command handlers, by meaning (cmds.h's: the things', the places', moving, the denizens', the self's)
**
** Memory: the program's code and data from $0800 (its BSS must end 768 bytes or more under $7F00: dq0.cfg); the
** overlays' code in banks of their own, put in the window at $8000 as they're called (bank.s).  Code in an overlay
** can call any function (a stub that changes the window never reads or writes what's in it).  And the swapped
** parts (swap.inc): code that runs in the main RAM's top (dq0.cfg's
** SWAP, under the C stack's least room), a part at a time, copied there from its overlay's bank (8, 9) as it's
** called: main.c's start and end (part 1) and its commands (part 2), look.c's map (part 3).  A part's code reads and
** writes the hub as the main code does: a command more goes in a part (a new one, or one with room), at no cost to
** the main RAM.
*/

#ifndef _DQ0_H
#define _DQ0_H

#define VERSION     "0.1"
extern char libdir[48];                         /* Its files' directory (the overlays, the world, the themes): its
                                                **   own directory's lib/dq0, as main.c finds it */
extern char gamedir[40];                        /* Its own directory (hmchar's too): /sd/0/games on the card */
extern char rs_stun[12], rs_own[10], rs_owns[32];   /* The ruleset's words (RS_HELLO's, main.c's): its C_STUNNED state
                                                **   ("in shock"), its own stance's word ("counter": "" none) and that
                                                **   stance told ("You will strike back first.") */

/* ---- term.c: the screen */

extern unsigned char tw, th;                    /* Its width and height */
void t_init (void);                             /* The screen made: status, story, the line typed */
void t_away (void);                             /* As it was (the scrolling region the whole screen again), a while */
void t_end (void);                              /* The same, as the game ends: the console's terminals as they were */
void __fastcall__ say (const char* s);          /* Words into the story, wrapped (\n: a new line) */
void sayf (const char* fmt, ...);               /* The same, with %s, %d, %u, %c (and %%) */
void __fastcall__ msg (const char* s);          /* A new line of the story, then s */
void __fastcall__ say_raw (const char* s);      /* A line of the story as it is (not wrapped: overlay 16's) */
void blank (void);                              /* An empty line in the story (only one in a row) */
void t_done (void);                             /* The cursor back to the line typed, and everything out */
void t_flush (void);                            /* What's said so far out now (the cursor left where it is) */
void __fastcall__ t_status (const char* left, const char* right);  /* The status line drawn */
void status (void);                             /* (Overlay 27, main.c) The status line, as the game is now */
void __fastcall__ o_t_status (const char* left, const char* right);  /* (Overlay 27's own: t_status's; left 0, drawn whole next) */
unsigned char __fastcall__ believed (unsigned char h);  /* (main.c) True heading h, as the character believes it
                                                **   (its bearings: the hub's pc_skew, slice 4.2's; not an
                                                **   overlay's, the hub not in the window) */
void inl_show (void);                           /* The line typed drawn again */
void __fastcall__ t_prompt (unsigned char ready);   /* The character free (1: the prompt "> ") or busy (0) */
extern unsigned char t_ready;
#define INMAX       100
extern char inl[INMAX + 1];                     /* The line being typed, */
extern unsigned char inn;                       /*   its length */
void __fastcall__ t_key (unsigned char k);      /* A key for the line being typed */
char* t_line (void);                            /* A line that's been entered (Enter), or NULL */
const char* t_next (void);                      /* The next t_line will give, left in the queue, or NULL */
unsigned char __fastcall__ ask (const char* keys);      /* One of keys typed, waiting (the world stopped): which */
unsigned char t_haskey (void);                  /* A key's come (conio's kbhit) */
extern unsigned char t_wrote;                   /* Something out since the keys were last looked for (t_haskey's) */
unsigned char t_getkey (void);                  /* The next key, waiting (conio's cgetc: Enter '
') */
void __fastcall__ consctl (const char* what);   /* (main.c) A line to the window's consctl (rawon, rawoff) */
void __fastcall__ ask_line (char* b, unsigned char max);  /* A line typed, waiting */

/* ---- The clock: the game's time, in tenths of a second (a round the ruleset's, RS_HELLO's), run at the pace */

#define PACE_RELAXED    0
#define PACE_NORMAL     1
#define PACE_MENACING   2
#define PACE_TURNS      3                       /* The world moves only as the character does (a turn game) */
extern unsigned long now;                       /* The game's time */
extern unsigned char pace, paused;
extern unsigned char hard;                      /* Hard mode (dq0 -H, D7's setting: no autosave to go back to, a
                                                **   death the save's end; kept in the save's pace, its top bit: slice
                                                **   4.4's) */
extern unsigned char plain;                     /* The map in plain letters, no colours (the player's: "colour off",
                                                **   pace.c's; kept with the game: main.c's save) */
extern const char* savedir;                     /* (main.c's: dq0 -d's, the save's folder) */
void __fastcall__ meta (unsigned char action);  /* Overlay 3 (task.c's): VB_META_PAUSE, the game paused; VB_META_SAVE,
                                                **   kept, if it's kept (-d) */
extern const char* const pacename[4];

/* ---- The character, as the referee names it (its sheet is the referee's) */

struct pc {
    char            name[16];
    char            occupation[20];             /* What it was before the dungeon (hmchar's: "Thief" ...) */
};
extern struct pc P;

/* ---- look.c: the place in words, and the map */

void __fastcall__ look (unsigned char first);   /* Where the character is (first: the first time there) */
void stir (void);                               /* The denizens it sees, told as they move */
void __fastcall__ map (const char* rules);      /* Part 3: the map of what it's seen round it, and the
                                                **   character's state beside it (rules: the referee's short lines) */

/* ---- A command: the command parser's (dq0c: games/dq0c, cmdmean.s's, by the verbs, commands.verbs), hub.h's
** struct cmd: an action (actions.h's VB_*) and its roles; cq_cmd's, the one the UI carries out (cmd) */

#include "actions.h"
enum { OB_NONE, OB_DOOR, OB_PASSAGE, OB_STAIRS, OB_LADDER, OB_SHAFT, OB_LEDGE, OB_WAY_OUT, OB_WALL, OB_WINDOW, OB_TRAP,
    OB_PORTC, OB_ROOM, OB_CORNER, OB_PILLAR, OB_CENTRE, OB_SIDE };  /* (OB_ROOM: the room, its ceiling: looked at closely; a corner, a
                                                **   pillar: looked round, "look around the corner") */
enum { HOW_ANY, HOW_TO, HOW_THROUGH, HOW_ACROSS };
enum { PR_NONE, PR_IN, PR_ON, PR_FROM, PR_WITH, PR_AT, PR_TO };    /* (The other thing: in it, on it, from it, with
                                                **   it, at it, to it) */
#define GO_UP       8                           /* (A direction: up, down; else a heading, H_N ...) */
#define GO_DOWN     9
#define EX_VALUE    0x01                        /* (A thing's aspects judged, an examine's aux, all said: commands.words' */
#define EX_QUALITY  0x02                        /*   [aspects], commands.verbs' own; 0 a closer look, 2026-10-22: its */
#define EX_BOTH     0x03                        /*   value, its quality, both (the referee's appraisal); its weight, its */
#define EX_WEIGHT   0x04                        /*   lock, a search of it for traps, its condition: no test but the */
#define EX_LOCK     0x08                        /*   search's) */
#define EX_TRAPS    0x10
#define EX_COND     0x20
#define SR_FOR      0x40                        /* (A search's aux, its verb's, commands.verbs': "search for the key",
                                                **   "search around for a dagger"; "search nearby", "search around",
                                                **   "feel around": search.c's, 2026-10-22) */
#define SR_NEAR     0x80
struct cmd;
struct cthing;
extern struct cmd cmd;                          /* (main.c's: the command being carried out, cq_cmd's) */
extern unsigned char pc_face;                   /* (The hub's, copied for the overlays) */
#define TOLD        24                          /* (A look's sentences said this answer: look.c's told, ntold) */
extern unsigned told[TOLD];
extern unsigned char ntold;
extern unsigned lb_salt;                        /* (lbsaid.s's: where the sentences said are said of, their hash begun
                                                **   with it: the character's square, main.c's) */
void lb_tell (void);                            /* (lb's words all said, each kept as told: lbsaid.s's) */
void lb_said (void);                            /* (A look's words said, lbsaid.s's: words.c's and seen.c's lb_out) */

/* ---- pace.c (part 4): the pace, the watchfulness, the state, small moves, from the line typed */

unsigned char __fastcall__ pace_cmd (const char* l);   /* 1 if it was one of them, carried out (else the command's;
                                                **   but a pace said with a move set first) */
unsigned char __fastcall__ pacing (const char* l);     /* Overlay 16 (pace.c): 0 if line l can't be one of them (none
                                                **   of the words pace_cmd acts on: its part not copied in) */
unsigned char __fastcall__ travel_go (const char* l);  /* The command's move to a place the character knows ("go back
                                                **   to the Mausoleum", the parser's cmd): its journey (I_TRAVEL), 1;
                                                **   else 0 (slice 4.2's) */
extern unsigned char busy;                      /* (main.c's: the character's doing something) */
struct req;
struct req* __fastcall__ ask_b (unsigned char kind, unsigned char a, unsigned char b);   /* (main.c's) The referee
                                                **   asked, the UI's slot */
void __fastcall__ intend2 (unsigned char kind, unsigned w1, unsigned w2);   /* (main.c's) An intent to the world (an
                                                **   overlay's too) */
extern unsigned char intend_a;                  /* (main.c's) The next intent's a (intend2's: then 0) */
void __fastcall__ wset (unsigned char what, unsigned char x, unsigned char y);  /* (main.c's) The character's state, or
                                                **   the game's, set (the world's: the hub's wreq, WQ_SET, WS_*), from
                                                **   anywhere (the window put back) */
extern unsigned char shand;                     /* (main.c's) The hand said: items.h's IH_* (slice 4.1b's) */
extern char hold_w[16];                         /* (main.c's) A blow's thing to take in hand first, as it's sent ("":
                                                **   none) */
extern unsigned char strike;                    /* (main.c's) What the character strikes with (SK_*: the parser's), sent
                                                **   with each blow (I_ATTACK's, I_PORTAL's x[1]) */
/* ---- choose.c (overlay 7): the thing meant, of those the character perceives; of several, which, asked */

#define LOOK_NEW    32                          /* (look's: only what's new since the command began) */
#define FW_NEW      0xFFFF                      /* (felt_words': that, task.c's) */
#define FW_TOUCH    0xFFFE                      /* (felt_words': what its hands find, in the dark, the look's: task.c's) */
#define PICK_TELL   0x80                        /* (pick's obj |: "You see no door here", if there's none) */
#define PICK_ANY    0x40                        /* (Any one: none asked) */
unsigned char __fastcall__ pick (unsigned char obj, unsigned char orel);   /* The thing of kind obj (OB_*), lying
                                                **   orel (or 0xFF: any way): 1, its square in tx, ty (the one there
                                                **   is, the one chosen, the nearest ahead); 0, none; 2, several and
                                                **   none ahead: they're listed, which asked */
unsigned char __fastcall__ choose (struct cmd* c);     /* The command (parsed) a choice's answer (its number, its
                                                **   way)?  Then c is the command asked about again, pick's the one
                                                **   chosen: 1; else 0.  A blow's question's answer the same; a blow
                                                **   with nothing named, what's in its hands named, or asked about */
unsigned char wield (void);                     /* (Part 9's, choose.c's) A blow with nothing named (cmd): what's in
                                                **   its hands what strikes, 1; a better thing carried asked about, 2 */
unsigned char __fastcall__ things (unsigned char op, const char* name);    /* (main.c's) The things asked, from the
                                                **   main code or a part */
#define IO_GO       0x80                        /* (things' op |: one path for things, COMMANDS-CRITIQUE.md's §5.2 H: out
                                                **   of reach, walked over to first, the command again (reach_go); several
                                                **   that differ, which asked (which_ask): either, IR_GONE, nothing more to do) */
#define IR_GONE     (IR_WHICH + 1)

/* ---- resolve.c (overlay 11): the command's things found among those the character perceives, its unsaid filled (it,
** the last thing named, the things near, the target), which of several; of several it must be one of, which, asked */

#define RV_ON       0                           /* (resolve's: the command carried on with; seen to, a question */
#define RV_DONE     1                           /*   asked, the target set, said why not) */
unsigned char resolve (void);                   /* The command (cmd) resolved: RV_ON or RV_DONE */

/* ---- actions.c (overlay 11): the dispatch table, a row a verb (VB_*, actions.h's), COMMANDS-CRITIQUE.md's §5.2 step L:
** its handler (H_*: main.c's s_command calls its group's entry by it), the kinds of thing it takes (TK_*), its refusals
** (refuse.c's rules[] till stage 2) and its request (PA_*: a door verb's, put in cmd.aux where the line gave none,
** stage 3).  The handlers by meaning, a group a file, in their order here (stage 3, 2026-11-08): each group's entry
** switches on the number; where each is, its pragma's (cmds.h) */
enum {
    H_NOT_YET,                                  /* (None: "You can't ... in this game yet.", refuse.c's U_YET) */
    H_TAKE, H_DROP, H_HANDLE, H_HIDE, H_EAT,    /* (The things' verbs: hthings.c's things_do) */
    H_USE, H_PUSH, H_PULL, H_TOUCH, H_TASTE,    /* (The things' verbs to come, the plan's batch B4: each a stub that
                                                **   says "not yet" till then, 2026-11-09) */
    H_FILL, H_LOOK_IN, H_TIE, H_SHUT, H_HIDDEN, /* (Their timed works: hthings.c's things_work; H_SHUT, H_HIDDEN no
                                                **   verb's, the door verbs' first and the hide's) */
    H_DOOR, H_CLIMB, H_EXAMINE, H_SEARCH, H_LOOK_WAY,  /* (The features' and the doors': hplaces.c's) */
    H_KNOCK,                                    /* (B2's, a stub till then) */
    H_GO, H_FACE, H_JUMP,                       /* (Moving: hmove.c's) */
    H_SWIM, H_FOLLOW,                           /* (B6's, stubs till then) */
    H_FIGHT, H_MISSILE, H_DISENGAGE, H_SWAY, H_GIVE,   /* (The denizens': hdens.c's) */
    H_CHARGE, H_THROW_WAY, H_ENGAGE, H_SPLIT,   /* (B1's, the fights': stubs till then) */
    H_SAY,                                      /* (B2's, the noise: a stub till then) */
    H_WAKE, H_TAUNT, H_ORATE,                   /* (B3's, the minds: stubs till then) */
    H_NOTHING, H_LIMBS, H_FEEL, H_LOOK, H_WAIT, H_STOP, H_TEND, H_DIG, H_INVENTORY, H_SHEET, H_TIME, H_META, H_HELP,
    H_QUIT,                                     /* (The body's, and the meta commands: hself.c's) */
    H_STATUS,                                   /* (The character's state told, pace.c's "status": "show status",
                                                **   "pace" alone, a pace it didn't know) */
    H_POWERS,                                   /* (Spells, psionics, invocations: refused, true to the character, the
                                                **   row's request its refusal, A_NO_MAGIC ...) */
    H_PRETEND,                                  /* (B3's, a stub till then) */
    H_PRAY, H_FAST,                             /* (B5's, stubs till then) */
    H_AGAIN                                     /* (B7's, the line: a stub till then) */
};
#define H_WORK      H_FILL                      /* (Each group's first) */
#define H_PLACES    H_DOOR
#define H_MOVING    H_GO
#define H_DENS      H_FIGHT
#define H_SELF      H_NOTHING
#define TK_CLOSER   0x01                        /* (A row's takes: a thing that closes, worked first, H_SHUT's: a
                                                **   chest, a sack; else a door's) */
#define TK_ONTO     0x02                        /* (A thing put somewhere: a part of the place its other refused, "You
                                                **   can't put the torch on the door." (but into a hole, a shaft's, a
                                                **   pit's), actions.c's o_refused, 2026-11-17) */
unsigned __fastcall__ verb_row (unsigned char action);  /* A verb's row: its handler, its takes times 256 (and its
                                                **   request into cmd.aux, if the line gave none) */
void __fastcall__ taken_next (char* w);         /* "Take all"'s next thing to take up, into w ("": none) */
void __fastcall__ throw_said (unsigned r);      /* A throw told (the things' IO_THROW's result, its TH_* times 256:
                                                **   IR_NOT_HERE, no mark it sees; IR_FULL, none said): THROW's rows */
extern unsigned char untarget;                  /* (main.c's: the target let go at the next command: the character
                                                **   made to turn, by a trap set off, a blow from one it doesn't see) */

/* ---- theme.c (overlay 5): the theme's words and its puzzle (theme.h), from its copy in the overlay's bank */

#define TCOPY       ((struct theme*) 0x9000)    /* (The theme, in overlay 5's bank: main.c reads it in) */
#include "life.h"                               /* (Its struct quest) */
extern struct quest Q;                          /* (main.c's) */
extern char tname[32];                          /* A theme's name for something (theme_named's) */
enum { TN_THEME, TN_STRATUM, TN_ROOM, TN_LIGHT, TN_END, TN_GONE };
enum { Q_NONE, Q_TOLD, Q_DONE };                 /* (puzzle's: not its; words only; a rule done) */
void __fastcall__ theme_load (unsigned char t);     /* Overlay 5: the theme (its number: the file's counted round)
                                                **   read in: into the hub's theme bank and the overlay's; its kinds
                                                **   into the hub's catalogue (the things' read in first) */
void __fastcall__ theme_named (unsigned char what, unsigned char n);   /* Overlay 5: its name, a stratum's (n its
                                                **   number) into tname; a room's (n its kind), its light, added to
                                                **   lb; an ending's words said (n its END_*); E_LEFT's (n its w1 less EL_WAY:
                                                **   how the character's out, or a take rule told; theme.c's: one stub,
                                                **   the main RAM's room) */
void __fastcall__ story (unsigned char resumed);    /* Overlay 5: the theme's name (and its story, a new game's) */
void way_words (void);                          /* Overlay 5: the way out, shut or open */
unsigned char __fastcall__ history (unsigned long seed);    /* Part 7: a game's own theme made, themegen's (main.c):
                                                **   the theme to read (0xFF its own, else a stand-in's) */
void __fastcall__ ending (unsigned char how);   /* Overlay 15: the game's end (END_*: its words, the save marked so, a game
                                                **   done isn't carried on); or, how EL_WAY on, E_LEFT's */
extern volatile unsigned char quit;             /* (main.c's) */
unsigned char __fastcall__ puzzle (struct cmd* c);      /* Overlay 5: a command the puzzle's (Q_*: the world's
                                                **   judgement, IO_PUZZLE, worded) */
unsigned char __fastcall__ read_cmd (struct cmd* c);    /* Part 25: a writing read or copied (Q_*: read.c's, the plan for
                                                **   the game between dungeons, its step 9) */
void __fastcall__ clue_make (unsigned char r);  /* Part 26: rule r's clue in the dungeon's clues.dat, made if it isn't */
void clues_kept (void);                         /* Part 26: the dungeon left alive, its writings' clues kept (its step 10) */
void more_said (void);                          /* Part 26: a critical success's line more (read.c's) */
void __fastcall__ swapped (unsigned char r);    /* Part 26: a critical failure's words, rule r's writing's, into lb */
unsigned char __fastcall__ othings (unsigned char op, const char* name, const char* into, unsigned char to);
                                                /* (main.c's) The things asked (from an overlay: the names in the
                                                **   main RAM): the result; the request's x in othings_x,
                                                **   the things' words in lb */
extern unsigned othings_x;
extern char nil[1];                             /* ("", in the main RAM: an overlay's name for othings, whose own "" is in its bank,
                                                **   swapped out as othings reads it: main.c's, 2026-11-08) */
void __fastcall__ hub_mem (unsigned char bank, void* to, const void* from, unsigned n);    /* (main.c's) Bytes
                                                **   between a hub's bank and the main RAM, from an overlay */

/* ---- The overlays (bank.s: their stubs) */

void title (void);                              /* Overlay 1: the title, the pace asked */
unsigned char __fastcall__ ovl_load (unsigned fd);      /* (dq0lib.inc's: the overlays' file read in, its fd) */
void __fastcall__ dots (unsigned char k);       /* (Overlay 6's) The game's start's progress (main.c's) */
extern unsigned char starting;                  /*   while it starts (main.c's) */
void __fastcall__ entered (unsigned char resumed);  /* Overlay 1: the character come into the dungeon */
void help (void);                               /* Overlay 1 (help.c's): the commands, listed (help.txt) */
void __fastcall__ unknown (unsigned char f);    /* Overlay 24: a command refused, said back from its line (cl): f U_* */
enum { U_VERB, U_YET, U_CANT, U_NOTE, U_WHAT,   /* (A verb the game doesn't know; one it doesn't do yet, or not to that
                                                **   yet; one that can't be done to that; nothing written on it; nothing
                                                **   named, "Take what?") */
    A_SEE_NO, A_SEE_NONE, A_HAVE_NO, A_CARRY_NONE, A_NO_TAKE, A_UNSEEN, A_UNSEEN_WAY, A_FAR, A_NEARER,
    A_NO_MAGIC, A_NO_PSI, A_NO_GODS, A_NO_GOD,  /* (The powers, H_POWERS's: no spells, no psionics; no invocations of
                                                **   the character's deity's, its name put in; none, worshipping none) */
    A_OUT,                                      /* (A line typed as it lies senseless: main.c's out_line) */
    A_SENSE, A_QUOTE,                           /* (Words the game knows, not a command its grammar has; words said
                                                **   with no quotes, the form shown: main.c's s_command, refuse.c's
                                                **   o_unknown, 2026-11-17) */
    A_COUNT };
#define A_LB        0x80                        /* (say_no's a |: added to lb, not said) */
#define A_THEY      0x40                        /* (say_no's a |: its thing a person, "go up to them first", none named
                                                **   "They are ...": A_FAR's, the user's people "they", 2026-11-17) */
void __fastcall__ say_no (unsigned char a, const char* w);   /* Overlay 24 (refuse.c's): a refusal said, the one
                                                **   wording of its meaning (U_*, A_*: "You see no %s here.",
                                                **   "You have no %s.", "%s is out of reach: go up to it first.",
                                                **   "You haven't seen a(n) %s here." ...), w its thing's name put in
                                                **   (0: "it"; the main RAM's, not an overlay's own); A_SEE_NO's and
                                                **   A_HAVE_NO's with no name to say (none, a "#"), their nameless
                                                **   forms */
unsigned char refused (void);                   /* Overlay 24: a thing named that its verb would pass over,
                                                **   refused (refuse.c's o_refused: 1) */
extern const char* cl;                          /* The command's line (main.c's) */
unsigned char __fastcall__ name_is (const char* name, const char* w);   /* (nameis.s's, the main RAM's) Word w names
                                                **   name: the one rule (the world's named's), the word said before it
                                                **   in the line too */
unsigned char __fastcall__ parts_in (unsigned char op);    /* Overlay 6 (parts.c's): the line typed queued in its
                                                **   parts, 0 no room (PO_LINE); */
#define PO_LINE     0
#define PO_STOP     1                           /*   the long task stopped: the rest of its line let go; */
#define PO_AGAIN    2                           /*   the command has no verb: the list's verb put to it, back in the queue: 1 */
extern char t_last[INMAX + 1];                  /* (term.c's: the last line typed, but one that says "again") */
extern unsigned char held;                      /* (main.c's: the next line held for the character; | 2, that line let go: its
                                                **   parse with it) */
#define QMAX        256                         /* The lines typed ahead (term.c's queue, qlen) */
extern char queue[QMAX];
extern unsigned qlen;
struct sway;
void __fastcall__ sway_said (const struct sway* s);    /* Overlay 15: the character's way with a denizen, the
                                                **   denizens' task's answer (the hub's sway, copied), told (SWAY's rows) */
unsigned char __fastcall__ task_words (unsigned w, unsigned char named);   /* Overlay 3: a long task's (task.c) */
void __fastcall__ felt_words (unsigned w);      /* (Overlay 17's) The look's in the dark: FW_NEW, FW_TOUCH (task.c's) */
unsigned __fastcall__ dnum (const char* s);     /* (The game's library's NUM: bank.s's, dq0lib.inc) A number, a word's (atoi's) */
void __fastcall__ turned_says (unsigned char go);                        /* (Overlay 1's) A turn's first words (task.c's, 2026-11-03) */
void facing_words (void);                       /* (Overlay 7's) The way it believes it faces (task.c's: slice 4.2's) */
#define FS_CMD      1                           /* (fore_set's: main.c's) */
#define FS_UI       2
#define FS_MAKE     4
#define FS_SENS     8
extern unsigned char fored;
extern unsigned char stir_held;                 /* (main.c's: a denizen's stir held, told after the line's answer) */
void __fastcall__ fore_set (unsigned char on);  /* (Overlay 6's) The UI and the answer's tasks in the kernel's
                                                **   foreground, or not (main.c's FS_*) */
unsigned char __fastcall__ stop_kind (const char* word);  /* Overlay 7 (task.c's): the task a stop names */
unsigned char __fastcall__ answer_of (const char* l);     /*   and a question's answer */
void __fastcall__ bar_text (char* s, unsigned done, unsigned len);  /* (Overlay 3) */
void __fastcall__ wait_go (unsigned char action);   /* (Overlay 13) A wait, a sleep or a rest begun (task.c's) */
void __fastcall__ time_said (unsigned w, unsigned char b);  /* (Overlay 13) Time passed, told: E_TIME's */
unsigned char __fastcall__ free_cmd (const char* l);   /* Overlay 7: a line typed as the character's busy, carried
                                                **   out at once (1), held (0), or held and its fight stopped for it
                                                **   (2) */
extern unsigned char autogo;                    /* (main.c's: dq0 -a, every soft question answered yes, unasked) */
extern unsigned char harm_told;                 /* (The states a harm's words told this answer: main.c's) */
extern unsigned char out_told;                  /* (main.c's: knocked senseless, told; no line taken, nothing sensed, till it
                                                **   comes round) */
extern unsigned long out_until;                 /* (main.c's: senseless till then, as the world had it at the blow told) */
void out_line (void);                           /* Part 2 (main.c): a line typed as it lies senseless, refused */
char* __fastcall__ env (const char* name, char* buf, unsigned char size);  /* The environment's (main.c) */
void __fastcall__ appraise (const struct ireq* q, signed char g, unsigned char ex);   /* Overlay 1: a thing judged
                                                **   (its value, its quality, both: ex's EX_*) */
extern char dug_with[16];                       /* The tool a dig took: main.c's */
extern char dig_with[16];                       /* The tool a dig's taken up, to name (then ""): main.c's */
extern char dig_word[16];                       /* The thing named to dig with, as said: main.c's */
void __fastcall__ trap_name (unsigned char kind, unsigned char trigger, unsigned char state);  /* Overlay 10: a
                                                **   trap's name, added to lb */
void den_way (void);                            /* Overlay 4: a denizen in the way (its name in lb) */
struct msg;
void __fastcall__ outcome_words (const struct msg* m);   /* Overlay 27 (said.c): an outcome event worded, its row
                                                **   of the phrase table (said.txt: by its kind and its outcome; none, nothing said) */
extern const char* said_s[6];                   /* (said.c's, the main RAM's: the UI's words for a row, the thing done
                                                **   to, the thing done with, a word of the row's own; three more, {w4}
                                                **   {w5} {w6}; 0 or "" none) */
enum { UK_THROW = 0x80, UK_SKY, UK_SWAY, UK_FIGHT, UK_NO, UK_FIRST, UK_SA, UK_SELF, UK_AHEAD, UK_LIST, UK_LI };
                                                /* (The UI's own kinds, worded by
                                                **   said.txt's rows as an outcome event is: no world's event, never in
                                                **   the hub; msgs.def's, narration as data's stage 4, 2026-11-12; UK_SA
                                                **   the world's things' coded words, stage 5's; UK_SELF the character's
                                                **   own state, 2026-11-15; UK_AHEAD a look one way's; UK_LIST a
                                                **   sentence with a list, UK_LI each thing in it: the look's lists,
                                                **   2026-11-16) */
enum { LF_LIE, LF_WHERE, LF_IN, LF_PACKED, LF_DEN, LF_GONE, LF_STIRS, LF_NOSTIR, LF_HEARD, LF_TOUCH };
                                                /* (UK_LIST's: things lying about, by where (seen.c's), in a sconce, the
                                                **   drawing things packed; a denizen seen, gone from sight, the stirs
                                                **   (dens.c's); the walls felt in the dark (task.c's touch_words)) */
enum { LI_THING, LI_DEN, LI_TOUCH, LI_AND, LI_COMMA, LI_SEMI, LI_ITEM };
                                                /* (UK_LI's: a thing seen, a denizen, a kind of wall felt and its sides;
                                                **   the words between them; a thing's own words, the world's: SAY_LI) */
struct li {                                     /* A list's entry (said.c's li: its fields where a struct msg has them,
                                                **   msgs.def's UK_LI) */
    unsigned char   kind, who, a, b;
    unsigned        w1, w2;
    char            cap[6];                     /*   its capital ("(C) ": letters.c's), w4 */
    const char*     name;                       /*   its name, text */
};
extern const struct li* said_li;                /* (said.c's: the list a UK_LIST row's {list} says, its a entries) */
void __fastcall__ list_words (const struct msg* m);    /* Overlay 27 (said.c): a UK_LIST sentence worded into lb at
                                                **   lbe, not said (the look's: their add ()s' streaming), lbe after it */
enum { AH_PEER, AH_COVER, AH_SHUT, AH_GRILLE, AH_THROUGH, AH_DARK, AH_SKY, AH_PASSAGE, AH_INTO, AH_ON, AH_LIT, AH_TOO,
    AH_END, AH_GLOW, AH_NOTHING };              /* (UK_AHEAD's: way.c's focus_words, each one of its say ()s before) */
enum { MY_WANT, MY_LIMB, MY_LIMP, MY_POS, MY_HIDDEN, MY_UNDER, MY_SWIM, MY_DEEP, MY_FACING, MY_STEP, MY_SETOFF, MY_FACE,
    MY_POSE, MY_POSED, MY_DRAUGHT, MY_STILL, MY_HEARD, MY_BEHIND, MY_STANCE, MY_TIME, MY_LOST, MY_NOISE };
                                                /* (UK_SELF's: the status's lines, a turn's and a step's first words, a
                                                **   pose, the air and the background heard, a stance, the time told; a
                                                **   denizen heard, not seen) */
enum { FS_THE, FS_YOUR, FS_INTO, FS_THEM, FS_IT, FS_PEER, FS_LISTEN };   /* (UK_FIRST's: a command's first words' form, */
enum { FV_REACH, FV_LOOK_FOR, FV_FEEL_FOR, FV_LOOK_AT, FV_HEAD, FV_TRY, FV_RAISE, FV_LIFT, FV_PUT_DOWN, FV_OVER,
    FV_FEEL };                                  /*   its verb: "You reach for the torch.", sway.c's) */
extern unsigned char said_n;                    /* (A choice of the UI's for a row: its {arg}) */
extern unsigned char said_q;                    /* (1: a row's words left in lb, not said: its caller's) */
extern char* said_to;                           /*   (where in lb: lb itself, but as tt's words are made) */
struct fight;
void __fastcall__ feed (unsigned char eat);     /* (Part 16's, feed.c: "eat", "drink", slice 4.9's) */
/* (Part 18's hide, main.c's and hthings.c's, not term.c's: HD_ROPE: the thing's word a rope's or a grapple's, or none: 1; HD_UP: a grapple or a rope thrown up, carried out: 1) */
#define HD_ROPE         ((const char*) 1)
#define HD_UP           ((const char*) 2)
unsigned __fastcall__ time_ui (unsigned w);     /* Overlay 16 (time.c): the time of day believed, worded (w TU_TELL,
                                                **   "time"; TU_SKY | its phase times 256, the sky seen; TU_UNTIL, a
                                                **   wait's minutes "until dawn", "watch the sun", 0 none said; TU_LIGHT,
                                                **   lb's theme words with the light outside, said: theme.c's) */
enum { TU_TELL, TU_SKY, TU_UNTIL, TU_LIGHT };
#define FIGHT_LB    ((struct fight*) (lb + sizeof lb) - 1)
void __fastcall__ fight_note (unsigned char n);   /* Overlay 22: a fight's word into lb (blows.c's) */
unsigned char __fastcall__ grapple (unsigned char g);  /* Overlay 19 (blows.c's): a grapple's move (rs.h's GM_*) at the
                                                **   denizen beside named (cmd.obj): GR_SEND | its foe (I_ATTACK's,
                                                **   intend_a its AIM_GRAPPLE b), or GR_SAID why not (lb); | GT_TRY:
                                                **   only if one's there (else 0, nothing said): HOUSE.md's grappling,
                                                **   hdens.c's grip_go sends it */
#define GT_TRY      0x80
#define GR_SEND     0x40
#define GR_SAID     0x20
unsigned char __fastcall__ fight_target (struct cmd* c);    /* Overlay 6: VB_ATTACK_OBJECT's denizen beside
                                                **   the character to strike at (its name has the word; it lies that
                                                **   way): its entry plus 1, or 0 and why not in lb; VB_DEFEND's stance
                                                **   set, told in lb (0); a blow at a door, a window, a portcullis:
                                                **   FT_THING */
#define FT_THING    0xFE
#define FT_CLOSE    0x40                        /* (| the actor plus 1: out of reach, closed with first, I_CLOSE) */
void __fastcall__ heard_words (unsigned char what, unsigned char rel);  /* Overlay 23 (caps.c; 9 till 2026-11-06): a denizen heard (an E_DEN's
                                                **   what, | 0x80 muffled, the way it lies as the character faces),
                                                **   into lb */
extern char heard_what[20];                     /* Its sound's words (SOUND.md: the sound task's, "a scurrying"; "":
                                                **   none), main.c's */
struct portal;
extern unsigned tx, ty;                         /* (main.c's: the square of the thing its find found) */
void __fastcall__ portal_look (const struct portal* p, signed char g, unsigned char ex);  /* Overlay 4: a portal
                                                **   looked at closely (the world's record of it, the hub's pview,
                                                **   copied), its aspects ex (EX_*: its quality judged at the
                                                **   appraiser's grade g), into lb */
void words (void);                              /* Overlay 2: what the character perceives, into lb */
void seen_words (void);                         /* Overlay 12: the things and the denizens it sees, added to lb */
void stir_words (void);                         /* Overlay 9: the denizens it sees, moving, into lb: what's
                                                **   changed since they were told (or "": as they were told last) */
void den_words (void);                          /* Overlay 9: the denizens it sees, added to lb (seen_words's), and
                                                **   kept as told */
extern char lb[400];
extern char* lbe;                               /* (lb's end: words.c's and seen.c's add) */
extern const char* const wl_said[2];               /* (A well's shaft overhead: look.c's) */
extern unsigned lbn;                            /* (What a look's said of its words already: lb's before) */
extern unsigned char lb_stream;                 /* (A look's words said a sentence at a time, as they're made) */
extern struct sight sight_copy;
struct sight* sight_get (void);                 /* The senses' record copied into sight_copy (look.c's): it */
const char* tt (void);                          /* The things' words (the UI bank's) copied into lb (look.c's), the
                                                **   world's coded sentences worded (tt_words): it */
void __fastcall__ tt_words (const char* t);     /* Overlay 24 (refuse.c's): lb's coded sentences (items.h's SAY_AT)
                                                **   worded by said.txt's SA rows, the text after each read again from
                                                **   the UI bank's t (things_text; trtext, the well's, 2026-11-16) */
extern unsigned char tt_sa;                     /* (The things' words' first sentence's SA_* | 0x80, if it's coded:
                                                **   tt's; else 0) */
extern unsigned char sight_light;
#define SS_LOOKED   0xFF                        /* (sight_same's: a look's, its sum kept) */
unsigned char __fastcall__ sight_same (unsigned char w);   /* (A light's change, E_VIEW's why: 1, the place seen as at the
                                                **   last look; part 8's, look.c's) */
extern char sight_lamp[14];                     /* (What gives its light, named: the hub's pc_lamp) */
extern unsigned char sight_close;               /* (A close look: the room's figures, its size and its ceiling's; 8,
                                                **   a look one way's words) */
/* ---- way.c: looking one way, listening and sniffing one way (HOUSE.md's slice 4.0) */
unsigned char __fastcall__ look_way (unsigned char action);  /* Part 6: "look north", "look at the door", "listen at
                                                **   the door", "sniff" (the command's action, VB_*); VB_NONE: a
                                                **   listen's or a sniff's end (main.c's E_READY); a thing examined,
                                                **   its aspects: 0, or a door's work to do (PA_CHECK) */
void __fastcall__ look_ov (unsigned char action);    /* Part 17: look_way's from an overlay, the window put back */
void focus_words (void);                       /* Overlay 13: a look one way's words, into lb (words.c's) */
extern unsigned char lsn;                       /* (main.c's: a listen, 1 one way, 2 about; 0, none) */
void __fastcall__ bg_words (unsigned char what);    /* Part 11 (main.c's): the background heard, the draught felt */
extern unsigned long heard_at;                  /* (main.c's: when a sound was last told, */
extern unsigned char heard_rel;                 /*   and the way it came) */
unsigned char __fastcall__ den_heard (const struct msg* m);   /* (Overlay 23's) A denizen's sound weighed: its way if it's
                                                **   heard (REL8), to be told; else 0xFF */

/* ---- bank.s */

void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
extern unsigned char ovbank[27];               /* (bank.s's OVMAX: overlay 21 the writings' parts, 22 the designators', 23 their
                                                **   capitals, 2026-11-06; 24 the things' chooser, 2026-11-07; 25 part 27's,
                                                **   2026-11-10; 26 part 28's; 27 the status line, 2026-11-11) */
unsigned char early (void);                     /* Part 12: before a command's things are looked for (in shock, refused:
                                                **   2; first aid's, 1; else 0) */
void tend (void);                               /* Part 12: first aid (VB_TEND) */
unsigned char body_words (void);                /* Part 12: what time did to its body, told (8: dead of it) */
extern unsigned char news_due;                  /* (main.c's: the body's news held as it sleeps, part 12's) */

/* ---- The swapped parts (bank.s's stubs: swap.inc), run in the main RAM's top (dq0.cfg's SWAP), a part at a time */

unsigned char __fastcall__ args (int argc, char* argv[]);    /* Part 7 (main.c): the game's arguments read (begin's): 1, -s said */
enum { LF_GOING, LF_NEXT, LF_NEW };             /* (A game in the save's folder: going on, one ended, none: main.c's life) */
void __fastcall__ between (unsigned char given);    /* Part 21 (main.c): dq0life's arguments made (given: the seed said) */
void life_dir (void);                           /* Part 22 (main.c): the save's folder's dungeon found, by the record */
void __fastcall__ begin (int argc, char* argv[]);   /* Part 1 (main.c): the arguments, the world started, the game
                                                **   begun */
void finish (void);                             /* Part 1: the game kept, the world stopped */
void save_game (void);                          /* Part 1: the game kept (if it's kept: -d) */
void __fastcall__ command (char* l);            /* Part 2 (main.c): a command, the parser's, carried out */
void ready (void);                              /* Part 2 (main.c): nothing, the part copied in for the next */
void __fastcall__ cq_post (const char* l);      /* Part 2 (main.c): a line posted to the command parser (dq0c) */
signed char __fastcall__ cq_cmd (const char* l);   /* Part 2 (main.c): the command line l makes, dq0c's (l posted
                                                **   first, if it isn't), into cmd, its ways made the compass's as the
                                                **   character faces now: the grammar's verdict (1 a command; 0 not
                                                **   one; -1 too long; -2 none, the game ending) */
void cq_drop (void);                            /* Part 2 (main.c): the line posted seen to (its reply waited for) */
void when_free (void);                          /* Part 2 (main.c): what waited for the character to be free (a door
                                                **   barred, a thing lifted out) */
extern unsigned char cq_again;                  /* (The line changed since it was posted: names_subst's) */
void __fastcall__ take (const char* name);      /* Part 2: a thing taken up, by its name */
extern char where_w[34];                        /* (Where in its room a thing lies: part 8's words) */
extern unsigned char reach;                     /* (main.c's: a command's thing walked over to first) */
unsigned char room_where (void);
unsigned char __fastcall__ den_seen (unsigned x, unsigned y);   /* Part 8: an actor's square in sight */
unsigned char __fastcall__ actor_seen (unsigned char b);        /* Part 8: an event's actor seen (b: 0 the character) */                /* Part 8: the character's place in the room, and the room's size */
unsigned char __fastcall__ lie_words (unsigned char i);   /* Part 8: a thing's place in the room (its sight_copy's) */
unsigned char reach_go (void);
extern unsigned char walk_a;                    /* (main.c's: reach_go's next walk, I_TAKE's a: 1, the command kept and
                                                **   again; 0, the world takes it up as it gets there, take's; 2, gone to,
                                                **   nothing after, a "go to": way.c's) */
unsigned char __fastcall__ tool_out (unsigned char which);    /* Part 19: a thing got out for its use (0 a bandage, 1 food, 2 water), a tool's steps */
extern unsigned char nap, what_hi, take_more, take_all, nosteps;    /* (main.c's) */
extern unsigned char step_k, step_n;            /* (main.c's: the reach's step begun, its kind and thing, till the character's
                                                **   free: a stop's words, task.c's) */
extern char pending[16];                        /* (main.c's: a thing to take up as the character's free, "take all"'s next) */
void step_go (void);                            /* Part 19: a step of the reach begun by the world (IO_STEP's) */
void __fastcall__ reach_said (unsigned w, unsigned char named);   /* Part 19: the reach's words */
unsigned char may_den (void);                   /* (sway.c's: the thing named may be a denizen: the denizens asked) */
extern unsigned char paced;                     /* (pace.c's: a manner said with the last pace: main.c's look at a line) */
void place_said (void);                         /* (Overlay 20's, sway.c: "from the floor", "on the ground": a place, not a thing) */
/* letters.c: the designators (hub.h's), a line's letters made its things' names */
#define NDES        2
struct ldes {
    unsigned        id;                         /* The thing's id (0: none yet, chosen by its name) */
    char            word[14];                   /* The word its letter was made ("torch") */
    unsigned char   low;                        /* (1: a lower case's, a thing carried) */
};
extern struct ldes ldes[NDES];                  /* (The line's letters' things, in their order) */
extern unsigned char ndes;
extern unsigned it_id;                          /* (The last thing named's id, "it"'s, if it's known: resolve.c's) */
extern unsigned char den_meant;                 /* (The actor a capital named: its sway's, deal's; letters.c's) */
extern unsigned char t_taken;                   /* (term.c's: the lines taken out of the queue) */
extern unsigned char which_on;                  /* (A take's which asked: its answer letters.c's which_in; caps.c's) */
void which_ask (void);                          /* (Overlay 23's) A take's which asked (the world's IR_WHICH) */
extern unsigned char cq_st;                     /* (main.c's: the line posted to the parser already, letters_in's) */
void __fastcall__ des_req (const char* w);      /* (Overlay 22's) A request's name and its other (the hub's ireq, main.c's
                                                **   things; w its name as asked) made "#" and the id of the thing a
                                                **   letter named, if one did */
void __fastcall__ des_seen (const struct cthing* t);     /* (Overlay 22's) A seen thing the resolver chose, by its id */
unsigned __fastcall__ des_add (const char* w, unsigned n);   /* (Overlay 24's, letters.c) Word w's thing the line's handle: id n
                                                **   if it's carried or in sight ("it"), or the store's record n | LT_REC
                                                **   ("take all"); its id, 0 none.  n 0: its handle's id.  (w in the main RAM) */
unsigned char __fastcall__ des_thing (struct cthing* t);   /* (Overlay 22's) Thing t named by a letter: 0 not; 1 so (a
                                                **   denizen's in sight found: its name, its square); 2 a denizen's out
                                                **   of sight, refused */
extern char capb[6];
const char* __fastcall__ cap_of (unsigned char i);  /* (Overlay 23's, caps.c) Thing i in sight's capital, "(C) " (capb), for the
                                                **   look's words: given as it's first told; "" none */
unsigned char __fastcall__ which_way (const char* p, unsigned char len, unsigned char n);  /* (Overlay 24's, which.c) An answer by
                                                **   where its choice lies: its place, 0xFE none that way, 0xFF no way */
unsigned char __fastcall__ cap_index (unsigned id, unsigned char what, unsigned char kind);  /* (Overlay 23's, caps.c) A thing's
                                                **   or a denizen's capital, given now if it has none: its place, NLET none */
void __fastcall__ which_put (unsigned char k, const char* w, unsigned x, unsigned y, unsigned char h);   /* (Overlay 24's,
                                                **   which.c) choose.c's question's choice k */
void __fastcall__ which_place (unsigned char n);   /* (Overlay 24's) choose.c's question, asked in lb, made which_in's */
extern unsigned pick_x, pick_y;                 /* (A door's or an opening's square chosen in answer: o_pick's to take; task.c's) */
void __fastcall__ which_seen (unsigned char r, unsigned char n);    /* (Overlay 24's, which.c) The resolver's question, its choices in
                                                **   the UI bank's which_w, by capitals (role r's thing) */
void __fastcall__ den_caps (unsigned char m);   /* (Overlay 23's) The denizens in sight of mask m their capitals, "(A, B) ",
                                                **   added to lb (dens.c's look) */
void __fastcall__ around (const char* l);       /* (Overlay 23's: "letter d as t" too) What's in sight listed, its capitals ("around") */
unsigned char __fastcall__ des_low (const char* w);   /* (Overlay 22's) A lower case's word, its thing carried, for a thing done with:
                                                **   1 (resolve.c's tool_ok: to hand) */
void __fastcall__ letters_in (char* l);         /* (Overlay 22's) The line's letters made its things' names, as it's posted */
unsigned char what_ask (void);                  /* (Overlay 22's) A command with nothing named asked about ("Drop what?
                                                **   [a, d, f]"): 1, or 0 */
void __fastcall__ walk_ahead (const char* name);    /* (sway.c's: a thing to take seen out of reach: its walk told before the world's asked) */
#define W_DIG           (PA_CHECK + 2)          /* (walk_says' name, a first word by what's asked: PA_* + 1 a portal's work, sway.c's) */
#define W_SWAY          (PA_CHECK + 3)
#define W_PEER          (PA_CHECK + 4)
#define W_LISTEN        (PA_CHECK + 5)
#define W_MAX           (PA_CHECK + 6)
extern unsigned char said_ahead, walk_op, worded, placing;       /* (main.c's: its walk told already; a walk over's arrival short) */
void __fastcall__ blow (const struct msg* m);    /* (Overlay 6's, blows.c: an E_FIGHT worded, and what comes of it) */
void __fastcall__ walk_says (const char* name);    /* (A walk over told as it begins, at once: "You go over to the pot."; 0: the command's thing) */
unsigned char search_go (void);                 /* (Part 10's) A search begun, by its form (search.c) */
void __fastcall__ sheet_map (unsigned char what);   /* (Part 3's) The sheet (0), or the map (1) */
void __fastcall__ list_say (unsigned char first);                           /* (Overlay 16's) What's carried told, the things' words */
unsigned char __fastcall__ feature (unsigned char obj, unsigned char how);   /* (Part 13's) A feature meant (obj: its
                                                **   kind | PICK_*) found, walked up to, faced, its first words (how:
                                                **   FG_* | walk_says' name): 1 to act on it, 2 turned to it first */
#define FG_NEAR         0x80                    /* (feature's how: walked up to unless beside it, a close look's; */
#define FG_FAR          0x40                    /*   never walked up to, a look's, a move's own walk; */
#define FG_GO           0x20                    /*   not faced first, a move's: it turns as it goes; */
#define FW_NONE         0x1F                    /*   no first words: else walk_says' PA_* + 1, W_*, 0 its own) */
void __fastcall__ search_said (const struct msg* m);   /* (A room searched whole's, as it goes: an E_BEAT's WB_SEARCH) */
unsigned char __fastcall__ place_go (const struct cmd* c);   /* Part 7: a place in the room walked to */                  /* Part 7: a command's thing out of reach walked over to first */
void __fastcall__ took_up (unsigned r);         /* Part 2: a thing taken up, or not, told (an E_TAKEN's: IR_*, the
                                                **   light before times 256) */
enum { NL_NONE, NL_DONE, NL_DEN };
#define NI_LINE     1                           /* (names_in's: a line that names, names_line's; */
#define NI_THING    2                           /*   one that may say a thing's name, names_subst's) */
unsigned char __fastcall__ names_in (const char* l);    /* Overlay 10 (names.c): line l's first look, NI_LINE and
                                                **   NI_THING (0: neither part's, not copied in) */
unsigned char __fastcall__ names_line (char* l);    /* Part 5 (names.c): a line that names (name, call, rename ...;
                                                **   forget, unname), before it's a command: a name given or
                                                **   forgotten (NL_DONE); a denizen's to name (NL_DEN: names_den
                                                **   after the command) */
unsigned __fastcall__ names_subst (char* l);    /* Part 6: the things' names in a line made theirs ("#12"); a fight's
                                                **   "with Linda" held first: the tenths it took, or 0 */
void names_den (void);                          /* Part 6: a denizen's name given (the hub's sway), recorded */
const char* __fastcall__ names_say (const char* s); /* Part 6: words with the things' marks ("#12"), as they're said
                                                **   ("Linda, your sword"): in the main RAM */
void __fastcall__ names_look (unsigned char after); /* Part 6: a look's things named marked (0, before its words); the
                                                **   room's name in them (1, after) */
void __fastcall__ letters_keep (int fd, unsigned char back);    /* Part 22: the letters (hub.h's designators) written to
                                                **   the save (back 0), read back (1; fd -1: none) */
void __fastcall__ keeps_back (int fd);          /* Part 22: the names, the shortcuts and the letters read back from the
                                                **   save (fd -1: none, a new game's) */
void __fastcall__ names_keep (int fd, unsigned char back);  /* Part 6: the names written to the save (back 0), read
                                                **   from it (1; fd -1: none) */
unsigned char __fastcall__ room_named (const char* text);  /* Part 6: a room whose name is in text: its entry's
                                                **   number (0xFF: none), the entry in named_room (DUNGEON.md) */
struct tname;
extern struct tname named_room;
extern unsigned char names_n;                   /* (names.c's: the names given, 0 none) */
unsigned char __fastcall__ names_hash (const char* s);  /* A word's hash (names.c's: up to a space, its case aside) */
unsigned char __fastcall__ alias_hit (const char* l);   /* (alias.c's) A line's first word maybe a shortcut's, */
unsigned char __fastcall__ alias_said (const char* l);  /* Overlay 4: a line the shortcuts' own ("alias ...") */
unsigned char __fastcall__ alias_line (char* l);    /* Part 9: a line the shortcuts' (or the answer to a replacement
                                                **   asked about): 1 done there */
void alias_expand (void);                       /* Part 9: the line typed (inl) made its shortcut's line */
void __fastcall__ alias_keep (int fd, unsigned char back);  /* Part 9: the shortcuts written to the save (back 0), read
                                                **   from it (1; fd -1: none) */
extern unsigned char aliases_n, alias_wait;     /* (The shortcuts made, 0 none; a replacement asked about) */
extern char _SWAP_START__[];                    /* (The area: the overlays read in through it, as the game starts) */
extern unsigned long fasting;                   /* (hself.c's: a vow of fasting's end, the clock's tenths; 0 none) */

#endif
