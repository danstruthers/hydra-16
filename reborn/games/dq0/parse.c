/*
** parse.c - (overlay 3) the player's commands understood: a line typed made a command (dq0.h's struct pcmd) of a
** verb, a direction, a thing (and the direction it lies), and how it's meant (up to it, or through it), after the
** user's grammar (C:\source\dq0's Language.txt): each verb its synonyms (move, mv, walk, go, trundle, mosey, saunter,
** run, crawl; look, peer, gaze, gander, take a gander; examine, inspect, look closely ...); directions by the compass
** (n, north, ne, northeast ...), as the character faces (f, fwd, forward, ahead, front; b, back, behind; l, left;
** r, right), up and down (over my head, above; under, underneath, below), and two together ("north and east",
** "front right", "behind to my left": the way between them); the little words (the, a, to the, off, of me, my ...)
** passed over.  A relative direction is made the compass's by the way the character faces (pc_face, the UI's copy of
** the hub's, for an overlay doesn't see the hub).  What's done with the command is main.c's.
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

#include <string.h>
#include "dq0.h"
#include "level.h"

/* The verbs: a word (or words) and its verb; several words first, so "look closely" is examining */
static const struct { char w[16]; unsigned char v; } verbs[] = {
    { "take a gander", V_LOOK }, { "look closely", V_EXAMINE }, { "look close", V_EXAMINE },
    { "look in detail", V_EXAMINE }, { "close look", V_EXAMINE }, { "pick up", V_GET }, { "put out", V_DOUSE },
    { "move", V_MOVE }, { "mv", V_MOVE }, { "walk", V_MOVE }, { "go", V_MOVE }, { "trundle", V_MOVE },
    { "mosey", V_MOVE }, { "saunter", V_MOVE }, { "run", V_MOVE }, { "crawl", V_MOVE }, { "head", V_MOVE },
    { "enter", V_ENTER }, { "approach", V_APPROACH },
    { "look", V_LOOK }, { "l", V_LOOK }, { "peer", V_LOOK }, { "gaze", V_LOOK }, { "gander", V_LOOK },
    { "examine", V_EXAMINE }, { "inspect", V_EXAMINE }, { "x", V_EXAMINE }, { "search", V_SEARCH },
    { "turn", V_TURN }, { "face", V_FACE }, { "climb", V_CLIMB }, { "jump", V_JUMP },
    { "open", V_OPEN }, { "close", V_CLOSE }, { "shut", V_CLOSE }, { "unlock", V_UNLOCK }, { "pick", V_UNLOCK },
    { "force", V_FORCE }, { "dig", V_DIG }, { "mine", V_DIG },
    { "get", V_GET }, { "take", V_GET }, { "grab", V_GET }, { "acquire", V_GET }, { "drop", V_DROP },
    { "light", V_LIGHT }, { "douse", V_DOUSE }, { "extinguish", V_DOUSE },
    { "wait", V_WAIT }, { "z", V_WAIT }, { "map", V_MAP }, { "m", V_MAP }, { "sheet", V_SHEET },
    { "skills", V_SHEET }, { "help", V_HELP }, { "?", V_HELP }, { "pause", V_PAUSE }, { "quit", V_QUIT },
    { "q", V_QUIT },
};

/* The directions: by the compass (0-7), as it faces (REL | 0-7), up, down */
#define REL     0x10
static const struct { char w[12]; unsigned char d; } dirs[] = {
    { "n", H_N }, { "north", H_N }, { "ne", H_NE }, { "northeast", H_NE }, { "e", H_E }, { "east", H_E },
    { "se", H_SE }, { "southeast", H_SE }, { "s", H_S }, { "south", H_S }, { "sw", H_SW }, { "southwest", H_SW },
    { "w", H_W }, { "west", H_W }, { "nw", H_NW }, { "northwest", H_NW },
    { "f", REL | 0 }, { "fwd", REL | 0 }, { "forward", REL | 0 }, { "forwards", REL | 0 }, { "ahead", REL | 0 },
    { "front", REL | 0 }, { "fr", REL | 1 }, { "r", REL | 2 }, { "right", REL | 2 }, { "br", REL | 3 },
    { "b", REL | 4 }, { "back", REL | 4 }, { "backward", REL | 4 }, { "backwards", REL | 4 }, { "behind", REL | 4 },
    { "bl", REL | 5 }, { "l", REL | 6 }, { "left", REL | 6 }, { "fl", REL | 7 },
    { "u", GO_UP }, { "up", GO_UP }, { "above", GO_UP }, { "overhead", GO_UP }, { "upward", GO_UP },
    { "upwards", GO_UP }, { "d", GO_DOWN }, { "down", GO_DOWN }, { "below", GO_DOWN }, { "under", GO_DOWN },
    { "underneath", GO_DOWN }, { "beneath", GO_DOWN }, { "downward", GO_DOWN }, { "downwards", GO_DOWN },
};

/* The things it can see and name */
static const struct { char w[12]; unsigned char o; } things[] = {
    { "door", OB_DOOR }, { "doors", OB_DOOR }, { "doorway", OB_DOOR }, { "gate", OB_DOOR }, { "portal", OB_DOOR },
    { "passage", OB_PASSAGE }, { "tunnel", OB_PASSAGE }, { "corridor", OB_PASSAGE }, { "opening", OB_PASSAGE },
    { "hallway", OB_PASSAGE }, { "stairs", OB_STAIRS }, { "stair", OB_STAIRS }, { "stairway", OB_STAIRS },
    { "staircase", OB_STAIRS }, { "steps", OB_STAIRS }, { "ladder", OB_LADDER }, { "shaft", OB_SHAFT },
    { "hole", OB_SHAFT }, { "ledge", OB_LEDGE }, { "exit", OB_WAY_OUT }, { "wall", OB_WALL },
};

