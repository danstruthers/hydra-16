/*
** fight.c - (overlay 2, the referee's: called through bank.s's stubs) fights, by HarnMaster's melee (Combat 7-14,
** 18): the creatures' sheets, a blow struck and its defence, morale, bleeding.  A request's read and answered in a
** copy of it in the main RAM (the hub's not in the window while the overlay is), its words put into its text.
**
** A creature's sheet comes from its kind's key (the denizens' kinds', kinds.txt's) in a stand-in bestiary of the
** game's own (its numbers are the game's, not the book's: a theme will bring its own): each attribute varied as the
** book suggests (less 7, and 2d6: at least 1, at most twice), by a die of its own (the game's seed and the record's
** number: the same creature each time, and no die of the referee's spent on it); its sheet kept in the referee's bank
** (dq0harn.c's sheet_get), its injuries, its state, its bloodloss, the time it's senseless till.
**
** A blow (RS_ATTACK): the attacker's weapon (the character's: the best it holds, by its attack mastery, its skill's
** EML and the weapon's attack class, Combat 3; its fists if it holds none; a creature's own, the first it has most
** often), the defender's defence (the character's by its stance, or the better of its block, with a weapon or a
** shield held, and its dodge; a creature dodges, or counterstrikes if it's the better at that, as the book advises
** for animals), each tested (the aim off the middle ten less, a prone defender twenty more to strike, the
** outnumbered defender ten less for each enemy more), the melee attack table's result: a miss, a block (a claw or a
** bite blocked by a weapon struck by it, a bare arm blocking a weapon struck: H2, Combat 12), a strike (the dice,
** the weapon's impact for its aspect, the sizes' difference a point a die, the book's bestiary's; the location by the
** aim; the armour there off it: the character's worn pieces, Combat 4-5, a poor piece a point less, a fine one a point
** more; a creature's hide), the wound (harn_wound: Combat 13-14), fumbles and stumbles (a fallen one's next turn
** spent getting up), the defender's tactical advantage.  One senseless is finished where it lies (the book's
** dispatch, a grope).  Bones that move (BF_MINDLESS) feel no shock, bleed nothing, fear nothing, and fall apart as
** their injuries reach their endurance (the game's).
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")

#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* ---- The bestiary: by the kinds' keys, each one's attributes (Strength, Stamina, Dexterity, Agility, Will), its
** weight (4 lb), its size (1 tiny ... 4 a man ... 6 very large), its Initiative (its courage: morale) and Dodge, its
** hide's protection (blunt, edge, point, fire), and its weapons (a word, its ML, its impact, its aspect, the part it
** strikes with: 0xFF a weapon held), the first the one it uses most */

#define BF_MINDLESS     0x01

struct nweapon {
    char            word[7];
    unsigned char   ml, imp, aspect, part;
};
static const struct bkind {
    char            key[8];
    unsigned char   att[5];
    unsigned char   wt, size, ini, dge;
    unsigned char   arm[4];
    unsigned char   flags;
    struct nweapon  w[2];
} bestiary[] = {
    { "rat", { 5, 8, 10, 14, 6 }, 3, 2, 30, 60, { 1, 1, 0, 1 }, 0,
      { { "bite", 45, 2, HARN_POINT, HARN_FACE }, { "" } } },
    { "spider", { 6, 9, 12, 15, 8 }, 4, 3, 40, 55, { 2, 3, 1, 0 }, 0,
      { { "bite", 50, 3, HARN_POINT, HARN_FACE }, { "" } } },
    { "ghoul", { 13, 14, 11, 11, 12 }, 35, 4, 70, 40, { 1, 1, 1, 0 }, 0,
      { { "claws", 55, 4, HARN_EDGE, HARN_HAND }, { "bite", 45, 4, HARN_POINT, HARN_FACE } } },
    { "skel", { 10, 10, 10, 10, 10 }, 10, 4, 90, 35, { 0, 4, 6, 2 }, BF_MINDLESS,
      { { "sword", 50, 4, HARN_EDGE, 0xFF }, { "" } } },
    { "bear", { 22, 18, 8, 10, 12 }, 150, 5, 60, 30, { 4, 3, 2, 3 }, 0,
      { { "claws", 65, 6, HARN_EDGE, HARN_HAND }, { "bite", 55, 7, HARN_POINT, HARN_FACE } } },
    { "kobold", { 8, 10, 12, 13, 9 }, 15, 3, 40, 55, { 2, 2, 1, 1 }, 0,
      { { "spear", 45, 5, HARN_POINT, 0xFF }, { "" } } },
};
#define NBEASTS         (sizeof bestiary / sizeof bestiary[0])

