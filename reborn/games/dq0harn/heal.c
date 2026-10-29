/*
** heal.c - (overlay 6, the referee's: called through bank.s's stubs) wounds over time (slice 4.3): the book's
** bleeding, a venom's course, shock's recovery and the healing of wounds as time passes (RS_TIME), a wound treated
** (RS_TREAT: first aid, the Treatment table), for the character and for every creature's sheet alike (Combat 14,
** Physician 2-4; the HarnWorld Bestiary's venomous bites).  The library's healing (sdk/c/lib/harnheal.c: the Treatment
** table, the five-day healing roll, the infection's daily roll, the shock's four-hour roll) compiled here, into the
** overlay, so none of it is in the main RAM.
**
** Time is told in minutes (RS_TIME's w1), with what the character was about (RS_TIME's a: at rest, about, or at
** vigorous work), and the clock (tnow): its marks the book's, a watch (four hours: the shock's roll), a day (the
** infection's roll, a wound treated EE healed), five days (the healing rolls, the blood got back), each made as the
** clock passes it, for the character and every sheet.  What came of it for the character is told in words (news,
** RS_DESCRIBE b 4's, the UI's to say as it can: E_TASK's TE_BODY, the world's), a line each.
**
** The house's (HOUSE.md's wounds over time, for the user's review): the character's healing rolls want half the five
** days' rest (the book's twelve hours a day: its waits, its sleep, its standing still, RS_EXERT's GA_REST), else
** forfeit; the creatures heal as they rest in their lairs, every roll made, but no infection; a bleeder staunched with
** a tourniquet by the character on itself (the book lets it treat only its minor and serious wounds), by anything to
** hand (a belt, a strip of cloth: the +50 always); the treatments' bonuses by what's to hand (bandages for a dressing,
** a knife and bandages for surgery, no needle and thread in the game yet; a compress's water and a splint's stick
** taken as found); a folk creature binds its own bleeding as a minute passes while it isn't fighting.
*/

#pragma code-name ("OV6CODE")
#pragma rodata-name ("OV6RODATA")
#pragma local-strings (on)                      /* (A function's strings beside it: overlay 7's too) */

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

#include "../../sdk/c/lib/harnheal.c"

signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps);    /* (Overlay 5's) */

/* Words onto news (the main RAM's, told as the UI asks: RS_DESCRIBE b 4) */
static void __fastcall__ nput (const char* s)
{
    unsigned char n = strlen (news);

    if (n + strlen (s) >= NEWS_N - 1) {
        n = 0;                                  /* (Full, its words not told yet: the latest kept) */
    }
    strcpy (news + n, s);
}

/* A wound's words, "the minor cut to your left forearm (M1)" (sheet.c's, the "a" made "the"), of injury k of c's */
static char ww[56];
static const char* __fastcall__ wname (const harn_char* c, unsigned char k)
{
    const harn_injury* j = &c->inj[k];

    wound_words (ww, j->sev, j->loc, j->aspect, j->il, c == &pc ? pc_by[k] : BY_ITS);
    k = strlen (ww) + 1;
    do {
        ww[k + 2] = ww[k];
    } while (k--);
    ww[0] = 't';                                /* ("a minor" ... "the minor"; "an" none: wound_words's "a ") */
    ww[1] = 'h';
    ww[2] = 'e';
    return ww;
}

/* ---- A creature's sheet (by its record's number) into fight.c's cr, and back (as fight.c's unpack and pack) */

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

/* ---- Time passed (RS_TIME) */

unsigned out_t;                                 /* (Knocked senseless by a venom's ache: the tenths, RS_TIME's w2) */

unsigned rest_m;                                /* (The character's rest since its last healing roll, minutes: RS_TIME's
                                                **   at rest; */
static unsigned long wnext;                     /*   the clock's next watch; and the watches gone by) */
static unsigned wcount;
#define WATCH_T         144000UL                /* (Four hours, tenths: the book's watch) */
#define REST_ENOUGH     3600                    /* (Sixty hours: half of five days, the book's twelve hours a day) */


