/*
** comp.c - (themec) a theme's lines compiled into its record (theme.h's struct theme, then its words: THEME_SIZE
** bytes, R), a line at a time as they're written, as ../dq0/make.js's compile makes it from its file (the tests hold
** the two to the same bytes); and the theme checker (make.js's check: every goal reachable, every faction placed).
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "comp.h"

unsigned char R[THEME_SIZE];
#define T               ((struct theme*) R)

static unsigned words;
static const char* err;
static unsigned char nstrata, npeoples, dens_set;
static char sname[TSTRATA][20], sstyle[TSTRATA][3];
static unsigned char spurp[TSTRATA];
static char pname[8][16];
static unsigned char ptongue[8];
static char pguard[NPIECES][12], gguard[12];

static const char* const mats[] = { "none", "wood", "iron", "steel", "bronze", "leather", "cloth", "bone", "clay",
    "stone", "glass", "silver", "other", 0 };
static const char* const crafts[] = { "weapon", "armour", "metal", "wood", "hide", "cloth", "pottery", "jewel",
    "book", "food", "bone", "stone", "glass", 0 };
static const char* const verbs[] = { "read", "combine", "put", "out", 0 };
static const char* const effects[] = { "-", "open", "rule", 0 };
static const char* const purposes[] = { "halls", "keep", "temple", "cellars", "crypt", "works", 0 };
static const char* const did[] = { "built", "ruined", "reused", 0 };
static const char* const endings[] = { "abandoned", "conquered", "collapsed", "flooded", "cursed", "held", 0 };
static const char* const works[] = { "keep", "temple", "mine", "tomb", "town", "hold", 0 };
static const char* const goals[] = { "none", "artifact", "evil", "prisoner", "through", 0 };
static const char* const ends[] = { "-", "escaped", "ruled", "died", 0 };
static const char* const uses[TUSES] = { "kitchen", "pantry", "barracks", "armoury", "chapel", "bedchamber", "solar",
    "well room", "cistern", "gatehouse", "guardroom", "workshop" };
static const char ways[] = "chtwgpsf", styles[] = "bnmx";
static const unsigned char wbits[] = { 0x01, 0x10, 0x90, 0x08, 0x20, 0x40, 0x10, 0x80 };

/* s's place in list (0 ...), or -1 */
static signed char __fastcall__ find (const char* const* list, const char* s)
{
    signed char k;

    for (k = 0; list[k]; ++k) {
        if (!strcmp (list[k], s)) {
            return k;
        }
    }
    return -1;
}

/* A string put in the record at at (shorter than max), or its fault */
static void __fastcall__ put (const char* s, unsigned at, unsigned char max)
{
    unsigned char n = strlen (s);

    if (n >= max) {
        err = "a name or a word too long";
    } else {
        memcpy (R + at, s, n);
    }
}

/* Words put after the record: their offset */
static unsigned __fastcall__ text (const char* s)
{
    unsigned at = words, n = strlen (s);

    if (at + n + 1 > THEME_SIZE) {
        err = "more words than the theme has room for";
        return 0;
    }
    memcpy (R + at, s, n + 1);
    words += n + 1;
    return at;
}

static void __fastcall__ word (unsigned at, unsigned w)
{
    R[at] = w;
    R[at + 1] = w >> 8;
}

static unsigned char nkinds, npieces, nrules;

/* A kind of the theme's by its name (0 ...), or its fault */
static unsigned char __fastcall__ kind (const char* s)
{
    unsigned char k;

    for (k = 0; k < nkinds; ++k) {
        if (!strcmp (T->kind[k].name, s)) {
            return k;
        }
    }
    err = "no such kind of the theme's";
    return 0;
}

/* "first-last" (or one, if one may be): 0, or 1 (its fault) */
static unsigned char __fastcall__ range (const char* s, unsigned char* a, unsigned char* b, unsigned char one)
{
    const char* d = strchr (s, '-');

    if (!*s || (!d && !one)) {
        return 1;
    }
    *a = atoi (s);
    *b = d ? atoi (d + 1) : *a;
    return *a > *b;
}

