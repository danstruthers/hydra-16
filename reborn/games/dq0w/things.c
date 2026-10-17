/*
** things.c - (dq0w) the item store (DUNGEON.md: items; items.h), the world's alone (hub.h's who writes what): the
** catalogue, read into the hub as the world starts (kinds.c's, from its file: a stand-in for the theme's data), the
** theme's kinds after it (the UI's request); the items' records kept, moved as the UI asks (the hub's ireq: the
** character's kit, a thing taken up, put down; held in a hand, stowed in the pack, put in a container or on a
** surface, hung from the belt, taken out again, worn, taken off; lit, put out), listed (what the character carries,
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
** its chunks' things read in, put out; begun, kept).
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
unsigned char __fastcall__ named (const char* k, const char* w);
unsigned char __fastcall__ on_pc (unsigned i);
unsigned __fastcall__ at_hand (const char* name);
unsigned __fastcall__ carried_named (const char* name);
unsigned __fastcall__ seen_named (const char* name);
unsigned char __fastcall__ in_reach (unsigned n);
unsigned __fastcall__ weight_in (unsigned where, unsigned x);
unsigned __fastcall__ weight_of (unsigned i);
unsigned __fastcall__ bulk_in (unsigned x);
unsigned __fastcall__ bulk_of (unsigned i);
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);
void about (void);
void list (void);                               /* Part 11 */
void __fastcall__ wear (const char* key, unsigned char by);
unsigned char search (void);
unsigned char __fastcall__ describe (const char* name);
unsigned char __fastcall__ hide (const char* name);
void __fastcall__ make_own (unsigned i);         /* Part 13 */
void seen_owned (void);
void kit (void);
void theme_kinds (void);

#define NK      NKINDS                          /* (The catalogue's, and the theme's after them: theme.h) */

extern const struct kind kinds[];               /* (kinds.c's: in the catalogue's bank) */

/* A buffer in the main RAM (the words said, the kit's names, the store kept a piece at a time) */
static char tbuf[80];

/* A thing's record, copied out: the one the functions share that call none that wants it while they use it (others
** have their own: make_own's, the recursive ones', light's ...) */
static struct item rec;

/* (item_at and items are items.s's, in assembly: its store's layout as here) */
typedef char store_fits[sizeof (struct item) == 22 && ITEM_BANK (0) == 14 && ITEM_BANKS == 2 ? 1 : -1];

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

