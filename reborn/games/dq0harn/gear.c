/*
** gear.c - (overlay 3, the referee's: called through bank.s's stubs) the character's gear in a fight (and since
** 2026-10-27 the denizens', their things told: the outfits' gear, below): what it strikes
** with and blocks with (what it holds: RS_EQUIP's marks, the tools' keys; its own hands, feet and head, the Unarmed
** skill's, as it's asked: the hub's pc_strike), and the armour it wears.
**
** Its weapons are the book's where it has them (Combat 3: a knife the dagger, a blade the broadsword, a torch a
** stick; a hand, a foot, a head the book's unarmed rows, by Unarmed), the tools' the game's; the best it holds by its
** attack mastery (its skill's EML and the weapon's attack class), its fists if it holds none.  A punch is the better
** for gauntlets, a kick for hobnailed boots (a point of impact: the book's "enhanced", Skills 18; the game's
** numbers).  A thing named (the hub's pc_strike, SK_THING: the user's, 2026-10-22, "A bar should just be treated as a
** club or stick, no?") strikes and no other: a weapon of the table's as itself (by its kind's key), anything else as
** the book treats a thing swung that isn't a melee weapon (Combat 8: a bow in melee a stick, a staff or a club), by its
** kind's data (rshouse.h's, the house's): a light thing the book's stick, a hefty one its club; nothing named, what it
** holds (RS_WIELD's: the UI's, and the world's as it fights back), its hands empty the best it carries asked about
** (pc_wield's).  Armour (Combat 4-5) by its pieces: each one's material's protection where it covers, a poor piece
** (quality 0-2) a point less, a fine one (7-9) a point more; footwear's soles their own (what a kick at a thing does
** to the foot: the game's).  Each thing it holds the worse for its hand (slice 4.1b's: RS_EQUIP's marks, the hub's
** pc_shand for the thing that strikes): in one hand, the book's one-handed penalty (Combat 3's HM: a spear's, a staff's
** 10); in the off hand a weapon's special penalty, 10, but a shield's, none if it's ambidextrous (Combat 3); its arm's
** hurt, the house's (each level of the injuries to that arm a step, in both hands the worse arm's); a useless arm's
** thing none of its (the world's to put down).
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

#include <stddef.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* The weapons, by the tools' keys: a word, its skill, its attack and defence classes, its blunt, edge and point
** impacts (-1: none), the part it is (0xFF: a thing held; TOOL: a tool, the one that hits a thing hardest offered for
** it, its hands empty: a weapon a thing would spoil), its penalty one-handed (the book's HM, a two-handed thing's
** always in both: the things'); the unarmed first (SK_PUNCH, SK_KICK, SK_HEAD's: the hand, the
** foot, the head); the book's stick, club and staff (Combat 3: their numbers, a point's optional aspect left out) what
** a thing swung is treated as (a theme's own kinds by these keys too); the shield last */
#define TOOL            0xFE
static const struct weapon {
    char            key[8];
    char            word[14];                   /* (A catalogue's name: the world finds a weapon let fall by it) */
    unsigned char   sk, wac, wdc;
    signed char     imp[3];
    unsigned char   part;
    unsigned char   hm;
} weapons[] = {
    { "hands", "fists", HARN_UNA, 0, 15, { 0, -1, -1 }, HARN_HAND },
    { "", "foot", HARN_UNA, 5, 5, { 1, -1, -1 }, HARN_FOOT },
    { "", "head", HARN_UNA, 0, 0, { 1, -1, -1 }, HARN_SKULL },
    { "dagger", "dagger", HARN_DAG, 5, 5, { 1, 2, 5 }, 0xFF },
    { "knife", "knife", HARN_DAG, 5, 0, { 0, 1, 4 }, 0xFF },
    { "taburi", "taburi", HARN_DAG, 5, 0, { 0, -1, 4 }, 0xFF },
    { "blade", "sword", HARN_SWD, 15, 10, { 3, 5, 3 }, 0xFF },
    { "shortsw", "shortsword", HARN_SWD, 10, 5, { 2, 4, 4 }, 0xFF },
    { "falchin", "falchion", HARN_SWD, 15, 5, { 4, 6, 1 }, 0xFF },
    { "longknf", "longknife", HARN_SWD, 10, 15, { 1, 3, 5 }, 0xFF },
    { "mankar", "mankar", HARN_SWD, 10, 5, { 2, 5, 0 }, 0xFF },     /* (The Gargun's, Combat 3's (G): 2026-10-29) */
    { "mang", "mang", HARN_SWD, 15, 10, { 3, 6, 0 }, 0xFF, 5 },
    { "bastard", "bastard sword", HARN_SWD, 20, 10, { 4, 7, 4 }, 0xFF, 10 },
    { "hatchet", "hatchet", HARN_AXE, 5, 5, { 3, 4, -1 }, 0xFF },
    { "sickle", "sickle", HARN_AXE, 5, 5, { 1, 4, 3 }, 0xFF },
    { "shorkan", "shorkana", HARN_AXE, 5, 5, { 3, 5, -1 }, 0xFF },
    { "battlax", "battleaxe", HARN_AXE, 20, 10, { 6, 9, -1 }, 0xFF, 15 },
    { "warhamm", "warhammer", HARN_AXE, 15, 5, { 6, -1, 5 }, 0xFF, 5 },
    { "poleaxe", "poleaxe", HARN_POL, 25, 5, { 6, 9, 6 }, 0xFF },
    { "net", "net", HARN_NET, 20, 0, { 2, -1, -1 }, 0xFF },
    { "whip", "whip", HARN_WHP, 25, 5, { 2, 1, -1 }, 0xFF },
    { "mace", "mace", HARN_CLU, 15, 5, { 6, -1, -1 }, 0xFF },
    { "spear", "spear", HARN_SPR, 20, 10, { 4, -1, 7 }, 0xFF, 10 },
    { "axe", "axe", HARN_AXE, 10, 5, { 4, 6, -1 }, 0xFF },  /* (The book's handaxe: a creature's let fall, slice
                                                **   4.1b's) */
    { "pick", "pick", HARN_AXE, 10, 5, { 2, -1, 5 }, TOOL },
    { "mattock", "mattock", HARN_AXE, 15, 5, { 3, 4, 4 }, TOOL },
    { "shovel", "shovel", HARN_SPR, 10, 10, { 2, 3, -1 }, TOOL },
    { "spade", "spade", HARN_SPR, 10, 10, { 2, 3, -1 }, TOOL },
    { "chisel", "chisel", HARN_DAG, 0, 0, { 1, -1, 2 }, TOOL },
    { "torch", "torch", HARN_CLU, 5, 5, { 2, -1, -1 }, 0xFF },
    { "stick", "stick", HARN_CLU, 5, 5, { 2, -1, -1 }, 0xFF },
    { "club", "club", HARN_CLU, 15, 5, { 4, -1, -1 }, 0xFF },
    { "staff", "staff", HARN_SPR, 20, 15, { 4, -1, -1 }, 0xFF, 10 },
    { "shield", "shield", HARN_SHD, 5, 20, { 2, -1, -1 }, 0xFF },
    { "buckler", "buckler", HARN_SHD, 5, 15, { 1, -1, -1 }, 0xFF },     /* (The shields last: Combat 3) */
    { "kshield", "knight shield", HARN_SHD, 5, 20, { 2, -1, -1 }, 0xFF },
    { "kite", "kite shield", HARN_SHD, 5, 25, { 3, -1, -1 }, 0xFF },
    { "tower", "tower shield", HARN_SHD, 5, 25, { 3, -1, -1 }, 0xFF },
};
#define NWEAPONS        (sizeof weapons / sizeof weapons[0])
#define NUNARMED        3
#define NSHIELDS        5
#define SHIELD          (NWEAPONS - NSHIELDS)   /* (The first shield) */
#define IS_SHIELD(j)    ((j) >= SHIELD && (j) < NWEAPONS)
#define TORCH           (SHIELD - 4)
#define STICK           (SHIELD - 3)
#define CLUB            (SHIELD - 2)
#define SOFT            NWEAPONS                /* (A soft thing's: none, rshouse.h's IM_SOFT) */

