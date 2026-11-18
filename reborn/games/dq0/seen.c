/*
** seen.c - (overlay 12, after part 6's image: overlay 7 full) more of what the character perceives, in words
** (words.c's overlay is full): the things it sees lying about (words.c's, after the place; then the denizens it sees,
** dens.c's, overlay 9).  As words.c's: only what it sees now (nothing behind it), and what's further off in less
** detail, the further it is and the weaker the character's sight (the referee's: struct sight's eye): a thing too far
** to make out only by its size ("something small").  Things alike told together ("On the floor ahead lie two iron
** pots.", "On the table lie two chisels."), not each the same words again; those on one square in one sentence ("At
** your feet lie a sack and a chest.": the user's, 2026-10-21).
*/

#pragma code-name ("OV12CODE")
#pragma rodata-name ("OV12RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

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

/* Thing i and its alike, n of them, a count gq: list entry e, worded by said.txt's LI.thing ("two chisels", "a lit
** torch", "some bones", "5 rations"; one too far off to make out, by its size, "something small"), its capital (letters.c's,
** the user's designators) given as it's put in the list, in the order it was */
static void __fastcall__ one (struct li* e, unsigned char i, unsigned char n)
{
    const struct sitem* p = &S->item[i];
    unsigned char sz = p->hid >> 4, b = afar (i);

    strcpy (e->cap, cap_of (i));
    e->name = p->name;
    e->b = n;
    e->w2 = gq;
    e->w1 = LI_THING << 8 | (n > 1 && !p->plural && gq <= 1 ? b : b ? 2 : gq > 1 ? 3 : 4) |
        (sz <= 2 ? 8 : sz >= 5 ? 16 : 0) | (p->hid & 4 ? 32 : 0) | (p->plural ? 64 : 0);
}

/* The things it sees lying about: "On the floor ahead lies a pick." "At your feet lie a sack and a chest." (those on
** one square told together, the user's: one too far off to make out each), "On the table lies a candle." (a surface's
** things after it, hid 2); "At your feet lies a lit torch." (hid 4); one too far off to make out (by its size, hid's
** high bits, and the character's sight), only by its size: "On the floor ahead lies something small." (and nothing
** on it told); none behind it; a sconce on the wall, what's in it with it: "On the wall to your left is a sconce, a
** lit torch in it." (part 8's words for where it is, its verb with them: lie_words's 2).  Each sentence a UK_LIST
** row, its things a list (said.txt's LIST and LI rows: the look's lists, the user's, 2026-11-16) */
static void items (void)
{
    unsigned char i, j, n, m, on = 0, b, skip = 0, told = 0, now, w = 0, k;
    struct li v[SEE_ITEMS];                     /* (On the C stack: the list, said_li) */
    struct msg f;

    f.kind = UK_LIST;
    f.who = 0;
    said_li = v;
    for (i = 0; i < S->nitems; ++i) {
        b = afar (i);
        if (S->item[i].hid & 2) {
            if (skip) {
                continue;                       /* (On a thing behind it, or too far to make out) */
            }
        } else {
            on = i;
            w = 0;
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
        f.w2 = S->item[i].rel & 7;
        if (S->item[i].hid & 2) {
            if (w) {
                --lbe;                          /* (In a sconce: its sentence's, its full stop still in lb) */
                k = LF_IN;
            } else {
                add (" \002");                  /* (A thing's sentence: \002, not said of the place, lbsaid.s's: the
                                                **   same thing seen from two places told once an answer) */
                said_s[0] = S->item[on].name;
                f.w2 = 5 << 4;
                k = LF_LIE;
            }
        } else if ((k = lie_words (i)) == 2) {
            add (" \002");
            said_s[0] = where_w + 1;            /* (A sconce on the wall, the verb with it: part 8's) */
            w = 1;
            k = LF_WHERE;
        } else {
            for (j = i + 1; !b && j < S->nitems; ++j) {     /* (The others on its square: with it) */
                if (!(S->item[j].hid & 2) && S->item[j].x == S->item[i].x && S->item[j].y == S->item[i].y &&
                    !((S->item[j].hid ^ S->item[i].hid) & 1) && !afar (j) && strcmp (S->item[j].name, "sconce") &&
                    group (j)) {                /* (A sconce on the wall: its own sentence) */
                    now |= 1 << j;
                    ++m;
                }
            }
            told |= now;
            add (" \002");
            if (k) {
                said_s[0] = where_w + 1;        /* (In a corner, against a wall: part 8's) */
                f.w2 = 4 << 4;
            } else {
                f.w2 |= (S->item[i].hid & 8 ? 6 : (S->item[i].hid & 1) << 1 | (S->item[i].rel != 0xFF)) << 4;  /* (At its
                                                **   feet, on the floor its way; hidden; in a pit its way, the senses' hid
                                                **   8: the user's, 2026-11-18, H10) */
            }
            k = LF_LIE;
        }
        group (i);
        one (v, i, n);
        f.w2 |= m || ((v->w1 & 7) != 2 && (n > 1 || S->item[i].plural || gq > 1)) ? 8 : 0;   /* ("lie", "lies": one
                                                **   too far off to make out "something small", whatever it is, one;
                                                **   "lie (F) something small" of a plural kind's till 2026-11-17) */
        f.w1 = k << 8;
        for (f.a = 1, j = i + 1; m; ++j) {
            if (now & 1 << j) {
                --m;
                one (v + f.a++, j, group (j));
            }
        }
        list_words (&f);
    }
}

/* The things and the denizens it sees, added to lb (words.c's, from look.c's copy of the hub's record); and, mapping
** as it goes with the things to draw with not handy (the world's pc_chart, CH_PACKED: what they're in), told once
** (HOUSE.md's handy tools: "Your drawing things are in your backpack.") till they're handy, or it's not mapping */
static unsigned char packed;
void o_seen_words (void)
{
    char nm[sizeof CATALOGUE[0].name];
    unsigned char w, c;
    struct msg f;

    S = &sight_copy;
    if (!(S->first & SIGHT_HURRY)) {                            /* (A run's or a sprint's look: not the things about) */
        items ();
    }
    den_words ();                               /* (The denizens it sees: overlay 9's, dens.c) */
    hub_mem (0, &w, &HUB->watch, 1);
    hub_mem (0, &c, &HUB->pc_chartk, 1);
    if (!(w & WA_MAP) || !c) {
        packed = 0;
    } else if (!packed) {
        packed = 1;
        hub_mem (KIND_BANK, nm, CATALOGUE[c].name, sizeof nm);
        add (" ");
        said_s[0] = nm;
        f.kind = UK_LIST;
        f.who = 0;
        f.a = 0;
        f.w1 = LF_PACKED << 8;
        list_words (&f);                        /* (said.txt's LIST.packed) */
    }
}
