/*
** harnfile.c - harn.h's characters in files: a text, a line a thing, that harn_write writes and harn_read reads
** back (and a person can read and change):
**
**     harn 1                       (the format)
**     att sp 0                     an attribute, by harn.dl's key, and its value
**     fam 2 1 3 4 0 3 0            culture, social class, sibling rank, family size, parent, estrangement, clanhead
**     look 1 4 2                   complexion, hair, eyes
**     med 20 12                    medical traits
**     psy 11 0                     a mental disorder and its severity (a line each)
**     sexuality 0
**     ml cli 36                    a skill's mastery level, by its key (the open ones)
**     vol 0 0 0 0 0                fatigue levels, bloodloss, development points, state, shock's healing rate
**     inj 14 1 2 2 0 0             an injury: location, aspect, severity, level, healing rate, flags
**
** Any other line is the program's: harn_read gives it to a function of the program's (a name, an occupation ...),
** and the program writes its own with harn_write_line.  A program's own skills go by their numbers ("ml 91 40").
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <harn.h>

static const char* const keys[HARN_NATTRS] = {
    "sp", "sx", "bd", "ss", "ht", "fr", "owt", "wt", "cml", "str", "sta", "dex", "agl", "eye", "hrg", "sml", "voi",
    "int", "aur", "wil", "mor", "dty", "pty", "end"
};

const char* __fastcall__ harn_attr_key (unsigned char a)
{
    return a < HARN_NATTRS ? keys[a] : "";
}

/* ---- Writing */

static int wfd;
static char wb[64];
static unsigned char wn;

static void __fastcall__ put (const char* s)
{
    while (*s) {
        if (wn == sizeof wb) {
            write (wfd, wb, wn);
            wn = 0;
        }
        wb[wn++] = *s++;
    }
}

static void __fastcall__ num (unsigned v)
{
    char b[8];

    put (" ");
    put (utoa (v, b, 10));
}

static void wflush (void)
{
    if (wn) {
        write (wfd, wb, wn);
        wn = 0;
    }
}

void __fastcall__ harn_write_line (int fd, const char* line)
{
    wfd = fd;
    put (line);
    put ("\n");
    wflush ();
}

int __fastcall__ harn_write (int fd, const harn_char* c)
{
    unsigned char k;
    const harn_injury* j;

    wfd = fd;
    wn = 0;
    put ("harn 1\n");
    for (k = 0; k < HARN_END; ++k) {
        put ("att ");
        put (keys[k]);
        num (c->att[k]);
        put ("\n");
    }
    put ("fam");
    num (c->culture);
    num (c->social);
    num (c->sibrank);
    num (c->famsize);
    num (c->parent);
    num (c->estrange);
    num (c->clanhead);
    put ("\nlook");
    num (c->complexion);
    num (c->hair);
    num (c->eyes);
    put ("\nmed");
    for (k = 0; k < HARN_MEDMAX && c->med[k]; ++k) {
        num (c->med[k]);
    }
    put ("\n");
    for (k = 0; k < HARN_PSYMAX && c->psy[k]; ++k) {
        put ("psy");
        num (c->psy[k]);
        num (c->psysev[k]);
        put ("\n");
    }
    put ("sexuality");
    num (c->sexuality);
    put ("\n");
    for (k = 0; k < HARN_MAXSKILLS; ++k) {
        if (c->ml[k]) {
            put ("ml ");
            if (k < HARN_NSKILLS) {
                put (harn_skill_key (k));
            } else {
                num (k);
            }
            num (c->ml[k]);
            put ("\n");
        }
    }
    put ("vol");
    num (c->fl);
    num (c->bl);
    num (c->dp);
    num (c->state);
    num (c->shockhr);
    put ("\n");
    for (k = 0; k < c->ninj; ++k) {
        j = &c->inj[k];
        put ("inj");
        num (j->loc);
        num (j->aspect);
        num (j->sev);
        num (j->il);
        num (j->hr);
        num (j->flags);
        put ("\n");
    }
    wflush ();
    return 0;
}

/* ---- Reading */

/* The numbers after a line's key, into v (up to n): how many */
static unsigned char __fastcall__ nums (char* s, unsigned* v, unsigned char n)
{
    unsigned char k = 0;

    while (k < n) {
        while (*s == ' ') {
            ++s;
        }
        if (*s < '0' || *s > '9') {
            break;
        }
        v[k++] = atoi (s);
        while (*s >= '0' && *s <= '9') {
            ++s;
        }
    }
    return k;
}

