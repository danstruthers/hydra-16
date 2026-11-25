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
** SK_OF) its kind read from the hub's catalogue (kind_in: wk).  Its summary gives each pace's step, and the notice
** tests go by the character's pace and state.  The sheet's words and the harm are overlay 1's (sheet.c), the
** fights overlay 2's (fight.c): DIR/dq0harn.ovl, read into banks of its own as it starts.  Its slots: the hub's
** three (the world's, the UI's, and the denizens' task's: DSLOT, a short one), each rung for, answered in place, the
** asker rung with the reply (hub.h's protocol).  hmchar's file read and written, and the creatures' sheets kept, are swapped part sheets's (../dq0/swap.inc: run
** in the main RAM's top, a part at a time, copied there from overlay 4's bank as it's called; harnfile.c), the names
** part names's (harnname.c), with the tool a dig takes.  And a test's, RS_DICE: its dice fixed (its unit tests').
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
#include "../dq0/life.h"
#include "dq0harn.h"
#include "../dq0/rshouse.h"

/* The engine's places on the body (items.h's PB_*: where a thing's worn) as HarnMaster's strike locations (harn.h's):
** the ruleset's mapping, the identity, the places in its locations' order (the head its skull, the wrist its forearm,
** the breast its thorax, the waist its abdomen), a limb's right side its HARN_RIGHT; checked here, for any reading of
** a place worn as a location (what's struck, what's covered) */
#define PB_HARN(p)      (p)
typedef char pb_harn_fit[PB_HEAD == HARN_SKULL && PB_FACE == HARN_FACE && PB_NECK == HARN_NECK && PB_SHOULDER ==
    HARN_SHOULDER && PB_UPPERARM == HARN_UPPERARM && PB_ELBOW == HARN_ELBOW && PB_WRIST == HARN_FOREARM && PB_HAND ==
    HARN_HAND && PB_BREAST == HARN_THORAX && PB_WAIST == HARN_ABDOMEN && PB_GROIN == HARN_GROIN && PB_HIP == HARN_HIP &&
    PB_THIGH == HARN_THIGH && PB_KNEE == HARN_KNEE && PB_CALF == HARN_CALF && PB_FOOT == HARN_FOOT && NPB ==
    HARN_NLOCATIONS && WP_RIGHT == HARN_RIGHT ? 1 : -1];

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

void __fastcall__ load_pc (struct req* r);      /* (Part sheets's stubs: bank.s's) */
void __fastcall__ save_pc (struct req* r);
void __fastcall__ dig_answer (struct req* r);   /* (Part dig_tool's) */
void __fastcall__ dice_set (struct req* r);
void __fastcall__ read_test (struct req* r);    /* (Part read_tongues's: a writing read) */
void hello_words (void);                        /* (Part read_tongues's: RS_HELLO's words) */
void __fastcall__ outfit_of (struct req* r);    /* (Overlay 1's: outfit.c, RS_OUTFIT) */
signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps);    /* (Overlay 3's) */
signed char __fastcall__ draw_test (signed char steps);     /* (Overlay 3's: a map drawn, slice 4.2's) */
unsigned char wantfl;
char news[NEWS_N];                              /* (What time did to the character, heal.c's: RS_DESCRIBE b 4's) */

char tpart[16];                                 /* (RS_TREAT's part said) */

harn_char pc;                                   /* The character */
char name[16], calling[20];
char tongues[TONGUES_N] = " common";            /* The tongues it speaks, a space before each: its own (the game's
                                                **   stand-in for every character's), and its file's "language" lines;
                                                **   and the scripts it reads and writes, each a * before it, its file's
                                                **   "script" lines (dq0harn.h's) */

/* ---- The words into a reply (put, putn) and what's to hand (tools, equip_all, has): the skeleton's (../dq0/rf/rf.c,
** rf.c's).  Digging judged by what's to hand: a skill used, the one versed in it takes out the tool that suits the job
** best (the user's: a shovel in earth, a pick in stone; a sword digs earth too, poorly, and dulls).  The book has no
** rules for digging: the game's own, every referee's (../dq0/rshouse.h's table: a tool's feet a spell of work by what
** it's into, and its wear) */

unsigned wealth;                                /* (The family's wealth, its file's "wealth" line: 0 none) */

/* The load the character carries, its equipment as harn.h has it (skills.c's set_load: one entry, its encumbering
** part, past its free pounds, D13's), so its encumbrance (EP) is the book's on it; and its skills used since their
** development rolls' stress, words not flagged yet (skills.c's: slice 4.4's) */
harn_equip carried = { "load", "load", 0, 0xFF, 0 };
unsigned char stress_lv;
unsigned char nobreak;                          /* (dq0 -I's, the tests': no weapon broken) */
unsigned char pc_might;
unsigned char fresh;

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

#pragma code-name (push, "SW_dig_tool_CODE")
#pragma rodata-name (push, "SW_dig_tool_RODATA")

/* The tool to hand that suits the material best (rshouse.h's house_dig_tool: its digger's number); it and the table part
** 3's, hdig.c (the main RAM's room, 2026-10-27) */
signed char __fastcall__ test (harn_char* c, unsigned char p, signed char steps);  /* (Overlay 10's o_test, its stub) */

/* A dig's test (RS_TEST's P_DIG): judged by the tool to hand that suits it best, its rate a spell at a marginal
** success (earth's untested: the tool's rate), its key and its wear in the reply */
void __fastcall__ s_quiet_test (struct req* r);
static void __fastcall__ dig_of (struct req* r);

/* The skill that knows a thing (rs.h's AP_*): weapons and armour Weaponcraft, metal Metalcraft, wood Timbercraft,
** hide Hideworking, cloth Textilecraft, pottery Ceramics, jewels Jewelcraft, books and bones Lore, food Cookery, stone
** Masonry, glass Glassworking, the rock where it lies Mining (part names's, the main RAM's room, 2026-10-29) */
static const unsigned char appraiser[NAP + 4] = {
    HARN_WPN, HARN_WPN, HARN_MET, HARN_TIM, HARN_HID, HARN_TEX, HARN_CER, HARN_JEW, HARN_LOR, HARN_COO, HARN_LOR,
    HARN_MSN, HARN_GLA, HARN_MIN,
    HARN_SNG, HARN_SNG, HARN_MUS, HARN_DAN      /* (Past the crafts, NAP + PF_*: a performance, entertaining one, its
                                                **   skill said, unsaid Singing's (every one's, the book's automatic skill): 2026-11-02) */
};

/* RS_TEST's own tests, through one stub (the main RAM's room): a dig's, moving quietly's, an appraisal's (by the
** craft that knows the thing) */
unsigned char __fastcall__ speaks (const char* t);    /* (Part read_tongues's: part dig_tool's room, the one-path plan's digging, 2026-11-23) */
void __fastcall__ s_dig_answer (struct req* r)
{
    unsigned char sk, m;
    int e;

    if (RS_TEST_PURPOSE (*r) == P_STEALTH || (r->actor && RS_TEST_PURPOSE (*r) != P_DIG)) {   /* (Moving quietly; or a
                                                **   creature's own test by its sheet; a dig, every actor's: dig_of) */
        s_quiet_test (r);
    } else if (RS_TEST_PURPOSE (*r) == P_APPRAISE) {
        sk = appraiser[RS_TEST_CRAFT (*r) < NAP + 4 ? RS_TEST_CRAFT (*r) : 0];
        if (RS_TEST_CRAFT (*r) < NAP) {                       /* (A thing judged, the first answer kept; a performance, a new die) */
            RS_TEST_MASTERY (*r) = pc.ml[sk];
            JUDGE_IN (RS_TEST_ITEM (*r), pc.ml[sk])
            RS_TEST_GRADE (*r) = test_sk (&pc, sk, RS_TEST_STEPS (*r));
            JUDGE_OUT
        } else {
            RS_TEST_GRADE (*r) = test_sk (&pc, sk, RS_TEST_STEPS (*r));
        }
    } else if (RS_TEST_PURPOSE (*r) == P_READ) {
        read_test (r);                          /* (A writing read: part read_tongues's, step 9's) */
    } else if (RS_TEST_PURPOSE (*r) != P_DIG && RS_TEST_PURPOSE (*r) != P_SWIM) {   /* (Persuading, intimidating, deceiving: main.c's) */
        if (!(sk = speaks (r->text))) {         /* (Words in a tongue it doesn't speak: nothing) */
            RS_TEST_CANT (*r) = 2;
        } else {
            m = RS_TEST_PURPOSE (*r) == P_DECEIVE ? HARN_NTR : HARN_RHE;
            e = pc.ml[m] ? harn_eml (&pc, m) : harn_get_oml (&pc, m) + harn_eml (&pc, m);
            e += STEP_PCT * (signed char) RS_TEST_STEPS (*r);
            if (e > sk) {                       /* (Its EML never past its mastery of the tongue: the book's, Skills 11) */
                RS_TEST_STEPS (*r) -= (e - sk + STEP_PCT - 1) / STEP_PCT;
            }
            RS_TEST_GRADE (*r) = test (&pc, RS_TEST_PURPOSE (*r), RS_TEST_STEPS (*r));
        }
    } else if (RS_TEST_PURPOSE (*r) == P_SWIM) {                /* (A minute's swimming, the book's, Skills 10: its squares, by the
                                                **   grade's yards (calm water) times its skill index; it sinks at
                                                **   a critical failure; its breath held Endurance times 10 seconds) */
        RS_TEST_GRADE (*r) = test_sk (&pc, HARN_SWI, RS_TEST_STEPS (*r));
        r->result = pc.ml[HARN_SWI] < 10 ? 1 : pc.ml[HARN_SWI] / 10;
        RS_TEST_SQUARES (*r) = (RS_TEST_GRADE (*r) == G_WELL ? 9 : RS_TEST_GRADE (*r) == G_PASS ? 7 : RS_TEST_GRADE (*r) == G_FAIL ? 2 : 0) * r->result * 3 / SQ_FT;
        r->result = harn_end (&pc);
        RS_TEST_BREATH (*r) = (r->result << 3) + (r->result << 1);    /* (Times ten: no multiplying, the main RAM's) */
        r->result = 0;
    } else {
        dig_of (r);
    }
}

static void __fastcall__ dig_of (struct req* r)
{
    unsigned char m = RS_TEST_MAT (*r) > MAT_SOIL ? MAT_STONE : RS_TEST_MAT (*r), k;

    if (!(k = house_dig (r)) || RS_TEST_STEPS (*r) == RS_DIG_QUERY) {
        return;                                 /* (Nothing bites; or only the tool and its rate asked: rshouse.h's,
                                                **   every actor's, the one-path plan's digging, 2026-11-23) */
    }
    if (!DIG_TESTED (m)) {
        RS_TEST_GRADE (*r) = G_PASS;
    } else if (r->actor) {
        s_quiet_test (r);                       /* (A creature's by its sheet, on dice of its own: slice 4.11's) */
    } else {
        RS_TEST_GRADE (*r) = test (&pc, RS_TEST_PURPOSE (*r), RS_TEST_STEPS (*r));
    }
    RS_TEST_RATE (*r) = RS_TEST_GRADE (*r) == G_WELL ? 2 * k : RS_TEST_GRADE (*r) == G_PASS ? k : RS_TEST_GRADE (*r) == G_FAIL ? k / 2 : 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- A purpose: the skill a GM would call for, or an attribute (an attribute | 0x80: tested times 5) */

#define ATTR    0x80
#pragma rodata-name (push, "OV10RODATA")       /* (With o_test, overlay 10's: the main RAM's room, speed12) */
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
    HARN_ACT,                                                           /* Feigning: Acting */
    HARN_ORA,                                                           /* A speech: Oratory */
    HARN_RAGR,                                                          /* A prayer: the deity's own Ritual
                                                                        **   (test's: Agrik's the first) */
    HARN_LEG,                                                           /* A thing filched: Legerdemain (the
                                                                        **   user's, 2026-11-23) */
    HARN_TIM,                                                           /* A door hung: Timbercraft */
    HARN_MSN,                                                           /* A way walled up: Masonry */
};
#pragma rodata-name (pop)

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
    static const signed char g[4] = { G_BADLY, G_FAIL, G_PASS, G_WELL };

    return g[level & 3];
}

#pragma code-name (push, "OV5CODE")            /* (Overlay 5's, beside part names's image: 2026-10-27) */
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
    if (c == &pc) {
        skill_used (sk);                        /* (Marked for its development roll; a [First Use] one opened: slice
                                                **   4.4's, skills.c) */
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

/* A purpose's test (RS_TEST's, part dig_tool's): overlay 10's, its stub test (the main RAM's room, speed12, 2026-11-22: no hub in
** it) */
#pragma code-name (push, "OV10CODE")
#pragma rodata-name (push, "OV10RODATA")
signed char __fastcall__ o_test (harn_char* c, unsigned char p, signed char steps)
{
    if (p >= P_THEME) {
        return grade (harn_test (50 + STEP_PCT * steps));   /* (A theme's own: till themes say, an even chance) */
    }
    if (p == P_STUMBLE) {                       /* (The book's stumble roll, Skills 22: 3d6, the physical penalty and
                                                **   the circumstances, over Agility a fall; a point of the roll two
                                                **   steps, the book's darkness 2 points, a heavy obstruction's 2) */
        return harn_stumble_roll (c, -(int) steps / 2) ? G_FAIL : G_PASS;
    }
    if (p == P_RISE) {
        return rise_of (c);                     /* (Getting up: the house's test, if any, gear.c's) */
    }
    if (p == P_RITUAL) {                        /* (A prayer: its deity's Ritual, Agrik's ... Siem's in harn.h's order;
                                                **   none worshipped, a critical failure: the UI's to ask first, its
                                                **   RD_DEITY) */
        if (c->att[HARN_DTY] >= HARN_NODEITY) {
            return G_BADLY;
        }
        return test_sk (c, HARN_RAGR + c->att[HARN_DTY], steps);
    }
    return test_sk (c, purpose[p], steps);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The character's state, in the hub's header: the UI's status line, and its summary (rs.h's: what the world
** needs often, its senses first: what it sees and hears is through them) */

/* A sense, 0-9, from its attribute (3-18 as rolled: 10 or 11 an average 4 or 5); what hurts it comes later (an
** eye's or a head's wounds, a helm) */
static const unsigned char sense_of[18] = { 0, 0, 0, 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6, 7, 7, 8 };  /* ((v - 3) * 3 / 5,
                                                **   none under 4: a blow's summary's divisions, speed10's) */
static unsigned char __fastcall__ sense (unsigned char a)
{
    unsigned v = pc.att[a];

    return v >= 18 ? 9 : sense_of[v];
}

/* ---- Fatigue (sheet.c's spent: the book's movement fatigue) */

unsigned tired, rested;                         /* Exertion toward the next level, rest toward the next gone */
unsigned char spent_at;                         /* (The pace the time went at, or GA_WORK, GA_REST: spent's) */

/* (Its limbs and its conditions: gear.c's, overlay 3's, the main RAM's room: limbs_of, cond_of) */
unsigned char lhurt[4];
unsigned char cplan = BF_BIPED;
unsigned char hheld;                            /* (The hands that hold something: RS_EQUIP 2's w2, DH_*) */

static unsigned char pc_ep;                     /* (Its encumbrance penalty, as the summary's rest last reckoned it: its
                                                **   load and its endurance, which no blow, harm or time changes) */

/* The character's summary in the hub, in two halves (the user's T4, 2026-11-22: the referee skeleton's rf_later).  Its
** own fields of what a blow, a harm, time or exertion changes, which the world reads as the reply comes (rs.h's cond,
** health, limbs, limp, step and gstep: wfight.c's knockout latch, fight_on's shock and health, a walk's pace), and the
** status line: before the reply (status_now) */
static unsigned char sk, sl, ss, sw;            /* (The steps last worked out by: its walk's k, its limp, in shock; the
                                                **   step: status_now's, k never 0) */
static void status_now (void)
{
    static const char* const word[4] = { "", " OUT", " SHOCK", " DEAD" };
    char b[8];
    struct rs_summary* s = &HUB->pcsum;
    static const unsigned char rate[3] = { 100, 50, 33 };
    unsigned char up = harn_get_up (&pc), pp = up + pc_ep, k, w, g, v, l;

    strcpy (HUB->pcstatus, "UP ");             /* (Its PP once: the load's sum each time, room6's) */
    strcat (HUB->pcstatus, utoa (up, b, 10));
    tnow = HUB->now;
    if (pc.state == HARN_UNCONSCIOUS) {
        rouse (&pc, &pc_out, 0);                /* (Senseless: its shock rolls due by now, as its time comes) */
    }
    s->cond = conditions ();
    if (pc_out > tnow) {
        s->cond |= C_UNCONSCIOUS;               /* (Senseless till it comes round: the minds' to know it helpless) */
    }
    strcat (HUB->pcstatus, s->cond & C_UNCONSCIOUS ? word[1] : pc.state ? word[pc.state & 3] : s->cond & C_BLEEDING ?
        " BLEEDING" : "");
    if (ko_at[0] && pc.state == HARN_DEAD) {
        s->cond |= C_UNCONSCIOUS;               /* (Its fall's time kept till it comes round: dead as it lay, the world's
                                                **   senseless still, no turn of its own after; its status as it was) */
    }
    s->health = health_of (&pc);
    k = pc.att[HARN_AGL] > pp ? pc.att[HARN_AGL] - pp : 1;  /* (harn_move's: walking: half its Move, squares a round: Combat 6; in shock, half
                                                **   that, and no faster: Skills 22's walk or crawl at half move) */
    if (pc.state == HARN_SHOCK) {
        ++k;
        k >>= 1;
    }
    s->limbs = v = limbs_of (&pc);              /* (Its hands and limbs; a leg hurt, its walk the slower, the
                                                **   house's: each level of the injuries to the legs it walks on a
                                                **   fifth more time a square, slice 4.1b's) */
    s->limp = l = (v & LB_LEG_L ? 0 : lhurt[2]) + (v & LB_LEG_R ? 0 : lhurt[3]);
    v = pc.state == HARN_SHOCK;
    if (k == sk && l == sl && v == ss && s->step == sw) {
        return;                                 /* (Its steps by the same: as they were, their divisions spared; a blow's
                                                **   as most leave them, speed11's) */
    }
    sk = k;
    sl = l;
    ss = v;
    w = k < 4 ? 50 : 200 / k;
    if (l) {                                    /* (A limp's: its fifths; none, the same, its divisions spared) */
        w = (unsigned) w * (5 + l) / 5 > 255 ? 255 : (unsigned) w * (5 + l) / 5;
    }
    s->step = sw = w;
    s->gstep[GA_CREEP] = w > 85 ? 255 : w + (w << 1);   /* (The game's: a sixth of its Move, a quarter; the book's: a
                                                **   jog its Move, a run twice, a sprint three times) */
    s->gstep[GA_SLOW] = w > 127 ? 255 : w << 1;
    s->gstep[GA_WALK] = w;
    for (g = GA_JOG; g < NGAITS; ++g) {
        v = rate[g - GA_JOG] / k;
        if (l) {
            v = (unsigned) v * (5 + l) / 5 > 255 ? 255 : (unsigned) v * (5 + l) / 5;
        }
        s->gstep[g] = pc.state == HARN_SHOCK ? w : !v ? 1 : v < w ? v : w;
    }
}

/* And the rest (its encumbrance first, the steps' by it), after the reply: rf_after's; or before the half above, the
** whole summary at once (status) */
static void status_rest (void)
{
    struct rs_summary* s = &HUB->pcsum;
    unsigned char ep = pc_ep = harn_get_ep (&pc);

    s->attacks = 1;
    s->sight = sense (HARN_EYE);
    s->hearing = sense (HARN_HRG) + (HUB->watch & WA_LISTEN ? 2 : 0);  /* (Listening as it goes: two better) */
    if (s->hearing > 9) {
        s->hearing = 9;
    }
    if (HUB->pc_task.kind == LT_SLEEP) {        /* (Asleep: at half, the UI's reach of a sound its hearing plus three: the
                                                **   book's, the user's, 2026-11-03) */
        s->hearing = s->hearing > 2 ? ((s->hearing + 3) >> 1) - 3 : 0;
    }
    s->smell = sense (HARN_SML);
    s->darkvision = 0;                          /* (The species' sight in the dark: the book's, to come) */
    s->nightvision = 1;
    s->morale = pc.att[HARN_MOR];
    s->load = !ep ? 0 : ep <= 2 ? 1 : ep <= 4 ? 2 : 3;   /* (Its encumbrance: a little burdened, heavily, overloaded) */
    s->power = pc_might;                        /* (As a creature's: skills.c's, the user's, 2026-11-03) */
    s->lead = pc.ml[HARN_ORA] > pc.ml[HARN_RHE] ? pc.ml[HARN_ORA] : pc.ml[HARN_RHE];   /* (Its influence: the
                                                **   morale field's, HOUSE.md's) */
    s->northsense = pc.att[HARN_SP] == HARN_KHU;    /* (The species' trait: the Khuzdul live below, and always know
                                                **   north there) */
    s->mapmind = pc.att[HARN_INT] >= 20 ? 100 : 5 * pc.att[HARN_INT];  /* (Its map in mind, the house's: five times its
                                                **   Intelligence, of a hundred; a tenth of it lost every twice its
                                                **   Intelligence and Will together, minutes) */
    s->mapfade = 2 * (pc.att[HARN_INT] + pc.att[HARN_WIL]);
}

static void status (void)
{
    status_rest ();
    status_now ();
}

/* (The skeleton's: after a reply that left the summary's rest, rf_later's) */
void rf_after (void)
{
    status_rest ();
}

#pragma code-name (push, "SW_sheets_CODE")
#pragma rodata-name (push, "SW_sheets_RODATA")

/* ---- hmchar's file: its own lines, the name and the occupation (and the game's: its pace, its watchfulness and its
** exertion toward the next fatigue level, "pace 2 1 1200": the pace and the watchfulness the world's, said in the reply
** to RS_LOAD_PC and written back as the hub has them, the exertion the referee's own; and what dealt each injury,
** a digit each in order, "dealt 0100": pc_by's, sheet.c's BY_*) */

static unsigned char gait = GA_WALK, watch;
static const char k_name[] = "name ", k_occ[] = "occupation ", k_dealt[] = "dealt ", k_dressed[] = "dressed ",
    k_time[] = "timesense";                    /* (Its lines' keys, read and written: once, part sheets's room, 2026-11-18) */

static void __fastcall__ own_line (char* l)
{
    unsigned char k;

    if (!strncmp (l, k_dealt, 6)) {
        for (k = 0; k < HARN_INJMAX && l[k + 6]; ++k) {
            pc_by[k] = l[k + 6] & 7;            /* (A digit: BY_* are 0-4) */
        }
    } else if (!strncmp (l, k_dressed, 8)) {
        dress_line (l + 8, 0);                  /* (Its dressings: dress.c's pc_dr, overlay 9's) */
    } else if (!strncmp (l, "pace ", 5) && strlen (l) >= 10) {
        gait = l[5] - '0' < NGAITS ? l[5] - '0' : GA_WALK;
        watch = l[7] - '0';
        tired = atoi (l + 9);
    } else if (!strncmp (l, k_name, 5)) {
        strncpy (name, l + 5, sizeof name - 1);
    } else if (!strncmp (l, k_occ, 11)) {
        strncpy (calling, l + 11, sizeof calling - 1);
    } else if (!strncmp (l, "body ", 5)) {
        body_line (l + 5, 1);                   /* (Its venom, its rest toward its healing rolls: heal.c's) */
    } else if (!strncmp (l, "wealth ", 7)) {
        wealth = atoi (l + 7);                  /* (Its family's, hmchar's: RS_OUTFIT's) */
    } else if (!strcmp (l, k_time)) {
        HUB->pcsum.timesense = 1;               /* (Absolute timesense: hmchar's, the house's rare trait, 2026-10-31:
                                                **   the world's, the hub's tb; the hub in the window as the file's
                                                **   read, its request's) */
    } else {
        body_line (l, 4);                       /* (Its "language" and "script" lines: afflict.c's, into tongues) */
    }
}

static char path[80];                           /* (The file's path read, and a line of it written: s_save_pc's line too, the
                                                **   main RAM's room for the tongues, the plan's step 12) */

void __fastcall__ s_load_pc (struct req* r)
{
    int fd;

    if (r->kind == RS_HELLO) {
        RS_HELLO_ROUND (*r) = 100;                            /* (A round: 10 seconds, Combat 7) */
        put ("HarnMaster 3.5\nhmchar");         /* (Its character generator: hmchar's files, RS_LOAD_PC's; its
                                                **   words after them, part read_tongues's: rf_answer's) */
        return;
    }
    RS_LOAD_PC_GAIT (*r) = gait;                                /* (Its file's pace and watchfulness: the world's to set) */
    RS_LOAD_PC_WATCH (*r) = watch;
    strncpy (path, r->text, sizeof path - 1);
    RS_LOAD_PC_BAD (*r) = 1;
    memset (pc_dr, 0, sizeof pc_dr);           /* (No dressings: its file's "dressed" line's) */
    unrouse ();                                 /* (Not senseless: its shock rolls none, felled1's) */
    pv.hr = pd.hr = afffl = 0;                 /* (No venom, no sickness, no rest yet: its file's "body" line's; */
    body_line (path, 3);                       /*   no skills used yet, its "body u" lines') */
    rest_m = HUB->pcsum.timesense = 0;
    if ((fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    RS_LOAD_PC_BAD (*r) = harn_read (fd, &pc, own_line) < 0 ? 1 : *tongues == '!' ? 2 : 0;   /* (2: more tongues and scripts
                                                **   than it can hold, refused, the UI's to say so) */
    close (fd);
    develop (0);                                /* (Its might, by its skills) */
    hub_map (0);
    put (name);
    put ("\n");
    put (calling);
    status ();
    RS_LOAD_PC_GAIT (*r) = gait;
    RS_LOAD_PC_WATCH (*r) = watch;
}

/* The character's sheet written (r's text a path): 0, or 1 */
#define line    path
void __fastcall__ s_save_pc (struct req* r)
{
    char b[12];
    unsigned char g = HUB->gait, w = HUB->watch;
    int fd;

    RS_SAVE_FAILED (*r) = 1;
    if ((fd = open (r->text, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    harn_write (fd, &pc);
    strcpy (line, k_name);
    strcat (line, name);
    harn_write_line (fd, line);
    strcpy (line, k_occ);
    strcat (line, calling);
    harn_write_line (fd, line);
    strcpy (line, "pace 0 0 ");                 /* (The game's: its pace, its watchfulness, its exertion) */
    line[5] += g;
    line[7] += w;
    strcat (line, utoa (tired, b, 10));
    harn_write_line (fd, line);
    body_line (line, 0);                        /* (Its venom, its rest: heal.c's) */
    harn_write_line (fd, line);
    while (body_line (line, 2), line[0]) {      /* (Its skills' growth: slice 4.4's, 2026-11-03; then its tongues
                                                **   and its scripts, "language" and "script" lines: afflict.c's) */
        harn_write_line (fd, line);
    }
    strcpy (line, k_dealt);                    /* (What dealt each injury: its words') */
    for (g = 0; g < pc.ninj; ++g) {
        line[g + 6] = '0' | pc_by[g];
    }
    line[g + 6] = 0;
    harn_write_line (fd, line);
    strcpy (line, k_dressed);                  /* (Its dressings: dress.c's pc_dr, HOUSE.md's bandages) */
    dress_line (line + 8, 1);
    if (line[8]) {
        harn_write_line (fd, line);
    }
    if (HUB->pcsum.timesense) {
        harn_write_line (fd, k_time);      /* (Its trait, kept: the house's) */
    }
    close (fd);
    RS_SAVE_FAILED (*r) = 0;
}
#undef line

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The creatures' sheets (fight.c's): a bank of the referee's, a sheet a denizen's record (its number the
** denizens' task's), kept in the save folder's beasts.dat with the game's other records (read back as the first
** creature's asked for, a game carried on) */

static unsigned char sb;                        /* Their bank */
unsigned char fb;                               /* (Their bodies' bank: each one's fatigue, struct bbody, 2026-10-31) */
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

#pragma code-name (push, "SW_names_CODE")       /* (Part sheets's room, 2026-10-27) */
#pragma rodata-name (push, "SW_names_RODATA")

/* The bank written to the save folder's beasts.dat (back 1: read from it, or cleared: none there; 2: cleared): the
** skeleton's rf_sheets (../dq0/referee.h's; the file opened by its sheets_open, rfsheets.c's, part names's too) */
void __fastcall__ s_rf_sheets (unsigned char back)
{
    unsigned off;
    char* p;
    int fd;
    unsigned char k;

    memset (piece, 0, sizeof piece);
    fd = back < 2 ? sheets_open (back) : -1;
    for (k = 0; !back && k < NACTORS; ++k) {    /* (The actors' falls and shock rolls, K16's, kept in the sheets past
                                                **   the denizens' records (KOSAVE): beasts.dat no bigger) */
        piece[0] = aden[k];
        memcpy (piece + 1, &ko_at[k + 1], 4);
        memcpy (piece + 5, &ko_n[k + 1], 2);
        bwrite (sb, KOSAVE + 7 * k, piece, 7);
    }
    for (off = 0; off < 0x2000 + BBODIES + (back == 2 ? KOSZ : 0); off += sizeof piece) {  /* (The sheets, then their
                                                **   bodies, fb's; cleared, the falls and rolls by sheet too, fb's past
                                                **   the skills' growth: KOS, K16's, not kept but the actors') */
        p = (char*) 0x8000 + (off & 0x1FFF) + (off >= 0x2000 + BBODIES ? 0x100 : 0);
        if (!back) {
            bread (off < 0x2000 ? sb : fb, p, piece, sizeof piece);
            write (fd, piece, sizeof piece);
        } else {
            if (fd < 0 || read (fd, piece, sizeof piece) != sizeof piece) {
                memset (piece, 0, sizeof piece);
            }
            bwrite (off < 0x2000 ? sb : fb, p, piece, sizeof piece);
        }
    }
    for (k = 0; back == 1 && k < NACTORS; ++k) {    /* (Read back: the actors' falls and rolls by their sheets, KOS,
                                                **   as RS_NEW_ACTOR finds them) */
        bread (sb, KOSAVE + 7 * k, piece, 7);
        if (piece[1] | piece[2] | piece[3] | piece[4]) {
            bwrite (fb, KOS + piece[0] * 6, piece + 1, 6);
        }
    }
    if (fd >= 0) {
        close (fd);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* Whether it speaks tongue t (a word of tongues, a space before it, =its mastery after it if it isn't its own, its file's: part names's,
** the main RAM's room, 2026-10-31; part read_tongues's since the one-path plan's digging, 2026-11-23, part dig_tool's room): 0, or its
** mastery in it (its own its Language's; 100 if it has none open) */
#pragma code-name (push, "SW_read_tongues_CODE")
#pragma rodata-name (push, "SW_read_tongues_RODATA")
unsigned char __fastcall__ s_speaks (const char* t)
{
    char* p = tongues;
    unsigned char n = strlen (t);

    while ((p = strstr (p, t)) != 0) {
        if (p[-1] == ' ' && (!p[n] || p[n] == ' ' || p[n] == '=')) {
            n = p[n] == '=' ? atoi (p + n + 1) : pc.ml[HARN_LAN];
            return n ? n : 100;
        }
        ++p;
    }
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- A writing read (RS_TEST's P_READ, rs.h's: the plan for the game between dungeons, its step 9): part read_tongues's */
#pragma code-name (push, "SW_read_tongues_CODE")
#pragma rodata-name (push, "SW_read_tongues_RODATA")

/* RS_HELLO's words after its generator's (rs.h's): the state C_STUNNED is, the book's shock (Combat 14); its own stance,
** D_OWN, the counterstrike, and that stance told (part sheets's s_load_pc's, its room's: 2026-11-12) */
void s_hello_words (void)
{
    put ("\nin shock\ncounter\nYou will strike back first.");
}

/* Its mastery of a tongue or a script (t: "khuzdul", "*delvic": a word of tongues, dq0harn.h's): 0 none; its own (no ML said)
** the Language skill's, a script's the Script skill's */
static unsigned char __fastcall__ known (const char* t)
{
    char* p = tongues;
    unsigned char n = strlen (t);

    while ((p = strstr (p, t)) != 0) {
        if (p[-1] == ' ' && (!p[n] || p[n] == ' ' || p[n] == '=')) {
            return p[n] == '=' ? atoi (p + n + 1) : pc.ml[*t == '*' ? HARN_SCR : HARN_LAN];
        }
        ++p;
    }
    return 0;
}

/* The writing's tongue and script in the text ("khuzdul *delvic"): only one who knows both reads it (the book's, Skills 11),
** tested at their masteries' average, less its penalty, its condition's steps (once: the UI keeps what was read, the clue's).
** Result 1 no script at all, 2 not the tongue, 3 not the script */
void __fastcall__ s_read_test (struct req* r)
{
    unsigned char t, s;
    int e;

    *strchr (r->text, ' ') = 0;
    t = known (r->text);
    s = known (r->text + strlen (r->text) + 1);
    RS_TEST_CANT (*r) = !strchr (tongues, '*') ? 1 : !t ? 2 : !s ? 3 : 0;
    if (!RS_TEST_CANT (*r)) {
        e = ((t + s) >> 1) - 5 * harn_get_pp (&pc) + STEP_PCT * (signed char) RS_TEST_STEPS (*r);
        RS_TEST_GRADE (*r) = grade (harn_test (e));
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The character's state in words (RS_DESCRIBE b 3: overlay 1's; a 1, its pace and watchfulness only, the hub's): what
** the words need copied for the overlay (the hub's not in the window while it is) */
struct pstate ps;

void __fastcall__ pace_set (unsigned char a);
void __fastcall__ stress (struct req* r);
void age_get (void);
#pragma code-name (push, "SW_dig_tool_CODE")    /* (The main RAM's room, 2026-11-03) */
#pragma rodata-name (push, "SW_dig_tool_RODATA")

/* Its age, for the sheet (the plan for the game between dungeons, its step 8): by the life's record (life.h's, at the
** top of the save's folder: the year it was born, the day this dungeon began: its age as the dungeon began, LIFE_AGE's),
** its birthday its sheet's; 0 without a record (no save's folder, dq0 -d's).  The record read into piece, the creatures'
** sheets' (s_rf_sheets' alone); the days since its birth year began in 16 bits (no long division's code in the main RAM) */
unsigned char age;

void s_age_get (void)
{
    char* path = (char*) piece + sizeof (struct life_head);
    const struct life_head* h = (const struct life_head*) piece;
    unsigned d;
    int fd;

    age = 0;
    if (!HUB->savedir[0]) {
        return;
    }
    strcpy (path, HUB->savedir);
    strcat (path, "/../" LIFE_FILE);
    if ((fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    if (read (fd, piece, sizeof (struct life_head)) == sizeof (struct life_head) && h->magic == LIFE_MAGIC) {
        d = (unsigned) (h->date - h->born * 360UL);
        age = d / 360 - (d % 360 < ((pc.att[HARN_BD] >> 8) - 1) * 30 + (pc.att[HARN_BD] & 0xFF) - 1);
    }
    close (fd);
}

void __fastcall__ s_pace_set (unsigned char a)
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

/* What a skill's used under, for its growth (HOUSE.md's, the user's, 2026-11-03: "a use under stress"), the character's
** test r or its treatment (the hub in the window): a point each for a hostile awake within 30 feet (a hard stop's reach),
** a trap's danger (disarmed, or dodged), the dark without a light; wounds and shock skills.c's own */
void __fastcall__ s_stress (struct req* r)
{
    struct actor* q = HUB->actor;
    unsigned char k, n = r->kind == RS_TEST && (RS_TEST_PURPOSE (*r) == P_DISARM || RS_TEST_PURPOSE (*r) == P_DODGE_HAZARD);
    unsigned dx, dy;

    for (k = NACTORS; k; --k, ++q) {
        dx = q->x > HUB->pc_x ? q->x - HUB->pc_x : HUB->pc_x - q->x;
        dy = q->y > HUB->pc_y ? q->y - HUB->pc_y : HUB->pc_y - q->y;
        if (q->state && q->attitude == AT_HOSTILE && q->sn == HUB->pc_stratum && q->doing != DO_SLEEP && dx <= 6 && dy <= 6) {
            ++n;
            break;
        }
    }
    if (!HUB->pc_light) {
        hub_map (SIGHT_BANK);
        n += SIGHT->dark != 0;
        hub_map (0);
    }
    stress_lv = n;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The dice fixed, for a test (RS_DICE: the referee's unit tests, tests/ram/t_rules): dice.h's hook, set by the
** first, rolls every die after it, the faces given first (then the generator's), and keeps what each was (its sides) */

#define DFIX    sizeof ((struct req*) 0)->x
static char dface[DFIX + 1];                    /* The faces forced, to a 0, */
static unsigned char dk, drep;                  /*   the next, and the face after them (0: the generator's) */
static unsigned char dlog[DFIX], dl;            /* The sides of the first dice rolled, how many kept */
static unsigned drolled;                        /* The dice rolled */

int __fastcall__ fixed (unsigned sides);        /* (Its stub, bank.s's: part names's since 2026-10-31, the main RAM's room;
                                                **   a test's alone) */

#pragma code-name (push, "SW_names_CODE")
#pragma rodata-name (push, "SW_names_RODATA")

int __fastcall__ s_fixed (unsigned sides)
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

void __fastcall__ s_dice_set (struct req* r)
{
    if (RS_DICE_SEEDED (*r)) {
        rseed (RS_DICE_SEED (*r));
    }
    RS_DICE_ROLLED (*r) = drolled;
    RS_DICE_UNROLLED (*r) = strlen (dface + dk);
    memcpy (dface, RS_DICE_FACES (*r), DFIX);
    dk = 0;
    drep = RS_DICE_REST (*r);
    memcpy (RS_DICE_SIDES (*r), dlog, DFIX);
    memset (dlog, 0, DFIX);
    dl = 0;
    drolled = 0;
    dice_hook = fixed;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- A request answered */

static unsigned char rq[offsetof (struct req, text)];  /* (A request's copy, for the overlay: but its text) */
/* One side of two actors' tests (RS_CONTEST): who's test of purpose p, at the steps: the character's here (one
** senseless, or in shock, tests nothing: a critical failure), a creature's by its sheet (overlay 2's, in a copy) */
signed char __fastcall__ side (unsigned char who, unsigned char p, signed char steps);
#pragma code-name (push, "SW_dig_tool_CODE")    /* (The main RAM's room, 2026-11-03) */
#pragma rodata-name (push, "SW_dig_tool_RODATA")
signed char __fastcall__ s_side (unsigned char who, unsigned char p, signed char steps)
{
    struct req* q = (struct req*) rq;

    tnow = HUB->now;
    if (!who) {
        return pc.state != HARN_CONSCIOUS || pc_out > tnow ? G_BADLY : test (&pc, p, steps);
    }
    q->kind = RS_CONTEST;
    q->actor = who;
    RS_CONTEST_PURPOSE (*q) = p;
    RS_CONTEST_STEPS (*q) = steps;
    fight (q);
    return RS_CONTEST_GRADE (*q);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* Moving quietly's test (RS_TEST's P_STEALTH: slice 4.6's), by its own dice (rshouse.h's QUIET_IN's: the game's
** as they stand, mixed, and put back after), so the game's other tests roll as they did: the character's, or a
** creature's by its sheet (a hunter's stalk, the denizens'); part names's, the main RAM's room */
#pragma code-name (push, "SW_dig_tool_CODE")
#pragma rodata-name (push, "SW_dig_tool_RODATA")
void __fastcall__ s_quiet_test (struct req* r)
{
    if (r->other == RS_ASKER) {
        RS_TEST_GRADE (*r) = side (r->actor, RS_TEST_PURPOSE (*r), RS_TEST_STEPS (*r));
        return;
    }
    QUIET_IN
    RS_TEST_GRADE (*r) = side (r->actor, RS_TEST_PURPOSE (*r), RS_TEST_STEPS (*r));  /* (The character's, or a creature's by its sheet: a tunneller's
                                                **   dig too, slice 4.11's) */
    QUIET_OUT
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A request answered (the skeleton's rf_answer, ../dq0/referee.h's: tp and tend at its text, its result 0; the
** creatures' sheets read back before a creature's asked for, written after the character's, the skeleton's) */
void __fastcall__ scene_rs (struct req* r);   /* (Part scene's: hscene.c) */
void __fastcall__ rf_answer (struct req* r)
{
    unsigned char k, m, t;
    signed char g;

    stress_lv = 0;                              /* (A skill's use here routine, but as it's found below: skills.c's) */
    if (pc.state == HARN_UNCONSCIOUS && r->kind != RS_DICE && r->kind != RS_SCENE) {
        tnow = HUB->now;
        rouse (&pc, &pc_out, 0);                /* (The character senseless: its shock rolls due by now first, whatever's
                                                **   asked, as its time comes; its time out the next's, 2026-11-21) */
    }
    switch (r->kind) {
    case RS_NEW_ACTOR:                          /* (The fights': overlay 2's, in a copy, as the harm's) */
        strncpy (bkey, r->text, sizeof bkey - 1);
    case RS_ATTACK:
    case RS_MORALE:
    case RS_WIELD:                              /* (What it would strike with: gear.c's, through fight's) */
    case RS_ROUSE:                              /* (A senseless one's shock rolls due: fight.c's) */
fights:
        tnow = r->kind == RS_ATTACK && RS_ATTACK_WHEN (*r) ? RS_ATTACK_WHEN (*r) : HUB->now;   /* (A blow at its
                                                **   moment, the world's: a denizen's its intent's, 2026-11-13) */
        nobreak = HUB->pc_lump & PL_NOFACE;
        stance = HUB->pc_stance;
        pc_way = HUB->pc_strike;
        pc_hand = HUB->pc_shand;
        if (SK_IS_KIND (pc_way)) {
            kind_in (SK_KIND (pc_way));            /* (The thing named that strikes: its kind, for the overlay) */
        }
        gseed = HUB->seed;
        memcpy (rq, r, sizeof rq);
        harn_ep_kept = harn_get_ep (&pc);       /* (Its encumbrance once for the blow's dozen tests: no blow */
        harn_ep_of = &pc;                       /*   changes its load or its endurance; speed12, 2026-11-22) */
        fight ((struct req*) rq);
        harn_ep_of = 0;
        memcpy (r, rq, sizeof rq);
        if (r->kind != RS_WIELD) {              /* (What it would strike with changes nothing of it: no summary,
                                                **   some 40,000 cycles a fight-back spared, fix2, 2026-11-24) */
            status_now ();                      /* (The blow's own summary in the reply, the rest after it: */
            rf_later = 1;                       /*   rf_after's, the skeleton's, the user's T4, 2026-11-22) */
        }
        break;
    case RS_HARM:                               /* (Harm, wounds over time, first aid: overlay 6's, heal.c, in a */
    case RS_TIME:                               /*   copy, the part said copied for it; the harm overlay 1's, its */
    case RS_TREAT:                              /*   words a line) */
        if (r->kind == RS_HARM && r->actor) {
            goto fights;                        /* (A creature's harm: its sheet's, the fights' overlay's, 2026-11-23) */
        }
        tnow = HUB->now;
        if (r->kind == RS_TREAT) {
            stress (r);
        }
        strncpy (tpart, r->text, sizeof tpart - 1);
        memcpy (rq, r, sizeof rq);
        memcpy (rq + offsetof (struct req, x) + WANT_X, &HUB->pc_hunger, 6);    /* (The hub's want: slice 4.9's) */
        memcpy (rq + offsetof (struct req, x) + 16, &r->w1, 2);     /* (Its minutes, the reply's w1 after) */
        body_req ((struct req*) rq);
        if (r->kind == RS_TIME) {
            want ((struct req*) rq);            /* (Hunger and thirst: overlay 7's, want.c) */
            memcpy (&HUB->pc_hunger, rq + offsetof (struct req, x) + WANT_X, 6);
        }
        memcpy (r, rq, sizeof rq);
        status_now ();                          /* (Its own summary in the reply, the rest after it) */
        rf_later = 1;
        break;
    case RS_HELLO:                              /* (Part sheets's, as the game's begun: the main RAM's room, 2026-11-02;
                                                **   its words part read_tongues's, part sheets's room) */
        load_pc (r);
        hello_words ();
        break;
    case RS_LOAD_PC:
        load_pc (r);
        break;
    case RS_TEST:
        if (!r->actor && RS_TEST_PURPOSE (*r) > P_NOTICE_SMELL) {
            stress (r);                         /* (A glance's, a sound's, a smell's routine: they're many) */
        }
        if (r->actor || RS_TEST_PURPOSE (*r) == P_DIG || RS_TEST_PURPOSE (*r) == P_STEALTH || RS_TEST_PURPOSE (*r) == P_APPRAISE || RS_TEST_PURPOSE (*r) == P_SWIM) {   /* (A creature's
                                                **   by its sheet, any test (Phase 3C's: a door forced); judged by the tool to hand that
                                                **   suits it best; moving quietly, by its own dice; by the craft that
                                                **   knows the thing: part dig_tool's) */
parted:
            dig_answer (r);
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_BEARING) {                /* (Its bearings, and the time it keeps (rs.h's TIME_STEPS), by
                                                **   their own dice: overlay 5's, bearing.c) */
            RS_TEST_GRADE (*r) = bearing_of (RS_TEST_STEPS (*r));
            RS_TEST_DRIFT (*r) = bway;
            break;
        }
        if ((RS_TEST_PURPOSE (*r) == P_SEARCH || RS_TEST_PURPOSE (*r) == P_DISARM) && RS_TEST_HOW (*r) == HIDE_MAGICAL) {   /* (Only by magic: its spells
                                                **   to come) */
            RS_TEST_CANT (*r) = 1;
            break;
        }
        if (RS_TEST_PURPOSE (*r) == P_PICKLOCK && !has ("picks")) {    /* (Lockcraft wants its tools: Skills 23) */
            RS_TEST_CANT (*r) = 1;
            break;
        }
        if (((unsigned char) (RS_TEST_PURPOSE (*r) - P_READ) <= P_INTIMIDATE - P_READ || RS_TEST_PURPOSE (*r) == P_DECEIVE) && r->text[0]) {   /* (Words in a
                                                **   tongue: one it speaks, or nothing; a writing read, its tongue and
                                                **   its script (P_READ, P_LORE with words; P_TRACK never has any): part dig_tool's) */
            goto parted;
        }
        if (RS_TEST_PURPOSE (*r) <= P_NOTICE_SMELL) {           /* (By its state: its pace, its shock; overlay 1's, in a copy) */
            ps.go = HUB->pc_task.kind == LT_SLEEP ? 0xFF : HUB->pc_going;   /* (0xFF asleep: at half, sheet.c's) */
            ps.watch = HUB->watch;
            memcpy (rq, r, sizeof rq);
            RS_TEST_STEPS (*r) += noticing ((struct req*) rq);
        }
        RS_TEST_GRADE (*r) = RS_TEST_PURPOSE (*r) == P_DRAW ? draw_test (RS_TEST_STEPS (*r)) : test (&pc, RS_TEST_PURPOSE (*r), RS_TEST_STEPS (*r));
        if (drawn_new) {
            RS_TEST_OPENED (*r) = 1;            /* (Drawing opened by it: the first map drawn) */
            drawn_new = 0;
        }
        break;
    case RS_EXERT:                              /* (Its fatigue: its summary anew; result 1, a level more: a long
                                                **   task's soft question, the world's) */
        spent_at = RS_EXERT_PACE (*r);
        k = pc.fl;
        spent (RS_EXERT_TENTHS (*r));
        RS_EXERT_TIRED (*r) = pc.fl > k;
        if (pc.fl != k || pc.state != HARN_CONSCIOUS || conditions () != HUB->pcsum.cond) {
            status_now ();                      /* (Its steps in the reply, the rest after it; its fatigue as it was,
                                                **   awake, its conditions too (tiring), its summary as it was: none worked out again, a fight's
                                                **   round's some 40,000 cycles, fix2, 2026-11-24) */
            rf_later = 1;
        }
        break;
    case RS_EQUIP:
        if (RS_EQUIP_HOW (*r) == 2 && !r->actor) {
            equip_all (r->text);
            hheld = RS_EQUIP_HELD (*r);                      /* (The hands that hold something, any thing: slice 4.1b's) */
            set_load (RS_EQUIP_LOAD (*r));                   /* (And its load: its summary anew) */
            status ();
            break;
        }
        strncpy (dtext, r->text, sizeof dtext - 1);
        if (r->actor) {                         /* (An actor's things held and worn: the outfits' gear, gear.c's) */
            den_equip (k = (r->actor - 1) & 7);
            if (dwr[k]) {                       /* (A weapon in hand again, drawn or taken up: its fists no longer, its
                                                **   own let fall or broken left behind; dq0shield's bandit, its dagger
                                                **   broken and its sword drawn, struck with its fists till 2026-11-24) */
                sheet_get (aden[k]);
                bs.state &= ~BS_DISARMED;
                sheet_put (aden[k]);
            }
        } else {
            equip (dtext, RS_EQUIP_HOW (*r));                /* (A tool to hand, or not: skills.c's, the main RAM's room) */
        }
        break;

    case RS_SAVE:                               /* (The sheet into the save: hmchar's format, its own lines; and the
                                                **   creatures', beasts.dat) */
        save_pc (r);                            /* (The creatures', after it: the skeleton's) */
        break;
    case RS_DESCRIBE:
        if (RS_DESCRIBE_WHAT (*r) == 1) {
            brief ();
        } else if (RS_DESCRIBE_WHAT (*r) == 2) {
            put (hurt);
        } else if (RS_DESCRIBE_WHAT (*r) == 3) {
            pace_set (RS_DESCRIBE_BRIEF (*r));
        } else if (RS_DESCRIBE_WHAT (*r) == 4) {
            put (news);                         /* (What time did to it: heal.c's, told once) */
            news[0] = 0;
        } else if (RS_DESCRIBE_WHAT (*r) == RD_DEITY) {
            *tp = 0;                            /* (None worshipped: "") */
            if (pc.att[HARN_DTY] < HARN_NODEITY) {
                put (harn_deity_name (pc.att[HARN_DTY]));   /* (Its deity's name: part names's, harnname.c) */
            }
        } else {
            age_get ();                         /* (Its age: part dig_tool's, the life's record) */
            sheet ();
        }
        break;
    case RS_DICE:                               /* (A test's: its dice fixed) */
        dice_set (r);
        break;
    case RS_SCENE:                              /* (A test's scene: an actor's sheet set, part scene's: hscene.c) */
        scene_rs (r);
        break;
    case RS_OUTFIT:                             /* (Its outfit's makings: overlay 1's, outfit.c, in a copy) */
        memcpy (rq, r, sizeof rq);
        outfit_of ((struct req*) rq);
        memcpy (r, rq, sizeof rq);
        break;
    case RS_CONTEST:                            /* (Each one's test, the actor's first: who did better) */
        stress_lv = 1;                          /* (Two meeting: a foe near) */
        g = side (r->actor, RS_CONTEST_PURPOSE (*r), RS_CONTEST_STEPS (*r));
        k = side (r->other, RS_CONTEST_AGAINST (*r), RS_CONTEST_OSTEPS (*r));
        RS_CONTEST_GRADE (*r) = g;
        RS_CONTEST_OGRADE (*r) = k;
        RS_CONTEST_BETTER (*r) = g > (signed char) k ? 1 : g < (signed char) k ? 2 : 0;
        break;
    default:
        r->result = 0xFF;                       /* (Not yet) */
        *tp = 0;
        break;
    }
}

/* The overlays: dir/dq0harn.ovl, an 8K bank each (dq0harn.cfg's OV1 ...; the file cut after the last one used, each
** bank packed: make.js, ../dq0/ovlpack.js), read and unpacked in place into banks of its own by the unpack library,
** the linker's fill ($FF) put back after its bytes, all of it the game's library's (modules/dq0lib, ../dq0/dq0lib.inc:
** 2026-11-09); and a bank for the creatures' sheets (cleared): 0, or -1.  (And its arguments' numbers, its NUM) */
unsigned char __fastcall__ ovl_load (const char* dir);
static int __fastcall__ ovl_in (const char* dir)
{
    int n;

    if (ovl_load (dir) || (n = hy_banks_alloc (2)) < 0) {
        return -1;
    }
    sb = n;
    fb = n + 1;                                 /* (The bodies' bank after the sheets': hy_banks_alloc's run) */
    return 0;
}

/* Its start (the skeleton's rf_start: its arguments SEG ROLE DIR, its overlays read in, the creatures' banks had), and
** its beginning, its dice seeded (rf_begin: its own skill, the creatures' sheets cleared) */
unsigned char __fastcall__ rf_start (int argc, char* argv[])
{
    return argc < 4 || ovl_in (argv[3]);
}

void rf_begin (void)
{
    harn_own_skills (own, 1);                   /* (Its own skill: Cartography) */
    rf_sheets (2);
}
