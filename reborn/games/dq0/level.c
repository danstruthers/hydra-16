/*
** level.c - Dingus Quest Zero's dungeon, the questions every task asks of a level (level.h): whose room a tile is,
** a room's doors, where a passage goes.  The UI's file; the world compiles it too (dq0w/level.c).
*/

#include "level.h"

const signed char dir_x[4] = { 0, 1, 0, -1 };
const signed char dir_y[4] = { -1, 0, 1, 0 };
const char* const dir_name[6] = { "north", "east", "south", "west", "up", "down" };
const signed char head_x[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
const signed char head_y[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
const char* const head_name[8] = {
    "north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"
};
const char* const head_abbr[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };

unsigned char door_x, door_y, wall_x, wall_y;

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

/* The kth tile of room p's wall d (but its corners), from the north or the west: wall_x, wall_y; 0 past its end */
unsigned char __fastcall__ wall_tile (struct room* p, unsigned char d, unsigned char k)
{
    if (d == DIR_N || d == DIR_S) {
        if (k >= p->w) {
            return 0;
        }
        wall_x = p->x + k;
        wall_y = d == DIR_N ? p->y - 1 : p->y + p->h;
    } else {
        if (k >= p->h) {
            return 0;
        }
        wall_y = p->y + k;
        wall_x = d == DIR_W ? p->x - 1 : p->x + p->w;
    }
    return 1;
}

/* The doors in room r's wall d: how many (the first, from the north or the west: door_x, door_y) */
unsigned char __fastcall__ doors_on (struct level* l, unsigned char r, unsigned char d)
{
    unsigned char n = 0, k;

    for (k = 0; wall_tile (&l->room[r], d, k); ++k) {
        if (TILE (l->tile[wall_y][wall_x]) == T_DOOR && !n++) {
            door_x = wall_x;
            door_y = wall_y;
        }
    }
    return n;
}

/* The heading nearest the way (dx, dy) goes: straight within about 22 degrees (tan 22.5 is a little over 2/5),
** else the diagonal */
unsigned char __fastcall__ bearing (int dx, int dy)
{
    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;

    if (ax * 5 < ay * 2) {
        return dy < 0 ? H_N : H_S;
    }
    if (ay * 5 < ax * 2) {
        return dx > 0 ? H_E : H_W;
    }
    return dx > 0 ? (dy < 0 ? H_NE : H_SE) : (dy > 0 ? H_SW : H_NW);
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
