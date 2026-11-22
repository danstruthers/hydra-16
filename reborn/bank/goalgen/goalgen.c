/*
** goalgen.c - (a bank program, themegen's: sdk/c/include/bankprog.h) the inhabitants now (the bestiary's factions that
** fit the strata and the history, the world's: below), then the goal made (G5: the goal kinds are data,
** games/themegen/goals.txt), into themegen's main RAM, from the history histgen made: its stratum by where its kind
** says it is (built: a stratum an era that's ended built or made its own; cursed: an era's that ended cursed; lair:
** the raiders' lair, the world's (the first world's kobolds'), where they keep a prisoner or their raiders' standard; deep:
** the deepest, a far exit), its era, its guardians (the strongest faction that lives there: the dead, the kobolds, the
** beasts, the spiders, the rats; a prisoner's its captors), a stratum above it (where the evil's undoing lies), its
** things (one of each role its kind has) and their names, its person (a prisoner, a seeker, an old priest: in the world's
** common tongue), and the theme's title.
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#define BANKPROG
#include "../../games/themegen/themegen.h"

static char rfac[12];

/* ---- The inhabitants now (histgen's till 2026-11-21, its bank full: the history's last, the goal's first): each of
** the bestiary's factions of the world's (its folk lines) that has a stratum its kinds live in, and that the history
** calls for, its strata those, its lair one of them (the dead's an era's that ended
** cursed, the raiders' the deepest of their mines), its number its kinds' lairs' groups.  The peoples (Phase 4's,
** 2026-10-29; the world's holds and colony lines, themegen's peoples): the first world's dwarves, elves and men's
** soldiers where an era of theirs holds its works still, on its strata; its gargun in mountains, hills or forest where
** the dwarves' works were lost (the book's: their colonies in abandoned Khuzdul halls), a tribe of one or two of their
** kinds (white orcs in cold mountains; else brown orcs ruled by black ones, red orcs, or brown and small orcs), with a
** king; the dwarves' elder and the soldiers' captain their leaders */
#pragma static-locals (push, on)                /* (Its locals static: the bank program's room, 2026-10-29) */
static void make_factions (void)
{
    unsigned char k, b, sn, n, ok, any, cursed = 0, dead = 0, nw, h, i, pass;
    unsigned pm = 0;                            /* (The peoples' factions, a bit each: the first pass's) */
    unsigned char where[TSTRATA];
    const char* ch;                             /* (Its kinds chosen: of its own, these only) */
    struct fac* f;
    struct beast* p;

    if (data (TK_RAIDERS, 0, 0)) {              /* (The world's raiders' faction: the first world's kobolds) */
        strncpy (rfac, D.v[1], sizeof rfac - 1);
    }
    for (k = 0; k < ne; ++k) {
        if (er[k].end == EN_CURSED) {
            cursed = k + 1;
        }
        dead |= er[k].end == EN_CURSED || er[k].end == EN_CONQUERED || er[k].work == W_TOMB || er[k].work == W_TEMPLE;
    }
    nf = 0;
    for (pass = 0; pass < 2; ++pass) {          /* (The peoples first, as they hold their works: then the rest, as
                                                **   many as there's room for) */
        for (k = 0; nf < TFACTIONS && data (TK_FACTION, 0, k); ++k) {
            f = &fa[nf];
            memset (f, 0, sizeof *f);
            strcpy (f->name, D.v[0]);           /* (Its line copied out: the world's lines asked after it) */
            f->att[0] = *D.v[1];
            strncpy (f->others, D.n > 2 ? D.v[2] : "-", sizeof f->others - 1);
            if (!data (TK_FOLK, f->name, 0)) {
                continue;                       /* (Not of the world's) */
            }
            ok = !strcmp (f->name, "dead") ? dead :
                !strcmp (f->name, "beasts") ? data (TK_WILD, 0, 0) && among (land, D.v[0]) : 1;
            h = 0;
            ch = 0;
            if (pass ? (pm & 1u << k) != 0 : (ch = peoples (f->name)) == 0) {
                continue;                           /* (A people's the first pass's: its era, its kinds, its leader) */
            }
            if (ch) {
                pm |= 1u << k;
                ok = (h = pk_era) != 0;
            }
            if (!ok) {
                continue;
            }
            for (nw = sn = 0; sn < ns; ++sn) {      /* (Where its kinds live) */
                for (any = b = 0; b < nbeasts; ++b) {
                    any |= beasts[b].faction == k && (!ch || among (beasts[b].key, ch)) && lives (b, sn);
                }
                if (any) {
                    where[nw++] = sn;
                }
            }
            for (i = sn = 0; h && sn < nw; ++sn) {  /* (A people's on its era's strata, if its kinds live there) */
                if (era_now (where[sn]) == h) {
                    where[i++] = where[sn];
                }
            }
            if (i) {
                nw = i;
            }
            if (!nw) {
                continue;
            }
            f->d0 = where[0];
            f->d1 = where[nw - 1];
            f->lair = where[random (nw)];
            for (n = b = 0, p = beasts; b < nbeasts; ++b, ++p) {
                if (p->faction == k && (!ch || among (p->key, ch))) {
                    if (*f->kinds) {
                        strcat (f->kinds, " ");
                    }
                    strcat (f->kinds, p->key);
                    n += (p->n0 + p->n1) * p->lairs / 2;
                    if (!strcmp (f->name, rfac) && !f->leader) {
                        f->leader = b + 1;          /* (Its chieftain: one of its own) */
                    }
                }
            }
            f->n = n ? n : 1;
            if (ch && pk_lead) {
                f->leader = beast_of (pk_lead) + 1;
            }
            if (cursed && !strcmp (f->name, "dead")) {
                for (sn = 0; sn < nw && era_now (where[sn]) != cursed; ++sn) {
                }
                if (sn < nw) {
                    f->lair = where[sn];
                }
            } else if (!strcmp (f->name, rfac)) {
                f->lair = where[nw - 1];
            }
            if (f->leader && !lives (f->leader - 1, f->lair)) {
                f->leader = 0;
            }
            ++nf;
        }
    }
}
#pragma static-locals (pop)

