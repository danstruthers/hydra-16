/*
** words.c - (overlay 2) what the character perceives, in words (look.c's look: a copy of the world's record of it,
** struct sight, in the main RAM, for the hub isn't in the window while an overlay is).  The words are the
** character's: only what it sees now (or, in the dark, feels), each where it lies from it as it faces (one of eight:
** ahead, ahead to its right ... ahead to its left), and how far above or below it; nothing behind it (the user's: "you
** can't really see" it), though the senses mark the squares beside it there.  A room's size it knows when it's seen
** the whole of it; a ceiling when the light reaches it.  At a glance (the user's), in general words: a small hall, a
** long gallery, a high ceiling, a low, rough one (overlay 6's ceiling_glance); its figures (its size in feet, its
** ceiling's heights) only on a close look (look closely, examine the room: look's sight_close), and only for what it's
** seen (else its far side is lost in the dark).  What's further off is told in less detail, the further it is and the
** weaker the character's sight (the referee's: struct sight's eye): a thing too far to make out only by its size
** ("something small"), a denizen too far to tell what it is only as something there.
**
** The words for a room and its light are the theme's (theme.h; theme.c's theme_named, added to lb).
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#define PS_OPEN_        0x01                    /* (A portal's state: open: dq0w/world.h's PS_OPEN) */

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

/* Seen door i one of the doors (mouths 0: but a portcullis, a window or a slit), or the passages' mouths (MOUTH),
** and not behind it */
static unsigned char __fastcall__ is (unsigned char i, unsigned char mouths)
{
    return (S->door[i].rel & MOUTH) == mouths && (mouths || S->door[i].kind == PK_DOOR) && !REL_BEHIND (S->door[i].rel);
}

/* The doors seen (mouths 0), or the passages' mouths (MOUTH) */
static void __fastcall__ doors (unsigned char mouths)
{
    unsigned char seen[8], d, k, n = 0, i;

    memset (seen, 0, sizeof seen);
    for (i = 0; i < S->ndoors; ++i) {
        if (is (i, mouths)) {
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
            if ((S->door[i].rel & 7) == d && is (i, mouths)) {
                if (!mouths) {
                    add (S->door[i].state == DOORWAY ? " (a doorway)" : S->door[i].state & PS_WRECKED ?
                         " (broken down)" : S->door[i].state & PS_OPEN_ ? " (open)" :
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

/* The portcullises, windows and slits seen, each: "There is a barred window ahead to your left (its shutters
** open)." */
static void portals (void)
{
    unsigned char i, k, p;

    for (i = 0; i < S->ndoors; ++i) {
        k = S->door[i].kind;
        p = S->door[i].parts;
        if ((S->door[i].rel & MOUTH) || k == PK_DOOR || REL_BEHIND (S->door[i].rel)) {
            continue;
        }
        add (k == PK_PORTCULLIS ? " There is a portcullis " : k == PK_SLIT ? " There is an arrow slit " :
             p & PP_BARS ? " There is a barred window " : p & PP_GLASS ? " There is a glazed window " :
             p & PP_SHUTTERS ? " There is a shuttered window " : " There is an empty window frame ");
        add (relname[S->door[i].rel & 7]);
        if (k == PK_PORTCULLIS) {
            add (S->door[i].state & PS_WRECKED ? " (its bars forced apart)" : S->door[i].state & PS_OPEN_ ? " (raised)" :
                 " (down)");
        } else if (k == PK_WINDOW && (p & PP_SHUTTERS)) {
            add (S->door[i].state & PS_OPEN_ ? " (its shutters open)" : " (its shutters closed)");
        }
        height (S->door[i].dz);
        add (".");
    }
}

/* A trap's name, added to lb: "a scything blade, set off by a tripwire" (kind, trigger, state: world.h's TK_*,
** TT_*, TS_*: a pit that's given way an open one) */
void __fastcall__ o_trap_name (unsigned char kind, unsigned char trigger, unsigned char state)
{
    static const char* const name[8] = {
        "an open pit", "a covered pit", "a covered pit, spikes in it", "a deadfall", "a scything blade",
        "a dart trap", "a gas trap", "an alarm"
    };

    if (kind <= TK_SPIKED) {
        strcat (lb, kind == TK_SPIKED ? (state & TS_SPRUNG ? "a pit, spikes in it" : name[kind]) :
            state & TS_SPRUNG ? name[TK_PIT] : name[kind]);
        return;
    }
    strcat (lb, name[kind & 7]);
    strcat (lb, trigger == TT_PLATE ? ", set off by a plate in the floor" : trigger == TT_WIRE ?
        ", set off by a tripwire" : ", set to go off as its door opens");
}

/* The traps it knows of and sees: "There is a dart trap, set off by a tripwire, ahead to your right." */
static void traps (void)
{
    unsigned char i;

    for (i = 0; i < S->ntraps; ++i) {
        if (REL_BEHIND (S->trap[i].rel)) {
            continue;
        }
        add (" There is ");
        o_trap_name (S->trap[i].kind, S->trap[i].trigger, S->trap[i].state);   /* (Here: its stub's overlay 2) */
        add (S->trap[i].kind <= TK_SPIKED ? " " : ", ");
        add (S->trap[i].rel == 0xFF ? "at your feet" : relname[S->trap[i].rel & 7]);
        add (S->trap[i].state & TS_DISARMED ? " (disarmed)." : S->trap[i].kind > TK_SPIKED &&
             (S->trap[i].state & TS_SPRUNG) ? " (sprung)." : ".");
    }
}

static void zones (void)
{
    unsigned char i, at;
    signed char dz;

    for (i = 0; i < S->nzones; ++i) {
        if (REL_BEHIND (S->zone[i].rel)) {
            continue;
        }
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

/* The ceiling: at a glance, or (a close look) its figures */
static void ceiling (void)
{
    if (S->ceil == C_UNSEEN) {
        if (!S->dark && !S->lit) {
            add (" Above, the dark hides the ceiling.");
        }
        return;
    }
    if (!sight_close) {
        ceiling_glance ();                      /* (Overlay 6's: this one's full) */
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
    theme_named (TN_ROOM, S->kind);
    if (S->lit || !sight_light) {
        if (S->lit) {
            add (", ");
            theme_named (TN_LIGHT, 0);
        }
        add (".");
    } else {
        add (", by the light of your ");
        add (sight_lamp);
        add (".");
    }
    if (sight_close && S->size != S_PART) {      /* (Its figures, a close look's: what it's seen of it) */
        add (" It is ");
        addn (5 * S->rw);
        add (" feet by ");
        addn (5 * S->rh);
        add (".");
    } else if (sight_close) {
        add (S->lit ? " You haven't seen all of it yet." : " Its far side is lost in the dark.");
    }
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
    portals ();
    for (i = 0; i < S->nopen; ++i) {
        if (!REL_BEHIND (S->open[i].rel)) {
            opening (i);
        }
    }
}

/* ---- A passage */

static void passage (void)
{
    unsigned char i, n;
    signed char dz;

    for (i = n = 0; i < S->ndoors; ++i) {     /* (The doors beside it, but behind it) */
        n += !REL_BEHIND (S->door[i].rel);
    }
    add (S->dark ? "It is dark; you feel your way. " : "");
    add (S->nways <= 1 && n ? "The passage comes to a door; it goes back " : S->nways <= 1 ?
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
    portals ();
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
    traps ();
    seen_words ();                              /* (The things and the denizens it sees: overlay 6's) */
}
