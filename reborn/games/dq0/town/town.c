/*
** town.c - Dingus Quest Zero's town between two dungeons, a menu (the plan for the game between dungeons, its section 3 and step
** 11; the user's Q8: "A menu"; HOUSE.md's between dungeons, the town), the engine's: one source, linked once per ruleset with its
** rules (../rsmeta.h's meta_test: its tests' dice), each link a program of its own in lib/acct (TOWN_NAME: games/dq0town,
** HarnMaster's, dq0town; games/dq0ntown, the null ruleset's, dq0ntown), as the account is (DECISIONS.md's "The town and the
** scholar's house: the ruleset's hooks").  Run by its ruleset's account (../acct/town.c's ACCT_TOWN) as the character comes into town, its wounds seen to and the days between passed, before its money's matters: the account
** has no room for it (its main RAM at its floor, its overlays where the kinds are).  Before any of the game's tasks: none of the
** game's main RAM, none of its answer times.
**
** dq0town -d folder -l libdir                 (or its ruleset's link's name, TOWN_NAME)
**   the save's folder; the game's lib.  The rest the account's, in the time between's record (records.def's struct between,
**   between.dat in the folder, ../dq0/acct/town.h's): the town's name and its inn's (the name maker's, by the life's seed); the
**   days gone by (whetting's months); its keep so far and a day's (farthings, pence: its class's habit, the money's matters');
**   its knacks, the ruleset's (its acct_town: the market's haggling, the inn's listening; HarnMaster's Rhetoric and Intrigue EMLs)
** A town is a name, a paragraph and a list (town.txt, beside it: the words); each errand some days, the time going by those days.  A
** number or a name goes to a place; inside, the dungeon's words work where they can ("sell my dagger", "haggle for the rope"); Enter
** goes back out, and out in the street, leaves:
**   the inn          a day; what's said (the rumours heard, rumours.txt and heard.txt, the account's, the plan's step 7; listening, once,
**                    one more: themegen -r 4 -a, the same rumour -r 4 makes); rest, the days asked
**   the market       a day; buy, sell, haggle, asked the price: the game's prices (HOUSE.md's the market: whole worth to buy, half by
**                    its condition to sell; haggling, a quarter or a tenth, never past two thirds)
**   the smith        a day; mend, whet (the account's prices: goods.h's), a new blade
**   who's asking     a day; the patrons among the rumours (a why line's, not a stranger's find), an offer taken: set out for it
**   the scholar's    a day; a program of its own (its ruleset's link of ./schol.c, TOWN_SCHOL: the plan's step 12: a writing read by a scribe or a sage, studied in
**                    the library, a master's month, the other skills), run with what it needs, the record (between.dat)
**                    handed to it and back
**   leave for        the places heard of, each its road and days; one followed (quest.txt, the account's own way)
** Its tests (haggling, listening) are the ruleset's (meta_test, by the knack's chance in the record: a grade, rs.h's G_*); the rest
** (the prices, a grade's worth in a haggle, the days) the house's.
** What's done is the player's: the account (dq0acct) leaves alone what it saw to here (the market seen, nothing more sold or bought
** for it; the smith, nothing more mended or whetted), and does the rest as ever (selling, mending, restocking "just happen", the
** user's Q7).  Written: kit.dat (its things and money), quest.txt (the place followed), the record (between.dat: the days, what
** was seen to, the road).  The same life's seed and the same commands, the same town and errands.  Its exit status 0, or 1 (its
** fault said).
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <dice.h>

#include "../hub.h"
#include "../rs.h"
#include "../life.h"
#include "../theme.h"
#include "../../dq0w/outfit.h"
#define TOWN_ROAD
#include "../acct/town.h"
#include "../rsmeta.h"                      /* (The ruleset's tests: meta_test, the link's) */

#pragma static-locals (on)

/* A bank of this program's at the window: the kinds (dq0w.cat's first part), what's carried */
#define CAT             ((const struct kind*) 0x8000)
#define K               ((struct kitrec*) (0x8000 + KI_COUNT * sizeof (struct kind)))
#define PENNY           4UL                     /* (Farthings) */
#define TOWN_ASKS       60                      /* (And the lines typed: the soak's random answers end) */
#define REST_MAX        60                      /* (A rest at the inn, at most) */

struct life L;
static unsigned char kn;
static unsigned char gone[NOFIT];
const char* top;
const char* lib;
char path[96];
unsigned passed;
unsigned long cash;
static const char* town;
static const char* inn;
static unsigned long keep;
static unsigned char habit, rhet, intr;
static struct between tr;
static char ans[64];
static unsigned char asks;

/* ---- The meta programs' library (../dq0/meta/meta.c): the fault said, a path, the words by the console's width */

#define META_NAME       TOWN_NAME
#define META_WB         64
#define META_OB         128
#define META_FAIL
#define META_PATH
#define META_WORDS
#include "../meta/meta.c"
#undef META_FAIL
#undef META_PATH
#undef META_WORDS

#pragma code-name (push, "OV1CODE")             /* (The overlay's: the link's .cfg's OV1, its own bank, the main RAM's room) */
#pragma rodata-name (push, "OV1RODATA")
#pragma bss-name (push, "OV1BSS")
/* A line typed (ans: lower case, its spaces trimmed); the console's width */
#define META_ASK
#define META_WIDTH
#include "../meta/meta.c"
#undef META_ASK
#undef META_WIDTH
#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* ---- What's carried, its money and its worth (the account's own) */

