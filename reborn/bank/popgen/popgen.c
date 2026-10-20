/*
** popgen.c - Dingus Quest Zero's population generator (Phase 3D: games/dq0d/pop.h has what it does): a bank program
** (sdk/c/include/bankprog.h), the denizens' task's, run in a bank of the task's own.  It places the theme's factions
** as the history leaves them: from the theme file's inhabitants (its faction lines) and the kinds' groups (the
** denizens' data file, dq0d.dat), each stratum's lairs, their kinds, their factions and how many, the leaders in their
** homes; and the factions' attitudes, how they stand with one another, and their first kinds (their tongues').  What
** it read is kept in its bank's BSS while it's loaded, and read again if it was given up (its BSS cleared).
**
** Its callers give it only the C library's files and strings (open, read, write, close, strncmp ...: the task's and
** bankrun's both) and the helpers both have, so it keeps its own die and does its own sums.  From the shell, bankrun
** runs its run:
**   bankrun popgen.bp LIBDIR THEME [SEED]
** says the factions it read, and each stratum's lairs (stratum n's seed SEED + n), the same each time.
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "../../games/dq0d/pop.h"

#pragma static-locals (on)

#define NKINDS          32                      /* (hub.h's SND_KINDS: the kinds at most, the first none) */
#define KREC            40                      /* (dq0d.c's struct dkind: its key at 12, its group at 24) */
#define LINE            160                     /* A theme file's line, at most (the longer cut short) */
#define AT_I            3                       /* (hub.h's AT_INDIFFERENT, AT_FRIENDLY) */
#define AT_F            4

static struct kin {                             /* The kinds (the first none): their keys, their groups' sizes */
    char            key[8];
    unsigned char   min, max;
} kin[NKINDS];
static unsigned char nk;

static struct fac {                             /* The factions (the first none) */
    char            name[12];
    unsigned char   kind[POP_KINDS];            /* (0 none) */
    unsigned char   s0, s1, lair, lead, att;
    unsigned        num;
} fac[POP_NFACT];
static struct fac* F;                           /* (The one at hand, and its number; the next one's place) */
static struct fac* nextf;
static unsigned char fi, nf;
static unsigned char frel[POP_NFACT][POP_NFACT];
static unsigned char loaded;

static int fd;                                  /* (A file read a line at a time, through buf) */
static char buf[128];
static unsigned char bpos, blen;
static char line[LINE + 1], more[LINE + 1];
static char* fld[10];                           /* (A line's fields, between its bars) */
static unsigned char nfld;

/* ---- Sums (by shifts and adds: the callers lend neither a division nor a multiplication) */

static unsigned ua, ub, uq;

static unsigned __fastcall__ udiv (unsigned a, unsigned b)
{
    ua = a;
    ub = b;
    for (uq = 0; ua >= ub; ++uq) {
        ua -= ub;
    }
    return uq;
}

static unsigned __fastcall__ umul (unsigned a, unsigned char b)
{
    ua = a;
    for (uq = 0; b; --b) {
        uq += ua;
    }
    return uq;
}

/* The die: its own (a 16-bit shift register), seeded by the stratum's and the faction's numbers: 0 to n - 1 */
static unsigned rs;
static unsigned char rx, rk;

static unsigned char __fastcall__ rnd (unsigned char n)
{
    for (rk = 8; rk; --rk) {
        rs = rs & 1 ? rs >> 1 ^ 0xB400 : rs >> 1;
    }
    for (rx = (unsigned char) rs; rx >= n; rx -= n) {
    }
    return rx;
}

/* ---- Files, a line at a time */

static unsigned char __fastcall__ nextc (char* c)
{
    if (bpos == blen) {
        if (fd < 0 || (blen = read (fd, buf, sizeof buf)) == 0 || blen > sizeof buf) {
            blen = bpos = 0;
            return 0;
        }
        bpos = 0;
    }
    *c = buf[bpos++];
    return 1;
}

/* The next line into l (its end's CR and LF gone, cut short at LINE): 0 at the file's end */
static unsigned char __fastcall__ getline_ (char* l)
{
    static unsigned char n, got;
    static char c;

    for (n = got = 0; nextc (&c);) {
        got = 1;
        if (c == '\n') {
            break;
        }
        if (c != '\r' && n < LINE) {
            l[n++] = c;
        }
    }
    l[n] = 0;
    return got;
}

/* s with its spaces round it gone (in place) */
static char* __fastcall__ trim (char* s)
{
    static char* e;

    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    for (e = s + strlen (s); e > s && (e[-1] == ' ' || e[-1] == '\t'); --e) {
    }
    *e = 0;
    return s;
}

/* s past its word and the spaces after it */
static char* __fastcall__ past (char* s)
{
    while (*s && *s != ' ' && *s != ',') {
        ++s;
    }
    while (*s == ' ') {
        ++s;
    }
    return s;
}

