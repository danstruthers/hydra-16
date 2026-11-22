/*
** town.c - (the account's town overlay, acct.h's ACCT_OV_TOWN) the town the character comes into between two dungeons (the
** plan's step 11; HOUSE.md's between dungeons, the town): its name (the name maker's, namegen's, by the life's seed and the
** dungeon's number), its inn's; the date in words; where it lodged; the town itself, dq0town (a program of its own, in lib/acct:
** the town's menu, its market, its smith, its inn, its patrons, the road), run as the player's asked (the rc variable dq0auto, the
** tests', passes it by: straight on), and what it says was done there (the time between's record, between.dat: town.h's); the road
** taken from it; and the console's width, for the words.
*/

#pragma code-name ("TOWNCODE")
#pragma rodata-name ("TOWNRODATA")
#pragma bss-name ("TOWNBSS")                    /* (Not in the file: each set before it's read) */
#pragma local-strings (on)
#pragma static-locals (on)

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <dice.h>
#include "../hub.h"
#include "../life.h"
#include "acct.h"

#define roll    ng_roll                         /* (The name maker: themegen's bank program, its code here, its names its own) */
#define name    ng_name
#define run     ng_run
#include "../../../bank/namegen/namegen.c"
#undef roll
#undef name
#undef run

extern struct life L;

/* The console's width, for the words */
#define META_LINK
#define META_WIDTH
#include "../meta/meta.c"
#undef META_WIDTH
#undef META_LINK

/* The town's name, the men's tongue's, by the life's seed and the dungeon's number; its inn's, by the account's die (ans[0]) */
void o_town (void)
{
    static const char* const inns[] = { "the Ram", "the Bell", "the Plough", "the Three Keys", "the Black Ox", "the Sheaf", "the Crown",
        "the Fleece" };
    static struct nq q;

    q.seed = (L.h.seed ^ 0x544F574EUL) + L.h.dungeon * 2654435761UL;    /* ("TOWN") */
    q.tongue = 2;
    q.snd = ng_first[1];                        /* (The first world's men's tongue: ../worlds/first/tongues.h) */
    q.form = NG_PLACE;
    ng_name (&q);
    strcpy (town, q.out);
    strcpy (inn, inns[ans[0] & 7]);
}

/* ---- The date */

void o_date (void)
{
    static const char months[12][10] = { "Nuzyael", "Peonu", "Kelen", "Nolus", "Larane", "Agrazhar", "Azura", "Halane", "Savor", "Ilvin",
        "Navek", "Morgat" };                    /* (The game's calendar, life.h's: the library's harn_month_name's names) */

    sayn (LIFE_DAY (o_t));
    say (" ");
    say (months[LIFE_MONTH (o_t) - 1]);
    say (" ");
    sayn (LIFE_YEAR (o_t));
}

/* ---- Where it lodged: the days in town, if it went into it, then its lodging all the days, what it came to */

