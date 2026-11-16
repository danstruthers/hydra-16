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

static unsigned char f;                         /* (say_no's: into lb, not said) */

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "actions.h"

/* Refusal a (U_*, A_*: dq0.h's) said, w its thing's name put in (0: "it"); | A_LB, into lb, not said (the sway's words, a
** blow's note: lb empty, every caller's).  w copied first, so it may be lb, or any of the main RAM's (not another
** overlay's: its bank's out).  Its words said.txt's NO rows, the UI's own kind (UK_NO), one wording a meaning: "You see
** no %s here.", "You have no %s.", "%s is out of reach: go up to it first." ... (refuse.c's said[] before, 2026-11-12) */
#pragma static-locals (push, off)               /* (Its event on the C stack, the main RAM: the words' overlay reads it) */
void __fastcall__ o_say_no (unsigned char a, const char* w)
{
    static char t[64];
    struct msg c;

    f = a & A_LB;
    a &= ~A_LB;
    if (!w) {
        w = "it";
    }
    if ((a == A_SEE_NO || a == A_HAVE_NO || a == A_NO_GODS) && (!*w || *w == '#')) {
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
    said_s[2] = strcpy (lb + 200, t);           /* (The main RAM's: the words' overlay in the window, not this one) */
    c.kind = UK_NO;
    c.who = 0;
    c.w1 = a << 8;
    said_q = f;
    outcome_words (&c);
    said_q = 0;
    if (f) {
        memmove (lb, lb + 1, strlen (lb));      /* (Its words alone, its row's new line not) */
    }
}
#pragma static-locals (pop)

/* The things' words (things_text, as tt copied them into lb) with the world's coded sentences worded (narration as
** data, stage 5, the user's, 2026-11-12: kinds.c's said[] the world's before): each code (items.h's SAY_AT, its SA_* |
** 0x80, the thing's name, SAY_SEP, the other's, SAY_SEP: things.c's say) worded by said.txt's SA rows where it stood,
** into lb (said_to, said_q: not said), the text after it read again from the hub after its words.  tt_sa the first
** sentence's code, if the text begins with one (s_hold_first's "held already": a code's, not the words').  A code past
** TT_AT cuts the text there (lb's room: a sentence's words, and its names kept at lb's end as it's worded) */
#define TT_AT           250
#pragma static-locals (push, off)
void __fastcall__ o_tt_words (const char* t)
{
    static char* q;
    static char* e;
    static char* y;
    static unsigned r;
    struct msg c;

    lb[sizeof lb - 1] = 0;
    tt_sa = lb[0] == SAY_AT ? lb[1] : 0;
    for (r = 0, q = lb; ; ) {
        for (e = q; *e && *e != SAY_AT && *e != SAY_AT2; ++e) {
        }                                       /* (SAY_AT, or SAY_AT2: a code past 127; not the library's strpbrk,
                                                **   the main RAM's 50 bytes) */
        if (!*e) {
            break;
        }
        r += e - q;                             /* (The code's place in the hub's text) */
        q = e;
        if (q > lb + TT_AT) {
            *q = 0;
            return;
        }
        e = strchr (q + 2, SAY_SEP);
        *e = 0;
        y = strchr (++e, SAY_SEP);
        *y = 0;
        r += ++y - q;                           /* (The hub's text's after it) */
        said_s[2] = strcpy (lb + sizeof lb - 40, q + 2);    /* (The thing's name: text; the other's: obj) */
        said_s[0] = strcpy (lb + sizeof lb - 20, e);
        c.kind = UK_SA;
        c.who = 0;
        c.w1 = (unsigned) ((q[1] & 0x7F) | (*q == SAY_AT2 ? 0x80 : 0)) << 8;
        said_to = q;
        said_q = 1;
        outcome_words (&c);
        said_q = 0;
        said_to = lb;
        q += strlen (q);
        hub_mem (UI_BANK, q, t + r, lb + sizeof lb - 1 - q);
    }
}
#pragma static-locals (pop)

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

