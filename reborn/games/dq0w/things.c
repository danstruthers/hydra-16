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
#include "hub0.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "../dq0/life.h"                        /* (The clues' CLUE_*: kit_put's) */
#include "wbody.h"
#include "acts.h"
#include "outfit.h"
#include "line.h"                               /* (ln_pass: a thing thrown stopped short, s_throw_at's) */

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
void inventory (void);                          /* Part 105 */
void __fastcall__ wear (const char* key, unsigned char by);
unsigned char search (void);                    /* Part 46 */
unsigned char seek (void);
void __fastcall__ thing_words (const struct item* q);   /* (Part 11's) */
unsigned char __fastcall__ hide (const char* name);
void __fastcall__ make_own (unsigned i);         /* Part 13 */
void kit (void);                                /* Part 63 (part 13's till 2026-10-28) */
void theme_kinds (void);
unsigned __fastcall__ pick (const char* name, unsigned char how);    /* Part 4 (its room): a thing of several */
unsigned __fastcall__ at_hand (const char* name);
unsigned __fastcall__ carried_named (const char* name);
unsigned __fastcall__ seen_named (const char* name);
unsigned char __fastcall__ moved (unsigned char op);
unsigned char shut_work (void);
unsigned char inside (void);                    /* Part 36 */
void tools (void);                              /* Part 10 */
unsigned __fastcall__ look_pick (void);         /* Part 4 */
void __fastcall__ look_on (unsigned n);         /* Part 39 */
unsigned char __fastcall__ describe (const char* name);    /* Part 40 */
unsigned __fastcall__ shut_in (unsigned n);     /* Part 35 */
unsigned __fastcall__ s_shut_in (unsigned n);
void __fastcall__ many (char* b, unsigned n, unsigned char k);    /* Part 105 */
void __fastcall__ s_many (char* b, unsigned n, unsigned char k);  /*   (its own, there) */
unsigned char bar_hand (void);                  /* Part 35 */
unsigned char __fastcall__ hold_in (unsigned n, unsigned char want);    /* Part 48 */
unsigned char swap (void);
unsigned char gear_words (void);                /* Part 62 */
void __fastcall__ kit_lock (unsigned b, unsigned k);   /* Part 61 */
void __fastcall__ mix_need (unsigned i);        /* Part 64: a pile of coins's mints (2026-10-28) */
void __fastcall__ mix_join (unsigned i, unsigned n);
unsigned __fastcall__ join (unsigned n);    /* Part 35: a lot joined to one alike where it is (the hands' purse, part 75's) */
void __fastcall__ mix_words (unsigned i);
unsigned char __fastcall__ mix_worth (unsigned i);
void __fastcall__ listed (unsigned n);          /* Part 11 */
unsigned __fastcall__ purse (unsigned n);       /* Part 10: coins' purse */
unsigned char textile (void);                   /* Part 107: a cloth torn up, washed (wcloth.c, 2026-11-10) */
#define HW_CARRIED      1                      /* (pick's how: carried; seen, lying about; either, as far as it sees,
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
unsigned char __fastcall__ throw_at (const char* name);    /* Part 88 */

/* A buffer in the main RAM (the words said, the kit's names, the store kept a piece at a time) */
static char tbuf[80];

/* A sentence of the world's put after the words so far, coded (items.h's SAY_AT, its SA_* | 0x80, the thing's name a,
** SAY_SEP, the other's b, SAY_SEP): said.txt's SA rows word it, the UI's (narration as data's rest, 2026-11-15: the
** parts' own sentences, the locks', the fill's, the hands').  A copy in each part that says one, SAY_PUT (its name):
** the main RAM's room and its layout as they were (a step's cost, dq0traces': the C library's place) */
#define SAY_PUT(f)  static void __fastcall__ f (unsigned char sa, const char* a, const char* b) { tbuf[0] = SAY_AT; \
                    tbuf[1] = sa | 0x80; tbuf[2] = 0; put (tbuf); put (a); put ("\2"); put (b); put ("\2"); }

/* A thing's record, copied out: the one the functions share that call none that wants it while they use it (others
** have their own: make_own's, the recursive ones', light's ...) */
static struct item rec;

/* (item_at and items are items.s's, in assembly: its store's layout the build's from these headers, dq0hub_items.inc) */

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

/* ---- The holders' compartments (COMMANDS-CRITIQUE.md's §4.9, step 6b, 2026-11-15: items.h's struct comp, kinds.c's
** comps at the start of the catalogue's bank).  Kind k's compartment c copied out into crow: 1; 0 if it has none (c
** past its last, or a kind that holds nothing); a kind with no rows (a theme's) its record's own, compartment 0.  The
** kinds' bank, or the catalogue's, left in the window */
extern const struct comp comps[];
extern const char cnouns[NCN][CN_LEN];          /* (The compartments' own names: kinds.c's, the catalogue's bank's) */
struct comp crow;
unsigned char ccomp;                            /* (The compartment a thing's put in: stow's, the catalogue's code's; 0
                                                **   anywhere else, the pack's, a sheath's ...) */
unsigned char lv_c;                             /* (level_of's: the compartment of the first container reached into,
                                                **   reach_to's, handy's; the next's as it goes out) */
unsigned char rs_cc;                            /* (The compartment a thing's put in, the reach's: stow's, rs_walk's) */
unsigned char cont_c;                           /* (cont's compartment: stow's, serve's reach_to) */
unsigned char slid;                             /* (What was on a lid slid off as it was opened: part 36's, serve's) */
unsigned char lst_c = 0xFF;                     /* (listed's: the one compartment's things told, 0xFF all) */
void cat_map (void);
unsigned char __fastcall__ o_comp_of (unsigned k, unsigned char c);   /* (The catalogue's bank's, below) */
unsigned char __fastcall__ comp_of (unsigned k, unsigned char c)
{
    cat_map ();
    return o_comp_of (k, c);
}

/* Thing i's first compartment from c on that's open, its row in crow; 0xFF none (the catalogue's bank's o_comp_open):
** a list's, what's in it told by each */
unsigned char cfilt;                            /* (Its filter, contents': the compartment plus 1, times 16; 0 a
                                                **   holder of one) */
unsigned char __fastcall__ o_comp_open (unsigned i, unsigned char c);

/* The compartment of kind k that name names by its own name ("the drawer", "the side pocket": the one name match,
** named's; the catalogue's bank's o_comp_named), its row in crow: its number; 0 none */
unsigned char wcn;                              /* (A compartment's name asked for too: the verbs that take one, a put's
                                                **   and a take's other, what closes worked, a look in) */
unsigned char wcomp;                            /* (The compartment the chooser's thing was named by: pick's; 0 none) */
unsigned char wcin;                             /* (The compartment a thing's looked for in, HW_IN's: 0 any) */
unsigned char __fastcall__ o_comp_named (unsigned k, const char* name);
unsigned char __fastcall__ comp_named (unsigned k, const char* name)
{
    cat_map ();
    return o_comp_named (k, name);
}

/* The compartment of kind k name names, if it doesn't name the kind (the chooser's thing's: pick's wcomp) */
static unsigned char __fastcall__ name_comp (unsigned char k, const char* name)
{
    hub_map (KIND_BANK);
    return named (kinds[k].name, name) ? 0 : comp_named (k, name);
}
unsigned char __fastcall__ comp_open (unsigned i, unsigned char c)
{
    cat_map ();
    return o_comp_open (i, c);
}

/* The next thing after i in thing x (items ()'s, for the catalogue's bank's code: its two arguments, m2's) */
unsigned __fastcall__ items_in (unsigned i, unsigned x)
{
    return items (i, IN_X | IN_ITEM, x);
}

/* Thing i's compartment as its record says (IC_OF, unchecked: the item bank left in the window) */
unsigned char __fastcall__ ic_of (unsigned i)
{
    return IC_OF (item_at (i)->y);
}

/* Thing i's compartment in what it's in (its record's y, IC_MARK's: one its holder has; else 0) */
unsigned char __fastcall__ icomp (unsigned i)
{
    const struct item* p = item_at (i);
    unsigned char c;

    if (p->where != IW_ITEM || !(c = ic_of (i))) {
        return 0;
    }
    return comp_of (item_at (p->x)->kind, c) ? c : 0;
}

/* Whether thing i (in another) is in a compartment of it that's closed (a chest's inside, its lid shut; not its lid's
** top) */
unsigned char __fastcall__ in_shut (unsigned i)
{
    unsigned x = item_at (i)->x;
    unsigned char c = icomp (i);

    return (item_at (x)->shut & IS_SHUT_C (c)) != 0;
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
        inventory ();
    } else if (k == IO_TOOLS) {
        tools ();                               /* (Part 10's alone, as every thing handled tells the referee what's to
                                                **   hand) */
    } else if (k == IO_LOOK) {
        look_on (look_pick ());                 /* (A thing looked at: part 4's chooser, then part 39's words, with no
                                                **   part 10 to come back to, four swaps less before its first letter:
                                                **   the user's D17, 2026-11-03) */
        return 0;
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
    unsigned char k, n;
    unsigned i;
    const struct item* p;

    for (k = 0; hub_map (SIGHT_BANK), n = SIGHT->nitems, k < n; ++k) {
        i = SIGHT->item[k].n;
        p = item_at (i);
        if (!p->id && p->kind) {
            make_own (i);                       /* (Part 13's copied only for one not yet its own) */
        }
    }
    hub_map (0);
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
    struct item* p = item_at (i);

    memcpy (p, q, sizeof *q);
    if (p->where != IW_ITEM && p->y >= IC_MARK) {
        p->y = 0;                               /* (Out of a compartment: none kept, items.h's IC_MARK) */
    }
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
unsigned hand_n;                                /* (The thing serve's at_hand found, for the handling's own: s_at_hand's) */
unsigned char hand_sel;                         /*   (and the selector it was asked with) */
unsigned csum;                                  /* (pick's: how many of those it chose from, all told) */
unsigned char chart_late;
unsigned chart_w[5];                            /* (The map making's things, part 26's, handy, by their keys) */
unsigned rtime, roff;                           /* (The reach owed, part 10's: its time, the pack taken off for it, */
unsigned char rsteps, rbody;                    /*   its steps; the main RAM's, part 35's too) */

/* The request, copied (serve's), and what its handling shares (the hands, the pack, the belt: about's; the last put
** down; how far the thing named is, seen_named's; out of reach, at_hand's; the things in a container, bulk_in's; the
** time it took) */
struct ireq iq;
static unsigned pack, belt, dropped, seen_d, took;
static unsigned char afar, nin;                /* (afar: 1 out of reach, 2 down in a pit from its edge, R_PIT's) */
static unsigned far_n, far_x, far_y;            /* (The thing at_hand found out of reach, and where it lies) */
#define R_PIT           29                      /* (pick_'s rank of a thing down in a pit, not the character's square: out
                                                **   of reach from its edge, the user's, 2026-11-18, H10; the worst) */
static char xn[16], yn[16];                     /* (The two names said: the words asked, then the things'; the
                                                **   catalogue's bank's code's, and a count's words) */
static unsigned char last_sa;                   /* (The words the catalogue's code said last: SA_*) */
static unsigned cont;                           /* (What a thing was put in or on last: stow's) */
static void __fastcall__ say (unsigned char n);   /* (The catalogue's bank's: its words, SA_*) */

/* A name said: a word of the kind's ("key" an iron key; "sword" a broken one too), or its start; or that word made
** plural ("sacks", "torches": "drop two sacks"); or the end of the name, a word longer than 3 ("pack" a backpack, "skin"
** a waterskin, 2026-10-26).  The one rule (COMMANDS-CRITIQUE.md's 5.1 step F, 2026-11-07): the chooser's (its copy,
** part 4's named4, gone into this), the light's, the market's, the search's; the UI's main.c's name_is, the same.  ""
** names any (the chooser asks none with it).  (The catalogue's bank's code's; the main RAM's since 2026-11-04, the swap
** study's: part 10 copied thousands of times for it and on_pc) */
static unsigned char __fastcall__ word_of (const char* k, const char* w, unsigned char n)
{
    for (;;) {
        if ((*k == *w || !n) && !strncmp (k, w, n)) {
            return 1;
        }
        if (!(k = strchr (k, ' '))) {
            return 0;
        }
        ++k;
    }
}

unsigned char __fastcall__ named (const char* k, const char* w)
{
    unsigned char n = strlen (w), m;

    return word_of (k, w, n) || (n > 2 && w[n - 1] == 's' && (word_of (k, w, n - 1) ||
        (w[n - 2] == 'e' && word_of (k, w, n - 2)))) || (n > 3 && (m = strlen (k)) > n && !strcmp (k + m - n, w));
}


/* The character's (on it, or in what it carries) */
unsigned char __fastcall__ on_pc (unsigned i)
{
    unsigned j;

    for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {         /* (Up to whoever has it) */
    }
    return j == rs_pack || (item_at (j)->where == IW_ACTOR && !item_at (j)->x);     /* (A pack off for a handling, at
                                                **   its feet, its as ever: part 85's) */
}

/* ==== Part 10: the requests answered, a thing handled (and what the catalogue's bank's code calls on: its stubs) */
#pragma code-name (push, "SW10CODE")
#pragma rodata-name (push, "SW10RODATA")

/* Where a thing lies about: on a square, or on a surface that is (a table's): the record on the square (its own, or
** the surface's, copied), or 0 */
static const struct item* __fastcall__ lying (const struct item* q)
{
#pragma bss-name (push, "SW10RODATA")
    static struct item s;                       /* (Part 10's own: its one caller, s_in_reach, swaps no part in while
                                                **   it's used; the main RAM's room, 2026-11-18) */
#pragma bss-name (pop)

    if (q->where == IW_SQUARE || q->where == IW_PORTAL) {
        return q;                               /* (A portal's part as a thing lying there) */
    }
    if (q->where != IW_ITEM) {
        return 0;
    }
    fetch (&s, q->x);
    return s.where == IW_SQUARE && (q->y >= IC_MARK ? IC_PREP (q->y) == CP_ON : kind_of (s.kind)->keeps & KF_ON) ? &s :
        0;                                      /* (On a chest's lid too: its compartment's prep, items.h's IC_MARK) */
}

/* Whether thing n can go in c (or on it: a surface; hung from it: a belt): 0, or the words why not (SA_*): the
** catalogue's bank's o_fits */
static unsigned char __fastcall__ o_fits (unsigned n, unsigned c);
static unsigned char __fastcall__ fits (unsigned n, unsigned c)
{
    cat_map ();
    return o_fits (n, c);
}


/* Within reach of the character (on a square next to it, or its own; or on a surface there); not down in a pit but
** its own square (the user's, 2026-11-18, H10: from inside it alone, every actor's: wtake.c's take rule the same) */
unsigned char __fastcall__ s_in_reach (unsigned n)
{
    const struct item* s;
    const unsigned char* p;
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
    return s && s->x - x + 1 <= 2 && s->y - y + 1 <= 2 && ((s->x == x && s->y == y) || !(p = at (s->sn, s->x, s->y)) ||
        TILE (*p) != T_PIT);
}

/* ---- What's in a thing, or on the character: the bulk and weight (quarter pounds) of what's in it (a thing's own
** with what's in it, or on it: a soft thing's bulk its own or what it holds, the more) */
unsigned load_qp;                               /* (All the character carries, last weighed, quarter pounds) */
unsigned __fastcall__ s_weight_in (unsigned where, unsigned x);
unsigned __fastcall__ s_bulk_in (unsigned x);
unsigned __fastcall__ s_weight_of (unsigned i)
{
    const struct kind* k = kind_at (i);
    unsigned w = (unsigned) k->weight * (k->flags & K_PLURAL && !(k->form & KF_COUNT) ? 1 : item_at (i)->qty);  /* (A
                                                **   lot counted, its count's: "drop two rations") */
    unsigned char v;

    if (k->craft == AP_WEAPON && (v = item_at (i)->size) != 0 && v <= WV_HEAVY) {
        if (v == WV_HEAVY) {                    /* (A weapon's version: a quarter heavier, or lighter, D13's) */
            w += w >> 2;
        } else {
            w -= w >> 2;
        }
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
    if (where == IW_ACTOR && !x) {
        load_qp = w;                            /* (All it carries, as the referee's told: the inventory's too) */
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
        pack = rs_pack;                         /* (Off for this handling, part 85's: its pack still) */
    }
}

/* The purse coins n go in as they're put away (the user's, 2026-11-17: "coins to the purse"; taken up, stowed, a hand
** freed of them): the first pouch hanging from the character's belt that they fit in (open, its room: a container that
** hangs, of no family, fits'); 0 none, or n no coins (a lot of small things counted: pennies, farthings, crowns).  The
** belt about's */
unsigned __fastcall__ s_purse (unsigned n)
{
    const struct kind* k;
    unsigned i = 0;

    s_about ();
    k = kind_at (n);                            /* (After about's: kind_of's copies go round) */
    if ((k->form & (KF_SMALL | KF_COUNT)) != (KF_SMALL | KF_COUNT) || !belt) {
        return 0;
    }
    while ((i = items (i, IN_X | IN_ITEM, belt)) != 0) {
        k = kind_at (i);
        if ((k->flags & (K_CONTAINER | K_HANG)) == (K_CONTAINER | K_HANG) && !k->slots && !fits (n, i)) {
            return i;
        }
    }
    return 0;
}

/* Thing n put somewhere (where, x: items.h's), as it's flagged (held, worn, or neither), no longer hidden (nor
** tended: a light its keepers kept burning, taken up, burns down) */
static void __fastcall__ place (unsigned n, unsigned char where, unsigned x, unsigned char flags)
{
    struct item* p = item_at (n);

    p->where = where;
    p->x = x;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | flags;
    if (p->y >= IC_MARK) {
        p->y = 0;                               /* (Out of a compartment past the first: its first, or none) */
    }
    if ((where = p->kind - KI_THEME) < NTKINDS) {
        take_go ((const struct msg*) (unsigned) where);   /* (A theme's own thing come to hand: its take rule's, Q2's) */
    }
}

/* Thing n carried: coins into the purse (s_purse's, the user's, 2026-11-17); else into the pack if it fits, else in a
** hand, the one that suits it (part 48's: hand first, the other
** way about, and 2 said so as it goes in the pack, the hands full: a thing taken up, the user's, 2026-11-06; and a thing taken from a fixture, a sconce's torch, the user's, 2026-11-03: the hand, the pack only with both
** full): 1 in the pack or the purse (cont), 2 in a hand, 0 neither (full; or a step of the reach begun first, rs_st) */
#define HF_FIXT         0x80                    /* (hand_free's: thing n's in a fixture) */
unsigned char __fastcall__ hand_free (unsigned n);
unsigned char __fastcall__ s_carry (unsigned n, unsigned char hand_first)
{
    unsigned char h;
    unsigned c;
    const char* w;

    h = hand_free (n);
    if (h & HF_FIXT) {
        hand_first = 1;                         /* (From a fixture: the hand, part 48's) */
        h &= ~HF_FIXT;
    }
    if (!(c = s_purse (n)) && (!hand_first || !h) && pack && !fits (n, pack)) {
        c = pack;                               /* (Coins in the purse first, the user's, 2026-11-17; else the pack) */
    }
    if (c) {
        if (reach_step (n, c)) {
            return 0;                           /* (A step of the reach first: part 85's) */
        }
        place (n, IW_ITEM, cont = c, 0);        /* (cont: where it went, carried's words) */
        lv_c = 0;
        reach_to (c);                           /* (Put away: its reach, HOUSE.md's: part 56's) */
        if (hand_first == 2 && c == pack) {     /* (Taken up, its hands full: said so, the user's, 2026-11-06; a plural
                                                **   "them", its name's last letter an s, not two: refuse.c's tt_words') */
            put ("You've no hand free for the ");
            put (w = kind_of (item_kind (n))->name);
            h = strlen (w);
            put (w[h - 1] == 's' && w[h - 2] != 's' ? ": you put them in your " : ": you put it in your ");
            put (kind_of (item_kind (pack))->name);
            put (".");
        }
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
    static unsigned char f, cc;

    cc = lv_c;                                  /* (Its compartment reached into: a chest's lid's top open to the hand,
                                                **   its lid shut or not; then the one c's in, the next level's) */
    lv_c = icomp (c);
    fetch (&rec, c);
    lw = rec.where;
    lx = rec.x;
    k = kind_of (rec.kind);
    f = k->flags;
    if (rec.shut & IS_SHUT_C (cc)) {
        return 1;
    }
    if (!(f & K_CONTAINER) || (f & K_HANG) || (cc && comp_of (rec.kind, cc) && crow.prep != CP_IN)) {
        return 0;
    }
    return !(rec.flags & I_WORN) ? 1 : k->form & KF_BACK ? 2 : 0;
}

/* Whether thing i is handy (HOUSE.md's handy tools: in hand, or on the body, in nothing that isn't open to the hand):
** 0 if it is, else the outermost thing it's in that isn't (a pack worn, a sack: part 26's map making's words) */
unsigned __fastcall__ s_handy (unsigned i)
{
    static unsigned c;

    lv_c = icomp (i);
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
            return;                             /* (Its levels the steps', part 85's) */
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
        lv_c = icomp (n);
        s_reach_to (rec.x);
    } else if (!(rec.flags & I_HELD)) {
        rbody = 1;
    }
}

/* What's owed, done: added to the character's next spell of time (engaged, a round a step: zone_step's), and, said, a
** pack taken off for it told ("You take off the backpack for it, and put it on again."); then none owed */
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
    if ((s = rsteps + rbody) != 0 && near_one () && zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y)) {
        hub_map (0);
        rtime = s * HUB->round;
    }
    t = rtime;
    if (!(said & RD_LONG)) {
        pextra += t;                            /* (A long task's its own length's instead: the caller's, RD_LONG) */
    }
    if (roff && (said & RD_SAID)) {
        put (" You take off the ");            /* (The world's words, SA.doff's: 2026-11-13) */
        put (kind_at (roff)->name);
        put (" for it, and put it on again.");
    }
    rclear ();
    return t;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Parts 85, 86, 87 (2026-11-02, the user's: "when you have to take off your backpack to get something from it, it
** should read "Taking off backpack", pause, "retrieving <item>", pause, then "putting your backpack back on".  Taking
** just as long as it might take when really doing it"; HOUSE.md's reach): a reach as a sequence of steps, each told as
** it begins, each its own time, each a grope engaged (a round; failed, the round wasted, the sequence over, the world
** as it stands), the world changed at each's end; stopped part way (hard, soft or the player's), the world stays as it
** got, the pack off told.  The handlers call reach_step just before they change anything, past their refusals: it
** begins the next step owed, and they answer nothing done (IR_NO, no words): the reply's IO_STEP, main.c's things' the
** UI's first words and wait (a long task of the step's kind, the world's act.work LW_REACH), the command kept and asked
** again as it's up (reach 5, 6); with none owed the handling goes on, its levels paid, as it was.  After the handling
** the pack goes back on as the character's free (rs_after_on, act.work's: the world's own step).  An intent's
** tool the same by the world alone (reach_do with its keys: lw_m asked again).  The requests that don't ask for the
** steps (IO_STEPS: a blow's weapon, a denizen's) keep the lump, reach_done's */
#pragma code-name (push, "SW85CODE")
#pragma rodata-name (push, "SW85RODATA")
#pragma register-vars (push, on)

unsigned rs_pack;                               /* (The pack off for this handling) */
unsigned char rs_owed;                          /* (A step's words owed, told as it's done: SV_AT_START's not, part 86's) */
unsigned char rs_spill (void);                  /* (Part 85's) */
void __fastcall__ rs_queue (unsigned char e);  /* (Part 85's) */
static unsigned char __fastcall__ no (unsigned char n);
struct step rstep;                              /* (The step under way, things.h's: rstep.thing too the pack on the back
                                                **   the reach comes to, rs_walk's; rstep.what the referee's grade, rs_roll's) */
struct step aside[2];                           /* (What was set down to free a hand, a redip's, a new light's: each a step
                                                **   to take it up again as the work's done, the last set down first) */
static unsigned char naside;
static unsigned char aq[4], naq;                /* (The after-work's steps planned, a queue in the order they're done, the
                                                **   next on top: AQ_*, each queued in its place as it's planned, rs_queue;
                                                **   popped by rs_after.  The tool put back and the pack put on, its first
                                                **   and last, are the reach's own state, rs_using's and rs_pack's) */
enum { AQ_UP = 1, AQ_STOW, AQ_RELIGHT };        /* (What was set down taken up, aside's next; the old torch put away; the
                                                **   torch lit again, the UI's: each above those of a lower kind, so done
                                                **   first, an AQ_UP above the AQ_UPs before it) */
#define rs_hit          lx                      /* (The level wanted, and from the innermost, how far: level_of's bytes, */
#define rs_want         lw                      /*   not in use across a call) */
unsigned char rs_ok;                            /* (This request's: 1 by the steps, the UI's; 2 the world's own; | 4, past
                                                **   a count's first thing: the rest by the lump) */
unsigned char rs_st;                            /* (A step begun: 1; 2 a grope failed; the request's reply) */
unsigned char rs_paid;                          /* (The levels reached into so far) */
unsigned char rs_after_on;                            /* (The pack to go back on as it's free: 1; 2 past a timed work's end) */
unsigned char rs_live;                          /* (A step done, its handling asked again next) */
unsigned char rs_free;                           /* (Things put away this sequence to free a hand: part 90's) */
unsigned char rs_using;                          /* (A tool's: 1 out; 2 caught, a fire-maker's; | 4 in hand before it, nothing to put away) */
#define rs_keys         iq.name                 /* (An intent's tool's keys, 20 bytes at most: the request's, none in hand as an intent's carried out) */
#define rs_hand         last_sa                 /* (The pack's hand, 0 its feet: a handling's own byte, none as it's waited on) */
#define rs_keep         lw_m.who                /* (The intent to ask again: its who, 0 as it came, the UI's) */
#define rs_old          lw_x                    /* (A redip's: the torch, to put away) */
unsigned char rs_redo;                          /* (A redip's, RR_*: under way; its words) */
enum { RR_ON = 1, RR_DIP = 32, RR_TWO = 64, RR_NEW = 128 };   /* (Its steps begun; a dipping's; two alike, told apart; a new
                                                **   light's) */

/* The containers a reach comes to, out from c (level_of's rule, its own copy: open to the hand, none; a pack on the back,
** rstep.thing; any other, or closed, a level): how many, and rs_hit the one rs_want from the innermost */
static unsigned char rs_wc;                     /* (rs_walk's: the compartment of c it comes to first, the next's as it
                                                **   goes out: a chest's lid's top open to the hand, not a level) */
static unsigned char __fastcall__ rs_walk (unsigned c)
{
    register const struct kind* k;
    unsigned char m = 0, sh, in;

    while (c) {
        fetch (&rec, c);
        k = kind_of (rec.kind);
        sh = rec.shut & IS_SHUT_C (rs_wc);
        in = (k->flags & K_CONTAINER) && !(rs_wc && comp_of (rec.kind, rs_wc) && crow.prep != CP_IN);
        rs_wc = icomp (c);
        if (!sh && in && !(k->flags & K_HANG) && (rec.flags & I_WORN) && (k->form & KF_BACK)) {
            rstep.thing = c;
        } else if (sh || (in && !(k->flags & K_HANG) && !(rec.flags & I_WORN))) {
            if (m++ == rs_want) {
                rs_hit = c;
            }
        }
        c = rec.where == IW_ITEM ? rec.x : 0;
    }
    return m;
}

/* The step begun (rstep.edge, rstep.say, rstep.what, rstep.time): engaged a round, and its grope (not the first's, of a thing's handling:
** the request's own test made already, serve's, get's): failed, the round wasted, the sequence over.  1 */
static unsigned char rs_begin (void)
{
    unsigned char s = rs_live;

    if (rstep.edge == SE_DROP) {
        goto go;                                /* (Let fall, set down: no grope, no round, the book's) */
    }
    rs_live = 0;
    if (rstep.edge != SE_TRY && (s || (rs_ok & 3) == 2 || !(iq.kind == IO_GET || (iq.kind >= IO_HOLD && iq.kind <= IO_TAKE))) &&
        fumbled (rstep.what)) {
        rs_st = 2;
        reach_do (RO_FUMBLED);
        return 1;
    }
    rs_live = s;
    hub_map (0);
    if (near_one () && zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y)) {
        hub_map (0);
        rstep.time = HUB->round;
    }
go:
    act.cut = 0;
    act.work = LW_REACH;
    act_plan (AK_WORK);
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
        if (d != (unsigned char) RS_BACK) {     /* (Its hands made free for it first: part 87's) */
            rs_tool = n;
            if (rs_more (0)) {
                return 1;
            }
        }
    }
    rstep.thing = 0;
    rs_want = 0xFF;
    if (n && item_at (n)->where == IW_ITEM) {
        up = item_at (n)->x;
    }
    rs_wc = n ? icomp (n) : 0;
    nout = rs_walk (up);
    rs_wc = rs_cc;                              /* (Where it's put: stow's compartment, 0 anywhere else) */
    nd = rs_walk (c);
    rstep.time = 50;
    if (d == (unsigned char) RS_BACK) {
        if (rs_using) {                         /* (The tool put away first, where it came from) */
            rs_using = 0;
            rs_after_on = 1;
            rstep.thing = rs_tool;
            rstep.edge = SE_BACKIN;
            hub_map (0);                        /* (Where it goes back, SE_BACKIN's: its room looked for first, the
                                                **   user's, 2026-11-18, H9; none, kept in hand, said so) */
            if ((c = HUB->rs_nout ? HUB->rs_home[HUB->rs_nout - 1] : 0) == 0 && (c = rs_pack) == 0) {
                about ();
                c = pack;
            }
            cat_map ();
            rstep.say = c && !o_fits (rs_tool, c) ? SV_PUTAWAY : SV_KEEP;
            rstep.what = item_kind (rstep.apart = rs_tool);
            rstep.time = 20;
            return rs_begin ();
        }
        rstep.edge = SE_ON;
        rstep.say = SV_ON;
        rstep.what = item_kind (rs_pack);
    } else if (rstep.thing) {                          /* (A pack on the back: off first) */
        rs_hand = d ? 0 : rs_for (up ? n : 0);
        rstep.edge = SE_OFF;
        rstep.say = rs_hand ? SV_OFF : SV_OFFDOWN;
        rstep.what = item_kind (rstep.thing);
    } else if (rs_paid < nout + nd) {           /* (Each level its own) */
        rs_want = rs_paid < nout ? nout - 1 - rs_paid : nout + nd - 1 - rs_paid;
        rs_wc = rs_paid < nout ? (n ? icomp (n) : 0) : rs_cc;
        rs_walk (rs_paid < nout ? up : c);
        rstep.edge = SE_IN;
        rstep.say = d ? SV_IN : rs_paid + 1 == nout ? SV_GETOUT : rs_paid + 1 == nout + nd ? SV_OPEN :
            SV_IN;
        rstep.what = item_kind (rstep.apart = rstep.say == SV_GETOUT ? n : rs_hit);
        rstep.time = 20;
    } else {
        if (!d) {
            return 0;
        }
        rs_tool = n;                            /* (A tool: got out, its tries if it's a fire-maker: part 87's) */
        if ((d = rs_for (d == (unsigned char) RS_FIRE ? RF_FIRE : RF_DRAW)) != 1) {
            return d;
        }
    }
    return rs_begin ();
}