/* The character's weapons, by what it holds (the tools' keys): a word, its skill, its attack and defence classes, its
** blunt, edge and point impacts (-1: none; the book's weapons where it has them: a knife the dagger, a blade the
** broadsword, a torch a stick, Combat 3; the tools' the game's); its fists first, its shield last */
static const struct weapon {
    char            key[8];
    char            word[8];
    unsigned char   sk, wac, wdc;
    signed char     imp[3];
} weapons[] = {
    { "hands", "fists", HARN_UNA, 0, 15, { 0, -1, -1 } },
    { "knife", "dagger", HARN_DAG, 5, 5, { 1, 2, 5 } },
    { "blade", "sword", HARN_SWD, 15, 10, { 3, 5, 3 } },
    { "mace", "mace", HARN_CLU, 15, 5, { 6, -1, -1 } },
    { "spear", "spear", HARN_SPR, 20, 10, { 4, -1, 7 } },
    { "pick", "pick", HARN_AXE, 10, 5, { 2, -1, 5 } },
    { "mattock", "mattock", HARN_AXE, 15, 5, { 3, 4, 4 } },
    { "shovel", "shovel", HARN_SPR, 10, 10, { 2, 3, -1 } },
    { "spade", "spade", HARN_SPR, 10, 10, { 2, 3, -1 } },
    { "chisel", "chisel", HARN_DAG, 0, 0, { 1, -1, 2 } },
    { "torch", "torch", HARN_CLU, 5, 5, { 2, -1, -1 } },
    { "shield", "shield", HARN_SHD, 5, 20, { 2, -1, -1 } },
};
#define NWEAPONS        (sizeof weapons / sizeof weapons[0])
#define SHIELD          (NWEAPONS - 1)

/* The armour the character can wear, by the tools' keys: its name, its material, the locations it covers (the book's
** pieces, Combat 4) */
#define L(x)            (1u << HARN_ ## x)
#define TUNIC           (L (UPPERARM) | L (SHOULDER) | L (THORAX) | L (ABDOMEN) | L (HIP) | L (GROIN))
#define HAUBERK         (TUNIC | L (FOREARM) | L (ELBOW) | L (THIGH))
static const struct piece {
    char            key[8];
    char            name[14];
    unsigned char   mat;
    unsigned        cover;
} pieces[] = {
    { "lcap", "leather cap", HARN_LEATHER, L (SKULL) }, { "ltunic", "leather tunic", HARN_LEATHER, TUNIC },
    { "gambson", "gambeson", HARN_QUILT, HAUBERK }, { "ringbyr", "ring byrnie", HARN_RING, TUNIC },
    { "mailhbk", "mail hauberk", HARN_MAIL, HAUBERK }, { "gauntlt", "gauntlets", HARN_LEATHER, L (HAND) },
};
#define NARMOUR         (sizeof pieces / sizeof pieces[0])

/* An injury's words, by its aspect and severity (minor, serious, grievous or worse: sheet.c's) */
static const char* const wound_word[4][3] = {
    { "bruise", "fracture", "crush" }, { "cut", "gash", "deep gash" }, { "stab", "stab", "deep stab" },
    { "burn", "burn", "burn" }
};

/* ---- A creature's sheet, unpacked (into a HarnMaster character, for the book's functions) and packed again */

static harn_char cr;                            /* (The creature in the fight) */
static unsigned char crn;                       /*   and its sheet's number */
static unsigned long pc_out;                    /* (The character senseless till then: it can't defend) */
static unsigned char pc_prone;                  /* (The character fallen: its next turn spent getting up) */

static const struct bkind* __fastcall__ unpack (unsigned char who)
{
    sheet_get (crn = aden[(who - 1) & 7]);
    memset (&cr, 0, sizeof cr);
    cr.att[HARN_STR] = bs.att[0];
    cr.att[HARN_STA] = bs.att[1];
    cr.att[HARN_DEX] = bs.att[2];
    cr.att[HARN_AGL] = bs.att[3];
    cr.att[HARN_WIL] = bs.att[4];
    cr.att[HARN_WT] = bs.wt * 4;
    cr.state = bs.state & 3;
    if (cr.state == HARN_UNCONSCIOUS && tnow >= bs.out) {
        cr.state = HARN_CONSCIOUS;              /* (Come round) */
    }
    cr.bl = bs.bl;
    cr.ninj = bs.ninj;
    memcpy (cr.inj, bs.inj, sizeof bs.inj);
    return &bestiary[bs.kind && bs.kind <= NBEASTS ? bs.kind - 1 : 0];
}