/* The character's wound k healed whole (EE, or its levels gone): told; its dealt kept in step (the library's drop
** moves the rest down) */
static void __fastcall__ healed (unsigned char k)
{
    nput (" ");
    wname (&pc, k);
    ww[0] = 'T';
    nput (ww);
    nput (" has healed.");
}

static void __fastcall__ by_drop (unsigned char k)
{
    for (; k < HARN_INJMAX - 1; ++k) {
        pc_by[k] = pc_by[k + 1];
    }
    pc_by[k] = 0;
}

/* A healing roll of c's injury k (the five-day roll, or an infection's daily one: the library's harn_heal_roll; a
** creature never infected, the house's); 1 if it healed whole.  The character's told */
static unsigned char __fastcall__ heal_one (harn_char* c, unsigned char k)
{
    harn_injury* j = &c->inj[k];
    unsigned char f = j->flags, il = j->il, n = c->ninj, h;

    if (c == &pc) {
        wname (c, k);                           /* (Its words before: it may be gone after) */
        ww[0] = 'T';
    }
    h = harn_heal_roll (c, k, 0);
    if (c != &pc) {
        if (c->ninj == n) {
            j->flags &= ~HARN_IF_INFECTED;      /* (No infection for creatures: the house's) */
        }
        return h;
    }
    if (c->ninj < n) {
        by_drop (k);
        nput (" ");
        nput (ww);
        nput (" has healed.");
        return 1;
    }
    if (c->state == HARN_DEAD) {
        nput (" The infection kills you.");
    } else if (!(f & HARN_IF_INFECTED) && (j->flags & HARN_IF_INFECTED)) {
        nput (" ");
        nput (ww);
        nput (" is hot and swollen: it's infected.");
    } else if ((f & HARN_IF_INFECTED) && !(j->flags & HARN_IF_INFECTED)) {
        nput (" The infection in ");
        ww[0] = 't';
        nput (ww);
        nput (" is gone.");
    } else if (!(j->flags & HARN_IF_INFECTED)) {
        nput (" ");
        nput (ww);
        nput (j->il < il ? " is mending." : " is no better.");    /* (Its roll failed: told, the house's) */
    }
    return 0;
}

/* A watch passed (the clock's n'th): c's shock's roll (the book's, Physician 4: each watch; a creature's never its
** death, the house's: it lies in shock till it's over it), and at a day's end each wound treated EE healed and each
** infected one's roll, at five days' each other one's healing roll and its blood got back (Physician 4: the Sindarin
** two rolls) */
static void __fastcall__ watch_of (harn_char* c, unsigned n)
{
    unsigned char k, s = c->state;

    if (s == HARN_DEAD) {
        return;
    }
    if (s == HARN_SHOCK) {
        harn_shock_recovery (c, 0);
        if (c == &pc) {
            nput (c->state == HARN_DEAD ? " You die of shock." : c->state == HARN_CONSCIOUS ? " You come out of shock." :
                "");
        } else if (c->state == HARN_DEAD) {
            c->state = HARN_SHOCK;              /* (A creature's: the house's) */
            c->shockhr = 1;
        }
    }
    if (n % 6) {
        return;
    }
    for (k = c->ninj; k--; ) {
        if ((c->inj[k].flags & HARN_IF_INFECTED) || c->inj[k].hr == EE) {
            heal_one (c, k);
        }
    }
    if (n % 30 || (c == &pc && rest_m < REST_ENOUGH)) {
        if (c == &pc && !(n % 30)) {
            rest_m = 0;                         /* (Its rest too little: the days forfeit, the book's) */
        }
        return;
    }
    if (c == &pc) {
        rest_m = 0;
    }
    harn_regen_blood (c);
    for (k = c->ninj; k--; ) {
        if (!(c->inj[k].flags & HARN_IF_INFECTED) && c->state != HARN_DEAD &&
            !heal_one (c, k) && c->att[HARN_SP] == HARN_SIN && c == &pc) {
            heal_one (c, k);                    /* (The Sindarin's second roll) */
        }
    }
}

/* Each sheet's watch (by its record's number) */
static void __fastcall__ sheets_watch (unsigned n)
{
    unsigned char k = 0;

    do {
        sheet_get (k);
        if (bs.kind && (bs.ninj || (bs.state & 3) == HARN_SHOCK || bs.bl)) {
            sheet_in (k);
            watch_of (&cr, n);
            sheet_out (k);
        }
    } while (++k);
}