/* The little words, passed over (to, toward, at: up to it; through, into, out: through it) */
static const char little[][10] = {
    "the", "a", "an", "this", "that", "these", "those", "of", "me", "my", "myself", "self", "off", "side",
    "direction", "dir", "and", "please", "on", "over", "head", "then", "now", "way", "here", "there", "it",
};

static unsigned char nw;                        /* The line's words */
static char* word[12];

static void __fastcall__ split (char* l)
{
    nw = 0;
    while (*l && nw < 12) {
        while (*l == ' ' || *l == ',' || *l == '.' || *l == '!') {
            *l++ = 0;
        }
        if (!*l) {
            break;
        }
        word[nw++] = l;
        while (*l && *l != ' ' && *l != ',' && *l != '.' && *l != '!') {
            if (*l >= 'A' && *l <= 'Z') {
                *l += 'a' - 'A';
            }
            ++l;
        }
    }
}

/* Words i on the same as phrase p (its words, spaces between): how many, or 0 */
static unsigned char __fastcall__ phrase (unsigned char i, const char* p)
{
    unsigned char n = 0, k;

    while (*p) {
        if (i + n >= nw) {
            return 0;
        }
        k = strlen (word[i + n]);
        if (strncmp (word[i + n], p, k) || (p[k] && p[k] != ' ')) {
            return 0;
        }
        p += k;
        ++n;
        if (*p == ' ') {
            ++p;
        }
    }
    return n;
}

/* Two directions together: the way between them (a quarter apart: the eighth between), or the second */
static unsigned char __fastcall__ between (unsigned char a, unsigned char b)
{
    if (((b - a) & 7) == 2) {
        return (a + 1) & 7;
    }
    if (((a - b) & 7) == 2) {
        return (b + 1) & 7;
    }
    return b;
}

void __fastcall__ o_parse (char* l, struct pcmd* c)
{
    unsigned char i = 0, k, n, d, dir = 0xFF;

    memset (c, 0, sizeof *c);
    c->dir = c->orel = 0xFF;
    split (l);
    if (!nw) {
        return;
    }
    for (k = 0; k < sizeof verbs / sizeof verbs[0]; ++k) {      /* (The verb, if the line starts with one) */
        if ((n = phrase (0, verbs[k].w)) != 0) {
            c->verb = verbs[k].v;
            i = n;
            break;
        }
    }
    if (!c->verb) {
        c->verb = V_MOVE;                       /* (A way, or a thing, alone: going there) */
    }
    for (; i < nw; ++i) {
        if (!strcmp (word[i], "to") || !strcmp (word[i], "toward") || !strcmp (word[i], "towards") ||
            !strcmp (word[i], "at") || !strcmp (word[i], "near") || !strcmp (word[i], "by")) {
            c->how = HOW_TO;
            continue;
        }
        if (!strcmp (word[i], "through") || !strcmp (word[i], "into") || !strcmp (word[i], "in") ||
            !strcmp (word[i], "out") || !strcmp (word[i], "thru")) {
            c->how = HOW_THROUGH;
            continue;
        }
        if (!strcmp (word[i], "half")) {
            c->half = 1;
            continue;
        }
        if (!strcmp (word[i], "around") || !strcmp (word[i], "about")) {
            c->around = 1;
            continue;
        }
        for (k = 0; k < sizeof little / sizeof little[0] && strcmp (word[i], little[k]); ++k) {
        }
        if (k < sizeof little / sizeof little[0]) {
            continue;
        }
        for (k = 0; k < sizeof dirs / sizeof dirs[0] && strcmp (word[i], dirs[k].w); ++k) {
        }
        if (k < sizeof dirs / sizeof dirs[0]) {
            d = dirs[k].d;
            d = d & REL ? (pc_face + (d & 7)) & 7 : d;  /* (As it faces: made the compass's) */
            dir = dir == 0xFF || d >= GO_UP || dir >= GO_UP ? d : between (dir, d);
            continue;
        }
        for (k = 0; k < sizeof things / sizeof things[0] && strcmp (word[i], things[k].w); ++k) {
        }
        if (k < sizeof things / sizeof things[0]) {
            c->obj = things[k].o;
            continue;
        }
        if (!c->word[0]) {                      /* (The first word not understood) */
            strncpy (c->word, word[i], sizeof c->word - 1);
        }
    }
    if (c->obj) {                               /* (With a thing, its direction is where it lies) */
        c->orel = dir;
    } else {
        c->dir = dir;
    }
    if (c->verb == V_TURN && c->half && dir < GO_UP) {  /* (Half left, half right: an eighth) */
        c->dir = ((dir - pc_face) & 7) == 6 ? (pc_face + 7) & 7 : ((dir - pc_face) & 7) == 2 ? (pc_face + 1) & 7 : dir;
    }
    if ((c->verb == V_TURN || c->verb == V_FACE) && c->around && dir == 0xFF) {
        c->dir = (pc_face + 4) & 7;
    }
}
