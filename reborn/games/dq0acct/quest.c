/*
** quest.c - (dq0acct, overlay 4) why the character came, and where it goes next (the plan's step 7; HOUSE.md's between dungeons):
** what solving the dungeon it's come out of brings (its theme's reward line: themegen's, from goals.txt's why lines, or a stand-in's
** own), the standing it has told (the life's reputation, life.h's), and the rumours heard in town (themegen -r's, which dq0life runs:
** rumours.txt in the save's folder; a clue's, the plan's step 10, from heard.txt, told first and taking a made one's place), the one
** followed written as quest.txt (dq0life puts it in the next dungeon's folder, and themegen fills it out: dq0 -t's door), its road's
** days on the date (town.h's road_of).  Asked of the character that goes straight on (the tests' dq0auto); the player's town
** (dq0town, the plan's step 11) tells them at its inn and follows one itself.
*/

#pragma code-name ("OV4CODE")
#pragma rodata-name ("OV4RODATA")
#pragma bss-name ("OV4BSS")                    /* (Not in the file: each set before it's read) */
#pragma local-strings (on)
#pragma static-locals (on)

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/life.h"
#include "../dq0/theme.h"
#define TOWN_ROAD
#include "dq0acct.h"

extern struct life L;

/* ---- A file's lines, the theme format's (a physical line at a time: 159 letters at most, its CR gone) */

static int fd;
static char rb[128], ln[160];
static unsigned char rpos, rlen;

static int __fastcall__ open_ (const char* p)
{
    rpos = rlen = 0;
    return fd = open (p, O_RDONLY);
}

static int getln (void)
{
    unsigned char n = 0;
    int c;

    for (;;) {
        if (rpos == rlen) {
            if ((c = read (fd, rb, sizeof rb)) <= 0) {
                if (!n) {
                    return -1;
                }
                break;
            }
            rlen = c;
            rpos = 0;
        }
        c = rb[rpos++];
        if (c == '\n') {
            break;
        }
        if (c != '\r' && n < sizeof ln - 1) {
            ln[n++] = c;
        }
    }
    ln[n] = 0;
    return n;
}

/* The file's line of key, whole (the lines that go on with it joined, a space between), into out; its values (between bars, their
** spaces trimmed) in v: how many, 0 none */
static char* v[8];
static unsigned char __fastcall__ line_of (const char* key, char* out, unsigned max)
{
    unsigned n = 0, k = strlen (key);
    unsigned char got = 0, i;
    char* s;

    while (getln () >= 0) {
        if (got) {
            if (*ln != ' ' && *ln != '\t') {
                break;                          /* (The next line's: it's whole) */
            }
            for (s = ln; *s == ' ' || *s == '\t'; ++s) {
            }
            if (*s && n + strlen (s) + 2 < max) {
                out[n++] = ' ';
                strcpy (out + n, s);
                n += strlen (s);
            }
        } else if (!strncmp (ln, key, k) && (ln[k] == ' ' || ln[k] == '|')) {
            got = 1;
            strncpy (out, ln + k, max - 1);
            out[max - 1] = 0;
            n = strlen (out);
        }
    }
    if (!got) {
        return 0;
    }
    for (i = 0, s = out; s && i < sizeof v / sizeof v[0]; ++i) {
        s = strchr (s, '|');
        if (!s) {
            break;
        }
        *s++ = 0;
        while (*s == ' ') {
            ++s;
        }
        v[i] = s;
    }
    for (k = 0; k < i; ++k) {                   /* (Each value's spaces after it trimmed) */
        for (s = v[k] + strlen (v[k]); s > v[k] && s[-1] == ' '; --s) {
        }
        *s = 0;
    }
    return i;
}

/* ---- What solving brings, and the standing */

unsigned rw_pence;
signed char rw_stand;
char rw_thing[16];
static char rw_words[480];
static unsigned char rw_has, rw_out;

static void __fastcall__ dungeon_path (unsigned n, const char* name)
{
    strcpy (path, top);
    strcat (path, "/d");
    utoa (n, path + strlen (path), 10);
    strcat (path, "/");
    strcat (path, name);
}

