/*
** letters.c - the designators (the user's design, 2026-11-06, DECISIONS.md's: "capital letters for in-the-environment
** things ... and lower-case letters for items carried/worn"; hub.h's): a letter for each thing the character carries or
** wears, the world's (things.c's ix_make, the UI bank's low), shown in the inventory's list, "(a) a torch"; a capital
** for each thing it's seen, the UI's (the UI bank's cap), shown in the look's words and the list of what's in sight.
**
** A letter typed where a thing's name goes names that thing ("drop d", "put d in c", "light a with b", "take d from
** c", "throw d at the rat"): the line's letters made the things' names before it's parsed (o_letters_in, as it's
** posted: "drop d" is parsed as "drop torch"), each one's id kept (ldes), the world asked of that thing and no other
** ("#" and its id in the request, as a name given is: things.c's is_named; main.c's things, des_put), the resolver's
** question of several not asked of it (des_find), its first words its own (sway.c's des_e).  So the grammar is the
** same: a lone letter is the grammar's word else (n, s, e, w, u, d, l, r, f, b, x, i, a), and the line's case is gone in
** the parser's words (a capital, "take D", is another thing's than "take d").  Which words are letters: after the
** line's first (never the verb: "d" alone is down), one or two letters of one case, of a thing carried now (a lower
** case's), at the line's end or before a word that ends a thing's name ("in", "on", "from", "with", "at", "to", "and",
** "then", a manner ...: so "take a torch" is an article's), and not a way straight after a verb of ways ("look d" is
** down, "look at d" the thing); a count before it ("drop 2f") kept.  A command with nothing named asks, with the
** letters of those it can be done to ("Drop what? [a, d, f]"), and a letter answers it ("f": "drop f").
**
** A capital, A ... Z then AA ... AZ (the UI bank's cap: each one's thing's id, its kind, when it was last told), for a
** thing seen, given as it's first told (the look's words, seen.c's one: o_cap_of; the list of what's in sight, "around":
** o_around), kept while it's remembered, given up only as a new one's wanted and none is free, the one told longest ago
** (some ten minutes at least) that isn't in sight now; a thing put down has its old one again (its id the same).  A
** capital typed names that thing as a letter does ("take D", "take D from C", "take 3E": its count; a capital's thing
** need not be in sight, the world's to find by its id).  The denizens (the plan's step 4) take theirs from the same
** capitals (CW_DEN), given before the things' in a look, so those seen first are A, B ...
**
** Its code: the main RAM's what the main code reads (ldes, desb, desk, capb); overlay 22's the rest of the line's
** (o_letters_in, o_what_ask, o_des_find, o_des_low, o_des_map) and the capitals' (o_cap_of, o_around); part 22's the
** save's.  A line's letters made its things' names as it's posted to the parser and only then (main.c's cq_st): a line
** with no word that may be a letter (one letter, or two an a's, a count before perhaps) let be at once.
*/

#include <ctype.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "dq0.h"
#include "hub.h"

#pragma static-locals (on)                      /* (Its locals static: the code the less; nothing here is reentered) */

/* ==== The main RAM's: the line's letters, made its things' names (o_letters_in's), for the requests and the resolver */
struct ldes ldes[NDES];
unsigned char ndes;
char capb[6];                                   /* (A capital's words, "(C) ": o_cap_of's, for overlay 12's look) */

char desb[8];                                   /* (A request's word as des_map made it: "#" and an id) */
unsigned char desk;                             /* (The letter des_map found last: its place plus 1, or 0) */

/* ==== Overlay 22's (its own, 2026-11-06: the designators'): a line's letters made its things' names (every line's, as
** it's posted: main.c's main loop); the question asked of a command with nothing named */
#pragma code-name (push, "OV22CODE")
#pragma rodata-name (push, "OV22RODATA")
#pragma data-name (push, "OV22RODATA")
#pragma bss-name (push, "OV22RODATA")
#pragma local-strings (push, on)

static struct cix ix[NIX];                      /* (What's carried, copied in: the world's, ix_make's) */
static unsigned char nix;
static unsigned lowc[NLET];                     /* (The UI bank's low, copied) */
static unsigned dsum;                           /* (The line's sum as its letters were made names: posted again, held for
                                                **   the character, its letters' things still its) */
