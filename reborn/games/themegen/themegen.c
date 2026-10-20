/*
** themegen.c - Dingus Quest Zero's theme generator (Phase 3D; the user's, DesignIdeas.md: "there needs to be a
** program who's sole purpose is to generate the history and theme for DQ0 to use to generate its dungeon"): a
** region's history made from a seed, and the dungeon's theme from it, written as a theme (the format:
** ../dq0/themes/1crypt.txt's): the region, its peoples and eras, the strata each era left as layers, the inhabitants
** now, the goal and its clues, the names and the story.  games/dq0/HISTORY.md has the design.
**
** themegen [-s seed] [-t theme] [-o folder] [-l lib] [-q]
**   -s seed    the same history each time (default: the clock's)
**   -t theme   a theme written by hand: what it gives kept, the rest made to fit it (by groups: the theme's name and
**              story; the region; the peoples and the eras; the strata and their layers; the rooms' words; the
**              inhabitants; the goal and its puzzle)
**   -o folder  theme.txt (the theme, to read) written there, then themec run in themegen's place (Plan 9's exec)
**              to compile it there into theme.dat (the record the game reads) and check it (the theme checker:
**              every goal reachable, every faction placed)
**   -l lib     the game's lib folder (themegen.dat, the bank programs and themegen's exports): default, its own
**              folder's lib/dq0
**   -q         the theme not said (without -o it's said, to read)
** Its exit status 0, or 1 (said why); with -o, themec's.  The UI runs it as a new game begins (theme.h): the game's
** seed, its save folder (or /ram/dq0).
**
** The work is its bank programs' (sdk/c/include/bankprog.h: each called as a function from a RAM bank of themegen's
** own, working on the history here in themegen's main RAM): histgen (the region, the eras, the strata, the
** inhabitants), goalgen (the goal), linegen (the theme's lines from them all, a hand-written theme's groups as they
** are), and namegen (names).  What they call of themegen's is here (themegen.h): the data's lines (data.c), the
** words' slots filled, the lines emitted.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <hydra.h>
#include <bankprog.h>
#include "themegen.h"

static const char* tfile;
static const char* outdir;
static char lib[48];
static unsigned char quiet, bad;
static int ofd = -1;

/* ---- The history, as it's made (the bank programs') */

const char* const works[] = { "keep", "temple", "mine", "tomb", "town", "hold", "deep", 0 };
const char* const endings[] = { "abandoned", "conquered", "collapsed", "flooded", "cursed", "held", 0 };
const char* const dids[] = { "built", "ruined", "reused", 0 };
char land[12], rname[28], noun[12], rock[12], water[12], climate[12];
unsigned char ne, ns, nf;
struct era er[TERAS];
struct str st[TSTRATA];
struct fac fa[TFACTIONS];
unsigned char gk, gs, ge, gg, gu, geu;
char gA[40], gB[40], gM[12], ga[16], gb[16], title[32], gmission[8];
int ga_line, gb_line, goal_line;
unsigned long seed;
struct era* const erp = er;
struct str* const stp = st;
struct fac* const fap = fa;
struct beast* const beastp = beasts;
struct line* const Dp = &D;

unsigned char __fastcall__ era_now (unsigned char k)
{
    struct str* s = &st[k];
    unsigned char i, e = 0;

    for (i = 0; i < s->nl; ++i) {
        if (s->l[i].did != TL_RUINED) {
            e = s->l[i].era;
        }
    }
    return e;
}

unsigned char __fastcall__ lives (unsigned char b, unsigned char k)
{
    struct beast* p = &beasts[b];

    return k >= p->d0 && k <= p->d1 && (p->styles & style_bits (st[k < ns ? k : ns - 1].styles));
}

int __fastcall__ work_of (const char* s)
{
    unsigned char k;

    for (k = 0; works[k]; ++k) {
        if (!strcmp (works[k], s)) {
            return k;
        }
    }
    return -1;
}

static const char* const purposes[] = { "halls", "keep", "temple", "cellars", "crypt", "works" };

