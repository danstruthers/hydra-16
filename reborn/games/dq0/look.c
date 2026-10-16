/*
** look.c - Dingus Quest Zero's places told (the UI's): what the character perceives, in words (look: words.c's, an
** overlay, from the world's record of it in the hub, level.h's struct sight: the UI never reads the dungeon); and the
** map (the maker draws its window into the hub, on asking), with the character's state beside it.
**
** The words are the character's: only what it sees now (or, in the dark, feels), each where it lies from it as it
** faces (one of eight: ahead, ahead to its right ... ahead to its left), and how far above or below it.  A room's
** size it knows when it's seen the whole of it; a ceiling when the light reaches it.
**
** The words for the places are the theme's (theme.h: a stratum's name, a room's by its kind, a lit room's light;
** theme.c's).
**
** The map's code is a swapped part's (part 3: swap.inc), called through its stub (bank.s's map).
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

char lb[400];                                   /* The words, as they're made (words.c's) */
struct sight sight_copy;                        /* The hub's record, copied for the overlay */
unsigned char sight_light;                      /*   and the character's light, */
char sight_lamp[14];                            /*   what gives it, named */
unsigned char sight_close;                      /*   and a close look (look's first 2) */

/* What the character perceives (first: the first time there; 2, a close look, the room's figures with it): the hub's
** record copied, worded (overlay 2) */
void __fastcall__ look (unsigned char first)
{
    sight_close = first == 2;                   /* (The theme's words for a place's first sight: later) */
    memcpy (&sight_copy, &HUB->sight, sizeof sight_copy);
    sight_light = HUB->pc_light;
    strcpy (sight_lamp, HUB->pc_lamp);
    pc_face = HUB->pc_face;
    words ();
    msg (lb);
}

/* A denizen the character sees has moved (the world's E_VIEW, V_STIR): the hub's record copied, worded (overlay 2),
** and told if what it sees of them has changed */
void stir (void)
{
    memcpy (&sight_copy, &HUB->sight, sizeof sight_copy);
    pc_face = HUB->pc_face;
    stir_words ();
    if (lb[0]) {
        msg (lb);
    }
}

/* ---- The map, and beside it (under it, if the screen's too narrow) the character's state: where it is (its
** stratum, its depth, its square), the way it faces, its light, its senses as they are now (the summary's, whatever
** lowers them in them), and the ruleset's short lines (the referee's: the attributes, the penalties) */

#define PANEL       14                          /* The panel's width */
#define PLINES      18
static char panel[PLINES][PANEL + 1];
static unsigned char np;

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

static void __fastcall__ pline (const char* s)
{
    if (np < PLINES) {
        strncpy (panel[np], s, PANEL);
        panel[np++][PANEL] = 0;
    }
}

/* A line of the panel: words, a number, words */
static void __fastcall__ num (const char* a, int v, const char* z)
{
    static char b[PANEL + 1];
    char n[8];

    strcpy (b, a);
    strcat (b, itoa (v, n, 10));
    strcat (b, z);
    pline (b);
}

/* The panel's lines, from the hub's header, and the referee's (rules: its lines, each ended by a new line) */
static void __fastcall__ make_panel (const char* rules)
{
    static char b[PANEL + 1];
    const char* e;
    unsigned char n;

    np = 0;
    theme_named (TN_STRATUM, HUB->sight.sname);
    pline (tname);
    num ("Depth ", -HUB->sight.z, " ft");
    num ("Sq ", (int) (HUB->pc_x - 32768U), ",");
    strcat (panel[np - 1], itoa ((int) (HUB->pc_y - 32768U), b, 10));
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
    pline ("1-9 up a-i dn");
    while (*rules) {                            /* (The referee's, a line each) */
        e = strchr (rules, '\n');
        n = e ? e - rules : strlen (rules);
        strncpy (b, rules, n > PANEL ? PANEL : n);
        b[n > PANEL ? PANEL : n] = 0;
        pline (b);
        rules += e ? n + 1 : n;
    }
}

void __fastcall__ s_map (const char* rules)
{
    static char line[MAP_W + PANEL + 3];
    unsigned char x, y, n, side, k;

    make_panel (rules);
    HUB->map_state = REQ_POSTED;                /* (The maker draws it into the hub) */
    hub_ring (T_MAKER);
    while (HUB->map_state != REQ_DONE) {
        hy_yield ();
        hub_map (0);
    }
    blank ();
    sayf ("The map (you face %s; heights in tens of feet against yours):", head_name[HUB->pc_face & 7]);
    for (n = 0; n < MAP_H - 1 && !HUB->map[n][0]; ++n) {
    }
    for (y = MAP_H - 1; y > n && !HUB->map[y][0]; --y) {
    }
    side = tw > MAP_W + PANEL + 1;
    for (k = 0; n + k <= y || (side && k < np); ++k) {  /* (The rows from the first seen to the last, the panel's
                                                **   beside them) */
        line[0] = 0;
        if (n + k <= y && n + k < MAP_H) {
            strcpy (line, HUB->map[n + k]);
        }
        if (side && k < np) {
            for (x = strlen (line); x < MAP_W + 1; ++x) {
                line[x] = ' ';
            }
            strcpy (line + MAP_W + 1, panel[k]);
        }
        say_raw (line);
    }
    HUB->map_state = REQ_FREE;
    if (!side) {
        blank ();
        for (k = 0; k < np; ++k) {
            say_raw (panel[k]);
        }
    }
    blank ();
}

#pragma code-name (pop)
#pragma rodata-name (pop)
