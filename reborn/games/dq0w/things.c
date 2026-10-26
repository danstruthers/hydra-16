/*
** things.c - (dq0w) the item store (DUNGEON.md: items; items.h), the world's alone (hub.h's who writes what): the
** catalogue, read into the hub as the world starts (kinds.c's, from its file: a stand-in for the theme's data), the
** theme's kinds after it (the UI's request); the items' records kept, moved as the UI asks (the hub's ireq: the
** character's kit, a thing taken up, put down; held in a hand, stowed in the pack, put in a container or on a
** surface, hung from the belt, taken out again, worn, taken off; lit, put out; a light renewed, a lantern filled, a
** torch dipped: slice 4.1's), listed (what the character carries,
** worded into the hub's text for the UI to say); the character's light, from the lit things it carries (the hub's
** pc_light and pc_lamp); a chunk's things put down as the maker listed them stocking it (the world bank's stock, its
** slot CS_FRESH), a denizen's remains as it dies (the world's: wbody.c); the things a look saw made their own.  The
** senses read the records (what the character perceives: dq0p/things.c), the UI a thing's it sees; only the world
** writes them, so there's no lock.  Paged (ipage.c): the records a cache of the things carried and the chunks'
** round the character, the rest by region in the save folder (no fixed count of things); kept with the game (dq0 -d):
** the store's banks as they are, and their chunks' table, the save folder's items.dat.  A kind's record copied out of the catalogue's bank as it's wanted; the code that handles things is
** there too (at the end, its words with it).
**
** Its main RAM's code is what everything calls on (a record copied out and back, a kind's, its words put in the
** hub's text); the rest is swapped parts' (../dq0/swap.inc, bank.s's stubs): part 10, the requests answered and a
** thing handled (a thing's name matched, where it lies, whose it is: what the catalogue's bank's code calls on there,
** through its stubs); part 11, what's carried listed, a tool's wear (those two called from the main RAM at once,
** not through part 10's serve: an inventory's, a dig's every spell's), a search, a thing described; part 13 the
** rarer work (the store begun, kept, the kit, the theme's kinds, a chunk's things and the remains put down, a thing
** hidden), and a thing made its own (the kit's ten, the things a look saw); part 17 the store's paging (ipage.c:
** its chunks' things read in, put out; begun, kept); part 27 (dq0w.c's, the lights burning: its room) a light renewed.
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

/* ---- The swapped parts' stubs (bank.s's: ../dq0/swap.inc, their code run in the main RAM's top, a part at a time,
** copied there as it's called from their bank) */

void light_now (void);                          /* Part 10 */
void chart_now (void);                          /* Part 26 */
unsigned char __fastcall__ named (const char* k, const char* w);
unsigned char __fastcall__ on_pc (unsigned i);
unsigned char __fastcall__ in_reach (unsigned n);
unsigned __fastcall__ weight_in (unsigned where, unsigned x);
unsigned __fastcall__ weight_of (unsigned i);
unsigned __fastcall__ bulk_in (unsigned x);
unsigned __fastcall__ bulk_of (unsigned i);
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);
void about (void);
void list (void);                               /* Part 11 */
void __fastcall__ wear (const char* key, unsigned char by);
unsigned char search (void);                    /* Part 46 */
unsigned char seek (void);
void __fastcall__ thing_words (const struct item* q);   /* (Part 11's) */
unsigned char __fastcall__ hide (const char* name);
void __fastcall__ make_own (unsigned i);         /* Part 13 */
void seen_owned (void);
void kit (void);
void theme_kinds (void);
unsigned __fastcall__ pick (const char* name, unsigned char how);    /* Part 4 (its room): a thing of several */
unsigned __fastcall__ at_hand (const char* name);
unsigned __fastcall__ carried_named (const char* name);
unsigned __fastcall__ seen_named (const char* name);
unsigned char __fastcall__ moved (unsigned char op);
unsigned char shut_work (void);
unsigned char inside (void);                    /* Part 36 */
unsigned char look_at (void);                   /* Part 39 */
unsigned char __fastcall__ describe (const char* name);    /* Part 40 */
unsigned __fastcall__ shut_in (unsigned n);     /* Part 35 */
unsigned __fastcall__ s_shut_in (unsigned n);
void __fastcall__ many (char* b, unsigned n, unsigned char k);    /* Part 35 */
void __fastcall__ s_many (char* b, unsigned n, unsigned char k);  /*   (its own, there) */
unsigned char bar_hand (void);                  /* Part 35 */
unsigned char __fastcall__ hold_in (unsigned n, unsigned char want);    /* Part 48 */
unsigned char swap (void);
void __fastcall__ listed (unsigned n);          /* Part 11 */
#define HW_CARRIED      1                       /* (pick's how: carried; seen, lying about; either, as far as it sees,
                                                **   or to hand, within reach (afar else); in win) */
#define HW_SEEN         2
#define HW_ANY          3
#define HW_HAND         4
#define HW_IN           5

#define NK      NKINDS                          /* (The catalogue's, and the theme's after them: theme.h) */

extern const struct kind kinds[];               /* (kinds.c's: in the catalogue's bank) */
extern const unsigned char throws[];            /* (kinds.c's: a kind's reach, thrown) */
unsigned char __fastcall__ throw_at (const char* name);    /* Part 4 */

/* A buffer in the main RAM (the words said, the kit's names, the store kept a piece at a time) */
static char tbuf[80];

/* A thing's record, copied out: the one the functions share that call none that wants it while they use it (others
** have their own: make_own's, the recursive ones', light's ...) */
static struct item rec;

/* (item_at and items are items.s's, in assembly: its store's layout as here) */
typedef char store_fits[sizeof (struct item) == 25 && ITEM_BANK (0) == 14 && ITEM_BANKS == 2 ? 1 : -1];

/* A kind's record, copied out of the catalogue's bank (into one of four, in turn: an expression may want two at
** once): the catalogue's bank left in the window */
static const struct kind* __fastcall__ kind_of (unsigned char k)
{
    static struct kind kb[4];
    static unsigned char next;
    struct kind* r = &kb[next++ & 3];

    hub_map (CAT_BANK);
    memcpy (r, kinds + (k < NK ? k : 0), sizeof *r);
    return r;
}

/* Item i's record copied out (into q); item i's kind's record (kind_of's) */
static void __fastcall__ fetch (struct item* q, unsigned i)
{
    struct item* p = item_at (i);

    if (!p->kind) {                             /* (A free record: only its kind, none, the rest unread; a look
                                                **   about goes through every record, and most are free) */
        q->kind = 0;
        return;
    }
    memcpy (q, p, sizeof *q);
}

static const struct kind* __fastcall__ kind_at (unsigned i)
{
    return kind_of (item_at (i)->kind);
}

/* How kind k burns (kinds.c's burns: a light's life and its fuels, a fuel's measure), copied out: 0 if it's in none
** (a light that never burns down, a glow; a thing that's no fuel); the catalogue's bank left in the window */
const struct burn* __fastcall__ burn_of (unsigned char k)
{
    static struct burn b;
    const struct burn* p;

    hub_map (CAT_BANK);
    for (p = burns; p < burns + NBURNS; ++p) {
        if (p->kind == k) {
            b = *p;
            return &b;
        }
    }
    return 0;
}

/* The hands a thing wants: both for a two-handed one, or a big one */
static unsigned char __fastcall__ two (const struct kind* k)
{
    return k->flags & K_TWO || k->bulk >= 20 ? 2 : 1;
}

unsigned char held_k[2], held_h[2];             /* (What the character holds: part 48's held_kinds) */

/* The catalogue's bank in the window (bank.s's m0 ... put it back with it) */
void cat_map (void)
{
    hub_map (CAT_BANK);
}

/* The UI's request answered (serve's, part 10's; but what's carried listed, and a tool's wear, by their own part,
** 11, alone: an inventory's, a dig's every spell's) */
unsigned char things_req (void)
{
    unsigned char k;

    hub_map (0);
    if ((k = HUB->ireq.kind) == IO_WEAR) {
        strcpy (tbuf, HUB->ireq.name);          /* (Copied: the hub's window moves) */
        wear (tbuf, HUB->ireq.to);
    } else if (k == IO_LIST) {
        list ();
    } else {
        return serve ();
    }
    hub_map (0);
    HUB->ireq.result = IR_OK;
    return 0;
}

/* The things a look saw (the senses' record's) made their own, as the senses did as they saw them (part 11's, if
** there are any) */
void owned (void)
{
    unsigned char n;

    hub_map (SIGHT_BANK);
    n = SIGHT->nitems;
    hub_map (0);
    if (n) {
        seen_owned ();
    }
}

/* ---- The hub's text (things_text, the UI bank's): the words put after what's there, the text begun again */
static char* tp;
static char* tq;                                /* (Or into the main RAM, on to tqe: things' words compared, alike:
                                                **   part 11's) */
static char* tqe;
static void __fastcall__ put (const char* s)
{
    if (tq) {
        while (*s && tq < tqe) {
            *tq++ = *s++;
        }
        *tq = 0;
        return;
    }
    hub_map (UI_BANK);                          /* (The text the UI bank's: the header back after) */
    while (*s && tp < UIREC->things_text + sizeof UIREC->things_text - 1) {
        *tp++ = *s++;
    }
    *tp = 0;
    hub_map (0);
}

static void text_new (void)
{
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    *tp = 0;
    hub_map (0);
}

/* Item i's record written back (from q); item i's kind (its number) */
static void __fastcall__ store (const struct item* q, unsigned i)
{
    memcpy (item_at (i), q, sizeof *q);
}

static unsigned __fastcall__ item_kind (unsigned i)
{
    return item_at (i)->kind;
}


/* ---- A thing of several (the user's, 2026-10-21: "if I have 3 sacks, and I say put the sack in the sack ..."): what
** the chooser (part 4's pick) chooses by: the selector (the request's sel, or its osel for the other: SL_*, its n times
** 16), the thing it mustn't be nor be in (the first, as the other's chosen), the thing it must be in (HW_IN's; else
** one it mustn't be in already: a count's put in it), the one it's to be (a count's next, a part of a lot split off) */
unsigned char wsel;
unsigned wnot, win, wforce;
unsigned handled;                               /* (The thing a request handled last: a count's, as each is moved) */
unsigned csum;                                  /* (pick's: how many of those it chose from, all told) */
unsigned rtime, roff;                           /* (The reach owed, part 10's: its time, the pack taken off for it, */
unsigned char rsteps, rbody;                    /*   its steps; the main RAM's, part 35's too) */

/* The request, copied (serve's), and what its handling shares (the hands, the pack, the belt: about's; the last put
** down; how far the thing named is, seen_named's; out of reach, at_hand's; the things in a container, bulk_in's; the
** time it took) */
static struct ireq iq;
static unsigned pack, belt, dropped, seen_d, took;
static unsigned char afar, nin;
static unsigned far_n, far_x, far_y;            /* (The thing at_hand found out of reach, and where it lies) */
static char xn[16], yn[16];                     /* (The two names said: the words asked, then the things'; the
                                                **   catalogue's bank's code's, and a count's words) */
static unsigned char last_sa;                   /* (The words the catalogue's code said last: SA_*) */
static unsigned cont;                           /* (What a thing was put in or on last: stow's) */
static void __fastcall__ say (unsigned char n);   /* (The catalogue's bank's: its words, SA_*) */

/* ==== Part 10: the requests answered, a thing handled (and what the catalogue's bank's code calls on: its stubs) */
#pragma code-name (push, "SW10CODE")
#pragma rodata-name (push, "SW10RODATA")

/* A name said: a word of the kind's ("key" an iron key; "sword" a broken one too), or its start; or that word made
** plural ("sacks", "torches": "drop two sacks"; the catalogue's bank's code's through its stub) */
static unsigned char __fastcall__ word_of (const char* k, const char* w, unsigned char n)
{
    for (;;) {
        if (!strncmp (k, w, n)) {
            return 1;
        }
        if (!(k = strchr (k, ' '))) {
            return 0;
        }
        ++k;
    }
}

unsigned char __fastcall__ s_named (const char* k, const char* w)
{
    unsigned char n = strlen (w);

    return word_of (k, w, n) || (n > 2 && w[n - 1] == 's' && (word_of (k, w, n - 1) ||
        (w[n - 2] == 'e' && word_of (k, w, n - 2))));
}


/* Where a thing lies about: on a square, or on a surface that is (a table's): the record on the square (its own, or
** the surface's, copied), or 0 */
static const struct item* __fastcall__ lying (const struct item* q)
{
    static struct item s;

    if (q->where == IW_SQUARE || q->where == IW_PORTAL) {
        return q;                               /* (A portal's part as a thing lying there) */
    }
    if (q->where != IW_ITEM) {
        return 0;
    }
    fetch (&s, q->x);
    return s.where == IW_SQUARE && (kind_of (s.kind)->keeps & KF_ON) ? &s : 0;
}

/* The character's (on it, or in what it carries) */
unsigned char __fastcall__ s_on_pc (unsigned i)
{
    unsigned j;

    for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {         /* (Up to whoever has it) */
    }
    return item_at (j)->where == IW_ACTOR && !item_at (j)->x;
}

/* Whether thing n can go in c (or on it: a surface; hung from it: a belt): 0, or the words why not (SA_*): the
** catalogue's bank's o_fits */
static unsigned char __fastcall__ o_fits (unsigned n, unsigned c);
static unsigned char __fastcall__ fits (unsigned n, unsigned c)
{
    cat_map ();
    return o_fits (n, c);
}


/* Within reach of the character (on a square next to it, or its own; or on a surface there) */
unsigned char __fastcall__ s_in_reach (unsigned n)
{
    const struct item* s;
    unsigned x, y;

    fetch (&rec, n);
    s = lying (&rec);
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    if (s) {
        far_x = s->x;                           /* (Where it lies: at_hand's, out of reach) */
        far_y = s->y;
    }
    return s && s->x - x + 1 <= 2 && s->y - y + 1 <= 2;
}

/* ---- What's in a thing, or on the character: the bulk and weight (quarter pounds) of what's in it (a thing's own
** with what's in it, or on it: a soft thing's bulk its own or what it holds, the more) */
unsigned __fastcall__ s_weight_in (unsigned where, unsigned x);
unsigned __fastcall__ s_bulk_in (unsigned x);
unsigned __fastcall__ s_weight_of (unsigned i)
{
    const struct kind* k = kind_at (i);
    unsigned w = (unsigned) k->weight * (k->flags & K_PLURAL && !(k->form & KF_COUNT) ? 1 : item_at (i)->qty);  /* (A
                                                **   lot counted, its count's: "drop two rations") */

    if (k->flags & (K_CONTAINER | K_BELT) || k->keeps & KF_ON) {
        w += s_weight_in (IW_ITEM, i);
    }
    return w;
}

unsigned __fastcall__ s_weight_in (unsigned where, unsigned x)
{
    unsigned i = 0, w = 0;

    while ((i = items (i, IN_X | 1 << where, x)) != 0) {
        w += s_weight_of (i);
    }
    return w;
}

unsigned __fastcall__ s_bulk_of (unsigned i)
{
    const struct kind* k = kind_at (i);
    unsigned b = k->bulk, c;

    if (k->flags & K_CONTAINER && (c = s_bulk_in (i)) > b) {
        b = c;
    }
    return b;
}

unsigned __fastcall__ s_bulk_in (unsigned x)
{
    unsigned i = 0, b = 0;
    unsigned char n = 0;

    while ((i = items (i, IN_X | IN_ITEM, x)) != 0) {
        b += s_bulk_of (i);
        ++n;
    }
    nin = n;                                    /* (The outermost call's, set last) */
    return b;
}

/* ---- The character's pack, its belt (about's), and where a thing will go */
void s_about (void)
{
    const struct kind* k;
    unsigned i = 0;

    pack = belt = 0;
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        fetch (&rec, i);
        k = kind_of (rec.kind);
        if ((rec.flags & I_WORN) && (k->flags & K_CONTAINER) && !pack) {
            pack = i;
        }
        if ((rec.flags & I_WORN) && (k->flags & K_BELT) && !belt) {
            belt = i;
        }
    }
}

/* Thing n put somewhere (where, x: items.h's), as it's flagged (held, worn, or neither), no longer hidden (nor
** tended: a light its keepers kept burning, taken up, burns down) */
static void __fastcall__ place (unsigned n, unsigned char where, unsigned x, unsigned char flags)
{
    struct item* p = item_at (n);

    p->where = where;
    p->x = x;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | flags;
}

/* Thing n carried: into the pack if it fits, else in a hand, the one that suits it (part 48's: hand first, the other
** way about): 1 in the pack, 2 in a hand, 0 neither (full) */
unsigned char __fastcall__ hand_free (unsigned n);
void __fastcall__ s_reach_to (unsigned c);
unsigned char __fastcall__ s_carry (unsigned n, unsigned char hand_first)
{
    unsigned char h;

    s_about ();
    h = hand_free (n);
    if ((!hand_first || !h) && pack && !fits (n, pack)) {
        place (n, IW_ITEM, pack, 0);
        s_reach_to (pack);                      /* (Put away: its reach, HOUSE.md's) */
        return 1;
    }
    if (h) {
        place (n, IW_ACTOR, 0, I_HELD);
        item_at (n)->sn = h;
        return 2;
    }
    return 0;
}

/* ---- The reach (HOUSE.md's: the user's, 2026-10-23, "Handy would be readily available, so in a backpack that is being
** worn is not as readily available as one in a belt pouch"): how readily a thing comes to hand, by where it is, for
** every taking out, putting away, look inside, search and tool's use, every actor's.  In hand, at once; on the body
** (worn, hung from a belt, in what's open to the hand: open_hand's) or lying within reach, a quick draw, no time more
** than the handling's own; in a container (held, set down, in another), R_IN a level; in a pack worn on the back,
** taken off first and put on again after, R_OFFON and its level, once a handling however often it's reached into.
** Engaged, a round a step (the book's grope, a turn: the readiest a turn, each level deeper a turn more, a pack's
** taking off and its putting on again a turn each).  What's owed (rtime, rsteps, rbody) is added to the character's
** next spell of time as the handling's done (reach_done: dq0w.c's busy, its pextra) */
#define R_IN            20                      /* (Tenths: a container's level, reached into) */
#define R_OFFON         100                     /* (A pack on the back taken off, and put on again) */
unsigned char __fastcall__ zone_step (unsigned ox, unsigned oy, unsigned x, unsigned y);  /* (dq0w.c's part 24) */

#pragma static-locals (push, on)