const char* __fastcall__ purpose_words (unsigned char p)
{
    static char w[20];

    strcpy (w, purposes[p & 7]);
    if ((p & P_FORT) && (p & 7) != P_KEEP) {
        strcat (w, " fortified");
    } else if (!(p & P_FORT) && (p & 7) == P_KEEP) {
        strcat (w, " open");
    }
    return w;
}

/* ---- Words */

const char* sl[58];
char W[LINE_TEXT];

void __fastcall__ fill (char* out, const char* tpl, unsigned max)
{
    unsigned n = 0, c;
    const char* s;
    unsigned char cap, prose = strchr (tpl, ' ') != 0;

    for (; *tpl && n + 1 < max; ++tpl) {
        if (*tpl != '%' || tpl[1] < 'A' || tpl[1] > 'z' || !(s = SL (tpl[1]))) {
            out[n++] = *tpl;
            continue;
        }
        ++tpl;
        for (c = n; c && out[c - 1] == ' '; --c) {  /* (A sentence's first words: nothing before, or a . ! ? or a
                                                **   quote, and spaces: its first letter a capital) */
        }
        cap = c ? strchr (".!?\"", out[c - 1]) != 0 : prose;  /* (A name alone: as it is) */
        for (; *s && n + 1 < max; ++s) {
            out[n++] = cap && *s >= 'a' && *s <= 'z' ? *s - 32 : *s;
            cap = 0;
        }
    }
    out[n] = 0;
}

char* __fastcall__ pick (const char* s, char* out)
{
    unsigned char n = 1, k;
    const char* p;
    char* o = out;

    for (p = s; *p; ++p) {
        n += *p == ' ';
    }
    for (k = random (n), p = s; k; ++p) {
        k -= *p == ' ';
    }
    while (*p && *p != ' ') {
        *o++ = *p++;
    }
    *o = 0;
    return out;
}

unsigned char __fastcall__ among (const char* s, const char* list)
{
    unsigned char n = strlen (s);
    const char* p;

    for (p = list; (p = strstr (p, s)) != 0; ++p) {
        if ((p == list || p[-1] == ' ') && (!p[n] || p[n] == ' ')) {
            return 1;
        }
    }
    return 0;
}

/* ---- namegen, the bank program */

static struct bank_fn* namer;
static struct nq {
    unsigned long   seed;
    unsigned char   tongue, form;
    char            out[16];
} nq;

const char* __fastcall__ name_of (unsigned char tongue, unsigned char form, unsigned long seed)
{
    nq.seed = seed;
    nq.tongue = tongue;
    nq.form = form;
    if (!namer || bank_call (namer, unsigned char __fastcall__ (*)(struct nq*), &nq) == 0xFFFF) {
        strcpy (nq.out, "Nameless");            /* (No namegen: a name all the same) */
    }
    return nq.out;
}

/* ---- A hand-written theme: the groups its keys are in (what it gives, the history doesn't make), its lines */

unsigned char given[GROUPS];
static const char* const groups[GROUPS] = {
    " theme mission story ", " region seed ", " people era ", " stratum layer ", " rooms uses light ",
    " denizens faction ", " goal kind piece rule lock opened ending "
};

static unsigned char __fastcall__ group_of (const char* key)
{
    static char k[12];
    unsigned char g;

    k[0] = ' ';
    strncpy (k + 1, key, 9);
    k[10] = 0;
    strcat (k, " ");
    for (g = 0; g < GROUPS && !strstr (groups[g], k); ++g) {
    }
    return g;
}

void __fastcall__ given_each (unsigned char g, void __fastcall__ (*fn) (struct line*))
{
    static struct line l;
    char* t = W;                                /* (Its lines' text: W, which nothing they're given to uses) */
    unsigned char k;

    if (!tfile || lines_open (tfile)) {
        return;
    }
    while (read_line (&l, t)) {
        k = group_of (l.key);
        if (g == GROUPS) {                      /* (Read through: the groups it gives noted) */
            if (k < GROUPS) {
                given[k] = 1;
            } else {
                say ("themegen: ");
                say (tfile);
                say (": no key ");
                say (l.key);
                say ("\n");
                bad = 1;
            }
        } else if (k == g && strcmp (l.key, "seed")) {
            fn (&l);
        }
    }
    lines_close ();
}

