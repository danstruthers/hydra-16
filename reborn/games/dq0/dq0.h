/*
** dq0.h - Dingus Quest Zero: a text adventure in real time, in random dungeons, on a ruleset's rules (HârnMaster's,
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
void t_end (void);                              /* As it was (the scrolling region the whole screen again) */
void __fastcall__ say (const char* s);          /* Words into the story, wrapped (\n: a new line) */
void sayf (const char* fmt, ...);               /* The same, with %s, %d, %u, %c (and %%) */
void __fastcall__ msg (const char* s);          /* A new line of the story, then s */
void __fastcall__ say_raw (const char* s);      /* A line of the story as it is (not wrapped) */
void blank (void);                              /* An empty line in the story (only one in a row) */
void t_done (void);                             /* The cursor back to the line typed, and everything out */
void __fastcall__ t_status (const char* left, const char* right);  /* The status line drawn */
void status (void);                             /* (main.c) The status line, as the game is now */
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

/* ---- The clock: the book's time, in tenths of a second (a combat round 100), run at the pace */

#define PACE_RELAXED    0
#define PACE_NORMAL     1
#define PACE_MENACING   2
#define PACE_TURNS      3                       /* The world moves only as the character does (a turn game) */
#define ROUND           100                     /* A combat round, 10 seconds (Combat 7) */
extern unsigned long now;                       /* The game's time */
extern unsigned char pace, paused;
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

/* ---- parse.c (overlay 3): a command understood */

enum {
    V_NONE, V_MOVE, V_ENTER, V_APPROACH, V_LOOK, V_EXAMINE, V_SEARCH, V_TURN, V_FACE, V_CLIMB, V_JUMP, V_OPEN, V_CLOSE,
    V_UNLOCK, V_FORCE, V_DIG, V_GET, V_DROP, V_DON, V_DOFF, V_LIGHT, V_DOUSE, V_WAIT, V_MAP, V_SHEET, V_HELP,
    V_PAUSE, V_QUIT, V_SAVE, V_INVENTORY, V_HIDE, V_HOLD, V_STOW, V_PUT, V_HANG, V_RAISE, V_LOWER, V_BREAK, V_DISARM,
    V_BAR, V_BEND,
    V_READ, V_USE, V_COMBINE,                   /* (The theme's puzzle's: theme.c) */
    V_TALK, V_PERSUADE, V_DECEIVE, V_THREATEN, V_CALM, V_BEFRIEND,     /* (A denizen swayed: hub.h's SW_TALK ... in
                                                **   order) */
    V_ATTACK, V_TELL, V_NAME,                   /* (A blow, a request to an ally, a name given: the last, SW_* in
                                                **   order) */
    V_DEFEND                                    /* (A fight's stance: blows.c) */
};
enum { OB_NONE, OB_DOOR, OB_PASSAGE, OB_STAIRS, OB_LADDER, OB_SHAFT, OB_LEDGE, OB_WAY_OUT, OB_WALL, OB_WINDOW, OB_TRAP,
    OB_PORTC, OB_ROOM };                        /* (OB_ROOM: the room, its ceiling: looked at closely) */
enum { HOW_ANY, HOW_TO, HOW_THROUGH };
enum { PR_NONE, PR_IN, PR_ON, PR_FROM, PR_WITH };    /* (The other thing: in it, on it, from it, with it) */
#define GO_UP       8                           /* (A direction: up, down; else a heading, H_N ...) */
#define GO_DOWN     9
struct pcmd {
    unsigned char   verb;                       /* V_* */
    unsigned char   dir;                        /* A heading (the compass's), GO_UP, GO_DOWN, or 0xFF */
    unsigned char   obj;                        /* OB_* */
    unsigned char   orel;                       /* The heading the thing lies (or 0xFF: any) */
    unsigned char   how;                        /* HOW_*: up to it, or through it */
    unsigned char   half, around;               /* (turn half left; turn around, look around) */
    char            word[16];                   /* The first word not understood */
    unsigned char   prep;                       /* PR_*: a word for the other thing (put X in Y, take X from Y) */
    char            word2[16];                  /*   and the first not understood after it (the other thing; a
                                                **   thing fetched; a name given, the last) */
    unsigned char   order;                      /* A request to an ally (V_TELL's: hub.h's OR_*; word to here
                                                **   struct sway's, copied whole) */
};
extern unsigned char pc_face;                   /* (The hub's, copied for the overlay) */
void __fastcall__ parse (char* l, struct pcmd* c);   /* Overlay 3 */

/* ---- pace.c (part 4): the pace, the watchfulness, the state, small moves, from the line typed */

unsigned char __fastcall__ pace_cmd (const char* l);   /* 1 if it was one of them, carried out (else the command's;
                                                **   but a pace said with a move set first) */
struct req;
struct req* __fastcall__ ask_b (unsigned char kind, unsigned char a, unsigned char b);   /* (main.c's) The referee
                                                **   asked, the UI's slot */
void __fastcall__ intend2 (unsigned char kind, unsigned w1, unsigned w2);   /* (main.c's) An intent to the world */

/* ---- theme.c (overlay 5): the theme's words and its puzzle (theme.h), from its copy in the overlay's bank */

