/*
** things.c - (dq0p) the things (DUNGEON.md: items; items.h): the catalogue, written into the hub as the task starts
** (a stand-in for the theme's data); the items' records kept, moved as the UI asks (the character's kit, a thing
** taken up, put down), listed (what the character carries, worded into the hub's text for the UI to say); and the
** things the character sees, named in what it perceives (sight.c).  The item store's free records are taken under
** the hub's lock (the maker puts chunks' things down too).  Kept with the game (dq0 -d): the store's banks as they
** are, the save folder's items.dat.  The catalogue's in the hub's catalogue bank (kinds.c: read in from the task's
** overlay file as it starts), each kind copied out as it's wanted.
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "things.h"

#define NK      KI_COUNT

/* The materials' toughness (items.h's MT_*) */
static const unsigned char tough[NMATS] = MAT_TOUGH;

/* Its own numbers, for the things' differences (not the game's: those are the referee's and the maker's) */
static unsigned rs = 0x2545;
static unsigned char __fastcall__ rnd (unsigned char n)
{
    rs = rs * 25173U + 13849U;
    return (rs >> 8) % n;
}

/* ---- The store kept (back: read again), the save folder's items.dat: its banks a piece at a time, through main
** RAM (the window's only the task's own: the kernel's copies want the task's memory) */
static unsigned char piece[256];

