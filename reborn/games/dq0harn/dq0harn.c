/*
** dq0harn.c - Dingus Quest Zero's HarnMaster referee: a task of the game's (dq0harn SEG ROLE, started by the UI),
** answering the rules' questions (rs.h's requests) in the hub's slots, with the C SDK's harn.h (the book's rules).
** It holds the characters' sheets; the rest of the game knows them only by what it answers: grades, conditions,
** effects, summaries and words.
**
** So far: RS_HELLO (its name, its round), RS_LOAD_PC (hmchar's file), RS_TEST (a purpose made a skill or an
** attribute, the circumstances' steps 5 points each), RS_DESCRIBE (the character's sheet; b 1: its short lines,
** for the map's panel, each 15 characters at most).
**
** The game's own choices in using the book (not the book's rules): a purpose names the skill (or attribute times 5)
** a GM would call for; a skill not opened is tested at its OML, as if opened on first use.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"

static harn_char pc;                            /* The character */
static char name[16], calling[20];

/* ---- Words into a reply */

static char* tp;                                /* Where the next goes, */
static char* tend;                              /*   and the end */

static void __fastcall__ put (const char* s)
{
    while (*s && tp < tend) {
        *tp++ = *s++;
    }
    *tp = 0;
}

static void __fastcall__ putn (int v)
{
    char b[8];

    put (itoa (v, b, 10));
}

/* ---- What's equipped (till the items layer: the keys of the tools taken up), and digging judged by it.  The
** book has no rules for digging: the game's own, a tool's feet a spell of work (ten minutes) at a marginal success,
** by what it's into (earth, stone, masonry); a critical success twice that, a marginal failure half, a critical
** failure none; bare hands dig only earth */

#define NTOOLS  4
static char tools[NTOOLS][12];

static const struct { char key[8]; unsigned char rate[3]; } digger[] = {
    { "hands", { 1, 0, 0 } }, { "shovel", { 3, 0, 0 } }, { "spade", { 3, 0, 0 } }, { "pick", { 2, 2, 1 } },
    { "mattock", { 3, 2, 1 } }, { "chisel", { 0, 1, 2 } },
};

static void __fastcall__ equip (const char* key, unsigned char on)
{
    unsigned char k;

    for (k = 0; k < NTOOLS; ++k) {
        if (on ? !tools[k][0] : !strcmp (tools[k], key)) {
            if (on) {
                strncpy (tools[k], key, sizeof tools[k] - 1);
            } else {
                tools[k][0] = 0;
            }
            return;
        }
    }
}

/* A tool equipped */
static unsigned char __fastcall__ has (const char* key)
{
    unsigned char k;

    for (k = 0; k < NTOOLS; ++k) {
        if (!strcmp (tools[k], key)) {
            return 1;
        }
    }
    return 0;
}

/* The best rate of what's equipped (and bare hands) for the material */
static unsigned char __fastcall__ dig_rate (unsigned char m)
{
    unsigned char k, t, best = digger[0].rate[m];

    for (k = 0; k < NTOOLS; ++k) {
        for (t = 1; t < sizeof digger / sizeof digger[0]; ++t) {
            if (tools[k][0] && !strcmp (tools[k], digger[t].key) && digger[t].rate[m] > best) {
                best = digger[t].rate[m];
            }
        }
    }
    return best;
}

/* ---- A purpose: the skill a GM would call for, or an attribute (an attribute | 0x80: tested times 5) */

#define ATTR    0x80
static const unsigned char purpose[P_THEME] = {
    HARN_AWA, HARN_AWA, HARN_AWA, HARN_AWA,                             /* Sight, sound, smell, search */
    HARN_STL, HARN_CLI, HARN_JMP, HARN_SWI, HARN_ACR, ATTR | HARN_STR, ATTR | HARN_STR,
    HARN_LCK, HARN_LCK,
    HARN_CND, ATTR | HARN_END, ATTR | HARN_WIL, HARN_DGE,
    HARN_PHY, HARN_LOR, HARN_TRK, HARN_RHE, HARN_RHE, HARN_ANI,
    HARN_MIN,                                                           /* Digging: Mining */
};

