/*
** hmchar.c - a HârnMaster character made, as the book's Character article makes one (harn.h), and written to a file
** (harn_write's text) for a game, Dingus Quest Zero's or any other, to read.  Standalone or run by a game.
**
** hmchar [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]
**   -s seed    the same character each time
**   -o file    where it's written (default: NAME.hm, in the current directory, NAME the character's)
**   -q         no questions: the first roll kept, an occupation (one of the book's), weapons and option points chosen
**              at random, a name made up
**   -p species the character's species, not rolled (Character 2's 97 in 100 human): human, elf (Sindarin) or
**              dwarf (Khuzdul), by its first letter (h, e or s, d or k); the book's changes to its attributes, its
**              culture and the rest made by harn_create
**   -c occupation  the occupation, by its name (case ignored, a beginning enough), not asked or drawn (-q's draw made
**              all the same: the rest of the character as it would be), any the character's culture has (not held to
**              its class or its sex: a game's or a test's choice)
**   -u culture a human's culture, not rolled (a test's, so its character is the same each time): feudal, imperial,
**              viking or tribal (a beginning enough); an elf's and a dwarf's their own
**
** The steps (Character 1): the attributes rolled, a human's culture too (Character 3: mostly Feudal, the user's,
** 2026-10-20), kept, or rolled again; a deity, the player's choice (Character 12: one of the ten or none, the book's
** Morality for each a guide; -q's none); an occupation, any of the book's that occupations.txt has that the
** character's class and sex allow it (the user's, 2026-10-20, the book's limits: a slave's, a serf's or an unguilded
** character's the unguilded occupations, a guilded one's those and the guilded, a noble's all; the soldiers and
** clerics any class's, but a knight a noble; a Lady a woman), the Occupational Skills table's (Character 15), a
** soldier's unit by the character's culture (Character 27), a cleric's church by its deity (Character 23, a church
** making its deity the character's if it has none; not yet a mage's, Character 25, whose skills come with magic), its
** skills opened as the book has them (the weapons or crafts it gives chosen); or the player's own occupation (open to
** any; the user's, 2026-10-20): 11 points (the book's occupations' median, a point opening a skill at its OML and a
** point for each SB more), two in a skill at most and three in one, its primary; then the five option points spent
** (Character 16: a skill opened at its OML for one, or improved by its SB for one; but no more than two points in one
** skill, the user's: so improved once at most; ? lists the skills, the open ones' ML and the others' OML); a name.
** Not here: the family's occupation and its skills, contacts, the veterans' points.
**
** Its lib/hmchar (in its own directory, as rc's $prog has it, else the card's /sd/0/games/lib/hmchar): the
** occupations (occupations.txt), and the culture's, the deity's and the occupation's steps, a bank program (hmocc.bp:
** bank/hmocc, bankprog.h; this main RAM hasn't the room for them, harn.h's tables taking most of it), with hmchar's
** exports for it (hmchar.exp).
*/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <bankprog.h>
#include "hmchar.h"

harn_char h;
static harn_char t;
static char name[24];
char ab[40], occname[20], libdir[48];
unsigned char quiet;
const char* cname;
const char* uname;

/* ---- Output and input, through fds 0 and 1 (no stdio: harn.h's tables and stdio leave too little room) */

static char ob[128];
static unsigned char on;
unsigned char col;

void flush (void)
{
    if (on) {
        write (1, ob, on);
        on = 0;
    }
}

void __fastcall__ ch (char c)
{
    if (on == sizeof ob) {
        flush ();
    }
    ob[on++] = c;
    col = c == '\n' ? 0 : col + 1;
}

void __fastcall__ s (const char* t)
{
    while (*t) {
        ch (*t++);
    }
}

/* t said after a space, or on a line of its own (indented) if it would pass the 78th column */
void __fastcall__ err (const char* t)
{
    write (2, t, strlen (t));
}

void __fastcall__ n (int v)
{
    char b[8];

    s (itoa (v, b, 10));
}

void __fastcall__ named (const char* label, int v, const char* after)
{
    s (label);
    s (" ");
    n (v);
    s (after);
}