static void pack (void)
{
    unsigned char k;

    for (k = 3; k < cr.ninj; ++k) {             /* (More than it keeps: the last takes their levels) */
        cr.inj[2].il += cr.inj[k].il;
        cr.inj[2].flags |= cr.inj[k].flags;
    }
    bs.state = (bs.state & B_PRONE) | cr.state;
    bs.bl = cr.bl;
    bs.ninj = cr.ninj > 3 ? 3 : cr.ninj;
    memcpy (bs.inj, cr.inj, sizeof bs.inj);
    sheet_put (crn);
}

/* ---- A side of a fight, as a blow wants it */

static struct side {
    harn_char*      c;
    unsigned char   who;                        /* 0 the character, else an actor's entry plus 1 */
    unsigned char   size;
    int             aml, dml, dodge;            /* Its attack's EML, its block's (with what it holds), its dodge's */
    signed char     imp[3];                     /* Its weapon's impacts */
    unsigned char   part;                       /* (A creature's own weapon: the part it strikes with; else 0xFF) */
    unsigned char   bare;                       /* (Its block a bare arm's) */
    const struct bkind* b;                      /* (A creature's entry; 0: the character) */
    char            word[8];                    /* Its weapon, in a word */
} A, D;

static struct req* R;                           /* The request (a copy) */
static unsigned char nfx;                       /* Its effects so far */
static char line[2][72];                        /* What the attacker's strike did, and the defender's */

static int __fastcall__ eml (harn_char* c, unsigned char sk)
{
    return c->ml[sk] ? harn_eml (c, sk) : harn_get_oml (c, sk) - 5 * harn_get_pp (c);
}

static void __fastcall__ pc_side (struct side* s)
{
    const struct weapon* w;
    unsigned char k, j, best = 0;
    int v, att = 0, blk = 0;

    s->c = &pc;
    s->size = 4;
    s->part = 0xFF;
    s->b = 0;
    s->dodge = eml (&pc, HARN_DGE);
    for (k = 0; k < NTOOLS; ++k) {              /* (What it holds that's a weapon: its best to strike with, its best
                                                **   to block with) */
        if (!tools[k][0] || !(tflag[k] & T_HELD)) {
            continue;
        }
        for (j = 1; j < NWEAPONS && strcmp (weapons[j].key, tools[k]); ++j) {
        }
        if (j == NWEAPONS) {
            continue;
        }
        w = &weapons[j];
        v = eml (&pc, w->sk);
        if (v + w->wdc > blk) {
            blk = v + w->wdc;
        }
        if (j != SHIELD && v + w->wac > att) {
            att = v + w->wac;
            best = j;
        }
    }
    w = &weapons[best];
    s->aml = best ? att : eml (&pc, HARN_UNA);
    s->bare = !blk;
    s->dml = blk ? blk : eml (&pc, HARN_UNA) + weapons[0].wdc;
    memcpy (s->imp, w->imp, sizeof s->imp);
    strcpy (s->word, w->word);
}

static void __fastcall__ beast_side (struct side* s, unsigned char who)
{
    const struct bkind* b = unpack (who);
    const struct nweapon* w = &b->w[b->w[1].word[0] && !random (3)];
    unsigned char pp = 5 * harn_get_pp (&cr);

    s->c = &cr;
    s->b = b;
    s->size = b->size;
    s->aml = w->ml - pp;
    s->dodge = b->dge - pp;
    s->dml = 0;
    s->bare = 0;
    s->imp[0] = s->imp[1] = s->imp[2] = -1;
    s->imp[w->aspect] = w->imp;
    s->part = w->part;
    strcpy (s->word, w->word);
}

/* ---- Armour */

static const char* arm_name;                    /* (The last piece that guards there, named) */

