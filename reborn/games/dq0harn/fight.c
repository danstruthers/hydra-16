/*
** fight.c - (overlay 2, the referee's: called through bank.s's stubs) fights, by HarnMaster's melee (Combat 7-14,
** 18): the creatures' sheets, a blow struck and its defence, morale, bleeding, two actors' tests (the engage
** initiative test's: a creature's by its sheet).  A request's read and answered in a
** copy of it in the main RAM (the hub's not in the window while the overlay is), its words put into its text.
**
** A creature's sheet comes from its kind's key (the denizens' kinds', kinds.txt's) in the referee's bestiary: the
** first six a stand-in of the game's own (their numbers the game's); the rest the HarnWorld Bestiary's averages for
** the like creature (each entry's page given), or for folk HarnMaster's for men (about 10 to 12, their skills by
** their trade).  Each attribute varied as the book suggests (less 7, and 2d6: at least 1, at most twice), by a die
** of its own (the game's seed and the record's number: the same creature each time, and no die of the referee's
** spent on it); its sheet kept in the referee's bank (dq0harn.c's sheet_get), its injuries, its state, its
** bloodloss, the time it's senseless till.
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
** spent getting up: the world's bodies' positions, the request's), the defender's tactical advantage.  One senseless
** is finished where it lies (the book's dispatch, a grope).  Bones that move (BF_MINDLESS) feel no shock, bleed
** nothing, fear nothing, and fall apart as their injuries reach their endurance (the game's).
*/

#pragma code-name ("OV2CODE")
#pragma rodata-name ("OV2RODATA")

#include <stddef.h>
#include <stdlib.h>
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
    { "rat", { 5, 8, 10, 14, 6 }, 3, 2, 30, 50, { 1, 1, 0, 1 }, 0,
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
    { "bat", { 3, 8, 10, 10, 8 }, 1, 1, 45, 50, { 1, 1, 0, 1 }, 0,                  /* (Bats 1, p10) */
      { { "bite", 45, 1, HARN_POINT, HARN_FACE }, { "" } } },
    { "adder", { 4, 10, 12, 12, 8 }, 1, 2, 45, 60, { 1, 1, 0, 1 }, 0,               /* (Snakes 3, p88) */
      { { "bite", 55, 3, HARN_POINT, HARN_FACE }, { "" } } },
    { "basilsk", { 16, 13, 14, 14, 10 }, 28, 5, 55, 70, { 6, 6, 4, 4 }, 0,         /* (Dragons 13, p36) */
      { { "bite", 65, 5, HARN_POINT, HARN_FACE }, { "" } } },
    { "lindwrm", { 22, 18, 4, 18, 16 }, 200, 6, 85, 90, { 5, 9, 4, 5 }, 0,         /* (Dragons 14, p37) */
      { { "claws", 85, 7, HARN_EDGE, HARN_HAND }, { "bite", 68, 7, HARN_POINT, HARN_FACE } } },
    { "wight", { 12, 12, 6, 10, 1 }, 40, 4, 60, 35, { 1, 1, 1, 0 }, BF_MINDLESS,    /* (Morvrin 5, p78: a man's, */
      { { "claws", 55, 3, HARN_POINT, HARN_HAND }, { "" } } },                      /*   its will gone) */
    { "shade", { 11, 12, 11, 11, 15 }, 38, 4, 70, 50, { 2, 2, 1, 0 }, 0,            /* (Morvrin 3, p76: a priest's) */
      { { "sword", 60, 5, HARN_EDGE, 0xFF }, { "" } } },
    { "koblord", { 11, 12, 12, 13, 12 }, 20, 3, 60, 55, { 3, 3, 2, 1 }, 0,
      { { "axe", 55, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "bandit", { 11, 11, 11, 11, 11 }, 40, 4, 50, 45, { 2, 2, 1, 1 }, 0,
      { { "sword", 50, 5, HARN_EDGE, 0xFF }, { "" } } },
    { "banlord", { 14, 13, 12, 12, 13 }, 45, 4, 65, 55, { 4, 5, 3, 2 }, 0,
      { { "sword", 65, 6, HARN_EDGE, 0xFF }, { "" } } },
    { "cultist", { 10, 10, 11, 10, 12 }, 38, 4, 55, 40, { 1, 1, 0, 0 }, 0,
      { { "dagger", 45, 3, HARN_POINT, 0xFF }, { "" } } },
    { "priest", { 10, 11, 12, 11, 16 }, 38, 4, 70, 45, { 2, 2, 1, 1 }, 0,
      { { "mace", 55, 5, HARN_BLUNT, 0xFF }, { "" } } },
};
#define NBEASTS         (sizeof bestiary / sizeof bestiary[0])

/* An injury's words, by its aspect and severity (minor, serious, grievous or worse: sheet.c's) */
static const char* const wound_word[4][3] = {
    { "bruise", "fracture", "crush" }, { "cut", "gash", "deep gash" }, { "stab", "stab", "deep stab" },
    { "burn", "burn", "burn" }
};

/* ---- A creature's sheet, unpacked (into a HarnMaster character, for the book's functions) and packed again */

static harn_char cr;                            /* (The creature in the fight) */
static unsigned char crn;                       /*   and its sheet's number */
unsigned long pc_out;                           /* (The character senseless till then: it can't defend; dq0harn.h's) */
static unsigned char prones;                    /* (The request's: the attacker prone, RS_PRONE_A; the defender,
                                                **   RS_PRONE_D: their bodies' positions, the world's) */

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
    bs.state = cr.state;
    bs.bl = cr.bl;
    bs.ninj = cr.ninj > 3 ? 3 : cr.ninj;
    memcpy (bs.inj, cr.inj, sizeof bs.inj);
    sheet_put (crn);
}

static struct side A, D;
static struct req* R;                           /* The request (a copy) */
static unsigned char nfx;                       /* Its effects so far */
static char line[2][72];                        /* What the attacker's strike did, and the defender's */

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

/* What guards side t at loc against aspect: the character's armour, a creature's hide */
static unsigned char __fastcall__ guard (const struct side* t, unsigned char loc, unsigned char aspect)
{
    if (t->b) {
        strcpy (arm, t->b->arm[aspect & 3] ? "hide" : "");
        return t->b->arm[aspect & 3];
    }
    return pc_armour (loc, aspect);
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
    char n[4];

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
        strcat (w, utoa (il, n, 10));           /* (Ten or more two digits) */
        strcat (w, ")");
    }
}