static char asked[24];                          /* (A question's line, "drop": its answer, "d", made "drop d") */
static unsigned char offer[(NLET + 7) / 8];     /* (The letters it offered, a bit each) */
static char o[INMAX + 32];                      /* (The line made, and the question's words) */
static char w[16];
static unsigned char ts[16], tl[16];            /* (The line's words: where each begins, its length) */
static struct ucap capc[NLET];                  /* (The UI bank's cap, copied in) */
static unsigned char cseq, cnew = 1;            /* (The look they're copied for, sight_copy's seq; 1, none yet) */
static unsigned sid[SEE_ITEMS];                 /* (The things in sight: their ids, */
static unsigned char skind[SEE_ITEMS];          /*   their kinds) */
static unsigned cnow;                           /* (The clock's stamp: its tenths over 1024) */
static char* __fastcall__ let_text (char* b, unsigned char j, char a);

/* The first of the line's letters, from ldes[k] on, that made word w, its thing known: its place plus 1, or 0 */
unsigned char __fastcall__ o_des_find (const char* w, unsigned char k)
{
    for (; k < ndes; ++k) {
        if (ldes[k].id && !strcmp (w, ldes[k].word)) {
            return k + 1;
        }
    }
    return 0;
}

/* Is word w a lower-case letter's, its thing carried, for a command done with it (resolve.c's tool_ok: to hand, the world not
** asked; but a blow's, a dig's, which want its kind)? */
unsigned char __fastcall__ o_des_low (const char* w)
{
    unsigned char k = o_des_find (w, 0);

    return k && ldes[k - 1].low && cmd.action != VB_ATTACK_OBJECT && cmd.action != VB_BREAK_OBJECT && cmd.action != VB_DIG;
}

/* Word w (a request's name, or its other's: main.c's things) the thing's own if a letter made it, from ldes[k] on: "#"
** and its id, as a name given is (the world chooses that one: things.c's is_named), in desb; else w.  desk the letter's
** place plus 1, or 0 */
const char* __fastcall__ o_des_map (const char* w, unsigned char k)
{
    if ((desk = o_des_find (w, k)) == 0) {
        return w;
    }
    desb[0] = '#';
    utoa (ldes[desk - 1].id, desb + 1, 10);
    return desb;
}

/* This bank's bytes copied into the UI bank (to_ui), or a hub bank's into this bank (from_hub), by way of the C stack (the
** two share the window), a piece at a time */
#pragma static-locals (push, off)
static void __fastcall__ to_ui (void* to, const void* from, unsigned n)
{
    char t[64];
    unsigned char k;

    while (n) {
        k = n > sizeof t ? sizeof t : n;
        memcpy (t, from, k);
        hub_mem (UI_BANK, to, t, k);
        to = (char*) to + k;
        from = (const char*) from + k;
        n -= k;
    }
}

#define from_ui(t, f, n)    from_hub (UI_BANK, (t), (f), (n))
static void __fastcall__ from_hub (unsigned char bank, void* to, const void* from, unsigned n)
{
    char t[64];
    unsigned char k;

    while (n) {
        k = n > sizeof t ? sizeof t : n;
        hub_mem (bank, t, from, k);
        memcpy (to, t, k);
        to = (char*) to + k;
        from = (const char*) from + k;
        n -= k;
    }
}
#pragma static-locals (pop)

/* What's carried copied in from the UI bank (the world's): its things, the letters */
static void ix_load (void)
{
    from_ui (&nix, &UIREC->ixn, 1);
    if (nix > NIX) {
        nix = NIX;
    }
    from_ui (ix, UIREC->ix, nix * sizeof (struct cix));
    from_ui (lowc, UIREC->low, sizeof lowc);
}

/* The things in sight (sight_copy's, as the look's words or the list of them have it): their ids and kinds; the clock's
** stamp; the capitals copied in: once a look */
#pragma static-locals (push, off)
static void caps_load (void)
{
    struct item it;
    unsigned long t;
    unsigned char k;

    if (!cnew && cseq == sight_copy.seq) {
        return;
    }
    cnew = 0;
    cseq = sight_copy.seq;
    for (k = 0; k < sight_copy.nitems; ++k) {
        hub_mem (ITEM_BANK (sight_copy.item[k].n), &it, ITEM_AT (sight_copy.item[k].n), sizeof it);
        sid[k] = it.id;
        skind[k] = it.kind;
    }
    hub_mem (0, &t, &HUB->now, sizeof t);
    cnow = (unsigned) (t >> 10);
    from_ui (capc, UIREC->cap, sizeof capc);
}
#pragma static-locals (pop)

