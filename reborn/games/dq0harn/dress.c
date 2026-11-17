/*
** dress.c - (overlay 9, the referee's: called through bank.s's stub, heal.c's RS_TREAT) first aid and the dressings
** (slice 4.3's, heal.c's till 2026-11-10; HOUSE.md's wounds over time and its bandages): a wound treated (RS_TREAT:
** Physician 2-3, the Treatment table), the character the healer, the patient itself (other 0) or an actor (its entry
** plus 1: its sheet's); and a dressing kept on each of the character's wounds (pc_dr: the user's, 2026-11-10, "A
** bandage's location should be known"), its grade the bandage's that went on it (the tools' digit, the world's: its
** cloth's suitability by its cleanliness, linen 9, silk 7, wool 5, hemp 3), soiled as the days pass (heal.c's) and
** changed ("change the bandage on my arm": a tend of five minutes, no new treatment roll, the old dressing kept as a
** bloody bandage, the world's).  The treatment's bonus, a dressing's or surgery's, the table's scaled by the best
** bandage to hand (d of 9: none, none).
*/

#pragma code-name ("OV9CODE")
#pragma rodata-name ("OV9RODATA")
#pragma local-strings (on)

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps);    /* (Overlay 5's) */

unsigned char pc_dr[HARN_INJMAX];               /* (The character's wounds' dressings, each its grade plus 1: 0 none;
                                                **   kept in step with pc_by, saved as its file's "dressed" line) */

#define EE      7                               /* (The library's, harnheal.c's: a treatment's EE, healed in a day; */
#define IMP     0x80                            /*   the table's *, an attribute the less once it's healed) */

/* The Treatment table (the library's, harnheal.c's, compiled into heal.c's overlay: its copy here), by aspect and
** severity: the treatment's bonus, the healing rate untreated, and with CF, MF, MS, CS */
static const unsigned char treatment[4][3][6] = {
    { { 30, 5, 4, 5, 6, EE }, { 20, 4 | IMP, 3 | IMP, 4 | IMP, 5, 6 }, { 10, 3 | IMP, 2 | IMP, 3 | IMP, 4, 5 } },
    { { 30, 5, 4, 5, 6, EE }, { 20, 4, 3, 4, 5, 6 }, { 10, 3 | IMP, 2 | IMP, 3 | IMP, 4 | IMP, 5 } },
    { { 25, 5, 4, 5, 6, EE }, { 15, 4, 3, 4, 5, 6 }, { 5, 3 | IMP, 2 | IMP, 3 | IMP, 4 | IMP, 5 } },
    { { 30, 5, 4, 5, 6, EE }, { 15, 3, 2, 3, 4, 5 }, { 5, 2, 1, 2, 3, 4 } },
};

/* ---- A creature's sheet (by its record's number) into cr, and back (heal.c's, its copy) */

static void __fastcall__ sheet_in (unsigned char n)
{
    sheet_get (n);
    memset (&cr, 0, sizeof cr);
    cr.att[HARN_STR] = bs.att[0];
    cr.att[HARN_STA] = bs.att[1];
    cr.att[HARN_DEX] = bs.att[2];
    cr.att[HARN_AGL] = bs.att[3];
    cr.att[HARN_WIL] = bs.att[4];
    cr.att[HARN_WT] = bs.wt * 4;
    cr.state = bs.state & 3;
    cr.shockhr = cr.state == HARN_SHOCK ? (unsigned char) bs.out : 0;
    cr.bl = bs.bl;
    cr.ninj = bs.ninj;
    memcpy (cr.inj, bs.inj, sizeof bs.inj);
}

static void __fastcall__ sheet_out (unsigned char n)
{
    bs.state = cr.state | (bs.state & (BS_DISARMED | BS_SHOCKED));
    if (cr.state == HARN_SHOCK) {
        bs.out = cr.shockhr;
    }
    bs.bl = cr.bl;
    bs.ninj = cr.ninj;
    memcpy (bs.inj, cr.inj, sizeof bs.inj);
    sheet_put (n);
}