static unsigned __fastcall__ number (const char* s)
{
    static unsigned n;

    for (n = 0; *s >= '0' && *s <= '9'; ++s) {
        n = n * 10 + (*s - '0');
    }
    return n;
}

/* Word w's length (up to a space, a comma or its end) */
static unsigned char __fastcall__ wlen (const char* w)
{
    static unsigned char n;

    for (n = 0; w[n] && w[n] != ' ' && w[n] != ','; ++n) {
    }
    return n;
}

/* Kind key k's number (k's first word), or 0 */
static unsigned char __fastcall__ kind_of (const char* k)
{
    static unsigned char i, n;

    if ((n = wlen (k)) > 7) {
        return 0;
    }
    for (i = 1; n && i < nk; ++i) {
        if (!strncmp (kin[i].key, k, n) && !kin[i].key[n]) {
            return i;
        }
    }
    return 0;
}

/* Faction name n's number (n's first word), or 0 */
static unsigned char __fastcall__ fac_of (const char* n)
{
    static unsigned char i, k;

    if ((k = wlen (n)) > 11) {
        return 0;
    }
    for (i = 1; k && i < nf; ++i) {
        if (!strncmp (fac[i].name, n, k) && !fac[i].name[k]) {
            return i;
        }
    }
    return 0;
}

/* An attitude's letter's AT_* (hub.h's: h u w i f), or 0xFF */
static unsigned char __fastcall__ att_of (char c)
{
    static const char at[] = "huwif";
    static unsigned char i;

    for (i = 0; at[i] && at[i] != c; ++i) {
    }
    return at[i] ? i : 0xFF;
}

/* ---- The theme read: the kinds (lib/dq0d.dat) and the inhabitants (the theme file's faction lines) */

/* The kinds' keys and groups */
static void __fastcall__ kinds (const char* lib)
{
    static struct kin* k;
    static unsigned char i;

    nk = 0;
    strcpy (line, lib);
    strcat (line, "/dq0d.dat");
    if ((fd = open (line, O_RDONLY)) < 0) {
        return;
    }
    for (k = kin, i = 0; read (fd, line, KREC) == KREC; ++i) {
        if (!i) {
            nk = (unsigned char) line[20] < NKINDS ? line[20] : NKINDS;
        }
        if (i >= nk) {
            break;
        }
        memcpy (k->key, line + 12, 8);
        k->min = line[24] ? line[24] : 1;
        k->max = (unsigned char) line[25] >= k->min ? line[25] : k->min;
        ++k;
    }
    close (fd);
}

/* A faction's line (fld: faction | NAME | KINDS | STRATA | NUMBER | LEADER | LAIR | ATTITUDE | OTHERS) taken in, as
** faction nf (pass 1); or its OTHERS, how it stands with the rest, the worse of the two ways it's said (pass 2) */
static unsigned char pass;

static void faction (void)
{
    static unsigned char j, k, g, a;
    static char* p;

    if (nfld < 9 || !*fld[1]) {
        return;
    }
    if (pass == 2) {
        if ((k = fac_of (fld[1])) == 0) {
            return;
        }
        for (p = fld[8]; *p && *p != '-';) {
            g = fac_of (p);
            a = att_of (*(p = past (p)));
            p = past (p);
            while (*p == ',' || *p == ' ') {
                ++p;
            }
            if (g && g != k && a != 0xFF && a < frel[k][g]) {
                frel[k][g] = frel[g][k] = a;
            }
        }
        return;
    }
    if (nf > POP_FACT || wlen (fld[1]) > 11) {
        return;
    }
    F = nextf;
    memset (F, 0, sizeof *F);
    memcpy (F->name, fld[1], wlen (fld[1]));
    for (j = 0, p = fld[2]; *p && j < POP_KINDS; ++j, p = past (p)) {
        if ((F->kind[j] = kind_of (p)) == 0) {
            return;                             /* (A kind there's none of: the faction refused) */
        }
    }
    if (!F->kind[0]) {
        return;
    }
    F->s0 = number (p = fld[3]);
    while (*p && *p != '-') {
        ++p;
    }
    F->s1 = *p ? number (p + 1) : F->s0;
    if (F->s1 < F->s0) {
        F->s1 = F->s0;
    }
    F->num = number (fld[4]);
    F->lead = kind_of (fld[5]);
    F->lair = number (fld[6]);
    if (F->lair < F->s0 || F->lair > F->s1) {
        F->lair = F->s0;                        /* (Its lair on a stratum of its own) */
    }
    if ((F->att = att_of (*fld[7])) == 0xFF) {
        F->att = AT_I;                          /* (Indifferent, if it doesn't say) */
    }
    ++nf;
    ++nextf;
}