void comp_begin (void)
{
    memset (R, 0, sizeof R);
    words = THEME_HEAD;
    nkinds = npieces = nrules = nstrata = npeoples = dens_set = 0;
    memset (pguard, 0, sizeof pguard);
    gguard[0] = 0;
    R[0] = 1;                                   /* (One theme in its file) */
}

/* A weight in pounds, to a quarter ("2", "2.25", "2.5", "2.75"), made quarter pounds (rs.h's WT_LB), max at most */
static unsigned __fastcall__ pounds (const char* s, unsigned max)
{
    unsigned long q = (unsigned long) atoi (s) * WT_LB;
    const char* d = strchr (s, '.');

    if (d) {
        q += d[1] == '2' ? 1 : d[1] == '5' ? 2 : d[1] == '7' ? 3 : 0;
    }
    return q > max ? max : (unsigned) q;
}

/* A kind's line: name, ways, weight (pounds), bulk (pints), how far off it's made out (feet, in fives: the map's
** squares), material, craft, worth (pennies); a container's room (pints) and the weight it bears (pounds): rs.h's
** units made of them, as make.js makes them */
static void __fastcall__ kind_line (struct line* l)
{
    char** v = l->v;
    struct kind* k = &T->kind[nkinds];
    signed char mat = find (mats, v[5]), craft = find (crafts, v[6]);
    unsigned char flags = 0, keeps = 0, i;
    const char* c;
    int w;

    if (nkinds >= NTKINDS || l->n < 8 || mat < 0 || craft < 0) {
        err = "a kind: name, ways, weight, bulk, how far seen, material, craft, worth";
        return;
    }
    if ((w = atoi (v[4])) % SQ_FT) {
        err = "how far off it's made out: feet, a square's five each";
        return;
    }
    k->seen = (w > 15 * SQ_FT ? 15 * SQ_FT : w) / SQ_FT;
    put (v[0], (unsigned char*) k->name - R, 14);
    if (strcmp (v[1], "-")) {
        for (c = v[1]; *c; ++c) {
            for (i = 0; ways[i] && ways[i] != *c; ++i) {
            }
            if (!ways[i]) {
                err = "a kind's way";
                return;
            }
            if (i >= 6) {
                keeps |= wbits[i];
            } else {
                flags |= wbits[i];
            }
        }
    }
    if (l->n > 8 && *v[8] && !(keeps & 0x10)) {     /* (Room: a container's, keeping things whole and small) */
        flags |= 0x01;
        keeps |= 0x03;
    }
    k->flags = flags;
    k->weight = pounds (v[2], 255);
    w = atoi (v[3]);
    k->bulk = w > 255 ? 255 : w;
    w = l->n > 8 ? atoi (v[8]) : 0;
    k->cap_bulk = w > 255 ? 255 : w;
    k->cap_weight = l->n > 9 ? pounds (v[9], 0xFFFF) : 0;
    k->mat = mat;
    k->craft = craft;
    k->value = atoi (v[7]);
    k->keeps = keeps;
    ++nkinds;
}

static void __fastcall__ piece_line (struct line* l)
{
    char** v = l->v;
    struct tpiece* p = &T->piece[npieces];
    unsigned char k = kind (v[0]), e;

    if (npieces >= NPIECES || err || l->n < 2 || range (v[1], &p->d0, &p->d1, 0)) {
        err = "a piece: kind, strata (first-last) ...";
        return;
    }
    p->kind = k;
    p->hide = l->n < 3 || !strcmp (v[2], "-") ? 0xFF : atoi (v[2]) > 30 ? 30 : atoi (v[2]);   /* (Steps, 30 at most) */
    p->qty = l->n > 3 && *v[3] ? atoi (v[3]) : 1;
    e = l->n > 4 && *v[4] && strcmp (v[4], "-") ? atoi (v[4]) : 0;
    if (e > TERAS) {
        err = "a piece's era";
    }
    T->pera[npieces] = e;
    if (l->n > 5 && *v[5] && strcmp (v[5], "-")) {
        strncpy (pguard[npieces], v[5], 11);
    }
    ++npieces;
}

