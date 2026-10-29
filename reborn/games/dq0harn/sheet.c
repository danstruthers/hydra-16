/*
** sheet.c - (overlay 1, the referee's: called through bank.s's stubs) the character's sheet in words, and harm taken
** on it.  The words go into the reply as dq0harn.c's put puts them (its own strings here, in the window, the hub's
** text put back in for each character); a request's read and answered in a copy of it in the main RAM (the hub's not
** in the window while the overlay is).
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

/* ---- Harm (rs.h's RS_HARM): a fall (harn.h's harn_fall, Skills 8), a trap's strike, a burn, a striker's own hurt; each
** an impact on a location made a HarnMaster injury on the sheet (harn_wound, Combat 13-14: its severity and levels,
** the kill and shock rolls, bleeding, a limb made useless), told back as effects and in words (kept for RS_DESCRIBE
** b 2).  The game's own choices in using the book: a fall strikes where a mid strike would (on its feet, a low one),
** no armour taken off it; one knocked senseless comes round by the book's shock rolls, a round each, within an
** hour (then wakes anyway, its last shock roll the book's: in shock, or not).  A trap's strike and a burn have the armour where they strike taken off them (Combat
** 13: gear.c's pc_armour).
**
** The striker's own part hurt by its blow at a thing (HC_STRUCK: fight.c's, the impact its own, its armour taken
** off already): a blunt hit on it, made an injury as any is; enough, and it stops (the game's: that part's injuries
** three levels or more, a serious one, or the shock of it) */

char hurt[200];                                 /* The last harm's words */

/* ---- A wound (the referee's every one, the fights' too: fight.c's through bank.s's stub) and its words.  By the
** book (harn_wound, Combat 13-14), and the game's own ways round the library's: its record keeps twelve injuries
** (HARN_INJMAX), and past them it adds each new wound's levels into the last, whose words then were the running total
** ("a serious stab (S37)"); so here, before a wound, the record's room made: its least two injuries (by severity, then
** levels) folded into one, the lesser hurts (IF_POOL: their levels together, so the universal penalty and the shock
** rolls still count every one; never named as an injury, nor the worst), and the least of the rest into that as more
** come.  And the book's shock roll with every injury (Combat 14), which the library makes only for one conscious:
** one in shock wounded again rolls too, and falls senseless if it fails (it was struck on and on, worse and worse,
** and never fell: the user's playtest, 2026-10-21).  What dealt it (a creature's own part: BY_*) is the character's
** injury's to keep, for its words: a bite a bite, not a stab */

unsigned char pc_by[HARN_INJMAX];

/* An injury's words, by its aspect and severity (minor, serious, grievous or worse); or by the creature's part that
** dealt it (BY_BITE ...: a bite, claws' rake, a sting, horns' gore) */
static const char* const wound_word[4 + BY_VENOM][3] = {
    { "bruise", "fracture", "crush" }, { "cut", "gash", "deep gash" }, { "stab", "stab", "deep stab" },
    { "burn", "burn", "burn" }, { "bite", "bite", "deep bite" }, { "scratch", "rake", "deep rake" },
    { "sting", "sting", "deep sting" }, { "gore", "gore", "deep gore" }, { "ache", "ache", "ache" }
};

static const char* __fastcall__ noun (unsigned char by, unsigned char aspect, unsigned char sev)
{
    by &= ~BY_ITS;
    return wound_word[by && by <= BY_VENOM ? 3 + by : aspect & 3][sev > HARN_GRIEVOUS ? 2 : sev ? sev - 1 : 0];
}

/* Where it is ("left thigh"), after w */
static void __fastcall__ cat_loc (char* w, unsigned char loc)
{
    if (harn_is_limb (loc)) {
        strcat (w, loc & HARN_RIGHT ? "right " : "left ");
    }
    strcat (w, harn_location_name (loc));
}

