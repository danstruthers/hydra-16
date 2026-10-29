/*
** data.c - (themegen, themec: data.h) a theme's lines read from its file, as ../dq0/make.js reads them; themegen's
** data file in banks of the program's own, its lines found and copied out (the window put back as it was: a bank
** program calls these from it); the bestiary from it.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "data.h"

void __fastcall__ say (const char* s)
{
    write (2, s, strlen (s));
}

const char* __fastcall__ num (unsigned n)
{
    static char b[8];

    return utoa (n, b, 10);
}

/* s's place in list (0 ...; list NULL-ended), or -1 */
signed char __fastcall__ find_word (const char* const* list, const char* s)
{
    signed char k;

    for (k = 0; list[k]; ++k) {
        if (!strcmp (list[k], s)) {
            return k;
        }
    }
    return -1;
}

static const char* const purposes[] = { "halls", "keep", "temple", "cellars", "crypt", "works", 0 };

void __fastcall__ setbank (unsigned char b)
{
    hy_bank (b);
}

unsigned char getbank (void)
{
    return *(volatile unsigned char*) 0;
}

/* A purpose (halls, keep ...; fortified or open after it: a keep's fortified unless it's open), or -1 */
int __fastcall__ purpose_of (const char* s)
{
    static char w[12];
    const char* sp = strchr (s, ' ');
    signed char p;
    unsigned char n = sp ? sp - s : strlen (s);

    if (n >= sizeof w) {
        return -1;
    }
    memcpy (w, s, n);
    w[n] = 0;
    if ((p = find_word (purposes, w)) < 0 || (sp && strcmp (sp, " fortified") && strcmp (sp, " open"))) {
        return -1;
    }
    return p | ((sp && !strcmp (sp, " fortified")) || (p == P_KEEP && !(sp && !strcmp (sp, " open"))) ? P_FORT : 0);
}

/* ---- A theme's lines */

static int rfd = -1;
static char rb[128], phys[160];
static unsigned char rpos, rlen, ahead;

int __fastcall__ lines_open (const char* path)
{
    rpos = rlen = ahead = 0;
    return (rfd = open (path, O_RDONLY)) < 0 ? -1 : 0;
}

void lines_close (void)
{
    if (rfd >= 0) {
        close (rfd);
    }
    rfd = -1;
}

/* A line of the file (its CR, LF gone): its length, or -1 at its end */
static int getphys (void)
{
    unsigned char n = 0;
    int c;

    for (;;) {
        if (rpos == rlen) {
            if ((c = read (rfd, rb, sizeof rb)) <= 0) {
                if (!n) {
                    return -1;
                }
                break;
            }
            rlen = c;
            rpos = 0;
        }
        c = rb[rpos++];
        if (c == '\n') {
            break;
        }
        if (c != '\r' && n < sizeof phys - 1) {
            phys[n++] = c;
        }
    }
    phys[n] = 0;
    return n;
}

static unsigned char __fastcall__ blank (const char* s)
{
    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    return !*s || *s == '#';
}

static char* __fastcall__ trim (char* s)
{
    char* e;

    while (*s == ' ' || *s == '\t') {
        ++s;
    }
    for (e = s + strlen (s); e > s && (e[-1] == ' ' || e[-1] == '\t'); --e) {
    }
    *e = 0;
    return s;
}

unsigned char __fastcall__ read_line (struct line* l, char* t)
{
    unsigned n;
    char* s;
    char* bar;

    for (;;) {                                  /* (Its first line: blanks and comments passed over) */
        if (!ahead && getphys () < 0) {
            return 0;
        }
        ahead = 0;
        if (!blank (phys) && *phys != ' ' && *phys != '\t') {
            break;
        }
    }
    strcpy (t, phys);
    n = strlen (t);
    while (getphys () >= 0) {                   /* (The lines that go on with it: its last value's words, or more
                                                **   values after a bar) */
        if (blank (phys)) {
            continue;
        }
        if (*phys != ' ' && *phys != '\t') {
            ahead = 1;
            break;
        }
        s = trim (phys);
        if (n + strlen (s) + 2 < LINE_TEXT) {
            if (*s != '|') {
                t[n++] = ' ';
            }
            strcpy (t + n, s);
            n += strlen (s);
        }
    }
    l->n = 0;
    if ((bar = strchr (t, '|')) != 0) {
        *bar = 0;
    }
    strncpy (l->key, trim (t), sizeof l->key - 1);
    l->key[sizeof l->key - 1] = 0;
    while (bar && l->n < MAXV) {
        s = bar + 1;
        if ((bar = strchr (s, '|')) != 0) {
            *bar = 0;
        }
        l->v[l->n++] = trim (s);
    }
    return 1;
}

/* ---- The data file: its index here, its text in banks of the program's own */

#define TGMAX           256
unsigned tg_lines;
unsigned char tg_key[TGMAX];
static unsigned tg_off[TGMAX];
static unsigned char tg_bank;
struct line D;
char Dt[LINE_TEXT];

