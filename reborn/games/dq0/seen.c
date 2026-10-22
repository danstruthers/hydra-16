/*
** seen.c - (overlay 7) more of what the character perceives, in words (words.c's overlay is full): the things it
** sees lying about (words.c's, after the place; then the denizens it sees, dens.c's, overlay 9).  As words.c's: only what it sees now (nothing behind it), and what's further off in less detail, the further
** it is and the weaker the character's sight (the referee's: struct sight's eye): a thing too far to make out only by
** its size ("something small").  Things alike told together ("On the floor ahead lie two iron pots.", "On the table
** lie two chisels."), not each the same words again; those on one square in one sentence ("At your feet lie a sack
** and a chest.": the user's, 2026-10-21).
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

static struct sight* S;

/* words.c's: the words so far said (a look's, streamed), and out (look.c's lb_said: none a look's said already this
** answer) */
#define lb_out  lb_said

#pragma register-vars (push, on)
static void __fastcall__ add (const char* s)
{
    register const char* q = s;
    register char* e = lbe;

    if (*q == ' ' && lb_stream && e != lb && e[-1] == '.') {
        lb_out ();
        e = lb;
    }
    while ((*e = *q) != 0) {
        ++e;
        ++q;
    }
    lbe = e;
}
#pragma register-vars (pop)

static void __fastcall__ addn (int v)
{
    char b[8];

    add (itoa (v, b, 10));
}

/* Squares from the character to square (x, y): the more of the two ways */
static unsigned char __fastcall__ apart (unsigned x, unsigned y)
{
    x -= S->x;
    y -= S->y;
    x = (int) x < 0 ? -x : x;
    y = (int) y < 0 ? -y : y;
    x = x > y ? x : y;
    return x > 255 ? 255 : x;
}

/* ---- The things and the denizens it sees (words.c's o_words, after the place), into lb */

static const char* const few[SEE_ITEMS + 1] = { "", "one", "two", "three", "four", "five", "six" };

/* A kind's name, more than one: "iron pots", "heaps of sand" (added to lb) */
static void __fastcall__ plural (const char* n)
{
    char b[20];
    unsigned char k, c;

    for (k = 0; n[k] && (n[k] != ' ' || n[k + 1] != 'o' || n[k + 2] != 'f' || n[k + 3] != ' '); ++k) {
    }                                           /* (Its first word's before " of ": "heaps of sand") */
    c = k ? n[k - 1] : 0;

    memcpy (b, n, k);
    if (c == 'y' && k > 1 && !strchr ("aeiou", n[k - 2])) {
        strcpy (b + k - 1, "ies");
    } else {
        strcpy (b + k, c == 's' || c == 'x' || c == 'z' || (c == 'h' && k > 1 && strchr ("cs", n[k - 2])) ? "es" : "s");
    }
    add (b);
    add (n + k);
}

/* Thing i too far off to make out (by its size, hid's high bits, and the character's sight) */
static unsigned char __fastcall__ afar (unsigned char i)
{
    return !(S->item[i].hid & 1) && apart (S->item[i].x, S->item[i].y) > 1 + S->eye / 2 + (S->item[i].hid >> 4);
}

/* Thing k one to be told with thing i (the same kind, where they lie, as easily made out, and the same in form: each
** one thing, "On the floor ahead lie two iron pots."; each some of a plural kind, "some potsherds"; or each a count of
** them, "5 rations": told once); both lying about with nothing on them, or both on the same surface ("On the table lie
** two chisels.", the user's, 2026-10-21); not named */
static unsigned char __fastcall__ alike (unsigned char i, unsigned char k)
{
    const struct sitem* p = &S->item[i];
    const struct sitem* q = &S->item[k];
    unsigned char j;

    if (i == k || ((p->hid ^ q->hid) & 2)) {
        return 0;
    }
    if (p->hid & 2) {
        for (j = i < k ? i : k; j < (i < k ? k : i); ++j) {
            if (!(S->item[j + 1].hid & 2)) {
                return 0;                       /* (On different surfaces) */
            }
        }
    } else if ((k + 1 < S->nitems && (q[1].hid & 2)) || (i + 1 < S->nitems && (p[1].hid & 2))) {
        return 0;                               /* (Things on it: told each) */
    }
    return (q->qty > 1) == (p->qty > 1) && q->plural == p->plural && q->name[0] != '#' && q->rel == p->rel &&
        q->hid == p->hid && !strcmp (q->name, p->name) && afar (i) == afar (k);
}

