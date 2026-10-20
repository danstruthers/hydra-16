/*
** names.c - (parts 5, 6) the names the character gives (the user, 2026-10-16: "I would like to be able to reference
** items by aliases, as long as the alias doesn't clash with something else"): a thing it carries or sees ("name my
** sword Linda", "alias the sword as Linda", "call the dagger Sting"), the room it's in ("call this room the Mausoleum",
** "name this room "The Mausoleum""), an ally (the denizens' task's: "name the rat Grub", recorded here too); and a
** name forgotten ("forget the name Linda", "unname Linda", "unname this room").  The names are a table in the hub's
** theme bank (hub.h's struct tname, NAMES_AT), kept in the save.  A name's refused if it clashes: with another name
** ("That name is taken by your dagger."), a kind of thing's, a kind of creature's, a denizen's in sight, or (a name
** of one word) a word the parser needs ("north", "take").
**
** A thing's name works wherever a command names a thing ("attack the goblin with Linda", "drop Linda", "put Linda in
** the pack"): the line's names made the things' own before it's parsed ("#" and the thing's id, a word the world's
** item store finds it by: dq0w/things.c's is_named), and a fight's "with Linda" taken in hand first.  The words
** say it: the things' list marks a thing named ("#12 a sword": I_NAMED's) and a look's things are marked here, each
** worded as it's said ("Linda, your sword": names_say, from term.c's say); a room named, as the character's in it
** ("You are in the Mausoleum, a small crypt": names_look's).  A room's name is the way to it: room_named finds one
** in a line's words (DUNGEON.md's names, for the orientation's travel).
**
** Its table's reading the main code's; its commands a swapped part's (part 5: swap.inc), its words another's (part
** 6), called through bank.s's stubs; their data and BSS the main RAM's.  A line's first look, whether it may be
** either part's (names_in: so an ordinary command copies neither in), is overlay 10's.
*/

#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

#pragma local-strings (on)                      /* (A function's strings beside it, in the part) */
#pragma static-locals (on)                      /* (Its locals static: the code the less; nothing here is reentered) */

unsigned char names_n;                          /* The names given (0: none, and the words, the looks and the lines
                                                **   pass the part by) */
struct tname named_room;                        /* (room_named's: the room's entry) */

static struct tname* np;                        /* An entry (in the theme bank), */
static struct tname e;                          /*   copied out (ent's) */
static unsigned fid;                            /* (find's: a thing's id) */
static char ob[480];                            /* names_say's words; the save's pieces; a path, a record (taken's,
                                                **   in its first 120 bytes); and, while a command names: */
#define nb      (ob + 120)                      /*   a line's words (a copy, the case kept), */
#define thw     (ob + 240)                      /*   the thing it names, */
#define nm      (ob + 360)                      /*   the name it's given (and the thing's kind's) */
typedef char ob_fits[360 + INMAX + 1 <= sizeof ob ? 1 : -1];
static char* wd[16];
static unsigned char nw;
static char dn[NAME_LEN];                       /* A name as it's said */

#define NM_ANY          0xFF                    /* (find's: any entry in use) */

/* ---- The table */

/* Entry np copied out into e (the theme bank in the window a moment, then the hub's header), or back */
static void ent (void)
{
    hub_map (THEME_BANK);
    memcpy (&e, np, sizeof e);
    hub_map (0);
}

static void ent_put (void)
{
    hub_map (THEME_BANK);
    memcpy (np, &e, sizeof e);
    hub_map (0);
}

/* A name's key: past a leading "the " */
static const char* __fastcall__ key (const char* n)
{
    return !strnicmp (n, "the ", 4) ? n + 4 : n;
}

static unsigned char __fastcall__ same (const char* a, const char* b)
{
    return !stricmp (key (a), key (b));
}

/* The first entry that's what (NM_NONE: a free one, cleared; NM_ANY: any in use) named k; or with no k, a thing's
** of id fid, the room the character's in: in np, copied into e; or 0 */
static struct tname* __fastcall__ find (unsigned char what, const char* k)
{
    static unsigned char sn;
    static unsigned rxy[2];

