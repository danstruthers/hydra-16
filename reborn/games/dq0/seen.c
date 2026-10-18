/*
** seen.c - (overlay 7) more of what the character perceives, in words (words.c's overlay is full): a ceiling at a
** glance; the things it sees lying about (words.c's, after the place; then the denizens it sees, dens.c's, overlay
** 9).  As words.c's: only what it sees now (nothing behind it), and what's further off in less detail, the further
** it is and the weaker the character's sight (the referee's: struct sight's eye): a thing too far to make out only by
** its size ("something small").  Things alike told together ("On the floor ahead lie two iron pots."), not each
** the same words again.
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

/* words.c's: the words so far said (a look's, streamed), and out */
static void lb_out (void)
{
    if (lbn) {
        say (lb);
    } else {
        msg (lb);
    }
    lbn += lbe - lb;
    *(lbe = lb) = 0;
    t_flush ();
}

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

/* ---- The ceiling in general words (the user's: a high ceiling, a low, rough one), as it's seen at a glance: its kind,
** and how high it is (low, under 8 feet; high, over 12; lofty, over 20); words.c's, added to lb (its record
** sight_copy) */
void o_ceiling_glance (void)
{
#define S sight_copy
    static const char* const and[4] = { " and low.", ".", " and high.", " and very high." };
    static const char* const over[4] = { " low overhead.", " overhead.", " high overhead.", " far overhead." };
    unsigned char k = S.ceil != C_ROUGH && S.ceil != C_SLOPED && S.cwall >= S.cmid ? C_FLAT : S.ceil, h;

    h = S.cmid < 8 ? 0 : S.cmid <= 12 ? 1 : S.cmid <= 20 ? 2 : 3;
    add (k == C_FLAT ? " The ceiling is flat" : k == C_BARREL ? " A barrel vault arches" : k == C_DOME ?
         " The ceiling is a dome," : k == C_CATHEDRAL ? " A pointed vault rises" : k == C_SLOPED ?
         " The ceiling slopes" : " The ceiling is rough stone");
    add (k == C_SLOPED ? (S.cmid >= S.cwall + 6 ? " steeply, high on one side and low on the other." :
         ", higher on one side.") : k == C_FLAT || k == C_ROUGH ? and[h] : over[h]);
#undef S
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

/* Thing k one to be told with thing i (both lying about, nothing on them, one of their kind, not named; the same
** kind, where they lie, as easily made out): "On the floor ahead lie two iron pots." */
static unsigned char __fastcall__ alike (unsigned char i, unsigned char k)
{
    const struct sitem* p = &S->item[i];
    const struct sitem* q = &S->item[k];

    return i != k && !(q->hid & 2) && !(p->hid & 2) && (k + 1 >= S->nitems || !(q[1].hid & 2)) &&
        (i + 1 >= S->nitems || !(p[1].hid & 2)) && q->qty <= 1 && !q->plural && q->name[0] != '#' &&
        q->rel == p->rel && q->hid == p->hid && p->qty <= 1 && !p->plural && !strcmp (q->name, p->name) &&
        afar (i) == afar (k);
}

/* The things it sees lying about: "On the floor ahead lies a pick." "At your feet lie some bones." "On the table
** lies a candle." (a surface's things after it, hid 2); "At your feet lies a lit torch." (hid 4); one too far off to
** make out (by its size, hid's high bits, and the character's sight), only by its size: "On the floor ahead lies
** something small." (and nothing on it told); none behind it */
static void items (void)
{
    unsigned char i, k, n, on = 0, sz, b, skip = 0;

    for (i = 0; i < S->nitems; ++i) {
        sz = S->item[i].hid >> 4;
        b = afar (i);
        n = 1;
        if (S->item[i].hid & 2) {
            if (skip) {
                continue;                       /* (On a thing behind it, or too far to make out) */
            }
            add (" On the ");
            add (S->item[on].name);
            add (" ");
        } else {
            on = i;
            if ((skip = REL_BEHIND (S->item[i].rel)) != 0) {
                continue;
            }
            skip = b;
            for (k = 0; k < S->nitems; ++k) {   /* (Those alike told together: with the first of them) */
                if (alike (i, k)) {
                    if (k < i) {
                        break;
                    }
                    ++n;
                }
            }
            if (k < S->nitems) {
                continue;
            }
            add (S->item[i].hid & 1 ? (S->item[i].rel == 0xFF ? " Hidden at your feet " : " Hidden on the floor ") :
                S->item[i].rel == 0xFF ? " At your feet " : " On the floor ");
            if (S->item[i].rel != 0xFF) {
                add (relname[S->item[i].rel & 7]);
                add (" ");
            }
        }
        if (n > 1) {
            add ("lie ");
            add (few[n]);
            add (b ? (sz <= 2 ? " small things." : sz >= 5 ? " large things." : " things.") :
                S->item[i].hid & 4 ? " lit " : " ");
            if (!b) {
                plural (S->item[i].name);
                add (".");
            }
            continue;
        }
        if (b) {
            add (sz <= 2 ? "lies something small." : sz >= 5 ? "lies something large." : "lies something.");
            continue;
        }
        add (S->item[i].plural || S->item[i].qty > 1 ? "lie " : "lies ");
        if (S->item[i].qty > 1) {
            addn (S->item[i].qty);
            add (" ");
        } else {
            add (S->item[i].plural ? "some " : !(S->item[i].hid & 4) && strchr ("aeiou", S->item[i].name[0]) ? "an " :
                "a ");
        }
        add (S->item[i].hid & 4 ? "lit " : "");
        add (S->item[i].name);
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