/* Thing i's alike (those after it it's told with): how many, their count in gq (a lot's); 0 if one before it is
** alike (told with that one) */
static unsigned gq;
static unsigned char __fastcall__ group (unsigned char i)
{
    unsigned char k, n = 1;

    gq = S->item[i].qty;
    for (k = 0; k < S->nitems; ++k) {           /* (Those alike told together: with the first of them) */
        if (alike (i, k)) {
            if (k < i) {
                return 0;
            }
            ++n;
            gq += gq > 1 ? S->item[k].qty : 0;  /* (Counted ones: their counts together) */
        }
    }
    return n;
}

/* Thing i and its alike, n of them, a count gq, in words: "two chisels", "a lit torch", "some bones", "5 rations";
** one too far off to make out, by its size, "something small" */
static void __fastcall__ one (unsigned char i, unsigned char n)
{
    const struct sitem* p = &S->item[i];
    unsigned char sz = p->hid >> 4, b = afar (i);

    if (n > 1 && !p->plural && gq <= 1) {
        add (few[n]);
        add (b ? (sz <= 2 ? " small things" : sz >= 5 ? " large things" : " things") : p->hid & 4 ? " lit " : " ");
        if (!b) {
            plural (p->name);
        }
        return;
    }
    if (b) {
        add (sz <= 2 ? "something small" : sz >= 5 ? "something large" : "something");
        return;
    }
    if (gq > 1) {
        addn (gq);
        add (" ");
    } else {
        add (p->plural ? "some " : !(p->hid & 4) && strchr ("aeiou", p->name[0]) ? "an " : "a ");
    }
    add (p->hid & 4 ? "lit " : "");
    add (p->name);
}

/* The things it sees lying about: "On the floor ahead lies a pick." "At your feet lie a sack and a chest." (those on
** one square told together, the user's: one too far off to make out each), "On the table lies a candle." (a surface's
** things after it, hid 2); "At your feet lies a lit torch." (hid 4); one too far off to make out (by its size, hid's
** high bits, and the character's sight), only by its size: "On the floor ahead lies something small." (and nothing
** on it told); none behind it */
static void items (void)
{
    unsigned char i, j, n, m, on = 0, b, skip = 0, told = 0, now;

    for (i = 0; i < S->nitems; ++i) {
        b = afar (i);
        if (S->item[i].hid & 2) {
            if (skip) {
                continue;                       /* (On a thing behind it, or too far to make out) */
            }
        } else {
            on = i;
            if ((skip = REL_BEHIND (S->item[i].rel)) != 0) {
                continue;
            }
            skip = b;
            if (told & 1 << i) {
                continue;                       /* (Told with those where it lies: what's on it after) */
            }
        }
        if (!(n = group (i))) {
            continue;
        }
        m = now = 0;
        if (S->item[i].hid & 2) {
            add (" On the ");
            add (S->item[on].name);
            add (" ");
        } else {
            for (j = i + 1; !b && j < S->nitems; ++j) {     /* (The others on its square: with it) */
                if (!(S->item[j].hid & 2) && S->item[j].x == S->item[i].x && S->item[j].y == S->item[i].y &&
                    !((S->item[j].hid ^ S->item[i].hid) & 1) && !afar (j) && group (j)) {
                    now |= 1 << j;
                    ++m;
                }
            }
            told |= now;
            if (lie_words (i)) {
                add (where_w);                  /* (In a corner, against a wall: part 8's) */
            } else {
                add (S->item[i].hid & 1 ? (S->item[i].rel == 0xFF ? " Hidden at your feet " : " Hidden on the floor ") :
                    S->item[i].rel == 0xFF ? " At your feet " : " On the floor ");
                if (S->item[i].rel != 0xFF) {
                    add (relname[S->item[i].rel & 7]);
                    add (" ");
                }
            }
        }
        group (i);
        add (m || n > 1 || S->item[i].plural || gq > 1 ? "lie " : "lies ");
        one (i, n);
        for (j = i + 1; m; ++j) {
            if (now & 1 << j) {
                add (--m ? ", " : " and ");
                one (j, group (j));
            }
        }
        add (".");
    }
}

/* The things and the denizens it sees, added to lb (words.c's, from look.c's copy of the hub's record) */
void o_seen_words (void)
{
    S = &sight_copy;
    if (!(S->first & SIGHT_HURRY)) {                            /* (A run's or a sprint's look: not the things about) */
        items ();
    }
    den_words ();                               /* (The denizens it sees: overlay 9's, dens.c) */
}