static void __fastcall__ keep (unsigned char back)
{
    static char p[48];
    unsigned char b;
    unsigned off;
    int fd;

    hub_map (0);
    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (p, HUB->savedir);
    strcat (p, "/items.dat");
    if ((fd = back ? open (p, O_RDONLY) : open (p, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    for (b = 0; b < ITEM_BANKS; ++b) {
        for (off = 0; off < 0x2000; off += sizeof piece) {
            if (back) {
                if (read (fd, piece, sizeof piece) != sizeof piece) {
                    memset (piece, 0, sizeof piece);
                }
                hub_map (CAT_BANK + 1 + b);
                memcpy ((char*) 0x8000 + off, piece, sizeof piece);
            } else {
                hub_map (CAT_BANK + 1 + b);
                memcpy (piece, (char*) 0x8000 + off, sizeof piece);
                write (fd, piece, sizeof piece);
            }
        }
    }
    close (fd);
}

/* The catalogue read into the hub's catalogue bank (dir/dq0p.ovl: kinds.c's, linked at $8000): 0, or -1 */
static int __fastcall__ catalogue (const char* dir)
{
    static char p[48];
    unsigned off;
    int fd, n;

    strcpy (p, dir);
    strcat (p, "/dq0p.ovl");
    if ((fd = open (p, O_RDONLY)) < 0) {
        return -1;
    }
    hub_map (CAT_BANK);
    memset ((void*) 0x8000, 0, 0x2000);
    for (off = 0; off < 0x2000 && (n = read (fd, piece, sizeof piece)) > 0; off += n) {
        hub_map (CAT_BANK);
        memcpy ((char*) 0x8000 + off, piece, n);
    }
    close (fd);
    return 0;
}

int __fastcall__ things_init (const char* dir)
{
    unsigned char b;

    if (catalogue (dir)) {
        return -1;
    }
    for (b = 0; b < ITEM_BANKS; ++b) {          /* (The store empty; a game carried on, its kept) */
        hub_map (CAT_BANK + 1 + b);
        memset ((void*) 0x8000, 0, 0x2000);
    }
    hub_map (0);
    rs = (unsigned) HUB->seed | 1;
    if (HUB->resume) {
        keep (1);
    }
    return 0;
}

struct item* __fastcall__ item_at (unsigned i)
{
    hub_map (ITEM_BANK (i));
    return ITEM_AT (i);
}

/* A kind's record, copied out of the catalogue's bank (into one of four, in turn: an expression may want two at
** once): the catalogue's bank left in the window */
static const struct kind* __fastcall__ kind_of (unsigned char k)
{
    static struct kind kb[4];
    static unsigned char next;
    struct kind* r = &kb[next++ & 3];

    hub_map (CAT_BANK);
    memcpy (r, CATALOGUE + (k < NK ? k : 0), sizeof *r);
    return r;
}

/* ---- A thing made its own, as the things task first comes to it (no two alike): its number (record 0 keeps the
** last given), its quality (most are middling), its material (a blade of iron, of steel if it's well made, of
** bronze if crudely), its durability's most (its material's toughness, its weight, its quality) and what's left of
** it (a thing found lying in the dungeon is the worse for its years there) */
static void __fastcall__ make_own (unsigned i)
{
    static struct item q;
    struct item* p;
    const struct kind* k;
    unsigned id;
    unsigned char m;

    memcpy (&q, item_at (i), sizeof q);
    if (q.id || !q.kind) {
        return;
    }
    p = item_at (0);
    id = ++p->x;
    q.id = id;
    q.quality = rnd (4) + rnd (4) + rnd (3);
    if (rnd (8) == 0) {
        ++q.quality;
    }
    k = kind_of (q.kind);
    m = k->mat;
    if (m == MT_IRON && !strcmp (k->tool, "blade")) {
        m = q.quality >= 7 ? MT_STEEL : q.quality <= 1 ? MT_BRONZE : MT_IRON;
    }
    q.mat = m;
    q.dur_max = tough[m] * (k->weight + 4) * (5 + q.quality) / 5;
    q.dur = q.where == IW_SQUARE ? q.dur_max - q.dur_max / 10 * rnd (7) : q.dur_max;
    if (!q.dur) {
        q.dur = 1;
    }
    memcpy (item_at (i), &q, sizeof q);
}

/* A free record, taken (under the hub's lock): its number, or 0xFFFF */
unsigned __fastcall__ item_new (unsigned char kind, unsigned char where, unsigned char sn, unsigned x, unsigned y,
    unsigned char qty)
{
    static struct item* p;
    unsigned i;

    hub_map (0);
    hy_sem_acquire (HUB->item_lock);
    for (i = 1; i < NITEMS; ++i) {              /* (Record 0 is never one: "none") */
        p = item_at (i);
        if (!p->kind) {
            p->kind = kind;
            p->flags = 0;
            p->where = where;
            p->sn = sn;
            p->x = x;
            p->y = y;
            p->qty = qty;
            p->owner = 0;
            p->wear = 0;
            p->hide = 0;
            p->id = 0;
            break;
        }
    }
    hub_map (0);
    hy_sem_release (HUB->item_lock);
    if (i >= NITEMS) {
        return 0xFFFF;
    }
    item_at (i)->id = 0;
    make_own (i);
    return i;
}

/* A kind by its name: its number, or 0 */
static unsigned char __fastcall__ kind_named (const char* name)
{
    unsigned char k;

    for (k = 1; k < NK; ++k) {
        if (!strcmp (kind_of (k)->name, name)) {
            return k;
        }
    }
    return 0;
}

/* ---- The character's kit (a stand-in for the theme's and the occupation's): a torch lit in hand, a backpack worn
** (rations, a tinderbox in it), a belt (a pouch of pennies, a dagger, a waterskin hanging from it); and a thing more
** (dq0 -k), held */

static void __fastcall__ kit (const char* more)
{
    unsigned pack, belt, pouch, i;
    unsigned char k;

    i = item_new (1, IW_ACTOR, 0, 0, 0, 1);
    item_at (i)->flags = I_HELD | I_LIT | I_KNOWN;
    pack = item_new (2, IW_ACTOR, 0, 0, 0, 1);
    item_at (pack)->flags = I_WORN | I_KNOWN;
    item_new (7, IW_ITEM, 0, pack, 0, 3);
    item_new (8, IW_ITEM, 0, pack, 0, 1);
    belt = item_new (3, IW_ACTOR, 0, 0, 0, 1);
    item_at (belt)->flags = I_WORN | I_KNOWN;
    pouch = item_new (4, IW_ITEM, 0, belt, 0, 1);
    item_new (15, IW_ITEM, 0, pouch, 0, 12);
    item_new (5, IW_ITEM, 0, belt, 0, 1);
    item_new (6, IW_ITEM, 0, belt, 0, 1);
    if (*more && (k = kind_named (more)) != 0) {
        i = item_new (k, IW_ACTOR, 0, 0, 0, 1);
        item_at (i)->flags = I_HELD | I_KNOWN;
    }
}

/* ---- What's in a thing, or on the character: the bulk and weight (quarter pounds) of what's in it */
static unsigned __fastcall__ weight_in (unsigned char where, unsigned x)
{
    static struct item q;
    unsigned i, w = 0;

    const struct kind* k;

    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (q.kind && q.where == where && q.x == x) {
            k = kind_of (q.kind);               /* (Its own, before what's in it: q's the next call's then) */
            w += (unsigned) k->weight * (k->flags & K_PLURAL ? 1 : q.qty);
            if (k->flags & (K_CONTAINER | K_BELT)) {
                w += weight_in (IW_ITEM, i);
            }
        }
    }
    return w;
}

static unsigned char __fastcall__ bulk_in (unsigned x)
{
    static struct item q;
    unsigned i;
    unsigned char b = 0;

    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (q.kind && q.where == IW_ITEM && q.x == x) {
            b += kind_of (q.kind)->bulk;
        }
    }
    return b;
}

/* The thing named that the character sees, the nearest (on a square it sees now): its number, or 0 */
static unsigned __fastcall__ seen_named (const char* name)
{
    static struct item q;
    unsigned i, best = 0, bd = 0xFFFF, d, x, y;
    unsigned char sn, n = strlen (name);
    unsigned char* p;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (!q.kind || q.where != IW_SQUARE || q.sn != sn || (q.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN ||
            strncmp (kind_of (q.kind)->name,
            name, n) || q.x - x + 20 > 40 || q.y - y + 20 > 40 || !(p = at (sn, q.x, q.y)) || !(*p & T_VIS)) {
            continue;
        }
        d = (q.x > x ? q.x - x : x - q.x) + (q.y > y ? q.y - y : y - q.y);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

/* The character's (on it, or in what it carries) */
static unsigned char __fastcall__ on_pc (unsigned i)
{
    unsigned j;

    for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {         /* (Up to whoever has it) */
    }
    return item_at (j)->where == IW_ACTOR && !item_at (j)->x;
}

/* The first thing named the character carries: its number, or 0 */
static unsigned __fastcall__ carried_named (const char* name)
{
    static struct item q;
    unsigned i;
    unsigned char n = strlen (name);

    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (q.kind && q.where != IW_SQUARE && !strncmp (kind_of (q.kind)->name, name, n) && on_pc (i)) {
            return i;
        }
    }
    return 0;
}

/* ---- The tools: the keys of all the character carries (anywhere on it: a skill used, the one that suits takes
** itself out), into the hub's things_text, a space between; and one used, worn (the referee's choice, by its key),
** its name back in the request (bare hands: "hands") */
static void tools (void)
{
    static struct item q;
    const struct kind* k;
    unsigned i;
    char* t;

    hub_map (0);
    HUB->things_text[0] = 0;
    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        k = kind_of (q.kind);                   /* (Copied: the hub's text is in another bank) */
        if (q.kind && q.where != IW_SQUARE && k->tool[0] && on_pc (i)) {
            hub_map (0);
            t = HUB->things_text;
            if (strlen (t) < sizeof HUB->things_text - 10) {
                strcat (t, k->tool);
                strcat (t, " ");
            }
        }
    }
}

static void __fastcall__ wear (const char* key, unsigned char by)
{
    static struct item q;
    static char name[16];
    struct item* p;
    unsigned i;

    strcpy (name, key);
    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (q.kind && q.where != IW_SQUARE && !strcmp (kind_of (q.kind)->tool, key) && on_pc (i)) {
            p = item_at (i);
            p->wear = p->wear + by < p->wear ? 255 : p->wear + by;
            strcpy (name, kind_of (q.kind)->name);
            break;
        }
    }
    hub_map (0);
    strcpy (HUB->ireq.name, name);
}