int __fastcall__ tg_load (const char* path)
{
    static unsigned char b[255];                /* (A READ costs some 24K cycles from a card, whatever its size:
                                                **   255 bytes at a time, the index's 85 lines) */
    unsigned i, j, off, n, k;
    unsigned char was = getbank ();
    int fd = open (path, O_RDONLY), first;

    if (fd < 0 || read (fd, b, 5) != 5 || b[0] != 'T' || b[1] != 'G' || b[2] != 1 ||
        (tg_lines = b[3] | b[4] << 8) > TGMAX) {
        return -1;
    }
    for (i = 0; i < tg_lines; i += n / 3) {
        k = 3 * (tg_lines - i < 85 ? tg_lines - i : 85);
        if ((n = read (fd, b, k)) != k) {
            return -1;
        }
        for (j = 0; j < n; j += 3) {
            tg_key[i + j / 3] = b[j];
            tg_off[i + j / 3] = b[j + 1] | b[j + 2] << 8;
        }
    }
    if ((first = hy_banks_alloc (3)) < 0) {
        return -1;
    }
    tg_bank = first;
    for (off = 0; (n = read (fd, b, sizeof b)) > 0 && n <= sizeof b; off += n) {
        for (i = 0; i < n; i += k) {            /* (A read across a bank's edge: in two) */
            k = 0x2000 - (off + i) % 0x2000;
            if (k > n - i) {
                k = n - i;
            }
            setbank (tg_bank + (off + i) / 0x2000);
            memcpy ((char*) 0x8000 + (off + i) % 0x2000, b + i, k);
        }
    }
    setbank (was);
    close (fd);
    return 0;
}

void __fastcall__ tg_get (int i, struct line* l, char* text)
{
    unsigned off = tg_off[i];
    const char* p = (const char*) 0x8000 + off % 0x2000;
    unsigned char k, n, was = getbank ();

    setbank (tg_bank + off / 0x2000);
    l->key[0] = 0;
    l->n = n = *p++;
    for (k = 0; k < n && k < MAXV; ++k) {
        l->v[k] = text;
        while ((*text++ = *p++) != 0) {
        }
    }
    setbank (was);
}

int __fastcall__ tg_find (unsigned char key, const char* first, unsigned char k)
{
    static char t[160];
    unsigned i;
    unsigned char n, was;
    const char* p;

    for (i = 0; i < tg_lines; ++i) {
        if (tg_key[i] == key) {
            if (first) {                        /* (Its first value, there in its bank) */
                was = getbank ();
                setbank (tg_bank + tg_off[i] / 0x2000);
                p = (const char*) 0x8000 + tg_off[i] % 0x2000 + 1;
                for (n = 0; n < sizeof t - 1 && (t[n] = p[n]) != 0; ++n) {
                }
                t[n] = 0;
                setbank (was);
                if (strcmp (t, first)) {
                    continue;
                }
            }
            if (!k--) {
                return i;
            }
        }
    }
    return -1;
}

unsigned char __fastcall__ tg_count (unsigned char key, const char* first)
{
    unsigned char n = 0;

    while (tg_find (key, first, n) >= 0) {
        ++n;
    }
    return n;
}

unsigned char __fastcall__ data (unsigned char key, const char* first, unsigned char k)
{
    int i = tg_find (key, first, k);

    if (i < 0) {
        return 0;
    }
    tg_get (i, &D, Dt);
    return 1;
}

/* ---- The bestiary: the data's kinds (kinds.txt's) and their factions (factions.txt's) */

struct beast beasts[NBEASTS];
unsigned char nbeasts;
static const char st[] = "bnmx";

unsigned char __fastcall__ style_bits (const char* s)
{
    unsigned char m = 0;
    const char* c;

    for (; *s; ++s) {
        if ((c = strchr (st, *s)) != 0) {
            m |= 1 << (c - st);
        }
    }
    return m;
}

int __fastcall__ beast_of (const char* key)
{
    unsigned char k;

    for (k = 0; k < nbeasts; ++k) {
        if (!strcmp (beasts[k].key, key)) {
            return k;
        }
    }
    return -1;
}

void bestiary (void)
{
    static char fac[16][12];                    /* (factions.txt's, 15 at most: the history's peoples, 2026-10-29) */
    unsigned char k, nf = 0;
    struct beast* b;

    for (k = 0; k < 16 && data (TK_FACTION, 0, k); ++k) {
        strncpy (fac[nf++], D.v[0], 11);
    }
    for (k = 0; k < NBEASTS && data (TK_BEAST, 0, k); ++k) {
        b = &beasts[nbeasts++];
        strncpy (b->key, D.v[0], 7);
        b->styles = style_bits (D.v[1]);
        b->d0 = atoi (D.v[2]);
        b->d1 = atoi (D.v[3]);
        b->n0 = atoi (D.v[4]);
        b->n1 = atoi (D.v[5]);
        b->lairs = atoi (D.v[6]);
        for (b->faction = 0; b->faction < nf && strcmp (fac[b->faction], D.v[7]); ++b->faction) {
        }
    }
}