/* The character's at loc against aspect: its worn pieces' protection there, summed (a poor piece, quality 0-2, a
** point less, a fine one, 7-9, a point more: the game's) */
unsigned char __fastcall__ o_pc_armour (unsigned char loc, unsigned char aspect)
{
    const struct piece* p;
    unsigned bit = 1u << HARN_LOC (loc);
    unsigned char k, j, sum = 0;
    signed char v;

    arm_name = 0;
    for (k = 0; k < NTOOLS; ++k) {
        if (!tools[k][0] || !(tflag[k] & T_WORN)) {
            continue;
        }
        for (j = 0, p = pieces; j < NARMOUR; ++j, ++p) {
            if ((p->cover & bit) && !strcmp (p->key, tools[k])) {
                v = harn_protection[p->mat][aspect & 3] + (tqual[k] <= 2 ? -1 : tqual[k] >= 7);
                sum += v > 0 ? v : 0;
                arm_name = p->name;
            }
        }
    }
    return sum;
}

/* What guards side t at loc against aspect: the character's armour, a creature's hide */
static unsigned char __fastcall__ guard (const struct side* t, unsigned char loc, unsigned char aspect)
{
    if (t->b) {
        arm_name = t->b->arm[aspect & 3] ? "hide" : 0;
        return t->b->arm[aspect & 3];
    }
    return o_pc_armour (loc, aspect);
}

/* ---- A blow's effects and words */

static void __fastcall__ effect (unsigned char target, unsigned char kind, unsigned cond, unsigned char band,
    unsigned char loc, unsigned char aspect)
{
    struct rs_effect* e = (struct rs_effect*) R->x + nfx;

    if (nfx < RS_EFFECTS) {
        e->target = target;
        e->kind = kind;
        e->cond = cond;
        e->band = band;
        e->x[0] = loc;
        e->x[1] = aspect;
        ++nfx;
    }
}

/* A wound's words into w: "a serious cut to its left thigh (S2)" */
static void __fastcall__ wound_words (char* w, unsigned char sev, unsigned char loc, unsigned char aspect,
    unsigned char il, unsigned char its)
{
    char n[2];

    strcpy (w, "a ");
    strcat (w, harn_severity_name (sev));
    strcat (w, " ");
    strcat (w, wound_word[aspect & 3][sev > HARN_GRIEVOUS ? 2 : sev - 1]);
    strcat (w, its ? " to its " : " to your ");
    if (harn_is_limb (loc)) {
        strcat (w, loc & HARN_RIGHT ? "right " : "left ");
    }
    strcat (w, harn_location_name (loc));
    if (sev <= HARN_GRIEVOUS) {
        strcat (w, sev == HARN_MINOR ? " (M" : sev == HARN_SERIOUS ? " (S" : " (G");
        n[0] = '0' + il;
        n[1] = 0;
        strcat (w, n);
        strcat (w, ")");
    }
}

/* Side s fallen (a stumble), or got up */
static void __fastcall__ prone (const struct side* s, unsigned char on)
{
    if (!s->who) {
        pc_prone = on;
    } else {
        bs.state = on ? bs.state | B_PRONE : bs.state & ~B_PRONE;
    }
    if (on) {
        effect (s->who, X_COND_ON, C_PRONE, 0, 0, 0);
    }
}

static unsigned char __fastcall__ is_prone (const struct side* s)
{
    return s->who ? (bs.state & B_PRONE) != 0 : pc_prone;
}

/* Side t senseless (its shock roll failed): the rounds till it comes round (the book's shock rolls, a round each,
** within an hour), the tenths in the reply's w2; the character then (it comes round as the UI waits it out: it
** can't defend meanwhile), a creature as the clock passes it (it lies there till then) */
static void __fastcall__ senseless (const struct side* t)
{
    unsigned k;

    for (k = 1; k < 360 && harn_wake (t->c) == HARN_UNCONSCIOUS; ++k) {
    }
    R->w2 = k * 100;
    if (t->who) {
        t->c->state = HARN_UNCONSCIOUS;
        bs.out = tnow + R->w2;
    } else {
        if (pc.state == HARN_UNCONSCIOUS) {
            pc.state = HARN_CONSCIOUS;          /* (Within the hour, anyway) */
        }
        pc_out = tnow + R->w2;
    }
    effect (t->who, X_COND_ON, C_UNCONSCIOUS, 0, 0, 0);
}