#define TCOPY       ((struct theme*) 0x9000)    /* (The theme, in overlay 5's bank: main.c reads it in) */
struct quest {                                  /* The puzzle as it stands (kept in the save) */
    unsigned char   theme;                      /* The theme's number (the save's: the one it began with) */
    unsigned char   done;                       /* The game's ending (theme.h's END_*: 0 not yet) */
    unsigned char   open;                       /* The way out opened */
    unsigned char   flags;                      /* The theme's rules done, a bit each */
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
void __fastcall__ ending (unsigned char how);   /* Overlay 5: an ending's words (END_*) */
unsigned char __fastcall__ puzzle (struct pcmd* c);     /* Overlay 5: a command the puzzle's (Q_*) */
unsigned char __fastcall__ fixed (const char* name);    /* Overlay 5: a thing to hand fixed where it stands: 1, told */
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
void help (void);                               /* Overlay 6: the commands, listed */
struct sway;
void __fastcall__ sway_words (const struct sway* s);   /* Overlay 6: the character's way with a denizen, the
                                                **   denizens' task's answer (the hub's sway, copied), into lb */
void __fastcall__ dig_words (unsigned char how, unsigned char feet);   /* Overlay 1: digging's, how it goes */
void __fastcall__ appraise (const struct ireq* q, signed char g);       /* Overlay 1: a thing judged */
extern char dig_with[16];                       /* The tool a dig's taken up, to name (then ""): main.c's */
void __fastcall__ climb_words (unsigned char how, unsigned char feet); /* Overlay 1: climbing's (a ledge's:
                                                                        **   CW_*) */
enum { CW_RISES = 0x40, CW_FALLS };
void __fastcall__ portal_words (unsigned char out);                     /* Overlay 1: a door worked's */
void __fastcall__ harm_words (unsigned w, unsigned cond);               /* Overlay 1: harm's (an E_HARM's w1,
                                                                        **   the conditions; the wounds in lb) */
void __fastcall__ fixt_words (unsigned w);      /* Overlay 4: a portal worked's (an E_PORTAL's w1: a door's,
                                                **   portal_words) */
void __fastcall__ trap_words (unsigned w);      /* Overlay 4: a trap's (an E_TRAP's w1) */
void __fastcall__ trap_name (unsigned char kind, unsigned char trigger, unsigned char state);  /* Overlay 4: a
                                                **   trap's name, added to lb */
void den_way (void);                            /* Overlay 4: a denizen in the way (its name in lb) */
struct msg;
struct fight;
unsigned __fastcall__ fight_words (const struct msg* m);    /* Overlay 6: a blow (an E_FIGHT; its record from the
                                                **   ring copied into lb's end: FIGHT_LB), into lb: the tenths the
                                                **   character's knocked senseless for, or 0 */
#define FIGHT_LB    ((struct fight*) (lb + sizeof lb) - 1)
unsigned char __fastcall__ fight_target (struct pcmd* c);   /* Overlay 6: V_ATTACK's denizen beside the
                                                **   character to strike at (its name has the word; it lies that way):
                                                **   its entry plus 1, or 0 and why not in lb; V_DEFEND's stance set,
                                                **   told in lb (0); a blow at a door, a window, a portcullis:
                                                **   FT_THING */
#define FT_THING    0xFE
void __fastcall__ heard_words (unsigned char what, unsigned char rel);  /* Overlay 2: a denizen heard (an E_DEN's
                                                **   what, | 0x80 muffled, the way it lies as the character faces),
                                                **   into lb */
extern char heard_what[20];                     /* Its sound's words (SOUND.md: the sound task's, "a scurrying"; "":
                                                **   none), main.c's */
struct portal;
void __fastcall__ portal_look (const struct portal* p, signed char g);  /* Overlay 4: a portal looked at closely
                                                **   (the maker's record of it, the hub's pview, copied), judged at
                                                **   the appraiser's grade g, into lb */
void ceiling_glance (void);                     /* Overlay 4: a ceiling at a glance (sight_copy's), added to lb */
void words (void);                              /* Overlay 2: what the character perceives, into lb */
void stir_words (void);                         /* Overlay 2: the denizens it sees, moving, into lb (or "": as
                                                **   they were told last) */
extern char lb[400];
extern struct sight sight_copy;
extern unsigned char sight_light;
extern char sight_lamp[14];                     /* (What gives its light, named: the hub's pc_lamp) */
extern unsigned char sight_close;               /* (A close look: the room's figures, its size and its ceiling's) */

/* ---- bank.s */

void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[9];

/* ---- The swapped parts (bank.s's stubs: swap.inc), run in the main RAM's top (dq0.cfg's SWAP), a part at a time */

void __fastcall__ begin (int argc, char* argv[]);   /* Part 1 (main.c): the arguments, the world started, the game
                                                **   begun */
void finish (void);                             /* Part 1: the game kept, the world stopped */
void save_game (void);                          /* Part 1: the game kept (if it's kept: -d) */
void __fastcall__ command (char* l);            /* Part 2 (main.c): a command, the parser's, carried out */
void __fastcall__ take (const char* name);      /* Part 2: a thing taken up, by its name */
extern char _SWAP_START__[];                    /* (The area: the overlays read in through it, as the game starts) */

#endif