/* c's bleeders' bloodloss for m minutes (Combat 14: a point a minute each, dead past its Endurance); 1 dead of it */
static unsigned char __fastcall__ bleed (harn_char* c, unsigned m)
{
    unsigned char k, n = 0, e = harn_end (c), b = c->bl;
    unsigned v;

    for (k = 0; k < c->ninj; ++k) {
        n += (c->inj[k].flags & HARN_IF_BLEEDER) != 0;
    }
    if (!n || c->state == HARN_DEAD) {
        return 0;
    }
    v = c->bl + n * (m > 255 ? 255 : m);
    c->bl = v > 255 ? 255 : v;
    if (c->bl > e) {
        c->state = HARN_DEAD;
        if (c == &pc) {
            nput (" You bleed to death.");
        }
        return 1;
    }
    if (c == &pc && 2 * b <= e && 2 * c->bl > e) {
        nput (" You feel faint from the loss of blood.");
    }
    return 0;
}

/* A folk creature's bleeding bound (a tourniquet: Physician +50, the house's layman's 30 its Physician, its injuries'
** penalty off it), a minute's try; 1 if it's stopped */
static unsigned char __fastcall__ binds (harn_char* c)
{
    unsigned char k;

    for (k = 0; k < c->ninj; ++k) {
        if ((c->inj[k].flags & HARN_IF_BLEEDER) && HARN_SUCCESS (harn_test (80 - 5 * harn_get_up (c)))) {
            c->inj[k].flags &= ~HARN_IF_BLEEDER;
            return 1;
        }
    }
    return 0;
}

/* RS_TIME (r a copy): w1 minutes passed; the character's (b 1) at a (0 at rest, 1 about, 2 at vigorous work: a
** venom's interval counted at the full, at half, at a quarter, the bestiary's), each actor in w2's bits bleeding (x[0]
** those fighting); the clock's watches passed since the last.  The reply: w1 the actors dead of it, c those whose
** bleeding's bound, result 1 the character dead, 2 news of it (its words: RS_DESCRIBE b 4), 4 hurt anew (a venom's
** ache: a hard stop), w2 the tenths it's senseless (0) */
static void __fastcall__ body_time (struct req* r)
{
    unsigned m = r->w1;
    unsigned char k, dead = 0, bound = 0, n = strlen (news), ninj = pc.ninj, u;

    out_t = 0;
    if ((!r->actor || r->b) && pc.state != HARN_DEAD) {
        if (!r->a) {
            rest_m = rest_m + m < rest_m ? 0xFFFF : rest_m + m;    /* (At rest: toward its healing rolls) */
        }
        bleed (&pc, m);
        if (pv.hr && pc.state != HARN_DEAD) {
            venom_time (m * (r->a == 2 ? 4 : r->a ? 2 : 1));    /* (Its venom's course: its interval at rest, half
                                                **   that about, a quarter at vigorous work: overlay 7's) */
        }
    }
    for (k = 0; k < NACTORS; ++k) {
        if (r->w2 & 1 << k) {
            u = aden[k];
            sheet_in (u);
            if (bleed (&cr, m)) {
                dead |= 1 << k;
            } else if (!(r->x[0] & 1 << k) && cr.state == HARN_CONSCIOUS && bs.kind &&
                (beast_flags (bs.kind) & BF_FOLK) && binds (&cr)) {
                bound |= 1 << k;
            }
            sheet_out (u);
        }
    }
    if (!wnext || wnext > tnow + WATCH_T) {
        wnext = tnow + WATCH_T;                 /* (The first: four hours on from now) */
    }
    while (tnow >= wnext) {
        wnext += WATCH_T;
        ++wcount;
        if (pc.state != HARN_DEAD) {
            watch_of (&pc, wcount);
        }
        sheets_watch (wcount);
    }
    memcpy (r->x + 4, &wnext, 4);              /* (When it's next to be asked, nothing bleeding: the world's) */
    r->w1 = dead;
    r->c = bound;
    r->w2 = out_t;
    r->b = health_of (&pc);
    r->result = pc.state == HARN_DEAD;
    r->grade = (strlen (news) != n ? 2 : 0) | (pc.ninj > ninj ? 4 : 0);
}