/* "X + Y = Z": the three kinds */
static void __fastcall__ combine (char* s, unsigned char* a, unsigned char* b, unsigned char* c)
{
    char* plus = strchr (s, '+');
    char* eq = plus ? strchr (plus, '=') : 0;
    char* e;

    if (!eq) {
        err = "combine X + Y = Z";
        return;
    }
    for (e = plus; e > s && e[-1] == ' '; --e) {
    }
    *e = 0;
    for (e = eq; e > plus + 1 && e[-1] == ' '; --e) {
    }
    *e = 0;
    for (++plus; *plus == ' '; ++plus) {
    }
    for (++eq; *eq == ' '; ++eq) {
    }
    *a = kind (s);
    *b = kind (plus);
    *c = kind (eq);
}

static void __fastcall__ rule_line (struct line* l)
{
    char** v = l->v;
    struct trule* r = &T->rule[nrules];
    signed char verb = find (verbs, v[0]), effect = 0;
    const char* w = 0;

    if (nrules >= NRULES || verb < 0 || l->n < 3) {
        err = "a rule: read, combine, put or out";
        return;
    }
    r->verb = verb;
    r->b = r->c = 0xFF;
    if (verb == TR_READ) {
        r->a = kind (v[1]);
        w = v[2];
    } else if (verb == TR_COMBINE) {
        combine (v[1], &r->a, &r->b, &r->c);
        w = v[2];
    } else if (verb == TR_PUT && l->n > 4) {
        r->a = kind (v[1]);
        r->b = kind (v[2]);
        effect = find (effects, v[3]);
        w = v[4];
    } else if (verb == TR_OUT && l->n > 3) {
        r->a = kind (v[1]);
        effect = find (effects, v[2]);
        w = v[3];
    }
    if (effect < 0 || !w || !*w) {
        err = "a rule's effect (- open rule) and its words";
        return;
    }
    r->effect = effect;
    r->words = text (w);
    ++nrules;
}

/* The history's lines (HISTORY.md) */
static void __fastcall__ era_line (struct line* l)
{
    char** v = l->v;
    struct tera* e = &T->era[T->neras];
    unsigned char p;
    signed char w = find (works, v[1]), end = find (endings, v[2]);

    for (p = 0; p < npeoples && strcmp (pname[p], v[0]); ++p) {
    }
    if (T->neras >= TERAS || l->n < 4 || p >= npeoples || w < 0 || end < 0 || atoi (v[3]) > 255) {
        err = "an era: a people of the theme's, what they made, how it ended, how long ago (centuries)";
        return;
    }
    e->people = p;
    e->tongue = ptongue[p];
    e->work = w;
    e->end = end;
    e->ago = atoi (v[3]);
    ++T->neras;
}

static void __fastcall__ layer_line (struct line* l)
{
    char** v = l->v;
    unsigned char sn = atoi (v[0]), era = l->n > 1 ? atoi (v[1]) : 0xFF;
    signed char d = l->n > 2 ? find (did, v[2]) : -1;
    struct tlayer* y;
    int w;

    if (sn >= TSTRATA || era > T->neras || d < 0 || l->n < 4) {
        err = "a layer: stratum, era, built ruined or reused, ...";
        return;
    }
    if (T->nlayers[sn] >= TLAYERS) {
        err = "more layers than a stratum has room for";
        return;
    }
    w = d == TL_RUINED ? find (endings, v[3]) : purpose_of (v[3]);
    if (w < 0) {
        err = "a layer: how it was ruined, or its purpose";
        return;
    }
    y = &T->layer[sn][T->nlayers[sn]++];
    y->era = era;
    y->did = d;
    y->work = w;
}

static void __fastcall__ faction_line (struct line* l)
{
    char** v = l->v;
    struct tfaction* f = &T->faction[T->nfactions];
    char* k;
    unsigned long m = 0;
    const char* at = "huwif";

    if (T->nfactions >= TFACTIONS || l->n < 7 || strlen (v[0]) > 11 || range (v[2], &f->d0, &f->d1, 1) ||
        !*v[6] || !strchr (at, *v[6])) {
        err = "a faction: name, kinds, strata, number, leader, lair, attitude, others";
        return;
    }
    for (k = strtok (v[1], " "); k; k = strtok (0, " ")) {    /* (Its kinds: the bestiary's keys) */
        if (beast_of (k) < 0) {
            err = "a faction's kind isn't the bestiary's";
            return;
        }
        m |= 1UL << (beast_of (k) + 1);
    }
    strcpy (f->name, v[0]);
    f->kinds = m;
    f->number = atoi (v[3]);
    f->leader = strcmp (v[4], "-") ? beast_of (v[4]) + 1 : 0;
    f->lair = atoi (v[5]);
    f->attitude = strchr (at, *v[6]) - at;
    ++T->nfactions;
}

