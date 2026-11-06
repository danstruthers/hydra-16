/*
** linegen.c - (a bank program, themegen's: sdk/c/include/bankprog.h) the theme's lines, from the history histgen and
** goalgen made in themegen's main RAM, emitted (themegen's emit: written to theme.txt and said) in the theme format
** (games/dq0/themes/1crypt.txt's), a group at a time, a hand-written theme's groups as it gives them: the seed; the
** theme's name, mission and story (the place, each era's words, who has it now, the goal's words, why the character
** came), why it came and what solving brings (the plan's step 7: goals.txt's why lines); the region; the
** peoples and the eras; the strata and their layers; the rooms' words, the uses' and the light (the people's who
** have the top now); the inhabitants; the goal, its things, its lines (goals.txt's, their slots filled), and a clue
** each era leaves among its works (history.txt's: a thing to read, its words the era's), while the record has room.
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#define BANKPROG
#include "../../games/themegen/themegen.h"

/* An era's words: what its people made (or made their own), and how it ended (ago: and how long ago), into out (200
** bytes), its first letter small */
static void __fastcall__ era_words (unsigned char e, char* out, unsigned char ago)
{
    static char c[8], t[260];
    struct era* r = &er[e];

    if (r->own) {
        if (out != r->words) {
            strcpy (out, r->words);
        }
        return;
    }
    SL ('P') = r->pthe;
    SL ('K') = r->kind;
    SL ('N') = r->great;
    SL ('X') = r->place;
    SL ('R') = rname;
    SL ('O') = e + 1 < ne ? er[e + 1].pthe : "enemies";
    SL ('W') = "";
    if (r->took != 0xFF) {                      /* (It made an older work its own: that work's word) */
        data (TK_NOUN, works[r->took], 0);
        strcpy (c, D.v[1]);
        SL ('W') = c;
        data (TK_REUSED, works[r->work], 0);
    } else {
        data (TK_BUILT, works[r->work], 0);
    }
    fill (t, D.v[1], sizeof t);
    data (TK_ENDED, endings[r->end], 0);
    strcat (t, ", ");
    fill (t + strlen (t), D.v[1], sizeof t - strlen (t));
    if (ago && r->end != EN_HELD) {
        data (TK_AGO, 0, 0);
        strcat (t, ", ");
        SL ('C') = num (r->ago);
        fill (t + strlen (t), D.v[0], sizeof t - strlen (t));
    }
    if (t[0] == 'T' && !strncmp (t + 1, "he ", 3)) {
        t[0] = 't';                             /* (Its first letter a capital only where a sentence begins) */
    }
    strncpy (out, t, 199);
    out[199] = 0;
}

/* The goal's slots, for its lines (goals.txt's): as goalgen made them (gsl, themegen's: the bank's room), put back after an era's
** words have had them */
static void goal_slots (void)
{
    memcpy (sl, gsl, sizeof sl);
}

/* A data line (the goal's, a clue's) emitted as a theme's: its key, its values from from on, their slots filled */
static void __fastcall__ theme_line (const char* key, unsigned char from)
{
    unsigned char k;

    begin_line (key);
    for (k = from; k < D.n; ++k) {
        fill (W, D.v[k], sizeof W);
        val (W);
    }
    end_line ();
}

/* The goal block's lines (after its goal line, before the next) */
static int __fastcall__ block_end (void)
{
    int end = tg_find (TK_GOAL, 0, gk + 1);

    return end < 0 ? tg_lines : end;
}

/* ---- Why the character came (the plan's step 7: goalgen's pick, themegen's gwl, gQ, gD, gpence) */

/* The why line (its goal kind, its reason, its patron: what a rumour gives, and the account reads) */
static void why_line (void)
{
    data (TK_GOAL, 0, gk);
    begin_line ("why");
    val (D.v[0]);
    tg_get (gwl, &D, Dt);
    val (D.v[0]);
    val (gQ);
    end_line ();
}

/* ---- The groups */

static char w[240];                             /* (A line's words: the head's, the history's, the goal's, one at a
                                                **   time; one for them all, the bank's room) */

/* A hand-written theme's line of the key, if it gives one: written as it is (the head's lines one by one: a name
** given, its story made) */
static const char* hkey;
static unsigned char hhad;
static void __fastcall__ emit_key (struct line* l)
{
    if (!strcmp (l->key, hkey)) {
        emit (l);
        hhad = 1;
    }
}

static unsigned char __fastcall__ given_key (const char* key)
{
    hhad = 0;
    if (given[GR_HEAD]) {
        hkey = key;
        given_each (GR_HEAD, emit_key);
    }
    return hhad;
}

static void story_line (void);