/* A wound's words, "the minor cut to your left forearm (M1)" (heal.c's wname, its copy: its words heal.c's ww, in the
** main RAM, where overlay 1 writes them) */
extern char ww[56];
static const char* __fastcall__ wname (const harn_char* c, unsigned char k)
{
    const harn_injury* j = &c->inj[k];

    wound_words (ww + 2, j->sev, j->loc, j->aspect, j->il, c == &pc ? pc_by[k] : BY_ITS | (beast_flags (bs.kind) & BF_THEY ?
        BY_THEIR : 0));                         /* (A person's "to their": 2026-11-17) */
    ww[0] = 't';                                /* ("a minor" ... "the minor") */
    ww[1] = 'h';
    ww[2] = 'e';
    return ww;
}

/* ---- First aid and treatment (RS_TREAT) */

enum { TR_TOURNIQUET, TR_DRESS, TR_COMPRESS, TR_SPLINT, TR_SURGERY, TR_CHANGE };

/* The treatment the book's table suggests for injury j (Physician 3: by its aspect and severity) */
static unsigned char __fastcall__ remedy (const harn_injury* j)
{
    if (j->flags & HARN_IF_BLEEDER) {
        return TR_TOURNIQUET;
    }
    switch (j->aspect & 3) {
    case HARN_BLUNT:
        return j->sev == HARN_MINOR ? TR_COMPRESS : j->sev == HARN_SERIOUS ? TR_SPLINT : TR_SURGERY;
    case HARN_FIRE:
        return j->sev == HARN_MINOR ? TR_COMPRESS : TR_DRESS;
    case HARN_EDGE:
        return j->sev == HARN_MINOR ? TR_DRESS : TR_SURGERY;
    }
    return j->sev == HARN_GRIEVOUS ? TR_SURGERY : TR_DRESS;
}

/* The best bandage to hand: its grade, the tools' digit (0-9: its cloth's suitability by its cleanliness, the world's;
** none, 0); its cloth in bmat (DR_MAT's: the tools' letter, dq0harn.c's equip_all; the first of that grade's, the lot
** the world uses up, s_use_key's) */
static unsigned char bmat;
static unsigned char best (void)
{
    unsigned char k, d = 0;

    for (k = 0; k < NTOOLS; ++k) {
        if (!strcmp (tools[k], "bandage") && (tqual[k] & 15) > d) {
            d = tqual[k] & 15;
            bmat = tqual[k] & DR_MAT;
        }
    }
    return d > 9 ? 9 : d;
}

/* Its bonus (the table's EML modifier, the tourniquet's +50), by what's to hand (the house's: bandages for a dressing,
** a knife or a dagger and bandages for surgery, the table's scaled by the best bandage's grade, of 9: HOUSE.md's
** bandages; none of them, no bonus) */
static int __fastcall__ bonus (const harn_injury* j, unsigned char how)
{
    int b = how == TR_TOURNIQUET ? 50 : treatment[j->aspect & 3][(j->sev ? j->sev : 1) - 1][0];

    if (how == TR_DRESS || how == TR_SURGERY) {
        b = how == TR_SURGERY && !has ("knife") && !has ("dagger") ? 0 : b * best () / 9;
    }
    return b;
}

/* The part said (tpart: "arm", "left leg", "head", "chest" ...) and whether injury j is there (none said: any) */
static unsigned char __fastcall__ at_part (const harn_injury* j)
{
    unsigned char l = HARN_LOC (j->loc), s;

    if (!tpart[0] || strstr (tpart, "wound") || strstr (tpart, "self") || strstr (tpart, "bandage") ||
        strstr (tpart, "dressing")) {
        return 1;
    }
    s = !strstr (tpart, "left") && !strstr (tpart, "right") ? 1 : (strstr (tpart, "right") != 0) == ((j->loc &
        HARN_RIGHT) != 0);
    return s && (strstr (tpart, harn_location_name (j->loc)) || (strstr (tpart, "arm") && l >= HARN_SHOULDER &&
        l <= HARN_HAND) || (strstr (tpart, "leg") && l >= HARN_HIP) || (strstr (tpart, "head") && l <= HARN_FACE) ||
        (strstr (tpart, "body") && l >= HARN_THORAX && l <= HARN_GROIN) || (strstr (tpart, "chest") && l ==
        HARN_THORAX) || (strstr (tpart, "belly") && l == HARN_ABDOMEN) || (strstr (tpart, "stomach") && l ==
        HARN_ABDOMEN));
}