#define GOODS_TOWN
#include "../acct/goods.h"

static void kit_read (void)
{
    int fd;

    kn = 0;
    if ((fd = open (in_ (top, TOWN_KIT), O_RDONLY)) >= 0) {
        while (kn < NOFIT && read (fd, &K[kn], sizeof K[0]) == sizeof K[0]) {
            ++kn;
        }
        close (fd);
    }
}

/* ---- The town's words (town.txt: a key's lines, each its values between bars) */

#define TW_N            3584                    /* (The words' bank: town.txt's lines but its notes, then the rumours) */
#define tw              ((char*) 0x8000)
#define RB_N            (0x2000 - TW_N)
#define rbuf            ((char*) 0x8000 + TW_N)
static unsigned char b_cat;
#define WORDS()         hy_bank (b_cat + 1)
#define KINDS()         hy_bank (b_cat)
#define OV(x)           (hy_bank (b_cat + 2), x)        /* (The overlay's code called, the kinds put back after it: KINDS) */
static unsigned twn;
static char* v[40];
static unsigned char nv;
static char said[360];                          /* (A rumour's value, value_of's; and a line of town.txt's, words_of's, v's: never
                                                **   both at once; and a program's arguments, run's: the main RAM's room, the plan's
                                                **   step 12) */
#define line    said

/* The n'th line of key (words_of: its values in v), and town.txt read (words_read): the library's, in the words' bank */
#define META_TEXT_ON()  WORDS ()
#define META_TEXT_OFF() KINDS ()
#define META_LINE       line
#define META_TEXT_MAX   TW_N
#define META_KEYS
#include "../meta/meta.c"
#undef META_KEYS

/* A key's lines: how many */
static unsigned char __fastcall__ lines_of (const char* key)
{
    unsigned char n = 0;

    while (words_of (key, n)) {
        ++n;
    }
    return n;
}

/* Words said, their slots filled: %T the town's name, %I its inn's */
static void __fastcall__ say_sub (const char* s)
{
    static char one[2];

    for (; *s; ++s) {
        if (*s == '%' && (s[1] == 'T' || s[1] == 'I')) {
            say (*++s == 'T' ? town : inn);
        } else {
            one[0] = *s;
            say (one);
        }
    }
}

/* One of a key's lines by the dice, its first value said (a second, a kind's "big": the town's a big one) */
static unsigned char big;

static void __fastcall__ say_one (const char* key)
{
    unsigned char n = lines_of (key);

    if (n && words_of (key, random (n))) {
        say_sub (v[0]);
        say (" ");
        big |= nv > 1;
    }
}

/* ---- The rumours (heard.txt's first, then rumours.txt's, the account's quest.c's way: four at most here, listening's one more) */

#define RUMOURS         4
static unsigned rused, rat[RUMOURS], rto[RUMOURS];
static unsigned char nr, nmade;                 /* (How many, and how many of them rumours.txt's) */

/* The rumours' files read (rumours_read: heard.txt's, then rumours.txt's, those of rumours.txt counted, nmade; rbuf in
** the words' bank, WORDS () and KINDS () round it), and rumour k's line of key: its n'th value (its lines joined) into
** said: 1, or 0 none (the library's) */
#define META_HEARD
#define META_MADE
#define META_RB_N       RB_N
#define META_VALUE
#define META_VALUE_N
#include "../meta/meta.c"
#undef META_HEARD
#undef META_VALUE

#pragma code-name (push, "OV1CODE")             /* (The overlay's: the link's .cfg's OV1, its own bank, the main RAM's room) */
#pragma rodata-name (push, "OV1RODATA")
#pragma bss-name (push, "OV1BSS")
/* The life's seed for dungeon n's (dq0life's seed_of): the rumours' seed, themegen -r's; a program run, and its arguments
** (the library's) */
#define META_SEED
#define META_RUN
#define META_ARG
#include "../meta/meta.c"
#undef META_SEED
#undef META_RUN
#undef META_ARG

/* A program of the game's run (lib's, prog its name past it), its arguments in said (each 0-ended, an empty one last), waited
** for: its exit status, or -1 */
static int __fastcall__ run (const char* prog)
{
    say_flush ();
    strcpy (path, lib);
    strcat (path, prog);
    return meta_run (path, said);
}
#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* One more rumour (the inn's listening): the files' next if they have one, else themegen's (-r with one more, -a the kinds made: the
** same one dq0life's themegen -r would have made next), its dots shown; 1 one more */
static unsigned char rumour_more (void)
{
    static const char* const kinds[] = { "artifact", "evil", "prisoner", "through", "lore", "scourge" };
    unsigned char k, j, mask = 0, was = nr;
    char* q = said;
    unsigned long s;

    if (was >= RUMOURS || L.h.seed < 65536UL) {
        return 0;                               /* (A stand-in's life hears none) */
    }
    for (k = nr - nmade; k < nr; ++k) {         /* (rumours.txt's kinds: themegen's to go on from) */
        if (value_of (k, "why", 0)) {
            for (j = 0; j < 6 && strcmp (said, kinds[j]); ++j) {
            }
            mask |= j < 6 ? 1 << j : 0;
        }
    }
    strcpy (q, "-q");
    strcpy (q += 3, "-p");
    strcpy (q += 3, "-r");
    utoa (nmade + 1, q += 3, 10);
    strcpy (q += strlen (q) + 1, "-a");
    utoa (mask, q += 3, 10);
    strcpy (q += strlen (q) + 1, "-s");
    OV (s = seed_of (L.h.seed, L.h.dungeon));
    KINDS ();
    ultoa (s ^ 0x52554D52UL, q += 3, 10);
    strcpy (q += strlen (q) + 1, "-o");
    strcpy (q += 3, top);
    q += strlen (q) + 1;
    *q = 0;
    OV (run ("/../../themegen"));
    KINDS ();
    say ("\n");
    rumours_read ();
    return nr > was;
}