/* A hand-written theme's faction lines read in (themegen -t), in place of the inhabitants made */
static void __fastcall__ take_faction (struct line* l)
{
    char** v = l->v;
    struct fac* f;

    if (!strcmp (l->key, "faction") && l->n >= 7 && nf < TFACTIONS) {
        f = &fa[nf++];
        memset (f, 0, sizeof *f);
        strncpy (f->name, v[0], 11);
        strncpy (f->kinds, v[1], sizeof f->kinds - 1);
        f->d0 = atoi (v[2]);
        f->d1 = strchr (v[2], '-') ? atoi (strchr (v[2], '-') + 1) : f->d0;
        f->n = atoi (v[3]);
        f->leader = strcmp (v[4], "-") ? beast_of (v[4]) + 1 : 0;
        f->lair = atoi (v[5]);
        f->att[0] = *v[6];
        strncpy (f->others, l->n > 7 ? v[7] : "-", sizeof f->others - 1);
    }
}

/* The faction (fa: 0 ...) that guards stratum k: the first that lives there of the dead, the kobolds, the beasts,
** the crawlers, the vermin (the gargun after the dead, 2026-10-29); any other; or 0xFF */
static unsigned char __fastcall__ guard_of (unsigned char k)
{
    static const char* const order[] = { "dead", "gargun", "kobolds", "beasts", "crawlers", "vermin", 0 };
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

/* The world's raiders' faction (its raiders line's: the first world's kobolds), fa's, or 0xFF (none there, or no raiders) */
static unsigned char __fastcall__ kobolds (void)
{
    unsigned char j;

    if (!data (TK_RAIDERS, 0, 0)) {
        return 0xFF;
    }
    for (j = 0; j < nf && strcmp (fa[j].name, D.v[1]); ++j) {
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

/* ---- Why the character came (the plan's step 7) */

/* A number from x (a pick that spends none of the history's dice) */
static unsigned long __fastcall__ mix (unsigned long x)
{
    x ^= x >> 15;
    x *= 2654435761UL;
    x ^= x >> 13;
    return x;
}

/* Its reason: the goal block's why line (the one given, a rumour's; else the seed's pick, none of the history's dice spent),
** its patron (given, or a name the seed's: %Q), what the patron pays (in its line's range, by the seed: %D) */
static void why_pick (void)
{
    unsigned char m = 0, k;
    int j, end = tg_find (TK_GOAL, 0, gk + 1);
    unsigned lo, hi;
    unsigned long h = mix (seed ^ 0x57485921UL);

    if (end < 0) {
        end = tg_lines;
    }
    for (j = goal_line + 1; j < end; ++j) {
        if (tg_key[j] == TK_WHY) {
            tg_get (j, &D, Dt);
            if (*gwhy && !strcmp (D.v[0], gwhy)) {
                gwl = j;                        /* (The one given) */
                break;
            }
            ++m;
        }
    }
    for (k = m ? (unsigned char) (h % m) : 0, j = goal_line + 1; gwl < 0 && m && j < end; ++j) {
        if (tg_key[j] == TK_WHY && !k--) {
            gwl = j;
        }
    }
    if (!*gQ) {
        strcpy (gQ, name_of (0, NG_PERSON, h ^ 0x50415452UL));   /* (In the world's common tongue) */
    }
    if (gwl >= 0) {
        tg_get (gwl, &D, Dt);
        lo = atoi (D.v[3]);
        hi = atoi (strchr (D.v[3], '-') + 1);
        gpence = lo + (unsigned) (mix (h) % (hi - lo + 1));
        strcpy (gD, num (gpence));
        strcat (gD, gpence == 1 ? " penny" : " pennies");
    }
}

/* A rumour (themegen -r: rmode), a part-written theme for dq0acct to tell and the next dungeon to be made from (quest.txt, the
** one followed): the words heard (%R and %Q its slots: the rest is made with the dungeon), the region, the why */
static void rumour (void)
{
    if (gwl < 0) {
        return;
    }
    SL ('R') = rname;
    SL ('Q') = gQ;
    tg_get (gwl, &D, Dt);
    fill (W, D.v[1], LINE_TEXT);
    begin_line ("rumour");
    val (W);
    end_line ();
    region_line ();
    data (TK_GOAL, 0, gk);
    begin_line ("why");
    val (D.v[0]);
    tg_get (gwl, &D, Dt);
    val (D.v[0]);
    val (gQ);
    end_line ();
}

/* The goal's slots, for linegen's lines (goals.txt's; %Q and %D why the character came), made in themegen's main RAM and kept there
** (gsl: linegen puts them back after an era's words have had them) */
static void goal_slots (void)
{
    struct era* g;
    unsigned char x;

    strcpy (gs_s, num (gs));
    strcpy (gu_s, num (gu));
    strcpy (ge_s, num (ge));
    strcpy (geu_s, num (geu));
    SL ('A') = gA;
    SL ('a') = ga;
    SL ('B') = gB;
    SL ('b') = gb;
    SL ('M') = gM;
    SL ('S') = st[gs].name;
    SL ('s') = gs_s;
    SL ('E') = ge_s;
    SL ('U') = st[gu].name;
    SL ('u') = gu_s;
    SL ('e') = geu_s;
    SL ('T') = st[0].name;
    for (x = (unsigned char) seed; ns > 1 && x >= ns - 1; x -= ns - 1) {
    }                                           /* (Another way out's stratum: below the top, by the seed, none of its dice) */
    strcpy (gx_s, num (ns > 1 ? x + 1 : 1));
    SL ('O') = gx_s;
    g = er + (ge - 1);                          /* (Through a pointer, as make's) */
    SL ('N') = g->great;
    SL ('P') = g->pthe;
    SL ('K') = g->kind;
    SL ('X') = g->place;
    SL ('R') = rname;
    SL ('G') = gg != 0xFF ? fa[gg].name : "-";
    strcpy (gg_w, "the dark");
    if (gg != 0xFF) {
        if (data (TK_FOLK, fa[gg].name, 0)) {
            strcpy (gg_w, D.v[1]);
        } else {
            strcpy (gg_w, "the ");
            strcat (gg_w, fa[gg].name);
        }
    }
    SL ('g') = gg_w;
    SL ('Q') = gQ;                              /* (Why it came: its patron, what it pays) */
    SL ('D') = gD;
    memcpy (gsl, sl, sizeof gsl);
}

void make (void)
{
    static char where[8];
    unsigned char k, n, cand[TSTRATA], nc, m;
    int j;
    struct era* g;

    if (!rmode) {                               /* (The inhabitants now, the history's last) */
        make_factions ();
        if (given[GR_DENS]) {
            nf = 0;
            given_each (GR_DENS, take_faction);
        }
    }
    gwl = -1;
    if (rmode && gk != 0xFF) {                  /* (A rumour's: its reason, its patron, its lines; the goal's made with the
                                                **   dungeon) */
        goal_line = tg_find (TK_GOAL, 0, gk);
        why_pick ();
        rumour ();
        return;
    }
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
    strcpy (gM, name_of (gk == G_EVIL - 1 && ge < ne ? g[1].tongue : gk == G_SCOURGE - 1 ? g->tongue : 0, NG_PERSON,
        rand32 ()));                            /* (A scourge's chief named in its own people's tongue) */
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
    why_pick ();                                /* (Why it came: none of the history's dice) */
    goal_slots ();
}
