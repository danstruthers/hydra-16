/*
** look.c - Dingus Quest Zero's places told (the UI's): where the character is, as it sees it (look), and the map of
** what it's seen of the level (map).  Both read the level in its hub bank (level.h), then put the hub's header back
** before a word goes out.
**
** The words are the character's: only what it sees now (the world's view: the tiles marked T_VIS, in front of it
** and to its sides, lit, near enough for its eyes), each where it is from the way it faces (ahead, to its left ...).
** A passage it can feel its way along in the dark: which ways it goes is told whatever the light.  A room's size
** it knows when it's seen the whole of it.
**
** The words are a stand-in theme's, till themes are data: a room's kind is one of eight names, a lit room's light
** torches.
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

static const char* const kindname[8] = {
    "chamber", "hall", "cell", "vault", "crypt", "gallery", "cave", "storeroom"
};
static const char* const relname[4] = { "ahead", "to your right", "behind you", "to your left" };

static char lb[400];                            /* The words, made while the level's in the window */
static struct level* L;
static unsigned char lv, px, py, face, light;

static void here (void)
{
    lv = HUB->pc_level;
    px = HUB->pc_x;
    py = HUB->pc_y;
    face = HUB->pc_face;
    hub_map (LEVEL_BANK (lv));
    L = LEVEL (lv);
}

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

#define VIS(x, y)       (L->tile[y][x] & T_VIS)

/* Ways in a list, as the character sees them (m: bits, by REL): "ahead", "ahead and to your left" ... */
static void __fastcall__ rels (unsigned char m)
{
    unsigned char d, n = 0, k = 0;

    for (d = 0; d < 4; ++d) {
        n += (m >> d) & 1;
    }
    for (d = 0; d < 4; ++d) {
        if (m & (1 << d)) {
            ++k;
            if (k > 1) {
                add (k == n ? " and " : ", ");
            }
            add (relname[d]);
        }
    }
}

/* The way to (x, y) from the character, as it sees it (REL's) */
static unsigned char __fastcall__ rel_to (unsigned char x, unsigned char y)
{
    int dx = (int) x - px, dy = (int) y - py, ahead, right;

    ahead = dx * dir_x[face] + dy * dir_y[face];
    right = dx * dir_x[(face + 1) & 3] + dy * dir_y[(face + 1) & 3];
    if ((ahead >= 0 ? ahead : -ahead) >= (right >= 0 ? right : -right)) {
        return ahead >= 0 ? 0 : 2;
    }
    return right > 0 ? 1 : 3;
}

/* A room's doors that the character sees, counted by the way their walls are from it (REL's: a door in the wall to
** its left is the way left goes) */
static unsigned char seen[4];
static void __fastcall__ doors_seen (struct room* p)
{
    unsigned char d, k, len, x, y;

    memset (seen, 0, sizeof seen);
    for (d = 0; d < 4; ++d) {
        len = d == DIR_N || d == DIR_S ? p->w : p->h;
        for (k = 0; k < len; ++k) {
            if (d == DIR_N || d == DIR_S) {
                x = p->x + k;
                y = d == DIR_N ? p->y - 1 : p->y + p->h;
            } else {
                y = p->y + k;
                x = d == DIR_W ? p->x - 1 : p->x + p->w;
            }
            if (TILE (L->tile[y][x]) == T_DOOR && VIS (x, y)) {
                ++seen[REL (d, face)];
            }
        }
    }
}

/* The doors seen, in words: "There is a door ahead." "There are two doors ahead and one to your left." */
static void doors_words (void)
{
    static const char* const count[5] = { "", "one", "two", "three", "four" };
    unsigned char d, n = 0, k = 0;

    for (d = 0; d < 4; ++d) {
        n += seen[d] != 0;
    }
    for (d = 0; d < 4; ++d) {
        if (!seen[d]) {
            continue;
        }
        if (!k++) {
            add (seen[d] == 1 ? " There is a door " : " There are ");
            if (seen[d] > 1) {
                add (count[seen[d] > 4 ? 4 : seen[d]]);
                add (" doors ");
            }
        } else {
            add (k == n ? " and " : ", ");
            add (count[seen[d] > 4 ? 4 : seen[d]]);
            add (" ");
        }
        add (relname[d]);
    }
    if (n) {
        add (".");
    }
}