/* A skill by its key, or a number (a program's own): the skill, or 0xFF */
static unsigned char __fastcall__ skill_of (const char* key, unsigned char len)
{
    unsigned char k;
    const char* s;

    if (*key >= '0' && *key <= '9') {
        return atoi (key);
    }
    for (k = 0; k < HARN_NSKILLS; ++k) {
        s = harn_skill_key (k);
        if (strlen (s) == len && !strncmp (s, key, len)) {
            return k;
        }
    }
    return 0xFF;
}

static void __fastcall__ line (harn_char* c, char* l, void (*other) (char* line))
{
    static unsigned v[8];
    char* rest = strchr (l, ' ');
    unsigned char n, k, len;
    harn_injury* j;

    if (!rest) {
        rest = l + strlen (l);
    }
    len = rest - l;
    if (len == 3 && !strncmp (l, "att", 3)) {
        while (*rest == ' ') {
            ++rest;
        }
        for (k = 0; k < HARN_END; ++k) {
            n = strlen (keys[k]);
            if (!strncmp (rest, keys[k], n) && rest[n] == ' ') {
                c->att[k] = atoi (rest + n);
                return;
            }
        }
    } else if (len == 3 && !strncmp (l, "fam", 3) && nums (rest, v, 7) == 7) {
        c->culture = v[0], c->social = v[1], c->sibrank = v[2], c->famsize = v[3];
        c->parent = v[4], c->estrange = v[5], c->clanhead = v[6];
    } else if (len == 4 && !strncmp (l, "look", 4) && nums (rest, v, 3) == 3) {
        c->complexion = v[0], c->hair = v[1], c->eyes = v[2];
    } else if (len == 3 && !strncmp (l, "med", 3)) {
        n = nums (rest, v, HARN_MEDMAX);
        for (k = 0; k < HARN_MEDMAX; ++k) {
            c->med[k] = k < n ? v[k] : HARN_MED_NONE;
        }
    } else if (len == 3 && !strncmp (l, "psy", 3) && nums (rest, v, 2) == 2) {
        for (k = 0; k < HARN_PSYMAX && c->psy[k]; ++k) {
        }
        if (k < HARN_PSYMAX) {
            c->psy[k] = v[0];
            c->psysev[k] = v[1];
        }
    } else if (len == 9 && !strncmp (l, "sexuality", 9) && nums (rest, v, 1)) {
        c->sexuality = v[0];
    } else if (len == 2 && !strncmp (l, "ml", 2)) {
        while (*rest == ' ') {
            ++rest;
        }
        for (n = 0; rest[n] && rest[n] != ' '; ++n) {
        }
        k = skill_of (rest, n);
        if (k < HARN_MAXSKILLS && nums (rest + n, v, 1)) {
            c->ml[k] = v[0];
        }
    } else if (len == 3 && !strncmp (l, "vol", 3) && nums (rest, v, 5) == 5) {
        c->fl = v[0], c->bl = v[1], c->dp = v[2], c->state = v[3], c->shockhr = v[4];
    } else if (len == 3 && !strncmp (l, "inj", 3) && nums (rest, v, 6) == 6 && c->ninj < HARN_INJMAX) {
        j = &c->inj[c->ninj++];
        j->loc = v[0], j->aspect = v[1], j->sev = v[2], j->il = v[3], j->hr = v[4], j->flags = v[5];
    } else if (len == 4 && !strncmp (l, "harn", 4)) {
        return;
    } else if (other) {
        other (l);
    }
}

int __fastcall__ harn_read (int fd, harn_char* c, void (*other) (char* line))
{
    static char b[128], l[100];
    int n, i;
    unsigned char k = 0, any = 0;

    memset (c, 0, sizeof *c);
    while ((n = read (fd, b, sizeof b)) > 0) {
        for (i = 0; i < n; ++i) {
            if (b[i] == '\n' || b[i] == '\r') {
                if (k) {
                    l[k] = 0;
                    line (c, l, other);
                    any = 1;
                }
                k = 0;
            } else if (k < sizeof l - 1) {
                l[k++] = b[i];
            }
        }
    }
    if (k) {
        l[k] = 0;
        line (c, l, other);
        any = 1;
    }
    return any ? 0 : -1;
}
