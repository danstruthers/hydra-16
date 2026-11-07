/*
** caps.c - (overlay 23) the designators' capitals (the user's design, 2026-11-06, DECISIONS.md's: "capital letters for
** in-the-environment things (items and denizens, unique, up to two characters) with denizens numbered first"; hub.h's):
** a capital, A ... Z then AA ... AZ (the UI bank's cap: each one's thing's id, or a denizen's, CAP_DEN's; its kind; when
** it was last told), for a thing or a denizen seen, given as it's first told (the look's words: seen.c's things, cap_of;
** dens.c's denizens, den_caps, theirs given first, so those seen are A, B ...; the list of what's in sight, "around":
** o_around), kept while it's remembered, given up only as a new one's wanted and none is free, the one told longest ago
** (some ten minutes at least) that isn't in sight now; a thing put down has its old one again (its id the same).  A
** capital typed is letters.c's (o_letters_in: a thing's its kind's name and its id; a denizen's its name, the resolver's
** to find, o_des_thing).  And a denizen heard (o_heard_words: dens.c's, overlay 9's room).  Overlay 22's till
** 2026-11-06 (letters.c's), its room for the line's letters.
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"

#pragma static-locals (on)                      /* (Its locals static: the code the less; nothing here is reentered) */
#pragma code-name ("OV23CODE")
#pragma rodata-name ("OV23RODATA")
#pragma data-name ("OV23BSS")
#pragma bss-name ("OV23BSS")
#pragma local-strings (on)

static struct ucap capc[NLET];                  /* (The UI bank's cap, copied in) */
static unsigned char cseq, cmade;               /* (The look they're copied for, sight_copy's seq; 1, once copied) */
static unsigned sid[SEE_ITEMS + SEE_DENS];      /* (The things in sight: their ids, the denizens' seen after them, */
static unsigned char skind[SEE_ITEMS];          /*   the things' kinds) */
static unsigned char nsid;
static unsigned cnow;                           /* (The clock's stamp: its tenths over 1024) */
static char o[80];                              /* ("around"'s line) */
static char w[8];
static const char* const rel[8] = {             /* (The ways, as the character faces) */
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};
static unsigned char __fastcall__ cap_give (unsigned id, unsigned char what, unsigned char kind);

/* Letter j's text (a capital: A ... Z, AA ... AZ) put at b: its end (letters.c's let_text) */
static char* __fastcall__ let_text (char* b, unsigned char j)
{
    if (j >= NLET / 2) {
        *b++ = 'A';
        j -= NLET / 2;
    }
    *b++ = 'A' + j;
    *b = 0;
    return b;
}

/* The UI bank's bytes copied in (from_ui), or this bank's out to it (to_ui), by way of the C stack (the two share the
** window), a piece at a time (letters.c's from_hub, to_hub) */
#pragma static-locals (push, off)
static void __fastcall__ ui_copy (unsigned char out, void* to, const void* from, unsigned n)
{
    char t[64];
    unsigned char k;

    while (n) {
        k = n > sizeof t ? sizeof t : n;
        if (out) {
            memcpy (t, from, k);
            hub_mem (UI_BANK, to, t, k);
        } else {
            hub_mem (UI_BANK, t, from, k);
            memcpy (to, t, k);
        }
        to = (char*) to + k;
        from = (const char*) from + k;
        n -= k;
    }
}
#pragma static-locals (pop)
#define to_ui(t, f, n)      ui_copy (1, (t), (f), (n))
#define from_ui(t, f, n)    ui_copy (0, (t), (f), (n))

/* Denizen q of those in sight (sight_copy's) seen, not heard only: not behind the character, but an ally (dens.c's) */
static unsigned char __fastcall__ den_eyed (const struct sden* q)
{
    return !REL_BEHIND (REL8 (bearing (q->x - sight_copy.x, q->y - sight_copy.y), pc_face)) || q->name[0] < 'a';
}

