/*
** gear.c - (overlay 3, the referee's: called through bank.s's stubs) the character's gear in a fight: what it strikes
** with and blocks with (what it holds: RS_EQUIP's marks, the tools' keys; its own hands, feet and head, the Unarmed
** skill's, as it's asked: the hub's pc_strike), and the armour it wears.
**
** Its weapons are the book's where it has them (Combat 3: a knife the dagger, a blade the broadsword, a torch a
** stick; a hand, a foot, a head the book's unarmed rows, by Unarmed), the tools' the game's; the best it holds by its
** attack mastery (its skill's EML and the weapon's attack class), its fists if it holds none.  A punch is the better
** for gauntlets, a kick for hobnailed boots (a point of impact: the book's "enhanced", Skills 18; the game's
** numbers).  At a thing (at_thing), asked for its best, it takes out the tool to hand that hits it hardest (not a
** weapon, which a thing would spoil), else its fists.  Armour (Combat 4-5) by its pieces: each one's material's protection where it covers, a poor piece
** (quality 0-2) a point less, a fine one (7-9) a point more; footwear's soles their own (what a kick at a thing does
** to the foot: the game's).
*/

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

#include <string.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* The weapons, by the tools' keys: a word, its skill, its attack and defence classes, its blunt, edge and point
** impacts (-1: none), the part it is (0xFF: a thing held; TOOL: a tool, the one that hits a thing hardest taken out
** for it: a weapon a thing would spoil); the unarmed first (SK_PUNCH, SK_KICK, SK_HEAD's: the hand,
** the foot, the head), the shield last */
#define TOOL            0xFE
static const struct weapon {
    char            key[8];
    char            word[8];
    unsigned char   sk, wac, wdc;
    signed char     imp[3];
    unsigned char   part;
} weapons[] = {
    { "hands", "fists", HARN_UNA, 0, 15, { 0, -1, -1 }, HARN_HAND },
    { "", "foot", HARN_UNA, 5, 5, { 1, -1, -1 }, HARN_FOOT },
    { "", "head", HARN_UNA, 0, 0, { 1, -1, -1 }, HARN_SKULL },
    { "knife", "dagger", HARN_DAG, 5, 5, { 1, 2, 5 }, 0xFF },
    { "blade", "sword", HARN_SWD, 15, 10, { 3, 5, 3 }, 0xFF },
    { "mace", "mace", HARN_CLU, 15, 5, { 6, -1, -1 }, 0xFF },
    { "spear", "spear", HARN_SPR, 20, 10, { 4, -1, 7 }, 0xFF },
    { "pick", "pick", HARN_AXE, 10, 5, { 2, -1, 5 }, TOOL },
    { "mattock", "mattock", HARN_AXE, 15, 5, { 3, 4, 4 }, TOOL },
    { "shovel", "shovel", HARN_SPR, 10, 10, { 2, 3, -1 }, TOOL },
    { "spade", "spade", HARN_SPR, 10, 10, { 2, 3, -1 }, TOOL },
    { "chisel", "chisel", HARN_DAG, 0, 0, { 1, -1, 2 }, TOOL },
    { "torch", "torch", HARN_CLU, 5, 5, { 2, -1, -1 }, 0xFF },
    { "shield", "shield", HARN_SHD, 5, 20, { 2, -1, -1 }, 0xFF },
};
#define NWEAPONS        (sizeof weapons / sizeof weapons[0])
#define NUNARMED        3
#define SHIELD          (NWEAPONS - 1)

/* The armour, by the tools' keys: its name, its material, the locations it covers (the book's pieces, Combat 4), a
** footwear's sole (against what a kick strikes), a punch's or a kick's impact more */
#define L(x)            (1u << HARN_ ## x)
#define TUNIC           (L (UPPERARM) | L (SHOULDER) | L (THORAX) | L (ABDOMEN) | L (HIP) | L (GROIN))
#define HAUBERK         (TUNIC | L (FOREARM) | L (ELBOW) | L (THIGH))
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
};
#define NARMOUR         (sizeof pieces / sizeof pieces[0])

char arm[16];
unsigned char pc_way, at_thing;

static int __fastcall__ eml (harn_char* c, unsigned char sk)
{
    return c->ml[sk] ? harn_eml (c, sk) : harn_get_oml (c, sk) - 5 * harn_get_pp (c);
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

/* Its side of a fight: what it strikes with (pc_way's: SK_BEST, the best weapon it holds, else its fists; else that
** part of it), its block (the best of what it holds, its weapon or its shield; else a bare arm) and each thing that
** blocks (a round's defences, fight.c's: each blocks one foe a round), its dodge */
void __fastcall__ o_pc_side (struct side* s)
{
    const struct weapon* w;
    unsigned char k, j, best = 0;
    int v, att = 0, blk = 0;

    s->c = &pc;
    s->size = 4;
    s->b = 0;
    s->dodge = eml (&pc, HARN_DGE);
    s->nblk = 0;
    s->aslot = BARE_ARM;
    for (k = 0; k < NTOOLS; ++k) {              /* (What it holds that's a weapon: its best to strike with, its best
                                                **   to block with) */
        if (!tools[k][0] || !(tflag[k] & T_HELD)) {
            continue;
        }
        for (j = NUNARMED; j < NWEAPONS && strcmp (weapons[j].key, tools[k]); ++j) {
        }
        if (j == NWEAPONS) {
            continue;
        }
        w = &weapons[j];
        v = eml (&pc, w->sk);
        if (v + w->wdc > blk) {
            blk = v + w->wdc;
        }
        if (s->nblk < SIDE_BLK) {
            s->bslot[s->nblk] = k;
            s->bml[s->nblk++] = v + w->wdc;
        }
        if (j != SHIELD && v + w->wac > att) {
            att = v + w->wac;
            best = j;
            s->aslot = k;
        }
    }
    if (at_thing && pc_way == SK_BEST) {        /* (At a thing: the tool to hand that hits it hardest) */
        for (best = 0, att = 0, k = 0; k < NTOOLS; ++k) {
            for (j = NUNARMED; tools[k][0] && j < NWEAPONS; ++j) {
                w = &weapons[j];
                if (w->part == TOOL && !strcmp (w->key, tools[k]) && w->imp[0] + w->imp[1] + w->imp[2] > att) {
                    att = w->imp[0] + w->imp[1] + w->imp[2];
                    best = j;
                }
            }
        }
        att = best ? eml (&pc, weapons[best].sk) + weapons[best].wac : 0;
    }
    if (pc_way != SK_BEST || !best) {           /* (A part of it: by Unarmed) */
        s->aslot = BARE_ARM;
        best = pc_way == SK_KICK ? 1 : pc_way == SK_HEAD ? 2 : 0;
        att = eml (&pc, HARN_UNA) + weapons[best].wac;
    }
    w = &weapons[best];
    s->aml = att;
    s->part = w->part == TOOL ? 0xFF : w->part;
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
    strcpy (s->word, w->word);
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
    return uhow[s];
}
