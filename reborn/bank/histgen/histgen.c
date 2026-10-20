/*
** histgen.c - (a bank program, themegen's: sdk/c/include/bankprog.h) a region's history made (games/dq0/HISTORY.md),
** into themegen's main RAM (games/themegen/themegen.h), from themegen's dice, already seeded: the region (its land,
** rock, water and climate: history.txt's lands), its eras oldest first (two or three: who came, what they made, how
** it ended, how long ago; their names namegen's), the strata they left, from the top down (each era's works over
** the older ones', natural caves under all; a later people may take an older work for its own, a layer reused; an
** era's end a layer ruined), and the inhabitants now (the bestiary's factions that fit the strata and the history:
** the dead where an era ended cursed or conquered or laid its dead, beasts in wild land, vermin, crawlers and kobolds
** wherever their kinds can live).  A hand-written theme's own groups (themegen -t) are read in in place of what it
** makes, so what's made after fits them.
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#define BANKPROG
#include "../../games/themegen/themegen.h"

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

/* The eras, oldest first: each its people (of the land, not the one before it; kobolds only the last), what they
** made, how it ended (the last may hold it still), how long ago; their names.  A prisoner's goal has the kobolds
** last, holding mines of their own (its keepers); the evil's an era that ended cursed */
static void make_eras (void)
{
    static char w[12], cand[8];
    unsigned char e, k, n, nc, i, cursed = 0, prisoner = gk == G_PRISONER - 1;
    struct era* r;

    ne = 2 + random (2);
    n = tg_count (TK_PEOPLE, 0);
    for (e = 0, r = er; e < ne; ++e, ++r) {
        for (nc = k = 0; k < n; ++k) {
            data (TK_PEOPLE, 0, k);
            if (prisoner && e + 1 == ne ? !strcmp (D.v[0], "kobolds") : among (land, D.v[3]) &&
                (strcmp (D.v[0], "kobolds") || e + 1 == ne) && (!e || k != r[-1].what)) {
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
    s->purpose = p < 0 ? P_HALLS : p;
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
    unsigned char e, k, n, i, nd, top, tries, kobolds = gk == G_PRISONER - 1;
    signed char sub;
    struct str* s;
    struct era* r;

    ns = 0;
    nd = 1 + random (2);
    for (sub = nd - 1; sub >= 0; --sub) {
        push (WK_DEEP, sub, "n", P_HALLS);
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
            layer (s, e + 1, TL_REUSED, k == 0xFF ? P_HALLS : k);
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

/* ---- The inhabitants now: each of the bestiary's factions that has a stratum its kinds live in, and that the
** history calls for, its strata those, its lair one of them (the dead's an era's that ended cursed, the kobolds' the
** deepest of their mines), its number its kinds' lairs' groups */
static void make_factions (void)
{
    unsigned char k, b, sn, n, ok, any, cursed = 0, dead = 0, nw;
    unsigned char where[TSTRATA];
    struct fac* f;
    struct beast* p;

    for (k = 0; k < ne; ++k) {
        if (er[k].end == EN_CURSED) {
            cursed = k + 1;
        }
        dead |= er[k].end == EN_CURSED || er[k].end == EN_CONQUERED || er[k].work == W_TOMB || er[k].work == W_TEMPLE;
    }
    nf = 0;
    for (k = 0; nf < TFACTIONS && data (TK_FACTION, 0, k); ++k) {
        f = &fa[nf];
        memset (f, 0, sizeof *f);
        for (nw = sn = 0; sn < ns; ++sn) {      /* (Where its kinds live) */
            for (any = b = 0; b < nbeasts; ++b) {
                any |= beasts[b].faction == k && lives (b, sn);
            }
            if (any) {
                where[nw++] = sn;
            }
        }
        ok = !strcmp (D.v[0], "dead") ? dead : !strcmp (D.v[0], "beasts") ? among (land, "mountains hills forest") : 1;
        if (!nw || !ok) {
            continue;
        }
        strcpy (f->name, D.v[0]);
        f->att[0] = *D.v[1];
        strncpy (f->others, D.n > 2 ? D.v[2] : "-", sizeof f->others - 1);
        f->d0 = where[0];
        f->d1 = where[nw - 1];
        f->lair = where[random (nw)];
        for (n = b = 0, p = beasts; b < nbeasts; ++b, ++p) {
            if (p->faction == k) {
                if (*f->kinds) {
                    strcat (f->kinds, " ");
                }
                strcat (f->kinds, p->key);
                n += (p->n0 + p->n1) * p->lairs / 2;
                if (!strcmp (f->name, "kobolds") && !f->leader) {
                    f->leader = b + 1;          /* (Its chieftain: one of its own) */
                }
            }
        }
        f->n = n ? n : 1;
        if (cursed && !strcmp (f->name, "dead")) {
            for (sn = 0; sn < nw && era_now (where[sn]) != cursed; ++sn) {
            }
            if (sn < nw) {
                f->lair = where[sn];
            }
        } else if (!strcmp (f->name, "kobolds")) {
            f->lair = where[nw - 1];
        }
        if (f->leader && !lives (f->leader - 1, f->lair)) {
            f->leader = 0;
        }
        ++nf;
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
    int p;                                      /* (A purpose: P_FORT its high bit) */
    struct era* r;
    struct str* s;
    struct fac* f;

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
        r->tongue = i < npl ? pl_tongue[i] : 2;
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
        p = l->n > 2 && *v[2] ? purpose_of (v[2]) : P_HALLS;
        s->purpose = p < 0 ? P_HALLS : p;
        p = s->purpose & 7;
        s->work = *v[1] == 'n' ? WK_DEEP : *v[1] == 'm' ? W_MINE : p == P_KEEP ? W_KEEP : p == P_TEMPLE ? W_TEMPLE :
            p == P_CRYPT ? W_TOMB : p == P_CELLARS ? W_TOWN : W_HOLD;
    } else if (!strcmp (k, "layer") && l->n >= 4 && (sn = atoi (v[0])) < TSTRATA) {
        if ((p = find_word (dids, v[2])) >= 0) {
            layer (&st[sn], atoi (v[1]), p, p == TL_RUINED ? find_word (endings, v[3]) : purpose_of (v[3]));
        }
    } else if (!strcmp (k, "faction") && l->n >= 7 && nf < TFACTIONS) {
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

/* The history: the region, the eras, the region's name (the oldest people's), the strata, the inhabitants; the
** goal's kind first (goalgen's, the data's goal lines': a prisoner where there are no kobolds to keep one an
** artifact), as the eras are made to fit it */
void make (void)
{
    unsigned char n = tg_count (TK_GOAL, 0);

    make_region ();
    gk = n ? random (n) : 0xFF;
    if (gk == G_PRISONER - 1 && !among (land, "mountains hills")) {
        gk = G_ARTIFACT - 1;
    }
    if (given[GR_GOAL]) {
        gk = 0xFF;                              /* (A hand-written theme's own puzzle) */
    }
    make_eras ();
    if (given[GR_HIST]) {
        ne = npl = 0;
        given_each (GR_HIST, take);
    }
    if (given[GR_REGION]) {
        given_each (GR_REGION, take);
    } else {
        strcpy (rname, "the ");
        strcat (rname, name_of (ne ? er[0].tongue : 2, NG_PLACE, rand32 ()));
        strcat (rname, " ");
        strcat (rname, noun);
    }
    make_strata ();
    if (given[GR_STRATA]) {
        ns = 0;
        given_each (GR_STRATA, take);
    }
    make_factions ();
    if (given[GR_DENS]) {
        nf = 0;
        given_each (GR_DENS, take);
    }
}
