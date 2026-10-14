/*
** look.c - Dingus Quest Zero's places told (the UI's): where the character is, as it sees it (look), and the map of
** what it's seen of the level (map).  Both read the level in its hub bank (level.h), then put the hub's header back
** before a word goes out.
**
** The words are the character's: only what it sees now (the world's view: the tiles marked T_VIS, in front of it
** and to its sides, lit, near enough for its eyes), each where it lies from it as it faces (one of eight: ahead,
** ahead to its right, to its right ... ahead to its left).
** A passage it can feel its way along in the dark: which ways it goes is told whatever the light.  A room's size
** it knows when it's seen the whole of it.
**
** The words are a stand-in theme's, till themes are data: a room's kind is one of eight names, a lit room's light
** torches.
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

static const char* const kindname[8] = {
    "chamber", "hall", "cell", "vault", "crypt", "gallery", "cave", "storeroom"
};
static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

static char lb[360];                            /* The words, made while the level's in the window */
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

/* Ways in a list, as the character sees them (m: bits, by REL8): "ahead", "ahead and to your left" ... */
static void __fastcall__ rels (unsigned char m)
{
    unsigned char d, n = 0, k = 0;

    for (d = 0; d < 8; ++d) {
        n += (m >> d) & 1;
    }
    for (d = 0; d < 8; ++d) {
        if (m & (1 << d)) {
            ++k;
            if (k > 1) {
                add (k == n ? " and " : ", ");
            }
            add (relname[d]);
        }
    }
}

/* Where (x, y) lies from the character, as it sees it (REL8's) */
static unsigned char __fastcall__ rel_to (unsigned char x, unsigned char y)
{
    return REL8 (bearing ((int) x - px, (int) y - py), face);
}

/* A room's doors that the character sees, counted by where they lie from it (REL8's) */
static unsigned char seen[8];
static void __fastcall__ doors_seen (struct room* p)
{
    unsigned char d, k;

    memset (seen, 0, sizeof seen);
    for (d = 0; d < 4; ++d) {
        for (k = 0; wall_tile (p, d, k); ++k) {
            if (TILE (L->tile[wall_y][wall_x]) == T_DOOR && VIS (wall_x, wall_y)) {
                ++seen[rel_to (wall_x, wall_y)];
            }
        }
    }
}

/* The doors seen, in words: "There is a door ahead." "There are two doors ahead and one to your left." */
static void doors_words (void)
{
    static const char* const count[5] = { "", "one", "two", "three", "four" };
    unsigned char d, n = 0, k = 0;

    for (d = 0; d < 8; ++d) {
        n += seen[d] != 0;
    }
    for (d = 0; d < 8; ++d) {
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
            r |= 1 << REL8 (H_OF (d), face);
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

/* ---- The map: the level's tiles seen, a character each (the character an @), and beside it (under it, if the
** screen's too narrow) the character's state: where it is (the level, its square: 5 feet across, from the level's
** northwest corner), the way it faces, its light, its senses as they are now (the summary's, whatever lowers them
** in them), and the ruleset's short lines (the referee's: the attributes, the penalties) */

#define PANEL       14                          /* The panel's width */
#define PLINES      16                          /* (Seven of the UI's, a gap, eight of the referee's) */
static char panel[PLINES][PANEL + 1];
static unsigned char np;

static void __fastcall__ pline (const char* s)
{
    if (np < PLINES) {
        strncpy (panel[np], s, PANEL);
        panel[np++][PANEL] = 0;
    }
}

/* A line of the panel: words, a number, words */
static void __fastcall__ num (const char* a, unsigned v, const char* z)
{
    static char b[PANEL + 1];
    char n[6];

    strcpy (b, a);
    strcat (b, utoa (v, n, 10));
    strcat (b, z);
    pline (b);
}

/* The panel's lines, from the hub's header (before the level's put in the window) and the referee's */
static void __fastcall__ make_panel (const char* rules)
{
    static char b[PANEL + 1];
    char n2[6];
    const char* e;
    unsigned char n;

    np = 0;
    num ("Level ", HUB->pc_level + 1, "");
    strcpy (b, "Square ");
    strcat (b, utoa (HUB->pc_x, n2, 10));
    strcat (b, ",");
    strcat (b, utoa (HUB->pc_y, n2, 10));
    pline (b);
    strcpy (b, "Facing ");
    strcat (b, head_abbr[HUB->pc_face & 7]);
    pline (b);
    if (HUB->pc_light) {
        num ("Light ", HUB->pc_light * 5, " ft");
    } else {
        pline ("No light");
    }
    num ("Sight ", HUB->pcsum.sight, "");
    num ("Hearing ", HUB->pcsum.hearing, "");
    num ("Smell ", HUB->pcsum.smell, "");
    pline ("");
    while (*rules) {                            /* (The referee's, a line each) */
        e = strchr (rules, '\n');
        n = e ? e - rules : strlen (rules);
        strncpy (b, rules, n > PANEL ? PANEL : n);
        b[n > PANEL ? PANEL : n] = 0;
        pline (b);
        rules += e ? n + 1 : n;
    }
}

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

/* Row y of the map into line (its length: the last tile seen, or the character's) */
static void __fastcall__ map_row (char* line, unsigned char y)
{
    unsigned char x, n = 0;

    if (y < LH) {
        for (x = 0; x < LW; ++x) {
            if ((line[x] = glyph (x, y)) != ' ') {
                n = x + 1;
            }
        }
        if (y == py) {
            line[px] = '@';
            if (n <= px) {
                n = px + 1;
            }
        }
    }
    line[n] = 0;
}

/* A row with a tile seen (or the character) */
static unsigned char __fastcall__ row_seen (unsigned char y)
{
    unsigned char x;

    if (y == py) {
        return 1;
    }
    for (x = 0; x < LW; ++x) {
        if (L->tile[y][x] & T_SEEN) {
            return 1;
        }
    }
    return 0;
}

void __fastcall__ map (const char* rules)
{
    static char line[LW + PANEL + 3];
    unsigned char x, y, n, side, k;

    make_panel (rules);
    blank ();
    sayf ("Level %u (you face %s):", HUB->pc_level + 1, head_name[HUB->pc_face & 7]);
    here ();                                    /* (The level in the window till the map's out: the words go to
                                                **   term.c's buffer, in the main RAM) */
    for (n = 0; n < LH - 1 && !row_seen (n); ++n) {
    }
    for (y = LH - 1; y > n && !row_seen (y); --y) {
    }
    side = tw > LW + PANEL + 1;            /* (Each line short of the last column: say_raw's) */
    for (k = 0; n + k <= y || (side && k < np); ++k) {  /* (The rows from the first seen to the last, the panel's
                                                **   beside them) */
        map_row (line, n + k <= y ? n + k : LH);
        if (side && k < np) {
            for (x = strlen (line); x < LW + 1; ++x) {
                line[x] = ' ';
            }
            strcpy (line + LW + 1, panel[k]);
        }
        say_raw (line);
    }
    hub_map (0);
    if (!side) {
        blank ();
        for (k = 0; k < np; ++k) {
            say_raw (panel[k]);
        }
    }
    blank ();
}
