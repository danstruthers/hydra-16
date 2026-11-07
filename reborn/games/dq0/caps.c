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
static char o[160];                             /* ("around"'s line; the choices of a which asked) */
static char w[8];
static char wb[32];                             /* (A thing in words: thing_said's; a carried one's name) */
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

/* A letter of what's carried typed at t (a ... z, aa ... az, as letters.c's let_at): its place, or NLET */
static unsigned char __fastcall__ low_at (const char* t)
{
    if (t[0] < 'a' || t[0] > 'z') {
        return NLET;
    }
    if (t[1] <= ' ') {
        return t[0] - 'a';
    }
    return t[0] == 'a' && t[1] >= 'a' && t[1] <= 'z' && t[2] <= ' ' ? NLET / 2 + t[1] - 'a' : NLET;
}

/* Letter j's thing's name (what's carried, the UI bank's ix: its kind's), "torch", into wb; 0, none carried */
static unsigned char __fastcall__ let_name (unsigned char j)
{
    static struct cix e;
    static unsigned char k, n;

    from_ui (&n, &UIREC->ixn, 1);
    for (k = 0; k < n && k < NIX; ++k) {
        from_ui (&e, UIREC->ix + k, sizeof e);
        if (e.let == j + 1 && e.kind < NIXK) {
            from_ui (wb, UIREC->ixks[e.kind].name, sizeof UIREC->ixks[0].name);
            return 1;
        }
    }
    return 0;
}

/* "letter d as t" (the user's designators, 2026-11-06: two of its letters swapped): d's thing t's, t's d's (the UI bank's
** low and lowt, and what's carried, ix: the world's from them as it makes ix again), told: "Your torch is (t) now." and
** " Your rope is (d)."; d one of nothing carried: "You have nothing lettered d." */
static void __fastcall__ swap_letters (const char* l)
{
    static unsigned u[2];
    static struct cix e;
    static unsigned char d, t, k, n, x;

    for (l += 7; *l == ' '; ++l) {
    }
    d = low_at (l);
    for (; *l > ' '; ++l) {
    }
    for (; *l == ' '; ++l) {
    }
    if (!strncmp (l, "as ", 3) || !strncmp (l, "to ", 3)) {
        l += 3;
    }
    t = low_at (l);
    if (d >= NLET || t >= NLET || !let_name (d)) {
        msg (d >= NLET || t >= NLET ? "Swap which letters? (letter d as t)" : "You have nothing by that letter.");
        return;
    }
    for (k = 0; k < 2; ++k) {                   /* (low, then lowt) */
        from_ui (u, (k ? UIREC->lowt : UIREC->low) + d, 2);
        from_ui (u + 1, (k ? UIREC->lowt : UIREC->low) + t, 2);
        to_ui ((k ? UIREC->lowt : UIREC->low) + d, u + 1, 2);
        to_ui ((k ? UIREC->lowt : UIREC->low) + t, u, 2);
    }
    from_ui (&n, &UIREC->ixn, 1);
    for (k = 0; k < n && k < NIX; ++k) {        /* (What's carried as the UI has it, till the world makes it again) */
        from_ui (&e, UIREC->ix + k, sizeof e);
        x = e.let == d + 1 ? t + 1 : e.let == t + 1 ? d + 1 : 0;
        if (x) {
            e.let = x;
            to_ui (UIREC->ix + k, &e, sizeof e);
        }
    }
    for (k = 0; k < 2; ++k) {                   /* ("Your torch is (t) now.", " Your rope is (d).") */
        if (let_name (k ? d : t)) {
            strcat (strcat (strcpy (o + (k ? strlen (o) : 0), k ? " Your " : "Your "), wb), " is (");
            for (n = strlen (o), let_text (o + n, k ? d : t); o[n]; ++n) {
                o[n] |= 0x20;
            }
            strcat (o, k ? ")." : ") now.");
        }
    }
    msg (o);
}