void __fastcall__ o_wound_words (char* w, unsigned char sev, unsigned char loc, unsigned char aspect, unsigned char il,
    unsigned char by)
{
    char n[4];

    strcpy (w, "a ");
    strcat (w, harn_severity_name (sev));
    strcat (w, " ");
    strcat (w, noun (by, aspect, sev));
    strcat (w, by & BY_ITS ? " to its " : " to your ");
    cat_loc (w, loc);
    if (sev <= HARN_GRIEVOUS) {
        strcat (w, sev == HARN_MINOR ? " (M" : sev == HARN_SERIOUS ? " (S" : " (G");
        strcat (w, utoa (il, n, 10));           /* (Ten or more two digits) */
        strcat (w, ")");
    }
}

/* Room in c's record for n injuries more: its least folded into the lesser hurts (made of the least two, the first
** time), the bleeding kept; the character's pc_by kept in step (the places past its injuries none's) */
static void __fastcall__ room (harn_char* c, unsigned char n)
{
    harn_injury* j;
    harn_injury* p;
    unsigned char k, least, pool;
    unsigned v;

    while (c->ninj + n > HARN_INJMAX) {
        least = pool = 0xFF;
        for (k = 0; k < c->ninj; ++k) {
            j = &c->inj[k];
            if (j->flags & IF_POOL) {
                pool = k;
            } else if (least == 0xFF || j->sev < c->inj[least].sev || (j->sev == c->inj[least].sev &&
                j->il < c->inj[least].il)) {
                least = k;
            }
        }
        j = &c->inj[least];
        if (pool == 0xFF) {
            j->flags = (j->flags & (HARN_IF_BLEEDER | HARN_IF_INFECTED)) | IF_POOL;    /* (The lesser hurts begun:
                                                **   the next least folded into it) */
            continue;
        }
        p = &c->inj[pool];
        v = p->il + j->il;
        p->il = v > 255 ? 255 : v;
        p->sev = j->sev > p->sev ? j->sev : p->sev;
        p->flags |= j->flags & (HARN_IF_BLEEDER | HARN_IF_INFECTED);
        --c->ninj;                              /* (Its place the last one's) */
        memcpy (j, &c->inj[c->ninj], sizeof *j);
        if (c == &pc) {
            pc_by[least] = pc_by[c->ninj];
        }
    }
    if (c == &pc) {
        memset (pc_by + c->ninj, 0, HARN_INJMAX - c->ninj);
    }
}

unsigned char __fastcall__ o_wound (harn_char* c, unsigned char loc, unsigned char aspect, int impact,
    unsigned char by)
{
    unsigned char w;

    room (c, 1);
    w = harn_wound (c, loc, aspect, impact);
    if ((w & HARN_W_INJURED) && !(w & HARN_W_DEAD)) {
        if (c == &pc) {
            pc_by[c->ninj - 1] = by;
        }
        if (c->state == HARN_SHOCK && harn_shock_roll (c)) {
            c->state = HARN_UNCONSCIOUS;        /* (In shock, wounded again: its shock roll failed, senseless) */
            w |= HARN_W_DOWN;
        }
    }
    return w;
}

/* Knocked senseless (its shock roll failed): the rounds till c comes round, by the book's shock rolls, a round each,
** within an hour (then anyway: the character's last shock roll the book's, in shock or not), in tenths */
static unsigned __fastcall__ wake (harn_char* c)
{
    unsigned k;

    for (k = 1; k < 360 && harn_wake (c) == HARN_UNCONSCIOUS; ++k) {
    }
    if (c == &pc && c->state == HARN_UNCONSCIOUS) {
        c->state = harn_shock_roll (c) ? HARN_SHOCK : HARN_CONSCIOUS;
    }
    return k * 100;
}