/* ---- Taken up: into the backpack if it fits (its room, its weight), else in a hand (two, at most); not within
** reach: IR_NOT_HERE, and where it is (the hub's ireq's x, y: the UI walks there, and asks again) */
static unsigned char __fastcall__ get (const char* name)
{
    static struct item q, r;
    const struct kind* k;
    unsigned i, pack = 0, n = seen_named (name), x, y;
    unsigned char hands = 0;

    if (!n) {
        return IR_NONE;
    }
    memcpy (&q, item_at (n), sizeof q);
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    if (q.x - x + 1 > 2 || q.y - y + 1 > 2) {
        HUB->ireq.x = q.x;
        HUB->ireq.y = q.y;
        return IR_NOT_HERE;
    }
    k = kind_of (q.kind);
    for (i = 1; i < NITEMS; ++i) {              /* (Its backpack, and its hands' things) */
        memcpy (&r, item_at (i), sizeof r);
        if (r.kind && r.where == IW_ACTOR && !r.x) {
            if (kind_of (r.kind)->flags & K_CONTAINER && (r.flags & I_WORN)) {
                pack = i;
            }
            if (r.flags & I_HELD) {
                ++hands;
            }
        }
    }
    if (pack && bulk_in (pack) + k->bulk <= kind_of (item_at (pack)->kind)->cap_bulk &&
        weight_in (IW_ITEM, pack) + k->weight <= kind_of (item_at (pack)->kind)->cap_weight * 4) {
        item_at (n)->where = IW_ITEM;
        item_at (n)->x = pack;
    } else if (hands < 2) {
        item_at (n)->where = IW_ACTOR;
        item_at (n)->x = 0;
        item_at (n)->flags |= I_HELD;
    } else {
        return IR_FULL;
    }
    item_at (n)->flags = (item_at (n)->flags | I_KNOWN) & ~I_HIDDEN;
    return IR_OK;
}

