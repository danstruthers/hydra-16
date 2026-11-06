/*
** dq0acct.c - Dingus Quest Zero's account of the time between two dungeons (the plan for the game between dungeons, its steps 4
** to 6; DUNGEON.md's a character's life; HOUSE.md's between dungeons): a program beside dq0life, in lib/acct (with its overlays:
** the start's time tells where a new file goes, DUNGEON.md's), that dq0life runs as a dungeon's been ended alive and before the next begins.  Before any of the game's tasks: none of the game's
** main RAM, the CPU its own.  It's HarnMaster's (run for dq0harn's games alone), on the saved files: the sheet (pc.hm), what's
** carried (kit.dat), what the life set out with (kit0.dat), the record (life.dat).  A program apart from dq0life so a new life's
** start reads none of it.
**
** dq0acct -d folder -l libdir
** By the life's seed and the dungeon's number alone (and what's said to its question): the time between, by the plan's three
** things (the days its wounds take by the real rolls, at most 90; the road; the lull); a physician if there's money; the wounds,
** infection, shock, venom and disease by the referee's own code told the days at rest (../dq0harn's course.h, watch.h, the
** library's harnheal.c); living, selling, mending, whetting, restocking and work; the month's practice.  It's told in a dozen
** lines, and the one question asked (its answer filled in), then Enter: the game clears the screen as it starts.  The rc
** variable dq0auto (the tests'): nothing waited for, the answers as they're filled in.
** Its exit status: 0; 2 the character dead of it (the record says so, dq0life's to tell the life's account); 1 its fault said.
** The rules' numbers marked [Suggest] in the plan: HOUSE.md's between dungeons, for the user's review.
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0/life.h"
#include "../dq0/theme.h"
#include "../dq0harn/dq0harn.h"
#include "../dq0w/outfit.h"
#include "dq0acct.h"

#pragma static-locals (on)

/* A bank of this program's at the window: the kinds (dq0w.cat's first part), what's carried, the sheet's other lines */
#define CAT             ((const struct kind*) 0x8000)
#define K               ((struct kitrec*) (0x8000 + KI_COUNT * sizeof (struct kind)))
typedef char bank_fits[KI_COUNT * 37 + NOFIT * 22 <= 0x2000 ? 1 : -1];


/* ---- The sheet: the referee's names (dq0harn.h's), and the lines of the file that are the game's */

harn_char pc;
unsigned char pc_by[HARN_INJMAX];
char news[NEWS_N];
struct venom pv;
struct sick pd;
unsigned char afffl, aff_breath;
unsigned rest_m;

struct life L;                                  /* (quest.c's too) */
static unsigned char kn;
static unsigned char gone[NOFIT];
const char* top;
const char* lib;
char path[96];
char calling[20];
char who[20];
unsigned char used[HARN_NSKILLS];
char tongue[NTONGUES][24];
unsigned char ntongues;
char ans[64];
unsigned char askit;                            /* (Waited for: not the tests' dq0auto) */

static void __fastcall__ fail (const char* why)
{
    write (2, "dq0acct: ", 9);
    write (2, why, strlen (why));
    write (2, "\n", 1);
    exit (1);
}

static const char* __fastcall__ in_ (const char* folder, const char* name)
{
    strcpy (path, folder);
    strcat (path, "/");
    strcat (path, name);
    return path;
}

/* ---- Words, by the console's width */

unsigned char width = 79;
static unsigned char col;
static char wb[96], ob[200];
static unsigned char wn, on;

static void out_flush (void)
{
    if (on) {
        write (1, ob, on);
        on = 0;
    }
}

static void __fastcall__ out_c (char c)
{
    if (on == sizeof ob) {
        out_flush ();
    }
    ob[on++] = c;
}

static void word_out (void)
{
    unsigned char k;

    if (!wn) {
        return;
    }
    if (col && col + wn + 1 > width) {
        out_c ('\n');
        col = 0;
    } else if (col) {
        out_c (' ');
        ++col;
    }
    for (k = 0; k < wn; ++k) {
        out_c (wb[k]);
    }
    col += wn;
    wn = 0;
}

void __fastcall__ say (const char* s)
{
    for (; *s; ++s) {
        if (*s == ' ') {
            word_out ();
        } else if (*s == '\n') {
            word_out ();
            out_c ('\n');
            col = 0;
        } else if (wn < sizeof wb) {
            wb[wn++] = *s;
        }
    }
}

void __fastcall__ sayn (unsigned long v)
{
    char b[12];

    say (ultoa (v, b, 10));
}

/* An amount, in farthings: "22d", "22d 1f", "3f" */
void __fastcall__ amount (unsigned long f)
{
    if (f >= PENNY || !f) {
        sayn (f / PENNY);
        say ("d");
    }
    if (f % PENNY) {
        if (f >= PENNY) {
            say (" ");
        }
        sayn (f % PENNY);
        say ("f");
    }
}

