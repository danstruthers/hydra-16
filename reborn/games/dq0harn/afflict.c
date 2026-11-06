/*
** afflict.c - (dq0harn, overlay 8) the character's afflictions (2026-10-31: HOUSE.md's poison, bad air and disease):
** the book's way with them (Physician 4: a poison an infection whose intervals are shortened to the toxin, a disease a
** Contagion Index and a healing rate, its recovery rolled daily on the Infection table, gone at H6, dead at H0; the
** HarnWorld Bestiary's venoms, its grave-wight's grave rot), one course for each, two at once: a toxin (a venom, a
** poison: pv) and a disease (pd); and the air it breathes (rsair.h's, every ruleset's).  Its symptoms the book's special
** penalty to physical skills, as fatigue levels held (afffl: in the universal penalty, every test's; not rested off
** till it's over, as the want's aren't).  Moved from heal.c's overlay 7 (its venoms, the status's words, the save's line)
** with the venom's course and dice as they were.
**
** The house's (HOUSE.md, for the user's review): the kinds, their rates, intervals and symptoms below; a disease
** caught told only as it shows (its incubation's hours on); the dose of a poison its rate (1 slight H5 ... 3 strong
** H3); spoiled food never kills (held at H1); the poisons' cure an antidote's (a remedy's hook: aff_cure).
*/

#pragma code-name ("OV8CODE")
#pragma rodata-name ("OV8RODATA")
#pragma local-strings (on)

#include <stdlib.h>
#include <string.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

extern unsigned long xs_seed;
extern unsigned long vseed;

static void vdice5 (void)                       /* (heal.c's vdice, overlay 8's own: the body's own dice) */
{
    unsigned long t = xs_seed;

    xs_seed = vseed ? vseed : t ^ 0x56454E4FUL;
    vseed = t;
}

static void __fastcall__ nput (const char* s)  /* (heal.c's nput, overlay 8's own) */
{
    unsigned char n = strlen (news);

    if (n + strlen (s) >= NEWS_N - 1) {
        n = 0;                                  /* (Full, its words not told yet: the latest kept) */
    }
    strcpy (news + n, s);
}

#include "course.h"

struct venom pv;                                /* (The character's toxin: dq0harn.h's) */
struct sick pd;                                 /* (Its disease) */
unsigned char afffl;                            /* (Its symptoms' and its breath's fatigue levels, held) */
unsigned char aff_breath;                       /* (Its breath's, rsair.h's: among them) */

#define AIR_SAY(s)      nput (s)
#define AIR_LEVELS(n)   (aff_breath = (n))
#include "../dq0/rsair.h"

/* A disease's exposure (kind k, its index less d steps the worse: a dose): the book's test, its Contagion Index times
** the Endurance (95 at most), failed caught; nothing told (it shows on its incubation's end) */
static void __fastcall__ expose (unsigned char k, unsigned char d)
{
    int t = (int) (venoms[k].ci > d ? venoms[k].ci - d : 1) * harn_end (&pc);

    if (pd.hr || HARN_SUCCESS (harn_test (t > 95 ? 95 : t))) {
        return;
    }
    pd.kind = k;
    pd.hr = venoms[k].hr;
    pd.inc = venoms[k].inc * 60;
    pd.left = 0;
}

