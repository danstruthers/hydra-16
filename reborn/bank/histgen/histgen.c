/*
** histgen.c - (a bank program, themegen's: sdk/c/include/bankprog.h) a region's history made (games/dq0/HISTORY.md),
** into themegen's main RAM (games/themegen/themegen.h), from themegen's dice, already seeded: the region (its land,
** rock, water and climate: history.txt's lands), its eras oldest first (two or three: who came, what they made, how
** it ended, how long ago; their names namegen's), the strata they left, from the top down (each era's works over
** the older ones', natural caves under all; a later people may take an older work for its own, a layer reused; an
** era's end a layer ruined); the inhabitants now goalgen's (2026-11-21: this bank full).  A hand-written theme's own
** groups (themegen -t) are read in in place of what it makes, so what's made after fits them.  Its peoples, its lands
** and its names are the world's (games/dq0/WORLDS.md: themegen -w's history.dat), and so are its raiders (the first
** world's kobolds).
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#define BANKPROG
#include "../../games/themegen/themegen.h"

/* The world's raiders (its raiders line: a people that comes only last, digs its own mines, keeps a prisoner and raids
** from them; their faction goalgen's): their name in raid; none, "" */
static char raid[12];
static void raiders (void)
{
    if (data (TK_RAIDERS, 0, 0)) {
        strncpy (raid, D.v[0], sizeof raid - 1);
    }
}

/* The region: a land, its words, its rock, its water and its climate */
static void make_region (void)
{
    data (TK_LAND, 0, random (tg_count (TK_LAND, 0)));
    strcpy (land, D.v[0]);
    pick (D.v[1], noun);
    pick (D.v[2], rock);
    pick (D.v[3], water);
    pick (D.v[4], climate);
}

/* The eras, oldest first: each its people (of the land, not the one before it; the raiders only the last), what they
** made, how it ended (the last may hold it still), how long ago; their names.  A prisoner's goal has the raiders
** last, holding mines of their own (its keepers), as a scourge's has; the evil's an era that ended cursed */
static void make_eras (void)
{
    static char w[12], cand[8];
    unsigned char e, k, n, nc, i, cursed = 0, prisoner = gk == G_PRISONER - 1 || gk == G_SCOURGE - 1;
    struct era* r;

    ne = 2 + random (2);
    n = tg_count (TK_PEOPLE, 0);
    for (e = 0, r = er; e < ne; ++e, ++r) {
        for (nc = k = 0; k < n; ++k) {
            data (TK_PEOPLE, 0, k);
            if (prisoner && e + 1 == ne ? !strcmp (D.v[0], raid) : among (land, D.v[3]) &&
                (strcmp (D.v[0], raid) || e + 1 == ne) && (!e || k != r[-1].what)) {
                cand[nc++] = k;
            }
        }
        memset (r, 0, sizeof *r);
        r->what = nc ? cand[random (nc)] : 1;
        data (TK_PEOPLE, 0, r->what);
        strcpy (r->kind, D.v[0]);
        r->tongue = atoi (D.v[1]);
        r->work = work_of (pick (D.v[4], w));
        r->end = random (5);                    /* (Abandoned ... cursed) */
        if (e + 1 == ne) {                      /* (The last: none came after to take it) */
            if (r->end == EN_CONQUERED || prisoner) {
                r->end = EN_HELD;
            }
            if (prisoner) {
                r->work = W_MINE;
            }
        }
        cursed |= r->end == EN_CURSED;
        r->ago = e ? r[-1].ago * (1 + random (2)) / 3 : 18 + random (23);
        if (e && r->ago >= r[-1].ago) {
            r->ago = r[-1].ago - 1;
        }
        if (r->end == EN_HELD) {
            r->ago = 0;
        } else if (!r->ago) {
            r->ago = 1;
        }
        strcpy (r->people, name_of (r->tongue, NG_PEOPLE, rand32 ()));
        strcpy (r->pthe, "the ");
        strcat (r->pthe, r->people);
        strcpy (r->great, name_of (r->tongue, NG_PERSON, rand32 ()));
        strcpy (r->place, name_of (r->tongue, NG_PLACE, rand32 ()));
        r->took = 0xFF;
    }
    if (gk == G_EVIL - 1 && !cursed) {          /* (The evil's: an era that ended cursed) */
        for (i = random (ne), k = 0; k < ne && er[(i + k) % ne].end == EN_HELD; ++k) {
        }
        er[(i + k) % ne].end = EN_CURSED;
    }
}

/* ---- The strata, from the bottom up, then turned over: the natural caves (one or two), then each era's works over
** the older ones' (or an older one made its own: a layer reused), each one its era's end a layer ruined; their
** names, one of their works' that fits and isn't another's */

static struct str* __fastcall__ push (unsigned char work, unsigned char sub, const char* styles, int p)
{
    struct str* s = &st[ns++];

    memset (s, 0, sizeof *s);
    s->work = work;
    s->sub = sub;
    strncpy (s->styles, styles, 2);
    s->purpose = p < 0 ? SP_HALLS : p;
    return s;
}