/* Thing id's capital (what: CW_THING; the denizens', step 4's, CW_DEN; kind its kind): its own, told again now; else a new
** one, of A ... Z the first free, else the one told longest ago (6 stamps at least, some ten minutes) that isn't in sight,
** then of AA ... AZ the same way (the UI bank's written).  Its place, or NLET (none) */
static unsigned char __fastcall__ cap_give (unsigned id, unsigned char what, unsigned char kind)
{
    static unsigned char j, e, b, k;
    static unsigned o, d;

    if (!id) {
        return NLET;
    }
    for (j = 0; j < NLET && (capc[j].id != id || capc[j].what != what); ++j) {
    }
    for (e = 0; j == NLET && e < NLET; e += NLET / 2) {
        for (b = NLET, o = 5, j = e; j < e + NLET / 2 && capc[j].id; ++j) {
            for (k = 0; k < sight_copy.nitems && sid[k] != capc[j].id; ++k) {
            }
            if (k == sight_copy.nitems && (d = cnow - capc[j].t) > o) {
                o = d;                          /* (Told the longest ago, a while ago, not in sight) */
                b = j;
            }
        }
        if (j == e + NLET / 2) {
            j = b;
        }
    }
    if (j < NLET && (capc[j].id != id || capc[j].t != cnow)) {
        capc[j].id = id;
        capc[j].what = what;
        capc[j].kind = kind;
        capc[j].t = cnow;
        to_ui (UIREC->cap + j, capc + j, sizeof capc[0]);
    }
    return j;
}

/* Thing i of those in sight (sight_copy's) its capital, "(C) ", into capb (the main RAM's): the look's words', seen.c's
** one (overlay 12's); "" none */
const char* __fastcall__ o_cap_of (unsigned char i)
{
    static unsigned char j;

    caps_load ();
    capb[0] = 0;
    if (i < sight_copy.nitems && (j = cap_give (sid[i], CW_THING, skind[i])) < NLET) {
        capb[0] = '(';
        strcat (let_text (capb + 1, j, 'A'), ") ");
    }
    return capb;
}

/* What's in sight listed, one a line, its capital and where it is ("around", the user's "a list of all in sight":
** "(C) a torch (lit), at your feet", "(D) an iron pot, ahead to your left, 15 feet away", "(E) a candle, on the table";
** one too far off to make out by its size, "(F) something small, ahead, 40 feet away").  The denizens' to come (the
** plan's step 4) */
void o_around (void)
{
    static const char* const rel[8] = {
        "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
        "behind you to your left", "to your left", "ahead to your left"
    };
    static const struct sitem* q;
    static unsigned char i, j, k, d, sz;
    static int dx, dy;

    sight_get ();
    caps_load ();
    if (!sight_copy.nitems) {
        msg ("You see nothing lying about.");
        return;
    }
    for (i = 0; i < sight_copy.nitems; ++i) {
        q = sight_copy.item + i;
        o[0] = 0;
        if ((j = cap_give (sid[i], CW_THING, skind[i])) < NLET) {
            o[0] = '(';
            strcat (let_text (o + 1, j, 'A'), ") ");
        }
        dx = q->x - sight_copy.x;
        dy = q->y - sight_copy.y;
        dx = dx < 0 ? -dx : dx;
        dy = dy < 0 ? -dy : dy;
        d = dx > dy ? dx : dy;
        sz = q->hid >> 4;
        if (!(q->hid & 1) && d > 1 + sight_copy.eye / 2 + sz) {   /* (Too far off to make out: by its size, seen.c's) */
            strcat (o, sz <= 2 ? "something small" : sz >= 5 ? "something large" : "something");
        } else {
            strcat (o, q->qty > 1 ? utoa (q->qty, w, 10) : q->plural ? "some" : strchr ("aeiou", q->name[0]) ? "an" : "a");
            strcat (o, " ");
            strcat (o, q->name);
            if (q->hid & 4) {
                strcat (o, " (lit)");
            }
        }
        strcat (o, ", ");
        if (q->hid & 2) {                       /* (On the thing before it) */
            for (k = i; k && (sight_copy.item[k].hid & 2); --k) {
            }
            strcat (strcat (o, "on the "), sight_copy.item[k].name);
        } else {
            strcat (o, q->hid & 1 ? "hidden " : "");
            if (q->rel == 0xFF) {
                strcat (o, "at your feet");
            } else {
                strcat (o, rel[q->rel & 7]);
                if (d > 1) {
                    strcat (strcat (strcat (o, ", "), utoa (5 * d, w, 10)), " feet away");
                }
            }
        }
        if (i) {
            say ("\n");
            say (o);
        } else {
            msg (o);
        }
    }
}

