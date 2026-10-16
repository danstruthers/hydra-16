/*
** sheet.c - (overlay 1, the referee's: called through bank.s's stubs) the character's sheet in words, and harm taken
** on it.  The words go into the reply as dq0harn.c's put puts them (its own strings here, in the window, the hub's
** text put back in for each character); a request's read and answered in a copy of it in the main RAM (the hub's not
** in the window while the overlay is).
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

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
** hour (then wakes anyway).  A trap's strike and a burn have the armour where they strike taken off them (Combat
** 13: gear.c's pc_armour).
**
** The striker's own part hurt by its blow at a thing (HC_STRUCK: fight.c's, the impact its own, its armour taken
** off already): a blunt hit on it, made an injury as any is; enough, and it stops (the game's: that part's injuries
** three levels or more, a serious one, or the shock of it) */

char hurt[200];                                 /* The last harm's words */

/* An injury's words, by its aspect and severity (minor, serious, grievous or worse) */
static const char* const wound_word[4][3] = {
    { "bruise", "fracture", "crush" }, { "cut", "gash", "deep gash" }, { "stab", "stab", "deep stab" },
    { "burn", "burn", "burn" }
};

/* Where it is ("left thigh") */
static void __fastcall__ put_loc (unsigned char loc)
{
    if (harn_is_limb (loc)) {
        put (loc & HARN_RIGHT ? "right " : "left ");
    }
    put (harn_location_name (loc));
}