/* A fight's (fight.c's strike, through bank.s's stub): c knocked senseless, its tenths out (wake's); *o its time out
** (a creature's bs.out, the character's pc_out) the later of its own and that (out already, struck again as it lies:
** AF_AGAIN in *f, its black-out told once); a creature lies there till then (its sheet's state), the character comes
** round as the UI waits it out (it can't defend meanwhile) */
unsigned __fastcall__ o_knocked_out (harn_char* c, unsigned long* o, unsigned char* f)
{
    unsigned w = wake (c);

    if (*o > tnow) {
        *f |= AF_AGAIN;
        if (*o - tnow > w) {
            w = (unsigned) (*o - tnow);
        }
    }
    *o = tnow + w;
    if (c != &pc) {
        bs.state = c->state == HARN_SHOCK ? bs.state | BS_SHOCKED : bs.state & ~BS_SHOCKED;  /* (Its last shock
                                                **   roll's: it comes round in shock, fight.c's unpack) */
        c->state = HARN_UNCONSCIOUS;
    }
    return w;
}

/* An injury told (sev HARN_KILL: a death): its effect, at e, and its words ("a serious fracture to your left thigh
** (S2)"), by what dealt it */
static void __fastcall__ told (const harn_injury* j, unsigned char sev, struct rs_effect* e, unsigned char by)
{
    char b[56];

    e->target = 0;
    e->kind = sev == HARN_KILL ? X_DEATH : X_WOUND;
    e->band = sev;
    e->phrase = 1;
    e->cond = conditions ();
    e->x[0] = j->loc;                           /* (rs.h's BP_* and AS_* are the book's order) */
    e->x[1] = j->aspect;
    if (hurt[0]) {
        put ("; ");
    }
    o_wound_words (b, sev, j->loc, j->aspect, j->il, by);
    put (b);
}

/* The levels of the injuries to a part (a hand, a foot, the skull), all told */
static unsigned char __fastcall__ part_levels (unsigned char part)
{
    unsigned char k, l = 0;

    for (k = 0; k < pc.ninj; ++k) {
        if (HARN_LOC (pc.inj[k].loc) == part) {
            l += pc.inj[k].il;
        }
    }
    return l;
}

