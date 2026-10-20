/*
** dq0harn.c - Dingus Quest Zero's HarnMaster referee: a task of the game's (dq0harn SEG ROLE DIR, started by the UI),
** answering the rules' questions (rs.h's requests) in the hub's slots, with the C SDK's harn.h (the book's rules).
** It holds the characters' sheets; the rest of the game knows them only by what it answers: grades, conditions,
** effects, summaries and words.
**
** So far: RS_HELLO (its name, its round), RS_LOAD_PC (hmchar's file), RS_TEST (a purpose made a skill or an
** attribute, the circumstances' steps 5 points each; words to a creature in a tongue, which it must speak: its own,
** common, and its file's "language" lines), RS_HARM (a fall, a trap's strike, a burn, a blow, a hand hurt
** by its blow at a thing: injuries on the sheet), RS_DESCRIBE (the character's sheet, its injuries with it; b 1: its
** short lines, for the map's panel, each 15 characters at most; b 2: the last harm's words), RS_SAVE (the sheet,
** its injuries and all, and the creatures' sheets), RS_NEW_ACTOR (a creature's sheet, by its kind's key: made the
** first time, kept in a bank after), RS_ATTACK (a blow struck, HarnMaster's melee: by the character, or at it),
** RS_MORALE, RS_TIME (bleeding), RS_EXERT (fatigue, by the pace moved at, and rest); RS_DESCRIBE b 3 (its state,
** the status command's, the hub's pace and watchfulness).  Its summary gives each pace's step, and the notice
** tests go by the character's pace and state.  The sheet's words and the harm are overlay 1's (sheet.c), the
** fights overlay 2's (fight.c): DIR/dq0harn.ovl, read into banks of its own as it starts.  Its slots: the hub's
** three (the world's, the UI's, and the denizens' task's: DSLOT, a short one), each rung for, answered in place, the
** asker rung with the reply (hub.h's protocol).  hmchar's file read and written, and the creatures' sheets kept, are swapped part 1's (../dq0/swap.inc: run
** in the main RAM's top, a part at a time, copied there from overlay 4's bank as it's called; harnfile.c), the names
** part 2's (harnname.c), with the tool a dig takes.
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

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

void __fastcall__ load_pc (struct req* r);      /* (Part 1's stubs: bank.s's) */
void __fastcall__ save_pc (struct req* r);
void __fastcall__ beasts (unsigned char back);
unsigned char __fastcall__ dig_tool (unsigned char m);     /* (Part 2's) */

harn_char pc;                                   /* The character */
char name[16], calling[20];
static char tongues[32] = " common";             /* The tongues it speaks, a space before each: its own (the game's
                                                **   stand-in for every character's), and its file's "language" lines */

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

char tools[NTOOLS][8];
unsigned char tflag[NTOOLS], tqual[NTOOLS];

static const struct { char key[8]; unsigned char rate[4], wear[4]; } digger[] = {    /* Earth, stone, masonry, flags */
    { "hands", { 1, 0, 0, 0 }, { 0, 0, 0, 0 } }, { "shovel", { 3, 0, 0, 1 }, { 0, 0, 0, 0 } },
    { "spade", { 3, 0, 0, 1 }, { 0, 0, 0, 0 } }, { "pick", { 2, 2, 1, 3 }, { 0, 1, 1, 1 } },
    { "mattock", { 3, 2, 1, 3 }, { 0, 1, 1, 1 } }, { "chisel", { 0, 1, 2, 1 }, { 0, 1, 1, 1 } },
    { "blade", { 1, 0, 0, 0 }, { 40, 0, 0, 0 } }, { "knife", { 1, 0, 0, 0 }, { 40, 0, 0, 0 } },
};

/* All that's to hand, anew: the keys in the text, each after its mark (* held, + worn, a space neither) and its
** quality after it (a digit) */
static void __fastcall__ equip_all (const char* keys)
{
    unsigned char k = 0, n, f;

    memset (tools, 0, sizeof tools);
    while (*keys && k < NTOOLS) {
        f = *keys == '*' ? T_HELD : *keys == '+' ? T_WORN : 0;
        if (*keys < 'a') {
            ++keys;
        }
        for (n = 0; keys[n] >= 'a'; ++n) {
        }
        if (n && n < sizeof tools[0]) {
            memcpy (tools[k], keys, n);
            tflag[k] = f;
            tqual[k] = keys[n] >= '0' && keys[n] <= '9' ? keys[n] - '0' : 4;
            ++k;
        }
        keys += n;
        if (*keys >= '0' && *keys <= '9') {
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
                tflag[k] = 0;
            } else {
                tools[k][0] = 0;
            }
            return;
        }
    }
}

