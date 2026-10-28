/*
** dens.c - (overlay 9) the denizens the character sees, in words (seen.c's overlay is full): a look's (seen.c's,
** after the things it sees), and as they stir (the world's V_STIR); and those it hears.  As words.c's: only what it
** sees now (nothing behind it, but an ally), and one too far off to make out (by the character's sight, struct
** sight's eye) only as something there.  Those alike (the same kind, where they lie, as far, doing the same) told
** together: "Two giant rats are ahead to your right, 10 feet away."  A stir tells only what's changed since they were
** told (a look's, or a stir's): one come into sight, or doing otherwise, or moved; one gone from it; nothing (lb "")
** if what it sees of them is as it was told.  What was told is kept in its overlay's bank (its data there).
*/

#pragma code-name ("OV9CODE")
#pragma rodata-name ("OV9RODATA")
#pragma data-name ("OV9RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};
static const char* const count[SEE_DENS + 1] = { "", "One", "Two", "Three", "Four" };
static const char* const few[SEE_DENS + 1] = { "", "one", "two", "three", "four" };
static const char* const doing[9] = {
    " feet away.", " feet away, watching you.", " feet away, coming towards you.", " feet away, keeping its distance.",
    " feet away, following you.", " feet away, waiting.", " feet away, fighting you.", " feet away, fleeing.",
    " feet away, lying senseless."
};

/* The denizens as they're told, alike ones together: each the way it lies (REL8 as the character faces), how far
** (squares), what it's doing (DO_*), its name (as it lies there; all 0: too far to make out), how many */
struct told { unsigned char rel, d, doing; char name[12]; unsigned char gear, n; };
#define ALIKE       16                          /* (Its bytes that make it alike: all but n) */
static struct told now_[SEE_DENS] = { { 0 } };  /* As they are now (group's), */
static unsigned char nnow = 0, heard = 0;       /*   so many; one heard close behind */
static struct told was[SEE_DENS] = { { 0 } };   /* As they were told last, */
static unsigned char nwas = 0, heard_was = 0;
static unsigned char now_m[SEE_DENS] = { 0 }, was_m[SEE_DENS] = { 0 };   /* (A stir's: told the same, now and then) */
#define S           sight_copy

/* The words so far said (a look's, streamed), and out: lbsaid.s's, as words.c's are (none a look's said already this
** answer: the sentences before a denizen's, the place's, kept as said too) */
#define lb_out  lb_said

static void __fastcall__ add (const char* s)
{
    char* e = lbe;

    if (*s == ' ' && lb_stream && e != lb && e[-1] == '.') {
        lb_out ();
        e = lb;
    }
    while ((*e = *s) != 0) {
        ++e;
        ++s;
    }
    lbe = e;
}

static void __fastcall__ addn (int v)
{
    char b[8];

    add (itoa (v, b, 10));
}

/* Squares from the character to square (x, y): the more of the two ways */
static unsigned char __fastcall__ apart (unsigned x, unsigned y)
{
    x -= S.x;
    y -= S.y;
    x = (int) x < 0 ? -x : x;
    y = (int) y < 0 ? -y : y;
    x = x > y ? x : y;
    return x > 255 ? 255 : x;
}

/* Two of them (pa, pb) differ, in their first n bytes */
static const char* pa = 0;
static const char* pb = 0;
static unsigned char __fastcall__ differ (unsigned char n)
{
    do {
        if (*pa++ != *pb++) {
            return 1;
        }
    } while (--n);
    return 0;
}

/* A kind's name, more than one: "giant rats", "cave spiders" (added to lb) */
static void __fastcall__ plural (const char* n)
{
    char b[16];
    unsigned char k = strlen (n), c = k ? n[k - 1] : 0;

    strcpy (b, n);
    if (c == 'y' && k > 1 && !strchr ("aeiou", n[k - 2])) {
        strcpy (b + k - 1, "ies");
    } else {
        strcat (b, c == 's' || c == 'x' || c == 'z' || (c == 'h' && k > 1 && strchr ("cs", n[k - 2])) ? "es" : "s");
    }
    add (b);
}