/* A line typed (b, 0-ended, its end of line gone); with the tests' dq0auto, none: "" */
void __fastcall__ ask (char* b, unsigned char n)
{
    int k;

    word_out ();
    out_flush ();
    b[0] = 0;
    if (askit) {
        write (1, " > ", 3);
        if ((k = read (0, b, n - 1)) > 0) {
            b[k] = 0;
            b[strcspn (b, "\r\n")] = 0;
        }
    } else {
        write (1, "\n", 1);
    }
    col = 0;
}

/* ---- What's carried */

static unsigned char __fastcall__ is_coin (const struct kitrec* r)
{
    return (CAT[r->o.kind].form & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT);
}

/* How many of it (a lot its count, else one) */
static unsigned char __fastcall__ many (const struct kitrec* r)
{
    return CAT[r->o.kind].form & KF_COUNT ? r->o.qty : 1;
}

/* What's used up, and comes back at a price (the plan's: torches, oil, food, bandages; every lot but coins and water) */
static unsigned char __fastcall__ used_up (unsigned char k)
{
    return k != KI_WATER && (k == KI_TORCH || k == KI_CANDLE || k == KI_OIL || k == KI_PITCH || k == KI_CHALK ||
        (CAT[k].form & (KF_COUNT | KF_SMALL)) == KF_COUNT);
}

/* A thing's worth, a sound one, one of it (the market's, things.c's goods: a kind written in silver by its material), farthings */
static unsigned long __fastcall__ worth (const struct kitrec* r)
{
    const struct kind* k = &CAT[r->o.kind];
    unsigned long v = k->value;
    unsigned char m = r->dur_max ? r->mat : r->o.mat;

    if (k->mat == MT_SILVER && m && m != MT_SILVER && m != MT_JEWEL) {
        v = m == MT_GOLD ? v * 20 : m == MT_STONE ? v * 8 : m == MT_GLASS ? v / 10 : v / 60;
    }
    return v;
}

/* Its condition, in tenths of its life (the market's: at least 1) */
static unsigned char __fastcall__ tenths (const struct kitrec* r)
{
    unsigned char t;

    if (!r->dur_max) {
        return 10;
    }
    t = (unsigned char) (((unsigned long) r->dur * 10 + r->dur_max - 1) / r->dur_max);
    return t ? t : 1;
}

/* What the market gives for it all: half its worth, by its condition (HOUSE.md's the market), at least a farthing */
static unsigned long __fastcall__ sell_price (const struct kitrec* r)
{
    unsigned long v = worth (r) * many (r) * tenths (r) / 20;

    return v ? v : 1;
}

/* Things in thing i */
static unsigned char __fastcall__ holds (unsigned char i)
{
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.place == PL_IN && K[k].o.in == i + 1) {
            return 1;
        }
    }
    return 0;
}

static unsigned named[24];                      /* (The ids of what the player's named: the dungeon's game.dat's) */
static unsigned char nnamed;
static unsigned char kept_blade;                /* (The weapon kept, none in hand: its dearest; 0xFF none) */

static unsigned char __fastcall__ is_named (unsigned id)
{
    unsigned char k;

    for (k = 0; id && k < nnamed; ++k) {
        if (named[k] == id) {
            return 1;
        }
    }
    return 0;
}

static unsigned char __fastcall__ in_hand (const struct kitrec* r)
{
    return r->o.place == PL_HAND || r->o.place == PL_OFF || r->o.place == PL_BOTH;
}

/* Spare: never what's named, its armour, what's worn, what's in its hand, what's used up (it'd buy it back), a key, what it lights
** its light with, nor what holds anything; its weapon if none's in its hand */
static unsigned char __fastcall__ spare (unsigned char i)
{
    const struct kitrec* r = &K[i];
    const struct kind* k = &CAT[r->o.kind];

    return !gone[i] && !is_coin (r) && !used_up (r->o.kind) && r->o.kind != KI_WATER && r->o.kind != KI_KEY && r->o.kind != KI_TINDERBOX && k->value &&
        r->o.place != PL_WORN && !in_hand (r) && k->craft != AP_ARMOUR && !is_named (r->id) && i != kept_blade && !holds (i);
}

static unsigned char __fastcall__ valuable (const struct kitrec* r)
{
    const struct kind* k = &CAT[r->o.kind];

    return k->craft == AP_JEWEL || k->mat == MT_GOLD || k->mat == MT_SILVER;
}

