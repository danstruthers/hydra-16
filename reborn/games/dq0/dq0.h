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
**   fixt.c     (overlay 4) the fixtures' words: portcullises, windows and slits worked, traps (an E_TRAP's)
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
**
** Memory: the program's code and data from $0800 (its BSS must end 768 bytes or more under $7F00: dq0.cfg); the
** overlays' code in banks of their own, put in the window at $8000 as they're called (bank.s).  Code in an overlay
** can call any function; but what's in the window goes away while bread and bwrite work, so they're only ever
** given the main RAM's addresses.  And the swapped parts (swap.inc): code that runs in the main RAM's top (dq0.cfg's
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

/* ---- term.c: the screen */

extern unsigned char tw, th;                    /* Its width and height */
void t_init (void);                             /* The screen made: status, story, the line typed */
void t_away (void);                             /* As it was (the scrolling region the whole screen again), a while */
void t_end (void);                              /* The same, as the game ends: the console's terminals as they were */
void __fastcall__ say (const char* s);          /* Words into the story, wrapped (\n: a new line) */
void sayf (const char* fmt, ...);               /* The same, with %s, %d, %u, %c (and %%) */
void __fastcall__ msg (const char* s);          /* A new line of the story, then s */
void __fastcall__ say_raw (const char* s);      /* A line of the story as it is (not wrapped) */
void blank (void);                              /* An empty line in the story (only one in a row) */
void t_done (void);                             /* The cursor back to the line typed, and everything out */
void t_flush (void);                            /* What's said so far out now (the cursor left where it is) */
void __fastcall__ t_status (const char* left, const char* right);  /* The status line drawn */
void status (void);                             /* (main.c) The status line, as the game is now */
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
extern unsigned char plain;                     /* The map in plain letters, no colours (the player's: "colour off",
                                                **   pace.c's; kept with the game: main.c's save) */
extern const char* savedir;                     /* (main.c's: dq0 -d's, the save's folder) */
unsigned char __fastcall__ more (unsigned char action);  /* Overlay 2 (words.c's): the commands chores hasn't, as
                                                **   they come (slice 4.1's on): 1 if it was one, carried out */
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
enum { HOW_ANY, HOW_TO, HOW_THROUGH };
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
extern struct cmd cmd;                          /* (main.c's: the command being carried out, cq_cmd's) */
extern unsigned char pc_face;                   /* (The hub's, copied for the overlays) */
#define TOLD        24                          /* (A look's sentences said this answer: look.c's told, ntold) */
extern unsigned told[TOLD];
extern unsigned char ntold;
void lb_said (void);                            /* (A look's words said, lbsaid.s's: words.c's and seen.c's lb_out) */

/* ---- pace.c (part 4): the pace, the watchfulness, the state, small moves, from the line typed */

unsigned char __fastcall__ pace_cmd (const char* l);   /* 1 if it was one of them, carried out (else the command's;
                                                **   but a pace said with a move set first) */
unsigned char __fastcall__ pacing (const char* l);     /* Overlay 9 (pace.c): 0 if line l can't be one of them (none
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

/* ---- resolve.c (overlay 11): the command's things found among those the character perceives, its unsaid filled (it,
** the last thing named, the things near, the target), which of several; of several it must be one of, which, asked */

#define RV_ON       0                           /* (resolve's: the command carried on with; seen to, a question */
#define RV_DONE     1                           /*   asked, the target set, said why not) */
unsigned char __fastcall__ answer (unsigned char chosen);   /* The command (cmd) an answer to resolve's question?
                                                **   Then cmd the command asked about, on the one chosen: 1; else 0
                                                **   (chosen: choose's answered, the question let go) */
unsigned char resolve (void);                   /* The command (cmd) resolved: RV_ON or RV_DONE */
void __fastcall__ taken_next (char* w);         /* "Take all"'s next thing to take up, into w ("": none) */
void __fastcall__ throw_words (unsigned r);     /* A throw told (the things' IO_THROW's result, its TH_* times 256:
                                                **   IR_NOT_HERE, no mark it sees; IR_FULL, none said) */
extern unsigned char untarget;                  /* (main.c's: the target let go at the next command: the character
                                                **   made to turn, by a trap set off, a blow from one it doesn't see) */

/* ---- theme.c (overlay 5): the theme's words and its puzzle (theme.h), from its copy in the overlay's bank */

#define TCOPY       ((struct theme*) 0x9000)    /* (The theme, in overlay 5's bank: main.c reads it in) */
struct quest {                                  /* The game's theme and its end (kept in the save) */
    unsigned char   theme;                      /* The theme's number (the save's: the one it began with) */
    unsigned char   done;                       /* The game's ending (theme.h's END_*: 0 not yet) */
};
extern struct quest Q;                          /* (main.c's) */
extern char tname[32];                          /* A theme's name for something (theme_named's) */
enum { TN_THEME, TN_STRATUM, TN_ROOM, TN_LIGHT };
enum { Q_NONE, Q_TOLD, Q_DONE, Q_OPEN, Q_RULED };   /* (puzzle's: not its; words only; a rule done; the way out
                                                **   opened; the place ruled) */
void __fastcall__ theme_load (unsigned char t);     /* Overlay 5: the theme (its number: the file's counted round)
                                                **   read in: into the hub's theme bank and the overlay's; its kinds
                                                **   into the hub's catalogue (the things' read in first) */
void __fastcall__ theme_named (unsigned char what, unsigned char n);   /* Overlay 5: its name, a stratum's (n its
                                                **   number) into tname; a room's (n its kind), its light, added to
                                                **   lb */
void __fastcall__ story (unsigned char resumed);    /* Overlay 5: the theme's name (and its story, a new game's) */
void way_words (void);                          /* Overlay 5: the way out, shut or open */
unsigned char __fastcall__ history (unsigned long seed);    /* Part 7: a game's own theme made, themegen's (main.c):
                                                **   the theme to read (0xFF its own, else a stand-in's) */
void __fastcall__ ending (unsigned char how);   /* Overlay 5: an ending's words (END_*) */
unsigned char __fastcall__ puzzle (struct cmd* c);      /* Overlay 5: a command the puzzle's (Q_*: the world's
                                                **   judgement, IO_PUZZLE, worded) */
unsigned char __fastcall__ othings (unsigned char op, const char* name, const char* into, unsigned char to);
                                                /* (main.c's) The things asked (from an overlay: the names in the
                                                **   main RAM): the result; the request's x in othings_x,
                                                **   the things' words in lb */
extern unsigned othings_x;
void __fastcall__ hub_mem (unsigned char bank, void* to, const void* from, unsigned n);    /* (main.c's) Bytes
                                                **   between a hub's bank and the main RAM, from an overlay */

/* ---- The overlays (bank.s: their stubs) */

void title (void);                              /* Overlay 1: the title, the pace asked */
void __fastcall__ entered (unsigned char resumed);  /* Overlay 1: the character come into the dungeon */
void help (void);                               /* Overlay 1 (help.c's): the commands, listed (help.txt) */
void __fastcall__ unknown (unsigned char f);    /* Overlay 1: a verb the game doesn't know (f 0), or doesn't do yet
                                                **   (f 1), said back from the command's line (cl) */
extern const char* cl;                          /* The command's line (main.c's) */
unsigned char parts_in (void);                  /* Overlay 6 (parts.c's): the line typed queued in its parts, 0 no room */
#define QMAX        256                         /* The lines typed ahead (term.c's queue, qlen) */
extern char queue[QMAX];
extern unsigned qlen;
struct sway;
void __fastcall__ sway_words (const struct sway* s);   /* Overlay 6: the character's way with a denizen, the
                                                **   denizens' task's answer (the hub's sway, copied), into lb */
void __fastcall__ dig_words (unsigned char how, unsigned char feet);   /* Overlay 1: digging's, how it goes */
unsigned char __fastcall__ task_words (unsigned w, unsigned char named);   /* Overlay 3: a long task's (task.c) */
void __fastcall__ beat_words (unsigned w);
void __fastcall__ felt_words (unsigned w);      /* (Overlay 7's) A walk in the dark's beat (task.c's) */
void facing_words (void);                       /* (Overlay 7's) The way it believes it faces (task.c's: slice 4.2's) */
unsigned char __fastcall__ stop_kind (const char* word);  /* Overlay 7 (task.c's): the task a stop names */
unsigned char __fastcall__ answer_of (const char* l);     /*   and a question's answer */
void __fastcall__ bar_text (char* s, unsigned done, unsigned len);  /* (Overlay 3) */
void __fastcall__ wait_go (unsigned char action);   /* (Overlay 13) A wait, a sleep or a rest begun (task.c's) */
void __fastcall__ time_said (unsigned w, unsigned char b);  /* (Overlay 13) Time passed, told: E_TIME's */
void __fastcall__ state_words (unsigned char cond);
unsigned char __fastcall__ free_cmd (const char* l);   /* Overlay 7: a line typed as the character's busy, carried
                                                **   out at once (1), held (0), or held and its fight stopped for it
                                                **   (2) */
extern unsigned char autogo;                    /* (main.c's: dq0 -a, every soft question answered yes, unasked) */
extern unsigned char harm_told;                 /* (The states a harm's words told this answer: main.c's) */
char* __fastcall__ env (const char* name, char* buf, unsigned char size);  /* The environment's (main.c) */
void __fastcall__ appraise (const struct ireq* q, signed char g, unsigned char ex);   /* Overlay 1: a thing judged
                                                **   (its value, its quality, both: ex's EX_*) */
extern char dig_with[16];                       /* The tool a dig's taken up, to name (then ""): main.c's */
void __fastcall__ climb_words (unsigned char how, unsigned char feet); /* Overlay 1: climbing's (a ledge's:
                                                                        **   CW_*) */
enum { CW_RISES = 0x40, CW_FALLS };
void __fastcall__ portal_words (unsigned char out);                     /* Overlay 1: a door worked's, into lb */
void __fastcall__ harm_words (unsigned w, unsigned cond);               /* Overlay 1: harm's (an E_HARM's w1,
                                                                        **   the conditions; the wounds in lb) */
void __fastcall__ fixt_words (const struct msg* m);    /* Overlay 4: a portal worked's (an E_PORTAL: a door's,
                                                **   portal_words; an opening's with what it shows) */
void __fastcall__ trap_words (unsigned w);      /* Overlay 4: a trap's (an E_TRAP's w1) */
void __fastcall__ trap_name (unsigned char kind, unsigned char trigger, unsigned char state);  /* Overlay 10: a
                                                **   trap's name, added to lb */
void den_way (void);                            /* Overlay 4: a denizen in the way (its name in lb) */
struct msg;
struct fight;
unsigned __fastcall__ fight_words (const struct msg* m);    /* Overlay 6: a blow (an E_FIGHT; its record from the
                                                **   ring copied into lb's end: FIGHT_LB), into lb: the tenths the
                                                **   character's knocked senseless for, or 0 */
#define FIGHT_LB    ((struct fight*) (lb + sizeof lb) - 1)
unsigned char __fastcall__ fight_target (struct cmd* c);    /* Overlay 6: VB_ATTACK_OBJECT's denizen beside
                                                **   the character to strike at (its name has the word; it lies that
                                                **   way): its entry plus 1, or 0 and why not in lb; VB_DEFEND's stance
                                                **   set, told in lb (0); a blow at a door, a window, a portcullis:
                                                **   FT_THING */
#define FT_THING    0xFE
#define FT_CLOSE    0x40                        /* (| the actor plus 1: out of reach, closed with first, I_CLOSE) */
void __fastcall__ heard_words (unsigned char what, unsigned char rel);  /* Overlay 9: a denizen heard (an E_DEN's
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
extern unsigned lbn;                            /* (What a look's said of its words already: lb's before) */
extern unsigned char lb_stream;                 /* (A look's words said a sentence at a time, as they're made) */
extern struct sight sight_copy;
struct sight* sight_get (void);                 /* The senses' record copied into sight_copy (look.c's): it */
const char* tt (void);                          /* The things' words (the UI bank's) copied into lb (look.c's): it */
extern unsigned char sight_light;
extern char sight_lamp[14];                     /* (What gives its light, named: the hub's pc_lamp) */
extern unsigned char sight_close;               /* (A close look: the room's figures, its size and its ceiling's; 8,
                                                **   a look one way's words) */
/* ---- way.c: looking one way, listening and sniffing one way (HOUSE.md's slice 4.0) */
unsigned char __fastcall__ look_way (unsigned char action);  /* Part 6: "look north", "look at the door", "listen at
                                                **   the door", "sniff" (the command's action, VB_*); VB_NONE: a
                                                **   listen's or a sniff's end (main.c's E_READY); a thing examined,
                                                **   its aspects: 0, or a door's work to do (PA_CHECK) */
void focus_words (void);                        /* Overlay 13: a look one way's words, into lb (words.c's) */
extern unsigned char lsn;                       /* (main.c's: a listen, 1 one way, 2 about; 0, none) */
extern unsigned long heard_at;                  /* (main.c's: when a sound was last told) */

/* ---- bank.s */

void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[15];
unsigned char early (void);                     /* Part 12: before a command's things are looked for (in shock, refused:
                                                **   2; first aid's, 1; else 0) */
void tend (void);                               /* Part 12: first aid (VB_TEND) */
unsigned char body_words (void);                /* Part 12: what time did to its body, told (8: dead of it) */
extern unsigned char news_due;                  /* (main.c's: the body's news held as it sleeps, part 12's) */

/* ---- The swapped parts (bank.s's stubs: swap.inc), run in the main RAM's top (dq0.cfg's SWAP), a part at a time */

void __fastcall__ args (int argc, char* argv[]);    /* Part 7 (main.c): the game's arguments read (begin's) */
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
unsigned char search_go (void);                 /* (Part 10's) A search begun, by its form (search.c) */
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

#endif
