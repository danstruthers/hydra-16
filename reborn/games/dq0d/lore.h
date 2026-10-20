/*
** lore.h - Dingus Quest Zero's world, as data (Phase 3D: the history's): its peoples and their ways of building
** (games/dq0d/peoples.txt, made lib/dq0/dq0f.dat) and its deities (deities.txt, made lib/dq0/dq0g.dat), each file a
** record a people (or a deity), the first none (its name's first byte the records' count), then their words, each
** 0-ended, an offset from the file's start (0: none).  make.js lays them out the same.  And the kinds' habitats, the
** bits of the denizens' kinds' records (kinds.txt's, in dq0d.dat: struct dkind's form, land, climate and arms).
**
** Their readers: the history's generator (what a region's eras were, who built what, to whom), the UI (each deity's
** name a word of the commands'), and the clues and the stocking (a people's walls, doors and marks; a deity's sign,
** its temples).
*/

#ifndef _LORE_H
#define _LORE_H

/* The kinds' habitats (struct dkind's): */
#define KF_SIZE(f)      ((f) & 7)               /* form: its size (1 tiny ... 4 a man ... 6 very large), */
#define KF_NATURE(f)    ((f) >> 3 & 7)          /*   its nature (NA_*), */
#define KF_LIGHT(f)     ((f) >> 6)              /*   the light it keeps to (LI_*) */
enum { NA_ANIMAL, NA_VERMIN, NA_MONSTER, NA_FOLK, NA_DEAD };
enum { LI_ANY, LI_DARK, LI_LIGHT };
#define LA_MOUNTAINS    0x01                    /* land: the lands of the regions it's found under (a bit each) */
#define LA_HILLS        0x02
#define LA_FOREST       0x04
#define LA_MARSH        0x08
#define LA_COAST        0x10
#define LA_OPEN         0x20
#define CL_COLD         0x01                    /* climate: a bit each */
#define CL_TEMPERATE    0x02
#define CL_WARM         0x04
#define AR_BITE         0x01                    /* arms: how it fights (a bit each) */
#define AR_CLAWS        0x02
#define AR_STING        0x04
#define AR_CRUSH        0x08
#define AR_HORNS        0x10
#define AR_TALONS       0x20
#define AR_TOUCH        0x40
#define AR_WEAPON       0x80

/* A people (dq0f.dat's: 64 bytes) */
#define NPEOPLE_KINDS   4
struct people {
    char            name[24];                   /* "the Ardish" (the first record's name[0]: the count) */
    char            key[8];                     /* "ardish" */
    unsigned char   kinds[NPEOPLE_KINDS];       /* Its folk among the bestiary's kinds (dq0d.dat's numbers; 0 none) */
    char            tongue[8];
    unsigned char   builds;                     /* What its works are for: a bit each, 1 << P_* (theme.h's) */
    unsigned char   styles;                     /* The plans it builds to: a bit each, 1 << ST_* (level.h's) */
    unsigned        deities;                    /* The deities it worships: a bit each, by dq0g.dat's number */
    unsigned char   lands, climate;             /* LA_*, CL_* */
    unsigned        walls, doors, light, marks; /* Its words (offsets) */
    unsigned        rooms;                      /* Its eight rooms' words, 0 after each, one after another */
    unsigned        uses;                       /* Its twelve uses' words, the same */
    unsigned char   spare[2];
};

/* A deity (dq0g.dat's: 64 bytes) */
enum { TE_GOOD, TE_NEUTRAL, TE_EVIL };          /* Its temper: good to evil, */
enum { TE_ORDER, TE_BALANCE, TE_CHAOS };        /*   and order to chaos */
enum { BU_NONE, BU_CRYPT, BU_BARROW, BU_PYRE, BU_EARTH };   /* How its faithful lie */
struct deity {
    char            name[12];                   /* "Orrun": one word, called on (the first record's name[0]: the
                                                **   count) */
    unsigned long   domains;                    /* What it's god of: a bit each (DO_*, deities.txt's order) */
    unsigned char   good, order;                /* Its temper (TE_*) */
    unsigned char   number;                     /* Its sacred number */
    unsigned char   day;                        /* Its monthly rite's day (1-30; 0 none) */
    unsigned char   burial;                     /* BU_* */
    unsigned char   spare1;
    unsigned        enemies;                    /* The deities its faith is at war with: a bit each, by number */
    unsigned long   servants;                   /* The bestiary's kinds that serve it: a bit each, dq0d.dat's
                                                **   numbers (1 the first: the themes' denizens' bits) */
    unsigned        title, followers, rite, feast, colours, sign, temple;  /* Its words (offsets) */
    unsigned char   spare[22];
};
enum { DO_WAR, DO_FIRE, DO_CRUELTY, DO_DEATH, DO_UNDEAD, DO_CHAOS, DO_WEALTH, DO_PLEASURE, DO_BARGAINS, DO_BEASTS,
       DO_CHANGE, DO_SERPENTS, DO_HONOUR, DO_BATTLE, DO_PROTECTION, DO_SHADOW, DO_DECEIT, DO_NIGHTMARES, DO_THIEVES,
       DO_HEALING, DO_HARVEST, DO_MERCY, DO_COURAGE, DO_STORMS, DO_COLD, DO_KNOWLEDGE, DO_PUZZLES, DO_LAW, DO_DREAMS,
       DO_MAGIC, DO_STARS, DO_STONE };

typedef char lore_fits[sizeof (struct people) == 64 && sizeof (struct deity) == 64 ? 1 : -1];

#endif
