/*
** refuse.c - (overlay 1) a command refused, said back from its line so the answer names what the line named: a verb
** the game doesn't know, or doesn't do yet; a thing named that its verb would pass over, or do to something else,
** unsaid (COMMANDS-CRITIQUE.md's step 4, held by tests.js's dq0sweep).  help.c's o_unknown till 2026-11-06.
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")
#pragma bss-name ("OV1RODATA")                  /* (Its working values in its overlay's bank, not the main RAM's) */
#pragma static-locals (on)

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "actions.h"

/* A command refused, said back from its line (cl):
**   U_VERB    no verb of the game's: "I don't know how to bribe." (its first word)
**   U_YET     a verb the game has but doesn't do yet, or not to such a thing yet: "You can't touch the pot in this
**             game yet." (the line, if it names a thing; else its first word: "You can't eat in this game yet.")
**   U_CANT    a thing the verb can't be done to: "You can't take the stairs."
**   U_NOTE    nothing written on it: "There's nothing written on the stairs." (the line after its verb) */
void __fastcall__ o_unknown (unsigned char f)
{
    static const char* const said[4] = { "\nI don't know how to %s.", "\nYou can't %s in this game yet.",
        "\nYou can't %s.", "\nThere's nothing written on %s." };
    static const char* p;
    static unsigned char d, all;

    p = cl;
    all = f > U_YET || (f && (cmd.obj.word[0] || cmd.obj.kind));
    if (f == U_NOTE) {
        p = strchr (p, ' ');                    /* (Past its verb) */
        p = p ? p + 1 : "";
    }
    for (d = 0; p[d] && (all || p[d] != ' ') && d < 60; ++d) {
        lb[d] = p[d];
    }
    while (d && (lb[d - 1] == ' ' || lb[d - 1] == '.' || lb[d - 1] == '!')) {
        --d;
    }
    lb[d] = 0;
    sayf (said[f], d ? lb : "it");
}

/* A command that names a thing its verb would pass over, or do to something else ("sit on the pot" sat down, "climb
** the pot" climbed the stairs, "light the stairs" lit a new torch, "take the stairs" asked "Take what?"): refused,
** said back (o_unknown), 1; else 0, carried out as ever.  Asked before the command's carried out (main.c's command,
** its things resolved), by its verb's row in rules[]: a thing named by its word (an item, a denizen), refused U_*
** (the row's high four bits; W_CLIMB: but a well, a rope, a thing's letter); a part of the place (a kind, OB_*) not
** among those the verb does (the row's mask, masks[]'s), refused U_* (its low four bits).  (A blow at a thing named that's no
** foe is main.c's: "You can't kick the pot.") */
#define BIT(k)      (1u << (k))
#define CLIMBS      (BIT (OB_STAIRS) | BIT (OB_LADDER) | BIT (OB_SHAFT) | BIT (OB_WINDOW) | BIT (OB_LEDGE))
#define WORKED      (BIT (OB_DOOR) | BIT (OB_PORTC) | BIT (OB_WINDOW) | BIT (OB_WAY_OUT) | BIT (OB_TRAP))
#define SEARCHED    (BIT (OB_DOOR) | BIT (OB_PORTC) | BIT (OB_WINDOW) | BIT (OB_ROOM) | BIT (OB_WALL))
#define WAYS        0x0ABEu                     /* (way.c's WAY_OBJ: what's looked at, listened at, sniffed at, a way) */
#define INS         (BIT (OB_DOOR) | BIT (OB_PASSAGE) | BIT (OB_SHAFT) | BIT (OB_WINDOW) | BIT (OB_PORTC))   /* (Looked in) */
#define W_CLIMB     0x80                        /* (A word refused but a well's, a rope's, a thing's letter) */
#define W_PART      0x40                        /* (A word refused but a door's or a window's part's, "bar", "bars",
                                                **   "pane", "shutters", or a letter's: dq0sweep2's, 2026-11-07) */
