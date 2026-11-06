/*
** town.c - (dq0acct, overlay 5, a file of its own: dq0acct.ov5) the town the character comes into between two dungeons (the
** plan's step 11; HOUSE.md's between dungeons, the town): its name (the name maker's, namegen's, by the life's seed and the
** dungeon's number), its inn's; the date in words; where it lodged; the town itself, dq0town (a program of its own, in lib/acct:
** the town's menu, its market, its smith, its inn, its patrons, the road), run as the player's asked (the rc variable dq0auto, the
** tests', passes it by: straight on), and what it says was done there (town.h's town.dat); the road taken from it.
*/

#pragma code-name ("OV5CODE")
#pragma rodata-name ("OV5RODATA")
#pragma bss-name ("OV5BSS")                    /* (Not in the file: each set before it's read) */
#pragma local-strings (on)
#pragma static-locals (on)

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/life.h"
#include "../dq0harn/dq0harn.h"
#include "dq0acct.h"

#define roll    ng_roll                         /* (The name maker: themegen's bank program, its code here, its names its own) */
#define name    ng_name
#define run     ng_run
#include "../../bank/namegen/namegen.c"
#undef roll
#undef name
#undef run

extern struct life L;

/* The town's name, the men's tongue's, by the life's seed and the dungeon's number; its inn's, by the account's die (ans[0]) */
void o_town (void)
{
    static const char* const inns[] = { "the Ram", "the Bell", "the Plough", "the Three Keys", "the Black Ox", "the Sheaf", "the Crown",
        "the Fleece" };
    static struct nq q;

    q.seed = (L.h.seed ^ 0x544F574EUL) + L.h.dungeon * 2654435761UL;    /* ("TOWN") */
    q.tongue = 2;
    q.form = NG_PLACE;
    ng_name (&q);
    strcpy (town, q.out);
    strcpy (inn, inns[ans[0] & 7]);
}

/* ---- The physician's words, the wounds' */

/* Where a wound is, "left forearm" */
static void __fastcall__ say_place (unsigned char loc)
{
    static const char* const locations[HARN_NLOCATIONS] = {
        "skull", "face", "neck", "shoulder", "upper arm", "elbow", "forearm", "hand", "thorax", "abdomen", "groin", "hip",
        "thigh", "knee", "calf", "foot"
    };
    unsigned char l = HARN_LOC (loc);

    if ((l >= HARN_SHOULDER && l <= HARN_HAND) || l >= HARN_HIP) {
        say (loc & HARN_RIGHT ? "right " : "left ");
    }
    say (locations[l < HARN_NLOCATIONS ? l : 0]);
}

static const char* const wound_word[9][3] = {
    { "bruise", "fracture", "crush" }, { "cut", "gash", "deep gash" }, { "stab", "stab", "deep stab" },
    { "burn", "burn", "burn" }, { "bite", "bite", "deep bite" }, { "scratch", "rake", "deep rake" },
    { "sting", "sting", "deep sting" }, { "gore", "gore", "deep gore" }, { "ache", "ache", "ache" }
};

static const char* const sev_word[4] = { "minor", "serious", "grievous", "grievous" };

static unsigned char __fastcall__ sev_of (unsigned char il)
{
    return il >= 4 ? 2 : il >= 2 ? 1 : 0;
}

/* What's become of each wound there was */
static void wounds_say (void)
{
    const struct wwas* w;
    unsigned char i, k, s;

    for (i = 0; i < nwas; ++i) {
        w = &was[i];
        for (k = 0; k < pc.ninj && wid[k] != i; ++k) {
        }
        say ("The ");
        say (sev_word[sev_of (w->il)]);
        say (" ");
        say (wound_word[w->by && w->by <= BY_VENOM ? 3 + w->by : w->aspect & 3][(w->aspect & 3) != HARN_BLUNT || w->by ? 0 : w->sev > 3 ? 2 : w->sev ? w->sev - 1 : 0]);
        say (" to your ");
        say_place (w->loc);
        if (k == pc.ninj) {
            say (w->fever ? " festers, then mends and is gone. " : " is gone. ");
        } else if (pc.inj[k].il < w->il) {
            s = sev_of (pc.inj[k].il);
            say (s == sev_of (w->il) ? " mends a little." : " heals to a ");
            if (s != sev_of (w->il)) {
                say (sev_word[s]);
                say (" one.");
            }
            say (" ");
        } else {
            say (w->fever ? " festers, and is no better. " : " is no better. ");
        }
    }
}