/* The denizens seen, as they're told (none behind the character but an ally; one close behind it heard), alike ones
** together: into now_ */
static void group (void)
{
    static struct told t = { 0 };
    unsigned char i, k;
    struct told* p;
    char* n;

    nnow = heard = 0;
    for (i = 0; i < S.ndens; ++i) {
        n = S.den[i].name;
        t.rel = REL8 (bearing (S.den[i].x - S.x, S.den[i].y - S.y), pc_face);
        t.d = apart (S.den[i].x, S.den[i].y);
        t.doing = S.den[i].doing % 9;
        t.n = 1;
        if (REL_BEHIND (t.rel) && n[0] >= 'a') {
            heard |= t.d <= 2;
            continue;
        }
        memset (t.name, 0, sizeof t.name);      /* (Too far off to make out: no name) */
        if (n[0] < 'a' || t.d <= 3 + S.eye) {
            strncpy (t.name, n, sizeof t.name - 1);
        }
        t.gear = t.d <= 1 + S.eye ? S.den[i].gear & (AG_ARMED | AG_ARMOUR) : 0;   /* (Its things at a look, made out
                                                **   nearer than its kind: "armed and armoured", HOUSE.md's three
                                                **   levels, 2026-10-27) */
        for (k = 0, p = now_; k < nnow; ++k, ++p) {     /* (One alike told already: with it) */
            pa = (char*) p;
            pb = (char*) &t;
            if (!differ (ALIKE)) {
                ++p->n;
                break;
            }
        }
        if (k == nnow) {
            *p = t;
            ++nnow;
        }
    }
}

/* Group t named (the: as one told already, "The giant rat"): "A giant rat", "Two giant rats", "Something", "Two
** things", an ally's name ("Grub") */
static void __fastcall__ named (const struct told* t, unsigned char the)
{
    if (t->name[0] && t->name[0] < 'a') {
        add (" ");
        add (t->name);
    } else if (t->n > 1) {
        add (the ? " The " : " ");
        add (the ? few[t->n] : count[t->n]);
        add (" ");
        if (t->name[0]) {
            plural (t->name);
        } else {
            add ("things");
        }
    } else if (!t->name[0]) {
        add (" Something");
    } else {
        add (the ? " The " : strchr ("aeiou", t->name[0]) ? " An " : " A ");
        add (t->name);
    }
}

/* Group t told: "A giant rat is ahead to your left, 15 feet away, watching you."; "Two giant rats are ..."; an ally
** named: "Grub is behind you, 5 feet away, following you."; "Something is ahead, 40 feet away." */
static void __fastcall__ tell_one (const struct told* t)
{
    unsigned char many = t->n > 1;

    named (t, 0);
    if (t->gear) {
        add (t->gear == AG_ARMED ? ", armed," : t->gear == AG_ARMOUR ? ", armoured," : ", armed and armoured,");
    }
    add (many ? " are " : " is ");
    add (relname[t->rel & 7]);
    add (", ");
    addn (SQ_FT * t->d);
    add (many && t->doing == DO_SHY ? " feet away, keeping their distance." : doing[t->doing]);
}

/* What was told kept: now_'s, its first n */
static void __fastcall__ keep (unsigned char n)
{
    memcpy (was, now_, sizeof was);
    nwas = n;
    heard_was = heard;
}

/* A look's denizens, added to lb (seen.c's, after the things it sees), as there's room in it (a look's said so far
** counted) */
void o_den_words (void)
{
    unsigned char i;

    group ();
    for (i = 0; i < nnow && lbn + (lbe - lb) < sizeof lb - 80; ++i) {
        tell_one (&now_[i]);
    }
    add (heard ? " You hear something close behind you." : "");
    keep (i);
}

/* "Something stirs.", once, before a stir's first words */
static unsigned char stirs = 0;
static void stirred (void)
{
    if (!stirs++) {
        add ("Something stirs.");
    }
}

