/*
** dq0schol.c - Dingus Quest Zero's scholar's house, in the town between two dungeons (the plan for the game between dungeons, its
** section 4 and step 12; the user's: "collect clues and take them back to a library or to a scholar ... Each clue might be in a
** unique script that needs a specialist to help decipher, or time in the library if the character is literate"; HOUSE.md's the
** scholar's house): a program of its own in lib/acct, run by the town (dq0town) as the character goes in, before any of the
** game's tasks: none of their main RAM, none of their answer times.
**
** dq0schol -d folder -l libdir -t town [-x first] [-b big -c money -w ids]
**   the save's folder; the game's lib; the town's name; the first command (what was typed in the street); and from town.dat
**   (dq0town's, town.h's): the money (farthings, handed back as it's left), whether it's a big town (town.txt's kind: a sage for
**   a dead tongue), the writings carried (their things' fuel, life.h's: an old writing's number at the top, a copy's CLUE_COPY
**   too); or (-b, -c, -w: a scene's, the tests', run alone) whether it's big (1), the money, the writings' fuel (a space between)
** What it works on: the save's top folder's clues.dat (life.h's struct clue: what the life's carried out of its dungeons), its
** sheet (pc.hm: its skills, its tongues and scripts), the record (life.dat: its seed), town.dat (town.h's: the days in town so
** far, read and written back with the days it took here, what it cost, the months with a master, what a master opened, the
** script studied), heard.txt (the rumour a writing read makes: rewritten as a writing's read better here).  Its words in
** scholar.txt beside it, and below.  The same life's seed and the same commands, the same scholars, prices and readings.
**   read 1           a scribe reads a writing carried (an old writing, a copy: about 3d a working day, some days); a dead
**                    tongue's (an ancient writing's, the book's Treasure 8: its tongue's old form) a sage, three times the
**                    price, only in a big town.  Its roll hidden: the reading told, never its grade (the book's, Skills 5:
**                    "the GM should make the roll secretly"; Skills 20's analysis); asked again, another's reading, a second
**                    opinion, at the same price.  Each one's honesty its Morality (the book's attribute, 3d6, Character 9's
**                    bands): corruptible or worse sells a copy on (talk in the street, nothing more yet), unscrupulous or worse
**                    asks half again, diabolical tells a wrong reading as confidently whatever it read
**   study 1          the library, for one who knows the writing's script and tongue: a penny a day, a week at a time, the
**                    reading test of step 9 each week (the masteries' average, the condition: the writing's, a copy's) till
**                    it's read, four weeks unless it's told ("study 1 for 2 weeks"); the script then the month's practice's
**                    first (town.dat's study)
**   learn delvic     a master's month: a script opened for good at 70 + its skill base (the book's, Skills 11), a tongue at
**                    its skill base (an alien family's, Skills 11), a sage's wage (84d a month, Character 13); the month's
**                    practice forfeit for it (the book's 30 points, a tongue's whole month)
**   folklore 1, reckon 1, astrology 1, heraldry 1   the other skills, once each for a writing: the character's own, or a sage's
**                    (3d), a day; each something true and small of what the clue's record holds (HOUSE.md's)
** A better reading replaces the clue's (the place, the reason, the line more), and heard.txt's rumour with it; a worse one
** doesn't undo a better.  Its exit status 0, or 1 (its fault said).
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
#include "../dq0harn/dq0harn.h"
#define TOWN_ROAD
#include "../dq0acct/town.h"

#define roll    ng_roll                         /* (The name maker: themegen's bank program, its code here, as the account's) */
#define name    ng_name
#define run     ng_run
#include "../../bank/namegen/namegen.c"
#undef roll
#undef name
#undef run

#pragma static-locals (on)
#pragma local-strings (on)

#define PENNY           4UL                     /* (Farthings) */
#define ASKS            40                      /* (The lines typed in a visit at most: the soak's random answers end) */
#define NCL             9                       /* (The writings told, at most: the latest) */
#define SCRIBE_DAY      3                       /* (A scribe's working day, pence: the book's 66d a month, Character 13, over 24 days) */
#define DEAD_TIMES      3                       /* (A dead tongue's sage: three times that, the plan's) */
#define MASTER_MONTH    84                      /* (A master's month: the book's sage's, 84d, Character 13) */
#define SAGE_DAY        3                       /* (A sage asked one of the other skills: a day's) */

harn_char pc;
struct life L;
static struct townrec tr;
static struct clue K;
static const char* top;
static const char* lib;
static const char* town;
#define cash    tr.money
#define big     tr.big
static char path[96];
static char ans[64];
static unsigned char asks;

