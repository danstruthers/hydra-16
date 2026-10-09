/*
** lbl.c - asm.h's symbols: the symbols of ld65's label files (ld65 -Ln, as -l: "al 000830 .main"; not cheap
** locals, nor the linker's __NAME__, which aren't places) and of a .inc's system calls ("WRITE = $F953", $F800 up,
** and r0-r15), in one table kept in order of address, so a symbol is found by halves (db and dis look up every
** instruction's).  A name two places have (cc65's L0001 in each module ...) is marked, so a label a source defines
** is one that names one place (lbl_label).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <asm.h>

#define GROW        64                  /* The table grows this many at a time */

struct sym {
    unsigned        addr;
    char*           name;
    unsigned char   dup;                /* Another place's name too */
};

static struct sym* syms;
static unsigned nsym, room;
static char near_buf[48];
static char lb[96];

unsigned lbl_count (void)
{
    return nsym;
}

static int bysym (const void* a, const void* b)
{
    unsigned x = ((const struct sym*) a)->addr, y = ((const struct sym*) b)->addr;

    return x < y ? -1 : x > y;
}

/* A symbol added: 0, or -1 (no room) */
static int add (unsigned addr, const char* name)
{
    struct sym* s;

    if (nsym == room) {
        s = realloc (syms, (room + GROW) * sizeof (struct sym));
        if (!s) {
            return -1;
        }
        syms = s;
        room += GROW;
    }
    if ((syms[nsym].name = strdup (name)) == 0) {
        return -1;
    }
    syms[nsym].dup = 0;
    syms[nsym++].addr = addr;
    return 0;
}

static int byname (const void* a, const void* b)
{
    return strcmp (syms[*(const unsigned*) a].name, syms[*(const unsigned*) b].name);
}

/* Each name two places have marked (dup), by an index in the names' order */
static void dups (void)
{
    unsigned* x = malloc (nsym * sizeof (unsigned));
    unsigned i, j, k;

    if (!x) {
        return;
    }
    for (i = 0; i < nsym; ++i) {
        x[i] = i;
    }
    qsort (x, nsym, sizeof (unsigned), byname);
    for (i = 0; i < nsym; i = j) {
        for (j = i + 1; j < nsym && !strcmp (syms[x[i]].name, syms[x[j]].name); ++j) {
        }
        for (k = i + 1; k < j && syms[x[k]].addr == syms[x[i]].addr; ++k) {
        }
        if (k < j) {
            for (k = i; k < j; ++k) {
                syms[x[k]].dup = 1;
            }
        }
    }
    free (x);
}

/* The line's end cut (its \r or \n) */
static void cut (char* p)
{
    while (*p && *p != '\r' && *p != '\n') {
        ++p;
    }
    *p = 0;
}

int __fastcall__ lbl_load (const char* file)
{
    FILE* f = fopen (file, "r");
    char* p;
    char* q;
    unsigned n = 0, a, len;

    if (!f) {
        return -1;
    }
    while (fgets (lb, sizeof lb, f)) {
        cut (lb);
        if (lb[0] == 'a' && lb[1] == 'l' && lb[2] == ' ') {     /* al 000830 .main */
            p = strchr (lb + 3, ' ');
            if (!p || p[1] != '.' || p[2] == '@') {
                continue;
            }
            len = strlen (p + 2);
            if (len > 4 && p[2] == '_' && p[3] == '_' && p[len] == '_' && p[len + 1] == '_') {
                continue;                                       /* (__DATA_LOAD__ ...: the linker's) */
            }
            a = (unsigned) strtoul (lb + 3, 0, 16);
            p += 2;
        } else if (isalpha ((unsigned char) lb[0])) {           /* WRITE           = $F953 ... */
            for (p = lb; isalnum ((unsigned char) *p) || *p == '_'; ++p) {
            }
            q = p;
            while (*q == ' ') {
                ++q;
            }
            if (*q != '=' || q[1] != ' ') {
                continue;
            }
            while (*++q == ' ') {
            }
            if (*q != '$') {
                continue;
            }
            a = (unsigned) strtoul (q + 1, 0, 16);
            *p = 0;
            p = lb;
            if (a < 0xF800 && !(p[0] == 'r' && isdigit ((unsigned char) p[1]) && a < 0x22)) {
                continue;                                       /* (Not a call, nor r0-r15) */
            }
        } else {
            continue;
        }
        if (add (a, p) < 0) {
            break;
        }
        ++n;
    }
    fclose (f);
    qsort (syms, nsym, sizeof (struct sym), bysym);
    dups ();
    return n;
}

/* The last symbol at or below addr, or NULL */
static struct sym* below (unsigned addr)
{
    unsigned lo = 0, hi = nsym, mid;

    while (lo < hi) {                                           /* (The first above addr: hi) */
        mid = (lo + hi) / 2;
        if (syms[mid].addr <= addr) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    return lo ? &syms[lo - 1] : 0;
}

const char* __fastcall__ lbl_at (unsigned addr)
{
    struct sym* s = below (addr);

    if (!s || s->addr != addr) {
        return 0;
    }
    while (s > syms && s[-1].addr == addr) {                     /* (The first of those there) */
        --s;
    }
    return s->name;
}

const char* __fastcall__ lbl_label (unsigned addr)
{
    struct sym* s = below (addr);

    if (!s || s->addr != addr) {
        return 0;
    }
    for (; s >= syms && s->addr == addr; --s) {
        if (!s->dup) {
            return s->name;
        }
    }
    return 0;
}

const char* __fastcall__ lbl_near (unsigned addr, unsigned within)
{
    struct sym* s = below (addr);

    if (!s || addr - s->addr >= within) {
        return 0;
    }
    if (s->addr == addr) {
        return lbl_at (addr);
    }
    sprintf (near_buf, "%.36s+%X", lbl_at (s->addr), addr - s->addr);
    return near_buf;
}

int __fastcall__ lbl_addr (const char* name, unsigned* addr)
{
    unsigned i;

    for (i = 0; i < nsym; ++i) {
        if (!strcmp (syms[i].name, name)) {
            *addr = syms[i].addr;
            return 0;
        }
    }
    return -1;
}
