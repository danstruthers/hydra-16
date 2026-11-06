/*
** dq0town.c - Dingus Quest Zero's town between two dungeons, a menu (the plan for the game between dungeons, its section 3 and step
** 11; the user's Q8: "A menu"; HOUSE.md's between dungeons, the town): a program of its own in lib/acct, run by the account (dq0acct's
** town.c) as the character comes into town, its wounds seen to and the days between passed, before its money's matters: the account
** has no room for it (its main RAM at its floor, its overlays where the kinds are).  Before any of the game's tasks: none of the
** game's main RAM, none of its answer times.
**
** dq0town -d folder -l libdir -t town -i inn -p days -k keep -h habit -r rhetoric -n intrigue
**   the save's folder; the game's lib; the town's name and its inn's (the account's: the name maker's, by the life's seed); the days
**   gone by (whetting's months); its keep so far and a day's (farthings, pence: its class's habit, the money's matters'); its
**   Rhetoric and its Intrigue (EMLs: the market's haggling, the inn's listening)
** A town is a name, a paragraph and a list (town.txt, beside it: the words); each errand some days, the time going by those days.  A
** number or a name goes to a place; inside, the dungeon's words work where they can ("sell my dagger", "haggle for the rope"); Enter
** goes back out, and out in the street, leaves:
**   the inn          a day; what's said (the rumours heard, rumours.txt and heard.txt, the account's, the plan's step 7; Intrigue, once,
**                    one more: themegen -r 4 -a, the same rumour -r 4 makes); rest, the days asked
**   the market       a day; buy, sell, haggle, asked the price: the game's prices (HOUSE.md's the market: whole worth to buy, half by
**                    its condition to sell; Rhetoric's haggling, a quarter or a tenth, never past two thirds)
**   the smith        a day; mend, whet (the account's prices: goods.h's), a new blade
**   who's asking     a day; the patrons among the rumours (a why line's, not a stranger's find), an offer taken: set out for it
**   leave for        the places heard of, each its road and days; one followed (quest.txt, the account's own way)
** What's done is the player's: the account (dq0acct) leaves alone what it saw to here (the market seen, nothing more sold or bought
** for it; the smith, nothing more mended or whetted), and does the rest as ever (selling, mending, restocking "just happen", the
** user's Q7).  Written: kit.dat (its things and money), quest.txt (the place followed), town.dat (town.h's: the days, what was seen
** to, the road).  The same life's seed and the same commands, the same town and errands.  Its exit status 0, or 1 (its fault said).
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <dice.h>

#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0/life.h"
#include "../dq0/theme.h"
#include "../dq0w/outfit.h"
#define TOWN_ROAD
#include "../dq0acct/town.h"

#pragma static-locals (on)

/* A bank of this program's at the window: the kinds (dq0w.cat's first part), what's carried */
#define CAT             ((const struct kind*) 0x8000)
#define K               ((struct kitrec*) (0x8000 + KI_COUNT * sizeof (struct kind)))
#define PENNY           4UL                     /* (Farthings) */
#define TOWN_DAYS       200                     /* (The days in town at most: then it sets out) */
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
static struct townrec tr;
static char ans[64];
static unsigned char asks;

