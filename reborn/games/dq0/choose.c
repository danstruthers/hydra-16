/*
** choose.c - (overlay 7) the thing the player means, of those the character perceives (main.c's find: the parser's
** thing, and the way it lies if one was said: "the door on my right", "the north door"): the one there is; of
** several, the one directly ahead (the nearest that way); else (the user's, 2026-10-16: "If I say open door and there
** are three closed doors in the room, it should give me a numbered list of choices if one is not directly in front
** of me") they're listed, numbered, where they lie as it faces ("Which door: 1. the door ahead to your left (closed),
** 2. the one to your right (closed), or 3. the double door behind you to your right (open)?"), and the command kept:
** the next line chooses one by its number, or by the way it lies ("left"), and the command's carried out on it
** (choose; any other line, the choice let go).  From the hub's record of what the character perceives (copied into
** look.c's sight_copy: the hub isn't in the window while an overlay is), and its facing as the command was made
** (pc_face).  Its own state is kept in its overlay's bank (its data there: the code's only).
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")
#pragma data-name ("OV7RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#define NPICK       8

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

static const char* const thing[] = { "", "door", "way on", "stairs", "ladder", "shaft", "ledge", "way out", "wall",
    "window", "trap", "portcullis", "room" };

static struct cmd* cur = 0;                     /* The command being carried out (main.c's: choose's) */
static struct cmd kept = { 0 };                 /* The command a choice was asked for, */
static unsigned char asked = 0;                 /*   its choices (0: none asked), */
static unsigned px[NPICK] = { 0 }, py[NPICK] = { 0 };   /*   each one's square, */
static unsigned char ph[NPICK] = { 0 };         /*   the heading it lies, */
static unsigned char pi[NPICK] = { 0 };         /*   and its record (a door's, or ndoors on: an opening's) */
static unsigned cx = 0, cy = 0;                 /* The one chosen, for the command again (cx 0: none; till the
                                                **   next) */

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

/* Squares from the character to square (x, y) (the more of the two ways) */
static unsigned char __fastcall__ apart (unsigned x, unsigned y)
{
    x -= sight_copy.x;
    y -= sight_copy.y;
    x = (int) x < 0 ? -x : x;
    y = (int) y < 0 ? -y : y;
    x = x > y ? x : y;
    return x > 255 ? 255 : x;
}

static const char* first;                      /* (The first choice's noun) */

/* How far choice k lies to the character's right, across the way it faces (less, its left); or (ahead) how far
** ahead of it */
static int __fastcall__ side (unsigned char k, unsigned char ahead)
{
    int dx = px[k] - sight_copy.x, dy = py[k] - sight_copy.y;

    return ahead ? head_x[pc_face] * dx + head_y[pc_face] * dy : head_x[pc_face] * dy - head_y[pc_face] * dx;
}

/* Choice k in words, added to lb: "the door ahead to your left (closed)" (and how far, if another lies the same way) */
static void __fastcall__ named (unsigned char k, unsigned char n)
{
    static const char* const open[8] = {
        "stairway up", "stairway down", "ladder up", "ladder down", "window", "shaft overhead", "shaft in the floor",
        "stairs up to the way out"
    };
    struct sight* s = &sight_copy;
    unsigned char i = pi[k], r, j, st;
    const char* w;
    char b[6];

    add (k ? (k + 1 == n ? ", or " : ", ") : " ");
    b[0] = '1' + k;
    b[1] = 0;
    add (b);
    add (". the ");
    if (i < s->ndoors) {
        r = s->door[i].rel;
        st = s->door[i].state;
        w = r & MOUTH ? "passage" : s->door[i].kind == PK_PORTCULLIS ? "portcullis" : s->door[i].kind == PK_WINDOW ?
            "window" : s->door[i].kind == PK_SLIT ? "arrow slit" : s->door[i].parts & SD_DOUBLE ? "double door" : "door";
    } else {
        r = s->open[i - s->ndoors].rel;
        w = open[s->open[i - s->ndoors].kind & 7];
    }
    add (k && w == first ? "one" : w);           /* ("the door ..., the one ...") */
    first = k ? first : w;
    add (" ");
    add (r == 0xFF ? "where you stand" : relname[r & 7]);
    for (j = 0; j < n; ++j) {                   /* (Another the same way: how far each is; as far, which side) */
        if (j != k && ph[j] == ph[k]) {
            if (apart (px[j], py[j]) == apart (px[k], py[k])) {
                add ((r & 3) == 2 ? (side (k, 1) > side (j, 1) ? ", further ahead" : ", further back") :
                     side (k, 0) < side (j, 0) ? ", on the left" : ", on the right");
                break;
            }
            add (", ");
            add (utoa (5 * apart (px[k], py[k]), b, 10));
            add (" feet off");
            break;
        }
    }
    if (i < s->ndoors && !(r & MOUTH)) {
        add (st == DOORWAY ? " (a doorway)" : st & PS_WRECKED ? " (broken down)" : s->door[i].kind == PK_PORTCULLIS ?
             (st & PS_OPEN ? " (raised)" : " (down)") : st & PS_OPEN ? " (open)" : " (closed)");
    }
}

