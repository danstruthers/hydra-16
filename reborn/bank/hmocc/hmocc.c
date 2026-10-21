/*
** hmocc.c - (a bank program: sdk/c/include/bankprog.h) hmchar's culture, deity and occupation (games/hmchar): a
** human's culture rolled (Character 3), or -u's; the deity the player's choice (Character 12); then the occupation,
** one of the book's that occupations.txt has (hmchar's lib/hmchar: the Occupational Skills table, Character 15; a
** soldier's unit by the character's culture, Character 27; a cleric's church by its deity, Character 23, the church's
** deity made the character's if it has none), those the character's class and sex allow it (the user's, 2026-10-20:
** the book's limits), chosen by its number or its name, or -c's, or -q's at random; its skills opened as the book has
** them (the weapons or crafts it gives chosen); or the player's own (hmchar's spend: 11 points, the user's,
** 2026-10-20).  The file's read a line at a time each time it's wanted, none of it kept; hmchar's main RAM hasn't
** the room for this (harn.h's tables take most of it), its bank has.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include "../../games/hmchar/hmchar.h"

#define same(a, b)  (!strnicmp ((a), (b), 255))  /* (Two names the same, case ignored: strnicmp, which hmchar has) */

static const unsigned char weapons[] = {
    HARN_AXE, HARN_BOW, HARN_CLU, HARN_DAG, HARN_FLA, HARN_NET, HARN_POL, HARN_SHD, HARN_SLG, HARN_SPR, HARN_SWD,
    HARN_WHP, HARN_BLO
};

/* t said after a space, or on a line of its own (indented) if it would pass the 78th column */
static void __fastcall__ w (const char* t)
{
    s (col + strlen (t) > 77 ? "\n  " : " ");
    s (t);
}

/* ---- occupations.txt: a line each, in groups by kind */

#define O_COUNT     0                           /* (scan's: those offered counted, and the menu said (talk)) */
#define O_LIST      1                           /*   a group listed, its kind's letter want's */
#define O_FIND      2                           /*   one found, numbered want, or (want 0) by wname's beginning */
#define O_TAKE      3                           /*   the one found (occname) said, and its skills taken (take) */

static int fd;
static char ib[64], back;                       /* (The file's bytes read, a byte put back) */
static unsigned char ii, in;
static char kind[12], nm[20], yr[4], tk[24];    /* The group's kind ("soldier"); a line's name, years; a field */
static unsigned char sect, sdty;                /* The group offered (1), its deity (a church's; else none) */
static unsigned char occk, occkind, occdty;     /* The occupation's number (0: the player's own), kind's letter, deity */
static unsigned char want, take, talk, cell;
static const char* wname;                       /* (O_FIND's name: -c's, or typed) */
static unsigned char pend_n, pend_m, pend_w;    /* Weapons (pend_w) or crafts to choose: pend_n, at pend_m */

/* The file opened: 0, or -1 */
static signed char lib_open (void)
{
    static char p[64];

    strcat (strcpy (p, libdir), "/occupations.txt");
    ii = in = back = 0;
    return (fd = open (p, O_RDONLY)) < 0 ? -1 : 0;
}

/* The file's next byte (its CRs passed over), or 0 at its end */
static char next (void)
{
    char c;
    int r;

    if ((c = back)) {
        back = 0;
        return c;
    }
    do {
        if (ii == in) {
            if ((r = read (fd, ib, sizeof ib)) <= 0) {
                return 0;
            }
            in = r;
            ii = 0;
        }
        c = ib[ii++];
    } while (c == '\r');
    return c;
}

/* A line's next field into f (to a | or a comma, or the line's end), its spaces trimmed: what ended it (| , \n, or
** 0: the file's end) */
static char __fastcall__ field (char* f, unsigned char size)
{
    unsigned char k = 0;
    char c;

    while ((c = next ()) && c != '|' && c != ',' && c != '\n') {
        if (k < size - 1 && (k || c != ' ')) {
            f[k++] = c;
        }
    }
    while (k && f[k - 1] == ' ') {
        --k;
    }
    f[k] = 0;
    return c;
}