static void kit_read (void)
{
    int fd;
    unsigned char k, armed = 0;
    unsigned long best = 0;

    kn = 0;
    if ((fd = open (in_ (top, "kit.dat"), O_RDONLY)) >= 0) {
        while (kn < NOFIT && read (fd, &K[kn], sizeof K[0]) == sizeof K[0]) {
            ++kn;
        }
        close (fd);
    }
    kept_blade = 0xFF;
    for (k = 0; k < kn; ++k) {
        if (CAT[K[k].o.kind].craft == AP_WEAPON && (CAT[K[k].o.kind].flags & K_HAND)) {
            if (in_hand (&K[k])) {
                armed = 1;
            } else if (worth (&K[k]) >= best) {
                best = worth (&K[k]);
                kept_blade = k;
            }
        }
    }
    if (armed) {
        kept_blade = 0xFF;
    }
}

/* The names given in the dungeon just left (a thing's: its id) */
static void names_read (unsigned n)
{
    static struct tname e;
    static char d[16];
    int fd;
    unsigned char k;

    strcpy (d, "d");
    utoa (n, d + 1, 10);
    strcat (d, "/game.dat");
    if ((fd = open (in_ (top, d), O_RDONLY)) < 0) {
        return;
    }
    lseek (fd, sizeof (struct save), SEEK_SET);
    for (k = 0; k < TNAMES && nnamed < sizeof named / sizeof named[0] && read (fd, &e, sizeof e) == sizeof e; ++k) {
        if (e.what == NM_ITEM) {
            named[nnamed++] = e.x;
        }
    }
    close (fd);
}

/* ---- Money */

unsigned long cash;                             /* (Farthings: quest.c's too) */

static void cash_count (void)
{
    unsigned char k;

    cash = 0;
    for (k = 0; k < kn; ++k) {
        if (!gone[k] && is_coin (&K[k])) {
            cash += CAT[K[k].o.kind].value * (unsigned long) K[k].o.qty;
        }
    }
}

static void __fastcall__ coin_add (unsigned char kind, unsigned char q)
{
    if (kn < NOFIT) {
        memset (&K[kn], 0, sizeof K[0]);
        K[kn].o.kind = kind;
        K[kn].o.qty = q;
        K[kn].o.quality = 4;
        K[kn].o.place = PL_POUCH;
        ++kn;
    }
}

/* cash laid out as the kit's piles: the dearest coins kept as far as they go, then pennies in lots of 250 at most, its odd farthings */
static void cash_lay (void)
{
    unsigned long f = cash, v, n;
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && is_coin (&K[k]) && K[k].o.kind != KI_PENNIES && K[k].o.kind != KI_FARTHINGS) {
            v = CAT[K[k].o.kind].value;
            n = v ? f / v : 0;
            if (n > K[k].o.qty) {
                n = K[k].o.qty;
            }
            f -= n * v;
            if (!(K[k].o.qty = (unsigned char) n)) {
                gone[k] = 1;
            }
        }
    }
    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.kind == KI_PENNIES) {
            n = f / PENNY;
            n = n > 250 ? 250 : n;
            f -= n * PENNY;
            if (!(K[k].o.qty = (unsigned char) n)) {
                gone[k] = 1;
            }
        }
    }
    while (f >= PENNY && kn < NOFIT) {
        n = f / PENNY;
        n = n > 250 ? 250 : n;
        coin_add (KI_PENNIES, (unsigned char) n);
        f -= n * PENNY;
    }
    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.kind == KI_FARTHINGS) {
            if (!(K[k].o.qty = (unsigned char) f)) {
                gone[k] = 1;
            }
            f = 0;
        }
    }
    if (f) {
        coin_add (KI_FARTHINGS, (unsigned char) f);
    }
}

/* The money in words: as it is, or (changed) as the next place's money-changer will give it (the 15%, each pile: outfit.c's) */
static void __fastcall__ coins_say (unsigned char changed)
{
    unsigned p = 0, f = 0, c = 0, q;
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && is_coin (&K[k])) {
            q = K[k].o.qty;
            if (changed) {
                q -= (q * MONEY_CUT + 50) / 100;
            }
            if (K[k].o.kind == KI_PENNIES) {
                p += q;
            } else if (K[k].o.kind == KI_FARTHINGS) {
                f += q;
            } else {
                c += q;
            }
        }
    }
    if (c) {
        sayn (c);
        say (c == 1 ? " crown" : " crowns");
        say (p || f ? (p && f ? ", " : " and ") : "");
    }
    if (p) {
        sayn (p);
        say (p == 1 ? " penny" : " pennies");
        say (f ? " and " : "");
    }
    if (f) {
        sayn (f);
        say (f == 1 ? " farthing" : " farthings");
    }
    if (!p && !f && !c) {
        say ("no money");
    }
}

/* ---- The days' words */

static const char* const towns0[] = { "Vel", "Ost", "Har", "Kel", "Dun", "Mar", "Tor", "Wen" };
static const char* const towns1[] = { "mor", "wick", "ham", "ford", "by", "stead", "mouth", "ley" };
static const char* const inns[] = { "the Ram", "the Bell", "the Plough", "the Three Keys", "the Black Ox", "the Sheaf", "the Crown", "the Fleece" };
const char* inn;
char town[16];

