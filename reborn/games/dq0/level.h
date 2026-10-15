/*
** level.h - Dingus Quest Zero's dungeon (DUNGEON.md), the part every task shares: the headings and directions and
** their names, what the character perceives (struct sight: the world's, which the UI words), and the map's window.
** The dungeon itself is the world's alone (dq0w/world.h: its strata, their plans, their chunks, in its own banks).
** A square is 5 feet.
*/

#ifndef _LEVEL_H
#define _LEVEL_H

/* Directions: the four of the walls and the passages, then up and down */
enum { DIR_N, DIR_E, DIR_S, DIR_W, DIR_UP, DIR_DOWN };
extern const signed char dir_x[4], dir_y[4];
extern const char* const dir_name[6];

/* Headings: the eight ways of the compass, the way the character faces or sets out (a direction d is heading
** H_OF (d)) */
enum { H_N, H_NE, H_E, H_SE, H_S, H_SW, H_W, H_NW };
#define H_OF(d)         ((d) << 1)
#define REL8(h, face)   (((h) - (face)) & 7)    /* A heading as the character sees it: 0 ahead, 1 ahead to its
                                                **   right, 2 its right, 3 behind it to its right, 4 behind it ... */
extern const signed char head_x[8], head_y[8];
extern const char* const head_name[8];          /* "north", "northeast" ... */
extern const char* const head_abbr[8];          /* "N", "NE" ... */
unsigned char __fastcall__ bearing (int dx, int dy);    /* The heading nearest the way (dx, dy) goes */

/* What the character perceives (the world's, after each look about; in the hub's header, struct hub's sight): the
** UI's words are made from it alone.  A way is REL8's (0xFF: where it stands); a height in feet, against the floor
** where the character stands */
#define SEE_DOORS       8
#define SEE_ZONES       4
#define SEE_OPEN        4
#define SEE_ITEMS       6
#define SEE_DENS        4
#define SEE_TRAPS       3
struct sight {
    unsigned char   seq;                        /* Bumped as it's made again */
    unsigned char   where;                      /* W_* */
    unsigned char   dark;                       /* Its own square unlit: it feels its way */
    unsigned char   kind;                       /* A room's kind (the theme's) */
    unsigned char   size;                       /* S_*: as much as it's seen of it */
    unsigned char   rw, rh;                     /* Its floor, in squares (the figures a close look gives) */
    unsigned char   lit;                        /* The room lit by its own light */
    unsigned char   first;                      /* The first time there */
    unsigned char   up_rel, up_ft;              /* The floor here: the way it rises, and feet in ten (0: level) */
    unsigned char   ceil, cmid, cwall;          /* Its zone's ceiling (ceil C_UNSEEN: not seen) */
    unsigned char   ndoors;                     /* Doors, and passages' mouths (rel | MOUTH) */
    struct { unsigned char rel; signed char dz; unsigned char state; unsigned x, y; unsigned char kind, parts; }
        door[SEE_DOORS];                        /* (state: PS_OPEN ...; x, y its square; a portal's kind and
                                                **   parts, world.h's PK_* and PP_*: a portcullis, a window, a slit) */
    unsigned char   nzones;                     /* The room's other zones seen */
    struct { unsigned char rel, how; signed char dz; } zone[SEE_ZONES];    /* how: Z_* */
    unsigned char   nopen;                      /* Stairways, ladders, shafts, windows seen */
    struct { unsigned char rel, kind; signed char dz; unsigned x, y; } open[SEE_OPEN];    /* kind: O_* */
    unsigned char   nways;                      /* A passage's ways on */
    struct { unsigned char rel; signed char dz; unsigned char steps; } way[4];
    unsigned char   nitems;                     /* The things it sees lying about (the things', named) */
    struct { char name[14]; unsigned char qty, plural, rel, hid; unsigned n, x, y; } item[SEE_ITEMS];
                                                /* (hid: 1 it hid it, 2 on the surface before it, 4 lit) */
    unsigned char   stratum, sname, style;      /* The stratum: its number, its name (the theme's), its style */
    int             z;                          /* The floor where it stands: feet above the way in */
    unsigned        x, y;                       /* Its square */
    unsigned char   ndens;                      /* The denizens it sees: each one's square, what it's doing (DO_*),
                                                **   its name (the hub's actor's, as it lies there) */
    struct { unsigned x, y; unsigned char doing; char name[12]; } den[SEE_DENS];
    unsigned char   ntraps;                     /* The traps it knows of and sees (world.h's TK_*, TT_*, TS_*) */
    struct { unsigned char rel, kind, trigger, state; } trap[SEE_TRAPS];
};
enum { DO_ROAM, DO_WATCH, DO_COMING, DO_SHY };  /* A denizen's doing: going about its own business, watching the
                                                **   character, coming towards it, keeping its distance */
#define MOUTH           0x08
#define DOORWAY         0x83                    /* (A seen door's state: no door in it, a doorway, open; else its
                                                **   portal's PS_OPEN and PS_WRECKED) */
enum { W_ROOM, W_PASSAGE };
enum { S_PART, S_SMALL, S_MID, S_LARGE, S_LONG };
enum { Z_STEPS, Z_SLOPE, Z_LEDGE };
enum { C_FLAT, C_BARREL, C_DOME, C_CATHEDRAL, C_SLOPED, C_ROUGH, C_UNSEEN };
enum { O_STAIRS_UP, O_STAIRS_DOWN, O_LADDER_UP, O_LADDER_DOWN, O_WINDOW, O_SHAFT_UP, O_SHAFT_DOWN, O_WAY_OUT };
enum { ST_BUILT, ST_NATURAL, ST_MINE, ST_MIXED };

/* The map's window: the stratum round the character, a square a character, drawn by the world into the hub
** (struct hub's map), each row's text and its end */
#define MAP_W           64
#define MAP_H           20

#endif