/* The letter text t (n letters) names: its place, 0 ... NLET - 1; or NLET (none) */
static unsigned char __fastcall__ let_at (const char* t, unsigned char n)
{
    static unsigned char c;

    c = t[n - 1] | 0x20;
    if (c < 'a' || c > 'z') {
        return NLET;
    }
    return n == 1 ? c - 'a' : (*t | 0x20) == 'a' ? NLET / 2 + c - 'a' : NLET;
}

/* Letter j's text (its case a's, a or A) put at b: its end */
static char* __fastcall__ let_text (char* b, unsigned char j, char a)
{
    if (j >= NLET / 2) {
        *b++ = a;
        j -= NLET / 2;
    }
    *b++ = a + j;
    *b = 0;
    return b;
}

/* The entry of what's carried with letter j (ix's), or nix */
static unsigned char __fastcall__ let_e (unsigned char j)
{
    static unsigned char e;

    for (e = 0; e < nix && ix[e].let != j + 1; ++e) {
    }
    return e;
}

/* Word s (n letters) one of list's (0-ended words, an empty one last)? */
static unsigned char __fastcall__ in_list (const char* list, const char* s, unsigned char n)
{
    for (; *list; list += strlen (list) + 1) {
        if (strlen (list) == n && !strncmp (list, s, n)) {
            return 1;
        }
    }
    return 0;
}

/* The line's key (posted again: the same line): its length, its first letter and its last */
static unsigned __fastcall__ sum (const char* l)
{
    static unsigned char n;

    n = strlen (l);
    return (unsigned) n << 8 ^ (unsigned char) l[0] ^ (unsigned char) (n ? l[n - 1] : 0) << 1;
}

/* Line l made o (l the queue's first line, term.c's t_next: the lines after it moved): 1, or 0 (no room) */
static unsigned char __fastcall__ line_put (char* l)
{
    static unsigned a, b;

    a = strlen (l) + 1;
    b = strlen (o) + 1;
    if (b > INMAX + 1 || (l == queue && qlen + b - a > QMAX)) {
        return 0;
    }
    if (l == queue) {
        memmove (queue + b, queue + a, qlen - a);
        qlen += b - a;
    }
    memcpy (l, o, b);
    return 1;
}

