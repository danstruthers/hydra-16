/*
** search.c - (swapped part 10) a search, by its form (HOUSE.md's traps, searching and hiding; the user's, 2026-10-22):
** "search" (about the character, as far as it sees), "search nearby", "search around" (within reach: its own square and
** the eight round it), "search the east wall", "search the wall ahead" (that wall's squares, for what's hidden in it),
** "search the floor", "search around on the floor" (the floor nearby), "search for the key", "search around for a
** dagger" (that thing, till it's found), "search the table", "search the chest" (what's on it or in it, what's hidden
** there, and its trap: examine's check of it, the one rule).  In the dark it's by touch (the world's), and nearby
** unless it's said otherwise; its words as it begins say so ("You feel about you in the dark...").  A long task, the
** world's (I_PORTAL's PA_SEARCH: b its form, SF_*, a wall's heading times 16; w1 a search for a thing's time, the
** things' IO_SEEK's, which keeps the thing for the search's end); a thing out of reach walked over to first (in the
** light); "search the chest for traps" in the light examine's (way.c's, its 10 seconds), "search the door" the world's
** going over it for traps (PA_CHECK, main.c's door work).  What the search finds told as it ends (main.c's E_PORTAL:
** the things' words, IO_SEARCH's, by the same form).  "search the room" a real search of the whole of it (the user's
** playtest, 2026-11-02; the world's wroom.inc), its words as it goes here (search_said); a thing searched worded by what
** it is: a heap poked through, a thing that holds things gone through, the rest looked over (the things' RC_*)
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "tell.h"

#pragma local-strings (on)
#pragma code-name ("SW10CODE")
#pragma rodata-name ("SW10RODATA")

extern unsigned char finds, busy;               /* (main.c's: a search's end awaited; the character busy) */
unsigned char __fastcall__ things (unsigned char op, const char* name);    /* (main.c's) */

/* Which of the words (0-ended, then an empty one) w is: 1, or 0 */
static unsigned char __fastcall__ among (const char* list, const char* w)
{
    for (; *list; list += strlen (list) + 1) {
        if (!strcmp (list, w)) {
            return 1;
        }
    }
    return 0;
}

/* The search the command says (cmd: VB_SEARCH, its aux SR_FOR or SR_NEAR as its verb's words said, EX_TRAPS for
** traps), begun: 0; or PA_CHECK, a door's (main.c's door work) */
unsigned char s_search_go (void)
{
    struct msg m;                               /* (On the C stack: the main RAM's room) */
    static const char* const lit[6] = { "\nSearching about...", "\nYou search about you, within reach...",
        "\nYou search the %s wall...", "\nYou search the floor about you...", "\nYou search about for the %s...",
        "\nYou search the %s..." };
    static const char* const felt[6] = { "", "\nYou feel about you in the dark...",
        "\nYou feel along the %s wall in the dark...", "\nYou feel over the floor about you in the dark...",
        "\nYou feel about in the dark for the %s...", "\nYou feel over the %s in the dark..." };
    static const char* const begun_on[4] = { "\nYou look the %s over...", "\nYou poke through the %s...",
        "\nYou search the %s...", "\nYou go through the %s..." };
    const char* w = cmd.obj.word;
    unsigned char k = cmd.obj.kind, a = cmd.aux, f, h = 0, d = sight_get ()->dark, c = RC_PLAIN;
    unsigned t = 0;

    if (k == OB_DOOR || k == OB_PORTC || k == OB_WINDOW) {
        return PA_CHECK;                        /* (A door gone over for traps: the world's, examine's) */
    }
    if (k == OB_ROOM && strcmp (w, "ceiling") && (d = sight_get ()->where) == W_ROOM) {
        if (sight_get ()->size >= S_VAST) {
            msg ("This cavern is too vast to go over from end to end: search about you, or search for traps as you go.");
            return 0;
        }
        finds = 0;
        m.kind = I_PORTAL;
        m.who = 0;
        m.a = PA_SEARCH;
        m.b = SF_ROOM;
        m.w1 = m.w2 = 0;
        m.x[0] = m.x[1] = 0;
        if (hub_send (&m)) {
            busy = 1;
            t_prompt (0);
        }
        return 0;
    }
    d = sight_get ()->dark;
    if (k == OB_ROOM) {
        w = "";                                 /* (No room here, a passage: a search about it) */
    }
    if (k == OB_WALL) {
        f = SF_WALL;
        h = cmd.obj.lies <= H_NW ? cmd.obj.lies : HUB->pc_face;   /* (Unsaid, the wall ahead) */
    } else if (!*w) {
        f = a & SR_NEAR || d ? SF_NEAR : SF_ABOUT;
    } else if (among ("floor\0ground\0", w)) {
        f = SF_FLOOR;
    } else if (among ("nearby\0around\0about\0here\0", w)) {
        f = SF_NEAR;
    } else if (among ("things\0thing\0traps\0trap\0everything\0anything\0something\0", w)) {
        f = d ? SF_NEAR : SF_ABOUT;             /* (Hidden things, traps: a search's) */
    } else if (a & SR_FOR) {
        f = SF_FOR;
    } else if ((a & EX_TRAPS) && !d) {
        return look_way (VB_EXAMINE_OBJECT);    /* (A thing gone over for traps, in the light: examine's, one rule) */
    } else {
        f = SF_ON;
        h = a & EX_TRAPS ? SF_TRAPS >> 4 : 0;   /* (By touch, for traps, said: its words, the things') */
    }
    if (f >= SF_FOR) {                          /* (The thing: the things' to find, and keep) */
        HUB->ireq.to = f | (d ? SF_TOUCH : 0);
        k = things (IO_SEEK, w);
        if (k == IR_NOT_HERE && reach_go ()) {
            return 0;                           /* (Out of reach: walked over to it first, then again) */
        }
        if (k != IR_OK) {
            if (d) {
                sayf ("\nYou feel about you, but there's no %s within reach.", w);
            } else {
                tell (T_YOU_SEE_NONE);
            }
            return 0;
        }
        t = HUB->ireq.x;                        /* (For it: the time to find it, the world's; 0, a whole search) */
        c = HUB->ireq.craft;
    }
    if (f == SF_ON && !d) {
        sayf (begun_on[c], w);                  /* (By what it is: the things') */
    } else {
        sayf (d ? felt[f] : lit[f], f == SF_WALL ? head_name[believed (h)] : w);   /* (By the north it believes in) */
    }
    finds = 0xFF;                               /* (Its end awaited: what it finds told, E_PORTAL's) */
    m.kind = I_PORTAL;
    m.who = 0;
    m.a = PA_SEARCH;
    m.b = f | h << 4;
    m.w1 = t;
    m.w2 = 0;
    m.x[0] = m.x[1] = 0;
    if (hub_send (&m)) {
        busy = 1;
        t_prompt (0);
    }
    return 0;
}