static signed char __fastcall__ grade (unsigned char level)
{
    static const signed char g[4] = { G_CF, G_MF, G_MS, G_CS };

    return g[level & 3];
}

static signed char __fastcall__ test (harn_char* c, unsigned char p, signed char steps)
{
    unsigned char sk;
    int mod = 5 * steps;

    if (p >= P_THEME) {
        return grade (harn_test (50 + mod));     /* (A theme's own: till themes say, an even chance) */
    }
    sk = purpose[p];
    if (sk & ATTR) {
        return grade (harn_test_attr (c, sk & ~ATTR, 5, mod));
    }
    if (!c->ml[sk]) {
        return grade (harn_test (harn_get_oml (c, sk) - 5 * harn_get_pp (c) + mod));
    }
    return grade (harn_test_skill (c, sk, mod));
}

/* ---- The character's state, in the hub's header: the UI's status line, and its summary (rs.h's: what the world
** needs often, its senses first: what it sees and hears is through them) */

/* A sense, 0-9, from its attribute (3-18 as rolled: 10 or 11 an average 4 or 5); what hurts it comes later (an
** eye's or a head's wounds, a helm) */
static unsigned char __fastcall__ sense (unsigned char a)
{
    unsigned v = pc.att[a];

    return v <= 3 ? 0 : v >= 18 ? 9 : (v - 3) * 3 / 5;
}

static void status (void)
{
    char b[8];
    struct rs_summary* s = &HUB->pcsum;
    unsigned char ep = harn_get_pp (&pc) - harn_get_up (&pc), k;

    strcpy (HUB->pcstatus, "UP ");
    strcat (HUB->pcstatus, utoa (harn_get_up (&pc), b, 10));
    s->cond = 0;
    s->health = H_FINE;
    k = harn_move (&pc);                       /* (Walking: half its Move, squares a round: Combat 6) */
    s->step = k < 4 ? 50 : 200 / k;
    s->attacks = 1;
    s->sight = sense (HARN_EYE);
    s->hearing = sense (HARN_HRG);
    s->smell = sense (HARN_SML);
    s->darkvision = 0;                          /* (The species' sight in the dark: the book's, to come) */
    s->nightvision = 1;
    s->morale = pc.att[HARN_MOR];
    s->load = ep > 3 ? 3 : ep;
    s->power = 10;
}

/* ---- hmchar's file: its own lines, the name and the occupation */

static void __fastcall__ own_line (char* l)
{
    if (!strncmp (l, "name ", 5)) {
        strncpy (name, l + 5, sizeof name - 1);
    } else if (!strncmp (l, "occupation ", 11)) {
        strncpy (calling, l + 11, sizeof calling - 1);
    }
}

