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

/* ---- The kinds (the HarnWorld Bestiary's venomous bites, the house's poisons and diseases): their key (a venomous or
** a sickly creature's, the bestiary's; a poison's, a disease's own), healing rate, recovery interval (minutes, at
** rest), what a venom does as it spreads (V_ACHE an M1 to each place it reaches, the adder's; V_NUMB each place made
** useless, its chest or belly reached its breath stopped, the lindworm's; V_BLEED its bite a bleeder, the adder's),
** whether it's a disease (V_SICK: daily, caught by its Contagion Index, shown after its incubation) or never kills
** (V_MILD), its symptoms' levels (the book's special penalty, 5 points a level), its Contagion Index and its
** incubation (hours) */

#define V_ACHE          0x01
#define V_NUMB          0x02
#define V_BLEED         0x04
#define V_SICK          0x08
#define V_MILD          0x10
static const struct vkind {
    char            key[8];
    unsigned char   hr;
    unsigned        mins;
    unsigned char   how, lv, ci, inc;
} venoms[] = {
    { "adder", 3, 60, V_ACHE | V_BLEED },       /* (Snakes 2-3, p88: the heath adder's) */
    { "basilsk", 2, 10, 0 },                    /* (Dragons 13, p36) */
    { "lindwrm", 3, 10, V_NUMB },               /* (Dragons 14, p37) */
    { "dart", 4, 10, 0, 2 },                    /* (A trap's dart's or needle's: sick and dizzy, the house's) */
    { "contact", 4, 20, 0, 2 },                 /* (A lock's, a chest's smear: the skin burning, numb) */
    { "food", 4, 60, V_MILD, 3 },               /* (Spoiled food's: cramps and vomiting, the book's critical failure
                                                **   tasting it, Skills; never deadly, the house's) */
    { "flux", 4, 1440, V_SICK, 3, 5, 24 },      /* (Foul water's: the bloody flux, the house's C5/H4) */
    { "rat", 4, 1440, V_SICK, 2, 5, 48 },       /* (A giant rat's bite: rat-bite fever, the house's C5/H4) */
    { "ghoul", 4, 1440, V_SICK, 4, 3, 7 },      /* (Grave rot, the grave-wight's: H4, its wound festering, the
                                                **   universal penalty 20, 2d6 hours' incubation (7), the bestiary's;
                                                **   its claws' and its teeth's when they draw blood; C3 the house's) */
};
#define NVENOMS         (sizeof venoms / sizeof venoms[0])
enum { VK_DART = 3, VK_CONTACT, VK_FOOD, VK_FLUX };

struct venom pv;                                /* (The character's toxin: dq0harn.h's) */
struct sick pd;                                 /* (Its disease) */
unsigned char afffl;                            /* (Its symptoms' and its breath's fatigue levels, held) */
unsigned char aff_breath;                       /* (Its breath's, rsair.h's: among them) */

/* The levels its afflictions hold now (afffl), pc.fl kept in step (the levels more; fewer, gone at once) */
static void levels (void)
{
    unsigned char n = aff_breath, k;

    if (pv.hr && (k = venoms[pv.kind].lv) != 0) {
        k += venoms[pv.kind].hr > pv.hr ? venoms[pv.kind].hr - pv.hr : 0;  /* (The worse, the more) */
        n += k;
    }
    if (pd.hr && !pd.inc) {
        k = venoms[pd.kind].lv + (venoms[pd.kind].hr > pd.hr ? venoms[pd.kind].hr - pd.hr : 0);
        n += k;
    }
    if (n > afffl) {
        pc.fl += n - afffl;
    } else {
        k = afffl - n;
        pc.fl = pc.fl > k ? pc.fl - k : 0;
    }
    afffl = n;
}

#define AIR_SAY(s)      nput (s)
#define AIR_LEVELS(n)   (aff_breath = (n))
#include "../dq0/rsair.h"

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

/* The Infection table's roll (Physician 4: the rate times the Endurance): the rate's change (a critical failure two
** worse, a marginal one one; a marginal success one better, a critical two) */
static signed char __fastcall__ course (unsigned char hr, unsigned char* r)
{
    *r = harn_test ((int) hr * harn_end (&pc));
    return (signed char) hr + (*r == HARN_CF ? -2 : *r == HARN_MF ? -1 : *r == HARN_MS ? 1 : 2);
}

