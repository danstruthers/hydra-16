/*
** acct.c - Dingus Quest Zero's account of the time between two dungeons, the engine's (the plan for the game between dungeons,
** its steps 4 to 6; DUNGEON.md's a character's life; HOUSE.md's between dungeons): its main RAM.  One source, linked once for
** each ruleset with that ruleset's part of it (../rsmeta.h's acct_* hooks: its sheet, its healing, its practice, its living
** and wages; acct.h has the banks): HarnMaster's dq0acct, the null ruleset's dq0nacct, each a program in lib/acct (with its
** overlays: the start's time tells where a new file goes, DUNGEON.md's), that dq0life runs (rsmeta.h's table, by the save's
** ruleset) as a dungeon's been ended alive and before the next begins.  Before any of the game's tasks: none of the game's main
** RAM, the CPU its own.  On the saved files: the sheet (pc.hm, the ruleset's), what's carried (kit.dat), what the life set out
** with (kit0.dat), the record (life.dat).  A program apart from dq0life so a new life's start reads none of it.
**
** NAME -d folder -l libdir
** By the life's seed and the dungeon's number alone (and what's said to its question): the time between, by the plan's three
** things (the days its wounds take, the ruleset's, at most 90; the road; the lull); its wounds the ruleset's, told the days at
** rest; living, selling, mending, whetting, restocking and work; the month's practice, the ruleset's.  It's told in a dozen
** lines, and the one question asked (its answer filled in), then Enter: the game clears the screen as it starts.  The rc
** variable dq0auto (the tests'): nothing waited for, the answers as they're filled in.  The player asked (no dq0auto), the town
** (dq0town, the plan's step 11: town.c's) comes after the first lines and before the money's matters, which then leave alone what the
** player saw to there.
** Its exit status: 0; 2 the character dead of it (the record says so, dq0life's to tell the life's account); 1 its fault said.
** The rules' numbers marked [Suggest] in the plan: HOUSE.md's between dungeons, for the user's review.
**
** The link's own file defines ACCT_NAME (its program's name: its overlays' file, lib/acct/ACCT_NAME.ovl) and ACCT_NOV (its
** overlays: the account's two and the ruleset's) before it includes this.
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <dice.h>
#include "../hub.h"
#include "../life.h"
#include "../theme.h"
#include "../../dq0w/outfit.h"
#include "acct.h"

#pragma static-locals (on)

/* A bank of this program's at the window: the kinds (dq0w.cat's first part), what's carried */
#define CAT             ((const struct kind*) 0x8000)
#define K               ((struct kitrec*) (0x8000 + KI_COUNT * sizeof (struct kind)))
typedef char bank_fits[KI_COUNT * 37 + NOFIT * 22 <= 0x2000 ? 1 : -1];

struct life L;                                  /* (quest.c's too) */
static unsigned char kn;
static unsigned char gone[NOFIT];
const char* top;
const char* lib;
char path[96];
char ans[64];
unsigned char askit;                            /* (Waited for: not the tests' dq0auto) */

/* ---- The meta programs' library (../meta/meta.c): the fault said, a path, the words by the console's width (fail, in_,
** say, sayn, amount, say_flush, width and ob: the overlays' too, acct.h's) */

#define META_NAME       ACCT_NAME
#define META_WB         96
#define META_OB         200
#define META_LINK
#define META_FAIL
#define META_PATH
#define META_WORDS
#include "../meta/meta.c"
#undef META_FAIL
#undef META_PATH
#undef META_WORDS