/* (The months, the library's harn_month_name's: none of the rest of its names linked) */
static const char months[12][10] = { "Nuzyael", "Peonu", "Kelen", "Nolus", "Larane", "Agrazhar", "Azura", "Halane", "Savor", "Ilvin", "Navek", "Morgat" };

static void __fastcall__ say_date (unsigned long t)
{
    sayn (LIFE_DAY (t));
    say (" ");
    say (months[LIFE_MONTH (t) - 1]);
    say (" ");
    sayn (LIFE_YEAR (t));
}

/* A kind's name, one or n */
static void __fastcall__ kind_say (unsigned char k, unsigned char n)
{
    const char* s = CAT[k].name;
    const char* of = strstr (s, " of ");
    unsigned char l = strlen (s);
    char b[20];

    if (CAT[k].flags & K_PLURAL) {
        if (n > 1 || CAT[k].form & KF_COUNT) {
            sayn (n);
            say (" ");
        }
        say (s);
        return;
    }
    if (n > 1) {
        sayn (n);
        say (" ");
        if (of) {
            l = of - s;
        }
        memcpy (b, s, l);
        b[l] = 0;
        say (b);
        say (b[l - 1] == 'h' || b[l - 1] == 's' ? "es" : "s");
        if (of) {
            say (of);
        }
        return;
    }
    say (strchr ("aeiou", *s) ? "an " : "a ");
    say (s);
}

/* ---- The money's matters */

unsigned long funds, fee, earned;
struct wwas was[HARN_INJMAX];
unsigned char wid[HARN_INJMAX], nwas;
unsigned char tended[HARN_INJMAX], ntended, selfnd, unpaid;
unsigned passed;

/* The overlays (a bank each, the kinds' put back) */
static unsigned char b_cat;

#define OV1(x)  (hy_bank (b_cat + 1), x)
#define OV2(x)  (hy_bank (b_cat + 2), x)
#define OV3(x)  (hy_bank (b_cat + 3), x)
#define OV4(x)  (hy_bank (b_cat + 4), x)
#define CATS()  hy_bank (b_cat)

static unsigned char a_day (void)
{
    unsigned char r;

    OV1 (r = o_day ());
    CATS ();
    return r;
}

static unsigned char wage_of (void)
{
    unsigned char r;

    OV2 (r = o_wage ());
    CATS ();
    return r;
}

static unsigned long __fastcall__ sell_total (void)
{
    unsigned long t = 0;
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (spare (k)) {
            t += sell_price (&K[k]);
        }
    }
    return t;
}

static const unsigned char habits[5] = { 1, 1, 2, 3, 8 };    /* (A day's pence by its class: the plan's table) */
static const unsigned char steps[4] = { 8, 3, 2, 1 };

static unsigned char rate, restocks;
static unsigned long living, sold_t, smith_t, buy_t;
unsigned work_days;
static unsigned char sold[NOFIT], nsold;
static unsigned char mended[NOFIT], whetted[NOFIT], nmend, nwhet, nosmith;

/* What it set out with of what's used up (kit0.dat: the world's first save of the life, as it came in), less what it has: each
** kind wanted and how many */
static struct want {
    unsigned char   kind, n, place, in;
} wants[16];
static unsigned char nwants;

static void wants_read (void)
{
    static struct kitrec r;
    int fd;
    unsigned char k, h, z;

    if ((fd = open (in_ (top, "kit0.dat"), O_RDONLY)) < 0) {
        return;
    }
    while (read (fd, &r, sizeof r) == sizeof r) {
        if (!used_up (r.o.kind)) {
            continue;
        }
        for (z = 0; z < nwants && wants[z].kind != r.o.kind; ++z) {
        }
        if (z == nwants) {
            if (nwants == sizeof wants / sizeof wants[0]) {
                continue;
            }
            ++nwants;
            wants[z].kind = r.o.kind;
            wants[z].n = 0;
            wants[z].place = PL_PACK;
        }
        h = many (&r);
        wants[z].n = wants[z].n + h > 250 ? 250 : wants[z].n + h;
    }
    close (fd);
    /* (Less what it has; what's burnt out or empty gone) */
    for (k = 0; k < kn; ++k) {
        if (!used_up (K[k].o.kind) || gone[k]) {
            continue;
        }
        if (K[k].dur_max && !K[k].fuel && (K[k].o.kind == KI_TORCH || K[k].o.kind == KI_CANDLE || K[k].o.kind == KI_OIL || K[k].o.kind == KI_PITCH)) {
            gone[k] = 1;
            continue;
        }
        for (z = 0; z < nwants; ++z) {
            if (wants[z].kind == K[k].o.kind) {
                h = many (&K[k]);
                wants[z].n = wants[z].n > h ? wants[z].n - h : 0;
                wants[z].place = K[k].o.place == PL_HAND || K[k].o.place == PL_OFF || K[k].o.place == PL_BOTH ? PL_PACK : K[k].o.place;
                wants[z].in = K[k].o.in;
            }
        }
    }
}