/* The armour, by the tools' keys: its name, its material, the locations it covers (the book's pieces, Combat 4), a
** footwear's sole (against what a kick strikes), a punch's or a kick's impact more */
#define L(x)            (1u << HARN_ ## x)
#define TUNIC           (L (UPPERARM) | L (SHOULDER) | L (THORAX) | L (ABDOMEN) | L (HIP) | L (GROIN))
#define HAUBERK         (TUNIC | L (FOREARM) | L (ELBOW) | L (THIGH))
#define LEGS            (L (HIP) | L (GROIN) | L (THIGH) | L (KNEE) | L (CALF))
#define ROBE            (TUNIC | L (FOREARM) | L (ELBOW) | L (THIGH) | L (KNEE) | L (CALF))
#define VEST            (L (SHOULDER) | L (THORAX) | L (ABDOMEN))
static const struct piece {
    char            key[8];
    char            name[16];
    unsigned char   mat;
    unsigned        cover;
    unsigned char   sole, more;
} pieces[] = {
    { "lcap", "leather cap", HARN_LEATHER, L (SKULL) }, { "helm", "iron helm", HARN_PLATE, L (SKULL) },
    { "ltunic", "leather tunic", HARN_LEATHER, TUNIC }, { "gambson", "gambeson", HARN_QUILT, HAUBERK },
    { "ringbyr", "ring byrnie", HARN_RING, TUNIC }, { "mailhbk", "mail hauberk", HARN_MAIL, HAUBERK },
    { "gauntlt", "gauntlets", HARN_LEATHER, L (HAND), 0, 1 },
    { "sandals", "sandals", HARN_CLOTH, 0, 1 }, { "shoes", "shoes", HARN_LEATHER, L (FOOT), 3 },
    { "boots", "knee boots", HARN_LEATHER, L (KNEE) | L (CALF) | L (FOOT), 4 },
    { "hobnail", "hobnail boots", HARN_LEATHER, L (KNEE) | L (CALF) | L (FOOT), 6, 1 },
    { "clothes", "clothes", HARN_CLOTH, TUNIC | LEGS },     /* (2026-10-27, the outfits': a tunic and leggings) */
    { "robe", "robe", HARN_CLOTH, ROBE }, { "surcoat", "surcoat", HARN_CLOTH, VEST | L (HIP) | L (GROIN) | L (THIGH) },
    { "lvest", "leather vest", HARN_LEATHER, VEST }, { "khelm", "kurbul helm", HARN_KURBUL, L (SKULL) },
    { "mcowl", "mail cowl", HARN_MAIL, L (SKULL) | L (NECK) }, { "mbyrnie", "mail byrnie", HARN_MAIL, TUNIC },
    { "sbyrnie", "scale byrnie", HARN_SCALE, TUNIC }, { "ghelm", "great helm", HARN_PLATE, L (SKULL) | L (FACE) | L (NECK) },
    { "mlegs", "mail leggings", HARN_MAIL, LEGS }, { "hide", "hide jerkin", HARN_QUILT, TUNIC },   /* (Hide and fur:
                                                **   quilt's, Combat 4's fur, the house's) */
    { "shroud", "shroud", HARN_CLOTH, ROBE },
};
#define NARMOUR         (sizeof pieces / sizeof pieces[0])

