/*
** things.c - (dq0p) the things (DUNGEON.md: items; items.h): the catalogue, written into the hub as the task starts
** (a stand-in for the theme's data); the items' records kept, moved as the UI asks (the character's kit, a thing
** taken up, put down), listed (what the character carries, worded into the hub's text for the UI to say); and the
** things the character sees, named in what it perceives (sight.c).  The item store's free records are taken under
** the hub's lock (the maker puts chunks' things down too).  Kept with the game (dq0 -d): the store's banks as they
** are, the save folder's items.dat.
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "things.h"

/* The catalogue: name, flags, weight (quarter pounds), bulk (pints), its room (pints, pounds), slots, light, size,
** tool's key */
static const struct kind kinds[] = {
    { "", 0 },
    { "torch", K_LIGHT | K_HAND, 4, 2, 0, 0, 0, 4, 3, "" },
    { "backpack", K_CONTAINER | K_WEAR, 8, 20, 32, 40, 0, 0, 5, "" },
    { "belt", K_BELT | K_WEAR, 2, 1, 0, 0, 4, 0, 2, "" },
    { "pouch", K_CONTAINER | K_HANG, 1, 1, 2, 4, 0, 0, 1, "" },
    { "dagger", K_HAND | K_HANG, 4, 1, 0, 0, 0, 0, 2, "knife" },
    { "waterskin", K_HANG, 4, 4, 0, 0, 0, 0, 2, "" },
    { "rations", K_PLURAL, 4, 2, 0, 0, 0, 0, 2, "" },
    { "tinderbox", 0, 1, 1, 0, 0, 0, 0, 1, "tinder" },
    { "pick", K_HAND, 24, 6, 0, 0, 0, 0, 4, "pick" },
    { "shovel", K_HAND, 20, 8, 0, 0, 0, 0, 4, "shovel" },
    { "lockpicks", 0, 1, 1, 0, 0, 0, 0, 1, "picks" },
    { "chisel", K_HAND, 2, 1, 0, 0, 0, 0, 1, "chisel" },
    { "rope", 0, 40, 8, 0, 0, 0, 0, 4, "rope" },
    { "bones", K_PLURAL, 8, 4, 0, 0, 0, 0, 4, "" },
    { "pennies", K_PLURAL, 0, 0, 0, 0, 0, 0, 1, "" },
    { "iron key", 0, 1, 0, 0, 0, 0, 0, 1, "key" },
    { "candle", K_LIGHT | K_HAND, 1, 1, 0, 0, 0, 2, 1, "" },
    { "lantern", K_LIGHT | K_HAND, 8, 4, 0, 0, 0, 6, 3, "" },
    { "sack", K_CONTAINER, 2, 2, 24, 30, 0, 0, 3, "" },
    { "chest", K_CONTAINER, 120, 80, 60, 100, 0, 0, 7, "" },
    { "skull", 0, 4, 2, 0, 0, 0, 0, 3, "" },
    { "broken sword", 0, 12, 2, 0, 0, 0, 0, 4, "" },
    { "cloak", K_WEAR, 8, 6, 0, 0, 0, 0, 4, "" },
    { "potsherds", K_PLURAL, 2, 1, 0, 0, 0, 0, 2, "" },
};
#define NK      (sizeof kinds / sizeof kinds[0])

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

void things_init (void)
{
    unsigned char b;

    hub_map (CAT_BANK);
    memset ((void*) 0x8000, 0, NKINDS * sizeof (struct kind));
    memcpy ((void*) 0x8000, kinds, sizeof kinds);
    for (b = 0; b < ITEM_BANKS; ++b) {          /* (The store empty; a game carried on, its kept) */
        hub_map (CAT_BANK + 1 + b);
        memset ((void*) 0x8000, 0, 0x2000);
    }
    hub_map (0);
    if (HUB->resume) {
        keep (1);
    }
}

struct item* __fastcall__ item_at (unsigned i)
{
    hub_map (ITEM_BANK (i));
    return ITEM_AT (i);
}

static const struct kind* __fastcall__ kind_of (unsigned char k)
{
    return k < NK ? &kinds[k] : &kinds[0];
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
            break;
        }
    }
    hub_map (0);
    hy_sem_release (HUB->item_lock);
    return i < NITEMS ? i : 0xFFFF;
}

/* A kind by its name: its number, or 0 */
static unsigned char __fastcall__ kind_named (const char* name)
{
    unsigned char k;

    for (k = 1; k < NK; ++k) {
        if (!strcmp (kinds[k].name, name)) {
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
        if (!q.kind || q.where != IW_SQUARE || q.sn != sn || (q.flags & I_HIDDEN) || strncmp (kind_of (q.kind)->name,
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

/* The first thing named the character carries (on it, or in what it carries): its number, or 0 */
static unsigned __fastcall__ carried_named (const char* name)
{
    static struct item q;
    unsigned i, j;
    unsigned char n = strlen (name);

    for (i = 1; i < NITEMS; ++i) {
        memcpy (&q, item_at (i), sizeof q);
        if (!q.kind || q.where == IW_SQUARE || strncmp (kind_of (q.kind)->name, name, n)) {
            continue;
        }
        for (j = i; item_at (j)->where == IW_ITEM; j = item_at (j)->x) {     /* (Up to whoever has it) */
        }
        if (item_at (j)->where == IW_ACTOR && !item_at (j)->x) {
            return i;
        }
    }
    return 0;
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
    item_at (n)->flags |= I_KNOWN;
    return IR_OK;
}

/* Put down where the character stands (what's in it with it) */
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
    if (q->flags & I_LIT) {
        put (" (lit)");
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
        if (!q.kind || q.where != IW_SQUARE || q.sn != sn || (q.flags & I_HIDDEN) || q.x - x + 20 > 40 ||
            q.y - y + 20 > 40 || !(p = at (sn, q.x, q.y)) || !(*p & T_VIS)) {
            continue;
        }
        k = s->nitems++;
        strcpy (s->item[k].name, kind_of (q.kind)->name);
        s->item[k].qty = q.qty;
        s->item[k].plural = (kind_of (q.kind)->flags & K_PLURAL) != 0;
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
    }
    hub_map (0);
    HUB->ireq.result = q.result;
    return (q.op == IO_GET || q.op == IO_DROP) && q.result == IR_OK;
}