/* The toxin's roll, its course: a venom's the worse spread (a critical failure twice, the bestiary's option), a
** poison's felt; the better; gone at H6 (a venom's aches heal at H6, its numbness lifts), dead at H0 (but a mild one) */
static void venom_roll (void)
{
    unsigned char r, k, how = venoms[pv.kind].how;
    signed char hr = course (pv.hr, &r);

    if (hr < (signed char) pv.hr) {
        if (pv.kind < VK_DART) {
            nput (r == HARN_CF ? " The venom burns fiercely." : " The venom burns.");
            spread ();
            if (r == HARN_CF && pc.state != HARN_DEAD) {
                spread ();
            }
        } else {
            nput (r == HARN_CF ? " The poison grips you: you shake and sweat." : " You feel sick and dizzy.");
        }
    }
    if (pc.state == HARN_DEAD) {
        return;
    }
    if (hr <= 0 && (how & V_MILD)) {
        hr = 1;                                 /* (Spoiled food: wretched, not dead) */
    }
    if (hr <= 0) {
        pc.state = HARN_DEAD;
        pv.hr = 0;
        nput (pv.kind < VK_DART ? " The venom reaches your heart." : " The poison stops your heart.");
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
        nput (pv.kind < VK_DART ? " The venom's fire dies away: you've thrown it off." :
            " The poison's hold on you passes.");
        return;
    }
    pv.hr = hr;
}

/* The disease's daily roll (the Infection table): the fever worse, better; gone at H6, dead at H0 */
static void sick_roll (void)
{
    unsigned char r;
    signed char hr = course (pd.hr, &r);

    if (hr <= 0) {
        pc.state = HARN_DEAD;
        pd.hr = 0;
        nput (" The sickness takes you.");
        return;
    }
    if (hr >= 6) {
        pd.hr = 0;
        nput (" The sickness leaves you.");
        return;
    }
    nput (hr < (signed char) pd.hr ? " The fever burns higher." : hr > (signed char) pd.hr ? " The fever eases a little." :
        " The fever holds.");
    pd.hr = hr;
}

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
unsigned char __fastcall__ o_aff_time (struct req* r)
{
    static const unsigned char of[NAK] = { 0, VK_DART, VK_CONTACT, VK_FOOD, VK_FLUX };
    static const char* const shows[3] = {
        " Your belly gripes and cramps: you have the flux.", " A fever's on you, and the old bite throbs.",
        " A wound the ghoul gave you festers: a fever's on you."
    };
    unsigned m = r->w1, t;
    unsigned char k = 0, d = 1, o = 0, n = strlen (news), q = 0, f = 0;

    if (r->actor == 0xFF) {                     /* (The world's: what it's exposed to, the air; the UI's, a drink) */
        k = r->x[18];
        d = r->x[19];
        q = r->x[20];
        f = r->x[21];
    } else if (r->x[15] & 0x80) {               /* (Still water drunk where it stands: foul, the flux's) */
        r->x[15] &= 0x7F;
        k = AK_FLUX;
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
    if (pv.hr && pc.state != HARN_DEAD) {
        t = m * (r->a == 2 ? 4 : r->a ? 2 : 1) + pv.left;
        while (t >= venoms[pv.kind].mins && pv.hr && pc.state != HARN_DEAD) {
            t -= venoms[pv.kind].mins;
            venom_roll ();
        }
        pv.left = t;
    }
    if (pd.hr && pc.state != HARN_DEAD) {
        if (pd.inc) {
            if (m < pd.inc) {
                pd.inc -= m;
                m = 0;
            } else {
                m -= pd.inc;
                pd.inc = 0;
                nput (shows[pd.kind - VK_FLUX]);    /* (It shows: the first the character knows of it) */
                o |= 4;
            }
        }
        if (!pd.inc) {
            for (t = m + pd.left; t >= 1440 && pd.hr && pc.state != HARN_DEAD; t -= 1440) {
                sick_roll ();
            }
            pd.left = t;
        }
    }
    levels ();
    return o | (strlen (news) != n ? 2 : 0);
}

/* A remedy taken (a remedy's hook: an antidote, the market's to bring): the toxin's rate the better by its power (a
** theriac's 2), its words; 1 if it did anything */
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

/* The save's line of the body (the character's file's "body": its venom, its rest toward its healing in minutes, its
** disease, its breath, "body 3 14 0 25 1200 0 0 0 0 0 0"), into l (rd 0); or read back from l (rd 1: after "body ") */
void __fastcall__ o_body_line (char* l, unsigned char rd)
{
    char b[8];
    unsigned v[11];
    unsigned char k;

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
