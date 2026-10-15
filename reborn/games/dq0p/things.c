/*
** things.c - (dq0p) the things (DUNGEON.md: items; items.h): the catalogue, written into the hub as the task starts
** (a stand-in for the theme's data); the items' records kept, moved as the UI asks (the character's kit, a thing
** taken up, put down; held in a hand, stowed in the pack, put in a container or on a surface, hung from the belt,
** taken out again, worn, taken off; lit, put out), listed (what the character carries, worded into the hub's text
** for the UI to say); the character's light, from the lit things it carries (the hub's pc_light: the world's to
** read), and the lit things lying about (view.c's lamps); and the things the character sees, named in what it
** perceives (sight.c).  The item store's free records are taken under the hub's lock (the maker puts chunks' things
** down too).  Kept with the game (dq0 -d): the store's banks as they are, the save folder's items.dat.  The
** catalogue's in the hub's catalogue bank (kinds.c: read in from the task's overlay file as it starts), each kind
** copied out as it's wanted; the code that handles things is there too (at the end, its words with it).
*/

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "things.h"

#define NK      KI_COUNT

extern const struct kind kinds[];              /* (kinds.c's: in the catalogue's bank) */

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
static unsigned char piece[256];               /* (And kit's names, say's words: a buffer in main RAM) */

/* A thing's record, copied out: the one the functions share that call none that wants it while they use it (others
** have their own: make_own's, the recursive ones', light's ...) */
static struct item rec;

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

static void light_now (void);

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
        light_now ();
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
    memcpy (r, kinds + (k < NK ? k : 0), sizeof *r);
    return r;
}

/* Item i's record copied out (into q); item i's kind's record (kind_of's) */
static void __fastcall__ fetch (struct item* q, unsigned i)
{
    memcpy (q, item_at (i), sizeof *q);
}

static const struct kind* __fastcall__ kind_at (unsigned i)
{
    return kind_of (item_at (i)->kind);
}

/* ---- A thing made its own, as the things task first comes to it (no two alike): its number (record 0 keeps the
** last given), its quality (most are middling), its material (a blade of iron, of steel if it's well made, of
** bronze if crudely), its durability's most (its material's toughness, its weight, its quality) and what's left of
** it (a thing found lying in the dungeon is the worse for its years there); a portal's part's material and quality
** the maker's already (its portal's) */
static void __fastcall__ make_own (unsigned i)
{
    static struct item q;
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

/* A kind by its name (or a word of it: "sand"; the whole name first, so "sword" isn't a broken one): its number,
** or 0 */
static unsigned char __fastcall__ named (const char* k, const char* w);
static unsigned char __fastcall__ kind_named (const char* name)
{
    unsigned char k;

    for (k = 1; k < NK; ++k) {
        if (!strcmp (kind_of (k)->name, name)) {
            return k;
        }
    }
    for (k = 1; k < NK; ++k) {
        if (named (kind_of (k)->name, name)) {
            return k;
        }
    }
    return 0;
}

/* ---- The character's kit (a stand-in for the theme's and the occupation's): a torch lit in hand, a backpack worn
** (rations, a tinderbox in it), a belt (a pouch of pennies, a dagger, a waterskin hanging from it); and things more
** (dq0 -k, commas between: in the hub's text), each held if there's a hand for it, else in the pack */

static unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);
static void __fastcall__ place (unsigned n, unsigned char where, unsigned x, unsigned char flags);

/* Each thing: its kind, what it's in (an earlier one's place in the list; 0xFF: on the character), how many, its
** flags */
#define NKIT    9
static const unsigned char kit_list[NKIT][4] = {
    { KI_TORCH, 0xFF, 1, I_HELD | I_LIT | I_KNOWN }, { KI_BACKPACK, 0xFF, 1, I_WORN | I_KNOWN }, { KI_RATIONS, 1, 3, 0 },
    { KI_TINDERBOX, 1, 1, 0 }, { KI_BELT, 0xFF, 1, I_WORN | I_KNOWN }, { KI_POUCH, 4, 1, 0 }, { KI_PENNIES, 5, 12, 0 },
    { KI_DAGGER, 4, 1, 0 }, { KI_WATERSKIN, 4, 1, 0 }
};