/* Choice k's place in the list: from the character's left round to its right, then behind it (where it stands last) */
static unsigned char __fastcall__ order (unsigned char k)
{
    return ph[k] == 0xFF ? 8 : (ph[k] - pc_face + 2) & 7;
}

/* The n choices put in their order (a few: a simple sort) */
static void __fastcall__ sort (unsigned char n)
{
    unsigned char k, j, t;
    unsigned u;

    for (k = 1; k < n; ++k) {
        for (j = k; j && order (j) < order (j - 1); --j) {
            u = px[j]; px[j] = px[j - 1]; px[j - 1] = u;
            u = py[j]; py[j] = py[j - 1]; py[j - 1] = u;
            t = ph[j]; ph[j] = ph[j - 1]; ph[j - 1] = t;
            t = pi[j]; pi[j] = pi[j - 1]; pi[j - 1] = t;
        }
    }
}

unsigned char __fastcall__ o_pick (unsigned char obj, unsigned char orel)
{
    struct sight* s = &sight_copy;
    unsigned char i, k, h, r, n = 0, how = obj & (PICK_TELL | PICK_ANY), best = 0xFF, bd = 0xFF;
    unsigned x, y;

    obj &= ~(PICK_TELL | PICK_ANY);
    hub_mem (SIGHT_BANK, s, SIGHT, sizeof *s);  /* (The senses' record as it is now) */
    for (i = 0; i < s->ndoors + s->nopen && n < NPICK; ++i) {
        if (i < s->ndoors) {
            r = s->door[i].rel;
            k = s->door[i].kind == PK_WINDOW || s->door[i].kind == PK_SLIT;    /* (A window, or a slit) */
            if (!(obj == OB_DOOR ? !(r & MOUTH) && !k : obj == OB_WINDOW ? k : obj == OB_PORTC ?
                s->door[i].kind == PK_PORTCULLIS : obj == OB_PASSAGE && (r & MOUTH))) {
                continue;
            }
            x = s->door[i].x;
            y = s->door[i].y;
        } else {
            r = s->open[i - s->ndoors].rel;
            k = s->open[i - s->ndoors].kind;
            if (!((obj == OB_STAIRS && (k == O_STAIRS_UP || k == O_STAIRS_DOWN || k == O_WAY_OUT)) ||
                (obj == OB_LADDER && (k == O_LADDER_UP || k == O_LADDER_DOWN)) ||
                (obj == OB_SHAFT && (k == O_SHAFT_UP || k == O_SHAFT_DOWN)) || (obj == OB_WAY_OUT && k == O_WAY_OUT))) {
                continue;
            }
            x = s->open[i - s->ndoors].x;
            y = s->open[i - s->ndoors].y;
        }
        h = r == 0xFF ? 0xFF : (pc_face + (r & 7)) & 7;
        if (orel != 0xFF && h != orel) {
            continue;
        }
        if (cx && x == cx && y == cy) {         /* (The one chosen from the list) */
            best = n;
        }
        px[n] = x;
        py[n] = y;
        ph[n] = h;
        pi[n++] = i;
    }
    if (!n) {
        if (how & PICK_TELL) {
            sayf ("\nYou see no %s %s.", thing[obj], orel == 0xFF ? "here" : "that way");
        }
        return 0;
    }
    if (best == 0xFF && n > 1 && !(how & PICK_ANY)) {
        for (k = 0; k < n; ++k) {               /* (The nearest directly ahead) */
            if (ph[k] == pc_face && (h = apart (px[k], py[k])) < bd) {
                bd = h;
                best = k;
            }
        }
        if (best == 0xFF) {                     /* (None: which, asked, from its left round to its right, then
                                                **   behind it) */
            sort (n);
            strcpy (lb, "Which ");
            add (thing[obj]);
            add (":");
            for (k = 0; k < n; ++k) {
                named (k, n);
            }
            add ("?");
            msg (lb);
            memcpy (&kept, cur, sizeof kept);
            asked = n;
            return 2;
        }
    }
    best = best == 0xFF ? 0 : best;
    tx = px[best];
    ty = py[best];
    return 1;
}

/* A command (main.c's, the parser's, before it's carried out): if a choice was asked, a number (1 ...) or a way
** ("left", "the one on the right": the nearest that way) chooses one, and the command asked about is carried out on
** it (c made it again; o_pick gives that one); any other command lets the choice go */
unsigned char __fastcall__ o_choose (struct cmd* c)
{
    unsigned char n = asked, k = 0xFF, j, d, bd = 0xFF;

    cur = c;
    asked = 0;
    cx = 0;
    if (!n || (c->action != VB_MOVE_DIRECTION && c->action != VB_MOVE_TOWARD) || c->obj.kind) {
        return 0;
    }
    if (c->n && c->n <= n && !c->obj.word[0]) {
        k = c->n - 1;
    } else if (c->dir <= H_NW) {
        for (j = 0; j < n; ++j) {
            if (ph[j] == c->dir && (d = apart (px[j], py[j])) < bd) {
                bd = d;
                k = j;
            }
        }
    }
    if (k == 0xFF) {
        return 0;
    }
    memcpy (c, &kept, sizeof kept);
    cx = px[k];
    cy = py[k];
    return 1;
}
