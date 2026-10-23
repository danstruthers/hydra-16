/*
** gear.c - (overlay 3, the referee's: called through bank.s's stubs) the character's gear in a fight: what it strikes
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

#include <string.h>
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
    char            word[8];
    unsigned char   sk, wac, wdc;
    signed char     imp[3];
    unsigned char   part;
    unsigned char   hm;
} weapons[] = {
    { "hands", "fists", HARN_UNA, 0, 15, { 0, -1, -1 }, HARN_HAND },
    { "", "foot", HARN_UNA, 5, 5, { 1, -1, -1 }, HARN_FOOT },
    { "", "head", HARN_UNA, 0, 0, { 1, -1, -1 }, HARN_SKULL },
    { "knife", "dagger", HARN_DAG, 5, 5, { 1, 2, 5 }, 0xFF },
    { "blade", "sword", HARN_SWD, 15, 10, { 3, 5, 3 }, 0xFF },
    { "mace", "mace", HARN_CLU, 15, 5, { 6, -1, -1 }, 0xFF },
    { "spear", "spear", HARN_SPR, 20, 10, { 4, -1, 7 }, 0xFF, 10 },
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
};
#define NWEAPONS        (sizeof weapons / sizeof weapons[0])
#define NUNARMED        3
#define SHIELD          (NWEAPONS - 1)
#define TORCH           (SHIELD - 4)
#define STICK           (SHIELD - 3)
#define CLUB            (SHIELD - 2)
#define SOFT            NWEAPONS                /* (A soft thing's: none, rshouse.h's IM_SOFT) */

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
        p = weapons[j].hm + (j != SHIELD && h == hoff ? 10 : 0);
    }
    return p + STEP_PCT * (a > b ? a : b);
}

static int __fastcall__ eml (harn_char* c, unsigned char sk)
{
    return c->ml[sk] ? harn_eml (c, sk) : harn_get_oml (c, sk) - 5 * harn_get_pp (c);
}

/* Its limbs (rs.h's LB_*, slice 4.1b's), as its sheet has them (the character's, or a creature's): its main hand (the
** medical traits', Character 9: left-handed, ambidextrous; an arm missing or deformed, its off arm the one, no
** two-handed use), each limb made useless (a grievous wound, till it heals to M1: Combat 14); and each one's hurt, its
** injuries' levels (lhurt: the left arm's, the right's, the left leg's, the right's) */
unsigned char __fastcall__ o_limbs_of (const harn_char* h)
{
    unsigned char k, l = 0, j, v = 0;
    const harn_injury* p;

    memset (lhurt, 0, sizeof lhurt);
    for (k = 0; h == &pc && k < HARN_MEDMAX; ++k) {
        j = pc.med[k];
        l |= j == HARN_LEFTHANDED ? LB_LEFT : j == HARN_AMBIDEXTROUS ? LB_AMBI : 0;
        v |= j == HARN_MED_ARM;
    }
    if (h == &pc && v) {
        l |= l & LB_LEFT ? LB_ARM_R : LB_ARM_L;
    }
    for (k = 0; k < h->ninj; ++k) {
        p = &h->inj[k];
        if ((j = harn_is_limb (p->loc)) != 0) {
            j = (j - 1) * 2 + ((p->loc & HARN_RIGHT) != 0);
            v = lhurt[j] + p->il;
            lhurt[j] = v < p->il ? 255 : v;
            if (p->flags & HARN_IF_LIMB) {
                l |= LB_ARM_L << j;
            }
        }
    }
    return l;
}

/* Its conditions (rs.h's C_*), as its sheet has them (the character's, or a creature's: its body cplan's, BF_*):
** its state (senseless, in shock, dead), bleeders, limbs made useless and how they move it now (the book's, Combat 14:
** on two legs, one useless, it hops or walks with a staff, its arms enough it crawls, none: C_CRAWLS, else C_STUCK; on
** four, one useless, at half its move, C_LAME, two, C_CRAWLS, more, C_STUCK); tired, or tiring */
unsigned __fastcall__ o_cond_of (const harn_char* h)
{
    static const unsigned state[4] = { 0, C_UNCONSCIOUS, C_STUNNED, C_DEAD };
    unsigned c = state[h->state & 3] | (h->fl || (h == &pc && tired) ? C_FATIGUED : 0);   /* (Tired, or tiring) */
    unsigned char k, l = o_limbs_of (h), arms, legs, b = h == &pc ? BF_BIPED : cplan;

    for (k = 0; k < h->ninj; ++k) {
        if (h->inj[k].flags & HARN_IF_BLEEDER) {
            c |= C_BLEEDING;
        }
    }
    arms = !!(l & LB_ARM_L) + !!(l & LB_ARM_R);
    legs = !!(l & LB_LEG_L) + !!(l & LB_LEG_R);
    c |= (arms ? C_ARM_USELESS : 0) | (legs ? C_LEG_USELESS : 0);
    if (b & BF_BIPED) {
        c |= !legs ? 0 : legs == 1 || !arms ? C_CRAWLS : C_STUCK;
    } else if (b & BF_QUAD) {
        k = arms + legs;                        /* (Its forelegs the strike table's arms) */
        c |= !k ? 0 : k == 1 ? C_LAME : k == 2 ? C_CRAWLS : C_STUCK;
    }
    return c;
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

/* Its side of a fight: what it strikes with (pc_way's: SK_BEST, the best weapon it holds, else its fists; SK_THING,
** the thing named, its kind in wk, and no other, its words its name, a soft one its fists (the world never asks it);
** else that part of it), its block (the best of what it holds, its weapon or its shield; else a bare arm) and each
** thing that blocks (a round's defences, fight.c's: each blocks one foe a round), its dodge */
void __fastcall__ o_pc_side (struct side* s)
{
    const struct weapon* w;
    unsigned char k, j, best = 0, named = 0, l = o_limbs_of (&pc), dead, h;
    int v, att = 0, blk = 0;

    dead = (l & LB_ARM_L ? T_LEFT : 0) | (l & LB_ARM_R ? T_RIGHT : 0);
    hoff = l & LB_AMBI ? 0 : l & LB_LEFT ? T_RIGHT : T_LEFT;
    s->c = &pc;
    s->size = 4;
    s->b = 0;
    s->dodge = eml (&pc, HARN_DGE);
    s->nblk = 0;
    s->aslot = BARE_ARM;
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
        if (j != SHIELD && v + w->wac > att) {
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