/* The line's rest said (a title: a long one broken at a space past the 68th column), or passed over */
static void __fastcall__ rest (unsigned char say)
{
    char c;

    while ((c = next ()) && c != '\n') {
        if (say) {
            if (c == ' ' && col > 68) {
                s ("\n  ");
            } else {
                ch (c);
            }
        }
    }
}

/* Whether a header's word (its kind, or a qualifier) lets the character have the group: a deity's name, if the
** character's is that deity or it has none (sdty: the deity); a culture's, if it's the character's; a class's (Guilded,
** Noble), if the character's is that or higher (a slave's, a serf's and an unguilded character's the unguilded
** occupations: the user's, 2026-10-20, the book's limits); a sex's, if it's the character's (the Lady's); any other
** (soldier, Unguilded), yes.  -c's occupation (a game's or a test's choice) isn't held to the class or the sex */
static unsigned char __fastcall__ admits (const char* w)
{
    unsigned char k, d = h.att[HARN_DTY];

    for (k = 0; k < HARN_NODEITY; ++k) {
        if (same (w, harn_deity_name (k))) {
            sdty = k;
            return d == HARN_NODEITY || d == k;
        }
    }
    for (k = 0; k <= HARN_SINDARIN_C; ++k) {
        if (same (w, harn_culture_name (k))) {
            return h.culture == k;
        }
    }
    for (k = HARN_GUILDED; k <= HARN_NOBLE; ++k) {
        if (same (w, harn_class_name (k))) {
            return cname || h.social >= k;
        }
    }
    for (k = HARN_F; k <= HARN_M; ++k) {
        if (same (w, harn_sex_name (k))) {
            return cname || h.att[HARN_SX] == k;
        }
    }
    return 1;
}

/* A group's header, [KIND] or [KIND QUALIFIER ...], its [ read: its kind, and whether it's offered (sect: every word
** of it admits the character), the deity (sdty: a church's, else none): the byte after it (a space: a title
** follows) */
static char header (void)
{
    unsigned char k = 0;
    char c, *f = kind;

    sdty = HARN_NODEITY;
    sect = 1;
    do {
        if ((c = next ()) && c != ']' && c != '\n' && c != ' ') {
            if (k < 11) {
                f[k++] = c;
            }
            continue;
        }
        f[k] = 0;                               /* (A word's end: the kind, then each qualifier in tk) */
        if (!admits (f)) {
            sect = 0;
        }
        f = tk;
        k = 0;
    } while (c && c != ']' && c != '\n');
    return c == ']' ? next () : c;
}

static void brk (void)
{
    if (col) {
        ch ('\n');
    }
    cell = 0;
}

/* A kind's numbers said: from first to last */
static void __fastcall__ upto (unsigned char first, unsigned char last)
{
    if (last != first) {
        ch ('-');
        n (last);
    }
    ch ('\n');
}

/* A skill of an occupation's (tk's): SKILL/N, SKILL (at its OML), Ritual/N (the deity's), or weapons N/M or crafts N/M
** (chosen after: pend_*); a skill listed twice, the higher */
static void skill_token (void)
{
    char* sl = tk;
    unsigned char m = 0, sk, v;

    while (*sl && *sl != '/') {
        ++sl;
    }
    if (*sl) {
        *sl = 0;
        m = atoi (sl + 1);
    }
    if (!strnicmp (tk, "weapons ", 8) || !strnicmp (tk, "crafts ", 7)) {
        pend_w = (tk[0] | 0x20) == 'w';
        pend_n = atoi (tk + 7 + pend_w);
        pend_m = m;
        return;
    }
    if (same (tk, "Ritual")) {
        if (h.att[HARN_DTY] == HARN_NODEITY) {
            return;
        }
        sk = HARN_RAGR + h.att[HARN_DTY];
    } else if ((sk = skill_named (tk, 0, 0, HARN_NSKILLS)) >= 0xFE) {
        err ("hmchar: occupations.txt: no skill ");
        err (tk);
        err ("\n");
        return;
    }
    v = m ? m * harn_get_sb (&h, sk) : harn_get_oml (&h, sk);
    if (h.ml[sk] < v) {
        h.ml[sk] = v;
    }
}

