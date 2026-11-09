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