/* r a copy of the request, in the main RAM (but its text) */
void __fastcall__ o_harm (struct req* r)
{
    static harn_injury d;
    struct rs_effect* e = (struct rs_effect*) r->x;
    unsigned char how = 0, aspect, zone = HARN_MID, n = pc.ninj, w, loc = 0xFF;
    int impact;

    memset (r->x, 0, sizeof r->x);
    tend = hurt + sizeof hurt - 1;
    if (r->a == HC_STRUCK && r->w2) {
        tp = hurt + strlen (hurt);              /* (A run of blows: the part's words added to) */
    } else {
        tp = hurt;
        *tp = 0;
    }
    if (r->other || pc.state == HARN_DEAD) {
        r->result = 1;                          /* (Only the character has a sheet, so far) */
        return;
    }
    switch (r->a) {
    case HC_FALL:
        room (&pc, 2);                          /* (A wrenched arm's room too: the library's own harn_wound's) */
        n = pc.ninj;
        impact = harn_fall (&pc, r->w1, r->b, (r->c & HF_CLEAR ? HARN_FALL_CLEAR : 0) |
            (r->c & HF_HOLDS ? HARN_FALL_HOLDS : 0), &how);
        r->result = (how & HARN_FALL_CAUGHT) != 0;
        if ((how & HARN_FALL_WRENCHED) && pc.state == HARN_SHOCK && harn_shock_roll (&pc)) {
            pc.state = HARN_UNCONSCIOUS;        /* (In shock, the wrench's shock roll: o_wound's) */
        }
        if (how & HARN_FALL_FEET) {
            zone = HARN_LOW;
        }
        aspect = HARN_BLUNT;
        break;
    case HC_STRUCK:                             /* (Its own part, by its blow at a thing: c which, a limb either one) */
        impact = r->w1;
        aspect = HARN_BLUNT;
        loc = r->c | (harn_is_limb (r->c) && dice (1, 2) == 2 ? HARN_RIGHT : 0);
        break;
    default:                                    /* (A strike, or fire: its dice, and more, where it's aimed) */
        impact = dice (r->c, 6) + (int) r->w1;
        aspect = r->a == HC_FIRE ? HARN_FIRE : r->b & 3;
        zone = r->w2 == Z_HIGH ? HARN_HIGH : r->w2 == Z_LOW ? HARN_LOW : HARN_MID;
        break;
    }
    r->b = 0;
    r->w2 = 0;
    if (how & HARN_FALL_WRENCHED) {             /* (Grabbing a hold: an arm wrenched, its injury taken) */
        told (&pc.inj[n], HARN_MINOR, e++, BY_ASPECT);
    }
    d.loc = loc != 0xFF ? loc : harn_strike_location (zone);
    d.aspect = aspect;
    if (r->a == HC_STRIKE || r->a == HC_FIRE) {
        impact -= pc_armour (d.loc, aspect);    /* (The armour there) */
    }
    w = o_wound (&pc, d.loc, aspect, impact, BY_ASPECT);
    if (w & HARN_W_DEAD) {
        told (&d, HARN_KILL, e, BY_ASPECT);
        r->b = HARN_KILL;
    } else if (w & HARN_W_INJURED) {
        told (&pc.inj[pc.ninj - 1], pc.inj[pc.ninj - 1].sev, e, BY_ASPECT);
        r->b = e->band;
        if ((w & HARN_W_STUMBLE) && ((w & HARN_W_LIMB) || harn_stumble_roll (&pc, 0))) {
            e->cond |= C_PRONE;                 /* (A leg's: fallen; a grievous one at once, Combat 14) */
        }
        n = d.loc & HARN_RIGHT ? DH_RIGHT : DH_LEFT;
        if ((w & HARN_W_FUMBLE) && (hheld & n) && ((w & HARN_W_LIMB) || harn_fumble_roll (&pc, 0))) {
            e[1].target = 0;                    /* (An arm's, its hand holding something: let go of, the world's to
                                                **   drop; Combat 14, slice 4.1b's) */
            e[1].kind = X_DROP;
            e[1].x[0] = n;
        }
    } else if (how & HARN_FALL_WRENCHED) {
        r->b = HARN_MINOR;
    }
    if (pc.state == HARN_UNCONSCIOUS) {         /* (Senseless: a shock roll a round to come round) */
        r->w2 = wake (&pc);
    }
    if (r->a == HC_STRUCK) {                    /* (Enough: the referee's say) */
        r->result = pc.state != HARN_CONSCIOUS || r->w2 || r->b >= HARN_SERIOUS || part_levels (HARN_LOC (loc)) >= 3;
    }
}

/* ---- The sheet */

static const unsigned char shown[] = {
    HARN_STR, HARN_STA, HARN_DEX, HARN_AGL, HARN_EYE, HARN_HRG, HARN_SML, HARN_VOI, HARN_INT, HARN_AUR, HARN_WIL,
    HARN_MOR
};

void o_sheet (void)
{
    unsigned char k;
    const harn_injury* j;
    char b[20];

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
    put ("; load ");
    putn (harn_get_load (&pc));
    put (" lb, encumbrance ");
    putn (harn_get_ep (&pc));
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
    put (".\n");
    if (!pc.ninj) {
        put ("No injuries");
    } else {
        put ("Injuries:");
    }
    for (k = 0; k < pc.ninj; ++k) {           /* ("left thigh S2 (fracture), bleeding"; "lesser hurts (7 levels)") */
        j = &pc.inj[k];
        put (k ? "; " : " ");
        if (j->flags & IF_POOL) {
            put ("lesser hurts (");
            putn (j->il);
            put (" levels");
        } else {
            b[0] = 0;
            cat_loc (b, j->loc);
            put (b);
            put (j->sev == HARN_MINOR ? " M" : j->sev == HARN_SERIOUS ? " S" : " G");
            putn (j->il);
            put (" (");
            put (noun (pc_by[k], j->aspect, j->sev));
        }
        put (j->flags & HARN_IF_BLEEDER ? "), bleeding" : j->flags & HARN_IF_INFECTED ? "), infected" : j->flags &
            HARN_IF_TREATED ? "), tended" : ")");
    }
    if (pc.bl) {
        put ("; bloodloss ");
        putn (pc.bl);
    }
    if (pc.state) {
        put (pc.state == HARN_UNCONSCIOUS ? "; senseless" : pc.state == HARN_SHOCK ? "; in shock" : "; dead");
    }
    put (".");
}