/* Put down where the character stands (what's in it with it) */
static unsigned dropped;                        /* (The last put down) */
static unsigned char __fastcall__ drop (const char* name)
{
    static struct item q;
    unsigned char sn;
    unsigned x, y, n = carried_named (name);

    if (!n) {
        return IR_NONE;
    }
    memcpy (&q, item_at (n), sizeof q);
    if (!q.kind || q.where == IW_SQUARE) {
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

/* Hidden where the character stands (the user: hiding is a skill, Concealment; a thing's difficulty to find by the
** method's quality and the concealer's skill): its difficulty, the method's 1 (a floor's dust and corners) and the
** referee's test's (rs.h's P_CONCEAL); known to the character, who hid it */
static unsigned char __fastcall__ hide (const char* name)
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

/* ---- What the character carries, worded into the hub's text (things_text): each thing, and what's in it */

static char* tp;
static void __fastcall__ put (const char* s)
{
    hub_map (0);
    while (*s && tp < HUB->things_text + sizeof HUB->things_text - 1) {
        *tp++ = *s++;
    }
    *tp = 0;
}

/* A thing in words: its kind, how many, and what anyone can see of its state (its make and worth are an appraiser's
** to judge) */
static void __fastcall__ thing_words (const struct item* q)
{
    const struct kind* k = kind_of (q->kind);
    char b[6];

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

static void __fastcall__ contents (unsigned char where, unsigned x, unsigned char depth)
{
    static struct item q;
    unsigned i;
    unsigned char first = 1;

    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (!q.kind || q.where != where || q.x != x) {
            continue;
        }
        if (!depth) {
            put (first ? "You carry " : "; ");
        } else {
            put (first ? (kind_of (item_at (x)->kind)->flags & K_BELT ? " (on it: " : " (in it: ") : ", ");
        }
        first = 0;
        thing_words (&q);
        if (!depth) {
            put (q.flags & I_HELD ? ", in your hand" : q.flags & I_WORN ? ", worn" : "");
        }
        if (kind_of (q.kind)->flags & (K_CONTAINER | K_BELT)) {
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

static void list (void)
{
    hub_map (0);
    tp = HUB->things_text;
    *tp = 0;
    contents (IW_ACTOR, 0, 0);
}

/* ---- A search for what's concealed about the character (on the squares it sees, within SEARCH_R): the referee's
** test for each (its search, at the concealment's difficulty: two steps of it each, from +2), the ones found no
** longer hidden, and named in the hub's text; but what's hidden by magic (found by magical means only) */
#define SEARCH_R        3
static unsigned char search (void)
{
    static struct item q;
    struct req* r;
    unsigned char* p;
    unsigned i, x, y;
    unsigned char sn, found = 0;
    signed char g;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    tp = HUB->things_text;
    *tp = 0;
    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (!q.kind || q.where != IW_SQUARE || q.sn != sn || (q.flags & (I_HIDDEN | I_MAGIC | I_KNOWN)) != I_HIDDEN ||
            q.x - x + SEARCH_R > 2 * SEARCH_R || q.y - y + SEARCH_R > 2 * SEARCH_R || !(p = at (sn, q.x, q.y)) ||
            !(*p & T_VIS)) {
            continue;
        }
        make_own (i);
        memcpy (&q, item_at (i), sizeof q);
        hub_map (0);
        r = &HUB->slot[SLOT_UI];                /* (The UI's: it waits on this request meanwhile) */
        r->kind = RS_TEST;
        r->actor = 0;
        r->a = P_SEARCH;
        r->b = HIDE_MECHANICAL;
        r->steps = 2 - 2 * q.hide;
        r = hub_ask (SLOT_UI);
        g = r->grade;
        r->state = REQ_FREE;
        if (g >= G_MS) {
            item_at (i)->flags = (q.flags & ~I_HIDDEN) | I_KNOWN;
            put (found ? ", and " : "Searching, you turn up ");
            thing_words (&q);
            ++found;
        }
    }
    if (found) {
        put (".");
    }
    return found;
}

/* ---- A thing looked at closely (carried, or seen): what it truly is, in the request, for the appraiser's judgement
** (the UI's, by the referee's test of the craft that knows it) */
static unsigned char __fastcall__ describe (const char* name)
{
    static struct item q;
    const struct kind* k;
    struct ireq* r;
    unsigned n = carried_named (name);

    if (!n && !(n = seen_named (name))) {
        return IR_NONE;
    }
    make_own (n);
    memcpy (&q, item_at (n), sizeof q);
    k = kind_of (q.kind);
    hub_map (0);
    r = &HUB->ireq;
    strcpy (r->name, k->name);
    r->item = q.id;
    r->craft = k->craft;
    r->quality = q.quality;
    r->mat = q.mat;
    r->plural = (k->flags & K_PLURAL) != 0;
    r->qty = q.qty;
    r->dur = q.dur;
    r->dur_max = q.dur_max;
    r->value = k->value;
    return IR_OK;
}

/* ---- The things the character sees (sight.c's): on the squares it sees now, not hidden, named, where they lie */
void __fastcall__ things_seen (struct sight* s, unsigned char sn, unsigned x, unsigned y, unsigned char face)
{
    static struct item q;
    unsigned i;
    unsigned char* p;
    unsigned char k;

    s->nitems = 0;
    for (i = 1; i < NITEMS && s->nitems < SEE_ITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (!q.kind || q.where != IW_SQUARE || q.sn != sn || (q.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN ||
            q.x - x + 20 > 40 ||
            q.y - y + 20 > 40 || !(p = at (sn, q.x, q.y)) || !(*p & T_VIS)) {
            continue;
        }
        make_own (i);
        k = s->nitems++;
        strcpy (s->item[k].name, kind_of (q.kind)->name);
        s->item[k].qty = q.qty;
        s->item[k].plural = (kind_of (q.kind)->flags & K_PLURAL) != 0;
        s->item[k].hid = (q.flags & I_HIDDEN) != 0;
        s->item[k].rel = q.x == x && q.y == y ? 0xFF : REL8 (bearing ((int) (q.x - x), (int) (q.y - y)), face);
        s->item[k].n = i;
        s->item[k].x = q.x;
        s->item[k].y = q.y;
    }
}

/* ---- A request (the hub's ireq), answered (but for its state, the caller's: done once it's looked again): 1 if
** what lies about has changed */
unsigned char things_serve (void)
{
    static struct ireq q;
    unsigned char changed = 0;

    hub_map (0);
    memcpy (&q, &HUB->ireq, sizeof q);
    q.result = IR_OK;
    switch (q.op) {
    case IO_KIT:
        kit (q.name);
        break;
    case IO_GET:
        q.result = get (q.name);
        break;
    case IO_DROP:
        q.result = drop (q.name);
        break;
    case IO_LIST:
        list ();
        break;
    case IO_SAVE:
        keep (0);
        break;
    case IO_TOOLS:
        tools ();
        break;
    case IO_WEAR:
        wear (q.name, q.to);
        break;
    case IO_SEARCH:
        changed = q.result = search ();
        break;
    case IO_DESCRIBE:
        q.result = describe (q.name);
        break;
    case IO_HIDE:
        q.result = hide (q.name);
        break;
    }
    hub_map (0);
    HUB->ireq.result = q.result;
    return changed || (q.op == IO_GET || q.op == IO_DROP || q.op == IO_HIDE) && q.result == IR_OK;
}
