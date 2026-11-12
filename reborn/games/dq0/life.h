/*
** life.h - Dingus Quest Zero's record of a life (DUNGEON.md's a character's life; the plan for the game between dungeons,
** its step 3): life.dat, a small file at the top of the save's folder (dq0 -d), read only as a game starts, by the UI (its
** head) and by dq0life (all of it): no main RAM during play.  Plain bytes, little-endian as cc65's are: any program of the
** game's reads it with read (fd, &l, sizeof l) and this header.  The save's folder:
**
**     life.dat         this record
**     pc.hm            the character's sheet (the harn library's: harn_read)
**     kit.dat          what it carries (../dq0w/outfit.h's struct kitrec, one after another)
**     kit0.dat         the same, what the life set out with (the world's, as a life begins): dq0acct restocks to it
**     lives.dat        the lives over in this folder: each one's record as it ended
**     rumours.txt      the rumours heard between two dungeons (themegen -r's, dq0life runs it: part-written themes, each begun by its
**                      rumour line; a real game's life alone, its seed 65,536 or more)
**     clues.dat        the clues the life has carried out of its dungeons (records.def's struct clue: dq0life's; each dungeon's own
**                      in its folder, the UI's)
**     heard.txt        the rumour a clue read makes (dq0life's: a part-written theme begun by its rumour line, the plan's
**                      step 10; the rumours between dungeons, step 7's, tell it first)
**     quest.txt        the rumour followed (dq0acct's), till dq0life moves it into the next dungeon's folder for themegen
**     d1, d2 ...       each dungeon's own folder (game.dat, world.dat, theme.txt, quest.txt, the change records ...), all of them
**                      kept; the hub's savedir, in the game, is the dungeon's
**
** The record itself (struct life: its head, struct life_head, and its dungeons' lines, struct life_dungeon; LIFE_FILE,
** LIFE_MAGIC, LIFE_VER, LIFE_DUNGEONS, the flags LF_* and LD_*) is declared in records.def, with the other records the
** programs between dungeons pass to each other: the build's obj/gen/dq0rec.h, included here.
*/

#ifndef _LIFE_H
#define _LIFE_H

#include "dq0rec.h"                             /* (struct life, life.dat's: records.def's) */

enum { LO_NONE, LO_SOLVED, LO_LEFT, LO_DIED };  /* (A dungeon's outcome: not over yet; the three ends) */

/* The head of a dungeon's game.dat (the UI's: main.c's the save, its versions there), what dq0life reads of it too: the dungeon's
** end */
#define SAVE_MAGIC      0x5144                  /* "DQ" */
#define SAVE_VER        45                      /* (43: the theme's record, theme.dat, its peoples' tongues and scripts:
                                                **   step 9's, 2026-11-06; 44: a clue's age and what's been asked of it,
                                                **   the scholar's, step 12's; 45: the letters, game.dat's after the
                                                **   shortcuts, kit.dat's struct kitrec's let: hub.h's designators) */
struct quest {                                  /* The game's theme and its end (kept in the save) */
    unsigned char   theme;                      /* The theme's number (the save's: the one it began with) */
    unsigned char   done;                       /* The game's ending (theme.h's END_*: 0 not yet) */
    unsigned char   solved;                     /* The dungeon solved, as it's told (the ending it's to have, END_ESCAPED or
                                                **   END_RULED; 0 not yet), kept whatever comes of it: dying on the way
                                                **   out still solved (Q2) */
};
struct save {
    unsigned        magic;
    unsigned char   version, pace;
    unsigned long   seed;
    char            referee[12];                /* The ruleset's referee (dq0 -r NAME's: dq0NAME) */
    unsigned char   outcome;                    /* How it ended (LO_*, from q: solved, else left, else died): the record's */
    struct quest    q;                          /* (Its theme, its puzzle, and its ending: a game done isn't carried
                                                **   on, but a new one begun) */
    unsigned char   plain;                      /* (The map in plain letters, bit 0: the player's, dq0.h's; bits 1-7 a
                                                **   vow of fasting's time left as it was kept, in FAST_STEPs rounded
                                                **   up, 0 none: hself.c's fasting, the user's, 2026-11-11) */
};

#define FAST_STEP       7200UL                  /* (A vow of fasting's time kept in steps of 12 minutes, the clock's tenths:
                                                **   a day's vow 120 of them, in a byte, save's plain and the head's fast) */

/* The clues (the plan for the game between dungeons, its steps 9 and 10): struct clue, clues.dat's records, CLUE_SIZE bytes each
** (CLUE_FILE, CLUE_WORDS, CL_MORE, CA_*), is records.def's (dq0rec.h).  A thing that carries one, an old writing (items.h's
** KI_WRITING) or a copy (KI_COPY): its fuel CLUE_HERE | its rule while it's in its own dungeon (CLUE_COPY too, a copy), else
** its number at the top of the save's folder (from 1) */
#define CLUE_HERE       0x8000
#define CLUE_COPY       0x4000
#define CLUE_NOCOPY     (-128)                  /* (copy's: none made) */
enum { CL_NONE, CL_UNREAD, CL_RIGHT, CL_PIECE, CL_WRONG };      /* (How it's been read, its state: no record; not read; the
                                                **   three grades' words: a success's, a failure's, a critical failure's;
                                                **   a critical success's the right one and CL_MORE) */
typedef char clue_fits[sizeof (struct clue) == CLUE_SIZE ? 1 : -1];

#define LIVES_FILE      "lives.dat"             /* (The lives over in this folder, each record as it ended, one after another: dq0life's, as a
                                                **   new life overwrites life.dat) */

/* The calendar (the book's, Character 3: twelve months of 30 days; harn.h's month names, 1 Nuzyael ... 12 Morgat): a date's a
** count of days, so what passes between dungeons need only add to it */
#define LIFE_YEAR(t)        ((unsigned) ((t) / 360))
#define LIFE_MONTH(t)       ((unsigned char) ((t) % 360 / 30) + 1)
#define LIFE_DAY(t)         ((unsigned char) ((t) % 30) + 1)
#define LIFE_DATE(y, m, d)  ((y) * 360UL + ((m) - 1) * 30 + (d) - 1)
/* Its age in whole years on date t: h the head, bd its birthday, its sheet's HARN_BD (month << 8 | day) */
#define LIFE_AGE(h, t, bd)  (LIFE_YEAR (t) - (h)->born - ((t) % 360 < (((bd) >> 8) - 1) * 30 + ((bd) & 0xFF) - 1))

#endif
