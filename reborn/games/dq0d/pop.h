/*
** pop.h - Dingus Quest Zero's populations (Phase 3D): the denizens' task (dq0d) and its population generator, popgen
** (bank/popgen: a bank program, sdk/c/include/bankprog.h's), between them.
**
** A theme file (the history's: themegen's, or one written by hand, in the save folder: THEME_FILE) says who lives in
** the dungeon now, its inhabitants, a line a faction:
**   faction | NAME | KINDS | STRATA | NUMBER | LEADER | LAIR | ATTITUDE | OTHERS
** NAME its name (11 letters at most); KINDS the bestiary's kinds it's made of (dq0d/kinds.txt's keys, spaces between:
** 4 at most, the first the most of it); STRATA the strata it lives on, first-last from the top (0); NUMBER how many
** it is; LEADER its leader's kind (a key), or -; LAIR the stratum its lair's on (its home: its leader there, and the
** most of it); ATTITUDE to the character as it first meets it, h hostile, u unfriendly, w wary, i indifferent, f
** friendly; OTHERS how it stands with the other factions, "NAME h, NAME w ..." (h hostile, w wary, f friendly; the
** rest indifferent; each pair the worse of the two ways it's said), or -.  A line that starts with a space goes on
** with the line before (the theme files' way).  POP_FACT factions at most; with none, the stand-in's (kinds.txt's
** factions, factions.txt's) are the game's.
**
** popgen reads them (pg_load: and the kinds' groups, from the denizens' data file, dq0d.dat), and gives each
** stratum's populations as the task asks, as its plan's grown (pg_stratum): each faction's number spread over its
** strata, the most on its lair's and fewer the further from it; on each, shared among its kinds (the first twice
** each other's share), and each kind's share in groups of its kind's size (its lairs), by the stratum's seed; its
** leader in its home, the first group of its first kind, on its lair's stratum.  Where each lair lies is the task's
** (by its kind's ways, as the stand-in's).  The same for the same theme and seeds, whenever it's asked.
*/

#ifndef _POP_H
#define _POP_H

#define THEME_FILE      "theme.txt"             /* The theme file, in the save folder */
#define POP_FACT        7                       /* Factions at most (the task's NFACT less the none) */
#define POP_NFACT       (POP_FACT + 1)
#define POP_KINDS       4                       /* A faction's kinds, at most */
#define POP_LAIRS       48                      /* A stratum's lairs, at most, as popgen gives them */

/* A lair as popgen gives it: its kind, its faction (1 on), how many, and what it is (PL_*) */
struct pglair {
    unsigned char   kind, fac, n, flags;
};
#define PL_HOME         0x01                    /* Its faction's home (the first group of its first kind, on its lair's
                                                **   stratum) */
#define PL_LEADER       0x02                    /* Its faction's leader, with its home (the lair before it) */

/* The task's arguments to popgen and its results, in the task's own RAM */
struct pgarg {
    const char*     lib;                        /* The game's lib folder (its dq0d.dat: the kinds) */
    const char*     theme;                      /* The theme file */
    unsigned char   sn;                         /* pg_stratum's: the stratum */
    unsigned        seed;                       /*   and its seed (for popgen's own die) */
    unsigned char*  att;                        /* pg_load's: each faction's attitude to the character (POP_NFACT:
                                                **   hub.h's AT_*, the first none's indifferent) */
    unsigned char*  rel;                        /*   how each stands with each (POP_NFACT x POP_NFACT) */
    unsigned char*  first;                      /*   each one's first kind (POP_NFACT: its tongue's) */
    struct pglair*  lair;                       /* pg_stratum's: its lairs (POP_LAIRS) */
};

/* popgen's entries (bank_call's: their values the factions read, the lairs given; 0 none) */
unsigned char __fastcall__ pg_load (struct pgarg* a);
unsigned char __fastcall__ pg_stratum (struct pgarg* a);

#endif
