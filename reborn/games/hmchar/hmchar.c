/*
** hmchar.c - a HârnMaster character made, as the book's Character article makes one (harn.h), and written to a file
** (harn_write's text) for a game, Dingus Quest Zero's or any other, to read.  Standalone or run by a game.
**
** hmchar [-s seed] [-o file] [-q]
**   -s seed    the same character each time
**   -o file    where it's written (default: NAME.hm, in the current directory, NAME the character's)
**   -q         no questions: the first roll kept, an occupation, weapons and option points chosen at random, a
**              name made up
**
** The steps (Character 1): the attributes rolled (kept, or rolled again); an occupation, its skills opened as the
** Occupational Skills table has them (Character 15; a few of the book's, those of use in a dungeon; "three weapons
** at OML + SB" chosen); the five option points spent (Character 16: a skill opened at its OML for one, or improved
** by its SB for one, a second time for two more); a name.  Not here: the family's occupation and its skills, contacts,
** the veterans' points.
*/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <harn.h>

static harn_char h;
static char name[24];
static unsigned char quiet, occ;

/* ---- Output and input, through fds 0 and 1 (no stdio: harn.h's tables and stdio leave too little room) */

static char ob[128];
static unsigned char on;

static void flush (void)
{
    if (on) {
        write (1, ob, on);
        on = 0;
    }
}

static void __fastcall__ s (const char* t)
{
    while (*t) {
        if (on == sizeof ob) {
            flush ();
        }
        ob[on++] = *t++;
    }
}

static void __fastcall__ err (const char* t)
{
    write (2, t, strlen (t));
}

static void __fastcall__ n (int v)
{
    char b[8];

    s (itoa (v, b, 10));
}

static void __fastcall__ named (const char* label, int v, const char* after)
{
    s (label);
    s (" ");
    n (v);
    s (after);
}

/* A line typed, trimmed, lower case: its length */
static unsigned char __fastcall__ ask (const char* q, char* b, unsigned char max)
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
    return k;
}

/* ---- The occupations (Character 15): each skill and its OML's multiple; HARN_NSKILLS + n: three weapons at
** OML + SB times n; the script, native (70 + SB) */

#define WEAPONS     0x80                        /* (Three weapons at OML + SB times the low bits) */
struct occupation {
    const char*     name;
    unsigned char   years;
    unsigned char   sk[9][2];                   /* Skill, its OML's multiple (0: the skill's own OML) */
};
static const struct occupation occs[] = {
    { "Gladiator", 4, { { HARN_INI, 6 }, { WEAPONS | 2, 0 }, { HARN_UNA, 6 }, { HARN_PHY, 2 } } },
    { "Hunter/Trapper", 5, { { HARN_TRK, 5 }, { WEAPONS | 1, 0 }, { HARN_STL, 4 }, { HARN_FOR, 4 }, { HARN_SUR, 4 },
                             { HARN_HID, 3 }, { HARN_HUN, 3 }, { HARN_FLE, 2 } } },
    { "Thief", 3, { { HARN_LEG, 4 }, { HARN_AWA, 5 }, { HARN_STL, 4 }, { HARN_NTR, 4 }, { HARN_LCK, 3 },
                    { HARN_CLU, 5 }, { HARN_DAG, 4 }, { HARN_ACR, 3 } } },
    { "Physician", 7, { { HARN_PHY, 4 }, { HARN_HRB, 3 }, { HARN_ALC, 3 }, { HARN_SCR, 0 } } },
    { "Miner", 6, { { HARN_MIN, 4 }, { HARN_ENG, 3 }, { HARN_WOO, 3 }, { HARN_MET, 2 }, { HARN_JEW, 1 } } },
    { "Sage", 5, { { HARN_FOL, 5 }, { HARN_MAT, 2 }, { HARN_SCR, 0 } } },
};
#define NOCC    (sizeof occs / sizeof occs[0])

static const unsigned char weapons[] = {
    HARN_AXE, HARN_BOW, HARN_CLU, HARN_DAG, HARN_FLA, HARN_NET, HARN_POL, HARN_SHD, HARN_SLG, HARN_SPR, HARN_SWD,
    HARN_WHP, HARN_BLO
};