/* The injury of c's to treat first: a bleeder; else the worst untreated (not the lesser hurts, nor a venom's ache), at
** the part said; 0xFF none.  A dressing changed (ch): the character's own wound at the part said that's tended and
** wants a dressing (a dressing's, surgery's), one dressed already first */
static unsigned char __fastcall__ to_treat (const harn_char* c, unsigned char ch)
{
    unsigned char k, w = 0xFF;
    const harn_injury* j;

    for (k = 0; k < c->ninj; ++k) {
        j = &c->inj[k];
        if (!at_part (j) || (j->flags & IF_POOL) || (c == &pc && pc_by[k] == BY_VENOM)) {
            continue;
        }
        if (ch) {
            if ((j->flags & HARN_IF_TREATED) && !(j->flags & HARN_IF_BLEEDER) && (remedy (j) == TR_DRESS || remedy (j)
                == TR_SURGERY) && (w == 0xFF || (pc_dr[k] && !pc_dr[w]))) {
                w = k;
            }
            continue;
        }
        if (j->flags & HARN_IF_BLEEDER) {
            return k;
        }
        if (!(j->flags & HARN_IF_TREATED) && (w == 0xFF || j->sev > c->inj[w].sev || (j->sev ==
            c->inj[w].sev && j->il > c->inj[w].il))) {
            w = k;
        }
    }
    return w;
}

static const char* const begun[] = {
    "You bind a tourniquet tight above ", "You clean and dress ", "You hold a cold compress to ",
    "You set and splint ", "You clean and stitch ", "You unwind the old dressing from "
};

/* The bandage the dressing took, a line after the words: its key and its grade ("\nbandage7": the world's to use up a
** lot of that grade), the old dressing kept, bloody (old: "+") */
static void __fastcall__ used (unsigned char d, unsigned char old)
{
    char b[3];

    b[0] = '0' + d;
    b[1] = old ? '+' : 0;
    b[2] = 0;
    put ("\nbandage");
    put (b);
}