    if (what == NM_ROOM && !k) {                /* (The room's: the senses' record's, copied out; not sight_copy,
                                                **   a look's words may be using it) */
        hub_mem (SIGHT_BANK, &sn, &SIGHT->stratum, 1);
        hub_mem (SIGHT_BANK, rxy, &SIGHT->rx, sizeof rxy);
    }
    for (np = NAMES_AT; np < NAMES_AT + TNAMES; ++np) {
        ent ();
        if (what == NM_ANY ? !e.what : e.what != what) {
            continue;
        }
        if (k ? same (e.name, k) : what == NM_ITEM ? e.x == fid : what != NM_ROOM || (e.sn == sn &&
            e.x == rxy[0] && e.y == rxy[1])) {
            if (!what) {
                memset (&e, 0, sizeof e);
            }
            return np;
        }
    }
    return 0;
}

/* A word's hash (its letters' case aside, up to a space or a mark at_key ends a name at): a bit of names_bits */
static unsigned char names_bits[32];            /* (The things' names' first words, hashed: names_in's) */
static unsigned char __fastcall__ hash (const char* s)
{
    static unsigned char h, c;

    for (h = 0; (c = *s) != 0 && !strchr (" ,.!?\"", c); ++s) {
        h = (h << 3 | h >> 5) ^ (c >= 'a' && c <= 'z' ? c - ('a' - 'A') : c);
    }
    return h;
}

/* The names given, counted (names_n); the things' names' first words, hashed (names_bits) */
static void count (void)
{
    unsigned char h;

    memset (names_bits, 0, sizeof names_bits);
    for (names_n = 0, np = NAMES_AT; np < NAMES_AT + TNAMES; ++np) {
        ent ();
        names_n += e.what != NM_NONE;
        if (e.what == NM_ITEM) {
            h = hash (key (e.name));
            names_bits[h >> 3] |= 1 << (h & 7);
        }
    }
}

/* Text t begun with key k (the case aside) up to a word's end there: its length, or 0 */
static unsigned char __fastcall__ at_key (const char* t, const char* k)
{
    unsigned char n = strlen (k);

    return !strnicmp (t, k, n) && (!t[n] || strchr (" ,.!?\"", t[n])) ? n : 0;
}

#pragma code-name (push, "OV10CODE")           /* ---- Overlay 10: a line's first look */
#pragma rodata-name (push, "OV10RODATA")

