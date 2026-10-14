/*
** look.c - Dingus Quest Zero's places told (the UI's): what the character perceives, in words (look), from the
** world's record of it in the hub (level.h's struct sight: the UI never reads the dungeon); and the map (the world
** draws its window into the hub, on asking), with the character's state beside it.
**
** The words are the character's: only what it sees now (or, in the dark, feels), each where it lies from it as it
** faces (one of eight: ahead, ahead to its right ... ahead to its left), and how far above or below it.  A room's
** size it knows when it's seen the whole of it; a ceiling when the light reaches it.
**
** The words are a stand-in theme's, till themes are data: a room's kind one of eight names, a stratum's one of
** eight, a lit room's light torches.
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

static const char* const kindname[8] = {
    "chamber", "hall", "cell", "vault", "crypt", "gallery", "cave", "storeroom"
};
const char* const stratum_name[8] = {
    "Upper Halls", "Old Cellars", "Cisterns", "Long Galleries", "Bone Vaults", "Sunken Ways", "Deep Halls",
    "Black Crypts"
};
static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};
static const char* const count[5] = { "", "one", "two", "three", "four" };

static char lb[400];                            /* The words, as they're made */
static struct sight* S;

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

static void __fastcall__ addn (int v)
{
    char b[8];

    add (itoa (v, b, 10));
}

/* " 6 feet higher" or " 4 feet lower" (nothing if within a couple of feet) */
static void __fastcall__ height (signed char dz)
{
    if (dz >= 3 || dz <= -3) {
        add (" ");
        addn (dz > 0 ? dz : -dz);
        add (dz > 0 ? " feet higher" : " feet lower");
    }
}

/* ---- A room */

static void doors (void)
{
    unsigned char seen[8], d, k, n = 0, i;

    memset (seen, 0, sizeof seen);
    for (i = 0; i < S->ndoors; ++i) {
        ++seen[S->door[i].rel & 7];
    }
    for (d = 0; d < 8; ++d) {
        n += seen[d] != 0;
    }
    for (d = 0, k = 0; d < 8; ++d) {
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
        for (i = 0; i < S->ndoors; ++i) {       /* (The first that way's height, if it's well above or below) */
            if ((S->door[i].rel & 7) == d) {
                if (S->door[i].dz >= 3 || S->door[i].dz <= -3) {
                    add (",");
                    height (S->door[i].dz);
                }
                break;
            }
        }
    }
    if (n) {
        add (".");
    }
}

static void zones (void)
{
    unsigned char i, at;
    signed char dz;

    for (i = 0; i < S->nzones; ++i) {
        dz = S->zone[i].dz;
        add (" ");
        at = strlen (lb);
        add (relname[S->zone[i].rel & 7]);
        lb[at] &= ~0x20;                        /* (Its first letter a capital) */
        if (dz >= 3 || dz <= -3) {
            add (dz > 0 ? " the floor rises " : " the floor falls ");
            addn (dz > 0 ? dz : -dz);
            add (dz > 0 ? " feet to a higher part of the room" : " feet to a lower part of the room");
            add (S->zone[i].how == Z_STEPS ? ", by steps." : S->zone[i].how == Z_SLOPE ? ", in a slope." :
                 ", at a ledge.");
        } else {
            add (" the room goes on, nearly level.");
        }
    }
}

static void ceiling (void)
{
    if (S->ceil == C_UNSEEN) {
        if (!S->dark && !S->lit) {
            add (" Above, the dark hides the ceiling.");
        }
        return;
    }
    switch (S->ceil != C_ROUGH && S->cwall >= S->cmid ? C_FLAT : S->ceil) {   /* (A vault no higher in the middle: flat) */
    case C_FLAT:
        add (" The ceiling is flat, ");
        addn (S->cmid);
        add (" feet up.");
        return;
    case C_BARREL:
        add (" A barrel vault rises to ");
        break;
    case C_DOME:
        add (" The ceiling is a dome, ");
        break;
    case C_CATHEDRAL:
        add (" A pointed vault rises high overhead, to ");
        break;
    case C_SLOPED:
        add (" The ceiling slopes from ");
        addn (S->cmid);
        add (" feet down to ");
        addn (S->cwall);
        add (".");
        return;
    default:
        add (" The ceiling is rough stone, about ");
        addn (S->cmid);
        add (" feet up.");
        return;
    }
    addn (S->cmid);
    add (S->ceil == C_DOME ? " feet at its height and " : " feet, ");
    addn (S->cwall);
    add (" at the walls.");
}