/* The load the character carries (the game's things, all told: quarter pounds), its equipment as harn.h has it, one
** entry its whole weight (tenths of a pound): so its encumbrance (EP, the load over endurance) is the book's, and
** with it the physical penalty that tests of the body and its move are the worse for (Skills 6, Combat 6) */
static harn_equip carried = { "load", "load", 0, 0xFF, 0 };

static void __fastcall__ set_load (unsigned qp)
{
    carried.wt = qp > 13000U ? 65000U : qp * 5 / 2;
    pc.eq[0] = &carried;
    pc.neq = 1;
}

/* A tool equipped (sheet.c's too: a blow at a thing struck with the best to hand) */
unsigned char __fastcall__ has (const char* key)
{
    unsigned char k;

    for (k = 0; k < NTOOLS; ++k) {
        if (!strcmp (tools[k], key)) {
            return 1;
        }
    }
    return 0;
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* The tool to hand that suits the material best (bare hands, digger's 0, if none does better; of two as good, the
** one it wears less): its digger's number */
unsigned char __fastcall__ s_dig_tool (unsigned char m)
{
    unsigned char k, t, best = 0, r, br = digger[0].rate[m];

    for (k = 0; k < NTOOLS; ++k) {
        for (t = 1; tools[k][0] && t < sizeof digger / sizeof digger[0]; ++t) {
            if (strcmp (tools[k], digger[t].key)) {
                continue;
            }
            r = digger[t].rate[m];
            if (r > br || (r && r == br && digger[t].wear[m] < digger[best].wear[m])) {
                best = t;
                br = r;
            }
        }
    }
    return best;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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
    HARN_LCK,                                                           /* A mechanism made out: Lockcraft (traps) */
    HARN_NTR,                                                           /* Guile: Intrigue */
};

/* The skill that knows a thing (rs.h's AP_*): weapons and armour Weaponcraft, metal Metalcraft, wood Timbercraft,
** hide Hideworking, cloth Textilecraft, pottery Ceramics, jewels Jewelcraft, books and bones Lore, food Cookery, stone
** Masonry, glass Glassworking */
static const unsigned char appraiser[NAP] = {
    HARN_WEA, HARN_WEA, HARN_MET, HARN_TIM, HARN_HID, HARN_TEX, HARN_CER, HARN_JEW, HARN_LOR, HARN_COO, HARN_LOR,
    HARN_MSN, HARN_GLA
};

static signed char __fastcall__ grade (unsigned char level)
{
    static const signed char g[4] = { G_CF, G_MF, G_MS, G_CS };

    return g[level & 3];
}

/* A skill's test (or an attribute's: ATTR), at the steps (a skill not learnt: its opening level, less a penalty) */
static signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps)
{
    int mod = STEP_PCT * steps;

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
        return grade (harn_test (50 + STEP_PCT * steps));   /* (A theme's own: till themes say, an even chance) */
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

/* ---- Fatigue (sheet.c's spent: the book's movement fatigue) */

unsigned tired, rested;                         /* Exertion toward the next level, rest toward the next gone */
unsigned char spent_at;                         /* (The pace the time went at: spent's) */

/* Its conditions (rs.h's C_*), as its sheet has them (the character's, or a creature's): its state (senseless, in
** shock, dead), bleeders, limbs made useless; tired, or tiring */
unsigned __fastcall__ cond_of (const harn_char* h)
{
    static const unsigned state[4] = { 0, C_UNCONSCIOUS, C_STUNNED, C_DEAD };
    unsigned c = state[h->state & 3] | (h->fl || (h == &pc && tired) ? C_FATIGUED : 0);   /* (Tired, or tiring) */
    unsigned char k;
    const harn_injury* j;

    for (k = 0; k < h->ninj; ++k) {
        j = &h->inj[k];
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
unsigned char __fastcall__ health_of (const harn_char* h)
{
    unsigned char ip = harn_get_ip (h), e = harn_end (h);

    return h->state == HARN_DEAD ? H_DEAD : h->state != HARN_CONSCIOUS ? H_DOWN : !ip ? H_FINE : 3 * ip < e ? H_HURT :
        3 * ip < 2 * e ? H_BADLY : H_CRITICAL;
}

static void status (void)
{
    static const char* const word[4] = { "", " OUT", " SHOCK", " DEAD" };
    char b[8];
    struct rs_summary* s = &HUB->pcsum;
    static const unsigned char rate[3] = { 100, 50, 33 };
    unsigned char ep = harn_get_pp (&pc) - harn_get_up (&pc), k, w, g, v;

    strcpy (HUB->pcstatus, "UP ");
    strcat (HUB->pcstatus, utoa (harn_get_up (&pc), b, 10));
    s->cond = conditions ();
    strcat (HUB->pcstatus, pc.state ? word[pc.state & 3] : s->cond & C_BLEEDING ? " BLEEDING" : "");
    s->health = health_of (&pc);
    k = harn_move (&pc);                       /* (Walking: half its Move, squares a round: Combat 6) */
    s->step = w = k < 4 ? 50 : 200 / k;
    s->gstep[GA_CREEP] = w + (w << 1);          /* (The game's: a sixth of its Move, a quarter; the book's: a jog its
                                                **   Move, a run twice, a sprint three times) */
    s->gstep[GA_SLOW] = w << 1;
    s->gstep[GA_WALK] = w;
    for (g = GA_JOG; g < NGAITS; ++g) {
        v = rate[g - GA_JOG] / k;
        s->gstep[g] = !v ? 1 : v < w ? v : w;
    }
    s->attacks = 1;
    s->sight = sense (HARN_EYE);
    s->hearing = sense (HARN_HRG) + (HUB->watch & WA_LISTEN ? 2 : 0);  /* (Listening as it goes: two better) */
    if (s->hearing > 9) {
        s->hearing = 9;
    }
    s->smell = sense (HARN_SML);
    s->darkvision = 0;                          /* (The species' sight in the dark: the book's, to come) */
    s->nightvision = 1;
    s->morale = pc.att[HARN_MOR];
    s->load = !ep ? 0 : ep <= 2 ? 1 : ep <= 4 ? 2 : 3;   /* (Its encumbrance: a little burdened, heavily, overloaded) */
    s->power = 10;
    s->northsense = pc.att[HARN_SP] == HARN_KHU;    /* (The species' trait: the Khuzdul live below, and always know
                                                **   north there) */
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* ---- hmchar's file: its own lines, the name and the occupation (and the game's: its pace, its watchfulness and its
** exertion toward the next fatigue level, "pace 2 1 1200": the pace and the watchfulness the world's, said in the reply
** to RS_LOAD_PC and written back as the hub has them, the exertion the referee's own) */

static unsigned char gait = GA_WALK, watch;

static void __fastcall__ own_line (char* l)
{
    if (!strncmp (l, "pace ", 5) && strlen (l) >= 10) {
        gait = l[5] - '0' < NGAITS ? l[5] - '0' : GA_WALK;
        watch = l[7] - '0';
        tired = atoi (l + 9);
    } else if (!strncmp (l, "name ", 5)) {
        strncpy (name, l + 5, sizeof name - 1);
    } else if (!strncmp (l, "occupation ", 11)) {
        strncpy (calling, l + 11, sizeof calling - 1);
    } else if (!strncmp (l, "language ", 9) && strlen (tongues) + strlen (l + 8) < sizeof tongues) {
        strcat (tongues, l + 8);
    }
}

void __fastcall__ s_load_pc (struct req* r)
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
    hub_map (0);
    put (name);
    put ("\n");
    put (calling);
    status ();
}

/* The character's sheet written (r's text a path): 0, or 1 */
void __fastcall__ s_save_pc (struct req* r)
{
    static char line[48];
    char b[12];
    unsigned char g = HUB->gait, w = HUB->watch;
    char* p;
    char* e;
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
    strcpy (line, "pace 0 0 ");                 /* (The game's: its pace, its watchfulness, its exertion) */
    line[5] += g;
    line[7] += w;
    strcat (line, utoa (tired, b, 10));
    harn_write_line (fd, line);
    for (p = tongues + 7; *p; p = e) {             /* (Its tongues but its own: "language" lines) */
        e = strchr (p + 1, ' ');
        e = e ? e : p + strlen (p);
        strcpy (line, "language");
        strncat (line, p, e - p);
        harn_write_line (fd, line);
    }
    close (fd);
    r->result = 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The creatures' sheets (fight.c's): a bank of the referee's, a sheet a denizen's record (its number the
** denizens' task's), kept in the save folder's beasts.dat with the game's other records (read back as the first
** creature's asked for, a game carried on) */

static unsigned char sb;                        /* Their bank */
static unsigned char piece[256];                /* (The bank through the main RAM, a piece at a time) */
struct beast bs;
unsigned char aden[NACTORS];
unsigned long tnow, gseed;
unsigned char stance;
char bkey[8];

void __fastcall__ sheet_get (unsigned char n)
{
    bread (sb, (struct beast*) 0x8000 + n, &bs, sizeof bs);
}

void __fastcall__ sheet_put (unsigned char n)
{
    bwrite (sb, (struct beast*) 0x8000 + n, &bs, sizeof bs);
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* The bank written to the save folder's beasts.dat (back 1: read from it, or cleared: none there; 2: cleared) */
void __fastcall__ s_beasts (unsigned char back)
{
    static char path[56];
    unsigned off;
    int fd = -1;

    memset (piece, 0, sizeof piece);
    if (back < 2 && HUB->savedir[0]) {
        strcpy (path, HUB->savedir);
        strcat (path, "/beasts.dat");
        fd = back ? open (path, O_RDONLY) : open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    }
    for (off = 0; off < 0x2000; off += sizeof piece) {
        if (!back) {
            bread (sb, (char*) 0x8000 + off, piece, sizeof piece);
            write (fd, piece, sizeof piece);
        } else {
            if (fd < 0 || read (fd, piece, sizeof piece) != sizeof piece) {
                memset (piece, 0, sizeof piece);
            }
            bwrite (sb, (char*) 0x8000 + off, piece, sizeof piece);
        }
    }
    if (fd >= 0) {
        close (fd);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* Whether it speaks tongue t (a word of tongues, a space before it) */
static unsigned char __fastcall__ speaks (const char* t)
{
    char* p = tongues;
    unsigned char n = strlen (t);

    while ((p = strstr (p, t)) != 0) {
        if (p[-1] == ' ' && (!p[n] || p[n] == ' ')) {
            return 1;
        }
        ++p;
    }
    return 0;
}

/* The character's state in words (RS_DESCRIBE b 3: overlay 1's; a 1, its pace and watchfulness only, the hub's): what
** the words need copied for the overlay (the hub's not in the window while it is) */
struct pstate ps;

static void __fastcall__ pace_set (unsigned char a)
{
    status ();                                  /* (Listening: its hearing) */
    ps.gait = HUB->gait;
    ps.watch = HUB->watch;
    ps.light = HUB->pc_light;
    ps.full = !a;
    memcpy (ps.lamp, HUB->pc_lamp, sizeof ps.lamp);
    hub_map (SIGHT_BANK);                       /* (The senses' record's, in their bank) */
    ps.dark = SIGHT->dark;
    hub_map (0);
    state ();
}

/* ---- A request answered */

static unsigned char rq[offsetof (struct req, text)];  /* (A request's copy, for the overlay: but its text) */
static unsigned tlim = REQ_TEXT;                /* (Its text's room: the denizens' slot's DTEXT) */
static unsigned char restored;                  /* (The creatures' sheets read back, a game carried on) */

static void sheets_in (void)
{
    if (HUB->resume && !restored) {
        restored = 1;
        beasts (1);
    }
}

static void __fastcall__ answer (struct req* r)
{
    unsigned char k, m, t;

    tp = r->text;
    tend = r->text + tlim - 1;
    r->result = 0;
    switch (r->kind) {
    case RS_NEW_ACTOR:                          /* (The fights': overlay 2's, in a copy, as the harm's) */
        sheets_in ();
        strncpy (bkey, r->text, sizeof bkey - 1);
    case RS_ATTACK:
    case RS_MORALE:
    case RS_TIME:
        tnow = HUB->now;
        stance = HUB->pc_stance;
        pc_way = HUB->pc_strike;
        gseed = HUB->seed;
        memcpy (rq, r, sizeof rq);
        fight ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        status ();
        break;
    case RS_HELLO:
        r->w1 = 100;                            /* (A round: 10 seconds, Combat 7) */
        put ("HarnMaster 3.5");
        break;
    case RS_LOAD_PC:
        load_pc (r);
        r->a = gait;                            /* (Its file's pace and watchfulness: the world's to set) */
        r->b = watch;
        break;
    case RS_TEST:
        if (r->a == P_DIG) {                    /* (Judged by the tool to hand that suits it best) */
            m = r->b > MAT_FLAGS ? MAT_STONE : r->b;
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
        if ((r->a == P_SEARCH || r->a == P_DISARM) && r->b == HIDE_MAGICAL) {   /* (Only by magic: its spells
                                                **   to come) */
            r->result = 1;
            break;
        }
        if (r->a == P_PICKLOCK && !has ("picks")) {    /* (Lockcraft wants its tools: Skills 23) */
            r->result = 1;
            break;
        }
        if ((r->a == P_PERSUADE || r->a == P_INTIMIDATE || r->a == P_DECEIVE) && r->text[0]) {   /* (Words in a
                                                **   tongue: one it speaks, or nothing) */
            if (!speaks (r->text)) {
                r->result = 2;
                break;
            }
        }
        if (r->a <= P_NOTICE_SMELL) {           /* (By its state: its pace, its shock; overlay 1's, in a copy) */
            ps.go = HUB->pc_going;
            ps.watch = HUB->watch;
            memcpy (rq, r, sizeof rq);
            r->steps += noticing ((struct req*) rq);
        }
        r->grade = test (&pc, r->a, r->steps);
        break;
    case RS_EXERT:                              /* (Its fatigue: its summary anew) */
        spent_at = r->a;
        spent (r->w1);
        status ();
        break;
    case RS_EQUIP:
        if (r->a == 2) {
            equip_all (r->text);
            set_load (r->w1);                   /* (And its load: its summary anew) */
            status ();
        } else {
            equip (r->text, r->a);
        }
        break;
    case RS_HARM:                               /* (In a copy, the overlay's: its words, a line) */
        memcpy (rq, r, sizeof rq);
        harm ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        strcpy (r->text, hurt);
        status ();
        break;
    case RS_SAVE:                               /* (The sheet into the save: hmchar's format, its own lines; and the
                                                **   creatures', beasts.dat) */
        save_pc (r);
        sheets_in ();
        beasts (0);
        break;
    case RS_DESCRIBE:
        if (r->b == 1) {
            brief ();
        } else if (r->b == 2) {
            put (hurt);
        } else if (r->b == 3) {
            pace_set (r->a);
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
** make.js), read into banks of its own, and a bank for the creatures' sheets (cleared): 0, or -1 */
static int __fastcall__ ovl_load (const char* dir)
{
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
        for (off = 0; off < 0x2000; off += sizeof piece) {
            n = read (fd, piece, sizeof piece);
            if (n <= 0) {
                break;
            }
            bwrite (first + k, (void*) (0x8000 + off), piece, n);
        }
    }
    close (fd);
    if ((n = hy_banks_alloc (1)) < 0) {
        return -1;
    }
    sb = n;
    return 0;
}

/* The denizens' task's request (its own slot, DSLOT), if it's posted: answered (DTEXT of its text) */
static void den_slot (void)
{
    hub_map (0);
    if (hub_posted (DSLOT)) {
        tlim = DTEXT;
        answer (DSLOT);
        tlim = REQ_TEXT;
        hub_reply (DSLOT);
    }
}

int main (int argc, char* argv[])
{
    unsigned char s;

    if (argc < 4 || ovl_load (argv[3]) || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    rseed (HUB->seed ^ 0x5245);                 /* (Its own dice: the game's seed, and its role) */
    beasts (2);
    for (;;) {
        hy_sem_acquire (HUB->sem[T_REFEREE]);   /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        for (s = 0; s < NSLOTS; ++s) {
            if (hub_posted (&HUB->slot[s])) {
                answer (&HUB->slot[s]);
                hub_reply (&HUB->slot[s]);
            }
        }
        den_slot ();
    }
    return 0;
}
