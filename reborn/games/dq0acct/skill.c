/*
** skill.c - (dq0acct, overlay 2) what it earns and learns: the wages, the occupation's skills, the month's practice.
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")
#pragma bss-name ("OV2RODATA")
#pragma local-strings (on)
#pragma static-locals (on)

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0harn/dq0harn.h"
#include "../dq0harn/weapons.h"
#include "dq0acct.h"

/* The console's width, for the words */
void o_width (void)
{
    static char b[100];
    char* p;
    int fd, n = 0;

    if ((fd = open ("/dev/consctl", O_RDONLY)) >= 0) {
        n = read (fd, b, sizeof b - 1);
        close (fd);
    }
    b[n > 0 ? n : 0] = 0;
    for (p = b; p; p = strchr (p, '\n')) {
        if (*p == '\n') {
            ++p;
        }
        if (!strncmp (p, "size ", 5) && atoi (p + 5) >= 40) {
            width = atoi (p + 5) > 80 ? 79 : atoi (p + 5) - 1;
        }
    }
}

/* ---- The wages (the book's incomes, Character 13: a month's pence by the occupation's first letters; else by its class) */

static const struct wage {
    char            k[7];
    unsigned char   d;
} wages[] = {
    { "Apothe", 60 }, { "Chandl", 54 }, { "Charco", 54 }, { "Clothi", 60 }, { "Embalm", 48 }, { "Glassw", 66 }, { "Harper", 42 },
    { "Herald", 78 }, { "Hidewo", 60 }, { "Innkee", 60 }, { "Jewell", 66 }, { "Lexigr", 66 }, { "Litiga", 72 }, { "Locksm", 60 },
    { "Mason", 96 }, { "Metals", 72 }, { "Miller", 84 }, { "Miner", 84 }, { "Ostler", 78 }, { "Perfum", 66 }, { "Physic", 72 },
    { "Potter", 60 }, { "Salter", 48 }, { "Seaman", 48 }, { "Shipwr", 90 }, { "Tentma", 72 }, { "Timber", 78 }, { "Weapon", 108 },
    { "Woodcr", 66 }, { "Animal", 72 }, { "Cartog", 84 }, { "Cook", 30 }, { "Farm", 24 }, { "Fisher", 48 }, { "Herdsm", 24 },
    { "Hunter", 42 }, { "Labore", 42 }, { "Longsh", 42 }, { "Ratter", 60 }, { "Sage", 84 }, { "Scribe", 66 }, { "Servan", 24 },
    { "Teamst", 72 }, { "Thatch", 54 }, { "Toymak", 48 }
};
static const unsigned char class_wage[5] = { 24, 24, 42, 60, 72 };

unsigned char o_wage (void)
{
    unsigned char k;

    for (k = 0; k < sizeof wages / sizeof wages[0]; ++k) {
        if (!strncmp (calling, wages[k].k, strlen (wages[k].k))) {
            return wages[k].d;
        }
    }
    return class_wage[pc.social < 5 ? pc.social : 2];
}

/* ---- The occupation's skills (hmchar's occupations.txt: its line, each named skill that's open) */

static unsigned char trade[12], ntrade;

static unsigned char __fastcall__ same_ci (const char* a, const char* b, unsigned char n)
{
    for (; n; --n, ++a, ++b) {
        if ((*a | 0x20) != (*b | 0x20)) {
            return 0;
        }
    }
    return 1;
}

static void __fastcall__ trade_line (char* l)
{
    char* e;
    unsigned char k, n;

    if (!(l = strchr (l, '|')) || !(l = strchr (l + 1, '|'))) {
        return;
    }
    for (++l; *l && ntrade < sizeof trade; l = e) {
        while (*l == ' ' || *l == ',') {
            ++l;
        }
        e = l + strcspn (l, ",");
        n = strncmp (l, "scripts ", 8) ? strcspn (l, "/,") : 6;    /* ("scripts N": Script, its native's) */
        for (k = 0; k < HARN_NSKILLS; ++k) {
            if (strlen (harn_skills[k].name) == n && same_ci (harn_skills[k].name, l, n)) {
                if (pc.ml[k]) {
                    trade[ntrade++] = k;
                }
                break;
            }
        }
    }
}

