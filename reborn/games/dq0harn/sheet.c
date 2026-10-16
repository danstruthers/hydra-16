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

/* ---- Harm (rs.h's RS_HARM): a fall (harn.h's harn_fall, Skills 8), a trap's strike, a burn, a blow struck; each
** an impact on a location made a HarnMaster injury on the sheet (harn_wound, Combat 13-14: its severity and levels,
** the kill and shock rolls, bleeding, a limb made useless), told back as effects and in words (kept for RS_DESCRIBE
** b 2).  The game's own choices in using the book: a fall strikes where a mid strike would (on its feet, a low one);
** no armour yet (items will bring it); a tool's impacts, where the book has no weapon like it, the game's; one
** knocked senseless comes round by the book's shock rolls, a round each, within an hour (then wakes anyway).
**
** Bare hands at a thing (a door battered: the user's, 2026-10-15, DUNGEON.md: portals): the blow a d4, the
** Strength's modifier (the game's, by the bands of the book's Strength table, Combat 3: a point of impact a band
** from 9-12's) and the hands' armour's bonus; the world takes the thing's resistance off it, and gives the lesser of
** the two back as the hand's own hurt (HC_HANDS): an automatic blunt hit on one hand or the other, less its armour,
** made an injury as any is.  Enough, and the character stops: the hands' injuries three levels or more, a serious
** one, or the shock of it.  Armour on the hands comes with the items' telling the referee what's worn (RS_EQUIP's,
** to come: harn_armour_at over the pieces): none till then */

#define HAND_BONUS      0                       /* (A gauntlet's, on a blow) */
#define HAND_COVER      0                       /* (The hand's armour, against a blunt hit) */

char hurt[200];                                 /* The last harm's words */
char blow[8];                                   /* (A blow's: what it's struck with, its key) */

/* What a blow's struck with (a thing's key for the referee): its blunt, edge and point impact (-1: none); a knife
** and a blade the book's dagger and broadsword (Combat 3) */
static const struct { char key[8]; signed char imp[3]; } striker[] = {
    { "hands", { 0, -1, -1 } }, { "knife", { 1, 2, 5 } }, { "blade", { 3, 5, 3 } }, { "pick", { 2, -1, 5 } },
    { "mattock", { 3, 4, 4 } }, { "shovel", { 2, 3, -1 } }, { "spade", { 2, 3, -1 } }, { "chisel", { 1, -1, 2 } },
};

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

/* The Strength's modifier to a bare-handed blow (the bands of Combat 3's Strength table: 1-5, 6-8, 9-12, 13-15, 16+) */
static signed char str_mod (void)
{
    unsigned char s = pc.att[HARN_STR];

    return s <= 5 ? -2 : s <= 8 ? -1 : s <= 12 ? 0 : s <= 15 ? 1 : 2;
}

/* The levels of the injuries to the hands, all told */
static unsigned char hand_levels (void)
{
    unsigned char k, l = 0;

    for (k = 0; k < pc.ninj; ++k) {
        if (HARN_LOC (pc.inj[k].loc) == HARN_HAND) {
            l += pc.inj[k].il;
        }
    }
    return l;
}

/* r a copy of the request, in the main RAM (but its text: blow has the key in it, and a blow at a thing's put back
** in it, what struck) */
void __fastcall__ o_harm (struct req* r)
{
    static harn_injury d;
    struct rs_effect* e = (struct rs_effect*) r->x;
    unsigned char how = 0, aspect, zone = HARN_MID, n = pc.ninj, k, w, loc = 0xFF;
    const signed char* imp;
    int impact;

    memset (r->x, 0, sizeof r->x);
    tend = hurt + sizeof hurt - 1;
    if ((r->a == HC_BLOW && r->other == RS_THING) || (r->a == HC_HANDS && r->w2)) {
        tp = hurt + strlen (hurt);              /* (A run of blows: the hands' words added to) */
    } else {
        tp = hurt;
        *tp = 0;
    }
    if ((r->other && r->other != RS_THING) || pc.state == HARN_DEAD) {
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
    case HC_BLOW:                               /* (With what's in hand: unknown, bare hands) */
        for (k = sizeof striker / sizeof striker[0] - 1; k && strcmp (striker[k].key, blow); --k) {
        }
        if (!*blow && r->other == RS_THING) {   /* (At a thing, none named: the tool to hand that hits it hardest,
                                                **   but a weapon, which a thing would spoil) */
            for (w = 3, aspect = 0; w < sizeof striker / sizeof striker[0]; ++w) {
                if (has (striker[w].key) && striker[w].imp[0] + striker[w].imp[1] + striker[w].imp[2] > aspect) {
                    aspect = striker[w].imp[0] + striker[w].imp[1] + striker[w].imp[2];
                    k = w;
                }
            }
        }
        imp = striker[k].imp;
        aspect = r->b;
        if (aspect > AS_POINT || imp[aspect] < 0) {
            for (aspect = AS_BLUNT, w = 1; w < 3; ++w) {    /* (Its best) */
                aspect = imp[w] > imp[aspect] ? w : aspect;
            }
        }
        impact = imp[aspect] + dice (r->c ? r->c : 1, 6);
        if (r->other == RS_THING) {
            if (!k) {                           /* (Bare hands: the user's d4, the Strength's, the armour's) */
                impact = dice (1, 4) + str_mod () + HAND_BONUS;
            }
            r->w1 = impact < 0 ? 0 : impact;    /* (A thing's: what it takes of it is the world's, by its aspect, */
            r->b = aspect;                      /*   b, and what struck it, the text) */
            strcpy (blow, striker[k].key);
            return;
        }
        break;
    case HC_HANDS:                              /* (Its own hand, by its blow at a thing: either one) */
        impact = (int) r->w1 - HAND_COVER;
        aspect = HARN_BLUNT;
        loc = HARN_HAND | (dice (1, 2) == 2 ? HARN_RIGHT : 0);
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
    if (r->a == HC_HANDS) {                     /* (Enough: the referee's say) */
        r->result = pc.state != HARN_CONSCIOUS || r->w2 || r->b >= HARN_SERIOUS || hand_levels () >= 3;
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