/* The dungeon just left: its outcome (the record's), its theme's reward line (its own theme's, theme.txt in its folder; a
** stand-in's, its file in lib/dq0/themes, named in dq0t.dat's record): the pennies, the standing, a thing, the words, if it was
** solved; left, or one with none, nothing */
void o_reward (void)
{
    static struct save sv;
    static char t[16];                          /* (struct theme's file) */
    unsigned n = L.h.dungeon - 1;
    unsigned char ok;

    rw_pence = rw_has = 0;
    rw_stand = 0;
    *rw_thing = 0;
    if (n < 1 || n > LIFE_DUNGEONS) {
        return;
    }
    rw_out = L.d[n - 1].outcome;
    dungeon_path (n, "game.dat");
    if (open_ (path) < 0) {
        return;
    }
    ok = read (fd, &sv, sizeof sv) == sizeof sv && sv.magic == SAVE_MAGIC;
    close (fd);
    if (!ok) {
        return;
    }
    if (sv.q.theme == 0xFF) {
        dungeon_path (n, "theme.txt");
    } else {
        strcpy (path, lib);
        strcat (path, "/dq0t.dat");
        if (open_ (path) < 0) {
            return;
        }
        lseek (fd, (long) sv.q.theme * THEME_SIZE + offsetof (struct theme, file), SEEK_SET);
        ok = read (fd, t, sizeof t) == sizeof t;
        close (fd);
        if (!ok || !*t) {
            return;
        }
        t[sizeof t - 1] = 0;
        strcpy (path, lib);
        strcat (path, "/themes/");
        strcat (path, t);
    }
    if (open_ (path) < 0) {
        return;
    }
    ok = line_of ("reward", rw_words, sizeof rw_words) >= 4;
    close (fd);
    if (!ok) {
        return;
    }
    rw_has = 1;
    if (rw_out == LO_SOLVED) {
        rw_pence = atoi (v[0]);
        rw_stand = atoi (v[1]);
        if (strcmp (v[2], "-")) {
            strncpy (rw_thing, v[2], sizeof rw_thing - 1);
        }
        cash += (unsigned long) rw_pence * PENNY;   /* (Paid as it comes into town: what's paid for the time between with the rest) */
        L.h.reputation += rw_stand;
    }
}

/* Told as the character comes into town: what it's paid (the reward's words), or, left unsolved, that nothing is */
void o_reward_say (void)
{
    if (!rw_has) {
        return;
    }
    if (rw_out == LO_SOLVED) {
        say (v[3]);
        say (" ");
    } else {
        say ("No one pays for what was left undone. ");
    }
}

/* The standing (the life's reputation) in words, after the account's last line */
void o_standing (void)
{
    static const char* const fame[5] = { " No one has heard of you yet", " A few folk in the inns know your name",
        " Your name is known in the inns hereabouts", " Your name is known across the land", " Your name is spoken of far and wide" };
    int r = L.h.reputation;

    say (".");
    say (fame[r <= 0 ? 0 : r <= 2 ? 1 : r <= 5 ? 2 : r <= 9 ? 3 : 4]);
    say (" (standing ");
    sayn (r < 0 ? 0 : r);
    say (").\n");
}

/* ---- The rumours */

#define RUMOURS         3
static char rbuf[2400];                         /* (The rumours' files whole: each a part-written theme begun by its rumour line) */
static unsigned rused, rat[RUMOURS], rto[RUMOURS];
static unsigned char nr;
static char said[480];

/* A file in the save's folder read in after what's there, its rumours (blocks begun by a rumour line) noted while there's room */
static void __fastcall__ heard (const char* name)
{
    unsigned at = rused, p;
    int k;

    strcpy (path, top);
    strcat (path, "/");
    strcat (path, name);
    if ((fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    while (rused < sizeof rbuf - 1 && (k = read (fd, rbuf + rused, sizeof rbuf - 1 - rused)) > 0) {
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
        }
    }
}

/* Rumour k's line of key (the first value of it, its lines joined) into said: 1, or 0 none */
static unsigned char __fastcall__ value_of (unsigned char k, const char* key)
{
    const char* p = rbuf + rat[k];
    const char* e = rbuf + rto[k];
    unsigned n = strlen (key), m = 0;
    unsigned char got = 0;

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
        } else if (!strncmp (p, key, n) && (p[n] == ' ' || p[n] == '|')) {
            if (!(p = strchr (p, '|'))) {
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
        for (; p < e && *p != '\n'; ++p) {
        }
        ++p;
    }
    while (m && said[m - 1] == ' ') {
        --m;
    }
    said[m] = 0;
    return got;
}

/* The rumours told (a clue's first, then themegen's: three at most), one followed (asked: Enter, the tests' dq0auto or anything not
** one of them takes the first), written as quest.txt for the next dungeon; none heard (a stand-in's life), Enter to go on */
void o_rumours (void)
{
    unsigned char k, c;

    rused = nr = 0;
    heard ("heard.txt");
    heard ("rumours.txt");
    if (!nr) {
        say ("Enter to go on");
        ask (ans, sizeof ans);
        return;
    }
    say ("\nIn ");
    say (town);
    say (" there's talk of other places:\n");
    for (k = 0; k < nr; ++k) {
        say ("  ");
        sayn (k + 1);
        say (". ");
        value_of (k, "rumour");
        say (said);
        say ("\n");
    }
    say (nr == 1 ? "Will you follow it? (Enter)" : nr == 2 ? "Which will you follow, 1 or 2?" : "Which will you follow, 1, 2 or 3?");
    ask (ans, sizeof ans);
    for (c = 0; ans[c] == ' '; ++c) {
    }
    c = ans[c] >= '1' && ans[c] < '1' + nr ? ans[c] - '1' : 0;
    strcpy (path, top);
    strcat (path, "/quest.txt");
    if ((fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, rbuf + rat[c], rto[c] - rat[c]);
        close (fd);
    }
    say ("You set out for ");
    say (value_of (c, "region") ? said : "it");
    say (", ");
    sayn (c = road_of (said));                  /* (Its road: the town's, town.h's, the date on by it) */
    say (" days on the road.\n");
    L.h.date += c;
}