/* The smith's price for thing i: mending a tenth of its price a tenth of its life, at most half, past that a new one at its whole
** price (the plan's); no mending but of what's its own and whole enough to call a thing */
static unsigned long __fastcall__ mend_price (const struct kitrec* r)
{
    unsigned long v = worth (r), m;

    if (!r->dur_max || r->dur >= r->dur_max || is_coin (r) || used_up (r->o.kind) || r->o.kind == KI_WRECK || r->o.kind == KI_BROKEN_SWORD) {
        return 0;
    }
    m = v * (10 - tenths (r) + 1) / 10;
    return m > v / 2 ? v : m;
}

/* Whetting and oiling: a hundredth of its price a month, at least a farthing, what's worn alone (only what wears today wears) */
static unsigned long __fastcall__ whet_price (const struct kitrec* r)
{
    unsigned long v;

    if (!r->wear || !r->dur_max || is_coin (r)) {
        return 0;
    }
    v = worth (r) * (passed / 30 ? passed / 30 : 1) / 100;
    return v ? v : 1;
}

/* What seeing to it would cost: mending (it's whetted with it), else whetting */
static unsigned long __fastcall__ owed (const struct kitrec* r)
{
    unsigned long v = mend_price (r);

    return v ? v : whet_price (r);
}

/* It still has one of these kinds */
static unsigned char __fastcall__ has_kind (unsigned char a, unsigned char b)
{
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k] && K[k].o.kind >= a && K[k].o.kind <= b) {
            return 1;
        }
    }
    return 0;
}

/* A kind wanted that's any use to it: no arrows, bolts or sling stones with nothing left to shoot them */
static unsigned char __fastcall__ of_use (unsigned char kind)
{
    return kind == KI_ARROWS ? has_kind (KI_SHORTBOW, KI_HARTBOW) : kind == KI_BOLTS ? has_kind (KI_CROSSBOW, KI_CROSSBOW) :
        kind == KI_STONES ? has_kind (KI_SLING, KI_SLING) : 1;
}

/* All it wants paid for, by what it has now: nothing goes on seeing to a thing it's sold, nor on what the thing it's sold was for */
static unsigned long upkeep (void)
{
    unsigned long t = 0;
    unsigned char k;

    for (k = 0; k < kn; ++k) {
        if (!gone[k]) {
            t += owed (&K[k]);
        }
    }
    for (k = 0; k < nwants; ++k) {
        if (of_use (wants[k].kind)) {
            t += wants[k].n * (unsigned long) CAT[wants[k].kind].value;
        }
    }
    return t;
}

static void __fastcall__ thing_in (unsigned char kind, unsigned char n, const struct want* w)
{
    if (kn >= NOFIT) {
        return;
    }
    memset (&K[kn], 0, sizeof K[0]);
    K[kn].o.kind = kind;
    K[kn].o.qty = n;
    K[kn].o.quality = 4;
    K[kn].o.place = w->place;
    K[kn].o.in = w->in;
    if (w->place == PL_IN && (!w->in || gone[w->in - 1])) {
        K[kn].o.place = PL_PACK;
        K[kn].o.in = 0;
    }
    ++kn;
}