char arm[16];
unsigned char pc_way, pc_hand;

/* What the thing in hand (or hands) h (T_LEFT, T_RIGHT: RS_EQUIP's marks) is the worse for, weapon j (the house's and
** the book's, above): in one hand, its one-handed penalty, and the off hand's (hoff: its mark, 0 if it's ambidextrous)
** for a weapon but a shield; its arm's hurt, a step a level of its injuries (lhurt's, limbs_of's: in both, the worse) */
static unsigned char hoff;
static int __fastcall__ hand_pen (unsigned char h, unsigned char j)
{
    unsigned char a = h & T_LEFT ? lhurt[0] : 0, b = h & T_RIGHT ? lhurt[1] : 0;
    int p = 0;

    if (h && h != (T_LEFT | T_RIGHT)) {
        p = weapons[j].hm + (!IS_SHIELD (j) && h == hoff ? 10 : 0);
    }
    return p + STEP_PCT * (a > b ? a : b);
}

static int __fastcall__ eml (harn_char* c, unsigned char sk)
{
    return c->ml[sk] ? harn_eml (c, sk) : harn_get_oml (c, sk) - 5 * harn_get_pp (c);
}

/* What a thing swung does, by its kind's data (rshouse.h's: the house's, every ruleset's) */
#define RS_HOUSE_SWING  swing
#include "../dq0/rshouse.h"