/* A faction's number (1 ...) by its name, or its fault */
static unsigned char __fastcall__ faction (const char* s)
{
    unsigned char k;

    for (k = 0; k < T->nfactions; ++k) {
        if (!strcmp (T->faction[k].name, s)) {
            return k + 1;
        }
    }
    err = "no such faction of the theme's";
    return 0;
}

const char* __fastcall__ comp_line (struct line* l)
{
    const char* k = l->key;
    char** v = l->v;
    int x;                                      /* (A purpose: P_FORT its high bit, so not a signed char) */
    unsigned char i;
    char* c;
    unsigned long m;

    err = 0;
    if (!l->n) {
        return "no values";
    }
    if (!strcmp (k, "theme")) {
        put (v[0], (unsigned char*) T->name - R, sizeof T->name);
    } else if (!strcmp (k, "mission")) {
        R[1] |= !strcmp (v[0], "both") ? 3 : !strcmp (v[0], "rule") ? 2 : !strcmp (v[0], "escape") ? 1 : 0;
        if (!(R[1] & 3)) {
            err = "escape, rule or both";
        }
    } else if (!strcmp (k, "start")) {          /* (How the character came in: walked, or fell, TM_FELL: its bearings,
                                                **   slice 4.2's; make.js holds a stand-in's to the two words) */
        R[1] |= *v[0] == 'f' ? TM_FELL : 0;
    } else if (!strcmp (k, "story")) {
        T->story = text (v[0]);
    } else if (!strcmp (k, "stratum")) {
        if (nstrata >= TSTRATA || l->n < 2 || !*v[1] || strlen (v[1]) > 2 || strspn (v[1], "bnmx") != strlen (v[1])) {
            return "a stratum: a name, and one or two styles";
        }
        x = l->n > 2 && *v[2] ? purpose_of (v[2]) : P_HALLS;
        if (x < 0) {
            return "a stratum's purpose: halls, keep, temple, cellars, crypt or works, maybe fortified or open";
        }
        if (strlen (v[0]) >= 20) {
            return "a stratum's name too long";
        }
        strcpy (sname[nstrata], v[0]);
        strcpy (sstyle[nstrata], v[1]);
        spurp[nstrata++] = x;
    } else if (!strcmp (k, "rooms")) {
        if (l->n != 8) {
            return "eight words for rooms";
        }
        for (i = 0; i < 8; ++i) {
            put (v[i], (unsigned char*) T->room[i] - R, sizeof T->room[0]);
        }
    } else if (!strcmp (k, "uses")) {
        if (l->n != TUSES) {
            return "twelve words for the uses";
        }
        for (i = 0; i < TUSES; ++i) {
            put (v[i], (unsigned char*) T->uses[i] - R, sizeof T->uses[0]);
        }
    } else if (!strcmp (k, "light")) {
        put (v[0], (unsigned char*) T->light - R, sizeof T->light);
    } else if (!strcmp (k, "denizens")) {
        for (m = 0, c = strtok (v[0], " "); c; c = strtok (0, " ")) {
            if (!strcmp (c, "*")) {
                m = 0xFFFFFFFEUL;
            } else if (beast_of (c) < 0) {
                return "no such denizen in dq0d/kinds.txt";
            } else {
                m |= 1UL << (beast_of (c) + 1);
            }
        }
        T->dens = m;
        dens_set = 1;
    } else if (!strcmp (k, "kind")) {
        kind_line (l);
    } else if (!strcmp (k, "piece")) {
        piece_line (l);
    } else if (!strcmp (k, "rule")) {
        rule_line (l);
    } else if (!strcmp (k, "lock")) {
        T->lock = text (v[0]);
    } else if (!strcmp (k, "opened")) {
        T->opened = text (v[0]);
    } else if (!strcmp (k, "ending")) {
        if ((x = find (ends, v[0])) < 1 || l->n < 2) {
            return "an ending: escaped, ruled or died";
        }
        T->end[x] = text (v[1]);
    } else if (!strcmp (k, "seed") || !strcmp (k, "region")) {
        /* (The history's, the file's to read: nothing in the record) */
    } else if (!strcmp (k, "people")) {
        if (npeoples >= 8 || l->n < 4 || atoi (v[2]) < 1 || atoi (v[2]) > 8 || strlen (v[0]) > 15) {
            return "a people: name, what, tongue (1-8), builds";
        }
        strcpy (pname[npeoples], v[0]);
        ptongue[npeoples++] = atoi (v[2]);
    } else if (!strcmp (k, "era")) {
        era_line (l);
    } else if (!strcmp (k, "layer")) {
        layer_line (l);
    } else if (!strcmp (k, "faction")) {
        faction_line (l);
    } else if (!strcmp (k, "goal")) {
        if ((x = find (goals, v[0])) < 1 || l->n < 3 || !*v[1] || atoi (v[2]) > 255) {
            return "a goal: artifact, evil, prisoner or through; its name; its stratum; its guardians";
        }
        T->goal = x;
        T->goal_sn = atoi (v[2]);
        strncpy (gguard, l->n > 3 && strcmp (v[3], "-") ? v[3] : "", 11);
    } else {
        return "no such key";
    }
    return err;
}