/* All the money's matters, by the plan's order: sold what's spare till the wants are met, cheapest first, valuables last, never what
** it's named; down to rough; work for what's short at half a master's wage, its room and board found (the book's journeyman,
** Character 13), at most WORK_MAX days; then what it can pay: its living, the mending, the restocking.  No debts */
static void money_matters (void)
{
    unsigned long need, nowant, v, best;
    unsigned char k, b, habit = habits[pc.social < 5 ? pc.social : 2], val;
    unsigned char s;
    unsigned w;

    need = fee + (unsigned long) habit * passed * PENNY;
    for (val = 0; val < 2; ++val) {
        while (cash < need + upkeep ()) {       /* (Asked again as each's sold: not what it's sold) */
            b = 0xFF;
            best = 0;
            for (k = 0; k < kn; ++k) {
                if (spare (k) && valuable (&K[k]) == val && ((v = sell_price (&K[k])) < best || b == 0xFF)) {
                    best = v;
                    b = k;
                }
            }
            if (b == 0xFF) {
                break;
            }
            gone[b] = 1;
            sold[nsold++] = b;
            sold_t += best;
            cash += best;
        }
    }
    nowant = upkeep ();
    rate = 1;
    for (s = 0; s < 4; ++s) {
        if (steps[s] <= habit && cash >= fee + nowant + (unsigned long) steps[s] * passed * PENNY) {
            rate = steps[s];
            break;
        }
    }
    need = fee + nowant + (unsigned long) rate * passed * PENNY;
    if (cash < need) {
        w = wage_of ();
        v = ((need - cash) * 15 + w - 1) / w;   /* (A month's w pence at half: w / 15 farthings a day) */
        work_days = v > WORK_MAX ? WORK_MAX : (unsigned) v;
    }
    living = (unsigned long) rate * passed * PENNY;
    for (w = 0; w < work_days; ++w) {
        if (!a_day ()) {
            return;
        }
    }
    earned = (unsigned long) work_days * wage_of () / 15;
    cash += earned;
    cash = cash > fee ? cash - fee : 0;
    if (cash < living) {
        living = cash;                          /* (What's short found for nothing: a barn, a kitchen's scraps) */
    }
    cash -= living;
    for (k = 0; k < kn; ++k) {
        if (gone[k]) {
            continue;
        }
        if ((v = mend_price (&K[k])) != 0) {
            if (v <= cash) {
                cash -= v;
                smith_t += v;
                K[k].dur = K[k].dur_max;
                K[k].wear = 0;
                mended[nmend++] = k;
                continue;
            }
            nosmith = 1;
        }
        if ((v = whet_price (&K[k])) != 0) {
            if (v <= cash) {
                cash -= v;
                smith_t += v;
                K[k].wear = 0;
                whetted[nwhet++] = k;
            } else {
                nosmith = 1;
            }
        }
    }
    for (k = 0; k < nwants; ++k) {
        v = CAT[wants[k].kind].value;
        b = of_use (wants[k].kind) ? wants[k].n : 0;
        wants[k].n = 0;
        while (b && v <= cash && kn < NOFIT) {
            cash -= v;
            buy_t += v;
            --b;
            ++wants[k].n;
            ++restocks;
        }
        if (b) {
            nosmith |= 2;
        }
        if ((b = wants[k].n) != 0) {
            if (CAT[wants[k].kind].form & KF_COUNT) {
                for (s = 0; s < kn; ++s) {
                    if (!gone[s] && K[s].o.kind == wants[k].kind && (unsigned) K[s].o.qty + b <= 250) {
                        K[s].o.qty += b;
                        b = 0;
                        break;
                    }
                }
                if (b) {
                    thing_in (wants[k].kind, b, &wants[k]);
                }
            } else {
                for (; b; --b) {
                    thing_in (wants[k].kind, 1, &wants[k]);
                }
            }
        }
    }
}

/* What's left to hand, after the money's matters, told to the overlay (skill.c's o_tool) a thing at a time: its kind, its flags, its tool's key, for the
** kinds aren't there with it */
static void tools_note (void)
{
    unsigned char k;

    ans[8] = 0;
    OV2 (o_tool ());
    CATS ();
    for (k = 0; k < kn; ++k) {
        if (!gone[k]) {
            ans[8] = K[k].o.kind;
            ans[9] = CAT[ans[8]].flags;
            memcpy (ans, CAT[ans[8]].tool, 8);
            OV2 (o_tool ());
            CATS ();
        }
    }
}

/* Each waterskin filled again (at any well: nothing to pay) */
static void water_in (void)
{
    static const struct want w = { KI_WATER, 0, PL_IN, 0 };
    struct want x;
    unsigned char k, z, n = kn, there;

    for (k = 0; k < n; ++k) {
        if (gone[k] || K[k].o.kind != KI_WATERSKIN) {
            continue;
        }
        there = 0;
        for (z = 0; z < n; ++z) {
            if (!gone[z] && K[z].o.kind == KI_WATER && K[z].o.place == PL_IN && K[z].o.in == k + 1) {
                K[z].o.qty = CAT[KI_WATERSKIN].cap_bulk;
                there = 1;
            }
        }
        if (!there) {
            x = w;
            x.in = k + 1;
            thing_in (KI_WATER, CAT[KI_WATERSKIN].cap_bulk, &x);
        }
    }
}