/* A line typed, trimmed, lower case: its length */
unsigned char __fastcall__ ask (const char* q, char* b, unsigned char max)
{
    unsigned char k;

    char c;

    s (q);
    flush ();
    k = 0;
    while (read (0, &c, 1) == 1 && c != '\n' && c != '\r') {
        if (k < max - 1) {
            b[k++] = c;
        }
    }
    while (k && b[k - 1] == ' ') {
        --k;
    }
    b[k] = 0;
    col = 0;
    return k;
}

/* A skill named (case ignored, a beginning of its name enough if only one has it), among count (among's, or skills
** base on): it, 0xFE (more than one begins so) or 0xFF */
unsigned char __fastcall__ skill_named (const char* w, const unsigned char* among, unsigned char base,
                                        unsigned char count)
{
    unsigned char k, sk, found = 0xFF, len = strlen (w);
    const char* nm;

    if (!len) {
        return 0xFF;
    }
    for (k = 0; k < count; ++k) {
        sk = among ? among[k] : base + k;
        nm = harn_skill_name (sk);
        if (!strnicmp (nm, w, len)) {
            if (strlen (nm) == len) {
                return sk;                      /* (Exactly: it) */
            }
            if (found != 0xFF) {
                return 0xFE;                    /* (Two have that beginning) */
            }
            found = sk;
        }
    }
    return found;
}

/* The skills listed (? at the points' prompt): the open ones at their ML; the others (but the rituals: the character
** has no deity, or its own's open) at the OML they'd open at; three to a line */
static void list_skills (void)
{
    unsigned char k, pass, col, v, w;
    const char* nm;

    for (pass = 0; pass < 2; ++pass) {
        s (pass ? "The others, at the OML they would open at:\n" : "Your skills, at their ML:\n");
        col = 0;
        for (k = 0; k < HARN_NSKILLS; ++k) {
            if ((h.ml[k] == 0) != pass || (pass && k >= HARN_RAGR && k <= HARN_RSIE)) {
                continue;
            }
            nm = harn_skill_name (k);
            v = pass ? harn_get_oml (&h, k) : h.ml[k];
            s ("  ");
            s (nm);
            for (w = strlen (nm) + 3 - (v < 10) - (v < 100); w < 20; ++w) {    /* (Its number at the 20th column) */
                s (" ");
            }
            n (v);
            if (++col == 3) {
                s ("\n");
                col = 0;
            }
        }
        if (col) {
            s ("\n");
        }
    }
}

/* Points spent, op of them (Character 16's option points; or own, the player's own occupation's): a skill's name opens
** it at its OML for one, or improves an open one by its SB for one; no more than two points in a skill (an option
** point's: an open skill has had one, its opening; the user's, the book's second improvement, two more, would make
** three), but three in one skill, the player's own occupation's primary */
void __fastcall__ spend (unsigned char op, unsigned char own)
{
    static unsigned char pts[HARN_NSKILLS];
    unsigned char sk, primary = !own;

    for (sk = 0; sk < HARN_NSKILLS; ++sk) {
        pts[sk] = !own && h.ml[sk];
    }
    if (!quiet) {
        s (own ? "\nYour own occupation, 11 points: a skill's name opens it at its OML (1 point),\n"
                 "or improves an open one by its SB (1 point); two points in a skill at most, but\n"
                 "three in one, its primary. ? lists them. Enter alone keeps what's left.\n" :
            "\nFive option points: a skill's name opens it at its OML (1 point), or improves an open one by its SB\n"
            "(1 point), no more than two points in a skill. ? lists them. Enter alone keeps what's left.\n");
    }
    while (op) {
        if (quiet) {
            sk = random (HARN_NSKILLS);
        } else {
            named ("Points left:", op, ". A skill: ");
            if (!ask ("", ab, sizeof ab)) {
                return;
            }
            if (ab[0] == '?' && !ab[1]) {
                list_skills ();
                continue;
            }
            sk = skill_named (ab, 0, 0, HARN_NSKILLS);
            if (sk >= 0xFE) {
                s (sk == 0xFE ? "Which? More than one begins so.\n" : "There's no such skill.\n");
                continue;
            }
        }
        if (sk >= HARN_RAGR && sk <= HARN_RSIE && (quiet || sk != HARN_RAGR + h.att[HARN_DTY])) {
            if (!quiet) {
                s ("Not your deity's ritual.\n");   /* (-q's draws none: its dice as they were) */
            }
            continue;
        }
        if (pts[sk] >= 2 && (primary || pts[sk] > 2)) {
            if (!quiet) {
                s ("No more than two points in a skill");
                if (own) {
                    s (", or three in one");
                }
                s (": that one's had its share.\n");
            }
            continue;
        }
        if (pts[sk]++ == 2) {
            primary = 1;
        }
        if (!h.ml[sk]) {
            harn_open_skill (&h, sk);
        } else {
            h.ml[sk] += harn_get_sb (&h, sk);
        }
        --op;
        if (!quiet) {
            s (harn_skill_name (sk));
            named (" is", h.ml[sk], "\n");
        }
    }
}

