/*
** refuse.c - (overlay 24; overlay 1's till 2026-11-07, its room) a command refused, said back from its line so the
** answer names what the line named: a verb the game doesn't know, or doesn't do yet; a thing named that its verb would
** pass over, or do to something else, unsaid (COMMANDS-CRITIQUE.md's step 4, held by tests.js's dq0sweep).  help.c's
** o_unknown till 2026-11-06.  And the refusals' one table (§5.2's step K, 2026-11-07): each meaning one wording, said
** by o_say_no from wherever it's refused, the thing's name put in.
*/

#pragma code-name ("OV24CODE")
#pragma rodata-name ("OV24RODATA")
#pragma data-name ("OV24BSS")
#pragma bss-name ("OV24BSS")                    /* (Its working values in its overlay's bank, not the main RAM's) */
#pragma static-locals (on)

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "actions.h"

/* The refusals, by U_* and A_* (dq0.h's): one wording a meaning, %s the thing's name, or the line's words */
static const char* const said[A_COUNT] = {
    "\nI don't know how to %s.",                /* U_VERB    no verb of the game's (the line's first word) */
    "\nYou can't %s in this game yet.",         /* U_YET     a verb the game doesn't do yet, or not to such a thing */
    "\nYou can't %s.",                          /* U_CANT    a thing the verb can't be done to (the line) */
    "\nThere's nothing written on %s.",         /* U_NOTE    (the line after its verb) */
    "\n%s what?",                               /* U_WHAT    nothing named: "Take what?", "Pick up what?" (the line) */
    "\nYou see no %s here.",                    /* A_SEE_NO  none of that name in sight */
    "\nYou see none of those here.",            /*   its name not to be said (none, or a handle's "#") */
    "\nYou have no %s.",                        /* A_HAVE_NO none of that name carried */
    "\nYou carry none of those.",               /*   its name not to be said */
    "\nThere's nothing here to take.",          /* A_NO_TAKE */
    "\nYou haven't seen %s here.",              /* A_UNSEEN  (a, an put in: a door, a way, a thing to go to) */
    "\nYou haven't seen %s that way.",          /* A_UNSEEN_WAY */
    "\n%s is out of reach: go up to it first.", /* A_FAR     (the put in, a name not capitalised: "The giant rat") */
    "\nYou can't get near enough to %s."        /* A_NEARER  walked toward, and stopped short */
};

/* Refusal a (U_*, A_*) said, w its thing's name put in (0: "it"); | A_LB, added to lb, not said (the sway's words, a
** blow's note).  w copied first, so it may be lb, or any of the main RAM's (not another overlay's: its bank's out) */
void __fastcall__ o_say_no (unsigned char a, const char* w)
{
    static char t[64];
    static const char* p;
    static char* q;
    static unsigned char f;

    f = a & A_LB;
    a &= ~A_LB;
    if (!w) {
        w = "it";
    }
    if ((a == A_SEE_NO || a == A_HAVE_NO) && (!*w || *w == '#')) {
        ++a;                                    /* (No name to say: the form without one) */
    }
    t[0] = 0;
    if (a == A_UNSEEN || a == A_UNSEEN_WAY) {
        strcpy (t, strchr ("aeiou", *w) ? "an " : "a ");
    } else if (a == A_FAR && *w >= 'a' && strcmp (w, "it")) {
        strcpy (t, "the ");
    }
    strcat (t, w);
    if ((a == A_FAR || a == U_WHAT) && t[0] >= 'a' && t[0] <= 'z') {
        t[0] -= 'a' - 'A';                      /* (A sentence's start) */
    }
    p = said[a];
    if (!f) {
        sayf (p, t);
        return;
    }
    q = lb + strlen (lb);
    for (++p; *p; ++p) {
        if (*p == '%') {
            strcpy (q, t);
            q += strlen (q);
            ++p;
        } else {
            *q++ = *p;
        }
    }
    *q = 0;
}

/* A command refused, said back from its line (cl):
**   U_VERB    no verb of the game's: "I don't know how to bribe." (its first word)
**   U_YET     a verb the game has but doesn't do yet, or not to such a thing yet: "You can't touch the pot in this
**             game yet." (the line, if it names a thing; else its first word: "You can't eat in this game yet.")
**   U_CANT    a thing the verb can't be done to: "You can't take the stairs."
**   U_NOTE    nothing written on it: "There's nothing written on the stairs." (the line after its verb)
**   U_WHAT    nothing named, asked what: "Take what?" (the line) */
void __fastcall__ o_unknown (unsigned char f)
{
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
    o_say_no (f, d ? lb : 0);
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