void o_lodging (void)
{
    static const char* const lodge[9] = { 0, "You sleep in a hayloft and eat market bread", "You take a place in the common room",
        "You share a room", 0, 0, 0, 0, "You take a room of your own, with a servant to help" };
    static const char* const lodge2[9] = { 0, "", " (a bed and a hot meal and a cold", " (hot meals and ale", 0, 0, 0, 0,
        " (hot meals and wine" };

    if (town_days) {
        say ("Your time in ");
        say (town);
        say (" comes to ");
        sayn (town_days);
        say (town_days == 1 ? " day. " : " days. ");
    }
    if (dead) {
        return;
    }
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

/* ---- The names given in the dungeon just left, the record written, the lull, the days of work (the main RAM's room) */

/* The names given in the dungeon just left (a thing's: its id) */
void __fastcall__ names_read (unsigned n)
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

void life_write (void)
{
    int fd;

    if ((fd = open (in_ (top, LIFE_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, &L, sizeof L) != sizeof L) {
        fail ("can't write the record of the life");
    }
    close (fd);
}

/* ---- The lull after the road (the plan's: a week at most half the time, a season at the most) */

unsigned char lull (void)
{
    unsigned char r = d100 ();

    return r <= 50 ? die (7) : r <= 80 ? 14 + random (8) : r <= 95 ? 30 + random (31) : 90 + random (91);
}

/* ---- The days of work, what they earned (what they taught: the ruleset's acct_worked, after) */
void o_workdays (void)
{
    const char* c;

    say ("For ");
    sayn (work_days);
    say (" of the days you work");
    if (*(c = acct_calling ())) {
        say (" as a ");
        say (c);
    }
    say (", your keep found, for half a master's wage (");
    amount (earned);
    say (").");
}

/* ---- The town (dq0town: its ruleset's link of it, ACCT_TOWN, the link's town.c's: HarnMaster's dq0town, the null's dq0ntown) */

static struct between tr;

/* An argument more in ob (after p): its flag and its value; the next's place; dq0town run and waited for (the
** meta programs' library's, ../meta/meta.c) */
#define META_ARG
#define META_RUN
#include "../meta/meta.c"
#undef META_ARG
#undef META_RUN

/* dq0town run (SPAWN itself, as dq0life runs this: its arguments in main RAM), waited for, and the time between's record read: 1
** it ran.  What it's told, in the record (records.def's struct between, between.dat): the town's name and its inn's, the days gone
** by, its keep owed so far and a day's by its station (the ruleset's acct_habit, the money's matters' habit), its knacks for the
** market's haggling and the inn's listening (the ruleset's acct_town); its arguments the folder and the lib alone */
unsigned char o_town_go (void)
{
    unsigned char h = acct_habit (), ok;
    char* p = ob;
    int fd;

    say_flush ();
    memset (&tr, 0, sizeof tr);
    tr.magic = BETWEEN_MAGIC;
    tr.version = BETWEEN_VER;
    strcpy (tr.town, town);
    strcpy (tr.inn, inn);
    tr.passed = passed;
    tr.keep = fee + (unsigned long) h * passed * PENNY;
    tr.habit = h;
    acct_town (&tr);
    if ((ok = (fd = open (in_ (top, BETWEEN_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) != 0) {
        ok = write (fd, &tr, sizeof tr) == sizeof tr;
        close (fd);
    }
    p = arg (arg (p, "-d", top), "-l", lib);
    *p = 0;
    strcpy (path, lib);
    strcat (path, "/../acct/" ACCT_TOWN);
    if (ok) {
        ok = !meta_run (path, ob) && (fd = open (in_ (top, BETWEEN_FILE), O_RDONLY)) >= 0;
    }
    if (ok) {
        ok = read (fd, &tr, sizeof tr) == sizeof tr && tr.magic == BETWEEN_MAGIC && tr.version == BETWEEN_VER;
        close (fd);
    }
    remove (in_ (top, BETWEEN_FILE));
    if (!ok) {
        memset (&tr, 0, sizeof tr);
        return 0;
    }
    town_days = tr.days;
    town_seen = tr.seen;
    town_taught = tr.taught;                    /* (The scholar's house's: the months with a master, the script studied, what
                                                **   was opened, the plan's step 12) */
    strcpy (town_study, tr.study);
    for (ok = 0; ok < BETWEEN_LEARNT && tr.learnt[ok][0]; ++ok) {
        if (!acct_learnt (tr.learnt[ok])) {     /* (Its sheet full: the scholar's won't teach past it, never cut short) */
            say ("There's no more room on your sheet for ");
            say (tr.learnt[ok]);
            say (".\n");
        }
    }
    return 1;
}

/* The road taken from the town (the place followed, quest.txt's, dq0town's: its days on the date), and Enter to go on, as the game
** clears the screen as it starts */
void o_set_out (void)
{
    if (*tr.whither) {
        say ("You set out for ");
        say (tr.whither);
        say (", ");
        sayn (tr.road);
        say (" days on the road.\n");
        L.h.date += tr.road;
    }
    say ("Enter to go on");
    ask (ans, sizeof ans);
}