/* Side s strikes side t with n dice at zone z (or part, not 0xFF: a block's strike on what struck or blocked): the
** impact, the armour there off it, the wound; its words into w (or the armour that stopped it: stop, AF_*, in the
** reply's c) */
static void __fastcall__ strike (struct side* s, struct side* t, unsigned char n, unsigned char z, unsigned char part,
    char* w, unsigned char stop)
{
    unsigned char aspect = s == &A ? R->b : AS_BEST, loc, f, p, sev, il, k;
    harn_char* c = t->c;
    harn_injury* j;
    int impact;

    if (aspect > HARN_POINT || s->imp[aspect] < 0) {
        for (aspect = HARN_BLUNT, k = 1; k < 3; ++k) {    /* (Its best) */
            aspect = s->imp[k] > s->imp[aspect] ? k : aspect;
        }
    }
    impact = dice (n, 6) + (int) n * ((int) s->size - (int) t->size) + s->imp[aspect];
    loc = part != 0xFF ? (harn_is_limb (part) && random (2) ? part | HARN_RIGHT : part) : harn_strike_location (z);
    p = guard (t, loc, aspect);
    if (impact - p < 1) {                       /* (Stopped by its armour, or no harm) */
        if (impact >= 1 && arm_name) {
            strcpy (w, t->who ? "its " : "your ");
            strcat (w, arm_name);
            R->c |= stop;
        }
        return;
    }
    if (!(f = harn_wound (c, loc, aspect, impact - p))) {
        return;
    }
    if (f & HARN_W_DEAD) {                      /* (A mortal wound's kill roll: dead, no injury kept) */
        sev = HARN_KILL;
        il = 0;
    } else {
        j = &c->inj[c->ninj - 1];
        sev = j->sev;
        il = j->il;
        if (t->b && (t->b->flags & BF_MINDLESS)) {  /* (Bones: no blood, no shock; apart at its endurance) */
            j->flags &= ~HARN_IF_BLEEDER;
            f &= ~HARN_W_DOWN;
            c->state = harn_get_ip (c) >= harn_end (c) ? HARN_DEAD : HARN_CONSCIOUS;
            f |= c->state == HARN_DEAD ? HARN_W_DEAD : 0;
        }
    }
    wound_words (w, sev, loc, aspect, il, t->who);
    if (f & HARN_W_DEAD) {
        effect (t->who, X_DEATH, C_DEAD, HARN_KILL, loc, aspect);
        return;
    }
    effect (t->who, X_WOUND, cond_of (c), sev, loc, aspect);
    if ((f & HARN_W_STUMBLE) && ((f & HARN_W_LIMB) || harn_stumble_roll (c, 0))) {
        prone (t, 1);                           /* (A leg's: fallen) */
    }
    if ((f & HARN_W_FUMBLE) && ((f & HARN_W_LIMB) || harn_fumble_roll (c, 0))) {
        effect (t->who, X_DROP, 0, 0, 0, 0);    /* (An arm's: what it held dropped) */
    }
    if (f & HARN_W_DOWN) {
        senseless (t);
    }
}

/* A fumble roll, or a stumble roll, for side s (which: its bit in the reply's a) */
static void __fastcall__ mishap (struct side* s, unsigned char stumble, unsigned char which)
{
    R->a |= which;
    if (stumble ? harn_stumble_roll (s->c, 0) : harn_fumble_roll (s->c, 0)) {
        if (stumble) {
            prone (s, 1);
        } else {
            effect (s->who, X_DROP, 0, 0, 0, 0);
        }
    }
}

/* The defender's defence (HARN_BLOCK ...): the character's by its stance (auto: the better of its block and its
** dodge), a creature's by its nature (a counterstrike if it's the better at it, else a dodge); in shock, or
** senseless, none */
static unsigned char defence (void)
{
    if (D.c->state != HARN_CONSCIOUS || (!D.who && pc_out > tnow)) {
        return HARN_IGNORE;
    }
    if (D.who) {
        return D.aml >= D.dodge ? HARN_COUNTER : HARN_DODGE;
    }
    switch (stance) {
    case D_BLOCK:
        return HARN_BLOCK;
    case D_DODGE:
        return HARN_DODGE;
    case D_COUNTER:
        return HARN_COUNTER;
    case D_NONE:
        return HARN_IGNORE;
    }
    return D.dml > D.dodge ? HARN_BLOCK : HARN_DODGE;
}