/* What's in sight listed, one a line, its capital and where it is ("around", the user's "a list of all in sight"): the
** denizens seen first, "(A) a giant rat, ahead to your left, 15 feet away", "(B) Grub, behind you, 5 feet away", one too
** far off to make out "(C) something, ahead, 40 feet away"; then the things, "(D) a torch (lit), at your feet", "(E) an
** iron pot, ahead to your left, 15 feet away", "(F) a candle, on the table"; one too far off to make out by its size,
** "(G) something small, ahead, 40 feet away" */
void __fastcall__ o_around (const char* l)
{
    static const struct sitem* q;
    static const struct sden* n;
    static unsigned char i, k, sz;

    if (l[6]) {
        swap_letters (l + 7);                   /* ("letter d as t": letters.c's "around letter d as t") */
        return;
    }
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

/* The number at p (its digits; the library's atoi in the main RAM): its value; p past it and the space after */
static const char* np;
static unsigned __fastcall__ num (void)
{
    static unsigned v;

    for (v = 0; *np >= '0' && *np <= '9'; ++np) {
        v = v * 10 + *np - '0';
    }
    ++np;
    return v;
}

/* A thing of kind k, n of them, in state f (WS_*) in words, as the inventory's (things.c's words): "a torch (lit)", "6
** arrows", "a flask of oil (empty)", into w8 (the C stack's: the catalogue's bank in the window as its kind's read) */
#pragma static-locals (push, off)
static void __fastcall__ thing_said (unsigned char k, unsigned n, unsigned char f)
{
    static const char* const sw[5] = { " (lit)", " (spent)", " (nearly spent)", " (burning low)", " (empty)" };
    struct kind c;
    unsigned char i;

    hub_mem (KIND_BANK, &c, CATALOGUE + k, sizeof c);
    if (n > 1) {
        strcat (utoa (n, wb, 10), " ");
    } else {
        strcpy (wb, c.flags & K_PLURAL ? "some " : strchr ("aeiou", c.name[0]) ? "an " : "a ");
    }
    strcat (wb, c.name);
    for (i = 0; i < 5; ++i) {
        if (f & 1 << i) {
            strcat (wb, sw[i]);
        }
    }
}
#pragma static-locals (pop)

/* A take asked which (the world's IR_WHICH, the designators' step 5: the user's playtest, 2026-11-06, "if all three are
** on the ground, I should get a prompt for which one, with longer descriptions of each"): the choices the world put in
** the text ("12 7 1 1|13 7 1 2|": each its id, its kind, its count and its state, WS_*) each given its capital and
** worded as the inventory words it, told, "Which one? (E) a torch (lit), (F) a torch (spent)"; the command's line and its
** thing's word kept, with each choice's words and capital (the UI bank's which_*), for its answer (letters.c's
** which_in: a capital, a number, or a word of its state, "spent", re-runs it on that one) */
void o_which_ask (void)
{
    static char* p;
    static char* e;
    static unsigned id, n;
    static unsigned char k, kd, f;

    sight_get ();
    caps_load ();
    from_ui (o, UIREC->things_text, sizeof o);
    o[sizeof o - 1] = 0;
    strcpy (lb, "Which one?");
    for (k = 0, p = o; k < 4 && (e = strchr (p, '|')) != 0; ++k, p = e + 1) {
        np = p;
        id = num ();
        kd = num ();
        n = num ();
        f = num ();
        thing_said (kd, n, f);
        cap_text (strcat (lb, k ? ", " : " ") + strlen (lb), kd = cap_give (id, CW_THING, kd));
        strcat (lb, wb);
        wb[sizeof UIREC->which_w[0] - 2] = 0;
        wb[sizeof UIREC->which_w[0] - 1] = kd;  /* (Its capital in the last byte: which_in's) */
        to_ui (UIREC->which_w[k], wb, sizeof UIREC->which_w[0]);
    }
    to_ui (UIREC->which_l, cl, strlen (cl) + 1);
    to_ui (UIREC->which_o, cmd.obj.word, sizeof UIREC->which_o);
    to_ui (&UIREC->which_n, &k, 1);
    which_on = 1;
    msg (lb);
}

/* A denizen's sound (an E_DEN: main.c's heard, the main RAM's room, 2026-11-06) weighed: heard (how far, the walls and
** doors between, the background masking it, its hearing; listened for that way, twice as far), not so soon again from the
** same way: its way (REL8), main.c's to tell (part 14's noise_told, from the main code: a part's hub_map leaves the hub in
** the window, not this bank); else 0xFF */
#pragma static-locals (push, off)
unsigned char __fastcall__ o_den_heard (const struct msg* m)
{
    unsigned pc[2], dx, dy;
    unsigned char mask, hear, rel;

    hub_mem (0, pc, &HUB->pc_x, sizeof pc);
    hub_mem (0, &mask, &HUB->pc_mask, 1);
    hub_mem (0, &hear, &HUB->pcsum.hearing, 1);
    dx = m->w1 - pc[0];
    dy = m->w2 - pc[1];
    rel = (bearing ((int) dx, (int) dy) - pc_face) & 7;
    dx = (int) dx < 0 ? -dx : dx;
    dy = (int) dy < 0 ? -dy : dy;
    dx = (dx > dy ? dx : dy) + 4 * (m->x[3] & 15) + 2 * (m->x[3] >> 4);
    if ((lsn & 1) && ((rel + 1) & 7) < 3) {
        dx >>= 1;                               /* (Listened for, that way: twice as far) */
    }
    if (dx * 2 > (m->b > mask ? m->b - mask : 0) * (hear + 3) ||   /* (The background masking it: slice 4.5's) */
        m->t < heard_at + (rel == heard_rel ? 300 : 50)) {
        return 0xFF;
    }
    heard_at = m->t;
    heard_rel = rel;
    return rel;
}
#pragma static-locals (pop)