static void __fastcall__ load_pc (struct req* r)
{
    static char path[64];
    int fd;

    strncpy (path, r->text, sizeof path - 1);
    r->result = 1;
    if ((fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    r->result = harn_read (fd, &pc, own_line) < 0;
    close (fd);
    put (name);
    put ("\n");
    put (calling);
    status ();
}

/* ---- The sheet */

static const unsigned char shown[] = {
    HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL,
    HARN_MOR
};

static void sheet (void)
{
    unsigned char k;

    put (name);
    put (", ");
    put (harn_species_name (pc.att[HARN_SP]));
    put (" ");
    put (harn_sex_name (pc.att[HARN_SX]));
    put (", ");
    put (calling);
    put ("; born ");
    putn (pc.att[HARN_BD] & 0xFF);
    put (" ");
    put (harn_month_name (pc.att[HARN_BD] >> 8));
    put (" (");
    put (harn_sunsign_name (pc.att[HARN_SS]));
    put (").\n");
    for (k = 0; k < sizeof shown; ++k) {
        put (harn_attr_name (shown[k]));
        put (" ");
        putn (pc.att[shown[k]]);
        put (k + 1 < sizeof shown ? ", " : ".\n");
    }
    put ("Endurance ");
    putn (harn_end (&pc));
    put (", Move ");
    putn (harn_move (&pc));
    put ("; universal penalty ");
    putn (harn_get_up (&pc));
    put (", physical ");
    putn (harn_get_pp (&pc));
    put (".\nSkills (EML):");
    for (k = 0; k < HARN_NSKILLS; ++k) {
        if (pc.ml[k]) {
            put (" ");
            put (harn_skill_name (k));
            put (" ");
            putn (harn_eml (&pc, k));
        }
    }
}

/* The short lines: the attributes in pairs, then endurance and the penalties, as they are now ("STR  3  STA  5") */
static const char abbr[12][4] = {
    "STR", "STA", "DEX", "AGL", "EYE", "HRG", "SML", "VOI", "INT", "AUR", "WIL", "MOR"
};

static void __fastcall__ pair (const char* a, int v, const char* b, int w)
{
    put (a);
    put (v < 10 ? "  " : " ");
    putn (v);
    put ("  ");
    put (b);
    put (w < 10 ? "  " : " ");
    putn (w);
    put ("\n");
}

static void brief (void)
{
    unsigned char k;

    for (k = 0; k < sizeof shown; k += 2) {
        pair (abbr[k], pc.att[shown[k]], abbr[k + 1], pc.att[shown[k + 1]]);
    }
    put ("END ");
    putn (harn_end (&pc));
    put ("\n");
    pair ("UP ", harn_get_up (&pc), "PP ", harn_get_pp (&pc));
}

/* ---- A request answered */

static void __fastcall__ answer (struct req* r)
{
    unsigned char k;

    tp = r->text;
    tend = r->text + REQ_TEXT - 1;
    r->result = 0;
    switch (r->kind) {
    case RS_HELLO:
        r->w1 = 100;                            /* (A round: 10 seconds, Combat 7) */
        put ("HarnMaster 3.5");
        break;
    case RS_LOAD_PC:
        load_pc (r);
        break;
    case RS_TEST:
        if (r->a == P_DIG) {                    /* (Judged by the tools equipped) */
            k = dig_rate (r->b > MAT_MASONRY ? MAT_STONE : r->b);
            if (!k) {
                r->result = 1;
                r->w1 = 0;
                break;
            }
            r->grade = test (&pc, r->a, r->steps);
            r->w1 = r->grade == G_CS ? 2 * k : r->grade == G_MS ? k : r->grade == G_MF ? k / 2 : 0;
            break;
        }
        if (r->a == P_SEARCH && r->b == HIDE_MAGICAL) {    /* (Only by magic: its spells to come) */
            r->result = 1;
            break;
        }
        if (r->a == P_PICKLOCK && !has ("picks")) {    /* (Lockcraft wants its tools: Skills 23) */
            r->result = 1;
            break;
        }
        r->grade = test (&pc, r->a, r->steps);
        break;
    case RS_EQUIP:
        equip (r->text, r->a);
        break;
    case RS_DESCRIBE:
        if (r->b == 1) {
            brief ();
        } else {
            sheet ();
        }
        break;
    default:
        r->result = 0xFF;                       /* (Not yet) */
        *tp = 0;
        break;
    }
}

int main (int argc, char* argv[])
{
    unsigned char s;

    if (argc < 3 || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    rseed (HUB->seed ^ 0x5245);                 /* (Its own dice: the game's seed, and its role) */
    for (;;) {
        hy_sem_acquire (HUB->sem[T_REFEREE]);   /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        for (s = 0; s < NSLOTS; ++s) {
            if (HUB->slot[s].state == REQ_POSTED) {
                answer (&HUB->slot[s]);
                HUB->slot[s].state = REQ_DONE;
            }
        }
    }
    return 0;
}