/* Rumour k told: its words, its place and its road (by), who's asking too */
static void __fastcall__ rumour_say (unsigned char k, unsigned char by)
{
    say ("  ");
    sayn (k + 1);
    say (". ");
    value_of (k, "rumour", 0);
    say (said);
    if (by && value_of (k, "why", 2)) {
        say (" (");
        say (said);
        say (" asks;");
        say (value_of (k, "region", 0) ? " " : ")");
    } else if (value_of (k, "region", 0)) {
        say (" (");
    }
    if (value_of (k, "region", 0)) {
        say (said);
        say (", ");
        sayn (road_of (said));
        say (" days' road)");
    }
    say ("\n");
}

/* Rumour k followed: quest.txt (the account's way: dq0life moves it into the next dungeon's folder), the road, the town done */
static unsigned char done;

static void __fastcall__ follow (unsigned char k)
{
    int fd;

    if ((fd = open (in_ (top, "quest.txt"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        WORDS ();
        write (fd, rbuf + rat[k], rto[k] - rat[k]);
        KINDS ();
        close (fd);
    }
    if (value_of (k, "region", 0)) {
        strncpy (tr.whither, said, sizeof tr.whither - 1);
        tr.road = road_of (said);
    }
    done = 1;
}

/* ---- A thing named: a kind (by its name, a plural's or a word of it), what's carried of it */

static unsigned char __fastcall__ same (const char* a, const char* b, unsigned char n)
{
    return strlen (b) == n && !strncmp (a, b, n);
}

/* How well a phrase (w, n letters) names kind k: 4 its name, 3 its plural, 2 its name's last word ("oil", "ink"), 1 its start; 0 */
static unsigned char __fastcall__ names (const char* w, unsigned char n, unsigned char k)
{
    const char* s = CAT[k].name;
    const char* l = strrchr (s, ' ');
    unsigned char m = strlen (s);

    if (!*s || !n) {
        return 0;
    }
    if (same (w, s, n)) {
        return 4;
    }
    if ((n == m + 1 && w[m] == 's' || n == m + 2 && w[m] == 'e' && w[m + 1] == 's') && !strncmp (w, s, m)) {
        return 3;
    }
    if (l && (same (w, l + 1, n) || (n == strlen (l + 1) + 1 && w[n - 1] == 's' && !strncmp (w, l + 1, n - 1)))) {
        return 2;
    }
    return n >= 3 && n < m && !strncmp (w, s, n) ? 1 : 0;
}

/* The kind a phrase names best, among those listed under key in town.txt (a shop's), or any (key 0): KI_NONE none */
static unsigned char __fastcall__ kind_named (const char* w, const char* key)
{
    unsigned char k, j, b = KI_NONE, best = 0, h, n = strlen (w);

    for (k = 1; k < KI_COUNT; ++k) {
        if ((h = names (w, n, k)) > best) {
            if (key) {
                words_of (key, 0);
                for (j = 0; j < nv && strcmp (v[j], CAT[k].name); ++j) {
                }
                if (j == nv) {
                    continue;
                }
            }
            best = h;
            b = k;
        }
    }
    return b;
}

/* What's carried a phrase names (not money): the first of the best named, those not in hand nor worn first; 0xFF none */
static unsigned char __fastcall__ thing_named (const char* w)
{
    unsigned char k, b = 0xFF, best = 0, h, n = strlen (w);

    for (k = 0; k < kn; ++k) {
        if (gone[k] || is_coin (&K[k])) {
            continue;
        }
        h = names (w, n, K[k].o.kind) * 2 + (K[k].o.place != PL_WORN && !in_hand (&K[k]));
        if (h > 1 && h > best) {
            best = h;
            b = k;
        }
    }
    return b;
}

/* A command's object: its little words off its front ("the", "my", "for" ...), a count first if there's one (n, else 1) */
static unsigned char cnt;

static const char* __fastcall__ object (const char* s)
{
    unsigned char k, n;

    cnt = 1;
    words_of ("little", 0);                     /* (town.txt's little words) */
    for (k = 0; k < nv; ) {
        n = strlen (v[k]);
        if (!strncmp (s, v[k], n) && s[n] == ' ') {
            s += n + 1;
            k = 0;
        } else {
            ++k;
        }
    }
    if (*s >= '0' && *s <= '9') {
        cnt = atoi (s);
        cnt = cnt < 1 ? 1 : cnt > 20 ? 20 : cnt;
        s = strchr (s, ' ');
        s = s ? s + 1 : "";
    }
    return s;
}

/* ---- The money's words, prices and haggling (HOUSE.md's the market) */

/* A price rounded to the penny, up for buying, down for selling; under a penny in farthings, a farthing at least */
static unsigned long __fastcall__ rounded (unsigned long f, unsigned char up)
{
    if (f >= PENNY) {
        f = up ? (f + PENNY - 1) / PENNY * PENNY : f / PENNY * PENNY;
    }
    return f ? f : 1;
}

/* This visit's haggling: each kind haggled over once a way (bought, sold), its price's hundredths of worth (0: an offence, no deal) */
#define HAGGLES         8
static unsigned char hk[HAGGLES], hs[HAGGLES], hp[HAGGLES], nh;

static unsigned char __fastcall__ haggled (unsigned char kind, unsigned char selling)
{
    unsigned char k;

    for (k = 0; k < nh; ++k) {
        if (hk[k] == kind && hs[k] == selling) {
            return k;
        }
    }
    return 0xFF;
}

/* The price to buy n of kind (by its haggling, if it's been), farthings: 0 no deal */
static unsigned long __fastcall__ buy_price (unsigned char kind, unsigned char n)
{
    unsigned char h = haggled (kind, 0);
    unsigned long v = (unsigned long) CAT[kind].value * n;

    if (h != 0xFF) {
        if (!hp[h]) {
            return 0;
        }
        v = v * hp[h] / 100;
    }
    return rounded (v, 1);
}

/* What it's given for thing i (half its worth by its condition, the account's; by its haggling), farthings: 0 no deal */
static unsigned long __fastcall__ sale_price (unsigned char i)
{
    unsigned char h = haggled (K[i].o.kind, 1);
    unsigned long v = sell_price (&K[i]);

    if (h != 0xFF) {
        if (!hp[h]) {
            return 0;
        }
        v = v * hp[h] / 50;                     /* (Of the half: 50 hundredths as asked) */
    }
    return rounded (v, 0);
}

/* A knack's test (the ruleset's: meta_test, its grade rs.h's G_*): 0 a bad failure (G_BADLY), 1 a failure, 2 a success, 3 a fine
** success (G_WELL) */
static unsigned char __fastcall__ test (unsigned char chance)
{
    signed char g = meta_test (chance);

    return g + 2 - (g > 0);
}

/* A haggle (the market's knack, the ruleset's test; HOUSE.md's market: a fine success a quarter better, a success a tenth, a failure
** as asked, a bad failure an offence; never past two thirds of its worth either way) */
static void __fastcall__ haggle (unsigned char kind, unsigned char selling)
{
    static const unsigned char buy[4] = { 0, 100, 90, 75 }, sell[4] = { 0, 50, 55, 63 };
    static const char* const said_[4] = { "The trader takes offence, and won't deal with you over it now.",
        "The trader won't move on the price.", "You talk the price round a little.", "You talk the price round a good deal." };
    unsigned char r;

    if (haggled (kind, selling) != 0xFF) {
        say ("You've haggled over that already.\n");
        return;
    }
    if (nh == HAGGLES) {
        say ("The trader's tired of haggling.\n");
        return;
    }
    r = test (rhet);
    hk[nh] = kind;
    hs[nh] = selling;
    hp[nh++] = selling ? sell[r] : buy[r];
    say (said_[r]);
    say ("\n");
}

/* A kind's name, one or n (one of a lot whose name's a plural: its name, "rations") */
static void __fastcall__ name_say (unsigned char kind, unsigned char n)
{
    if (n == 1 && (CAT[kind].flags & K_PLURAL)) {
        say (CAT[kind].name);
    } else {
        kind_say (kind, n);
    }
}

/* ---- The days */

static void __fastcall__ days_pass (unsigned n)
{
    tr.days += n;
    if (tr.days >= TOWN_DAYS) {
        tr.days = TOWN_DAYS;
    }
}

static void today (void)
{
    unsigned long t = L.h.date + passed + tr.days;

    say ("It is ");
    sayn (LIFE_DAY (t));
    say (" ");
    say (words_of ("month", 0) >= LIFE_MONTH (t) ? v[LIFE_MONTH (t) - 1] : "");
    say (" ");
    sayn (LIFE_YEAR (t));
    say (". You have ");
    amount (cash);
    say ("; your keep so far comes to ");
    amount (keep + (unsigned long) habit * tr.days * PENNY);
    say (".\n");
}

/* ---- The places */

enum { TOWN_STREET, TOWN_INN, TOWN_MARKET, TOWN_SMITH, TOWN_ASKING, TOWN_SCHOLAR, TOWN_LEAVE, TOWN_SHUT };

/* Where a word goes: a place's number or its name, or a word that's done there; TOWN_STREET none */
static unsigned char __fastcall__ place_of (const char* w)
{
    unsigned char k, m, n = strcspn (w, " ");
    const char* s;

    for (k = 0; n && words_of ("words", k); ++k) {   /* (town.txt's words lines: each place's, in order, then what's shut's) */
        for (s = v[0]; *s; s += m) {
            if ((m = strcspn (s, " ")) == n && !strncmp (s, w, n)) {
                return k + 1;
            }
            while (s[m] == ' ') {
                ++m;
            }
        }
    }
    return TOWN_STREET;
}

static void menu (void)
{
    unsigned char k;

    say ("\n");
    for (k = 0; words_of ("place", k); ++k) {
        say ("  ");
        say (v[0]);
        say (" ");
        say (v[1]);
        say (": ");
        say (v[2]);
        say ("\n");
    }
    if (words_of ("shut", 0)) {
        say (v[0]);
        say ("\n");
    }
}

/* The inn: a day; what's said there (listening, once in a town, one more), a rest */
static unsigned char listened;

static void inn_ (void)
{
    unsigned char k;

    if (!nr) {
        say ("There's no talk of anywhere in particular.\n");
    } else {
        say ("There's talk of other places:\n");
        for (k = 0; k < nr; ++k) {
            rumour_say (k, 0);
        }
        if (!listened) {
            listened = 1;
            say ("You keep your ears open a while.");
            if (test (intr) >= 2 && rumour_more ()) {
                say (" Someone in a corner speaks of one more place:\n");
                rumour_say (nr - 1, 0);
            } else {
                say (" You hear nothing more worth the hearing.\n");
            }
        }
    }
}

static void __fastcall__ rest (const char* w)
{
    unsigned n = *w >= '0' && *w <= '9' ? atoi (w) : !strncmp (w, "a week", 6) || !strncmp (w, "week", 4) ? 7 : 1;

    n = n > REST_MAX ? REST_MAX : n;
    days_pass (n);
    say ("You rest ");
    sayn (n);
    say (n == 1 ? " day at " : " days at ");
    say (inn);
    say (".\n");
}

/* The market: its goods and their prices */
static void goods_say (const char* key, const char* who)
{
    unsigned char j, k, any = 0;

    words_of (key, 0);
    for (j = 0; j < nv; ++j) {
        for (k = 1; k < KI_COUNT && strcmp (v[j], CAT[k].name); ++k) {
        }
        if (k == KI_COUNT) {
            continue;
        }
        say (any ? ", " : who);
        name_say (k, 1);
        say (" ");
        amount (buy_price (k, 1));
        any = 1;
    }
    if (any) {
        say (".\n");
    }
}

/* Buying (n of a kind, from key's list): paid, into its pack */
static void __fastcall__ buy (const char* w, const char* key)
{
    static struct want x;
    unsigned char k = kind_named (w, key), s;
    unsigned long p;

    if (k == KI_NONE) {
        say (*w ? "There's none of that for sale here.\n" : "Buy what?\n");
        return;
    }
    if (!(CAT[k].form & KF_COUNT) && cnt > 10) {
        cnt = 10;
    }
    if (!(p = buy_price (k, cnt))) {
        say ("The trader won't deal with you over that now.\n");
        return;
    }
    if (p > cash) {
        say ("You haven't the money: ");
        name_say (k, cnt);
        say (" comes to ");
        amount (p);
        say (".\n");
        return;
    }
    if (kn >= NOFIT) {
        say ("You can carry no more.\n");
        return;
    }
    cash -= p;
    x.kind = k;
    x.place = PL_PACK;
    x.in = 0;
    if (CAT[k].form & KF_COUNT) {
        for (s = 0; s < kn; ++s) {
            if (!gone[s] && K[s].o.kind == k && (unsigned) K[s].o.qty + cnt <= 250) {
                K[s].o.qty += cnt;
                break;
            }
        }
        if (s == kn) {
            thing_in (k, cnt, &x);
        }
    } else {
        for (s = 0; s < cnt && kn < NOFIT; ++s) {
            thing_in (k, 1, &x);
        }
    }
    say ("You buy ");
    name_say (k, cnt);
    say (" (");
    amount (p);
    say (").\n");
}

static void __fastcall__ sell (const char* w)
{
    unsigned char i = thing_named (w);
    unsigned long p;

    if (i == 0xFF) {
        say (*w ? "You have nothing like that to sell.\n" : "Sell what?\n");
        return;
    }
    if (holds (i)) {
        say ("Not with things in it.\n");
        return;
    }
    if (!(p = sale_price (i))) {
        say ("The trader won't deal with you over that now.\n");
        return;
    }
    gone[i] = 1;
    cash += p;
    say ("You sell your ");
    say (CAT[K[i].o.kind].name);
    say (" (");
    amount (p);
    say (").\n");
}

/* Asked a price: to buy (from key's list) or to sell (what's carried) */
static void __fastcall__ price (const char* w, const char* key)
{
    unsigned char i = thing_named (w), k;

    if (i != 0xFF) {
        say ("For your ");
        say (CAT[K[i].o.kind].name);
        say (" you'd be given ");
        amount (sale_price (i));
        say (".\n");
    } else if ((k = kind_named (w, key)) != KI_NONE) {
        say ("The price of ");
        name_say (k, 1);
        say (": ");
        amount (buy_price (k, 1));
        say (".\n");
    } else {
        say ("There's none of that here.\n");
    }
}

static void __fastcall__ haggle_for (const char* w, const char* key)
{
    unsigned char i = thing_named (w), k;

    if (i != 0xFF && !strcmp (key, "goods")) {
        haggle (K[i].o.kind, 1);
        price (w, key);
    } else if ((k = kind_named (w, key)) != KI_NONE) {
        haggle (k, 0);
        price (w, key);
    } else {
        say ("Haggle over what?\n");
    }
}

/* The smith: what wants seeing to, its prices; mending and whetting (the account's prices: goods.h's) */
static unsigned char __fastcall__ wants_smith (unsigned char i)
{
    return !gone[i] && owed (&K[i]);
}

static void smith_list (void)
{
    unsigned char k, any = 0;

    for (k = 0; k < kn; ++k) {
        if (wants_smith (k)) {
            say (any ? "; your " : "Your ");
            say (CAT[K[k].o.kind].name);
            say (mend_price (&K[k]) ? " wants mending, " : " wants whetting, ");
            amount (rounded (owed (&K[k]), 1));
            any = 1;
        }
    }
    say (any ? ".\n" : "Nothing of yours wants mending or whetting.\n");
    goods_say ("blades", "For sale: ");
}

static void __fastcall__ mend (const char* w, unsigned char whet)
{
    unsigned char i, k, any = 0;
    unsigned long p;

    for (k = 0; k < kn; ++k) {
        if (!wants_smith (k) || (strcmp (w, "all") && strcmp (w, "everything") && strcmp (w, "") && k != (i = thing_named (w)))) {
            continue;
        }
        if (whet && mend_price (&K[k])) {
            say ("Your ");
            say (CAT[K[k].o.kind].name);
            say (" wants mending, not only whetting.\n");
            continue;
        }
        p = rounded (owed (&K[k]), 1);
        if (p > cash) {
            say ("You haven't the money for your ");
            say (CAT[K[k].o.kind].name);
            say (": ");
            amount (p);
            say (".\n");
            continue;
        }
        cash -= p;
        any = 1;
        K[k].wear = 0;
        if (mend_price (&K[k])) {
            K[k].dur = K[k].dur_max;
            say ("The smith mends your ");
        } else {
            say ("The smith whets your ");
        }
        say (CAT[K[k].o.kind].name);
        say (" (");
        amount (p);
        say (").\n");
    }
    if (!any && (!*w || !strcmp (w, "all") || !strcmp (w, "everything"))) {
        say (whet ? "The smith finds nothing that wants only whetting.\n" : "The smith finds nothing to do.\n");
    } else if (!any && thing_named (w) == 0xFF) {
        say ("You have nothing like that.\n");
    } else if (!any && !wants_smith (thing_named (w))) {
        say ("It wants nothing doing.\n");
    }
}

/* Who's asking: the patrons among the rumours (a why line's patron, not a stranger's find); 0xFF none */
static unsigned char __fastcall__ patron (unsigned char k)
{
    return value_of (k, "why", 1) && strcmp (said, "found");
}

static void asking (void)
{
    unsigned char k, any = 0;

    for (k = 0; k < nr; ++k) {
        if (patron (k)) {
            if (!any) {
                say (L.h.reputation >= 3 ? "Word of you has gone round, and they come to you:\n" : "You ask about, and hear who's asking:\n");
            }
            any = 1;
            rumour_say (k, 1);
        }
    }
    if (!any) {
        say ("No one's asking for anyone's help just now.\n");
    }
}

/* said, lower case, in w or w in it (w 3 letters or more) */
static unsigned char __fastcall__ said_in (const char* w)
{
    char* s;

    for (s = said; *s; ++s) {
        if (*s >= 'A' && *s <= 'Z') {
            *s |= 0x20;
        }
    }
    return strlen (w) >= 3 && *said && (strstr (said, w) || strstr (w, said));
}

/* Whose offer, which place: a number, or a patron's name or a place's (a word of it, or the words about it); 0xFF none */
static unsigned char __fastcall__ which (const char* w, unsigned char patrons)
{
    unsigned char k;

    if (*w >= '1' && *w <= '0' + nr && (!w[1] || w[1] == ' ')) {
        k = *w - '1';
        return !patrons || patron (k) ? k : 0xFF;
    }
    for (k = 0; k < nr; ++k) {
        if (patrons && !patron (k)) {
            continue;
        }
        if ((patrons && value_of (k, "why", 2) && said_in (w)) || (value_of (k, "region", 0) && said_in (w))) {
            return k;
        }
    }
    return 0xFF;
}

static void leave_list (void)
{
    unsigned char k;

    if (!nr) {
        say ("You've heard of nowhere in particular: you go on as you came.\n");
        done = 1;
        return;
    }
    say ("The places you've heard of:\n");
    for (k = 0; k < nr; ++k) {
        rumour_say (k, 0);
    }
}

#pragma code-name (push, "OV1CODE")             /* (The overlay's: the link's .cfg's OV1, its own bank, the main RAM's room) */
#pragma rodata-name (push, "OV1RODATA")
#pragma bss-name (push, "OV1BSS")
/* The time between's record written (wr), or read (between.dat: the account's, and the scholar's house's to it and back) */
static void __fastcall__ tr_io (unsigned char wr)
{
    int fd;

    if ((fd = open (in_ (top, BETWEEN_FILE), wr ? O_WRONLY | O_CREAT | O_TRUNC : O_RDONLY, 0644)) < 0 ||
        (wr ? write (fd, &tr, sizeof tr) : read (fd, &tr, sizeof tr)) != sizeof tr || tr.magic != BETWEEN_MAGIC ||
        tr.version != BETWEEN_VER) {
        fail (wr ? "can't write " BETWEEN_FILE : "no record of the time between (" BETWEEN_FILE ")");
    }
    close (fd);
}
#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

#pragma code-name (push, "OV1CODE")             /* (The overlay's: the link's .cfg's OV1, its own bank, the main RAM's room) */
#pragma rodata-name (push, "OV1RODATA")
#pragma bss-name (push, "OV1BSS")
/* The scholar's house (the plan's step 12): its program run (TOWN_SCHOL), its first command (what was typed in the street, if it was one of its
** own) handed to it, and the record to it and back (the money in it, whether it's a big town, the writings carried: scholar's), its
** days, what's left of the money, what a master taught, the script studied; the rumours heard again after it (a writing read
** better, heard.txt's) */
static void __fastcall__ o_scholar (const char* first)
{
    char* q;

    q = arg (arg (said, "-d", top), "-l", lib);
    *q = 0;
    strcpy (tr.first, first);
    tr_io (1);
    if (run ("/../acct/" TOWN_SCHOL)) {
        fail ("the scholar's house failed");
    }
    tr_io (0);
}
#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

static void __fastcall__ scholar (const char* first)
{
    unsigned char k, n = 0;

    tr.money = cash;
    tr.big = big;
    memset (tr.clue, 0, sizeof tr.clue);
    for (k = 0; k < kn; ++k) {                  /* (Its writings carried: an old writing's and a copy's clues, their fuel) */
        if (!gone[k] && (K[k].o.kind == KI_WRITING || K[k].o.kind == KI_COPY) && n < 8) {
            tr.clue[n++] = K[k].fuel;
        }
    }
    OV (o_scholar (first));
    KINDS ();
    cash = tr.money;
    rumours_read ();
}

/* ---- The town */

static unsigned char here;                        /* (Where it is: TOWN_*) */

/* Into place p: its day (each visit), its words (the scholar's house, first its first command: "" none) */
static const char* first = "";

static void __fastcall__ go (unsigned char p)
{
    static const char* const keys[] = { 0, "inn", "market", "smith", "asking" };

    if (p == TOWN_SHUT) {
        if (words_of ("shut", 0)) {
            say (v[0]);
            say ("\n");
        }
        return;
    }
    here = p;
    nh = 0;
    if (p < TOWN_LEAVE) {
        days_pass (1);
        if (p < TOWN_SCHOLAR) {
            say_one (keys[p]);                  /* (The place's words, one by the dice; a day: the scholar's, its own) */
            say ("\n");
        }
    }
    switch (p) {
    case TOWN_INN:
        inn_ ();
        break;
    case TOWN_MARKET:
        tr.seen |= TS_MARKET;
        goods_say ("goods", "For sale: ");
        break;
    case TOWN_SMITH:
        tr.seen |= TS_SMITH;
        smith_list ();
        break;
    case TOWN_ASKING:
        asking ();
        break;
    case TOWN_SCHOLAR:
        scholar (first);
        here = TOWN_STREET;                        /* (Its own questions asked: back in the street after it) */
        menu ();
        break;
    case TOWN_LEAVE:
        leave_list ();
        break;
    }
}

/* An offer taken, a place followed: 1 it was one */
static unsigned char __fastcall__ chosen (const char* w)
{
    unsigned char k = which (w, here == TOWN_ASKING);

    if (k == 0xFF) {
        return 0;
    }
    if (here == TOWN_ASKING) {
        value_of (k, "why", 2);
        say ("You give ");
        say (said);
        say (" your word.\n");
    }
    follow (k);
    return 1;
}

/* A line typed, where it is */
static void __fastcall__ command (char* l)
{
    char* w = strchr (l, ' ');
    const char* o;
    unsigned char p = place_of (l), q;
    const char* key;

    o = object (w ? w + 1 : "");
    q = place_of (o);
    if ((!strncmp (l, "go ", 3) || !strncmp (l, "visit ", 6) || !strncmp (l, "enter ", 6)) && q != TOWN_STREET && q != TOWN_LEAVE) {
        go (q);                                 /* ("go to the market") */
        return;
    }
    if (here == TOWN_INN && *l >= '0' && *l <= '9') {
        rest (l);                               /* (The days to rest, asked) */
        return;
    }
    if ((here == TOWN_LEAVE || here == TOWN_ASKING) && (chosen (l) || (w && chosen (o)))) {
        return;
    }
    if (here == TOWN_ASKING && *l >= '1' && *l <= '9') {
        say ("No one's asking that.\n");
        return;
    }
    if (p == TOWN_STREET) {
        say ("There's nothing like that to do here: a number or a place's name, or Enter to go back.\n");
        return;
    }
    if (p == TOWN_SCHOLAR) {
        first = w && (*l < '1' || *l > '9') ? l : "";   /* (A command of its own, done there: "read the plaque") */
        go (p);
        first = "";
        return;
    }
    if (p != here && (p == TOWN_SHUT || !w || (*l >= '1' && *l <= '9'))) {
        go (p);                                 /* (A place named: gone to) */
        return;
    }
    if (p != here && p != TOWN_INN && !(p == TOWN_MARKET && here == TOWN_SMITH && strncmp (l, "sell", 4) && kind_named (o, "blades") != KI_NONE)) {
        go (p);                                 /* (Its errand's place gone to first, then done there) */
        if ((here == TOWN_LEAVE || here == TOWN_ASKING) && *o) {
            chosen (o);                         /* ("follow the second", "take Arlen's offer") */
            return;
        }
    }
    key = here == TOWN_SMITH ? "blades" : "goods";
    if (!strncmp (l, "buy", 3)) {
        buy (o, key);
    } else if (!strncmp (l, "sell", 4)) {
        sell (o);
    } else if (!strncmp (l, "haggle", 6) || !strncmp (l, "bargain", 7)) {
        haggle_for (o, key);
    } else if (!strncmp (l, "how", 3) || !strncmp (l, "price", 5) || !strncmp (l, "what", 4)) {
        price (o, key);
    } else if (!strncmp (l, "mend", 4) || !strncmp (l, "repair", 6)) {
        mend (o, 0);
    } else if (!strncmp (l, "whet", 4) || !strncmp (l, "sharpen", 7) || !strncmp (l, "oil", 3)) {
        mend (o, 1);
    } else if (!strncmp (l, "rest", 4) || !strncmp (l, "sleep", 5)) {
        if (here != TOWN_INN) {
            go (TOWN_INN);
        }
        rest (o);
    } else if (!strncmp (l, "listen", 6) || !strncmp (l, "hear", 4)) {
        if (here != TOWN_INN) {
            go (TOWN_INN);
        } else {
            inn_ ();                            /* (What's said, told again) */
        }
    } else if (p != here) {
        go (p);
    } else {
        say ("You're there already.\n");
    }
}

static void town_say (void)
{
    say ("\n");
    say (town);
    say (", ");
    say_one ("kind");
    say_one ("sight");
    say ("\n");
}

static void __fastcall__ rest (const char* w);

int main (int argc, char* argv[])
{
    unsigned char i;
    int fd;

    for (i = 1; i + 1 < argc; i += 2) {
        switch (argv[i][1]) {
        case 'd': top = argv[i + 1]; break;
        case 'l': lib = argv[i + 1]; break;
        }
    }
    if (!top || !lib) {
        fail ("usage: " TOWN_NAME " -d folder -l libdir");
    }
    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) < 0 || read (fd, &L, sizeof L) != sizeof L) {
        fail ("no record of the life");
    }
    close (fd);
    if ((fd = hy_banks_alloc (3)) < 0) {
        fail ("no banks");
    }
    b_cat = fd;
    strcpy (path, lib);                         /* (The overlay, its bank after the words') */
    strcat (path, "/../acct/" TOWN_NAME ".ovl");
    if ((fd = open (path, O_RDONLY)) < 0) {
        fail ("no overlay");
    }
    hy_bank (b_cat + 2);
    read (fd, (void*) 0x8000, 0x2000);
    close (fd);
    tr_io (0);                                  /* (The account's record: the town, the inn, the days ...) */
    KINDS ();
    town = tr.town;
    inn = tr.inn;
    passed = tr.passed;
    keep = tr.keep;
    habit = tr.habit;
    rhet = tr.rhet;
    intr = tr.intr;
    strcpy (path, lib);
    strcat (path, "/dq0w.cat");
    if ((fd = open (path, O_RDONLY)) < 0 || read (fd, (void*) 0x8000, KI_COUNT * sizeof (struct kind)) != KI_COUNT * sizeof (struct kind)) {
        fail ("no catalogue");
    }
    close (fd);
    strcpy (path, lib);
    strcat (path, "/../acct/town.txt");
    if ((fd = open (path, O_RDONLY)) < 0) {
        fail ("no town.txt");
    }
    words_read (fd);
    close (fd);
    OV (width_read ());
    KINDS ();
    rseed ((L.h.seed ^ 0x544F574EUL) + L.h.dungeon * 40503UL);  /* ("TOWN": the same life, the same town and errands) */
    kit_read ();
    cash_count ();
    rumours_read ();
    town_say ();
    menu ();
    while (!done) {
        if (!here) {
            today ();
        }
        words_of ("prompt", here);             /* (Its prompt: in line, v[0]) */
        OV (ask (nv ? v[0] : ""));
        KINDS ();
        if (asks > TOWN_ASKS || tr.days >= TOWN_DAYS) {
            say ("The days go by, and at last you set out.\n");
            here = TOWN_LEAVE;
            *ans = 0;
        }
        if (!*ans) {                            /* (Enter: back to the street; there, leave; leaving, the first) */
            if (here == TOWN_LEAVE) {
                if (nr) {
                    follow (0);
                }
                done = 1;
            } else if (here) {
                here = TOWN_STREET;
                menu ();
            } else {
                go (TOWN_LEAVE);
            }
            continue;
        }
        command (ans);
    }
    cash_lay ();
    kit_write (TOWN_KIT);
    OV (tr_io (1));
    KINDS ();
    say ("\n");
    say_flush ();
    return 0;
}