/* The head's lines: its name, its mission, how it came in, its story (a hand-written theme's, line by line, else made), why it
** came and what solving brings (a goal's) */
static void head (void)
{
    if (!given_key ("theme")) {
        begin_line ("theme");
        val (title);
        end_line ();
    }
    if (!given_key ("mission")) {
        begin_line ("mission");
        val (gmission);
        end_line ();
    }
    given_key ("start");                        /* (How the character came in, if it's given: walked
                                                **   in if not, the compiler's; slice 4.2's bearings) */
    if (!given_key ("story")) {
        story_line ();
    }
    if (gk == 0xFF || gwl < 0) {
        return;                                 /* (No goal kind's why: goalgen's) */
    }
    why_line ();                                /* (Why it came, what solving brings: the account's to read) */
    if (!given_key ("reward")) {
        goal_slots ();
        tg_get (gwl, &D, Dt);
        begin_line ("reward");
        valn (gpence);
        val (D.v[4]);
        val (D.v[5]);
        fill (W, D.v[6], LINE_TEXT);
        val (W);
        end_line ();
    }
}

/* The story made: the place, its eras, who has it now, the goal, why the character came (%Y) */
static void story_line (void)
{
    char* story = W;                            /* (W: its own words its eras', made apart) */
    unsigned char e, k;
    int j, end;

    SL ('R') = rname;
    data (TK_OPEN, 0, 0);
    fill (story, D.v[0], LINE_TEXT);
    for (e = 0; e < ne; ++e) {
        era_words (e, w, 1);
        strcat (story, " ");
        k = strlen (story);
        strncat (story, w, LINE_TEXT - k - 3);
        if (story[k] >= 'a' && story[k] <= 'z') {
            story[k] -= 32;
        }
        strcat (story, ".");
    }
    for (k = 0; k < nf && !among (fa[k].name, "dwarves elves soldiers"); ++k) {
    }                                           /* (A people that holds it still first, 2026-10-29; else the dead, the
                                                **   gargun, the kobolds: the first of them) */
    for (k = k < nf ? k : 0; k < nf && !among (fa[k].name, "dwarves elves soldiers dead kobolds gargun"); ++k) {
    }
    if (data (TK_NOW, k < nf ? fa[k].name : nf ? fa[nf - 1].name : "vermin", 0)) {
        strcat (story, " ");
        strcat (story, D.v[1]);
    }
    if (gk != 0xFF) {
        goal_slots ();
        *w = 0;
        if (gwl >= 0) {                         /* (Why it came: its why line's words, its slots filled) */
            tg_get (gwl, &D, Dt);
            fill (w, D.v[2], sizeof w);
        }
        SL ('Y') = w;
        for (j = goal_line + 1, end = block_end (); j < end; ++j) {
            if (tg_key[j] == TK_THEME + 3) {    /* (Its story's words) */
                tg_get (j, &D, Dt);
                strcat (story, " ");
                fill (story + strlen (story), D.v[0], LINE_TEXT - strlen (story));
            }
        }
    }
    begin_line ("story");
    val (story);
    end_line ();
}

/* The region's mints (a region given without any, a rumour's: made all the same) */
static void mint_lines (void)
{
    static char nm[16], adj[16];
    unsigned char e, n;
    struct era* r;

    for (e = 0; e < 4; ++e) {                   /* (Its mints, 2026-10-28, the user's "each coin should include its
                                                **   provenance": today's, the region's own realm, two neighbours (one
                                                **   in two times hard to pass), the dwarves' hold, gold and taken at
                                                **   par, as Azadmere's are; named in the men's tongue and the
                                                **   dwarves', from the seed: none of the history's dice spent; each
                                                **   its word, its name and an, or n after a vowel) */
        strcpy (nm, name_of (e == 3 ? 1 : 2, NG_PLACE, seed ^ (0x6D696E74UL + e * 0x9E37UL)));
        strcpy (adj, nm);
        n = strlen (adj);
        if (n < 14) {
            strcat (adj, strchr ("aeiou", adj[n - 1]) ? "n" : "an");
        }
        begin_line ("mint");
        val (nm);
        val (adj);
        val ("now");
        val (e == 0 ? "l" : e == 3 ? "gp" : e == 2 && (seed & 4) ? "h" : "-");
        end_line ();
    }
    for (e = 0, r = er; e < ne; ++e, ++r) {     /* (Of old: each era's people who struck coin, the kobolds and the
                                                **   hill-men none, the dwarves' gold too; their word their name) */
        if (strcmp (r->kind, "kobolds") && strcmp (r->kind, "hill-men")) {
            begin_line ("mint");
            val (r->pthe);
            val (r->people);
            valn (e + 1);
            val (strcmp (r->kind, "dwarves") ? "-" : "g");
            end_line ();
        }
    }
}

