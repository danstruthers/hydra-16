/*
** level.c - Dingus Quest Zero's dungeon, the questions every task asks of a level (level.h): whose room a tile is,
** a room's doors, where a passage goes.  The UI's file; the world compiles it too (dq0w/level.c).
*/

#include "level.h"

const signed char dir_x[4] = { 0, 1, 0, -1 };
const signed char dir_y[4] = { -1, 0, 1, 0 };
const char* const dir_name[6] = { "north", "east", "south", "west", "up", "down" };

unsigned char door_x, door_y;

unsigned char __fastcall__ room_at (struct level* l, unsigned char x, unsigned char y)
{
    unsigned char r;
    struct room* p;

    for (r = 0; r < l->nrooms; ++r) {
        p = &l->room[r];
        if (x >= p->x && x < p->x + p->w && y >= p->y && y < p->y + p->h) {
            return r;
        }
    }
    return 0xFF;
}

/* The doors in room r's wall d: how many (the first, from the north or the west: door_x, door_y) */
unsigned char __fastcall__ doors_on (struct level* l, unsigned char r, unsigned char d)
{
    struct room* p = &l->room[r];
    unsigned char x, y, n = 0, k, len;

    len = d == DIR_N || d == DIR_S ? p->w : p->h;
    for (k = 0; k < len; ++k) {
        if (d == DIR_N || d == DIR_S) {
            x = p->x + k;
            y = d == DIR_N ? p->y - 1 : p->y + p->h;
        } else {
            y = p->y + k;
            x = d == DIR_W ? p->x - 1 : p->x + p->w;
        }
        if (TILE (l->tile[y][x]) == T_DOOR) {
            if (!n++) {
                door_x = x;
                door_y = y;
            }
        }
    }
    return n;
}

/* The directions a passage's tile (or a door's) goes on in: the tiles beside it that can be walked */
unsigned char __fastcall__ ways (struct level* l, unsigned char x, unsigned char y)
{
    unsigned char d, m = 0, nx, ny;

    for (d = 0; d < 4; ++d) {
        nx = x + dir_x[d];
        ny = y + dir_y[d];
        if (nx < LW && ny < LH && T_PASS (l->tile[ny][nx])) {
            m |= 1 << d;
        }
    }
    return m;
}
