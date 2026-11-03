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
#include "outfit.h"

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
void kit (void);                                /* Part 63 (part 13's till 2026-10-28) */
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
unsigned char gear_words (void);                /* Part 62 */
void __fastcall__ kit_lock (unsigned b, unsigned k);   /* Part 61 */
void __fastcall__ mix_need (unsigned i);        /* Part 64: a pile of coins's mints (2026-10-28) */
void __fastcall__ mix_join (unsigned i, unsigned n);
void __fastcall__ mix_words (unsigned i);
unsigned char __fastcall__ mix_worth (unsigned i);
void __fastcall__ listed (unsigned n);          /* Part 11 */
#define HW_CARRIED      1                       /* (pick's how: carried; seen, lying about; either, as far as it sees,
                                                **   or to hand, within reach (afar else); in win) */
#define HW_SEEN         2
#define HW_ANY          3
#define HW_HAND         4                       /* (HW_HAND and after: remembered in the room too, out of the light) */
#define HW_KNOWN        6                       /* (Carried, or seen or remembered) */
#define HW_GONE         7                       /* (Lying about, seen or remembered) */
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

/* A kind's record, copied out of the kinds' bank (into one of four, in turn: an expression may want two at once):
** the kinds' bank left in the window */
static const struct kind* __fastcall__ kind_of (unsigned char k)
{
    static struct kind kb[4];
    static struct kind* r = kb;
    static unsigned o;

    r = r == kb + 3 ? kb : r + 1;
    o = k < NK ? k : 0;
    o = (o << 5) + (o << 2) + o;                /* (Its place, 37 bytes a kind: no multiplying, an inventory's
                                                **   every thing's, 2026-10-27) */
    hub_map (KIND_BANK);
    memcpy (r, (const char*) kinds + o, sizeof *r);
    return r;
}
typedef char kind_fits[sizeof (struct kind) == 37 ? 1 : -1];

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

/* The hands a thing wants: both for a two-handed one, or a big one (a macro: its parts' own, 48 and 63, the main
** RAM's room) */
#define TWO(k)          ((k)->flags & K_TWO || (k)->bulk >= 20 ? 2 : 1)

struct ost ost;                                 /* (The outfits' draw's state: parts 57 and 59's, outfit.h's) */
unsigned char held_k[2], held_h[2];             /* (What the character holds: part 48's held_kinds) */

/* The catalogue's bank in the window (bank.s's m0 ... put it back with it) */
void cat_map (void)
{
    hub_map (CAT_BANK);
}