static void __fastcall__ fail (const char* why)
{
    write (2, "dq0schol: ", 10);
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

/* ---- Words, by the console's width (dq0town's) */

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
    word_out ();
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

/* ---- Its words (scholar.txt: a key's lines, each its values between bars) */

static char tw[1280];
static unsigned twn;
static char* v[16];
static unsigned char nv;
static char lb[400];

/* The n'th line of key (0 the first): its values in v (nv of them, their spaces trimmed); 0 none */
static unsigned char __fastcall__ words_of (const char* key, unsigned char n)
{
    unsigned p = 0, e;
    unsigned char k = strlen (key);
    char* s;

    for (; p < twn; p = e + 1) {
        for (e = p; e < twn && tw[e] != '\n'; ++e) {
        }
        if (strncmp (tw + p, key, k) || (tw[p + k] != ' ' && tw[p + k] != '|') || n--) {
            continue;
        }
        memcpy (lb, tw + p, e - p < sizeof lb - 1 ? e - p : sizeof lb - 1);
        lb[e - p < sizeof lb - 1 ? e - p : sizeof lb - 1] = 0;
        for (nv = 0, s = strchr (lb, '|'); s && nv < sizeof v / sizeof v[0]; s = strchr (s, '|')) {
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
    return nv = 0;
}

/* One of a key's lines by the dice, its first value said */
static void __fastcall__ say_one (const char* key)
{
    unsigned char n = 0;

    while (words_of (key, n)) {
        ++n;
    }
    if (n && words_of (key, random (n))) {
        say (v[0]);
        say (" ");
    }
}

/* Word w (n letters) among key's values: its place, or 0xFF */
static unsigned char __fastcall__ among (const char* key, const char* w, unsigned char n)
{
    unsigned char k;

    words_of (key, 0);
    for (k = 0; k < nv; ++k) {
        if (strlen (v[k]) == n && !strncmp (v[k], w, n)) {
            return k;
        }
    }
    return 0xFF;
}

/* ---- The dice: the life's, the town's days so far (so the same commands, the same scholars); and the tests' scenes (the rc
** variable dq0sg: numbers, each taken in turn by a roll that's set: a scribe's Morality, then its grade; the library's grade a
** week; another skill's grade) */

static char fq[48];
static unsigned char fp;

static unsigned char forced (void)
{
    unsigned char n;

    while (fq[fp] == ' ') {
        ++fp;
    }
    if (fq[fp] < '0' || fq[fp] > '9') {
        return 0xFF;
    }
    n = atoi (fq + fp);
    while (fq[fp] >= '0' && fq[fp] <= '9') {
        ++fp;
    }
    return n;
}

/* A skill's test (the book's, Skills 5: d100 against its EML held to 5-95, a success at or under it, a critical a roll of 5, 10
** ... 100): 0 a critical failure, 1 a failure, 2 a success, 3 a critical success */
static unsigned char __fastcall__ test (int eml)
{
    unsigned char r = forced ();

    if (r != 0xFF) {
        return r & 3;
    }
    r = d100 ();
    eml = eml < 5 ? 5 : eml > 95 ? 95 : eml;
    return r <= eml ? 2 + (r % 5 == 0) : 1 - (r % 5 == 0);
}

/* A name of the men's tongue (the name maker's) */
static char who[16];

static void a_name (void)
{
    static struct nq q;

    q.seed = random (30000) + 1;
    q.tongue = 2;
    q.form = NG_PERSON;
    ng_name (&q);
    strcpy (who, q.out);
}

/* ---- The days and the money */

static unsigned char __fastcall__ time_for (unsigned n)
{
    if (tr.days + n > TOWN_DAYS) {
        say ("You can't stay so long in ");
        say (town);
        say (": you must be on your way within ");
        sayn (TOWN_DAYS - tr.days);
        say (" days.\n");
        return 0;
    }
    return 1;
}

static void __fastcall__ pay (unsigned long f, unsigned n)
{
    cash -= f;
    tr.days += n;
}

/* ---- The sheet: its skills (pc), its tongues and scripts past its own (its "language" and "script" lines, the referee's
** store's; and what a master's opened in this stay, town.dat's, in the store's form) */

static char tl[TONGUE_LINES][24];
static unsigned char ntl;

static void __fastcall__ own_line (char* l)
{
    if (!strncmp (l, "language ", 9) || !strncmp (l, "script ", 7)) {
        if (ntl == TONGUE_LINES) {
            fail ("more tongues and scripts on the sheet than the game can hold");
        }
        strncpy (tl[ntl++], l, sizeof tl[0] - 1);
    }
}

/* Key k's place among keys (" a b c "): 0xFF none */
static unsigned char __fastcall__ key_at (const char* keys, const char* k)
{
    unsigned char n = strlen (k), i = 0;
    const char* p = keys + 1;

    while (*p) {
        if (!strncmp (p, k, n) && p[n] == ' ') {
            return i;
        }
        p = strchr (p, ' ') + 1;
        ++i;
    }
    return 0xFF;
}

/* A line of the sheet (hmchar's file, the library's harnfile.c's format): its attributes ("att str 12", in harn.h's order), the
** masteries this needs ("ml lan 62": Language, Script, Folklore, Mathematics, Astrology, Heraldry), its tongues and scripts; the
** rest passed over (the library's reader not linked: the main RAM's room) */
static void __fastcall__ sheet_line (char* l)
{
    static const char attrs[] = " sp sx bd ss ht fr owt wt cml str sta dex agl eye hrg sml voi int aur wil mor dty pty end ";
    static const char mls[] = " lan scr fol mat ast hrl ";
    static const unsigned char ml_of[6] = { HARN_LAN, HARN_SCR, HARN_FOL, HARN_MAT, HARN_AST, HARN_HRL };
    unsigned char a = !strncmp (l, "att ", 4), k;
    char* e;

    if (!a && strncmp (l, "ml ", 3)) {
        own_line (l);
        return;
    }
    l = strchr (l, ' ') + 1;
    if (!(e = strchr (l, ' '))) {
        return;
    }
    *e++ = 0;
    if ((k = key_at (a ? attrs : mls, l)) == 0xFF) {
        return;
    }
    if (a) {
        pc.att[k] = atoi (e);
    } else {
        pc.ml[ml_of[k]] = atoi (e);
    }
}

static void sheet_read (void)
{
    static char b[64];
    unsigned char k, i = 0;
    int fd, n, j;

    if ((fd = open (in_ (top, "pc.hm"), O_RDONLY)) < 0) {
        fail ("no sheet");
    }
    while ((n = read (fd, b, sizeof b)) > 0) {  /* (Its lines, each in lb) */
        for (j = 0; j < n; ++j) {
            if (b[j] == '\n' || b[j] == '\r') {
                if (i) {
                    lb[i] = 0;
                    sheet_line (lb);
                }
                i = 0;
            } else if (i < 80) {
                lb[i++] = b[j];
            }
        }
    }
    close (fd);
    for (k = 0; k < TOWN_LEARNT && tr.learnt[k][0] && ntl < TONGUE_LINES; ++k) {
        strcpy (tl[ntl], tr.learnt[k][0] == '*' ? "script " : "language ");
        strcat (tl[ntl++], tr.learnt[k] + (tr.learnt[k][0] == '*'));
    }
}

/* Its mastery of tongue t (sc 0) or script t (sc 1): 0 none; its own tongue (common) its Language's, its native script (a line
** with no mastery said) its Script's (dq0harn's known) */
static unsigned char __fastcall__ known (const char* t, unsigned char sc)
{
    unsigned char k, n = strlen (t), o = sc ? 7 : 9;
    const char* p;

    if (!sc && !strcmp (t, "common")) {
        return pc.ml[HARN_LAN];
    }
    for (k = 0; k < ntl; ++k) {
        p = tl[k] + o;
        if ((tl[k][0] == 's') == sc && !strncmp (p, t, n) && (!p[n] || p[n] == '=')) {
            return p[n] == '=' ? atoi (p + n + 1) : sc ? pc.ml[HARN_SCR] : pc.ml[HARN_LAN];
        }
    }
    return 0;
}

static unsigned char literate (void)
{
    unsigned char k;

    for (k = 0; k < ntl && tl[k][0] != 's'; ++k) {
    }
    return k < ntl;
}

/* ---- The writings: the top's clues, those carried (the -w list) or read, the latest first */

static unsigned char ncl, crec[NCL], chand[NCL];        /* (Each told: its record, and in hand: 1 the writing, 2 a copy) */

static unsigned char __fastcall__ clue_io (unsigned char i, unsigned char wr)
{
    int fd;
    unsigned char ok = 0;

    if ((fd = open (in_ (top, CLUES_FILE), wr ? O_RDWR : O_RDONLY)) >= 0) {
        lseek (fd, (long) crec[i] * CLUE_SIZE, SEEK_SET);
        ok = wr ? write (fd, &K, CLUE_SIZE) == CLUE_SIZE : read (fd, &K, CLUE_SIZE) == CLUE_SIZE;
        close (fd);
    }
    if (!ok) {
        fail ("can't read the clues");
    }
    return ok;
}

/* What's carried of record r (town.dat's clue list): 1 the writing itself, 2 a copy, both 3 */
static unsigned char __fastcall__ hand_of (unsigned r)
{
    unsigned char k, h = 0;
    unsigned id;

    for (k = 0; k < 8; ++k) {
        id = tr.clue[k];
        if (id && !(id & CLUE_HERE) && (id & ~CLUE_COPY) == r + 1) {
            h |= id & CLUE_COPY ? 2 : 1;
        }
    }
    return h;
}

static void clues_read (void)
{
    int fd;
    unsigned r;
    unsigned char h, k;

    ncl = 0;
    if ((fd = open (in_ (top, CLUES_FILE), O_RDONLY)) < 0) {
        return;
    }
    for (r = 0; read (fd, &K, CLUE_SIZE) == CLUE_SIZE; ++r) {
        h = hand_of (r);
        if (!K.state || (!h && (K.state & ~CL_MORE) == CL_UNREAD)) {
            continue;
        }
        if (ncl == NCL) {                       /* (The latest kept: the oldest out) */
            memmove (crec, crec + 1, NCL - 1);
            memmove (chand, chand + 1, NCL - 1);
            --ncl;
        }
        crec[ncl] = r;
        chand[ncl++] = h;
    }
    close (fd);
    for (k = 0; k < ncl / 2; ++k) {             /* (The latest first) */
        h = crec[k], crec[k] = crec[ncl - 1 - k], crec[ncl - 1 - k] = h;
        h = chand[k], chand[k] = chand[ncl - 1 - k], chand[ncl - 1 - k] = h;
    }
}

static const char* tongue_of (void)
{
    return K.tongue[0] ? K.tongue : "common";
}

static const char* script_of (void)
{
    return K.script[0] ? K.script : "ardic";
}

/* Its condition for a reading, the steps: the writing's own if it's carried, else its copy's (the better of the two if both) */
static signed char __fastcall__ cond_at (unsigned char i)
{
    signed char c = chand[i] & 1 ? K.cond : -128, p = K.copy == CLUE_NOCOPY ? 0 : K.copy;

    return chand[i] & 2 && p > c ? p : c;
}

/* The writings told: those to show, those known of */
static void clues_say (void)
{
    unsigned char k;

    if (!ncl) {
        say ("You've no writings to show, and none you know of.\n");
        return;
    }
    say ("Your writings:");
    for (k = 0; k < ncl; ++k) {
        clue_io (k, 0);
        say (" ");
        sayn (k + 1);
        say (chand[k] == 2 ? " a copy of the " : " the ");
        say (K.thing);
        if (!chand[k]) {
            say (", left where you read it");
        }
        say (k + 1 < ncl ? ";" : ".\n");
    }
    for (k = 0; k < ncl; ++k) {                 /* (A copy sold on: talk in the street, once) */
        clue_io (k, 0);
        if ((K.asked & (CA_SOLD | CA_TOLD)) == CA_SOLD) {
            say ("There's talk in the street of an old ");
            say (K.thing);
            say (" like yours: someone has been selling copies of it.\n");
            K.asked |= CA_TOLD;
            clue_io (k, 1);
        }
    }
}

/* ==== The overlay (dq0schol.cfg's OV1, dq0schol.ovl: read into a bank as it starts and left at the window, $8000, all the while:
** the main RAM's room, nothing else using the window) */
#pragma code-name (push, "OV1CODE")
#pragma rodata-name (push, "OV1RODATA")
#pragma bss-name (push, "OV1BSS")

/* ---- Readings and their words (read.c's, dq0life's) */

static const char* const works[6] = { "raised another keep", "built another temple", "dug other mines", "laid others of their dead",
    "built another town", "carved another hold" };
static const char* const lies[6] = { "in the mountains", "in the hills", "in the woods", "in the marshes", "on the coast",
    "on the plains" };
static const char* const landk[6] = { "mountains | granite | springs | cold", "hills | limestone | springs | mild",
    "forest | sandstone | river | mild", "marsh | sandstone | lake | wet", "coast | chalk | sea | wet", "plain | chalk | river | dry" };
static char mixed[28];

/* A failed scribe's place, told as confidently (the book's analysis, Skills 20: a failure's estimate further out): the land the
** writing names, under the realm a wrong reading's ("the Velmorian Fells" for "the Ostmarkan Fells") */
static void mixed_make (void)
{
    const char* l = strrchr (K.place, ' ');
    const char* w = strrchr (K.wrong, ' ');

    strcpy (mixed, K.place);
    if (l && w && w - K.wrong > 4 && (w - K.wrong) + strlen (l) < sizeof mixed) {
        memcpy (mixed, K.wrong, w - K.wrong);
        strcpy (mixed + (w - K.wrong), l);
    }
}

/* Its place and its reason (wr: a wrong reading's; mx: a failed scribe's), "the Sindsaman Fens, where the Houshani laid ..." */
static void __fastcall__ place_said (unsigned char wr, unsigned char mx)
{
    say (wr ? K.wrong : mx ? mixed : K.place);
    say (", where ");
    say (K.people[0] ? K.people : "its makers");
    say (" ");
    say (works[((K.work & 15) + wr) % 6]);
}

/* Its people's, "the hill-kings'", "the Houshani's" */
static void people_s (void)
{
    say (K.people);
    say (K.people[strlen (K.people) - 1] == 's' ? "'" : "'s");
}

/* A critical success's line more, here: whose hand it is and how old, and the place's road from here (true and small) */
static void more_said (void)
{
    say (" the hand is ");
    if (K.people[0]) {
        people_s ();
    } else {
        say ("an old one");
    }
    if (K.ago) {
        say (", of some ");
        sayn (K.ago);
        say (K.ago == 1 ? " century ago" : " centuries ago");
    }
    say (", and the place it names lies some ");
    sayn (road_of (K.place));
    say (" days' road from here.");
}

/* A reading told: g its grade, by (a scribe's name, its roll hidden: a failure a whole reading too, further out; else its own,
** step 9's words) */
static void __fastcall__ reading_said (unsigned char g, const char* by)
{
    unsigned n;

    if (g == 1 && !by) {                        /* (A piece of it: its first half, to a word's end) */
        strcpy (lb, K.words);
        for (n = strlen (lb) / 2; n && lb[n] != ' '; --n) {
        }
        strcpy (lb + n, " ...");
        say (lb);
        say (" The rest you can't make out, but it speaks of some place ");
        say (lies[(K.land & 15) % 6]);
        say (".\n");
        return;
    }
    say (K.words);
    say (" It tells of ");
    place_said (g == 0, g == 1);
    say (".");
    if (g == 3) {
        if (by) {
            say (" ");
            say (by);
            say (" says too that");
        } else {
            say (" And more:");
        }
        more_said ();
    }
    say ("\n");
}

/* A reading's rank: none or unread, a wrong one, a piece, a whole one, a whole one and its line more */
static unsigned char __fastcall__ rank (unsigned char st)
{
    static const unsigned char r[5] = { 0, 0, 3, 2, 1 };

    return (st & CL_MORE) ? 4 : r[st & 7];
}

/* A reading here (grade g; by a scribe, sc): kept if it's better than the clue's (the place, the reason, the line more), and its
** rumour heard.txt's (a piece's only if heard.txt has none, or a piece) */
static void __fastcall__ better (unsigned char i, unsigned char g, unsigned char sc)
{
    static const unsigned char st[4] = { CL_WRONG, CL_PIECE, CL_RIGHT, CL_RIGHT | CL_MORE };
    static char h[240];
    int fd;
    unsigned char piece = g == 1 && !sc, n;

    if (rank (st[g]) <= rank (K.state)) {
        return;                                 /* (A worse one doesn't undo a better) */
    }
    K.state = st[g];
    clue_io (i, 1);
    if (piece && (fd = open (in_ (top, "heard.txt"), O_RDONLY)) >= 0) {
        n = read (fd, h, 40);
        close (fd);
        h[n > 0 ? n : 0] = 0;
        if (!strstr (h, "What you made out")) {
            return;                             /* (A whole reading's rumour there: kept over a piece) */
        }
    }
    strcpy (h, "rumour   | ");
    if (piece) {
        strcat (h, "What you made out of the ");
        strcat (h, K.thing);
        strcat (h, " speaks of some place ");
        strcat (h, lies[(K.land & 15) % 6]);
    } else {
        strcat (h, "The ");
        strcat (h, K.thing);
        strcat (h, sc ? " you had read tells of " : " you read tells of ");
        strcat (h, g == 0 ? K.wrong : g == 1 ? mixed : K.place);
        strcat (h, ", where ");
        strcat (h, K.people[0] ? K.people : "its makers");
        strcat (h, " ");
        strcat (h, works[((K.work & 15) + (g == 0)) % 6]);
    }
    strcat (h, ".\nregion   | ");
    strcat (h, g == 0 ? K.wrong : g == 1 && sc ? mixed : K.place);
    strcat (h, " | ");
    strcat (h, landk[(g == 0 ? K.work >> 4 : K.land & 15) % 6]);
    strcat (h, "\n");
    if ((fd = open (in_ (top, "heard.txt"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, h, strlen (h));
        close (fd);
    }
}

/* ---- What's done here */

static unsigned char __fastcall__ shown (unsigned char i, const char* what)
{
    if (chand[i]) {
        return 1;
    }
    say ("You've nothing of the ");
    say (K.thing);
    say (" to ");
    say (what);
    say (": it was left where you read it.\n");
    return 0;
}

/* A scribe hired (a sage for a dead tongue): its honesty, its price, its days, its reading */
static void __fastcall__ hire (unsigned char i)
{
    unsigned char dead, days, m, g;
    unsigned long rate, cost;

    clue_io (i, 0);
    if (!shown (i, "show a scribe")) {
        return;
    }
    dead = K.ago >= 10;
    if (dead && !big) {
        say ("No one in ");
        say (town);
        say (" reads the old ");
        say (tongue_of ());
        say (" of the ");
        say (K.thing);
        say (": the sages of a bigger town might.\n");
        return;
    }
    if ((m = forced ()) == 0xFF) {
        m = dice (3, 6);                        /* (Its Morality: the book's, 3d6) */
    }
    a_name ();
    days = dead ? dice (2, 4) : die (3);
    rate = (dead ? DEAD_TIMES * SCRIBE_DAY : SCRIBE_DAY) * PENNY;
    if (m <= 7) {
        rate = (rate * 3 / 2 + PENNY - 1) / PENNY * PENNY;     /* (Unscrupulous: half again, to the penny) */
    }
    cost = rate * days;
    if (!time_for (days)) {
        return;
    }
    say (who);
    say (dead ? ", a sage, " : ", a scribe, ");
    if (cost > cash) {
        say ("asks ");
        amount (cost);
        say (" for ");
        sayn (days);
        say (days == 1 ? " day's work: you haven't the money.\n" : " days' work: you haven't the money.\n");
        return;
    }
    pay (cost, days);
    say ("keeps the ");
    say (K.thing);
    say (" ");
    sayn (days);
    say (days == 1 ? " day and asks " : " days and asks ");
    amount (cost);
    say ("; then reads it to you: ");
    g = test ((dead ? 50 : 45) + dice (4, 10) + 5 * cond_at (i));
    if (m <= 4) {
        g = 0;                                  /* (Diabolical: a wrong reading, told as confidently) */
    }
    if (m <= 10) {
        K.asked |= CA_SOLD;                     /* (Corruptible or worse: a copy sold on) */
    }
    mixed_make ();
    reading_said (g, who);
    better (i, g, 1);
    clue_io (i, 1);
}

/* The library: a week at a time, a penny a day, the reading test each week till it's read (weeks at most) */
static void __fastcall__ study (unsigned char i, unsigned char weeks)
{
    static const char* const nth[8] = { "first", "second", "third", "fourth", "fifth", "sixth", "seventh", "eighth" };
    unsigned char t, s, w, g = 1;
    int eml;

    clue_io (i, 0);
    if (!literate ()) {
        say ("You can't read: the library's no use to you. A scribe could read the ");
        say (K.thing);
        say (" for you.\n");
        return;
    }
    if (!shown (i, "study")) {
        return;
    }
    t = known (tongue_of (), 0);
    s = known (script_of (), 1);
    if (!s || !t) {
        say (!s ? "The " : "You know the letters of the ");
        say (K.thing);
        say (!s ? " is written in letters you've never learnt (" : ", but not its tongue (");
        say (!s ? script_of () : tongue_of ());
        say (!s ? "): a master could teach you them, or a scribe read it for you.\n" : "): a master could teach you it, or a scribe read it for you.\n");
        return;
    }
    eml = ((t + s) >> 1) + 5 * cond_at (i);
    for (w = 0; w < weeks && w < 8; ) {
        if (tr.days + 7 > TOWN_DAYS || cash < 7 * PENNY) {
            break;
        }
        pay (7 * PENNY, 7);
        ++w;
        if ((g = test (eml)) != 1) {
            break;
        }
    }
    if (!w) {
        if (!time_for (7)) {
            return;
        }
        say ("A week in the library is 7d: you haven't the money.\n");
        return;
    }
    strncpy (tr.study, script_of (), sizeof tr.study - 1);  /* (The month's practice on it first: the account's) */
    say ("You study the ");
    say (K.thing);
    say (" in the library, a penny a day");
    if (g != 1) {
        say (", and in the ");
        say (nth[w - 1]);
        say (" week you make it out: ");
    } else {
        say (w < weeks ? ", till you must stop" : "");
        say (": after ");
        sayn (w);
        say (w == 1 ? " week you've made out only a piece of it: " : " weeks you've made out only a piece of it: ");
    }
    reading_said (g, 0);
    say ("That's ");
    sayn (w * 7);
    say (" days, ");
    sayn (w * 7);
    say ("d.\n");
    better (i, g, 0);
}

/* A master's month: a script or a tongue opened for good */
static void __fastcall__ learn (const char* w)
{
    unsigned char n = strcspn (w, " "), sc, k, ml;
    char* e;

    sc = among ("scripts", w, n) != 0xFF;
    if (!n || (!sc && among ("tongues", w, n) == 0xFF)) {
        say ("No one here teaches that. Masters in ");
        say (town);
        say (" teach ");
        for (k = 0; words_of (big ? "scripts" : "small", 0) && k < nv; ++k) {
            say (k ? ", " : "");
            say (v[k]);
        }
        if (big) {
            for (k = 0; words_of ("tongues", 0) && k < nv; ++k) {
                say (", ");
                say (v[k]);
            }
        }
        say (".\n");
        return;
    }
    if (!big && among ("small", w, n) == 0xFF) {
        say ("No master in ");
        say (town);
        say (" teaches that: a bigger town's might.\n");
        return;
    }
    strncpy (who, w, n);
    who[n] = 0;
    if (known (who, sc)) {
        say ("You know ");
        say (who);
        say (" already.\n");
        return;
    }
    if (ntl >= TONGUE_LINES) {                  /* (Refused, never cut short: the referee's store's, the account's) */
        say ("Your sheet holds all the tongues and scripts the game can keep, eight past your own: you can learn no more.\n");
        return;
    }
    for (k = 0; k < TOWN_LEARNT && tr.learnt[k][0]; ++k) {
    }
    if (k == TOWN_LEARNT) {
        say ("You've had all the teaching you've time for in this stay.\n");
        return;
    }
    if (!time_for (30)) {
        return;
    }
    if (MASTER_MONTH * PENNY > cash) {
        say ("A master asks ");
        sayn (MASTER_MONTH);
        say ("d for a month's teaching: you haven't the money.\n");
        return;
    }
    pay (MASTER_MONTH * PENNY, 30);
    ml = sc ? 70 + harn_get_sb (&pc, HARN_SCR) : harn_get_sb (&pc, HARN_LAN);   /* (The book's, Skills 11) */
    e = tr.learnt[k];
    strcpy (e, sc ? "*" : "");
    strcat (e, who);
    strcat (e, "=");
    utoa (ml, e + strlen (e), 10);
    ++tr.taught;
    strcpy (tl[ntl], sc ? "script " : "language ");
    strcat (tl[ntl++], e + sc);
    strcpy (lb, who);
    a_name ();
    say (sc ? "You study " : "You learn ");
    say (lb);
    say (sc ? " for a month with Master " : " for a month from ");
    say (who);
    say (sc ? " (" : ", who was raised speaking it (");
    sayn (MASTER_MONTH);
    say (sc ? "d): you can read and write it now (" : "d): you speak a little of it now (");
    say (lb);
    say (" ");
    sayn (ml);
    say (").\n");
}

/* The other skills, once each for a writing: Folklore, Mathematics, Astrology, Heraldry (sk: 0 ... 3) */
static void __fastcall__ asked (unsigned char i, unsigned char sk)
{
    static const unsigned char skill[4] = { HARN_FOL, HARN_MAT, HARN_AST, HARN_HRL };
    unsigned char own, g, k;
    const char* p;

    clue_io (i, 0);
    if (K.asked & (1 << sk)) {
        say ("You've asked that of the ");
        say (K.thing);
        say (" already.\n");
        return;
    }
    if (sk == 1) {                              /* (Only what reckons: a tally, a ledger ...) */
        words_of ("reckons", 0);
        for (k = 0; k < nv && !strstr (K.thing, v[k]); ++k) {
        }
        if (k == nv) {
            say ("There's nothing in the ");
            say (K.thing);
            say (" to reckon.\n");
            return;
        }
    }
    own = pc.ml[skill[sk]];
    if (!time_for (1)) {
        return;
    }
    if ((own ? PENNY : SAGE_DAY * PENNY) > cash) {
        say ("You haven't the money.\n");
        return;
    }
    pay (own ? PENNY : SAGE_DAY * PENNY, 1);
    if (own) {
        say ("You spend a day in the library over it (1d). ");
    } else {
        a_name ();
        say (who);
        say (", a sage, spends a day over it for you (");
        sayn (SAGE_DAY);
        say ("d). ");
    }
    g = test (own ? own : 45 + dice (4, 10));
    K.asked |= 1 << sk;
    clue_io (i, 1);
    if (g < 2) {
        if (own) {
            say ("You can make nothing of it.\n");
        } else {
            say (who);
            say (" can tell you nothing of it.\n");
        }
        return;
    }
    switch (sk) {
    case 0:                                     /* (Folklore: its people, what they made, where they went on to) */
        if (!K.people[0]) {
            say ("The old tales know nothing of whoever made it.\n");
            break;
        }
        say ("The old tales know of ");
        say (K.people);
        say (", who ");
        say (words_of ("made", 0) > (K.work & 15) % 6 ? v[(K.work & 15) % 6] : "");
        say (", and say they went on ");
        say (words_of ("went", 0) > (K.land & 15) % 6 ? v[(K.land & 15) % 6] : "");
        say (".\n");
        break;
    case 1:                                     /* (Mathematics: its reckonings, the place's road from here) */
        say ("Its reckonings come to a road: the place it speaks of lies some ");
        sayn (road_of (K.place));
        say (" days from here.\n");
        break;
    case 2:                                     /* (Astrology: its date) */
        say ("By the signs on it, it was made ");
        if (K.ago) {
            say ("some ");
            sayn (K.ago);
            say (K.ago == 1 ? " century ago.\n" : " centuries ago.\n");
        } else {
            say ("within the last hundred years.\n");
        }
        break;
    default:                                    /* (Heraldry: whose marks, and the realm of the place it names) */
        p = strlen (K.place) > 4 && !strncmp (K.place, "the ", 4) ? strchr (K.place + 4, ' ') : 0;
        if (!K.people[0] && !p) {
            say ("The heralds' rolls know nothing of its marks.\n");
            break;
        }
        say ("The marks on it are ");
        if (K.people[0]) {
            people_s ();
            say (" own");
        } else {
            say ("no house's the rolls know");
        }
        if (p) {
            strncpy (lb, K.place + 4, p - K.place - 4);
            lb[p - K.place - 4] = 0;
            say (", and it bears the device of the ");
            say (lb);
            say (" lords");
        }
        say (".\n");
        break;
    }
}

#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)

/* ---- A line typed */

/* The writing it names (a number, or a word of its thing's name); 0xFF none, told */
static unsigned char __fastcall__ clue_named (const char* o)
{
    unsigned char k, n;
    const char* w;

    if (!ncl) {
        say ("You've no writings to show.\n");
        return 0xFF;
    }
    if (*o >= '1' && *o <= '9' && (!o[1] || o[1] == ' ')) {
        if (*o - '1' < ncl) {
            return *o - '1';
        }
    } else {
        for (w = o; *w; w += n) {
            while (*w == ' ') {
                ++w;
            }
            n = strcspn (w, " ");
            for (k = 0; n >= 3 && k < ncl; ++k) {
                clue_io (k, 0);
                strncpy (lb, w, n);
                lb[n] = 0;
                if (strstr (K.thing, lb)) {
                    return k;
                }
            }
        }
        if (!*o && ncl == 1) {
            return 0;
        }
    }
    say (*o ? "You have no writing like that.\n" : "Which writing? (a number)\n");
    return 0xFF;
}

static void __fastcall__ command (const char* l)
{
    static const char* const verbs[] = { "read", "have", "hire", "scribe", "sage", "translate", "decipher", "study", "library",
        "research", "learn", "teach", "master", "folklore", "tales", "legend", "reckon", "mathematics", "maths", "count", "astrology",
        "stars", "date", "heraldry", "herald", "seal", "device", "list", "look" };
    static const unsigned char what[] = { 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 6, 7, 7 };
    unsigned char k, n = strcspn (l, " "), i, weeks = 4;
    const char* o = l + n;
    const char* f;

    while (*o == ' ') {
        ++o;
    }
    for (k = 0; k < sizeof what && (strlen (verbs[k]) != n || strncmp (l, verbs[k], n)); ++k) {
    }
    if (k == sizeof what) {
        say ("There's nothing like that to do here: read, study, learn, or Enter to go back.\n");
        return;
    }
    k = what[k];
    while (!strncmp (o, "the ", 4) || !strncmp (o, "my ", 3) || !strncmp (o, "me ", 3) || !strncmp (o, "of ", 3) ||
        !strncmp (o, "a ", 2) || !strncmp (o, "to ", 3)) {
        o = strchr (o, ' ') + 1;                /* (Its little words off its front: "learn the delvic script") */
    }
    if (k == 2) {
        learn (o);
        return;
    }
    if (k == 7) {
        clues_say ();
        return;
    }
    if (k == 1 && (f = strstr (o, "for ")) != 0 && f[4] >= '1' && f[4] <= '9') {
        weeks = atoi (f + 4);                   /* ("study the journal for 3 weeks") */
        weeks = weeks > 8 ? 8 : weeks;
    }
    if ((i = clue_named (o)) == 0xFF) {
        return;
    }
    if (k == 0) {
        hire (i);
    } else if (k == 1) {
        study (i, weeks);
    } else {
        asked (i, k - 3);
    }
}

int main (int argc, char* argv[])
{
    static struct hy_regs regs;
    const char* first = "";
    const char* ov[3] = { 0, 0, 0 };
    const char* p;
    unsigned char i;
    int fd, n, j;

    for (i = 1; i + 1 < argc; i += 2) {
        switch (argv[i][1]) {
        case 'd': top = argv[i + 1]; break;
        case 'l': lib = argv[i + 1]; break;
        case 't': town = argv[i + 1]; break;
        case 'x': first = argv[i + 1]; break;
        case 'b': ov[0] = argv[i + 1]; break;
        case 'c': ov[1] = argv[i + 1]; break;
        case 'w': ov[2] = argv[i + 1]; break;
        }
    }
    if (!top || !lib || !town) {
        fail ("usage: dq0schol -d folder -l libdir -t town [-x first] [-b big -c money -w ids]");
    }
    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) < 0 || read (fd, &L, sizeof L) != sizeof L) {
        fail ("no record of the life");
    }
    close (fd);
    if ((fd = open (in_ (top, TOWN_FILE), O_RDONLY)) >= 0) {
        read (fd, &tr, sizeof tr);              /* (The town's so far: none, a stay begun, all 0) */
        close (fd);
    }
    if (ov[0]) {                                /* (A scene's, run alone: the town's kind, the money, the writings) */
        big = *ov[0] == '1';
    }
    if (ov[1]) {
        cash = atol (ov[1]);
    }
    for (i = 0, p = ov[2]; p && *p && i < 8; ++i) {
        tr.clue[i] = atoi (p);
        p += strcspn (p, " ");
        p += strspn (p, " ");
    }
    strcpy (path, lib);
    strcat (path, "/../acct/dq0schol.ovl");
    if ((fd = open (path, O_RDONLY)) < 0 || (n = hy_banks_alloc (1)) < 0) {
        fail ("no overlay");
    }
    hy_bank (n);                                /* (At the window from now on) */
    read (fd, (void*) 0x8000, 0x2000);
    close (fd);
    strcpy (path, lib);
    strcat (path, "/../acct/scholar.txt");
    if ((fd = open (path, O_RDONLY)) < 0) {
        fail ("no scholar.txt");
    }
    for (i = 1; (n = read (fd, lb, sizeof lb)) > 0; ) {    /* (Its lines but its notes: i at a lb's start, 2 in a note) */
        for (j = 0; j < n && twn < sizeof tw - 1; ++j) {
            if (i == 1 && lb[j] == '#') {
                i = 2;
            }
            if (i != 2 && lb[j] != '\r') {
                tw[twn++] = lb[j];
            }
            i = lb[j] == '\n' ? 1 : i == 2 ? 2 : 0;
        }
    }
    close (fd);
    regs.a = 0xFF;                              /* (The rc variable dq0sg, the tests' scenes: ENV_GET itself, getenv's room) */
    regs.r[0] = (unsigned) "dq0sg";
    regs.r[1] = (unsigned) fq;
    regs.r[2] = sizeof fq - 1;
    regs.r[3] = 0;
    n = hy_call (HY_ENV_GET, &regs) ? 0 : regs.a | regs.x << 8;
    n = n > sizeof fq - 1 ? sizeof fq - 1 : n;
    for (fq[n] = 0; n--; ) {
        fq[n] = fq[n] ? fq[n] : ' ';            /* (An rc list's words, each 0-ended: a space between) */
    }
    width_read ();
    rseed ((L.h.seed ^ 0x5343484FUL) + L.h.dungeon * 40503UL + tr.days * 2654435761UL);  /* ("SCHO": the same life and the same
                                                **   days in town, the same scholars) */
    sheet_read ();
    clues_read ();
    say_one ("enter");
    say ("\n");
    if (words_of ("help", 0)) {
        say (v[0]);
        say ("\n");
    }
    clues_say ();
    if (*first) {
        command (first);                        /* (What was typed in the street: done here) */
    }
    for (;;) {
        ask (words_of ("prompt", 0) ? v[0] : "");
        if (!*ans || asks > ASKS) {
            break;
        }
        command (ans);
    }
    word_out ();
    out_flush ();
    if ((fd = open (in_ (top, TOWN_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, &tr, sizeof tr) != sizeof tr) {
        fail ("can't write town.dat");
    }
    close (fd);
    return 0;
}