/* A line's first look (main.c's, before the parts are called: each copied in only for a line that may be its):
** NI_LINE, a line that names (its first word name, call, alias, rename, dub, forget or unname: s_names_line's);
** NI_THING, one that may say a thing's name (a "#", or a word where one of the things' names' first words could
** be, by names_bits: s_names_subst's, which finds none in a line without them).  Overlay code: the main RAM's
** data only (names_bits; not the hub's table) */
unsigned char __fastcall__ o_names_in (const char* l)
{
    static const char* v;
    static unsigned char n, r, h;

    r = 0;
    for (v = "name\0call\0alias\0rename\0dub\0forget\0unname\0"; *v; v += n + 1) {
        n = strlen (v);
        if (!strnicmp (l, v, n) && (!l[n] || l[n] == ' ')) {
            r = NI_LINE;
            break;
        }
    }
    if (strchr (l, '#')) {
        return r | NI_THING;
    }
    for (v = l; *v; ++v) {
        if ((v == l || v[-1] == ' ') && (names_bits[(h = hash (v)) >> 3] & 1 << (h & 7))) {
            return r | NI_THING;
        }
    }
    return r;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW5CODE")            /* ---- Part 5: the commands */
#pragma rodata-name (push, "SW5RODATA")

/* A free entry (np, e cleared), or 0, told */
static struct tname* free_one (void)
{
    if (!find (NM_NONE, 0)) {
        msg ("You have given names enough.");
        return 0;
    }
    return np;
}

/* Name n begun with k (the case aside) at one of its words: "sw" a sword's, "rat" a giant rat's */
static unsigned char __fastcall__ word_of (const char* n, const char* k)
{
    unsigned char m = strlen (k);

    for (; n; n = strchr (n, ' ') ? strchr (n, ' ') + 1 : 0) {
        if (!strnicmp (n, k, m)) {
            return 1;
        }
    }
    return 0;
}

/* Word w one of list l (its words each n bytes, an empty one at its end) */
static unsigned char __fastcall__ one_of (const char* w, const char* l, unsigned char n)
{
    for (; *l; l += n) {
        if (!stricmp (w, l)) {
            return 1;
        }
    }
    return 0;
}

static const char little[][7] = {
    "the", "a", "an", "this", "that", "my", "your", "our", "its", "his", "her", "these", "those", "of", "and", ""
};
static const char rooms[][8] = { "room", "chamber", "hall", "cave", "cell", "vault", "place", "here", "" };
#define LITTLE(w)       one_of ((w), little[0], sizeof little[0])
#define ROOM(w)         one_of ((w), rooms[0], sizeof rooms[0])

/* ---- A name: as it's said, and whether it's free */

/* The name in nm made dn: its words capitalised, but "the" and "of" ("the Hall of Kings"), quotes and its end's
** stops gone: 0 if it's empty or too long */
static unsigned char tidy (void)
{
    char* s = nm;
    char* d = dn;
    char* w;

    for (; *s; ++s) {
        if (*s == '"' || (*s == ' ' && (d == dn || d[-1] == ' '))) {
            continue;
        }
        if (d >= dn + NAME_LEN - 1) {
            return 0;
        }
        *d++ = *s;
    }
    while (d > dn && strchr (" .,!?", d[-1])) {
        --d;
    }
    *d = 0;
    for (w = dn; w; w = strchr (w, ' ') ? strchr (w, ' ') + 1 : 0) {
        *w = strnicmp (w, "the ", 4) && strnicmp (w, "of ", 3) ? toupper (*w) : tolower (*w);
    }
    return *key (dn) != 0;
}

/* Whether name k is taken, but by entry self (0: none): 1, told why */
static unsigned char __fastcall__ taken (const char* k, struct tname* self)
{
    static struct actor* a;
    static int fd;
    static unsigned char i, n;

    if (!isalpha (*key (k))) {
        msg ("A name begins with a letter.");
        return 1;
    }
    if (find (NM_ANY, k) && np != self) {     /* (Another name) */
        if (e.what == NM_ROOM) {
            msg ("That name is taken by another room.");
        } else {
            sayf ("\nThat name is taken by your %s.", e.kind);
        }
        return 1;
    }
    k = key (k);
    for (i = 1; i < NKINDS; ++i) {              /* (A kind of thing's: the catalogue's) */
        hub_map (CAT_BANK);
        memcpy (ob, CATALOGUE[i].name, sizeof CATALOGUE[0].name);
        hub_map (0);
        if (ob[0] && word_of (ob, k)) {
            sayf ("\nThat name is taken by a kind of thing (%s).", ob);
            return 1;
        }
    }
    strcpy (ob, libdir);                        /* (A kind of creature's: the denizens' task's file, 40 bytes a kind,
                                                **   the first none, its 21st byte the kinds' count) */
    strcat (ob, "/dq0d.dat");
    if ((fd = open (ob, O_RDONLY)) >= 0) {
        for (i = 0, n = 1; i < n && read (fd, ob, 40) == 40; ++i) {
            n = i ? n : ob[20];
            ob[12] = 0;
            if (i && ob[0] && word_of (ob, k)) {
                break;
            }
        }
        close (fd);
        if (i < n) {
            sayf ("\nThat name is taken by a kind of creature (%s).", ob);
            return 1;
        }
    }
    for (a = HUB->actor; a < HUB->actor + NACTORS; ++a) {   /* (A denizen's near: an ally's, given before) */
        if (a->name[0] && same (a->name, k)) {
            sayf ("\nThat name is taken by %s.", a->name);
            return 1;
        }
    }
    if (!strchr (k, ' ') && strlen (k) < sizeof cmd.obj.word) {    /* (A word the parser needs: a word of a command) */
        cq_drop ();                             /* (The line's, posted as it was typed, let go: asked again if
                                                **   it's a command's) */
        cq_cmd (k);
        cq_drop ();
        if (!(cmd.flags & CF_NOVERB) || cmd.dir != 0xFF || cmd.obj.kind || cmd.obj.sel || cmd.n ||
            stricmp (cmd.obj.word, k)) {
            msg ("That name is taken: it's a word your commands need.");
            return 1;
        }
    }
    return 0;
}

/* Entry self (a free one's, if 0) given name dn, the rest e's */
static void __fastcall__ put_name (struct tname* self)
{
    np = self;
    strcpy (e.name, dn);
    ent_put ();
    count ();
}

/* ---- The lines */

/* Line l's words, a copy, into nb (wd, nw) */
static void __fastcall__ split (const char* l)
{
    static char* p;

    p = strcpy (nb, l);
    for (nw = 0; nw < sizeof wd / sizeof wd[0]; ) {
        while (*p == ' ') {
            *p++ = 0;
        }
        if (!*p) {
            break;
        }
        wd[nw++] = p;
        while (*p && *p != ' ') {
            ++p;
        }
    }
    *p = 0;
}

/* Words a to b (not b) into d, a space between */
static void __fastcall__ join (char* d, unsigned char a, unsigned char b)
{
    for (*d = 0; a < b; ++a) {
        if (*d) {
            strcat (d, " ");
        }
        strcat (d, wd[a]);
    }
}

/* The mark of thing fid ("#12"), into thw */
static void mark (void)
{
    thw[0] = '#';
    utoa (fid, thw + 1, 10);
}

/* A thing named by thw's words (carried, or seen): its id in fid (its kind's name in the hub's ireq), or 0 */
static unsigned thing (void)
{
    names_subst (thw);
    return fid = thw[0] && othings (IO_DESCRIBE, thw, "", 0) == IR_OK ? HUB->ireq.item : 0;
}

/* A denizen near named by thw's words (its name's, or its kind's: "the rat" a giant rat, not the rations) */
static unsigned char denizen (void)
{
    static struct actor* a;

    for (a = HUB->actor; a < HUB->actor + NACTORS; ++a) {
        if (a->name[0] && word_of (a->name, thw)) {
            return 1;
        }
    }
    return 0;
}

/* A name given ("name my sword Linda", "alias the sword as Linda", "call this room the Mausoleum", "name the rat
** Grub"), the line in nb's words */
static unsigned char __fastcall__ give (char* l)
{
    static unsigned char i, a;
    static struct tname* self;
    static struct sight* s;
    static char* p;
    static char* q;

    nm[0] = 0;
    if ((p = strchr (nb, '"')) != 0 && (q = strchr (p + 1, '"')) != 0) {   /* (A name in quotes) */
        *q = 0;
        strcpy (nm, p + 1);
        strcpy (p, q + 1);                      /* (The rest the thing's words) */
        split (strcpy (thw, nb));
        a = nw;
    } else {
        for (a = 1; a < nw && stricmp (wd[a], "as"); ++a) {
        }
        if (a < nw) {                           /* ("alias the sword as Linda") */
            join (nm, a + 1, nw);
        } else {
            for (i = 1; i < nw && LITTLE (wd[i]); ++i) {
            }
            a = i < nw && ROOM (wd[i]) ? i + 1 : nw - 1;    /* ("call this room the Mausoleum"; else the name's the
                                                **   last word: "name my sword Linda") */
            join (nm, a, nw);
        }
    }
    for (i = 1; i < a && LITTLE (wd[i]); ++i) {
    }
    join (thw, i, a);                           /* (The thing named: its words, the little ones before them passed) */
    if (!tidy ()) {
        msg (nm[0] ? "That name is too long." : "Name it what?");
        return NL_DONE;
    }
    if (i < a && ROOM (wd[i])) {                /* ---- A room (the one the character's in) */
        s = sight_get ();
        if (s->where != W_ROOM) {
            msg ("You can only name a room you're in.");
        } else if (!taken (dn, self = find (NM_ROOM, 0)) && (self || (self = free_one ()) != 0)) {
            memset (&e, 0, sizeof e);
            e.what = NM_ROOM;
            e.sn = s->stratum;
            e.x = s->rx;
            e.y = s->ry;
            e.w = s->rw;
            e.h = s->rh;
            put_name (self);
            sayf ("\nYou name this room %s.", dn);
        }
        return NL_DONE;
    }
    if (thw[0] && !denizen () && thing ()) {
        strncpy (nm, HUB->ireq.name, sizeof e.kind - 1);   /* ---- A thing (carried, or seen): its kind's name */
        nm[sizeof e.kind - 1] = 0;
        if (!taken (dn, self = find (NM_ITEM, 0)) && (self || (self = free_one ()) != 0)) {
            memset (&e, 0, sizeof e);
            e.what = NM_ITEM;
            e.x = fid;
            strcpy (e.kind, nm);
            put_name (self);
            mark ();                            /* (Marked named: the things' list says it) */
            othings (IO_NAME, thw, "", 1);
            sayf ("\nYou name your %s %s.", nm, dn);
        }
        return NL_DONE;
    }
    if (strchr (dn, ' ') || strlen (dn) > 11) {  /* ---- A denizen (the denizens' task's: its name a word) */
        msg ("An ally's name is a word of 11 letters at most.");
        return NL_DONE;
    }
    if (taken (dn, 0)) {
        return NL_DONE;
    }
    if (strlen (thw) + strlen (dn) + 6 <= INMAX) {  /* ("name the rat Grub", for the parser: the name last) */
        strcat (strcat (strcat (strcpy (l, "name "), thw), " "), dn);
    }
    return NL_DEN;
}

/* A name forgotten ("forget the name Linda", "unname my sword", "unname this room"), the line in nb's words */
static void forget (void)
{
    static unsigned char i;

    for (i = 1; i < nw && (LITTLE (wd[i]) || !stricmp (wd[i], "name") || !stricmp (wd[i], "names") ||
        !stricmp (wd[i], "alias")); ++i) {
    }
    join (thw, i, nw);
    if (!thw[0]) {
        msg ("Forget which name?");
        return;
    }
    if (!find (NM_ANY, thw) && !(ROOM (wd[i]) ? find (NM_ROOM, 0) != 0 : thing () && find (NM_ITEM, 0))) {
        msg ("You've given nothing that name.");
        return;
    }
    if (e.what == NM_DEN) {
        sayf ("\n%s keeps the name you gave it.", e.name);
        return;
    }
    sayf ("\nYou forget the name %s.", e.name);
    if (e.what == NM_ITEM) {                    /* (A thing's mark taken off, if it's to hand or in sight) */
        fid = e.x;
        mark ();
        othings (IO_NAME, thw, "", 0);
    }
    e.what = NM_NONE;
    ent_put ();
    count ();
}

/* A line typed that names (main.c's naming: name, call, alias, rename, dub; forget, unname): a name given or
** forgotten (NL_DONE: done here); a denizen's to be named (NL_DEN: names_den after the command) */
unsigned char __fastcall__ s_names_line (char* l)
{
    split (l);
    if (!nw) {
        return NL_NONE;
    }
    if (one_of (wd[0], "name\0\0\0call\0\0\0alias\0\0rename\0dub\0\0\0\0", 7)) {
        if (nw < 2) {
            msg ("Name what?");
            return NL_DONE;
        }
        return give (l);
    }
    if (!stricmp (wd[0], "forget") || !stricmp (wd[0], "unname")) {
        forget ();
        return NL_DONE;
    }
    return NL_NONE;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma code-name (push, "SW6CODE")            /* ---- Part 6: the words */
#pragma rodata-name (push, "SW6RODATA")

/* ---- The words */

/* The things' names in line l made the things' own ("#" and the id: the item store finds it by that), each where
** it's a word or words (the case aside); a fight's "with Linda" taken in hand first: the tenths that took (main.c
** charges them), or 0 */
unsigned __fastcall__ s_names_subst (char* l)
{
    static char id[8];
    static char* p;
    static const char* v;
    static unsigned char n, k;

    for (np = NAMES_AT; np < NAMES_AT + TNAMES; ++np) {
        ent ();
        if (e.what != NM_ITEM) {
            continue;
        }
        id[0] = '#';
        k = strlen (utoa (e.x, id + 1, 10)) + 1;
        for (p = l; *p; ++p) {
            if ((p == l || p[-1] == ' ') && (n = at_key (p, key (e.name))) != 0 && strlen (l) - n + k <= INMAX) {
                memmove (p + k, p + n, strlen (p + n) + 1);
                memcpy (p, id, k);
                cq_again = 1;                   /* (The command parser's to see it as it is now) */
            }
        }
    }
    for (v = "attack\0hit\0strike\0fight\0kill\0"; *v; v += strlen (v) + 1) {
        if (at_key (l, v) && (p = strstr (l, "with #")) != 0) {
            strncpy (id, p + 5, sizeof id - 1); /* (The thing to strike with, in hand first) */
            if ((p = strchr (id, ' ')) != 0) {
                *p = 0;
            }
            if (othings (IO_HOLD, id, "", 0) == IR_OK) {
                msg (lb);
                return othings_x;
            }
            break;
        }
    }
    return 0;
}

/* After a denizen's naming (NL_DEN's command): the name the denizens' task gave it, recorded (its old name's entry,
** or a new one, its kind the name it had) */
void s_names_den (void)
{
    struct sway* s = &HUB->sway;

    if (s->kind != SW_NAME || s->result != SR_OK || !s->word2[0]) {
        return;
    }
    if (!find (NM_DEN, s->name)) {
        if (!find (NM_NONE, 0)) {
            return;
        }
        e.what = NM_DEN;
        strncpy (e.kind, s->name, sizeof e.kind - 1);
    }
    strncpy (e.name, s->word2, sizeof e.name - 1);
    ent_put ();
    count ();
}

/* Words to be said (term.c's say: with a mark of the things', "#" and an id), as they're said: the things' list's
** "#12 a sword" "Linda, your sword"; a look's "lies a #12" "lies Linda, your sword"; else "#12" "Linda" (a name
** forgotten: its list's mark gone).  Into the main RAM (the window as it was put back) */
const char* __fastcall__ s_names_say (const char* s)
{
    static char r[NAME_LEN + 24];
    static char* p;
    static char* q;
    static unsigned char b, n;

    b = getbank ();
    strncpy (ob, s, sizeof ob - 41);
    ob[sizeof ob - 41] = 0;
    for (p = ob; *p; ++p) {
        if (*p != '#' || !isdigit (p[1])) {
            continue;
        }
        for (fid = 0, q = p + 1; isdigit (*q); ++q) {
            fid = fid * 10 + *q - '0';
        }
        r[0] = 0;
        if (!find (NM_ITEM, 0)) {
            if (*q != ' ') {
                continue;                       /* (Not a name's) */
            }
            ++q;                                /* (The list's mark of a name forgotten: gone) */
        } else {
            strcpy (r, e.name);
            if (*q == ' ') {                    /* (The list's: its words follow, "a sword") */
                q += !strncmp (q, " a ", 3) ? 3 : !strncmp (q, " an ", 4) ? 4 : 1;
                strcat (r, ", your ");
            } else if (p > ob + 1 && p[-1] == ' ' && (n = p[-2] == 'a' ? 2 : p[-2] == 'e' ? 4 : 0) != 0 &&
                p - ob >= n && !strncmp (p - n, n == 2 ? "a " : "the ", n)) {
                p -= n;                         /* (A look's, its article before it: its kind after it) */
                strcat (strcat (r, ", your "), e.kind);
            }
        }
        n = strlen (r);
        if (strlen (ob) - (q - p) + n >= sizeof ob) {
            break;
        }
        memmove (p + n, q, strlen (q) + 1);
        memcpy (p, r, n);
        p += n - 1;
    }
    setbank (b);
    return ob;
}

/* A look's words (look.c's): before them (after 0), its things named marked (the things' records' I_NAMED, and
** their ids: "#12", said as names_say says it); after them, the room's name, if it's named ("You are in the
** Mausoleum, a small crypt") */
void __fastcall__ s_names_look (unsigned char after)
{
    static struct item it;
    static struct sight* s;
    static struct sitem* q;
    static char* p;
    static unsigned char n;

    s = &sight_copy;
    if (!after) {
        for (q = s->item; q < s->item + s->nitems; ++q) {
            hub_map (ITEM_BANK (q->n));
            memcpy (&it, ITEM_AT (q->n), sizeof it);
            hub_map (0);
            fid = it.id;
            if ((it.flags & I_NAMED) && q->qty <= 1 && find (NM_ITEM, 0)) {
                q->name[0] = '#';
                utoa (fid, q->name + 1, 10);
                q->plural = 0;
                q->hid &= ~4;                   /* (Its words the name's: "Linda, your torch") */
            }
        }
        return;
    }
    p = !strncmp (lb, "You are in", 10) ? lb + 10 : !strncmp (lb, "You can see part of", 19) ? lb + 19 : 0;
    if (s->where != W_ROOM || s->dark || !p || !find (NM_ROOM, 0) ||
        strlen (lb) + (n = strlen (e.name) + 2) >= sizeof lb) {
        return;
    }
    memmove (p + n, p, strlen (p) + 1);         /* ("You are in" the Mausoleum, "a small crypt") */
    *p = ' ';
    memcpy (p + 1, e.name, n - 2);
    p[n - 1] = ',';
}

/* ---- The names kept: written to the save's fd (back 0), read back from it (1; fd -1: none, a new game's), a piece
** at a time through the main RAM */
#define PIECE       (TNAMES * sizeof (struct tname) / 6)
typedef char pieces[PIECE * 6 == TNAMES * sizeof (struct tname) && PIECE <= sizeof ob ? 1 : -1];

void __fastcall__ s_names_keep (int fd, unsigned char back)
{
    static char* at;

    for (at = (char*) NAMES_AT; at < (char*) (NAMES_AT + TNAMES); at += PIECE) {
        if (back) {
            if (fd < 0 || read (fd, ob, PIECE) != PIECE) {
                memset (ob, 0, PIECE);
            }
            hub_map (THEME_BANK);
            memcpy (at, ob, PIECE);
            hub_map (0);
        } else {
            hub_map (THEME_BANK);
            memcpy (ob, at, PIECE);
            hub_map (0);
            write (fd, ob, PIECE);
        }
    }
    count ();
}

/* A room named in text (its name's words there, the case aside, its "the" aside): its entry's number, the entry in
** named_room (its stratum, a square in it, its floor); or 0xFF.  (The orientation's: "go back to the mausoleum") */
unsigned char __fastcall__ s_room_named (const char* text)
{
    static unsigned char b;
    static const char* p;

    b = getbank ();
    strncpy (ob, text, INMAX);
    for (np = NAMES_AT; np < NAMES_AT + TNAMES; ++np) {
        ent ();
        for (p = ob; e.what == NM_ROOM && *p; ++p) {
            if ((p == ob || p[-1] == ' ') && at_key (p, key (e.name))) {
                memcpy (&named_room, &e, sizeof e);
                setbank (b);
                return np - NAMES_AT;
            }
        }
    }
    setbank (b);
    return 0xFF;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