/* The weapon a thing is (its kind, wk): the table's by its key; else what the book treats a thing swung as, by the
** house's (a light one the stick, a hefty one the club); a soft one SOFT */
static unsigned char row_of (void)
{
    unsigned char j;

    for (j = NUNARMED; wk.tool[0] && j < NWEAPONS; ++j) {
        if (!strcmp (weapons[j].key, wk.tool)) {
            return j;
        }
    }
    j = swing (&wk);
    return j == IM_SOFT ? SOFT : j == IM_LIGHT ? STICK : CLUB;
}

/* What weapon j does for the blow (at a thing: the impact it does; else the attack mastery it strikes with); a soft
** thing's the least */
static int __fastcall__ worth (unsigned char j, unsigned char at)
{
    const struct weapon* w = &weapons[j];

    return j == SOFT ? -1000 : at ? w->imp[0] + w->imp[1] + w->imp[2] : eml (&pc, w->sk) + w->wac;
}

/* Its worn pieces: the sum of what fn gives for each (wp the piece, wq its quality) */
static const struct piece* wp;
static unsigned char wq;
static unsigned bit;                            /* (The location asked about, a bit) */
static unsigned char asp;                       /*   (and the aspect) */

static unsigned char __fastcall__ worn (unsigned char (*fn) (void))
{
    unsigned char k, j, sum = 0;

    for (k = 0; k < NTOOLS; ++k) {
        if (!tools[k][0] || !(tflag[k] & T_WORN)) {
            continue;
        }
        for (j = 0, wp = pieces; j < NARMOUR; ++j, ++wp) {
            if (!strcmp (wp->key, tools[k])) {
                wq = tqual[k];
                sum += fn ();
            }
        }
    }
    return sum;
}

static unsigned char guards (void)              /* (A piece's protection at bit against asp, named) */
{
    signed char v;

    if (!(wp->cover & bit)) {
        return 0;
    }
    v = harn_protection[wp->mat][asp & 3] + (wq <= 2 ? -1 : wq >= 7);
    strcpy (arm, wp->name);
    return v > 0 ? v : 0;
}

static unsigned char more (void)                /* (A piece's impact more, on the part at bit that strikes) */
{
    return wp->cover & bit ? wp->more : 0;
}

static unsigned char sole (void)                /* (Its sole's, named) */
{
    if (wp->sole) {
        strcpy (arm, wp->name);
    }
    return wp->sole;
}

/* The armour at loc against aspect (SOLE: the soles, against what a kick strikes) */
unsigned char __fastcall__ o_pc_armour (unsigned char loc, unsigned char aspect)
{
    arm[0] = 0;
    if (loc == SOLE) {
        return worn (sole);
    }
    bit = 1u << HARN_LOC (loc);
    asp = aspect;
    return worn (guards);
}

/* ---- The denizens' things (the outfits' gear, 2026-10-27: DENIZENS.md's gear; the world's RS_EQUIP for an actor):
** what each actor holds and wears, as the world tells it (its weapon's row, its shield's, its pieces), its blow by the
** weapon it holds (the table's, the character's: its kind's own weapon skill the bestiary's mastery, another weapon's
** the worse or better by their attack classes, another skill's ten less, the house's), its armour by its pieces (a poor
** one a point less, a fine one a point more, as the character's); nothing held, its fists (ten less, as a weapon let
** fall); never told (a beast, dq0 -K's), the bestiary's, as before */
unsigned char dwr[NACTORS];                     /* (Each actor's weapon held: its row, 0 none, 0xFF untold; */
unsigned char dsh[NACTORS];                     /*   its shield held: its row, 0 none; */
unsigned char dpc[NACTORS][DPIECES];            /*   its pieces worn: each its entry plus 1, | DP_POOR, DP_FINE) */
char dtext[DTEXT_N];                            /* (Its marks, RS_EQUIP's text, copied: dq0harn.c's) */
#define DP_POOR         0x20
#define DP_FINE         0x40

