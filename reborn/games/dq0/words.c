/*
** words.c - (overlay 2) what the character perceives, in words (look.c's look: a copy of the world's record of it,
** struct sight, in the main RAM, for the hub isn't in the window while an overlay is).  The words are the
** character's: only what it sees now (or, in the dark, feels), each where it lies from it as it faces (one of eight:
** ahead, ahead to its right ... ahead to its left), and how far above or below it.  A room's size it knows when it's
** seen the whole of it; a ceiling when the light reaches it.
**
** The words are a stand-in theme's, till themes are data: a room's kind one of eight names, a lit room's light
** torches.
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "level.h"

#define PS_OPEN_        0x01                    /* (A portal's state: open: dq0w/world.h's PS_OPEN) */

static const char* const kindname[8] = {
    "chamber", "hall", "cell", "vault", "crypt", "gallery", "cave", "storeroom"
};
static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};
static const char* const count[5] = { "", "one", "two", "three", "four" };

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

/* The doors seen (mouths 0), or the passages' mouths (MOUTH) */
static void __fastcall__ doors (unsigned char mouths)
{
    unsigned char seen[8], d, k, n = 0, i;

    memset (seen, 0, sizeof seen);
    for (i = 0; i < S->ndoors; ++i) {
        if ((S->door[i].rel & MOUTH) == mouths) {
            ++seen[S->door[i].rel & 7];
        }
    }
    for (d = 0; d < 8; ++d) {
        n += seen[d] != 0;
    }
    for (d = 0, k = 0; d < 8; ++d) {
        if (!seen[d]) {
            continue;
        }
        if (!k++) {
            add (seen[d] == 1 ? (mouths ? " A passage leads off " : " There is a door ") : " There are ");
            if (seen[d] > 1) {
                add (count[seen[d] > 4 ? 4 : seen[d]]);
                add (mouths ? " passages leading off " : " doors ");
            }
        } else {
            add (k == n ? " and " : ", ");
            add (count[seen[d] > 4 ? 4 : seen[d]]);
            add (" ");
        }
        add (relname[d]);
        for (i = 0; i < S->ndoors; ++i) {       /* (The first that way's state and height) */
            if ((S->door[i].rel & 7) == d && (S->door[i].rel & MOUTH) == mouths) {
                if (!mouths) {
                    add (S->door[i].state == DOORWAY ? " (a doorway)" : S->door[i].state & PS_OPEN_ ? " (open)" :
                         " (closed)");
                }
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
    add (S->lit ? ", lit by torches in iron brackets." : sight_light ? ", by the light of your torch." : ".");
    if (S->up_ft) {                             /* (The slope's words, the user's: a slight rise, a rise, steep) */
        add (S->up_ft <= 2 ? " The floor rises gently " : S->up_ft <= 5 ? " The floor rises " : S->up_ft <= 9 ?
             " The floor rises steeply " : " The floor climbs sharply ");
        add (relname[S->up_rel & 7]);
        add (".");
    }
    ceiling ();
    zones ();
    doors (0);
    doors (MOUTH);
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
    add (S->nways <= 1 && S->ndoors ? "The passage comes to a door; it goes back " : S->nways <= 1 ?
         "The passage ends here; it goes back " : S->nways == 2 ? "You are in a passage. It goes " :
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
            add (dz == 1 ? ", rising gently" : dz == -1 ? ", falling gently" : dz > 0 ? ", rising" : ", falling");
        }
    }
    add (".");
    doors (0);                                  /* (A door beside it) */
}

/* The things it sees lying about: "On the floor ahead lies a pick." "At your feet lie some bones." */
static void items (void)
{
    unsigned char i;

    for (i = 0; i < S->nitems; ++i) {
        add (S->item[i].rel == 0xFF ? " At your feet " : " On the floor ");
        if (S->item[i].rel != 0xFF) {
            add (relname[S->item[i].rel & 7]);
            add (" ");
        }
        add (S->item[i].plural || S->item[i].qty > 1 ? "lie " : "lies ");
        if (S->item[i].qty > 1) {
            addn (S->item[i].qty);
            add (" ");
        } else {
            add (S->item[i].plural ? "some " : strchr ("aeiou", S->item[i].name[0]) ? "an " : "a ");
        }
        add (S->item[i].name);
        add (".");
    }
}

/* What the character perceives, in words, into lb (from look.c's copy of the hub's record) */
void o_words (void)
{
    lb[0] = 0;
    S = &sight_copy;
    if (S->where == W_ROOM) {
        room ();
    } else {
        passage ();
    }
    items ();
}