/* What reaching into container c is (rec its record, copied: what holds it, lw and lx): 0 open to the hand, a quick
** draw (not a container: a belt, a surface; one that hangs from a belt, a pouch, a quiver, a sheath; one worn, but a
** pack on the back), 1 a container's level (held, set down, in another; or closed), 2 a pack on the back (the
** catalogue's KF_BACK: off, reached into, on again) */
static unsigned char lw;
static unsigned lx;
static unsigned char __fastcall__ level_of (unsigned c)
{
    static const struct kind* k;
    static unsigned char f;

    fetch (&rec, c);
    lw = rec.where;
    lx = rec.x;
    k = kind_of (rec.kind);
    f = k->flags;
    if (rec.shut & IS_CLOSED) {
        return 1;
    }
    if (!(f & K_CONTAINER) || (f & K_HANG)) {
        return 0;
    }
    return !(rec.flags & I_WORN) ? 1 : k->form & KF_BACK ? 2 : 0;
}

/* Whether thing i is handy (HOUSE.md's handy tools: in hand, or on the body, in nothing that isn't open to the hand):
** 0 if it is, else the outermost thing it's in that isn't (a pack worn, a sack: part 26's map making's words) */
unsigned __fastcall__ s_handy (unsigned i)
{
    static unsigned c;

    for (c = 0; fetch (&rec, i), rec.where == IW_ITEM; ) {
        if (level_of (i = rec.x)) {
            c = i;
        }
    }
    return c;
}

/* Container c reached into (a thing put in it, or got out of it, looked for in it), and what holds it, out to whoever
** has it or where it lies */
void __fastcall__ s_reach_to (unsigned c)
{
    static unsigned char l;

    for (;;) {
        if (!(l = level_of (c))) {
            rbody = 1;
        } else if (l == 2) {
            if (roff != c) {                    /* (A pack on the back: off, reached into, on again, once) */
                roff = c;
                rtime += R_OFFON + R_IN;
                rsteps += 3;
            }
            return;
        } else {
            rtime += R_IN;
            ++rsteps;
        }
        if (lw != IW_ITEM) {
            return;
        }
        c = lx;
    }
}

/* Thing n got out, to hand (a hand's, held, at once; worn, or lying within reach, a quick draw) */
void __fastcall__ s_reach_from (unsigned n)
{
    fetch (&rec, n);
    if (rec.where == IW_ITEM) {
        s_reach_to (rec.x);
    } else if (!(rec.flags & I_HELD)) {
        rbody = 1;
    }
}

/* What's owed, done: added to the character's next spell of time (engaged, a round a step: zone_step's), and, said, a
** pack taken off for it told ("You take off your backpack for it, and put it on again."); then none owed */
static void rclear (void)
{
    rtime = roff = 0;
    rsteps = rbody = 0;
}

unsigned __fastcall__ s_reach_done (unsigned char said)
{
    static unsigned char s;
    static unsigned t;

    hub_map (0);
    if ((s = rsteps + rbody) != 0 && zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y)) {
        hub_map (0);
        rtime = s * HUB->round;
    }
    pextra += t = rtime;
    if (roff && said) {
        put (" You take off your ");
        put (kind_at (roff)->name);
        put (" for it, and put it on again.");
    }
    rclear ();
    return t;
}

