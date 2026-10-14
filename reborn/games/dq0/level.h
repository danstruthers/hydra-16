/*
** level.h - Dingus Quest Zero's dungeon: its levels, each a grid of tiles (rooms with walls and doors, the passages
** between them, the stairs), kept in the hub's banks (four to a bank, after its header's bank 0): the world makes
** them and changes them, the UI reads them to tell what the character sees, and to draw the map.  A tile is 5 feet.
**
** A place, as the game tells it, is a room (wherever in it the character stands), or a stretch of passage that
** ends where the passage forks, meets a door, or stops.  Going north from a room is going out by a door in its
** north wall and along the passage beyond, to the next place.
**
** The character sees the dungeon through its own eyes (the world's view.c): what's in front of it and to its
** sides, in a line it can see along, near enough for its sight, and lit, by the room or by the light it carries.
** What it sees now is T_VIS (the UI tells only that); what it's ever seen T_SEEN (the map).
*/

#ifndef _LEVEL_H
#define _LEVEL_H

#define LW              64                      /* A level's tiles: across, */
#define LH              20                      /*   and down (the map fits under the status line) */
#define NROOMS          12                      /* Rooms on a level, at most */
#define NLEVELS         8                       /* Levels in the dungeon */
#define LEVEL_SIZE      0x0800                  /* Each level's room in its bank */
#define LEVEL_BANK(n)   (1 + ((n) >> 2))        /* Its hub bank, */
#define LEVEL(n)        ((struct level*) (0x8000 + ((n) & 3) * LEVEL_SIZE))    /*   and where it is there */

/* Tiles (a tile's low six bits; T_SEEN: the character has seen it) */
enum { T_ROCK, T_FLOOR, T_WALL, T_DOOR, T_CORR, T_UP, T_DOWN };
#define T_SEEN          0x80
#define T_VIS           0x40                    /* Seen now (the world's view) */
#define T_MARK          0x40                    /* (Generation's, the same bit: a tile reached) */
#define TILE(t)         ((t) & 0x3F)
#define T_PASS(t)       (TILE (t) != T_ROCK && TILE (t) != T_WALL)  /* A tile that can be walked */
#define T_INSIDE(t)     (TILE (t) == T_FLOOR || TILE (t) == T_UP || TILE (t) == T_DOWN)    /* A room's */

/* A room: its floor, x to x + w - 1 and y to y + h - 1; its walls round it */
struct room {
    unsigned char   x, y, w, h;
    unsigned char   flags;                      /* R_* */
    unsigned char   kind;                       /* What the theme calls it (the UI's words) */
};
#define R_VISITED       0x01
#define R_LIT           0x02                    /* Lit, by a light of its own (torches, a glow: the theme's) */

struct level {
    unsigned char   made;                       /* Generated yet */
    unsigned char   depth;                      /* 0: the top */
    unsigned char   nrooms;
    unsigned char   upx, upy, downx, downy;     /* The stairs (no way down on the last level: downx 0) */
    struct room     room[NROOMS];
    unsigned char   tile[LH][LW];
};

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

/* level.c (each task its own copy: the world's and the UI's) */
unsigned char __fastcall__ room_at (struct level* l, unsigned char x, unsigned char y);  /* Its room, or 0xFF */
unsigned char __fastcall__ wall_tile (struct room* p, unsigned char d, unsigned char k);  /* A wall's kth tile
                                                **   (wall_x, wall_y): 1, or 0 past its end */
extern unsigned char wall_x, wall_y;
unsigned char __fastcall__ doors_on (struct level* l, unsigned char r, unsigned char d); /* Doors in a room's wall */
extern unsigned char door_x, door_y;            /* doors_on's first door's tile */
unsigned char __fastcall__ bearing (int dx, int dy);    /* The heading nearest the way (dx, dy) goes */
unsigned char __fastcall__ ways (struct level* l, unsigned char x, unsigned char y);    /* A passage's tile: the
                                                **   directions it goes on in (bits, 1 << DIR_N ...) */

#endif