static void kit (void)
{
    static unsigned made[NKIT];
    const unsigned char* e;
    unsigned i;
    unsigned char k;
    char* c;
    char* more = (char*) piece;

    hub_map (0);
    strncpy (more, HUB->things_text, 63);       /* (The things more: the UI's -k, in the hub's text) */
    more[63] = 0;
    for (k = 0; k < NKIT; ++k) {
        e = kit_list[k];
        made[k] = i = item_new (e[0], e[1] == 0xFF ? IW_ACTOR : IW_ITEM, 0, e[1] == 0xFF ? 0 : made[e[1]], 0, e[2]);
        item_at (i)->flags = e[3];
    }
    for (; *more; more = c) {
        if ((c = strchr (more, ',')) != 0) {
            *c++ = 0;
        } else {
            c = more + strlen (more);
        }
        if ((k = kind_named (more)) != 0 && (i = item_new (k, IW_ACTOR, 0, 0, 0, 1)) != 0xFFFF && !carry (i, 1)) {
            place (i, IW_ACTOR, 0, I_HELD);     /* (No room: held all the same) */
        }
    }
}

/* ---- What's in a thing, or on the character: the bulk and weight (quarter pounds) of what's in it (a thing's own
** with what's in it, or on it: a soft thing's bulk its own or what it holds, the more) */
static unsigned __fastcall__ weight_in (unsigned where, unsigned x);
static unsigned __fastcall__ weight_of (unsigned i)
{
    const struct kind* k = kind_at (i);
    unsigned w = (unsigned) k->weight * (k->flags & K_PLURAL ? 1 : item_at (i)->qty);

    if (k->flags & (K_CONTAINER | K_BELT) || k->keeps & KF_ON) {
        w += weight_in (IW_ITEM, i);
    }
    return w;
}

static unsigned __fastcall__ weight_in (unsigned where, unsigned x)
{
    struct item* p;
    unsigned i, w = 0;

    for (i = 1; i < NITEMS; ++i) {
        p = item_at (i);
        if (p->kind && p->where == where && p->x == x) {
            w += weight_of (i);
        }
    }
    return w;
}

static unsigned char nin;                       /* (bulk_in's: the things in it, or hanging from it) */
static unsigned __fastcall__ bulk_in (unsigned x);
static unsigned __fastcall__ bulk_of (unsigned i)
{
    const struct kind* k = kind_at (i);
    unsigned b = k->bulk, c;

    if (k->flags & K_CONTAINER && (c = bulk_in (i)) > b) {
        b = c;
    }
    return b;
}

static unsigned __fastcall__ bulk_in (unsigned x)
{
    struct item* p;
    unsigned i, b = 0;
    unsigned char n = 0;

    for (i = 1; i < NITEMS; ++i) {
        p = item_at (i);
        if (p->kind && p->where == IW_ITEM && p->x == x) {
            b += bulk_of (i);
            ++n;
        }
    }
    nin = n;                                    /* (The outermost call's, set last) */
    return b;
}

/* A name said: a word of the kind's ("key" an iron key; "sword" a broken one too), or its start */
static unsigned char __fastcall__ named (const char* k, const char* w)
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