/* Side s fallen (a stumble: an effect, the world's to lay it down), or got up (O_RISE's: the world's to stand it up) */
static void __fastcall__ prone (const struct side* s, unsigned char on)
{
    if (on) {
        effect (s->who, X_COND_ON, C_PRONE, 0, 0, 0);
    }
}

static unsigned char __fastcall__ is_prone (const struct side* s)
{
    return (prones & (s == &A ? RS_PRONE_A : RS_PRONE_D)) != 0;
}

/* Side t senseless (its shock roll failed): the rounds till it comes round (the book's shock rolls, a round each,
** within an hour), the tenths in the reply's w2; the character then (it comes round as the UI waits it out: it
** can't defend meanwhile), a creature as the clock passes it (it lies there till then).  In the main RAM (the
** overlay's full): strike's alone, its overlay in */
#pragma code-name (push, "CODE")
static void __fastcall__ senseless (const struct side* t)
{
    unsigned long* o = t->who ? &bs.out : &pc_out;
    unsigned k;

    for (k = 1; k < 360 && harn_wake (t->c) == HARN_UNCONSCIOUS; ++k) {
    }
    R->w2 = k * 100;
    if (*o > tnow) {                            /* (Out already, struck again as it lies: out till the later, its
                                                **   black-out told once: AF_AGAIN) */
        R->c |= AF_AGAIN;
        if (*o - tnow > R->w2) {
            R->w2 = (unsigned) (*o - tnow);
        }
    }
    *o = tnow + R->w2;
    if (t->who) {
        t->c->state = HARN_UNCONSCIOUS;
    } else if (pc.state == HARN_UNCONSCIOUS) {
        pc.state = HARN_CONSCIOUS;              /* (Within the hour, anyway) */
    }
    effect (t->who, X_COND_ON, C_UNCONSCIOUS, 0, 0, 0);
}
#pragma code-name (pop)

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
        if (impact >= 1 && arm[0]) {
            strcpy (w, t->who ? "its " : "your ");
            strcat (w, arm);
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

/* The defender's defence (HARN_BLOCK ...): the character's, its round's (gear.c's pc_defence); a creature's by its
** nature (a counterstrike if it's the better at it, else a dodge); in shock, or senseless, none */
static unsigned char defence (void)
{
    if (D.c->state != HARN_CONSCIOUS || (!D.who && pc_out > tnow)) {
        return HARN_IGNORE;
    }
    if (D.who) {
        return D.aml >= D.dodge ? HARN_COUNTER : HARN_DODGE;
    }
    return pc_defence (&D, A.who, A.part == 0xFF);
}

static void attack (void)
{
    unsigned char z = R->a == Z_HIGH ? HARN_HIGH : R->a == Z_LOW ? HARN_LOW : HARN_MID, def, al, dl, res, n, k, ab;
    int mod = STEP_PCT * (signed char) R->steps, dmod = -2 * STEP_PCT * (int) R->c;   /* (Outnumbered: two steps
                                                **   worse each one more) */

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
        mod -= 2 * STEP_PCT;                    /* (Aimed high or low, two steps: Ignore's free) */
    }
    if (is_prone (&D)) {
        mod += 4 * STEP_PCT;                    /* (Prone, four steps the easier) */
    }
    al = harn_test (A.aml + mod);
    dl = def == HARN_IGNORE ? 0 : harn_test ((def == HARN_COUNTER ? D.aml : def == HARN_DODGE ? D.dodge : D.dml) +
        dmod);
    res = harn_melee_result (def, al, dl);
    n = HARN_R_DICE (res);
    k = HARN_R_KIND (res);
    if (k == HARN_R_DTA && (D.c->state != HARN_CONSCIOUS || (!D.who && pc_out > tnow))) {
        k = HARN_R_MISS;                        /* (Senseless, or in shock: no advantage it can take, a miss) */
    }
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
        R->result = def == HARN_DODGE && al >= HARN_MS ? O_DODGED : O_MISS;    /* (Dodged: a blow that was good) */
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

/* A creature's courage tested (Combat 18's Initiative test: its injuries its penalty, steps its situation): steady,
** cautious this turn (a marginal failure), or a critical failure's breakdown (berserk, desperate, broken, cautious:
** the book's table's odds); its grade in the reply.  The creature's sheet unpacked (cr), b its entry */
static unsigned char __fastcall__ courage (const struct bkind* b, signed char steps)
{
    unsigned char lv;

    R->grade = G_MS;
    if (b->flags & BF_MINDLESS) {
        return M_STEADY;                        /* (Bones fear nothing) */
    }
    lv = harn_test (b->ini - 5 * harn_get_pp (&cr) + STEP_PCT * steps);
    R->grade = lv == HARN_CF ? G_CF : lv == HARN_MF ? G_MF : lv == HARN_MS ? G_MS : G_CS;
    if (lv == HARN_MF) {
        return M_CAUTIOUS;
    }
    if (lv != HARN_CF) {
        return M_STEADY;
    }
    lv = d100 ();
    return lv <= 25 ? M_BERSERK : lv <= 50 ? M_DESPERATE : lv <= 75 ? M_BROKEN : M_CAUTIOUS;
}

/* Morale (RS_MORALE): an actor's courage, the result its state (the character's the player's own: steady) */
static void morale (void)
{
    R->result = M_STEADY;
    R->grade = G_MS;
    if (R->actor) {
        R->result = courage (unpack (R->actor), (signed char) R->steps);
    }
}

/* Time passed (RS_TIME, w1 minutes): the bleeders' bloodloss, a point a minute each, death past the endurance (Combat
** 14: harn.h's harn_bleed, here, for its file brings all the healing with it): of actor (0 the character), or, actor
** 0xFF, of each actor in w2's bits (bit k: entry k), w1 back its bits of those dead of it */
static unsigned char __fastcall__ bled (harn_char* c)
{
    unsigned char k, n = 0;
    unsigned v;

    for (k = 0; k < c->ninj; ++k) {
        n += (c->inj[k].flags & HARN_IF_BLEEDER) != 0;
    }
    v = c->bl + n * R->w1;
    c->bl = v > 255 ? 255 : v;
    if (n && c->bl > harn_end (c)) {
        c->state = HARN_DEAD;
    }
    return c->state == HARN_DEAD;
}

static void bleed (void)
{
    unsigned char k, dead = 0;

    if (R->actor != 0xFF) {
        R->result = R->actor ? (unpack (R->actor), bled (&cr)) : bled (&pc);
        R->b = health_of (R->actor ? &cr : &pc);
        if (R->actor) {
            pack ();
        }
        return;
    }
    for (k = 0; k < NACTORS; ++k) {
        if (R->w2 & 1 << k) {
            unpack (k + 1);
            dead |= bled (&cr) << k;
            pack ();
        }
    }
    R->w1 = dead;
}

/* A creature's side of two actors' tests (RS_CONTEST's, dq0harn.c's: the actor's test of purpose a, at the steps): by
** its sheet, Initiative its bestiary's, anything else its Dexterity times 5 (a grope's), its injuries' penalty off it
** (one senseless, or in shock, tests nothing: a critical failure); the grade in the reply */
static void tested (void)
{
    const struct bkind* b = unpack (R->actor);

    R->grade = cr.state != HARN_CONSCIOUS ? G_CF : grade (harn_test ((R->a == P_INITIATIVE ? b->ini :
        5 * cr.att[HARN_DEX]) - 5 * harn_get_pp (&cr) + STEP_PCT * (signed char) R->steps));
}

/* What a blow left them in, for the world (rs.h's AF_*: the reply's grade as hub.h's FF_*: it keeps it), and the
** creature's courage if it was wounded and stands (shy, w2's bit 0 in the request: a cautious one flees too) */
static void __fastcall__ left (unsigned char shy)
{
    const struct rs_effect* e = (const struct rs_effect*) R->x;
    const struct side* s = A.who ? &A : &D;
    unsigned char k, f = R->c & (AF_STOPPED | AF_STOPPED_D) ? FF_STOPPED : 0, hurt = 0, m;

    for (k = 0; k < RS_EFFECTS && e->kind; ++k, ++e) {
        if (e->kind == X_DEATH) {
            f |= e->target == A.who ? FF_A_DEAD : FF_D_DEAD;
        } else if (e->kind == X_COND_ON) {
            f |= e->cond & C_UNCONSCIOUS ? (e->target == A.who ? FF_A_DOWN : FF_D_DOWN) : FF_PRONE;
        } else if (e->kind == X_DROP) {
            f |= FF_FUMBLE;
        }
        hurt |= e->kind == X_WOUND && e->target == s->who;
    }
    if (hurt && cr.state == HARN_CONSCIOUS) {
        m = courage (s->b, 0);
        if (m == M_BROKEN || (m == M_CAUTIOUS && shy)) {
            f |= FF_FLEES;
        } else if (m == M_CAUTIOUS) {
            R->c |= AF_CAUTIOUS;
        }
    }
    if (cond_of (&cr) & C_BLEEDING) {
        R->c |= AF_BLEEDING;
    }
    R->grade = f;
}

/* A blow at a thing (RS_ATTACK at RS_THING: rs.h's), which ignores it: the character's attack tested (its weapon's
** attack mastery, or Unarmed's), the Ignore table's strike (Combat 12: a critical failure none), its impact (the
** dice, the weapon's for the aspect that does the thing most harm), the thing's resistance off it; and a part that
** struck it hurt if the blow went wrong (rs.h's: the book's H2 on a bare part meeting something hard, the game's
** numbers), HC_STRUCK's harm (sheet.c's) */
static void thing (void)
{
    static unsigned char hq[offsetof (struct req, text)];   /* (The harm's request: but its text) */
#define h   (*(struct req*) hq)
    unsigned char res[4], k, lv, n, asp = 0, run = R->c;
    int imp, v, top = -100;

    memcpy (res, R->x, sizeof res);
    memset (R->x, 0, sizeof R->x);
    at_thing = 1;
    pc_side (&A);
    at_thing = 0;
    for (k = 0; k < 3; ++k) {
        if (A.imp[k] >= 0 && (v = A.imp[k] - res[k]) > top) {
            top = v;
            asp = k;
        }
    }
    lv = harn_test (A.aml);
    n = HARN_R_DICE (harn_melee_result (HARN_IGNORE, lv, 0));
    imp = n ? dice (n, 6) + A.imp[asp] : 0;
    R->w1 = imp > res[asp] ? imp - res[asp] : 0;
    R->b = asp;
    R->c = 0;
    R->w2 = 0;
    R->result = A.part != 0xFF ? 2 : 0;
    put (A.word);
    if (A.part != 0xFF && lv <= HARN_MF) {      /* (Gone wrong: the part hurt) */
        v = dice (lv == HARN_CF ? 2 : 1, 6) + res[0] / 2 -
            (A.part == HARN_FOOT ? pc_armour (SOLE, 0) : pc_armour (A.part, HARN_BLUNT));
        if (v > 0) {
            memset (hq, 0, sizeof hq);
            h.kind = RS_HARM;
            h.a = HC_STRUCK;
            h.c = A.part;
            h.w1 = v;
            h.w2 = run;
            harm (&h);
            R->c = h.b;
            R->w2 = h.w2;
            R->result |= h.result;
        }
    }
#undef h
}

/* r a copy of the request, in the main RAM (but its text: its words put into it) */
void __fastcall__ o_fight (struct req* r)
{
    unsigned char shy = r->w2 & 1;

    prones = r->w2;
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
        bleed ();
        return;
    case RS_CONTEST:
        tested ();
        return;
    }
    if (r->other == RS_THING) {
        thing ();
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
    if (r->result != 0xFF) {
        left (shy);
    }
    pack ();
}