/* RS_TREAT (r a copy): a 0, planned: the injury (b), the treatment (c), its time (w1 tenths: a tourniquet a minute,
** a dressing five minutes a level, a compress or a splint 5d6 minutes, surgery 10d6, a dressing changed five), its
** first words; result 1 nothing to treat, 2 too grave to treat itself (a grievous wound but a bleeder's tourniquet,
** Physician 2: the house's for the bleeder), 3 the patient isn't one (no sheet).  a 1, done (b, c the plan's, x[0] its
** place): the Treatment roll (Physician EML, the bonus as to hand: test_sk's, its own penalties), its words; a dressing
** changed, none (the fresh one on); result 1 a bleeder still bleeding (a minute more, the book's), 2 the plan gone
** (healed, changed); the text's second line, after a newline, the key of what it used up ("bandage7": a dressing's,
** surgery's, a change's, its grade; "bandage7+", the old one kept).  The part said a * first: a dressing changed */
void __fastcall__ o_treat (struct req* r)
{
    harn_char* c = &pc;
    harn_injury* j;
    unsigned char k, how, lv, u = 0, ch = 0, d;
    signed char g;
    unsigned t;

    if (*tpart == '*') {
        for (k = 0; (tpart[k] = tpart[k + 1]) != 0; ++k) {
        }                                       /* (Not memmove: the main RAM's room) */
        ch = 1;
    }
    if (r->other) {
        u = aden[(r->other - 1) & 7];
        sheet_in (u);
        if (!bs.kind) {
            RS_TREAT_CANT (*r) = 3;
            return;
        }
        if (ch) {
            RS_TREAT_CANT (*r) = 1;
            put (beast_flags (bs.kind) & BF_THEY ? "You keep no dressings on them." : "You keep no dressings on it.");
            return;
        }
        c = &cr;
    }
    if (!RS_TREAT_DONE (*r)) {
        if ((k = to_treat (c, ch)) == 0xFF) {
            RS_TREAT_CANT (*r) = 1;
            put (ch ? (tpart[0] ? "There's no dressing there to change." : "You have no dressings to change.") :
                tpart[0] && !strstr (tpart, "wound") ? "There's nothing there that wants tending." : c == &pc ?
                "You have no wounds that want tending." : beast_flags (bs.kind) & BF_THEY ?
                "They have no wounds that want tending." : "It has no wounds that want tending.");   /* (A person "they",
                                                **   2026-11-17) */
            return;
        }
        j = &c->inj[k];
        how = ch ? TR_CHANGE : remedy (j);
        if (ch && !best ()) {
            RS_TREAT_CANT (*r) = 1;
            put ("You have no clean bandage to put on it.");
            return;
        }
        if (c == &pc && j->sev >= HARN_GRIEVOUS && how != TR_TOURNIQUET && !ch) {
            RS_TREAT_CANT (*r) = 2;
            put ("You can't treat a wound so grave yourself.");
            return;
        }
        t = how == TR_TOURNIQUET ? 600 : how == TR_CHANGE ? 3000 : how == TR_DRESS ? 3000 * j->il : how == TR_SURGERY ?
            600 * dice (10, 6) : 600 * dice (5, 6);
        RS_TREAT_WOUND (*r) = k;
        RS_TREAT_HOW (*r) = how;
        RS_TREAT_TENTHS (*r) = t;
        RS_TREAT_LOC (*r) = j->loc;
        put (how == TR_CHANGE && !pc_dr[k] ? "You dress " : begun[how]);
        put (wname (c, k));
        put (".");
        return;
    }
    k = RS_TREAT_WOUND (*r);
    how = RS_TREAT_HOW (*r);
    if (k >= c->ninj || c->inj[k].loc != RS_TREAT_LOC (*r) || (how == TR_TOURNIQUET) != ((c->inj[k].flags & HARN_IF_BLEEDER) !=
        0)) {
        RS_TREAT_UNDONE (*r) = 2;
        return;
    }
    j = &c->inj[k];
    d = best ();
    if (how == TR_CHANGE) {                     /* (A dressing changed: no treatment roll, the fresh one on) */
        if (!d) {
            put ("You have no clean bandage to put on it.");
            return;
        }
        put ("The fresh dressing is on.");
        used (d, pc_dr[k]);
        pc_dr[k] = d + 1 | bmat;                /* (Its cloth too: the sheet's) */
        return;
    }
    g = test_sk (&pc, HARN_PHY, bonus (j, how) / STEP_PCT);
    lv = g == G_BADLY ? HARN_CF : g == G_FAIL ? HARN_MF : g == G_PASS ? HARN_MS : HARN_CS;
    if (how == TR_TOURNIQUET) {
        if (HARN_SUCCESS (lv)) {
            j->flags &= ~HARN_IF_BLEEDER;
            put ("The bleeding stops.");
        } else {
            RS_TREAT_UNDONE (*r) = 1;
            put ("The blood still flows.");
        }
    } else {
        t = (unsigned) treatment[j->aspect & 3][(j->sev ? j->sev : 1) - 1][2 + lv];
        j->hr = t & 7;
        j->flags |= HARN_IF_TREATED | (t & IMP ? HARN_IF_IMPAIR : 0) | (j->hr == EE ? HARN_IF_EE : 0);
        put (j->hr == EE ? "It's well done: it should mend within the day." : j->hr >= 6 ? "It's well tended." :
            j->hr == 5 ? "It's tended." : j->hr == 4 ? "It's tended, roughly." : "It's badly done.");
        if ((how == TR_DRESS || how == TR_SURGERY) && d) {
            used (d, 0);                        /* (A bandage on it, its dressing: kept, the character's) */
            if (c == &pc) {
                pc_dr[k] = d + 1 | bmat;
            }
        }
    }
    if (c != &pc) {
        sheet_out (u);
    }
}

#define DRESS_LINE      o_dress_line
#include "dressln.h"                            /* (The character's file's "dressed" line: dq0acct's too) */
