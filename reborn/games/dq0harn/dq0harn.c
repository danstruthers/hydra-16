/*
** dq0harn.c - Dingus Quest Zero's HarnMaster referee: a task of the game's (dq0harn SEG ROLE DIR, started by the UI),
** answering the rules' questions (rs.h's requests) in the hub's slots, with the C SDK's harn.h (the book's rules).
** It holds the characters' sheets; the rest of the game knows them only by what it answers: grades, conditions,
** effects, summaries and words.
**
** So far: RS_HELLO (its name, its round), RS_LOAD_PC (hmchar's file), RS_TEST (a purpose made a skill or an
** attribute, the circumstances' steps 5 points each), RS_HARM (a fall, a trap's strike, a burn, a blow: injuries on
** the sheet), RS_DESCRIBE (the character's sheet, its injuries with it; b 1: its short lines, for the map's panel,
** each 15 characters at most; b 2: the last harm's words), RS_SAVE (the sheet, its injuries and all).  The sheet's
** words and the harm are overlay 1's (sheet.c: DIR/dq0harn.ovl, read into a bank of its own as it starts).
**
** The game's own choices in using the book (not the book's rules): a purpose names the skill (or attribute times 5)
** a GM would call for; a skill not opened is tested at its OML, as if opened on first use.
*/

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

harn_char pc;                                   /* The character */
char name[16], calling[20];

/* ---- Words into a reply (from the overlay too: s may be in the window, where the hub is, so each character's
** taken with s's bank in, and put with the hub's) */

char* tp;                                       /* Where the next goes, */
char* tend;                                     /*   and the end */

void __fastcall__ put (const char* s)
{
    unsigned char b = getbank (), h, c;

    hub_map (0);
    h = getbank ();
    for (;;) {
        setbank (b);
        c = *s++;
        setbank (h);
        if (!c || tp >= tend) {
            break;
        }
        *tp++ = c;
    }
    *tp = 0;
    setbank (b);
}

void __fastcall__ putn (int v)
{
    char b[8];

    put (itoa (v, b, 10));
}

/* ---- What's to hand (the keys of the tools the character carries, all of them: the UI's, from the things), and
** digging judged by it: a skill used, the one versed in it takes out the tool that suits the job best (the user's: a
** shovel in earth, a pick in stone; a sword digs earth too, poorly, and dulls).  The book has no rules for digging:
** the game's own, a tool's feet a spell of work (ten minutes) at a marginal success, by what it's into (earth, stone,
** masonry); a critical success twice that, a marginal failure half, a critical failure none; bare hands dig only
** earth; and the wear a spell does the tool */

#define NTOOLS  12
static char tools[NTOOLS][8];

static const struct { char key[8]; unsigned char rate[3], wear[3]; } digger[] = {
    { "hands", { 1, 0, 0 }, { 0, 0, 0 } }, { "shovel", { 3, 0, 0 }, { 0, 0, 0 } }, { "spade", { 3, 0, 0 }, { 0, 0, 0 } },
    { "pick", { 2, 2, 1 }, { 0, 1, 1 } }, { "mattock", { 3, 2, 1 }, { 0, 1, 1 } }, { "chisel", { 0, 1, 2 }, { 0, 1, 1 } },
    { "blade", { 1, 0, 0 }, { 40, 0, 0 } }, { "knife", { 1, 0, 0 }, { 40, 0, 0 } },
};

/* All that's to hand, anew: the keys in the text, a space between */
static void __fastcall__ equip_all (const char* keys)
{
    unsigned char k = 0, n;

    memset (tools, 0, sizeof tools);
    while (*keys && k < NTOOLS) {
        for (n = 0; keys[n] && keys[n] != ' '; ++n) {
        }
        if (n && n < sizeof tools[0]) {
            memcpy (tools[k++], keys, n);
        }
        keys += n;
        while (*keys == ' ') {
            ++keys;
        }
    }
}

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