/* ---- The lines out: written to theme.txt and said as each fills (OUTW columns, then a line more: 11 spaces, and a
** value that begins there its bar) */

#define OUTW            116
static char ob[256];
static unsigned char on, fresh;
unsigned wused;

static void oflush (void)
{
    ob[on++] = '\n';
    if (ofd >= 0) {
        write (ofd, ob, on);
    }
    if (!quiet) {
        write (1, ob, on);
    }
    on = 0;
    fresh = 1;
}

/* A word added (sep: it begins a value, its bar before it) */
static void __fastcall__ oword (const char* w, unsigned char n, unsigned char sep)
{
    if (on + 1 + (sep ? 2 : 0) + n > OUTW && on > 11) {
        oflush ();
        memset (ob, ' ', 11);
        on = 11;
    } else if (!fresh) {
        ob[on++] = ' ';
    }
    fresh = 0;
    if (sep) {
        ob[on++] = '|';
        ob[on++] = ' ';
    }
    if (n > sizeof ob - 2 - on) {
        n = sizeof ob - 2 - on;
    }
    memcpy (ob + on, w, n);
    on += n;
}

void __fastcall__ emit (struct line* l)
{
    unsigned char k, first;
    const char* w;
    const char* e;

    on = strlen (l->key);
    memcpy (ob, l->key, on);
    while (on < 9) {
        ob[on++] = ' ';
    }
    fresh = 1;
    for (k = 0; k < l->n; ++k) {                /* (Each value's words, the bar before its first) */
        w = l->v[k];
        first = 1;
        do {
            for (e = w; *e && *e != ' '; ++e) {
            }
            oword (w, e - w, first);
            first = 0;
            w = e + 1;
        } while (*e);
    }
    oflush ();
    if (l->n && (!strcmp (l->key, "story") || !strcmp (l->key, "lock") || !strcmp (l->key, "opened") ||
        !strcmp (l->key, "rule") || !strcmp (l->key, "ending"))) {
        wused += strlen (l->v[l->n - 1]) + 1;   /* (Its words, in the record's room for them) */
    }
}

static struct line O;
static char Ot[LINE_TEXT];
static unsigned ot;

void __fastcall__ begin_line (const char* key)
{
    strcpy (O.key, key);
    O.n = 0;
    ot = 0;
}

void __fastcall__ val (const char* s)
{
    unsigned n = strlen (s);

    if (O.n < MAXV && ot + n + 1 < LINE_TEXT) {
        O.v[O.n++] = Ot + ot;
        memcpy (Ot + ot, s, n + 1);
        ot += n + 1;
    }
}

void __fastcall__ valn (unsigned n)
{
    val (num (n));
}

void end_line (void)
{
    emit (&O);
}

/* ---- Its arguments; the bank programs called */

static char path[64], expath[64], bp[5][48];
static const char* __fastcall__ in (const char* dir, const char* name)
{
    strcpy (path, dir);
    strcat (path, "/");
    strcat (path, name);
    return path;
}

static struct bank_fn* __fastcall__ child (unsigned char k, const char* file, const char* entry)
{
    struct bank_fn* h = bank_fn_ (strcpy (bp[k], in (lib, file)), entry);

    if (!h) {
        say ("themegen: its bank program ");
        say (file);
        say (" can't be loaded (");
        say (num (bank_err));
        say (") ");
        say (bank_missing);
        say ("\n");
        exit (1);
    }
    return h;
}