/* A room searched whole, as it goes (an E_BEAT's WB_SEARCH, the world's wroom.inc; main.c's news): a line for each stage,
** by touch in the dark; a thing's time up, or a stop's, the things asked what came of it, told as it's found */
void __fastcall__ s_search_said (const struct msg* m)
{
    static const char* const lit[7] = { "\nYou set about searching the whole room.", "\nYou go on searching the room.",
        "\nYou've been all over this room already: search about you, a wall, or a thing, for a closer look.",
        "\nYou work along the wall to your left.", "",
        "\nYou work back and forth over the floor in the middle of the room.", "\nYou go over the walls and the floor about you." };
    static const char* const felt[7] = { "\nYou set about feeling your way round the whole room.",
        "\nYou go on feeling your way round the room.", "",
        "\nYou feel along the wall to your left.", "",
        "\nYou feel your way back and forth over the floor in the middle of the room.",
        "\nYou feel over the walls and the floor about you." };
    static const char* const thing[4] = { "\nYou look over the %s.", "\nYou poke through the %s.", "\nYou go through the %s.",
        "\nYou go through the %s." };
    unsigned char w = m->w1 >> 8, t = m->b, s = w >> 4;
    const char* q;

    w &= 15;
    if (w == RSW_CORNER) {                      /* (By the north it believes in: each wall its own) */
        sayf (t ? "\nYou turn the corner, and feel along the %s wall." : "\nYou turn the corner, and work along the %s wall.",
            head_name[believed (m->w2 << 1)]);
    } else if (w < RSW_END) {
        say (t ? felt[w] : lit[w]);
    } else if (w == RSW_DONE) {
        msg ("You've been over the whole room.");
    } else if (w == RSW_THING) {
        hub_mem (KIND_BANK, lb, CATALOGUE[(unsigned char) m->w2].name, sizeof CATALOGUE[0].name);
        q = strstr (lb, " of ");
        if (t) {
            sayf (s == RC_PLAIN ? "\nYou feel over the %s." : "\nYou feel through the %s.", q && s == RC_HEAP ? q + 4 : lb);
        } else {
            sayf (thing[s], q && s == RC_HEAP ? q + 4 : lb);
        }
    } else {
        HUB->ireq.to = (w == RSW_END ? IOS_THING : IOS_FLOOR) | (t ? SF_TOUCH : 0);
        HUB->ireq.item = HUB->ireq.x = m->w2;
        HUB->ireq.y = m->x[0] | (unsigned) m->x[1] << 8;
        if (things (IO_SEARCH, "")) {
            q = tt ();
            msg (*q == ' ' ? q + 1 : q);
        }
    }
    t_flush ();
}
