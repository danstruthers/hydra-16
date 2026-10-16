/*
** level.c - Dingus Quest Zero's dungeon, the part every task shares (level.h): the directions' and headings' names,
** and the heading a way goes.  The UI's file; the world compiles it too (dq0w/level.c), and the other tasks (the
** names left out where LEVEL_NO_NAMES is defined: only the UI says them).
*/

#include "level.h"

const signed char dir_x[4] = { 0, 1, 0, -1 };
const signed char dir_y[4] = { -1, 0, 1, 0 };
#ifndef LEVEL_NO_NAMES
const char* const dir_name[6] = { "north", "east", "south", "west", "up", "down" };
#endif
const signed char head_x[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
const signed char head_y[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
#ifndef LEVEL_NO_NAMES
const char* const head_name[8] = {
    "north", "northeast", "east", "southeast", "south", "southwest", "west", "northwest"
};
const char* const head_abbr[8] = { "N", "NE", "E", "SE", "S", "SW", "W", "NW" };
#endif

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