/* The occupations offered (sect's), each in turn, by mode (O_COUNT ...): O_COUNT's how many there are (not the
** player's own), O_LIST's how many it listed, O_FIND's how many it found (2: more than one begins so) */
static unsigned char __fastcall__ scan (unsigned char mode)
{
    unsigned char k = 0, got = 0, first = 0, len = 0, exact = 0, all, say, apply;
    char c, last = 0;

    if (mode == O_FIND && !want) {
        len = strlen (wname);
    }
    if (lib_open ()) {
        return 0;
    }
    while ((c = next ())) {
        if (c == '[') {
            c = header ();
            if ((say = c == ' ' && mode == O_LIST && sect && kind[0] == want)) {
                brk ();
            }
            if (c && c != '\n') {
                rest (say);
            }
            if (say) {
                ch ('\n');
            }
            continue;
        }
        if (c == ';') {
            rest (0);
        }
        if (c == ';' || c == '\n') {
            continue;
        }
        back = c;
        if ((c = field (nm, sizeof nm)) == '|') {
            c = field (yr, sizeof yr);
        }
        all = same (nm, "all");
        say = apply = 0;
        if (sect && nm[0]) {
            k += !all;
            switch (mode) {
            case O_COUNT:
                if (!all && talk && kind[0] != last) {
                    if (last) {
                        upto (first, k - 1);
                    }
                    s ("  ");
                    s (kind);
                    named ("", k, "");
                    first = k;
                    last = kind[0];
                }
                break;
            case O_LIST:
                if (kind[0] == want) {
                    if (all) {
                        brk ();
                        say = 1;
                    } else {
                        if (cell == 3) {
                            brk ();
                        }
                        while (col < cell * 26) {
                            ch (' ');
                        }
                        s (k < 10 ? "   " : "  ");
                        n (k);
                        ch (' ');
                        s (nm);
                        ++cell;
                        ++got;
                    }
                }
                break;
            case O_FIND:
                if (!all && (want ? k == want : !exact && !strnicmp (nm, wname, len))) {
                    exact = !want && !nm[len];
                    if (exact || !got++) {
                        occk = k;
                        occkind = kind[0];
                        occdty = sdty;
                        strcpy (occname, nm);
                    }
                    if (exact) {
                        got = 1;
                    }
                }
                break;
            default:
                if (kind[0] == occkind && (all || !strcmp (nm, occname))) {
                    say = !quiet;
                    apply = take;
                }
            }
            if (say) {
                if (all) {
                    s ("Every ");
                    s (kind);
                    ch (':');
                } else {
                    s (nm);
                    s (", ");
                    s (yr);
                    s (" years:");
                }
            }
        }
        while (c == '|' || c == ',') {
            c = field (tk, sizeof tk);
            if (say) {
                w (tk);
                if (c == ',') {
                    ch (',');
                }
            }
            if (apply) {
                skill_token ();
            }
        }
        if (say) {
            ch ('\n');
        }
    }
    close (fd);
    if (mode == O_COUNT) {
        if (talk) {
            if (last) {
                upto (first, k);
            }
            named ("  your own", k + 1, "\n");
        }
        return k;
    }
    if (mode == O_LIST) {
        brk ();
    }
    return got;
}

/* ---- The weapons or crafts an occupation gives the character to choose */

static const char* const counts[] = { "One", "Two", "Three" };
static const char* const nths[] = { "first", "second", "third" };
static const char* const times[] = { "", "", "twice ", "three times " };