/* A filling's or a dipping's end in the dark, by touch (the dark's test: "a fumble can spill oil"; handled the fuel):
** spilt, half the flask's less, told: 1 (part 85's since 2026-11-20: part 86's room) */
unsigned char s_rs_spill (void)
{
    hub_map (SIGHT_BANK);
    if (!SIGHT->dark || rs_for (RF_ROLL)) {     /* (By touch alone: in the light nothing's spilt) */
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

/* An after-work step planned (AQ_*: AQ_STOW, AQ_RELIGHT once): queued in its place, above those of a lower kind
** (part 85's since 2026-11-20: part 86's room) */
void __fastcall__ s_rs_queue (unsigned char e)
{
    register unsigned char i;

    for (i = naq; e != AQ_UP && i; ) {
        if (aq[--i] == e) {
            return;
        }
    }
    for (i = naq++; i && aq[i - 1] > e; --i) {
        aq[i] = aq[i - 1];
    }
    aq[i] = e;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW87CODE")
#pragma rodata-name (push, "SW87RODATA")
#pragma static-locals (push, off)

/* A flame to hand (IO_LIGHT's, the catalogue's bank's light): a thing lit, carried or within reach, 1; or a fire-maker
** carried (the catalogue's tool key, "tinder"), rs_tool, one not known empty first: 3, its steps the lighting's (part
** 82's); 4, it's known empty; 0 none */
extern char dig_key[9];                         /* (dq0w.c's) */
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

/* A word to the UI (E_TASK's w: TE_*, | its kind times 256; b rstep.what) */
static void __fastcall__ rs_tell (unsigned w)
{
    hub_map (0);
    wk.m.kind = E_TASK;
    wk.m.who = 0;
    wk.m.b = rstep.what;
    wk.m.w1 = w;
    wk.m.t = HUB->now;
    hub_event (&wk.m);
}

/* The sequence over, part way: the pack left off told */
static void rs_cut (void)
{
    unsigned char i;

    if (rs_pack) {
        rstep.what = item_kind (rs_pack);
        rs_tell (item_at (rs_pack)->where == IW_SQUARE ? TE_PACKED | 1 << 8 : TE_PACKED);
    }
    hub_map (0);
    for (i = HUB->rs_nout; i; --i) {            /* (The tools out stay out, in hand, the world true: the user's, 2026-11-03) */
        hub_map (0);
        rstep.what = item_kind (HUB->rs_out[i - 1]);
        rs_tell (TE_PACKED);
    }
    hub_map (0);
    HUB->rs_nout = 0;
    rs_pack = 0;
    rs_after_on = rstep.edge = rs_paid = rs_live = rs_keep = rs_using = rs_redo = rs_free = naside = naq = rs_owed = 0;
    ix_make ();                                 /* (What it carries now, the pack off at its feet: the UI's first words') */
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
        rs_using = rec.flags & I_HELD ? 5 : 1;
    }
    if (!(rec.flags & I_HELD) && (hand_free (rs_tool) & ~HF_FIXT)) {   /* (No hand to be had, an arm's: used as it is) */
        fetch (&rec, rs_tool);
        rstep.thing = rs_tool;
        rstep.edge = SE_HAND;
        rstep.say = SV_GETOUT;
        rstep.apart = rs_tool;
        rstep.what = rec.kind;
        rstep.time = 10;
        return 1;
    }
    if (!fire || (rs_using & 3) == 2) {
        return 0;
    }
    make_own (rs_tool);
    fetch (&rec, rs_tool);
    if (!rec.fuel) {
        item_at (rs_tool)->shut |= IS_SPENT;
        return 2;
    }
    rstep.edge = SE_TRY;
    rstep.say = SV_KINDLE;
    rstep.what = rec.kind;
    rstep.time = 50;
    return 1;
}

/* The referee's grope (Dexterity's; every ruleset's), 20 less in the dark: whether it went, rstep.what */
static void rs_roll (void)
{
    hub_map (SIGHT_BANK);
    rstep.what = SIGHT->dark;
    hub_map (0);
    HUB->slot[SLOT_WORLD].kind = RS_TEST;
    HUB->slot[SLOT_WORLD].actor = 0;
    RS_TEST_PURPOSE (HUB->slot[SLOT_WORLD]) = P_GROPE;
    RS_TEST_HOW (HUB->slot[SLOT_WORLD]) = 0;
    RS_TEST_STEPS (HUB->slot[SLOT_WORLD]) = rstep.what ? -4 : 0;
    hub_ask (SLOT_WORLD);
    rstep.what = G_OK (RS_TEST_GRADE (HUB->slot[SLOT_WORLD]));
    HUB->slot[SLOT_WORLD].state = REQ_FREE;
}

/* A try's end: caught, one lighting off it, told */
static void rs_tried (void)
{
    rs_roll ();
    if (rstep.what) {
        rs_using = (rs_using & 4) | 2;
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
        if ((n >= RF_PICKS && n <= RF_DIGGERS) || n == RF_DOORKEY) {  /* (An intent's tool got out first, the world's own steps, lw_m the intent
                                                **   asked again as they're done: 1 if a step's begun; 0 out, on with it,
                                                **   put away and the pack back on after it.  Its keys in the main RAM:
                                                **   strings here gone as tool_reach's part is copied in; the door's key,
                                                **   by its number: the key that fits, of those carried, 2026-11-04) */
            if (!rs_live) {
                rs_paid = rs_using = rs_free = 0;
                rs_pack = 0;
                hub_map (0);
                HUB->rs_nout = 0;
            }
            memcpy (rs_keys, n == RF_DOORKEY ? "\0\0\0\0\0\0\0\0" : n == RF_PICKS ? "picks\0\0" : dig_key, 9);
            hub_map (0);
            if (HUB->pc_lump & (PL_NOSTEPS | PL_TOLD)) {
                tool_reach (rs_keys);                /* (dq0 -J's: its time owed the next spell) */
                reach_done (0);
                return 0;
            }
            rs_ok = 2;
            rs_keep = 1;
            f = tool_reach (rs_keys);
            rs_ok = 0;
            if (!f && !strcmp (rs_keys, "chisel")) {   /* (And its hammer in the other hand: the user's, 2026-11-03) */
                memcpy (rs_keys, "hammer\0", 8);
                rs_ok = 2;
                f = tool_reach (rs_keys);
                rs_ok = 0;
            }
            if (!f) {
                rs_live = rs_keep = 0;
                if (rs_using & 4) {
                    rs_using = 0;
                }
                if (rs_pack || rs_using) {
                    rs_after_on = 1;
                    act.cut = 0;
                }
            }
            return f;
        }
        if (n == RF_ROLL) {
            rs_roll ();
            return rstep.what;
        }
        if (n == RF_TRIED) {
            rs_tried ();
        } else if (n == RF_CUT) {
            rs_cut ();
        } else {
            rs_tell ((unsigned char) n == TE_BEGUN ? TE_BEGUN | (unsigned) rstep.say << 8 : n & ~RF_TELL);   /* (A step's
                                                **   words its own, SM_* its top bits: not RF_TELL's) */
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
#pragma code-name (push, "SW90CODE")
#pragma rodata-name (push, "SW90RODATA")
#pragma static-locals (push, off)
/* ---- Hands made free (the user's, 2026-11-03: "If your hands are busy and your want to ready tools or other things, it should
** probably put the other things away first (in their appropriate place) then take the tools in hand.  A torch would be placed in a
** nearby empty sconce or laid on the ground, a sword put in its scabbard, etc."; HOUSE.md's): the next thing in the way of rs_tool in
** hand (a chisel, both hands for it and its hammer), its step begun: 1; or 0, its hand free, or none to be had.  Of two in the way, the
** one whose thing has a home first, the light last (the user's, 2026-11-17: "the hand whose thing has a home goes first (a sword to
** its scabbard, coins to the purse), the light last"; the rule of 2026-11-03 kept, only the order chosen): home_of's rank, the item
** store's order between two alike.  What goes where is the house's: a flame in hand into an empty sconce within reach, else laid on
** the ground lit; what's kept worn, slung (a shield); coins the purse (s_purse's); a carrier its family's on the body; else the pack,
** by the steps of the reach; else set down.  Never what's being readied: rs_tool, the tools out already (a tinderbox), the thing
** they're for (handled, a request's by the steps: the torch to light, the rope tied as its hammer's readied), a chisel's
** hammer or a hammer's chisel; nor, engaged (in an enemy's zone: the reach's own test), a weapon in hand, a shield too (the
** user's, 2026-11-17: "A drawn weapon keeps its hand in a fight; outside one, the home-first order stands"); the pack off
** for this in
** a hand set down, never slung to come off again; no more than RS_FREE of them a sequence, so nothing can go round for ever: then no
** hand, the handler's words (the soak, 2026-11-04).  A light set down burning, or in a sconce, left_lit: taken up again as a move
** away starts (s_lit_back's) */
#define RS_FREE         6
unsigned left_lit;                              /* (The light the hands' rule set down burning, or in a sconce: 0 none) */
static unsigned char rd, rt, rn;                /* (home_of's: the step's edge, its words, its time; */
static unsigned rc, ri;                         /*   the place it goes, a sconce, the purse, a carrier; the pack) */

/* Where held thing o goes to free its hand (rd, rt, rn, rc, ri), w 0 but its rank: 0 a home (a scabbard, a sheath, the purse,
** slung, the pack; the pack off for this, set down), 1 none (set down at its feet), 2 a light burning (its place not looked for,
** w 0).  The pack and the belt about's */
static unsigned char __fastcall__ home_of (unsigned o, unsigned char w)
{
    unsigned i;
    const struct kind* k;
    unsigned char lit;

    rc = 0;
    rn = 20;
    fetch (&rec, o);
    k = kind_of (rec.kind);
    if ((lit = rec.flags & I_LIT) != 0 && !w) {
        return 2;
    }
    if (lit && !(k->flags & K_HANG)) {
        for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {
            if (ip->kind == KI_SCONCE && in_reach (i)) {
                cat_map ();
                if (!o_fits (o, i)) {
                    rc = i;
                    break;
                }
            }
        }
        rd = rc ? SE_PLACE : SE_DROP;
        rt = rc ? SV_SCONCE : SV_LAYLIT;
    } else if ((k->flags & K_WEAR) && o != rs_pack) {
        rd = SE_SLING;
        rt = SV_SLING;
        rn = 50;
    } else {
        if ((k->form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) {
            rc = purse (o);                     /* (Coins: the purse) */
        }
        for (i = 0; !rc && (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
            k = kind_of (ip->kind);
            if ((k->flags & K_HANG) && k->slots && i != o && on_pc (i)) {
                cat_map ();
                if (!o_fits (o, i)) {
                    rc = i;
                }
            }
        }
        if (rc) {
            rd = SE_PLACE;
            rt = SV_SHEATHE;
        } else {
            i = pack ? pack : rs_pack;          /* (Worn, or off for this at its feet) */
            cat_map ();
            if (i && i != o && !o_fits (o, i)) {
                rd = SE_STOW;
                rt = SV_INPACK;
                ri = i;
            } else {
                rd = SE_DROP;
                rt = SV_SETDOWN;
                rn = 10;
            }
        }
    }
    return lit ? 2 : rd == SE_DROP && o != rs_pack;
}

static unsigned char s_room (void)
{
    unsigned i = 0, o = 0, o1, o2;
    unsigned char t, r, best = 3, pair, eng;

    if (item_at (rs_tool)->flags & I_HELD) {
        return 0;
    }
    if (hand_free (rs_tool) & ~HF_FIXT) {
        if (strcmp (kind_at (rs_tool)->tool, "chisel")) {
            return 0;
        }
    } else {
        hub_map (0);
        if (HUB->pcsum.limbs & (LB_ARM_L | LB_ARM_R)) {
            return 0;                           /* (An arm useless: the hand's refusal, or used as it is) */
        }
    }
    if (rs_free >= RS_FREE) {
        return 0;
    }
    pair = !strcmp (kind_at (rs_tool)->tool, "chisel") || !strcmp (kind_at (rs_tool)->tool, "hammer");
    about ();                                   /* (The pack, the belt: home_of's) */
    hub_map (0);
    o1 = HUB->rs_nout ? HUB->rs_out[0] : 0;
    o2 = HUB->rs_nout > 1 ? HUB->rs_out[1] : 0;
    eng = near_one () && zone_step (HUB->pc_x, HUB->pc_y, HUB->pc_x, HUB->pc_y);   /* (Engaged: its weapons kept) */
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if ((ip->flags & I_HELD) && i != rs_tool && i != o1 && i != o2 && (rs_ok != 1 || i != handled) &&
            !(pair && (!strcmp (kind_at (i)->tool, "chisel") || !strcmp (kind_at (i)->tool, "hammer"))) &&
            !(eng && kind_at (i)->craft == AP_WEAPON) && (r = home_of (i, 0)) < best) {
            best = r;                           /* (The one with a home first, the light last: the first of the best) */
            o = i;
        }
    }
    if (!o) {
        return 0;
    }
    home_of (o, 1);                             /* (Its place, its words: the chosen one's) */
    if (rd == SE_STOW && reach_step (o, ri)) {
        return 1;                               /* (The pack off first, its level) */
    }
    if (best == 2) {
        left_lit = o;                           /* (A light set down burning: taken up again as a walk starts) */
    }
    t = rt;
    if (o != rs_pack && ++rs_free > 1) {        /* (The other hand's after it: its words not the first's again) */
        t = t == SV_LAYLIT ? SV_LAYLIT2 : t == SV_INPACK ? SV_INPACK2 : t == SV_SETDOWN ? SV_LETFALL : t;
    }
    pair = rc || rd == SE_STOW ? item_kind (rc ? rc : ri) : 0;   /* (Where it goes, its kind: the step's words', the UI's, "in your
                                                **   backpack", "in the sconce": one wording, the world's, 2026-11-13) */
    rstep.what = item_kind (rstep.apart = o);
    hub_map (0);
    HUB->rs_place = rc;
    HUB->ireq.mat = pair;
    rstep.thing = o;
    rs_hand = 0;
    rstep.edge = rd;
    rstep.say = t;
    rstep.time = rn;
    reach_step (0, RS_GO);
    return 1;
}

/* A light the hands' rule set down burning (left_lit: laid at the character's feet, set in a sconce), as a move away starts (the
** user's, 2026-11-17: "a light left burning is picked up when a walk starts, if a hand is free"; and "Any move away takes up a light
** the rule left burning: a walk, a climb, going up or down"; dq0w.c's intent, go its kind): any move's (a walk, I_GO, I_GOTO,
** I_STEP, I_TAKE, I_TRAVEL; a climb, up or down, I_CLIMB: a stairway, a ladder, a ledge, a pit, a rope, a shaft; a close-in,
** I_CLOSE; a disengage or a flight, I_DISENGAGE; a gathering, I_GATHER), before it, taken up again into a free hand, a second's
** work, told ("You take up the lantern."), none free, kept for the move's start; | LB_GO, the move begun, left behind, told ("You
** leave the lantern behind."); gone from where it was set, out, or in hand again, nothing.  Forgotten but for a move not begun, no
** hand free.  The hub's header in the window after */
static const unsigned char lb_moves[4] = {      /* (The moves, I_* by its bit: I_GO ... I_GATHER, all under 32) */
    1 << I_GO | 1 << I_CLIMB, 1 << (I_GOTO - 8) | 1 << (I_STEP - 8), 1 << (I_TAKE - 16) | 1 << (I_CLOSE - 16) |
    1 << (I_DISENGAGE - 16), 1 << (I_TRAVEL - 24) | 1 << (I_GATHER - 24)
};
typedef char lb_moves_fit[I_GO < 8 && I_CLIMB < 8 && I_GOTO >= 8 && I_STEP < 16 && I_TAKE >= 16 && I_DISENGAGE < 24 &&
    I_TRAVEL >= 24 && I_GATHER < 32 ? 1 : -1];  /* (hub.h's I_* as the table has them) */
void __fastcall__ s_lit_back (unsigned char go)
{
    struct msg m;
    unsigned n = left_lit;
    unsigned char h = 0, k = go & ~LB_GO, sc;

    if (k >= 32 || !(lb_moves[k >> 3] >> (k & 7) & 1)) {
        goto out;                               /* (Not a move: as it was) */
    }
    go &= LB_GO;
    fetch (&rec, n);
    k = rec.kind;
    if (!k || (rec.flags & (I_LIT | I_HELD)) != I_LIT) {
        goto gone;
    }
    if ((sc = rec.where == IW_ITEM) != 0) {
        if (item_at (rec.x)->kind != KI_SCONCE || (!go && !in_reach (rec.x))) {
            goto gone;
        }
    } else {
        hub_map (0);
        if (rec.where != IW_SQUARE || (!go && (rec.sn != HUB->pc_stratum || rec.x != HUB->pc_x || rec.y != HUB->pc_y))) {
            goto gone;                          /* (Where it was set: at its feet, in a sconce within reach; the walk
                                                **   begun, it may be a step on) */
        }
    }
    if (!go) {
        if (!(h = hand_free (n) & ~HF_FIXT)) {
            goto out;                           /* (No hand free: left behind if the walk begins) */
        }
        fetch (&rec, n);
        rec.where = IW_ACTOR;
        rec.x = rec.y = 0;
        rec.sn = h;
        rec.flags = (rec.flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD;
        store (&rec, n);
        light_now ();                           /* (Its light: what it holds lit) */
        if (sc) {
            sconce_lights (1);                  /* (Out of its sconce: the room's light) */
        }
        pextra += 10;                           /* (A second's: the walk's first spell the longer) */
    }
    hub_map (0);
    m.kind = E_TASK;
    m.who = 0;
    m.b = k;
    m.w1 = TE_BEGUN | (unsigned) (h ? SV_TAKEUP : SV_LEAVE) << 8;    /* (A step's words, the UI's TASK.step) */
    m.t = HUB->now;
    hub_event (&m);
gone:
    left_lit = 0;
out:
    hub_map (0);
}

/* A step's end of these (rstep.edge, rstep.thing): the tool in hand, kept to go back; the tool back where it came from, or the pack; put in its
** place; slung.  (Part 75's: part 90's overlay's room, 2026-11-04) */
#pragma code-name (push, "SW75CODE")
#pragma rodata-name (push, "SW75RODATA")
void s_rs_did2 (void)
{
    unsigned n = rstep.thing, c;
    unsigned char h, d = rstep.edge;

    fetch (&rec, n);
    if (d == SE_HAND) {
        if (!(h = hand_free (n) & ~HF_FIXT)) {
            return;
        }
        fetch (&rec, n);
        hub_map (0);
        if (HUB->rs_nout < 2) {
            HUB->rs_home[HUB->rs_nout] = rec.where == IW_ITEM ? rec.x : 0;
            HUB->rs_out[HUB->rs_nout++] = n;
        }
        rec.where = IW_ACTOR;
        rec.x = 0;
        rec.sn = h;
        rec.flags = (rec.flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD;
    } else if (d == SE_BACKIN) {
        hub_map (0);
        c = 0;
        if (HUB->rs_nout) {
            c = HUB->rs_home[--HUB->rs_nout];
            if (HUB->rs_nout) {
                rs_tool = HUB->rs_out[HUB->rs_nout - 1];
                rs_using = 1;                   /* (Its fellow next: a hammer and a chisel) */
            }
        }
        if (!c) {
            c = rs_pack;
        }
        if (!c) {
            about ();
            c = pack;
        }
        cat_map ();
        if (!c || !(rec.flags & I_HELD) || o_fits (n, c)) {
            return;                             /* (No place for it: it stays in its hand) */
        }
        fetch (&rec, n);
        rec.where = IW_ITEM;
        rec.x = c;
        rec.y = 0;                              /* (Its first compartment) */
        rec.flags &= ~(I_WORN | I_HELD);
    } else if (d == SE_PLACE) {
        hub_map (0);
        c = HUB->rs_place;
        rec.where = IW_ITEM;
        rec.x = c;
        rec.y = 0;                              /* (Its first compartment) */
        rec.flags &= ~(I_WORN | I_HELD);
    } else {
        rec.where = IW_ACTOR;
        rec.x = 0;
        rec.sn = 0;
        rec.flags = (rec.flags & ~I_HELD) | I_WORN;
    }
    store (&rec, n);
    if (d == SE_PLACE) {
        sconce_lights (1);
        if (kind_at (n)->form & KF_COUNT) {
            join (n);                           /* (Coins in the purse: with those there, part 35's) */
        }
    }
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A thing sharpened with a whetstone (the user's, 2026-11-02, "like sharpening your sword (if you have a whetstone)"; the
** book has none: the house's; a tool's use, part 84's; handled the thing): a metal weapon, the whetstone carried, wear on
** it: its steps (IR_NO); begun, 0xFE (a minute's timed work); done (IO_DONE), half its wear gone, told */
static unsigned char __fastcall__ rs_no2 (unsigned n, unsigned char sa)
{
    cat_map ();
    name_of (n, 1);
    return no (sa);
}

static unsigned char rs_hone (void)
{
    unsigned n = handled, i = 0;

    fetch (&rec, n);
    if (kind_at (n)->craft != AP_WEAPON || !(rec.mat == MT_IRON || rec.mat == MT_STEEL || rec.mat == MT_BRONZE)) {
        return rs_no2 (n, SA_NO_FILL);
    }
    if (iq.to & IO_DONE) {
        rec.wear >>= 1;
        store (&rec, n);
        equip_now ();                           /* (The referee told: its edge's its blow's) */
        cat_map ();
        name_of (n, 1);
        return yes (SA_HONED, 0);
    }
    if (iq.into[0] && (i = at_hand (iq.into)) != 0 && strcmp (kind_at (i)->tool, "whet")) {
        return rs_no2 (n, SA_NO_STONE);         /* (Sharpened with something else named: that's no whetstone, the one
                                                **   used or none: 2026-11-03) */
    }
    i = 0;
    if (!rec.wear) {
        return rs_no2 (n, SA_SHARP);
    }
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (!strcmp (kind_at (i)->tool, "whet") && on_pc (i)) {
            if (reach_step (i, RS_TOOL)) {
                return IR_NO;
            }
            handled = n;                        /* (What's sharpened, the tool's search over) */
            return 0xFE;
        }
    }
    return rs_no2 (n, SA_NO_STONE);
}
unsigned char __fastcall__ s_rs_more (unsigned char w)
{
    if (w == 2) {
        return rs_hone ();
    }
    return s_room ();
}
#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW86CODE")
#pragma rodata-name (push, "SW86RODATA")

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
    register struct step* sp;

    rstep.thing = i;
    rstep.edge = SE_DROP;
    rs_hand = 0;
    rstep.say = SV_SETDOWN;
    rstep.what = item_kind (rstep.apart = i);
    if (rstep.what == item_kind (rs_old)) {
        rs_redo |= RR_TWO;
        rstep.say = SV_SETDOWN | SM_NEW;
    }
    rstep.time = 1;
    rs_redo |= RR_ON;
    if (naside < 2) {
        rs_queue (AQ_UP);                       /* (A third the second's place: none more to take up) */
    }
    if (naside) {                               /* (Taken up again after the work, the last set down first: a step on
                                                **   the stack aside, its words the other's if it's the first's kind) */
        if (rstep.what == item_kind (aside[0].thing)) {
            rstep.say = SV_DOWN2;               /* (The other of two alike: its words not the first's again, nor the
                                                **   first's as it's taken up) */
            aside[0].say = SV_TAKEUP | SM_OTHER;
        }
        naside = 1;
    }
    sp = &aside[naside++];
    sp->edge = SE_UP;
    sp->say = SV_TAKEUP;
    sp->thing = i;
    sp->time = 10;
    return rs_go ();
}

/* The other thing in a hand, not n: 0 none */
static unsigned __fastcall__ rs_held (unsigned n)
{
    unsigned i = 0;

    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        if ((ip->flags & I_HELD) && i != n && !(rs_using && i == rs_tool)) {   /* (The tool out for it, its fuel, not set down) */
            return i;
        }
    }
    return 0;
}

/* A spare (sp) lit from a burning one (old) of its kind, the next step of it, a redip's and a new light's alike (the user's,
** 2026-11-03: "light the new torch from the old one before it is extinguished"): the other hand's thing set down if it
** wants the hand; got out; lit, 3 seconds, in a hand.  IR_NO, a step begun */
static unsigned char __fastcall__ rs_lit_from (unsigned sp, unsigned old)
{
    unsigned i;
    unsigned char h = item_at (sp)->flags & I_HELD ? item_at (sp)->sn : 0;   /* (In its other hand already: lit there) */

    if (!h && !hand_free (sp) && (i = rs_held (old)) != 0) {
        return rs_set_down (i);
    }
    rs_redo |= RR_TWO;
    if (!h && reach_step (sp, 0)) {
        if (rstep.say == SV_GETOUT) {
            rstep.say |= SM_NEW;
        }
        return IR_NO;
    }
    rs_hand = h ? h : hand_free (sp) & ~HF_FIXT;
    rstep.thing = sp;
    rstep.edge = SE_LIGHT;
    rstep.say = SV_LIGHT | SM_NEW;
    rstep.what = item_kind (old);
    rstep.time = 30;
    rs_redo |= RR_ON;
    return rs_go ();
}

/* A new light (rs_tool) with the old one (rs_old) burning in a hand (HOUSE.md's a new light; light()'s, the catalogue's bank's,
** each ask as a step's up): lit from it, then the old one blown out, a second, then let fall, no time; what was set down
** taken up and the pack put back as any handling's end (rs_after).  IR_NO, a step begun; IR_OK, all done, its words
** the steps' */
static unsigned char rs_newl (void)
{
    if (!(item_at (rs_tool)->flags & I_LIT)) {
        return rs_lit_from (rs_tool, rs_old);
    }
    fetch (&rec, rs_old);
    if (!(rec.flags & I_HELD)) {
        rs_redo &= ~RR_NEW;
        return IR_OK;
    }
    rstep.thing = rs_old;
    rstep.what = rec.kind;
    rs_redo |= RR_ON | RR_TWO;
    if (rec.flags & I_LIT) {
        rstep.edge = SE_OUT;
        rstep.say = SV_BLOWOUT | SM_OLD;
        rstep.time = 10;
    } else {
        rstep.edge = SE_DROP;
        rs_hand = 0;
        rstep.say = SV_LETFALL | SM_OLD;
        rstep.time = 1;
        item_at (rs_tool)->sn = rec.sn;         /* (The new one in the old one's hand as it falls: a sword set down goes
                                                **   back in its own) */
    }
    return rs_go ();
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
    rs_old = n;                                 /* (The one dipped, kept to its end: each step asks again) */
    rs_redo |= RR_DIP;
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
            } else if (ip->kind == kn && (!sp || (ip->flags & I_HELD)) && (!ip->id || ip->fuel) && on_pc (i)) {
                sp = i;                         /* (One in the other hand first: none got out and that one set down) */
            }
        }
        if (!ot && !s && sp) {                  /* (The preferred order: a spare lit from it first) */
            rs_queue (AQ_STOW);
            return rs_lit_from (sp, n);
        }
        rstep.thing = n;                               /* (Put out: by another light, or to the dark, asked first) */
        rstep.edge = SE_OUT;
        rstep.what = kn;
        rstep.time = 10;
        rstep.say = SV_BLOWOUT;
        rs_redo |= RR_ON;
        if (ot) {
            rs_queue (AQ_STOW);
            if (item_kind (ot) == kn) {         /* (Two of a kind: each step about the one it means) */
                rs_redo |= RR_TWO;
                rstep.say = SV_BLOWOUT | SM_OLD;
            }
        } else if (!s) {
            rstep.say = SV_BLOWDARK;
            rs_queue (AQ_RELIGHT);
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

/* A step's end (rstep.edge, rstep.thing): what it changes changed: a pack, a thing let fall; a redip's: put out, lit from the old,
** put away, taken up again */
static void rs_did (void)
{
    unsigned char h;
    unsigned n = rstep.edge == SE_ON ? rs_pack : rstep.thing;

    den_saw = 1;                                /* (What lies about changed: the senses' look of it, the things taken all from) */
    if (rstep.edge >= SE_HAND) {
        rs_did2 ();
        return;
    }
    if (rstep.edge == SE_OFF) {
        rs_pack = rstep.thing;
    }
    fetch (&rec, n);
    if (rstep.edge == SE_OUT) {
        rec.flags &= ~I_LIT;
        den_saw = 1;
    } else if (rstep.edge == SE_LIGHT) {
        make_own (n);
        fetch (&rec, n);
        rec.where = IW_ACTOR;
        rec.x = 0;
        rec.flags = (rec.flags & ~(I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD | I_LIT;
        rec.sn = rs_hand;
    } else if (rstep.edge == SE_STOW) {
        about ();
        if (!pack) {
            pack = rs_pack;                     /* (Off for this, at its feet or in a hand) */
        }
        fetch (&rec, n);                        /* (Back in the pack; no room, or none, it stays in its hand) */
        cat_map ();
        if (pack && !o_fits (n, pack)) {
            fetch (&rec, n);
            rec.where = IW_ITEM;
            rec.x = pack;
            rec.y = 0;
            rec.flags &= ~(I_WORN | I_HELD);
        } else {
            fetch (&rec, n);
        }
    } else if (rstep.edge == SE_UP) {
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
        if (rstep.edge == SE_ON) {
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

/* What's next as a dipping's over, the fuel put away, the after-work's queue popped: the torch to light again (2: rs_after_on
** 2, the UI's IO_LIGHT, then here again); the old one put away, if it's in a hand and the pack's off for it, else on; what
** was set down taken up (1: the step set, RS_GO's); 0 no more */
static unsigned char rs_after (void)
{
    while (naq) {
        if (aq[--naq] == AQ_RELIGHT) {
            rs_after_on = 2;
            return 2;
        }
        if (aq[naq] == AQ_STOW) {
            fetch (&rec, rs_old);
            if (rec.kind && (rec.flags & I_HELD) && rs_pack) {   /* (The pack off for it already; else it stays in its hand) */
                rstep.thing = rs_old;
                rstep.edge = SE_STOW;
                rstep.say = rs_redo & RR_TWO ? SV_PUTAWAY | SM_OLD : SV_PUTAWAY;
                rstep.apart = rs_old;
                rstep.what = rec.kind;
                rstep.time = 20;
                return 1;
            }
            continue;
        }
        rstep = aside[--naside];                /* (AQ_UP: the next aside popped, the last set down first, the burning
                                                **   torch before a sword) */
        rstep.what = item_kind (rstep.apart = rstep.thing);
        if (rstep.say == SV_TAKEUP && (rs_redo & RR_TWO) && rstep.what == item_kind (rs_old)) {
            rstep.say |= SM_NEW;                /* (The new one's kind, the old one's: told apart) */
        }
        return 1;
    }
    rs_redo = 0;
    return 0;
}

#pragma static-locals (pop)

/* What the reach's steps want of this part, by w: RO_*, or an intent's tool's keys.  RO_ASK a request by the steps taken
** up; RO_REPLY its reply made (1: a step's, no more); RO_END a step's time up, or the character free after its handling
** (act.work's): stopped first, the sequence over, told; a step's change made, 0 (the UI's to ask again), or 2 (an intent
** asked again); its handling done, the pack back on, the world's own step, 2; a timed work's end still to come, or the
** pack not where it was (a denizen's taken it), 0.  RO_GO the world's own step begun (its intent kept: lw_m); RO_FUMBLED a
** step's grope failed.  Keys (an intent's tool got out first, a dig's, a door's lock's): 1 if a step's begun; 0 got out,
** the pack back on, on with it */
unsigned char __fastcall__ s_reach_do (unsigned w)
{
    hub_map (0);
    switch ((unsigned char) w) {
    case RO_ASK:
        rs_ok = HUB->pc_lump & (PL_NOSTEPS | PL_TOLD) ? 8 : 1;   /* (8: asked by the steps, the lump's) */
        HUB->ireq.kind &= ~IO_STEPS;
        if (rs_after_on == 2) {
            rs_after_on = 0;                          /* (A timed work's end, the torch lit again: its sequence going on) */
        } else if (!rs_live) {
            rs_paid = rs_using = rs_redo = rs_free = naside = naq = 0;
            rs_pack = 0;
            HUB->rs_nout = 0;
        }
        return 0;
    case RO_REDIP:
        return rs_ok == 1 ? rs_redip (handled) : IR_OK;
    case RO_NEWL:
        return rs_newl ();
    case RO_SPILL:
        return rs_ok == 1 ? rs_spill () : 0;
    case RO_HONE:
        w = rs_more (2);
        hub_map (0);
        HUB->ireq.x = HUB->ireq.qty = 0;
        if ((unsigned char) w != 0xFE) {
            return w;
        }
        HUB->ireq.x = 600;
        HUB->ireq.qty = SV_HONE;
        w = item_kind (handled);                /* (The item's bank in the window: then the hub's again) */
        hub_map (0);
        HUB->ireq.item = w;
        return IR_OK;
    case RO_REPLY:
        if ((rs_using & 4) && !rs_st) {
            rs_using = 0;                        /* (In hand before: none to put away; kept through its steps, the
                                                **   tinderbox in hand lit with not "put away" at their end: the user's
                                                **   playtest, 2026-11-06) */
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
            w = rstep.apart ? tell_apart (rstep.apart) : AW_NONE;    /* (Its thing told apart: "You put the spent torch in your pack.") */
            rstep.apart = 0;
            hub_map (0);
            HUB->ireq.plural = w;
            HUB->ireq.y = (rstep.say == SV_BLOWDARK) | (rstep.time == HUB->round ? 2 : 0);    /* (Its question put, the UI's; 2, engaged:
                                                **   the round's action, no long task for a blow to stop) */
            HUB->pc_ready = HUB->now + rstep.time;    /* (Not free till the UI's wait comes: engaged, act.fight's, its end not looked at first) */
            HUB->ireq.x = rstep.time;
            HUB->ireq.qty = rstep.say;
            HUB->ireq.item = rstep.what;
            rs_st = rs_ok = 0;
            rs_owed = rstep.say && !SV_AT_START (rstep.say);  /* (Its words the UI's as it's done: the world's at its end, below) */
            return 1;
        }
        if (rs_pack && !rs_using && (HUB->pc_lump & PL_MORE) && iq.kind == IO_GET && iq.result == IR_OK) {
            rs_ok = 0;                          /* (More on this line to come, "take all": the pack left off, rs_live) */
            return 0;
        }
        if (HUB->ireq.qty == LT_DIP && (rs_redo & RR_TWO)) {
            HUB->ireq.qty = SV_DIP | SM_OLD;   /* (The old torch's: "You set to dipping the old torch.") */
        }
        if (rs_pack || rs_using || naq) {   /* (The handling done: the pack back on as it's free, past a timed
                                                **   work's end if that's what's begun) */
            rs_after_on = (iq.kind == IO_SHUT || iq.kind == IO_FILL || iq.kind == IO_HIDE) && iq.result == IR_OK && HUB->ireq.qty ?
                2 : 1;
            act.cut = 0;
            act.work = LW_REACH;
            act_plan (AK_WORK);
            if (rs_after_on == 1 && (iq.kind == IO_SHUT || iq.kind == IO_FILL || iq.kind == IO_HIDE)) {
                HUB->ireq.y = naq && aq[naq - 1] == AQ_RELIGHT ? 0xFE : 0xFD;   /* (A timed work's end: more to come, the UI to wait
                                                **   for it; and the torch to light again, words.c's worked) */
            }
        }
        rs_ok = rs_live = 0;
        return 0;
    case RO_FUMBLED:
        rstep.time = HUB->round;
        rstep.say = 0;
        if ((rs_ok & 3) == 2) {
            rs_for (RF_TELL | TE_FUMBLE);
            hub_map (0);
            HUB->pc_ready = HUB->now + rstep.time;
            rs_st = 0;
        }
        rs_for (RF_CUT);
        return 0;
    case RO_GO:
        if (SV_AT_START (rstep.say)) {
            rs_for (RF_TELL | TE_BEGUN);         /* (A work set to, a question: its words as it begins) */
        } else {
            rs_owed = rstep.say != 0;           /* (The rest told as they're done: RO_END's) */
        }
        wk.m.kind = I_WAIT;
        I_WAIT_HOW (wk.m) = 0;
        I_WAIT_TENTHS (wk.m) = rstep.time;
        I_WAIT_BUSY (wk.m) = rstep.time == HUB->round ? I_WAIT_W2 (0, 0, 1) : I_WAIT_W2 (rstep.say & ~SM_ALL, rstep.what, 1);   /* (Engaged, the round's action; its
                                                **   long task the verb, its words' word the TE_BEGUN's alone) */
        intent_few (&wk.m);
        return 0;
    }
    if (act.cut) {                               /* (RO_END) */
        if (HUB->pc_task.kind != LT_FIGHT) {
            rs_for (RF_CUT);
            return 0;
        }
        act.cut = 0;                             /* (A fight begun as it came free, its line a step in hand, stopped: that the stop, not the step) */
    }
    if (rs_owed) {
        rs_owed = 0;
        rs_for (RF_TELL | TE_BEGUN);             /* (The step's words as it's done, not before: a stop part way
                                                **   leaves it undone and untold, "You stop taking off the backpack.", the
                                                **   pack still on; the game tells nothing that didn't happen, reach1's,
                                                **   2026-11-20) */
    }
    w = 0;
    if (rstep.edge != SE_IN && rstep.edge != SE_TRY && rstep.edge) {   /* (What it changes changed) */
        w = rstep.edge == SE_ON;
        rs_did ();
        rstep.edge = 0;
        light_now ();                           /* (What's handy with it) */
        equip_now ();                           /* (The referee told what's in its hands) */
    }
    if (rs_after_on) {                                /* (The character free, its handling done) */
        if (rs_after_on == 2) {
            return 0;
        }
        if (rs_using) {
            if (item_kind (rs_tool)) {
                goto back;                      /* (The tool put away first, the pack after it) */
            }
            rs_using = 0;                       /* (Used up: the last ration, the last bandage; none to put away) */
            hub_map (0);
        }
        if (rs_redo || naq) {
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
    if (rstep.edge == SE_IN) {
        ++rs_paid;
    } else if (rstep.edge == SE_TRY) {
        rs_for (RF_TRIED);
    }
    rstep.edge = 0;
    hub_map (0);
    HUB->pc_task.kind = LT_NONE;
    if (rs_keep) {
        rs_keep = 0;
        return 3;                               /* (Its intent asked again: work_done's, the parts' stack shallower) */
    }
    return den_saw ? 2 : 0;                     /* (The senses' look of what's changed first, the main loop's: then free, the next line's
                                                **   look at what lies about true) */
}

#pragma register-vars (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 92 (part 10's till 2026-11-03: every call a handling makes out to another part comes back here, and part 10
** all its 3,600 bytes each time: the user's D17): a request of the things served */
#pragma code-name (push, "SW92CODE")
#pragma rodata-name (push, "SW92RODATA")

/* (Part 92's own: none owed, serve's) */
static void rclear10 (void)
{
    rtime = roff = 0;
    rsteps = rbody = 0;
}


#pragma static-locals (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The tools: the keys of all the character carries (anywhere on it: a skill used, the one that suits takes
** itself out), into the hub's things_text, each after its mark (held: < in the left hand, > the right, * both; + worn;
** a space neither: what's held fights, what's worn guards) and its quality after it (a digit: rs.h's RS_EQUIP), and its load (the weight of all it
** carries, worn or not, in quarter pounds: the referee's encumbrance) in the request's x, the hands that hold
** something (DH_*, slice 4.1b's) in y; and one used, worn (the
** referee's choice, by its key), its name back in the request (bare hands: "hands") */
void s_tools (void)
{
    static const char hmark[4] = " <>*";
    char m[4];
    const struct kind* k;
    unsigned i = 0;
    char* t;
    unsigned char mh, hh = 0, cm;

    m[1] = m[3] = 0;
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
        if (k->tool[0] && on_pc (i)) {
            hub_map (UI_BANK);
            t = UIREC->things_text;
            if (strlen (t) < sizeof UIREC->things_text - 11) {
                m[0] = rec.flags & I_HELD ? hmark[rec.sn & IH_BOTH ? rec.sn & IH_BOTH : mh] : rec.flags & I_WORN ? '+' :
                    ' ';
                strcat (t, m);
                strcat (t, k->tool);
                m[0] = '0' + (rec.kind == KI_BANDAGES ? CL_GRADE (rec.mat ? rec.mat : k->mat, rec.wear) : rec.quality);
                                                /* (A bandage's its grade, its cloth's by its cleanliness: HOUSE.md's
                                                **   bandages, 2026-11-10; its cloth after it, R hemp, S linen, T wool,
                                                **   U silk: the referee's dressing's, the user's, 2026-11-11) */
                if (rec.kind == KI_BANDAGES) {
                    cm = rec.mat ? rec.mat : k->mat;
                    m[1] = cm >= MT_LINEN ? 'S' + cm - MT_LINEN : 'R';
                    m[2] = 0;
                }
                if (k->craft == AP_WEAPON) {
                    m[1] = rec.size == WV_HEAVY ? 'H' : rec.size ? 'L' : 'M';    /* (Its version: D13's, RS_EQUIP's) */
                    m[2] = "NEEEDDDD"[rec.wear >> 5];   /* (Its edge: dulled, dull: 2026-11-03) */
                }
                strcat (t, m);
                m[1] = 0;
            }
        }
    }
    i = s_weight_in (IW_ACTOR, 0);
    hub_map (0);
    HUB->ireq.x = i;
    HUB->ireq.y = hh;
}

#pragma code-name (push, "SW92CODE")
#pragma rodata-name (push, "SW92RODATA")

/* What's in the character's hands (IO_HANDS: a blow with nothing named, the UI's), their kinds in the request's x, its
** main hand's (or a thing's in both), and y, its other's (0 none: held_kinds') */

#pragma code-name (push, "SW102CODE")         /* (Part 102's, 2026-11-06: overlay 5's room, part 92's image's) */
#pragma rodata-name (push, "SW102RODATA")
/* ---- A thing told apart (the user's playtest, 2026-11-06: "You put the torch in your backpack." of the spent one as a
** new one's lit): its state's word (AW_*), the thing in rec: a light lit, burnt out, or neither; a fuel empty */
static unsigned char aw_of (void)
{
    unsigned char e = rec.id && !rec.fuel;

    if (kind_of (rec.kind)->flags & K_LIGHT) {
        return rec.flags & I_LIT ? AW_LIT : e ? AW_SPENT : AW_UNLIT;
    }
    return e && (unsigned char) (rec.kind - KI_OIL) <= KI_PITCH - KI_OIL ? AW_EMPTY : AW_NONE;
}

/* Thing n's word, if another of its kind is at hand (carried, or lying within reach) and none of those is in its state:
** "the spent torch" beside a lit one (HOUSE.md's things told apart: the things' words, the reach's steps); else AW_NONE */
unsigned char __fastcall__ s_tell_apart (unsigned n)
{
    struct item sv;                             /* (rec kept for the caller: the catalogue's bank's name_of's) */
    const struct item* p;
    unsigned i, j, x, y;
    unsigned char a, k, sn, b = AW_NONE;

    memcpy (&sv, &rec, sizeof sv);
    fetch (&rec, n);
    k = rec.kind;
    if (!k || !(a = aw_of ())) {
        goto out;
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    p = 0;
    for (i = 1; i < ihigh; ++i) {               /* (The store gone through as the chooser does: a kind passed over, some
                                                **   70 cycles) */
        if (!p || p == (const struct item*) 0x8000 + 0x2000 / sizeof (struct item)) {
            p = item_at (i);
        }
        if ((p++)->kind != k || i == n) {
            continue;
        }
        p = 0;
        for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {
        }
        fetch (&rec, j);
        if (!on_pc (i) && (rec.where != IW_SQUARE || rec.sn != sn || rec.x - x + 1 > 2 || rec.y - y + 1 > 2)) {
            continue;                           /* (Not carried, not within reach) */
        }
        fetch (&rec, i);
        if ((rec.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN) {
            continue;
        }
        if (aw_of () == a) {
            b = AW_NONE;                        /* (One alike: its state tells them not apart) */
            break;
        }
        b = a;
    }
out:
    memcpy (&rec, &sv, sizeof sv);
    return b;
}

/* Thing i's name into d (16 bytes), told apart by its state if it fits: the catalogue's bank's name_of's */
void __fastcall__ s_name_apart (unsigned i, char* d)
{
    static const char* const aw[AW_EMPTY + 1] = { "", "lit ", "unlit ", "spent ", "empty " };
    const char* w = aw[s_tell_apart (i)];
    const char* k = kind_of (item_kind (i))->name;

    strcpy (d, strlen (k) + strlen (w) < 16 ? w : "");
    strcat (d, k);
}

/* What the character carries, for the UI's first words (the UI bank's ix, struct cix's: the store's order), made as what
** it carries changes (light_now's, serve's) */
#pragma bss-name (push, "SW102RODATA")        /* (Part 102's image's, wanted only as it's made: the main RAM's room) */
unsigned ixi[NIX];                              /* (Each one's record, */
unsigned ixd[NIX];                              /*   its key in the letters, its id or its record's | LT_REC, */
struct cix ixc[NIX];                            /*   its words for the UI: letter.s's too) */
static unsigned char ixkk[NIXK];
static unsigned char ixqq[NIX];                 /* (And how many each is: a lot's count, else 1) */
#pragma bss-name (pop)

/* Each thing it carries its letter (the user's designators, 2026-11-06: hub.h's low): its own, kept while it's carried
** and given back as it's taken up again; one that's had none, of a ... z the first free, else the one whose thing it
** let go of longest ago (that one's given up), and only then of aa ... az the same way; by its id, by its record's
** number till it has one.  In the store's order: those that come together, the kit, in its order (letter.s's, in
** assembly: part 102's room) */
extern unsigned char ixl_n;
extern unsigned ixl_t;
extern struct cix* ixl_old;
void __fastcall__ ix_letters (unsigned* low);

void s_ix_make (void)
{
    const struct kind* k;
    unsigned i = 0;
    unsigned char n = 0, m, j, f;

    while (n < NIX && (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (on_pc (i)) {
            ixi[n++] = i;
        }
    }
    for (m = 0; m < n; ++m) {
        fetch (&rec, ixi[m]);
        ixd[m] = rec.id ? rec.id : ixi[m] | LT_REC;
        k = kind_of (rec.kind);
        ixqq[m] = (k->form & KF_COUNT) && rec.qty ? rec.qty : 1;
        f = (rec.flags & I_HELD ? CX_HELD : 0) | (rec.flags & I_WORN ? CX_WORN : 0) | (rec.flags & I_LIT ? CX_LIT : 0) |
            (rec.shut & IS_CLOSED ? CX_SHUT : 0) | ((k->form & KF_BACK) && (rec.flags & I_WORN) ? CX_BACK : 0);
        j = rec.id && !rec.fuel;                /* (Its state as the chooser's unlike has it, 2026-11-07: a container or
                                                **   a surface empty as nothing's in it, below; a fuel burnt, empty and
                                                **   spent; a light burnt out, lit or not, spent: the UI's state said
                                                **   chooses as the world does, its choice made final, sway.c's bind) */
        if ((k->flags & K_CONTAINER) || (k->keeps & KF_ON)) {
            f |= CX_FILLS | CX_EMPTY;
        } else if ((unsigned char) (rec.kind - KI_OIL) <= KI_PITCH - KI_OIL) {
            f |= j ? CX_FILLS | CX_EMPTY | CX_SPENT : CX_FILLS;
        } else if (j && (k->flags & K_LIGHT)) {
            f |= CX_SPENT;
        }
        ixc[m].flags = f;
        ixc[m].kind = rec.kind;
        ixc[m].up = 0;
        if (rec.where == IW_ITEM) {
            for (j = 0; j < n && ixi[j] != rec.x; ++j) {
            }
            ixc[m].up = j < n ? j + 1 : 0xFF;
        }
    }
    for (m = 0; m < n; ++m) {
        if ((j = ixc[m].up) != 0 && j != 0xFF && !ic_of (ixi[m])) {
            ixc[j - 1].flags &= ~CX_EMPTY;      /* (Something in it: in its first compartment, a chest's inside, not on
                                                **   its lid, the chooser's unlike's, 2026-11-16) */
        }
    }
    for (m = j = 0; m < n && j < NIXK; ++m) {   /* (Its kinds, each once) */
        for (f = 0; f < j && ixkk[f] != ixc[m].kind; ++f) {
        }
        if (f == j) {
            ixkk[j++] = ixc[m].kind;
        }
    }
    for (m = 0; m < n; ++m) {                   /* (Each by its kind's place among them) */
        for (f = 0; f < j && ixkk[f] != ixc[m].kind; ++f) {
        }
        ixc[m].kind = f < j ? f : 0xFF;
    }
    hub_map (0);
    ixl_t = (unsigned) (HUB->now >> 10);        /* (The clock's stamp: some 100 seconds a step) */
    ixl_n = n;
    ixl_old = UIREC->ix;
    hub_map (UI_BANK);
    ix_letters (UIREC->low);
    UIREC->ixn = n;
    UIREC->ixk = j;
    memcpy (UIREC->ix, ixc, n * sizeof (struct cix));
    memcpy (UIREC->ixq, ixqq, n);
    for (f = 0; f < j; ++f) {
        k = kind_of (ixkk[f]);                  /* (Copied out: the UI's bank put back for it) */
        hub_map (UI_BANK);
        UIREC->ixks[f].kind = ixkk[f];
        UIREC->ixks[f].flags = (k->flags & ~CK_CLOSES) | (k->keeps & KF_CLOSES ? CK_CLOSES : 0);
        memcpy (UIREC->ixks[f].name, k->name, sizeof k->name);
    }
    hub_map (0);                                /* (The hub's header back in the window: rs_cut's caller's events) */
}

#pragma rodata-name (pop)
#pragma code-name (pop)

/* The character's light: the brightest lit thing it carries (in a hand, worn, hung from its belt: one stowed is
** put out), into the hub, named (its minutes left the lights' burning's: part 27's); and its map making with it; what
** it carries, for the UI's first words (ix_make) */
void s_light_now (void)
{
    char lamp[14];
    const struct kind* k;
    const struct item* p;
    unsigned i = 0, j;
    unsigned char best = 0;

    lamp[0] = 0;
    while ((i = items (i, IN_FLAGS | IN_ACTOR | IN_ITEM | IN_PORTAL, I_LIT)) != 0) {
        fetch (&rec, i);
        for (j = i; (p = item_at (j))->where == IW_ITEM; j = p->x) {    /* (Up to whoever has it: part 10's on_pc, no swap) */
        }
        if (j == rs_pack || (p->where == IW_ACTOR && !p->x)) {
            k = kind_of (rec.kind);
            if (k->light > best) {
                best = k->light;
                strcpy (lamp, k->name);
            }
        }
    }
    ix_make ();
    hub_map (0);
    HUB->pc_light = best;
    strcpy (HUB->pc_lamp, lamp);
    if (HUB->tpr) {
        chart_late |= 1;                        /* (Its map making: the main loop's at the real paces, a command's answer
                                                **   first, some 230,000 cycles through what it carries each thing
                                                **   handled) */
    } else {
        chart_now ();
    }
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
        for (c = n; item_at (c)->where == IW_ITEM && !in_shut (c); c = item_at (c)->x) {
        }                                       /* (Up through each compartment it's in that's open: a chest's lid's top,
                                                **   its lid shut or not, 2026-11-16) */
        p = item_at (c);
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

/* A light named that's lit in a hand, none named unlit in a hand: it (HOUSE.md's which is lit, "the one in a hand first,
** whichever way it's lit", none put away for another: "light the torch" of the one in hand, lit already, said so, not a
** spare got out of the pack; 2026-11-07, dq0sweep's); else 0 */
static unsigned __fastcall__ held_lit (void)
{
    unsigned n, lit = 0;
    unsigned char k, f;

    for (n = 1; n < NITEMS; ++n) {
        k = item_at (n)->kind;
        f = item_at (n)->flags;
        if (k && (f & I_HELD) && item_at (n)->where == IW_ACTOR && !item_at (n)->x && (kind_flags (k) & K_LIGHT) &&
            named (kind_of (k)->name, iq.name)) {
            if (!(f & I_LIT)) {
                return 0;
            }
            lit = n;
        }
    }
    return lit;
}

static unsigned char o_handle (void);
unsigned char drag (void);                      /* (Part 109's: wdrag.c, IO_DRAG) */
unsigned char __fastcall__ handout (const struct msg* m);  /* (Part 111's: wempty.c, IO_EMPTY, IO_PLACE) */
unsigned char s_serve (void)
{
    unsigned char changed = 0, k;
    unsigned n;

    rclear10 ();                                /* (No reach owed yet) */
    hub_map (0);
    if (HUB->ireq.kind & IO_STEPS) {            /* (By the reach's steps, the UI's: part 86's RO_ASK, here, its copies
                                                **   a tenth of a thing's answer, 2026-11-03) */
        rs_ok = HUB->pc_lump & (PL_NOSTEPS | PL_TOLD) ? 8 : 1;   /* (8: asked by the steps, the lump's) */
        HUB->ireq.kind &= ~IO_STEPS;
        if (rs_after_on == 2) {
            rs_after_on = 0;                    /* (A timed work's end, the torch lit again: its sequence on) */
        } else if (!rs_live) {
            rs_paid = rs_using = rs_redo = rs_free = naside = naq = 0;
            rs_pack = 0;
        }
    }
    memcpy (&iq, &HUB->ireq, sizeof iq);
    handled = 0;                                /* (This request's own, as it's found: an earlier one's never kept from
                                                **   the hands' rule, a falchion drawn before a bandage got out, the play
                                                **   test's, 2026-11-17) */
    if (HUB->pc_hid) {
        hid_op (HO_THING);                      /* (A thing handled: out of hiding, part 94's) */
    }
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
        } else if (iq.result == IR_NONE && iq.kind == IO_GET && (n = carried_named (iq.name)) != 0 &&
            (item_at (n)->flags & (I_HELD | I_WORN))) {
            iq.kind = item_at (n)->flags & I_HELD ? IO_HOLD : IO_DON;   /* (None lying about, but in hand or worn: "You
                                                **   hold the sack in your left hand already.", "You wear the backpack
                                                **   already.", the hold's and the wear's words, not "none here":
                                                **   2026-11-07, dq0sweep's) */
            iq.to = 0;
            cat_map ();
            iq.result = o_handle ();
            iq.kind = IO_GET;
            hub_map (0);
        }
        break;
    case IO_SHUT:                               /* (A lid, a tie, a stopper, a lock worked: part 4's) */
        slid = 0;
        iq.result = shut_work ();
        changed = slid;                         /* (What was on its lid slid off: the look's) */
        break;
    case IO_INSIDE:                             /* (What's in it, or on it: part 11's) */
        iq.result = inside ();
        break;
    case IO_THROW:
        iq.result = throw_at (iq.name);
        break;
    case IO_LIST:
        inventory ();
        break;
    case IO_HANDS:
        bar_hand ();                            /* (Part 4's: part 10's room) */
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
    case IO_DRAG:                               /* (A heavy thing shoved, dragged; a thing tasted: part 109's, wdrag.c) */
        changed = (iq.result = drag ()) == IR_OK && iq.to != DG_TASTE;
        break;
    case IO_EMPTY:                              /* (A thing's things put out, a thing put down on a square: part 111's, */
    case IO_PLACE:                              /*   wempty.c, every actor's: P4, P5, 2026-11-18) */
        changed = (iq.result = handout (0)) == IR_OK;
        break;
    case IO_USE:                                /* (A thing used up by its key: a bandage, first aid's, part 70's; by the
                                                **   steps, got out for its use, a tool's, part 85's: IR_NO a step first) */
        iq.result = rs_ok == 1 ? (tool_reach (iq.name) ? IR_NO : IR_OK) : rs_ok ? IR_OK : use_key (iq.name);
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
        act.cut = 1;
        rs_for (RF_CUT);
        break;
    case IO_RIP:                                /* (A cloth torn up for bandages, washed: part 107's, wcloth.c) */
    case IO_WASH:
        iq.result = textile ();
        break;
    case IO_PUT:
        if (!strcmp (iq.into, "well") || !strcmp (iq.into, "shaft")) {   /* (Into a well, down a shaft or a hole: part
                                                **   104's, read in first: 2026-11-06; its words the trtext's, the
                                                **   things' text begun empty, not the last answer's said again) */
            text_new ();
            look_more (0, LM_ROPES);
            iq.result = well_put ();
            break;
        }
    case IO_DOFF:                               /* (Worn in two places or more, by its word: which, asked by place, part
                                                **   107's, 2026-11-10) */
        if (iq.kind == IO_DOFF && (iq.result = worn_which ()) != 0) {
            break;
        }
    default:                                    /* (A thing handled: IO_HOLD ...) */
        afar = 0;
        k = iq.kind;
        if (!noinit && k >= IO_HOLD && k <= IO_DOFF && (n = at_hand (iq.name), hand_n = n, hand_sel = wsel, n) != 0 &&
            (k <= IO_TAKE || (kind_at (n)->form & KF_BACK)) && fumbled (item_kind (n))) {
            hub_map (0);                        /* (Held, stowed, put or taken out engaged, a grope failed: the round
                                                **   wasted, nothing done, its words) */
            took = HUB->round;
        } else if (k >= IO_STOW && k <= IO_TAKE) {
            if ((iq.result = moved (k)) == IR_OK && k != IO_TAKE) {
                lv_c = cont_c;
                reach_to (cont);                /* (Stowed, put, taken from: one, or as many as it says; what it went
                                                **   in reached into, once) */
            }
        } else if (k == IO_DOFF && (n = hand_n ? hand_n : carried_named (iq.name)) != 0 && (item_at (n)->flags & I_WORN) &&
            !(hand_free (n) & ~HF_FIXT)) {
            iq.result = doff_feet (n);          /* (Taken off, no hand free: at its feet, not refused, nor the pack off for
                                                **   it; a place said, "put the cloak in the sack", the put's) */
        } else if (k == IO_LIGHT && wsel != SL_NEW && *iq.name && *iq.name != '#' && !*iq.into && (n = held_lit ()) != 0) {
            text_new ();                        /* ("The torch is lit already.": held_lit's) */
            strcpy (xn, kind_of (item_at (n)->kind)->name);
            cat_map ();
            iq.result = no (SA_LIT);
        } else {
            cat_map ();
            if ((iq.result = o_handle ()) == IR_NO && k == IO_DOFF && last_sa == SA_HANDS && !rs_st) {
                iq.result = doff_feet (handled);    /* (No hand free, no room in the pack: at its feet, as a drop) */
            }
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
    if (iq.result != IR_NOT_HERE && (iq.kind == IO_KIT || iq.kind == IO_GET || iq.kind == IO_DROP || iq.kind == IO_HIDE ||
        iq.kind == IO_THROW || iq.kind == IO_PLACE || iq.kind == IO_EMPTY || (iq.kind >= IO_HOLD && iq.kind <= IO_NAME))) {   /* (Out of reach: nothing's moved yet) */
        s_light_now ();                         /* (Its light: what it holds lit) */
        changed |= iq.result == IR_OK;
    } else if (iq.result == IR_OK && (iq.kind == IO_FILL || iq.kind == IO_SHUT || iq.kind == IO_USE)) {
        ix_make ();                             /* (A light filled, a thing closed, one used up: the UI's first words') */
    }
    if (iq.result == IR_OK && (iq.kind == IO_GET || (iq.kind >= IO_HOLD && iq.kind <= IO_DOUSE))) {
        hub_map (0);
        if (HUB->tpr) {
            chart_late |= 2;                    /* (The main loop's at the real paces: its answer first) */
        } else {
            sconce_lights (1);                  /* (A sconce's torch taken, put in, lit, put out: its room's light) */
        }
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
    if (iq.result == IR_NOT_HERE && afar == 2) {   /* (Down in a pit, from its edge: said so, no walk over to it, the
                                                **   user's, 2026-11-18, H10: climbing down reaches it) */
        text_new ();
        cat_map ();                             /* (The catalogue's bank's words) */
        name_of (far_n, 1);
        iq.result = no (SA_IN_PIT);
    }
    rclear10 ();
    hand_n = 0;
    hub_map (0);
    HUB->ireq.result = iq.result;
    if (rs_ok && reach_do (RO_REPLY)) {         /* (The reach's steps: part 83's, a step begun the reply's) */
        return 0;
    }
    return changed && iq.kind != IO_KIT;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
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
unsigned char ui_long (void);                   /* (dq0w.c's) */
void s_chart_now (void)
{
    static const char keys[5][6] = { "sheet", "map", "draw", "quill", "ink" };
    const struct kind* k;
    unsigned i = 0, j, sheet = 0, map = 0, w[5];
    unsigned char has = 0, busy, mh, h, f, pk = 0;

    hub_map (0);
    mh = HUB->pcsum.limbs & LB_LEFT ? IH_LEFT : IH_RIGHT;
    busy = (HUB->pcsum.limbs & LB_ARM_L ? IH_LEFT : 0) | (HUB->pcsum.limbs & LB_ARM_R ? IH_RIGHT : 0);
    memset (w, 0, sizeof w);
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (ui_long ()) {
            chart_late |= 1;                    /* (A command or a line being typed, its answer first: all of it again
                                                **   after it, the main loop's, a quarter of a million cycles, 2026-11-04) */
            return;
        }
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
        w[f] = i;                               /* (Each handy, by its key: what it draws with, used up as it draws,
                                                **   dq0w.c's drawn_down) */
        if (!f && !sheet) {
            sheet = i;
        }
    }
    memcpy (chart_w, w, sizeof w);
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
    HUB->pc_chart = f ? CH_DRAW : pk ? CH_PACKED : 0;
    HUB->pc_chartk = f ? 0 : pk;
    HUB->pc_map = i;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 4 (store.c's, its room): a thing thrown */
#pragma code-name (push, "SW88CODE")           /* (Part 4 full, 2026-11-03) */
#pragma rodata-name (push, "SW88RODATA")

/* A thing thrown (the UI's IO_THROW, the character's: the same for every actor, as their minds come to throw), the
** one named that it carries ("": the first in its hands), at a mark (the request's x, y: a square it sees, a denizen's
** or a thing's), how (the request's to: struct cmd's manner, MN_HARD), as the plan has it (anything throwable thrown
** by the same code): a throwable thing's reach (the catalogue's throws: the short band's squares, kinds.c's) and
** HarnMaster's missile ranges (Combat 15: short, medium, long and extreme, each twice the last, at 0, 20, 40 and 80
** less; thrown hard, twice as far, and 10 less), the referee's Throwing test (P_THROW, the band's steps): it lands on
** the mark, hit or missed (a miss falls there, by it), or short of it, beyond the extreme band (along the line to it:
** squares it sees); out of the thrower's hands.  IR_OK (how it went in the request's qty, TH_*; where it lies in x,
** y), IR_NONE (none carried), IR_NO (not a thing to throw).  name 0, body bd's (2026-11-04: FE_THROW, wtake.c's): thing
** tk_x, at the character; the referee's test by its sheet; its value TH_* (its reach's own, in hand or not: a denizen's
** reach no steps, the lump's), or IR_NO's TH_SHORT, not a thing to throw, left held */
extern unsigned tk_x;
unsigned char __fastcall__ s_throw_at (const char* name)
{
    static unsigned char reach, band, d, sn, who;
    static unsigned px, py, mx, my, lx, ly, qx, qy;
    static int dx, dy, ex, ey;
    static signed char steps, g;
    struct req* r;
    unsigned n = 0;

    who = name ? 0 : bk + 1;
    if (who) {
        n = tk_x;
    } else if (*name) {
        n = carried_named (name);
    } else {
        while ((n = items (n, IN_FLAGS | IN_ACTOR, I_HELD)) != 0 && !on_pc (n)) {
        }
    }
    if (!n) {
        return IR_NONE;
    }
    if (!who) {
        reach_from (n);                         /* (Got out first: its reach, HOUSE.md's) */
    }
    fetch (&rec, n);
    cat_map ();
    reach = throws[rec.kind < KI_COUNT ? rec.kind : 0];
    if (!reach) {
        return who ? TH_SHORT : IR_NO;
    }
    if (!who && reach_step (n, 0)) {
        return IR_NO;
    }
    hub_map (0);
    if (who) {
        sn = bd.sn;
        px = bd.x;
        py = bd.y;
        mx = bpx;
        my = bpy;
    } else {
        sn = HUB->pc_stratum;
        px = mx = HUB->pc_x;
        py = my = HUB->pc_y;
        /* (Stopped short on its line by rock, a wall, a door shut, glass: it falls on the last square it gets to,
        ** ln_pass's, a missile's; the mark the UI's squares along a way, "throw the dagger north", the verbs plan's
        ** B1, 2026-11-10; a mark it sees as before, its line clear) */
        dx = iq.x - px;
        dy = iq.y - py;
        lx = dx < 0 ? -dx : dx;
        ly = dy < 0 ? -dy : dy;
        d = lx > ly ? (lx > 0xFF ? 0xFF : lx) : (ly > 0xFF ? 0xFF : ly);
        for (ex = ey = 0, band = d; band; --band) {
            qx = mx;
            qy = my;
            if ((ex += lx) * 2 >= d) {
                ex -= d;
                qx += dx < 0 ? -1 : 1;
            }
            if ((ey += ly) * 2 >= d) {
                ey -= d;
                qy += dy < 0 ? -1 : 1;
            }
            if (!ln_pass (sn, qx, qy, LN_FIRE)) {
                break;
            }
            mx = qx;
            my = qy;
        }
        hub_map (0);
    }
    dx = mx - px;
    dy = my - py;
    d = (dx < 0 ? -dx : dx) > (dy < 0 ? -dy : dy) ? (dx < 0 ? -dx : dx) : (dy < 0 ? -dy : dy);
    steps = 0;
    if (!who && (iq.to & MN_HARD)) {
        reach <<= 1;
        steps = -2;
    }
    for (band = 0; band < 4 && d > (reach << band); ++band) {
    }
    g = TH_SHORT;
    if (band == 4) {                            /* (Too far: it falls at the extreme band's end, on the line) */
        mx = px + dx * (reach << 3) / d;
        my = py + dy * (reach << 3) / d;
    } else {
        r = &HUB->slot[SLOT_WORLD];
        r->kind = RS_TEST;
        r->actor = who;
        RS_TEST_PURPOSE (*r) = P_THROW;
        RS_TEST_HOW (*r) = 0;
        RS_TEST_STEPS (*r) = steps - 4 * band;            /* (20 less a band: 4 steps of 5) */
        r = hub_ask (SLOT_WORLD);
        g = RS_TEST_GRADE (*r);
        r->state = REQ_FREE;
        g = G_OK (g) ? TH_HIT : TH_MISS;
    }
    item_at (n)->where = IW_SQUARE;             /* (Out of its hands: where it falls) */
    item_at (n)->sn = sn;
    item_at (n)->x = mx;
    item_at (n)->y = my;
    item_at (n)->owner = item_at (n)->hide = 0;
    item_at (n)->flags &= ~(I_HELD | I_WORN);
    hub_map (0);
    if (who) {
        return g;
    }
    dropped = n;
    HUB->ireq.qty = g;
    HUB->ireq.x = mx;
    HUB->ireq.y = my;
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
static unsigned char look_q;                    /* (A look's asking: s_look_pick's) */

/* Thing i (in rec, its kind k) not in the state SL_EMPTY's three say: empty (a container or a surface with nothing in or
** on it, a fuel burnt), spent (a light or a fuel burnt out: the user's "drop the spent torch", 2026-11-06), full (the
** rest of those): 1 */
static unsigned char __fastcall__ unlike (unsigned i, const struct kind* k)
{
    unsigned char e = rec.id && !rec.fuel;
    unsigned j = 0;

    if ((k->flags & K_CONTAINER) || (k->keeps & KF_ON)) {
        while ((j = items (j, IN_X | IN_ITEM, i)) != 0 && ic_of (j)) {
        }
        e = !j;                                 /* (Nothing in its first compartment: a chest's inside, not its lid's
                                                **   top, 2026-11-16) */
    } else if ((unsigned char) (rec.kind - KI_OIL) > KI_PITCH - KI_OIL && !(wsel == SL_SPENT && (k->flags & K_LIGHT))) {
        return 1;                               /* (Neither a container nor a fuel, nor a light said spent) */
    }
    return wsel == SL_FULL ? e : !e;
}

static unsigned __fastcall__ pick_ (const char* name, unsigned char how, unsigned char* no);

/* Compartment c of thing h (its row in crow) hidden and not found yet (CS_UNSEEN's: the chooser's, twice, part 4's
** room) */
static unsigned char __fastcall__ unseen (unsigned h, unsigned char c)
{
    return CS_UNSEEN (crow.noun, item_at (h)->fuel, c);
}
#pragma static-locals (push, off)
unsigned __fastcall__ s_pick (const char* name, unsigned char how)
{
    unsigned char no[NK];                       /* (The kinds this name's no name of: asked once a kind, a world's every
                                                **   thing passed over at a look, the user's D17: a command that names a
                                                **   thing answers at once; on the C stack, the main RAM's room) */

    memset (no, 0, NK);
    return pick_ (name, how, no);
}
#pragma static-locals (pop)

#pragma bss-name (push, "SW4RODATA")            /* (pick_'s locals, static: part 4's own, none of its calls swapping a
                                                **   part in; the main RAM's room, 2026-11-18) */
static unsigned __fastcall__ pick_ (const char* name, unsigned char how, unsigned char* no)
{
    struct { unsigned n; unsigned char r; } c[NCAND];
    unsigned char* t;
    const struct item* p;
    const struct kind* k;
    unsigned i = 0, x, y, px, py, dx, dy, wid;
    unsigned char m = 0, j, r, d, sn, s = wsel & 15, pw, ps, pk, all = wsel == SL_WHICH;

    if (all) {
        s = 0;                                  /* (Every one: which's, part 35's) */
    }
    afar = 0;
    csum = 0;
    hand_n = 0;
    wcomp = 0;
    if (wforce && !wnot) {
        return wforce;                          /* (A count's next, chosen already: a lot's part split off) */
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    for (wid = 0, t = (unsigned char*) name + 1; *name == '#' && *t; ++t) {
        wid = wid * 10 + *t - '0';            /* (A thing by its id: a name given's, a letter's, the UI's "#" and the id;
                                                **   the records' own looked at, none copied out, 2026-11-06) */
    }
    p = 0;                                      /* (The store gone through here, not by items (): a kind passed over, or
                                                **   a free record, some 70 cycles, not 1,400: a thing named answers the
                                                **   sooner, the user's D17, 2026-11-03; p 0, its bank to be put back) */
    for (i = 1; i < ihigh; ++i) {
        if (!p || p == (const struct item*) 0x8000 + 0x2000 / sizeof (struct item)) {
            p = item_at (i);
        }
        pk = p->kind;
        ++p;
        if (!pk || no[pk] || p[-1].where >= IW_DEN || (p[-1].flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN ||
            (*name == '#' && p[-1].id != wid)) {
            continue;
        }
        p = 0;
        if (*name != '#') {                     /* (A kind's name asked where it lies, once a kind: nothing copied out,
                                                **   a thing named its first letter the sooner, the user's D17) */
            hub_map (KIND_BANK);
            if (*name ? !named ((const char*) kinds + ((unsigned) pk << 5) + ((unsigned) pk << 2) + pk, name) : how != HW_IN) {
                                                /* (The main RAM's named, the one rule, no swap; "" names none here, but
                                                **   any in a thing: "take all from the case", 2026-11-17) */
                if (!wcn || !*name || !(j = comp_named (pk, name))) {
                    no[pk] = 1;
                    continue;
                }
                if (unseen (i, j)) {
                    continue;                   /* (Named by a compartment of its, "the drawer": the user's, 2026-11-16;
                                                **   a hidden one not found, none) */
                }
            }
        }
        fetch (&rec, i);
        if (d = chain (i), cnot && s != SL_NTH) {
            continue;                           /* (Not the first, nor in it, unless it's the nth said: "the second
                                                **   sack" of them all, not of the rest) */
        }
        p = item_at (ctop);
        pw = p->where;
        ps = p->sn;
        pk = p->kind;
        px = p->x;
        py = p->y;
        p = 0;
        if (ctop == rs_pack) {
            pw = IW_ACTOR;                      /* (Off for a handling, at its feet: part 85's) */
            px = 0;
        }
        if (how == HW_IN) {
            if (rec.where != IW_ITEM || rec.x != win || (wcin && ic_of (i) != wcin) ||
                (!*name && (j = icomp (i)) != 0 && unseen (win, j))) {
                continue;                       /* (In it; in the compartment named, wcin's; any, "", not in a hidden
                                                **   one not found yet: "take all from the case", a found one's too, the
                                                **   user's, 2026-11-17) */
            }
            r = 0;
        } else if (rec.where == IW_ITEM && rec.x == win) {
            continue;                           /* (In the one a count's put in already) */
        } else if (pw == IW_ACTOR) {
            if (px || how == HW_SEEN || how == HW_GONE) {
                continue;                       /* (Another's: a denizen's) */
            }
            r = d ? 1 + (d > 6 ? 6 : d) : rec.flags & I_HELD ? 0 : 1;
            if (iq.kind == IO_DON ? rec.flags & I_WORN : iq.kind == IO_DOFF && !(rec.flags & I_WORN)) {
                r = 7;                          /* (To wear, one not worn first; to take off, one worn: the second
                                                **   ring, 2026-11-10) */
            }
        } else {
            if (how == HW_CARRIED || d > 1 || ps != sn || (dx = px - x + 20) > 40 || (dy = py - y + 20) > 40 ||
                (d && !(rec.y >= IC_MARK ? IC_PREP (rec.y) == CP_ON : kind_of (pk)->keeps & KF_ON) &&   /* (On a
                                                **   chest's lid too, its compartment's prep: lying's, 2026-11-16) */
                (how != HW_HAND || !(rec.flags & I_KNOWN) || !(kind_of (pk)->flags & K_CONTAINER) || in_shut (i))) ||
                                                /* (To hand, in an open container set down too, one it knows is in it:
                                                **   the user's reach, 2026-10-23, "a backpack on the ground ... is more
                                                **   readily available"; DECISIONS.md's "in a container set down":
                                                **   dq0disarm's gambeson in the pack it dropped, hear2's) */
                !(t = at (sn, px, py)) || (!(*t & T_VIS) &&
                (how < HW_HAND || !(*t & T_SEEN) || !rec.id || room_of (sn, px, py) != room_of (sn, x, y)))) {
                continue;                       /* (Lying about, or on a surface there, in sight, or remembered there
                                                **   in the room it's in, as HW_HAND and those after it ask; not in a
                                                **   thing, but to hand in an open one it knows of) */
            }
            dx = dx > 20 ? dx - 20 : 20 - dx;
            dy = dy > 20 ? dy - 20 : 20 - dy;
            r = 8 + (dx > dy ? dx : dy);
            if ((px != x || py != y) && (t = at (sn, px, py)) != 0 && TILE (*t) == T_PIT) {
                r = R_PIT;                      /* (Down in a pit: reached from inside it alone) */
            }
        }
        k = kind_of (rec.kind);
        if (s == SL_LIT ? !(rec.flags & I_LIT) : wsel == SL_NEW ? rec.flags & (I_HELD | I_LIT) : s == SL_UNLIT ? (rec.flags &
            I_LIT) || !(k->flags & K_LIGHT) :
            s == SL_OPEN ? !(k->keeps & KF_CLOSES) || (rec.shut & IS_CLOSED) : s == SL_SHUT ? !(rec.shut & IS_CLOSED) :
            s == SL_EMPTY && unlike (i, k)) {
            continue;                           /* (Not in the state said: "the lit torch", "the empty sack", "the spent
                                                **   torch") */
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
        if (!r && look_q && !s) {
            break;                              /* (A look at what's in its hand: none's more to hand, nor a look's count
                                                **   to make of them: the rest of the store not gone through, the user's
                                                **   D17, 2026-11-03) */
        }
    }
    look_q = 0;
    if (all) {
        for (j = 0; j < m; ++j) {               /* (Their numbers, the most to hand first, in tbuf: which's) */
            ((unsigned*) tbuf)[j] = c[j].n;
        }
        return m;
    }
    j = s == SL_NTH ? wsel >> 4 : s == SL_OTHER && !wnot ? 2 : 1;
    if (!j || j > m) {
        return 0;
    }
    i = c[j - 1].n;
    r = c[j - 1].r;
    if (wcn && *name != '#') {
        wcomp = name_comp (item_at (i)->kind, name);   /* (Named by a compartment of its: which, for the verb) */
    }
    seen_d = r < 8 ? 0 : r - 8;
    if (how == HW_HAND && r >= 10) {            /* (Seen, but out of reach: where it lies, the UI's to walk to; down in a
                                                **   pit, s_serve's words, no walk) */
        afar = r == R_PIT ? 2 : 1;
        far_n = i;
        chain (i);
        p = item_at (ctop);
        far_x = p->x;
        far_y = p->y;
        return 0;
    }
    return i;
}
#pragma bss-name (pop)

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
    if (hand_n && wsel == hand_sel && !wnot && !win && !wforce && !strcmp (name, iq.name)) {
        name = (const char*) hand_n;            /* (What serve asked, just now, the same thing asked again as the handling
                                                **   begins: no second pass of the store, the user's D17) */
        hand_n = 0;
        return (unsigned) name;
    }
    return *name ? s_pick (name, HW_HAND) : 0;
}

/* The thing a look's asked of (IO_LOOK: the request's to, LM_*): carried, or seen: the one most to hand, of several; all
** but a plain look, remembered too, walked to; gone to: lying about; or the holder of a compartment named by its own name
** ("look at the drawer": wcomp, the user's, 2026-11-17).  The request read for part 39's s_look_on */
unsigned __fastcall__ s_look_pick (void)
{
    unsigned n;

    hub_map (0);
    memcpy (&iq, &HUB->ireq, sizeof iq);
    wsel = iq.sel;
    wnot = win = wforce = 0;
    look_q = wcn = 1;
    n = s_pick (iq.name, iq.to == LM_AT ? HW_ANY : iq.to == LM_GO ? HW_GONE : HW_KNOWN);
    wcn = 0;
    return n;
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

    if (!*keys) {                               /* (The door's key: the one that fits, part 80's) */
        if ((i = tool_of (0, KI_KEY, 0)) != 0) {   /* (The work's door's key: wport.c's) */
            reach_from (i);
            return reach_step (i, RS_TOOL);
        }
        return 0;
    }
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        for (k = keys; *k; k += strlen (k) + 1) {
            if (!strcmp (kind_at (i)->tool, k) && on_pc (i)) {
                if (*k == 'w' && item_at (i)->where == IW_ITEM) {
                    i = item_at (i)->x;         /* (Water: what it's in, the waterskin) */
                }
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
    unsigned char h;

    for (i = n; item_at (i)->where == IW_ITEM; i = item_at (i)->x) {
    }                                           /* (On a surface: the surface's square) */
    fetch (&rec, i);
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    h = iq.count || (HUB->pc_lump & (PL_MORE | PL_ALL)) ? 0 : 2;   /* (In a hand, the pack with both full; by the count, "take all", the
                                                **   pack first: the user's, 2026-11-06) */
    HUB->ireq.x = rec.x;                        /* (Where it is: IR_NOT_HERE's, IR_FIXED's; and which, I_TAKE's) */
    HUB->ireq.y = rec.y;
    HUB->ireq.item = n;
    if (rec.x - x + 1 > 2 || rec.y - y + 1 > 2) {
        return IR_NOT_HERE;
    }
    if ((rec.x != x || rec.y != y) && TILE (*at (rec.sn, rec.x, rec.y)) == T_PIT) {
        afar = 2;                               /* (Down in a pit, from its edge: s_serve's words, no walk; H10) */
        far_n = n;
        return IR_NOT_HERE;
    }
    fetch (&rec, n);
    k = kind_of (rec.kind);
    if (k->form & KF_COUNT) {
        h = 0;                                  /* (A lot counted, coins, rations, arrows: the pack first, with others alike) */
    }
    rbody = 1;                                  /* (Its reach: lying within reach, a grope engaged, HOUSE.md's) */
    return rec.where == IW_PORTAL ? IR_FIXED : k->keeps & KF_FIXED ? IR_NO :
        !noinit && fumbled (rec.kind) ? IR_FUMBLED : carry (n, h) ? IR_OK : rs_st ? IR_NO : IR_FULL;  /* (A portal's part: lifted
                                                **   out first, the world's; a theme's altar, never: its words the
                                                **   handling's; engaged, a grope failed: the round wasted) */
}

/* The closed thing that thing n is in (or in what that's in: a sack tied shut in an open pack), or 0 (none: it's to
** hand: part 35's, the catalogue's bank's code's through its stub) */
/* (Part 106's, where the packer put it to make part 35 fit: there by its pragma now, part 35 its headroom kept,
** 2026-11-18) */
#pragma code-name (push, "SW106CODE")
#pragma rodata-name (push, "SW106RODATA")
unsigned __fastcall__ s_shut_in (unsigned n)
{
    unsigned c;

    while (item_at (n)->where == IW_ITEM) {
        c = item_at (n)->x;
        if (in_shut (n)) {
            return c;                           /* (Its compartment closed: a chest's inside, not its lid's top) */
        }
        n = c;
    }
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A door's bar to hand (carried, or within reach: not one in another's brackets), the widest: its width (took's,
** the request's x) and IR_NO (nothing about it changed); or (iq's to 1) it set in the door's brackets, a portal's part
** again on iq's square, the bar's side (the world's barred the door): part 35's, the catalogue's bank's room (its
** o_handle's through its stub) */
#pragma code-name (push, "SW105CODE")           /* (Part 105's since 2026-11-06: part 4's room, the letters'; part 4's since
                                                **   2026-11-03, part 35's room) */
#pragma rodata-name (push, "SW105RODATA")
unsigned char s_bar_hand (void)
{
    unsigned n, best;

    if (iq.kind == IO_HANDS) {                  /* (And what's in its hands, IO_HANDS's, below: which are lights, mat's
                                                **   bits 1 and 2, a door never battered with one, the UI's wield, the
                                                **   user's, 2026-11-17) */
        held_kinds ();
        n = (kind_flags (held_k[0]) & K_LIGHT ? 1 : 0) | (kind_flags (held_k[1]) & K_LIGHT ? 2 : 0);
        hub_map (0);
        HUB->ireq.x = held_k[0];
        HUB->ireq.y = held_k[1];
        HUB->ireq.mat = n;
        return 0;
    }

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
#pragma code-name (pop)
#pragma rodata-name (pop)

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
    sky_ask (n);                                /* (On a shaft's head: it falls, part 17's) */
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

unsigned __fastcall__ s_join (unsigned n)
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
            (q.where != IW_SQUARE || (rec.sn == q.sn && rec.y == q.y)) && (q.where != IW_ITEM || IC_OF (rec.y) ==
            IC_OF (q.y)) && (q.kind != KI_BANDAGES ||  /* (In another, in the same compartment: step 6b's) */
            (rec.wear == q.wear && rec.mat == q.mat))) {     /* (Bandages of a cloth and a cleanliness alone: theirs
                                                **   each band's own wear, wcloth.c's; HOUSE.md's bandages) */
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

/* ---- Which one (the user's playtest, 2026-11-06: "if all three are on the ground, I should get a prompt for which one,
** with longer descriptions of each"; the designators' step 5): of those lying about that a take's name fits (none
** chosen by a selector, a count, a letter, "take all"), all found in one pass (pick's SL_WHICH, their numbers in
** tbuf), each one's kind, count and state as the inventory's words tell them (lit; spent, nearly spent, burning low; empty),
** those alike once: two or more that differ, each one's id, kind, count and state (WS_*) into the text for the UI to
** word and ask ("12 7 1 1|13 7 1 2|"), IR_WHICH; else 0 (one, or all alike: the chooser's, as ever) */
#define NWHICH          4
#pragma bss-name (push, "SW35RODATA")           /* (Its locals the part's own: the main RAM's room) */
static unsigned char which (void)
{
    static unsigned id[NWHICH], q[NWHICH];
    static unsigned char kd[NWHICH], st[NWHICH];
    static unsigned char j, m, n, k, f;
    static char b[6];

    wsel = SL_WHICH;
    n = (unsigned char) pick (iq.name, HW_GONE);
    wsel = 0;                                   /* (As it was: none said, s_moved's; this part's data copied in again by
                                                **   the chooser's call) */
    for (j = m = 0; j < n; ++j) {
        fetch (&rec, ((unsigned*) tbuf)[j]);
        if (!rec.id) {
            return 0;                           /* (One not its own yet: the chooser's) */
        }
        f = rec.flags & I_LIT ? WS_LIT : 0;
        if (rec.fuel <= BURN_LOW && burn_of (rec.kind)) {   /* (A light or a fuel, low or burnt out: words.c's) */
            f |= !(kind_of (rec.kind)->flags & K_LIGHT) ? (rec.fuel ? 0 : WS_EMPTY) : !rec.fuel ? WS_SPENT : f ? WS_LOW :
                WS_NEARLY;
        }
        for (k = 0; k < m && (kd[k] != rec.kind || q[k] != rec.qty || st[k] != f); ++k) {
        }
        if (k == m && m < NWHICH) {             /* (Not alike one found already: a choice) */
            id[m] = rec.id;
            kd[m] = rec.kind;
            q[m] = rec.qty;
            st[m++] = f;
        }
    }
    if (m < 2) {
        return 0;
    }
    text_new ();
    for (k = 0; k < m; ++k) {
        put (utoa (id[k], b, 10));
        put (" ");
        put (utoa (kd[k], b, 10));
        put (" ");
        put (utoa (q[k], b, 10));
        put (" ");
        put (utoa (st[k], b, 10));
        put ("|");
    }
    return IR_WHICH;
}
#pragma bss-name (pop)

/* ---- Stowed, no place said (the user's, 2026-11-06: "when stowing something, it should ask where if there is more than
** one place it could go (pouch(es) or backpack), of course depending on size.  Stowing a weapon with a sheath should
** choose the sheath if worn."; the designators' step 7): of the containers the character wears (the pack, what hangs
** from its belt), not shut, that thing n fits in (its size, its weight, its family: fits'), a holder made for its family
** (a sheath a knife's, a scabbard a sword's, a quiver arrows': its kind's slots) first, without asking, and coins the
** purse (s_purse's: the user's, 2026-11-17, "coins to the purse"); else two or more that keep anything, the pack's side
** pocket a place of its own (the user's, 2026-11-17: open, its room, a thing no bigger than a pouch takes), IR_WHICH, the
** thing's id and each place's id in the text for the UI to ask where ("W12|30|30p|31|": p, the side pocket of that one);
** one, that one (the request made a put into it, "#" and its id, or "side pocket", the compartment by its own name; the
** pack's, a stow as ever); none, as ever */
#pragma code-name (push, "SW74CODE")            /* (Part 74's: part 35's room, overlay 19's; not dq0w.rop's, part 104's,
                                                **   read in only as a rope's first wanted) */
#pragma rodata-name (push, "SW74RODATA")
#pragma static-locals (push, off)               /* (Its locals the C stack's: the catalogue's o_fits calls parts, this one
                                                **   copied in again after, its own data as it was) */
unsigned char s_stow_where (void)
{
    const struct item* p;
    const struct kind* k;
    unsigned c[NWHICH], n, i;
    unsigned char m, f, s, z, q[NWHICH];
    char b[6];

    about ();                                   /* (Before the chooser: a part's call copies this one in again, its data
                                                **   as it was) */
    if (!(n = pick (iq.name, HW_HAND))) {
        return 0;
    }
    if ((c[0] = purse (n)) != 0) {
        m = 1;                                  /* (Coins: the purse, unasked) */
        q[0] = 0;
        goto one;
    }
    f = kind_at (n)->slots;
    z = kind_at (n)->seen;                      /* (Its size, as it's made out: a pouch holds nothing much bigger than
                                                **   itself, a sword not, whatever its pints) */
    for (m = 0, i = 1; i < ihigh; ++i) {
        p = item_at (i);
        if (i == n || !p->kind || !p->id || (i != pack && (!belt || p->where != IW_ITEM || p->x != belt))) {
            continue;                           /* (Not the pack, nor hanging from its belt) */
        }
        for (s = p->kind, ccomp = 0; comp_of (s, ccomp) && crow.prep != CP_IN; ++ccomp) {
        }                                       /* (Its compartment a put goes in: its first that's in it, the world's
                                                **   cfor's, 2026-11-16) */
        k = kind_at (i);
        s = k->slots;                           /* (Kept: the kind's copy o_fits's to use again) */
        if (!((item_at (i)->shut & IS_SHUT_C (ccomp)) || !(k->flags & K_CONTAINER) || (k->keeps & KF_ON) || (i != pack &&
            !s && z > k->seen + 1) || (cat_map (), o_fits (n, i)))) {
            if (f && s == f) {                  /* (Made for its family, worn and free: first) */
                m = 1;
                c[0] = i;
                q[0] = 0;
                ccomp = 0;
                break;
            }
            if (!s && m < NWHICH) {
                q[m] = 0;
                c[m++] = i;
            }
        }
        for (ccomp = 1; i == pack && z <= kind_of (KI_POUCH)->seen + 1 && comp_of (item_kind (i), ccomp); ++ccomp) {
            if ((crow.noun & CN_MASK) == CN_POCKET && m < NWHICH && !(item_at (i)->shut & IS_SHUT_C (ccomp)) &&
                (cat_map (), !o_fits (n, i))) {
                q[m] = 1;                       /* (The pack's side pocket, open, room in it, a pouch's size: nothing much
                                                **   bigger than one, as a pouch's: a place of its own) */
                c[m++] = i;
            }
        }
        ccomp = 0;
    }
    if (m > 1) {
        text_new ();
        put ("W");
        put (utoa (item_at (n)->id, b, 10));
        for (f = 0; f < m; ++f) {
            put ("|");
            put (utoa (item_at (c[f])->id, b, 10));
            if (q[f]) {
                put ("p");
            }
        }
        put ("|");
        return IR_WHICH;
    }
one:
    if (m && (c[0] != pack || q[0])) {
        iq.kind = IO_PUT;                       /* (Into that one: a put, by its id; the side pocket by its own name) */
        if (q[0]) {
            strcpy (iq.into, "side pocket");
        } else {
            iq.into[0] = '#';
            utoa (item_at (c[0])->id, iq.into + 1, 10);
        }
    }
    return 0;
}
#pragma static-locals (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
unsigned char stow_where (void);                /* (Part 104's: its stub) */

#define STRIP_T         100
#define TK_N            5                       /* (The kinds "take all from" names, at most: more, "everything") */
extern unsigned char tk_n;                      /* (Part 105's: the kinds taken, kept as they're taken; its stubs) */
void __fastcall__ took_kind (unsigned n);
void __fastcall__ took_list (char* b);
unsigned char __fastcall__ s_moved (unsigned char op)
{
    static unsigned want, got, t, c;
    static unsigned char r, mode, times, kind, sa, away, parted;
    unsigned n;
    char b[24];

    if (op == IO_STOW && !iq.count && stow_where ()) {
        return IR_WHICH;                        /* (Several places it fits: where, asked) */
    }
    want = iq.count == CT_ALL ? 0xFFFF : iq.count;
    got = t = 0;
    times = away = tk_n = 0;
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
        if (op == IO_GET && !times && n && csum > item_at (n)->qty && !iq.count && !wsel && *iq.name != '#') {
            hub_map (0);                        /* (Several of the name lying about, all told: which, perhaps; one, as
                                                **   ever, no second pass of the store nor the parts' swaps for it: a
                                                **   take's answer as quick as it was, 2026-11-06) */
            if (!(HUB->pc_lump & (PL_MORE | PL_ALL)) && which ()) {
                return IR_WHICH;                /* (Several that differ: which, asked) */
            }
            n = pick (iq.name, mode);           /* (All alike: the chooser's, as ever; rec and the rest its again) */
        }
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
        if (op == IO_TAKE && !*iq.name) {
            took_kind (n);                      /* (Each kind taken and its count: "take all from the case" names them,
                                                **   the user's, 2026-11-17) */
        }
        rs_ok |= 4;                             /* (The rest by the lump, their levels paid: part 85's) */
        sa = last_sa;
        t += took;
        if (win && op == IO_TAKE && (item_at (win)->kind == KI_BODY || item_at (win)->kind == KI_BONES) &&
            (kind_of (kind)->flags & K_WEAR)) {
            t += STRIP_T;                       /* (Stripped from a body: twice a doff's 5 seconds, the user's, 2026-11-03) */
        }
        join (n);
        ++times;
        if (*iq.name == '#' && !(kind_of (kind)->form & KF_COUNT)) {
            strcpy (iq.name, kind_of (kind)->name);   /* (A thing named by its letter, a count of it: the rest of those
                                                **   alike by its kind's name; a lot's count its own alone, the user's
                                                **   designators, 2026-11-06) */
        }
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
        many (b, got, kind);
        if (want != 0xFFFF && got < want && !away) {
            if (op == IO_DROP) {
                put ("You drop ");           /* ("You have only one." the UI's first words, as it carries fewer than
                                                **   it counted, its own view: sway.c's count20, 2026-11-18) */
            } else {
                put (got == 1 ? "There is only " : "There are only ");
            }
            if (op <= IO_DROP || op == IO_TAKE) {   /* ("There are only 2 there; you take both.", the user's, 2026-11-06) */
                if (op != IO_DROP) {
                    put (got == 1 ? "one" : utoa (got, b, 10));
                    put (" there; you take ");
                }
                put (got == 1 ? "it." : got == 2 ? "both." : "them all.");
                return r;
            }
            put (b);
            put (". ");
        }
        if (op == IO_GET || op == IO_DROP || op == IO_TAKE) {
            put (op == IO_DROP ? "You put down " : "You take ");
            if (op == IO_TAKE && !*iq.name) {
                took_list (b);                  /* ("You take a chisel and a key from the case.") */
            } else {
                put (b);
            }
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
** digits, "4 rations"; one of a lot, "a ration", "a penny" (part 105's since 2026-11-06, part 35's room: through its
** stub; an inventory's alike the quicker, part 105 the smaller to swap in) */
#pragma code-name (push, "SW105CODE")
#pragma rodata-name (push, "SW105RODATA")
void __fastcall__ s_many (char* b, unsigned n, unsigned char k)
{
    static const char* const few[11] = { "no", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine",
        "ten" };
    const struct kind* p = kind_of (k);
    unsigned char i, c;

    if (!*iq.name && iq.kind == IO_TAKE && tk_n > TK_N) {   /* ("Take all from the case", more kinds than its list names:
                                                **   all of them, or all that would come, any left in it, pick's) */
        strcpy (b, pick ("", HW_IN) ? "what you can" : "everything");
        return;
    }
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

/* "Take all from the case" names what's taken in one list (the user's, 2026-11-17: "You take a chisel and a key from
** the case."): each kind taken and its count, kept as it's taken (s_moved's: thing n; TK_N kinds at most, more
** "everything", s_many's b), then said, each by its count's words (s_many's), into b and the text */
unsigned char tk_n;                             /* (The main RAM's: kept across the parts' swaps) */
static unsigned char tk_k[TK_N];
static unsigned tk_q[TK_N];
#pragma static-locals (push, off)
void __fastcall__ s_took_kind (unsigned n)
{
    unsigned char j, k = item_at (n)->kind;

    if (tk_n > TK_N) {
        return;
    }
    for (j = 0; j < tk_n && tk_k[j] != k; ++j) {
    }
    if (j == TK_N) {
        tk_n = TK_N + 1;
        return;
    }
    if (j == tk_n) {
        tk_k[tk_n++] = k;
        tk_q[j] = 0;
    }
    tk_q[j] += item_at (n)->qty;
}

void __fastcall__ s_took_list (char* b)
{
    unsigned char j;

    if (tk_n > TK_N) {
        put (b);                                /* (More kinds than it names: "everything", s_many's) */
        return;
    }
    for (j = 0; j < tk_n; ++j) {
        s_many (b, tk_q[j], tk_k[j]);
        put (b);
        put (j + 2 == tk_n ? " and " : j + 1 < tk_n ? ", " : "");
    }
}
#pragma static-locals (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* (Part 16's: I_TAKE's walk over to a thing, a count taken up as it gets there: the request as it was, its count;
** wgame.c's take_up, its one caller, in its part since 2026-11-18: part 35's room, its call of s_moved the swap
** take_up's call of it was) */
#pragma code-name (push, "SW16CODE")
#pragma rodata-name (push, "SW16RODATA")
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
#pragma code-name (pop)
#pragma rodata-name (pop)

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

static unsigned char rv_c;                      /* (The compartment opened: shut_work's) */
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
    lst_c = comp_of (item_at (n)->kind, 1) ? rv_c : 0xFF;   /* (The compartment opened's alone: its inside, not what's
                                                **   on its lid, step 6b's; a drawer's) */
    for (w = 0; (w = items (w, IN_X | IN_ITEM, n)) != 0 && lst_c != 0xFF && icomp (w) != rv_c; ) {
    }
    if (!w) {
        lst_c = 0xFF;
        put (": it's empty.");
        return;
    }
    put (", revealing ");
    hub_map (UI_BANK);
    t0 = tp;
    hub_map (0);
    listed (n);
    lst_c = 0xFF;
    and_last36 (t0);
    put (".");
}

#define tested(p, s)    act_test (0, (p), (s))  /* (The referee's test of the character's, in SLOT_UI (the UI waits on it
                                                **   meanwhile): its grade, -100 no tools for it (lockpicks); every
                                                **   actor's sender, part 69's, acts.h's, 2026-11-19) */

/* A key the character carries (in hand, on it, in what it carries; in the pack it set down for this handling, rs_pack:
** the full reach's own step, 2026-11-17) that fits the lock of thing number id (its number in the key's size and thick:
** HOUSE.md's keys that fit, 2026-10-27): 1; 2, keys but none that fits; 0, none */
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
        if ((item_at (j)->where != IW_ACTOR || item_at (j)->x) && j != rs_pack) {
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

void __fastcall__ slide (unsigned n);          /* (Part 61's: what's on a lid slid off) */
void __fastcall__ own_comps (struct item* q);  /* (Part 61's: a thing's compartments made its own) */

SAY_PUT (say36)

/* The character's lid work's act (the act table's row ac: a key turned, a lock picked at or picked, quiet, heard close
** by, the user's, 2026-11-17; a lid wrenched at, loud): its noise where it stands (act_noise, every actor's: a denizen's
** reach into a box, wfight.c's den_pry, the same rows), its row's time (the one-path plan's stage 1, 2026-11-19) */
static unsigned __fastcall__ box_act (unsigned char ac)
{
    hub_map (0);
    return act_noise (0, ac, HUB->pc_x, HUB->pc_y);
}

unsigned char s_shut_work (void)
{
    const struct kind* k;
    unsigned n, t;                              /* (Its time: a minute's picking, 600, past a char's 255: it was 88
                                                **   tenths, a char's, till 2026-10-22) */
    unsigned char w = iq.to & ~IO_DONE, s, c, ok = IR_NO, d = iq.to & IO_DONE, lt = 0, os, cc, h, pk = 0;
    signed char g;
    char nm[CN_LEN];
    unsigned st;

    if (w == PA_PICK) {
        w = PA_UNLOCK;                          /* ("Pick the lock": picked, no key tried, the user's, 2026-11-17) */
        pk = 1;
    }
    text_new ();
    wcn = 1;                                    /* (Or a compartment of one by its own name: "open the drawer", the
                                                **   user's, 2026-11-16) */
    n = at_hand (iq.name);
    wcn = 0;
    cc = n ? wcomp : 0;
    if (!n) {
        if (afar) {
            hub_map (0);
            HUB->ireq.x = far_x;
            HUB->ireq.y = far_y;
            HUB->ireq.item = far_n;
            return IR_NOT_HERE;
        }
        return IR_NONE;                         /* (No such thing: a door's work, the UI's) */
    }
    t = box_act (AC_LID);                       /* (A lid's handling: no noise; the act table's row) */
    make_own (n);                               /* (Its lid and its lock its own: found closed, a chest locked
                                                **   sometimes) */
    if (!d) {
        reach_from (n);                         /* (Its reach, as it's begun: HOUSE.md's) */
    }
    fetch (&rec, n);
    st = rec.where == IW_ITEM ? rec.x : 0;      /* (What it's in: reached into first, past the refusals: part 85's) */
    k = kind_of (rec.kind);
    strcpy (nm, k->name);
    c = (k->keeps & KF_CLOSES) >> 5;
    s = rec.shut;
    if (cc && comp_of (rec.kind, cc) && (crow.noun & CN_MASK) == CN_LID) {
        cc = 0;                                 /* ("Open the lid": its holder's, the chest's) */
    }
    if (cc) {                                   /* (A compartment past its first: its own closing, its lock (a drawer's,
                                                **   its holder's key: CC_LOCK), its state in its holder's shut and fuel,
                                                **   worked as the first's is, items.h's CS_*) */
        comp_of (rec.kind, cc);
        if ((crow.noun & CC_INNER) && (s & IS_CLOSED)) {
            say36 (SA_SHUT, "", nm);           /* (A false bottom: "The chest is closed.") */
            return IR_NO;
        }
        c = (crow.keeps & KF_CLOSES) >> 5;
        h = rec.fuel >> (cc - 1) * 4;
        s = (s & IS_SHUT_C (cc) ? IS_CLOSED : 0) | (crow.noun & CC_LOCK ? IS_LOCK : 0) | (h & CS_LOCKED ? IS_LOCKED : 0) |
            (h & CS_BURST ? IS_BURST : 0);
        strcpy (nm, cnouns[crow.noun & CN_MASK]);
    }
    os = s;
    rv_c = cc;                                  /* (What it shows opened: that compartment's, reveal's) */
    if (!c) {
        say36 (k->flags & K_CONTAINER ? SA_NO_LID : w == PA_CLOSE ? SA_NO_CLOSE : SA_NO_OPEN, nm, "");
        return IR_NO;
    }
    --c;
    if ((w == PA_LOCK || w == PA_UNLOCK) && (s & (IS_LOCK | IS_BURST | IS_CLOSED)) == (IS_LOCK | IS_CLOSED) &&
        ((s & IS_LOCKED) != 0) == (w == PA_UNLOCK) && !pk && (lt = key_fits (rec.id)) == 1) {
        if (reach_step (rs_tool, RS_TOOL)) {
            return IR_NO;                       /* (Its key got out: a tool's steps) */
        }
        s ^= IS_LOCKED;                         /* (With its own key: no test, a few seconds) */
        say36 (w == PA_LOCK ? SA_LOCK : SA_UNLOCK, nm, "");
        t = box_act (AC_LIDKEY);
        lt = 0;
        ok = IR_OK;
        goto done;
    }
    if (w == PA_LOCK) {
        say36 (!(s & IS_LOCK) ? SA_NO_LOCK : s & IS_BURST ? SA_LOCK_BROKE : s & IS_LOCKED ? SA_LOCKED_ALREADY :
            SA_NO_KEY, nm, "");
        return IR_NO;
    }
    if (w == PA_CLOSE) {
        if (s & IS_CLOSED) {
            say36 (SA_CLOSED_ALREADY, nm, "");
            return IR_NO;
        }
        s |= IS_CLOSED;
        say36 (SA_CLOSE + c, nm, "");      /* (Closed, tied shut, stoppered) */
        ok = IR_OK;
    } else if (!(s & IS_CLOSED)) {
        say36 (w == PA_UNLOCK && (s & IS_LOCK) ? SA_IS_OPEN : SA_OPEN_ALREADY, nm, "");
        return IR_NO;
    } else if (w == PA_UNLOCK) {
        if (!(s & IS_LOCKED)) {
            say36 (s & IS_LOCK ? SA_NOT_LOCKED : SA_NO_LOCK, nm, "");
            return IR_NO;
        }
        if ((g = tested (P_PICKLOCK, -1 - (signed char) (rec.quality / 3))) == -100) {
            say36 (lt ? SA_NO_FIT : SA_NO_PICKS, "", "");
            return IR_NO;
        }
        lt = 0;
        if (!d) {
            memcpy (yn, "picks\0", 7);          /* (Its keys in the main RAM: this part gone as tool_reach's copied in) */
            if (reach_step (0, st) || tool_reach (yn)) {
                return IR_NO;
            }
            t = box_act (AC_LIDPICK);           /* (The means to hand: a minute a try, as a door's, a long task, its
                                                **   test made again at its end; the lockpicks' reach first; its
                                                **   scratching at the lock) */
            lt = LT_PICK;
        } else if (g > 0) {
            s &= ~IS_LOCKED;
            say36 (SA_PICKED, nm, "");
            box_act (AC_LIDPICK);
        } else {
            say36 (SA_NOT_PICKED, nm, "");
        }
        ok = IR_OK;
    } else if (s & IS_LOCKED) {
        if (w != PA_FORCE) {
            say36 (SA_LOCKED, nm, "");
            return IR_NO;
        }
        if (!d) {
            if (reach_step (0, st)) {
                return IR_NO;
            }
            t = box_act (AC_LIDFORCE);          /* (Loud: a lock burst, or a lid wrenched at, ten seconds' of it, its
                                                **   test at its end) */
            lt = LT_FORCE;
        } else if (tested (P_FORCE, (rec.mat == MT_WOOD ? 0 : -2) - (signed char) (rec.quality / 3)) > 0) {
            s = (s & ~(IS_LOCKED | IS_CLOSED)) | IS_BURST;
            say36 (SA_FORCED, nm, "");      /* (What it shows after it: reveal's words) */
            reveal (n);
        } else {
            say36 (SA_NOT_FORCED, nm, "");
        }
        ok = IR_OK;
    } else {
        s &= ~IS_CLOSED;                        /* (Opened; forced open, one not locked: what it shows told) */
        say36 (SA_OPEN + c, nm, "");       /* (Opened, untied, unstoppered: what it shows after it) */
        reveal (n);
        ok = IR_OK;
    }
done:
    if (!d && !lt && reach_step (0, st)) {
        return IR_NO;                           /* (What it's in reached into first: part 85's) */
    }
    if (cc) {
        ip = item_at (n);                       /* (A compartment's state back: its closed bit, its lock's) */
        ip->shut = (ip->shut & ~IS_SHUT_C (cc)) | (s & IS_CLOSED ? IS_SHUT_C (cc) : 0);
        ip->fuel = (ip->fuel & ~CS_AT (cc, CS_LOCKED | CS_BURST)) | CS_AT (cc, (s & IS_LOCKED ? CS_LOCKED : 0) |
            (s & IS_BURST ? CS_BURST : 0));
    } else {
        if ((os & IS_CLOSED) && !(s & IS_CLOSED) && comp_of (item_at (n)->kind, 1)) {
            slide (n);                          /* (Opened, forced open: what's on its lid slides off) */
        }
        for (h = 1; (s & IS_CLOSED) && comp_of (item_at (n)->kind, h); ++h) {
            if (crow.noun & CC_INNER) {
                s |= IS_SHUT_C (h);             /* (Closed, what's reached through it closed with it: a false
                                                **   bottom) */
            }
        }
        item_at (n)->shut = s;
    }
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
** part 39's); 8, a compartment looked at by its own name ("look at the drawer", part 39's): its state, then what's in
** it */
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
    unsigned char f, kk, on, pl, to = iq.to, w = iq.to & 4, cc = 0, hk, hs, multi, did = 0, ic;
    unsigned hf;
    char nm[CN_LEN + 4], hn[CN_LEN + 4];
    char* t0;

    if (!(to & 2)) {
        text_new ();
    }
    to &= 1;
    wcn = !to;                                  /* ("Look in the drawer": a compartment by its own name, the user's,
                                                **   2026-11-16) */
    n = at_hand (iq.name);
    wcn = 0;
    ic = n ? wcomp : 0;
    if (!n) {
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
    hk = rec.kind;                              /* (Its compartments, step 6b's: "look on the chest" its lid's top, a
                                                **   look in its inside, examined each that holds something) */
    hs = rec.shut;
    hf = rec.fuel;
    if ((iq.to & 8) && ic) {                    /* (A compartment looked at, iq's to 8: itself, not its holder, the user's,
                                                **   2026-11-17: "The drawer is closed and locked.", "The side pocket is
                                                **   untied. In it lies a coin."; a lid's top its holder's lid's state) */
        comp_of (hk, ic);
        strcpy (hn, cnouns[crow.noun & CN_MASK]);   /* (Its name: the catalogue's bank's, in the window) */
        put ("The ");
        put (hn);
        put (" is ");
        f = crow.keeps & KF_CLOSES;
        on = (hs & IS_SHUT_C (ic)) || ((crow.noun & CC_INNER) && (hs & IS_CLOSED));
        did = (hf & CS_AT (ic, CS_LOCKED)) != 0;
        if (!f) {
            f = kk & KF_CLOSES;                 /* (A lid's top: its holder's lid, its lock; what's on it seen, the lid
                                                **   shut or not) */
            pl = hs & IS_CLOSED;
            did = (hs & (IS_LOCKED | IS_BURST)) == IS_LOCKED;
        } else {
            pl = on;
        }
        put (f == KF_TIE ? (pl ? "tied shut" : "untied") : f == KF_STOPPER ? (pl ? "stoppered" : "unstoppered") : pl ?
            "closed" : "open");
        if (did) {
            put (" and locked");
        }
        put (".");
        if (on) {
            return IR_OK;                       /* (Closed: nothing in it seen) */
        }
        put (" ");
        did = to = 1;                           /* (What's in it as examined, "In it lies ...", that one alone: no reach,
                                                **   a look's) */
        strcpy (hn, "it");
    } else {
        strcpy (hn, nm);
    }
    multi = comp_of (hk, 1);
    for (pl = 0; !ic && comp_of (hk, pl); ++pl) {
        if (crow.prep == (w ? CP_ON : CP_IN) && !CS_UNSEEN (crow.noun, hf, pl)) {
            cc = pl;                            /* (Its first that's on it, or in it, as said: a chest's lid's top, a
                                                **   desk's drawer) */
            break;
        }
    }
    if (ic) {
        cc = ic;                                /* (The one named: that one alone, its own name) */
        w = 4;
    }
    for (;;) {
        comp_of (hk, cc);
        if (cc && CS_UNSEEN (crow.noun, hf, cc)) {
            goto next;                          /* (Hidden, not found: not told) */
        }
        strcpy (nm, hn);
        if (cc && crow.prep == CP_IN && (crow.noun & CN_MASK) && !(iq.to & 8)) {
            strcpy (nm, to ? "its " : "the "); /* (One in it of its own name: "In the drawer lies a key.", "In its
                                                **   side pocket lies a coin.") */
            strcat (nm, cnouns[crow.noun & CN_MASK]);
        }
        on = crow.prep == CP_ON ? 1 : crow.prep == CP_HUNG ? 2 : 0;
        pl = 0;
        if (hs & IS_SHUT_C (cc)) {
            if (!to || !multi) {
                put (to ? "It's closed." : "The ");
                if (!to) {
                    put (nm + 4);
                    put (" is closed.");
                }
            }
            goto next;                          /* (Examined, said closed already: the look's) */
        }
        if (iq.kind == IO_INSIDE && !did) {
            rs_cc = cc;
            j = reach_step (0, n);
            rs_cc = 0;
            if (j) {
                return IR_NO;
            }
            lv_c = cc;
            reach_to (n);                       /* (Its reach, looked into: a pack worn taken off, HOUSE.md's) */
            did = 1;
        }
        for (j = 0; (j = items (j, IN_X | IN_ITEM, n)) != 0; ) {
            if (!multi || icomp (j) == cc) {
                fetch (&rec, j);
                pl += 1 + (rec.qty > 1 || (kind_of (rec.kind)->flags & K_PLURAL));
            }
        }
        if (cc && to && !w) {
            if (!pl) {
                goto next;                      /* (Examined: another compartment told if it holds something) */
            }
            put (" ");
        }
        if (!pl) {
            put (on == 1 ? "There's nothing on " : on == 2 ? "Nothing hangs from " : to ? "It's empty" : "The ");
            if (on || !to) {
                put (on ? nm : nm + 4);
            }
            put (on || to ? "." : " is empty.");
            goto next;
        }
        put (on == 1 ? "On " : on == 2 ? "From " : "In ");
        put (nm);
        put (on == 2 ? (pl > 1 ? " hang " : " hangs ") : pl > 1 ? " lie " : " lies ");
        hub_map (UI_BANK);
        t0 = tp;
        hub_map (0);
        lst_c = multi ? cc : 0xFF;              /* (That compartment's alone) */
        listed (n);
        lst_c = 0xFF;
        and_last (t0);
        put (".");
next:
        if (!to || w || !comp_of (hk, ++cc)) {
            return IR_OK;                       /* (A look in, or on: that one) */
        }
    }
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

/* Thing q's compartments past its first made its own as it is (part 13's s_make_own's, part 61's its room, the user's, 2026-11-16): one that closes
** found closed, as its first is; one with a lock, locked one time in three; a hidden one there one time in two, by its
** number, no dice: items.h's CS_* */
void __fastcall__ s_own_comps (struct item* q)
{
    static unsigned char h, m, w;
    static unsigned id;

    id = q->id;
    w = q->where == IW_SQUARE;
    for (h = 1; comp_of (q->kind, h); ++h) {
        m = crow.keeps & KF_CLOSES;
        if (m && (w || m == KF_STOPPER)) {
            q->shut |= IS_SHUT_C (h);
        }
        if ((crow.noun & CC_LOCK) && w && (unsigned char) (id * 53 + (id >> 2) + h) % 3 == 0) {
            q->fuel |= CS_AT (h, CS_LOCKED);
        }
        if ((crow.noun & CC_HIDDEN) && ((unsigned char) (id * 37 + (id >> 3) + h) & 2)) {
            q->fuel |= CS_AT (h, CS_NONE);
        }
    }
}

/* What's on its lid slid off as it's opened (the user's, 2026-11-16: "Things on a chest's lid slide off when it's opened"),
** to the floor (its square's; a carried one's, the character's), told: "From the lid a chisel slides onto the floor."; the
** world's look told it's changed (slid).  Every actor's opening (the user's, 2026-11-17: "a denizen opening a lid tips off
** what lies on it"): n | SL_QUIET, a denizen's (wfight.c's den_pry), untold, as its opening is */
void __fastcall__ s_slide (unsigned n)
{
    unsigned j, x, y;
    unsigned char c, sn, pl = 0, q = n >> 15;
    char* t0;

    n &= ~SL_QUIET;
    for (c = 1; comp_of (item_at (n)->kind, c) && (crow.noun & CN_MASK) != CN_LID; ++c) {
    }
    if (!comp_of (item_at (n)->kind, c)) {
        return;                                 /* (No lid's top) */
    }
    for (j = 0; (j = items (j, IN_X | IN_ITEM, n)) != 0; ) {
        if (ic_of (j) == c) {
            pl += 1 + (item_at (j)->qty > 1 || (kind_at (j)->flags & K_PLURAL));
        }
    }
    if (!pl) {
        return;
    }
    if (!q) {
        put (" From the lid ");              /* ("From the lid a chisel slides onto the floor.": its list the
                                                **   UI's list renderer's, its words lower case) */
        hub_map (UI_BANK);
        t0 = tp;
        hub_map (0);
        lst_c = c;
        listed (n);
        lst_c = 0xFF;
        and_last (t0);
        put (pl > 1 ? " slide onto the floor." : " slides onto the floor.");
    }
    for (j = n; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    if ((ip = item_at (j))->where == IW_SQUARE) {
        sn = ip->sn;                            /* (Where it stands; carried, the character's square) */
        x = ip->x;
        y = ip->y;
    }
    for (j = 0; (j = items (j, IN_X | IN_ITEM, n)) != 0; ) {
        if (ic_of (j) == c) {
            ip->where = IW_SQUARE;
            ip->sn = sn;
            ip->x = x;
            ip->y = y;
            sky_ask (j);
            j = 0;                              /* (The list again: this one gone from it) */
        }
    }
    slid = 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* Box b locked, key k made to fit it alone (the kit's, dq0 -k's "chest,iron key": outmake.c's rule, every making's;
** HOUSE.md's keys that fit).  Part 13's, the stock's (part 61's till 2026-11-17: its room, the headroom rule's, a
** denizen's opening's slide) */
#pragma code-name (push, "SW13CODE")
#pragma rodata-name (push, "SW13RODATA")
void __fastcall__ s_kit_lock (unsigned b, unsigned k)
{
    struct item* p;
    unsigned id;

    unsigned char c;

    make_own (b);
    make_own (k);
    for (c = 1; comp_of (item_at (b)->kind, c) && !(crow.noun & CC_LOCK); ++c) {
    }
    p = item_at (b);
    if (comp_of (p->kind, c)) {
        p = item_at (b);                        /* (A drawer's lock its holder's first: a desk's, 2026-11-16) */
        p->shut |= IS_SHUT_C (c);
        p->fuel |= CS_AT (c, CS_LOCKED);
    } else {
        p = item_at (b);
        p->shut = IS_CLOSED | IS_LOCK | IS_LOCKED;
    }
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
static unsigned char gd, geye, glv, gn, gfar, gfo;     /* (gfo: the denizen a person, "they": the hub's folk,
                                                **   2026-11-17) */
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
            put (m == MT_IRON || m == MT_STEEL ? "rusted" : m == MT_LEATHER ? "cracked" : MT_TEXTILE (m) ? "frayed" :
                m == MT_WOOD ? "rotten" : m == MT_SILVER || m == MT_BRONZE ? "tarnished" : "worn");
        }
    } else {
        put (nm);
    }
    if ((gr.flags & (I_WORN | I_HELD)) == I_WORN) {
        worn_put (i, gfo ? WH_THEY : WH_DEN);   /* (" on its left hand": part 107's, 2026-11-10; a person's "their") */
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
        if ((i != j || !(gr.flags & (I_WORN | I_HELD)) || gr.kind == KI_PENNIES || gr.kind == KI_FARTHINGS) && named (kind_of (gr.kind)->name, w)) {
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
            gfo = HUB->folk[k];
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
                gthing (i, any ? gfo ? ". They hold " : ". It holds " : " holds ", 0);
            } else if (part == 1 && (gr.flags & (I_WORN | I_HIDDEN | I_HELD)) == I_WORN) {
                if (kind_of (gr.kind)->flags & K_BELT) {
                    belt = i;
                }
                gthing (i, any ? gfo ? ". They wear " : ". It wears " : " wears ", 0);
            }
        }
        if (part == 2 && belt) {
            for (i = 0; (i = items (i, IN_X | IN_ITEM, belt)) != 0; ) {
                fetch (&gr, i);
                if ((gr.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN) {
                    continue;
                }
                j = kind_of (gr.kind)->slots ? items (0, IN_X | IN_ITEM, i) : 0;   /* (A carrier's: what's in it) */
                gthing (j ? j : i, any ? gfo ? ". At their belt: " : ". At its belt: " : gfo ? " has at their belt " :
                    " has at its belt ", j ? i : 0);
            }
        }
        if (gn) {
            any = 1;
            and_last62 (t0);
        }
    }
    put (!any ? (did && gfar ? gfo ? " is too far off for you to make out what they carry." :
        " is too far off for you to make out what it carries." : " carries nothing that you can see.") :
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

static unsigned char __fastcall__ look_at (unsigned n);

/* A thing looked at (n, s_look_pick's: 0, none), its reply's result (a look into what's in it may owe a reach: reach_done's,
** the character's next spell's, in the reply's y) */
void __fastcall__ s_look_on (unsigned n)
{
    static unsigned char r;

    r = look_at (n);
    hub_map (0);
    if (HUB->ropes) {
        rope2 (RA_LOOK | n);                    /* (A rope in place: what it's tied to, how far it goes: part 98's) */
    }
    if (rsteps | rbody) {
        n = reach_done (RD_SAID);               /* (Nothing owed after it) */
        hub_map (0);
        HUB->ireq.y = n;
    }
    HUB->ireq.result = r;
}

static unsigned char __fastcall__ look_at (unsigned n)
{
    static const char* const where[6] = { "worn", "on you", "in your hand", "in your left hand", "in your right hand",
        "in both hands" };
    static const char* const shut[3][2] = { { "open", "closed" }, { "untied", "tied shut" },
        { "unstoppered", "stoppered" } };
    const struct kind* q;
    struct item* p;
    unsigned c, px, py;
    unsigned char mode = iq.to, sh, cl, k, mine;
    char b[6];

    text_new ();
    if (!n) {
        if (mode == LM_GO) {
            closer (LM_GO);                     /* (A trap it knows of: part 42's room, its reply in ld) */
            return ld;
        }
        return IR_NONE;
    }
    ld = seen_d > 99 ? 99 : seen_d;
    for (c = n, cl = 0; (p = item_at (c))->where == IW_ITEM; ) {
        px = p->x;
        if (in_shut (c)) {
            cl = 1;                             /* (In a thing closed: not seen; on a chest's lid, seen) */
            py = px;
        }
        c = px;
    }                                           /* (Whose: the character's, on it or in what it carries; where it lies) */
    px = py;
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
    if (wcomp && ld <= 1 && mode <= LM_CLOSE) {
        iq.to = 8 | 2;                          /* (A compartment named, within reach: itself, open or closed, locked,
                                                **   what's in it, not its holder: part 61's, the user's, 2026-11-17) */
        return inside ();
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
        if (rec.flags & I_WORN) {
            worn_put (n, WH_LOOK);              /* (" on your left hand": part 107's, 2026-11-10) */
        }
    } else if (rec.where == IW_ITEM) {
        sh = item_at (rec.x)->kind;             /* (What it's in, or on: its compartment's) */
        sh = comp_of (sh, icomp (n)) && crow.prep != CP_IN;
        q = kind_of (item_at (rec.x)->kind);
        put (sh ? "on " : "in ");
        put (mine ? "your " : "the ");
        put (q->name);
    } else if (rec.where == IW_PORTAL) {
        put (rec.kind == KI_BAR ? "set in its door" : "set in its window");
    } else if (ld > 1) {
        put ("some ");
        goto feet;
off:
        put (" is some ");
feet:
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
                (rec.wear >= 128 ? "dull" : "dulled") : MT_TEXTILE (rec.mat) && rec.wear >= 128 ? (rec.wear ==
                CL_BLOODY ? "bloody" : "soiled") : "worn");    /* (A cloth's cleanliness: HOUSE.md's bandages) */
        }
        if (sh & IS_LOCK) {
            word (sh & IS_BURST ? "its lock broken" : "with a lock");
        }
    }
    c = (lk.flags & (K_CONTAINER | K_BELT) || lk.keeps & KF_ON) && (!(sh & IS_CLOSED) || comp_of (rec.kind, 1)) &&
        ld <= 1;                                /* (Closed, a chest's lid's top still told: step 6b's) */
    if (c && !items (0, IN_X | IN_ITEM, n)) {
        if (!(sh & IS_CLOSED)) {
            word (lk.keeps & KF_ON ? "bare" : "empty");
        }
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
        handled = n;
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
    r->plural = ((k->flags & K_PLURAL) != 0) | (strcmp (k->tool, "food") ? 0 : 2);   /* (2: food, what's eaten, by its key, not
                                                **   the appraisal's craft, a candle's) */
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
static struct kitrec ke;
static unsigned kq[KQ];                         /* (Each one's number, */
static unsigned char kqe[KQ], kn, kqn, kmh;     /*   its entry; the entries so far; how many of them; its main hand) */
static int kfd;
static char kp[56];

/* Thing i (ip's record), in entry in less one (0: on the character), listed */
static void __fastcall__ kit_put (unsigned i, unsigned char in)
{
    unsigned char r;

    memcpy (&kr, ip, sizeof kr);
    if (kr.kind >= KI_THEME && kr.kind < KI_THEME + NTKINDS) {    /* (A writing, the theme's kind a read rule's: an old
                                                **   writing, its clue the rule's, life.h's, the plan's step 10) */
        hub_map (THEME_BANK);
        for (r = 0; r < THEME->nrules && r < NRULES && (THEME->rule[r].verb != TR_READ ||
            THEME->rule[r].a != kr.kind - KI_THEME); ++r) {
        }
        if (r >= THEME->nrules || r >= NRULES) {
            return;
        }
        kr.kind = KI_WRITING;
        kr.fuel = CLUE_HERE | r;
    }
    if (kn >= NOFIT || kr.kind == KI_KEY) {
        return;
    }
    ke.o.kind = kr.kind;
    ke.o.qty = kr.qty;
    ke.o.quality = kr.quality;
    ke.o.place = in ? PL_IN : !(kr.flags & I_HELD) ? PL_WORN : (kr.sn & IH_BOTH) == IH_BOTH ? PL_BOTH :
        kr.sn & kmh ? PL_HAND : PL_OFF;
    ke.o.in = in ? in : (kr.flags & I_WORN) && (kr.sn & 0x30) == WP_AT ? WP_KIT | (kr.sn & (WP_LOC | WP_PAIR)) |
        (kr.sn & WP_RIGHT ? 0x20 : 0) : 0;      /* (Worn, its place: WP_KIT's, 2026-11-10) */
    ke.o.flags = kr.flags & I_LIT ? TF_LIT : 0;
    ke.o.mat = kr.mat;
    memcpy (&ke.wear, &kr.wear, KIT_ITEM);      /* (Each thing as it is: the plan's step 3) */
    hub_map (UI_BANK);                          /* (And its letter: hub.h's designators, 2026-11-06) */
    for (r = 0; r < NLET && UIREC->low[r] != (kr.id ? kr.id : i | LT_REC); ++r) {
    }
    ke.let = r < NLET ? r + 1 : 0;
    write (kfd, &ke, sizeof ke);
    hub_map (KIND_BANK);
    if (kqn < KQ && (CATALOGUE[kr.kind].flags & (K_CONTAINER | K_BELT) || CATALOGUE[kr.kind].keeps & KF_ON)) {
        kq[kqn] = i;
        kqe[kqn++] = kn;
    }
    ++kn;
}

static void kit_file (void)
{
    static unsigned i;
    static unsigned char k;

    if (kfd < 0) {
        return;
    }
    hub_map (0);
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

/* kit.dat; and kit0.dat if it's not there, the first kept of a life: what it set out with, dq0acct's to restock by (a new life's removed it) */
static void kit_keep (void)
{
    hub_map (0);
    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (kp, HUB->savedir);
    strcat (kp, "/../kit.dat");                 /* (The save's, above the dungeon's folder: life.h's) */
    kfd = open (kp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    kit_file ();
    strcpy (strchr (kp, 0) - 4, "0.dat");
    if ((kfd = open (kp, O_RDONLY)) >= 0) {
        close (kfd);
    } else {
        kfd = open (kp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        kit_file ();
    }
}

#pragma static-locals (pop)
#pragma bss-name (pop)

/* Its weight, its lock, it gone over for traps (thing n, rec its record, lk its kind; its name said) */
/* The last three overlays read in (dq0w.rop, ld65's, as they are: climbing, part 97; rope and grapples, parts 95, 96, 98; a denizen's weapon let fall, part 91; a tunneller's dig, part 79), as a climb, a
** dig, a rope, a dropped weapon or a tunneller first wants them: none of a game's start's time, the card's, for what most games come to late or never (dq0starttime,
** 2026-11-04); carried on with a rope in place, at once.  Once (the hub's rope_in), its time then, some 3M cycles */
#pragma static-locals (push, off)
static void ropes_in (void)
{
    char p[40];
    int fd;
    unsigned char k;

    hub_map (0);
    if (!HUB->rope_in) {
        strcpy (p, HUB->ldir);
        strcat (p, "/dq0w.rop");
        if ((fd = open (p, O_RDONLY)) >= 0) {
            for (k = OVBANKS - 3; k < OVBANKS; ++k) {
                hy_bank (ovbank[k]);
                read (fd, (void*) 0x8000, 0x2000);
            }
            close (fd);
        }
        hub_map (0);
        HUB->rope_in = 1;
    }
}
#pragma static-locals (pop)

unsigned char __fastcall__ s_look_more (unsigned n, unsigned char mode)
{
    unsigned w;
    unsigned char sh = rec.shut, k;
    char b[6];

    if (mode == LM_ROPES) {
        ropes_in ();
        return IR_OK;
    }
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

static const char matname[NMATS][8] = { "", "wood", "iron", "steel", "bronze", "leather", "hemp", "bone", "clay",
    "stone", "glass", "silver", "", "beeswax", "tallow", "gold", "linen", "wool", "silk" };

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
            (rec.wear >= 128 ? "its edge dull" : "its edge dulled") : MT_TEXTILE (rec.mat) && rec.wear >= 128 ?
            (rec.wear == CL_BLOODY ? "bloody" : "soiled") : "worn with use");     /* (A cloth's cleanliness) */
    }
    if (rec.id && (b = burn_of (rec.kind)) != 0) {
        w = b->life * 10;                       /* (A whole one's minutes: the quarters left) */
        if ((lk.flags & K_LIGHT) && !b->fuels) {
            w = w / 6 * rec.thick * (rec.mat == MT_WAX ? 3 : 2) / 2 * (15 + rec.quality) / 19;   /* (A candle: its own, as
                                                **   it was made, part 28's) */
        }
        word (left[lk.flags & K_LIGHT ? rec.kind == KI_LANTERN : (b->fuels & FU_DRAW) && rec.kind != KI_INK ? 3 : 2]
            [!rec.fuel ? 0 : rec.fuel >= w ? 5 : 1 + rec.fuel * 4 / w]);
    }
}

/* What a closer look shows (mode LM_CLOSE: "It's made of wood; it's little worn, half burnt down. Its lock is a simple
** one."; a portal's part's measure, "It's made of glass, 2 by 3 feet, a quarter inch thick"); its condition alone
** (LM_COND: after its name, "... is battered, its edge dulled.") */
/* Gone to a trap it knows of, the nearest in the chunk it's in ("go to the pit": the UI's resolve.c): where it is, as a
** thing's out of reach (the walk beside it, as near as it gets): ld the reply, IR_NOT_HERE; beside it, IR_OK; IR_NONE none */
#pragma bss-name (push, "SW42RODATA")
static void spot_at (void)
{
    static unsigned ox, oy;
    static unsigned char k, b, d, e, bx, by;
    struct trap* t;

    hub_map (0);
    ox = HUB->pc_x;
    oy = HUB->pc_y;
    ld = IR_NONE;
    if (*iq.name != 't' || !at (HUB->pc_stratum, ox, oy)) {
        return;
    }
    ox -= cx_;
    oy -= cy_;
    b = 0xFF;
    for (k = C->ntraps, t = C->trap; k; --k, ++t) {
        if (!(t->state & TS_HIDDEN)) {
            d = t->x > cx_ ? t->x - cx_ : cx_ - t->x;
            e = t->y > cy_ ? t->y - cy_ : cy_ - t->y;
            if (e > d) {
                d = e;
            }
            if (d < b) {
                b = d;
                bx = t->x;
                by = t->y;
            }
        }
    }
    hub_map (0);
    if (b < 2) {
        put ("It's beside you.");
        ld = IR_OK;
    } else if (b != 0xFF) {
        HUB->ireq.x = ox + bx;
        HUB->ireq.y = oy + by;
        HUB->ireq.item = 0;
        ld = IR_NOT_HERE;
    }
}
#pragma bss-name (pop)

void __fastcall__ s_closer (unsigned char mode)
{
    unsigned char k;
    char b[6];

    if (mode == LM_GO) {
        spot_at ();
        return;
    }

    lnw = 0;
    if (mode == LM_COND) {
        worn_words ();
        put (lnw ? "." : "as good as ever.");
        return;
    }
    put (lpl ? " They're made of " : " It's made of ");
    put (*matname[rec.mat] ? matname[rec.mat] : "something you can't name");
    if ((k = rec.size) != 0 && lk.craft != AP_WEAPON) {    /* (Its measure: a portal's part's, its opening's; a
                                                **   weapon's size its version, WV_*, not a measure: "1 feet long", a
                                                **   light dagger's, till 2026-11-17, the soak's) */
        put (", ");
        put (utoa (k & 15, b, 10));
        if (k >> 4) {
            put (" by ");
            put (utoa (k >> 4, b, 10));
            put (" feet");
        } else {
            put (k == 1 ? " foot long" : " feet long");     /* ("1 feet long" till 2026-11-17, the soak's dagger) */
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
    if ((lk.keeps & KF_CLOSES) == KF_STOPPER && (lk.flags & K_CONTAINER)) {   /* (A vessel, how full: by its heft, stoppered
                                                **   or not, the flask's words, 2026-11-03: handled the thing's, the callers') */
        word (!items (0, IN_ITEM | IN_X, handled) ? "empty" : ip->qty >= lk.cap_bulk ? "full" : "part full");
    }
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
static unsigned char lmode;                     /* (The words a list in a sentence, look in's, 1: "a candle and two
                                                **   chisels", not "(in it: ...)"; the inventory's, 2: one a line) */

/* Thing i's letter (tw its record: hub.h's designators, the UI bank's low), "(c) ", put (letter.s's lput, in assembly:
** part 11's room) */
void __fastcall__ lput (unsigned key, unsigned* low);
#define let_put(i)      (hub_map (UI_BANK), lput (tw.id ? tw.id : (i) | LT_REC, UIREC->low))

/* A thing in words (tw, twk its kind's): its kind, how many, and what anyone can see of its state (its make and worth
** are an appraiser's to judge).  Coded, the UI's to word (items.h's SAY_LI: how its count is said, its noun's form, its
** state, its count, its kind's name, SAY_SEP), by said.txt's LI.item: the list renderer's (the look's lists, the user's,
** 2026-11-16; its words put here before, many's for a count, part 105's) */
static void words (void)
{
    static char b[6];                           /* (Its id's digits: the world's BSS as it was, the step's cost with it,
                                                **   dq0traces') */
    static unsigned char c;
    static unsigned q;
    unsigned char f, s, w;

    c = twk->keeps & KF_CLOSES;
    if (tw.flags & I_NAMED) {                   /* (A name the character gave it: its id, marked for the UI, "#12 a
                                                **   sword", worded "Linda, your sword") */
        put ("#");
        put (utoa (tw.id, b, 10));
        put (" ");
    }
    q = tw_sum ? tw_sum : tw.qty;
    if (tw_n > 1 || (q == 1 && (twk->form & KF_COUNT))) {   /* (Alike, told together, "two sacks"; one of a lot, "a
                                                **   ration": as many's words them) */
        if ((q = tw_n > 1 ? tw_n : 1) == 1) {
            f = (twk->flags & K_PLURAL && !(twk->form & KF_COUNT) ? LN_SOME : LN_A) | (twk->form & KF_COUNT ? LN_SINGLE :
                0);
        } else {
            f = (q <= 10 && !(twk->form & KF_COUNT) ? LN_FEW : LN_COUNT) | (twk->flags & K_PLURAL ? 0 : LN_PLURAL);
        }
    } else {
        f = q > 1 ? (twk->form & KF_LIQUID ? LN_PINTS : LN_COUNT) : twk->flags & K_PLURAL ? 0 : LN_A;   /* (Water's, lamp
                                                **   oil's pints: H14, 2026-11-19) */
    }
    s = !tw.dur_max ? 0 : tw.dur * 10 < tw.dur_max ? 1 : tw.dur * 3 < tw.dur_max ? 2 : 0;   /* (Nearly broken, battered) */
    s |= tw.flags & I_LIT ? 4 : 0;
    if (tw.id && tw.fuel <= BURN_LOW && (tw.fuel || (twk->flags & K_LIGHT) || (unsigned char) (tw.kind - KI_OIL) <=
        KI_PITCH - KI_OIL) && burn_of (tw.kind)) {  /* (A light spent or low, a fuel empty: slice 4.1's; only a
                                                **   light's or a fuel's spent, burn_of not asked of the rest) */
        s |= (!(twk->flags & K_LIGHT) ? (tw.fuel ? 0 : 1) : !tw.fuel ? 2 : tw.flags & I_LIT ? 3 : 4) << 3;   /* (Empty,
                                                **   spent, burning low, nearly spent) */
    }
    w = tw.wear < 32 ? 0 : !(twk->flags & K_BELT) && (twk->slots == FM_SWORD || twk->slots == FM_KNIFE) ? (tw.wear >= 128 ?
        1 : 2) : MT_TEXTILE (tw.mat) && tw.wear >= 128 ? (tw.wear == CL_BLOODY ? 3 : 4) : 5;   /* (Its wear: a blade dull,
                                                **   dulled; a cloth bloody, soiled (HOUSE.md's bandages, 2026-11-10);
                                                **   worn) */
    if (c == KF_STOPPER ? tw.id && !(tw.shut & IS_CLOSED) : c && (tw.shut & IS_CLOSED)) {
        w |= c == KF_STOPPER ? 8 : 16;          /* (What closes it: a stopper out; a lid, a tie, shut) */
    }
    tbuf[0] = SAY_LI;
    tbuf[1] = f | 0x80;
    tbuf[2] = s | 0x80;
    tbuf[3] = w | 0x80;
    tbuf[4] = q | 0x80;
    tbuf[5] = q >> 7 | 0x80;
    tbuf[6] = 0;
    put (tbuf);
    put (twk->name);
    put ("\2");
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

/* Each thing at where, x worded, and what's in it, or on it */
static unsigned char lsig, lkf, lwo, ln, lin, lsn;
static unsigned char lfilt;                     /* (The one compartment this pass tells, 0xFF all: contents' depth's) */
unsigned char cprep = 0xFF;                     /* (A compartment told's words: "on it" 1, "in it" 0, by its kind 0xFF) */
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
    r = cl_p->qty == 1 && (cl_p->flags & I_WORN) == lwo && !(cl_p->flags & (I_NAMED | I_HELD)) && sig (cl_p) == lsig &&
        (!lwo || cl_p->sn == lsn) &&            /* (Worn in the same place: two rings on the left hand, 2026-11-10) */
        (lfilt == 0xFF || ic_of (jn) == lfilt);    /* (In the one compartment listed: contents') */
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
    unsigned char first = 1, nt = 0, gk = 0, c, lf = (depth >> 4) - 1;   /* (depth's high bits the one compartment
                                                **   told plus 1, 0 all: comp_open's cfilt, listed's lst_c) */

    depth &= 15;

    while (gcur = gk, nx_n = i, nx_w = where, nx_x = x, (i = next_in ()) != 0) {  /* (From its place: what's in it,
                                                **   and alike's, looked elsewhere) */
        gk = gcur;
        if ((lfilt = lf) != 0xFF && ic_of (i) != lf) {
            continue;                           /* (Another compartment's: listed's lst_c, a chest's lid's top) */
        }
        fetch (&tw, i);
        twk = kind_of (tw.kind);
        lkf = twk->flags;
        lkf = lkf & (K_CONTAINER | K_BELT) || twk->keeps & KF_ON;
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
            lsn = tw.sn;
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
        if (lmode == 2) {                       /* (The inventory: a list, one a line, what's in a thing under it, each
                                                **   with its letter, the user's, 2026-11-06) */
            put (first && !depth ? "You carry:\n" : "\n");
            put ("\t\t\t\t\t\t" + (depth < 3 ? 6 - 2 * depth : 0));   /* (Hard spaces, two a level: term.c's say;
                                                **   the answer's letters the fewer at the console's pace) */
            let_put (i);
        } else if (depth == 1) {
            put (first ? "" : ", ");
        } else {
            put (first ? (cprep != 0xFF ? cprep : (cl_k = kind_at (x), cl_k->flags & K_BELT || cl_k->keeps & KF_ON)) ?
                " (on it: " : " (in it: " : ", ");      /* (A compartment's own: a chest's lid's top, on it) */
        }
        first = 0;
        tw_n = ln;
        words ();
        tw_n = 0;
        tw_sum = 0;
        if (lkf && (twk->keeps & KF_CLOSES) == KF_STOPPER && (tw.shut & IS_CLOSED)) {   /* (A vessel stoppered: how full,
                                                **   by its heft, 2026-11-03) */
            nx_n = 0;                           /* (By the list gathered, not the store gone through: dq0answer's) */
            nx_w = IW_ITEM;
            nx_x = i;
            put (!(jn = next_in ()) ? " (empty)" : item_at (jn)->qty >= twk->cap_bulk ? " (full)" : "");
        }
        if (!depth) {
            if (tw.flags & I_WORN) {
                worn_put (i, WH_LIST);          /* (Where: ", on your left hand", part 107's, 2026-11-10) */
            } else {
                put (tw.flags & I_HELD ? hands_in[tw.sn & IH_BOTH] : "");  /* (The hand it's in, its sn: slice 4.1b's) */
            }
        }
        for (c = lkf ? 0 : 0xFF; c != 0xFF && (c = comp_open (i, c)) != 0xFF; ++c) {   /* (lkf the deeper call's after it) */   /* (Each of its compartments that's open (closed:
                                                **   what's in it not told), a holder of several's each its own, "(in it:
                                                **   a sack) (on it: a chisel)": the reader of a thing's place by its
                                                **   compartment, the user's, 2026-11-16) */
            contents (IW_ITEM, i, depth + 1 | cfilt);
        }
        cprep = 0xFF;
    }
    if (!first && lmode == 1 && depth > 1) {
        put (")");
    }
    if (!depth && first) {
        put ("You carry nothing.");
    }
}

void s_list (void)
{
    char b[6];

    text_new ();
    lmode = 2;
    contents (IW_ACTOR, 0, 0);
    lmode = 0;
    put ("\nIn all, ");                         /* (Its load: the referee's encumbrance of it, the UI's to word) */
    put (utoa ((load_qp + WT_LB / 2) / WT_LB, b, 10));   /* (As it was last weighed, the referee's load, the one path:
                                                **   a weapon's version and all, 2026-11-03) */
    put (" lb.");
}

#pragma code-name (push, "SW105CODE")
#pragma rodata-name (push, "SW105RODATA")
/* What the character carries listed (IO_LIST): gathered first, then worded by part 11's s_list (part 105's, part 11's
** room: the letters, 2026-11-06; a part of its own, its swaps the shorter) */
void s_inventory (void)
{
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
    list ();
}
#pragma rodata-name (pop)
#pragma code-name (pop)

/* What's in thing n (or on it, hanging from it) worded as a list, alike told together ("a candle, two chisels":
** part 36's inside, its words in a sentence) */
void __fastcall__ s_listed (unsigned n)
{
    lmode = 1;
    ngot = 0xFF;                                /* (No list gathered: next_in's passes) */
    contents (IW_ITEM, n, 1 | (unsigned char) ((lst_c + 1) << 4));
    lmode = 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 27 (dq0w.c's, the lights burning; its room): a light renewed */
#pragma code-name (push, "SW100CODE")            /* (Part 100's since 2026-11-04, the swap study's: part 27's half, seldom run,
                                                **   that every minute's look copied) */
#pragma rodata-name (push, "SW100RODATA")

SAY_PUT (say100)

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
    unsigned n, c, room, give, n2, h = 0;
    struct item* lp;
    unsigned char takes, k, bk = 0;
    char ln[14], fn[14];

    if ((k = water (W_FILL)) != IR_NONE) {
        return k;                               /* (A waterskin filled: part 74's, slice 4.9's) */
    }
    text_new ();
    if (rs_redo & RR_DIP) {
        n = rs_old;                             /* (Its steps under way: the one they began on, whatever's burnt since) */
    } else {
        for (k = 1, room = 0; !(iq.sel & 15) && k < 8; ++k) {   /* (Of several of the name, none said, the one with the
                                                **   most room for more: the spent torch, not the one lit) */
            wsel = SL_NTH | k << 4;
            if (!(n = at_hand (iq.name))) {
                break;
            }
            fetch (&l, n);
            if ((b = burn_of (l.kind)) != 0 && (give = b->life * 10) > l.fuel && give - l.fuel > room) {
                room = give - l.fuel;
                bk = k;
            }
        }
        wsel = bk ? SL_NTH | bk << 4 : iq.sel;
        if (!(n = at_hand (iq.name))) {
            say100 (*iq.name ? SA_NO_THING : SA_FILL_WHAT, "", "");
            return IR_NO;
        }
    }
    make_own (n);
    fetch (&l, n);
    strcpy (ln, kind_of (l.kind)->name);
    if (!(kind_of (l.kind)->flags & K_LIGHT) || (iq.to & IO_SHARP)) {
        handled = n;                            /* (A blade sharpened, "fill" its words' verb, said so of anything: part
                                                **   85's: HOUSE.md's) */
        return reach_do (RO_HONE);
    }
    if (!(b = burn_of (l.kind)) || !(takes = b->fuels)) {
        say100 (SA_NO_FUEL, ln, "");
        return IR_NO;
    }
    room = (give = b->life * 10) > l.fuel ? give - l.fuel : 0;
    wsel = iq.osel;                             /* (The fuel's own selector) */
    if (*iq.into) {
        c = at_hand (iq.into);
    } else {
        for (c = 0; (c = items (c, IN_ACTOR | IN_ITEM, 0)) != 0; ) {    /* (The first carried it takes, not empty) */
            fetch (&f, c);
            if (f.fuel && ((f.kind == KI_LAMP_OIL && (takes & FU_OIL)) || ((b = burn_of (f.kind)) != 0 && (b->fuels & takes) &&
                !(kind_of (f.kind)->flags & K_LIGHT)))) {   /* (Lamp oil in what it carries too: H14's) */
                for (n2 = c; item_at (n2)->where == IW_ITEM; n2 = ip->x) {
                }
                if (n2 == rs_pack || (ip->where == IW_ACTOR && !ip->x)) {
                    break;                      /* (The character's: on it, or in what it carries) */
                }
            }
        }
    }
    if (!c) {
        say100 (SA_NO_FUELS, "", takes & FU_PITCH ? "oil, or pitch" : "a flask of oil");
        return IR_NO;
    }
    make_own (c);
    fetch (&f, c);
    if ((takes & FU_OIL) && (kind_of (f.kind)->flags & K_CONTAINER)) {
        for (n2 = 0; (n2 = items (n2, IN_X | IN_ITEM, c)) != 0 && (IC_OF (ip->y) || ip->kind != KI_LAMP_OIL); ) {
        }
        if (n2) {
            c = n2;                             /* ("From the vial": the lamp oil in it, H14's) */
            fetch (&f, c);
        }
    }
    if (f.kind == KI_LAMP_OIL && f.where == IW_ITEM) {
        h = f.x;                                /* (Its vessel: reached for, named) */
    }
    strcpy (fn, kind_of (h ? item_at (h)->kind : f.kind)->name);
    if (f.kind == KI_LAMP_OIL ? !(takes & FU_OIL) : (kind_of (f.kind)->flags & K_LIGHT) || !(b = burn_of (f.kind)) ||
        !(b->fuels & takes)) {
        say100 (SA_WONT_BURN, ln, "");
        return IR_NO;
    }
    if (!f.fuel) {
        say100 (SA_EMPTY, "", fn);
        return IR_NO;
    }
    if (!room) {
        say100 (SA_FULL, ln, "");
        return IR_NO;
    }
    if (!(iq.to & IO_DONE)) {
        handled = n;                            /* (The light filled never put away to free a hand for its fuel: the
                                                **   hands' rule's, part 90's, the user's, 2026-11-17) */
        if ((takes & FU_PITCH) && reach_do (RO_REDIP) != IR_OK) {
            return IR_NO;                       /* (A torch dipped: what comes first, part 86's) */
        }
        if (reach_step (h ? h : c, RS_TOOL)) {
            return IR_NO;                       /* (Its fuel got out: a tool's steps; put away after the filling) */
        }
        reach_from (n);                         /* (The light and its fuel got out first: their reach, HOUSE.md's) */
        reach_from (h ? h : c);
    }
    hub_map (0);
    HUB->ireq.x = takes == FU_OIL ? 200 : 100;
    if (!(iq.to & IO_DONE)) {
        HUB->ireq.qty = takes & FU_PITCH ? LT_DIP : LT_FILL;   /* (Begun: its first words the UI's, a torch dipped, a lantern
                                                **   filled; two torches, part 86's the old) */
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
    lp = item_at (c);
    lp->fuel = f.fuel - give;
    if (f.kind == KI_LAMP_OIL && !(lp->qty = (lp->fuel + OIL_PINT - 1) / OIL_PINT)) {
        lp->kind = 0;                           /* (Lamp oil's pints as it's used; none left, gone: H14's) */
    }
    say100 ((takes == FU_OIL ? SA_FILL : SA_SOAK) + (give == f.fuel), ln, fn);  /* (Its fuel used up: "and it's empty now") */
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
    if (q.where == IW_SQUARE && m >= MT_LINEN) {
        q.wear = CL_SOILED;                     /* (A cloth found lying in the dungeon soiled: HOUSE.md's bandages) */
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
        if (q.where == IW_SQUARE || (q.where == IW_ITEM && !(q.flags & I_TENDED) && item_at (q.x)->kind == KI_SCONCE)) {
                                                /* (Found lying, or cold in a sconce: part of it used before it was left, a
                                                **   sixteenth less a century of its stratum's age; a candle some left still, a
                                                **   lantern's oil dried, a torch's pitch crumbled) */
            a = STRATUM (q.sn).age;
            h = h % 12 + 2;
            h = h > a ? h - a : !b->fuels || q.where != IW_SQUARE;   /* (A sconce's part burnt, its last sixteenth) */
            life = life * h / 16;
        }
        q.fuel = life;
    }
    if ((m = k->keeps & KF_CLOSES) != 0) {     /* (What closes it, the user's, 2026-10-21: one found closed, a
                                                **   chest's lid locked one time in three, by its number, no dice; the
                                                **   character's own open, but what's stoppered) */
        q.shut = q.where == IW_SQUARE || m == KF_STOPPER ? IS_CLOSED : 0;
        if (q.kind == KI_CHEST || q.kind == KI_CASE) {   /* (A case's lid a chest's: furniture's, 2026-11-16) */
            q.shut |= IS_LOCK | (q.where == IW_SQUARE && (unsigned char) (id * 53 + (id >> 2)) % 3 == 0 ? IS_LOCKED : 0);
        }
    }
    if ((k = kind_of (q.kind))->flags & K_CONTAINER || k->keeps & KF_ON) {
        own_comps (&q);                         /* (Its compartments past the first: part 61's) */
    }
    memcpy (item_at (i), &q, sizeof q);
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
    unsigned x0, y0, i, prev = 0, box = 0;
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
        if (e.kind == KI_CHEST) {
            box = i;                            /* (The maker's found key's chest: below) */
        }
        if (e.kind == KI_KEY) {                 /* (A found key: the lock near it the maker chose, its door's number, or
                                                **   the chest before it in the list locked to fit it, HOUSE.md's keys
                                                **   that fit, A17) */
            if (e.size) {
                p->size = e.make;
                p->thick = e.size;
            } else if (box) {
                kit_lock (box, i);
            }
        } else if (e.make && e.make < 16) {     /* (Coins of an era of old, the maker's: their mint, whole, part 63's
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
#pragma code-name (push, "SW101CODE")
#pragma rodata-name (push, "SW101RODATA")
unsigned char __fastcall__ s_hide (const char* name)
{
    struct req* r;
    signed char g;
    unsigned char d;
    unsigned n;

    if (iq.to & IO_ROPE) {                      /* (A rope's work, its shape this one's: part 96's) */
        look_more (0, LM_ROPES);
        return rope ();
    }
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
    RS_TEST_PURPOSE (*r) = P_CONCEAL;
    RS_TEST_HOW (*r) = 0;
    RS_TEST_STEPS (*r) = 0;
    r = hub_ask (SLOT_UI);
    g = RS_TEST_GRADE (*r);
    r->state = REQ_FREE;
    d = g == G_WELL ? 12 : g == G_PASS ? 6 : g == G_FAIL ? 2 : 0;
    item_at (dropped)->flags |= I_HIDDEN | I_KNOWN;
    item_at (dropped)->hide = d;
    hub_map (0);
    HUB->ireq.qty = d;
    return IR_OK;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

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

/* Kind k's compartment c copied out into crow (comp_of's, the main RAM's, here for its room): 1; 0 if it has none (c
** past its last, or a kind that holds nothing); a kind with no rows (a theme's) its record's own, compartment 0 */
unsigned char __fastcall__ o_comp_of (unsigned k, unsigned char c)
{
    static const struct comp* p;
    static const struct kind* q;

    for (p = comps; p->kind && p->kind != k; ++p) {
    }
    if (p->kind) {
        for (; c && p->kind == k; ++p, --c) {
        }
        if (p->kind != k) {
            return 0;
        }
        memcpy (&crow, p, sizeof crow);
        return 1;
    }
    q = KIND_K (k);
    if (c || (!(q->flags & (K_CONTAINER | K_BELT)) && !(q->keeps & KF_ON))) {
        return 0;
    }
    crow.kind = k;
    crow.prep = q->flags & K_BELT ? CP_HUNG : q->keeps & KF_ON ? CP_ON : CP_IN;
    crow.keeps = q->keeps & ~(KF_ON | KF_FIXED);
    crow.bulk = q->cap_bulk;
    crow.weight = q->cap_weight;
    crow.fam = q->slots;
    crow.noun = 0;
    return 1;
}

/* The compartment of kind k name names by its own name (comp_named's): its number, its row in crow; 0 none */
unsigned char __fastcall__ o_comp_named (unsigned k, const char* name)
{
    static unsigned char c;

    for (c = 1; o_comp_of (k, c); ++c) {
        if (*cnouns[crow.noun & CN_MASK] && named (cnouns[crow.noun & CN_MASK], name)) {
            return c;
        }
    }
    return 0;
}

/* Thing i's first compartment from c on that's open, its row in crow (comp_open's): its number, 0xFF none; cfilt, a
** list's filter for it (0 for a holder of one), cprep its words ("on it" 1, "in it" 0; a holder of one, 0xFF: by its
** kind) */
unsigned char __fastcall__ o_comp_open (unsigned i, unsigned char c)
{
    static unsigned char m;

    FETCH (i);
    m = o_comp_of (o.kind, 1);
    for (; o_comp_of (o.kind, c); ++c) {
        if (!(o.shut & IS_SHUT_C (c)) && !CS_UNSEEN (crow.noun, o.fuel, c)) {
            cfilt = m ? c + 1 << 4 : 0;
            cprep = m ? crow.prep != CP_IN : 0xFF;
            return c;
        }
    }
    return 0xFF;
}

/* Its words (SA_*: items.h's) put in the hub's text, as a code and the two names (said.txt's SA rows the UI's: narration
** as data, stage 5, the user's, 2026-11-12; kinds.c's said[] the words before) */
static void __fastcall__ say (unsigned char n)
{
    static char* d;

    last_sa = n;
    d = tbuf;
    *d++ = n & 0x80 ? SAY_AT2 : SAY_AT;         /* (Past 127, SAY_AT2's: SA_NOT_ON, 2026-11-17) */
    *d++ = n | 0x80;
    strcpy (d, xn);
    d += strlen (d);
    *d++ = SAY_SEP;
    strcpy (d, yn);
    d += strlen (d);
    *d++ = SAY_SEP;
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

/* A thing's kind's name, as it's said (x: the thing's, else the other's), told apart by its state from another of its
** kind at hand ("the spent torch": apart's, the user's playtest, 2026-11-06), if that fits */
static void __fastcall__ name_of (unsigned i, unsigned char x)
{
    m2 (name_apart, i, (unsigned) (x ? xn : yn));
}

/* Thing n put somewhere (where, x: items.h's), as it's flagged (held, worn, or neither), no longer hidden (nor
** tended: a keepers' light, taken, burns down); its flags else (set, and cleared) */
static void __fastcall__ o_place (unsigned n, unsigned char where, unsigned x, unsigned char flags)
{
    FETCH (n);
    o.where = where;
    o.x = x;
    o.flags = (o.flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | flags;
    if (where == IW_ITEM && ccomp) {
        o.y = IC_MARK | crow.prep << 4 | ccomp; /* (Its compartment, its prep: stow's, o_fits' crow) */
    } else if (o.y >= IC_MARK) {
        o.y = 0;
    }
    m2 (store, (unsigned) &o, n);
}

/* The compartment of holder c a thing's put in, by the prep said (iq's to 1: on it): its first that's on it (or in
** it), else its first (a sack's "on" its inside, a table's "in" its top, as they always were) */
static unsigned char __fastcall__ cfor (unsigned c)
{
    static unsigned char k, i;

    k = B (m1 (item_kind, c));
    for (i = 0; B (m2 (comp_of, k, i)); ++i) {
        if (crow.prep == (iq.to & 1 ? CP_ON : CP_IN)) {
            return i;
        }
    }
    return 0;
}

static void __fastcall__ o_flags (unsigned n, unsigned char set, unsigned char clear)
{
    FETCH (n);
    o.flags = (o.flags & ~clear) | set;
    m2 (store, (unsigned) &o, n);
}

/* Whether thing n can go in c's compartment ccomp (in it, on it: a surface; hung from it: a belt): 0, or the words why
** not (SA_*): its form (a basket won't keep sand, nor water; a sack won't keep water), lit (a flame in a pack), its room
** (a belt's slots), the weight it bears, the compartment's own (crow, comp_of's: left for o_place); never into itself,
** nor into what's in it, nor on a lid's top with its lid up */
static unsigned char __fastcall__ o_fits (unsigned n, unsigned c)
{
    static const struct kind* k;
    static unsigned j, bear, b, w;
    static unsigned char kf, form, lit, cp, keeps, room, slots, fam, lid;

    k = KIND (n);
    kf = k->flags;
    form = k->form & KF_FORMS ? k->form & KF_FORMS : KF_SOLID;
    fam = k->slots;
    FETCH (n);
    lit = o.flags & I_LIT;
    if (!B (m2 (comp_of, B (m1 (item_kind, c)), ccomp))) {
        return SA_NOT_HOLDER;                   /* (It holds nothing) */
    }
    cp = crow.prep;
    keeps = crow.keeps;
    room = crow.bulk;
    bear = crow.weight;
    slots = crow.fam;
    lid = (crow.noun & CN_MASK) == CN_LID;
    for (j = c; j; j = o.where == IW_ITEM ? o.x : 0) {
        if (j == n) {
            return SA_ITSELF;
        }
        FETCH (j);
    }
    if (cp == CP_HUNG) {
        m1 (bulk_in, c);
        return !(kf & K_HANG) ? SA_NO_HANG : nin >= slots ? SA_NO_SLOT : 0;
    }
    if (cp == CP_IN) {
        if (!(keeps & form) || (slots && fam != slots)) {   /* (A carrier its family's alone: a scabbard a sword's, a
                                                **   quiver arrows', outfits' kinds.md, 2026-10-27) */
            return form == KF_SOLID || slots ? SA_WONT_GO : SA_RUN_OUT;
        } else if (lit) {
            return SA_PUT_OUT;
        }
    }
    FETCH (c);
    if (o.shut & IS_SHUT_C (ccomp)) {
        return SA_SHUT;                         /* (Closed: its lid down, its tie tied, its stopper in) */
    }
    if (lid && !(o.shut & IS_CLOSED)) {
        return SA_LID_OPEN;                     /* (A lid's top, its lid up: nothing would stay there, the user's,
                                                **   2026-11-17) */
    }
    for (j = 0, b = w = 0; (j = m2 (items_in, j, c)) != 0; ) {
        if (B (m1 (icomp, j)) == ccomp) {       /* (What's in that compartment alone) */
            b += m1 (bulk_of, j);
            w += m1 (weight_of, j);
        }
    }
    if (b + m1 (bulk_of, n) > room) {
        return SA_NO_ROOM;
    }
    return w + m1 (weight_of, n) > bear ? SA_TOO_HEAVY : 0;
}

/* The other thing (iq's into: carried, or within reach), named in yn: its number; else 0, its words why.  Named by
** a compartment of its ("the drawer"), that one in ocomp, its own name in yn; else ocomp 0 */
static unsigned char ocomp;
static unsigned other (void)
{
    static unsigned c, f;

    f = wforce;                                 /* (The first's, a count's: not this one's) */
    wforce = 0;
    wsel = iq.osel;                             /* (Its own selector; of the same name, not the first, nor one in it:
                                                **   "put the sack in the sack", the user's) */
    wnot = strcmp (iq.into, iq.name) ? 0 : handled;
    wcn = 1;                                    /* (Or one of its compartments by its own name: "the side pocket") */
    c = m1 (at_hand, (unsigned) iq.into);
    ocomp = c ? wcomp : 0;
    wcn = 0;
    wsel = iq.sel;
    wnot = 0;
    wforce = f;
    if (ocomp) {
        strcpy (yn, cnouns[crow.noun & CN_MASK]);   /* ("You put the coin in the side pocket.") */
    } else if (c) {
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
    static unsigned char r, cc;

    m0 (about);
    if (!put || !*iq.into) {
        if (!(c = put ? belt : pack)) {
            return no (put ? SA_NO_BELT : SA_NO_PACK);
        }
        name_of (c, 0);
    } else if (!(c = other ())) {
        return IR_NO;
    }
    cc = put && *iq.into ? ocomp ? ocomp : cfor (c) : 0;    /* (Which of its compartments: the one named, else in it,
                                                **   on it, as said) */
    FETCH (n);
    if (o.where == IW_ITEM && o.x == c && B (m1 (icomp, n)) == cc) {
        return no (SA_THERE);
    }
    ccomp = cc;
    r = o_fits (n, c);
    ccomp = 0;
    if (r) {
        return no (r);
    }
    rs_cc = cc;                                 /* (The reach's way in: that compartment's, part 56's) */
    r = B (m2 (reach_step, n, c));
    rs_cc = 0;
    if (r) {
        return IR_NO;                           /* (A step of the reach first: part 85's) */
    }
    m2 (comp_of, B (m1 (item_kind, c)), cc);    /* (Its row, crow: its prep, o_place's and the words') */
    ccomp = cc;
    o_place (n, IW_ITEM, c, 0);
    ccomp = 0;
    cont = c;
    cont_c = cc;
    return yes (crow.prep == CP_HUNG ? SA_HANG : crow.prep == CP_ON && B (m1 (item_kind, c)) != KI_SCONCE ? SA_PUT_ON :
        c == pack ? SA_STOW : SA_PUT_IN, 20);   /* (A sconce's: in it) */
}

/* The thing named (iq's name) in the other thing (iq's into: other's, in c), or on it: its number, named in xn; else
** 0, its words why */
static unsigned c;
static unsigned in_other (void)
{
    static unsigned n;
    static unsigned char s, k;

    if (!(c = other ())) {
        return 0;
    }
    FETCH (c);
    s = o.shut;
    k = o.kind;
    if ((s & IS_CLOSED) && !B (m2 (comp_of, k, 1))) {
        no (SA_SHUT);                           /* (Its one compartment closed: nothing in it looked for) */
        return 0;
    }
    win = c;                                    /* (In it, by name, of several the one the selector says; in the
                                                **   compartment named, if one is) */
    wcin = ocomp;
    handled = n = m2 (pick, (unsigned) iq.name, HW_IN);
    wcin = 0;
    if (n ? s & IS_SHUT_C (B (m1 (icomp, n))) : s & IS_SHUT_C (ocomp)) {
        no (SA_SHUT);                           /* (In one closed: a chest's inside, its lid shut, not on its lid) */
        return 0;
    }
    if (!n) {                                   /* (On a thing that holds things on it, a table's, a shelf's, a lid's
                                                **   top named: "on the shelves", not "in", 2026-11-17) */
        no ((ocomp ? (m2 (comp_of, k, ocomp), crow.prep == CP_ON) : !(KFLAGS (k) & K_CONTAINER)) ? SA_NOT_ON : SA_NOT_IN);
        return 0;
    }
    name_of (n, 1);
    return n;
}

/* A thing carried (cr, carry's: 1 into the pack, else a hand; 0 none free), its words (s first, then where it went),
** its time t; take_from's and doff's (one tail, 2026-11-09: the catalogue's bank's room) */
static unsigned char cr;
static unsigned char __fastcall__ carried (unsigned char s, unsigned char t)
{
    if (!cr) {
        return rs_st ? IR_NO : no (SA_HANDS);   /* (A step of the reach first: part 85's) */
    }
    say (s);
    name_of (cont, 0);                          /* (Where carry put it: the pack, the purse) */
    return yes (cr == 1 ? SA_IN_PACK : SA_IN_HAND, t);
}

/* Taken from the other thing: into the pack, else a hand (from what's on the character: into a hand) */
static unsigned char take_from (void)
{
    static unsigned n;

    if (!(n = in_other ())) {
        return IR_NO;
    }
    m1 (reach_from, n);
    if (B (m1 (on_pc, c))) {
        return hold (n);
    }
    cr = B (m2 (carry, n, !iq.count));          /* (In a hand, the pack only with both full or by the count: the user's, 2026-11-06) */
    return carried (SA_TAKE, 20);
}

/* Worn (a thing held in the hand, not worn: held, equip's; "wear"'s, to DN_WORN, refused, 2026-11-09) */
static unsigned char __fastcall__ don (unsigned n)
{
    static unsigned char k, kf;

    k = B (m1 (item_kind, n));
    kf = KFLAGS (k);
    if (!(kf & K_WEAR)) {
        return kf & K_HAND && !(iq.to & DN_WORN) ? hold (n) : no (SA_NO_WEAR);
    }
    if ((k = B (m2 (worn_at, n, k))) != 0) {   /* (Worn already; one of a kind worn, a second cloak; where it's worn, its
                                                **   room there: part 107's, 2026-11-10, worn places) */
        return k == 0xFF ? IR_NO : no (k);
    }
    if (B (m2 (reach_step, n, 0))) {
        return IR_NO;
    }
    o_place (n, IW_ACTOR, 0, I_WORN);
    o.sn = wplace;                              /* (Its place: WP_NONE, one worn the one way) */
    m2 (store, (unsigned) &o, n);
    return yes (SA_DON, 50);
}

/* Taken off: into a hand, else the pack */
static unsigned char __fastcall__ doff (unsigned n)
{
    FETCH (n);
    if (!(o.flags & I_WORN)) {
        return no (SA_NOT_WORN);
    }
    cr = B (m2 (carry, n, 1));
    return carried (SA_DOFF, 50);
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
#define n               handled                 /* (The thing to light what's handled: not put away for its fire-maker, part 90's) */
static unsigned char __fastcall__ light (unsigned char on)
{
    static unsigned c, nf;
    static unsigned char r, x, fr, f, hh;

    c = 0;
    fr = on && wsel == SL_NEW;                 /* ("Light a new torch", the user's, 2026-11-02: HOUSE.md's */
    if (fr && (rs_redo & RR_NEW)) {             /*   a new light lit from the old one, its next step, part 86's) */
        m0 (text_new);
        return B (m1 (reach_do, RO_NEWL));
    }
    for (n = 1; n < NITEMS && *iq.name != '#' && !*iq.into; ++n) {  /*   a new light: the old let fall, a spare got out, lit) */
        FETCH (n);
        if (o.kind && (KFLAGS (o.kind) & K_LIGHT) && !(o.flags & I_LIT) == on && (!on || !spent ()) &&
            (!*iq.name || named (KIND_K (o.kind)->name, iq.name)) && B (m1 (on_pc, n))) {
            if (o.flags & I_HELD) {
                break;                          /* (One in a hand first, no pack off for it, none put away: 2026-11-04) */
            }
            if (!c) {
                c = n;
            }
        }
    }
    if (n >= NITEMS && c) {
        n = c;
    } else if (n >= NITEMS || *iq.name == '#' || *iq.into) {
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
    nf = o.where == IW_SQUARE && o.id ? o.fuel : 0xFFFF;   /* (One lying within reach: new only if it has more left
                                                **   than the one burning, the old one let fall no spare yet) */
    hh = o.flags & I_HELD;
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
        return yes (SA_DOUSE, 10);              /* (A second, as the redip's: main.c's after) */
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
            if (c != n && o.kind && o.where == IW_ACTOR && !o.x && (o.flags & I_HELD) && (KFLAGS (o.kind) & K_LIGHT) &&
                named (KIND_K (o.kind)->name, iq.name)) {
                if ((o.flags & I_LIT) && o.fuel >= nf) {
                    return no (SA_NO_SPARE);
                }
                if ((o.flags & I_LIT) && rs_ok == 1) {  /* (Burning, by the steps: lit from it, part 86's) */
                    rs_old = c;
                    rs_tool = n;
                    rs_redo |= RR_NEW;
                    m0 (text_new);
                    return B (m1 (reach_do, RO_NEWL));
                }
                rstep.thing = c;
                rstep.edge = SE_DROP;
                rs_hand = 0;
                rstep.say = o.flags & I_LIT ? SV_LETBURN : SV_LETFALL;
                rstep.what = o.kind;
                rstep.time = 1;
                if (B (m2 (reach_step, 0, RS_GO))) {
                    return IR_NO;
                }
                break;
            }
        }
    }
    x = !hh && (fr || (x && B (m1 (on_pc, n))));
    if (x && B (m1 (hand_free, n)) && B (m2 (reach_step, n, 0))) {
        return IR_NO;                           /* (The spare got out first, past its hands' refusal, hold's) */
    }
    if (f == 3) {
        m1 (reach_from, rs_tool);
    }
    if (f == 3 && (f = B (m2 (reach_step, rs_tool, RS_FIRE))) != 0) {
        if (f == 2) {
            name_of (rs_tool, 0);               /* (Found empty as it's used: told, known from now on) */
            return no (SA_EMPTY);
        }
        return IR_NO;                           /* (The fire-maker's steps first: part 85's) */
    }
    if (x && (m1 (reach_from, n), (r = hold (n)) != IR_OK)) {
        return r;                               /* (Out of the pack first, into a hand: its reach) */
    }
    m0 (text_new);                              /* (Its words alone) */
    o_flags (n, I_LIT, 0);
    return yes (SA_LIGHT, 0);
}
#undef n


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
    wcn = op == IO_FIND;                        /* (The UI's other to hand: a compartment by its own name too, "put the
                                                **   coin in the side pocket") */
    n = m1 (op == IO_DOFF ? (void*) carried_named : (void*) at_hand, (unsigned) xn);
    wcn = 0;
    if (!n) {
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
    RS_TEST_PURPOSE (*r) = P_SEARCH;
    RS_TEST_HOW (*r) = HIDE_MECHANICAL;
    RS_TEST_STEPS (*r) = ease - (signed char) rec.hide - (touch ? 2 : 0);
    r = hub_ask (SLOT_UI);
    g = RS_TEST_GRADE (*r);
    r->state = REQ_FREE;
    return g >= G_PASS;
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
            if (item_at (sitem)->shut & IS_CLOSED ? reach_step (sitem, 0) : reach_step (0, sitem)) {
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

/* The referee's search test of a hidden compartment of the thing in rec (a false bottom, a secret drawer: the user's,
** 2026-11-16), the steps worse the better it's made, 2 more by touch, as hidden things' (part 46's turned_up): 1 found */
static unsigned char __fastcall__ found_c (unsigned char touch)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_UI];
    r->kind = RS_TEST;
    r->actor = 0;
    RS_TEST_PURPOSE (*r) = P_SEARCH;
    RS_TEST_HOW (*r) = HIDE_MECHANICAL;
    RS_TEST_STEPS (*r) = -(signed char) (rec.quality / 3) - (touch ? 2 : 0);
    r = hub_ask (SLOT_UI);
    g = RS_TEST_GRADE (*r);
    r->state = REQ_FREE;
    return g >= G_PASS;
}

unsigned char __fastcall__ s_on_it (unsigned n, unsigned char touch)
{
    unsigned char on;
    unsigned j;
    char cn[CN_LEN + 2];

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
    lst_c = comp_of (rec.kind, 1) ? 0 : 0xFF;   /* (Its compartments each, step 6b's: its inside, then its lid's top) */
    for (j = 0; (j = items (j, IN_X | IN_ITEM, n)) != 0 && lst_c != 0xFF && icomp (j); ) {
    }
    if (rec.shut & IS_CLOSED) {
        put (lpl ? "They're closed: you go over the outside of them." : "It's closed: you go over the outside of it.");
    } else if (j) {
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
    for (on = 1; lst_c != 0xFF && (fetch (&rec, n), comp_of (rec.kind, on)); ++on) {   /* (Its other compartments,
                                                **   each that holds something: a chest's lid's top, open to the hand) */
        strcpy (cn, crow.prep == CP_IN && (crow.noun & CN_MASK) ? cnouns[crow.noun & CN_MASK] : lk.name);
        if ((crow.noun & CC_INNER) && (rec.shut & IS_CLOSED)) {
            continue;                           /* (Reached through its first, closed: a false bottom) */
        }
        if (CS_UNSEEN (crow.noun, rec.fuel, on)) {
            if (!(rec.fuel & CS_AT (on, CS_NONE)) && found_c (touch)) {
                item_at (n)->fuel |= CS_AT (on, CS_FOUND);  /* (A hidden one found, the referee's test: known from now
                                                **   on, the user's, 2026-11-16) */
                put (" You find a ");
                put (cn);
                put (" in the ");
                put (lk.name);
                put (".");
            }
            continue;
        }
        if (rec.shut & IS_SHUT_C (on)) {
            continue;                           /* (Closed: a drawer shut, not looked in) */
        }
        for (j = 0; (j = items (j, IN_X | IN_ITEM, n)) != 0 && icomp (j) != on; ) {
        }
        if (j) {
            comp_of (rec.kind, on);
            put (crow.prep == CP_ON ? " On the " : " In the ");
            put (cn);
            put (" you find ");
            lst_c = on;
            listed (n);
            put (".");
        }
    }
    lst_c = 0xFF;
    if (iq.to & IOS_THING) {
        if (rec.mat != MT_OTHER) {
            if ((lk.form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) {
                rec.size = rec.thick = 0;       /* (A pile of coins: no measure, s_look_at's) */
            }
            tqe = tbuf + sizeof tbuf - 1;
            handled = n;
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

/* The hand that holds the thing that strikes (s, the hub's pc_strike: SK_OF its kind), for the hub's pc_shand: the
** main hand's, if it holds one of that kind, else the other's; 0 none (a part of it, or the best it holds: the
** referee's) */
unsigned char __fastcall__ s_shand_of (unsigned char s)
{
    s_held_kinds ();
    return !SK_IS_KIND (s) ? 0 : held_k[0] == SK_KIND (s) ? held_h[0] : held_k[1] == SK_KIND (s) ? held_h[1] : 0;
}

/* The character's hands as they are, but for thing n (n held: its own hand, hcur) */
void __fastcall__ s_hands_now (unsigned n)
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
static unsigned hrs;
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

SAY_PUT (say48)

/* A thing worn taken off with no hand free and no room in the pack (the doff's SA_HANDS, the catalogue's bank's, its room none):
** set down at the character's feet, said so, as "drop" would (the user's playtest, 2026-11-06: "take off the backpack" refused
** with its hands full, "drop backpack" not): the pack, a cloak, a helm, a belt, a ring alike.  Its time the doff's */
unsigned char __fastcall__ s_doff_feet (unsigned n)
{
    struct item* p;
    unsigned x, y;
    unsigned char sn;

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
    sky_ask (n);                                /* (On a shaft's head: it falls, part 17's) */
    text_new ();
    say48 (SA_DOFF_FEET, kind_of (item_kind (n))->name, "");
    took = 50;
    return IR_OK;
}

/* The hand thing n would go in, the one that suits it (carry's: a thing taken up, its hand first, else the pack); 0
** none free; | HF_FIXT, it's in a fixture */
unsigned char __fastcall__ s_hand_free (unsigned n)
{
    const struct item* p = item_at (n);
    unsigned char f = p->where == IW_ITEM && (kind_at (p->x)->keeps & KF_FIXED) ? HF_FIXT : 0;

    s_hands_now (n);
    return hand_for (n, 0) | f;
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

/* A hold's words (SA_HOLD, SA_SHIFT, SA_HELD), coded as the catalogue's say codes its own: the thing's name, then the
** hand's words, h's, the other's (the UI's said.txt's SA rows: "You take the dagger in your right hand.") */
static void __fastcall__ hold_said (unsigned char c, unsigned char h)
{
    tbuf[0] = SAY_AT;
    tbuf[1] = c | 0x80;
    tbuf[2] = 0;
    put (tbuf);
    put (xn);
    put ("\2");
    hand_words (h);
    put ("\2");
}

/* Why not, in words (hwhy's) */
static void why_not (void)
{
    const struct kind* k;

    const char* h = hword[hside == IH_BOTH ? IH_LEFT : hside];

    if (hwhy == HW_FULL || hwhy == HW_TWO) {
        say48 (hwhy == HW_FULL ? SA_HANDS : SA_BOTH_HANDS, xn, "");   /* (SA.hands: the same words, one row) */
        return;
    }
    if (hwhy == HW_LIMB) {
        say48 (SA_ARM, "", h);
        if (htwo) {
            say48 (SA_ONE_HAND, xn, "");
        }
        return;
    }
    k = kind_of (hkind[hside == IH_RIGHT]);
    say48 (SA_HAND_HOLDS, k->name, h);
}

/* Thing n taken into a hand (IO_HOLD's, the catalogue's bank's hold: from anywhere on the character, or within reach;
** its name in xn): the one want says (IH_*, IH_OTHER: the request's to), or, none said, the one that suits it; one held
** already moved to it.  Its words into the hub's text: IR_OK (its time, took) or IR_NO (why not) */
#pragma register-vars (push, on)
unsigned char __fastcall__ s_hold_in (unsigned n, unsigned char want)
{
    register struct item* p;
    unsigned char h;

    s_hands_now (n);
    if (hcur && (!want || want == hcur || (want == IH_OTHER && hcur == IH_BOTH))) {
        hold_said (SA_HELD, hcur);              /* (Held already: in that hand, or none said) */
        return IR_NO;
    }
    if (!(h = hand_for (n, want))) {
        if ((hwhy == HW_FULL || hwhy == HW_TWO) && (rs_ok == 1 || rs_ok == 2)) {
            hrs = rs_tool;                      /* (The tool in use kept: a torch got out as its tinderbox is in hand) */
            rs_tool = n;
            h = rs_more (0);
            rs_tool = hrs;
            if (h) {
                return IR_NO;                   /* (Its hands made free first, a step at a time: A25's, part 90's) */
            }
        }
        why_not ();
        return IR_NO;
    }
    if (reach_step (n, 0)) {
        return IR_NO;                           /* (A step of the reach first: part 85's) */
    }
    p = item_at (n);
    p->where = IW_ACTOR;
    p->x = 0;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | I_KNOWN | I_HELD;
    p->sn = h;
    hold_said (hcur ? SA_SHIFT : SA_HOLD, h);
    took = 10;
    return IR_OK;
}
#pragma register-vars (pop)

/* What's in the character's hands changed over, left to right and right to left (IO_SWAP's: "swap hands"), its words
** into the hub's text, its time in the request's x: IR_OK, or IR_NO (nothing held, a thing in both hands, a hand to go
** into useless) */
void __fastcall__ hands_now (unsigned n);       /* (Its stub: part 48's) */
#pragma code-name (push, "SW101CODE")
#pragma rodata-name (push, "SW101RODATA")
SAY_PUT (say101)

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
                say101 (SA_BOTH_YOUR, kind_of (ip->kind)->name, "");
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
        say101 (SA_HANDS_EMPTY, "", "");
        return IR_NO;
    }
    if ((l && (hdead & IH_RIGHT)) || (r && (hdead & IH_LEFT))) {
        say101 (SA_ARM, "", hdead & IH_RIGHT ? "right" : "left");
        return IR_NO;
    }
    if (l) {
        item_at (l)->sn = IH_RIGHT;
    }
    if (r) {
        item_at (r)->sn = IH_LEFT;
    }
    if (l && r) {
        strcpy (xn, kind_of (hkind[1])->name);  /* (Two names: the left's copied out first, kind_of's one copy) */
        say101 (SA_CHANGE, xn, kind_of (hkind[0])->name);
    } else {
        say101 (SA_SHIFT, kind_of (hkind[!l])->name, l ? "your right hand" : "your left hand");
    }
    hub_map (0);
    HUB->ireq.x = 10;
    return IR_OK;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

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

unsigned char __fastcall__ look_more (unsigned n, unsigned char mode);    /* (Part 40's: LM_KIT's kit_keep) */
void s_kit (void)
{
    unsigned made[NKIT];
    const unsigned char* e;
    const struct kind* kk;
    unsigned i, box = 0;
    unsigned char k, off, old, carried;
    char* c;
    char* more = tbuf;

    hub_map (0);
    k = HUB->gear & GE_OLD ? 0 : outfit ();
    carried = k == OF_CARRIED;
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
        if (k == OF_CARRIED) {                  /* (Its kit carried on: each thing as it was, its wear, its life, its version,
                                                **   its fuel, outfit.h's kitrec: it's the first made in the store, in the
                                                **   file's order, what wasn't made passed over; coins as they're changed) */
            for (i = 1; read (ost.kept, tbuf, sizeof (struct kitrec)) == sizeof (struct kitrec); ) {
                if (item_at (i)->kind == ((struct kitrec*) tbuf)->o.kind) {
                    if (((struct kitrec*) tbuf)->dur_max && !(kind_at (i)->form & KF_SMALL)) {
                        item_at (i)->wear = ((struct kitrec*) tbuf)->wear;
                        memcpy (&item_at (i)->dur, &((struct kitrec*) tbuf)->dur, KIT_STATE);
                    }
                    if ((k = ((struct kitrec*) tbuf)->o.in) & WP_KIT && ((struct kitrec*) tbuf)->o.place == PL_WORN) {
                        item_at (i)->sn = WP_AT | (k & (WP_LOC | WP_PAIR)) | (k & 0x20 ? WP_RIGHT : 0);   /* (Where it was
                                                **   worn: kit_put's, 2026-11-10) */
                    }
                    if ((k = ((struct kitrec*) tbuf)->let - 1) < NLET) {
                        box = item_at (i)->id;  /* (Its letter as it was: hub.h's designators, by its id, or by its
                                                **   record's number till it has one) */
                        hub_map (UI_BANK);
                        UIREC->low[k] = box ? box : i | LT_REC;
                    }
                    ++i;
                }
            }
            close (ost.kept);
            box = 0;
        } else if (--k) {                       /* (Its weapons their version: the referee's, by its Strength, D13's;
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
        if (i != 0xFFFF && ((k = item_at (i)->kind) == KI_CHEST || k == KI_CASE || k == KI_DESK)) {
            box = i;                            /* (A case's, a desk's drawer's too: furniture's, 2026-11-16) */                            /* (A key after a chest locks it, and fits it alone: the outfits'
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
    hub_map (0);
    if (!carried && HUB->savedir[0]) {          /* (A life begun, a folder: what it sets out with kept now, kit0.dat, before anything's used: kit_keep's) */
        look_more (0, LM_KIT);
        hub_map (0);
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

static unsigned char cm_n, cm_era[TMINTS];      /* (The theme's mints: how many, each one's era) */
/* (Each one's ways, and a pile's coins by mint, counts: part 64's own, each used with no part swapped in between, as
** s_mix_words' puts do; the main RAM's room, 2026-11-18) */
#pragma bss-name (push, "SW64RODATA")
static unsigned char cm_ways[TMINTS];
static unsigned cm_c[TMINTS + 1];
#pragma bss-name (pop)
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
#pragma bss-name (push, "SW64RODATA")          /* (Its locals its part's own: no part swapped in while it runs;
                                                **   the main RAM's room, 2026-11-18) */
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
#pragma bss-name (pop)

/* Thing i's mix, if it's a pile of coins with none yet: today's (its numbers', no die: four coins in five, a fifth more
** or less, the region's own, the rest another's of today, a neighbour's or the dwarves'; a crown the gold one's, the
** dwarves') */
#pragma bss-name (push, "SW64RODATA")          /* (Its locals its part's own: no part swapped in while it runs;
                                                **   the main RAM's room, 2026-11-18) */
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
#pragma bss-name (pop)

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