/* The short lines: the attributes in pairs, then endurance, the penalties, the injuries' levels and bloodloss, as
** they are now ("STR  3  STA  5") */
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

void o_brief (void)
{
    unsigned char k;

    for (k = 0; k < sizeof shown; k += 2) {
        pair (abbr[k], pc.att[shown[k]], abbr[k + 1], pc.att[shown[k + 1]]);
    }
    put ("END ");
    putn (harn_end (&pc));
    put ("\n");
    pair ("UP ", harn_get_up (&pc), "PP ", harn_get_pp (&pc));
    pair ("IP ", harn_get_ip (&pc), "BL ", pc.bl);
}

/* ---- What the character notices, by its state (DUNGEON.md: moving; the game's steps): its pace as it moves, its
** listening, a hazard underfoot by a glance or a search (the house's, every referee's: rshouse.h's house_notice); in
** shock four worse (the book's universal penalty, its injuries' and its fatigue's, is in every test already) (r a
** copy; the hub's pace and watchfulness in ps) */

#define RS_HOUSE_NOTICE                         /* (The pace's and the watchfulness's: every referee's) */
#define RS_HOUSE_STATE
#include "../dq0/rshouse.h"

signed char __fastcall__ o_noticing (const struct req* r)
{
    return house_notice (r, ps.go, ps.watch) - (pc.state == HARN_SHOCK ? 4 : 0);
}

/* ---- Fatigue (DUNGEON.md: moving; the book's movement fatigue, Combat 6: a level a day walking, one each five
** minutes jogging, each two running, each minute sprinting; ten minutes' rest a level gone, Skills 6).  The game's
** own: slower than a walk tires as a walk does; the time at each pace added up across moves (exertion toward the next
** level, 6000 a level: a sprint's 10 a tenth, a run's 5, a jog's 2, a walk's a 144th), and rest wears that off (1 a
** tenth) before it takes a level off.  Strenuous work (GA_WORK: HOUSE.md's tiring, a climb, a dig, a battering;
** swimming to come) the book's for it, a level each five minutes (Skills 6; a swimmer's the same), as a jog's */

#define LEVEL   6000
#define WORK    2                               /* (Strenuous work's exertion a tenth: a level each five minutes) */

void __fastcall__ o_spent (unsigned t)
{
    static const unsigned char rate[NGAITS] = { 0, 0, 0, 2, 5, 10 };
    static unsigned walkt;                      /* (A walk's tenths, toward its next unit) */
    unsigned char a = spent_at, k;
    unsigned n;

    while (t) {                                 /* (A minute at a time: in 16 bits) */
        n = t > 600 ? 600 : t;
        t -= n;
        if (a == GA_REST) {

            if (n <= tired) {
                tired -= n;
                continue;
            }
            rested += n - tired;
            tired = 0;
            while (rested >= LEVEL && pc.fl) {
                rested -= LEVEL;
                --pc.fl;
            }
            if (!pc.fl) {
                rested = 0;
            }
            continue;
        }
        rested = 0;
        k = a < NGAITS ? rate[a] : a == GA_WORK ? WORK : 0;
        if (!k) {
            walkt += n;
            n = walkt / 144;
            walkt %= 144;
        }
        tired += k ? n * k : n;
        while (tired >= LEVEL) {
            tired -= LEVEL;
            if (pc.fl < 99) {
                ++pc.fl;
            }
        }
    }
}