/* Line l's letters made its things' names (its every line's, as it's posted, before the parser has it); the answer to
** the question asked (a letter alone, one it offered) made its command ("d": "drop d") first */
void __fastcall__ o_letters_in (char* l)
{
    static const char joins[] = "in\0into\0inside\0on\0onto\0from\0with\0at\0to\0and\0then\0under\0over\0down\0up\0off\0"
        "away\0back\0out\0through\0toward\0towards\0carefully\0slowly\0quickly\0quietly\0hard\0gently\0lightly\0firmly\0";
    static const char ways[] = "go\0walk\0run\0move\0sprint\0crawl\0sneak\0step\0turn\0face\0look\0peer\0climb\0dig\0"
        "head\0jump\0swim\0flee\0";
    static const char* p;
    static unsigned char nt, i, j, e, n, c, nd, loaded, up, ok;
    static unsigned id;

    for (nd = nt = 0, p = l; *p && nt < sizeof ts; ) {  /* (Its words; nd, one after the first that may be a letter) */
        if (*p == ' ') {
            ++p;
            continue;
        }
        ts[nt] = p - l;
        while (*p && *p != ' ') {
            ++p;
        }
        tl[nt] = p - l - ts[nt];
        for (c = ts[nt]; isdigit (l[c]); ++c) {
        }
        if (nt && ((c = p - l - c) == 1 || (c == 2 && (p[-2] | 0x20) == 'a'))) {
            nd = 1;                             /* (A letter perhaps: one, or two an a's, a count before it perhaps) */
        }
        ++nt;
    }
    if (!nd && !asked[0]) {
        if (sum (l) != dsum) {
            ndes = 0;                           /* (A line of its own, its letters none: at once, before the parser has
                                                **   it, nothing more done) */
        }
        return;
    }
    if (asked[0]) {                             /* (The question's answer: a letter alone, one offered) */
        if (nt == 1 && tl[0] <= 2 && islower (l[ts[0]]) && (j = let_at (l + ts[0], tl[0])) < NLET &&
            (offer[j >> 3] & 1 << (j & 7))) {
            strcpy (o, asked);
            strcat (o, " ");
            strcat (o, l + ts[0]);
            if (line_put (l)) {
                asked[0] = 0;
                o_letters_in (l);               /* (Its letter made its thing's name, as any line's) */
                return;
            }
        }
        asked[0] = 0;
    }
    o[0] = 0;
    nd = loaded = 0;
    if (nt && in_list ("alias\0unalias\0say\0yell\0whisper\0shout\0", l + ts[0], tl[0])) {
        nt = 0;                                 /* (A shortcut's line, words said: their letters as they are) */
    }
    for (i = 0; i < nt; ++i) {
        if (i) {
            strcat (o, " ");
        }
        p = l + ts[i];
        n = tl[i];
        for (c = 0; c < n && isdigit (p[c]); ++c) {   /* (A count before it: "2f") */
        }
        up = isupper (p[c]) != 0;
        if (i && n - c && n - c <= 2 && (up ? isupper (p[n - 1]) : islower (p[c]) && islower (p[n - 1])) && nd < NDES &&
            (i + 1 == nt || in_list (joins, l + ts[i + 1], tl[i + 1])) &&   /* (Then the line's end, or a word that ends a
                                                **   thing's name: so not an article) */
            !(n == 1 && i == 1 && strchr ("nsewudlrfbNSEWUDLRFB", *p) && in_list (ways, l + ts[0], tl[0]))) {   /* (Nor a way
                                                **   straight after a verb of ways) */
            j = let_at (p + c, n - c);
            ok = 0;
            if (j < NLET && up) {
                from_ui (capc + j, UIREC->cap + j, sizeof capc[0]);   /* (Its capital alone: the line's the sooner
                                                **   to the parser) */
            } else if (j < NLET && !loaded) {
                ix_load ();
                loaded = 1;
            }
            if (j < NLET && up && capc[j].id && capc[j].what == CW_THING) {   /* (A capital: a thing seen, its kind's
                                                **   name from the catalogue's) */
                id = capc[j].id;
                ok = 1;
                from_hub (KIND_BANK, w, CATALOGUE[capc[j].kind].name, sizeof CATALOGUE[0].name);
            } else if (j < NLET && !up && (e = let_e (j)) < nix && ix[e].kind < NIXK) {
                id = lowc[j] & LT_REC ? 0 : lowc[j];    /* (One with no id yet: by its name, the world's to choose) */
                ok = 1;
                from_ui (w, UIREC->ixks[ix[e].kind].name, sizeof UIREC->ixks[0].name);
            }
            if (ok) {
                strncat (o, p, c);              /* (Its count: "2 rations") */
                if (c) {
                    strcat (o, " ");
                }
                if ((p = strstr (w, " of ")) != 0) {   /* (Its kind's name's head word: "flask" of "flask of oil", "key"
                                                **   of "iron key") */
                    *(char*) p = 0;
                }
                for (p = w + strlen (w); p > w && p[-1] != ' '; --p) {
                }
                strcat (o, p);
                ldes[nd].id = id;
                ldes[nd].low = !up;
                strcpy (ldes[nd++].word, p);
                continue;
            }
        }
        strncat (o, l + ts[i], n);
    }
    if (nd && line_put (l)) {
        ndes = nd;
        dsum = sum (l);
    } else if (sum (l) != dsum) {
        ndes = 0;                               /* (A line of its own: its letters none) */
    }
}