/* RS_EQUIP for actor k (its marks in dtext: < the left hand, > the right, * both, + worn, each key's quality after it) */
void __fastcall__ o_den_equip (unsigned char k)
{
    const char* t = dtext;
    char key[8];
    unsigned char m, j, n = 0, q;

    dwr[k] = dsh[k] = 0;
    memset (dpc[k], 0, DPIECES);
    while (*t) {
        m = *t++;
        for (j = 0; *t && (*t < '0' || *t > '9'); ++t) {
            if (j < sizeof key - 1) {
                key[j++] = *t;
            }
        }
        key[j] = 0;
        q = *t ? *t++ - '0' : 0;
        if (m == '+') {
            for (j = 0; j < NARMOUR && strcmp (pieces[j].key, key); ++j) {
            }
            if (j < NARMOUR && n < DPIECES) {
                dpc[k][n++] = (j + 1) | (q <= 2 ? DP_POOR : q >= 7 ? DP_FINE : 0);
            }
            continue;
        }
        for (j = NUNARMED; j < NWEAPONS && strcmp (weapons[j].key, key); ++j) {
        }
        if (j < NWEAPONS && IS_SHIELD (j)) {
            dsh[k] = j;
        } else if (j < NWEAPONS && (m != '<' || !dwr[k])) {
            dwr[k] = j;
        }
    }
}

/* Actor k's blow, its things told (fight.c's beast_side: s's aml its kind's own weapon's, s's word its word): the
** weapon it holds (none, or let fall: its fists, ten less); and the shield it holds, if it does (its row in s's dslot,
** its word dshw's, its impacts dshi's: what a fist that meets it takes): its block's EML in s's dml, its kind's own
** weapon's mastery (the bestiary's one figure for its fighting: the house's, 2026-10-29) and the shield's defence class
** (Combat 3's; a shield none the worse in the off hand) */
char dshw[14];
signed char dshi[3];
void __fastcall__ o_den_side (struct side* s, unsigned char k)
{
    const struct weapon* w;
    unsigned char j, o;

    if ((s->dslot = j = dsh[k]) != 0) {
        s->dml = s->aml + weapons[j].wdc;
        strcpy (dshw, weapons[j].word);
        memcpy (dshi, weapons[j].imp, sizeof dshi);
    }
    for (o = NUNARMED; o < NWEAPONS && strcmp (weapons[o].word, s->word); ++o) {
    }
    j = bs.state & BS_DISARMED ? 0 : dwr[k];
    w = &weapons[j];
    if (!j) {
        s->aml -= 10;
    } else if (o < NWEAPONS) {
        s->aml += w->wac - weapons[o].wac - (w->sk != weapons[o].sk ? 10 : 0);
    }
    memcpy (s->imp, w->imp, sizeof s->imp);
    s->part = j ? 0xFF : w->part;
    strcpy (s->word, w->word);
}

/* Actor k's armour at loc against aspect, its pieces' (the last that guards named in arm: "" none) */
unsigned char __fastcall__ o_den_armour (unsigned char k, unsigned char loc, unsigned char aspect)
{
    unsigned char j, c, sum = 0;

    arm[0] = 0;
    bit = 1u << HARN_LOC (loc);
    asp = aspect;
    for (j = 0; j < DPIECES && (c = dpc[k][j]) != 0; ++j) {
        wp = &pieces[(c & 0x1F) - 1];
        wq = c & DP_POOR ? 0 : c & DP_FINE ? 9 : 5;
        sum += guards ();
    }
    return sum;
}