/* ---- The character's state (the status command's: RS_DESCRIBE b 3, ps's): its pace, what it's held to and its feet
** a round (the book's rates of its Move, Combat 6; the game's two slower), its watchfulness; then (all of it) its
** condition (its health, its injuries in brief, its fatigue and its load, the penalties they make) and its light (the
** pace's, the watchfulness's and the light's words the house's, every referee's: rshouse.h's) */

static const unsigned char feet[NGAITS][2] = { { 5, 6 }, { 5, 4 }, { 5, 2 }, { 5, 1 }, { 10, 1 }, { 15, 1 } };

static void __fastcall__ penalty (unsigned char n, const char* a, const char* b, const char* c, const char* what)
{
    if (n) {
        put ("; ");
        put (n <= 2 ? a : n <= 4 ? b : c);
        put (" (");
        put (what);
        putn (n);
        put (")");
    }
}

void o_state (void)
{
    unsigned char h, k, worst = 0xFF, bleed = 0, n = 0, pool = 0;
    const harn_injury* j;
    char b[56];

    h = house_pace (ps.gait, ps.watch, ps.dark == 1);   /* (The pace's words, every referee's: rshouse.h; its
                                                **   pace said outright in the dark, held to nothing there) */
    put (" (");
    k = harn_move (&pc);
    if (pc.state == HARN_SHOCK) {
        k = (k + 1) / 2;                        /* (In shock, half its Move, a walk at most: dq0harn.c's status) */
        h = h > GA_WALK ? GA_WALK : h;
    }
    putn (k * feet[h][0] / feet[h][1]);
    put (" feet a round)");
    house_watching (ps.watch);
    if (!ps.full) {
        return;
    }
    put ("\nCondition: ");
    put (pc.state == HARN_DEAD ? "dead" : pc.state == HARN_UNCONSCIOUS ? "senseless" : pc.state == HARN_SHOCK ?
        "in shock" : !pc.ninj ? "unhurt" : "hurt");
    for (k = 0; k < pc.ninj; ++k) {             /* (The worst a real one, by its severity, then its levels: never
                                                **   the lesser hurts) */
        j = &pc.inj[k];
        bleed |= j->flags & HARN_IF_BLEEDER;
        if (j->flags & IF_POOL) {
            pool = 1;
            continue;
        }
        ++n;
        if (worst == 0xFF || j->sev > pc.inj[worst].sev || (j->sev == pc.inj[worst].sev && j->il >= pc.inj[worst].il)) {
            worst = k;
        }
    }
    if (n) {                                    /* ("hurt: 2 injuries, the worst a serious gash to your left thigh
                                                **   (S3), bleeding"; "11 injuries and lesser hurts, the worst ...") */
        put (": ");
        putn (n);
        put (n == 1 && !pool ? " injury, " : pool ? " injuries and lesser hurts, the worst " : " injuries, the worst ");
        j = &pc.inj[worst];
        o_wound_words (b, j->sev, j->loc, j->aspect, j->il, pc_by[worst]);
        put (b);
        put (bleed ? ", bleeding" : "");
    }
    body_state ();                              /* (Tended, poisoned, its blood lost: heal.c's) */
    penalty (pc.fl, "a little tired", "tired", "exhausted", "fatigue ");
    penalty (harn_get_ep (&pc), "a little burdened", "heavily burdened", "overloaded", "encumbrance ");
    put ("; penalties: universal ");
    putn (harn_get_up (&pc));
    put (", physical ");
    putn (harn_get_pp (&pc));
    put (".");
    house_light (ps.light, ps.lamp, ps.dark);
}