/* An injury told (sev HARN_KILL: a death): its effect, at e, and its words ("a serious fracture to your left thigh
** (S2)") */
static void __fastcall__ told (const harn_injury* j, unsigned char sev, struct rs_effect* e)
{
    e->target = 0;
    e->kind = sev == HARN_KILL ? X_DEATH : X_WOUND;
    e->band = sev;
    e->phrase = 1;
    e->cond = conditions ();
    e->x[0] = j->loc;                           /* (rs.h's BP_* and AS_* are the book's order) */
    e->x[1] = j->aspect;
    put (hurt[0] ? "; a " : "a ");
    put (harn_severity_name (sev));
    put (" ");
    put (wound_word[j->aspect & 3][sev > HARN_GRIEVOUS ? 2 : sev - 1]);
    put (" to your ");
    put_loc (j->loc);
    if (sev <= HARN_GRIEVOUS) {
        put (sev == HARN_MINOR ? " (M" : sev == HARN_SERIOUS ? " (S" : " (G");
        putn (j->il);
        put (")");
    }
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
    unsigned char how = 0, aspect, zone = HARN_MID, n = pc.ninj, k, w, loc = 0xFF;
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
        impact = harn_fall (&pc, r->w1, r->b, (r->c & HF_CLEAR ? HARN_FALL_CLEAR : 0) |
            (r->c & HF_HOLDS ? HARN_FALL_HOLDS : 0), &how);
        r->result = (how & HARN_FALL_CAUGHT) != 0;
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
        told (&pc.inj[n < HARN_INJMAX ? n : HARN_INJMAX - 1], HARN_MINOR, e++);
    }
    d.loc = loc != 0xFF ? loc : harn_strike_location (zone);
    d.aspect = aspect;
    if (r->a == HC_STRIKE || r->a == HC_FIRE) {
        impact -= pc_armour (d.loc, aspect);    /* (The armour there) */
    }
    w = harn_wound (&pc, d.loc, aspect, impact);
    if (w & HARN_W_DEAD) {
        told (&d, HARN_KILL, e);
        r->b = HARN_KILL;
    } else if (w & HARN_W_INJURED) {
        told (&pc.inj[pc.ninj - 1], pc.inj[pc.ninj - 1].sev, e);
        r->b = e->band;
        if ((w & HARN_W_STUMBLE) && harn_stumble_roll (&pc, 0)) {
            e->cond |= C_PRONE;                 /* (A leg's: fallen) */
        }
    } else if (how & HARN_FALL_WRENCHED) {
        r->b = HARN_MINOR;
    }
    if (pc.state == HARN_UNCONSCIOUS) {         /* (Senseless: a shock roll a round to come round) */
        for (k = 1; k < 360 && harn_wake (&pc) == HARN_UNCONSCIOUS; ++k) {
        }
        if (pc.state == HARN_UNCONSCIOUS) {
            pc.state = HARN_CONSCIOUS;
        }
        r->w2 = k * 100;
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
    for (k = 0; k < pc.ninj; ++k) {           /* ("left thigh S2 (fracture), bleeding") */
        j = &pc.inj[k];
        put (k ? "; " : " ");
        put_loc (j->loc);
        put (j->sev == HARN_MINOR ? " M" : j->sev == HARN_SERIOUS ? " S" : " G");
        putn (j->il);
        put (" (");
        put (wound_word[j->aspect & 3][j->sev > HARN_GRIEVOUS ? 2 : j->sev - 1]);
        put (j->flags & HARN_IF_BLEEDER ? "), bleeding" : ")");
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

/* ---- What the character notices, by its state (DUNGEON.md: moving; the game's steps): its pace as it moves (a creep
** or a slow walk a step better, but not by feel in the dark; a jog two worse, a run four, a sprint six); in shock four
** worse (the book's universal penalty, its injuries' and its fatigue's, is in every test already); listening, a sound
** two better.  A hazard underfoot (P_NOTICE_SIGHT's b 1, its steps the hiding's) by a search's chance while it
** searches as it goes, else a passing glance's: twice its hiding's steps, and two more (r a copy; the hub's pace and
** watchfulness in ps) */

static const signed char by_pace[NGAITS] = { 1, 1, 0, -2, -4, -6 };

signed char __fastcall__ o_noticing (const struct req* r)
{
    unsigned char g = ps.go & ~GOING_DARK;
    signed char s = 0, p;

    if (r->a == P_NOTICE_SIGHT && r->b == 1 && !(ps.watch & WA_TRAPS)) {
        s = r->steps - 2;
    }
    if (g && g <= NGAITS) {
        p = by_pace[g - 1];
        s += p > 0 && (ps.go & GOING_DARK) ? 0 : p;
    }
    if (pc.state == HARN_SHOCK) {
        s -= 4;
    }
    if (r->a == P_NOTICE_SOUND && (ps.watch & WA_LISTEN)) {
        s += 2;
    }
    return s;
}

/* ---- Fatigue (DUNGEON.md: moving; the book's movement fatigue, Combat 6: a level a day walking, one each five
** minutes jogging, each two running, each minute sprinting; ten minutes' rest a level gone, Skills 6).  The game's
** own: slower than a walk tires as a walk does; the time at each pace added up across moves (exertion toward the next
** level, 6000 a level: a sprint's 10 a tenth, a run's 5, a jog's 2, a walk's a 144th), and rest wears that off (1 a
** tenth) before it takes a level off */

#define LEVEL   6000

void __fastcall__ o_spent (unsigned t)
{
    static const unsigned char rate[NGAITS] = { 0, 0, 0, 2, 5, 10 };
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
        k = a < NGAITS ? rate[a] : 0;
        tired += k ? n * k : n / 144;
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
** condition (its health, its injuries in brief, its fatigue and its load, the penalties they make) and its light */

static const char* const pace_name[NGAITS] = { "a creep", "a slow walk", "a walk", "a jog", "a run", "a sprint" };
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
    unsigned char g = ps.gait, h = g, k, worst = 0, bleed = 0;
    const harn_injury* j;

    if (h > GA_SLOW && ((ps.watch & WA_TRAPS) || ps.dark)) {
        h = GA_SLOW;
    } else if (h > GA_WALK && (ps.watch & WA_LISTEN)) {
        h = GA_WALK;
    }
    put ("Pace: ");
    put (pace_name[g]);
    if (h != g) {
        put (", held to ");
        put (pace_name[h]);
        put (ps.watch & WA_TRAPS ? " while you search as you go" : ps.dark && h == GA_SLOW ? " in the dark" :
            " while you listen");
    }
    put (" (");
    putn (harn_move (&pc) * feet[h][0] / feet[h][1]);
    put (" feet a round).\nWatching: ");
    put (!ps.watch ? "nothing in particular." : ps.watch == WA_LISTEN ? "listening as you go." : ps.watch & WA_LISTEN ?
        "for traps and hidden doors as you go, and listening." : "for traps and hidden doors as you go.");
    if (!ps.full) {
        return;
    }
    put ("\nCondition: ");
    put (pc.state == HARN_DEAD ? "dead" : pc.state == HARN_UNCONSCIOUS ? "senseless" : pc.state == HARN_SHOCK ?
        "in shock" : !pc.ninj ? "unhurt" : "hurt");
    for (k = 0; k < pc.ninj; ++k) {
        j = &pc.inj[k];
        if (j->sev >= pc.inj[worst].sev) {
            worst = k;
        }
        bleed |= j->flags & HARN_IF_BLEEDER;
    }
    if (pc.ninj) {                              /* ("hurt: 2 injuries, the worst a serious gash to your left thigh
                                                **   (S3), bleeding") */
        put (": ");
        putn (pc.ninj);
        put (pc.ninj == 1 ? " injury, a " : " injuries, the worst a ");
        j = &pc.inj[worst];
        put (harn_severity_name (j->sev));
        put (" ");
        put (wound_word[j->aspect & 3][j->sev > HARN_GRIEVOUS ? 2 : j->sev - 1]);
        put (" to your ");
        put_loc (j->loc);
        put (j->sev == HARN_MINOR ? " (M" : j->sev == HARN_SERIOUS ? " (S" : " (G");
        putn (j->il);
        put (bleed ? "), bleeding" : ")");
    }
    penalty (pc.fl, "a little tired", "tired", "exhausted", "fatigue ");
    penalty (harn_get_ep (&pc), "a little burdened", "heavily burdened", "overloaded", "encumbrance ");
    put ("; penalties: universal ");
    putn (harn_get_up (&pc));
    put (", physical ");
    putn (harn_get_pp (&pc));
    put (".\nLight: ");
    if (ps.light) {
        put ("your ");
        put (ps.lamp);
        put (" (");
        putn (5 * ps.light);
        put (" feet).");
    } else {
        put (ps.dark ? "none: it's dark here, and you feel your way." : "none of your own: this place's own light.");
    }
}
