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

/* The denizens as they're told, alike ones together: each the way it lies (REL8 as the character faces), how far
** (squares), what it's doing (DO_*), its name (as it lies there; all 0: too far to make out), how many */
struct told { unsigned char rel, d, doing; char name[12]; unsigned char gear, n, folk; };
#define ALIKE       16                          /* (Its bytes that make it alike: all but n, and folk, its kind's: a
                                                **   person, "keeping their distance", the hub's, 2026-11-17) */
static struct told now_[SEE_DENS] = { { 0 } };  /* As they are now (group's), */
static unsigned char nnow = 0, heard = 0;       /*   so many; one heard close behind */
static struct told was[SEE_DENS] = { { 0 } };   /* As they were told last, */
static unsigned char nwas = 0, heard_was = 0;
static unsigned char now_m[SEE_DENS] = { 0 }, was_m[SEE_DENS] = { 0 };   /* (A stir's: told the same, now and then) */
static unsigned char gm[SEE_DENS] = { 0 };      /* (Those of each told now, a bit each: their capitals, letters.c's) */
static unsigned char gcap = 0;                  /* (The group told now: its capitals said, tell_one's) */
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

/* Denizen record r's actor a person (the hub's folk: one of the speaking peoples, "they", the user's, 2026-11-17), by the
** actor whose record it is */
static unsigned char __fastcall__ folk_of (unsigned char r)
{
    unsigned char k, b;

    for (k = 0; k < NACTORS; ++k) {
        hub_mem (0, &b, &HUB->actor[k].den, 1);
        if (b == r) {
            hub_mem (0, &b, &HUB->folk[k], 1);
            return b;
        }
    }
    return 0;
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
    memset (gm, 0, sizeof gm);
    for (i = 0; i < S.ndens; ++i) {
        n = S.den[i].name;
        t.rel = REL8 (bearing (S.den[i].x - S.x, S.den[i].y - S.y), pc_face);
        t.d = apart (S.den[i].x, S.den[i].y);
        t.doing = S.den[i].doing % (DO_LEISURE + NPAST);
        t.n = 1;
        t.folk = folk_of (S.den[i].den);
        if (REL_BEHIND (t.rel) && n[0] >= 'a') {
            heard |= t.d <= 2;
            continue;
        }
        memset (t.name, 0, sizeof t.name);      /* (Too far off to make out: no name) */
        if (n[0] < 'a' || t.d <= 3 + S.eye) {
            strncpy (t.name, n, sizeof t.name - 1);
        }
        t.gear = t.d <= 1 + S.eye ? S.den[i].gear & (AG_ARMED | AG_ARMOUR | AG_MAIL) : 0;   /* (Its things at a look, made out
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
        gm[k] |= 1 << i;
        if (k == nnow) {
            *p = t;
            ++nnow;
        }
    }
}

/* A sentence of a denizen's (said.txt's LIST rows: the look's lists, the user's, 2026-11-16): UK_LIST, its form and
** choice w1, its fields w2 and b, its list the one entry e (on the C stack: said.c's to read, the main RAM's), worded
** into lb at lbe */
static void __fastcall__ den_list (struct li* e, unsigned w1, unsigned w2, unsigned char b)
{
    struct msg f;

    f.kind = UK_LIST;
    f.who = 0;
    f.a = e != 0;
    f.b = b;
    f.w1 = w1;
    f.w2 = w2;
    said_li = e;
    list_words (&f);
}

/* Group t named (the: as one told already, "The giant rat"): "A giant rat", "Two giant rats", "Something", "Two
** things", an ally's name ("Grub"): list entry d, said.txt's LI.den */
struct named { struct li e; char nm[12]; };   /* (A denizen's entry, its name copied: said.c's to read, the main RAM's
                                                **   C stack, not this overlay's bank) */
static void __fastcall__ named (struct named* d, const struct told* t, unsigned char the)
{
    struct li* e = &d->e;

    add ("\002");                               /* (Its sentence said of nothing, not of the place, as a thing's:
                                                **   lbsaid.s's, a denizen told as it stirs, a walk stopped by it a
                                                **   step on, not told again in the look there) */
    add (" ");
    if (gcap) {
        den_caps (gcap);                        /* (Their capitals, "(A) ", "(A, B) ": letters.c's, the user's
                                                **   designators, given before the things') */
    }
    e->cap[0] = 0;
    e->name = strcpy (d->nm, t->name);
    e->b = t->n;
    e->w1 = LI_DEN << 8 | (t->name[0] && t->name[0] < 'a' ? 0 : t->n > 1 ? (t->name[0] ? 1 : 2) : !t->name[0] ? 3 : 4) |
        the << 7;
}

/* Group t told: "A giant rat is ahead to your left, 15 feet away, watching you."; "Two giant rats are ..."; an ally
** named: "Grub is behind you, 5 feet away, following you."; "Something is ahead, 40 feet away."; "A dwarf in mail,
** armed, is ..." (2026-10-29): said.txt's LIST.den */
static void __fastcall__ tell_one (const struct told* t)
{
    unsigned char many = t->n > 1, g = t->gear;
    struct named e;

    gcap = gm[t - now_];
    named (&e, t, 0);
    gcap = 0;
    den_list (&e.e, LF_DEN << 8 | (many && t->doing == DO_SHY ? DO_LEISURE + NPAST : t->doing),
        (t->rel & 7) | many << 3 | (g & AG_MAIL ? (g & AG_ARMED) << 8 | 1 << 10 : (g & 3) << 8) | (unsigned) t->folk << 11, t->d);
}

/* What was told kept: now_'s, its first n */
static void __fastcall__ keep (unsigned char n)
{
    memcpy (was, now_, sizeof was);
    nwas = n;
    heard_was = heard;
}

/* A look's denizens, added to lb (seen.c's, after the things it sees), as there's room in it (what's in it now: a
** look streamed has said the rest, lb_said; its whole length counted till 2026-11-06, the things' capitals left a
** long look's denizens out, dq0inhab's rats) */
void o_den_words (void)
{
    unsigned char i;

    group ();
    for (i = 0; i < nnow && lbe - lb < sizeof lb - 80; ++i) {
        tell_one (&now_[i]);
    }
    if (heard) {
        add (" ");
        den_list (0, LF_HEARD << 8, 0, 0);      /* (" You hear something close behind you.") */
    }
    keep (i);
}

/* "Something stirs.", once, before a stir's first words */
static unsigned char stirs = 0;
static void stirred (void)
{
    if (!stirs++) {
        den_list (0, LF_STIRS << 8, 0, 0);
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
    struct named e;

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
        den_list (0, LF_NOSTIR << 8, 0, 0);
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
            named (&e, w, 1);                   /* ("The giant rat to your left is gone from sight."; "Grub is ...") */
            den_list (&e.e, LF_GONE << 8, (w->rel & 7) | (w->n > 1) << 3 | (!w->name[0] || w->name[0] >= 'a') << 4, 0);
        }
    }
    if (heard && !heard_was) {
        stirred ();
        add (" ");
        den_list (0, LF_HEARD << 8, 0, 0);
    }
    keep (nnow);
}