/* A skill named (case ignored, a beginning of its name enough if only one has it): it, or 0xFF */
static unsigned char __fastcall__ skill_named (const char* w, const unsigned char* among, unsigned char count)
{
    unsigned char k, sk, found = 0xFF, len = strlen (w);
    const char* nm;

    if (!len) {
        return 0xFF;
    }
    for (k = 0; k < count; ++k) {
        sk = among ? among[k] : k;
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

static void three_weapons (unsigned char times)
{
    static char b[40];
    unsigned char got = 0, k, sk;

    if (!quiet) {
        s (times == 1 ? "Three weapon skills, opened at their OML plus SB. The weapons:" :
            "Three weapon skills, opened at their OML plus twice SB. The weapons:");
        for (k = 0; k < sizeof weapons; ++k) {
            s (k ? ", " : " ");
            s (harn_skill_name (weapons[k]));
        }
        s (".\n");
    }
    while (got < 3) {
        if (quiet) {
            sk = weapons[random (sizeof weapons)];
        } else {
            s (got == 0 ? "The first: " : got == 1 ? "The second: " : "The third: ");
            ask ("", b, sizeof b);
            sk = skill_named (b, weapons, sizeof weapons);
            if (sk >= 0xFE) {
                s (sk == 0xFE ? "Which? More than one begins so.\n" : "That's not one of them.\n");
                continue;
            }
        }
        if (h.ml[sk] && h.ml[sk] > harn_get_oml (&h, sk)) {
            if (!quiet) {
                s ("That one's chosen already.\n");
            }
            continue;
        }
        h.ml[sk] = harn_get_oml (&h, sk) + times * harn_get_sb (&h, sk);
        ++got;
    }
}

/* The occupation's skills: an automatic skill listed takes the occupation's OML, not the two added (Character 16) */
static void occupation (void)
{
    const struct occupation* o;
    static char b[8];
    unsigned char k, sk, m;

    if (quiet) {
        occ = random (NOCC);
    } else {
        s ("\nAn occupation (Character 15):\n");
        for (k = 0; k < NOCC; ++k) {
            s ("  ");
            n (k + 1);
            s (" ");
            s (occs[k].name);
            named (",", occs[k].years, " years\n");
        }
        do {
            ask ("Which (1-6)? ", b, sizeof b);
            occ = atoi (b) - 1;
        } while (occ >= NOCC);
    }
    o = &occs[occ];
    for (k = 0; k < 9 && (o->sk[k][0] | o->sk[k][1]); ++k) {    /* (An empty place is 0, 0: Acrobatics is skill 0) */
        sk = o->sk[k][0];
        m = o->sk[k][1];
        if (sk & WEAPONS) {
            three_weapons (sk & 7);
        } else {
            h.ml[sk] = m ? m * harn_get_sb (&h, sk) : harn_get_oml (&h, sk);
        }
    }
}

/* Five option points: a skill opened at OML for one; an open skill improved by its SB for one, the second time for
** two more (Character 16) */
static void option_points (void)
{
    static unsigned char improved[HARN_NSKILLS];
    static char b[40];
    unsigned char op = 5, sk, cost;

    if (!quiet) {
        s ("\nFive option points: a skill's name opens it at its OML (1 point), or improves an open one by its SB\n"
           "(1 point; the second time 2 more). Enter alone keeps what's left.\n");
    }
    while (op) {
        if (quiet) {
            sk = random (HARN_NSKILLS);
            if (sk >= HARN_RAGR && sk <= HARN_RSIE) {
                continue;                       /* (Not a ritual: the character has no deity) */
            }
        } else {
            named ("Points left:", op, ". A skill: ");
            if (!ask ("", b, sizeof b)) {
                return;
            }
            sk = skill_named (b, 0, HARN_NSKILLS);
            if (sk >= 0xFE) {
                s (sk == 0xFE ? "Which? More than one begins so.\n" : "There's no such skill.\n");
                continue;
            }
        }
        cost = !h.ml[sk] ? 1 : improved[sk] == 0 ? 1 : improved[sk] == 1 ? 2 : 0xFF;
        if (cost > op) {
            if (!quiet) {
                s (cost == 0xFF ? "It can be improved only twice.\n" : "Not enough points left for that.\n");
            }
            continue;
        }
        if (!h.ml[sk]) {
            harn_open_skill (&h, sk);
        } else {
            h.ml[sk] += harn_get_sb (&h, sk);
            ++improved[sk];
        }
        op -= cost;
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
    if (occ < NOCC) {
        s (", ");
        s (occs[occ].name);
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
    named ("Piety", h.att[HARN_PTY], "; medical:");
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

/* (The occupation into the file, by name, a line of hmchar's own) */
int main (int argc, char* argv[])
{
    static char b[40], path[64], line[48];
    const char* out = 0;
    unsigned char i;
    int fd;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            rseed (strtoul (argv[++i], 0, 10));
        } else if (!strcmp (argv[i], "-o") && i + 1 < argc) {
            out = argv[++i];
        } else if (!strcmp (argv[i], "-q")) {
            quiet = 1;
        } else {
            err ("usage: hmchar [-s seed] [-o file] [-q]\n");
            return 1;
        }
    }
    occ = 0xFF;
    for (;;) {
        harn_create (&h, 0);
        if (quiet) {
            break;
        }
        profile ();
        ask ("\nKeep this character, or roll again (k or r)? ", b, sizeof b);
        if (b[0] == 'k' || b[0] == 'K') {
            break;
        }
    }
    occupation ();
    option_points ();
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
    strcat (line, occs[occ].name);
    harn_write_line (fd, line);
    close (fd);
    s ("Written to ");
    s (out);
    s ("\n");
    flush ();
    return 0;
}