#define ROW(a, w, k, m)  (a), (w) << 4 | (k), (m)
enum { M_NONE, M_CLIMBS, M_WAYS, M_SEARCHED, M_OUT, M_WORKED, M_IN };
static const unsigned masks[] = { 0, CLIMBS, WAYS, SEARCHED, BIT (OB_WAY_OUT), WORKED, INS };   /* (The kinds a verb does) */
static const unsigned char rules[] = {          /* (A verb; a word's refusal, a kind's; the kinds it does, masks[]'s) */
    ROW (VB_SIT, U_YET, U_YET, M_NONE), ROW (VB_STAND, U_YET, U_YET, M_NONE), ROW (VB_CROUCH, U_YET, U_YET, M_NONE),
    ROW (VB_LIE_DOWN, U_YET, U_YET, M_NONE),         /* (Sitting on a thing, kneeling by it: not yet) */
    ROW (VB_CLIMB, U_CANT | (W_CLIMB >> 4), U_CANT, M_CLIMBS),
    ROW (VB_SMELL, U_YET, U_YET, M_WAYS), ROW (VB_LISTEN, U_YET, U_YET, M_WAYS),     /* (A way alone, as yet) */
    ROW (VB_SEARCH, 0, U_YET, M_SEARCHED),
    ROW (VB_READ_OBJECT, 0, U_NOTE, M_OUT),   /* (The way out: a theme's may bear words, the puzzle's) */
    ROW (VB_LIGHT_OBJECT, 0, U_CANT, M_NONE), ROW (VB_EXTINGUISH_OBJECT, 0, U_CANT, M_NONE), ROW (VB_RETRIEVE_OBJECT, 0, U_CANT, M_NONE),
    ROW (VB_OPEN_OBJECT, 0, U_CANT, M_WORKED), ROW (VB_CLOSE_OBJECT, 0, U_CANT, M_WORKED),    /* (A door's work, a blow) */
    ROW (VB_UNLOCK_OBJECT, 0, U_CANT, M_WORKED), ROW (VB_LOCK_OBJECT, 0, U_CANT, M_WORKED),
    ROW (VB_FORCE_OBJECT, 0, U_CANT, M_WORKED), ROW (VB_RAISE_OBJECT, U_CANT | (W_PART >> 4), U_CANT, M_WORKED),
    ROW (VB_LOWER_OBJECT, U_CANT | (W_PART >> 4), U_CANT, M_WORKED), ROW (VB_BREAK_OBJECT, 0, U_CANT, M_WORKED),
    ROW (VB_DISARM_OBJECT, U_CANT | (W_PART >> 4), U_CANT, M_WORKED), ROW (VB_BAR_OBJECT, U_CANT | (W_PART >> 4), U_CANT, M_WORKED),
    ROW (VB_BEND_OBJECT, U_CANT | (W_PART >> 4), U_CANT, M_WORKED), ROW (VB_ATTACK_OBJECT, 0, U_CANT, M_WORKED),
    ROW (VB_EAT_OBJECT, 0, U_CANT, M_NONE), ROW (VB_DRINK, 0, U_CANT, M_NONE), ROW (VB_FILL_OBJECT, 0, U_CANT, M_NONE),
    ROW (VB_TIE_OBJECT, 0, U_CANT, M_NONE), ROW (VB_UNTIE_OBJECT, 0, U_CANT, M_NONE), ROW (VB_EQUIP_OBJECT, 0, U_CANT, M_NONE),
    ROW (VB_OBSERVE_IN, 0, U_CANT, M_IN), 0      /* (A feature eaten, drunk, filled, tied, worn, looked in: dq0sweep2's) */
};
unsigned char o_refused (void)
{
    static const unsigned char* r;
    static const char* w;
    static unsigned char f;

    w = cmd.obj.word;
    f = 0;
    for (r = rules; *r && *r != cmd.action; r += 3) {
    }
    if (!*r) {
    } else if (*w) {
        f = r[1] >> 4 & 3;
        if (*w == '#' || ((r[1] & W_CLIMB) && (!strcmp (w, "well") || strstr (w, "rope"))) || ((r[1] & W_PART) &&
            (strstr (w, "bar") || strstr (w, "pane") || strstr (w, "shutter")))) {
            f = (r[1] & (W_CLIMB | W_PART)) ? 0 : f;
        }
    } else if (cmd.obj.kind && !(masks[r[2]] >> (cmd.obj.kind & 15) & 1)) {
        f = r[1] & 15;
    }
    if (f) {
        o_unknown (f);
    }
    return f != 0;
}
