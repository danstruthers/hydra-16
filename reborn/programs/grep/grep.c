/*
** grep.c - grep [-chilnsv] [-e] pattern [file ...]: the lines that match a regular expression (Plan 9's grep).  The
** expression is Plan 9's regexp(6), less back references: a character matches itself, \c the character c (\. a
** dot), . any character, [abc] and [a-z] one of a set, [^abc] one not in it, ^ the line's start, $ its end; e* none
** or more of e, e+ one or more, e? none or one; e1e2 e1 then e2, e1|e2 either; (e) e.  Each file's lines (none:
** fd 0), with the file's name before each when there are several (-h: never).  -c: the count of the lines that
** match, not them; -l: the names of the files with one; -n: each line's number before it; -i: case ignored; -v:
** the lines that don't match; -s: no messages about files that can't be read; -e: the pattern next (one that
** starts with -).  Its status: none if a line matched, "no matches" if none did, 1 if a file couldn't be read;
** "bad expression"; "usage".  A line longer than 254 bytes is matched in pieces.
**   The expression is compiled to a machine of states (Thompson's), and each line run through it a character at
** a time with every state it can be in at once: no backtracking, so no expression takes long, and the 6502's
** stack isn't used up.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <hydra.h>

#define LINE_SIZE   256         /* A line: 255 bytes at most (with its LF), and the 0 after it */
#define POST_MAX    128         /* The expression, postfix: its items at most */
#define SETS_MAX    8           /* Its sets ([...]) at most */
#define DEPTH_MAX   16          /* Its ( ) nested this deep at most */

/* The postfix items: a character (0-255), or one of these.  (A state's c is one of them too, or S_SPLIT or
** S_MATCH) */
#define P_ANY       0x100       /* . */
#define P_BOL       0x101       /* ^ */
#define P_EOL       0x102       /* $ */
#define P_CAT       0x103       /* e1 then e2 */
#define P_ALT       0x104       /* e1 or e2 */
#define P_STAR      0x105       /* e* */
#define P_PLUS      0x106       /* e+ */
#define P_QUEST     0x107       /* e? */
#define P_SET       0x110       /* + n: set n */
#define S_SPLIT     0x200       /* Both of out and out1 */
#define S_MATCH     0x201       /* The end: a match */

struct state {
    unsigned        c;          /* What it is (above) */
    struct state*   out;        /* Where it goes next ... */
    struct state*   out1;       /*   (S_SPLIT's other) */
    unsigned        mark;       /* The step it was last put on a list at */
};

/* A piece of the machine as it's built: its start, and the out pointers still to be set (a list through them) */
typedef union outs {
    union outs*     next;
    struct state*   s;
} outs;
struct frag {
    struct state*   start;
    outs*           out;
};

static unsigned post[POST_MAX];
static unsigned char npost;
static unsigned char sets[SETS_MAX][32];
static unsigned char nsets;
static struct state states[POST_MAX + 1];
static unsigned char nstates;
static struct state* start;
static struct state* lista[POST_MAX + 1];
static struct state* listb[POST_MAX + 1];
static unsigned step;

static unsigned char cflag, hflag, iflag, lflag, nflag, sflag, vflag;
static unsigned char matched, failed, named;

static void usage (void)
{
    fputs ("usage: grep [-chilnsv] [-e] pattern [file ...]\n", stderr);
    hy_exits ("usage");
}

static void bad (const char* why)
{
    fprintf (stderr, "grep: bad expression: %s\n", why);
    hy_exits ("bad expression");
}

static const char* why (void)
{
    return _oserror ? _stroserror (_oserror) : strerror (errno);
}

/* ---- The expression: postfix, with each "then" made an item (Thompson's) */

static void emit (unsigned p)
{
    if (npost == POST_MAX) {
        bad ("too long");
    }
    post[npost++] = p;
}

static unsigned char fold (unsigned char c)
{
    return iflag ? tolower (c) : c;
}

/* A set, from re (at its [) to its ]: its item.  A ] first is one of it; - between two is a range */
static unsigned set (const char** rp)
{
    const char* r = *rp + 1;
    unsigned char* s;
    unsigned char neg = 0, first = 1, i;
    unsigned c, lo, hi;

    if (nsets == SETS_MAX) {
        bad ("too many sets");
    }
    s = sets[nsets];
    if (*r == '^') {
        neg = 1;
        ++r;
    }
    while (*r && (*r != ']' || first)) {
        first = 0;
        if (*r == '\\' && r[1]) {
            ++r;
        }
        lo = hi = (unsigned char) *r;
        if (r[1] == '-' && r[2] && r[2] != ']') {
            r += 2;
            if (*r == '\\' && r[1]) {
                ++r;
            }
            hi = (unsigned char) *r;
        }
        for (c = lo; c <= hi; ++c) {
            s[c >> 3] |= 1 << (c & 7);
        }
        ++r;
    }
    if (*r != ']') {
        bad ("no ]");
    }
    if (iflag) {                                        /* (A line's characters are folded too) */
        for (c = 'A'; c <= 'Z'; ++c) {
            if (s[c >> 3] & (1 << (c & 7))) {
                s[(c + 32) >> 3] |= 1 << ((c + 32) & 7);
            }
        }
    }
    if (neg) {
        for (i = 0; i < 32; ++i) {
            s[i] = ~s[i];
        }
    }
    *rp = r;
    return P_SET + nsets++;
}