/* The thing named that the character sees, the nearest (on a square it sees now, or on a surface there): its number,
** or 0 */
static unsigned __fastcall__ seen_named (const char* name)
{
    const struct item* s;
    unsigned i, best = 0, bd = 0xFFFF, d, x, y;
    unsigned char sn;
    unsigned char* p;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    for (i = 1; i < NITEMS; ++i) {
        fetch (&rec, i);
        if (!rec.kind || !(s = lying (&rec)) || s->sn != sn || (rec.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN ||
            !named (kind_of (rec.kind)->name, name) || s->x - x + 20 > 40 || s->y - y + 20 > 40 ||
            !(p = at (sn, s->x, s->y)) || !(*p & T_VIS)) {
            continue;
        }
        d = (s->x > x ? s->x - x : x - s->x) + (s->y > y ? s->y - y : y - s->y);
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
    unsigned i;

    for (i = 1; i < NITEMS; ++i) {
        fetch (&rec, i);
        if (rec.kind && rec.where != IW_SQUARE && named (kind_of (rec.kind)->name, name) && on_pc (i)) {
            return i;
        }
    }
    return 0;
}

/* Within reach of the character (on a square next to it, or its own; or on a surface there) */
static unsigned char __fastcall__ in_reach (unsigned n)
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
static unsigned char afar;
static unsigned __fastcall__ at_hand (const char* name)
{
    unsigned n;

    afar = 0;
    if (!*name) {
        return 0;
    }
    if ((n = carried_named (name)) != 0) {
        return n;
    }
    if ((n = seen_named (name)) != 0 && !(afar = !in_reach (n))) {
        return n;
    }
    return 0;
}

/* ---- The character's hands, its pack, its belt (about's), and where a thing will go */
static unsigned pack, belt;
static unsigned char hands;                     /* (Taken: a two-handed thing both) */

/* The hands a thing wants: both for a two-handed one, or a big one */
static unsigned char __fastcall__ two (const struct kind* k)
{
    return k->flags & K_TWO || k->bulk >= 20 ? 2 : 1;
}

static void about (void)
{
    const struct kind* k;
    unsigned i;

    pack = belt = hands = 0;
    for (i = 1; i < NITEMS; ++i) {
        fetch (&rec, i);
        if (rec.kind && rec.where == IW_ACTOR && !rec.x) {
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
}

/* Whether thing n can go in c (or on it: a surface; hung from it: a belt): 0, or the words why not (SA_*): the
** catalogue's bank's o_fits */
void cat_map (void)
{
    hub_map (CAT_BANK);
}

static unsigned char __fastcall__ o_fits (unsigned n, unsigned c);
static unsigned char __fastcall__ fits (unsigned n, unsigned c)
{
    cat_map ();
    return o_fits (n, c);
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
static unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first)
{
    unsigned char h;

    about ();
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
** itself out), into the hub's things_text, a space between; and one used, worn (the referee's choice, by its key),
** its name back in the request (bare hands: "hands") */
static void tools (void)
{
    const struct kind* k;
    unsigned i;
    char* t;

    hub_map (0);
    HUB->things_text[0] = 0;
    for (i = 1; i < NITEMS; ++i) {
        fetch (&rec, i);
        k = kind_of (rec.kind);                   /* (Copied: the hub's text is in another bank) */
        if (rec.kind && rec.where != IW_SQUARE && k->tool[0] && on_pc (i)) {
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
    static char name[16];
    struct item* p;
    unsigned i;

    strcpy (name, key);
    for (i = 1; i < NITEMS; ++i) {
        fetch (&rec, i);
        if (rec.kind && rec.where != IW_SQUARE && !strcmp (kind_of (rec.kind)->tool, key) && on_pc (i)) {
            p = item_at (i);
            p->wear = p->wear + by < p->wear ? 255 : p->wear + by;
            strcpy (name, kind_of (rec.kind)->name);
            break;
        }
    }
    hub_map (0);
    strcpy (HUB->ireq.name, name);
}

/* ---- Taken up: into the backpack if it fits (its room, its weight, what it keeps), else in a hand (two, at most);
** not within reach: IR_NOT_HERE, and where it is (the hub's ireq's x, y: the UI walks there, and asks again) */
static unsigned char __fastcall__ get (const char* name)
{
    const struct item* s;
    unsigned n = seen_named (name), x, y;

    if (!n) {
        return IR_NONE;
    }
    fetch (&rec, n);
    s = lying (&rec);
    hub_map (0);
    x = HUB->pc_x;
    y = HUB->pc_y;
    HUB->ireq.x = s->x;                         /* (Where it is: IR_NOT_HERE's, IR_FIXED's) */
    HUB->ireq.y = s->y;
    if (s->x - x + 1 > 2 || s->y - y + 1 > 2) {
        return IR_NOT_HERE;
    }
    return rec.where == IW_PORTAL ? IR_FIXED : carry (n, 0) ? IR_OK : IR_FULL;  /* (A portal's part: lifted out
                                                **   first, the maker's) */
}

/* Put down where the character stands (what's in it with it) */
static unsigned dropped;                        /* (The last put down) */
static unsigned char __fastcall__ drop (const char* name)
{
    unsigned char sn;
    unsigned x, y, n = carried_named (name);

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
        fetch (&q, i);
        if (!q.kind || q.where != where || q.x != x) {
            continue;
        }
        if (!depth) {
            put (first ? "You carry " : "; ");
        } else {
            put (first ? (kind_at (x)->flags & K_BELT || kind_at (x)->keeps & KF_ON ?
                " (on it: " : " (in it: ") : ", ");
        }
        first = 0;
        thing_words (&q);
        if (!depth) {
            put (q.flags & I_HELD ? (two (kind_of (q.kind)) == 2 ? ", in both hands" : ", in your hand") :
                q.flags & I_WORN ? ", worn" : "");
        }
        if (kind_of (q.kind)->flags & (K_CONTAINER | K_BELT) || kind_of (q.kind)->keeps & KF_ON) {
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
    char b[6];

    hub_map (0);
    tp = HUB->things_text;
    *tp = 0;
    contents (IW_ACTOR, 0, 0);
    put (" In all, ");                          /* (Its load: the referee's encumbrance, to come) */
    put (utoa ((weight_in (IW_ACTOR, 0) + 2) / 4, b, 10));
    put (" lb.");
}

/* ---- A search for what's concealed about the character (on the squares it sees, within SEARCH_R): the referee's
** test for each (its search, at the concealment's difficulty: two steps of it each, from +2), the ones found no
** longer hidden, and named in the hub's text; but what's hidden by magic (found by magical means only) */
#define SEARCH_R        3
static unsigned char search (void)
{
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
        fetch (&rec, i);
        if (!rec.kind || rec.where != IW_SQUARE || rec.sn != sn ||
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
            thing_words (&rec);
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
    const struct kind* k;
    struct ireq* r;
    unsigned n = carried_named (name);

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
    return IR_OK;
}

/* ---- The things the character sees (sight.c's): on the squares it sees now, not hidden, named, where they lie; a
** surface's things after it, seen with it (hid 2: on the one before).  Each entry made in se (struct sight's item's
** layout: its way and square the square's), then copied in */
static struct { char name[14]; unsigned char qty, plural, rel, hid; unsigned n, x, y; } se;
typedef char se_fits[sizeof se == sizeof ((struct sight*) 0)->item[0] ? 1 : -1];

static void __fastcall__ seen_one (struct sight* s, unsigned i, unsigned char on)
{
    const struct kind* k;

    make_own (i);
    k = kind_at (i);
    strcpy (se.name, k->name);
    se.plural = (k->flags & K_PLURAL) != 0;
    se.qty = item_at (i)->qty;
    on = on ? 2 : (item_at (i)->flags & I_HIDDEN) != 0;
    se.hid = item_at (i)->flags & I_LIT ? on | 4 : on;
    se.n = i;
    memcpy (&s->item[s->nitems++], &se, sizeof se);
}

void __fastcall__ things_seen (struct sight* s, unsigned char sn, unsigned x, unsigned y, unsigned char face)
{
    static struct item q;
    struct item* r;
    unsigned i, j;
    unsigned char* p;

    s->nitems = 0;
    for (i = 1; i < NITEMS && s->nitems < SEE_ITEMS; ++i) {
        fetch (&q, i);
        if (!q.kind || q.where != IW_SQUARE || q.sn != sn || (q.flags & (I_HIDDEN | I_KNOWN)) == I_HIDDEN ||
            q.x - x + 20 > 40 ||
            q.y - y + 20 > 40 || !(p = at (sn, q.x, q.y)) || !(*p & T_VIS)) {
            continue;
        }
        se.rel = q.x == x && q.y == y ? 0xFF : REL8 (bearing ((int) (q.x - x), (int) (q.y - y)), face);
        se.x = q.x;
        se.y = q.y;
        seen_one (s, i, 0);
        if (kind_of (q.kind)->keeps & KF_ON) {
            for (j = 1; j < NITEMS && s->nitems < SEE_ITEMS; ++j) {
                r = item_at (j);
                if (r->kind && r->where == IW_ITEM && r->x == i) {
                    seen_one (s, j, 1);
                }
            }
        }
    }
}

/* ---- A thing handled (held, stowed, put in or on another, taken from it, worn, taken off, lit, put out): each
** worded into the hub's text (kinds.c's words: % the thing's name, # the other's), and the time it took (took,
** tenths).  The code's in the catalogue's bank (below); here, what it calls on in the main RAM */
static char xn[16], yn[16];                     /* (The two names said: the words asked, then the things') */
static unsigned char took;
static struct ireq iq;                          /* (The request, copied: things_serve's) */

/* Item i's record written back (from q); item i's kind (its number); the hub's text begun again */
static void __fastcall__ store (const struct item* q, unsigned i)
{
    memcpy (item_at (i), q, sizeof *q);
}

static unsigned __fastcall__ item_kind (unsigned i)
{
    return item_at (i)->kind;
}

static void text_new (void)
{
    hub_map (0);
    tp = HUB->things_text;
    *tp = 0;
}

/* The character's light: the brightest lit thing it carries (in a hand, worn, hung from its belt: one stowed is
** put out), into the hub, named */
static void light_now (void)
{
    static char lamp[14];
    const struct kind* k;
    unsigned i;
    unsigned char best = 0;

    lamp[0] = 0;
    for (i = 1; i < NITEMS; ++i) {
        fetch (&rec, i);
        if (rec.kind && (rec.flags & I_LIT) && on_pc (i)) {
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

/* The lit things lying about (on squares within LAMP_R of (x, y), on surfaces there), into view.c's lamps: their
** squares, their reach */
#define LAMP_R          28
void __fastcall__ things_lamps (unsigned char sn, unsigned x, unsigned y)
{
    const struct item* s;
    struct lamp* l = lamp;
    unsigned i;

    nlamps = 0;
    for (i = 1; i < NITEMS && nlamps < NLAMPS; ++i) {
        fetch (&rec, i);
        if (!rec.kind || !(rec.flags & I_LIT) || !(s = lying (&rec)) || s->sn != sn || s->x - x + LAMP_R > 2 * LAMP_R ||
            s->y - y + LAMP_R > 2 * LAMP_R) {
            continue;
        }
        l->x = s->x;
        l->y = s->y;
        l->r = kind_of (rec.kind)->light;
        ++l;
        ++nlamps;
    }
}

/* ==== In the catalogue's bank (dq0p.cfg's OV1: linked at $8000 after the kinds and their words, and run with that
** bank in the window, things_serve's o_handle and fits' o_fits): the things handled.  Its code never touches the
** window: a main-RAM function that does is called through m0, m1 or m2 (bank.s: the catalogue's bank put back after
** it; its arguments 16 bits, its result a char's in the low byte), an item's record copied out (fetch) and back
** (store); the kinds read where they are (kinds.c's kinds) */
#pragma code-name (push, "OV1CODE")
#pragma rodata-name (push, "OV1RODATA")

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

    d = (char*) piece;
    for (s = said[n]; *s; ++s) {                /* (Its words made in the main RAM, then put in the hub's text) */
        if (*s == '%' || *s == '#') {
            strcpy (d, *s == '%' ? xn : yn);
            d += strlen (d);
        } else {
            *d++ = *s;
        }
    }
    *d = 0;
    m1 (put, (unsigned) piece);
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
    static unsigned c, n;
    static unsigned char r;

    if (!(c = other ())) {
        return IR_NO;
    }
    for (n = 1; n < NITEMS; ++n) {
        FETCH (n);
        if (o.kind && o.where == IW_ITEM && o.x == c && named (kinds[o.kind].name, iq.name)) {
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

/* A thing handled, as the request (iq) asks: IR_OK, or IR_NO (refused: the words why) */
static unsigned char o_handle (void)
{
    static unsigned n;
    static unsigned char op;

    op = iq.op;
    m0 (text_new);
    took = 0;
    strcpy (xn, iq.name);
    strcpy (yn, iq.into);
    if (op >= IO_LIGHT) {
        return light (op == IO_LIGHT);
    }
    if (op == IO_TAKE) {
        return take_from ();
    }
    if (!*xn) {
        return no (SA_WHAT);
    }
    if (!(n = m1 (op == IO_DOFF ? (void*) carried_named : (void*) at_hand, (unsigned) xn))) {
        return no (op == IO_DOFF ? SA_NOT_WORN : afar ? SA_REACH : SA_NO_SUCH);
    }
    name_of (n, 1);
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

/* ---- A request (the hub's ireq), answered (but for its state, the caller's: done once it's looked again): 1 if
** what lies about has changed */
unsigned char things_serve (void)
{
    unsigned char changed = 0;

    hub_map (0);
    memcpy (&iq, &HUB->ireq, sizeof iq);
    iq.result = IR_OK;
    switch (iq.op) {
    case IO_KIT:
        kit ();
        break;
    case IO_GET:
        iq.result = get (iq.name);
        break;
    case IO_DROP:
        iq.result = drop (iq.name);
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
    default:                                    /* (A thing handled: IO_HOLD ...) */
        cat_map ();
        iq.result = o_handle ();
        hub_map (0);
        HUB->ireq.x = took;
        break;
    }
    if (iq.op == IO_KIT || iq.op == IO_GET || iq.op == IO_DROP || iq.op == IO_HIDE || iq.op >= IO_HOLD) {
        light_now ();                           /* (Its light: what it holds lit) */
        changed |= iq.result == IR_OK;
    }
    hub_map (0);
    HUB->ireq.result = iq.result;
    return changed && iq.op != IO_KIT;
}