/* The physician's words, what's become of the wounds */
void o_first (void)
{
    unsigned char k;

    if (ntended) {
        say ("A physician sees to your ");
        for (k = 0; k < ntended; ++k) {
            say_place (tended[k]);
            say (k + 2 < ntended ? ", " : k + 2 == ntended ? " and " : "");
        }
        say (" (");
        amount (fee);
        say ("). ");
    }
    if (selfnd) {
        say ("You tend what you can yourself. ");
    }
    if (unpaid) {
        say ("There's no money for a physician. ");
    }
    wounds_say ();
}

/* ---- The date */


void o_date (void)
{
    static const char months[12][10] = { "Nuzyael", "Peonu", "Kelen", "Nolus", "Larane", "Agrazhar", "Azura", "Halane", "Savor", "Ilvin",
        "Navek", "Morgat" };                    /* (The library's harn_month_name's: none of the rest of its names linked) */

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
    if (pc.state == HARN_DEAD) {
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

/* ---- The town (dq0town) */

static struct townrec tr;

/* An argument more in ob (after p): its flag and its value; the next's place */
static char* __fastcall__ arg (char* p, const char* flag, const char* v)
{
    strcpy (p, flag);
    p += strlen (p) + 1;
    strcpy (p, v);
    return p + strlen (p) + 1;
}

static char* __fastcall__ argn (char* p, const char* flag, unsigned long v)
{
    char b[12];

    return arg (p, flag, ultoa (v, b, 10));
}

/* dq0town run (SPAWN itself, as dq0life runs this: its arguments in main RAM), waited for, and town.dat read: 1 it ran.  What it's
** told: the folder, the lib, the town's name and its inn's, the days gone by, its keep owed so far and a day's by its class (the
** money's matters' habit), its Rhetoric and its Intrigue (their EMLs: the market's haggling, the inn's listening) */
unsigned char o_town_go (void)
{
    static const unsigned char habits[5] = { 1, 1, 2, 3, 8 };
    static struct hy_regs r;
    unsigned char h = habits[pc.social < 5 ? pc.social : 2], ok;
    char* p = ob;
    int fd;

    say_flush ();
    p = arg (p, "-d", top);
    p = arg (p, "-l", lib);
    p = arg (p, "-t", town);
    p = arg (p, "-i", inn);
    p = argn (p, "-p", passed);
    p = argn (p, "-k", fee + (unsigned long) h * passed * PENNY);
    p = argn (p, "-h", h);
    fd = harn_eml (&pc, HARN_RHE);
    p = argn (p, "-r", fd < 0 ? 0 : fd);
    fd = harn_eml (&pc, HARN_NTR);
    p = argn (p, "-n", fd < 0 ? 0 : fd);
    *p = 0;
    strcpy (path, lib);
    strcat (path, "/../acct/dq0town");
    tr.days = tr.seen = tr.road = 0;
    r.a = 0;                                    /* (Its fds: this one's 0, 1 and 2) */
    r.r[0] = (unsigned) path;
    r.r[1] = (unsigned) ob;
    if (hy_call (HY_SPAWN, &r)) {
        return 0;
    }
    r.r[0] = 0;
    if (hy_call (HY_WAIT, &r) || r.x) {
        return 0;
    }
    strcpy (path, top);
    strcat (path, "/" TOWN_FILE);
    if ((fd = open (path, O_RDONLY)) < 0) {
        return 0;
    }
    ok = read (fd, &tr, sizeof tr) == sizeof tr;
    close (fd);
    remove (path);
    if (!ok) {
        return 0;
    }
    town_days = tr.days;
    town_seen = tr.seen;
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