/* The theme file read through (each line and the lines that go on with it at a time), its faction lines taken in */
static void __fastcall__ read_theme (const char* theme)
{
    static unsigned char k, mk;

    if ((fd = open (theme, O_RDONLY)) < 0) {
        return;
    }
    bpos = blen = 0;
    for (mk = getline_ (more); mk;) {
        strcpy (line, more);
        while ((mk = getline_ (more)) != 0 && (more[0] == ' ' || more[0] == '\t')) {
            if (strlen (line) + strlen (more) < LINE) {
                strcat (line, " ");
                strcat (line, trim (more));
            }
        }
        for (nfld = 0, fld[nfld++] = line, k = 0; line[k]; ++k) {
            if (line[k] == '|' && nfld < sizeof fld / sizeof fld[0]) {
                line[k] = 0;
                fld[nfld++] = line + k + 1;
            }
        }
        for (k = 0; k < nfld; ++k) {
            fld[k] = trim (fld[k]);
        }
        if (!strncmp (fld[0], "faction", 8)) {
            faction ();
        }
    }
    close (fd);
}

/* The kinds and the theme's factions read, and how they stand (itself friendly, the none indifferent) */
static void __fastcall__ load (struct pgarg* a)
{
    static unsigned char f, g;

    loaded = 1;
    nf = 1;
    nextf = fac + 1;
    kinds (a->lib);
    if (nk < 2) {
        return;
    }
    pass = 1;
    read_theme (a->theme);
    for (f = 0; f < POP_NFACT; ++f) {
        for (g = 0; g < POP_NFACT; ++g) {
            frel[f][g] = f == g && f ? AT_F : AT_I;
        }
    }
    pass = 2;
    if (nf > 1) {
        read_theme (a->theme);
    }
}

/* The factions read (the theme's, and the kinds), their attitudes, relations and first kinds the task's: how many */
unsigned char __fastcall__ pg_load (struct pgarg* a)
{
    static unsigned char f;

    load (a);
    for (f = 0; f < POP_NFACT; ++f) {
        a->att[f] = f && f < nf ? fac[f].att : AT_I;
        a->first[f] = f && f < nf ? fac[f].kind[0] : 0;
    }
    memcpy (a->rel, frel, sizeof frel);
    return nf - 1;
}

/* ---- A stratum's lairs */

static struct pglair* out;
static unsigned char nout;

static void __fastcall__ lair (unsigned char kind, unsigned char n, unsigned char flags)
{
    if (nout < POP_LAIRS && n) {
        out->kind = kind;
        out->fac = fi;
        out->n = n;
        out->flags = flags;
        ++out;
        ++nout;
    }
}

/* Faction F's share on stratum sn: its number spread over its strata, each weighed by how near its lair's it is (the
** lair's the most, its strata's count and one, each further a step less), the shares' rounding left over its lair's */
static unsigned __fastcall__ share (unsigned char sn)
{
    static unsigned char s, s0, s1, lr, w, top, d;
    static unsigned total, mine, given, n;

    total = given = mine = 0;
    s0 = F->s0;
    s1 = F->s1;
    lr = F->lair;
    top = s1 - s0 + 2;
    for (s = s0; s <= s1; ++s) {
        d = s > lr ? s - lr : lr - s;
        total += top - d;
    }
    for (s = s0; s <= s1; ++s) {
        d = s > lr ? s - lr : lr - s;
        w = top - d;
        n = udiv (umul (F->num, w), total);
        given += n;
        if (s == sn) {
            mine = n;
        }
    }
    if (sn == lr) {
        mine += F->num - given;
    }
    return mine;
}

/* Kind k's count n in its lairs, faction F's (groups of its kind's size, eight at most: the bigger, if it's more), the
** first its home if home, its leader with it */
static void __fastcall__ groups (unsigned char k, unsigned n, unsigned char home)
{
    static unsigned char g, least, most;
    static unsigned left;

    left = n;
    least = kin[k].min;
    most = kin[k].max;
    while (most < 255 && udiv (left + most - 1, most) > 8) {
        ++most;                                 /* (Too many for eight: bigger groups) */
    }
    while (left) {
        if (left <= most) {
            g = left;                           /* (What's left one group) */
        } else {
            g = least + rnd (most - least + 1);
            if (left - g < least) {
                g = left - least;               /* (Enough left for another) */
            }
        }
        lair (k, g, home ? PL_HOME : 0);
        if (home && F->lead) {
            lair (F->lead, 1, PL_LEADER);
        }
        home = 0;
        left -= g;
    }
}