/* The whole of a room's floor seen (now, or before) */
static unsigned char __fastcall__ all_seen (struct room* p)
{
    unsigned char i, j;

    for (j = p->y; j < p->y + p->h; ++j) {
        for (i = p->x; i < p->x + p->w; ++i) {
            if (!(L->tile[j][i] & T_SEEN)) {
                return 0;
            }
        }
    }
    return 1;
}

/* Stairs seen: "You stand by stairs leading down." "You see the stairs up to the way out ahead." */
static void __fastcall__ stairs (unsigned char x, unsigned char y, const char* name)
{
    if (!x || !VIS (x, y)) {
        return;
    }
    if (x == px && y == py) {
        add (" You stand by ");
        add (name);
    } else {
        add (" You see ");
        add (name);
        add (" ");
        add (relname[rel_to (x, y)]);
    }
    add (".");
}

static void __fastcall__ room_words (unsigned char r)
{
    struct room* p = &L->room[r];
    unsigned char area = p->w * p->h, whole = all_seen (p);

    if (!VIS (px, py)) {
        add ("It is dark. You can see nothing, but you feel the open floor of a room about you.");
        return;
    }
    add (whole ? "You are in a" : "You can see part of a");
    add (!whole ? " " : area <= 8 ? " small " : area >= 30 ? " large " : p->w >= 3 * p->h ? " long " : " ");
    add (kindname[p->kind & 7]);
    add (p->flags & R_LIT ? ", lit by torches in iron brackets." : light ? ", by the light of your torch." : ".");
    doors_seen (p);
    doors_words ();
    stairs (L->upx, L->upy, lv ? "stairs leading up" : "the stairs up to the way out");
    stairs (L->downx, L->downy, "stairs leading down");
}

static void passage_words (void)
{
    unsigned char m = ways (L, px, py), r = 0, d, n;

    for (d = 0; d < 4; ++d) {
        if (m & (1 << d)) {
            r |= 1 << REL (d, face);
        }
    }
    n = (m & 1) + (m >> 1 & 1) + (m >> 2 & 1) + (m >> 3 & 1);
    add (VIS (px, py) ? "" : "It is dark; you feel your way. ");
    if (n <= 1) {
        add ("The passage ends here; it goes back ");
    } else if (n == 2) {
        add ("You are in a passage. It goes ");
    } else {
        add ("The passage forks here. It goes ");
    }
    rels (r);
    add (".");
}

/* Where the character is, as it sees it (first: the first time there) */
void __fastcall__ look (unsigned char first)
{
    unsigned char r;

    (void) first;                               /* (The theme's words for a room's first sight: later) */
    lb[0] = 0;
    light = HUB->pc_light;
    here ();
    if ((r = room_at (L, px, py)) != 0xFF) {
        room_words (r);
    } else {
        passage_words ();
    }
    hub_map (0);
    msg (lb);
}

/* ---- The map: the level's tiles seen, a character each (the character an @) */

static char rows[LH][LW + 1];

static char __fastcall__ glyph (unsigned char x, unsigned char y)
{
    unsigned char t = L->tile[y][x];

    if (!(t & T_SEEN)) {
        return ' ';
    }
    switch (TILE (t)) {
    case T_FLOOR:
        return '.';
    case T_DOOR:
        return '+';
    case T_CORR:
        return '#';
    case T_UP:
        return '<';
    case T_DOWN:
        return '>';
    case T_WALL:
        if ((y && T_INSIDE (L->tile[y - 1][x])) || (y + 1 < LH && T_INSIDE (L->tile[y + 1][x]))) {
            return '-';
        }
        if ((x && T_INSIDE (L->tile[y][x - 1])) || (x + 1 < LW && T_INSIDE (L->tile[y][x + 1]))) {
            return '|';
        }
        return '-';
    }
    return ' ';
}

void map (void)
{
    unsigned char x, y, n;

    here ();
    for (y = 0; y < LH; ++y) {
        for (x = 0, n = 0; x < LW; ++x) {
            if ((rows[y][x] = glyph (x, y)) != ' ') {
                n = x + 1;
            }
        }
        if (y == py) {
            rows[y][px] = '@';
            if (n <= px) {
                n = px + 1;
            }
        }
        rows[y][n] = 0;
    }
    hub_map (0);
    blank ();
    sayf ("Level %u (you face %s):", lv + 1, dir_name[face]);
    for (n = 0; n < LH - 1 && !rows[n][0]; ++n) {
    }
    for (y = LH - 1; y > n && !rows[y][0]; --y) {
    }
    for (; n <= y; ++n) {                       /* (The rows from the first seen to the last) */
        say_raw (rows[n]);
    }
    blank ();
}