/* A kind's flags (the catalogue's code's: the kinds in a bank of their own) */
static unsigned char __fastcall__ kind_flags (unsigned char k)
{
    hub_map (KIND_BANK);
    return CATALOGUE[k < NK ? k : 0].flags;
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

/* ---- The hub's text (things_text, the UI bank's): the words put after what's there (tlist.s's put, in assembly: an
** inventory's every word, 2026-10-30), the text begun again */
void __fastcall__ put (const char* s);
extern char* tp;                                /* (tlist.s's: the text's end so far, and how far it may go, its bank) */
extern char* tpe;
extern unsigned char put_bank;
extern char* tq;                                /* (Or into the main RAM, on to tqe: things' words compared, alike:
                                                **   part 11's) */
extern char* tqe;

static void text_new (void)
{
    put_bank = UI_BANK;
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
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
unsigned chart_w[5];                            /* (The map making's things, part 26's, handy, by their keys) */
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
    return j == rs_pack || (item_at (j)->where == IW_ACTOR && !item_at (j)->x);     /* (A pack off for a handling, at
                                                **   its feet, its as ever: part 84's) */
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
    unsigned char v;

    if (k->craft == AP_WEAPON && (v = item_at (i)->size) != 0 && v <= WV_HEAVY) {
        w += (v == WV_HEAVY ? w : -w) >> 2;     /* (A weapon's version: a quarter heavier, or lighter, D13's) */
    }

    if (k->form & KF_TENS) {
        w = LOT_WT (k->form, w);                /* (A lot by the ten: the missiles', a tenth of a pound each; coins
                                                **   by the dram) */
    }

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

    if (k->form & KF_TENS) {
        b = (b * item_at (i)->qty + 9) / 10;    /* (Its bulk by the ten too) */
    }
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
    if (!pack) {
        pack = rs_pack;                         /* (Off for this handling, part 84's: its pack still) */
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
** way about): 1 in the pack, 2 in a hand, 0 neither (full; or a step of the reach begun first, rs_st) */
unsigned char __fastcall__ hand_free (unsigned n);
unsigned char __fastcall__ s_carry (unsigned n, unsigned char hand_first)
{
    unsigned char h;

    s_about ();
    h = hand_free (n);
    if ((!hand_first || !h) && pack && !fits (n, pack)) {
        if (reach_step (n, pack)) {
            return 0;                           /* (A step of the reach first: part 84's) */
        }
        place (n, IW_ITEM, pack, 0);
        reach_to (pack);                        /* (Put away: its reach, HOUSE.md's: part 56's) */
        return 1;
    }
    if (h) {
        if (reach_step (n, 0)) {
            return 0;
        }
        place (n, IW_ACTOR, 0, I_HELD);
        item_at (n)->sn = h;
        return 2;
    }
    return 0;
}

/* ---- The reach (HOUSE.md's: the user's, 2026-10-23, "Handy would be readily available, so in a backpack that is being
** worn is not as readily available as one in a belt pouch"): how readily a thing comes to hand, by where it is, for
** every taking out, putting away, look inside, search and tool's use, every actor's (part 56's, its own: part 10's
** room, an inventory's swap the smaller).  In hand, at once; on the body
** (worn, hung from a belt, in what's open to the hand: level_of's) or lying within reach, a quick draw, no time more
** than the handling's own; in a container (held, set down, in another), R_IN a level; in a pack worn on the back,
** taken off first and put on again after, R_OFFON and its level, once a handling however often it's reached into.
** Engaged, a round a step (the book's grope, a turn: the readiest a turn, each level deeper a turn more, a pack's
** taking off and its putting on again a turn each).  What's owed (rtime, rsteps, rbody) is added to the character's
** next spell of time as the handling's done (reach_done: dq0w.c's busy, its pextra) */
#define R_IN            20                      /* (Tenths: a container's level, reached into) */
#define R_OFFON         100                     /* (A pack on the back taken off, and put on again) */
unsigned char __fastcall__ zone_step (unsigned ox, unsigned oy, unsigned x, unsigned y);  /* (dq0w.c's part 24) */

#pragma static-locals (push, on)
#pragma code-name (push, "SW56CODE")
#pragma rodata-name (push, "SW56RODATA")

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
        } else if (rs_ok == 1 || rs_ok == 2) {
            return;                             /* (Its levels the steps', part 84's) */
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
    t = rtime;
    if (!(said & RD_LONG)) {
        pextra += t;                            /* (A long task's its own length's instead: the caller's, RD_LONG) */
    }
    if (roff && (said & RD_SAID)) {
        put (" You take off your ");
        put (kind_at (roff)->name);
        put (" for it, and put it on again.");
    }
    rclear ();
    return t;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Parts 84, 85, 86 (2026-11-02, the user's: "when you have to take off your backpack to get something from it, it
** should read "Taking off backpack", pause, "retrieving <item>", pause, then "putting your backpack back on".  Taking
** just as long as it might take when really doing it"; HOUSE.md's reach): a reach as a sequence of steps, each told as
** it begins, each its own time, each a grope engaged (a round; failed, the round wasted, the sequence over, the world
** as it stands), the world changed at each's end; stopped part way (hard, soft or the player's), the world stays as it
** got, the pack off told.  The handlers call reach_step just before they change anything, past their refusals: it
** begins the next step owed, and they answer nothing done (IR_NO, no words): the reply's IO_STEP, main.c's things' the
** UI's first words and wait (a long task of the step's kind, the world's lwork LW_REACH), the command kept and asked
** again as it's up (reach 5, 6); with none owed the handling goes on, its levels paid, as it was.  After the handling
** the pack goes back on as the character's free (rs_after_on, lwork's: the world's own step).  An intent's
** tool the same by the world alone (reach_do with its keys: lw_m asked again).  The requests that don't ask for the
** steps (IO_STEPS: a blow's weapon, a denizen's) keep the lump, reach_done's */
#pragma code-name (push, "SW84CODE")
#pragma rodata-name (push, "SW84RODATA")
#pragma register-vars (push, on)

unsigned rs_pack;                               /* (The pack off for this handling) */
#define rs_t            far_y                   /* (A step's time, tenths; its kind: scratch bytes, not in use from the */
#define rs_k            nin                     /*   step's setting to the reply) */
#define rs_c            dropped                 /* (The thing the step's about, the pack on the back the reach comes to) */
#define rs_hit          lx                      /* (The level wanted, and from the innermost, how far: level_of's bytes, */
#define rs_want         lw                      /*   not in use across a call) */
unsigned char rs_ok;                            /* (This request's: 1 by the steps, the UI's; 2 the world's own; | 4, past
                                                **   a count's first thing: the rest by the lump) */
unsigned char rs_st;                            /* (A step begun: 1; 2 a grope failed; the request's reply) */
unsigned char rs_what;                          /* (The step's thing's kind) */
unsigned char rs_paid;                          /* (The levels reached into so far) */
unsigned char rs_after_on;                            /* (The pack to go back on as it's free: 1; 2 past a timed work's end) */
unsigned char rs_live;                          /* (A step done, its handling asked again next) */
unsigned char rs_using;                          /* (A fire-maker's: 1 out; 2 caught) */
static unsigned char rs_do;                     /* (What the step's end does, RS_*) */
#define rs_hand         last_sa                 /* (The pack's hand, 0 its feet: a handling's own byte, none as it's waited on) */
#define rs_keep         lw_m.who                /* (The intent to ask again: its who, 0 as it came, the UI's) */
enum { RS_OFF = 1, RS_IN, RS_ON, RS_DROP, RS_TRY, RS_OUT, RS_LIGHT, RS_STOW, RS_UP };
#define rs_old          lw_x                    /* (A redip's: the torch, to put away) */
#define rs_down         lw_y                    /* (Things it set down: taken up again after) */
#define rs_down2        far_x
unsigned char rs_redo;                          /* (A redip's, RR_*: its end's steps) */
enum { RR_ON = 1, RR_STOW = 2, RR_RELIGHT = 4, RR_D1 = 8, RR_D2 = 16 };   /* (RR_D*: rs_down, rs_down2 in use) */

/* The containers a reach comes to, out from c (level_of's rule, its own copy: open to the hand, none; a pack on the back,
** rs_c; any other, or closed, a level): how many, and rs_hit the one rs_want from the innermost */
static unsigned char __fastcall__ rs_walk (unsigned c)
{
    register const struct kind* k;
    unsigned char m = 0;

    while (c) {
        fetch (&rec, c);
        k = kind_of (rec.kind);
        if (!(rec.shut & IS_CLOSED) && (k->flags & K_CONTAINER) && !(k->flags & K_HANG) && (rec.flags & I_WORN) &&
            (k->form & KF_BACK)) {
            rs_c = c;
        } else if ((rec.shut & IS_CLOSED) || ((k->flags & K_CONTAINER) && !(k->flags & K_HANG) && !(rec.flags & I_WORN))) {
            if (m++ == rs_want) {
                rs_hit = c;
            }
        }
        c = rec.where == IW_ITEM ? rec.x : 0;
    }
    return m;
}

/* The step begun (rs_do, rs_k, rs_what, rs_t): engaged a round, and its grope (not the first's, of a thing's handling:
** the request's own test made already, serve's, get's): failed, the round wasted, the sequence over.  1 */
static unsigned char rs_begin (void)
{
    unsigned char s = rs_live;

    if (rs_do == RS_DROP) {
        goto go;                                /* (Let fall, set down: no grope, no round, the book's) */
    }
    rs_live = 0;
    if (rs_do != RS_TRY && (s || (rs_ok & 3) == 2 || !(iq.kind == IO_GET || (iq.kind >= IO_HOLD && iq.kind <= IO_TAKE))) &&
        fumbled (rs_what)) {
        rs_st = 2;
        reach_do (RO_FUMBLED);
        return 1;
    }
    rs_live = s;
    hub_map (0);
    if (zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y)) {
        hub_map (0);
        rs_t = HUB->round;
    }
go:
    pc_cut = 0;
    lwork = LW_REACH;
    if ((rs_ok & 3) == 2) {
        reach_do (RO_GO);
    } else {
        rs_st = 1;
    }
    return 1;
}

unsigned char __fastcall__ s_reach_step (unsigned n, unsigned c)
{
    unsigned char nout, nd, d = 0;
    unsigned up = 0;

    if (rs_ok != 1 && rs_ok != 2) {
        return 0;
    }
    if (c >= RS_GO) {
        d = c;                                  /* (RS_BACK: the pack back on; RS_TOOL: n a tool; RS_FIRE: n a fire-maker;
                                                **   RS_GO: the step the caller's set) */
        c = 0;
        if (d == (unsigned char) RS_GO) {
            return rs_begin ();
        }
    }
    rs_c = 0;
    rs_want = 0xFF;
    if (n && item_at (n)->where == IW_ITEM) {
        up = item_at (n)->x;
    }
    nout = rs_walk (up);
    nd = rs_walk (c);
    rs_t = 50;
    if (d == (unsigned char) RS_BACK) {
        if (rs_using) {                          /* (The fire-maker put away first) */
            rs_using = 0;
            rs_after_on = 1;
            rs_do = RS_IN;
            rs_k = LT_PUTAWAY;
            rs_what = item_kind (rs_tool);
            rs_t = 20;
            return rs_begin ();
        }
        rs_do = RS_ON;
        rs_k = LT_PACKON;
        rs_what = item_kind (rs_pack);
    } else if (rs_c) {                          /* (A pack on the back: off first) */
        rs_hand = d ? 0 : rs_for (up ? n : 0);
        rs_do = RS_OFF;
        rs_k = rs_hand ? LT_PACKOFF : LT_PACKDOWN;
        rs_what = item_kind (rs_c);
    } else if (rs_paid < nout + nd) {           /* (Each level its own) */
        rs_want = rs_paid < nout ? nout - 1 - rs_paid : nout + nd - 1 - rs_paid;
        rs_walk (rs_paid < nout ? up : c);
        rs_do = RS_IN;
        rs_k = d ? LT_REACHIN : rs_paid + 1 == nout ? LT_GETOUT : rs_paid + 1 == nout + nd ? LT_OPENUP :
            LT_REACHIN;
        rs_what = item_kind (rs_k == LT_GETOUT ? n : rs_hit);
        rs_t = 20;
    } else {
        if (!d) {
            return 0;
        }
        rs_tool = n;                            /* (A tool: got out, its tries if it's a fire-maker: part 86's) */
        if ((d = rs_for (d == (unsigned char) RS_FIRE ? RF_FIRE : RF_DRAW)) != 1) {
            return d;
        }
    }
    return rs_begin ();
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW86CODE")
#pragma rodata-name (push, "SW86RODATA")
#pragma static-locals (push, off)

/* A flame to hand (IO_LIGHT's, the catalogue's bank's light): a thing lit, carried or within reach, 1; or a fire-maker
** carried (the catalogue's tool key, "tinder"), rs_tool, one not known empty first: 3, its steps the lighting's (part
** 82's); 4, it's known empty; 0 none */
unsigned rs_tool;
unsigned char s_flame (void)
{
    unsigned i = 0;

    rs_tool = 0;
    while ((i = items (i, IN_SQUARE | IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {
        if (ip->flags & I_LIT) {
            if (on_pc (i) || in_reach (i)) {
                return 1;
            }
        } else if ((!rs_tool || (item_at (rs_tool)->shut & IS_SPENT)) && !strcmp (kind_at (i)->tool, "tinder") && on_pc (i)) {
            rs_tool = i;
        }
    }
    return !rs_tool ? 0 : item_at (rs_tool)->shut & IS_SPENT ? 4 : 3;
}

/* A word to the UI (E_TASK's w: TE_*, | its kind times 256; b rs_what) */
static void __fastcall__ rs_tell (unsigned w)
{
    hub_map (0);
    wk.m.kind = E_TASK;
    wk.m.who = 0;
    wk.m.b = rs_what;
    wk.m.w1 = w;
    wk.m.t = HUB->now;
    hub_event (&wk.m);
}

/* The sequence over, part way: the pack left off told */
static void rs_cut (void)
{
    if (rs_pack) {
        rs_what = item_kind (rs_pack) | (item_at (rs_pack)->where == IW_SQUARE ? 0x80 : 0);
        rs_tell (TE_PACKED);
    }
    rs_pack = 0;
    rs_after_on = rs_do = rs_paid = rs_live = rs_keep = rs_using = rs_redo = 0;
}

/* A tool's next step (rs_tool: the user's, 2026-11-02, "grope (take the tinder box), grope (to use the tinder box,
** penalized roll in the dark, continue until successful, reporting as it goes along)", and "Applies to any tool use";
** HOUSE.md's): got out, a second (in hand already, none), then its use (0).  A fire-maker: empty, told as it's reached (2),
** and known from then on (IS_SPENT); a try, 5 seconds, its roll at its end whether engaged or not; caught, on with the
** lighting (0) */
static unsigned char __fastcall__ rs_draw (unsigned char fire)
{
    fetch (&rec, rs_tool);
    if (!rs_using) {
        rs_using = 1;
        if (!(rec.flags & I_HELD)) {
            rs_do = RS_IN;
            rs_k = LT_GETOUT;
            rs_what = rec.kind;
            rs_t = 10;
            return 1;
        }
    }
    if (!fire || rs_using == 2) {
        return 0;
    }
    make_own (rs_tool);
    fetch (&rec, rs_tool);
    if (!rec.fuel) {
        item_at (rs_tool)->shut |= IS_SPENT;
        return 2;
    }
    rs_do = RS_TRY;
    rs_k = LT_KINDLE;
    rs_what = rec.kind;
    rs_t = 50;
    return 1;
}

/* The referee's grope (Dexterity's; every ruleset's), 20 less in the dark: whether it went, rs_what */
static void rs_roll (void)
{
    hub_map (SIGHT_BANK);
    rs_what = SIGHT->dark;
    hub_map (0);
    HUB->slot[SLOT_WORLD].kind = RS_TEST;
    HUB->slot[SLOT_WORLD].actor = 0;
    HUB->slot[SLOT_WORLD].a = P_GROPE;
    HUB->slot[SLOT_WORLD].b = 0;
    HUB->slot[SLOT_WORLD].steps = rs_what ? -4 : 0;
    hub_ask (SLOT_WORLD);
    rs_what = G_OK (HUB->slot[SLOT_WORLD].grade);
    HUB->slot[SLOT_WORLD].state = REQ_FREE;
}

/* A try's end: caught, one lighting off it, told */
static void rs_tried (void)
{
    rs_roll ();
    if (rs_what) {
        rs_using = 2;
        --item_at (rs_tool)->fuel;
    }
    rs_tell (TE_KINDLED);
}

/* The hand the pack goes in as it's taken off: a thing to get out too (n), two free, the other than it'll go in; else
** any hand free, the off hand first; 0 none: at its feet */
unsigned char __fastcall__ s_rs_for (unsigned n)
{
    unsigned char m, h, f;

    if (n >= RF_TELL) {
        if (n <= RF_FIRE && n >= RF_DRAW) {
            return rs_draw (n == RF_FIRE);
        }
        if (n >= RF_PICKS && n <= RF_DIGGERS) {  /* (An intent's tool got out first, the world's own steps, lw_m the intent
                                                **   asked again as they're done: 1 if a step's begun; 0 out, on with it,
                                                **   put away and the pack back on after it.  Its keys in the main RAM:
                                                **   strings here gone as tool_reach's part is copied in) */
            if (!rs_live) {
                rs_paid = rs_using = 0;
                rs_pack = 0;
            }
            memcpy (yn, n == RF_PICKS ? "picks\0" : "pick\0shovel\0", 13);
            rs_ok = 2;
            rs_keep = 1;
            f = tool_reach (yn);
            rs_ok = 0;
            if (!f) {
                rs_live = rs_keep = 0;
                if (rs_using && (item_at (rs_tool)->flags & I_HELD)) {
                    rs_using = 0;
                }
                if (rs_pack || rs_using) {
                    rs_after_on = 1;
                    pc_cut = 0;
                }
            }
            return f;
        }
        if (n == RF_ROLL) {
            rs_roll ();
            return rs_what;
        }
        if (n == RF_TRIED) {
            rs_tried ();
        } else if (n == RF_CUT) {
            rs_cut ();
        } else {
            rs_tell (n & ~RF_TELL);
        }
        return 0;
    }

    held_kinds ();
    h = HUB->pcsum.limbs;
    m = h & LB_LEFT ? IH_LEFT : IH_RIGHT;
    f = IH_BOTH & ~((h & LB_ARM_L ? IH_LEFT : 0) | (h & LB_ARM_R ? IH_RIGHT : 0) | held_h[0] | held_h[1]);
    if (!n) {
        return f == IH_BOTH ? m ^ IH_BOTH : f;
    }
    if (f != IH_BOTH) {
        return 0;
    }
    h = iq.kind == IO_HOLD ? iq.to : 0;
    h = !h ? hand_free (n) : h == IH_OTHER ? m ^ IH_BOTH : h;
    return !h || h == IH_BOTH ? 0 : h ^ IH_BOTH;
}

#pragma static-locals (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)

static void __fastcall__ name_of (unsigned n, unsigned char first);     /* (The catalogue's bank's: its words, its checks) */
static unsigned char __fastcall__ no (unsigned char n);
static unsigned char __fastcall__ yes (unsigned char n, unsigned char t);
static unsigned char __fastcall__ hold (unsigned n);
#pragma code-name (push, "SW85CODE")
#pragma rodata-name (push, "SW85RODATA")

/* ---- A torch dipped (HOUSE.md's a torch redipped: the user's, 2026-11-02, "Fumbling about in the dark should be
** difficult ..."; part 27's fill, past its refusals, each ask; RO_REDIP, handled the torch): the next step of what comes
** before the dipping, begun: IR_NO; or IR_OK, on with it (its fuel got out, the timed work).  The torch never dipped
** burning: another light by it, put out and dipped by that; none, a spare lit from it, then put out; no spare, put out,
** asked first, dipped in the dark and lit again after.  Dipping asks both hands, the torch and the flask: what's in the
** other set down at its feet.  Part 85's: its own words the catalogue's bank's no, name_of */
#pragma static-locals (push, off)
static unsigned char __fastcall__ rs_no (unsigned n, unsigned char sa)
{
    cat_map ();
    name_of (n, 1);
    return no (sa);
}

static unsigned char rs_go (void)
{
    return reach_step (0, RS_GO) ? IR_NO : IR_OK;
}

static unsigned char __fastcall__ rs_set_down (unsigned i)
{
    rs_c = i;
    rs_do = RS_DROP;
    rs_hand = 0;
    rs_k = LT_SETDOWN;
    rs_what = item_kind (i);
    rs_t = 1;
    if (rs_redo & RR_D1) {
        rs_down2 = i;
        rs_redo |= RR_ON | RR_D2;
    } else {
        rs_down = i;
        rs_redo |= RR_ON | RR_D1;
    }
    return rs_go ();
}

/* The other thing in a hand, not n: 0 none */
static unsigned __fastcall__ rs_held (unsigned n)
{
    unsigned i = 0;

    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if ((ip->flags & I_HELD) && i != n) {
            return i;
        }
    }
    return 0;
}

static unsigned char __fastcall__ rs_redip (unsigned n)
{
    unsigned i = 0, sp = 0, ot = 0;
    unsigned char s, kn;

    hub_map (SIGHT_BANK);
    s = SIGHT->lit;
    hub_map (0);
    if (HUB->pcsum.limbs & (LB_ARM_L | LB_ARM_R)) {
        return rs_no (n, SA_TWO_HANDS);
    }
    fetch (&rec, n);
    kn = rec.kind;
    if (rec.flags & I_LIT) {
        while ((i = items (i, IN_SQUARE | IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {   /* (Another light; a spare, not
                                                **   spent, of its kind) */
            if (i == n) {
                continue;
            }
            if (ip->flags & I_LIT) {
                if (kind_flags (ip->kind) & K_LIGHT && (on_pc (i) || in_reach (i))) {
                    ot = i;
                }
            } else if (ip->kind == kn && !sp && (!ip->id || ip->fuel) && on_pc (i)) {
                sp = i;
            }
        }
        if (!ot && !s && sp) {                  /* (The preferred order: a spare lit from it first) */
            if (!hand_free (sp) && (i = rs_held (n)) != 0) {
                return rs_set_down (i);
            }
            if (reach_step (sp, 0)) {
                return IR_NO;
            }
            rs_hand = hand_free (sp);
            rs_c = sp;
            rs_old = n;
            rs_do = RS_LIGHT;
            rs_k = LT_LIGHTFROM;
            rs_what = kn;
            rs_t = 30;
            rs_redo |= RR_ON | RR_STOW;
            return rs_go ();
        }
        rs_c = n;                               /* (Put out: by another light, or to the dark, asked first) */
        rs_do = RS_OUT;
        rs_what = kn;
        rs_t = 10;
        rs_k = LT_BLOWOUT;
        rs_old = n;
        rs_redo |= RR_ON;
        if (ot) {
            rs_redo |= RR_STOW;
        } else if (!s) {
            rs_k = LT_BLOWDARK;
            rs_redo |= RR_RELIGHT;
        }
        return rs_go ();
    }
    if (!(rec.flags & I_HELD)) {                /* (In a hand, to dip it) */
        if (!hand_free (n) && (i = rs_held (n)) != 0) {
            return rs_set_down (i);
        }
        if (reach_step (n, 0)) {
            return IR_NO;
        }
        cat_map ();
        name_of (n, 1);
        if (hold (n) != IR_OK) {
            return IR_NO;
        }
    }
    if ((i = rs_held (n)) != 0) {               /* (Both hands to it: the other's set down) */
        return rs_set_down (i);
    }
    return IR_OK;
}

/* A step's end (rs_do, rs_c): what it changes changed: a pack, a thing let fall; a redip's: put out, lit from the old,
** put away, taken up again */
static void rs_did (void)
{
    unsigned char h;
    unsigned n = rs_do == RS_ON ? rs_pack : rs_c;

    if (rs_do == RS_OFF) {
        rs_pack = rs_c;
    }
    fetch (&rec, n);
    if (rs_do == RS_OUT) {
        rec.flags &= ~I_LIT;
        den_saw = 1;
    } else if (rs_do == RS_LIGHT) {
        make_own (n);
        fetch (&rec, n);
        rec.where = IW_ACTOR;
        rec.x = 0;
        rec.flags = (rec.flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD | I_LIT;
        rec.sn = rs_hand;
    } else if (rs_do == RS_STOW) {
        about ();
        fetch (&rec, n);                        /* (Back in the pack; no room, or none, it stays in its hand) */
        cat_map ();
        if (pack && !o_fits (n, pack)) {
            fetch (&rec, n);
            rec.where = IW_ITEM;
            rec.x = pack;
            rec.flags &= ~(I_WORN | I_HELD);
        } else {
            fetch (&rec, n);
        }
    } else if (rs_do == RS_UP) {
        if (!rec.kind || rec.where != IW_SQUARE || !(h = hand_free (n))) {
            return;
        }
        fetch (&rec, n);
        rec.where = IW_ACTOR;
        rec.x = 0;
        rec.sn = h;
        rec.flags = (rec.flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD;
    } else {                                    /* (A pack taken off, put on; a thing let fall, set down) */
        rec.flags &= ~(I_WORN | I_HELD);
        if (rs_do == RS_ON) {
            rec.where = IW_ACTOR;
            rec.x = 0;
            rec.sn = 0;
            rec.flags |= I_WORN;
            rs_pack = 0;
        } else if (rs_hand) {
            rec.flags |= I_HELD;
            rec.sn = rs_hand;
        } else {
            hub_map (0);
            rec.where = IW_SQUARE;
            rec.sn = HUB->pc_stratum;
            rec.x = HUB->pc_x;
            rec.y = HUB->pc_y;
            den_saw = 1;                        /* (The world to look about again: it seen lying there, its light) */
        }
    }
    store (&rec, n);
}

/* What's next as a dipping's over, the fuel put away: the torch to light again (2: rs_after_on 2, the UI's IO_LIGHT, then here
** again); the old one put away; what was set down taken up (1: the step set, RS_GO's); 0 no more */
static unsigned char rs_after (void)
{
    unsigned i;

    if (rs_redo & RR_RELIGHT) {
        rs_redo &= ~RR_RELIGHT;
        rs_after_on = 2;
        return 2;
    }
    if (rs_redo & RR_STOW) {
        rs_redo &= ~RR_STOW;
        fetch (&rec, rs_old);
        if (rec.kind && (rec.flags & I_HELD)) {
            rs_c = rs_old;
            rs_do = RS_STOW;
            rs_k = LT_PUTAWAY;
            rs_what = rec.kind;
            rs_t = 20;
            return 1;
        }
    }
    if (rs_redo & (RR_D1 | RR_D2)) {
        i = rs_redo & RR_D1 ? rs_down : rs_down2;
        rs_redo &= rs_redo & RR_D1 ? ~RR_D1 : ~RR_D2;
        rs_c = i;
        rs_do = RS_UP;
        rs_k = LT_TAKEUP;
        rs_what = item_kind (i);
        rs_t = 10;
        return 1;
    }
    rs_redo = 0;
    return 0;
}

/* A filling's or a dipping's end in the dark, by touch (the dark's test: "a fumble can spill oil"; handled the fuel):
** spilt, half the flask's less, told: 1 */
static unsigned char rs_spill (void)
{
    if (rs_for (RF_ROLL)) {
        return 0;
    }
    fetch (&rec, handled);
    rec.fuel >>= 1;
    store (&rec, handled);
    strcpy (xn, iq.name);
    cat_map ();
    no (SA_SPILT);
    return 1;
}

/* A thing sharpened with a whetstone (the user's, 2026-11-02, "like sharpening your sword (if you have a whetstone)"; the
** book has none: the house's; a tool's use, part 84's; handled the thing): a metal weapon, the whetstone carried, wear on
** it: its steps (IR_NO); begun, 0xFE (a minute's timed work); done (IO_DONE), half its wear gone, told */
static unsigned char rs_hone (void)
{
    unsigned n = handled, i = 0;

    fetch (&rec, n);
    if (kind_at (n)->craft != AP_WEAPON || !(rec.mat == MT_IRON || rec.mat == MT_STEEL || rec.mat == MT_BRONZE)) {
        return rs_no (n, SA_NO_FILL);
    }
    if (iq.to & IO_DONE) {
        rec.wear >>= 1;
        store (&rec, n);
        cat_map ();
        name_of (n, 1);
        return yes (SA_HONED, 0);
    }
    if (!rec.wear) {
        return rs_no (n, SA_SHARP);
    }
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (!strcmp (kind_at (i)->tool, "whet") && on_pc (i)) {
            return reach_step (i, RS_TOOL) ? IR_NO : 0xFE;
        }
    }
    return rs_no (n, SA_NO_STONE);
}
#pragma static-locals (pop)

/* What the reach's steps want of this part, by w: RO_*, or an intent's tool's keys.  RO_ASK a request by the steps taken
** up; RO_REPLY its reply made (1: a step's, no more); RO_END a step's time up, or the character free after its handling
** (lwork's): stopped first, the sequence over, told; a step's change made, 0 (the UI's to ask again), or 2 (an intent
** asked again); its handling done, the pack back on, the world's own step, 2; a timed work's end still to come, or the
** pack not where it was (a denizen's taken it), 0.  RO_GO the world's own step begun (its intent kept: lw_m); RO_FUMBLED a
** step's grope failed.  Keys (an intent's tool got out first, a dig's, a door's lock's): 1 if a step's begun; 0 got out,
** the pack back on, on with it */
unsigned char __fastcall__ s_reach_do (unsigned w)
{
    hub_map (0);
    switch ((unsigned char) w) {
    case RO_ASK:
        rs_ok = 1;
        HUB->ireq.kind &= ~IO_STEPS;
        if (rs_after_on == 2) {
            rs_after_on = 0;                          /* (A timed work's end, the torch lit again: its sequence going on) */
        } else if (!rs_live) {
            rs_paid = rs_using = rs_redo = 0;
            rs_pack = 0;
        }
        return 0;
    case RO_REDIP:
        return rs_redip (handled);
    case RO_SPILL:
        return rs_spill ();
    case RO_HONE:
        w = rs_hone ();
        hub_map (0);
        HUB->ireq.x = HUB->ireq.qty = 0;
        if ((unsigned char) w != 0xFE) {
            return w;
        }
        HUB->ireq.x = 600;
        HUB->ireq.qty = LT_HONE;
        HUB->ireq.item = item_kind (handled);
        return IR_OK;
    case RO_REPLY:
        if (rs_using && (item_at (rs_tool)->flags & I_HELD)) {
            rs_using = 0;                        /* (In hand: none to put away) */
        }
        hub_map (0);
        if (!rs_st && (rs_pack || rs_using) && iq.result != IR_OK) {
            reach_step (0, RS_BACK);            /* (Refused with the pack off, its way changed since: its words first, the
                                                **   pack back on, the UI's the same step: part 18's) */
            hub_map (0);
            rs_ok = HUB->ireq.kind = IO_STEP;
        } else if (rs_st) {
            if (rs_st == 1) {
                hub_map (UI_BANK);
                UIREC->things_text[0] = 0;      /* (No words of the handling's: the step's its own) */
                hub_map (0);
            }
            HUB->ireq.result = iq.kind == IO_THROW ? IR_FIXED : IR_NO;  /* (Nothing done, nothing to say) */
            rs_ok = HUB->ireq.kind = IO_STEP;
        }
        if (rs_ok == IO_STEP) {
            HUB->ireq.y = (rs_k == LT_BLOWDARK) | (rs_t == HUB->round ? 2 : 0);    /* (Its question put, the UI's; 2, engaged:
                                                **   the round's action, no long task for a blow to stop) */
            HUB->ireq.x = rs_t;
            HUB->ireq.qty = rs_k;
            HUB->ireq.item = rs_what;
            rs_st = rs_ok = 0;
            return 1;
        }
        if (rs_pack || rs_using) {               /* (The handling done: the pack back on as it's free, past a timed
                                                **   work's end if that's what's begun) */
            rs_after_on = (iq.kind == IO_SHUT || iq.kind == IO_FILL || iq.kind == IO_HIDE) && iq.result == IR_OK && HUB->ireq.qty ?
                2 : 1;
            pc_cut = 0;
            lwork = LW_REACH;
            if (rs_after_on == 1 && (iq.kind == IO_SHUT || iq.kind == IO_FILL || iq.kind == IO_HIDE)) {
                HUB->ireq.y = rs_redo & RR_RELIGHT ? 0xFE : 0xFD;   /* (A timed work's end: more to come, the UI to wait
                                                **   for it; and the torch to light again, words.c's worked) */
            }
        }
        rs_ok = rs_live = 0;
        return 0;
    case RO_FUMBLED:
        rs_t = HUB->round;
        rs_k = 0;
        if ((rs_ok & 3) == 2) {
            rs_for (RF_TELL | TE_FUMBLE);
            hub_map (0);
            HUB->pc_ready = HUB->now + rs_t;
            rs_st = 0;
        }
        rs_for (RF_CUT);
        return 0;
    case RO_GO:
        rs_for (RF_TELL | TE_BEGUN | (unsigned) rs_k << 8);
        wk.m.kind = I_WAIT;
        wk.m.a = 0;
        wk.m.w1 = rs_t;
        wk.m.w2 = rs_t == HUB->round ? 1 : (unsigned) rs_k << 8 | rs_what << 1 | 1;  /* (Engaged, the round's action) */
        intent_few (&wk.m);
        return 0;
    }
    if (pc_cut) {                               /* (RO_END) */
        rs_for (RF_CUT);
        return 0;
    }
    w = 0;
    if (rs_do != RS_IN && rs_do != RS_TRY && rs_do) {   /* (What it changes changed) */
        w = rs_do == RS_ON;
        rs_did ();
        rs_do = 0;
        light_now ();                           /* (What's handy with it) */
        equip_now ();                           /* (The referee told what's in its hands) */
    }
    if (rs_after_on) {                                /* (The character free, its handling done) */
        if (rs_after_on == 2) {
            return 0;
        }
        if (rs_using) {
            goto back;                          /* (The tool put away first, the pack after it) */
        }
        if (rs_redo) {
            w = rs_after ();
            hub_map (0);
            if (w == 2) {
                return 0;                       /* (The torch to light again: the UI's, rs_after_on 2 meanwhile) */
            }
            if (w) {
                rs_ok = 2;
                reach_step (0, RS_GO);
                rs_ok = rs_keep = 0;
                return 2;
            }
        }
        rs_after_on = 0;
        if (!rs_pack) {
            return 0;
        }
        fetch (&rec, rs_pack);
        hub_map (0);
        if (rec.where == IW_SQUARE ? rec.x != HUB->pc_x || rec.y != HUB->pc_y : rec.where != IW_ACTOR || rec.x) {
            rs_pack = 0;
            return 0;
        }
back:
        rs_ok = 2;
        reach_step (0, RS_BACK);
        rs_ok = rs_keep = 0;
        return 2;
    }
    rs_live = !w;                               /* (The pack back on, after its handling: all done) */
    if (rs_do == RS_IN) {
        ++rs_paid;
    } else if (rs_do == RS_TRY) {
        rs_for (RF_TRIED);
    }
    rs_do = 0;
    hub_map (0);
    HUB->pc_task.kind = LT_NONE;
    if (rs_keep) {
        rs_keep = 0;
        return 3;                               /* (Its intent asked again: work_done's, the parts' stack shallower) */
    }
    return 0;
}

#pragma register-vars (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* (Part 10's own: none owed, serve's) */
static void rclear10 (void)
{
    rtime = roff = 0;
    rsteps = rbody = 0;
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
                if (k->craft == AP_WEAPON && rec.size && rec.size <= WV_HEAVY) {
                    m[0] = rec.size == WV_HEAVY ? 'H' : 'L';    /* (Its version: D13's, RS_EQUIP's) */
                    strcat (t, m);
                }
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
/* ---- The thing the UI names for an ally's errand, a gift (slice 4.8's, 2026-10-30; the hub's ireq: IO_FETCH, its name fn,
** the request's copy; to 0, one seen lying about, its item and where it lies back in item, x, y; to 1, one the character carries, IO_DESCRIBE's reply, its worth in value, its item in item):
** IR_OK, IR_NONE (none such), IR_NO (in something closed, carried, a denizen's) */
static unsigned char __fastcall__ fetch_find (const char* fn)
{
    struct item* p;
    unsigned n, c, x = 0, y = 0;
    unsigned char r = IR_NONE, g, sn;

    hub_map (0);
    g = HUB->ireq.to;
    sn = HUB->pc_stratum;
    if (g) {                                    /* (A gift: the character's, IO_DESCRIBE's reply) */
        if ((n = pick (fn, HW_CARRIED)) != 0 && describe (fn) == IR_OK) {
            r = IR_OK;
        }
    } else if ((n = pick (fn, HW_SEEN)) != 0) {
        for (c = n; (p = item_at (c))->where == IW_ITEM && (c == n || !(p->shut & IS_CLOSED)); c = p->x) {
        }
        r = IR_NO;                              /* (In a thing closed, carried, on another stratum) */
        if (p->where == IW_SQUARE && p->sn == sn) {
            x = p->x;
            y = p->y;
            r = IR_OK;
        }
    }
    hub_map (0);
    if (r == IR_OK) {
        HUB->ireq.item = n;
        HUB->ireq.x = x;
        HUB->ireq.y = y;
    }
    return r;
}

static unsigned char o_handle (void);
unsigned char s_serve (void)
{
    unsigned char changed = 0, k;
    unsigned n;

    rclear10 ();                                /* (No reach owed yet) */
    hub_map (0);
    if (HUB->ireq.kind & IO_STEPS) {
        reach_do (RO_ASK);                      /* (By the reach's steps, the UI's: part 85's) */
    }
    memcpy (&iq, &HUB->ireq, sizeof iq);
    iq.result = IR_OK;
    wsel = iq.sel;                              /* (Which of several: the chooser's, part 4's pick) */
    wnot = win = wforce = 0;
    if (iq.kind == IO_GET && wsel == SL_NEW) {
        iq.kind = IO_HOLD;                      /* ("Take a new torch": in hand, from wherever it is) */
    }
    switch (iq.kind) {
    case IO_KIT:
        kit ();
        break;
    case IO_GET:                                /* (Taken up, put down: one, or as many as it says, part 4's) */
    case IO_DROP:
        if ((iq.result = moved (iq.kind)) == IR_NO && iq.kind == IO_GET && !rs_st) {
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
    case IO_GEAR:                               /* (A denizen's things made out: part 61's) */
        iq.result = gear_words ();
        break;
    case IO_SWAP:                               /* (What's in its hands changed over: part 48's) */
        iq.result = swap ();
        break;
    case IO_FETCH:                              /* (A thing for an ally to fetch, or a gift: slice 4.8's) */
        iq.result = fetch_find (iq.name);
        break;
    case IO_USE:                                /* (A thing used up by its key: a bandage, first aid's, part 70's; by the
                                                **   steps, got out for its use, a tool's, part 84's: IR_NO a step first) */
        iq.result = rs_ok ? (tool_reach (iq.name) ? IR_NO : IR_OK) : use_key (iq.name);
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
    case IO_CUT:
        pc_cut = 1;
        rs_for (RF_CUT);
        break;
    default:                                    /* (A thing handled: IO_HOLD ...) */
        afar = 0;
        k = iq.kind;
        if (!noinit && k >= IO_HOLD && k <= IO_DOFF && (n = at_hand (iq.name)) != 0 &&
            (k <= IO_TAKE || (kind_at (n)->form & KF_BACK)) && fumbled (item_kind (n))) {
            hub_map (0);                        /* (Held, stowed, put or taken out engaged, a grope failed: the round
                                                **   wasted, nothing done, its words) */
            took = HUB->round;
        } else if (k >= IO_STOW && k <= IO_TAKE) {
            if ((iq.result = moved (k)) == IR_OK && k != IO_TAKE) {
                reach_to (cont);                /* (Stowed, put, taken from: one, or as many as it says; what it went
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
    k = iq.kind == IO_SHUT || iq.kind == IO_FILL;   /* (A timed work begun, or a lid's: its reach its time's) */
    if (iq.result == IR_OK && iq.kind != IO_KIT && iq.kind != IO_HIDE &&
        (n = reach_done ((iq.kind != IO_GET && iq.kind != IO_DROP ? RD_SAID : 0) | (k ? RD_LONG : 0))) != 0 &&
        iq.kind != IO_THROW) {
        hub_map (0);                            /* (Its reach owed, the character's next spell's, and in the reply's y
                                                **   (the tests'; a throw's where it lies); a pack taken off for it told,
                                                **   but a thing taken up's or put down's: the UI's words alone; a timed
                                                **   work's its length the more, its time in x, as its task's) */
        HUB->ireq.y = n;
        if (k) {
            HUB->ireq.x += n;
        }
    }
    rclear10 ();
    hub_map (0);
    HUB->ireq.result = iq.result;
    if (rs_ok && reach_do (RO_REPLY)) {         /* (The reach's steps: part 83's, a step begun the reply's) */
        return 0;
    }
    return changed && iq.kind != IO_KIT;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 13 (part 26's till 2026-10-28: overlay 9's room, the lights' burning's): the map making, as the light
** (slice 4.2's; the world's parts are 51 at most, swap.inc's table) */
#pragma code-name (push, "SW13CODE")
#pragma rodata-name (push, "SW13RODATA")

/* The character's map making (the hub's pc_chart and pc_map: DUNGEON.md, the map kept; HOUSE.md's), by what it
** carries: the things to draw a map with handy (in hand, or on the body, in nothing that isn't open to the hand:
** HOUSE.md's handy tools, the user's, 2026-10-23), the catalogue's by their keys (a sheet to draw on, "sheet", or its
** map, "map"; a point that draws, "draw", or a quill, "quill", and ink, "ink"), and a hand to draw with (free, or
** holding one of them; its arm not useless); and the first map it carries (anywhere on it: shown).  Mapping as it goes
** with them, and no map, its first sheet is made the map (the kind keyed "map").  Not handy, what they're in (its kind
** in pc_chart: CH_PACKED's, the UI's to tell).  Part 13's (its room), called as the light is (s_light_now's) */
void s_chart_now (void)
{
    static const char keys[5][6] = { "sheet", "map", "draw", "quill", "ink" };
    const struct kind* k;
    unsigned i = 0, j, sheet = 0, map = 0;
    unsigned char has = 0, busy, mh, h, f, pk = 0;

    hub_map (0);
    mh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    busy = (HUB->pcsum.limbs & LB_ARM_L ? IH_LEFT : 0) | (HUB->pcsum.limbs & LB_ARM_R ? IH_RIGHT : 0);
    memset (chart_w, 0, sizeof chart_w);
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
        if (f == 1 && !map) {
            map = i;
        }
        if ((j = handy (i)) != 0) {
            pk = item_kind (j);                 /* (Not handy: in a pack worn, a sack) */
            continue;
        }
        has |= 1 << f;
        chart_w[f] = i;                         /* (Each handy, by its key: what it draws with, used up as it draws,
                                                **   dq0w.c's drawn_down) */
        if (!f && !sheet) {
            sheet = i;
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
    HUB->pc_chart = f ? CH_DRAW : pk << 1;
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
    if (reach_step (n, 0)) {
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
    unsigned char n = strlen (w), m = strlen (k);

    return word_of4 (k, w, n) || (n > 2 && w[n - 1] == 's' && (word_of4 (k, w, n - 1) ||
        (w[n - 2] == 'e' && word_of4 (k, w, n - 2)))) || (n > 3 && m > n && !strcmp (k + m - n, w));    /* (Or the end
                                                **   of a name of one word and another: "pack" a backpack, "skin" a
                                                **   waterskin, 2026-10-26) */
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
        if (ctop == rs_pack) {
            pw = IW_ACTOR;                      /* (Off for a handling, at its feet: part 84's) */
            px = 0;
        }
        if (how == HW_IN) {
            if (rec.where != IW_ITEM || rec.x != win) {
                continue;
            }
            r = 0;
        } else if (rec.where == IW_ITEM && rec.x == win) {
            continue;                           /* (In the one a count's put in already) */
        } else if (pw == IW_ACTOR) {
            if (px || how == HW_SEEN || how == HW_GONE) {
                continue;                       /* (Another's: a denizen's) */
            }
            r = d ? 1 + (d > 6 ? 6 : d) : rec.flags & I_HELD ? 0 : 1;
        } else {
            if (how == HW_CARRIED || d > 1 || ps != sn || (dx = px - x + 20) > 40 || (dy = py - y + 20) > 40 ||
                (d && !(kind_of (pk)->keeps & KF_ON)) || !(t = at (sn, px, py)) || (!(*t & T_VIS) &&
                (how < HW_HAND || !(*t & T_SEEN) || !rec.id || room_of (sn, px, py) != room_of (sn, x, y)))) {
                continue;                       /* (Lying about, or on a surface there, in sight, or remembered there
                                                **   in the room it's in, as HW_HAND and those after it ask; not in a thing) */
            }
            dx = dx > 20 ? dx - 20 : 20 - dx;
            dy = dy > 20 ? dy - 20 : 20 - dy;
            r = 8 + (dx > dy ? dx : dy);
        }
        k = kind_of (rec.kind);
        if (s == SL_LIT ? !(rec.flags & I_LIT) : wsel == SL_NEW ? rec.flags & (I_HELD | I_LIT) : s == SL_UNLIT ? (rec.flags &
            I_LIT) || !(k->flags & K_LIGHT) :
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
#pragma code-name (push, "SW72CODE")            /* (Part 72's since 2026-11-02: part 4's room, overlay 2's) */
#pragma rodata-name (push, "SW72RODATA")
unsigned char __fastcall__ s_tool_reach (const char* keys)
{
    const char* k;
    unsigned i = 0;

    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        for (k = keys; *k; k += strlen (k) + 1) {
            if (!strcmp (kind_at (i)->tool, k) && on_pc (i)) {
                reach_from (i);
                return reach_step (i, RS_TOOL);
            }
        }
    }
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

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
        !noinit && fumbled (rec.kind) ? IR_FUMBLED : carry (n, 0) ? IR_OK : rs_st ? IR_NO : IR_FULL;  /* (A portal's part: lifted
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
        if (reach_step (n, 0)) {
            return IR_NO;
        }
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
            if (i >= ihigh) {
                ihigh = i + 1;
            }
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
            if (rec.fuel != q.fuel || rec.size != q.size || rec.thick != q.thick) {
                mix_join (i, n);                /* (Two piles of coins: their mints' groups merged, part 63's) */
            }
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
    mode = op == IO_GET ? HW_GONE : op == IO_DROP ? HW_CARRIED : op == IO_TAKE ? (win ? HW_IN : HW_HAND) : HW_HAND;
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
        rs_ok |= 4;                             /* (The rest by the lump, their levels paid: part 84's) */
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
    rs_ok &= 3;
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

/* (The list's last ", " made " and ": part 61's and_last, its own copy) */
static void __fastcall__ and_last36 (char* from)
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
    and_last36 (t0);
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

/* A key the character carries (in hand, on it, in what it carries) that fits the lock of thing number id (its number
** in the key's size and thick: HOUSE.md's keys that fit, 2026-10-27): 1; 2, keys but none that fits; 0, none */
static unsigned char __fastcall__ key_fits (unsigned id)
{
    unsigned i = 0, j;
    unsigned char r = 0;
    struct item* p;

    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (item_at (i)->kind != KI_KEY) {
            continue;
        }
        for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {
        }
        if (item_at (j)->where != IW_ACTOR || item_at (j)->x) {
            continue;
        }
        p = item_at (i);
        if ((p->size | p->thick << 8) == id) {
            rs_tool = i;
            return 1;
        }
        r = 2;
    }
    return r;
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
    unsigned st;

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
    st = rec.where == IW_ITEM ? rec.x : 0;      /* (What it's in: reached into first, past the refusals: part 84's) */
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
    if ((w == PA_LOCK || w == PA_UNLOCK) && (s & (IS_LOCK | IS_BURST | IS_CLOSED)) == (IS_LOCK | IS_CLOSED) &&
        ((s & IS_LOCKED) != 0) == (w == PA_UNLOCK) && (lt = key_fits (rec.id)) == 1) {
        if (reach_step (rs_tool, RS_TOOL)) {
            return IR_NO;                       /* (Its key got out: a tool's steps) */
        }
        s ^= IS_LOCKED;                         /* (With its own key: no test, a few seconds) */
        say3 (w == PA_LOCK ? "You lock the " : "You unlock the ", nm, " with its key.");
        t = 30;
        lt = 0;
        ok = IR_OK;
        goto done;
    }
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
            put (lt ? "Your key doesn't fit its lock." : "You have nothing to pick its lock with.");
            return IR_NO;
        }
        lt = 0;
        if (!d) {
            memcpy (yn, "picks\0", 7);          /* (Its keys in the main RAM: this part gone as tool_reach's copied in) */
            if (reach_step (0, st) || tool_reach (yn)) {
                return IR_NO;
            }
            t = 600;                            /* (The means to hand: a minute a try, as a door's, a long task, its
                                                **   test made again at its end; the lockpicks' reach first) */
            lt = LT_PICK;
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
            if (reach_step (0, st)) {
                return IR_NO;
            }
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
done:
    if (!d && !lt && reach_step (0, st)) {
        return IR_NO;                           /* (What it's in reached into first: part 84's) */
    }
    item_at (n)->shut = s;
    hub_map (0);
    HUB->ireq.x = d ? 0 : t;                    /* (Done: its time spent already) */
    HUB->ireq.qty = lt;                         /* (Begun: its first words the UI's) */
    HUB->ireq.item = rec.kind;
    return ok;
}

#pragma static-locals (pop)

/* ==== Part 61: what's in a thing (part 36's, its room: the keys that fit, 2026-10-27) */
#pragma code-name (push, "SW61CODE")
#pragma rodata-name (push, "SW61RODATA")

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
        if (reach_step (0, n)) {
            return IR_NO;
        }
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

/* The theme's kinds (the theme bank's, the UI's: theme.h) into the catalogue, after its own
** (part 61's, part 13's room: 2026-10-27) */
void s_theme_kinds (void)
{
    struct kind k;
    unsigned char i;

    for (i = 0; i < NTKINDS; ++i) {
        hub_map (THEME_BANK);
        memcpy (&k, &THEME->kind[i], sizeof k);
        hub_map (KIND_BANK);
        memcpy ((struct kind*) CATALOGUE + KI_THEME + i, &k, sizeof k);
    }
    hub_map (0);
}

/* Box b locked, key k made to fit it alone (the kit's, dq0 -k's "chest,iron key": outmake.c's rule, every making's;
** HOUSE.md's keys that fit) */
void __fastcall__ s_kit_lock (unsigned b, unsigned k)
{
    struct item* p;
    unsigned id;

    make_own (b);
    make_own (k);
    p = item_at (b);
    p->shut = IS_CLOSED | IS_LOCK | IS_LOCKED;
    id = p->id;
    p = item_at (k);
    p->size = id;
    p->thick = id >> 8;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 62: a denizen's things made out (2026-10-27) */
#pragma code-name (push, "SW62CODE")
#pragma rodata-name (push, "SW62RODATA")

/* (The list's last ", " made " and ": part 61's and_last, its own copy) */
static void __fastcall__ and_last62 (char* from)
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

/* ---- A denizen's things as the character makes them out (IO_GEAR: the user's, 2026-10-27, "the game say general
** things when looking, more detail when examining, including basic quality of visible equipment when assessing ...
** eyesight should dictate quality/granularity ... based on viewing distance"; HOUSE.md's three levels): the actor
** named (iq.name, a word of its name: the nearest on the character's stratum), its things in sight (in hand, worn but
** what's beneath its clothes, at its belt: hung there, or in a carrier there; what's in its pouch or its pack never),
** each one the eyes make out from there (within its kind's seen squares and half the eyes' sight more: a thing's look
** from afar, appraise.c's), assessed (level 2) its make and its condition too, within half that.  Every viewer's rule;
** the character's alone asks it yet */
static const char gmake[10][9] = { "crude", "crude", "poor", "plain", "sound", "sound", "good", "fine", "fine",
    "master's" };                               /* (appraise.c's makes) */
static unsigned char gd, geye, glv, gn, gfar;
static struct item gr;

/* Thing i's words, if the eyes make it out (gd its distance, geye the eyes, glv the level): a list's next, after
** pre (the list's first) or ", "; in its carrier c (0 none): 1, or 0 (not made out: gfar) */
static unsigned char __fastcall__ gthing (unsigned i, const char* pre, unsigned c)
{
    const struct kind* k;
    unsigned char r, life, m;
    char nm[14], cn[14];

    fetch (&gr, i);
    k = kind_of (gr.kind);
    strcpy (nm, k->name);
    r = k->seen + (geye >> 1);
    if (gd > r) {
        gfar = 1;
        return 0;
    }
    put (gn++ ? ", " : pre);
    if (!(k->flags & K_PLURAL)) {
        put (glv == 2 && gd * 2 <= r ? (strchr ("aeiou", gmake[gr.quality % 10][0]) ? "an " : "a ") :
            strchr ("aeiou", nm[0]) ? "an " : "a ");
    }
    if (glv == 2 && gd * 2 <= r) {              /* (Assessed, near enough: its make, its condition) */
        put (gmake[gr.quality % 10]);
        put (" ");
        put (nm);
        life = gr.dur_max ? (unsigned char) (gr.dur / (gr.dur_max / 10 + 1)) : 10;
        if (life < 7) {
            m = gr.mat;
            put (life < 4 ? ", badly " : ", ");
            put (m == MT_IRON || m == MT_STEEL ? "rusted" : m == MT_LEATHER ? "cracked" : m == MT_CLOTH ? "frayed" :
                m == MT_WOOD ? "rotten" : m == MT_SILVER || m == MT_BRONZE ? "tarnished" : "worn");
        }
    } else {
        put (nm);
    }
    if (c) {
        strcpy (cn, kind_at (c)->name);
        put (" in a ");
        put (cn);
    }
    return 1;
}

/* ---- The market's goods (IO_GEAR's to | GR_GOODS, 2026-11-02: DENIZENS.md's trading): the thing named into that actor
** k (its did) carries, not what it wears or holds itself (its own gear in use), in what it carries (its pouch, its
** pack: what's at its belt): its item, its worth a unit's (as describe's, the appraisal's: a kind written in silver by
** its material, coins by their mints, as a local takes them), its count, its condition (dur, dur_max); the actor plus 1
** in x, its name in name.  IR_OK, or IR_NO (it has none) */
#define GR_GOODS        0x80
static unsigned char __fastcall__ goods (unsigned did, unsigned char k, const char* nm)
{
    const struct kind* kp;
    struct ireq* r;
    unsigned i, j, n = 0, v;
    char w[16];

    strcpy (w, iq.into);
    for (i = 0; !n && (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {
        for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {
        }
        fetch (&gr, j);
        if (gr.where != IW_DEN || (gr.owner | gr.hide << 8) != did) {
            continue;
        }
        fetch (&gr, i);
        if ((i != j || !(gr.flags & (I_WORN | I_HELD)) || gr.kind == KI_PENNIES) && named (kind_of (gr.kind)->name, w)) {
            n = i;
        }
    }
    hub_map (0);
    r = &HUB->ireq;
    r->x = k + 1;
    strcpy (r->name, nm);
    if (!n) {
        return IR_NO;
    }
    fetch (&gr, n);
    kp = kind_of (gr.kind);
    v = kp->value;
    if (kp->mat == MT_SILVER && gr.mat != MT_SILVER) {
        v = gr.mat == MT_GOLD ? v * 20 : gr.mat == MT_STONE ? v * 8 : gr.mat == MT_GLASS ? v / 10 : v / 60;
    }
    if ((kp->form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) {
        v = (unsigned) ((unsigned long) v * mix_worth (n) / 100);
        fetch (&gr, n);
        gr.dur = gr.dur_max = 0;
    }
    hub_map (0);
    r->item = n;
    r->value = v;
    r->qty = gr.qty;
    r->dur = gr.dur;
    r->dur_max = gr.dur_max;
    return IR_OK;
}

unsigned char s_gear_words (void)
{
    struct actor* a;
    unsigned char k, best = 0xFF, d, part, any = 0;
    unsigned i, j, did = 0, belt = 0;
    int dx, dy;
    char nm[12];
    char* t0;

    if (!(iq.to & GR_GOODS)) {
        text_new ();
    }
    glv = iq.to & 3;
    geye = iq.to >> 4;
    for (gd = 0xFF, k = 0; k < NACTORS; ++k) {  /* (The nearest of that name on its stratum) */
        hub_map (0);
        a = &HUB->actor[k];
        if (!a->state || a->sn != HUB->pc_stratum || !strstr (a->name, iq.name)) {
            continue;
        }
        dx = a->x - HUB->pc_x;
        dy = a->y - HUB->pc_y;
        dx = dx < 0 ? -dx : dx;
        dy = dy < 0 ? -dy : dy;
        d = dx > 250 || dy > 250 ? 250 : dx > dy ? dx : dy;
        if (d < gd) {
            gd = d;
            best = k;
            did = a->did;
            strcpy (nm, a->name);
        }
    }
    if (best == 0xFF) {
        return IR_NONE;
    }
    if (iq.to & GR_GOODS) {
        return goods (did, best, nm);           /* (The market's: a thing it carries, 2026-11-02) */
    }
    put (nm[0] < 'a' ? "" : "The ");
    put (nm);
    gfar = 0;
    for (part = 0; did && part < 3; ++part) {   /* (In hand, worn, at its belt) */
        gn = 0;
        hub_map (UI_BANK);
        t0 = tp;
        for (i = 0; (i = items (i, IN_DEN, 0)) != 0; ) {
            if ((ip->owner | ip->hide << 8) != did) {
                continue;
            }
            fetch (&gr, i);
            if (part == 0 && (gr.flags & I_HELD)) {
                gthing (i, any ? ". It holds " : " holds ", 0);
            } else if (part == 1 && (gr.flags & (I_WORN | I_HIDDEN | I_HELD)) == I_WORN) {
                if (kind_of (gr.kind)->flags & K_BELT) {
                    belt = i;
                }
                gthing (i, any ? ". It wears " : " wears ", 0);
            }
        }
        if (part == 2 && belt) {
            for (i = 0; (i = items (i, IN_X | IN_ITEM, belt)) != 0; ) {
                fetch (&gr, i);
                if ((gr.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN) {
                    continue;
                }
                j = kind_of (gr.kind)->slots ? items (0, IN_X | IN_ITEM, i) : 0;   /* (A carrier's: what's in it) */
                gthing (j ? j : i, any ? ". At its belt: " : " has at its belt ", j ? i : 0);
            }
        }
        if (gn) {
            any = 1;
            and_last62 (t0);
        }
    }
    put (!any ? (did && gfar ? " is too far off for you to make out what it carries." : " carries nothing that you can see.") :
        gfar ? ". There's more you can't make out from here." : ".");
    return IR_OK;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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
    if (!(n = pick (iq.name, mode == LM_AT ? HW_ANY : mode == LM_GO ? HW_GONE : HW_KNOWN))) {    /* (Carried, or seen: the
                                                **   one most to hand, of several; all but a plain look, remembered too,
                                                **   walked to; gone to: lying about) */
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
    if (ld > 1 && mode) {
        hub_map (0);                            /* (Out of reach, all but a plain look: walked over to first, where it
                                                **   lies, the UI's) */
        HUB->ireq.x = px;
        HUB->ireq.y = py;
        HUB->ireq.item = n;
        return IR_NOT_HERE;
    }
    if (mode == LM_GO) {
        mode = LM_AT;                           /* (Gone to, within reach already: where it lies) */
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
            word (!(lk.flags & K_BELT) && (lk.slots == FM_SWORD || lk.slots == FM_KNIFE) ?
                (rec.wear >= 128 ? "dull" : "dulled") : "worn");
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
        if ((cl = (lk.form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) != 0) {
            rec.size = rec.thick = 0;           /* (A pile of coins: no measure, its size and thick its mints' shares;
                                                **   its stamps told after, part 64's, 2026-10-28) */
        }
        closer (LM_CLOSE);                      /* (What a closer look shows besides: part 42's) */
        if (cl && ld <= NEAR_SQ) {
            mix_words (n);
        }
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
    unsigned n = pick (name, iq.to ? HW_KNOWN : HW_ANY);    /* (Carried, or seen: the one most to hand, of several; an
                                                **   appraisal's, remembered too; a name given's not) */
    unsigned char c = 100;

    if (!n) {
        return IR_NONE;
    }
    if (iq.to && seen_d > 1) {
        hub_map (0);                            /* (An appraisal, out of reach: walked over to first, the UI's) */
        HUB->ireq.item = n;
        for (fetch (&rec, n); rec.where == IW_ITEM; fetch (&rec, rec.x)) {
        }
        hub_map (0);
        HUB->ireq.x = rec.x;
        HUB->ireq.y = rec.y;
        return IR_NOT_HERE;
    }
    make_own (n);
    fetch (&rec, n);
    k = kind_of (rec.kind);
    if ((k->form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) {
        c = mix_worth (n);                      /* (A pile of coins: its worth as a local would take it, by its mints,
                                                **   part 63's; no measure, its size and thick its mints' shares) */
        fetch (&rec, n);
        rec.size = rec.thick = 0;
        rec.quality = 4;                        /* (A coin's worth its face, not its make's: the appraisal's middling
                                                **   make, its worth times one) */
        k = kind_of (rec.kind);
    }
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
    if (k->mat == MT_SILVER && rec.mat != MT_SILVER) {   /* (Worth by material, HOUSE.md's: a kind written in silver,
                                                **   by Treasure 5's jewelry materials, silver's 60d an ounce: twenty
                                                **   times in gold, eight in stone (jade's), a tenth in glass, a sixtieth
                                                **   in the base ones (bronze, pewter, bone and horn), 2026-10-28) */
        r->value = rec.mat == MT_GOLD ? k->value * 20 : rec.mat == MT_STONE ? k->value * 8 : rec.mat == MT_GLASS ?
            k->value / 10 : k->value / 60;
    }
    r->value = (unsigned) ((unsigned long) r->value * c / 100);
    r->size = rec.size;
    r->thick = rec.thick;
    r->to = k->seen << 8 | seen_d;
    return IR_OK;
}

#pragma bss-name (push, "SW40RODATA")           /* (Its own data, wanted only while it runs: not the main RAM's) */
#pragma static-locals (push, on)
/* The character's things as an outfit's list (outfit.h's struct ofit), its kit to carry on into a new dungeon (D8's,
** slice 4.4's): kit.dat in the save folder, written with the game (wgame.c's, LM_KIT: part 40's, none of
** the commands' path), read as a carried-on game's outfit (part 57's).  Each thing on it: what it wears and holds first
** (worn, in its main hand, its other, both), then what's in each in turn, its container's entry before it (PL_IN;
** KQ things that hold others at most); how many, its make, its material, lit; NOFIT at most; not a key (its lock the old dungeon's), nor the old
** theme's own things (its kind's number another thing's in the next theme's: its puzzle's pieces) */
#define KQ      12                              /* (The things it's listed that hold others, at most) */
static struct item kr;
static struct ofit ke;
static unsigned kq[KQ];                         /* (Each one's number, */
static unsigned char kqe[KQ], kn, kqn, kmh;     /*   its entry; the entries so far; how many of them; its main hand) */
static int kfd;
static char kp[56];

/* Thing i (ip's record), in entry in less one (0: on the character), listed */
static void __fastcall__ kit_put (unsigned i, unsigned char in)
{
    memcpy (&kr, ip, sizeof kr);
    if (kn >= NOFIT || kr.kind == KI_KEY || (kr.kind >= KI_THEME && kr.kind < KI_THEME + NTKINDS)) {
        return;
    }
    ke.kind = kr.kind;
    ke.qty = kr.qty;
    ke.quality = kr.quality;
    ke.place = in ? PL_IN : !(kr.flags & I_HELD) ? PL_WORN : (kr.sn & IH_BOTH) == IH_BOTH ? PL_BOTH :
        kr.sn & kmh ? PL_HAND : PL_OFF;
    ke.in = in;
    ke.flags = kr.flags & I_LIT ? TF_LIT : 0;
    ke.mat = kr.mat;
    write (kfd, &ke, sizeof ke);
    hub_map (KIND_BANK);
    if (kqn < KQ && (CATALOGUE[kr.kind].flags & (K_CONTAINER | K_BELT) || CATALOGUE[kr.kind].keeps & KF_ON)) {
        kq[kqn] = i;
        kqe[kqn++] = kn;
    }
    ++kn;
}

static void kit_keep (void)
{
    static unsigned i;
    static unsigned char k;

    hub_map (0);
    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (kp, HUB->savedir);
    strcat (kp, "/kit.dat");
    if ((kfd = open (kp, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    kmh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    kn = kqn = 0;
    for (i = 0; (i = items (i, IN_X | IN_ACTOR, 0)) != 0; ) {
        kit_put (i, 0);
    }
    for (k = 0; k < kqn; ++k) {
        for (i = 0; (i = items (i, IN_X | IN_ITEM, kq[k])) != 0; ) {
            kit_put (i, kqe[k] + 1);
        }
    }
    close (kfd);
}

#pragma static-locals (pop)
#pragma bss-name (pop)

/* Its weight, its lock, it gone over for traps (thing n, rec its record, lk its kind; its name said) */
unsigned char __fastcall__ s_look_more (unsigned n, unsigned char mode)
{
    unsigned w;
    unsigned char sh = rec.shut, k;
    char b[6];

    if (mode == LM_KIT) {
        kit_keep ();                            /* (The character's things kept: the world's save, wgame.c's) */
        return IR_OK;
    }
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
    "stone", "glass", "silver", "", "beeswax", "tallow", "gold" };

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
    static const char* const left[4][6] = {
        { "burnt out", "nearly burnt out", "mostly burnt down", "half burnt down", "a quarter burnt down", "hardly burnt" },
        { "out of oil", "nearly out of oil", "low on oil", "half full of oil", "three-quarters full of oil", "full of oil" },
        { "empty", "nearly empty", "a quarter full", "half full", "three-quarters full", "full" },
        { "used up", "nearly used up", "mostly used", "half used", "a little used", "hardly used" } };   /* (A thing drawn
                                                **   with, but ink: HOUSE.md's things used up) */
    const struct burn* b;
    unsigned w;

    if (rec.dur_max) {
        w = rec.dur / (rec.dur_max / 10 + 1);   /* (Its life left, in tenths) */
        word (life[w >= 10 ? 4 : w >= 7 ? 3 : w >= 4 ? 2 : w >= 2 ? 1 : 0]);
    }
    if (rec.wear >= 32) {                       /* (A blade dug with, dulled; a tool worn) */
        word (!(lk.flags & K_BELT) && (lk.slots == FM_SWORD || lk.slots == FM_KNIFE) ?
            (rec.wear >= 128 ? "its edge dull" : "its edge dulled") : "worn with use");
    }
    if (rec.id && (b = burn_of (rec.kind)) != 0) {
        w = b->life * 10;                       /* (A whole one's minutes: the quarters left) */
        word (left[lk.flags & K_LIGHT ? rec.kind == KI_LANTERN : (b->fuels & FU_DRAW) && rec.kind != KI_INK ? 3 : 2]
            [!rec.fuel ? 0 : rec.fuel >= w ? 5 : 1 + rec.fuel * 4 / w]);
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
static unsigned tw_sum;                         /* (A lot's count with its others' told with it: 0 its own) */
static struct item tw;                          /* (The thing worded, copied out, and its kind's record: the main */
static const struct kind* twk;                  /*   RAM's, many's call swapping this part out and in again) */

/* A thing in words (tw, twk its kind's): its kind, how many, and what anyone can see of its state (its make and worth
** are an appraiser's to judge) */
static void words (void)
{
    static char b[6];
    static unsigned char c;
    static unsigned q;

    c = twk->keeps & KF_CLOSES;
    if (tw.flags & I_NAMED) {                   /* (A name the character gave it: its id, marked for the UI, "#12 a
                                                **   sword", worded "Linda, your sword") */
        put ("#");
        put (utoa (tw.id, b, 10));
        put (" ");
    }
    q = tw_sum ? tw_sum : tw.qty;
    if (tw_n > 1 || (q == 1 && (twk->form & KF_COUNT))) {
        many (tbuf, tw_n > 1 ? tw_n : 1, tw.kind);   /* (Alike, told together; one of a lot, "a ration") */
        put (tbuf);
    } else {
        if (q > 1) {
            put (utoa (q, b, 10));
            put (" ");
        } else if (!(twk->flags & K_PLURAL)) {
            put (strchr ("aeiou", twk->name[0]) ? "an " : "a ");
        }
        put (twk->name);
    }
    if (tw.dur_max && tw.dur * 10 < tw.dur_max) {
        put (" (nearly broken)");
    } else if (tw.dur_max && tw.dur * 3 < tw.dur_max) {
        put (" (battered)");
    }
    if (tw.flags & I_LIT) {
        put (" (lit)");
    }
    if (tw.id && tw.fuel <= BURN_LOW && (tw.fuel || (twk->flags & K_LIGHT) || (unsigned char) (tw.kind - KI_OIL) <=
        KI_PITCH - KI_OIL) && burn_of (tw.kind)) {  /* (A light spent or low, a fuel empty: slice 4.1's; only a
                                                **   light's or a fuel's spent, burn_of not asked of the rest) */
        put (!(twk->flags & K_LIGHT) ? (tw.fuel ? "" : " (empty)") : tw.fuel ? " (burning low)" : " (spent)");
    }
    if (tw.wear >= 32) {                        /* (Its wear: a blade dulled, a tool worn) */
        put (!(twk->flags & K_BELT) && (twk->slots == FM_SWORD || twk->slots == FM_KNIFE) ? (tw.wear >= 128 ?
            " (dull)" : " (dulled)") : " (worn)");
    }
    if (c == KF_STOPPER ? tw.id && !(tw.shut & IS_CLOSED) : c && (tw.shut & IS_CLOSED)) {
        put (c == KF_STOPPER ? " (unstoppered)" : " (closed)");    /* (What closes it: a lid, a tie, shut; a stopper
                                                **   out) */
    }
}

/* A thing in words, any (part 11's to the others: its record copied, its words put as the text is going) */
void __fastcall__ s_thing_words (const struct item* q)
{
    memcpy (&tw, q, sizeof tw);
    twk = kind_of (tw.kind);
    words ();
}

/* What its words say of a thing's state, in a byte (two alike if theirs are: a thing's words are its kind's, how many,
** its make's wear and harm, lit, its fuel, closed: words's), p's (tw, or a record in the store's window: alike's, not
** copied out, 2026-10-30) */
static unsigned char __fastcall__ sig (const struct item* p)
{
    return (p->flags & I_LIT) | (p->dur_max && p->dur * 10 < p->dur_max ? 2 : p->dur_max && p->dur * 3 < p->dur_max ?
        4 : 0) | (p->wear >= 128 ? 8 : p->wear >= 32 ? 16 : 0) | (p->shut & IS_CLOSED ? 32 : 0) | (p->fuel <= BURN_LOW ?
        (p->fuel ? 64 : 128) : 0) | (p->id ? 0 : 1);
}

/* What the character carries gathered first, in one pass of the store (the records on an actor, and every record in
** or on a thing: what's in what it carries among them), its list and its load worded from that: each thing's
** contents found there, not by a pass each (a pass some 25,000 cycles: an inventory's passes were a third of a
** million); more than GOT_MAX of them, the passes as before (ngot 0xFF).  The list in the kinds' bank, after the outfits'
** (outfit.h's OF_LIST: the world's), not the C stack (its 48 there, 288 bytes, took the stack into the swapped parts'
** area, a part's code overwritten: the soak's hangs, 2026-10-27) */
#define GOT_MAX         200
static struct got { unsigned n, x; unsigned char where, kind; } ge;
#define GOT_AT          ((struct got*) 0x9B00)
extern unsigned char ngot;                      /* (tlist.s's, as next_got's are) */
extern unsigned char gcur;                      /* (The entry next_in gave last: the next looked for after it, a
                                                **   container's things in turn, not the list again for each,
                                                **   2026-10-27, the outfits' more things) */

/* The next record after i at where, x (IN_X's: items ()'s order, the records' own): the list gathered, tlist.s's
** next_got (in assembly: an inventory's every thing, 2026-10-30) */
#pragma static-locals (push, on)
extern unsigned nx_n, nx_x;                     /* (next_in's arguments: the caller's, set in these, no C stack's) */
extern unsigned char nx_w;
unsigned next_got (void);
static unsigned next_in (void)
{
    if (ngot == 0xFF) {
        return items (nx_n, IN_X | 1 << nx_w, nx_x);
    }
    hub_map (KIND_BANK);                        /* (The list's bank) */
    return next_got ();
}
#pragma static-locals (pop)

/* Each thing at where, x worded, and what's in it, or on it; its weight (quarter pounds) added to load's, the
** things it goes into the same as weight_in's (part 10's): what it all weighs */
static unsigned load;
static unsigned char lmode;                     /* (The words a list in a sentence, look in's: "a candle and two
                                                **   chisels", not "(in it: ...)") */
static unsigned char quiet;                     /* (In a thing closed: weighed, not told) */
#define wb              (tbuf + 40)             /* (A text's end, a closed thing's: nothing put) */
static unsigned char lsig, lkf, lwo, ln, lin;
static const struct kind* cl_k;
static struct item* cl_p;
static struct got* gp;
static unsigned char gm;
static unsigned j, jn;

/* Thing jn alike the one worded (tw, its sig lsig, worn lwo, nothing in it lin; i its number): the same kind, one each,
** worn alike, neither named nor held, nothing in either, the same in what their words say (sig's): 1 (told: its number
** added to those, nt their count); jn's record read where it is, not copied out (tw left as it is, 2026-10-30) */
static unsigned char __fastcall__ alike (unsigned i, unsigned* told, unsigned char* nt)
{
    static unsigned char r;

    cl_p = item_at (jn);
    r = cl_p->qty == 1 && (cl_p->flags & I_WORN) == lwo && !(cl_p->flags & (I_NAMED | I_HELD)) && sig (cl_p) == lsig;
    if (r && lin == 0xFF) {
        nx_n = 0;
        nx_w = IW_ITEM;
        nx_x = i;
        lin = lkf && next_in () != 0;           /* (Something in it: none alike; asked once, a candidate found) */
    }
    if (r) {
        nx_n = 0;
        nx_w = IW_ITEM;
        nx_x = jn;
        r = !lin && (!lkf || !next_in ());      /* (Nothing in it: asked only of a kind that holds things, its kind's
                                                **   the same; the list not gone through for each, 2026-10-28) */
    }
    if (r && *nt < 8) {
        told[(*nt)++] = jn;
    }
    return r;
}

static void __fastcall__ contents (unsigned char where, unsigned x, unsigned char depth)
{
    static const char* const hands_in[4] = { ", in your hand", ", in your left hand", ", in your right hand",
        ", in both hands" };
    unsigned i = 0, told[8];
    unsigned char first = 1, nt = 0, gk = 0;
    char* t;
    char* e;

    while (gcur = gk, nx_n = i, nx_w = where, nx_x = x, (i = next_in ()) != 0) {  /* (From its place: what's in it,
                                                **   and alike's, looked elsewhere) */
        gk = gcur;
        fetch (&tw, i);
        twk = kind_of (tw.kind);
        lkf = twk->flags;
        j = (unsigned) twk->weight * (lkf & K_PLURAL && !(twk->form & KF_COUNT) ? 1 : tw.qty);
        load += twk->form & KF_TENS ? LOT_WT (twk->form, j) : j;     /* (A lot by the ten: the missiles'; coins by
                                                **   the dram) */
        lkf = lkf & (K_CONTAINER | K_BELT) || twk->keeps & KF_ON;
        if (quiet) {                            /* (In a thing closed: its weight, not its words) */
            if (lkf) {
                contents (IW_ITEM, i, depth + 1);
            }
            continue;
        }
        if ((tw.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN) {
            continue;                           /* (Hidden, not found yet: worn beneath a body's clothes, the outfits'
                                                **   phase 3, found by searching it) */
        }
        for (ln = 0; ln < nt && told[ln] != i; ++ln) {
        }
        if (ln < nt) {
            continue;                           /* (Told with one alike before it: nothing in it, nothing more) */
        }
        ln = 1;
        if ((twk->form & (KF_COUNT | KF_SMALL)) == (KF_COUNT | KF_SMALL) && tw.qty > 125 && !(tw.flags & I_NAMED)) {
                                                /* (A pile of coins past 125 (the only lots past 250 together: a rich
                                                **   purse), the others after it there told with it, dq0answer's
                                                **   inventory the quicker for none else; a purse's 600 pennies in
                                                **   piles of 250, 2026-10-28) */
            tw_sum = tw.qty;
            for (jn = i; nx_n = jn, nx_w = where, nx_x = x, (jn = next_in ()) != 0; ) {
                cl_p = item_at (jn);
                if (cl_p->kind == tw.kind && !((cl_p->flags ^ tw.flags) & (I_NAMED | I_HIDDEN | I_HELD | I_WORN)) &&
                    nt < 8) {
                    tw_sum += cl_p->qty;
                    told[nt++] = jn;
                }
            }
        } else if (tw.qty == 1 && !(tw.flags & (I_NAMED | I_HELD))) {   /* (One that may have others alike: alike's) */
            lsig = sig (&tw);
            lwo = tw.flags & I_WORN;
            lin = 0xFF;                         /* (Whether something's in it: alike's to ask) */
            if (ngot != 0xFF) {
                hub_map (KIND_BANK);            /* (The list's bank in the window once, and again after alike's: an
                                                **   inventory the quicker, 2026-10-28) */
                for (gp = GOT_AT + gk + 1, gm = ngot - gk - 1; gm; --gm, ++gp) {  /* (Those after it in the list
                                                **   gathered, from its place: next_in's, not the list again for each) */
                    if (gp->where == where && gp->x == x && gp->kind == tw.kind) {
                        jn = gp->n;
                        ln += alike (i, told, &nt);
                        hub_map (KIND_BANK);
                    }
                }
            } else {
                for (jn = i; nx_n = jn, nx_w = where, nx_x = x, (jn = next_in ()) != 0; ) {
                    if (item_at (jn)->kind == tw.kind) {
                        ln += alike (i, told, &nt);
                    }
                }
            }
        }
        if (!depth) {
            put (first ? "You carry " : "; ");
        } else if (lmode && depth == 1) {
            put (first ? "" : ", ");
        } else {
            put (first ? (cl_k = kind_at (x), cl_k->flags & K_BELT || cl_k->keeps & KF_ON) ? " (on it: " : " (in it: " :
                ", ");
        }
        first = 0;
        tw_n = ln;
        words ();
        tw_n = 0;
        tw_sum = 0;
        if (!depth) {
            put (tw.flags & I_HELD ? hands_in[tw.sn & IH_BOTH] : tw.flags & I_WORN ? ", worn" : "");  /* (The hand
                                                **   it's in, its sn: slice 4.1b's) */
        }
        if (lkf) {
            t = tq;
            e = tqe;
            if (tw.shut & IS_CLOSED) {
                tq = tqe = wb + 39;             /* (Closed: what's in it not told, but its weight counted) */
                quiet = 1;
            }
            contents (IW_ITEM, i, depth + 1);
            quiet = 0;
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
    char b[6];
    unsigned i = 0;
#define d       ln                              /* (contents' own, j too, free before it: the main RAM's room) */
#define k       gm

    ngot = 0;
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        ge.n = i;
        ge.x = j = ip->x;
        ge.where = d = ip->where;
        ge.kind = ip->kind;
        for (k = 8; d == IW_ITEM && k; --k) {   /* (The character's alone: on it, or in what it carries, its chain
                                                **   followed up; the others' things, and what's in things lying
                                                **   about, left out: the list the shorter, its passes the quicker,
                                                **   2026-10-30) */
            cl_p = item_at (j);
            d = cl_p->where;
            j = cl_p->x;
        }
        if (d != IW_ACTOR || j) {
            continue;
        }
        if (ngot == GOT_MAX) {
            ngot = 0xFF;
            break;
        }
        hub_map (KIND_BANK);
        gp = GOT_AT + ngot++;
        gp->n = ge.n;
        gp->x = ge.x;
        gp->where = ge.where;
        gp->kind = ge.kind;
    }
#undef d
#undef k
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
    return i == rs_pack || (item_at (i)->where == IW_ACTOR && !item_at (i)->x);
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

    if ((k = water (W_FILL)) != IR_NONE) {
        return k;                               /* (A waterskin filled: part 74's, slice 4.9's) */
    }
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
        handled = n;                            /* (A blade sharpened, "fill" its words' verb: part 85's: HOUSE.md's) */
        return reach_do (RO_HONE);
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
        if (takes & FU_PITCH) {                 /* (A torch dipped: what comes first, part 85's) */
            handled = n;
            if (reach_do (RO_REDIP) != IR_OK) {
                return IR_NO;
            }
        }
        if (reach_step (c, RS_TOOL)) {
            return IR_NO;                       /* (Its fuel got out: a tool's steps; put away after the filling) */
        }
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
    handled = c;
    if (reach_do (RO_SPILL)) {
        return IR_OK;                           /* (In the dark, spilt: nothing filled) */
    }
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
    a = q.mat;                                  /* (Its make the maker's already: a portal's part's) */
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
    q.dur = q.dur_max;
    if (q.where == IW_SQUARE && a) {            /* (A portal's part set down: its wear, up to six tenths, as before) */
        q.dur -= q.dur_max / 10 * rnd (7);
    } else if (q.where == IW_SQUARE) {          /* (Found, lost or left: its condition by Treasure 1's table, its Found
                                                **   column, its life's tenths: broken 0-1, poor 2-3, fair 4-6, good
                                                **   7-9, as new 10; silver and gold the less decayed, the house's
                                                **   30 more on the roll, 2026-10-28) */
        h = rnd (100) + (m == MT_SILVER || m == MT_GOLD ? 30 : 0);
        q.dur -= q.dur_max / 10 * (h < 20 ? 9 + (h & 1) : h < 60 ? 7 + (h & 1) : h < 80 ? 4 + h % 3 : h < 95 ? 1 +
            h % 3 : 0);
    }
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
unsigned ihigh = NITEMS;                        /* (Every record from here on free: items ()'s end, items.s's, raised
                                                **   as a record's taken, 2026-10-27) */
static unsigned __fastcall__ put_down (unsigned char kind, unsigned char where, unsigned char sn, unsigned x,
    unsigned y, unsigned char qty)
{
    struct item* p;
    unsigned i;

    for (i = 1; i < ilim || it_room () && (i = 1); ++i) {   /* (Record 0 is never one: "none") */
        p = item_at (i);
        if (!p->kind) {
            if (i >= ihigh) {
                ihigh = i + 1;
            }
            p = item_at (i);
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
        if (e.make && e.make < 16) {            /* (Coins of an era of old, the maker's: their mint, whole, part 63's
                                                **   piles, 2026-10-28; a portal's part's make its material's above) */
            p->size = e.make << 4 | 15;
        } else if (e.make) {                    /* (A portal's part's make: the portal's) */
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
        if (reach_step (n, 0)) {
            return IR_NO;
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
#define KIND(i)         ((const struct kind*) m1 (kind_at, i))  /* (Item i's kind's, copied out: the kinds in their
                                                **   own bank) */
#define KIND_K(k)       ((const struct kind*) m1 (kind_of, k))
#define KFLAGS(k)       B (m1 (kind_flags, k))
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
    static unsigned char kf, form, lit, cf, keeps, room, slots, fam;

    k = KIND (n);
    kf = k->flags;
    form = k->form & KF_FORMS ? k->form & KF_FORMS : KF_SOLID;
    fam = k->slots;
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
    } else if (!(keeps & form) || (slots && fam != slots)) {    /* (A carrier its family's alone: a scabbard a
                                                **   sword's, a quiver arrows', outfits' kinds.md, 2026-10-27) */
        return form == KF_SOLID || slots ? SA_WONT_GO : SA_RUN_OUT;
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
    if (B (m2 (reach_step, n, c))) {
        return IR_NO;                           /* (A step of the reach first: part 84's) */
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
        return rs_st ? IR_NO : no (SA_HANDS);   /* (A step of the reach first: part 84's) */
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
    kf = KFLAGS (k);
    if (!(kf & K_WEAR)) {
        return kf & K_HAND ? hold (n) : no (SA_NO_WEAR);
    }
    for (i = 1; i < NITEMS; ++i) {              /* (One of a kind worn: a second pack, a second cloak, no) */
        FETCH (i);
        if (o.kind == k && o.where == IW_ACTOR && !o.x && (o.flags & I_WORN)) {
            return no (i == n ? SA_WORN : SA_WEAR_ONE);
        }
    }
    if (B (m2 (reach_step, n, 0))) {
        return IR_NO;
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
        return rs_st ? IR_NO : no (SA_HANDS);
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
    static unsigned char r, x, fr, f;

    fr = on && wsel == SL_NEW;                  /* ("Light a new torch", the user's, 2026-11-02: HOUSE.md's */
    for (n = 1; n < NITEMS && *iq.name != '#' && !*iq.into; ++n) {  /*   a new light: the old let fall, a spare got out, lit) */
        FETCH (n);
        if (o.kind && (KFLAGS (o.kind) & K_LIGHT) && !(o.flags & I_LIT) == on && (!on || !spent ()) &&
            !(fr && (o.flags & I_HELD)) &&
            (!*iq.name || named (KIND_K (o.kind)->name, iq.name)) && B (m1 (on_pc, n))) {
            break;
        }
    }
    if (n >= NITEMS || *iq.name == '#' || *iq.into) {
        if (!*iq.name) {
            return no (on ? SA_NONE_TO_LIGHT : SA_NONE_LIT);
        }
        if (fr && !*iq.into && !(n = m1 (at_hand, (unsigned) iq.name))) {
            return no (afar ? SA_REACH : SA_NO_SPARE);
        }
        if (fr) {
            goto spare;
        }
        if (*iq.into) {                         /* (The one in the other, or on it: a sconce's torch) */
            if (!(n = in_other ())) {
                return IR_NO;
            }
        } else if (!(n = m1 (at_hand, (unsigned) iq.name))) {
            return no (afar ? SA_REACH : SA_NO_SUCH);
        }
    }
spare:
    name_of (n, 1);
    FETCH (n);
    r = o.flags & I_LIT;
    x = o.where == IW_ITEM && !(KIND (o.x)->flags & K_BELT);  /* (In a container: the pack ...) */
    if (!(KFLAGS (o.kind) & K_LIGHT)) {
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
    if (!(f = B (m0 (flame)))) {
        return no (SA_NO_FLAME);
    }
    if (f == 4) {
        name_of (rs_tool, 0);                   /* (Known empty: at once, no steps) */
        return no (SA_EMPTY);
    }
    if ((c = m1 (shut_in, n)) != 0) {
        name_of (c, 0);                         /* (In a thing closed: opened first) */
        return no (SA_SHUT);
    }
    if (fr) {                                   /* (The old one in a hand let fall first, a step of its own) */
        for (c = 1; c < NITEMS; ++c) {
            FETCH (c);
            if (o.kind && o.where == IW_ACTOR && !o.x && (o.flags & I_HELD) && (KFLAGS (o.kind) & K_LIGHT) &&
                named (KIND_K (o.kind)->name, iq.name)) {
                rs_c = c;
                rs_do = RS_DROP;
                rs_hand = 0;
                rs_k = o.flags & I_LIT ? LT_LETBURN : LT_LETFALL;
                rs_what = o.kind;
                rs_t = 1;
                if (B (m2 (reach_step, 0, RS_GO))) {
                    return IR_NO;
                }
                break;
            }
        }
    }
    x = fr || (x && B (m1 (on_pc, n)));
    if (x && B (m1 (hand_free, n)) && B (m2 (reach_step, n, 0))) {
        return IR_NO;                           /* (The spare got out first, past its hands' refusal, hold's) */
    }
    if (f == 3 && (f = B (m2 (reach_step, rs_tool, RS_FIRE))) != 0) {
        if (f == 2) {
            name_of (rs_tool, 0);               /* (Found empty as it's used: told, known from now on) */
            return no (SA_EMPTY);
        }
        return IR_NO;                           /* (The fire-maker's steps first: part 84's) */
    }
    if (x && (m1 (reach_from, n), (r = hold (n)) != IR_OK)) {
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

/* The referee's search test of the thing in rec, its hiding the steps worse (2 more by touch; ease the better, 2 for a
** heap poked through, all the search on it): 1 if it's found */
static unsigned char __fastcall__ turned_up (unsigned char touch, unsigned char ease)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_UI];                    /* (The UI's: it waits on this request meanwhile) */
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_SEARCH;
    r->b = HIDE_MECHANICAL;
    r->steps = ease - (signed char) rec.hide - (touch ? 2 : 0);
    r = hub_ask (SLOT_UI);
    g = r->grade;
    r->state = REQ_FREE;
    return g >= G_MS;
}

/* The things lying on the squares within the search's reach (a square's distance off, squared, less than r) that the
** character sees (by touch, all of them), but what's hidden by magic: seek, the nearest named sname that it would find
** (in plain sight, or hidden and found by the referee's test, made now), sitem, sd2 its distance off squared; else each
** it finds (in the light, what's hidden alone, by the test; by touch, everything there, what's hidden by the test, the
** rest felt plainly), made known and worded ("You turn up a dagger", "Your hands find a sack, and a chisel"): how many.
** A room searched whole's stop (IOS_FLOOR): about square x, y, what's hidden alone, seen or not */
static unsigned char __fastcall__ scan (unsigned char seek, unsigned char r, unsigned char touch)
{
    unsigned i = 0, x, y, dx, dy;
    unsigned char sn, d, n = 0, b = iq.to >= IOS_FLOOR;
    unsigned char* t;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = b ? iq.x : HUB->pc_x;
    y = b ? iq.y : HUB->pc_y;
    while ((i = items (i, IN_SQUARE, 0)) != 0) {
        fetch (&rec, i);
        if (rec.sn != sn || (rec.flags & I_MAGIC) || (dx = rec.x - x + SEARCH_R) > 2 * SEARCH_R ||
            (dy = rec.y - y + SEARCH_R) > 2 * SEARCH_R || (d = d2_[dx] + d2_[dy]) >= r ||
            (!touch && !b && (!(t = at (sn, rec.x, rec.y)) || !(*t & T_VIS))) ||
            (seek ? d >= sd2 || !named (kind_of (rec.kind)->name, sname) :
            (!touch || b) && (rec.flags & (I_HIDDEN | I_KNOWN)) != I_HIDDEN)) {
            continue;
        }
        make_own (i);
        fetch (&rec, i);
        if ((rec.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN && !turned_up (touch, 0)) {
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

/* What the thing searched on is (the reply's craft: RC_*): the UI's words as it begins, hub_map (0) after */
#pragma static-locals (push, off)
static void sclass (void)
{
    const struct kind* k = kind_of (item_at (sitem)->kind);
    unsigned char c = RC_PLAIN;

    if (k->flags & K_CONTAINER) {
        c = RC_HOLDS;
    } else if (k->keeps & KF_ON) {
        c = item_at (sitem)->kind == KI_BODY ? RC_BODY : RC_HOLDS;
    } else if (k->cap_bulk) {
        c = RC_HEAP;
    }
    hub_map (0);
    HUB->ireq.craft = c;
}
#pragma static-locals (pop)

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
            if (reach_step (0, sitem)) {
                return IR_NO;                   /* (A pack worn taken off to search it, its levels, each its time first,
                                                **   and put on again after the search's minute, HOUSE.md's reach) */
            }
            sclass ();
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
        if (sitem) {
            sclass ();
        }
        return sitem ? IR_OK : IR_NONE;
    }
    HUB->ireq.x = sitem ? 30 + 32 * sd2 : 0;
    return IR_OK;
}

/* A search's end (IO_SEARCH: the request's to its reach, the world's, wtrap.c's: a square searched if its distance off,
** squared, is less): what it turns up, in the hub's text, by its form; the number found (or 1: words, found or not).
** About the character, nearby, on the floor: scan's.  For a thing: the one IO_SEEK found, if the search's reach came to
** it ("You find a dagger.", "You find no dagger here."); on or in one, part 47's on_it, a heap's poked; a wall's, none
** (the walls' alone).  And a room searched whole's, by the request's to (IOS_*: the world's at a stop, wroom.inc's) */
static unsigned char __fastcall__ poked (unsigned n, unsigned char touch);
extern unsigned char rs_said;
unsigned char s_search (void)
{
    unsigned char f = srch_form & 7, touch = srch_form & SF_TOUCH, n;

    text_new ();
    if (iq.to & IOS_THING) {
        ++rs_said;                              /* (The world waits on it: wroom.inc's) */
        sitem = iq.item;
        touch = iq.to & SF_TOUCH;
        f = SF_ON;
    } else if (iq.to & IOS_FLOOR) {
        touch = iq.to & SF_TOUCH;
        f = SF_FLOOR;
    }
    if (f == SF_WALL) {
        return 0;
    }
    if (f == SF_ON) {
        return !sitem ? 0 : kind_of (item_at (sitem)->kind)->cap_bulk && !(kind_of (item_at (sitem)->kind)->flags & K_CONTAINER) &&
            !(kind_of (item_at (sitem)->kind)->keeps & KF_ON) ? poked (sitem, touch) : on_it (sitem, touch);
    }
    if (f != SF_FOR) {
        if ((n = scan (0, iq.to & IOS_FLOOR ? 3 : iq.to, touch)) != 0) {
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

/* A heap searched (the user's playtest, 2026-11-02: a kind that hides, its cap_bulk): what's hidden on its square, in
** it, each by the search's test, 2 steps the easier (all the search on it: a draft); found, known: "You find a dagger in
** the straw.", "You find nothing in the straw.": 1 (its words) */
#pragma static-locals (push, off)
static unsigned char __fastcall__ poked (unsigned n, unsigned char touch)
{
    const char* s;
    unsigned i = 0, x, y;
    unsigned char sn, m = 0;

    make_own (n);
    fetch (&rec, n);
    memcpy (&lk, kind_of (rec.kind), sizeof lk);
    s = strstr (lk.name, " of ");               /* ("heap of straw": the straw) */
    s = s ? s + 4 : lk.name;
    sn = rec.sn;
    x = rec.x;
    y = rec.y;
    while (rec.where == IW_SQUARE && (i = items (i, IN_SQUARE, 0)) != 0) {
        if (ip->sn != sn || ip->x != x || ip->y != y || (ip->flags & (I_HIDDEN | I_KNOWN | I_MAGIC)) != I_HIDDEN) {
            continue;
        }
        make_own (i);
        fetch (&rec, i);
        if (!turned_up (touch, 2)) {
            continue;
        }
        item_at (i)->flags = (rec.flags & ~I_HIDDEN) | I_KNOWN;
        put (m++ ? ", and " : touch ? "Your hands find " : "You find ");
        thing_words (&rec);
    }
    put (m ? " in the " : touch ? "Your hands find nothing in the " : "You find nothing in the ");
    put (s);
    put (".");
    return 1;
}
#pragma static-locals (pop)

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 47 (its own, 2026-10-22): what's on a thing, or in it, searched (part 46's search's SF_ON: thing n): its
** things listed; closed, not looked into; a thing that holds nothing, nothing hidden about it.  No words of traps but
** for a search said to be for them (by touch, SF_TRAPS: examine's, part 40's look_more: "The chest shows no sign of a
** trap ..."; the user's playtest, 2026-11-02: "it says 'no sign of a trap', but I am not looking for traps").  A room
** searched whole's look over a thing (IOS_THING), what a closer look shows besides (part 42's).  1 (its words) */
#pragma code-name (push, "SW47CODE")
#pragma rodata-name (push, "SW47RODATA")
#pragma static-locals (push, on)

unsigned char __fastcall__ s_on_it (unsigned n, unsigned char touch)
{
    unsigned char on;
    unsigned j;

    make_own (n);
    fetch (&rec, n);
    memcpy (&lk, kind_of (rec.kind), sizeof lk);
    lpl = (lk.flags & K_PLURAL) || rec.qty > 1;
    on = (lk.keeps & KF_ON) != 0;
    for (on = 0, j = 0; !(rec.shut & IS_CLOSED) && (j = items (j, IN_X | IN_ITEM, n)) != 0; ) {
        if ((ip->flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN) {
            ip->flags |= I_KNOWN;               /* (Searched: what's worn beneath its clothes found, a body's, the
                                                **   outfits' phase 3, 2026-10-27; the search's minute its time) */
        }
    }
    on = (lk.keeps & KF_ON) != 0;
    if (rec.shut & IS_CLOSED) {
        put (lpl ? "They're closed: you go over the outside of them." : "It's closed: you go over the outside of it.");
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
        put (".");
    } else if (on || (lk.flags & (K_CONTAINER | K_BELT))) {
        put (on ? "There's nothing on the " : "There's nothing in the ");
        put (lk.name);
        put (".");
    } else if (!(iq.to & IOS_THING)) {
        put (touch ? "Your hands find nothing hidden about the " : "You find nothing hidden about the ");
        put (lk.name);
        put (".");
    }
    fetch (&rec, n);
    if (iq.to & IOS_THING) {
        if (rec.mat != MT_OTHER) {
            if ((lk.form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) {
                rec.size = rec.thick = 0;       /* (A pile of coins: no measure, s_look_at's) */
            }
            tqe = tbuf + sizeof tbuf - 1;
            closer (LM_CLOSE);                  /* (Its look over: what it's made of, its wear) */
        }
    } else if (srch_form & SF_TRAPS) {
        put (" The ");
        put (lk.name);
        look_more (n, LM_TRAPS);                /* (Its trap, asked: examine's check, part 40's) */
    }
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

    htwo = TWO (k) == 2;
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
#pragma register-vars (push, on)
unsigned char __fastcall__ s_hold_in (unsigned n, unsigned char want)
{
    register struct item* p;
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
    if (reach_step (n, 0)) {
        return IR_NO;                           /* (A step of the reach first: part 84's) */
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
#pragma register-vars (pop)

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

/* ==== Part 63 (2026-10-28: part 13's room, the coins'): the character's kit */
#pragma code-name (push, "SW63CODE")
#pragma rodata-name (push, "SW63RODATA")
#pragma static-locals (push, on)

/* A free record taken, its thing put down (part 13's put_down, a copy of its own) */
static unsigned __fastcall__ put_down63 (unsigned char kind, unsigned char where, unsigned char sn, unsigned x,
    unsigned y, unsigned char qty)
{
    struct item* p;
    unsigned i;

    for (i = 1; i < ilim || it_room () && (i = 1); ++i) {
        p = item_at (i);
        if (!p->kind) {
            if (i >= ihigh) {
                ihigh = i + 1;
            }
            p = item_at (i);
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
    unsigned i = put_down63 (kind, where, 0, x, 0, qty);

    if (!i) {
        return 0xFFFF;
    }
    make_own (i);                               /* (Part 13's) */
    return i;
}

/* ---- The character's kit: its outfit (part 57's, outfit.c: the universal kit and its occupation's, as the referee
** says, the outfits' file's); or, dq0 -K's (the tests') or without the outfits, the kit as it was: a torch lit in hand,
** a backpack worn (rations, a tinderbox in it), a belt (a pouch of pennies, a dagger, a waterskin hanging from it), and
** a spare torch and a flask of oil in the pack after (D5: slice 4.1's); and things more (dq0 -k, commas between: in the
** hub's text), each held if there's a hand for it, else in the pack */
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
    const struct kind* kk;
    unsigned i, box = 0;
    unsigned char k, off, old;
    char* c;
    char* more = tbuf;

    hub_map (0);
    k = HUB->gear & GE_OLD ? 0 : outfit ();
    if (!(old = !k)) {
        hub_map (KIND_BANK);                    /* (The money: in the pouch, silver pennies in lots of 250 at most (a
                                                **   lot's count a byte, joined to 250), all of it, told together; its
                                                **   odd farthings farthings, cut pennies: the user's, 2026-10-28,
                                                **   "Please keep farthings": part 57's till slice 4.4, its room) */
        for (i = ost.budget >> 2; i && (off = OF_N) < NOFIT; i -= box) {
            box = i > 250 ? 250 : i;
            memset (OF_LIST + off, 0, sizeof *OF_LIST);
            OF_LIST[off].kind = KI_PENNIES;
            OF_LIST[off].qty = box;
            OF_LIST[off].quality = 4;
            OF_LIST[off].place = PL_POUCH;
            OF_N = off + 1;
        }
        if ((box = ost.budget & 3) != 0 && (off = OF_N) < NOFIT) {
            memset (OF_LIST + off, 0, sizeof *OF_LIST);
            OF_LIST[off].kind = KI_FARTHINGS;
            OF_LIST[off].qty = box;
            OF_LIST[off].quality = 4;
            OF_LIST[off].place = PL_POUCH;
            OF_N = off + 1;
        }
        box = 0;
        hub_map (0);
        outmake (0);                            /* (Its outfit drawn: its things made) */
        if (--k) {                              /* (Its weapons their version: the referee's, by its Strength, D13's;
                                                **   the character's alone, on it) */
            for (i = 0; (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
                kk = kind_at (i);
                if (kk->craft == AP_WEAPON && (kk->flags & K_HAND) && on_pc (i)) {
                    item_at (i)->size = k;
                }
            }
        }
    }

    hub_map (UI_BANK);
    strncpy (more, UIREC->things_text, 63);     /* (The things more: the UI's -k, in the UI bank's text) */
    hub_map (0);
    more[63] = 0;
    off = HUB->pcsum.limbs & LB_LEFT ? IH_RIGHT : IH_LEFT;
    if (HUB->pcsum.limbs & (off == IH_LEFT ? LB_ARM_L : LB_ARM_R)) {
        off ^= IH_BOTH;                         /* (Its off arm useless: its torch in the other) */
    }
    for (k = 0; old && k < NKIT; ++k) {
        e = kit_list[k];
        made[k] = i = item_new (e[0], e[1] == 0xFF ? IW_ACTOR : IW_ITEM, e[1] == 0xFF ? 0 : made[e[1]], e[2]);
        item_at (i)->flags = e[3];
        if (e[3] & I_HELD) {
            item_at (i)->sn = off;              /* (Its torch a light: in its off hand, slice 4.1b's) */
        }
    }
    for (; *more; more = c) {
        i = 0xFFFF;
        if ((c = strchr (more, ',')) != 0) {
            *c++ = 0;
        } else {
            c = more + strlen (more);
        }
        if ((k = kind_named (more)) != 0 && (i = item_new (k, IW_ACTOR, 0, 1)) != 0xFFFF && !carry (i, 1)) {
            item_at (i)->where = IW_ACTOR;      /* (No room: held all the same) */
            item_at (i)->x = 0;
            item_at (i)->flags = (item_at (i)->flags & ~(I_WORN | I_HIDDEN)) | I_KNOWN | I_HELD;
            kk = kind_at (i);
            k = TWO (kk) == 2 ? IH_BOTH : off ^ IH_BOTH;
            item_at (i)->sn = k;                /* (Its hand: both for a two-handed thing, else the main) */
        }
        if (i != 0xFFFF && (k = item_at (i)->kind) == KI_CHEST) {
            box = i;                            /* (A key after a chest locks it, and fits it alone: the outfits'
                                                **   rule, every making's, HOUSE.md's keys that fit; part 61's) */
        } else if (i != 0xFFFF && k == KI_KEY && box) {
            kit_lock (box, i);
            box = 0;
        }
    }
    for (k = 0; old && k < sizeof spare; ++k) { /* (The pack's spare torch and flask of oil, D5's, after the things
                                                **   more: theirs made as they were) */
        item_new (spare[k], IW_ITEM, made[1], 1);
    }
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 64 (2026-10-28): the coins' mints (the user's: "each coin should include its provenance (where it was
** minted), with the local region mintage most common (80%) unless belonging to an historic/anchient hoard, which
** should, of course, be appropriately stocked", and "a single group of coins should have a mint, yes, but one "pile"
** may contain many different mintings"; HOUSE.md's money).  A pile of coins (a lot of pennies, of crowns: its kind's
** form KF_SMALL and KF_COUNT) is one thing; within it up to four groups by mint, each its mint (the theme's, theme.h's
** struct tmint: 1 ...) and its share of the pile (fifteenths, as near as they come) a byte, the high four bits and the
** low, of its size, thick and fuel (no coin's measured, none burns).  A part of a pile taken is the same mix; two piles
** put together, their groups merged (the four greatest, the rest folded into the greatest: the house's).  A pile with
** no mix yet (its fuel 0) is given today's as it's first wanted (a purse, a hoard of now: four coins in five the
** region's own, the rest its neighbours' and the dwarves', the house's); an old era's coins are the maker's (its mint,
** whole: s_stock_down) */
#pragma code-name (push, "SW64CODE")
#pragma rodata-name (push, "SW64RODATA")
#pragma static-locals (push, on)

static unsigned char cm_n, cm_era[TMINTS], cm_ways[TMINTS];    /* (The theme's mints: how many, each one's era, ways) */
static unsigned cm_c[TMINTS + 1];               /* (A pile's coins by mint: counts) */
static unsigned char* cm_b;                     /* (Its groups: its size, thick and fuel's bytes, each a mint (the high
                                                **   four bits: 0 none) and its share (the low four)) */
static unsigned char cm_s;                      /* (Their shares' sum) */

/* The theme's mints' eras and ways copied out */
static void cm_mints (void)
{
    const struct tmint* m = THEME->mint;
    unsigned char k;

    hub_map (THEME_BANK);
    cm_n = THEME->nmints;
    for (k = 0; k < TMINTS; ++k, ++m) {
        cm_era[k] = m->era;
        cm_ways[k] = m->ways;
    }
}

/* Pile i's groups found (cm_b, cm_s) */
static void __fastcall__ cm_at (unsigned i)
{
    unsigned char k;

    cm_b = &item_at (i)->size;
    for (cm_s = 0, k = 0; k < 4; ++k) {
        cm_s += cm_b[k] & 15;
    }
}

/* The groups made from the counts (cm_c, by mint), the four greatest, in fifteenths of their sum (each at least one,
** the greatest the rest), put in the pile's bytes (cm_b) */
static void cm_groups (void)
{
    unsigned t = 0, c;
    unsigned char k, j, b, left = 15, w;

    for (k = 1; k <= TMINTS; ++k) {
        t += cm_c[k];
    }
    for (j = 0; j < 4; ++j) {
        for (b = 0, k = 1; k <= TMINTS; ++k) {
            if (cm_c[k] > cm_c[b]) {
                b = k;
            }
        }
        w = 0;
        if ((c = cm_c[b]) != 0) {
            w = (unsigned char) ((c * 15 + t / 2) / t);
            w = !w ? 1 : w > left ? left : w;
            left -= w;
        }
        cm_b[j] = b << 4 | w;
        cm_c[b] = 0;
    }
    cm_b[0] += left;                            /* (The greatest the rest: fifteen in all) */
}

/* Thing i's mix, if it's a pile of coins with none yet: today's (its numbers', no die: four coins in five, a fifth more
** or less, the region's own, the rest another's of today, a neighbour's or the dwarves'; a crown the gold one's, the
** dwarves') */
void __fastcall__ s_mix_need (unsigned i)
{
    struct item* p = item_at (i);
    unsigned h = (p->id + i) * 40503U;
    unsigned char q = p->qty, kind = p->kind, k, f = 0, n = 0, local = 0, o[TMINTS];

    if (p->fuel || p->size || !(kind_of (kind)->form & KF_SMALL)) {
        return;
    }
    cm_mints ();
    for (k = 0; k < cm_n; ++k) {
        if (!cm_era[k]) {
            if (!local && cm_ways[k] & (kind == KI_CROWNS ? MW_GOLD : MW_LOCAL)) {
                local = k + 1;
            } else if (kind != KI_CROWNS) {
                o[n++] = k + 1;
            }
        }
    }
    if (n) {                                    /* (A fifth, more or less, the others': one at least one time in five) */
        f = !local ? q : (unsigned char) (((unsigned) q * (10 + h % 21) + 50) / 100);
        if (!f && !(h % 5)) {
            f = 1;
        }
    } else if (!local) {
        return;
    }
    memset (cm_c, 0, sizeof cm_c);
    cm_c[local] = q - f;
    cm_c[o[(h >> 5) % (n | !n)]] += f;
    cm_at (i);
    cm_groups ();
}

/* Pile n joined to pile i (alike, its count not yet added): their groups merged, each group's coins counted (its
** share of its pile) */
void __fastcall__ s_mix_join (unsigned i, unsigned n)
{
    unsigned char k, j, g;
    unsigned q;

    s_mix_need (i);
    s_mix_need (n);
    memset (cm_c, 0, sizeof cm_c);
    for (j = 0; j < 2; ++j) {
        q = item_at (j ? n : i)->qty;
        cm_at (j ? n : i);
        for (k = 0; k < 4 && cm_s; ++k) {
            g = cm_b[k];
            cm_c[g >> 4] += (q * (g & 15) + cm_s / 2) / cm_s;
        }
    }
    cm_c[0] = 0;
    cm_at (i);
    cm_groups ();
}

/* Pile i's mix in words, put (examined, near): its stamps, each group by how much of it it is ("most", "many", "some",
** "a few"; one alone "all"), of whom, old if an era's of old ("Their stamps: most of Velmor, a few of Kharzad.") */
void __fastcall__ s_mix_words (unsigned i)
{
    static const char* const much[4] = { "a few", "some", "many", "most" };
    static char nm[16];
    static unsigned char g[4];
    unsigned char k, m;

    s_mix_need (i);
    cm_mints ();
    cm_at (i);
    memcpy (g, cm_b, 4);
    for (k = 0; k < 4 && (m = g[k] >> 4) != 0 && m <= cm_n; ++k) {
        put (k ? ", " : " Their stamps: ");
        put (g[1] < 16 ? "all" : much[(g[k] & 15) * 4 / (cm_s + 1)]);
        put (" of ");
        hub_map (THEME_BANK);
        strcpy (nm, THEME->mint[m - 1].name);
        put (nm);
        put (cm_era[m - 1] ? " (old)" : "");
    }
    put (k ? "." : "");
}

/* Pile i's worth as a local would take it, in hundredths of its face (the book's, HarnWorld 23: coins of elsewhere
** discounted 10 to 20%, the house's 15; at par the dwarves' and the region's own; hard to pass, half; an old era's, no
** realm's now, its silver's, three quarters) */
unsigned char __fastcall__ s_mix_worth (unsigned i)
{
    unsigned char k, w, m, v;
    unsigned t = 0;

    s_mix_need (i);
    cm_mints ();
    cm_at (i);
    for (k = 0; k < 4; ++k) {
        v = 100;
        if ((m = cm_b[k] >> 4) != 0 && m <= cm_n) {
            w = cm_ways[m - 1];
            v = cm_era[m - 1] ? 75 : w & (MW_LOCAL | MW_PAR) ? 100 : w & MW_HARD ? 50 : 85;
        }
        t += (cm_b[k] & 15) * v;
    }
    return cm_s ? (unsigned char) (t / cm_s) : 100;
}

#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