/* The record made whole: the strata (past the last, the last again), their purposes (their layers' last built or
** reused, if they have any), their ages (their oldest layer's era's), the uses' words, the counts, the guardians */
const char* comp_end (void)
{
    unsigned char k, i, s;
    struct tlayer* y;

    err = 0;
    if (!nstrata || !R[4] || !(R[1] & 3)) {
        return "no name, mission or strata";
    }
    for (k = 0; k < TSTRATA; ++k) {
        s = k < nstrata ? k : nstrata - 1;
        memcpy (T->stratum[k], sname[s], strlen (sname[s]));
        T->style[k][0] = strchr (styles, sstyle[s][0]) - styles;
        T->style[k][1] = strchr (styles, sstyle[s][strlen (sstyle[s]) - 1]) - styles;
        T->purpose[k] = spurp[s];
        for (i = 0, y = T->layer[k]; i < T->nlayers[k]; ++i, ++y) {
            if (y->did != TL_RUINED) {
                T->purpose[k] = y->work;
            }
        }
        for (i = 0, y = T->layer[k]; i < T->nlayers[k] && !y->era; ++i, ++y) {
        }
        if (i < T->nlayers[k]) {
            T->sage[k] = T->era[y->era - 1].ago >= 255 ? 255 : T->era[y->era - 1].ago + 1;
        }
    }
    for (k = 0; k < TUSES; ++k) {
        if (!T->uses[k][0]) {                   /* (By the struct's fields: make.js's numbers move with it) */
            put (uses[k], (unsigned char*) T->uses[k] - R, sizeof T->uses[0]);
        }
    }
    T->npieces = npieces;
    T->nrules = nrules;
    if (T->goal && *gguard) {
        T->goal_guard = faction (gguard);
    }
    for (k = 0; k < npieces; ++k) {
        if (*pguard[k]) {
            T->pguard[k] = faction (pguard[k]);
        }
    }
    if (!dens_set) {
        for (k = 0; k < T->nfactions; ++k) {
            T->dens |= T->faction[k].kinds;
        }
    }
    return err;
}

unsigned comp_used (void)
{
    return words;
}

/* ---- The theme checker */

/* A kind of the bestiary's lives in stratum sn (its strata, and one of the stratum's styles its: sbits, the strata's
** styles a bit each, made first: cc65's optimizer lost a high byte in T->style[s][0] here, the index of a byte
** array of arrays after a struct's field, and the plain index keeps clear of it) */
static unsigned char sbits[TSTRATA];
static unsigned char __fastcall__ lives (unsigned char b, unsigned char sn)
{
    const struct beast* p = &beasts[b];

    if (b >= nbeasts || sn < p->d0 || sn > p->d1) {
        return 0;
    }
    return (p->styles & sbits[sn < TSTRATA ? sn : TSTRATA - 1]) != 0;
}