static void topost (const char* re)
{
    static struct {
        unsigned char nalt, natom;
    } paren[DEPTH_MAX];
    unsigned char depth = 0, nalt = 0, natom = 0;
    unsigned c;

    for (; *re; ++re) {
        switch (*re) {
        case '(':
            if (natom > 1) {
                --natom;
                emit (P_CAT);
            }
            if (depth == DEPTH_MAX) {
                bad ("too deep");
            }
            paren[depth].nalt = nalt;
            paren[depth].natom = natom;
            ++depth;
            nalt = natom = 0;
            continue;
        case '|':
            if (natom == 0) {
                bad ("nothing before |");
            }
            while (--natom > 0) {
                emit (P_CAT);
            }
            ++nalt;
            continue;
        case ')':
            if (depth == 0) {
                bad ("no (");
            }
            if (natom == 0) {
                bad ("nothing in ( )");
            }
            while (--natom > 0) {
                emit (P_CAT);
            }
            for (; nalt > 0; --nalt) {
                emit (P_ALT);
            }
            --depth;
            nalt = paren[depth].nalt;
            natom = paren[depth].natom + 1;
            continue;
        case '*':
        case '+':
        case '?':
            if (natom == 0) {
                bad ("nothing before *, + or ?");
            }
            emit (*re == '*' ? P_STAR : *re == '+' ? P_PLUS : P_QUEST);
            continue;
        case '.':
            c = P_ANY;
            break;
        case '^':
            c = P_BOL;
            break;
        case '$':
            c = P_EOL;
            break;
        case '[':
            c = set (&re);
            break;
        case '\\':
            if (re[1]) {
                ++re;
            }
            /* Fall through */
        default:
            c = fold (*re);
            break;
        }
        if (natom > 1) {
            --natom;
            emit (P_CAT);
        }
        emit (c);
        ++natom;
    }
    if (depth) {
        bad ("no )");
    }
    if (nalt && natom == 0) {
        bad ("nothing after |");
    }
    if (natom) {
        while (--natom > 0) {
            emit (P_CAT);
        }
    }
    for (; nalt > 0; --nalt) {
        emit (P_ALT);
    }
}

/* ---- The machine, from the postfix */

static struct state* state (unsigned c, struct state* out, struct state* out1)
{
    register struct state* s = &states[nstates++];

    s->c = c;
    s->out = out;
    s->out1 = out1;
    s->mark = 0;
    return s;
}

static outs* one (struct state** p)
{
    outs* l = (outs*) p;

    l->next = 0;
    return l;
}

static void patch (outs* l, struct state* s)
{
    outs* next;

    for (; l; l = next) {
        next = l->next;
        l->s = s;
    }
}

static outs* append (outs* l1, outs* l2)
{
    outs* l = l1;

    while (l->next) {
        l = l->next;
    }
    l->next = l2;
    return l1;
}

static void build (void)
{
    static struct frag stack[POST_MAX];
    register struct frag* sp = stack;
    struct frag e1, e2;
    struct state* s;
    unsigned char i;
    unsigned p;

    if (npost == 0) {                                   /* (An empty expression: every line) */
        start = state (S_MATCH, 0, 0);
        return;
    }
    for (i = 0; i < npost; ++i) {
        p = post[i];
        switch (p) {
        case P_CAT:
            e2 = *--sp;
            e1 = *--sp;
            patch (e1.out, e2.start);
            sp->start = e1.start;
            sp->out = e2.out;
            break;
        case P_ALT:
            e2 = *--sp;
            e1 = *--sp;
            sp->start = state (S_SPLIT, e1.start, e2.start);
            sp->out = append (e1.out, e2.out);
            break;
        case P_QUEST:
            e1 = *--sp;
            s = state (S_SPLIT, e1.start, 0);
            sp->start = s;
            sp->out = append (e1.out, one (&s->out1));
            break;
        case P_STAR:
            e1 = *--sp;
            s = state (S_SPLIT, e1.start, 0);
            patch (e1.out, s);
            sp->start = s;
            sp->out = one (&s->out1);
            break;
        case P_PLUS:
            e1 = *--sp;
            s = state (S_SPLIT, e1.start, 0);
            patch (e1.out, s);
            sp->start = e1.start;
            sp->out = one (&s->out1);
            break;
        default:
            s = state (p, 0, 0);
            sp->start = s;
            sp->out = one (&s->out);
            break;
        }
        ++sp;
    }
    e1 = *--sp;
    patch (e1.out, state (S_MATCH, 0, 0));
    start = e1.start;
}

/* ---- A line through the machine */

/* The next step (each state on a list once a step) */
static void next (void)
{
    unsigned char i;

    if (++step == 0) {
        for (i = 0; i < nstates; ++i) {
            states[i].mark = 0;
        }
        step = 1;
    }
}

