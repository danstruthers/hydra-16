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
**   intro.c    (overlay 1) the title and the pace (the character is hmchar's: a program of its own,
**              games/hmchar, that main.c runs, and whose file the referee reads)
**
** Memory: the program's code and data from $0800 (its BSS must end 768 bytes or more under $7F00: dq0.cfg); the
** overlays' code in banks of their own, put in the window at $8000 as they're called (bank.s).  Code in an overlay
** can call any function; but what's in the window goes away while bread and bwrite work, so they're only ever
** given the main RAM's addresses.
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
unsigned char __fastcall__ ask (const char* keys);      /* One of keys typed, waiting (the world stopped): which */
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
extern const char* const stratum_name[8];       /* (A stand-in theme's names of the strata) */
void __fastcall__ map (const char* rules);      /* The map of what it's seen round it, and the character's
                                                **   state beside it (rules: the referee's short lines) */

/* ---- The overlays (bank.s: their stubs) */

void title (void);                              /* Overlay 1: the title, the pace asked */
void help (void);                               /* Overlay 1: the commands, listed */
void __fastcall__ dig_words (unsigned char how, unsigned char feet);   /* Overlay 1: digging's, how it goes */
void words (void);                              /* Overlay 2: what the character perceives, into lb */
extern char lb[400];
extern struct sight sight_copy;
extern unsigned char sight_light;

/* ---- bank.s */

void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[4];

#endif