static void __fastcall__ layer (struct str* s, unsigned char era, unsigned char did, unsigned char work)
{
    struct tlayer* y = &s->l[s->nl];

    if (s->nl < TLAYERS) {
        y->era = era;
        y->did = did;
        y->work = work;
        ++s->nl;
    }
}

static void make_strata (void)
{
    static struct str t;
    static char nm[40];
    unsigned char e, k, n, i, nd, top, tries, kobolds = gk == G_PRISONER - 1 || gk == G_SCOURGE - 1;   /* (The raiders') */
    signed char sub;
    struct str* s;
    struct era* r;

    ns = 0;
    nd = 1 + random (2);
    for (sub = nd - 1; sub >= 0; --sub) {
        push (WK_DEEP, sub, "n", SP_HALLS);
    }
    for (e = 0, r = er; e < ne; ++e, ++r) {
        data (TK_WORK, works[r->work], 0);
        n = (D.n - 1) / 2;                      /* (Its strata) */
        top = ns;
        if (e && ns > nd && !(kobolds && e + 1 == ne && ns + n <= TSTRATA) && (!random (3) || ns + n > TSTRATA)) {
            s = &st[ns - 1];                    /* (An older work taken for its own: its top stratum; a prisoner's
                                                **   kobolds dig their own) */
            r->took = s->work;
            k = purpose_of (D.v[1]);
            layer (s, e + 1, TL_REUSED, k == 0xFF ? SP_HALLS : k);
            top = ns - 1;
        } else {
            for (sub = n - 1; sub >= 0 && ns < TSTRATA; --sub) {
                s = push (r->work, sub, D.v[2 + 2 * sub], purpose_of (D.v[1 + 2 * sub]));
                layer (s, e + 1, TL_BUILT, s->purpose);
            }
        }
        if (r->end != EN_HELD) {                /* (Its end: a layer ruined, on what it held) */
            for (k = top, s = &st[top]; k < ns; ++k, ++s) {
                if (s->l[s->nl - 1].era == e + 1) {
                    layer (s, e + 1, TL_RUINED, r->end);
                    if (r->end == EN_COLLAPSED && s->styles[0] == 'b') {
                        strcpy (s->styles, "bx");   /* (The rock broken in) */
                    } else if (r->end == EN_FLOODED && s->styles[0] == 'b' && !s->styles[1]) {
                        strcpy (s->styles, "bn");
                    }
                }
            }
        }
    }
    for (i = 0; i < ns / 2; ++i) {              /* (Turned over: the top first) */
        memcpy (&t, &st[i], sizeof t);
        memcpy (&st[i], &st[ns - 1 - i], sizeof t);
        memcpy (&st[ns - 1 - i], &t, sizeof t);
    }
    for (k = 0, s = st; k < ns; ++k, ++s) {
        e = s->nl ? s->l[0].era : 0;
        SL ('X') = e ? er[e - 1].place : "Old";
        for (tries = 0; tries < 8; ++tries) {
            for (i = 0; data (TK_NAME, works[s->work], i) && atoi (D.v[1]) != s->sub; ++i) {
            }
            fill (nm, D.v[2 + (tries < 6 ? random (D.n - 2) : D.n - 3)], sizeof nm);
            if (strlen (nm) < 20) {
                for (i = 0; i < k && strcmp (st[i].name, nm); ++i) {
                }
                if (i == k || tries == 7) {
                    break;
                }
            }
        }
        strncpy (s->name, nm, 19);
    }
}

/* ---- A hand-written theme's own groups read in, so that what's made fits them */

static unsigned char npl;
static char pl_name[TERAS][12], pl_kind[TERAS][12];
static unsigned char pl_tongue[TERAS];

