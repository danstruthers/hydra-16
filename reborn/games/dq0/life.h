/*
** life.h - Dingus Quest Zero's record of a life (DUNGEON.md's a character's life; the plan for the game between dungeons,
** its step 3): life.dat, a small file at the top of the save's folder (dq0 -d), read only as a game starts, by the UI (its
** head) and by dq0life (all of it): no main RAM during play.  Plain bytes, little-endian as cc65's are: any program of the
** game's reads it with read (fd, &l, sizeof l) and this header.  The save's folder:
**
**     life.dat         this record
**     pc.hm            the character's sheet (the harn library's: harn_read)
**     kit.dat          what it carries (../dq0w/outfit.h's struct kitrec, one after another)
**     d1, d2 ...       each dungeon's own folder (game.dat, world.dat, theme.txt, the change records ...), all of them
**                      kept; the hub's savedir, in the game, is the dungeon's
**
** The file: the head, 32 bytes, then LIFE_DUNGEONS records of 8 (all 0 past the last dungeon; a dungeon past the last record
** not recorded), 544 bytes in all, whatever's been done.  What later steps add (the clues, the thread) goes after that, each
** found by its offset in the head (0: none), so nothing here moves.
*/

#ifndef _LIFE_H
#define _LIFE_H

#define LIFE_FILE       "life.dat"
#define LIFE_MAGIC      0x464C                  /* "LF" */
#define LIFE_VER        1
#define LIFE_DUNGEONS   64

enum { LO_NONE, LO_SOLVED, LO_LEFT, LO_DIED };  /* (A dungeon's outcome: not over yet; the three ends) */

/* The head of a dungeon's game.dat (the UI's: main.c's the save, its versions there), what dq0life reads of it too: the dungeon's
** end */
#define SAVE_MAGIC      0x5144                  /* "DQ" */
#define SAVE_VER        42
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
    unsigned char   plain;                      /* (The map in plain letters: the player's, dq0.h's) */
};

struct life_head {
    unsigned        magic;
    unsigned char   version;
    unsigned char   flags;                      /* (None yet) */
    unsigned long   seed;                       /* The life's: dungeon n's comes from it and n (dq0life's) */
    unsigned long   dseed;                      /* This dungeon's (its record's too: the UI reads only the head) */
    unsigned        dungeon;                    /* Which it's in: 1 on, its folder d1 ... */
    unsigned long   date;                       /* The day this dungeon began (LIFE_YEAR's and the like's): the game's clock
                                                **   starts afresh in each; the hour's its seed's, as ever (dq0d's s_hour) */
    unsigned        born;                       /* The year the character was born, its birthday its sheet's (harn.h's
                                                **   HARN_BD: month and day, no year; Character 3 leaves the year to the GM) */
    int             reputation;                 /* (Nothing moves it yet: the plan's step 7) */
    unsigned        clues, thread;              /* (Their offsets in the file, 0 none: the plan's steps 10, 15) */
    unsigned char   room[6];
};

struct life_dungeon {
    unsigned long   seed;                       /* Its seed */
    unsigned        days;                       /* The whole days it took, by the game's clock: set as the next begins */
    unsigned char   outcome;                    /* LO_*: set as the next begins */
    unsigned char   spare;
};

struct life {
    struct life_head    h;
    struct life_dungeon d[LIFE_DUNGEONS];       /* (Dungeon n's: d[n - 1]) */
};

/* The calendar (the book's, Character 3: twelve months of 30 days; harn.h's month names, 1 Nuzyael ... 12 Morgat): a date's a
** count of days, so what passes between dungeons need only add to it */
#define LIFE_YEAR(t)        ((unsigned) ((t) / 360))
#define LIFE_MONTH(t)       ((unsigned char) ((t) % 360 / 30) + 1)
#define LIFE_DAY(t)         ((unsigned char) ((t) % 30) + 1)
#define LIFE_DATE(y, m, d)  ((y) * 360UL + ((m) - 1) * 30 + (d) - 1)
/* Its age in whole years on date t: h the head, bd its birthday, its sheet's HARN_BD (month << 8 | day) */
#define LIFE_AGE(h, t, bd)  (LIFE_YEAR (t) - (h)->born - ((t) % 360 < (((bd) >> 8) - 1) * 30 + ((bd) & 0xFF) - 1))

#endif