/* ---- The profile */

static const unsigned char attrs[] = {
    HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL,
    HARN_MOR
};

static void profile (void)
{
    unsigned char k, col = 0, w;

    s ("\n");
    if (name[0]) {
        s (name);
        s (", ");
    }
    s (harn_species_name (h.att[HARN_SP]));
    s (" ");
    s (harn_sex_name (h.att[HARN_SX]));
    s (", born ");
    n (h.att[HARN_BD] & 0xFF);
    s (" ");
    s (harn_month_name (h.att[HARN_BD] >> 8));
    s (" (");
    s (harn_sunsign_name (h.att[HARN_SS]));
    s ("); ");
    s (harn_culture_name (h.culture));
    s (" ");
    s (harn_class_name (h.social));
    if (occname[0]) {
        s (", ");
        s (occname);
    }
    s ("\n");
    n (h.sibrank);
    s (" of ");
    n (h.famsize);
    s ("; ");
    s (harn_parent_name (h.parent));
    s (", ");
    s (harn_estrangement_name (h.estrange));
    s ("; clanhead: ");
    s (harn_clanhead_name (h.clanhead));
    s ("\n");
    n (h.att[HARN_HT]);
    s ("\", ");
    s (harn_frame_name (h.att[HARN_FR]));
    s (", ");
    n (h.att[HARN_WT]);
    s (" lb (size ");
    n (harn_size (&h));
    s ("); ");
    named (harn_comeliness_name (h.att[HARN_CML]), h.att[HARN_CML], "; ");
    s (harn_complexion_name (h.complexion));
    s (", ");
    s (harn_hair_name (h.hair));
    s (" hair, ");
    s (harn_eye_name (h.eyes));
    s (" eyes\n");
    for (k = 0; k < sizeof attrs; ++k) {
        named (harn_attr_name (attrs[k]), h.att[attrs[k]], k == 7 || k + 1 == sizeof attrs ? "\n" : ", ");
    }
    named ("Endurance", harn_end (&h), ", ");
    named ("Move", harn_move (&h), ", ");
    named ("Piety", h.att[HARN_PTY], "");
    if (h.att[HARN_DTY] != HARN_NODEITY) {      /* (The deity worshipped) */
        s (" (");
        s (harn_deity_name (h.att[HARN_DTY]));
        s (")");
    }
    s ("; medical:");
    for (k = 0; k < HARN_MEDMAX && h.med[k]; ++k) {
        s (" ");
        s (harn_medical_name (h.med[k]));
    }
    s (h.med[0] ? "\n" : " none\n");
    s ("Skills (ML):");
    for (k = 0; k < HARN_NSKILLS; ++k) {
        if (h.ml[k]) {
            w = strlen (harn_skill_name (k)) + 5;
            if (col + w > 78) {
                s ("\n ");
                col = 1;
            }
            s (" ");
            named (harn_skill_name (k), h.ml[k], "");
            col += w;
        }
    }
    s ("\n");
}

/* ---- Names (-q's): two syllables of a few, Hârnic-sounding enough */

static void make_name (void)
{
    static const char* const a[] = { "Al", "Bra", "Dor", "Ka", "Ly", "Mo", "Ne", "Ro", "Sa", "Tor", "Wy", "Ha" };
    static const char* const b[] = { "dan", "ric", "nel", "gyn", "wen", "lor", "vik", "mar", "seth", "ka", "ven" };

    strcpy (name, a[random (sizeof a / sizeof a[0])]);
    strcat (name, b[random (sizeof b / sizeof b[0])]);
}