/* A command with nothing named (CF_UNSAID: main.c's, before its unsaid thing's filled) of those that take a thing it
** carries: asked about, with the letters of those it can be done to ("Drop what? [a, d, f]", "Wear what? [m]": dropped,
** any; worn, one worn not, of a kind that's worn; taken off, one worn; held, wielded, one not in hand; put away, one in
** hand), its answer a letter alone (o_letters_in's).  1 asked; 0 not one of those, or none it can be done to (as
** ever) */
unsigned char o_what_ask (void)
{
    static char* p;
    static unsigned char e, f, a, any, j;

    a = cmd.aux;
    if ((cmd.action != VB_DROP_OBJECT && cmd.action != VB_EQUIP_OBJECT && cmd.action != VB_HOLD_OBJECT &&
        cmd.action != VB_UNEQUIP_OBJECT) || !*cl || strlen (cl) >= sizeof asked) {
        return 0;
    }
    ix_load ();
    strcpy (o, cl);
    *o = toupper (*o);
    p = strcat (o, " what? [") + strlen (o);
    memset (offer, 0, sizeof offer);
    for (any = j = 0; j < NLET; ++j) {      /* (In their order, a ... z, aa ... az) */
        if ((e = let_e (j)) >= nix || ix[e].kind >= NIXK || p > o + sizeof o - 8) {
            continue;
        }
        f = ix[e].flags;
        if (a == IO_DON) {
            from_ui (&f, &UIREC->ixks[ix[e].kind].flags, 1);
            f = (f & K_WEAR) && !(ix[e].flags & CX_WORN);
        } else {
            f = a == IO_HOLD ? !(f & CX_HELD) : a == IO_DOFF ? f & CX_WORN : a == IO_STOW ? f & CX_HELD : a == IO_DROP;
        }
        if (!f) {
            continue;
        }
        if (any) {
            *p++ = ',';
            *p++ = ' ';
        }
        any = 1;
        offer[j >> 3] |= 1 << (j & 7);
        p = let_text (p, j, 'a');
    }
    if (!any) {
        return 0;
    }
    strcpy (p, "]");
    strcpy (asked, cl);
    msg (o);
    return 1;
}

#pragma local-strings (pop)
#pragma bss-name (pop)
#pragma data-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* ==== Part 22's (seldom: the save, its start) */
#pragma code-name (push, "SW22CODE")
#pragma rodata-name (push, "SW22RODATA")
#pragma bss-name (push, "SW22RODATA")

#define LETS        (offsetof (struct uirec, cap) + sizeof UIREC->cap - offsetof (struct uirec, low))
typedef char lets_fit[offsetof (struct uirec, lowt) == offsetof (struct uirec, low) + sizeof UIREC->low &&
                      offsetof (struct uirec, cap) == offsetof (struct uirec, lowt) + sizeof UIREC->lowt ? 1 : -1];
static char lk[64];                             /* (Through the main RAM, a piece at a time: the swapped part's own) */

/* The letters kept: written to the save's fd (back 0), read back from it (1; fd -1: none, a new game's) */
void __fastcall__ s_letters_keep (int fd, unsigned char back)
{
    static char* at;
    static unsigned n;

    for (at = (char*) UIREC->low; at < (char*) UIREC->low + LETS; at += n) {
        n = (char*) UIREC->low + LETS - at;
        if (n > sizeof lk) {
            n = sizeof lk;
        }
        if (back) {
            if (fd < 0 || read (fd, lk, n) != n) {
                memset (lk, 0, n);
            }
            hub_map (UI_BANK);
            memcpy (at, lk, n);
        } else {
            hub_map (UI_BANK);
            memcpy (lk, at, n);
            hub_map (0);
            write (fd, lk, n);
        }
    }
    hub_map (0);
}

/* The names given, the shortcuts and the letters read back from the save (after its header; fd -1: none, a new game's:
** main.c's, part 1's room) */
void __fastcall__ s_keeps_back (int fd)
{
    names_keep (fd, 1);
    alias_keep (fd, 1);
    s_letters_keep (fd, 1);
}

#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