/* A toxin taken (kind k, its rate hr): the worse kept if it's poisoned already (its course as it was, the house's);
** else its first roll at once, its words */
static unsigned char __fastcall__ take (unsigned char k, unsigned char hr, unsigned char loc, unsigned char sw)
{
    static const char* const took[] = {
        " The bite burns: there's venom in it.", " The dart's poison burns in the wound.",
        " Your skin burns and tingles: there was poison on it.", " Your belly cramps: the food was bad."
    };

    if (pv.hr) {
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
    nput (took[k < VK_DART ? 0 : k - VK_DART + 1]);
    if (sw) {
        vdice5 ();                              /* (Its first roll the body's own dice, as the venom's was) */
    }
    venom_roll ();
    if (sw) {
        vdice5 ();
    }
    levels ();
    return 1;
}

/* A venomous or sickly creature's bite or claws at the character (fight.c's strike: key its kind's, the bestiary's;
** at loc, its effective impact): a venom in it, unless a minor bite's too weak to carry it (its rate a point more for
** each point of impact under 5, over 5 none: the bestiary's option), and its first roll at once, 1 if it's told
** (news); a disease's exposure (a rat's, a ghoul's: drawing blood), nothing told */
unsigned char __fastcall__ o_envenom (const char* key, unsigned char loc, int impact)
{
    unsigned char k, hr, t = 0;

    for (k = 0; k < NVENOMS && strcmp (venoms[k].key, key); ++k) {
    }
    if (k == NVENOMS || pc.state == HARN_DEAD) {
        return 0;
    }
    if (venoms[k].how & V_SICK) {
        if (!pd.hr) {
            vdice5 ();                          /* (The body's own dice: a fight's roll as they did) */
            expose (k, 0);
            vdice5 ();
        }
    } else {
        hr = venoms[k].hr + (impact < 5 ? 5 - impact : 0);
        if (hr <= 5) {
            if ((venoms[k].how & V_BLEED) && pc.ninj) {
                pc.inj[pc.ninj - 1].flags |= HARN_IF_BLEEDER;  /* (The adder's: its bite a B1 bleeder) */
            }
            t = take (k, hr, loc, 1);
        }
    }
    return t;
}

/* RS_TIME's for the character (heal.c's body_time, r a copy): what it's exposed to now (x[18], its dose x[19]), the
** air it breathes (x[20], its flame x[21]: rsair.h's), its toxin's course for w1 minutes at what it's about (a: its
** interval at rest, half that about, a quarter at vigorous work, the bestiary's), its disease's (shown after its
** incubation, then daily); the grade's bits (2 news, 4 worse: a hard stop) */
signed char __fastcall__ test_sk (harn_char* c, unsigned char sk, signed char steps);    /* (Overlay 5's stub) */
static void __fastcall__ remedy (unsigned char k);
unsigned char __fastcall__ o_aff_cure (unsigned char power);
unsigned char __fastcall__ o_aff_time (struct req* r)
{
    static const unsigned char of[NAK] = { 0, VK_DART, VK_CONTACT, VK_FOOD, VK_FLUX };
    unsigned m = r->w1;
    unsigned char k = 0, d = 1, o = 0, n = strlen (news), q = 0, f = 0;

    if (r->actor == 0xFF) {                     /* (The world's: what it's exposed to, the air; the UI's, a drink) */
        k = r->x[18];
        d = r->x[19];
        q = r->x[20];
        f = r->x[21];
    } else if (r->x[15] & 0x80) {               /* (Still water drunk where it stands: foul, the flux's) */
        r->x[15] &= 0x7F;
        k = AK_FLUX;
    } else if (r->x[15] & RT_TAKEN) {           /* (A remedy taken, 2026-11-02: HOUSE.md's remedies) */
        remedy (r->x[15] & 3);
        r->x[15] = 0;
    }
    if (k && k < NAK) {                         /* (Exposed: a poison taken at once, a disease caught or not) */
        k = of[k];
        if (venoms[k].how & V_SICK) {
            expose (k, d ? d - 1 : 0);
        } else {
            take (k, venoms[k].hr + 2 - (d > 2 ? 2 : d), 0, 0);
        }
    }
    if (q || f || air_lv || air_co) {
        o = air_of (q, f, m);                   /* (The air: rsair.h's) */
        if (o & 1) {
            pc.state = HARN_DEAD;
        }
        o &= 6;
    }
    o |= aff_course (m, r->a);
    levels ();
    return o | (strlen (news) != n ? 2 : 0);
}

/* A remedy taken (RS_TIME's RT_TAKEN, the UI's: k 1 an antidote, against the toxin, its power 2; 2 a remedy, against
** the disease, its power 1), dosed by the taker's Physician (the book's, Physician 4: medicine combats an infection;
** the house's: a critical success a point the more, a critical failure a point the less), the rate the better by it;
** nothing for it, told so (HOUSE.md's remedies, 2026-11-02) */
static void __fastcall__ remedy (unsigned char k)
{
    signed char g = test_sk (&pc, HARN_PHY, 2);
    unsigned char p = (k == 1 ? 2 : 1) + (g == G_CS) - (g == G_CF);

    if (k == 1 ? !o_aff_cure (p) : !pd.hr || !p) {
        nput (k == 1 ? " It does nothing for you: there's no poison in you that it can reach." :
            " It does nothing for you.");
        return;
    }
    if (k == 2) {
        if ((pd.hr += p) >= 6) {
            pd.hr = 0;
            nput (" The remedy works: the sickness leaves you.");
        } else {
            nput (" The remedy eases the fever a little.");
        }
        levels ();
    }
}

/* A remedy's hook (remedy's: an antidote): the toxin's rate the better by its power (a theriac's 2), its words; 1 if it
** did anything */
unsigned char __fastcall__ o_aff_cure (unsigned char power)
{
    if (!pv.hr || pv.kind < VK_DART && venoms[pv.kind].how & V_NUMB && power < 2) {
        return 0;
    }
    pv.hr += power;
    if (pv.hr >= 6) {
        pv.hr = 0;
        nput (" The remedy works: the poison's hold on you passes.");
    } else {
        nput (" The remedy eases you a little.");
    }
    levels ();
    return 1;
}

/* Its tongues and scripts (tongues, dq0harn.h's: the plan for the game between dungeons, its step 8): its file's line l,
** "language NAME=ML" or "script NAME[=ML]", into it (a script's " *NAME=ML", a tongue's " NAME=ML"; any other line
** passed over); or its next line made in l from it, past its own tongue (tw: where; l[0] 0, no more, and from the
** first again) */
static unsigned char tw;

static void __fastcall__ tongue_in (char* l)
{
    unsigned char n = 0;
    const char* p;

    if (!strncmp (l, "language ", 9) || !strncmp (l, "script ", 7)) {
        l = *l == 's' ? (l[5] = ' ', l[6] = '*', l + 5) : l + 8;
        for (p = tongues + 1; (p = strchr (p, ' ')) != 0; ++p) {
            ++n;                                /* (Those past its own so far) */
        }
        if (n < TONGUE_LINES && strlen (tongues) + strlen (l) < sizeof tongues) {
            strcat (tongues, l);
        } else {
            tongues[0] = '!';                   /* (More than it can hold: the sheet refused, loudly, not cut short) */
        }
    }
}

static void __fastcall__ tongue_out (char* l)
{
    const char* p = tongues + (tw ? tw : 7);
    const char* e;
    unsigned char g;

    if (!*p) {
        tw = l[0] = 0;
        return;
    }
    e = strchr (p + 1, ' ');
    e = e ? e : p + strlen (p);
    g = p[1] == '*' ? 2 : 0;
    strcpy (l, g ? "script " : "language");
    strncat (l, p + g, e - p - g);
    tw = e - tongues;
}

/* The save's line of the body (the character's file's "body": its venom, its rest toward its healing in minutes, its
** disease, its breath, "body 3 14 0 25 1200 0 0 0 0 0 0"), into l (rd 0); or read back from l (rd 1: after "body ").
** And its skills' growth, "body u..." lines (skills.c's: rd 2 the next made, rd 3 cleared, rd 1 its u read), and after
** them its tongues' and scripts' lines (rd 2 on; rd 3 cleared, its own alone; rd 4 a line of the file's, l whole) */
void __fastcall__ o_body_line (char* l, unsigned char rd)
{
    char b[8];
    unsigned v[11];
    unsigned char k;

    if (rd == 4) {
        tongue_in (l);
        return;
    }
    if (rd == 2 && tw) {
        tongue_out (l);
        return;
    }
    if (rd >= 2 || (rd && *l == 'u')) {
        if (rd == 3) {
            tongues[7] = tw = 0;
            tongues[0] = ' ';                   /* (Not refused: tongue_in's) */
        }
        skill_line (l, rd);
        if (rd == 2 && !l[0]) {
            tongue_out (l);
        }
        return;
    }
    if (!rd) {
        v[0] = pv.hr;
        v[1] = pv.loc;
        v[2] = pv.kind;
        v[3] = pv.left;
        v[4] = rest_m;
        v[5] = pd.hr;
        v[6] = pd.kind;
        v[7] = pd.left;
        v[8] = pd.inc;
        v[9] = air_co;
        v[10] = afffl;
        strcpy (l, "body");
        for (k = 0; k < 11; ++k) {
            strcat (l, " ");
            strcat (l, utoa (v[k], b, 10));
        }
        return;
    }
    memset (v, 0, sizeof v);
    for (k = 0; l && k < 11; ++k) {
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
    pd.hr = v[5];
    pd.kind = v[6] < NVENOMS && (venoms[v[6]].how & V_SICK) ? v[6] : VK_FLUX;
    pd.left = v[7];
    pd.inc = v[8];
    air_co = v[9];
    afffl = v[10];                              /* (In its fatigue already, the sheet's) */
    aff_breath = air_lv = 0;
}

/* The status's (sheet.c's o_state's): the venom or poison in it, its sickness (shown), its breath, the blood it's lost,
** a wound treated (HOUSE.md's wounds over time) */
void o_body_state (void)
{
    unsigned char k, n = 0;

    for (k = 0; k < pc.ninj; ++k) {
        n += (pc.inj[k].flags & HARN_IF_TREATED) != 0;
    }
    if (n) {
        put (n == pc.ninj ? "; all tended" : "; some tended");
    }
    if (pv.hr && pv.kind < VK_DART) {
        put ("; poisoned, the venom in your ");
        put (place (pv.loc));
    } else if (pv.hr) {
        put ("; poisoned");
    }
    if (pd.hr && !pd.inc) {
        put ("; feverish");
    }
    if (aff_breath) {
        put (aff_breath >= 6 ? "; overcome by the foul air" : "; short of breath");
    } else if (air_co >= 30) {
        put ("; your head aching");
    }
    if (pc.bl) {
        put (2 * pc.bl > harn_end (&pc) ? "; faint from the loss of blood" : "; weak from the loss of blood");
    }
}