static void attack (void)
{
    unsigned char z = R->a == Z_HIGH ? HARN_HIGH : R->a == Z_LOW ? HARN_LOW : HARN_MID, def, al, dl, res, n, k, ab;
    int mod = 5 * (signed char) R->steps, dmod = -10 * (int) R->c;

    R->a = 0;
    R->c = 0;
    if (A.c->state != HARN_CONSCIOUS || (!A.who && pc_out > tnow)) {
        R->result = 0xFF;                       /* (It can't) */
        return;
    }
    if (is_prone (&A)) {                        /* (Fallen: it gets up, its turn spent) */
        prone (&A, 0);
        R->result = O_RISE;
        return;
    }
    if (D.who && D.c->state == HARN_UNCONSCIOUS) {
        D.c->state = HARN_DEAD;                 /* (Senseless: finished where it lies) */
        effect (D.who, X_DEATH, C_DEAD, HARN_KILL, 0, 0);
        R->result = O_DISPATCH;
        R->b = D_NONE;
        return;
    }
    def = defence ();
    if (z != HARN_MID && def != HARN_IGNORE) {
        mod -= 10;                              /* (Aimed high or low: Ignore's free) */
    }
    if (is_prone (&D)) {
        mod += 20;
    }
    al = harn_test (A.aml + mod);
    dl = def == HARN_IGNORE ? 0 : harn_test ((def == HARN_COUNTER ? D.aml : def == HARN_DODGE ? D.dodge : D.dml) +
        dmod);
    res = harn_melee_result (def, al, dl);
    n = HARN_R_DICE (res);
    k = HARN_R_KIND (res);
    switch (k) {
    case HARN_R_BLOCK:
        R->result = O_BLOCKED;
        if (D.bare && A.part == 0xFF) {         /* (A bare arm against a weapon: struck, H2) */
            strike (&A, &D, 2, 0, HARN_FOREARM, line[0], AF_STOPPED);
        } else if (A.part != 0xFF && !D.bare) { /* (A claw or a bite against a weapon: struck by it) */
            strike (&D, &A, 2, 0, A.part, line[1], AF_STOPPED_D);
        }
        break;
    case HARN_R_AH:
        R->result = O_HIT;
        strike (&A, &D, n, z, 0xFF, line[0], AF_STOPPED);
        break;
    case HARN_R_DH:
        R->result = O_COUNTER_HIT;
        strike (&D, &A, n, HARN_MID, 0xFF, line[1], AF_STOPPED_D);
        break;
    case HARN_R_BH:
        R->result = O_BOTH_HIT;
        strike (&A, &D, n, z, 0xFF, line[0], AF_STOPPED);
        strike (&D, &A, n, HARN_MID, 0xFF, line[1], AF_STOPPED_D);
        break;
    case HARN_R_AF:
    case HARN_R_DF:
    case HARN_R_BF:
    case HARN_R_AS:
    case HARN_R_DS:
    case HARN_R_BS:
        ab = k == HARN_R_AF || k == HARN_R_AS ? 1 : k == HARN_R_DF || k == HARN_R_DS ? 2 : 3;
        R->result = k >= HARN_R_AS ? O_STUMBLE : O_FUMBLE;
        if (ab & 1) {
            mishap (&A, k >= HARN_R_AS, 1);
        }
        if (ab & 2) {
            mishap (&D, k >= HARN_R_AS, 2);
        }
        break;
    case HARN_R_DTA:
        R->result = O_ADVANTAGE;
        R->c |= AF_TA;
        break;
    default:
        R->result = def == HARN_DODGE ? O_DODGED : O_MISS;
        break;
    }
    R->b = def == HARN_BLOCK ? D_BLOCK : def == HARN_COUNTER ? D_COUNTER : def == HARN_DODGE ? D_DODGE : D_NONE;
}

/* ---- The requests */

/* A creature's sheet (RS_NEW_ACTOR): made from the bestiary the first time (an unknown key: the first entry's, and
** result 1), kept after; its summary in the reply */
static unsigned hs;                             /* (Its own die) */

static unsigned char hd6 (void)
{
    hs = hs * 31421U + 6927U;
    return (unsigned char) ((hs >> 8) % 6 + 1);
}