/* kit.dat written, what's gone out of it, each thing's place in the list kept */
static void kit_write (void)
{
    static unsigned char to[NOFIT];
    unsigned char k, n = 0;
    int fd;

    for (k = 0; k < kn; ++k) {
        to[k] = gone[k] ? 0 : ++n;
    }
    if ((fd = open (in_ (top, "kit.dat"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        fail ("can't write what's carried");
    }
    for (k = 0; k < kn; ++k) {
        if (!gone[k]) {
            if (K[k].o.place == PL_IN) {
                K[k].o.in = K[k].o.in ? to[K[k].o.in - 1] : 0;
                if (!K[k].o.in) {
                    K[k].o.place = PL_PACK;     /* (Its container sold, or gone) */
                }
            }
            write (fd, &K[k], sizeof K[0]);
        }
    }
    close (fd);
}

static void life_write (void)
{
    int fd;

    if ((fd = open (in_ (top, LIFE_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, &L, sizeof L) != sizeof L) {
        fail ("can't write the record of the life");
    }
    close (fd);
}

/* ---- The account */

static const char* const lodge[9] = { 0, "You sleep in a hayloft and eat market bread", "You take a place in the common room", "You share a room",
    0, 0, 0, 0, "You take a room of your own, with a servant to help" };
static const char* const lodge2[9] = { 0, "", " (a bed and a hot meal and a cold", " (hot meals and ale", 0, 0, 0, 0, " (hot meals and wine" };

static void say_days (void)
{
    sayn (passed);
    say (passed == 1 ? " day passes" : " days pass");
}

/* The days' first lines: where it lodged, the physician, the wounds */
static void first_lines (void)
{
    unsigned char k;

    say ("You walk to ");
    say (town);
    say (". ");
    OV4 (o_reward_say ());                      /* (What it's paid for what it did, the plan's step 7) */
    CATS ();
    if (pc.state != HARN_DEAD) {
        say (lodge[rate]);
        if (rate > 1) {
            say (" at ");
            say (inn);
            say (lodge2[rate]);
            say (": ");
        } else {
            say (" (");
        }
        sayn (rate);
        say ("d a day, ");
        amount (living);
        say ("). ");
    }
    OV2 (o_first ());
    CATS ();
    say ("\n");
}

static void __fastcall__ list_things (const unsigned char* l, unsigned char n)
{
    unsigned char k;

    for (k = 0; k < n; ++k) {
        say (CAT[K[l[k]].o.kind].name);
        say (k + 2 < n ? ", " : k + 2 == n ? " and " : "");
    }
}

static void goods_lines (void)
{
    unsigned char k, any = 0;

    if (nsold) {
        say ("You sell ");
        for (k = 0; k < nsold; ++k) {
            kind_say (K[sold[k]].o.kind, many (&K[sold[k]]));
            say (k + 2 < nsold ? ", " : k + 2 == nsold ? " and " : "");
        }
        say (" (");
        amount (sold_t);
        say ("). ");
        any = 1;
    }
    if (nmend || nwhet) {
        say ("You have your ");
        if (nmend) {
            list_things (mended, nmend);
            say (" mended");
            say (nwhet ? " and your " : "");
        }
        if (nwhet) {
            list_things (whetted, nwhet);
            say (" whetted");
        }
        say (" (");
        amount (smith_t);
        say ("). ");
        any = 1;
    }
    if (nosmith & 1) {
        say ("Not everything that wants mending gets it: the money won't run to it. ");
        any = 1;
    }
    if (restocks) {
        say ("You buy ");
        for (k = 0, any = nwants; k < nwants; ++k) {
            if (wants[k].n) {
                kind_say (wants[k].kind, wants[k].n);
                say (", ");
            }
        }
        say ("for your pack (");
        amount (buy_t);
        say ("). ");
        any = 1;
    }
    if (nosmith & 2) {
        say ("You go on without some of what you'd like to carry. ");
        any = 1;
    }
    if (any) {
        say ("\n");
    }
}

static void last_lines (void)
{
    unsigned long d = L.h.date + passed;
    unsigned char k, any = 0;

    say ("You have ");
    coins_say (0);
    for (k = 0; k < kn; ++k) {                  /* (Told if it comes to anything: a pile of three or fewer pays nothing) */
        any |= !gone[k] && is_coin (&K[k]) && K[k].o.qty > 3;
    }
    if (any) {
        say (" (");
        coins_say (1);
        say (" once it's changed for the next place's coin)");
    }
    say (". It is ");
    say_date (d);
    say (", and you are ");
    sayn (LIFE_AGE (&L.h, d, pc.att[HARN_BD]));
    OV4 (o_standing ());                        /* (The sentence's end, the standing: the life's reputation, the plan's step 7) */
    CATS ();
}

static void dead_lines (void)
{
    unsigned long d = L.h.date + passed;

    say ("\n");
    say_days ();
    say (". ");
    first_lines ();
    say (news[0] == ' ' ? news + 1 : news);
    say (" You die on ");
    say_date (d);
    say (", at ");
    sayn (LIFE_AGE (&L.h, d, pc.att[HARN_BD]));
    say (".\n");
    word_out ();
    out_flush ();
}

static unsigned char __fastcall__ lull (void)
{
    unsigned char r = d100 ();

    return r <= 50 ? die (7) : r <= 80 ? 14 + random (8) : r <= 95 ? 30 + random (31) : 90 + random (91);
}

static struct hy_regs regs;

int main (int argc, char* argv[])
{
    unsigned char i;
    int fd, b;
    unsigned n;
    unsigned long seed;
    unsigned char more, gap = 0;

    for (i = 1; i + 1 < argc; i += 2) {
        if (!strcmp (argv[i], "-d")) {
            top = argv[i + 1];
        } else if (!strcmp (argv[i], "-l")) {
            lib = argv[i + 1];
        }
    }
    if (!top || !lib) {
        fail ("usage: dq0acct -d folder -l libdir");
    }
    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) < 0 || read (fd, &L, sizeof L) != sizeof L) {
        fail ("no record of the life");
    }
    close (fd);
    regs.a = 0xFF;                              /* (The rc variable dq0auto, the tests': ENV_GET itself, getenv's room) */
    regs.r[0] = (unsigned) "dq0auto";
    regs.r[1] = (unsigned) path;
    regs.r[2] = 8;
    regs.r[3] = 0;
    askit = hy_call (HY_ENV_GET, &regs) != 0;
    if ((b = hy_banks_alloc (5)) < 0) {
        fail ("no banks");
    }
    b_cat = b;
    strcpy (path, lib);
    strcat (path, "/../acct/dq0acct.ovl");
    if ((fd = open (path, O_RDONLY)) < 0) {
        fail ("no overlays");
    }
    hy_bank (b_cat + 1);
    read (fd, (void*) 0x8000, 0x2000);
    hy_bank (b_cat + 2);
    read (fd, (void*) 0x8000, 0x2000);
    hy_bank (b_cat + 3);
    read (fd, (void*) 0x8000, 0x2000);
    hy_bank (b_cat + 4);
    read (fd, (void*) 0x8000, 0x2000);
    close (fd);
    OV2 (o_width ());
    CATS ();
    strcpy (path, lib);
    strcat (path, "/dq0w.cat");
    if ((fd = open (path, O_RDONLY)) < 0 || read (fd, (void*) 0x8000, KI_COUNT * sizeof (struct kind)) != KI_COUNT * sizeof (struct kind)) {
        fail ("no catalogue");
    }
    close (fd);
    OV3 (i = o_sheet_read ());
    CATS ();
    if (!i) {
        L.h.flags &= ~LF_BETWEEN;               /* (No sheet: nothing to tell) */
        life_write ();
        return 0;
    }
    n = L.h.dungeon;
    seed = L.h.seed + n * 2654435761UL;
    rseed (seed ? seed : 1);
    strcpy (town, towns0[random (8)]);
    strcat (town, towns1[random (8)]);
    inn = inns[random (8)];
    kit_read ();
    names_read (n - 1);
    cash_count ();
    OV4 (o_reward ());                          /* (What solving the dungeon just left brings, its theme's reward line: its pennies
                                                **   into the money and its standing into the life's reputation, there; a thing
                                                **   given, a kind of the catalogue's by its name, a ring, put in its pouch, made new) */
    CATS ();
    for (i = *rw_thing ? 0 : KI_COUNT; i < KI_COUNT && strcmp (CAT[i].name, rw_thing); ++i) {
    }
    if (i < KI_COUNT) {
        coin_add (i, 1);
    }
    funds = cash + sell_total ();
    /* The wounds first: the bleeding bound, the physician; then the days they take, the road, the lull, all at rest */
    OV1 (o_note ());
    OV1 (more = o_bind ());
    if (more) {
        OV1 (o_treat ());
    }
    for (;;) {
        OV1 (i = more && !o_mended () && passed < HEAL_MAX);
        if (!i) {
            break;
        }
        more = o_day ();
    }
    CATS ();
    if (more) {
        gap = dice (2, 6) + lull ();            /* (Its road, its lull) */
    }
    for (; more && gap; --gap) {
        more = a_day ();
    }
    if (more) {
        wants_read ();
        money_matters ();
        more = pc.state != HARN_DEAD;
    }
    if (!more) {                                /* (Dead of it: the outcome died, the record the life's end; dq0life tells it) */
        dead_lines ();
        OV3 (o_sheet_write ());
        CATS ();
        L.h.date += passed;
        L.h.flags = (L.h.flags & ~LF_BETWEEN) | LF_OVER | LF_ABROAD;
        --L.h.dungeon;
        if (L.h.dungeon < LIFE_DUNGEONS) {
            L.d[L.h.dungeon].seed = 0;
        }
        life_write ();
        return 2;
    }
    CATS ();
    water_in ();
    tools_note ();
    pc.fl = afffl;                              /* (Fatigue clear, but what a sickness holds: the plan's; hunger and thirst
                                                **   are the world's, begun afresh with the dungeon) */
    pc.state = HARN_CONSCIOUS;
    cash_lay ();
    /* It's told */
    say ("\n");
    say_days ();
    say (". ");
    first_lines ();
    goods_lines ();
    OV2 (o_work ());
    OV2 (o_practice ());
    CATS ();
    last_lines ();
    OV4 (o_rumours ());                         /* (Where next: the rumours heard, one followed, or Enter to go on; asked
                                                **   before anything's written, so a reset tells the time between again, the same) */
    CATS ();
    word_out ();
    out_flush ();
    OV3 (o_sheet_write ());
    CATS ();
    kit_write ();
    L.h.date += passed;
    L.h.flags &= ~LF_BETWEEN;
    life_write ();
    return 0;
}