/* Stratum a->sn's lairs into a->lair (its seed a->seed): how many */
unsigned char __fastcall__ pg_stratum (struct pgarg* a)
{
    static unsigned char j, m, sn;
    static unsigned n, each;

    if (!loaded) {
        load (a);                               /* (Given up and loaded again: read again) */
    }
    out = a->lair;
    nout = 0;
    sn = a->sn;
    for (fi = 1, F = fac + 1; fi < nf; ++fi, ++F) {
        if (sn < F->s0 || sn > F->s1 || (n = share (sn)) == 0) {
            continue;
        }
        rs = a->seed ^ ((unsigned) fi << 8 | fi) ^ 0x5A5A;
        rs = rs ? rs : 1;
        for (m = 0; m < POP_KINDS && F->kind[m]; ++m) {
        }
        each = udiv (n, m + 1);                 /* (The first kind twice each other's share, and what's left) */
        groups (F->kind[0], n - umul (each, m - 1), sn == F->lair);
        for (j = 1; j < m; ++j) {
            groups (F->kind[j], each, 0);
        }
    }
    return nout;
}

/* ---- From the shell (bankrun): the factions, and each stratum's lairs */

static char ob[160];
static unsigned char on;

static void __fastcall__ say (const char* s)
{
    while (*s && on < sizeof ob - 1) {
        ob[on++] = *s++;
    }
}

static void __fastcall__ sayn (unsigned n)
{
    static char d[6];
    static unsigned char i;
    static unsigned q;

    i = 5;
    do {
        q = udiv (n, 10);
        d[--i] = '0' + (unsigned char) (n - umul (q, 10));
        n = q;
    } while (n);
    d[5] = 0;
    say (d + i);
}

static void flush (void)
{
    ob[on++] = '\n';
    write (1, ob, on);
    on = 0;
}

int __fastcall__ run (int argc, char** argv)
{
    static const char* const att_[] = { "hostile to ", "unfriendly to ", "wary of ", "indifferent to ", "friendly to " };
    static struct pgarg a;
    static struct pglair l[POP_LAIRS];
    static unsigned char att[POP_NFACT], rel[POP_NFACT * POP_NFACT], first[POP_NFACT];
    static unsigned char f, g, j, n, s, deep;
    static unsigned seed;

    if (argc < 3) {
        say ("usage: bankrun popgen.bp LIBDIR THEME [SEED]");
        flush ();
        return 1;
    }
    a.lib = argv[1];
    a.theme = argv[2];
    a.att = att;
    a.rel = rel;
    a.first = first;
    a.lair = l;
    seed = argc > 3 ? number (argv[3]) : 1;
    say ("popgen: ");
    sayn (pg_load (&a));
    say (" factions, ");
    sayn (nk ? nk - 1 : 0);
    say (" kinds");
    flush ();
    for (deep = 0, f = 1; f < nf; ++f) {        /* "Gnawers (rat bat): 40 on strata 0-3, their lair on 0; hostile to
                                                **   you; hostile to Diggers, wary of Brigands" */
        F = &fac[f];
        say (F->name);
        say (" (");
        for (j = 0; j < POP_KINDS && F->kind[j]; ++j) {
            say (j ? " " : "");
            say (kin[F->kind[j]].key);
        }
        say ("): ");
        sayn (F->num);
        say (" on strata ");
        sayn (F->s0);
        say ("-");
        sayn (F->s1);
        if (F->lead) {
            say (", led by a ");
            say (kin[F->lead].key);
        }
        say (", their lair on ");
        sayn (F->lair);
        say ("; ");
        say (att_[att[f]]);
        say ("you");
        for (n = 0, g = 1; g < nf; ++g) {
            if (g != f && rel[f * POP_NFACT + g] != AT_I) {
                say (n++ ? ", " : "; ");
                say (att_[rel[f * POP_NFACT + g]]);
                say (fac[g].name);
            }
        }
        flush ();
        deep = F->s1 > deep ? F->s1 : deep;
    }
    for (s = 0; nf > 1 && s <= deep; ++s) {     /* "stratum 0: rat 5 home, rat 4 (Gnawers); bandit 3 home, banlord 1
                                                **   leader (Brigands)" */
        a.sn = s;
        a.seed = seed + s;
        n = pg_stratum (&a);
        say ("stratum ");
        sayn (s);
        say (":");
        for (j = 0; j < n; ++j) {
            if (on > sizeof ob - 32) {
                flush ();
                say (" ");
            }
            say (" ");
            say (kin[l[j].kind].key);
            say (" ");
            sayn (l[j].n);
            say (l[j].flags & PL_HOME ? " home" : l[j].flags & PL_LEADER ? " leader" : "");
            if (j + 1 == n || l[j + 1].fac != l[j].fac) {
                say (" (");
                say (fac[l[j].fac].name);
                say (j + 1 < n ? ");" : ")");
            } else {
                say (",");
            }
        }
        flush ();
    }
    return 0;
}