/* pend_n weapons (pend_w) or crafts, each opened at its OML plus SB times pend_m (a weapon), or at SB times pend_m (a
** craft) */
static void choose (void)
{
    unsigned char got = 0, k, sk, v, count = sizeof weapons;
    const unsigned char* among = weapons;

    if (!pend_w) {
        among = 0;
        count = HARN_NSKILLS - HARN_AGR;
    }
    if (!quiet) {
        s (counts[pend_n - 1]);
        s (pend_w ? " weapon skill" : " craft");
        s (pend_n > 1 ? "s, opened at " : ", opened at ");
        if (pend_w) {
            s ("their OML");
            if (pend_m) {
                s (" plus ");
            }
        }
        if (pend_m || !pend_w) {
            s (times[pend_m]);
            s (pend_w ? "SB" : "their SB");
        }
        s (pend_w ? ". The weapons:" : ". The crafts:");
        for (k = 0; k < count; ++k) {
            w (harn_skill_name (among ? among[k] : HARN_AGR + k));
            ch (k + 1 < count ? ',' : '.');
        }
        ch ('\n');
    }
    while (got < pend_n) {
        if (quiet) {
            k = random (count);
            sk = among ? among[k] : HARN_AGR + k;
        } else {
            s ("The ");
            s (nths[got]);
            ask (": ", ab, sizeof ab);
            sk = skill_named (ab, among, HARN_AGR, count);
            if (sk >= 0xFE) {
                s (sk == 0xFE ? "Which? More than one begins so.\n" : "That's not one of them.\n");
                continue;
            }
        }
        v = pend_m * harn_get_sb (&h, sk) + (pend_w ? harn_get_oml (&h, sk) : 0);
        if (h.ml[sk] >= v) {                    /* (Chosen already: its ML that, or more) */
            if (!quiet) {
                s ("That one's chosen already.\n");
            }
            continue;
        }
        h.ml[sk] = v;
        ++got;
    }
}

/* ---- The culture (Character 3: by birthplace, most of Harn's Feudal, the common exceptions Imperial, Viking and
** Tribal): a human's rolled (the user's, 2026-10-20: mostly Feudal; 01-70 Feudal, 71-80 Imperial, 81-90 Viking, 91-00
** Tribal), or -u's (a test's: the same character each time); an elf's Sindarin and a dwarf's Khuzan, harn_create's.
** A culture not harn_create's (its Feudal) has its own social class rolled (the book's table by culture), and so the
** native tongue's OML (by class) again.  0; or 1, -u's not one of the four (a beginning of the name enough) */
unsigned char culture (void)
{
    unsigned char c = 0xFF, r;

    if (uname) {
        for (c = HARN_TRIBAL; c <= HARN_IMPERIAL && (!*uname || strnicmp (harn_culture_name (c), uname, strlen (uname)));
            ++c) {
        }
        if (c > HARN_IMPERIAL) {
            return 1;
        }
    }
    if (h.att[HARN_SP] == HARN_HUM) {
        if (c == 0xFF) {
            r = d100 ();
            c = r <= 70 ? HARN_FEUDAL : r <= 80 ? HARN_IMPERIAL : r <= 90 ? HARN_VIKING : HARN_TRIBAL;
        }
        if (c != HARN_FEUDAL) {
            h.culture = c;
            h.social = harn_rand_class (c);
            h.ml[HARN_LAN] = harn_get_oml (&h, HARN_LAN);
        }
    }
    return 0;
}

/* ---- The deity (Character 12): the player's choice (the user's, 2026-10-20), one of the ten or none (-q's none), the
** book's Morality for each shown as a guide; its ritual skill opened, as harn_create opens a religious character's
** (Skills 12).  A cleric's church (the occupation, after) is its deity's, or any of the ten for one with none */
static const unsigned char morals[] = { 3, 13, 5, 16, 5, 16, 8, 18, 3, 7, 3, 10, 8, 18, 5, 16, 8, 18, 11, 18 };