static void __fastcall__ take (struct line* l)
{
    char** v = l->v;
    const char* k = l->key;
    unsigned char i, sn;
    int p;                                      /* (A purpose: SP_FORT its high bit) */
    struct era* r;
    struct str* s;

    if (!strcmp (k, "region") && l->n >= 5) {
        strncpy (rname, v[0], sizeof rname - 1);
        strncpy (land, v[1], sizeof land - 1);
        strncpy (rock, v[2], sizeof rock - 1);
        strncpy (water, v[3], sizeof water - 1);
        strncpy (climate, v[4], sizeof climate - 1);
    } else if (!strcmp (k, "people") && l->n >= 4 && npl < TERAS) {
        strncpy (pl_name[npl], v[0], 11);
        strncpy (pl_kind[npl], v[1], 11);
        pl_tongue[npl++] = atoi (v[2]);
    } else if (!strcmp (k, "era") && l->n >= 4 && ne < TERAS) {
        r = &er[ne++];
        memset (r, 0, sizeof *r);
        for (i = 0; i < npl && strcmp (pl_name[i], v[0]); ++i) {
        }
        r->tongue = i < npl ? pl_tongue[i] : data (TK_COMMON, 0, 0) ? atoi (D.v[0]) : 1;   /* (Else the world's common tongue) */
        strcpy (r->kind, i < npl ? pl_kind[i] : "folk");
        strncpy (r->people, v[0], 11);
        strcpy (r->pthe, "the ");
        strcat (r->pthe, r->people);
        p = work_of (v[1]);
        r->work = p < 0 ? W_HOLD : p;
        p = find_word (endings, v[2]);
        r->end = p < 0 ? EN_ABANDONED : p;
        r->ago = atoi (v[3]);
        r->took = 0xFF;
        strcpy (r->great, name_of (r->tongue, NG_PERSON, rand32 ()));
        strcpy (r->place, name_of (r->tongue, NG_PLACE, rand32 ()));
        if (l->n > 5 && *v[5]) {                /* (Its words its own, the period gone) */
            strncpy (r->words, v[5], sizeof r->words - 1);
            i = strlen (r->words);
            if (i && r->words[i - 1] == '.') {
                r->words[i - 1] = 0;
            }
            r->own = 1;
        }
    } else if (!strcmp (k, "stratum") && l->n >= 2 && ns < TSTRATA) {
        s = &st[ns++];
        memset (s, 0, sizeof *s);
        strncpy (s->name, v[0], 19);
        strncpy (s->styles, v[1], 2);
        p = l->n > 2 && *v[2] ? purpose_of (v[2]) : SP_HALLS;
        s->purpose = p < 0 ? SP_HALLS : p;
        p = s->purpose & 7;
        s->work = *v[1] == 'n' ? WK_DEEP : *v[1] == 'm' ? W_MINE : p == SP_KEEP ? W_KEEP : p == SP_TEMPLE ? W_TEMPLE :
            p == SP_CRYPT ? W_TOMB : p == SP_CELLARS ? W_TOWN : W_HOLD;
    } else if (!strcmp (k, "layer") && l->n >= 4 && (sn = atoi (v[0])) < TSTRATA) {
        if ((p = find_word (dids, v[2])) >= 0) {
            layer (&st[sn], atoi (v[1]), p, p == TL_RUINED ? find_word (endings, v[3]) : purpose_of (v[3]));
        }
    }
}

/* A goal kind (the data's goal line k) a history picks by itself: not one that's only heard of (goals.txt's heard), but making
** rumours (rmode), any */
static unsigned char __fastcall__ pickable (unsigned char k)
{
    return data (TK_GOAL, 0, k) && (rmode || D.n < 4 || strcmp (D.v[3], "heard"));
}

/* Why the character came, given (a theme's why line: the rumour it followed, quest.txt's): its goal kind, its reason, its
** patron */
static void __fastcall__ why_of (struct line* l)
{
    unsigned char k;

    if (strcmp (l->key, "why")) {
        return;
    }
    for (k = 0; data (TK_GOAL, 0, k); ++k) {
        if (!strcmp (D.v[0], l->v[0])) {
            gk = k;
            strncpy (gwhy, l->n > 1 ? l->v[1] : "", sizeof gwhy - 1);
            strncpy (gQ, l->n > 2 ? l->v[2] : "", sizeof gQ - 1);
            return;
        }
    }
}

/* The history: the region (a hand-written theme's, if it gives it: the goal and the eras fit its land), the eras, the region's
** name (the oldest people's), the strata (the inhabitants goalgen's, 2026-11-21: this bank full); the goal's kind first
** (goalgen's, the data's goal lines': one of those a history picks by itself, or the why's it's given; a prisoner or a
** scourge where there are no raiders to keep one or raid, an artifact), as the eras are made to fit it */
void make (void)
{
    unsigned char n = tg_count (TK_GOAL, 0), m, k, r;

    make_region ();
    if (given[GR_REGION]) {
        given_each (GR_REGION, take);
    }
    for (m = k = 0; k < n; ++k) {
        m += pickable (k);
    }
    gk = 0xFF;
    if (m) {
        for (r = random (m), k = 0; k < n; ++k) {   /* (The r'th of those it picks by itself: the same dice as ever) */
            if (pickable (k) && !r--) {
                gk = k;
                break;
            }
        }
    }
    if (given[GR_HEAD]) {
        given_each (GR_HEAD, why_of);
    }
    raiders ();                                 /* (The world's raiders: raid) */
    if ((gk == G_PRISONER - 1 || gk == G_SCOURGE - 1) && !(*raid && data (TK_PEOPLE, raid, 0) && among (land, D.v[3]))) {
        gk = G_ARTIFACT - 1;                    /* (Not where the world's raiders live, or it has none) */
    }
    if (given[GR_GOAL]) {
        gk = 0xFF;                              /* (A hand-written theme's own puzzle) */
    }
    make_eras ();
    if (given[GR_HIST]) {
        ne = npl = 0;
        given_each (GR_HIST, take);
    }
    if (!given[GR_REGION]) {
        strcpy (rname, "the ");
        strcat (rname, name_of (ne ? er[0].tongue : 0, NG_PLACE, rand32 ()));
        strcat (rname, " ");
        strcat (rname, noun);
    }
    if (rmode) {
        return;                                 /* (A rumour's: its place and its goal kind are all it tells) */
    }
    make_strata ();
    if (given[GR_STRATA]) {
        ns = 0;
        given_each (GR_STRATA, take);
    }
}
