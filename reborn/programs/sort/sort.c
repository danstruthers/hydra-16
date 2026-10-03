/*
** sort.c - sort [-bfnru] [file ...]: the lines of the files (none: fd 0), in order (Plan 9's sort, its common
** flags): by their bytes; -n by the number each starts with (blanks, a sign, digits; none is 0), then by their
** bytes; -f with case folded; -b with leading blanks ignored; -r the other way round; -u the first of each run of
** equal lines alone.  The lines are held in memory (some 20K of them); more is "too big".  A line without an LF
** at the end gets one; a line longer than 254 bytes is sorted in pieces.  Its status: none, 1 if a file couldn't
** be read, "too big", "usage".
**   A merge sort, bottom up: stable, n log n at worst, and no recursion (the 6502's stack is small).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hydra.h>

#define LINE_SIZE   256         /* A line: 255 bytes at most (with its LF), and the 0 after it */
#define GROW        64          /* The lines' table grows this many at a time */

static char** lines;
static unsigned n, room;
static unsigned char bflag, fflag, nflag, rflag, uflag, failed;

static void usage (void)
{
    fputs ("usage: sort [-bfnru] [file ...]\n", stderr);
    hy_exits ("usage");
}

static void toobig (void)
{
    fputs ("sort: too big: out of memory\n", stderr);
    hy_exits ("too big");
}

static const char* why (void)
{
    return _oserror ? _stroserror (_oserror) : strerror (errno);
}

/* The lines of f, into the table */
static void take (FILE* f, const char* name)
{
    static char line[LINE_SIZE];
    unsigned char len;
    char* p;

    while (fgets (line, sizeof line - 1, f)) {
        len = strlen (line);
        if (line[len - 1] != '\n') {
            line[len++] = '\n';
            line[len] = 0;
        }
        if (n == room) {
            room += GROW;
            if ((lines = realloc (lines, room * sizeof (char*))) == 0) {
                toobig ();
            }
        }
        if ((p = malloc (len + 1)) == 0) {
            toobig ();
        }
        memcpy (p, line, len + 1);
        lines[n++] = p;
    }
    if (ferror (f)) {
        fprintf (stderr, "sort: %s: %s\n", name, why ());
        failed = 1;
    }
}

static const char* blanks (register const char* s)
{
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    return s;
}

/* Line a against b: < 0, 0 or > 0 (-r: the other way round) */
static int compare (const char* a, const char* b)
{
    long x, y;
    int r = 0;

    if (bflag) {
        a = blanks (a);
        b = blanks (b);
    }
    if (nflag) {
        x = strtol (a, 0, 10);
        y = strtol (b, 0, 10);
        r = x < y ? -1 : x > y;
    }
    if (r == 0) {
        r = fflag ? strcasecmp (a, b) : strcmp (a, b);
    }
    return rflag ? -r : r;
}

/* The table sorted: runs of 1, 2, 4 ... merged, from one table into the other */
static void sort (void)
{
    char** a = lines;
    char** b;
    char** t;
    unsigned w, lo, mid, hi, x, y, k;

    if (n < 2) {
        return;
    }
    if ((b = malloc (n * sizeof (char*))) == 0) {
        toobig ();
    }
    for (w = 1; w < n; w *= 2) {
        for (lo = 0; lo < n; lo += 2 * w) {
            mid = n - lo > w ? lo + w : n;
            hi = n - lo > 2 * w ? lo + 2 * w : n;
            x = lo;
            y = mid;
            k = lo;
            while (x < mid && y < hi) {
                b[k++] = compare (a[y], a[x]) < 0 ? a[y++] : a[x++];   /* (Equal: the first's first) */
            }
            while (x < mid) {
                b[k++] = a[x++];
            }
            while (y < hi) {
                b[k++] = a[y++];
            }
        }
        t = a;
        a = b;
        b = t;
    }
    lines = a;
}

int main (int argc, char* argv[])
{
    const char* p;
    FILE* f;
    unsigned i;
    int k;

    for (k = 1; k < argc && argv[k][0] == '-' && argv[k][1]; ++k) {
        if (strcmp (argv[k], "--") == 0) {
            ++k;
            break;
        }
        for (p = argv[k] + 1; *p; ++p) {
            switch (*p) {
            case 'b':   bflag = 1;  break;
            case 'f':   fflag = 1;  break;
            case 'n':   nflag = 1;  break;
            case 'r':   rflag = 1;  break;
            case 'u':   uflag = 1;  break;
            default:
                usage ();
            }
        }
    }
    if (k == argc) {
        take (stdin, "stdin");
    }
    for (; k < argc; ++k) {
        if ((f = fopen (argv[k], "r")) == 0) {
            fprintf (stderr, "sort: %s: %s\n", argv[k], why ());
            failed = 1;
            continue;
        }
        take (f, argv[k]);
        fclose (f);
    }
    sort ();
    for (i = 0; i < n; ++i) {
        if (uflag && i && compare (lines[i - 1], lines[i]) == 0) {
            continue;
        }
        fputs (lines[i], stdout);
    }
    if (fflush (stdout) || ferror (stdout)) {
        fprintf (stderr, "sort: write error: %s\n", why ());
        hy_exits ("write error");
    }
    return failed;
}
