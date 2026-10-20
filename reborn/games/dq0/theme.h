/*
** theme.h - Dingus Quest Zero's themes (DENIZENS.md: the theme): what a dungeon is about, as data the engine reads,
** not code: a text file (its format in games/dq0/themes/1crypt.txt), its record (THEME_SIZE bytes: struct theme's
** layout, then its words) made from it.  A game's own theme is themegen's (games/themegen): as the game begins, its
** region's history and its theme made from the game's seed into the save's folder (or /ram/dq0 without one),
** theme.txt, and its record compiled from it, theme.dat (themec's); dq0 -t FILE a theme written by hand, filled out by
** themegen.  The stand-ins (games/dq0/themes, made at build time into lib/dq0/dq0t.dat by make.js) are the tests': a
** seed under 65,536 picks one by its second byte, counted round them (the small seeds the first, the one they were
** written against).  The save keeps which (Q.theme: a stand-in's number, or 0xFF the game's own).  The UI reads the
** record into the hub's theme bank (THEME_BANK, at $8000 there), where every task finds it, and into its own overlay
** 5's bank (theme.c's copy, at $9000 there).
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
**
** And its history (the user's, DesignIdeas.md: "determine what the history of the geographic area is, then you can
** build a believable dungeon"; games/dq0/HISTORY.md): the region, its peoples and its eras (who came, what they
** made, how it ended, how long ago), each stratum's layers of it (one to three, oldest first: built to a purpose,
** ruined, reused for another: a temple over an older keep's ruin), each piece's era (it lies among that era's works)
** and its guardians, the inhabitants now (the factions: their kinds, strata, numbers, leaders, lairs and attitudes),
** and the goal (an artifact found, the evil cleared out, a prisoner rescued, a far exit won through to: themegen's
** goal kinds).  A stand-in's history is a line or two of its own, or none (each stratum one layer: built to its
** purpose).  The maker builds each stratum from its layers (dq0m: stratagen), the denizens' populations come from
** the factions (the theme file's lines: popgen's).
*/

#ifndef _THEME_H
#define _THEME_H

#define THEME_SIZE      0x1000                  /* A theme's bytes in dq0t.dat (its record, then its words) */
#define THEME_HEAD      1321                    /* (struct theme's: make.js lays it out the same) */
#define NTKINDS         8                       /* Its own kinds of things, */
#define KI_THEME        (NKINDS - NTKINDS)      /*   the catalogue's from here (items.h's NKINDS) */
#define NPIECES         8                       /* The puzzle's pieces (and its fixtures: an altar, a throne) */
#define NRULES          8                       /* Its rules */
#define TSTRATA         8                       /* (world.h's MAXSTRATA) */

struct tpiece {
    unsigned char   kind;                       /* Its kind (0 ...: KI_THEME on) */
    unsigned char   d0, d1;                     /* The strata it may lie in, from the top (0) down */
    unsigned char   hide;                       /* Concealed, how well (steps: found by searching); 0xFF in plain view */
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

/* ---- The history (HISTORY.md) */
#define TLAYERS         3                       /* A stratum's layers of history, at most */
#define TERAS           6                       /* The eras, */
#define TFACTIONS       8                       /*   the factions */
struct tlayer {                                 /* A stratum's layer (oldest first) */
    unsigned char   era;                        /* Its era (1 ...: the theme's; 0 none said) */
    unsigned char   did;                        /* TL_* */
    unsigned char   work;                       /* Built or reused: its purpose (P_*, P_FORT); ruined: how (EN_*) */
};
enum { TL_BUILT, TL_RUINED, TL_REUSED };
enum { EN_ABANDONED, EN_CONQUERED, EN_COLLAPSED, EN_FLOODED, EN_CURSED, EN_HELD };   /* How an era ended (or not: still held), a work was ruined */
enum { W_KEEP, W_TEMPLE, W_MINE, W_TOMB, W_TOWN, W_HOLD };    /* What an era's people made */
struct tera {                                   /* An era (oldest first) */
    unsigned char   people;                     /* Its people (0 ...: the theme's) */
    unsigned char   tongue;                     /* Their tongue (namegen's: 1 ...) */
    unsigned char   work;                       /* W_* */
    unsigned char   end;                        /* EN_* */
    unsigned char   ago;                        /* How long ago it ended: centuries */
};
struct tfaction {                               /* The inhabitants now (the theme file's faction lines: popgen's) */
    char            name[12];
    unsigned long   kinds;                      /* Its kinds (the bestiary's: as dens, a bit each) */
    unsigned char   d0, d1;                     /* The strata it lives in */
    unsigned char   lair;                       /* Its lair's stratum */
    unsigned char   number;                     /* About how many on each stratum it lives in */
    unsigned char   leader;                     /* Its leader's kind (kinds.txt's, 1 ...; 0 none) */
    unsigned char   attitude;                   /* To the character as it first meets it (hub.h's AT_*) */
};
enum { G_NONE, G_ARTIFACT, G_EVIL, G_PRISONER, G_THROUGH };   /* The goal's kind (G_NONE: the mission's puzzle alone) */

/* stratagen (bank/stratagen: a bank program, the maker's and themegen's): which of a stratum's layers a chunk of it is
** now, from the layers, the stratum's size in chunks and its way in's chunk, and its seed (HISTORY.md) */
struct sgq {
    unsigned char   nl;                         /* Its layers, oldest first (fewer than two: none to plan, 0) */
    struct tlayer   l[TLAYERS];
    unsigned        cw, ch;                     /* Its chunks, each way */
    unsigned        ix, iy;                     /* The way in's chunk */
    unsigned long   seed;                       /* The stratum's */
    unsigned        cx, cy;                     /* The chunk asked about */
};
#define SG_LAYER        0x03                    /* The layer whose work it is now (its purpose the uses' plan's), */
#define SG_RUINED       0x04                    /*   left ruined by a ruin after it (the stock's debris), */
#define SG_UNDER        0x30                    /*   an older layer under it (its remnants the stock's), */
#define SG_HASUNDER     0x40                    /*   if it has one */
unsigned char __fastcall__ layer_of (struct sgq* q);   /* (stratagen's entry: bank_call's) */

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
    unsigned char   purpose[TSTRATA];           /* Each stratum's purpose (P_*, P_FORT: its layers' last built or
                                                **   reused, the uses' plan's) */
    /* Its history (HISTORY.md) */
    unsigned char   goal, goal_sn, goal_guard;  /* The goal (G_*), its stratum (a far exit's: the way out there), its
                                                **   guardians (a faction: 1 ...; 0 none) */
    unsigned char   sage[TSTRATA];              /* Each stratum's age, centuries (0: the world's dice) */
    unsigned char   nlayers[TSTRATA];           /* Each stratum's layers (0: one, built to its purpose) */
    struct tlayer   layer[TSTRATA][TLAYERS];
    unsigned char   neras;
    struct tera     era[TERAS];
    unsigned char   pera[NPIECES];              /* Each piece's era: it lies among that era's works (0: anywhere) */
    unsigned char   pguard[NPIECES];            /*   and its guardians (a faction: 1 ...; 0 none) */
    unsigned char   nfactions;
    struct tfaction faction[TFACTIONS];
    char            file[16];                   /* Its file, to read (the factions' lines, popgen's): a stand-in's
                                                **   name in lib/dq0/themes (make.js's: "1crypt.txt"); a game's own,
                                                **   "": theme.txt in the save's folder (the hub's savedir), or in
                                                **   /ram/dq0 without one */
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