/* The things in sight (sight_copy's, as the look's words or the list of them have it): their ids and kinds, and after
** them the denizens seen (their ids CAP_DEN's), into sid; the clock's stamp; the capitals copied in; the denizens their
** capitals first (the user's "denizens numbered first"): once a look */
#pragma static-locals (push, off)
static void caps_load (void)
{
    struct item it;
    unsigned long t;
    const struct sden* q;
    unsigned char k;

    if (cmade && cseq == sight_copy.seq) {
        return;
    }
    cmade = 1;
    cseq = sight_copy.seq;
    hub_mem (0, &t, &HUB->now, sizeof t);
    cnow = (unsigned) (t >> 10);
    from_ui (capc, UIREC->cap, sizeof capc);
    for (k = 0; k < sight_copy.nitems; ++k) {
        hub_mem (ITEM_BANK (sight_copy.item[k].n), &it, ITEM_AT (sight_copy.item[k].n), sizeof it);
        sid[k] = it.id;
        skind[k] = it.kind;
    }
    for (nsid = k, q = sight_copy.den; q < sight_copy.den + sight_copy.ndens; ++q) {
        if (den_eyed (q)) {
            sid[nsid++] = CAP_DEN (q->den);
        }
    }
    while (k < nsid) {
        cap_give (sid[k++], CW_DEN, 0);
    }
}
#pragma static-locals (pop)

/* Thing id's capital (what: CW_THING, kind its kind; a denizen's, CW_DEN, its id CAP_DEN's): its own, told again now;
** else a new one, of A ... Z the first free, else the one told longest ago (6 stamps at least, some ten minutes) that
** isn't in sight, then of AA ... AZ the same way (the UI bank's written).  Its place, or NLET (none) */
static unsigned char __fastcall__ cap_give (unsigned id, unsigned char what, unsigned char kind)
{
    static struct ucap* c;
    static struct ucap* e;
    static struct ucap* b;
    static unsigned o, d;
    static unsigned char k;

    if (!id) {
        return NLET;
    }
    for (c = capc; c < capc + NLET && (c->id != id || c->what != what); ++c) {
    }
    for (e = capc; c == capc + NLET && e < capc + NLET; e += NLET / 2) {
        for (b = capc + NLET, o = 5, c = e; c < e + NLET / 2 && c->id; ++c) {
            for (k = 0; k < nsid && sid[k] != c->id; ++k) {
            }
            if (k == nsid && (d = cnow - c->t) > o) {
                o = d;                          /* (Told the longest ago, a while ago, not in sight) */
                b = c;
            }
        }
        if (c == e + NLET / 2) {
            c = b;
        }
    }
    if (c < capc + NLET && (c->id != id || c->t != cnow)) {
        c->id = id;
        c->what = what;
        c->kind = kind;
        c->t = cnow;
        to_ui (UIREC->cap + (c - capc), c, sizeof *c);
    }
    return c - capc;
}

/* Capital j's text, "(C) ", at b ("" none) */
static void __fastcall__ cap_text (char* b, unsigned char j)
{
    *b = 0;
    if (j < NLET) {
        *b = '(';
        strcat (let_text (b + 1, j), ") ");
    }
}

/* Thing i of those in sight (sight_copy's) its capital, "(C) ", into capb (the main RAM's): the look's words', seen.c's
** one (overlay 12's); "" none */
const char* __fastcall__ o_cap_of (unsigned char i)
{
    caps_load ();
    cap_text (capb, i < sight_copy.nitems ? cap_give (sid[i], CW_THING, skind[i]) : NLET);
    return capb;
}

/* The denizens in sight of mask m (a bit each, sight_copy's: those told together, dens.c's) their capitals, "(A) ", "(A,
** B) ", added to the look's words (lb, at lbe) */
void __fastcall__ o_den_caps (unsigned char m)
{
    static const struct sden* q;
    static char* p;
    static unsigned char j;

    caps_load ();
    for (p = lbe, q = sight_copy.den; m; m >>= 1, ++q) {
        if ((m & 1) && (j = cap_give (CAP_DEN (q->den), CW_DEN, 0)) < NLET) {
            if (p == lbe) {
                *p++ = '(';
            } else {
                *p++ = ',';
                *p++ = ' ';
            }
            p = let_text (p, j);
        }
    }
    if (p != lbe) {
        *p++ = ')';
        *p++ = ' ';
        lbe = p;
    }
    *p = 0;
}

/* Square (x, y) from the character: how many squares off (the more of the two ways), into sd, and its value */
static unsigned sd;
static unsigned __fastcall__ span (unsigned x, unsigned y)
{
    static unsigned ay;

    sd = x - sight_copy.x;
    ay = y - sight_copy.y;
    sd = (int) sd < 0 ? -sd : sd;
    ay = (int) ay < 0 ? -ay : ay;
    return sd = sd > ay ? sd : ay;
}

