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
**     clues.dat        the clues the life has carried out of its dungeons (struct clue, below: dq0life's; each dungeon's own
**                      in its folder, the UI's)
**     heard.txt        the rumour a clue read makes (dq0life's: a part-written theme begun by its rumour line, the plan's
**                      step 10; the rumours between dungeons, step 7's, tell it first)
**     quest.txt        the rumour followed (dq0acct's), till dq0life moves it into the next dungeon's folder for themegen
**     d1, d2 ...       each dungeon's own folder (game.dat, world.dat, theme.txt, quest.txt, the change records ...), all of them
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

#define LF_BETWEEN      0x01                    /* (The record's flags: the time between yet to be told, dq0acct's, the next dungeon's seed and
                                                **   folder not begun: dq0life runs it again if it's found so) */
#define LF_OVER         0x02                    /* The life's over: dead, in hard mode's dungeon or between two; the next start's a new life */
#define LF_ABROAD       0x04                    /*   dead between two dungeons (dq0acct's), of its wounds' rolls */
#define LF_HARD         0x08                    /* Hard mode's life: its dungeon's save said so as it ended, the next's kept so (the UI's) */
#define LD_DIED         0x01                    /* (A dungeon's: it died in it, hard mode's; its outcome still LO_SOLVED if it was, Q2's) */

struct life_head {
    unsigned        magic;
    unsigned char   version;
    unsigned char   flags;                      /* LF_* */
    unsigned long   seed;                       /* The life's: dungeon n's comes from it and n (dq0life's) */
    unsigned long   dseed;                      /* This dungeon's (its record's too: the UI reads only the head) */
    unsigned        dungeon;                    /* Which it's in: 1 on, its folder d1 ... */
    unsigned long   date;                       /* The day this dungeon began (LIFE_YEAR's and the like's): the game's clock
                                                **   starts afresh in each; the hour's its seed's, as ever (dq0d's s_hour) */
    unsigned        born;                       /* The year the character was born, its birthday its sheet's (harn.h's
                                                **   HARN_BD: month and day, no year; Character 3 leaves the year to the GM) */
    int             reputation;                 /* The standing: what solving earns (the theme's reward line, dq0acct's:
                                                **   the plan's step 7), told in the account and the life's */
    unsigned        clues, thread;              /* (Their offsets in the file, 0 none: the plan's steps 10, 15) */
    unsigned char   room[6];
};

struct life_dungeon {
    unsigned long   seed;                       /* Its seed */
    unsigned        days;                       /* The whole days it took, by the game's clock: set as the next begins */
    unsigned char   outcome;                    /* LO_*: set as the next begins, or (hard mode) as it dies in it */
    unsigned char   flags;                      /* LD_* */
};

struct life {
    struct life_head    h;
    struct life_dungeon d[LIFE_DUNGEONS];       /* (Dungeon n's: d[n - 1]) */
};

/* The clues (the plan for the game between dungeons, its steps 9 and 10): each writing of a dungeon's theme (a kind of its own with a
** read rule), a record of CLUE_SIZE bytes in clues.dat, the dungeon's folder's by its rule (record r its rule r: the UI's, as it's
** read or copied, and as the dungeon's left alive); at the top of the save's folder, one after another, what the life has carried out
** of its dungeons (dq0life's, as the next begins: those read, those carried or copied).  A thing that carries one: an old writing
** (items.h's KI_WRITING), a copy (KI_COPY), its fuel CLUE_HERE | its rule while it's in its own dungeon (CLUE_COPY too, a copy),
** else its number at the top (from 1) */
#define CLUES_FILE      "clues.dat"
#define CLUE_SIZE       448
#define CLUE_HERE       0x8000
#define CLUE_COPY       0x4000
#define CLUE_NOCOPY     (-128)                  /* (copy's: none made) */
enum { CL_NONE, CL_UNREAD, CL_RIGHT, CL_PIECE, CL_WRONG };      /* (How it's been read: no record; not read; the three grades'
                                                **   words: a success's, a failure's, a critical failure's; a critical
                                                **   success's the right one and CL_MORE) */
#define CL_MORE         0x80
struct clue {
    unsigned char   state;                      /* CL_*, CL_MORE */
    unsigned char   dungeon;                    /* Where it was found (1 ...) */
    unsigned char   rule;                       /*   its rule there */
    signed char     cond, copy;                 /* Its condition (the book's, Treasure 8: steps of 5, 2 perfect ... -2
                                                **   very poor, 4 less rigged), and a copy's (by its copier's Drawing;
                                                **   CLUE_NOCOPY none) */
    unsigned char   work, land;                 /* What its place is (theme.h's W_*; its high four bits the wrong place's
                                                **   land), its land (themegen's history.txt's: 0 mountains, hills,
                                                **   forest, marsh, coast, plain; its high four bits its apparent
                                                **   condition's steps plus 2: a rigged one's told as another's) */
    unsigned char   carried;                    /* (In the dungeon's file, the record's check, by the game's seed: read.c's;
                                                **   at the top, dq0life's: carried out on a thing, 1; a copy, 2) */
    char            tongue[9], script[9];       /* What it's in (its people's: "" common, ardic) */
    char            thing[14];                  /* What it's written on ("grave slab") */
    char            people[16];                 /* Who wrote it ("the hill-kings"; "" unknown) */
    char            place[24];                  /* The place it names (the one-link clue's: "the Ostmarkan Fells"), */
    char            wrong[24];                  /*   and the place a wrong reading names in its stead */
    char            words[CLUE_SIZE - 104];     /* What it says (its theme's rule's words) */
};
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