/* The denizens it sees, as one comes into sight, goes from it, or does something else (the world's V_STIR: into
** lb): what's changed since they were told; "" if nothing has.  "Something stirs. A giant rat is ahead, 10 feet away,
** coming towards you."; "Something stirs. The giant rat to your left is gone from sight."; "Nothing stirs in sight
** now." */
void o_stir_words (void)
{
    unsigned char i, k;
    struct told* w;

    *(lbe = lb) = 0;
    lbn = 0;
    stirs = 0;
    group ();
    memset (now_m, 0, sizeof now_m);
    memset (was_m, 0, sizeof was_m);
    for (i = 0; i < nnow; ++i) {                /* (Those told the same before) */
        for (k = 0; k < nwas; ++k) {
            pa = (char*) &now_[i];
            pb = (char*) &was[k];
            if (!was_m[k] && !differ (sizeof was[0])) {
                now_m[i] = was_m[k] = 1;
                break;
            }
        }
    }
    if (!nnow && nwas) {
        add ("Nothing stirs in sight now.");
    } else {
        for (i = 0; i < nnow; ++i) {            /* (Come into sight, or doing otherwise, or moved, or more or fewer of
                                                **   them: each told, and one told before taken as what it was, the
                                                **   same but how many, else the same kind) */
            if (now_m[i]) {
                continue;
            }
            stirred ();
            tell_one (&now_[i]);
            for (k = 0; k < nwas; ++k) {
                pa = (char*) &now_[i];
                pb = (char*) &was[k];
                if (!was_m[k] && !differ (ALIKE)) {
                    break;
                }
            }
            for (k = k < nwas ? k : 0; k < nwas && (was_m[k] || strcmp (now_[i].name, was[k].name)); ++k) {
            }
            if (k < nwas) {
                was_m[k] = 1;
            }
        }
        for (k = 0, w = was; k < nwas; ++k, ++w) {  /* (Gone from sight: those told before not taken; alike the
                                                **   same way told together, "The two giant rats to your right") */
            if (was_m[k]) {
                continue;
            }
            for (i = k + 1; i < nwas; ++i) {
                if (!was_m[i] && was[i].rel == w->rel && !strcmp (was[i].name, w->name)) {
                    w->n += was[i].n;           /* (was's own: now_ kept over it after) */
                    was_m[i] = 1;
                }
            }
            stirred ();
            named (w, 1);                       /* ("The giant rat to your left is gone from sight."; "Grub is ...") */
            if (!w->name[0] || w->name[0] >= 'a') {
                add (" ");
                add (relname[w->rel & 7]);
            }
            add (w->n > 1 ? " are gone from sight." : " is gone from sight.");
        }
    }
    if (heard && !heard_was) {
        stirred ();
        add (" You hear something close behind you.");
    }
    keep (nnow);
}

/* A denizen heard, not seen (main.c's heard: one of the denizens' events), into lb, in its sound's words (the sound
** task's, SOUND.md: heard_what) if it has them: "You hear a scurrying to your right.", "You hear a muffled squeak
** behind you." (what | 0x80: walls or a door between); else "You hear something moving behind you to your left." */
void __fastcall__ o_heard_words (unsigned char what, unsigned char rel)
{
    if (heard_what[0]) {
        strcpy (lb, "You hear ");
        if (what & 0x80) {
            if (heard_what[0] == 'a' && heard_what[1] == ' ') {
                strcat (lb, "a muffled ");      /* ("a muffled squeak") */
                strcat (lb, heard_what + 2);
            } else {
                strcat (lb, "muffled ");
                strcat (lb, heard_what);
            }
        } else {
            strcat (lb, heard_what);
        }
        strcat (lb, " ");
    } else {
        strcpy (lb, (what & 0x7F) == DE_NOISE ? "You hear a noise " : "You hear something moving ");
    }
    strcat (lb, relname[rel & 7]);
    strcat (lb, ".");
}