/* The layers checked (the theme checker's, as the history's made): each stratum of more than one layer planned by
** stratagen, the bank program the maker plans by, as a stratum of 8 by 6 chunks, its way in at (2, 3): every layer
** built or reused some chunks of it, so its era's works are there (its clues, its goal among them) */
static void check_layers (void)
{
    static struct sgq q;
    struct bank_fn* sg = child (4, "stratagen.bp", "_layer_of");
    unsigned char k, i, seen;
    struct str* s;

    for (k = 0, s = st; k < ns; ++k, ++s) {
        if (s->nl < 2) {
            continue;
        }
        q.nl = s->nl;
        memcpy (q.l, s->l, sizeof q.l);
        q.cw = 8;
        q.ch = 6;
        q.ix = 2;
        q.iy = 3;
        q.seed = seed * 69069UL + k;
        for (seen = 0, q.cy = 0; q.cy < q.ch; ++q.cy) {
            for (q.cx = 0; q.cx < q.cw; ++q.cx) {
                seen |= 1 << (bank_call (sg, unsigned char __fastcall__ (*)(struct sgq*), &q) & SG_LAYER);
            }
        }
        for (i = 0; i < s->nl; ++i) {
            if (s->l[i].did != TL_RUINED && !(seen & 1 << i)) {
                say ("themegen: the theme checker: stratum ");
                say (num (k));
                say (": a layer of it nowhere\n");
                bad = 1;
            }
        }
    }
}

int main (int argc, char* argv[])
{
    static char* targs[8];
    static char gamedir[40], dat[64], txt[64];
    struct bank_fn* hist;
    struct bank_fn* goal;
    struct bank_fn* lines;
    const char* prog = getenv ("prog");
    char* slash;
    unsigned char i;

    seed = hy_time () ^ hy_ticks ();
    for (i = 1; i < argc; ++i) {
        if (i + 1 < argc && argv[i][0] == '-' && strchr ("stol", argv[i][1]) && !argv[i][2]) {
            switch (argv[i++][1]) {
            case 's':
                for (seed = 0, slash = argv[i]; *slash >= '0' && *slash <= '9'; ++slash) {
                    seed = seed * 10 + *slash - '0';
                }
                break;
            case 't':
                tfile = argv[i];
                break;
            case 'o':
                outdir = argv[i];
                break;
            default:
                strncpy (lib, argv[i], sizeof lib - 1);
            }
        } else if (!strcmp (argv[i], "-q")) {
            quiet = 1;
        } else {
            say ("usage: themegen [-s seed] [-t theme] [-o folder] [-l lib] [-q]\n");
            return 1;
        }
    }
    strcpy (gamedir, "/sd/0/games");            /* (Its own folder, and its lib/dq0; else the card's) */
    if (prog && strlen (prog) < sizeof gamedir && (slash = strrchr (strcpy (path, prog), '/')) != 0) {
        *slash = 0;
        strcpy (gamedir, path);
    }
    if (!*lib) {
        strcpy (lib, gamedir);
        strcat (lib, "/lib/dq0");
    }
    if (tg_load (in (lib, "themegen.dat"))) {
        say ("themegen: its data (lib/dq0/themegen.dat) not found\n");
        return 1;
    }
    bestiary ();
    bank_setup (strcpy (expath, in (lib, "themegen.exp")), 0x60, 16);
    namer = child (0, "namegen.bp", "_name");
    hist = child (1, "histgen.bp", "_make");
    goal = child (2, "goalgen.bp", "_make");
    lines = child (3, "linegen.bp", "_make");
    if (tfile) {
        given_each (GROUPS, 0);                 /* (The groups it gives) */
    }
    if (outdir) {
        mkdir (outdir, 0755);
        if ((ofd = open (strcpy (txt, in (outdir, "theme.txt")), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
            say ("themegen: can't write theme.txt there\n");
            return 1;
        }
    }
    rseed (seed ^ 0x48495354UL);
    bank_call (hist, void (*)(void));           /* (The region, the eras, the strata, the inhabitants) */
    bank_call (goal, void (*)(void));           /* (The goal) */
    bank_call (lines, void (*)(void));          /* (The theme's lines) */
    check_layers ();
    if (ofd >= 0) {
        close (ofd);
    }
    if (bad || !outdir) {
        return bad;
    }
    targs[0] = "themec";                        /* (Compiled into its record there, and checked: themec in its
                                                **   place) */
    targs[1] = "-q";
    targs[2] = "-l";
    targs[3] = lib;
    targs[4] = "-o";
    targs[5] = strcpy (dat, in (outdir, "theme.dat"));
    targs[6] = txt;
    targs[7] = 0;
    hy_exec (in (gamedir, "themec"), targs);
    say ("themegen: themec can't be run\n");
    return 1;
}