/* Where something in sight lies (span's, r its way, REL8's, 0xFF its feet; 0xFE none) added to o: "at your feet",
** "ahead to your left, 15 feet away"; and o said, the first line a new answer's ("around"'s) */
static unsigned char any;
static void __fastcall__ where_put (unsigned char r)
{
    if (r == 0xFF) {
        strcat (o, "at your feet");
    } else if (r != 0xFE) {
        strcat (o, rel[r & 7]);
        if (sd > 1) {
            strcat (strcat (strcat (o, ", "), utoa (5 * sd, w, 10)), " feet away");
        }
    }
    if (any++) {
        say ("\n");
        say (o);
    } else {
        msg (o);
    }
}

/* What's in sight listed, one a line, its capital and where it is ("around", the user's "a list of all in sight"): the
** denizens seen first, "(A) a giant rat, ahead to your left, 15 feet away", "(B) Grub, behind you, 5 feet away", one too
** far off to make out "(C) something, ahead, 40 feet away"; then the things, "(D) a torch (lit), at your feet", "(E) an
** iron pot, ahead to your left, 15 feet away", "(F) a candle, on the table"; one too far off to make out by its size,
** "(G) something small, ahead, 40 feet away" */
void o_around (void)
{
    static const struct sitem* q;
    static const struct sden* n;
    static unsigned char i, k, sz;

    sight_get ();
    caps_load ();
    for (any = 0, n = sight_copy.den; n < sight_copy.den + sight_copy.ndens; ++n) {
        if (!den_eyed (n)) {
            continue;
        }
        cap_text (o, cap_give (CAP_DEN (n->den), CW_DEN, 0));
        if (n->name[0] < 'a') {
            strcat (o, n->name);                /* (An ally, by its name) */
        } else if (span (n->x, n->y) > 3 + sight_copy.eye) {
            strcat (o, "something");            /* (Too far off to make out: dens.c's) */
        } else {
            strcat (strcat (o, strchr ("aeiou", n->name[0]) ? "an " : "a "), n->name);
        }
        strcat (o, ", ");
        span (n->x, n->y);
        where_put (REL8 (bearing (n->x - sight_copy.x, n->y - sight_copy.y), pc_face));
    }
    for (i = 0, q = sight_copy.item; i < sight_copy.nitems; ++i, ++q) {
        cap_text (o, cap_give (sid[i], CW_THING, skind[i]));
        sz = q->hid >> 4;
        if (!(q->hid & 1) && span (q->x, q->y) > 1 + sight_copy.eye / 2 + sz) {   /* (Too far off to make out: by its
                                                **   size, seen.c's) */
            strcat (o, sz <= 2 ? "something small" : sz >= 5 ? "something large" : "something");
        } else {
            strcat (o, q->qty > 1 ? utoa (q->qty, w, 10) : q->plural ? "some" : strchr ("aeiou", q->name[0]) ? "an" : "a");
            strcat (strcat (o, " "), q->name);
            if (q->hid & 4) {
                strcat (o, " (lit)");
            }
        }
        strcat (o, ", ");
        if (q->hid & 2) {                       /* (On the thing before it) */
            for (k = i; k && (sight_copy.item[k].hid & 2); --k) {
            }
            strcat (strcat (o, "on the "), sight_copy.item[k].name);
            where_put (0xFE);
            continue;
        }
        strcat (o, q->hid & 1 ? "hidden " : "");
        span (q->x, q->y);
        where_put (q->rel);
    }
    if (!any) {
        msg ("You see nothing lying about.");
    }
}

/* (Overlay 9's till 2026-11-06: its room for the denizens' capitals) A denizen heard, not seen (main.c's heard: one of the denizens' events), into lb, in its sound's words (the sound
** task's, SOUND.md: heard_what) if it has them: "You hear a scurrying to your right.", "You hear a muffled squeak
** behind you." (what | 0x80: walls or a door between); else "You hear something moving behind you to your left." */
void __fastcall__ o_heard_words (unsigned char what, unsigned char rel_)
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
    strcat (lb, rel[rel_ & 7]);
    strcat (lb, ".");
}
