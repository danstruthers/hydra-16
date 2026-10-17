/*
** theme.h - Dingus Quest Zero's themes (DENIZENS.md: the theme): what a dungeon is about, as data the engine reads,
** not code.  Each a text file (games/dq0/themes, a .txt each: its format in the first), made at build time into the
** game's lib/dq0/dq0t.dat (make.js: THEME_SIZE bytes a theme, struct theme's layout then its words); one picked by
** the game's seed as it starts (the seed's second byte, counted round the themes: a game's seed, the clock's, picks
** any; the tests' small seeds the first, the one they were written against) and kept in the save.  The UI reads it
** into the hub's theme bank (THEME_BANK, at $8000 there), where every task finds it, and into its own overlay 5's
** bank (theme.c's copy, at $9000 there).
**
** A theme says: its name and its story; its strata from the top down (each one's name, and the styles its plan may
** be grown in, picked from by its seed, and its purpose: a keep, a temple ..., fortified or not, its built rooms'
** uses' plan, the maker's); its words for the rooms (by a room's kind, world.h's: eight, then the uses') and their
** light; the
** bestiary's kinds that live in it (dq0d/kinds.txt's, a bit each); its own kinds of things (the catalogue's last
** NTKINDS, KI_THEME on: the world copies them into the hub's catalogue as the game starts, the UI asking); its
** puzzle: the pieces (a kind of its own each, the strata it may lie in, hidden or not), each one's stratum picked
** from the seed and its place (a chunk of that stratum's halls) as the stratum's plan is grown, the maker's
** (dq0m/plan.c), put down as the chunk is first made (dq0m/stock.c's list, the world's to put down); and the rules
** of what's done with them (read, combine, put one on or in another, use one on the way out), each with its words
** and what it does (the way out opened, the place ruled);
** its mission (to escape, to rule the place, or either); the way out's lock in words, shut and open; and the
** endings' words (escaped, ruled, dead).
*/

#ifndef _THEME_H
#define _THEME_H

#define THEME_SIZE      0x1000                  /* A theme's bytes in dq0t.dat (its record, then its words) */
#define THEME_HEAD      982                     /* (struct theme's: make.js lays it out the same) */
#define NTKINDS         8                       /* Its own kinds of things, */
#define KI_THEME        (NKINDS - NTKINDS)      /*   the catalogue's from here (items.h's NKINDS) */
#define NPIECES         8                       /* The puzzle's pieces (and its fixtures: an altar, a throne) */
#define NRULES          8                       /* Its rules */
#define TSTRATA         8                       /* (world.h's MAXSTRATA) */

struct tpiece {
    unsigned char   kind;                       /* Its kind (0 ...: KI_THEME on) */
    unsigned char   d0, d1;                     /* The strata it may lie in, from the top (0) down */
    unsigned char   hide;                       /* Concealed, how well (0-9: found by searching); 0xFF in plain view */
    unsigned char   qty;
};
struct trule {
    unsigned char   verb;                       /* TR_* */
    unsigned char   a, b, c;                    /* Its things' kinds (the theme's: 0 ...; 0xFF none): read a; combine
                                                **   a with b, making c; put a on (or in) b; use a on the way out */
    unsigned char   effect;                     /* TE_* */
    unsigned        words;                      /* What's told as it's done (an offset of the theme's words) */
};
enum { TR_READ, TR_COMBINE, TR_PUT, TR_OUT };
enum { TE_NONE, TE_OPEN, TE_RULE };             /* Nothing more; the way out opened; the place ruled (an ending) */
#define TM_ESCAPE       0x01                    /* The mission: to escape, */
#define TM_RULE         0x02                    /*   to rule the place (either, if both) */
enum { END_NONE, END_ESCAPED, END_RULED, END_DIED };
enum { P_HALLS, P_KEEP, P_TEMPLE, P_CELLARS, P_CRYPT, P_WORKS };  /* A stratum's purpose (DUNGEON.md: built to a plan) */
#define P_FORT          0x80                    /*   and fortified */
#define TUSES           12                      /* The uses' words past the rooms' eight (world.h's U_KITCHEN on) */

struct theme {
    unsigned char   count;                      /* The themes in the file (the first one's) */
    unsigned char   mission;                    /* TM_* */
    unsigned char   npieces, nrules;
    char            name[32];                   /* "the Crypt of the Dead King" */
    unsigned char   style[TSTRATA][2];          /* Each stratum's styles (level.h's ST_*), one picked by its seed */
    char            stratum[TSTRATA][20];       /* Each stratum's name, from the top down */
    char            room[8][16];                /* A room's words by its kind (world.h's struct room's kind) */
    char            light[40];                  /* A room lit by its own light: by what ("lit by torches ...") */
    unsigned long   dens;                       /* The bestiary's kinds that live in it: a bit each (kinds.txt's
                                                **   order, 1 the first) */
    struct kind     kind[NTKINDS];              /* Its own kinds of things (items.h's) */
    struct tpiece   piece[NPIECES];
    struct trule    rule[NRULES];
    unsigned        story, lock, opened;        /* Its words (offsets in the theme; 0 none): its story; the way out
                                                **   shut (tried, examined), and open; */
    unsigned        end[4];                     /*   the endings' (END_*) */
    char            uses[TUSES][16];            /* A room's words by its use, past the eight (U_KITCHEN ...) */
    unsigned char   purpose[TSTRATA];           /* Each stratum's purpose (P_*, P_FORT) */
};
#define THEME           ((struct theme*) 0x8000)    /* (In the hub's theme bank) */
typedef char theme_head[sizeof (struct theme) == THEME_HEAD ? 1 : -1];

/* A piece's place (the maker's, in the theme bank after the theme): its stratum (0xFF: not decided yet, its stratum
** not grown), its chunk (from the stratum's x0, y0), and once it's put down its square (in the frame; 0: not yet) */
struct tplace {
    unsigned char   sn;
    unsigned        cx, cy;
    unsigned        x, y;
};
#define THEME_PLACE     ((struct tplace*) 0x9000)

#endif