static void new_beast (void)
{
    struct rs_summary* s = (struct rs_summary*) R->x;
    const struct bkind* b;
    unsigned char k = R->actor - 1, n = R->w1, j;
    int v, a;

    if (k >= NACTORS) {
        R->result = 1;
        return;
    }
    aden[k] = n;
    for (j = 0; j < NBEASTS && strcmp (bestiary[j].key, bkey); ++j) {
    }
    if (j == NBEASTS) {
        j = 0;
        R->result = 1;
    }
    b = &bestiary[j];
    sheet_get (n);
    if (bs.kind != j + 1) {
        memset (&bs, 0, sizeof bs);
        bs.kind = j + 1;
        hs = (unsigned) gseed ^ (unsigned) n * 40503U;
        for (k = 0; k < 5; ++k) {               /* (The book's: less 7, and 2d6; at least 1, at most twice) */
            a = b->att[k];
            v = a - 7 + hd6 () + hd6 ();
            bs.att[k] = v < 1 ? 1 : v > 2 * a ? 2 * a : v;
        }
        bs.wt = b->wt;
        sheet_put (n);
    }
    unpack (R->actor);
    memset (s, 0, sizeof *s);
    s->cond = cond_of (&cr);
    s->health = health_of (&cr);
    s->attacks = 1;
    s->morale = b->ini;
    s->power = 3 * b->size * b->size + b->w[0].ml / 10;    /* (The game's: by its size, its weapon's skill) */
}

/* Morale (RS_MORALE, Combat 18's Initiative test: the creature's courage, its injuries its penalty, the steps its
** situation): steady, cautious this turn (a marginal failure), or a critical failure's breakdown (berserk, desperate,
** broken, cautious: the book's table's odds) */
static void morale (void)
{
    const struct bkind* b;
    unsigned char lv;

    R->grade = G_MS;
    R->result = M_STEADY;
    if (!R->actor || ((b = unpack (R->actor))->flags & BF_MINDLESS)) {
        return;                                 /* (The character's the player's own; bones fear nothing) */
    }
    lv = harn_test (b->ini - 5 * harn_get_pp (&cr) + 5 * (signed char) R->steps);
    R->grade = lv == HARN_CF ? G_CF : lv == HARN_MF ? G_MF : lv == HARN_MS ? G_MS : G_CS;
    if (lv == HARN_MF) {
        R->result = M_CAUTIOUS;
    } else if (lv == HARN_CF) {
        lv = d100 ();
        R->result = lv <= 25 ? M_BERSERK : lv <= 50 ? M_DESPERATE : lv <= 75 ? M_BROKEN : M_CAUTIOUS;
    }
}

/* Time passed (RS_TIME, w1 minutes): its bleeders' bloodloss, a point a minute each, death past its endurance
** (Combat 14: harn.h's harn_bleed, here, for its file brings all the healing with it) */
static void bleed (void)
{
    harn_char* c = &pc;
    unsigned char k, n = 0;
    unsigned v;

    if (R->actor) {
        unpack (R->actor);
        c = &cr;
    }
    for (k = 0; k < c->ninj; ++k) {
        n += (c->inj[k].flags & HARN_IF_BLEEDER) != 0;
    }
    v = c->bl + n * R->w1;
    c->bl = v > 255 ? 255 : v;
    if (c->bl > harn_end (c)) {
        c->state = HARN_DEAD;
    }
    if ((R->result = c->state == HARN_DEAD) != 0) {
        effect (R->actor, X_DEATH, C_DEAD, 0, 0, 0);
    }
    R->b = health_of (c);
    if (R->actor) {
        pack ();
    }
}

/* r a copy of the request, in the main RAM (but its text: its words put into it) */
void __fastcall__ o_fight (struct req* r)
{
    R = r;
    nfx = 0;
    switch (r->kind) {
    case RS_NEW_ACTOR:
        new_beast ();
        return;
    case RS_MORALE:
        morale ();
        return;
    case RS_TIME:
        memset (r->x, 0, sizeof r->x);
        bleed ();
        return;
    }
    memset (r->x, 0, sizeof r->x);              /* (RS_ATTACK) */
    line[0][0] = line[1][0] = 0;
    if (r->actor && r->other) {
        r->result = 0xFF;                       /* (Creatures fighting each other: later, with allies) */
        return;
    }
    if (r->actor) {
        beast_side (&A, r->actor);
        pc_side (&D);
    } else {
        pc_side (&A);
        beast_side (&D, r->other);
    }
    A.who = r->actor;
    D.who = r->other;
    attack ();
    put (A.word);
    put ("\n");
    put (D.word);
    put ("\n");
    put (line[0]);
    put ("\n");
    put (line[1]);
    r->w1 = health_of (A.c) | health_of (D.c) << 8;
    pack ();
}
