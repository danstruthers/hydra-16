/*
** goalgen.c - (a bank program, themegen's: sdk/c/include/bankprog.h) the goal made (G5: the goal kinds are data,
** games/themegen/goals.txt), into themegen's main RAM, from the history histgen made: its stratum by where its kind
** says it is (built: a stratum an era that's ended built or made its own; cursed: an era's that ended cursed; lair:
** the kobolds' lair, where they keep a prisoner; deep: the deepest, a far exit), its era, its guardians (the
** strongest faction that lives there: the dead, the kobolds, the beasts, the spiders, the rats; a prisoner's its
** captors), a stratum above it (where the evil's undoing lies), its things (one of each role its kind has) and their
** names, its person (a prisoner, a seeker, an old priest), and the theme's title.
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#define BANKPROG
#include "../../games/themegen/themegen.h"

/* The faction (fa: 0 ...) that guards stratum k: the first that lives there of the dead, the kobolds, the beasts,
** the crawlers, the vermin; any other; or 0xFF */
static unsigned char __fastcall__ guard_of (unsigned char k)
{
    static const char* const order[] = { "dead", "kobolds", "beasts", "crawlers", "vermin", 0 };
    unsigned char i, j;

    for (i = 0; order[i]; ++i) {
        for (j = 0; j < nf; ++j) {
            if (!strcmp (fa[j].name, order[i]) && k >= fa[j].d0 && k <= fa[j].d1) {
                return j;
            }
        }
    }
    for (j = 0; j < nf; ++j) {
        if (k >= fa[j].d0 && k <= fa[j].d1) {
            return j;
        }
    }
    return 0xFF;
}

static unsigned char __fastcall__ kobolds (void)
{
    unsigned char j;

    for (j = 0; j < nf && strcmp (fa[j].name, "kobolds"); ++j) {
    }
    return j < nf ? j : 0xFF;
}

/* The kth line of key (a thing's: of role, a or b) in the goal's block (after its goal line, before the next), or
** -1; and how many there are */
static int __fastcall__ nth (unsigned char key, char role, unsigned char k)
{
    int j, end = tg_find (TK_GOAL, 0, gk + 1);

    for (j = goal_line + 1; j < (int) tg_lines && (end < 0 || j < end); ++j) {
        if (tg_key[j] != key) {
            continue;
        }
        if (role) {
            tg_get (j, &D, Dt);
            if (*D.v[0] != role) {
                continue;
            }
        }
        if (!k--) {
            return j;
        }
    }
    return -1;
}

static unsigned char __fastcall__ count (unsigned char key, char role)
{
    unsigned char m = 0;

    while (nth (key, role, m) >= 0) {
        ++m;
    }
    return m;
}

void make (void)
{
    static char where[8];
    unsigned char k, n, cand[TSTRATA], nc, m;
    int j;
    struct era* g;

    if (gk == 0xFF) {                           /* (A hand-written theme's own puzzle: its name the deeps') */
        strcpy (gmission, "escape");
        strcpy (title, "the Deeps of ");
        strcat (title, ne ? er[0].place : "Old");
        return;
    }
    tg_get (goal_line = tg_find (TK_GOAL, 0, gk), &D, Dt);
    strcpy (where, D.v[2]);
    strcpy (gmission, D.v[1]);
    for (nc = k = 0; k < ns; ++k) {             /* (Where it is: its stratum) */
        n = era_now (k);
        if (!strcmp (where, "built") ? n && er[n - 1].end != EN_HELD && k + 1 < ns :
            !strcmp (where, "cursed") ? n && er[n - 1].end == EN_CURSED :
            !strcmp (where, "lair") ? kobolds () != 0xFF && fa[kobolds ()].lair == k : k + 1 == ns) {
            cand[nc++] = k;
        }
    }
    if (!nc) {                                  /* (None: the deepest built) */
        for (k = ns; k && !era_now (k - 1); --k) {
        }
        cand[nc++] = k ? k - 1 : 0;
    }
    gs = cand[random (nc)];
    if (!(ge = era_now (gs))) {
        ge = 1;                                 /* (The deeps: the oldest people made its way) */
    }
    gg = !strcmp (where, "lair") ? kobolds () : guard_of (gs);
    gu = gs ? random (gs) : 0;
    geu = era_now (gu) ? era_now (gu) : ge;
    g = er + (ge - 1);                          /* (Its era: through a pointer, never an import's address and an
                                                **   offset, which a bank program can't have) */
    strcpy (gM, name_of (gk == G_EVIL - 1 && ge < ne ? g[1].tongue : 2, NG_PERSON, rand32 ()));
    SL ('N') = g->great;
    SL ('M') = gM;
    SL ('X') = g->place;
    SL ('P') = g->pthe;
    SL ('R') = rname;
    ga_line = gb_line = -1;
    for (k = 0; k < 2; ++k) {                   /* (Its things: one of each role, a and b) */
        if (!(m = count (TK_THING, 'a' + k))) {
            continue;
        }
        tg_get (j = nth (TK_THING, 'a' + k, random (m)), &D, Dt);
        if (k) {
            gb_line = j;
            strcpy (gb, D.v[2]);
            fill (gB, D.v[1], sizeof gB);
        } else {
            ga_line = j;
            strcpy (ga, D.v[2]);
            fill (gA, D.v[1], sizeof gA);
        }
    }
    if ((m = count (TK_EXIT, 0)) != 0) {        /* (A far exit's name) */
        tg_get (nth (TK_EXIT, 0, random (m)), &D, Dt);
        fill (gA, D.v[0], sizeof gA);
    }
    SL ('A') = gA;
    if ((m = count (TK_TITLE, 0)) != 0) {       /* (The title: one picked, or the first after it that fits) */
        for (n = random (m), k = 0; k < m; ++k) {
            tg_get (nth (TK_TITLE, 0, (n + k) % m), &D, Dt);
            fill (W, D.v[0], LINE_TEXT);
            if (strlen (W) < sizeof title) {
                break;
            }
        }
        strncpy (title, W, sizeof title - 1);
    }
}