/* -p's species, by its first letter (h a human; e or s an elf, Sindarin; d or k a dwarf, Khuzdul): it, or 0xFF */
static unsigned char __fastcall__ species (const char* w)
{
    switch (w[0] | 0x20) {
    case 'h':
        return HARN_HUM;
    case 'e': case 's':
        return HARN_SIN;
    case 'd': case 'k':
        return HARN_KHU;
    }
    return 0xFF;
}

/* ---- The program: its lib/hmchar (its directory's, as rc's $prog has it, if it's there; else the card's), its bank
** program (the culture, the deity and the occupation, bank/hmocc's) loaded; the character made; its file written (the
** occupation by name, a line of hmchar's own) */

static char exppath[56], bppath[56];

static int usage (void)
{
    err ("usage: hmchar [-s seed] [-o file] [-q] [-p species] [-c occupation] [-u culture]\n");
    return 1;
}

static char* __fastcall__ in (char* p, const char* name)
{
    return strcat (strcat (strcpy (p, libdir), "/"), name);
}

int main (int argc, char* argv[])
{
    static char path[64], line[48];
    const char* out = 0;
    unsigned char i, sp = 0xFF;
    struct bank_fn *occ, *cul;
    char* e;
    int fd;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            rseed (strtoul (argv[++i], 0, 10));
        } else if (!strcmp (argv[i], "-o") && i + 1 < argc) {
            out = argv[++i];
        } else if (!strcmp (argv[i], "-q")) {
            quiet = 1;
        } else if (!strcmp (argv[i], "-c") && i + 1 < argc) {
            cname = argv[++i];
        } else if (!strcmp (argv[i], "-u") && i + 1 < argc) {
            uname = argv[++i];                  /* (hmocc's culture: one of the four, or the usage) */
        } else if (!strcmp (argv[i], "-p") && i + 1 < argc && (sp = species (argv[++i])) != 0xFF) {
        } else {
            return usage ();
        }
    }
    fd = open ("/env/prog", O_RDONLY);
    i = 0;
    if (fd >= 0) {
        i = read (fd, libdir, 30);
        close (fd);
    }
    libdir[i < 31 ? i : 0] = 0;
    e = strrchr (libdir, '/');
    strcpy (e ? e + 1 : libdir, "lib/hmchar");
    if ((fd = open (in (path, "hmocc.bp"), O_RDONLY)) < 0) {
        strcpy (libdir, "/sd/0/games/lib/hmchar");
    } else {
        close (fd);
    }
    bank_setup (in (exppath, "hmchar.exp"), 0x60, 16);
    if (!(occ = bank_fn (in (bppath, "hmocc.bp"), "occupation")) || !(cul = bank_fn (bppath, "culture"))) {
        err ("hmchar: its bank program (lib/hmchar/hmocc.bp) can't be loaded\n");
        return 1;
    }
    harn_template (&t);                         /* (A species asked for, the rest rolled) */
    t.att[HARN_SP] = sp != 0xFF ? sp : HARN_ANY;
    for (;;) {
        harn_create (&h, &t);
        if (bank_call (cul, unsigned char (*)(void))) {
            return usage ();
        }
        if (quiet) {
            break;
        }
        profile ();
        ask ("\nKeep this character, or roll again (k or r)? ", ab, sizeof ab);
        if (ab[0] == 'k' || ab[0] == 'K') {
            break;
        }
    }
    if ((i = bank_call (occ, unsigned char (*)(void)))) {
        flush ();
        err (i == 1 ? "hmchar: -c: no such occupation, or more than one begins so\n" :
            "hmchar: no lib/hmchar/occupations.txt\n");
        return 1;
    }
    spend (5, 0);
    if (quiet) {
        make_name ();
    } else {
        while (!ask ("\nA name: ", name, sizeof name)) {
        }
    }
    profile ();

    if (!out) {
        strcpy (path, name);
        strcat (path, ".hm");
        out = path;
    }
    if ((fd = open (out, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        flush ();
        err ("hmchar: ");
        err (out);
        err (": can't write it\n");
        return 1;
    }
    harn_write (fd, &h);
    strcpy (line, "name ");
    strcat (line, name);
    harn_write_line (fd, line);
    strcpy (line, "occupation ");
    strcat (line, occname);
    harn_write_line (fd, line);
    close (fd);
    s ("Written to ");
    s (out);
    s ("\n");
    flush ();
    return 0;
}