/* The hands a thing wants: both for a two-handed one, or a big one */
static unsigned char __fastcall__ two (const struct kind* k)
{
    return k->flags & K_TWO || k->bulk >= 20 ? 2 : 1;
}

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
static void __fastcall__ put (const char* s)
{
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

/* The request, copied (serve's), and what its handling shares (the hands, the pack, the belt: about's; the last put
** down; how far the thing named is, seen_named's; out of reach, at_hand's; the things in a container, bulk_in's; the
** time it took) */
static struct ireq iq;
static unsigned pack, belt, dropped, seen_d, took;
static unsigned char hands, afar, nin;

/* ==== Part 10: the requests answered, a thing handled (and what the catalogue's bank's code calls on: its stubs) */
#pragma code-name (push, "SW10CODE")
#pragma rodata-name (push, "SW10RODATA")

/* A name said: a word of the kind's ("key" an iron key; "sword" a broken one too), or its start */
unsigned char __fastcall__ s_named (const char* k, const char* w)
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

/* The thing in rec (fetched) named so: by its kind's name (named's), or by the name the character gave it (the UI's
** "#" and its id: hub.h's struct tname) */
static unsigned char __fastcall__ is_named (const char* w)
{
    char b[6];

    return *w == '#' ? !strcmp (utoa (rec.id, b, 10), w + 1) : s_named (kind_of (rec.kind)->name, w);
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


/* The thing named that the character sees, the nearest (on a square it sees now, or on a surface there): its number,
** or 0; how far it is, in squares, in seen_d */
unsigned __fastcall__ s_seen_named (const char* name)
{
    const struct item* s;
    unsigned i = 0, best = 0, bd = 0xFFFF, d, x, y, e;
    unsigned char sn;
    unsigned char* p;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    while ((i = items (i, IN_SQUARE | IN_ITEM | IN_PORTAL, 0)) != 0) {
        fetch (&rec, i);
        if (!(s = lying (&rec)) || s->sn != sn || (rec.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN ||
            !is_named (name) || s->x - x + 20 > 40 || s->y - y + 20 > 40 ||
            !(p = at (sn, s->x, s->y)) || !(*p & T_VIS)) {
            continue;
        }
        d = s->x > x ? s->x - x : x - s->x;
        e = s->y > y ? s->y - y : y - s->y;
        if ((d = d > e ? d : e) < bd) {
            seen_d = bd = d;
            best = i;
        }
    }
    return best;
}

/* The first thing named the character carries: its number, or 0 */
unsigned __fastcall__ s_carried_named (const char* name)
{
    unsigned i = 0;

    while ((i = items (i, IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {
        fetch (&rec, i);
        if (is_named (name) && s_on_pc (i)) {
            return i;
        }
    }
    return 0;
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
    return s && s->x - x + 1 <= 2 && s->y - y + 1 <= 2;
}

/* The thing named that's to hand: carried, or seen within reach (afar: seen, but out of reach): its number, or 0 */
unsigned __fastcall__ s_at_hand (const char* name)
{
    unsigned n;

    afar = 0;
    if (!*name) {
        return 0;
    }
    if ((n = s_carried_named (name)) != 0) {
        return n;
    }
    if ((n = s_seen_named (name)) != 0 && !(afar = !s_in_reach (n))) {
        return n;
    }
    return 0;
}

/* ---- What's in a thing, or on the character: the bulk and weight (quarter pounds) of what's in it (a thing's own
** with what's in it, or on it: a soft thing's bulk its own or what it holds, the more) */
unsigned __fastcall__ s_weight_in (unsigned where, unsigned x);
unsigned __fastcall__ s_bulk_in (unsigned x);
unsigned __fastcall__ s_weight_of (unsigned i)
{
    const struct kind* k = kind_at (i);
    unsigned w = (unsigned) k->weight * (k->flags & K_PLURAL ? 1 : item_at (i)->qty);

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

/* ---- The character's hands, its pack, its belt (about's), and where a thing will go */
void s_about (void)
{
    const struct kind* k;
    unsigned i = 0;

    pack = belt = hands = 0;
    while ((i = items (i, IN_X | IN_ACTOR, 0)) != 0) {
        fetch (&rec, i);
        k = kind_of (rec.kind);
        if ((rec.flags & I_WORN) && (k->flags & K_CONTAINER) && !pack) {
            pack = i;
        }
        if ((rec.flags & I_WORN) && (k->flags & K_BELT) && !belt) {
            belt = i;
        }
        if (rec.flags & I_HELD) {
            hands += two (k);
        }
    }
}

/* Thing n put somewhere (where, x: items.h's), as it's flagged (held, worn, or neither), no longer hidden */
static void __fastcall__ place (unsigned n, unsigned char where, unsigned x, unsigned char flags)
{
    struct item* p = item_at (n);

    p->where = where;
    p->x = x;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN)) | I_KNOWN | flags;
}

/* Thing n carried: into the pack if it fits, else in a hand (hand first: the other way about): 1 in the pack, 2 in
** a hand, 0 neither (full) */
unsigned char __fastcall__ s_carry (unsigned n, unsigned char hand_first)
{
    unsigned char h;

    s_about ();
    h = hands + two (kind_at (n)) <= 2;
    if ((!hand_first || !h) && pack && !fits (n, pack)) {
        place (n, IW_ITEM, pack, 0);
        return 1;
    }
    if (h) {
        place (n, IW_ACTOR, 0, I_HELD);
        return 2;
    }
    return 0;
}

/* ---- The tools: the keys of all the character carries (anywhere on it: a skill used, the one that suits takes
** itself out), into the hub's things_text, each after its mark (* held, + worn, a space neither: what's held fights,
** what's worn guards) and its quality after it (a digit: rs.h's RS_EQUIP), and its load (the weight of all it
** carries, worn or not, in quarter pounds: the referee's encumbrance) in the request's x; and one used, worn (the
** referee's choice, by its key), its name back in the request (bare hands: "hands") */
static void tools (void)
{
    char m[2];
    const struct kind* k;
    unsigned i = 0;
    char* t;

    m[1] = 0;

    hub_map (UI_BANK);
    UIREC->things_text[0] = 0;
    while ((i = items (i, IN_ACTOR | IN_ITEM | IN_PORTAL, 0)) != 0) {
        fetch (&rec, i);
        k = kind_of (rec.kind);                 /* (Copied: the hub's text is in another bank) */
        if (k->tool[0] && s_on_pc (i)) {
            hub_map (UI_BANK);
            t = UIREC->things_text;
            if (strlen (t) < sizeof UIREC->things_text - 11) {
                m[0] = rec.flags & I_HELD ? '*' : rec.flags & I_WORN ? '+' : ' ';
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
}

/* ---- Taken up: into the backpack if it fits (its room, its weight, what it keeps), else in a hand (two, at most);
** not within reach: IR_NOT_HERE, and where it is (the hub's ireq's x, y: the UI walks there, and asks again) */
static unsigned char __fastcall__ get (const char* name)
{
    const struct item* s;
    unsigned n = s_seen_named (name), x, y;

    if (!n) {
        return IR_NONE;
    }
    fetch (&rec, n);
    s = lying (&rec);
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    HUB->ireq.x = s->x;                         /* (Where it is: IR_NOT_HERE's, IR_FIXED's; and which, I_TAKE's) */
    HUB->ireq.y = s->y;
    HUB->ireq.item = n;
    if (s->x - x + 1 > 2 || s->y - y + 1 > 2) {
        return IR_NOT_HERE;
    }
    return rec.where == IW_PORTAL ? IR_FIXED : kind_of (rec.kind)->keeps & KF_FIXED ? IR_NO : s_carry (n, 0) ?
        IR_OK : IR_FULL;                        /* (A portal's part: lifted out first, the world's; a theme's altar,
                                                **   never: its words the handling's) */
}

/* Put down where the character stands (what's in it with it) */
unsigned char __fastcall__ s_drop (const char* name)
{
    unsigned char sn;
    unsigned x, y, n = s_carried_named (name);

    if (!n) {
        return IR_NONE;
    }
    fetch (&rec, n);
    if (!rec.kind || rec.where == IW_SQUARE) {
        return IR_NONE;
    }
    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    item_at (n)->where = IW_SQUARE;
    item_at (n)->sn = sn;
    item_at (n)->x = x;
    item_at (n)->y = y;
    item_at (n)->flags &= ~(I_HELD | I_WORN);
    dropped = n;
    return IR_OK;
}

/* The character's light: the brightest lit thing it carries (in a hand, worn, hung from its belt: one stowed is
** put out), into the hub, named */
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
}

/* ---- A request (the hub's ireq), answered (but for its state, the caller's: done once it's looked again): 1 if
** what lies about has changed */
static unsigned char o_handle (void);
unsigned char s_serve (void)
{
    unsigned char changed = 0;

    hub_map (0);
    memcpy (&iq, &HUB->ireq, sizeof iq);
    iq.result = IR_OK;
    switch (iq.kind) {
    case IO_KIT:
        kit ();
        break;
    case IO_GET:
        if ((iq.result = get (iq.name)) == IR_NO) {
            cat_map ();                         /* (Fixed where it stands: the words why) */
            o_handle ();
        }
        break;
    case IO_DROP:
        iq.result = s_drop (iq.name);
        break;
    case IO_LIST:
        list ();
        break;
    case IO_TOOLS:
        tools ();
        break;
    case IO_WEAR:
        wear (iq.name, iq.to);
        break;
    case IO_SEARCH:
        changed = iq.result = search ();
        break;
    case IO_DESCRIBE:
        iq.result = describe (iq.name);
        break;
    case IO_HIDE:
        iq.result = hide (iq.name);
        break;
    case IO_THEME:
        theme_kinds ();
        break;
    case IO_BEGIN:
        break;
    default:                                    /* (A thing handled: IO_HOLD ...) */
        cat_map ();
        iq.result = o_handle ();
        hub_map (0);
        HUB->ireq.x = took;
        break;
    }
    if (iq.kind == IO_KIT || iq.kind == IO_GET || iq.kind == IO_DROP || iq.kind == IO_HIDE ||
        (iq.kind >= IO_HOLD && iq.kind <= IO_NAME)) {
        s_light_now ();                         /* (Its light: what it holds lit) */
        changed |= iq.result == IR_OK;
    }
    hub_map (0);
    HUB->ireq.result = iq.result;
    return changed && iq.kind != IO_KIT;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 11: what's carried listed, a search, a thing described, a thing hidden */
#pragma code-name (push, "SW11CODE")
#pragma rodata-name (push, "SW11RODATA")

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

/* ---- What the character carries, worded into the hub's text (things_text): each thing, and what's in it */

/* A thing in words: its kind, how many, and what anyone can see of its state (its make and worth are an appraiser's
** to judge) */
#pragma static-locals (push, on)                /* (Called again by nothing before it's done: quicker) */
void __fastcall__ s_thing_words (const struct item* q)
{
    const struct kind* k = kind_of (q->kind);
    char b[6];

    if (q->flags & I_NAMED) {                   /* (A name the character gave it: its id, marked for the UI, "#12 a
                                                **   sword", worded "Linda, your sword") */
        put ("#");
        put (utoa (q->id, b, 10));
        put (" ");
    }
    if (q->qty > 1) {
        put (utoa (q->qty, b, 10));
        put (" ");
    } else if (!(k->flags & K_PLURAL)) {
        put (strchr ("aeiou", k->name[0]) ? "an " : "a ");
    }
    put (k->name);
    if (q->dur_max && q->dur * 10 < q->dur_max) {
        put (" (nearly broken)");
    } else if (q->dur_max && q->dur * 3 < q->dur_max) {
        put (" (battered)");
    }
    if (q->flags & I_LIT) {
        put (" (lit)");
    }
    if (q->wear >= 32) {                        /* (Its wear: a blade dulled, a tool worn) */
        put (!strcmp (k->tool, "blade") || !strcmp (k->tool, "knife") ? (q->wear >= 128 ? " (dull)" : " (dulled)") :
            " (worn)");
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
static void __fastcall__ contents (unsigned char where, unsigned x, unsigned char depth)
{
    const struct kind* k;
    struct item q;
    unsigned i = 0;
    unsigned char first = 1;

    while ((i = next_in (i, where, x)) != 0) {
        fetch (&q, i);
        k = kind_of (q.kind);
        load += (unsigned) k->weight * (k->flags & K_PLURAL ? 1 : q.qty);
        if (!depth) {
            put (first ? "You carry " : "; ");
        } else {
            put (first ? (kind_at (x)->flags & K_BELT || kind_at (x)->keeps & KF_ON ?
                " (on it: " : " (in it: ") : ", ");
        }
        first = 0;
        s_thing_words (&q);
        if (!depth) {
            put (q.flags & I_HELD ? (two (k) == 2 ? ", in both hands" : ", in your hand") :
                q.flags & I_WORN ? ", worn" : "");
        }
        if (k->flags & (K_CONTAINER | K_BELT) || k->keeps & KF_ON) {     /* (k kind_of's still: three more
                                                **   since, of its four) */
            contents (IW_ITEM, i, depth + 1);
        }
    }
    if (!first && depth) {
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
    put (utoa ((load + 2) / 4, b, 10));
    put (" lb.");
}

/* ---- A search for what's concealed about the character (on the squares it sees, within SEARCH_R): the referee's
** test for each (its search, at the concealment's difficulty: two steps of it each, from +2), the ones found no
** longer hidden, and named in the hub's text; but what's hidden by magic (found by magical means only) */
#define SEARCH_R        3
unsigned char s_search (void)
{
    struct req* r;
    unsigned char* p;
    unsigned i = 0, x, y;
    unsigned char sn, found = 0;
    signed char g;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    text_new ();
    while ((i = items (i, IN_SQUARE, 0)) != 0) {
        fetch (&rec, i);
        if (rec.sn != sn ||
            (rec.flags & (I_HIDDEN | I_MAGIC | I_KNOWN)) != I_HIDDEN ||
            rec.x - x + SEARCH_R > 2 * SEARCH_R || rec.y - y + SEARCH_R > 2 * SEARCH_R || !(p = at (sn, rec.x, rec.y)) ||
            !(*p & T_VIS)) {
            continue;
        }
        make_own (i);
        fetch (&rec, i);
        hub_map (0);
        r = &HUB->slot[SLOT_UI];                /* (The UI's: it waits on this request meanwhile) */
        r->kind = RS_TEST;
        r->actor = 0;
        r->a = P_SEARCH;
        r->b = HIDE_MECHANICAL;
        r->steps = 2 - 2 * rec.hide;
        r = hub_ask (SLOT_UI);
        g = r->grade;
        r->state = REQ_FREE;
        if (g >= G_MS) {
            item_at (i)->flags = (rec.flags & ~I_HIDDEN) | I_KNOWN;
            put (found ? ", and " : "Searching, you turn up ");
            s_thing_words (&rec);
            ++found;
        }
    }
    if (found) {
        put (".");
    }
    return found;
}

/* ---- A thing looked at closely (carried, or seen): what it truly is, in the request, for the appraiser's judgement
** (the UI's, by the referee's test of the craft that knows it); and how far off it is, in squares (0 carried), its
** kind's size above it (the request's to: the UI's, what it makes out at a distance) */
unsigned char __fastcall__ s_describe (const char* name)
{
    const struct kind* k;
    struct ireq* r;
    unsigned n = carried_named (name);

    seen_d = 0;
    if (!n && !(n = seen_named (name))) {
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
    r->to = k->size << 8 | seen_d;
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
    unsigned id;
    unsigned char m;

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
        q.mat = m;
    }
    q.dur_max = tough[m] * (k->weight + 4) * (5 + q.quality) / 5;
    q.dur = q.where == IW_SQUARE ? q.dur_max - q.dur_max / 10 * rnd (7) : q.dur_max;
    if (!q.dur) {
        q.dur = 1;
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
** (dq0 -k, commas between: in the hub's text), each held if there's a hand for it, else in the pack */

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
    unsigned char k;
    char* c;
    char* more = tbuf;

    hub_map (UI_BANK);
    strncpy (more, UIREC->things_text, 63);     /* (The things more: the UI's -k, in the UI bank's text) */
    hub_map (0);
    more[63] = 0;
    for (k = 0; k < NKIT; ++k) {
        e = kit_list[k];
        made[k] = i = item_new (e[0], e[1] == 0xFF ? IW_ACTOR : IW_ITEM, e[1] == 0xFF ? 0 : made[e[1]], e[2]);
        item_at (i)->flags = e[3];
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
        }
    }
}

/* Cache slot k's chunk's things put down, as the maker listed them stocking it (the world bank's stock: a thing on a
** square of the chunk, concealed or not; a portal's part, in its portal or by it, its make the portal's), in the
** list's order (the hub's header in the window after) */
void __fastcall__ s_stock_down (unsigned char k)
{
    struct sthing e;
    struct item* p;
    unsigned x0, y0, i;
    unsigned char sn, n, j, m;

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
        if (!(i = put_down (e.kind, e.hide == ST_PORTAL ? IW_PORTAL : IW_SQUARE, sn, x0 + e.x, y0 + e.y, e.qty))) {
            break;                              /* (The store full: no more) */
        }
        p = item_at (i);
        if (e.hide < ST_PORTAL) {
            p->flags = I_HIDDEN;
            p->hide = e.hide;
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

/* A denizen's remains put down where it fell (kind: KI_BODY, KI_BONES; the world's, wbody.c's: it died) */
void __fastcall__ s_lay_body (unsigned char kind, unsigned char sn, unsigned x, unsigned y)
{
    it_need (sn, (x - STRATUM (sn).x0) >> CW_BITS, (y - STRATUM (sn).y0) >> CW_BITS);  /* (Its chunk's things in the
                                                **   store first) */
    put_down (kind, IW_SQUARE, sn, x, y, 1);
    hub_map (0);
}

/* (Part 13's: a rare command's) Hidden where the character stands (the user: hiding is a skill, Concealment; a thing's difficulty to find by the
** method's quality and the concealer's skill): its difficulty, the method's 1 (a floor's dust and corners) and the
** referee's test's (rs.h's P_CONCEAL); known to the character, who hid it */
unsigned char __fastcall__ drop (const char* name);
unsigned char __fastcall__ s_hide (const char* name)
{
    struct req* r;
    signed char g;
    unsigned char d;

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
    d = 1 + (g == G_CS ? 6 : g == G_MS ? 3 : g == G_MF ? 1 : 0);
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
static char xn[16], yn[16];                     /* (The two names said: the words asked, then the things') */

#define B(v)            ((unsigned char) (v))   /* (A char's result) */
#define KIND(i)         (&kinds[B (m1 (item_kind, i))])
#define FETCH(i)        m2 (fetch, (unsigned) &o, i)

static void __fastcall__ say (unsigned char n)
{
    static const char* s;
    static char* d;

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

/* Thing n put somewhere (where, x: items.h's), as it's flagged (held, worn, or neither), no longer hidden; its
** flags else (set, and cleared) */
static void __fastcall__ o_place (unsigned n, unsigned char where, unsigned x, unsigned char flags)
{
    FETCH (n);
    o.where = where;
    o.x = x;
    o.flags = (o.flags & ~(I_HELD | I_WORN | I_HIDDEN)) | I_KNOWN | flags;
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
    static unsigned j;
    static unsigned char kf, form, lit, cf, keeps, room, bear, slots;

    k = KIND (n);
    kf = k->flags;
    form = k->form ? k->form : KF_SOLID;
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
    if (m1 (bulk_in, c) + m1 (bulk_of, n) > room) {
        return SA_NO_ROOM;
    }
    return m2 (weight_in, IW_ITEM, c) + m1 (weight_of, n) > bear * 4 ? SA_TOO_HEAVY : 0;
}

/* The other thing (iq's into: carried, or within reach), named in yn: its number; else 0, its words why */
static unsigned other (void)
{
    static unsigned c;

    if ((c = m1 (at_hand, (unsigned) iq.into)) != 0) {
        name_of (c, 0);
    } else {
        strcpy (xn, iq.into);
        no (afar ? SA_REACH : SA_NO_OTHER);
    }
    return c;
}

/* Into a hand (from anywhere on the character, or within reach) */
static unsigned char __fastcall__ hold (unsigned n)
{
    static unsigned char t;

    t = two (KIND (n));
    m0 (about);
    FETCH (n);
    if ((o.flags & I_HELD) && B (m1 (on_pc, n))) {
        return no (SA_HELD);
    }
    if (hands + t > 2) {
        return no (SA_HANDS);
    }
    o_place (n, IW_ACTOR, 0, I_HELD);
    return yes (t == 2 ? SA_HOLD2 : SA_HOLD, 10);
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
    return yes (cf & K_BELT ? SA_HANG : r & KF_ON ? SA_PUT_ON : c == pack ? SA_STOW : SA_PUT_IN, 20);
}

/* Taken from the other thing: into the pack, else a hand (from what's on the character: into a hand) */
static unsigned char take_from (void)
{
    static unsigned c, n, id;
    static unsigned char r;
    static const char* w;

    if (!(c = other ())) {
        return IR_NO;
    }
    id = 0;
    if (*(w = iq.name) == '#') {                /* (A name the character gave it: the UI's "#" and its id) */
        while (*++w >= '0' && *w <= '9') {
            id = id * 10 + *w - '0';
        }
    }
    for (n = 1; n < NITEMS; ++n) {
        FETCH (n);
        if (o.kind && o.where == IW_ITEM && o.x == c && (id ? o.id == id : named (kinds[o.kind].name, iq.name))) {
            break;
        }
    }
    if (n >= NITEMS) {
        return no (SA_NOT_IN);
    }
    name_of (n, 1);
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

/* ---- Light: a flame to hand (a tinderbox carried, a lit thing carried or within reach) */
static unsigned char flame (void)
{
    static unsigned i;

    for (i = 1; i < NITEMS; ++i) {
        FETCH (i);
        if (o.kind && ((o.flags & I_LIT) || !strcmp (kinds[o.kind].tool, "tinder")) &&
            (B (m1 (on_pc, i)) || ((o.flags & I_LIT) && B (m1 (in_reach, i))))) {
            return 1;
        }
    }
    return 0;
}

/* Lit, or put out (on 0): the thing named, else the first carried that would be (or one within reach, named) */
static unsigned char __fastcall__ light (unsigned char on)
{
    static unsigned n;
    static unsigned char r, x;

    if (!*iq.name) {
        for (n = 1; n < NITEMS; ++n) {
            FETCH (n);
            if (o.kind && (kinds[o.kind].flags & K_LIGHT) && !(o.flags & I_LIT) == on && B (m1 (on_pc, n))) {
                break;
            }
        }
        if (n >= NITEMS) {
            return no (on ? SA_NONE_TO_LIGHT : SA_NONE_LIT);
        }
    } else if (!(n = m1 (at_hand, (unsigned) iq.name))) {
        return no (afar ? SA_REACH : SA_NO_SUCH);
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
    if (!flame ()) {
        return no (SA_NO_FLAME);
    }
    if (x && B (m1 (on_pc, n)) && (r = hold (n)) != IR_OK) {
        return r;                               /* (Out of the pack first, into a hand) */
    }
    m0 (text_new);                              /* (Its words alone) */
    o_flags (n, I_LIT, 0);
    return yes (SA_LIGHT, 0);
}

/* A door's bar to hand (carried, or within reach: not one in another's brackets), the widest: its width (took's,
** the request's x) and IR_NO (nothing about it changed); or (iq's to 1) it set in the door's brackets, a portal's part
** again on iq's square, the bar's side (the world's barred the door) */
static unsigned char bar (void)
{
    static unsigned n, best;

    for (best = 0, n = 1; n < NITEMS; ++n) {
        FETCH (n);
        if (o.kind == KI_BAR && o.where != IW_PORTAL && (o.size & 15) > took && (B (m1 (on_pc, n)) ||
            B (m1 (in_reach, n)))) {
            best = n;
            took = o.size & 15;
        }
    }
    if (!best || !iq.to) {
        return IR_NO;
    }
    FETCH (best);
    o.where = IW_PORTAL;
    o.sn = iq.item;
    o.x = iq.x;
    o.y = iq.y;
    o.flags &= ~(I_HELD | I_WORN | I_HIDDEN);
    m2 (store, (unsigned) &o, best);
    return IR_OK;
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
        return bar ();
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