static void __fastcall__ opening (unsigned char i)
{
    unsigned char k = S->open[i].kind, rel = S->open[i].rel;

    add (rel == 0xFF ? " You stand by " : " You see ");
    add (k == O_WAY_OUT ? "the stairs up to the way out" : k == O_STAIRS_UP ? "a stairway leading up" :
         k == O_STAIRS_DOWN ? "a stairway leading down" : k == O_LADDER_UP ? "a ladder going up" :
         k == O_LADDER_DOWN ? "a ladder going down" : k == O_SHAFT_UP ? "a shaft in the ceiling" :
         k == O_SHAFT_DOWN ? "a shaft in the floor" : "a window");
    if (rel != 0xFF) {
        add (" ");
        add (relname[rel & 7]);
        height (S->open[i].dz);
    }
    add (".");
}

static void room (void)
{
    unsigned char i;

    if (S->dark) {
        add ("It is dark. You can see nothing, but you feel the floor of a room about you.");
        return;
    }
    add (S->size != S_PART ? "You are in a" : "You can see part of a");
    add (S->size == S_SMALL ? " small " : S->size == S_LARGE ? " large " : S->size == S_LONG ? " long " : " ");
    add (kindname[S->kind & 7]);
    add (S->lit ? ", lit by torches in iron brackets." : HUB->pc_light ? ", by the light of your torch." : ".");
    if (S->up_ft) {
        add (" The floor slopes up ");
        add (relname[S->up_rel & 7]);
        add (", ");
        addn (S->up_ft);
        add (S->up_ft == 1 ? " foot in ten." : " feet in ten.");
    }
    ceiling ();
    zones ();
    doors ();
    for (i = 0; i < S->nopen; ++i) {
        opening (i);
    }
}

/* ---- A passage */

static void passage (void)
{
    unsigned char i;
    signed char dz;

    add (S->dark ? "It is dark; you feel your way. " : "");
    add (S->nways <= 1 ? "The passage ends here; it goes back " : S->nways == 2 ? "You are in a passage. It goes " :
         "The passage forks here. It goes ");
    for (i = 0; i < S->nways; ++i) {
        if (i) {
            add (i + 1 == S->nways ? " and " : ", ");
        }
        add (relname[S->way[i].rel & 7]);
        dz = S->way[i].dz;
        if (S->way[i].steps) {
            add (dz > 0 ? " up steps" : " down steps");
        } else if (dz >= 1 || dz <= -1) {
            add (dz > 0 ? ", rising" : ", falling");
        }
    }
    add (".");
}

/* What the character perceives (first: the first time there) */
void __fastcall__ look (unsigned char first)
{
    (void) first;                               /* (The theme's words for a place's first sight: later) */
    lb[0] = 0;
    S = &HUB->sight;
    if (S->where == W_ROOM) {
        room ();
    } else {
        passage ();
    }
    msg (lb);
}

/* ---- The map, and beside it (under it, if the screen's too narrow) the character's state: where it is (its
** stratum, its depth, its square), the way it faces, its light, its senses as they are now (the summary's, whatever
** lowers them in them), and the ruleset's short lines (the referee's: the attributes, the penalties) */

#define PANEL       14                          /* The panel's width */
#define PLINES      18
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
    pline (stratum_name[HUB->sight.sname & 7]);
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

void __fastcall__ map (const char* rules)
{
    static char line[MAP_W + PANEL + 3];
    unsigned char x, y, n, side, k;

    make_panel (rules);
    HUB->map_state = REQ_POSTED;                /* (The world draws it into the hub) */
    hub_ring (T_WORLD);
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