/* The region and its mints; a hand-written theme's as it gives them, its mints made if it gives none */
static unsigned char minted;
static void __fastcall__ emit_region (struct line* l)
{
    minted |= !strcmp (l->key, "mint");
    emit (l);
}

static void region_lines (void)
{
    if (given[GR_REGION]) {
        minted = 0;
        given_each (GR_REGION, emit_region);
        if (minted) {
            return;
        }
    } else {
        region_line ();
    }
    mint_lines ();
}

static void hist_lines (void)
{
    unsigned char e, k;
    struct era* r;

    for (e = 0, r = er; e < ne; ++e, ++r) {     /* (Each people once) */
        for (k = 0; k < e && strcmp (er[k].people, r->people); ++k) {
        }
        if (k < e) {
            continue;
        }
        data (TK_PEOPLE, 0, r->what);
        begin_line ("people");
        val (r->people);
        val (r->kind);
        valn (r->tongue);
        strcpy (w, D.v[2]);
        val (w);
        strcpy (w, D.v[5]);                     /* (Its tongue's word and its script: the plan for the game between */
        val (w);                                /*   dungeons, its step 8) */
        strcpy (w, D.v[6]);
        val (w);
        end_line ();
    }
    for (e = 0, r = er; e < ne; ++e, ++r) {
        era_words (e, w, 1);
        if (w[0] >= 'a' && w[0] <= 'z') {
            w[0] -= 32;
        }
        strcat (w, ".");
        begin_line ("era");
        val (r->people);
        val (works[r->work]);
        val (endings[r->end]);
        valn (r->ago);
        val (r->end == EN_CURSED ? "the dead" : r->end == EN_CONQUERED ? "bones" : r->end == EN_HELD ? "its people" :
            r->end == EN_ABANDONED ? "stores" : "nothing");
        val (w);
        end_line ();
    }
}

static void strata_lines (void)
{
    unsigned char k, i;
    struct tlayer* y;
    struct str* s;

    for (k = 0, s = st; k < ns; ++k, ++s) {
        begin_line ("stratum");
        val (s->name);
        val (s->styles);
        if (s->work != WK_DEEP) {
            val (purpose_words (s->purpose));
        }
        end_line ();
    }
    for (k = 0, s = st; k < ns; ++k, ++s) {
        for (i = 0, y = s->l; i < s->nl; ++i, ++y) {
            begin_line ("layer");
            valn (k);
            valn (y->era);
            val (dids[y->did]);
            val (y->did == TL_RUINED ? endings[y->work] : purpose_words (y->work));
            end_line ();
        }
    }
}

/* The rooms' words, the uses' and the light: the people's who have the top now */
static void __fastcall__ words_of (unsigned char key, const char* who)
{
    unsigned char k;

    if (data (key, who, 0) || (key != TK_USES && data (key, "default", 0))) {
        begin_line (key == TK_ROOMS ? "rooms" : key == TK_USES ? "uses" : "light");
        for (k = 1; k < D.n; ++k) {
            val (D.v[k]);
        }
        end_line ();
    }
}

static void words_lines (void)
{
    unsigned char e = era_now (0);
    const char* who = e ? er[e - 1].kind : "default";

    words_of (TK_ROOMS, who);
    words_of (TK_USES, who);
    words_of (TK_LIGHT, who);
}

/* A faction's others ("NAME h, NAME w ...", the bestiary's), those of the theme's alone: popgen's (dq0d/pop.h), or - */
static const char* __fastcall__ others_of (const struct fac* f)
{
    static char o[sizeof f->others];
    const char* p = f->others;
    const char* e;
    unsigned char k, n, i;

    for (i = 0; *p && *p != '-'; ) {
        for (e = p; *e && *e != ','; ++e) {
        }
        for (n = 0; p + n < e && p[n] != ' '; ++n) {
        }
        for (k = 0; k < nf && (strlen (fa[k].name) != n || strncmp (fa[k].name, p, n)); ++k) {
        }
        if (k < nf) {
            if (i) {
                o[i++] = ',';
                o[i++] = ' ';
            }
            while (p < e) {
                o[i++] = *p++;
            }
        }
        for (p = e; *p == ',' || *p == ' '; ++p) {
        }
    }
    o[i] = 0;
    return i ? o : "-";
}

static void dens_lines (void)
{
    static char r[8];
    unsigned char k;
    struct fac* f;

    for (k = 0, f = fa; k < nf; ++k, ++f) {
        begin_line ("faction");
        val (f->name);
        val (f->kinds);
        strcpy (r, num (f->d0));
        strcat (r, "-");
        strcat (r, num (f->d1));
        val (r);
        valn (f->n);
        val (f->leader ? beasts[f->leader - 1].key : "-");
        valn (f->lair);
        val (f->att);
        val (others_of (f));
        end_line ();
    }
}