/* A line typed (b, 0-ended, its end of line gone); with the tests' dq0auto, none: "" */
void __fastcall__ ask (char* b, unsigned char n)
{
    int k;

    say_flush ();
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

#include "goods.h"

unsigned named[24];                             /* (The ids of what the player's named: the dungeon's game.dat's, town.c's names_read) */
unsigned char nnamed;
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

static unsigned char needed[NOFIT];             /* (What its calling or its practice needs: the ruleset's acct_tool, each thing's) */

/* Spare: never what's named, its armour, what's worn, what's in its hand, what's used up (it'd buy it back), a key, what it lights
** its light with, nor what holds anything; its weapon if none's in its hand; nor what its calling or its practice needs (its
** writing things, its tools: the plan's step 11, HOUSE.md's) */
static unsigned char __fastcall__ spare (unsigned char i)
{
    const struct kitrec* r = &K[i];
    const struct kind* k = &CAT[r->o.kind];

    return !gone[i] && !is_coin (r) && !used_up (r->o.kind) && r->o.kind != KI_WATER && r->o.kind != KI_KEY && r->o.kind != KI_TINDERBOX && k->value &&
        r->o.place != PL_WORN && !in_hand (r) && k->craft != AP_ARMOUR && !is_named (r->id) && i != kept_blade && !holds (i) && !needed[i];
}

static unsigned char __fastcall__ valuable (const struct kitrec* r)
{
    const struct kind* k = &CAT[r->o.kind];

    return k->craft == AP_JEWEL || k->mat == MT_GOLD || k->mat == MT_SILVER;
}

static void __fastcall__ kit_read (const char* name)
{
    int fd;
    unsigned char k, armed = 0;
    unsigned long best = 0;

    kn = 0;
    if ((fd = open (in_ (top, name), O_RDONLY)) >= 0) {
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

/* ---- Money */

unsigned long cash;                             /* (Farthings: quest.c's too) */

/* ---- The days' words (town.c's: the town's name and its inn's, the date) */

char inn[16];
char town[16];
unsigned town_days;
unsigned char town_seen;
unsigned char town_taught;
char town_study[10];
unsigned long o_t;                              /* (o_date's: the main RAM's, an overlay's own BSS being at the window) */

/* ---- The money's matters */

unsigned long funds, fee, earned;
unsigned passed;
unsigned char dead;

/* The overlays (a bank each, the kinds' put back) */
unsigned char acct_b;

#define QUEST(x)    (hy_bank (acct_b + ACCT_OV_QUEST), x)
#define TOWN(x)     (hy_bank (acct_b + ACCT_OV_TOWN), x)

static void __fastcall__ say_date (unsigned long t)
{
    o_t = t;
    TOWN (o_date ());
    ACCT_CATS ();
}

/* A day at rest, the ruleset's, counted: 1 alive */
static unsigned char a_day (void)
{
    unsigned char r = acct_day ();

    ++passed;
    dead = !r;
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

static const unsigned char steps[4] = { 8, 3, 2, 1 };  /* (The lodgings, a day's pence: the plan's table) */

unsigned char rate;
static unsigned char restocks;
unsigned long living;
static unsigned long sold_t, smith_t, buy_t;
unsigned work_days;
static unsigned char sold[NOFIT], nsold;
static unsigned char mended[NOFIT], whetted[NOFIT], nmend, nwhet, nosmith;

/* What it set out with of what's used up (kit0.dat: the world's first save of the life, as it came in), less what it has: each
** kind wanted and how many */
static struct want wants[16];
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
        if (K[k].dur_max && !K[k].fuel && (K[k].o.kind == KI_TORCH || K[k].o.kind == KI_CANDLE || K[k].o.kind == KI_PITCH)) {   /* (A flask of oil a vessel, its oil in it: H16) */
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

/* All it wants paid for, by what it has now: nothing goes on seeing to a thing it's sold, nor on what the thing it's sold was for;
** nor what the player saw to itself in town (the smith's, the market's: town_seen) */
static unsigned long upkeep (void)
{
    unsigned long t = 0;
    unsigned char k;

    for (k = 0; k < kn && !(town_seen & TS_SMITH); ++k) {
        if (!gone[k]) {
            t += owed (&K[k]);
        }
    }
    for (k = 0; k < nwants && !(town_seen & TS_MARKET); ++k) {
        if (of_use (wants[k].kind)) {
            t += wants[k].n * (unsigned long) CAT[wants[k].kind].value;
        }
    }
    return t;
}

/* All the money's matters, by the plan's order: sold what's spare till the wants are met, cheapest first, valuables last, never what
** it's named; down to rough; work for what's short at half a master's wage (the ruleset's acct_wage), its room and board found (the
** book's journeyman, Character 13), at most WORK_MAX days; then what it can pay: its living (a day's by its station, the ruleset's
** acct_habit), the mending, the restocking.  No debts.  What the player saw to itself in town is its own: the market seen, nothing
** sold or bought for it; the smith, nothing mended or whetted */
static void money_matters (void)
{
    unsigned long need, nowant, v, best;
    unsigned char k, b, habit = acct_habit (), val;
    unsigned char s;
    unsigned w;

    need = fee + (unsigned long) habit * passed * PENNY;
    for (val = town_seen & TS_MARKET ? 2 : 0; val < 2; ++val) {
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
        w = acct_wage ();
        v = ((need - cash) * 15 + w - 1) / w;   /* (A month's w pence at half: w / 15 farthings a day) */
        work_days = v > WORK_MAX ? WORK_MAX : (unsigned) v;
    }
    living = (unsigned long) rate * passed * PENNY;
    for (w = 0; w < work_days; ++w) {
        if (!a_day ()) {
            return;
        }
    }
    earned = (unsigned long) work_days * acct_wage () / 15;
    cash += earned;
    cash = cash > fee ? cash - fee : 0;
    if (cash < living) {
        living = cash;                          /* (What's short found for nothing: a barn, a kitchen's scraps) */
    }
    cash -= living;
    for (k = 0; k < kn && !(town_seen & TS_SMITH); ++k) {
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
    for (k = 0; k < nwants && !(town_seen & TS_MARKET); ++k) {
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

/* What's carried told to the ruleset (acct_tool) a thing at a time: its kind, its flags, its tool's key, in ans, for the kinds
** aren't at the window with the ruleset's overlays: what's left to hand to practise with, after the money's matters; or (needs)
** what its calling or its practice needs, before them (never sold unasked) */
static void __fastcall__ tools_note (unsigned char needs)
{
    unsigned char k;

    ans[8] = 0;
    acct_tool (0);
    for (k = 0; k < kn; ++k) {
        if (!gone[k]) {
            ans[8] = K[k].o.kind;
            ans[9] = CAT[ans[8]].flags;
            memcpy (ans, CAT[ans[8]].tool, 8);
            if (needs) {
                needed[k] = acct_tool (1);
            } else {
                acct_tool (0);
            }
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

/* ---- The account */

static unsigned char told;                      /* (Its first lines told before the town, the player's: 1; and the town run, 2) */

/* The days' first lines: what's paid, where it lodged (told after the town, if there's one), its wounds (the ruleset's) */
static void first_lines (void)
{
    say ("\n");
    sayn (passed);
    say (passed == 1 ? " day passes. You walk to " : " days pass. You walk to ");
    say (town);
    say (". ");
    QUEST (o_reward_say ());                    /* (What it's paid for what it did, the plan's step 7) */
    if (!told) {
        TOWN (o_lodging ());
    }
    ACCT_CATS ();
    acct_first ();
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
    sayn (LIFE_AGE (&L.h, d, acct_birthday ()));
    QUEST (o_standing ());                      /* (The sentence's end, the standing: the life's reputation, the plan's step 7) */
    ACCT_CATS ();
}

static void dead_lines (void)
{
    unsigned long d = L.h.date + passed;
    const char* s;

    if (!told) {
        first_lines ();
    }
    s = acct_death ();
    say (*s == ' ' ? s + 1 : s);
    say (" You die on ");
    say_date (d);
    say (", at ");
    sayn (LIFE_AGE (&L.h, d, acct_birthday ()));
    say (".\n");
    say_flush ();
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
        fail ("usage: " ACCT_NAME " -d folder -l libdir");
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
    if ((b = hy_banks_alloc (ACCT_NOV + 1)) < 0) {
        fail ("no banks");
    }
    acct_b = b;
    strcpy (path, lib);                         /* (The overlays: the account's, then the ruleset's, in ACCT_NAME.ovl) */
    strcat (path, "/../acct/" ACCT_NAME ".ovl");
    if ((fd = open (path, O_RDONLY)) < 0) {
        fail ("no overlays");
    }
    for (i = 1; i <= ACCT_NOV; ++i) {
        hy_bank (acct_b + i);
        read (fd, (void*) 0x8000, 0x2000);
    }
    close (fd);
    TOWN (width_read ());
    ACCT_CATS ();
    strcpy (path, lib);
    strcat (path, "/dq0w.cat");
    if ((fd = open (path, O_RDONLY)) < 0 || read (fd, (void*) 0x8000, KI_COUNT * sizeof (struct kind)) != KI_COUNT * sizeof (struct kind)) {
        fail ("no catalogue");
    }
    close (fd);
    if (!acct_sheet_read ()) {
        L.h.flags &= ~LF_BETWEEN;               /* (No sheet: nothing to tell) */
        TOWN (life_write ());
        return 0;
    }
    n = L.h.dungeon;
    seed = L.h.seed + n * 2654435761UL;
    rseed (seed ? seed : 1);
    random (8);                                 /* (The dice as they were: the town's name the name maker's now, its own) */
    random (8);
    ans[0] = random (8);
    TOWN (o_town ());
    ACCT_CATS ();
    kit_read ("kit.dat");
    TOWN (names_read (n - 1));
    ACCT_CATS ();
    cash_count ();
    QUEST (o_reward ());                        /* (What solving the dungeon just left brings, its theme's reward line: its pennies
                                                **   into the money and its standing into the life's reputation, there; a thing
                                                **   given, a kind of the catalogue's by its name, a ring, put in its pouch, made new) */
    ACCT_CATS ();
    for (i = *rw_thing ? 0 : KI_COUNT; i < KI_COUNT && strcmp (CAT[i].name, rw_thing); ++i) {
    }
    if (i < KI_COUNT) {
        coin_add (i, 1);
    }
    funds = cash + sell_total ();
    /* The wounds first (the ruleset's: what's done at once, a physician); then the days they take, the road, the lull, all at rest */
    more = acct_wounds ();
    dead = !more;
    while (more && !acct_mended () && passed < HEAL_MAX) {
        more = a_day ();
    }
    if (more) {
        TOWN (gap = dice (2, 6) + lull ());     /* (Its road, its lull) */
        ACCT_CATS ();
    }
    for (; more && gap; --gap) {
        more = a_day ();
    }
    if (more) {
        wants_read ();
        if (askit) {                            /* (The town, the player's: the account's first lines told before it, the
                                                **   things and the money handed to dq0town in the files, read again after;
                                                **   its days at rest; what the player didn't see to itself done as ever) */
            told = 1;
            first_lines ();
            cash_lay ();
            kit_write (TOWN_KIT);
            TOWN (told += o_town_go ());
            ACCT_CATS ();
            memset (gone, 0, sizeof gone);
            kit_read (TOWN_KIT);
            remove (path);
            cash_count ();
            nwants = 0;
            wants_read ();
            for (n = town_days; more && n; --n) {
                more = a_day ();
            }
        }
        if (more) {
            tools_note (1);
            money_matters ();
            more = !dead;
        }
    }
    if (!more) {                                /* (Dead of it: the outcome died, the record the life's end; dq0life tells it) */
        dead_lines ();
        acct_sheet_write ();
        L.h.date += passed;
        L.h.flags = (L.h.flags & ~LF_BETWEEN) | LF_OVER | LF_ABROAD;
        --L.h.dungeon;
        if (L.h.dungeon < LIFE_DUNGEONS) {
            L.d[L.h.dungeon].seed = 0;
        }
        TOWN (life_write ());
        return 2;
    }
    water_in ();
    tools_note (0);
    acct_rested ();
    cash_lay ();
    /* It's told (its first lines before the town, if there was one: the rest after it) */
    if (!told) {
        first_lines ();
    } else {
        TOWN (o_lodging ());
        ACCT_CATS ();
    }
    goods_lines ();
    if (work_days) {                            /* (The days of work, what they earned; what they taught, the ruleset's) */
        TOWN (o_workdays ());
        ACCT_CATS ();
        acct_worked ();
        say ("\n");
    }
    acct_practice ();
    last_lines ();
    hy_bank (acct_b + ACCT_OV_QUEST);           /* (Where next: the road taken from the town; else the rumours heard, one
                                                **   followed, or Enter to go on; asked before anything's written, so a reset
                                                **   tells the time between again, the same) */
    if (told == 2) {
        TOWN (o_set_out ());
    } else {
        o_rumours ();
    }
    ACCT_CATS ();
    say_flush ();
    acct_sheet_write ();
    kit_write ("kit.dat");
    L.h.date += passed;
    L.h.flags &= ~LF_BETWEEN;
    TOWN (life_write ());
    return 0;
}