/* State s onto list l (*n of them), and those it goes to without a character (S_SPLIT's, ^ at the start, $ at
** the end) */
static void add (struct state** l, unsigned char* n, struct state* s, unsigned char pos, unsigned char len)
{
    static struct state* todo[2 * (POST_MAX + 1)];
    unsigned k = 0;

    todo[k++] = s;
    while (k) {
        s = todo[--k];
        if (s == 0 || s->mark == step) {
            continue;
        }
        s->mark = step;
        if (s->c == S_SPLIT) {
            todo[k++] = s->out1;
            todo[k++] = s->out;
        } else if (s->c == P_BOL) {
            if (pos == 0) {
                todo[k++] = s->out;
            }
        } else if (s->c == P_EOL) {
            if (pos == len) {
                todo[k++] = s->out;
            }
        } else {
            l[(*n)++] = s;
        }
    }
}

/* Does line s (len bytes) match: somewhere in it? */
static unsigned char match (const char* s, unsigned char len)
{
    struct state** cl = lista;
    struct state** nl = listb;
    struct state** t;
    register struct state* st;
    unsigned char cn = 0, nn, i, pos, c;
    unsigned sc;

    next ();
    add (cl, &cn, start, 0, len);
    for (pos = 0; ; ++pos) {
        for (i = 0; i < cn; ++i) {
            if (cl[i]->c == S_MATCH) {
                return 1;
            }
        }
        if (pos == len) {
            return 0;
        }
        c = fold (s[pos]);
        next ();
        nn = 0;
        for (i = 0; i < cn; ++i) {
            st = cl[i];
            sc = st->c;
            if (sc == c || sc == P_ANY
                || (sc >= P_SET && sc < S_SPLIT && (sets[sc - P_SET][c >> 3] & (1 << (c & 7))))) {
                add (nl, &nn, st->out, pos + 1, len);
            }
        }
        add (nl, &nn, start, pos + 1, len);             /* (A match may start anywhere) */
        t = cl;
        cl = nl;
        nl = t;
        cn = nn;
    }
}

/* ---- The files */

static void grep (FILE* f, const char* name)
{
    static char line[LINE_SIZE];
    unsigned count = 0, lineno = 0;
    unsigned char len, whole = 1, end, m;

    while (fgets (line, sizeof line, f)) {
        if (whole) {
            ++lineno;
        }
        len = strlen (line);
        end = len && line[len - 1] == '\n';
        whole = end;                                    /* (Its next piece isn't a new line) */
        m = match (line, len - end) != vflag;
        if (!m) {
            continue;
        }
        matched = 1;
        ++count;
        if (lflag) {
            puts (name ? name : "(standard input)");
            return;
        }
        if (cflag) {
            continue;
        }
        if (named) {
            printf ("%s:", name);
        }
        if (nflag) {
            printf ("%u:", lineno);
        }
        fputs (line, stdout);
        if (!end && feof (f)) {
            putchar ('\n');
        }
    }
    if (ferror (f) && !sflag) {
        fprintf (stderr, "grep: %s: %s\n", name ? name : "read error", why ());
        failed = 1;
    }
    if (cflag) {
        if (named) {
            printf ("%s:", name);
        }
        printf ("%u\n", count);
    }
}

int main (int argc, char* argv[])
{
    const char* re = 0;
    const char* p;
    FILE* f;
    int i;

    for (i = 1; i < argc && argv[i][0] == '-' && argv[i][1] && !re; ++i) {
        if (strcmp (argv[i], "--") == 0) {
            ++i;
            break;
        }
        for (p = argv[i] + 1; *p; ++p) {
            switch (*p) {
            case 'c':   cflag = 1;  break;
            case 'h':   hflag = 1;  break;
            case 'i':   iflag = 1;  break;
            case 'l':   lflag = 1;  break;
            case 'n':   nflag = 1;  break;
            case 's':   sflag = 1;  break;
            case 'v':   vflag = 1;  break;
            case 'e':
                if (p[1] || i + 1 >= argc) {
                    usage ();
                }
                re = argv[++i];
                break;
            default:
                usage ();
            }
        }
    }
    if (re == 0) {
        if (i >= argc) {
            usage ();
        }
        re = argv[i++];
    }
    topost (re);
    build ();
    named = argc - i > 1 && !hflag;
    if (i == argc) {
        grep (stdin, 0);
    }
    for (; i < argc; ++i) {
        if ((f = fopen (argv[i], "r")) == 0) {
            if (!sflag) {
                fprintf (stderr, "grep: %s: %s\n", argv[i], why ());
            }
            failed = 1;
            continue;
        }
        grep (f, argv[i]);
        fclose (f);
    }
    if (fflush (stdout) || ferror (stdout)) {
        fprintf (stderr, "grep: write error: %s\n", why ());
        hy_exits ("write error");
    }
    if (failed) {
        return 1;
    }
    if (!matched) {
        hy_exits ("no matches");
    }
    return 0;
}