/* Its side of a fight: what it strikes with (pc_way's: SK_BEST, the best weapon it holds, else its fists; SK_THING,
** the thing named, its kind in wk, and no other, its words its name, a soft one its fists (the world never asks it);
** else that part of it), its block (the best of what it holds, its weapon or its shield; else a bare arm) and each
** thing that blocks (a round's defences, fight.c's: each blocks one foe a round), its dodge */
void __fastcall__ o_pc_side (struct side* s)
{
    const struct weapon* w;
    unsigned char k, j, best = 0, named = 0, l = limbs_of (&pc), dead, h;
    int v, att = 0, blk = 0;

    dead = (l & LB_ARM_L ? T_LEFT : 0) | (l & LB_ARM_R ? T_RIGHT : 0);
    hoff = l & LB_AMBI ? 0 : l & LB_LEFT ? T_RIGHT : T_LEFT;
    s->c = &pc;
    s->size = 4;
    s->b = 0;
    s->dodge = eml (&pc, HARN_DGE);
    s->nblk = 0;
    s->aslot = BARE_ARM;
    s->dslot = 0xFF;
    for (k = 0; k < NTOOLS; ++k) {              /* (What it holds that's a weapon: its best to strike with, its best
                                                **   to block with, each the worse for its hand; a useless arm's none) */
        if (!tools[k][0] || !(tflag[k] & T_HELD) || ((h = tflag[k] & (T_LEFT | T_RIGHT)) & dead)) {
            continue;
        }
        for (j = NUNARMED; j < NWEAPONS && strcmp (weapons[j].key, tools[k]); ++j) {
        }
        if (j == NWEAPONS) {
            continue;
        }
        w = &weapons[j];
        v = eml (&pc, w->sk) - hand_pen (h, j);
        if (v + w->wdc > blk) {
            blk = v + w->wdc;
        }
        if (s->nblk < SIDE_BLK) {
            s->bslot[s->nblk] = k;
            s->bml[s->nblk++] = v + w->wdc;
        }
        if (!IS_SHIELD (j) && v + w->wac > att) {
            att = v + w->wac;
            best = j;
            s->aslot = k;
        }
    }
    if ((pc_way & SK_THING) && (j = row_of ()) != SOFT) {     /* (The thing named: as itself, or as the book treats
                                                **   it; its slot, if it's one the table knows, its counterstrike's) */
        named = 1;
        best = j;
        att = eml (&pc, weapons[j].sk) + weapons[j].wac - hand_pen ((pc_hand & IH_LEFT ? T_LEFT : 0) |
            (pc_hand & IH_RIGHT ? T_RIGHT : 0), j);    /* (In the hand that holds it, the hub's pc_shand) */
        for (s->aslot = BARE_ARM, k = 0; k < NTOOLS; ++k) {
            if (wk.tool[0] && !strcmp (tools[k], wk.tool)) {
                s->aslot = k;
            }
        }
    } else if (pc_way != SK_BEST || !best) {    /* (A part of it: by Unarmed) */
        s->aslot = BARE_ARM;
        best = pc_way == SK_KICK ? 1 : pc_way == SK_HEAD ? 2 : 0;
        att = eml (&pc, HARN_UNA) + weapons[best].wac;
    }
    w = &weapons[best];
    s->aml = att;
    s->part = w->part == TOOL || named ? 0xFF : w->part;
    s->bare = !blk;
    s->dml = blk ? blk : eml (&pc, HARN_UNA) + weapons[0].wdc;
    if (!blk) {                                 /* (Nothing held that blocks: a bare arm) */
        s->bslot[0] = BARE_ARM;
        s->bml[0] = s->dml;
        s->nblk = 1;
    }
    memcpy (s->imp, w->imp, sizeof s->imp);
    if (s->part != 0xFF) {                      /* (Gauntlets on a punch, hobnailed boots on a kick) */
        bit = 1u << w->part;
        s->imp[0] += worn (more);
    }
    strcpy (s->word, named ? wk.name : w->word);
}

/* RS_WIELD (rs.h's): what the character would strike with, nothing named (the user's, 2026-10-22: "I don't think it
** should auto-fallback to something better unless you have nothing at hand, and then it should ask for
** confirmation"), at a denizen (q's b 0) or a thing (b 1): of the kinds it holds (x[0] its main hand's, x[1] its
** other's), its main hand's, but a soft thing (the other's then, if it isn't: slice 4.1b's), result 1 or 2; its hands
** empty, result 0, and the key of the best it carries for
** such a blow, if it does better than its bare hands (at a denizen a weapon, but its shield or a torch, by its attack
** mastery; at a thing the tool that hits it hardest, not a weapon a thing would spoil), put in the reply's words */
void __fastcall__ o_pc_wield (struct req* q)
{
    unsigned char k, j, at = q->b, soft = 2;
    int v, top;
    const char* key = 0;

    q->result = 0;
    for (k = 0; k < 2; ++k) {
        if (q->x[k]) {
            kind_in (q->x[k]);
            if ((j = row_of () == SOFT) < soft) {
                soft = j;
                q->result = k + 1;
            }
        }
    }
    if (q->result) {
        return;
    }
    top = worth (0, at);                        /* (Its bare hands': a fist's) */
    for (k = 0; k < NTOOLS; ++k) {
        for (j = NUNARMED; tools[k][0] && j < SHIELD; ++j) {
            if (!strcmp (weapons[j].key, tools[k]) && (at ? weapons[j].part == TOOL : j != TORCH) && (v = worth (j, at)) >
                top) {                          /* (A torch carried for its light, never offered) */
                top = v;
                key = tools[k];
            }
        }
    }
    put (key ? key : "");                       /* (Its words: "" none, the reply's text cleared) */
}