/* The tool to hand that suits the material best (bare hands, digger's 0, if none does better; of two as good, the
** one it wears less): its digger's number */
static unsigned char __fastcall__ dig_tool (unsigned char m)
{
    unsigned char k, t, best = 0;

    for (k = 0; k < NTOOLS; ++k) {
        for (t = 1; t < sizeof digger / sizeof digger[0]; ++t) {
            if (tools[k][0] && !strcmp (tools[k], digger[t].key) && (digger[t].rate[m] > digger[best].rate[m] ||
                digger[t].rate[m] && digger[t].rate[m] == digger[best].rate[m] &&
                digger[t].wear[m] < digger[best].wear[m])) {
                best = t;
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
    HARN_WEA,                                                           /* Appraising: by its craft (appraiser) */
    HARN_STL,                                                           /* Concealment: HarnMaster's Stealth */
};

/* The skill that knows a thing (rs.h's AP_*): weapons and armour Weaponcraft, metal Metalcraft, wood Timbercraft,
** hide Hideworking, cloth Textilecraft, pottery Ceramics, jewels Jewelcraft, books and bones Lore, food Cookery */
static const unsigned char appraiser[NAP] = {
    HARN_WEA, HARN_WEA, HARN_MET, HARN_TIM, HARN_HID, HARN_TEX, HARN_CER, HARN_JEW, HARN_LOR, HARN_COO, HARN_LOR
};

static signed char __fastcall__ grade (unsigned char level)
{
    static const signed char g[4] = { G_CF, G_MF, G_MS, G_CS };

    return g[level & 3];
}

/* A skill's test (or an attribute's: ATTR), at the steps (a skill not learnt: its opening level, less a penalty) */
static signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps)
{
    int mod = 5 * steps;

    if (sk & ATTR) {
        return grade (harn_test_attr (c, sk & ~ATTR, 5, mod));
    }
    if (!c->ml[sk]) {
        return grade (harn_test (harn_get_oml (c, sk) - 5 * harn_get_pp (c) + mod));
    }
    return grade (harn_test_skill (c, sk, mod));
}

static signed char __fastcall__ test (harn_char* c, unsigned char p, signed char steps)
{
    if (p >= P_THEME) {
        return grade (harn_test (50 + 5 * steps));      /* (A theme's own: till themes say, an even chance) */
    }
    return test_sk (c, purpose[p], steps);
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

/* Its conditions (rs.h's C_*), as its sheet has them: its state (senseless, in shock, dead), bleeders, limbs made
** useless */
unsigned conditions (void)
{
    static const unsigned state[4] = { 0, C_UNCONSCIOUS, C_STUNNED, C_DEAD };
    unsigned c = state[pc.state & 3];
    unsigned char k;
    harn_injury* j;

    for (k = 0; k < pc.ninj; ++k) {
        j = &pc.inj[k];
        if (j->flags & HARN_IF_BLEEDER) {
            c |= C_BLEEDING;
        }
        if (j->flags & HARN_IF_LIMB) {
            c |= harn_is_limb (j->loc) == 1 ? C_ARM_USELESS : C_LEG_USELESS;
        }
    }
    return c;
}

/* Its health in words any ruleset can say (the game's: by its injuries' levels against its Endurance; down, in
** shock or senseless) */
static unsigned char health (void)
{
    unsigned char ip = harn_get_ip (&pc), e = harn_end (&pc);

    return pc.state == HARN_DEAD ? H_DEAD : pc.state != HARN_CONSCIOUS ? H_DOWN : !ip ? H_FINE : 3 * ip < e ? H_HURT :
        3 * ip < 2 * e ? H_BADLY : H_CRITICAL;
}

static void status (void)
{
    static const char* const word[4] = { "", " OUT", " SHOCK", " DEAD" };
    char b[8];
    struct rs_summary* s = &HUB->pcsum;
    unsigned char ep = harn_get_pp (&pc) - harn_get_up (&pc), k;

    strcpy (HUB->pcstatus, "UP ");
    strcat (HUB->pcstatus, utoa (harn_get_up (&pc), b, 10));
    s->cond = conditions ();
    strcat (HUB->pcstatus, pc.state ? word[pc.state & 3] : s->cond & C_BLEEDING ? " BLEEDING" : "");
    s->health = health ();
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

/* The character's sheet written (r's text a path): 0, or 1 */
static void __fastcall__ save_pc (struct req* r)
{
    static char line[48];
    int fd;

    r->result = 1;
    if ((fd = open (r->text, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    harn_write (fd, &pc);
    strcpy (line, "name ");
    strcat (line, name);
    harn_write_line (fd, line);
    strcpy (line, "occupation ");
    strcat (line, calling);
    harn_write_line (fd, line);
    close (fd);
    r->result = 0;
}

/* ---- A request answered */

static unsigned char rq[offsetof (struct req, text)];  /* (A request's copy, for the overlay: but its text) */

static void __fastcall__ answer (struct req* r)
{
    unsigned char k, m, t;

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
        if (r->a == P_DIG) {                    /* (Judged by the tool to hand that suits it best) */
            m = r->b > MAT_MASONRY ? MAT_STONE : r->b;
            t = dig_tool (m);
            k = digger[t].rate[m];
            strcpy (r->text, digger[t].key);
            r->w2 = digger[t].wear[m];
            if (!k) {
                r->result = 1;
                r->w1 = 0;
                r->w2 = 0;
                break;
            }
            r->grade = test (&pc, r->a, r->steps);
            r->w1 = r->grade == G_CS ? 2 * k : r->grade == G_MS ? k : r->grade == G_MF ? k / 2 : 0;
            break;
        }
        if (r->a == P_APPRAISE) {               /* (By the craft that knows the thing) */
            r->grade = test_sk (&pc, appraiser[r->b < NAP ? r->b : 0], r->steps);
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
        if (r->a == 2) {
            equip_all (r->text);
        } else {
            equip (r->text, r->a);
        }
        break;
    case RS_HARM:                               /* (In a copy, the overlay's: its words, a line) */
        memcpy (rq, r, sizeof rq);
        strncpy (blow, r->text, sizeof blow - 1);
        harm ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        strcpy (r->text, hurt);
        status ();
        break;
    case RS_SAVE:                               /* (The sheet into the save: hmchar's format, its own lines) */
        save_pc (r);
        break;
    case RS_DESCRIBE:
        if (r->b == 1) {
            brief ();
        } else if (r->b == 2) {
            put (hurt);
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

/* The overlays: dir/dq0harn.ovl, an 8K bank each (dq0harn.cfg's OV1 ...; the file cut after the last one used:
** make.js), read into banks of its own: 0, or -1 */
static int __fastcall__ ovl_load (const char* dir)
{
    static unsigned char b[256];
    static char path[64];
    int fd, first, n;
    unsigned char k;
    unsigned off;

    strcpy (path, dir);
    strcat (path, "/dq0harn.ovl");
    if ((fd = open (path, O_RDONLY)) < 0 || (first = hy_banks_alloc (OVLS)) < 0) {
        return -1;
    }
    for (k = 0; k < OVLS; ++k) {
        ovbank[k] = first + k;
        for (off = 0; off < 0x2000; off += sizeof b) {
            n = read (fd, b, sizeof b);
            if (n <= 0) {
                break;
            }
            bwrite (first + k, (void*) (0x8000 + off), b, n);
        }
    }
    close (fd);
    return 0;
}

int main (int argc, char* argv[])
{
    unsigned char s;

    if (argc < 4 || ovl_load (argv[3]) || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
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