static unsigned char __fastcall__ fixed (unsigned char k)
{
    return k < NTKINDS && (T->kind[k].keeps & 0x80);
}

static unsigned char have[NTKINDS];

static unsigned char __fastcall__ can (const struct trule* r)
{
    switch (r->verb) {
    case TR_READ:
        return have[r->a];
    case TR_COMBINE:
        return have[r->a] && have[r->b] && !fixed (r->a) && !fixed (r->b);
    case TR_PUT:
        return have[r->a] && have[r->b] && !fixed (r->a);
    }
    return have[r->a] && !fixed (r->a);
}

static unsigned char faults;
static void __fastcall__ fault (const char* what, const char* s)
{
    say ("the theme checker: ");
    if (what) {
        say (what);
        say (": ");
    }
    say (s);
    say ("\n");
    ++faults;
}

unsigned char comp_check (void)
{
    const struct trule* r;
    const struct tfaction* f;
    unsigned char k, i, sn, more, ends = 0, through = T->goal == G_THROUGH, any, lair;

    faults = 0;
    for (k = 0; k < TSTRATA; ++k) {
        sbits[k] = 1 << T->style[k][0];
        sbits[k] |= 1 << T->style[k][1];
    }
    memset (have, 0, sizeof have);
    for (k = 0; k < T->npieces; ++k) {
        if (T->piece[k].qty && T->piece[k].kind < NTKINDS) {
            have[T->piece[k].kind] = 1;
        }
    }
    do {                                        /* (What the rules make from what's to be had) */
        more = 0;
        for (k = 0, r = T->rule; k < T->nrules; ++k, ++r) {
            if (r->verb == TR_COMBINE && can (r) && !have[r->c]) {
                have[r->c] = more = 1;
            }
        }
    } while (more);
    for (k = 0, r = T->rule; k < T->nrules; ++k, ++r) {
        if (can (r)) {
            ends |= 1 << r->effect;
        } else {
            fault (T->kind[r->a].name, "its rule can't be done: its things aren't all to be had");
        }
    }
    if ((T->mission & TM_ESCAPE) && !(ends & 1 << TE_OPEN) && !through) {
        fault (0, "the way out can't be opened: no rule that opens it has its things to be had");
    }
    if ((T->mission & TM_RULE) && !(ends & 1 << TE_RULE)) {
        fault (0, "the place can't be ruled: no rule that rules it has its things to be had");
    }
    for (k = 0, f = T->faction; k < T->nfactions; ++k, ++f) {
        for (any = lair = 0, sn = f->d0; sn <= f->d1 && sn < 64; ++sn) {
            for (i = 0; i < nbeasts; ++i) {
                if ((f->kinds & 1UL << (i + 1)) && lives (i, sn)) {
                    any = 1;
                    lair |= sn == f->lair;
                }
            }
        }
        if (!any) {
            fault (f->name, "none of its kinds lives in its strata");
        } else if (!lair) {
            fault (f->name, "its lair isn't where its kinds live");
        }
        if (f->leader && !lives (f->leader - 1, f->lair)) {
            fault (f->name, "its leader doesn't live in its lair's stratum");
        }
    }
    for (k = 0; k < T->npieces; ++k) {
        if (T->pguard[k]) {
            f = &T->faction[T->pguard[k] - 1];
            if (f->d1 < T->piece[k].d0 || f->d0 > T->piece[k].d1) {
                fault (T->kind[T->piece[k].kind].name, "its guardians don't live where it lies");
            }
        }
        if (T->pera[k] > T->neras) {
            fault (T->kind[T->piece[k].kind].name, "no such era");
        }
    }
    if (T->goal_guard) {
        f = &T->faction[T->goal_guard - 1];
        if (f->d1 < T->goal_sn || f->d0 > T->goal_sn) {
            fault ("the goal", "its guardians don't live where it is");
        }
    }
    return faults;
}