/* The hands that let go of what side s fumbled (the melee table's fumble: rs.h's DH_*, the referee's X_DROP's; 0 none):
** the character's attack's thing (its slot's hand; a thing named, the hub's pc_shand's) or its block's (att 0); a
** creature's weapon, held, in its right; a part of it (a fist, a bite) none */
unsigned char __fastcall__ o_fumbled (const struct side* s, unsigned char att)
{
    unsigned char k;

    if (s->b) {
        return s->part == 0xFF ? DH_RIGHT : 0;
    }
    k = att ? s->aslot : s->dslot;
    if (k < NTOOLS) {
        k = tflag[k];
        return (k & T_LEFT ? DH_LEFT : 0) | (k & T_RIGHT ? DH_RIGHT : 0);
    }
    return att && (pc_way & SK_THING) ? pc_hand & IH_BOTH : 0;
}

/* The hand a wound at loc (an arm's) lets go of what it holds (side t's: DH_*; 0 none): the character's on that side
** (the world's to drop what's there); a creature's weapon, held in its right, a wound to its right arm */
unsigned char __fastcall__ o_wound_drops (const struct side* t, unsigned char loc)
{
    unsigned char h = loc & HARN_RIGHT ? DH_RIGHT : DH_LEFT;

    return !t->b ? h : t->part == 0xFF && h == DH_RIGHT ? h : 0;
}

/* The character's defences in a round (the user's, 2026-10-20; HOUSE.md's): each thing that blocks blocks one foe a
** round (a shield one, a sword another), a bare arm's too, the dodge one; a counterstrike only on command (its stance,
** D_COUNTER), what it strikes with then spent for the round as a block would be; a foe met this round meets the
** same again.  To whom each went (the foe's who, 0 none yet), by the slot in tools of what blocked (BARE_ARM a bare
** arm's), the dodge last; and how (HARN_*).  The round from the first blow at it after the last one ended */
#define DODGED          (BARE_ARM + 1)
#define ROUND_T         100                     /* (A round's tenths: RS_HELLO's) */
static unsigned char used[DODGED + 1], uhow[DODGED + 1];
static unsigned long rnd_at;

/* The character's defence (its side d) against foe f (its who) this round, armed if f strikes with a weapon: its
** stance's, D_AUTO the better of a block and the dodge left (a bare arm's block against a weapon the last: struck,
** H2), D_BLOCK or D_DODGE that first; none left, none.  A block's EML in d->dml, d->bare if it's a bare arm's */
unsigned char __fastcall__ o_pc_defence (struct side* d, unsigned char f, unsigned char armed)
{
    unsigned char k, s, how = HARN_IGNORE;
    int v = -0x7FFF, w;

    if (tnow >= rnd_at + ROUND_T) {            /* (A round on: its defences all free again) */
        rnd_at = tnow;
        memset (used, 0, sizeof used);
    }
    if (stance == D_NONE) {
        return HARN_IGNORE;
    }
    for (s = 0; s <= DODGED && used[s] != f; ++s) {
    }
    if (s > DODGED) {                           /* (A foe not met yet this round) */
        if (stance == D_COUNTER && !used[d->aslot]) {
            s = d->aslot;
            how = HARN_COUNTER;
        } else {
            for (s = 0xFF, k = 0; k < d->nblk; ++k) {
                w = d->bml[k] - (d->bslot[k] == BARE_ARM && armed ? 1000 : 0);
                if (!used[d->bslot[k]] && w > v) {
                    v = w;
                    s = d->bslot[k];
                    how = HARN_BLOCK;
                }
            }
            if (!used[DODGED] && (s == 0xFF || stance == D_DODGE || (stance != D_BLOCK && d->dodge > v))) {
                s = DODGED;
                how = HARN_DODGE;
            }
            if (s == 0xFF) {
                return HARN_IGNORE;             /* (Every defence spent this round) */
            }
        }
        used[s] = f;
        uhow[s] = how;
    }
    for (k = 0; k < d->nblk && d->bslot[k] != s; ++k) {
    }
    if (k < d->nblk) {
        d->dml = d->bml[k];
        d->bare = s == BARE_ARM;
    }
    d->dslot = uhow[s] == HARN_DODGE ? 0xFF : s;   /* (What it defends with: its fumble's, slice 4.1b's) */
    if (uhow[s] == HARN_BLOCK && s < NTOOLS) {  /* (A block's words what blocked: "you fend it off with your
                                                **   buckler", not the weapon it strikes with, 2026-10-29) */
        for (k = NUNARMED; k < NWEAPONS && strcmp (weapons[k].key, tools[s]); ++k) {
        }
        if (k < NWEAPONS) {
            strcpy (d->word, weapons[k].word);
        }
    }
    return uhow[s];
}