/* ---- First aid and treatment (RS_TREAT: Physician 2-3, the Treatment table): the character the healer, the patient
** itself (other 0) or an actor (its entry plus 1: its sheet's) */

enum { TR_TOURNIQUET, TR_DRESS, TR_COMPRESS, TR_SPLINT, TR_SURGERY };

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

/* Its bonus (the table's EML modifier, the tourniquet's +50), by what's to hand (the house's: bandages for a dressing,
** a knife or a dagger and bandages for surgery; none of them, no bonus) */
static int __fastcall__ bonus (const harn_injury* j, unsigned char how)
{
    int b = how == TR_TOURNIQUET ? 50 : treatment[j->aspect & 3][(j->sev ? j->sev : 1) - 1][0];

    if ((how == TR_DRESS && !has ("bandage")) ||
        (how == TR_SURGERY && (!has ("bandage") || (!has ("knife") && !has ("dagger"))))) {
        b = 0;
    }
    return b;
}

/* The part said (tpart: "arm", "left leg", "head", "chest" ...) and whether injury j is there (none said: any) */
static unsigned char __fastcall__ at_part (const harn_injury* j)
{
    unsigned char l = HARN_LOC (j->loc), s;

    if (!tpart[0] || strstr (tpart, "wound") || strstr (tpart, "self")) {
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
** the part said; 0xFF none */
static unsigned char __fastcall__ to_treat (const harn_char* c)
{
    unsigned char k, best = 0xFF;
    const harn_injury* j;

    for (k = 0; k < c->ninj; ++k) {
        j = &c->inj[k];
        if (!at_part (j) || (j->flags & IF_POOL) || (c == &pc && pc_by[k] == BY_VENOM)) {
            continue;
        }
        if (j->flags & HARN_IF_BLEEDER) {
            return k;
        }
        if (!(j->flags & HARN_IF_TREATED) && (best == 0xFF || j->sev > c->inj[best].sev || (j->sev ==
            c->inj[best].sev && j->il > c->inj[best].il))) {
            best = k;
        }
    }
    return best;
}

static const char* const begun[] = {
    "You bind a tourniquet tight above ", "You clean and dress ", "You hold a cold compress to ",
    "You set and splint ", "You clean and stitch "
};

/* RS_TREAT (r a copy): a 0, planned: the injury (b), the treatment (c), its time (w1 tenths: a tourniquet a minute,
** a dressing five minutes a level, a compress or a splint 5d6 minutes, surgery 10d6), its first words; result 1
** nothing to treat, 2 too grave to treat itself (a grievous wound but a bleeder's tourniquet, Physician 2: the house's
** for the bleeder), 3 the patient isn't one (no sheet).  a 1, done (b, c the plan's, x[0] its place): the Treatment
** roll (Physician EML, the bonus as to hand: test_sk's, its own penalties), its words; result 1 a bleeder still
** bleeding (a minute more, the book's), 2 the plan gone (healed, changed); the text's second line, after a newline,
** the key of what it used up ("bandage": a dressing's, surgery's) */
static void __fastcall__ treat (struct req* r)
{
    harn_char* c = &pc;
    harn_injury* j;
    unsigned char k, how, lv, u = 0;
    signed char g;
    unsigned t;

    if (r->other) {
        u = aden[(r->other - 1) & 7];
        sheet_in (u);
        if (!bs.kind) {
            r->result = 3;
            return;
        }
        c = &cr;
    }
    if (!r->a) {
        if ((k = to_treat (c)) == 0xFF) {
            r->result = 1;
            put (tpart[0] && !strstr (tpart, "wound") ? "There's nothing there that wants tending." : c == &pc ?
                "You have no wounds that want tending." : "It has no wounds that want tending.");
            return;
        }
        j = &c->inj[k];
        how = remedy (j);
        if (c == &pc && j->sev >= HARN_GRIEVOUS && how != TR_TOURNIQUET) {
            r->result = 2;
            put ("You can't treat a wound so grave yourself.");
            return;
        }
        t = how == TR_TOURNIQUET ? 600 : how == TR_DRESS ? 3000 * j->il : how == TR_SURGERY ? 600 * dice (10, 6) :
            600 * dice (5, 6);
        r->b = k;
        r->c = how;
        r->w1 = t;
        r->x[0] = j->loc;
        put (begun[how]);
        put (wname (c, k));
        put (".");
        return;
    }
    k = r->b;
    how = r->c;
    if (k >= c->ninj || c->inj[k].loc != r->x[0] || (how == TR_TOURNIQUET) != ((c->inj[k].flags & HARN_IF_BLEEDER) !=
        0)) {
        r->result = 2;
        return;
    }
    j = &c->inj[k];
    g = test_sk (&pc, HARN_PHY, bonus (j, how) / STEP_PCT);
    lv = g == G_CF ? HARN_CF : g == G_MF ? HARN_MF : g == G_MS ? HARN_MS : HARN_CS;
    if (how == TR_TOURNIQUET) {
        if (HARN_SUCCESS (lv)) {
            j->flags &= ~HARN_IF_BLEEDER;
            put ("The bleeding stops.");
        } else {
            r->result = 1;
            put ("The blood still flows.");
        }
    } else {
        t = (unsigned) treatment[j->aspect & 3][(j->sev ? j->sev : 1) - 1][2 + lv];
        j->hr = t & 7;
        j->flags |= HARN_IF_TREATED | (t & IMP ? HARN_IF_IMPAIR : 0) | (j->hr == EE ? HARN_IF_EE : 0);
        put (j->hr == EE ? "It's well done: it should mend within the day." : j->hr >= 6 ? "It's well tended." :
            j->hr == 5 ? "It's tended." : j->hr == 4 ? "It's tended, roughly." : "It's badly done.");
        if (how == TR_DRESS || how == TR_SURGERY) {
            put (has ("bandage") ? "\nbandage" : "");
        }
    }
    if (c != &pc) {
        sheet_out (u);
    }
}

/* The body's own dice (as the bearings' are, rshouse.h's): the generator's state swapped for theirs about a time's
** work and a venom's rolls, before it and after, so a fight's, a test's, a walk's dice roll as they did whether a
** body bleeds, heals or is poisoned meanwhile or not (vseed: begun from the generator's as it is then) */
extern unsigned long xs_seed;
unsigned long vseed;

static void vdice (void)
{
    unsigned long t = xs_seed;

    xs_seed = vseed ? vseed : t ^ 0x56454E4FUL;
    vseed = t;
}

/* RS_HARM (overlay 1's, sheet.c's harm: its words, hurt, the reply's), RS_TIME, RS_TREAT (r a copy) */
void __fastcall__ o_body_req (struct req* r)
{
    char* t = tp;
    char* e = tend;

    switch (r->kind) {
    case RS_HARM:
        harm (r);
        tp = t;
        tend = e;
        put (hurt);
        break;
    case RS_TIME:
        vdice ();
        body_time (r);
        vdice ();
        break;
    default:
        treat (r);
        break;
    }
}

/* ==== Overlay 7's (its room: overlay 6 full, 5 the bestiary's): a venom's course, and the status's words of the body */
#pragma code-name (push, "OV7CODE")
#pragma rodata-name (push, "OV7RODATA")

static void vdice5 (void)                       /* (vdice's, overlay 7's own) */
{
    unsigned long t = xs_seed;

    xs_seed = vseed ? vseed : t ^ 0x56454E4FUL;
    vseed = t;
}

static void __fastcall__ nput5 (const char* s)  /* (nput's, overlay 7's own) */
{
    unsigned char n = strlen (news);

    if (n + strlen (s) >= NEWS_N - 1) {
        n = 0;                                  /* (Full, its words not told yet: the latest kept) */
    }
    strcpy (news + n, s);
}
#define nput    nput5

/* ---- Venom (the HarnWorld Bestiary's venomous bites and stings: an infection's course, Physician 4): the kinds that
** have one, its healing rate and its recovery interval (minutes, at rest), what it does as it spreads (V_ACHE an M1
** to each place it reaches, the adder's; V_NUMB each place made useless, its chest or belly reached its breath
** stopped, the lindworm's) and whether its bite bleeds (V_BLEED: the adder's B1 bleeder) */

#define V_ACHE          0x01
#define V_NUMB          0x02
#define V_BLEED         0x04
static const struct vkind {
    char            key[8];
    unsigned char   hr, mins, how;
} venoms[] = {
    { "adder", 3, 60, V_ACHE | V_BLEED },       /* (Snakes 2-3, p88: the heath adder's) */
    { "basilsk", 2, 10, 0 },                    /* (Dragons 13, p36) */
    { "lindwrm", 3, 10, V_NUMB },               /* (Dragons 14, p37) */
};
#define NVENOMS         (sizeof venoms / sizeof venoms[0])

struct venom pv;                                /* (The character's: dq0harn.h's) */

/* The places a venom spreads to, the next nearer the heart, a limb's side kept (the trunk's up to the skull, there
** at the last) */
static const unsigned char toward[HARN_NLOCATIONS] = {
    HARN_SKULL, HARN_SKULL, HARN_FACE, HARN_THORAX, HARN_SHOULDER, HARN_UPPERARM, HARN_ELBOW, HARN_FOREARM,
    HARN_NECK, HARN_THORAX, HARN_ABDOMEN, HARN_GROIN, HARN_HIP, HARN_THIGH, HARN_KNEE, HARN_CALF
};

static char lw[16];
static const char* __fastcall__ place (unsigned char loc)
{
    strcpy (lw, harn_is_limb (loc) ? (loc & HARN_RIGHT ? "right " : "left ") : "");
    strcat (lw, harn_location_name (loc));
    return lw;
}


/* The venom spread to the next place (the character's): its ache, or the place numb (its breath stopped, the chest or
** the belly: dead) */
static void spread (void)
{
    const struct vkind* v = &venoms[pv.kind];
    unsigned char l = pv.loc & 0x7F, w, k;

    l = toward[l] | (harn_is_limb (toward[l]) ? pv.loc & HARN_RIGHT : 0);
    if (l == pv.loc) {
        return;                                 /* (At the skull: nowhere further) */
    }
    pv.loc = l;
    nput (" It spreads to your ");
    nput (place (l));
    nput (".");
    if ((v->how & V_NUMB) && (HARN_LOC (l) == HARN_THORAX || HARN_LOC (l) == HARN_ABDOMEN)) {
        pc.state = HARN_DEAD;                   /* (Its breath stopped: the book's "likely dead of asphyxiation") */
        nput (" You can't breathe.");
        return;
    }
    if (v->how & (V_ACHE | V_NUMB)) {
        k = pc.state;
        w = wound (&pc, l, HARN_BLUNT, 1, BY_VENOM);   /* (Its M1: the book's for the adder; a numb place's too, its
                                                **   levels the house's; no senselessness of it, the house's) */
        pc.state = k;
        if ((w & HARN_W_INJURED) && pc.ninj) {
            pc.inj[pc.ninj - 1].il = 1;
            pc.inj[pc.ninj - 1].sev = HARN_MINOR;
            pc.inj[pc.ninj - 1].flags = v->how & V_NUMB && harn_is_limb (l) ? HARN_IF_LIMB : 0;
            if (v->how & V_NUMB) {
                nput (" It goes numb.");
            }
        }
    }
}

/* The venom's roll (the Infection table, Physician 4: its rate times the Endurance), its course: the worse, spread (a
** critical failure twice, the bestiary's option); the better; gone at H6 (its aches heal at H6, its numbness lifts),
** dead at H0 */
static void venom_roll (void)
{
    unsigned char r = harn_test ((int) pv.hr * harn_end (&pc)), k;
    signed char hr = (signed char) pv.hr + (r == HARN_CF ? -2 : r == HARN_MF ? -1 : r == HARN_MS ? 1 : 2);

    if (hr < (signed char) pv.hr) {
        nput (r == HARN_CF ? " The venom burns fiercely." : " The venom burns.");
        spread ();
        if (r == HARN_CF && pc.state != HARN_DEAD) {
            spread ();
        }
    }
    if (pc.state == HARN_DEAD) {
        return;
    }
    if (hr <= 0) {
        pc.state = HARN_DEAD;
        pv.hr = 0;
        nput (" The venom reaches your heart.");
        return;
    }
    if (hr >= 6) {
        pv.hr = 0;
        for (k = 0; k < pc.ninj; ++k) {
            if (pc_by[k] == BY_VENOM) {
                pc.inj[k].flags = HARN_IF_TREATED;
                pc.inj[k].hr = 6;               /* (Its aches heal at H6, the book's; numbness gone with it) */
            }
        }
        nput (" The venom's fire dies away: you've thrown it off.");
        return;
    }
    pv.hr = hr;
}

/* A venomous creature's bite at the character (fight.c's strike: key its kind's, the bestiary's; at loc, its effective
** impact): the venom in it, unless a minor bite's too weak to carry it (its rate a point more for each point of impact
** under 5, over 5 none: the bestiary's option), and its first roll at once; 1 if it's told (news) */
unsigned char __fastcall__ o_envenom (const char* key, unsigned char loc, int impact)
{
    unsigned char k, hr;

    for (k = 0; k < NVENOMS && strcmp (venoms[k].key, key); ++k) {
    }
    if (k == NVENOMS || pc.state == HARN_DEAD) {
        return 0;
    }
    hr = venoms[k].hr + (impact < 5 ? 5 - impact : 0);
    if (hr > 5) {
        return 0;
    }
    if ((venoms[k].how & V_BLEED) && pc.ninj) {
        pc.inj[pc.ninj - 1].flags |= HARN_IF_BLEEDER;  /* (The adder's: its bite a B1 bleeder) */
    }
    if (pv.hr) {                                /* (Bitten again: the worse kept, its course as it was, the house's) */
        if (hr < pv.hr) {
            pv.hr = hr;
            pv.kind = k;
        }
        return 0;
    }
    pv.hr = hr;
    pv.kind = k;
    pv.left = 0;
    pv.loc = loc;
    out_t = 0;
    nput (" The bite burns: there's venom in it.");
    vdice5 ();
    venom_roll ();
    vdice5 ();
    return 1;
}

/* The venom's course for m minutes at rest's (RS_TIME's: the body's time, its rolls at its interval) */
void __fastcall__ o_venom_time (unsigned m)
{
    const struct vkind* v = &venoms[pv.kind];

    m += pv.left;
    while (m >= v->mins && pv.hr && pc.state != HARN_DEAD) {
        m -= v->mins;
        venom_roll ();
    }
    pv.left = m;
}

/* The save's line of the body (the character's file's "body": its venom, its rest toward its healing in minutes,
** "body 3 14 0 25 1200"), into l (rd 0); or read back from l (rd 1: after "body ") */
void __fastcall__ o_body_line (char* l, unsigned char rd)
{
    char b[8];
    unsigned v[5];
    unsigned char k;

    if (!rd) {
        strcpy (l, "body");
        for (k = 0; k < 5; ++k) {
            strcat (l, " ");
            strcat (l, utoa (k == 0 ? pv.hr : k == 1 ? pv.loc : k == 2 ? pv.kind : k == 3 ? pv.left : rest_m, b, 10));
        }
        return;
    }
    memset (v, 0, sizeof v);
    for (k = 0; l && k < 5; ++k) {
        v[k] = atoi (l);
        if ((l = strchr (l, ' ')) != 0) {
            ++l;
        }
    }
    pv.hr = v[0];
    pv.loc = v[1];
    pv.kind = v[2] < NVENOMS ? v[2] : 0;
    pv.left = v[3];
    rest_m = v[4];
}

/* The status's (sheet.c's o_state's): the venom in it, the blood it's lost, a wound treated (HOUSE.md's wounds over
** time) */
void o_body_state (void)
{
    unsigned char k, n = 0;

    for (k = 0; k < pc.ninj; ++k) {
        n += (pc.inj[k].flags & HARN_IF_TREATED) != 0;
    }
    if (n) {
        put (n == pc.ninj ? "; all tended" : "; some tended");
    }
    if (pv.hr) {
        put ("; poisoned, the venom in your ");
        put (place (pv.loc));
    }
    if (pc.bl) {
        put (2 * pc.bl > harn_end (&pc) ? "; faint from the loss of blood" : "; weak from the loss of blood");
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