/* A flame to hand (IO_LIGHT's, the catalogue's bank's light; part 10's, its room): a thing lit, carried or within
** reach; or a tinderbox carried, its reach the lighting's (HOUSE.md's handy tools): 1, or 0 */
unsigned char s_flame (void)
{
    static unsigned i, t;

    for (i = t = 0; (i = items (i, IN_SQUARE | IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0; ) {
        if (ip->flags & I_LIT) {
            if (s_on_pc (i) || s_in_reach (i)) {
                return 1;
            }
        } else if (!t && s_on_pc (i) && !strcmp (kind_at (i)->tool, "tinder")) {
            t = i;
        }
    }
    if (t) {
        s_reach_from (t);
    }
    return t != 0;
}

#pragma static-locals (pop)

/* ---- The tools: the keys of all the character carries (anywhere on it: a skill used, the one that suits takes
** itself out), into the hub's things_text, each after its mark (held: < in the left hand, > the right, * both; + worn;
** a space neither: what's held fights, what's worn guards) and its quality after it (a digit: rs.h's RS_EQUIP), and its load (the weight of all it
** carries, worn or not, in quarter pounds: the referee's encumbrance) in the request's x, the hands that hold
** something (DH_*, slice 4.1b's) in y; and one used, worn (the
** referee's choice, by its key), its name back in the request (bare hands: "hands") */
static void tools (void)
{
    static const char hmark[4] = " <>*";
    char m[2];
    const struct kind* k;
    unsigned i = 0;
    char* t;
    unsigned char mh, hh = 0;

    m[1] = 0;
    hub_map (0);
    mh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;

    hub_map (UI_BANK);
    UIREC->things_text[0] = 0;
    while ((i = items (i, IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {
        fetch (&rec, i);
        if ((rec.flags & I_HELD) && rec.where == IW_ACTOR && !rec.x) {
            hh |= rec.sn & IH_BOTH ? rec.sn & IH_BOTH : mh;     /* (Its hands that hold something, any thing) */
        }
        k = kind_of (rec.kind);                 /* (Copied: the hub's text is in another bank) */
        if (k->tool[0] && s_on_pc (i)) {
            hub_map (UI_BANK);
            t = UIREC->things_text;
            if (strlen (t) < sizeof UIREC->things_text - 11) {
                m[0] = rec.flags & I_HELD ? hmark[rec.sn & IH_BOTH ? rec.sn & IH_BOTH : mh] : rec.flags & I_WORN ? '+' :
                    ' ';
                strcat (t, m);
                strcat (t, k->tool);
                m[0] = '0' + rec.quality;
                strcat (t, m);
            }
        }
    }
    i = s_weight_in (IW_ACTOR, 0);
    hub_map (0);
    HUB->ireq.x = i;
    HUB->ireq.y = hh;
}

/* What's in the character's hands (IO_HANDS: a blow with nothing named, the UI's), their kinds in the request's x, its
** main hand's (or a thing's in both), and y, its other's (0 none: held_kinds') */
static void hands_held (void)
{
    held_kinds ();
    HUB->ireq.x = held_k[0];
    HUB->ireq.y = held_k[1];
}

/* The character's light: the brightest lit thing it carries (in a hand, worn, hung from its belt: one stowed is
** put out), into the hub, named (its minutes left the lights' burning's: part 27's); and its map making with it */
void s_light_now (void)
{
    char lamp[14];
    const struct kind* k;
    unsigned i = 0;
    unsigned char best = 0;

    lamp[0] = 0;
    while ((i = items (i, IN_FLAGS | IN_ACTOR | IN_ITEM | IN_PORTAL, I_LIT)) != 0) {
        fetch (&rec, i);
        if (s_on_pc (i)) {
            k = kind_of (rec.kind);
            if (k->light > best) {
                best = k->light;
                strcpy (lamp, k->name);
            }
        }
    }
    hub_map (0);
    HUB->pc_light = best;
    strcpy (HUB->pc_lamp, lamp);
    chart_now ();
}

/* ---- A request (the hub's ireq), answered (but for its state, the caller's: done once it's looked again): 1 if
** what lies about has changed */
static unsigned char o_handle (void);
unsigned char s_serve (void)
{
    unsigned char changed = 0, k;
    unsigned n;

    rclear ();                                  /* (No reach owed yet) */
    hub_map (0);
    memcpy (&iq, &HUB->ireq, sizeof iq);
    iq.result = IR_OK;
    wsel = iq.sel;                              /* (Which of several: the chooser's, part 4's pick) */
    wnot = win = wforce = 0;
    switch (iq.kind) {
    case IO_KIT:
        kit ();
        break;
    case IO_GET:                                /* (Taken up, put down: one, or as many as it says, part 4's) */
    case IO_DROP:
        if ((iq.result = moved (iq.kind)) == IR_NO && iq.kind == IO_GET) {
            cat_map ();                         /* (Fixed where it stands: the words why) */
            o_handle ();
        }
        break;
    case IO_SHUT:                               /* (A lid, a tie, a stopper, a lock worked: part 4's) */
        iq.result = shut_work ();
        break;
    case IO_INSIDE:                             /* (What's in it, or on it: part 11's) */
        iq.result = inside ();
        break;
    case IO_THROW:
        iq.result = throw_at (iq.name);
        break;
    case IO_LIST:
        list ();
        break;
    case IO_TOOLS:
        tools ();
        break;
    case IO_HANDS:
        hands_held ();
        break;
    case IO_SWAP:                               /* (What's in its hands changed over: part 48's) */
        iq.result = swap ();
        break;
    case IO_WEAR:
        wear (iq.name, iq.to);
        break;
    case IO_SEARCH:
        changed = iq.result = search ();
        break;
    case IO_SEEK:                               /* (A search's thing, for or on: part 46's) */
        iq.result = seek ();
        break;
    case IO_DESCRIBE:
        iq.result = describe (iq.name);
        break;
    case IO_LOOK:                               /* (A thing looked at, as anyone sees it: part 39's) */
        iq.result = look_at ();
        break;
    case IO_HIDE:
        iq.result = hide (iq.name);
        break;
    case IO_FILL:
        iq.result = fill ();                    /* (A light renewed: its time in the request's x, part 27's) */
        break;
    case IO_THEME:
        theme_kinds ();
        break;
    case IO_BEGIN:
        break;
    default:                                    /* (A thing handled: IO_HOLD ...) */
        afar = 0;
        k = iq.kind;
        if (!noinit && k >= IO_HOLD && k <= IO_TAKE && (n = at_hand (iq.name)) != 0 && fumbled (item_kind (n))) {
            hub_map (0);                        /* (Held, stowed, put or taken out engaged, a grope failed: the round
                                                **   wasted, nothing done, its words) */
            took = HUB->round;
        } else if (k >= IO_STOW && k <= IO_TAKE) {
            if ((iq.result = moved (k)) == IR_OK && k != IO_TAKE) {
                s_reach_to (cont);              /* (Stowed, put, taken from: one, or as many as it says; what it went
                                                **   in reached into, once) */
            }
        } else {
            cat_map ();
            iq.result = o_handle ();
        }
        hub_map (0);
        HUB->ireq.x = took;
        if (iq.result == IR_NO && afar) {       /* (Its thing out of reach: where it lies, the UI's to walk over to
                                                **   it first, I_TAKE's walk alone, and ask again) */
            iq.result = IR_NOT_HERE;
            HUB->ireq.x = far_x;
            HUB->ireq.y = far_y;
            HUB->ireq.item = far_n;
        }
        break;
    }
    if (iq.kind == IO_KIT || iq.kind == IO_GET || iq.kind == IO_DROP || iq.kind == IO_HIDE || iq.kind == IO_THROW ||
        (iq.kind >= IO_HOLD && iq.kind <= IO_NAME)) {
        s_light_now ();                         /* (Its light: what it holds lit) */
        changed |= iq.result == IR_OK;
    }
    if (iq.result == IR_OK && (iq.kind == IO_GET || (iq.kind >= IO_HOLD && iq.kind <= IO_DOUSE))) {
        sconce_lights (1);                      /* (A sconce's torch taken, put in, lit, put out: its room's light) */
    }
    if (iq.result == IR_OK && iq.kind != IO_KIT && iq.kind != IO_HIDE &&
        (n = s_reach_done (iq.kind != IO_GET && iq.kind != IO_DROP)) != 0 && iq.kind != IO_THROW) {
        hub_map (0);                            /* (Its reach owed, the character's next spell's, and in the reply's y
                                                **   (the tests'; a throw's where it lies); a pack taken off for it told,
                                                **   but a thing taken up's or put down's: the UI's words alone) */
        HUB->ireq.y = n;
    }
    rclear ();
    hub_map (0);
    HUB->ireq.result = iq.result;
    return changed && iq.kind != IO_KIT;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 26 (dq0w.c's, its room): the map making, as the light (slice 4.2's; the world's parts are 51 at most,
** swap.inc's table) */
#pragma code-name (push, "SW26CODE")
#pragma rodata-name (push, "SW26RODATA")

/* The character's map making (the hub's pc_chart and pc_map: DUNGEON.md, the map kept; HOUSE.md's), by what it
** carries (anywhere on it): the things to draw a map with to hand, the catalogue's by their keys (a sheet to draw on,
** "sheet", or its map, "map"; a point that draws, "draw", or a quill, "quill", and ink, "ink"), and a hand to draw
** with (free, or holding one of them; its arm not useless); and the first map it carries.  Mapping as it goes with
** them, and no map, its first sheet is made the map (the kind keyed "map").  Part 26's (its room), called as the
** light is (s_light_now's) */
void s_chart_now (void)
{
    static const char keys[5][6] = { "sheet", "map", "draw", "quill", "ink" };
    const struct kind* k;
    unsigned i = 0, j, sheet = 0, map = 0;
    unsigned char has = 0, busy, mh, h, f;

    hub_map (0);
    mh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    busy = (HUB->pcsum.limbs & LB_ARM_L ? IH_LEFT : 0) | (HUB->pcsum.limbs & LB_ARM_R ? IH_RIGHT : 0);
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        fetch (&rec, i);
        for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {   /* (On it, or in what it carries) */
        }
        if (item_at (j)->where != IW_ACTOR || item_at (j)->x) {
            continue;
        }
        k = kind_of (rec.kind);
        for (f = 0; f < 5 && strcmp (k->tool, keys[f]); ++f) {
        }
        h = (rec.flags & I_HELD) && rec.where == IW_ACTOR ? (rec.sn & IH_BOTH ? rec.sn & IH_BOTH : mh) : 0;
        if (f == 5) {
            busy |= h;                          /* (A hand that holds something else) */
            continue;
        }
        has |= 1 << f;
        if (!f && !sheet) {
            sheet = i;
        } else if (f == 1 && !map) {
            map = i;
        }
    }
    f = (has & 3) && ((has & 4) || (has & 24) == 24) && (busy & IH_BOTH) != IH_BOTH;
    hub_map (0);
    if (f && !map && sheet && (HUB->watch & WA_MAP)) {
        for (h = 1; h < NK && strcmp (kind_of (h)->tool, keys[1]); ++h) {
        }
        if (h < NK) {
            item_at (sheet)->kind = h;          /* (Its first sheet made the map) */
            make_own (sheet);
            map = sheet;
        }
    }
    i = map ? item_at (map)->id : 0;
    hub_map (0);
    HUB->pc_chart = f ? CH_DRAW : 0;
    HUB->pc_map = i;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 4 (store.c's, its room): a thing thrown */
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* A thing thrown (the UI's IO_THROW, the character's: the same for every actor, as their minds come to throw), the
** one named that it carries ("": the first in its hands), at a mark (the request's x, y: a square it sees, a denizen's
** or a thing's), how (the request's to: struct cmd's manner, MN_HARD), as the plan has it (anything throwable thrown
** by the same code): a throwable thing's reach (the catalogue's throws: the short band's squares, kinds.c's) and
** HarnMaster's missile ranges (Combat 15: short, medium, long and extreme, each twice the last, at 0, 20, 40 and 80
** less; thrown hard, twice as far, and 10 less), the referee's Throwing test (P_THROW, the band's steps): it lands on
** the mark, hit or missed (a miss falls there, by it), or short of it, beyond the extreme band (along the line to it:
** squares it sees); out of the thrower's hands.  IR_OK (how it went in the request's qty, TH_*; where it lies in x,
** y), IR_NONE (none carried), IR_NO (not a thing to throw) */
unsigned char __fastcall__ s_throw_at (const char* name)
{
    static unsigned char reach, band, d, sn;
    static unsigned px, py;
    static int dx, dy;
    static signed char steps, g;
    struct req* r;
    unsigned n = 0;

    if (*name) {
        n = carried_named (name);
    } else {
        while ((n = items (n, IN_FLAGS | IN_ACTOR, I_HELD)) != 0 && !on_pc (n)) {
        }
    }
    if (!n) {
        return IR_NONE;
    }
    reach_from (n);                             /* (Got out first: its reach, HOUSE.md's) */
    fetch (&rec, n);
    cat_map ();
    reach = throws[rec.kind < KI_COUNT ? rec.kind : 0];
    if (!reach) {
        return IR_NO;
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    px = HUB->pc_x;
    py = HUB->pc_y;
    dx = iq.x - px;
    dy = iq.y - py;
    d = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    steps = 0;
    if (iq.to & MN_HARD) {
        reach <<= 1;
        steps = -2;
    }
    for (band = 0; band < 4 && d > (reach << band); ++band) {
    }
    iq.qty = TH_SHORT;
    if (band == 4) {                            /* (Too far: it falls at the extreme band's end, on the line) */
        iq.x = px + dx * (reach << 3) / d;
        iq.y = py + dy * (reach << 3) / d;
    } else {
        r = &HUB->slot[SLOT_WORLD];
        r->kind = RS_TEST;
        r->actor = 0;
        r->a = P_THROW;
        r->b = 0;
        r->steps = steps - 4 * band;            /* (20 less a band: 4 steps of 5) */
        r = hub_ask (SLOT_WORLD);
        g = r->grade;
        r->state = REQ_FREE;
        iq.qty = G_OK (g) ? TH_HIT : TH_MISS;
    }
    item_at (n)->where = IW_SQUARE;             /* (Out of its hands: where it falls) */
    item_at (n)->sn = sn;
    item_at (n)->x = iq.x;
    item_at (n)->y = iq.y;
    item_at (n)->flags &= ~(I_HELD | I_WORN);
    dropped = n;
    hub_map (0);
    HUB->ireq.qty = iq.qty;
    HUB->ireq.x = iq.x;
    HUB->ireq.y = iq.y;
    return IR_OK;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 4 (its room): a thing of several chosen (the user's, 2026-10-21: "There is also no way to disambiguate
** between which item of multiple that you want to use"); things moved by the count ("drop two sacks", "take 4
** rations"), a lot split and joined again; what closes worked (a lid, a tie, a stopper, a lock: "open the chest") */
#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")
#pragma static-locals (push, on)              /* (None called again before it's done: their code the less) */

/* A name said: a word of the kind's ("key" an iron key; "sword" a broken one too), or its start; or that word made
** plural ("sacks", "torches": "drop two sacks"; part 10's named, a copy: the chooser's, every record's, no swap) */
static unsigned char __fastcall__ word_of4 (const char* k, const char* w, unsigned char n)
{
    for (;;) {
        if (!strncmp (k, w, n)) {
            return 1;
        }
        if (!(k = strchr (k, ' '))) {
            return 0;
        }
        ++k;
    }
}

static unsigned char __fastcall__ named4 (const char* k, const char* w)
{
    unsigned char n = strlen (w);

    return word_of4 (k, w, n) || (n > 2 && w[n - 1] == 's' && (word_of4 (k, w, n - 1) ||
        (w[n - 2] == 'e' && word_of4 (k, w, n - 2))));
}

/* The thing in rec (fetched) named so: by its kind's name (named's), or by the name the character gave it (the UI's
** "#" and its id: hub.h's struct tname) */
static unsigned char __fastcall__ is_named (const char* w)
{
    char b[6];

    return *w == '#' ? !strcmp (utoa (rec.id, b, 10), w + 1) : named4 (kind_of (rec.kind)->name, w);
}

/* How thing i lies, for the chooser: how deep it is in others (0: in none), its outermost (on the character, on a
** square) in ctop; cnot, it's the one it mustn't be (wnot) or in it */
static unsigned ctop;
static unsigned char cnot;
static unsigned char __fastcall__ chain (unsigned i)
{
    const struct item* p;
    unsigned char d = 0;

    for (cnot = 0; ; ) {
        cnot |= i == wnot;
        if ((p = item_at (i))->where != IW_ITEM) {
            break;
        }
        i = p->x;
        ++d;
    }
    ctop = i;
    return d;
}

/* ---- The thing named (name; how: TH_*, carried, lying about that it sees, either, to hand, in win), of several: of
** those the name fits (and the selector's state, wsel's: lit, not lit, open, closed, empty), the one most to hand, held
** first, then worn or hanging from the belt, then in what it carries (the shallower first), then lying within reach,
** then further off (the nearer first; on a surface there too; none it hasn't found that's hidden); the selector's nth
** of them, "the other" one not the first (the second, if none's named first); never wnot, nor one in it; nor one in
** win already (a count's put there).  Its number, or 0 (HW_HAND's: one seen but out of reach, afar, far_n where it
** is); seen_d how far off, in squares; csum how many there were to choose from, all told (a lot its count) */
#define NCAND           8
unsigned __fastcall__ s_pick (const char* name, unsigned char how)
{
    struct { unsigned n; unsigned char r; } c[NCAND];
    unsigned char* t;
    const struct item* p;
    const struct kind* k;
    unsigned i = 0, x, y, px, py, dx, dy;
    unsigned char m = 0, j, r, d, sn, s = wsel & 15, pw, ps, pk;

    afar = 0;
    csum = 0;
    if (wforce && !wnot) {
        return wforce;                          /* (A count's next, chosen already: a lot's part split off) */
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    while ((i = items (i, IN_SQUARE | IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {
        fetch (&rec, i);
        if ((rec.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN || !is_named (name) || (d = chain (i), cnot && s != SL_NTH)) {
            continue;                           /* (Not the first, nor in it, unless it's the nth said: "the second
                                                **   sack" of them all, not of the rest) */
        }
        p = item_at (ctop);
        pw = p->where;
        ps = p->sn;
        pk = p->kind;
        px = p->x;
        py = p->y;
        if (how == HW_IN) {
            if (rec.where != IW_ITEM || rec.x != win) {
                continue;
            }
            r = 0;
        } else if (rec.where == IW_ITEM && rec.x == win) {
            continue;                           /* (In the one a count's put in already) */
        } else if (pw == IW_ACTOR) {
            if (px || how == HW_SEEN) {
                continue;                       /* (Another's: a denizen's) */
            }
            r = d ? 1 + (d > 6 ? 6 : d) : rec.flags & I_HELD ? 0 : 1;
        } else {
            if (how == HW_CARRIED || d > 1 || ps != sn || (dx = px - x + 20) > 40 || (dy = py - y + 20) > 40 ||
                (d && !(kind_of (pk)->keeps & KF_ON)) || !(t = at (sn, px, py)) || !(*t & T_VIS)) {
                continue;                       /* (Lying about, or on a surface there, in sight; not in a thing) */
            }
            dx = dx > 20 ? dx - 20 : 20 - dx;
            dy = dy > 20 ? dy - 20 : 20 - dy;
            r = 8 + (dx > dy ? dx : dy);
        }
        k = kind_of (rec.kind);
        if (s == SL_LIT ? !(rec.flags & I_LIT) : s == SL_UNLIT ? (rec.flags & I_LIT) || !(k->flags & K_LIGHT) :
            s == SL_OPEN ? !(k->keeps & KF_CLOSES) || (rec.shut & IS_CLOSED) : s == SL_SHUT ? !(rec.shut & IS_CLOSED) :
            s == SL_EMPTY && (!((k->flags & K_CONTAINER) || (k->keeps & KF_ON)) || items (0, IN_X | IN_ITEM, i))) {
            continue;                           /* (Not in the state said: "the lit torch", "the empty sack") */
        }
        csum += rec.qty;
        if (m == NCAND) {                       /* (The best NCAND kept, by how much to hand, the store's order else) */
            if (c[NCAND - 1].r <= r) {
                continue;
            }
            --m;
        }
        for (j = m++; j && c[j - 1].r > r; --j) {
            c[j] = c[j - 1];
        }
        c[j].n = i;
        c[j].r = r;
    }
    j = s == SL_NTH ? wsel >> 4 : s == SL_OTHER && !wnot ? 2 : 1;
    if (!j || j > m) {
        return 0;
    }
    i = c[j - 1].n;
    r = c[j - 1].r;
    seen_d = r < 8 ? 0 : r - 8;
    if (how == HW_HAND && r >= 10) {            /* (Seen, but out of reach: where it lies, the UI's to walk to) */
        afar = 1;
        far_n = i;
        chain (i);
        p = item_at (ctop);
        far_x = p->x;
        far_y = p->y;
        return 0;
    }
    return i;
}

/* The first thing named that the character carries; that it sees lying about; that's to hand (carried, or seen within
** reach: afar, seen but out of reach) */
unsigned __fastcall__ s_carried_named (const char* name)
{
    return s_pick (name, HW_CARRIED);
}

unsigned __fastcall__ s_seen_named (const char* name)
{
    return s_pick (name, HW_SEEN);
}

unsigned __fastcall__ s_at_hand (const char* name)
{
    afar = 0;
    return *name ? s_pick (name, HW_HAND) : 0;
}

/* A tool used, got out first (HOUSE.md's handy tools: its reach owed, part 10's; part 4's, its room): the first the
** character carries whose key (the catalogue's: the referee's to choose by) is one of keys ("pick\0shovel\0": each
** 0-ended, an empty one last: a dig begun, a lock's picking) */
void __fastcall__ s_tool_reach (const char* keys)
{
    const char* k;
    unsigned i = 0;

    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        for (k = keys; *k; k += strlen (k) + 1) {
            if (!strcmp (kind_at (i)->tool, k) && on_pc (i)) {
                reach_from (i);
                return;
            }
        }
    }
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 35 (its own, slice 4.1's: the user's, 2026-10-21): things moved by the count ("drop two sacks", "take 4
** rations"), a lot split and joined again */
#pragma code-name (push, "SW35CODE")
#pragma rodata-name (push, "SW35RODATA")
#pragma static-locals (push, on)

/* ---- Taken up (n: seen, the chooser's): into the backpack if it fits (its room, its weight, what it keeps), else in
** a hand (two, at most); not within reach: IR_NOT_HERE, and where it is (the hub's ireq's x, y: the UI walks there, and
** asks again) */
static unsigned char __fastcall__ get (unsigned n)
{
    const struct kind* k;
    unsigned x, y, i;

    for (i = n; item_at (i)->where == IW_ITEM; i = item_at (i)->x) {
    }                                           /* (On a surface: the surface's square) */
    fetch (&rec, i);
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    HUB->ireq.x = rec.x;                        /* (Where it is: IR_NOT_HERE's, IR_FIXED's; and which, I_TAKE's) */
    HUB->ireq.y = rec.y;
    HUB->ireq.item = n;
    if (rec.x - x + 1 > 2 || rec.y - y + 1 > 2) {
        return IR_NOT_HERE;
    }
    fetch (&rec, n);
    k = kind_of (rec.kind);
    rbody = 1;                                  /* (Its reach: lying within reach, a grope engaged, HOUSE.md's) */
    return rec.where == IW_PORTAL ? IR_FIXED : k->keeps & KF_FIXED ? IR_NO :
        !noinit && fumbled (rec.kind) ? IR_FUMBLED : carry (n, 0) ? IR_OK : IR_FULL;  /* (A portal's part: lifted
                                                **   out first, the world's; a theme's altar, never: its words the
                                                **   handling's; engaged, a grope failed: the round wasted) */
}

/* The closed thing that thing n is in (or in what that's in: a sack tied shut in an open pack), or 0 (none: it's to
** hand: part 35's, the catalogue's bank's code's through its stub) */
unsigned __fastcall__ s_shut_in (unsigned n)
{
    const struct item* p;

    while ((p = item_at (n))->where == IW_ITEM) {
        if (item_at (n = p->x)->shut & IS_CLOSED) {
            return n;
        }
    }
    return 0;
}

/* A door's bar to hand (carried, or within reach: not one in another's brackets), the widest: its width (took's,
** the request's x) and IR_NO (nothing about it changed); or (iq's to 1) it set in the door's brackets, a portal's part
** again on iq's square, the bar's side (the world's barred the door): part 35's, the catalogue's bank's room (its
** o_handle's through its stub) */
unsigned char s_bar_hand (void)
{
    unsigned n, best;

    for (best = 0, n = 1; n < NITEMS; ++n) {
        fetch (&rec, n);
        if (rec.kind == KI_BAR && rec.where != IW_PORTAL && (rec.size & 15) > took && (on_pc (n) || in_reach (n))) {
            best = n;
            took = rec.size & 15;
        }
    }
    if (!best || !iq.to) {
        return IR_NO;
    }
    fetch (&rec, best);
    rec.where = IW_PORTAL;
    rec.sn = iq.item;
    rec.x = iq.x;
    rec.y = iq.y;
    rec.flags &= ~(I_HELD | I_WORN | I_HIDDEN);
    store (&rec, best);
    return IR_OK;
}

/* Put down where the character stands (n: carried; what's in it with it): not from a thing that's closed (its words
** why, IR_NO) */
static unsigned char __fastcall__ put_down_here (unsigned n)
{
    struct item* p;
    unsigned c, x, y;
    unsigned char sn;

    if ((c = s_shut_in (n)) != 0) {
        put ("The ");
        put (kind_of (item_at (c)->kind)->name);
        put (" is closed.");
        return IR_NO;
    }
    if (!(item_at (n)->flags & I_WORN)) {
        reach_from (n);                         /* (Got out first: its reach, HOUSE.md's; let fall from a hand, none) */
    } else if (!(kind_at (n)->form & KF_BACK)) {
        rtime += 50;                            /* (Worn: taken off first, a doff's 5 seconds; a pack on the back
                                                **   shrugged off, none) */
        ++rsteps;
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    p = item_at (n);
    p->where = IW_SQUARE;
    p->sn = sn;
    p->x = x;
    p->y = y;
    p->flags &= ~(I_HELD | I_WORN);
    dropped = n;
    return IR_OK;
}

/* Put down (hide's: part 13's, through its stub), by name */
unsigned char __fastcall__ s_drop (const char* name)
{
    unsigned n = pick (name, HW_CARRIED);

    return n ? put_down_here (n) : IR_NONE;
}

/* ---- A lot split (thing n, counted: rations, pennies, arrows): k of them made a lot of their own, in the same place
** (a free record, its own number), n the fewer: the part's number (n itself, if there's no free record); and a lot
** joined to one alike where it is now (the same kind, the same place, neither named: "put back with others alike"):
** the one it's joined to, or n */
static unsigned __fastcall__ split (unsigned n, unsigned char k)
{
    struct item* p;
    unsigned i;

    fetch (&rec, n);
    if (!rec.id) {
        make_own (n);                           /* (Its make its own first: the part's the same) */
        fetch (&rec, n);
    }
    for (i = 1; i < ilim; ++i) {
        p = item_at (i);
        if (!p->kind) {
            rec.qty = k;
            rec.flags &= ~I_NAMED;
            rec.id = ++item_at (0)->x;
            store (&rec, i);
            item_at (n)->qty -= k;
            return i;
        }
    }
    return n;
}

static unsigned __fastcall__ join (unsigned n)
{
    struct item q;
    unsigned i = 0;

    fetch (&q, n);
    if (!(kind_of (q.kind)->form & KF_COUNT) || (q.flags & I_NAMED)) {
        return n;
    }
    while ((i = items (i, IN_X | 1 << q.where, q.x)) != 0) {
        fetch (&rec, i);
        if (i != n && rec.kind == q.kind && !(rec.flags & (I_NAMED | I_HIDDEN)) && rec.qty + q.qty <= 250 &&
            (rec.flags & (I_HELD | I_WORN)) == (q.flags & (I_HELD | I_WORN)) &&
            (q.where != IW_SQUARE || (rec.sn == q.sn && rec.y == q.y))) {
            item_at (i)->qty += q.qty;
            item_at (n)->kind = 0;              /* (The lot moved, gone into the other) */
            return i;
        }
    }
    return n;
}

/* ---- A thing moved (taken up, put down, stowed, put in or on another, taken from it: op, the request's), or as many
** as the request's count says (iq's count: n, CT_ALL, CT_HALF; 0 one): each chosen in turn (pick's: the next most to
** hand), a lot split if it's more than are wanted, each moved as one is (get, put_down_here, the catalogue's bank's
** o_handle), joined to one alike where it went, and its time; their words by the count ("You take 4 rations."; fewer
** than asked: "There are only 3 rations."), the one's as they are (none for one taken up or put down: the UI's).  The
** result: IR_OK if any moved, else the first one's */
static char take_nm[16];                        /* (A count to take up, walked over to first: its name, selector and
                                                **   count, s_take_rest's) */
static unsigned char take_sel, take_cnt;
static void take_keep (void)
{
    strcpy (take_nm, iq.name);
    take_sel = iq.sel;
    take_cnt = iq.count;
}

unsigned char __fastcall__ s_moved (unsigned char op)
{
    static unsigned want, got, t, c;
    static unsigned char r, mode, times, kind, sa, away, parted;
    unsigned n;
    char b[24];

    want = iq.count == CT_ALL ? 0xFFFF : iq.count;
    got = t = 0;
    times = away = 0;
    kind = 0;
    win = 0;
    take_cnt = 0;
    if (op == IO_TAKE) {                        /* (From the other: found first, the things in it) */
        wsel = iq.osel;
        if ((c = at_hand (iq.into)) != 0) {
            win = c;
        }
        wsel = iq.sel;
    }
    mode = op == IO_GET ? HW_SEEN : op == IO_DROP ? HW_CARRIED : op == IO_TAKE ? (win ? HW_IN : HW_HAND) : HW_HAND;
    text_new ();
    for (;;) {
        wforce = 0;
        n = op == IO_TAKE && !win ? 0 : pick (iq.name, mode);
        if (!times && iq.count == CT_HALF) {
            want = csum > 1 ? csum >> 1 : 1;    /* (Half of all there are: a lot's count, or the things alike) */
        }
        parted = 0;
        if (n && want && item_at (n)->qty > want - got && (kind_of (item_at (n)->kind)->form & KF_COUNT) &&
            (op != IO_GET || in_reach (n))) {   /* (Out of reach: walked over to first, its count taken there) */
            n = split (n, want - got);          /* (Part of a lot: those wanted, a lot of their own) */
            parted = 1;
        }
        if (times && !n) {
            break;
        }
        wforce = n;
        handled = n;
        r = op == IO_GET ? (n ? get (n) : IR_NONE) : op == IO_DROP ? (n ? put_down_here (n) : IR_NONE) :
            (cat_map (), o_handle ());
        if (r != IR_OK) {
            if (parted) {
                join (n);                       /* (Not moved: the part a lot with the rest again) */
            }
            away = r == IR_NOT_HERE;
            if (away && !times && iq.count) {
                take_keep ();                   /* (Walked over to first: the count taken as it gets there) */
            }
            if (times) {
                r = IR_OK;                      /* (Some moved: those told; this one's words not) */
                text_new ();
            }
            break;
        }
        n = handled;
        kind = item_at (n)->kind;
        got += item_at (n)->qty;
        sa = last_sa;
        t += took;
        join (n);
        ++times;
        if (op == IO_STOW || op == IO_PUT) {
            win = cont;                         /* (The next not one put there already) */
        }
        if (!want || got >= want) {
            break;
        }
    }
    wforce = 0;
    took = t;
    if (r == IR_OK && (iq.count || times > 1)) {
        text_new ();                            /* (Their words by the count: "You take 4 rations.") */
        s_many (b, got, kind);
        if (want != 0xFFFF && got < want && !away) {
            put (op == IO_DROP ? "You have only " : got == 1 ? "There is only " : "There are only ");
            put (b);
            put (". ");
        }
        if (op == IO_GET || op == IO_DROP || op == IO_TAKE) {
            put (op == IO_DROP ? "You put down " : "You take ");
            put (b);
            if (op == IO_TAKE) {
                put (" from the ");
                put (kind_of (item_at (win)->kind)->name);
            }
            put (".");
        } else {
            strcpy (xn, b);                     /* (The catalogue's words, the count's name for the thing's) */
            cat_map ();
            say (sa);
        }
    }
    return r;
}

/* How many of kind k, in words, into b: "a sack", "two sacks", "12 sacks"; a lot counted (KF_COUNT), its count in
** digits, "4 rations"; one of a lot, "a ration", "a penny" (part 35's: the main RAM's room; part 11's through its stub) */
void __fastcall__ s_many (char* b, unsigned n, unsigned char k)
{
    static const char* const few[11] = { "no", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
        "ten" };
    const struct kind* p = kind_of (k);
    unsigned char i, c;

    if (n == 1) {
        strcpy (b, p->flags & K_PLURAL && !(p->form & KF_COUNT) ? "some " : strchr ("aeiou", p->name[0]) ? "an " : "a ");
    } else if (n <= 10 && !(p->form & KF_COUNT)) {
        strcpy (b, few[n]);
        strcat (b, " ");
    } else {
        utoa (n, b, 10);
        strcat (b, " ");
    }
    b += strlen (b);
    if (n == 1 || (p->flags & K_PLURAL)) {
        strcpy (b, p->name);                    /* ("rations", "candle stubs": plural already) */
        if (n == 1 && (p->form & KF_COUNT) && (i = strlen (b)) > 3) {
            strcpy (b + i - (b[i - 3] == 'i' && b[i - 2] == 'e' ? 3 : 1), b[i - 3] == 'i' && b[i - 2] == 'e' ? "y" : "");
        }                                       /* (One of a lot: "a ration", "a penny") */
        return;
    }
    for (i = 0; p->name[i] && (p->name[i] != ' ' || strncmp (p->name + i, " of ", 4)); ++i) {
    }                                           /* (Its first word's before " of ": "flasks of oil") */
    memcpy (b, p->name, i);
    c = i ? p->name[i - 1] : 0;
    if (c == 'y' && i > 1 && !strchr ("aeiou", p->name[i - 2])) {
        strcpy (b + i - 1, "ies");
    } else {
        strcpy (b + i, c == 's' || c == 'x' || c == 'z' || (c == 'h' && i > 1 && strchr ("cs", p->name[i - 2])) ?
            "es" : "s");
    }
    strcat (b, p->name + i);
}

/* (Part 16's: I_TAKE's walk over to a thing, a count taken up as it gets there: the request as it was, its count;
** wgame.c's take_up) */
unsigned char s_take_rest (void)
{
    if (!take_cnt) {
        return IR_NONE;
    }
    strcpy (iq.name, take_nm);
    wsel = iq.sel = take_sel;
    iq.count = take_cnt;
    take_cnt = 0;
    wnot = win = wforce = 0;
    return s_moved (IO_GET);
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 36 (its own, slice 4.1's): what closes worked; a tool's wear, a thing described (part 11's, its room) */
#pragma code-name (push, "SW36CODE")
#pragma rodata-name (push, "SW36RODATA")
#pragma static-locals (push, on)

/* ---- What closes worked (IO_SHUT, the user's: "open the chest", "open the sack"; the catalogue's KF_CLOSES: a lid, a
** tie, a stopper): the thing named (to hand: carried, or within reach; seen but out of reach, IR_NOT_HERE: the UI walks
** over to it first) opened, closed; its lock (a chest's: IS_LOCK) picked (the referee's P_PICKLOCK, the tools to hand,
** a minute a try, harder the better made: HOUSE.md's), forced (P_FORCE: its lock burst, loud), locked (no key to it
** yet).  Opened, what it shows told in the same sentence (the user's, 2026-10-22: "You open the chest, revealing a
** pile of gold coins"): what's in it, alike told together ("..., revealing a candle and two chisels."; ": it's
** empty."); a stopper's, how full it is (a flask of oil's: ": it's half full.").  Its words in the hub's text, its
** time in the request's x (tenths); IR_NONE if there's no such thing.  Picking and forcing timed work (HOUSE.md's long
** tasks): begun (its long task's kind in qty, its thing's in item: its first words the UI's), its test made and its
** outcome told as it's done (IO_DONE, the UI's as its time's up) */
static void __fastcall__ say3 (const char* a, const char* b, const char* c)
{
    put (a);
    put (b);
    put (c);
}

static void __fastcall__ and_last (char* from);
static void __fastcall__ reveal (unsigned n)
{
    static const char* const full[6] = { ": it's empty.", ": it's nearly empty.", ": it's a quarter full.",
        ": it's half full.", ": it's three-quarters full.", ": it's full." };
    const struct burn* b;
    char* t0;
    unsigned w;

    if ((b = burn_of (item_at (n)->kind)) != 0) {
        w = b->life * 10;                       /* (A fuel's measure, a whole one's minutes: its quarters left) */
        fetch (&rec, n);
        put (full[!rec.fuel ? 0 : rec.fuel >= w ? 5 : 1 + rec.fuel * 4 / w]);
        return;
    }
    if (!items (0, IN_X | IN_ITEM, n)) {
        put (": it's empty.");
        return;
    }
    put (", revealing ");
    hub_map (UI_BANK);
    t0 = tp;
    hub_map (0);
    listed (n);
    and_last (t0);
    put (".");
}

static signed char __fastcall__ tested (unsigned char p, signed char steps)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_UI];                    /* (The UI's: it waits on this request meanwhile) */
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = p;
    r->b = 0;
    r->steps = steps;
    r = hub_ask (SLOT_UI);
    g = r->grade;
    if (r->result) {
        g = -100;                               /* (No tools for it: lockpicks) */
    }
    r->state = REQ_FREE;
    return g;
}

unsigned char s_shut_work (void)
{
    static const char* const opened[3] = { "You open the ", "You untie the ", "You unstopper the " };
    static const char* const closed[3] = { "You close the ", "You tie the ", "You stopper the " };
    struct msg m;
    const struct kind* k;
    unsigned n, t = 20;                         /* (Its time: a minute's picking, 600, past a char's 255: it was 88
                                                **   tenths, a char's, till 2026-10-22) */
    unsigned char w = iq.to & ~IO_DONE, s, c, ok = IR_NO, d = iq.to & IO_DONE, lt = 0;
    signed char g;
    char nm[14];

    text_new ();
    if (!(n = at_hand (iq.name))) {
        if (afar) {
            hub_map (0);
            HUB->ireq.x = far_x;
            HUB->ireq.y = far_y;
            HUB->ireq.item = far_n;
            return IR_NOT_HERE;
        }
        return IR_NONE;                         /* (No such thing: a door's work, the UI's) */
    }
    make_own (n);                               /* (Its lid and its lock its own: found closed, a chest locked
                                                **   sometimes) */
    if (!d) {
        reach_from (n);                         /* (Its reach, as it's begun: HOUSE.md's) */
    }
    fetch (&rec, n);
    k = kind_of (rec.kind);
    strcpy (nm, k->name);
    c = (k->keeps & KF_CLOSES) >> 5;
    s = rec.shut;
    if (!c) {
        if (k->flags & K_CONTAINER) {
            say3 ("The ", nm, " has no lid.");
        } else {
            say3 (w == PA_CLOSE ? "You can't close the " : "You can't open the ", nm, ".");
        }
        return IR_NO;
    }
    --c;
    if (w == PA_LOCK) {
        say3 ("The ", nm, !(s & IS_LOCK) ? " has no lock." : s & IS_BURST ? "'s lock is broken." : s & IS_LOCKED ?
            " is locked already." : " has a lock, but you have no key to it.");
        return IR_NO;
    }
    if (w == PA_CLOSE) {
        if (s & IS_CLOSED) {
            say3 ("The ", nm, " is closed already.");
            return IR_NO;
        }
        s |= IS_CLOSED;
        say3 (closed[c], nm, c == 1 ? " shut." : ".");
        ok = IR_OK;
    } else if (!(s & IS_CLOSED)) {
        say3 ("The ", nm, w == PA_UNLOCK && (s & IS_LOCK) ? " isn't locked: it's open." : " is open already.");
        return IR_NO;
    } else if (w == PA_UNLOCK) {
        if (!(s & IS_LOCKED)) {
            say3 ("The ", nm, s & IS_LOCK ? " isn't locked." : " has no lock.");
            return IR_NO;
        }
        if ((g = tested (P_PICKLOCK, -1 - (signed char) (rec.quality / 3))) == -100) {
            put ("You have nothing to pick its lock with.");
            return IR_NO;
        }
        if (!d) {
            t = 600;                            /* (The means to hand: a minute a try, as a door's, a long task, its
                                                **   test made again at its end; the lockpicks' reach first) */
            lt = LT_PICK;
            tool_reach ("picks\0");
        } else if (g > 0) {
            s &= ~IS_LOCKED;
            say3 ("You pick the lock of the ", nm, ".");
        } else {
            say3 ("You fail to pick the lock of the ", nm, ".");
        }
        ok = IR_OK;
    } else if (s & IS_LOCKED) {
        if (w != PA_FORCE) {
            say3 ("The ", nm, " is locked.");
            return IR_NO;
        }
        if (!d) {
            hub_map (0);
            m.kind = E_NOISE;                   /* (Loud: a lock burst, or a lid wrenched at, ten seconds' of it, its
                                                **   test at its end) */
            m.who = 0;
            m.a = 6;
            m.b = NZ_WORK;
            m.w1 = HUB->pc_x;
            m.w2 = HUB->pc_y;
            m.t = HUB->now;
            hub_event (&m);
            t = 100;
            lt = LT_FORCE;
        } else if (tested (P_FORCE, (rec.mat == MT_WOOD ? 0 : -2) - (signed char) (rec.quality / 3)) > 0) {
            s = (s & ~(IS_LOCKED | IS_CLOSED)) | IS_BURST;
            say3 ("You force the ", nm, ", and its lock bursts open");
            reveal (n);
        } else {
            say3 ("The ", nm, " holds: its lock doesn't give.");
        }
        ok = IR_OK;
    } else {
        s &= ~IS_CLOSED;                        /* (Opened; forced open, one not locked: what it shows told) */
        say3 (opened[c], nm, "");
        reveal (n);
        ok = IR_OK;
    }
    item_at (n)->shut = s;
    hub_map (0);
    HUB->ireq.x = d ? 0 : t;                    /* (Done: its time spent already) */
    HUB->ireq.qty = lt;                         /* (Begun: its first words the UI's) */
    HUB->ireq.item = rec.kind;
    return ok;
}

#pragma static-locals (pop)

/* ---- What's in a thing, or on it (IO_INSIDE, the user's, 2026-10-21: "look in the sack", "look on the table", "look
** into the well"): the thing named (to hand: carried, or within reach; seen but out of reach, IR_NOT_HERE: the UI
** walks over to it first), what's in it, on it, hanging from it, in words, alike told together: "In the sack lie a
** candle and two chisels."; "The sack is empty."; "The chest is closed." (nothing in it seen); a well's shaft; a
** thing that holds nothing, "You can't see into the sword."  The request's to 1: as it's examined (it's named
** already: "In it lie ..."; a thing that holds nothing, no words); | 2, after the words there already (a look at it:
** part 39's) */
static void __fastcall__ and_last (char* from)
{
    char* p;
    char* l = 0;
    unsigned char d = 0;

    hub_map (UI_BANK);
    for (p = from; *p; ++p) {                   /* (The list's last ", ", not in a thing's own "(in it: ...)") */
        if (*p == '(') {
            ++d;
        } else if (*p == ')') {
            --d;
        } else if (!d && *p == ',' && p[1] == ' ') {
            l = p;
        }
    }
    if (l && tp + 4 < UIREC->things_text + sizeof UIREC->things_text) {
        memmove (l + 5, l + 2, strlen (l + 2) + 1);
        memcpy (l, " and ", 5);
        tp += 3;
    }
    hub_map (0);
}

unsigned char s_inside (void)
{
    const struct kind* k;
    unsigned n, j;
    unsigned char f, kk, on, pl = 0, to = iq.to;
    char nm[20];
    char* t0;

    if (!(to & 2)) {
        text_new ();
    }
    to &= 1;
    if (!(n = at_hand (iq.name))) {
        if (afar) {
            hub_map (0);
            HUB->ireq.x = far_x;
            HUB->ireq.y = far_y;
            HUB->ireq.item = far_n;
            return IR_NOT_HERE;
        }
        return IR_NONE;
    }
    make_own (n);
    fetch (&rec, n);
    k = kind_of (rec.kind);
    f = k->flags;
    kk = k->keeps;
    strcpy (nm, to ? "it" : "the ");
    if (!to) {
        strcat (nm, k->name);
    }
    on = kk & KF_ON ? 1 : f & K_BELT ? 2 : 0;
    if (!(f & (K_CONTAINER | K_BELT)) && !(kk & KF_ON)) {
        if (!to) {
            put (rec.kind == KI_WELL ? "The well's shaft drops away into the dark." : "You can't see into ");
            if (rec.kind != KI_WELL) {
                put (nm);
                put (".");
            }
        }
        return IR_OK;
    }
    if (rec.shut & IS_CLOSED) {
        put (to ? "It's closed." : "The ");
        if (!to) {
            put (nm + 4);
            put (" is closed.");
        }
        return IR_OK;
    }
    if (iq.kind == IO_INSIDE) {
        reach_to (n);                           /* (Its reach, looked into: a pack worn taken off, HOUSE.md's) */
    }
    for (j = 0; (j = items (j, IN_X | IN_ITEM, n)) != 0; ) {
        fetch (&rec, j);
        pl += 1 + (rec.qty > 1 || (kind_of (rec.kind)->flags & K_PLURAL));
    }
    if (!pl) {
        put (on == 1 ? "There's nothing on " : on == 2 ? "Nothing hangs from " : to ? "It's empty" : "The ");
        if (on || !to) {
            put (on ? nm : nm + 4);
        }
        put (on || to ? "." : " is empty.");
        return IR_OK;
    }
    put (on == 1 ? "On " : on == 2 ? "From " : "In ");
    put (nm);
    put (on == 2 ? (pl > 1 ? " hang " : " hangs ") : pl > 1 ? " lie " : " lies ");
    hub_map (UI_BANK);
    t0 = tp;
    hub_map (0);
    listed (n);
    and_last (t0);
    put (".");
    return IR_OK;
}

/* (Part 40's, its room: part 36's once, part 11's before it) */
#pragma code-name (push, "SW40CODE")
#pragma rodata-name (push, "SW40RODATA")

/* The character's (on it, or in what it carries): part 10's on_pc, here too (wear's: a dig's every spell's, the
** parts not swapped for it) */
static unsigned char __fastcall__ on_me (unsigned i)
{
    unsigned j;

    for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {
    }
    return item_at (j)->where == IW_ACTOR && !item_at (j)->x;
}

/* A tool worn by a spell of work (the dig's: the referee's choice, by its key), by; its name in the request */
void __fastcall__ s_wear (const char* key, unsigned char by)
{
    char name[16];
    struct item* p;
    unsigned i = 0;

    strcpy (name, key);
    while ((i = items (i, IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {
        fetch (&rec, i);
        if (!strcmp (kind_of (rec.kind)->tool, key) && on_me (i)) {
            p = item_at (i);
            p->wear = p->wear + by < p->wear ? 255 : p->wear + by;
            strcpy (name, kind_of (rec.kind)->name);
            break;
        }
    }
    hub_map (0);
    strcpy (HUB->ireq.name, name);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 39 (its own, 2026-10-22, the user's: "look at the chest", "examine the sword", "check its condition"): a
** thing looked at as anyone sees it, no test (IO_LOOK); its weight, its lock, it gone over for traps, part 40's, and a
** thing described for the appraiser there too (IO_DESCRIBE: part 36's once, its room) */
#pragma code-name (push, "SW39CODE")
#pragma rodata-name (push, "SW39RODATA")
#pragma static-locals (push, on)

/* ---- A thing looked at (IO_LOOK: the request's to, LM_*), its words in the hub's text, as anyone sees it, no test:
**   LM_AT     what it is and where ("Your torch is in your hand."), its state ("It's lit, burning low.", "It's
**             closed, with a lock."), and what's in it or on it, within reach (look in's words: "In it lie two
**             chisels.")
**   LM_CLOSE  that, and what a closer look shows besides: what it's made of, how thick, how worn, how much of a light
**             or a fuel is left ("It's made of wood; it's little worn, half burnt down."), a lock's make ("Its lock is
**             a simple one.")
**   LM_COND   its wear and its damage alone ("Your sword is battered, its edge dulled.")
**   LM_WEIGHT how heavy it is, felt in the hand or lifted where it lies ("The chest weighs about 30 pounds, with what's
**             in it."), part 40's: out of reach, IR_NOT_HERE (walked over to first)
**   LM_LOCK   its lock, its make, how hard it looks to pick (the picking is the test), part 40's
**   LM_TRAPS  gone over for traps (a thing has none yet: "The chest shows no sign of a trap ..."), part 40's: out of
**             reach, IR_NOT_HERE
** Too far off to make out (1 + sight / 2 + its kind's seen squares: HOUSE.md's noticing), its size alone; further
** off than NEAR_SQ, a look at it only (no closer look), and nothing in it */
struct kind lk;                                 /* (The thing's kind, copied: its fields reached quickly, part 40's too) */
unsigned char lpl, ld;                          /* (Its name a plural; how far off it is, in squares) */
unsigned char lnw;                              /* (The words of a list said: part 42's too) */

/* A word of a list ("lit, burning low": ", " before all but the first) */
static void __fastcall__ word (const char* s)
{
    if (lnw++) {
        put (", ");
    }
    put (s);
}

unsigned char __fastcall__ look_more (unsigned n, unsigned char mode);    /* (Part 40's) */
void __fastcall__ closer (unsigned char mode);  /* (Part 42's) */

unsigned char s_look_at (void)
{
    static const char* const where[6] = { "worn", "on you", "in your hand", "in your left hand", "in your right hand",
        "in both hands" };
    static const char* const shut[3][2] = { { "open", "closed" }, { "untied", "tied shut" },
        { "unstoppered", "stoppered" } };
    const struct kind* q;
    struct item* p;
    unsigned n, c, px, py;
    unsigned char mode = iq.to, sh, cl, k, mine;
    char b[6];

    text_new ();
    if (!(n = pick (iq.name, HW_ANY))) {        /* (Carried, or seen: the one most to hand, of several) */
        return IR_NONE;
    }
    ld = seen_d > 99 ? 99 : seen_d;
    for (c = n, cl = 0; (p = item_at (c))->where == IW_ITEM; ) {
        if (item_at (c = p->x)->shut & IS_CLOSED) {
            cl = 1;                             /* (In a thing closed: not seen) */
            px = c;
        }
    }                                           /* (Whose: the character's, on it or in what it carries; where it lies) */
    if (cl) {
        put ("The ");
        put (kind_of (item_at (px)->kind)->name);
        put (" is closed.");
        return IR_OK;
    }
    mine = p->where == IW_ACTOR && !p->x;
    px = p->x;
    py = p->y;
    if (ld > 1 && (mode == LM_WEIGHT || mode == LM_TRAPS)) {
        hub_map (0);                            /* (Out of reach: walked over to first, where it lies, the UI's) */
        HUB->ireq.x = px;
        HUB->ireq.y = py;
        HUB->ireq.item = n;
        return IR_NOT_HERE;
    }
    make_own (n);                               /* (Its make its own: its lid, its lock, its fuel decided) */
    fetch (&rec, n);
    memcpy (&lk, kind_of (rec.kind), sizeof lk);
    hub_map (0);
    if ((k = ld > 1 + HUB->pcsum.sight / 2 + lk.seen) != 0) {    /* (Too far off to make out: its size alone) */
        put (lk.seen <= 2 ? "Something small" : lk.seen >= 5 ? "Something large" : "Something");
        goto off;
    }
    lpl = (lk.flags & K_PLURAL) || rec.qty > 1;
    put (mine ? "Your " : "The ");              /* What it is, and where */
    if (rec.qty > 1) {
        put (utoa (rec.qty, b, 10));
        put (" ");
    }
    put (lk.name);
    if (mode >= LM_WEIGHT) {
        return look_more (n, mode);             /* (Its weight, its lock, it gone over for traps: part 40's) */
    }
    put (lpl ? " are " : " is ");
    if (mode == LM_COND) {
        closer (LM_COND);                       /* (Its condition alone: part 42's) */
        return IR_OK;
    }
    if (rec.where == IW_ACTOR) {
        put (where[rec.flags & I_HELD ? 2 + (rec.sn & IH_BOTH) : !(rec.flags & I_WORN)]);   /* (The hand it's in, its sn:
                                                **   slice 4.1b's) */
    } else if (rec.where == IW_ITEM) {
        q = kind_of (item_at (rec.x)->kind);     /* (What it's in, or on) */
        put (q->keeps & KF_ON || q->flags & K_BELT ? "on " : "in ");
        put (mine ? "your " : "the ");
        put (q->name);
    } else if (rec.where == IW_PORTAL) {
        put (rec.kind == KI_BAR ? "set in its door" : "set in its window");
    } else if (ld > 1) {
off:
        put (" is some ");
        put (utoa (SQ_FT * ld, b, 10));
        put (" feet off");
    } else {
        put (ld ? "beside you" : "at your feet");
    }
    put (".");
    if (k) {
        return IR_OK;                           /* (Something, too far off to make out) */
    }
    if (ld > NEAR_SQ && mode) {                 /* (Seen from afar: no closer look) */
        put (" You'd have to go closer to make out more.");
        return IR_OK;
    }
    tq = tbuf;                                  /* Its state, at a glance: the words gathered first (none, none said) */
    tqe = tbuf + sizeof tbuf - 1;
    *tq = 0;
    lnw = 0;
    sh = rec.shut;
    if (lk.flags & K_LIGHT) {
        word (rec.flags & I_LIT ? "lit" : !rec.id || rec.fuel || !burn_of (rec.kind) ? "unlit" : rec.kind == KI_LANTERN ?
            "out of oil" : "burnt out");
        if ((rec.flags & I_LIT) && rec.id && rec.fuel <= BURN_LOW && burn_of (rec.kind)) {
            word ("burning low");
        }
    } else if (rec.id && !rec.fuel && burn_of (rec.kind)) {
        word ("empty");                         /* (A fuel used up: a flask of oil) */
    }
    if ((cl = (lk.keeps & KF_CLOSES) >> 5) != 0) {
        word (shut[cl - 1][(sh & IS_CLOSED) != 0]);
    }
    if (!mode) {                                /* (Its damage, its wear, its lock, plain to see) */
        if (rec.dur_max && rec.dur * 10 < rec.dur_max) {
            word ("nearly broken");
        } else if (rec.dur_max && rec.dur * 3 < rec.dur_max) {
            word ("battered");
        }
        if (rec.wear >= 32) {
            word (lk.tool[1] == 'l' || lk.tool[1] == 'n' ? (rec.wear >= 128 ? "dull" : "dulled") : "worn");
        }
        if (sh & IS_LOCK) {
            word (sh & IS_BURST ? "its lock broken" : "with a lock");
        }
    }
    c = (lk.flags & (K_CONTAINER | K_BELT) || lk.keeps & KF_ON) && !(sh & IS_CLOSED) && ld <= 1;
    if (c && !items (0, IN_X | IN_ITEM, n)) {
        word (lk.keeps & KF_ON ? "bare" : "empty");
        c = 0;
    }
    tq = 0;
    if (lnw) {
        put (lpl ? " They're " : " It's ");
        put (tbuf);
        put (".");
    }
    if (mode) {
        closer (LM_CLOSE);                      /* (What a closer look shows besides: part 42's) */
    }
    if (c) {                                    /* What's in it, on it, within reach (look in's words) */
        put (" ");
        iq.to = 3;
        inside ();
    }
    return IR_OK;
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 40 (its own, 2026-10-22): a thing's weight, its lock, it gone over for traps (part 39's look_at's: its
** name said already), and a thing described for the appraiser */
#pragma code-name (push, "SW40CODE")
#pragma rodata-name (push, "SW40RODATA")
#pragma static-locals (push, on)

/* ---- A thing looked at closely (carried, or seen): what it truly is, in the request, for the appraiser's judgement
** (the UI's, by the referee's test of the craft that knows it); and how far off it is, in squares (0 carried), its
** kind's seen above it (how far off it's made out: the request's to, the UI's, what it makes out at a distance) */
unsigned char __fastcall__ s_describe (const char* name)
{
    const struct kind* k;
    struct ireq* r;
    unsigned n = pick (name, HW_ANY);         /* (Carried, or seen: the one most to hand, of several) */

    if (!n) {
        return IR_NONE;
    }
    make_own (n);
    fetch (&rec, n);
    k = kind_of (rec.kind);
    hub_map (0);
    r = &HUB->ireq;
    strcpy (r->name, k->name);
    r->item = rec.id;
    r->craft = k->craft;
    r->quality = rec.quality;
    r->mat = rec.mat;
    r->plural = (k->flags & K_PLURAL) != 0;
    r->qty = rec.qty;
    r->dur = rec.dur;
    r->dur_max = rec.dur_max;
    r->value = k->value;
    r->size = rec.size;
    r->thick = rec.thick;
    r->to = k->seen << 8 | seen_d;
    return IR_OK;
}

/* Its weight, its lock, it gone over for traps (thing n, rec its record, lk its kind; its name said) */
unsigned char __fastcall__ s_look_more (unsigned n, unsigned char mode)
{
    unsigned w;
    unsigned char sh = rec.shut, k;
    char b[6];

    if (mode == LM_LOCK) {
        k = 1 + rec.quality / 3;
        put (!(sh & IS_LOCK) ? (lpl ? " have no lock." : " has no lock.") : sh & IS_BURST ?
            "'s lock is broken: it won't lock again." : ld > NEAR_SQ ?
            " has a lock, but you'd have to go closer to make out more." : k <= 2 ?
            " has a simple lock: easy to pick, by the look of it." : k == 3 ?
            " has a sound lock: a fair test of a lockpick." : " has a good lock: a hard one to pick.");
        return IR_OK;
    }
    if (mode == LM_TRAPS) {
        put (lpl ? " show no sign of a trap: you go over them carefully, and find none." :
            " shows no sign of a trap: you go over it carefully, and find none.");
        return IR_OK;
    }
    if (lk.keeps & KF_FIXED) {
        put (lpl ? " are fixed where they stand." : " is fixed where it stands.");
        return IR_OK;
    }
    w = weight_of (n);                          /* (Quarter pounds, with what's in it) */
    put (lpl ? " weigh " : " weighs ");
    if (w < WT_LB) {
        put ("less than a pound");
    } else if (w < WT_LB * 3 / 2) {
        put ("about a pound");
    } else {
        put ("about ");
        put (utoa ((w + WT_LB / 2) / WT_LB, b, 10));
        put (" pounds");
    }
    put (items (0, IN_X | IN_ITEM, n) ? ", with what's in it." : ".");
    return IR_OK;
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 42 (its own, 2026-10-22): what a closer look at a thing shows (part 39's look_at's: its name said, its
** state told): what it's made of, its thickness, its wear, what's left of a light, a fuel, a lock's make; and its
** condition alone */
#pragma code-name (push, "SW42CODE")
#pragma rodata-name (push, "SW42RODATA")
#pragma static-locals (push, on)

static const char matname[NMATS][8] = { "", "wood", "iron", "steel", "bronze", "leather", "cloth", "bone", "clay",
    "stone", "glass", "silver", "", "beeswax", "tallow" };

/* A word of a list (part 39's word, its own copy) */
#define word            word42
static void __fastcall__ word (const char* s)
{
    if (lnw++) {
        put (", ");
    }
    put (s);
}

/* Its wear, its damage, how much of a light or a fuel is left: each a word of the list (rec's, lk its kind) */
static void worn_words (void)
{
    static const char* const life[5] = { "close to breaking", "battered", "well worn", "little worn", "as good as new" };
    static const char* const left[3][6] = {
        { "burnt out", "nearly burnt out", "mostly burnt down", "half burnt down", "a quarter burnt down", "hardly burnt" },
        { "out of oil", "nearly out of oil", "low on oil", "half full of oil", "three-quarters full of oil", "full of oil" },
        { "empty", "nearly empty", "a quarter full", "half full", "three-quarters full", "full" } };
    const struct burn* b;
    unsigned w;

    if (rec.dur_max) {
        w = rec.dur / (rec.dur_max / 10 + 1);   /* (Its life left, in tenths) */
        word (life[w >= 10 ? 4 : w >= 7 ? 3 : w >= 4 ? 2 : w >= 2 ? 1 : 0]);
    }
    if (rec.wear >= 32) {                       /* (A blade dug with, dulled; a tool worn) */
        word (lk.tool[1] == 'l' || lk.tool[1] == 'n' ? (rec.wear >= 128 ? "its edge dull" : "its edge dulled") :
            "worn with use");
    }
    if (rec.id && (b = burn_of (rec.kind)) != 0) {
        w = b->life * 10;                       /* (A whole one's minutes: the quarters left) */
        word (left[!(lk.flags & K_LIGHT) ? 2 : rec.kind == KI_LANTERN][!rec.fuel ? 0 : rec.fuel >= w ? 5 :
            1 + rec.fuel * 4 / w]);
    }
}

/* What a closer look shows (mode LM_CLOSE: "It's made of wood; it's little worn, half burnt down. Its lock is a simple
** one."; a portal's part's measure, "It's made of glass, 2 by 3 feet, a quarter inch thick"); its condition alone
** (LM_COND: after its name, "... is battered, its edge dulled.") */
void __fastcall__ s_closer (unsigned char mode)
{
    unsigned char k;
    char b[6];

    lnw = 0;
    if (mode == LM_COND) {
        worn_words ();
        put (lnw ? "." : "as good as ever.");
        return;
    }
    put (lpl ? " They're made of " : " It's made of ");
    put (*matname[rec.mat] ? matname[rec.mat] : "something you can't name");
    if ((k = rec.size) != 0) {                  /* (Its measure: a portal's part's, its opening's) */
        put (", ");
        put (utoa (k & 15, b, 10));
        if (k >> 4) {
            put (" by ");
            put (utoa (k >> 4, b, 10));
            put (" feet");
        } else {
            put (" feet long");
        }
    }
    if ((k = rec.thick) != 0) {
        put (", ");
        put (k >= 8 ? utoa (k >> 3, b, 10) : k >= 4 ? "half an" : k >= 2 ? "a quarter" : "an eighth of an");
        put (k >= 16 ? " inches thick" : " inch thick");
    }
    tq = tbuf;
    *tq = 0;
    worn_words ();
    tq = 0;
    if (lnw) {
        put (lpl ? "; they're " : "; it's ");
        put (tbuf);
    }
    put (".");
    if ((rec.shut & (IS_LOCK | IS_BURST)) == IS_LOCK) {
        k = 1 + rec.quality / 3;
        put (k <= 2 ? " Its lock is a simple one." : k == 3 ? " Its lock is a sound one." : " Its lock is a good one.");
    }
}
#undef word

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 11: what's carried listed, a search, a thing described, a thing hidden */
#pragma code-name (push, "SW11CODE")
#pragma rodata-name (push, "SW11RODATA")

/* ---- What the character carries, worded into the hub's text (things_text): each thing, and what's in it */

/* A thing in words: its kind, how many, and what anyone can see of its state (its make and worth are an appraiser's
** to judge) */


static unsigned char tw_n;                      /* (How many alike told together: "two sacks") */
#pragma static-locals (push, on)                /* (Called again by nothing before it's done: quicker) */
void __fastcall__ s_thing_words (const struct item* q)
{
    const struct kind* k = kind_of (q->kind);
    char b[6];
    unsigned char c = k->keeps & KF_CLOSES;

    if (q->flags & I_NAMED) {                   /* (A name the character gave it: its id, marked for the UI, "#12 a
                                                **   sword", worded "Linda, your sword") */
        put ("#");
        put (utoa (q->id, b, 10));
        put (" ");
    }
    if (tw_n > 1 || (q->qty == 1 && (k->form & KF_COUNT))) {
        many (tbuf, tw_n > 1 ? tw_n : 1, q->kind);    /* (Alike, told together; one of a lot, "a ration") */
        put (tbuf);
    } else {
        if (q->qty > 1) {
            put (utoa (q->qty, b, 10));
            put (" ");
        } else if (!(k->flags & K_PLURAL)) {
            put (strchr ("aeiou", k->name[0]) ? "an " : "a ");
        }
        put (k->name);
    }
    if (q->dur_max && q->dur * 10 < q->dur_max) {
        put (" (nearly broken)");
    } else if (q->dur_max && q->dur * 3 < q->dur_max) {
        put (" (battered)");
    }
    if (q->flags & I_LIT) {
        put (" (lit)");
    }
    if (q->id && q->fuel <= BURN_LOW && burn_of (q->kind)) {    /* (A light spent or low, a fuel empty: slice
                                                **   4.1's) */
        put (!(k->flags & K_LIGHT) ? (q->fuel ? "" : " (empty)") : q->fuel ? " (burning low)" : " (spent)");
    }
    if (q->wear >= 32) {                        /* (Its wear: a blade dulled, a tool worn) */
        put (!strcmp (k->tool, "blade") || !strcmp (k->tool, "knife") ? (q->wear >= 128 ? " (dull)" : " (dulled)") :
            " (worn)");
    }
    if (c == KF_STOPPER ? q->id && !(q->shut & IS_CLOSED) : c && (q->shut & IS_CLOSED)) {
        put (c == KF_STOPPER ? " (unstoppered)" : " (closed)");    /* (What closes it: a lid, a tie, shut; a stopper
                                                **   out) */
    }
}
#pragma static-locals (pop)

/* What the character carries gathered first, in one pass of the store (the records on an actor, and every record in
** or on a thing: what's in what it carries among them), its list and its load worded from that: each thing's
** contents found there, not by a pass each (a pass some 25,000 cycles: an inventory's passes were a third of a
** million); more than GOT_MAX of them, the passes as before (ngot 0xFF) */
#define GOT_MAX         48
static struct got { unsigned n, x; unsigned char where; }* got;
static unsigned char ngot;

/* The next record after i at where, x (IN_X's: items ()'s order, the records' own) */
#pragma static-locals (push, on)
static unsigned __fastcall__ next_in (unsigned i, unsigned char where, unsigned x)
{
    struct got* g = got;
    unsigned char k;

    if (ngot == 0xFF) {
        return items (i, IN_X | 1 << where, x);
    }
    for (k = ngot; k; --k, ++g) {
        if (g->n > i && g->where == where && g->x == x) {
            return g->n;
        }
    }
    return 0;
}
#pragma static-locals (pop)

/* Each thing at where, x worded, and what's in it, or on it; its weight (quarter pounds) added to load's, the
** things it goes into the same as weight_in's (part 10's): what it all weighs */
static unsigned load;
static unsigned char lmode;                     /* (The words a list in a sentence, look in's: "a candle and two
                                                **   chisels", not "(in it: ...)") */
#define wa              tbuf                    /* (Two things' words compared, alike's: the main RAM's buffer, idle
                                                **   meanwhile) */
#define wb              (tbuf + 40)

/* Things a and b alike, to be told together ("two sacks": the same kind, one each, worn alike, neither named nor
** held (each in a hand told), nor with anything in it, the same in the words that tell them) */
#pragma static-locals (push, on)
static unsigned char __fastcall__ alike (unsigned a, unsigned b)
{
    struct item p, q;
    char* t = tq;
    char* e = tqe;
    unsigned char r;

    if (item_at (a)->kind != item_at (b)->kind) {
        return 0;                               /* (Quickly, most of them) */
    }
    fetch (&p, a);
    fetch (&q, b);
    if (p.kind != q.kind || p.qty != 1 || q.qty != 1 || ((p.flags ^ q.flags) & I_WORN) ||
        ((p.flags | q.flags) & (I_NAMED | I_HELD)) || next_in (0, IW_ITEM, a) || next_in (0, IW_ITEM, b)) {
        return 0;
    }
    tq = wa;
    tqe = wa + 39;
    s_thing_words (&p);
    tq = wb;
    tqe = wb + 39;
    s_thing_words (&q);
    r = !strcmp (wa, wb);
    tq = t;
    tqe = e;
    return r;
}
#pragma static-locals (pop)

static struct got* gp;                          /* (contents's, its alike's search: static, none of it live as it
                                                **   goes on into what's in a thing) */
static unsigned char gm;
static void __fastcall__ contents (unsigned char where, unsigned x, unsigned char depth)
{
    const struct kind* k;
    static const char* const hands_in[4] = { ", in your hand", ", in your left hand", ", in your right hand",
        ", in both hands" };
    struct item q;
    unsigned i = 0, j, told[8];
    unsigned char first = 1, n, kf, kk, nt = 0;
    char* t;
    char* e;

    while ((i = next_in (i, where, x)) != 0) {
        fetch (&q, i);
        k = kind_of (q.kind);
        kf = k->flags;
        kk = k->keeps;
        load += (unsigned) k->weight * (kf & K_PLURAL && !(k->form & KF_COUNT) ? 1 : q.qty);
        for (n = 0; n < nt && told[n] != i; ++n) {
        }
        if (n < nt) {
            continue;                           /* (Told with one alike before it: nothing in it, nothing more) */
        }
        n = 1;
        if (q.qty == 1 && !(q.flags & (I_NAMED | I_HELD))) {     /* (One that may have others alike: alike's) */
            if (ngot != 0xFF) {
                for (gp = got, gm = ngot; gp->n != i; ++gp, --gm) {
                }                               /* (Those after it in the list gathered, from its place: not the
                                                **   list again for each, next_in's) */
                while (--gm) {
                    if ((++gp)->where == where && gp->x == x && alike (i, gp->n)) {
                        ++n;
                        if (nt < 8) {
                            told[nt++] = gp->n;
                        }
                    }
                }
            } else {
                for (j = i; (j = next_in (j, where, x)) != 0; ) {
                    if (alike (i, j)) {         /* (Those alike after it: "two sacks", the user's) */
                        ++n;
                        if (nt < 8) {
                            told[nt++] = j;
                        }
                    }
                }
            }
        }
        if (!depth) {
            put (first ? "You carry " : "; ");
        } else if (lmode && depth == 1) {
            put (first ? "" : ", ");
        } else {
            put (first ? (kind_at (x)->flags & K_BELT || kind_at (x)->keeps & KF_ON ?
                " (on it: " : " (in it: ") : ", ");
        }
        first = 0;
        tw_n = n;
        s_thing_words (&q);
        tw_n = 0;
        if (!depth) {
            put (q.flags & I_HELD ? hands_in[q.sn & IH_BOTH] : q.flags & I_WORN ? ", worn" : "");   /* (The hand it's
                                                **   in, its sn: slice 4.1b's) */
        }
        if (kf & (K_CONTAINER | K_BELT) || kk & KF_ON) {
            t = tq;
            e = tqe;
            if (q.shut & IS_CLOSED) {
                tq = tqe = wb + 39;             /* (Closed: what's in it not told, but its weight counted) */
            }
            contents (IW_ITEM, i, depth + 1);
            tq = t;
            tqe = e;
        }
    }
    if (!first && depth && !(lmode && depth == 1)) {
        put (")");
    }
    if (!depth) {
        put (first ? "You carry nothing." : ".");
    }
}

void s_list (void)
{
    struct got g[GOT_MAX];
    struct got* p = g;
    char b[6];
    unsigned i = 0;

    got = g;
    ngot = 0;
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (ngot == GOT_MAX) {
            ngot = 0xFF;
            break;
        }
        p->n = i;
        p->x = ip->x;
        p->where = ip->where;
        ++p;
        ++ngot;
    }
    text_new ();
    load = 0;
    contents (IW_ACTOR, 0, 0);
    put (" In all, ");                          /* (Its load: the referee's encumbrance of it, the UI's to word) */
    put (utoa ((load + WT_LB / 2) / WT_LB, b, 10));
    put (" lb.");
}

/* What's in thing n (or on it, hanging from it) worded as a list, alike told together ("a candle, two chisels":
** part 36's inside, its words in a sentence) */
void __fastcall__ s_listed (unsigned n)
{
    lmode = 1;
    load = 0;
    ngot = 0xFF;                                /* (No list gathered: next_in's passes) */
    contents (IW_ITEM, n, 1);
    lmode = 0;
}


#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 27 (dq0w.c's, the lights burning; its room): a light renewed */
#pragma code-name (push, "SW27CODE")
#pragma rodata-name (push, "SW27RODATA")

/* Thing i the character's (on it, or in what it carries) */
static unsigned char __fastcall__ carried (unsigned i)
{
    for (; item_at (i)->where == IW_ITEM; i = item_at (i)->x) {
    }
    return item_at (i)->where == IW_ACTOR && !item_at (i)->x;
}

static void __fastcall__ put3 (const char* a, const char* b, const char* c)
{
    put (a);
    put (b);
    put (c);
}

/* ---- A light renewed (IO_FILL, slice 4.1's: "fill the lantern", "dip the torch in the oil"): the light named
** (carried, or within reach) from the fuel named (into: carried, or within reach; "": the first carried that it
** takes), as much as it has room for, its whole life less what it has left, the fuel's measure the less by what it
** gave (a flask of oil a lantern's whole fill); one that burns away (a candle) takes none.  Its words in the hub's text,
** its time in the request's x (a lantern filled, 20 seconds; a torch dipped, 10): timed work (HOUSE.md's long tasks),
** begun (LT_FILL in qty, its thing's kind in item: its first words the UI's), the fuel moved as it's done (IO_DONE) */
unsigned char s_fill (void)
{
    struct item l, f;                           /* (On the C stack: the main RAM's room) */
    const struct burn* b;
    unsigned n, c, room, give;
    unsigned char takes, k, bk = 0;
    char ln[14], fn[14];

    text_new ();
    for (k = 1, room = 0; !(iq.sel & 15) && k < 8; ++k) {   /* (Of several of the name, none said, the one with the
                                                **   most room for more: the spent torch, not the one lit) */
        wsel = SL_NTH | k << 4;
        if (!(n = at_hand (iq.name))) {
            break;
        }
        fetch (&l, n);
        if ((b = burn_of (l.kind)) != 0 && l.fuel < b->life * 10 && b->life * 10 - l.fuel > room) {
            room = b->life * 10 - l.fuel;
            bk = k;
        }
    }
    wsel = bk ? SL_NTH | bk << 4 : iq.sel;
    if (!(n = at_hand (iq.name))) {
        put (*iq.name ? "You have no such thing to hand." : "Fill what?");
        return IR_NO;
    }
    make_own (n);
    fetch (&l, n);
    strcpy (ln, kind_of (l.kind)->name);
    if (!(kind_of (l.kind)->flags & K_LIGHT)) {
        put3 ("You can't fill the ", ln, ".");
        return IR_NO;
    }
    if (!(b = burn_of (l.kind)) || !(takes = b->fuels)) {
        put3 ("The ", ln, " takes no fuel: it burns away.");
        return IR_NO;
    }
    room = l.fuel < b->life * 10 ? b->life * 10 - l.fuel : 0;
    wsel = iq.osel;                             /* (The fuel's own selector) */
    if (*iq.into) {
        c = at_hand (iq.into);
    } else {
        for (c = 0; (c = items (c, IN_ACTOR | IN_ITEM, 0)) != 0; ) {    /* (The first carried it takes, not empty) */
            fetch (&f, c);
            if (f.fuel && (b = burn_of (f.kind)) != 0 && (b->fuels & takes) && !(kind_of (f.kind)->flags & K_LIGHT) &&
                carried (c)) {
                break;
            }
        }
    }
    if (!c) {
        put3 ("You have nothing to fill it with: ", takes & FU_PITCH ? "oil, or pitch" : "a flask of oil", ".");
        return IR_NO;
    }
    make_own (c);
    fetch (&f, c);
    strcpy (fn, kind_of (f.kind)->name);
    if ((kind_of (f.kind)->flags & K_LIGHT) || !(b = burn_of (f.kind)) || !(b->fuels & takes)) {
        put3 ("The ", ln, " won't burn that.");
        return IR_NO;
    }
    if (!f.fuel) {
        put3 ("The ", fn, " is empty.");
        return IR_NO;
    }
    if (!room) {
        put3 ("The ", ln, " has all it can take.");
        return IR_NO;
    }
    if (!(iq.to & IO_DONE)) {
        reach_from (n);                         /* (The light and its fuel got out first: their reach, HOUSE.md's) */
        reach_from (c);
    }
    hub_map (0);
    HUB->ireq.x = takes == FU_OIL ? 200 : 100;
    if (!(iq.to & IO_DONE)) {
        HUB->ireq.qty = LT_FILL;                /* (Begun: its first words the UI's) */
        HUB->ireq.item = l.kind;
        return IR_OK;
    }
    HUB->ireq.x = HUB->ireq.qty = 0;            /* (Done: its time spent already) */
    give = room < f.fuel ? room : f.fuel;
    item_at (n)->fuel = l.fuel + give;
    item_at (c)->fuel = f.fuel - give;
    put3 (takes == FU_OIL ? "You fill the " : "You soak the ", ln, takes == FU_OIL ? " from the " : " in the ");
    put3 (fn, give == f.fuel ? ", and it's empty now" : "", ".");
    return IR_OK;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 13: the store's rarer work: begun, kept, the kit, the theme's kinds, a chunk's things and the remains
** put down */
#pragma code-name (push, "SW13CODE")
#pragma rodata-name (push, "SW13RODATA")

/* (Part 13's, with the kit that makes its things its own, ten at a time: once part 11's, the kit put out and in
** again for each) The materials' toughness (items.h's MT_*) */
static const unsigned char tough[NMATS] = MAT_TOUGH;

/* Its own numbers, for the things' differences (not the game's: those are the referee's and the maker's) */
unsigned irs = 0x2545;                          /* (ipage.c's begun: the game's seed's) */
static unsigned char __fastcall__ rnd (unsigned char n)
{
    irs = irs * 25173U + 13849U;
    return (irs >> 8) % n;
}

/* ---- A thing made its own, as the world first comes to it (no two alike): its number (record 0 keeps the last
** given), its quality (most are middling), its material (a blade of iron, of steel if it's well made, of bronze if
** crudely), its durability's most (its material's toughness, its weight, its quality) and what's left of it (a thing
** found lying in the dungeon is the worse for its years there); a portal's part's material and quality the maker's
** already (its portal's) */
void __fastcall__ s_make_own (unsigned i)
{
    struct item q;
    struct item* p;
    const struct kind* k;
    const struct burn* b;
    unsigned id, life;
    unsigned char m, h, a;

    fetch (&q, i);
    if (q.id || !q.kind) {
        return;
    }
    p = item_at (0);
    id = ++p->x;
    q.id = id;
    k = kind_of (q.kind);
    if (!(m = q.mat)) {
        q.quality = rnd (4) + rnd (4) + rnd (3);
        if (rnd (8) == 0) {
            ++q.quality;
        }
        m = k->mat;
        if (m == MT_IRON && !strcmp (k->tool, "blade")) {
            m = q.quality >= 7 ? MT_STEEL : q.quality <= 1 ? MT_BRONZE : MT_IRON;
        }
        if (m == MT_TALLOW && q.quality >= 6) {
            m = MT_WAX;                         /* (A fine candle beeswax, a common one tallow) */
        }
        q.mat = m;
    }
    q.dur_max = tough[m] * (k->weight + 4) * (5 + q.quality) / 5;
    q.dur = q.where == IW_SQUARE ? q.dur_max - q.dur_max / 10 * rnd (7) : q.dur_max;
    if (!q.dur) {
        q.dur = 1;
    }
    if ((b = burn_of (q.kind)) != 0) {          /* (A light's life, a fuel's measure: slice 4.1's, HOUSE.md's burning;
                                                **   no dice, its number's: the things' numbers, rnd's, as they were) */
        life = b->life * 10;
        h = (unsigned char) (id * 37 + (id >> 3));
        if ((k->flags & K_LIGHT) && !b->fuels) {    /* (One that burns away, a candle: by its thickness, half an inch
                                                **   to an inch and a quarter, its wax, beeswax half as long again
                                                **   as tallow, and its make) */
            q.thick = 4 + 2 * (h & 3);
            life = life / 6 * q.thick * (m == MT_WAX ? 3 : 2) / 2 * (15 + q.quality) / 19;
        }
        if (q.where == IW_SQUARE) {             /* (Found lying: part of it used before it was left, a sixteenth less a
                                                **   century of its stratum's age; a candle some left still, a
                                                **   lantern's oil dried, a torch's pitch crumbled) */
            a = STRATUM (q.sn).age;
            h = h % 12 + 2;
            h = h > a ? h - a : !b->fuels;
            life = life * h / 16;
        }
        q.fuel = life;
    }
    if ((m = k->keeps & KF_CLOSES) != 0) {     /* (What closes it, the user's, 2026-10-21: one found closed, a
                                                **   chest's lid locked one time in three, by its number, no dice; the
                                                **   character's own open, but what's stoppered) */
        q.shut = q.where == IW_SQUARE || m == KF_STOPPER ? IS_CLOSED : 0;
        if (q.kind == KI_CHEST) {
            q.shut |= IS_LOCK | (q.where == IW_SQUARE && (unsigned char) (id * 53 + (id >> 2)) % 3 == 0 ? IS_LOCKED : 0);
        }
    }
    memcpy (item_at (i), &q, sizeof q);
}

/* The things a look saw (the senses' record's, in the order they're in it), each made its own (as the senses did as
** they saw them: the things' numbers, rnd's, the same) */
void s_seen_owned (void)
{
    unsigned char k, n;
    unsigned i;

    for (k = 0; hub_map (SIGHT_BANK), n = SIGHT->nitems, k < n; ++k) {
        i = SIGHT->item[k].n;
        s_make_own (i);
    }
    hub_map (0);
}

/* A free record taken, its thing put down (where, sn, x, y, qty: the rest none, not yet its own): its number, or 0
** (the store full: a chunk of it put out, ipage.c's, if one can be) */
unsigned ilim = NITEMS;                         /* (The store's records used, at most: dq0 -C's fewer) */
static unsigned __fastcall__ put_down (unsigned char kind, unsigned char where, unsigned char sn, unsigned x,
    unsigned y, unsigned char qty)
{
    struct item* p;
    unsigned i;

    for (i = 1; i < ilim || it_room () && (i = 1); ++i) {   /* (Record 0 is never one: "none") */
        p = item_at (i);
        if (!p->kind) {
            memset (p, 0, sizeof *p);
            p->kind = kind;
            p->where = where;
            p->sn = sn;
            p->x = x;
            p->y = y;
            p->qty = qty;
            return i;
        }
    }
    return 0;
}

/* The theme's kinds (the theme bank's, the UI's: theme.h) into the catalogue, after its own */
void s_theme_kinds (void)
{
    struct kind k;
    unsigned char i;

    for (i = 0; i < NTKINDS; ++i) {
        hub_map (THEME_BANK);
        memcpy (&k, &THEME->kind[i], sizeof k);
        hub_map (CAT_BANK);
        memcpy ((struct kind*) CATALOGUE + KI_THEME + i, &k, sizeof k);
    }
    hub_map (0);
}

/* A name said (named's, part 10's: a copy of its own) */
static unsigned char __fastcall__ a_word (const char* k, const char* w)
{
    unsigned char n = strlen (w);

    for (;;) {
        if (!strncmp (k, w, n)) {
            return 1;
        }
        if (!(k = strchr (k, ' '))) {
            return 0;
        }
        ++k;
    }
}

/* A kind by its name (or a word of it: "sand"; the whole name first, so "sword" isn't a broken one): its number,
** or 0 */
static unsigned char __fastcall__ kind_named (const char* name)
{
    unsigned char k;

    for (k = 1; k < NK; ++k) {
        if (!strcmp (kind_of (k)->name, name)) {
            return k;
        }
    }
    for (k = 1; k < NK; ++k) {
        if (a_word (kind_of (k)->name, name)) {
            return k;
        }
    }
    return 0;
}

/* A thing made for the character (its kit's): a record taken, made its own: its number, or 0xFFFF */
static unsigned __fastcall__ item_new (unsigned char kind, unsigned char where, unsigned x, unsigned char qty)
{
    unsigned i = put_down (kind, where, 0, x, 0, qty);

    if (!i) {
        return 0xFFFF;
    }
    s_make_own (i);
    return i;
}

/* ---- The character's kit (a stand-in for the theme's and the occupation's): a torch lit in hand, a backpack worn
** (rations, a tinderbox in it), a belt (a pouch of pennies, a dagger, a waterskin hanging from it); and things more
** (dq0 -k, commas between: in the hub's text), each held if there's a hand for it, else in the pack; then a spare
** torch and a flask of oil in the pack (D5: slice 4.1's) */
static const unsigned char spare[] = { KI_TORCH, KI_OIL };

/* Each thing: its kind, what it's in (an earlier one's place in the list; 0xFF: on the character), how many, its
** flags */
#define NKIT    9
static const unsigned char kit_list[NKIT][4] = {
    { KI_TORCH, 0xFF, 1, I_HELD | I_LIT | I_KNOWN }, { KI_BACKPACK, 0xFF, 1, I_WORN | I_KNOWN }, { KI_RATIONS, 1, 3, 0 },
    { KI_TINDERBOX, 1, 1, 0 }, { KI_BELT, 0xFF, 1, I_WORN | I_KNOWN }, { KI_POUCH, 4, 1, 0 }, { KI_PENNIES, 5, 12, 0 },
    { KI_DAGGER, 4, 1, 0 }, { KI_WATERSKIN, 4, 1, 0 }
};

void s_kit (void)
{
    unsigned made[NKIT];
    const unsigned char* e;
    unsigned i;
    unsigned char k, off;
    char* c;
    char* more = tbuf;

    hub_map (UI_BANK);
    strncpy (more, UIREC->things_text, 63);     /* (The things more: the UI's -k, in the UI bank's text) */
    hub_map (0);
    more[63] = 0;
    off = HUB->pcsum.limbs & LB_LEFT ? IH_RIGHT : IH_LEFT;
    if (HUB->pcsum.limbs & (off == IH_LEFT ? LB_ARM_L : LB_ARM_R)) {
        off ^= IH_BOTH;                         /* (Its off arm useless: its torch in the other) */
    }
    for (k = 0; k < NKIT; ++k) {
        e = kit_list[k];
        made[k] = i = item_new (e[0], e[1] == 0xFF ? IW_ACTOR : IW_ITEM, e[1] == 0xFF ? 0 : made[e[1]], e[2]);
        item_at (i)->flags = e[3];
        if (e[3] & I_HELD) {
            item_at (i)->sn = off;              /* (Its torch a light: in its off hand, slice 4.1b's) */
        }
    }
    for (; *more; more = c) {
        if ((c = strchr (more, ',')) != 0) {
            *c++ = 0;
        } else {
            c = more + strlen (more);
        }
        if ((k = kind_named (more)) != 0 && (i = item_new (k, IW_ACTOR, 0, 1)) != 0xFFFF && !carry (i, 1)) {
            item_at (i)->where = IW_ACTOR;      /* (No room: held all the same) */
            item_at (i)->x = 0;
            item_at (i)->flags = (item_at (i)->flags & ~(I_WORN | I_HIDDEN)) | I_KNOWN | I_HELD;
            k = two (kind_at (i)) == 2 ? IH_BOTH : off ^ IH_BOTH;
            item_at (i)->sn = k;                /* (Its hand: both for a two-handed thing, else the main) */
        }
    }
    for (k = 0; k < sizeof spare; ++k) {        /* (The pack's spare torch and flask of oil, D5's, after the things
                                                **   more: theirs made as they were) */
        item_new (spare[k], IW_ITEM, made[1], 1);
    }
}

/* Cache slot k's chunk's things put down, as the maker listed them stocking it (the world bank's stock: a thing on a
** square of the chunk, concealed or not; a portal's part, in its portal or by it, its make the portal's; a sconce's
** torch in it, a kept room's lit and tended: DUNGEON.md, light kept), in the list's order (the hub's header in the
** window after) */
void __fastcall__ s_stock_down (unsigned char k)
{
    struct sthing e;
    struct item* p;
    unsigned x0, y0, i, prev = 0;
    unsigned char sn, n, j, m, w;

    hub_map (WORLD_BANK);
    sn = WREC->cslot[k].sn;
    x0 = WREC->cslot[k].cx;
    y0 = WREC->cslot[k].cy;
    it_need (sn, x0, y0);                       /* (Its chunk's things in the store: none yet, as a rule) */
    hub_map (WORLD_BANK);
    x0 = WREC->strata[SLOT (sn)].x0 + (WREC->cslot[k].cx << CW_BITS);
    y0 = WREC->strata[SLOT (sn)].y0 + (WREC->cslot[k].cy << CW_BITS);
    n = WREC->stock[k].n;
    for (j = 0; j < n; ++j) {
        hub_map (WORLD_BANK);
        memcpy (&e, &WREC->stock[k].e[j], sizeof e);
        w = e.hide == ST_PORTAL ? IW_PORTAL : e.hide == ST_IN || e.hide == ST_LIT ? IW_ITEM : IW_SQUARE;
        if (!(i = put_down (e.kind, w, sn, w == IW_ITEM ? prev : x0 + e.x, y0 + e.y, e.qty))) {
            break;                              /* (The store full: no more) */
        }
        prev = i;
        p = item_at (i);
        if (e.hide < ST_LIT) {
            p->flags = I_HIDDEN;
            p->hide = e.hide;
        }
        if (e.hide == ST_LIT) {                 /* (A kept room's sconce's torch: the room lit by it, store.c's) */
            p->flags = I_LIT | I_TENDED;
        }
        if (e.make) {                           /* (A portal's part's make: the portal's) */
            p->mat = m = e.make >> 4;
            p->quality = e.make & 15;
            p->size = e.size;
            p->thick = e.kind == KI_PANE ? 2 : e.kind == KI_BARS ? 8 : m == MT_WOOD ? 24 : 12;
        }
    }
    hub_map (0);
}

/* A denizen's remains put down where it fell (kind: KI_BODY, KI_BONES; the world's, wbody.c's: it died), or a thing
** of its (its weapon let fall: wlimbs.c's): its item's number (0 none) */
unsigned __fastcall__ s_lay_body (unsigned char kind, unsigned char sn, unsigned x, unsigned y)
{
    unsigned i;

    it_need (sn, (x - STRATUM (sn).x0) >> CW_BITS, (y - STRATUM (sn).y0) >> CW_BITS);  /* (Its chunk's things in the
                                                **   store first) */
    i = put_down (kind, IW_SQUARE, sn, x, y, 1);
    hub_map (0);
    return i;
}

/* (Part 13's: a rare command's) Hidden where the character stands (the user: hiding is a skill, Concealment; a thing's difficulty to find by the
** method's quality and the concealer's skill): its difficulty, in steps, the referee's test's (rs.h's P_CONCEAL: 12
** at a critical success, 6 a marginal one, 2 a marginal failure; at a critical failure 0, a floor's dust and corners
** only); known to the character, who hid it */
unsigned char __fastcall__ drop (const char* name);
unsigned char __fastcall__ s_hide (const char* name)
{
    struct req* r;
    signed char g;
    unsigned char d;
    unsigned n;

    if (!(iq.to & IO_DONE)) {                   /* (Begun: a minute's work, a timed work, HOUSE.md's long tasks: hidden,
                                                **   the test made, at its end, IO_DONE, the UI's) */
        if (!(n = carried_named (name))) {
            return IR_NONE;
        }
        n = item_kind (n);
        hub_map (0);
        HUB->ireq.item = n;
        HUB->ireq.qty = LT_HIDE;
        HUB->ireq.x = 600;
        return IR_OK;
    }
    if (drop (name) != IR_OK) {
        return IR_NONE;
    }
    hub_map (0);
    r = &HUB->slot[SLOT_UI];                    /* (The UI's: it waits on this request meanwhile) */
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_CONCEAL;
    r->b = 0;
    r->steps = 0;
    r = hub_ask (SLOT_UI);
    g = r->grade;
    r->state = REQ_FREE;
    d = g == G_CS ? 12 : g == G_MS ? 6 : g == G_MF ? 2 : 0;
    item_at (dropped)->flags |= I_HIDDEN | I_KNOWN;
    item_at (dropped)->hide = d;
    hub_map (0);
    HUB->ireq.qty = d;
    return IR_OK;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== In the catalogue's bank (dq0w.cfg's CAT: linked at $8000 after the kinds and their words, and run with that
** bank in the window, serve's o_handle and fits' o_fits): the things handled.  Its code never touches the window: a
** main-RAM function that does is called through m0, m1 or m2 (bank.s: the catalogue's bank put back after it; its
** arguments 16 bits, its result a char's in the low byte), part 10's through its stub (part 10 in the area as it's
** called: serve's), an item's record copied out (fetch) and back (store); the kinds read where they are (kinds.c's
** kinds) */
#pragma code-name (push, "CATCODE")
#pragma rodata-name (push, "CATRODATA")

unsigned __fastcall__ m0 (void* f);
unsigned __fastcall__ m1 (void* f, unsigned a);
unsigned __fastcall__ m2 (void* f, unsigned a, unsigned b);

static struct item o;                           /* (A record copied out, for the code here) */

#define B(v)            ((unsigned char) (v))   /* (A char's result) */
#define KIND(i)         (&kinds[B (m1 (item_kind, i))])
#define FETCH(i)        m2 (fetch, (unsigned) &o, i)

static void __fastcall__ say (unsigned char n)
{
    static const char* s;
    static char* d;

    last_sa = n;
    d = tbuf;
    for (s = said[n]; *s; ++s) {                /* (Its words made in the main RAM, then put in the hub's text) */
        if (*s == '%' || *s == '#') {
            strcpy (d, *s == '%' ? xn : yn);
            d += strlen (d);
        } else {
            *d++ = *s;
        }
    }
    *d = 0;
    m1 (put, (unsigned) tbuf);
}

static unsigned char __fastcall__ no (unsigned char n)
{
    say (n);
    return IR_NO;
}

static unsigned char __fastcall__ yes (unsigned char n, unsigned char t)
{
    say (n);
    took = t;
    return IR_OK;
}

/* A thing's kind's name, as it's said (x: the thing's, else the other's) */
static void __fastcall__ name_of (unsigned i, unsigned char x)
{
    strcpy (x ? xn : yn, KIND (i)->name);
}

/* Thing n put somewhere (where, x: items.h's), as it's flagged (held, worn, or neither), no longer hidden (nor
** tended: a keepers' light, taken, burns down); its flags else (set, and cleared) */
static void __fastcall__ o_place (unsigned n, unsigned char where, unsigned x, unsigned char flags)
{
    FETCH (n);
    o.where = where;
    o.x = x;
    o.flags = (o.flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | flags;
    m2 (store, (unsigned) &o, n);
}

static void __fastcall__ o_flags (unsigned n, unsigned char set, unsigned char clear)
{
    FETCH (n);
    o.flags = (o.flags & ~clear) | set;
    m2 (store, (unsigned) &o, n);
}

/* Whether thing n can go in c (or on it: a surface; hung from it: a belt): 0, or the words why not (SA_*): its
** form (a basket won't keep sand, nor water; a sack won't keep water), lit (a flame in a pack), its room (a belt's
** slots), the weight it bears; never into itself, nor into what's in it */
static unsigned char __fastcall__ o_fits (unsigned n, unsigned c)
{
    static const struct kind* k;
    static unsigned j, bear;
    static unsigned char kf, form, lit, cf, keeps, room, slots;

    k = KIND (n);
    kf = k->flags;
    form = k->form & KF_FORMS ? k->form & KF_FORMS : KF_SOLID;
    FETCH (n);
    lit = o.flags & I_LIT;
    k = KIND (c);
    cf = k->flags;
    keeps = k->keeps;
    room = k->cap_bulk;
    bear = k->cap_weight;
    slots = k->slots;
    for (j = c; j; j = o.where == IW_ITEM ? o.x : 0) {
        if (j == n) {
            return SA_ITSELF;
        }
        FETCH (j);
    }
    if (cf & K_BELT) {
        m1 (bulk_in, c);
        return !(kf & K_HANG) ? SA_NO_HANG : nin >= slots ? SA_NO_SLOT : 0;
    }
    if (!(cf & K_CONTAINER)) {
        if (!(keeps & KF_ON)) {
            return SA_NOT_HOLDER;
        }
    } else if (!(keeps & form)) {
        return form == KF_SOLID ? SA_WONT_GO : SA_RUN_OUT;
    } else if (lit) {
        return SA_PUT_OUT;
    }
    FETCH (c);
    if (o.shut & IS_CLOSED) {
        return SA_SHUT;                         /* (Closed: its lid down, its tie tied, its stopper in) */
    }
    if (m1 (bulk_in, c) + m1 (bulk_of, n) > room) {
        return SA_NO_ROOM;
    }
    return m2 (weight_in, IW_ITEM, c) + m1 (weight_of, n) > bear ? SA_TOO_HEAVY : 0;
}

/* The other thing (iq's into: carried, or within reach), named in yn: its number; else 0, its words why */
static unsigned other (void)
{
    static unsigned c, f;

    f = wforce;                                 /* (The first's, a count's: not this one's) */
    wforce = 0;
    wsel = iq.osel;                             /* (Its own selector; of the same name, not the first, nor one in it:
                                                **   "put the sack in the sack", the user's) */
    wnot = strcmp (iq.into, iq.name) ? 0 : handled;
    c = m1 (at_hand, (unsigned) iq.into);
    wsel = iq.sel;
    wnot = 0;
    wforce = f;
    if (c) {
        name_of (c, 0);
    } else {
        strcpy (xn, iq.into);
        no (afar ? SA_REACH : SA_NO_OTHER);
    }
    return c;
}

/* Into a hand (from anywhere on the character, or within reach): IO_HOLD's the one its to says, else the one that suits
** it (part 48's, and its words) */
static unsigned char __fastcall__ hold (unsigned n)
{
    return B (m2 (hold_in, n, iq.kind == IO_HOLD ? iq.to : 0));
}

/* Put in the pack (put 0); or in the other thing, on it (iq's to 1: a surface's; a belt's: hung from it; none named:
** the belt worn) */
static unsigned char __fastcall__ stow (unsigned n, unsigned char put)
{
    static unsigned c;
    static unsigned char r, cf;

    m0 (about);
    if (!put || !*iq.into) {
        if (!(c = put ? belt : pack)) {
            return no (put ? SA_NO_BELT : SA_NO_PACK);
        }
        name_of (c, 0);
    } else if (!(c = other ())) {
        return IR_NO;
    }
    FETCH (n);
    if (o.where == IW_ITEM && o.x == c) {
        return no (SA_THERE);
    }
    if ((r = o_fits (n, c)) != 0) {
        return no (r);
    }
    cf = KIND (c)->flags;
    r = KIND (c)->keeps;
    o_place (n, IW_ITEM, c, 0);
    cont = c;
    return yes (cf & K_BELT ? SA_HANG : (r & KF_ON) && B (m1 (item_kind, c)) != KI_SCONCE ? SA_PUT_ON : c == pack ?
        SA_STOW : SA_PUT_IN, 20);               /* (A sconce's: in it) */
}

/* The thing named (iq's name) in the other thing (iq's into: other's, in c), or on it: its number, named in xn; else
** 0, its words why */
static unsigned c;
static unsigned in_other (void)
{
    static unsigned n;

    if (!(c = other ())) {
        return 0;
    }
    FETCH (c);
    if (o.shut & IS_CLOSED) {
        no (SA_SHUT);
        return 0;
    }
    win = c;                                    /* (In it, by name, of several the one the selector says) */
    if (!(handled = n = m2 (pick, (unsigned) iq.name, HW_IN))) {
        no (SA_NOT_IN);
        return 0;
    }
    name_of (n, 1);
    return n;
}

/* Taken from the other thing: into the pack, else a hand (from what's on the character: into a hand) */
static unsigned char take_from (void)
{
    static unsigned n;
    static unsigned char r;

    if (!(n = in_other ())) {
        return IR_NO;
    }
    m1 (reach_from, n);
    if (B (m1 (on_pc, c))) {
        return hold (n);
    }
    if (!(r = B (m2 (carry, n, 0)))) {
        return no (SA_HANDS);
    }
    say (SA_TAKE);
    name_of (pack, 0);                          /* (carry's about's) */
    return yes (r == 1 ? SA_IN_PACK : SA_IN_HAND, 20);
}

/* Worn (a thing held in the hand, not worn: held) */
static unsigned char __fastcall__ don (unsigned n)
{
    static unsigned i;
    static unsigned char k, kf;

    k = B (m1 (item_kind, n));
    kf = kinds[k].flags;
    if (!(kf & K_WEAR)) {
        return kf & K_HAND ? hold (n) : no (SA_NO_WEAR);
    }
    for (i = 1; i < NITEMS; ++i) {              /* (One of a kind worn: a second pack, a second cloak, no) */
        FETCH (i);
        if (o.kind == k && o.where == IW_ACTOR && !o.x && (o.flags & I_WORN)) {
            return no (i == n ? SA_WORN : SA_WEAR_ONE);
        }
    }
    o_place (n, IW_ACTOR, 0, I_WORN);
    return yes (SA_DON, 50);
}

/* Taken off: into a hand, else the pack */
static unsigned char __fastcall__ doff (unsigned n)
{
    static unsigned char r;

    FETCH (n);
    if (!(o.flags & I_WORN)) {
        return no (SA_NOT_WORN);
    }
    if (!(r = B (m2 (carry, n, 1)))) {
        return no (SA_HANDS);
    }
    say (SA_DOFF);
    name_of (pack, 0);
    return yes (r == 1 ? SA_IN_PACK : SA_IN_HAND, 50);
}

/* ---- Light: a flame to hand (part 10's flame, its room) */
unsigned char flame (void);

/* A light spent (its fuel all burnt: a lantern dry, a torch burnt out; slice 4.1's): its words why it won't light, or
** 0 (o's) */
static unsigned char spent (void)
{
    static const struct burn* b;

    return !o.id || o.fuel || !(b = (const struct burn*) m1 (burn_of, o.kind)) ? 0 : b->fuels == FU_OIL ? SA_NO_OIL :
        SA_BURNT;
}

/* Lit, or put out (on 0): the thing named, else the first carried that would be (one that would be first, of those
** named: a spare torch, not the one burnt out; or one within reach, named) */
static unsigned char __fastcall__ light (unsigned char on)
{
    static unsigned n, c;
    static unsigned char r, x;

    for (n = 1; n < NITEMS && *iq.name != '#' && !*iq.into; ++n) {
        FETCH (n);
        if (o.kind && (kinds[o.kind].flags & K_LIGHT) && !(o.flags & I_LIT) == on && (!on || !spent ()) &&
            (!*iq.name || named (kinds[o.kind].name, iq.name)) && B (m1 (on_pc, n))) {
            break;
        }
    }
    if (n >= NITEMS || *iq.name == '#' || *iq.into) {
        if (!*iq.name) {
            return no (on ? SA_NONE_TO_LIGHT : SA_NONE_LIT);
        }
        if (*iq.into) {                         /* (The one in the other, or on it: a sconce's torch) */
            if (!(n = in_other ())) {
                return IR_NO;
            }
        } else if (!(n = m1 (at_hand, (unsigned) iq.name))) {
            return no (afar ? SA_REACH : SA_NO_SUCH);
        }
    }
    name_of (n, 1);
    FETCH (n);
    r = o.flags & I_LIT;
    x = o.where == IW_ITEM && !(KIND (o.x)->flags & K_BELT);  /* (In a container: the pack ...) */
    if (!(kinds[o.kind].flags & K_LIGHT)) {
        return no (SA_NO_LIGHT);
    }
    if (!on) {
        if (!r) {
            return no (SA_NOT_LIT);
        }
        o_flags (n, 0, I_LIT);
        return yes (SA_DOUSE, 0);
    }
    if (r) {
        return no (SA_LIT);
    }
    if ((r = spent ()) != 0) {
        return no (r);                          /* (Its fuel all burnt: renewed first, a torch, a lantern) */
    }
    if (!B (m0 (flame))) {
        return no (SA_NO_FLAME);
    }
    if ((c = m1 (shut_in, n)) != 0) {
        name_of (c, 0);                         /* (In a thing closed: opened first) */
        return no (SA_SHUT);
    }
    if (x && B (m1 (on_pc, n)) && (m1 (reach_from, n), (r = hold (n)) != IR_OK)) {
        return r;                               /* (Out of the pack first, into a hand: its reach) */
    }
    m0 (text_new);                              /* (Its words alone) */
    o_flags (n, I_LIT, 0);
    return yes (SA_LIGHT, 0);
}


/* A thing handled, as the request (iq) asks: IR_OK, or IR_NO (refused: the words why) */
static unsigned char o_handle (void)
{
    static unsigned n, c2;
    static unsigned char op;

    op = iq.kind;
    m0 (text_new);
    took = 0;
    strcpy (xn, iq.name);
    strcpy (yn, iq.into);
    if (op == IO_BAR) {
        return B (m0 (bar_hand));      /* (Part 35's) */
    }
    if (op == IO_LIGHT || op == IO_DOUSE) {
        return light (op == IO_LIGHT);
    }
    if (op == IO_TAKE) {
        return take_from ();
    }
    if (!*xn) {
        return no (SA_WHAT);
    }
    if (op == IO_NAME) {                        /* (The UI's: a name given it, or taken back; carried, or seen) */
        if (!(n = m1 (carried_named, (unsigned) xn)) && !(n = m1 (seen_named, (unsigned) xn))) {
            return IR_NONE;
        }
        o_flags (n, iq.to ? I_NAMED : 0, I_NAMED);
        return IR_OK;
    }
    if (!(n = m1 (op == IO_DOFF ? (void*) carried_named : (void*) at_hand, (unsigned) xn))) {
        return no (op == IO_DOFF ? SA_NOT_WORN : afar ? SA_REACH : SA_NO_SUCH);
    }
    name_of (n, 1);
    handled = n;
    if (op == IO_FIND) {                        /* (The puzzle's: its kind, and 256 if it's carried) */
        took = B (m1 (item_kind, n)) | (unsigned) B (m1 (on_pc, n)) << 8;
        return IR_OK;
    }
    if (op == IO_COMBINE) {                     /* (The other gone, this one made the rule's) */
        if (!(c2 = other ()) || c2 == n) {
            return IR_NO;
        }
        FETCH (c2);
        o.kind = 0;
        m2 (store, (unsigned) &o, c2);
        FETCH (n);
        o.kind = iq.to;
        m2 (store, (unsigned) &o, n);
        return IR_OK;
    }
    m1 (reach_from, n);                         /* (Its reach: part 10's, HOUSE.md's) */
    if ((c2 = m1 (shut_in, n)) != 0) {
        name_of (c2, 0);                        /* (In a thing closed: "The backpack is closed.") */
        return no (SA_SHUT);
    }
    if (KIND (n)->keeps & KF_FIXED) {
        return no (SA_FIXED);                   /* (A theme's altar, its throne) */
    }
    if (op == IO_HOLD) {
        return hold (n);
    }
    if (op == IO_DON) {
        return don (n);
    }
    if (op == IO_DOFF) {
        return doff (n);
    }
    return op == IO_PUT && !*iq.into && !iq.to ? no (SA_WHERE) : stow (n, op == IO_PUT);
}

#pragma rodata-name (pop)
#pragma code-name (pop)

/* ==== Part 46 (its own, 2026-10-22, the user's: "search around for a dagger", "search the table", a search in the
** dark by touch): the thing a search is for or on (IO_SEEK), and what a search turns up (IO_SEARCH, part 11's once), by
** its form (the world's srch_form: SF_*, | SF_TOUCH in the dark: HOUSE.md's searching); what's on a thing or in it,
** part 47's */
#pragma code-name (push, "SW46CODE")
#pragma rodata-name (push, "SW46RODATA")
#pragma static-locals (push, on)

extern unsigned char srch_form;                 /* (dq0w.c's: the search under way's form) */
static char sname[16];                          /* (The thing's name, IO_SEEK's) */
static unsigned sitem;                          /* (The thing searched on, or the one the search will find: 0 none) */
static unsigned char sd2;                       /*   (how far off, squared) */
static const unsigned char d2_[7] = { 9, 4, 1, 0, 1, 4, 9 };   /* (A square's distance off, squared, by its offset) */
#define SEARCH_R        3
unsigned char __fastcall__ on_it (unsigned n, unsigned char touch);   /* (Part 47's) */

/* The referee's search test of the thing in rec, its hiding the steps worse (2 more by touch): 1 if it's found */
static unsigned char __fastcall__ turned_up (unsigned char touch)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_UI];                    /* (The UI's: it waits on this request meanwhile) */
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_SEARCH;
    r->b = HIDE_MECHANICAL;
    r->steps = -(signed char) rec.hide - (touch ? 2 : 0);
    r = hub_ask (SLOT_UI);
    g = r->grade;
    r->state = REQ_FREE;
    return g >= G_MS;
}

/* The things lying on the squares within the search's reach (a square's distance off, squared, less than r) that the
** character sees (by touch, all of them), but what's hidden by magic: seek, the nearest named sname that it would find
** (in plain sight, or hidden and found by the referee's test, made now), sitem, sd2 its distance off squared; else each
** it finds (in the light, what's hidden alone, by the test; by touch, everything there, what's hidden by the test, the
** rest felt plainly), made known and worded ("You turn up a dagger", "Your hands find a sack, and a chisel"): how many */
static unsigned char __fastcall__ scan (unsigned char seek, unsigned char r, unsigned char touch)
{
    unsigned i = 0, x, y, dx, dy;
    unsigned char sn, d, n = 0;
    unsigned char* t;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    while ((i = items (i, IN_SQUARE, 0)) != 0) {
        fetch (&rec, i);
        if (rec.sn != sn || (rec.flags & I_MAGIC) || (dx = rec.x - x + SEARCH_R) > 2 * SEARCH_R ||
            (dy = rec.y - y + SEARCH_R) > 2 * SEARCH_R || (d = d2_[dx] + d2_[dy]) >= r ||
            (!touch && (!(t = at (sn, rec.x, rec.y)) || !(*t & T_VIS))) ||
            (seek ? d >= sd2 || !named (kind_of (rec.kind)->name, sname) :
            !touch && (rec.flags & (I_HIDDEN | I_KNOWN)) != I_HIDDEN)) {
            continue;
        }
        make_own (i);
        fetch (&rec, i);
        if ((rec.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN && !turned_up (touch)) {
            continue;                           /* (Hidden, and the search passes it by) */
        }
        if (seek) {
            sitem = i;
            sd2 = d;
            continue;
        }
        item_at (i)->flags = (rec.flags & I_KNOWN ? rec.flags : rec.flags & ~I_HIDDEN) | I_KNOWN;  /* (Found: known; one
                                                **   it hid, felt there, hidden still) */
        put (n++ ? ", and " : touch ? "Your hands find " : "You turn up ");
        thing_words (&rec);
    }
    return n;
}

/* A search's thing (IO_SEEK: the request's name, to its form, | SF_TOUCH by touch): on or in one (SF_ON), the one most
** to hand that it sees (in the dark, the nearest within reach it feels: its own square's or the eight round it's),
** IR_NONE, none; IR_NOT_HERE, seen out of reach (where it lies in x, y: the UI's to walk over to); for one (SF_FOR),
** the nearest of that name the search would come to and find, as far as it reaches (in the light the squares it sees
** within 3, by touch within reach): the time to find it back in x (3 seconds, and 3.2 more a square's distance off,
** squared, as a search's reach grows: wtrap.c's), or 0 (none: a whole search), told as the search ends */
unsigned char s_seek (void)
{
    unsigned char f = iq.to & 7, touch = iq.to & SF_TOUCH;

    strcpy (sname, iq.name);
    sitem = 0;
    sd2 = 0xFF;
    hub_map (0);
    HUB->ireq.x = 0;
    if (f == SF_ON && !touch) {
        if ((sitem = pick (sname, HW_HAND)) != 0) {
            reach_to (sitem);                   /* (Carried, or within reach: its reach, a pack worn taken off to
                                                **   search it, and put on again, HOUSE.md's) */
            return IR_OK;
        }
        if (!afar) {
            return IR_NONE;
        }
        hub_map (0);                            /* (Seen out of reach: walked over to first, the UI's) */
        HUB->ireq.x = far_x;
        HUB->ireq.y = far_y;
        HUB->ireq.item = far_n;
        return IR_NOT_HERE;
    }
    scan (1, touch || f == SF_ON ? 3 : 19, touch);
    hub_map (0);
    if (f == SF_ON) {
        return sitem ? IR_OK : IR_NONE;
    }
    HUB->ireq.x = sitem ? 30 + 32 * sd2 : 0;
    return IR_OK;
}

/* A search's end (IO_SEARCH: the request's to its reach, the world's, wtrap.c's: a square searched if its distance off,
** squared, is less): what it turns up, in the hub's text, by its form; the number found (or 1: words, found or not).
** About the character, nearby, on the floor: scan's.  For a thing: the one IO_SEEK found, if the search's reach came to
** it ("You find a dagger.", "You find no dagger here."); on or in one, part 47's on_it; a wall's, none (the walls'
** alone) */
unsigned char s_search (void)
{
    unsigned char f = srch_form & 7, touch = srch_form & SF_TOUCH, n;

    text_new ();
    if (f == SF_WALL) {
        return 0;
    }
    if (f == SF_ON) {
        return sitem ? on_it (sitem, touch) : 0;
    }
    if (f != SF_FOR) {
        if ((n = scan (0, iq.to, touch)) != 0) {
            put (".");
        }
        return n;
    }
    if (sitem && sd2 < iq.to) {                 /* (Come to it as the search went: found) */
        fetch (&rec, sitem);
        item_at (sitem)->flags = (rec.flags & I_KNOWN ? rec.flags : rec.flags & ~I_HIDDEN) | I_KNOWN;
        put (touch ? "Your hands find " : "You find ");
        thing_words (&rec);
        put (".");
    } else {
        put (touch ? "Your hands find no " : "You find no ");
        put (sname);
        put (" here.");
    }
    return 1;
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 47 (its own, 2026-10-22): what's on a thing, or in it, searched (part 46's search's SF_ON: thing n): its
** things listed; closed, not looked into; then it gone over for traps (examine's, part 40's look_more: "The chest shows
** no sign of a trap ..."), one rule for it; 1 (its words) */
#pragma code-name (push, "SW47CODE")
#pragma rodata-name (push, "SW47RODATA")
#pragma static-locals (push, on)

unsigned char __fastcall__ s_on_it (unsigned n, unsigned char touch)
{
    unsigned char on;

    make_own (n);
    fetch (&rec, n);
    memcpy (&lk, kind_of (rec.kind), sizeof lk);
    lpl = (lk.flags & K_PLURAL) || rec.qty > 1;
    on = (lk.keeps & KF_ON) != 0;
    if (rec.shut & IS_CLOSED) {
        put (lpl ? "They're closed: you go over the outside of them. " : "It's closed: you go over the outside of it. ");
    } else if (items (0, IN_X | IN_ITEM, n)) {
        put (touch ? "Your hands find " : on ? "On the " : "In the ");
        if (!touch) {
            put (lk.name);
            put (" you find ");
        }
        listed (n);
        if (touch) {
            put (on ? " on the " : " in the ");
            put (lk.name);
        }
        put (". ");
    } else {
        put (on ? "There's nothing on the " : "There's nothing in the ");
        put (lk.name);
        put (". ");
    }
    put ("The ");
    put (lk.name);
    fetch (&rec, n);
    look_more (n, LM_TRAPS);                    /* (Its trap: examine's check, part 40's) */
    return 1;
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 48 (dq0w.c's work and what came of it, and here, its room): the hands (slice 4.1b; the user's, 2026-10-21:
** "is there handedness (right handed versus left versus ambidextrous) in the character profile?"): each thing held in
** the left hand, the right, or both (its sn, items.h's IH_*); the hand a thing taken up goes to, the one that suits it
** (a weapon or a tool the main hand, a shield or a light the other, a two-handed thing both, the one free if only one
** is), or the one said ("hold the sword in my left hand", "put the dagger in my other hand", "hold the spear in both
** hands"); a useless arm's hand holds nothing (the user's, 2026-10-22: "For an arm or hand, that means no picking things
** up with that hand"), nor a two-handed thing; what's in them changed over ("swap hands").  The main hand is the
** referee's (its summary's limbs: the medical traits', left-handed, ambidextrous) */
#pragma code-name (push, "SW48CODE")
#pragma rodata-name (push, "SW48RODATA")

static const char* const hword[4] = { "", "left", "right", "both" };
static unsigned char hmain, hdead, hused, hcur;  /* (The main hand, the useless ones, those holding something else, its
                                                **   own: hands_now's) */
static unsigned char hkind[2];                  /* (What's in the left hand and the right: their kinds, 0 none) */

/* What the character holds in its hands (held_k[0] the main hand's kind, or a thing's in both; held_k[1] the other
** hand's; 0 none), and the hand each is in (held_h, items.h's IH_*): its main hand the summary's, the left if it's
** left-handed (the hub's header in the window after) */
void s_held_kinds (void)
{
    unsigned i = 0;
    unsigned char m, h;

    hub_map (0);
    m = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    held_k[0] = held_k[1] = held_h[0] = held_h[1] = 0;
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if (ip->flags & I_HELD) {
            h = ip->sn & IH_BOTH;
            h = h ? h : m;
            if (h & m) {
                held_k[0] = ip->kind;
                held_h[0] = h;
            } else {
                held_k[1] = ip->kind;
                held_h[1] = h;
            }
        }
    }
    hub_map (0);
}

/* The hand that holds the thing that strikes (s, the hub's pc_strike: SK_THING | its kind), for the hub's pc_shand: the
** main hand's, if it holds one of that kind, else the other's; 0 none (a part of it, or the best it holds: the
** referee's) */
unsigned char __fastcall__ s_shand_of (unsigned char s)
{
    s_held_kinds ();
    return !(s & SK_THING) ? 0 : held_k[0] == (s & 0x7F) ? held_h[0] : held_k[1] == (s & 0x7F) ? held_h[1] : 0;
}

/* The character's hands as they are, but for thing n (n held: its own hand, hcur) */
static void __fastcall__ hands_now (unsigned n)
{
    unsigned i = 0;
    unsigned char l, h;

    hub_map (0);
    l = HUB->pcsum.limbs;
    hmain = l & LB_LEFT ? IH_LEFT : IH_RIGHT;
    hdead = (l & LB_ARM_L ? IH_LEFT : 0) | (l & LB_ARM_R ? IH_RIGHT : 0);
    hused = hcur = hkind[0] = hkind[1] = 0;
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if (ip->flags & I_HELD) {
            h = ip->sn & IH_BOTH ? ip->sn & IH_BOTH : hmain;
            if (h & IH_LEFT) {
                hkind[0] = ip->kind;
            }
            if (h & IH_RIGHT) {
                hkind[1] = ip->kind;
            }
            if (i == n) {
                hcur = h;
            } else {
                hused |= h;
            }
        }
    }
    hub_map (0);
}

/* The hand thing n would go in (IH_*) as want says (IH_*, IH_OTHER; 0 the one that suits it, the other if that's taken),
** its hands as they are (hands_now's); 0 none, and why not in hwhy: HW_FULL (no hand free), HW_TAKEN (the one said
** holds something else: in hside), HW_LIMB (an arm useless, hside's), HW_TWO (a two-handed thing, one hand said) */
enum { HW_FULL = 1, HW_TAKEN, HW_LIMB, HW_TWO };
static unsigned char hwhy, hside, htwo;
static unsigned char __fastcall__ hand_for (unsigned n, unsigned char want)
{
    const struct kind* k = kind_at (n);
    unsigned char h, off = hmain ^ IH_BOTH;

    htwo = two (k) == 2;
    if (htwo && want && want != IH_BOTH) {
        hwhy = HW_TWO;
        return 0;
    }
    if (htwo || want == IH_BOTH) {
        h = IH_BOTH;
    } else if (want == IH_OTHER) {
        h = hcur == IH_LEFT ? IH_RIGHT : hcur == IH_RIGHT ? IH_LEFT : off;
    } else if (want) {
        h = want;
    } else {
        h = (k->flags & K_LIGHT) || !strcmp (k->tool, "shield") ? off : hmain;    /* (What suits it) */
        if (h & (hused | hdead)) {
            h ^= IH_BOTH;                       /* (Taken, or its arm useless: the other) */
        }
    }
    hwhy = h & hdead ? HW_LIMB : !(h & hused) ? 0 : want ? HW_TAKEN : HW_FULL;
    hside = h & (hwhy == HW_LIMB ? hdead : hused);
    return hwhy ? 0 : h;
}

/* The hand thing n would go in, the one that suits it (carry's: a thing taken up, its hand first, else the pack); 0
** none free */
unsigned char __fastcall__ s_hand_free (unsigned n)
{
    hands_now (n);
    return hand_for (n, 0);
}

/* A hand's words ("your left hand", "both hands") */
static void __fastcall__ hand_words (unsigned char h)
{
    put (h == IH_BOTH ? "both hands" : "your ");
    if (h != IH_BOTH) {
        put (hword[h]);
        put (" hand");
    }
}

/* Why not, in words (hwhy's) */
static void why_not (void)
{
    const struct kind* k;

    if (hwhy == HW_FULL) {
        put ("Your hands are full: stow something, or put it down.");
        return;
    }
    if (hwhy == HW_TWO) {
        put ("The ");
        put (xn);
        put (" takes both hands.");
        return;
    }
    put ("Your ");
    put (hword[hside == IH_BOTH ? IH_LEFT : hside]);
    if (hwhy == HW_LIMB) {
        put (" arm is useless.");
        if (htwo) {
            put (" You can't hold the ");
            put (xn);
            put (" in one hand.");
        }
        return;
    }
    k = kind_of (hkind[hside == IH_RIGHT]);
    put (" hand holds the ");
    put (k->name);
    put (".");
}

/* Thing n taken into a hand (IO_HOLD's, the catalogue's bank's hold: from anywhere on the character, or within reach;
** its name in xn): the one want says (IH_*, IH_OTHER: the request's to), or, none said, the one that suits it; one held
** already moved to it.  Its words into the hub's text: IR_OK (its time, took) or IR_NO (why not) */
unsigned char __fastcall__ s_hold_in (unsigned n, unsigned char want)
{
    struct item* p;
    unsigned char h;

    hands_now (n);
    if (hcur && (!want || want == hcur || (want == IH_OTHER && hcur == IH_BOTH))) {
        put ("You hold the ");              /* (Held already: in that hand, or none said) */
        put (xn);
        put (" in ");
        hand_words (hcur);
        put (" already.");
        return IR_NO;
    }
    if (!(h = hand_for (n, want))) {
        why_not ();
        return IR_NO;
    }
    p = item_at (n);
    p->where = IW_ACTOR;
    p->x = 0;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD;
    p->sn = h;
    put (hcur ? "You shift the " : "You take the ");
    put (xn);
    put (hcur ? " to " : " in ");
    hand_words (h);
    put (".");
    took = 10;
    return IR_OK;
}

/* What's in the character's hands changed over, left to right and right to left (IO_SWAP's: "swap hands"), its words
** into the hub's text, its time in the request's x: IR_OK, or IR_NO (nothing held, a thing in both hands, a hand to go
** into useless) */
unsigned char s_swap (void)
{
    unsigned i = 0, l = 0, r = 0;
    unsigned char h;

    text_new ();
    hands_now (0);
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if (ip->flags & I_HELD) {
            h = ip->sn & IH_BOTH ? ip->sn & IH_BOTH : hmain;
            if (h == IH_BOTH) {
                strcpy (xn, kind_of (ip->kind)->name);
                put ("The ");
                put (xn);
                put (" takes both your hands.");
                return IR_NO;
            }
            if (h == IH_LEFT) {
                l = i;
            } else {
                r = i;
            }
        }
    }
    if (!l && !r) {
        put ("Your hands are empty.");
        return IR_NO;
    }
    if ((l && (hdead & IH_RIGHT)) || (r && (hdead & IH_LEFT))) {
        put (hdead & IH_RIGHT ? "Your right arm is useless." : "Your left arm is useless.");
        return IR_NO;
    }
    if (l) {
        item_at (l)->sn = IH_RIGHT;
    }
    if (r) {
        item_at (r)->sn = IH_LEFT;
    }
    if (l && r) {
        put ("You change hands: the ");
        put (kind_of (hkind[1])->name);
        put (" in your left, the ");
        put (kind_of (hkind[0])->name);
        put (" in your right.");
    } else {
        put ("You shift the ");
        put (kind_of (hkind[!l])->name);
        put (l ? " to your right hand." : " to your left hand.");
    }
    hub_map (0);
    HUB->ireq.x = 10;
    return IR_OK;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