static void trade_read (void)
{
    static char b[128], l[200];
    static unsigned char done;
    int fd, n, i;
    unsigned char k = 0, c = strlen (calling);

    if (done) {
        return;
    }
    done = 1;
    strcpy (path, lib);
    strcat (path, "/../hmchar/occupations.txt");
    if (!c || (fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    while (!ntrade && (n = read (fd, b, sizeof b)) > 0) {
        for (i = 0; i < n; ++i) {
            if (b[i] == '\n' || b[i] == '\r') {
                l[k] = 0;
                if (k > c && !strncmp (l, calling, c) && (l[c] == ' ' || l[c] == '|')) {
                    trade_line (l);
                    break;
                }
                k = 0;
            } else if (k < sizeof l - 1) {
                l[k++] = b[i];
            }
        }
    }
    close (fd);
}

/* ---- What's to hand to practise with (HOUSE.md's between dungeons): a weapon skill wants a weapon of its kind (the referee's own table, its key the kind's; a
** tool's no weapon to practise with, nor a torch; bows and slings the missiles'), Script and Drawing something to write or draw with (the book's, Skills
** 11), Lockcraft its lockpicks.  What's left after the money's matters, told a thing at a time (ans: dq0acct.c's tools_note) */

static unsigned char lacks[HARN_NSKILLS];

void o_tool (void)
{
    unsigned char i = ans[8], j;

    if (!i) {
        for (j = HARN_AXE; j <= HARN_WHP; ++j) {
            lacks[j] = 1;
        }
        lacks[HARN_SCR] = lacks[HARN_DRA] = lacks[HARN_LCK] = 1;
    } else if (i == KI_PAPER || i == KI_PARCHMENT || i == KI_QUILL || i == KI_CHARCOAL || i == KI_PENCIL) {
        lacks[HARN_SCR] = lacks[HARN_DRA] = 0;
    } else if (i == KI_CHALK) {
        lacks[HARN_DRA] = 0;
    } else if (i == KI_LOCKPICKS) {
        lacks[HARN_LCK] = 0;
    } else if (i >= KI_SHORTBOW && i <= KI_CROSSBOW) {
        lacks[HARN_BOW] = 0;
    } else if (i == KI_SLING) {
        lacks[HARN_SLG] = 0;
    } else if (ans[0] && (ans[9] & K_HAND) && i != KI_TORCH) {
        for (j = 0; j < sizeof weapons / sizeof weapons[0]; ++j) {
            if (weapons[j].part != TOOL && !strncmp (weapons[j].key, ans, sizeof weapons[0].key)) {
                lacks[weapons[j].sk] = 0;
                return;
            }
        }
    }
}

/* ---- What its calling or its practice needs, never sold unasked (the plan's step 11, HOUSE.md's between dungeons: the general
** kind of a broke scribe's quill): a thing (as o_tool's) a skill of its own is practised with, that skill its occupation's or one
** it's used since its last night's rest; its writing things if it knows a script at all.  The tools by their skills: what Script
** and Drawing write and draw with, Lockcraft's picks, Mining's picks and shovels, Masonry's chisels, a bow or a sling, a weapon of
** its skill's kind */

static const struct craft_tool {
    unsigned char   kind, sk;
} craft_tools[] = {
    { KI_CHALK, HARN_DRA }, { KI_LOCKPICKS, HARN_LCK }, { KI_PICK, HARN_MIN }, { KI_MATTOCK, HARN_MIN }, { KI_SHOVEL, HARN_MIN },
    { KI_SPADE, HARN_MIN }, { KI_CHISEL, HARN_MSN }, { KI_STONE_CHISEL, HARN_MSN }, { KI_SLING, HARN_SLG }, { KI_HERBS, HARN_HRB }
};

unsigned char o_needs (void)
{
    unsigned char i = ans[8], sk = 0xFF, k;

    if (i == KI_PAPER || i == KI_PARCHMENT || i == KI_QUILL || i == KI_CHARCOAL || i == KI_PENCIL || i == KI_INK) {
        for (k = 0; k < ntongues && tongue[k][0] != 's'; ++k) {
        }
        if (pc.ml[HARN_SCR] || k < ntongues) {
            return 1;                           /* (A script known: its writing things) */
        }
        sk = HARN_DRA;
    } else if (i >= KI_SHORTBOW && i <= KI_CROSSBOW) {
        sk = HARN_BOW;
    } else {
        for (k = 0; k < sizeof craft_tools / sizeof craft_tools[0]; ++k) {
            if (craft_tools[k].kind == i) {
                sk = craft_tools[k].sk;
            }
        }
        for (k = 0; sk == 0xFF && ans[0] && (ans[9] & K_HAND) && i != KI_TORCH && k < sizeof weapons / sizeof weapons[0]; ++k) {
            if (weapons[k].part != TOOL && !strncmp (weapons[k].key, ans, sizeof weapons[0].key)) {
                sk = weapons[k].sk;
            }
        }
    }
    if (sk == 0xFF || !pc.ml[sk]) {
        return 0;
    }
    if (used[sk]) {
        return 1;
    }
    trade_read ();
    for (k = 0; k < ntrade && trade[k] != sk; ++k) {
    }
    return k < ntrade;
}

/* ---- Its tongues and scripts past its own (the plan for the game between dungeons, its step 8: its sheet's "language NAME=ML" and
** "script NAME=ML" lines, sheet.c's tongue): each practised as a skill is, its number HARN_NSKILLS on (the j-th line's HARN_NSKILLS + j),
** its development roll the book's (Skills 7: d100 and its skill base over its ML, as harn_develop's), its skill base Language's or
** Script's; a script wants something to write with, as Script does (Skills 11), a tongue nothing */

static const char* __fastcall__ tongue_word (unsigned char j)
{
    return tongue[j] + (tongue[j][0] == 's' ? 7 : 9);
}

/* The tongue or script a word begins (n letters): its line's number, or 0xFF */
static unsigned char __fastcall__ tongue_named (const char* l, unsigned char n)
{
    unsigned char j;
    const char* t;

    for (j = 0; j < ntongues; ++j) {
        t = tongue_word (j);
        if (strcspn (t, "=") >= n && same_ci (t, l, n)) {
            return j;
        }
    }
    return 0xFF;
}

static char word[12];

static const char* __fastcall__ sk_name (unsigned char sk)
{
    const char* t;
    unsigned char k;

    if (sk < HARN_NSKILLS) {
        return harn_skills[sk].name;
    }
    t = tongue_word (sk - HARN_NSKILLS);
    for (k = 0; t[k] && t[k] != '=' && k < sizeof word - 1; ++k) {
        word[k] = t[k];
    }
    word[k] = 0;
    return word;
}

static unsigned char __fastcall__ ml_of (unsigned char sk)
{
    const char* t;

    if (sk < HARN_NSKILLS) {
        return pc.ml[sk];
    }
    t = strchr (tongue[sk - HARN_NSKILLS], '=');
    return t ? atoi (t + 1) : 0;
}

static void __fastcall__ grow (unsigned char sk)
{
    char* t;
    unsigned char ml, sb;

    if (sk < HARN_NSKILLS) {
        harn_develop (&pc, sk);
        return;
    }
    t = tongue[sk - HARN_NSKILLS];
    sb = harn_get_sb (&pc, *t == 's' ? HARN_SCR : HARN_LAN);
    ml = ml_of (sk);
    if (ml < 100 + sb && d100 () + sb > ml) {
        utoa (ml + 1, strchr (t, '=') + 1, 10);
    }
}

/* ---- Practice (the book's, Skills 7: 30 points a whole month, ten a roll; no weapon past 70, Skills 18; Script wants writing
** things, Skills 11; a weapon wants one of its kind, the house's: lacks, above) */

static unsigned char __fastcall__ can_practise (unsigned char sk)
{
    if (sk >= HARN_NSKILLS) {                   /* (A tongue or a script past its own, its line's) */
        sk -= HARN_NSKILLS;
        return sk < ntongues && strchr (tongue[sk], '=') && !(tongue[sk][0] == 's' && lacks[HARN_SCR]);
    }
    if (!pc.ml[sk] || sk == HARN_INI || sk == HARN_DGE || harn_skills[sk].group == HARN_RELIGION) {
        return 0;
    }
    if (sk >= HARN_AXE && sk <= HARN_WHP && pc.ml[sk] >= WEAPON_MAX) {
        return 0;
    }
    return !lacks[sk];
}

static unsigned char chosen[3], nchosen;

static void __fastcall__ choose (unsigned char sk)
{
    unsigned char k;

    for (k = 0; k < nchosen; ++k) {
        if (chosen[k] == sk) {
            return;
        }
    }
    if (nchosen < 3 && can_practise (sk)) {
        chosen[nchosen++] = sk;
    }
}

/* The answer filled in: the script it studied in the library first (the scholar's house's, the plan's step 12: its native one
** the Script skill, else its line's), then those it used most, then its occupation's, then its best */
static void choose_filled (void)
{
    unsigned char k, b, m;

    if (town_study[0]) {
        for (k = 0; k < ntongues && !(tongue[k][0] == 's' && !strncmp (tongue[k] + 7, town_study, strlen (town_study)) &&
            (!tongue[k][7 + strlen (town_study)] || tongue[k][7 + strlen (town_study)] == '=')); ++k) {
        }
        if (k < ntongues) {
            choose (strchr (tongue[k], '=') ? HARN_NSKILLS + k : HARN_SCR);
        }
    }
    while (nchosen < 3) {
        b = 0xFF;
        m = 0;
        for (k = 0; k < HARN_NSKILLS; ++k) {
            if (used[k] > m && can_practise (k) && !(chosen[0] == k && nchosen > 0) && !(chosen[1] == k && nchosen > 1)) {
                m = used[k];
                b = k;
            }
        }
        if (b == 0xFF) {
            break;
        }
        choose (b);
    }
    trade_read ();
    for (k = 0; k < ntrade; ++k) {
        choose (trade[k]);
    }
    while (nchosen < 3) {
        b = 0xFF;
        m = 0;
        for (k = 0; k < HARN_NSKILLS; ++k) {
            if (pc.ml[k] > m && can_practise (k) && !(chosen[0] == k && nchosen > 0) && !(chosen[1] == k && nchosen > 1)) {
                m = pc.ml[k];
                b = k;
            }
        }
        if (b == 0xFF) {
            break;
        }
        choose (b);
    }
}

/* The three said: each word the skill it begins (the first that fits), open and to be practised; 1 all understood */
static unsigned char __fastcall__ choose_said (char* l)
{
    unsigned char k, n, j;

    nchosen = 0;
    while (*l) {
        l += strspn (l, " ,.");
        if (!(n = strcspn (l, " ,."))) {
            break;
        }
        for (k = 0; k < HARN_NSKILLS; ++k) {
            if (can_practise (k) && strlen (harn_skills[k].name) >= n && same_ci (harn_skills[k].name, l, n)) {
                choose (k);
                break;
            }
        }
        j = 0xFF;
        if (k == HARN_NSKILLS && (j = tongue_named (l, n)) != 0xFF && can_practise (HARN_NSKILLS + j)) {
            choose (HARN_NSKILLS + j);          /* (A tongue or a script of its own past its native ones) */
        } else if (k == HARN_NSKILLS) {
            l[n] = 0;
            for (k = 0; k < HARN_NSKILLS && !(lacks[k] && pc.ml[k] && strlen (harn_skills[k].name) >= n && same_ci (harn_skills[k].name, l, n)); ++k) {
            }
            if (j != 0xFF && tongue[j][0] == 's' && lacks[HARN_SCR]) {
                k = 0;                          /* (A script, nothing to write with) */
            }
            say (k < HARN_NSKILLS ? "You have nothing to practise " : "You can't practise ");
            say (l);
            say (k < HARN_NSKILLS ? " with.\n" : ".\n");
            return 0;
        }
        l += n;
    }
    return 1;
}

void o_work (void)
{
    unsigned char n, k, sk, was_ml[3];
    unsigned char ns;

    if (!work_days) {
        return;
    }
    say ("For ");
    sayn (work_days);
    say (" of the days you work");
    if (calling[0]) {
        say (" as a ");
        say (calling);
    }
    say (", your keep found, for half a master's wage (");
    amount (earned);
    say (").");
    /* (The book's bonus practice, Skills 7, for work under a better master: 2 points a day, eight hours' work, on 24 days of 30) */
    n = work_days * 4 / 25;
    trade_read ();
    if (n && ntrade) {
        ns = ntrade > 3 ? 3 : ntrade;           /* (Its first three: its trade's own) */
        for (k = 0; k < ns; ++k) {
            was_ml[k] = pc.ml[trade[k]];
        }
        for (k = 0; k < n; ++k) {
            sk = trade[k % ns];
            if (pc.ml[sk] < MASTER_ML) {
                harn_develop (&pc, sk);
            }
        }
        say (" The work teaches you:");
        for (k = 0; k < ns; ++k) {
            sk = trade[k];
            say (" ");
            say (harn_skills[sk].name);
            say (" ");
            if (pc.ml[sk] > was_ml[k]) {
                sayn (was_ml[k]);
                say (" to ");
                sayn (pc.ml[sk]);
            } else {
                say ("no better");
            }
            say (k + 1 < ns ? "," : ".");
        }
    }
    say ("\n");
}

void o_practice (void)
{
    unsigned char mon = passed / 30, k, sk, was_ml[3], tries = 0;
    unsigned r;

    mon = mon > town_taught ? mon - town_taught : 0;    /* (A month with a master is the month's practice: the book's, Skills
                                                **   11, a new script's 30 points, a tongue's whole month) */
    if (!mon) {
        return;
    }
    choose_filled ();
    if (!nchosen) {
        return;
    }
    say ("In the spare hours you could practise ");
    for (k = 0; k < nchosen; ++k) {
        say (sk_name (chosen[k]));
        say (k + 2 < nchosen ? ", " : k + 2 == nchosen ? " and " : ". ");
    }
    say ("Enter takes them, or name your three");
    for (;;) {
        ask (ans, sizeof ans);
        if (!ans[0] || ++tries > 5) {
            break;
        }
        if (choose_said (ans)) {
            choose_filled ();
            break;
        }
        nchosen = 0;
        choose_filled ();
        say ("Enter takes the ones you've got, or name your three");
    }
    for (k = 0; k < nchosen; ++k) {
        was_ml[k] = ml_of (chosen[k]);
    }
    for (r = 0; r < 3 * mon; ++r) {             /* (Three rolls a whole month, one after another: Skills 7) */
        if (can_practise (chosen[r % nchosen])) {
            grow (chosen[r % nchosen]);
        }
    }
    say ("You practise for ");
    sayn (mon);
    say (mon == 1 ? " month (" : " months (");
    sayn (3 * mon);
    say (" rolls): ");
    for (k = 0; k < nchosen; ++k) {
        sk = chosen[k];
        say (sk_name (sk));
        say (" ");
        if (ml_of (sk) > was_ml[k]) {
            sayn (was_ml[k]);
            say (" to ");
            sayn (ml_of (sk));
        } else {
            say ("no better");
        }
        say (k + 1 < nchosen ? ", " : ".\n");
    }
}