/* The best block the shields the character holds give (side s's, its blocks: a missile's, Combat 15: at Shield ML,
** high or low velocity alike), its word in s's: 0 none */
int __fastcall__ o_pc_shield (struct side* s)
{
    unsigned char k, j;
    int v = 0;

    for (k = 0; k < s->nblk; ++k) {
        if (s->bslot[k] < NTOOLS) {
            for (j = SHIELD; j < NWEAPONS && strcmp (weapons[j].key, tools[s->bslot[k]]); ++j) {
            }
            if (j < NWEAPONS && s->bml[k] > v) {
                v = s->bml[k];
                strcpy (s->word, weapons[j].word);
            }
        }
    }
    return v;
}

/* A blow at a thing (RS_ATTACK at RS_THING: rs.h's), which ignores it: the character's attack tested (its weapon's
** attack mastery, or Unarmed's), the Ignore table's strike (Combat 12: a critical failure none), its impact (the
** dice, the weapon's for the aspect that does the thing most harm), the thing's resistance off it; and a part that
** struck it hurt if the blow went wrong (rs.h's: the book's H2 on a bare part meeting something hard, the game's
** numbers), HC_STRUCK's harm (sheet.c's).  A part's blow the house's Unarmed test (HOUSE.md's battering, the user's,
** 2026-10-22): a marginal failure harms neither, a critical failure the part alone; a bare part's hurt stopped short
** (RS_SHORT: none, result | 4).  (Fight.c's, its request q, the attacker's side s: here, overlay 2's room) */
void __fastcall__ o_hit_thing (struct req* q, struct side* s)
{
    static unsigned char hq[offsetof (struct req, text)];   /* (The harm's request: but its text) */
#define h   (*(struct req*) hq)
    unsigned char res[4], k, lv, n, asp = 0, run = q->c, a, pull = q->w2 & RS_SHORT;
    int imp, v, top = -100;

    memcpy (res, q->x, sizeof res);
    memset (q->x, 0, sizeof q->x);
    o_pc_side (s);
    for (k = 0; k < 3; ++k) {
        if (s->imp[k] >= 0 && (v = s->imp[k] - res[k]) > top) {
            top = v;
            asp = k;
        }
    }
    lv = harn_test (s->aml);
    n = s->part != 0xFF && lv <= HARN_MF ? 0 : HARN_R_DICE (harn_melee_result (HARN_IGNORE, lv, 0));
    imp = n ? dice (n, 6) + s->imp[asp] : 0;
    q->w1 = imp > res[asp] ? imp - res[asp] : 0;
    q->b = asp;
    q->c = 0;
    q->w2 = 0;
    q->result = s->part != 0xFF ? 2 : 0;
    put (s->word);
    if (s->part != 0xFF && lv == HARN_CF) {     /* (Gone wrong: the part hurt; a bare one stopped short of it, asked) */
        a = s->part == HARN_FOOT ? o_pc_armour (SOLE, 0) : o_pc_armour (s->part, HARN_BLUNT);
        v = dice (2, 6) + res[0] / 2 - a;
        if (v > 0 && pull && !a) {
            q->result |= 4;
        } else if (v > 0) {
            memset (hq, 0, sizeof hq);
            h.kind = RS_HARM;
            h.a = HC_STRUCK;
            h.c = s->part;
            h.w1 = v;
            h.w2 = run;
            harm (&h);
            q->c = h.b;
            q->w2 = h.w2;
            q->result |= h.result;
        }
    }
#undef h
}
