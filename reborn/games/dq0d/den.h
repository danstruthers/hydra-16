/*
** den.h - (dq0d) the minds' records: the lairs and the denizens, in a bank of the minds' own (dq0d.c's pb), and their
** head.  A header of its own (scene1, 2026-11-24) so the build lays them out (games/dq0/hubgen.js's HEADERS: their
** offsets in obj/gen/dq0hub.json), and a test's scene names a record's fields by the build's offsets, not by hand
** (tests/lib/dq0.js's dq0Rec)
*/

#ifndef _DEN_H
#define _DEN_H

#define NFACT           8                                    /* (The factions at most: dq0d.c's struct faction too) */

/* ---- The records: the lairs and the denizens, in a bank of their own (a cache: its records' places free, kind 0,
** or a region's that's in it) */

#define NLAIRS          64
#define NDENS           224                     /* (240 till their want, 2026-10-31: two bytes each more) */
struct lair {
    unsigned char   sn, kind, n;                /* Its stratum, its kind, its denizens */
    unsigned char   flags;                      /* L_* */
    unsigned char   link;                       /* The stratum's link it's by, or 0xFF (a room of its chunk) */
    unsigned        id;                         /* Its number (1 on: the order the lairs were decided in) */
    unsigned        cx, cy;                     /* Its chunk (from the stratum's x0, y0) */
    unsigned        hx, hy;                     /* Its home square (placed) */
    unsigned char   rx, ry;                     /* Its region (its chunk's first square's, the frame's) */
};
#define L_PLACED        0x01                    /* Its denizens placed */
#define L_LEADER        0x02                    /* Its faction's leader with them (a theme's home: popgen's, 2026-10-29), its
                                                **   faction the high bits (L_FAC) */
#define L_FAC(f)        (((f) >> 4) & 7)
struct den {
    unsigned char   kind;
    unsigned char   flags;                      /* D_* */
    unsigned char   sn, face;
    unsigned        id, lid;                    /* Its number (1 on), its lair's */
    unsigned        x, y;                       /* Its square (placed) */
    unsigned long   ready;                      /* When it can act again */
    unsigned char   attitude;                   /* To the character (hub.h's AT_*) */
    unsigned char   order;                      /* An ally's request (hub.h's OR_*) */
    unsigned char   lsn, lrx, lry;              /* Its lair's stratum, and its region (its own, not placed yet: the
                                                **   lair's rx, ry) */
    unsigned        hx, hy;                     /* Its lair's home square (placed) */
    unsigned char   fac;                        /* Its faction (its kind's, the stand-in's; or the theme's) */
    unsigned char   serve;                      /* The weeks it had served its leaders as the game began (a leader's,
                                                **   it had led: its band's age), */
    unsigned char   contact;                    /*   and how much it's with them, 0-255 (part morale's: its bonds' growth) */
    unsigned char   ate, drank;                 /* The clock (NOW_U's) it last ate, and drank (its want: 2026-10-31) */
};
#define D_OUT           0x01                    /* On a square of its own */
#define D_ACTOR         0x02                    /* An actor now (the hub's entry the truth while it is) */
#define D_DEAD          0x04                    /* Dead (its body left where it fell: an item) */
#define D_MET           0x08                    /* Met (made an actor once: its personal things made, I_KIT's KT_PURSE,
                                                **   the outfits' phase 3, 2026-10-27) */
#define D_LOST          0x10                    /* It knows a leader of its is lost (dead): no bond to a dead one after
                                                **   (part morale's; one it doesn't know of holds), and of the one after it */
#define D_RANK(f)       ((f) >> 5 & 3)          /* A successor's rank, its post's (RK_*: its kind's the less; part morale's
                                                **   succession, 2026-10-30), */
#define D_PROM          0x80                    /*   a successor: its serve the week it took its post (its tenure) */
#define NNAMES          8
#define NDREG           32
#define DN_MAGIC        0x4E44                  /* "DN": dens.dat's */
struct head {
    unsigned        magic;
    unsigned char   nlairs, ndens;              /* The records' places used, up to the last in use */
    unsigned char   fatt[NFACT];                /* Each faction's attitude to the character now (its newcomers') */
    unsigned char   deeds[NFACT];               /* Each faction's regard: the points of the deeds it knows of, towards
                                                **   its next step (slice 4.8's reputation: regard) */
    struct { unsigned id; char name[12]; } names[NNAMES];    /* The names the character's given (a denizen's
                                                **   number; 0: none) */
    unsigned        ids, lids;                  /* The denizens' and the lairs' numbers given */
    unsigned char   done[32];                   /* Each stratum's populations decided (a bit each: DONE) */
    struct {
        unsigned char sn, rx, ry;
        unsigned    stamp;
    }               reg[NDREG];                 /* The regions whose records are in the cache (sn its stratum plus 1:
                                                **   0 the entry free) */
    unsigned        rstamp;
};
struct pop {
    struct head     h;
    struct lair     lair[NLAIRS];
    struct den      den[NDENS];
    unsigned char   nowork[32];                 /* (A lair's number's, its low bits: the clock's high byte plus 1 when
                                                **   its diggers last found no dig to do, part day's target: not asked
                                                **   again till it moves, near enough two hours: part tunnel_work's a dearer
                                                **   swap than a record's move) */
};
#define POP             ((struct pop*) 0x8000)
typedef char pop_fits[sizeof (struct pop) <= 0x2000 ? 1 : -1];

#endif
