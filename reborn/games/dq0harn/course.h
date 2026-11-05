/*
** course.h - (dq0harn) the course of the character's toxin and disease, the referee's (afflict.c's, overlay 8) and what passes
** between dungeons (../dq0acct's, told the days at rest): the kinds, the book's rolls, a venom's spread, their words (news, by
** the includer's nput).  The includer has pc, pc_by, pv, pd, afffl, aff_breath, wound and nput.  Moved from afflict.c whole,
** 2026-11-04; its course as it was.
*/

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
    { "spider", 3, 60, V_ACHE | V_BLEED },      /* (A cave spider's bite, 2026-11-04: its kin the adder's, the house's) */
    { "dart", 4, 10, 0, 2 },                    /* (A trap's dart's or needle's: sick and dizzy, the house's) */
    { "contact", 4, 20, 0, 2 },                 /* (A lock's, a chest's smear: the skin burning, numb) */
    { "food", 4, 60, V_MILD, 3 },               /* (Spoiled food's: cramps and vomiting, the book's critical failure
                                                **   tasting it, Skills; never deadly, the house's) */
    { "flux", 4, 1440, V_SICK, 3, 5, 24 },      /* (Foul water's: the bloody flux, the house's C5/H4) */
    { "rat", 4, 1440, V_SICK, 2, 5, 48 },       /* (A giant rat's bite: rat-bite fever, the house's C5/H4) */
    { "ghoul", 4, 1440, V_SICK, 4, 4, 7 },      /* (Grave rot, the grave-wight's: H4, C4, its wound festering, the
                                                **   universal penalty 20, 2d6 hours' incubation (7), the bestiary's;
                                                **   its claws' and its teeth's when they draw blood) */
};
#define NVENOMS         (sizeof venoms / sizeof venoms[0])
enum { VK_DART = 4, VK_CONTACT, VK_FOOD, VK_FLUX };

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

/* The toxin's course for m minutes at what it's about (a: its interval at rest, half that about, a quarter at vigorous work,
** the bestiary's), the disease's (shown after its incubation, then daily): 4 if it showed now */
static unsigned char __fastcall__ aff_course (unsigned m, unsigned char a)
{
    static const char* const shows[3] = {
        " Your belly gripes and cramps: you have the flux.", " A fever's on you, and the old bite throbs.",
        " A wound the ghoul gave you festers: a fever's on you."
    };
    unsigned t;
    unsigned char o = 0;

    if (pv.hr && pc.state != HARN_DEAD) {
        t = m * (a == 2 ? 4 : a ? 2 : 1) + pv.left;
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
    return o;
}
