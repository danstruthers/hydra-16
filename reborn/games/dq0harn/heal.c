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

/* A wound's words, "the minor cut to your left forearm (M1)" (sheet.c's, the "a" made "the"), of injury k of c's (its
** words dress.c's too) */
char ww[56];
static const char* __fastcall__ wname (const harn_char* c, unsigned char k)
{
    const harn_injury* j = &c->inj[k];

    wound_words (ww, j->sev, j->loc, j->aspect, j->il, c == &pc ? pc_by[k] : BY_ITS);  /* (The character's alone: its
                                                **   callers'; a creature's wounds told nowhere here) */
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
        pc_dr[k] = pc_dr[k + 1];                /* (Its dressing gone with it: dress.c's) */
    }
    pc_by[k] = pc_dr[k] = 0;
}

/* The healing roll's bonus by the dressing on the character's wound k (HOUSE.md's bandages, 2026-11-10): a clean one
** (its grade 7-9) +5, a fair one none, a soiled one (0-3) -10, the infection the likelier; none, none */
static signed char __fastcall__ dressed (unsigned char k)
{
    unsigned char d = pc_dr[k] & DR_GRADE;

    return !d ? 0 : d >= 8 ? 5 : d >= 5 ? 0 : -10;
}

/* A day's end: each dressing on a serious or grievous wound of the character's soiled, three grades the worse (its
** grade plus 1 kept: 1 the filthiest; DR_SOILED, the sheet's word), told as it's soiled (HOUSE.md's bandages: it
** wants changing) */
static void soil (void)
{
    unsigned char k, d;

    for (k = 0; k < pc.ninj; ++k) {
        if ((d = pc_dr[k] & DR_GRADE) != 0 && pc.inj[k].sev >= HARN_SERIOUS && !(pc.inj[k].flags & IF_POOL)) {
            pc_dr[k] = DR_SOIL (pc_dr[k]);      /* (Its cloth kept: the sheet's) */
            if (d > 4 && d <= 7) {
                nput (" The dressing on ");
                nput (wname (&pc, k));
                nput (" is soiled: it wants changing.");
            }
        }
    }
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
    h = harn_heal_roll (c, k, c == &pc ? dressed (k) : 0);  /* (Its dressing's: HOUSE.md's bandages) */
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

#include "watch.h"

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
    unsigned m = RS_TIME_MINUTES (*r);
    unsigned char k, dead = 0, bound = 0, n = strlen (news), ninj = pc.ninj, u, ag = 0;

    out_t = 0;
    if (RS_TIME_DROWNED (*r) == 3 && pc.state != HARN_DEAD) {    /* (Drowned: the world's, slice 4.9's) */
        pc.state = HARN_DEAD;
        nput (" You drown.");
    }
    if ((!r->actor || RS_TIME_PC (*r)) && pc.state != HARN_DEAD) {
        if (!RS_TIME_ABOUT (*r)) {
            rest_m = rest_m + m < rest_m ? 0xFFFF : rest_m + m;    /* (At rest: toward its healing rolls, */
            develop (m);                        /*   and its skills' development rolls: slice 4.4's, skills.c) */
        }
        bleed (&pc, m);
        if (pc.state != HARN_DEAD) {
            ag = aff_time (r);                  /* (Its afflictions' course: a venom's interval at rest, half
                                                **   that about, a quarter at vigorous work; a disease's, a poison's,
                                                **   the air it breathes: overlay 8's, afflict.c) */
        }
    }
    for (k = 0; k < NACTORS; ++k) {
        if (RS_TIME_BLEEDERS (*r) & 1 << k) {
            u = aden[k];
            sheet_in (u);
            if (bleed (&cr, m)) {
                dead |= 1 << k;
            } else if (!(RS_TIME_FIGHTING (*r) & 1 << k) && cr.state == HARN_CONSCIOUS && bs.kind &&
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
            if (!(wcount % 6)) {
                soil ();                        /* (A day's end: its dressings soiled, after its rolls) */
            }
        }
        sheets_watch (wcount);
    }
    memcpy (&RS_TIME_NEXT (*r), &wnext, 4);              /* (When it's next to be asked, nothing bleeding: the world's) */
    RS_TIME_DEAD (*r) = dead;
    RS_TIME_BOUND (*r) = bound;
    RS_TIME_OUT (*r) = out_t;
    RS_TIME_HEALTH (*r) = health_of (&pc);
    RS_TIME_DIED (*r) = pc.state == HARN_DEAD;
    RS_TIME_NEWS (*r) = (strlen (news) != n || fresh ? 2 : 0) | (pc.ninj > ninj ? 4 : 0) | ag;   /* (A skill opened as it
                                                **   was used, news since: told now, slice 4.4's) */
    fresh = 0;
}

/* ---- First aid and treatment (RS_TREAT): overlay 9's since 2026-11-10 (dress.c: the dressings', HOUSE.md's bandages;
** this overlay's room) */

void __fastcall__ treat (struct req* r);        /* (Overlay 9's: its stub) */

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