/* The goal (its line, its things' kinds, its lines), and a clue each era leaves (its kind, where it lies, how it
** reads) while there's room for it */
static void goal_lines (void)
{
    static const char* const keys[TK_THEMES] = { "kind", "piece", "rule", "story", "lock", "opened", "ending", "wayin", "wayout" };
    static char used[NTKINDS][14];
    unsigned char nk = 0, nr = 0, e, k, i, cand[TSTRATA], nc;
    int j, end;
    struct str* s;

    if (gk == 0xFF) {
        return;
    }
    goal_slots ();
    for (j = goal_line + 1, end = block_end (); j < end && tg_key[j] != TK_CALLED; ++j) {
    }
    *w = 0;
    if (j < end) {                              /* (Its name: its kind's called line's) */
        tg_get (j, &D, Dt);
        fill (w, D.v[0], sizeof w);
    }
    data (TK_GOAL, 0, gk);
    begin_line ("goal");
    val (D.v[0]);
    val (*w ? w : gA);
    valn (gs);
    val (gg != 0xFF ? fa[gg].name : "-");
    end_line ();
    for (k = 0; k < 2; ++k) {                   /* (Its things' kinds) */
        if ((j = k ? gb_line : ga_line) >= 0) {
            tg_get (j, &D, Dt);
            theme_line ("kind", 2);
            strcpy (used[nk++], D.v[2]);
        }
    }
    for (j = goal_line + 1, end = block_end (); j < end; ++j) {
        k = tg_key[j] - TK_THEME;
        if (k < TK_THEMES && k != 3) {          /* (Its story the theme's story's) */
            tg_get (j, &D, Dt);
            if (!k) {
                strcpy (used[nk++], D.v[0]);
            }
            theme_line (keys[k], 0);
            nr += k == 2;
        }
    }
    for (e = 0; e < ne && nk < NTKINDS && nr < NRULES; ++e) {   /* (The clues: an era's, among its works) */
        if (!data (TK_CLUE, works[er[e].work], 0)) {
            continue;
        }
        for (k = 0; k < nk && strcmp (used[k], D.v[1]); ++k) {
        }
        if (k < nk) {
            continue;                           /* (A kind it has already) */
        }
        for (nc = i = 0, s = st; i < ns; ++i, ++s) {
            for (k = 0; k < s->nl; ++k) {
                if (s->l[k].era == e + 1 && s->l[k].did != TL_RUINED) {
                    cand[nc++] = i;
                    break;
                }
            }
        }
        if (!nc || THEME_HEAD + wused + strlen (D.v[D.n - 1]) + strlen (er[e].words) + 64 > THEME_SIZE) {
            continue;                           /* (Nowhere it lies, or no room for its words) */
        }
        strcpy (used[nk++], D.v[1]);
        begin_line ("kind");                    /* (Its kind: the clue line's values, but its words) */
        for (k = 1; k + 1 < D.n; ++k) {
            val (D.v[k]);
        }
        end_line ();
        k = cand[random (nc)];
        begin_line ("piece");
        val (used[nk - 1]);
        strcpy (w, num (k));
        strcat (w, "-");
        strcat (w, num (k));
        val (w);
        val (random (3) ? "-" : num (2 + 2 * random (3)));   /* (In plain view, or concealed: steps) */
        val ("1");
        valn (e + 1);
        end_line ();
        SL ('P') = er[e].pthe;
        SL ('W') = er[e].words;
        data (TK_CLUE, works[er[e].work], 0);
        fill (w, D.v[D.n - 1], sizeof w);
        begin_line ("rule");
        val ("read");
        val (used[nk - 1]);
        val (w);
        end_line ();
        ++nr;
    }
}

/* ---- The lines, a group at a time (a hand-written theme's groups as it gives them) */

static void __fastcall__ group (unsigned char g, void (*make) (void))
{
    if (given[g]) {
        given_each (g, emit);
    } else {
        make ();
    }
}

void make (void)
{
    static char sb[12];
    unsigned char e;

    for (e = 0; e < ne; ++e) {                  /* (Each era's words, for its clue) */
        era_words (e, er[e].words, 0);
    }
    begin_line ("seed");
    val (ultoa (seed, sb, 10));
    end_line ();
    head ();                                    /* (Its lines one by one: a hand-written theme's name, say, and the
                                                **   story made) */
    region_lines ();                            /* (Its mints made if a region given has none: a rumour's) */
    group (GR_HIST, hist_lines);
    group (GR_STRATA, strata_lines);
    group (GR_WORDS, words_lines);
    group (GR_DENS, dens_lines);
    group (GR_GOAL, goal_lines);
}