static void __fastcall__ fail (const char* why)
{
    write (2, "dq0town: ", 9);
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

/* ---- Words, by the console's width (dq0acct's) */

static unsigned char width = 79, col;
static char wb[64], ob[128];
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

static void __fastcall__ say (const char* s)
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

static void __fastcall__ sayn (unsigned long v)
{
    char b[12];

    say (ultoa (v, b, 10));
}

/* An amount, in farthings: "22d", "22d 1f", "3f" */
static void __fastcall__ amount (unsigned long f)
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

/* A line typed, lower case, its spaces trimmed (ans): asked on a line of its own, then the game's "> " */
static void __fastcall__ ask (const char* prompt)
{
    int k;
    char* s;

    say (prompt);
    say ("\n");
    out_flush ();
    write (1, "> ", 2);
    ans[0] = 0;
    if ((k = read (0, ans, sizeof ans - 1)) > 0) {
        ans[k] = 0;
        ans[strcspn (ans, "\r\n")] = 0;
    }
    col = 0;
    for (s = ans; *s; ++s) {
        if (*s >= 'A' && *s <= 'Z') {
            *s |= 0x20;
        } else if (*s == '\t' || *s == ',' || *s == '.' || *s == '?' || *s == '!') {
            *s = ' ';
        }
    }
    for (s = ans + strlen (ans); s > ans && s[-1] == ' '; --s) {
    }
    *s = 0;
    for (s = ans; *s == ' '; ++s) {
    }
    memmove (ans, s, strlen (s) + 1);
    ++asks;
}

/* The console's width (dq0acct's o_width) */
static void width_read (void)
{
    static char b[100];
    char* p;
    int fd, n = 0;

    if ((fd = open ("/dev/consctl", O_RDONLY)) >= 0) {
        n = read (fd, b, sizeof b - 1);
        close (fd);
    }
    b[n > 0 ? n : 0] = 0;
    for (p = b; p; p = strchr (p, '\n')) {
        if (*p == '\n') {
            ++p;
        }
        if (!strncmp (p, "size ", 5) && atoi (p + 5) >= 40) {
            width = atoi (p + 5) > 80 ? 79 : atoi (p + 5) - 1;
        }
    }
}

/* ---- What's carried, its money and its worth (the account's own) */

#define GOODS_TOWN
#include "../dq0acct/goods.h"

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
static unsigned twn;
static char* v[40];
static unsigned char nv;
static char line[320];

/* The n'th line of key (0 the first): its values in v (nv of them, their spaces trimmed); 0 none */
static unsigned char __fastcall__ words_of (const char* key, unsigned char n)
{
    unsigned p = 0, e;
    unsigned char k = strlen (key);
    char* s;

    WORDS ();
    for (; p < twn; p = e + 1) {
        for (e = p; e < twn && tw[e] != '\n'; ++e) {
        }
        if (strncmp (tw + p, key, k) || (tw[p + k] != ' ' && tw[p + k] != '|') || n--) {
            continue;
        }
        memcpy (line, tw + p, e - p < sizeof line - 1 ? e - p : sizeof line - 1);
        KINDS ();
        line[e - p < sizeof line - 1 ? e - p : sizeof line - 1] = 0;
        for (nv = 0, s = strchr (line, '|'); s && nv < sizeof v / sizeof v[0]; s = strchr (s, '|')) {
            *s++ = 0;
            while (*s == ' ') {
                ++s;
            }
            v[nv++] = s;
        }
        for (k = 0; k < nv; ++k) {
            for (s = v[k] + strlen (v[k]); s > v[k] && (s[-1] == ' ' || s[-1] == '\r'); --s) {
            }
            *s = 0;
        }
        return nv;
    }
    KINDS ();
    return nv = 0;
}

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

/* One of a key's lines by the dice, its first value said */
static void __fastcall__ say_one (const char* key)
{
    unsigned char n = lines_of (key);

    if (n && words_of (key, random (n))) {
        say_sub (v[0]);
        say (" ");
    }
}

/* ---- The rumours (heard.txt's first, then rumours.txt's, the account's quest.c's way: four at most here, Intrigue's one more) */

#define RUMOURS         4
static unsigned rused, rat[RUMOURS], rto[RUMOURS];
static unsigned char nr, nmade;                 /* (How many, and how many of them rumours.txt's) */
static char said[360];

static void __fastcall__ heard (const char* name)
{
    unsigned at = rused, p;
    int fd, k;

    if ((fd = open (in_ (top, name), O_RDONLY)) < 0) {
        return;
    }
    WORDS ();
    while (rused < RB_N - 1 && (k = read (fd, rbuf + rused, RB_N - 1 - rused)) > 0) {
        rused += k;
    }
    close (fd);
    rbuf[rused] = 0;
    for (p = at; p < rused; ++p) {
        if ((p == at || rbuf[p - 1] == '\n') && !strncmp (rbuf + p, "rumour", 6) && (rbuf[p + 6] == ' ' || rbuf[p + 6] == '|')) {
            if (nr && rto[nr - 1] > p) {
                rto[nr - 1] = p;
            }
            if (nr == RUMOURS) {
                break;
            }
            rat[nr] = p;
            rto[nr++] = rused;
            nmade += *name == 'r';
        }
    }
    KINDS ();
}

static void rumours_read (void)
{
    rused = nr = nmade = 0;
    heard ("heard.txt");
    heard ("rumours.txt");
}

/* Rumour k's line of key: its n'th value (its lines joined) into said: 1, or 0 none */
static unsigned char __fastcall__ value_of (unsigned char k, const char* key, unsigned char n)
{
    const char* p = rbuf + rat[k];
    const char* e = rbuf + rto[k];
    unsigned char len = strlen (key), got = 0, i;
    unsigned m = 0;

    WORDS ();
    for (; p < e; ) {
        if (got) {
            if (*p != ' ' && *p != '\t') {
                break;
            }
            while (*p == ' ' || *p == '\t') {
                ++p;
            }
            if (m && m < sizeof said - 1) {
                said[m++] = ' ';
            }
        } else if (!strncmp (p, key, len) && (p[len] == ' ' || p[len] == '|')) {
            for (i = 0; i <= n && p; ++i) {
                p = strchr (p + 1, '|');
            }
            if (!p || p >= e) {
                break;
            }
            got = 1;
            ++p;
            while (*p == ' ') {
                ++p;
            }
        } else {
            for (; p < e && *p != '\n'; ++p) {
            }
            ++p;
            continue;
        }
        for (; p < e && *p != '\n' && *p != '\r' && *p != '|'; ++p) {
            if (m < sizeof said - 1) {
                said[m++] = *p;
            }
        }
        if (n) {
            break;                              /* (A value past the first: its line alone) */
        }
        for (; p < e && *p != '\n'; ++p) {
        }
        ++p;
    }
    while (m && said[m - 1] == ' ') {
        --m;
    }
    said[m] = 0;
    KINDS ();
    return got;
}

/* The life's seed for dungeon n's (dq0life's seed_of): the rumours' seed, themegen -r's */
static unsigned long __fastcall__ seed_of (unsigned long s, unsigned n)
{
    unsigned long x;

    if (n <= 1) {
        return s;
    }
    x = s + n * 2654435761UL;
    x ^= x >> 15;
    x *= 40503UL;
    x ^= x >> 13;
    return s < 65536UL ? x & 0xFFFF : x | 0x10000UL;
}

/* One more rumour (the inn's Intrigue): the files' next if they have one, else themegen's (-r with one more, -a the kinds made: the
** same one dq0life's themegen -r would have made next), its dots shown; 1 one more */
static unsigned char rumour_more (void)
{
    static const char* const kinds[] = { "artifact", "evil", "prisoner", "through", "lore", "scourge" };
    static struct hy_regs r;
    static char a[160];
    unsigned char k, j, mask = 0, was = nr;
    char* q = a;

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
    ultoa (seed_of (L.h.seed, L.h.dungeon) ^ 0x52554D52UL, q += 3, 10);
    strcpy (q += strlen (q) + 1, "-o");
    strcpy (q += 3, top);
    q += strlen (q) + 1;
    *q = 0;
    word_out ();
    out_flush ();
    strcpy (path, lib);
    strcat (path, "/../../themegen");
    r.a = 0;
    r.r[0] = (unsigned) path;
    r.r[1] = (unsigned) a;
    if (!hy_call (HY_SPAWN, &r)) {
        r.r[0] = 0;
        hy_call (HY_WAIT, &r);
    }
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

/* A skill's test (the book's, Skills 5, as the library's harn_test: d100 against its EML held to 5-95, a success at or under it, a
** critical a roll of 5, 10 ... 100; here so the library's skills' table isn't linked): 0 a critical failure ... 3 a critical success */
static unsigned char __fastcall__ test (unsigned char eml)
{
    unsigned char r = d100 ();

    eml = eml < 5 ? 5 : eml > 95 ? 95 : eml;
    return r <= eml ? 2 + (r % 5 == 0) : 1 - (r % 5 == 0);
}

/* A haggle (Rhetoric's, the book's test; HOUSE.md's market: a critical success a quarter better, a success a tenth, a failure as
** asked, a critical failure an offence; never past two thirds of its worth either way) */
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

enum { P_STREET, P_INN, P_MARKET, P_SMITH, P_ASKING, P_LEAVE, P_SHUT };

/* Where a word goes: a place's number or its name, or a word that's done there; P_STREET none */
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
    return P_STREET;
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

/* The inn: a day; what's said there (Intrigue, once in a town, one more), a rest */
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
        say (whet ? "Nothing of yours wants only whetting.\n" : "Nothing of yours wants mending or whetting.\n");
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

/* ---- The town */

static unsigned char here;                        /* (Where it is: P_*) */

/* Into place p: its day (each visit), its words */
static void __fastcall__ go (unsigned char p)
{
    static const char* const keys[] = { 0, "inn", "market", "smith", "asking" };

    if (p == P_SHUT) {
        if (words_of ("shut", 0)) {
            say (v[0]);
            say ("\n");
        }
        return;
    }
    here = p;
    nh = 0;
    if (p != P_LEAVE) {
        days_pass (1);
        say_one (keys[p]);                      /* (The place's words, one by the dice; a day) */
        say ("\n");
    }
    switch (p) {
    case P_INN:
        inn_ ();
        break;
    case P_MARKET:
        tr.seen |= TS_MARKET;
        goods_say ("goods", "For sale: ");
        break;
    case P_SMITH:
        tr.seen |= TS_SMITH;
        smith_list ();
        break;
    case P_ASKING:
        asking ();
        break;
    case P_LEAVE:
        leave_list ();
        break;
    }
}

/* An offer taken, a place followed: 1 it was one */
static unsigned char __fastcall__ chosen (const char* w)
{
    unsigned char k = which (w, here == P_ASKING);

    if (k == 0xFF) {
        return 0;
    }
    if (here == P_ASKING) {
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
    if ((!strncmp (l, "go ", 3) || !strncmp (l, "visit ", 6) || !strncmp (l, "enter ", 6)) && q != P_STREET && q != P_LEAVE) {
        go (q);                                 /* ("go to the market") */
        return;
    }
    if (here == P_INN && *l >= '0' && *l <= '9') {
        rest (l);                               /* (The days to rest, asked) */
        return;
    }
    if ((here == P_LEAVE || here == P_ASKING) && (chosen (l) || (w && chosen (o)))) {
        return;
    }
    if (here == P_ASKING && *l >= '1' && *l <= '9') {
        say ("No one's asking that.\n");
        return;
    }
    if (p == P_STREET) {
        say ("There's nothing like that to do here: a number or a place's name, or Enter to go back.\n");
        return;
    }
    if (p != here && (p == P_SHUT || !w || (*l >= '1' && *l <= '9'))) {
        go (p);                                 /* (A place named: gone to) */
        return;
    }
    if (p != here && p != P_INN && !(p == P_MARKET && here == P_SMITH && strncmp (l, "sell", 4) && kind_named (o, "blades") != KI_NONE)) {
        go (p);                                 /* (Its errand's place gone to first, then done there) */
        if ((here == P_LEAVE || here == P_ASKING) && *o) {
            chosen (o);                         /* ("follow the second", "take Arlen's offer") */
            return;
        }
    }
    key = here == P_SMITH ? "blades" : "goods";
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
        if (here != P_INN) {
            go (P_INN);
        }
        rest (o);
    } else if (!strncmp (l, "listen", 6) || !strncmp (l, "hear", 4)) {
        if (here != P_INN) {
            go (P_INN);
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
    int fd, n, k;

    for (i = 1; i + 1 < argc; i += 2) {
        switch (argv[i][1]) {
        case 'd': top = argv[i + 1]; break;
        case 'l': lib = argv[i + 1]; break;
        case 't': town = argv[i + 1]; break;
        case 'i': inn = argv[i + 1]; break;
        case 'p': passed = atoi (argv[i + 1]); break;
        case 'k': keep = atol (argv[i + 1]); break;
        case 'h': habit = atoi (argv[i + 1]); break;
        case 'r': rhet = atoi (argv[i + 1]); break;
        case 'n': intr = atoi (argv[i + 1]); break;
        }
    }
    if (!top || !lib || !town || !inn) {
        fail ("usage: dq0town -d folder -l libdir -t town -i inn -p days -k keep -h habit -r rhetoric -n intrigue");
    }
    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) < 0 || read (fd, &L, sizeof L) != sizeof L) {
        fail ("no record of the life");
    }
    close (fd);
    if ((fd = hy_banks_alloc (2)) < 0) {
        fail ("no banks");
    }
    b_cat = fd;
    KINDS ();
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
    for (i = 1; (n = read (fd, said, sizeof said)) > 0; ) {   /* (Its lines but its notes: i at a line's start, 2 in a note) */
        WORDS ();
        for (k = 0; k < n && twn < TW_N - 1; ++k) {
            if (i == 1 && said[k] == '#') {
                i = 2;
            }
            if (i != 2 && said[k] != '\r') {
                tw[twn++] = said[k];
            }
            i = said[k] == '\n' ? 1 : i == 2 ? 2 : 0;
        }
        KINDS ();
    }
    close (fd);
    width_read ();
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
        ask (words_of ("prompt", here) ? v[0] : "");
        if (asks > TOWN_ASKS || tr.days >= TOWN_DAYS) {
            say ("The days go by, and at last you set out.\n");
            here = P_LEAVE;
            *ans = 0;
        }
        if (!*ans) {                            /* (Enter: back to the street; there, leave; leaving, the first) */
            if (here == P_LEAVE) {
                if (nr) {
                    follow (0);
                }
                done = 1;
            } else if (here) {
                here = P_STREET;
                menu ();
            } else {
                go (P_LEAVE);
            }
            continue;
        }
        command (ans);
    }
    cash_lay ();
    kit_write (TOWN_KIT);
    if ((fd = open (in_ (top, TOWN_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, &tr, sizeof tr) != sizeof tr) {
        fail ("can't write town.dat");
    }
    close (fd);
    say ("\n");
    word_out ();
    out_flush ();
    return 0;
}