static void deity (void)
{
    static char b[20];
    unsigned char d, got, len;

    s ("\nA deity (Character 12): the Morality each favours, a guide (yours is ");
    n (h.att[HARN_MOR]);
    s ("):");
    for (d = 0; d < HARN_NODEITY; ++d) {        /* ("Agrik 3-13," ... "Siem 11-18.") */
        strcat (strcpy (b, harn_deity_name (d)), " ");
        itoa (morals[d * 2], b + strlen (b), 10);
        strcat (b, "-");
        itoa (morals[d * 2 + 1], b + strlen (b), 10);
        strcat (b, d + 1 < HARN_NODEITY ? "," : ".");
        w (b);
    }
    ch ('\n');
    for (;;) {
        if (!(len = ask ("Which (a beginning enough; Enter alone, none)? ", ab, sizeof ab))) {
            return;
        }
        got = 0;
        for (d = 0; d <= HARN_NODEITY; ++d) {   /* (None too) */
            if (!strnicmp (harn_deity_name (d), ab, len)) {
                got = got ? 0xFF : d + 1;
            }
        }
        if (got && got != 0xFF) {
            break;
        }
        s (got ? "Which? More than one begins so.\n" : "There's no such deity.\n");
    }
    if (--got != HARN_NODEITY) {
        h.att[HARN_DTY] = got;
        harn_open_skill (&h, got += HARN_RAGR);
        s (harn_skill_name (got));
        named (" is", h.ml[got], "\n");
    }
}

/* ---- The occupation: one of the book's, chosen (by its number or its name, a group listed by its kind's first
** letter, ? and one its skills shown), or -c's, or -q's at random (one draw, -c or not); its skills opened (an
** automatic skill listed takes the occupation's OML, not the two added: Character 16); or the player's own.  The
** deity first (Character 12), asked of the player */
unsigned char occupation (void)
{
    unsigned char count, show, got;
    char* p;
    int k;

    if (!quiet) {
        deity ();
    }
    wname = cname;
    talk = !quiet && !cname;
    if (talk) {
        s ("\nAn occupation (Character 15; not yet a mage's, Character 25: theirs come with magic):\n");
    }
    if (!(count = scan (O_COUNT))) {
        return 2;
    }
    want = 0;
    if (quiet) {
        want = random (count) + 1;
    }
    if (cname) {
        want = 0;
        if (scan (O_FIND) != 1) {
            return 1;
        }
    } else if (quiet) {
        scan (O_FIND);
    } else {
        s ("A group's first letter lists it; a number or a name chooses one (a beginning\n"
           "enough), and ? before one shows its skills.\n");
        for (;;) {
            ask ("Which? ", ab, sizeof ab);
            show = ab[0] == '?';
            for (p = ab + show; *p == ' '; ++p) {
            }
            if (!*p) {
                continue;
            }
            want = *p | 0x20;
            if (!show && !p[1] && want >= 'a' && want <= 'z' && scan (O_LIST)) {
                continue;
            }
            got = 1;
            occk = 0;                           /* (The player's own) */
            if (*p >= '0' && *p <= '9') {
                k = atoi (p);
                if (k != count + 1) {
                    want = k;
                    got = k > 0 && k <= count ? scan (O_FIND) : 0;
                }
            } else if (strnicmp ("your own", p, strlen (p))) {
                want = 0;
                wname = p;
                got = scan (O_FIND);
            }
            if (got != 1) {
                s (got ? "Which? More than one begins so.\n" : "There's no such occupation.\n");
                continue;
            }
            if (!show) {
                break;
            }
            if (occk) {
                take = 0;
                scan (O_TAKE);
            } else {
                s ("Your own: 11 points spent as option points are, three in one skill (its primary).\n");
            }
        }
    }
    if (!occk) {
        spend (11, 1);
        while (!ask ("\nYour occupation's name: ", occname, sizeof occname)) {
        }
        return 0;
    }
    if (occdty != HARN_NODEITY && h.att[HARN_DTY] == HARN_NODEITY) {   /* (A church: its deity the character's) */
        h.att[HARN_DTY] = occdty;
        harn_open_skill (&h, HARN_RAGR + occdty);
    }
    take = 1;
    pend_n = 0;
    scan (O_TAKE);
    if (pend_n) {
        choose ();
    }
    return 0;
}
