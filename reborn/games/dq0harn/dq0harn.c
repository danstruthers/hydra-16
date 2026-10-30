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
** RS_MORALE, RS_TIME (bleeding), RS_CONTEST (two actors' tests: the engage initiative test's, a creature's by its
** sheet), RS_EXERT (fatigue, by the pace moved at or strenuous work, and rest); RS_DESCRIBE b 3 (its state,
** the status command's, the hub's pace and watchfulness); RS_WIELD (what the character would strike with, nothing
** named: of what it holds, else the best it carries, gear.c's); P_BEARING, the bearings underground (bearing.c's,
** overlay 3: its own dice).  A thing named that strikes (the hub's pc_strike,
** SK_THING) its kind read from the hub's catalogue (kind_in: wk).  Its summary gives each pace's step, and the notice
** tests go by the character's pace and state.  The sheet's words and the harm are overlay 1's (sheet.c), the
** fights overlay 2's (fight.c): DIR/dq0harn.ovl, read into banks of its own as it starts.  Its slots: the hub's
** three (the world's, the UI's, and the denizens' task's: DSLOT, a short one), each rung for, answered in place, the
** asker rung with the reply (hub.h's protocol).  hmchar's file read and written, and the creatures' sheets kept, are swapped part 1's (../dq0/swap.inc: run
** in the main RAM's top, a part at a time, copied there from overlay 4's bank as it's called; harnfile.c), the names
** part 2's (harnname.c), with the tool a dig takes.  And a test's, RS_DICE: its dice fixed (its unit tests').
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
void __fastcall__ dig_answer (struct req* r);   /* (Part 2's) */
void __fastcall__ dice_set (struct req* r);
void __fastcall__ outfit_of (struct req* r);    /* (Overlay 1's: outfit.c, RS_OUTFIT) */
signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps);    /* (Overlay 3's) */
signed char __fastcall__ draw_test (signed char steps);     /* (Overlay 3's: a map drawn, slice 4.2's) */
char news[NEWS_N];                              /* (What time did to the character, heal.c's: RS_DESCRIBE b 4's) */

char tpart[16];                                 /* (RS_TREAT's part said) */

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
** the game's own, every referee's (../dq0/rshouse.h's table: a tool's feet a spell of work by what it's into, and
** its wear) */

char tools[NTOOLS][8];
unsigned char tflag[NTOOLS], tqual[NTOOLS];
unsigned wealth;                                /* (The family's wealth, its file's "wealth" line: 0 none) */

/* All that's to hand, anew: the keys in the text, each after its mark (held: < in the left hand, > the right, * both;
** + worn; a space neither) and its quality after it (a digit) */
static void __fastcall__ equip_all (const char* keys)
{
    unsigned char k = 0, n, f, j;

    memset (tools, 0, sizeof tools);
    while (*keys) {
        f = *keys == '*' ? T_HELD | T_LEFT | T_RIGHT : *keys == '<' ? T_HELD | T_LEFT : *keys == '>' ? T_HELD | T_RIGHT :
            *keys == '+' ? T_WORN : 0;
        if (*keys < 'a') {
            ++keys;
        }
        for (n = 0; keys[n] >= 'a'; ++n) {
        }
        if (n && n < sizeof tools[0]) {
            if (k < NTOOLS) {
                j = k++;
            } else {                            /* (Full, an outfit's many things: what's held or worn in the place of
                                                **   a thing carried, what fights first, 2026-10-27) */
                for (j = NTOOLS; j && tflag[j - 1]; --j) {
                }
                j = f && j ? j - 1 : NTOOLS;
            }
            if (j < NTOOLS) {
                memset (tools[j], 0, sizeof tools[j]);
                memcpy (tools[j], keys, n);
                tflag[j] = f;
                tqual[j] = keys[n] >= '0' && keys[n] <= '9' ? keys[n] - '0' : 4;
            }
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

/* A tool equipped (Lockcraft's picks) */
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

/* A thing's kind, the catalogue's (in the hub's bank: the thing that strikes, the hub's pc_strike's; RS_WIELD's, each
** held in turn), copied into wk; the window put back (gear.c's, the overlay's, too) */
struct kind wk;

void __fastcall__ kind_in (unsigned char k)
{
    unsigned char b = getbank ();

    hub_map (KIND_BANK);
    memcpy (&wk, CATALOGUE + k, sizeof wk);
    setbank (b);
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* The tool to hand that suits the material best (rshouse.h's, s_dig_tool: its digger's number); the table part 2's
** alone (the main RAM's room, 2026-10-27) */
static signed char __fastcall__ test (harn_char* c, unsigned char p, signed char steps);
#define RS_HOUSE_DIG
#define RS_HOUSE_DIG_TOOL   s_dig_tool
#define RS_HOUSE_QUIET                          /* (Moving quietly's own dice: s_quiet_test's) */
#include "../dq0/rshouse.h"

/* A dig's test (RS_TEST's P_DIG): judged by the tool to hand that suits it best, its rate a spell at a marginal
** success (earth's untested: the tool's rate), its key and its wear in the reply */
void __fastcall__ s_quiet_test (struct req* r);
static void __fastcall__ dig_of (struct req* r);

/* The skill that knows a thing (rs.h's AP_*): weapons and armour Weaponcraft, metal Metalcraft, wood Timbercraft,
** hide Hideworking, cloth Textilecraft, pottery Ceramics, jewels Jewelcraft, books and bones Lore, food Cookery, stone
** Masonry, glass Glassworking (part 2's, the main RAM's room, 2026-10-29) */
static const unsigned char appraiser[NAP] = {
    HARN_WPN, HARN_WPN, HARN_MET, HARN_TIM, HARN_HID, HARN_TEX, HARN_CER, HARN_JEW, HARN_LOR, HARN_COO, HARN_LOR,
    HARN_MSN, HARN_GLA
};

/* RS_TEST's own tests, through one stub (the main RAM's room): a dig's, moving quietly's, an appraisal's (by the
** craft that knows the thing) */
void __fastcall__ s_dig_answer (struct req* r)
{
    if (r->a == P_STEALTH) {
        s_quiet_test (r);
    } else if (r->a == P_APPRAISE) {
        r->grade = test_sk (&pc, appraiser[r->b < NAP ? r->b : 0], r->steps);
    } else {
        dig_of (r);
    }
}

static void __fastcall__ dig_of (struct req* r)
{
    unsigned char m = r->b > MAT_SOIL ? MAT_STONE : r->b, t = s_dig_tool (m), k = digger[t].rate[m];

    strcpy (r->text, digger[t].key);
    r->w2 = digger[t].wear[m];
    if (!k) {
        r->result = 1;
        r->w1 = 0;
        r->w2 = 0;
        return;
    }
    r->grade = DIG_TESTED (m) ? test (&pc, r->a, r->steps) : G_MS;
    r->w1 = r->grade == G_CS ? 2 * k : r->grade == G_MS ? k : r->grade == G_MF ? k / 2 : 0;
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
    HARN_WPN,                                                           /* Appraising: by its craft (appraiser) */
    HARN_STL,                                                           /* Concealment: HarnMaster's Stealth */
    HARN_LCK,                                                           /* A mechanism made out: Lockcraft (traps) */
    HARN_NTR,                                                           /* Guile: Intrigue */
    HARN_THR,                                                           /* A thing thrown: Throwing */
    HARN_INI,                                                           /* Quickness, engaging: Initiative */
    ATTR | HARN_DEX,                                                    /* A grope: Dexterity */
    ATTR | HARN_AGL,                                                    /* Keeping one's feet: Agility (the
                                                                        **   book's stumble roll: test's) */
    ATTR | HARN_AGL,                                                    /* Getting up, prone: Agility (the
                                                                        **   book's Rise, Combat 8) */
    HARN_DRA,                                                           /* A map drawn: Drawing (Cartography
                                                                        **   first, overlay 3's draw_test) */
    HARN_AWA,                                                           /* Bearings: Awareness, or Mining if
                                                                        **   higher (bearing.c's) */
};

/* The referee's own skill (harn.h's a program's own, after the book's: its file's "ml 91 N" line): Drawing's
** specialty Cartography (Skills 14: poetic maps; a specialty chosen at 40 or more, opened at the skill's mastery,
** Skills 2), Drawing's attributes and sunsigns; opened only as it's taught (overlay 3's draw_learn: none teach it yet) */
#define HARN_CAR        HARN_NSKILLS
static const harn_skill own[1] = {
    { "Cartography", { HARN_DEX, HARN_EYE, HARN_EYE }, { HARN_S_SKO << 4 | 1, HARN_S_TAI << 4 | 1 }, 2, 0, HARN_CRAFT, 1 }
};
unsigned char drawn_new;                        /* (A skill opened by a map drawn: draw_test's, RS_TEST's result 1) */

signed char __fastcall__ grade (unsigned char level)
{
    static const signed char g[4] = { G_CF, G_MF, G_MS, G_CS };

    return g[level & 3];
}

#pragma code-name (push, "OV5CODE")            /* (Overlay 5's, beside part 2's image: 2026-10-27) */
#pragma rodata-name (push, "OV5RODATA")

/* A skill's test (or an attribute's: ATTR), at the steps (a skill not learnt: its opening level, less its group's
** penalty, as a learnt one's: harn_eml's, its level 0, the penalty alone; the user's, 2026-10-20): overlay 5's (the main
** RAM's room, slice 4.2's: no hub in it), its stub test_sk */
signed char __fastcall__ o_test_sk (harn_char* c, unsigned char sk, signed char steps)
{
    int mod = STEP_PCT * steps;

    if (sk & ATTR) {
        return grade (harn_test_attr (c, sk & ~ATTR, 5, mod));
    }
    if (!c->ml[sk]) {
        return grade (harn_test (harn_get_oml (c, sk) + harn_eml (c, sk) + mod));
    }
    return grade (harn_test_skill (c, sk, mod));
}

/* A map drawn as the character goes (RS_TEST's P_DRAW: HOUSE.md's the map kept), its grade: the book's Drawing, its
** specialty Cartography first (the referee's own skill, HARN_CAR); neither open, Drawing opened at its opening level
** (the house's, over the book's month of teaching: the first map it draws), drawn_new set */
signed char __fastcall__ o_draw_test (signed char steps)
{
    if (!pc.ml[HARN_CAR] && !pc.ml[HARN_DRA]) {
        harn_open_skill (&pc, HARN_DRA);
        drawn_new = 1;
    }
    return o_test_sk (&pc, pc.ml[HARN_CAR] ? HARN_CAR : HARN_DRA, steps);
}

/* Cartography opened as it's taught (a teacher's, a book's: none yet, the hook for them): the book's rule, a specialty
** of Drawing at 40 or more, opened at Drawing's mastery (Skills 2); 1 opened, 0 not (Drawing under 40) */
unsigned char o_draw_learn (void)
{
    if (pc.ml[HARN_DRA] < 40) {
        return 0;
    }
    if (!pc.ml[HARN_CAR]) {
        pc.ml[HARN_CAR] = pc.ml[HARN_DRA];
    }
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

static signed char __fastcall__ test (harn_char* c, unsigned char p, signed char steps)
{
    if (p >= P_THEME) {
        return grade (harn_test (50 + STEP_PCT * steps));   /* (A theme's own: till themes say, an even chance) */
    }
    if (p == P_STUMBLE) {                       /* (The book's stumble roll, Skills 22: 3d6, the physical penalty and
                                                **   the circumstances, over Agility a fall; a point of the roll two
                                                **   steps, the book's darkness 2 points, a heavy obstruction's 2) */
        return harn_stumble_roll (c, -(int) steps / 2) ? G_MF : G_MS;
    }
    if (p == P_RISE) {
        return rise_of (c);                     /* (Getting up: the house's test, if any, gear.c's) */
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
unsigned char spent_at;                         /* (The pace the time went at, or GA_WORK, GA_REST: spent's) */

/* (Its limbs and its conditions: gear.c's, overlay 3's, the main RAM's room: limbs_of, cond_of) */
unsigned char lhurt[4];
unsigned char cplan = BF_BIPED;
unsigned char hheld;                            /* (The hands that hold something: RS_EQUIP 2's w2, DH_*) */

static void status (void)
{
    static const char* const word[4] = { "", " OUT", " SHOCK", " DEAD" };
    char b[8];
    struct rs_summary* s = &HUB->pcsum;
    static const unsigned char rate[3] = { 100, 50, 33 };
    unsigned char ep = harn_get_pp (&pc) - harn_get_up (&pc), k, w, g, v, l;

    strcpy (HUB->pcstatus, "UP ");
    strcat (HUB->pcstatus, utoa (harn_get_up (&pc), b, 10));
    s->cond = conditions ();
    if (pc_out > (tnow = HUB->now)) {
        s->cond |= C_UNCONSCIOUS;               /* (Senseless till it comes round: the minds' to know it helpless) */
    }
    strcat (HUB->pcstatus, s->cond & C_UNCONSCIOUS ? word[1] : pc.state ? word[pc.state & 3] : s->cond & C_BLEEDING ?
        " BLEEDING" : "");
    s->health = health_of (&pc);
    k = harn_move (&pc);                       /* (Walking: half its Move, squares a round: Combat 6; in shock, half
                                                **   that, and no faster: Skills 22's walk or crawl at half move) */
    if (pc.state == HARN_SHOCK) {
        ++k;
        k >>= 1;
    }
    s->limbs = v = limbs_of (&pc);              /* (Its hands and limbs; a leg hurt, its walk the slower, the
                                                **   house's: each level of the injuries to the legs it walks on a
                                                **   fifth more time a square, slice 4.1b's) */
    s->limp = l = (v & LB_LEG_L ? 0 : lhurt[2]) + (v & LB_LEG_R ? 0 : lhurt[3]);
    w = k < 4 ? 50 : 200 / k;
    s->step = w = (unsigned) w * (5 + l) / 5 > 255 ? 255 : (unsigned) w * (5 + l) / 5;
    s->gstep[GA_CREEP] = w > 85 ? 255 : w + (w << 1);   /* (The game's: a sixth of its Move, a quarter; the book's: a
                                                **   jog its Move, a run twice, a sprint three times) */
    s->gstep[GA_SLOW] = w > 127 ? 255 : w << 1;
    s->gstep[GA_WALK] = w;
    for (g = GA_JOG; g < NGAITS; ++g) {
        v = rate[g - GA_JOG] / k;
        v = (unsigned) v * (5 + l) / 5 > 255 ? 255 : (unsigned) v * (5 + l) / 5;
        s->gstep[g] = pc.state == HARN_SHOCK ? w : !v ? 1 : v < w ? v : w;
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
    s->mapmind = pc.att[HARN_INT] >= 20 ? 100 : 5 * pc.att[HARN_INT];  /* (Its map in mind, the house's: five times its
                                                **   Intelligence, of a hundred; a tenth of it lost every twice its
                                                **   Intelligence and Will together, minutes) */
    s->mapfade = 2 * (pc.att[HARN_INT] + pc.att[HARN_WIL]);
}

#pragma code-name (push, "SW1CODE")
#pragma rodata-name (push, "SW1RODATA")

/* ---- hmchar's file: its own lines, the name and the occupation (and the game's: its pace, its watchfulness and its
** exertion toward the next fatigue level, "pace 2 1 1200": the pace and the watchfulness the world's, said in the reply
** to RS_LOAD_PC and written back as the hub has them, the exertion the referee's own; and what dealt each injury,
** a digit each in order, "dealt 0100": pc_by's, sheet.c's BY_*) */

static unsigned char gait = GA_WALK, watch;

static void __fastcall__ own_line (char* l)
{
    unsigned char k;

    if (!strncmp (l, "dealt ", 6)) {
        for (k = 0; k < HARN_INJMAX && l[k + 6]; ++k) {
            pc_by[k] = l[k + 6] & 7;            /* (A digit: BY_* are 0-4) */
        }
    } else if (!strncmp (l, "pace ", 5) && strlen (l) >= 10) {
        gait = l[5] - '0' < NGAITS ? l[5] - '0' : GA_WALK;
        watch = l[7] - '0';
        tired = atoi (l + 9);
    } else if (!strncmp (l, "name ", 5)) {
        strncpy (name, l + 5, sizeof name - 1);
    } else if (!strncmp (l, "occupation ", 11)) {
        strncpy (calling, l + 11, sizeof calling - 1);
    } else if (!strncmp (l, "body ", 5)) {
        body_line (l + 5, 1);                   /* (Its venom, its rest toward its healing rolls: heal.c's) */
    } else if (!strncmp (l, "wealth ", 7)) {
        wealth = atoi (l + 7);                  /* (Its family's, hmchar's: RS_OUTFIT's) */
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
    pv.hr = 0;                                  /* (No venom, no rest yet: its file's "body" line's, if any) */
    rest_m = 0;
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
    body_line (line, 0);                        /* (Its venom, its rest: heal.c's) */
    harn_write_line (fd, line);
    strcpy (line, "dealt ");                    /* (What dealt each injury: its words') */
    for (g = 0; g < pc.ninj; ++g) {
        line[g + 6] = '0' | pc_by[g];
    }
    line[g + 6] = 0;
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

#pragma code-name (push, "SW2CODE")             /* (Part 2's: part 1's room, 2026-10-27) */
#pragma rodata-name (push, "SW2RODATA")

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
    a = HUB->pc_going & GOING_BOLD;             /* (A pace said outright in the dark: held to nothing there) */
    hub_map (SIGHT_BANK);                       /* (The senses' record's, in their bank) */
    ps.dark = SIGHT->dark ? 1 + (a != 0) : 0;   /* (2: dark, but its pace held to nothing there) */
    hub_map (0);
    state ();
}

/* ---- The dice fixed, for a test (RS_DICE: the referee's unit tests, tests/ram/t_rules): dice.h's hook, set by the
** first, rolls every die after it, the faces given first (then the generator's), and keeps what each was (its sides) */

#define DFIX    sizeof ((struct req*) 0)->x
static char dface[DFIX + 1];                    /* The faces forced, to a 0, */
static unsigned char dk, drep;                  /*   the next, and the face after them (0: the generator's) */
static unsigned char dlog[DFIX], dl;            /* The sides of the first dice rolled, how many kept */
static unsigned drolled;                        /* The dice rolled */

static int __fastcall__ fixed (unsigned sides)
{
    ++drolled;
    if (dl < DFIX) {
        dlog[dl++] = sides;
    }
    if (dface[dk]) {
        return dface[dk++];
    }
    return drep ? drep : 1 + (int) random (sides);
}

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

void __fastcall__ s_dice_set (struct req* r)
{
    if (r->a & 1) {
        rseed (r->w1);
    }
    r->w1 = drolled;
    r->c = strlen (dface + dk);
    memcpy (dface, r->x, DFIX);
    dk = 0;
    drep = r->b;
    memcpy (r->x, dlog, DFIX);
    memset (dlog, 0, DFIX);
    dl = 0;
    drolled = 0;
    dice_hook = fixed;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

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

/* One side of two actors' tests (RS_CONTEST): who's test of purpose p, at the steps: the character's here (one
** senseless, or in shock, tests nothing: a critical failure), a creature's by its sheet (overlay 2's, in a copy) */
static signed char __fastcall__ side (unsigned char who, unsigned char p, signed char steps)
{
    struct req* q = (struct req*) rq;

    tnow = HUB->now;
    if (!who) {
        return pc.state != HARN_CONSCIOUS || pc_out > tnow ? G_CF : test (&pc, p, steps);
    }
    q->kind = RS_CONTEST;
    q->actor = who;
    q->a = p;
    q->steps = steps;
    fight (q);
    return q->grade;
}

/* Moving quietly's test (RS_TEST's P_STEALTH: slice 4.6's), by its own dice (rshouse.h's RS_HOUSE_QUIET's: the game's
** as they stand, mixed, and put back after), so the game's other tests roll as they did: the character's, or a
** creature's by its sheet (a hunter's stalk, the denizens'); part 2's, the main RAM's room */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")
void __fastcall__ s_quiet_test (struct req* r)
{
    QUIET_IN
    r->grade = r->actor ? side (r->actor, P_STEALTH, r->steps) : test (&pc, P_STEALTH, r->steps);
    QUIET_OUT
}
#pragma code-name (pop)
#pragma rodata-name (pop)

static void __fastcall__ answer (struct req* r)
{
    unsigned char k, m, t;
    signed char g;

    tp = r->text;
    tend = r->text + tlim - 1;
    r->result = 0;
    switch (r->kind) {
    case RS_NEW_ACTOR:                          /* (The fights': overlay 2's, in a copy, as the harm's) */
        sheets_in ();
        strncpy (bkey, r->text, sizeof bkey - 1);
    case RS_ATTACK:
    case RS_MORALE:
    case RS_WIELD:                              /* (What it would strike with: gear.c's, through fight's) */
        tnow = HUB->now;
        stance = HUB->pc_stance;
        pc_way = HUB->pc_strike;
        pc_hand = HUB->pc_shand;
        if (pc_way & SK_THING) {
            kind_in (pc_way & 0x7F);            /* (The thing named that strikes: its kind, for the overlay) */
        }
        gseed = HUB->seed;
        memcpy (rq, r, sizeof rq);
        fight ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        status ();
        break;
    case RS_HARM:                               /* (Harm, wounds over time, first aid: overlay 6's, heal.c, in a */
    case RS_TIME:                               /*   copy, the part said copied for it; the harm overlay 1's, its */
    case RS_TREAT:                              /*   words a line) */
        tnow = HUB->now;
        strncpy (tpart, r->text, sizeof tpart - 1);
        memcpy (rq, r, sizeof rq);
        body_req ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        status ();
        break;
    case RS_HELLO:
        r->w1 = 100;                            /* (A round: 10 seconds, Combat 7) */
        put ("HarnMaster 3.5\nhmchar");         /* (Its character generator: hmchar's files, RS_LOAD_PC's) */
        break;
    case RS_LOAD_PC:
        load_pc (r);
        r->a = gait;                            /* (Its file's pace and watchfulness: the world's to set) */
        r->b = watch;
        break;
    case RS_TEST:
        if (r->a == P_DIG || r->a == P_STEALTH || r->a == P_APPRAISE) {   /* (Judged by the tool to hand that
                                                **   suits it best; moving quietly, by its own dice; by the craft that
                                                **   knows the thing: part 2's) */
            dig_answer (r);
            break;
        }
        if (r->a == P_BEARING) {                /* (Its bearings, by its own dice: overlay 3's, bearing.c) */
            r->grade = bearing_of (r->steps);
            r->result = bway;
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
        r->grade = r->a == P_DRAW ? draw_test (r->steps) : test (&pc, r->a, r->steps);
        if (drawn_new) {
            r->result = 1;                      /* (Drawing opened by it: the first map drawn) */
            drawn_new = 0;
        }
        break;
    case RS_EXERT:                              /* (Its fatigue: its summary anew; result 1, a level more: a long
                                                **   task's soft question, the world's) */
        spent_at = r->a;
        k = pc.fl;
        spent (r->w1);
        r->result = pc.fl > k;
        status ();
        break;
    case RS_EQUIP:
        if (r->actor) {                         /* (An actor's things held and worn: the outfits' gear, gear.c's) */
            strncpy (dtext, r->text, sizeof dtext - 1);
            den_equip ((r->actor - 1) & 7);
        } else if (r->a == 2) {
            equip_all (r->text);
            hheld = r->w2;                      /* (The hands that hold something, any thing: slice 4.1b's) */
            set_load (r->w1);                   /* (And its load: its summary anew) */
            status ();
        } else {
            equip (r->text, r->a);
        }
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
        } else if (r->b == 4) {
            put (news);                         /* (What time did to it: heal.c's, told once) */
            news[0] = 0;
        } else {
            sheet ();
        }
        break;
    case RS_DICE:                               /* (A test's: its dice fixed) */
        dice_set (r);
        break;
    case RS_OUTFIT:                             /* (Its outfit's makings: overlay 1's, outfit.c, in a copy) */
        memcpy (rq, r, sizeof rq);
        outfit_of ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        break;
    case RS_CONTEST:                            /* (Each one's test, the actor's first: who did better) */
        g = side (r->actor, r->a, r->steps);
        k = side (r->other, r->b, r->c);
        r->grade = g;
        r->c = k;
        r->result = g > (signed char) k ? 1 : g < (signed char) k ? 2 : 0;
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
    harn_own_skills (own, 1);                   /* (Its own skill: Cartography) */
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
