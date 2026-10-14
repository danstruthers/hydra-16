/*
** harnheal.c - harn.h's healing (Physician 2-4): a treatment roll gives an injury its healing rate (the Treatment
** table, by the injury's aspect and severity); a healing roll every five days takes its levels away (or infects an
** open wound, which then rolls daily); shock recovers by four-hour rolls; a fainted character wakes by shock rolls;
** grievous wounds bleed a point a minute, and bloodloss regenerates; rest takes fatigue away.
*/

#include <harn.h>

#define EE      7                               /* A treatment's "EE": healed in a day */
#define IMP     0x80                            /* (The table's *: an attribute 1d3 less once it's healed) */

/* By aspect (blunt, edge, point, fire) and severity (minor, serious, grievous): the treatment's bonus, then the
** healing rate untreated, and with CF, MF, MS, CS: bruise, fracture, crush; cuts; stabs; burns */
static const unsigned char treatment[4][3][6] = {
    { { 30, 5, 4, 5, 6, EE }, { 20, 4 | IMP, 3 | IMP, 4 | IMP, 5, 6 }, { 10, 3 | IMP, 2 | IMP, 3 | IMP, 4, 5 } },
    { { 30, 5, 4, 5, 6, EE }, { 20, 4, 3, 4, 5, 6 }, { 10, 3 | IMP, 2 | IMP, 3 | IMP, 4 | IMP, 5 } },
    { { 25, 5, 4, 5, 6, EE }, { 15, 4, 3, 4, 5, 6 }, { 5, 3 | IMP, 2 | IMP, 3 | IMP, 4 | IMP, 5 } },
    { { 30, 5, 4, 5, 6, EE }, { 15, 3, 2, 3, 4, 5 }, { 5, 2, 1, 2, 3, 4 } },
};

/* An open wound (it can be infected): a grievous blunt one, a serious or grievous burn, any cut or stab */
static unsigned char __fastcall__ open (const harn_injury* j)
{
    return j->aspect == HARN_EDGE || j->aspect == HARN_POINT || (j->aspect == HARN_BLUNT && j->sev == HARN_GRIEVOUS) ||
        (j->aspect == HARN_FIRE && j->sev >= HARN_SERIOUS);
}

/* Injury i gone (healed) */
static void __fastcall__ drop (harn_char* c, unsigned char i)
{
    for (--c->ninj; i < c->ninj; ++i) {
        c->inj[i] = c->inj[i + 1];
    }
}

unsigned char __fastcall__ harn_treat (harn_char* c, unsigned char i, int eml)
{
    harn_injury* j = &c->inj[i];
    const unsigned char* t;
    unsigned char hr;

    if (i >= c->ninj || (j->flags & HARN_IF_TREATED)) {
        return j->hr;
    }
    if (j->flags & HARN_IF_BLEEDER) {           /* A bleeder first (tourniquet, +50): stopped with MS or CS */
        if (eml < 0 || !HARN_SUCCESS (harn_test (eml + 50))) {
            return 0;
        }
        j->flags &= ~HARN_IF_BLEEDER;
    }
    t = treatment[j->aspect & 3][(j->sev ? j->sev : 1) - 1];
    hr = eml < 0 ? t[1] : t[2 + harn_test (eml + t[0])];     /* (eml under 0: untreated, the NT column) */
    j->hr = hr & 7;
    j->flags |= HARN_IF_TREATED | (hr & IMP ? HARN_IF_IMPAIR : 0) | (j->hr == EE ? HARN_IF_EE : 0);
    return j->hr;
}

unsigned char __fastcall__ harn_heal_roll (harn_char* c, unsigned char i, int bonus)
{
    harn_injury* j = &c->inj[i];
    unsigned char r, e = harn_end (c);
    int hr;

    if (i >= c->ninj) {
        return 0;
    }
    if (!j->hr) {                               /* Never treated: the untreated rate */
        harn_treat (c, i, -1);
    }
    if (j->hr == EE) {
        drop (c, i);
        return 1;
    }
    r = harn_test ((int) j->hr * e + bonus - 5 * harn_get_fp (c));
    if (j->flags & HARN_IF_INFECTED) {          /* (Infected: the daily infection roll) */
        hr = j->hr + (r == HARN_CF ? -2 : r == HARN_MF ? -1 : r == HARN_MS ? 1 : 2);
        if (hr <= 0) {
            c->state = HARN_DEAD;
            j->hr = 0;
            return 0;
        }
        if (hr >= 6) {
            hr = 6;
            j->flags &= ~HARN_IF_INFECTED;
        }
        j->hr = hr;
        return 0;
    }
    switch (r) {
    case HARN_CF:
        if (open (j) && c->att[HARN_SP] != HARN_SIN) {     /* (Sindarin are immune to infection) */
            j->flags |= HARN_IF_INFECTED;
        }
        return 0;
    case HARN_MF:
        return 0;
    }
    hr = (int) j->il - (r == HARN_CS ? 2 : 1);
    if (hr <= 0) {
        drop (c, i);
        return 1;
    }
    j->il = hr;
    if (hr <= 1) {
        j->flags &= ~HARN_IF_LIMB;              /* (A limb is usable again at M1) */
    }
    return 0;
}

unsigned char __fastcall__ harn_shock_recovery (harn_char* c, int bonus)
{
    unsigned char r;
    int hr;

    if (c->state != HARN_SHOCK) {
        return c->state;
    }
    r = harn_test ((int) c->shockhr * harn_end (c) + bonus);
    hr = c->shockhr + (r == HARN_CF ? -2 : r == HARN_MF ? -1 : r == HARN_MS ? 1 : 2);
    if (hr <= 0) {
        c->state = HARN_DEAD;
    } else if (hr >= 6) {
        c->state = HARN_CONSCIOUS;
    }
    c->shockhr = hr < 0 ? 0 : hr;
    return c->state;
}

/* A fainted character's turn: a shock roll not over Endurance wakes it, then one more decides whether it's in
** shock (Skills 22, Combat 14) */
unsigned char __fastcall__ harn_wake (harn_char* c)
{
    if (c->state == HARN_UNCONSCIOUS && !harn_shock_roll (c)) {
        if (harn_shock_roll (c)) {
            c->state = HARN_SHOCK;
            c->shockhr = 5;
        } else {
            c->state = HARN_CONSCIOUS;
        }
    }
    return c->state;
}

unsigned char __fastcall__ harn_bleed (harn_char* c, unsigned minutes)
{
    unsigned char k, n = 0;
    unsigned bl;

    for (k = 0; k < c->ninj; ++k) {
        if (c->inj[k].flags & HARN_IF_BLEEDER) {
            ++n;
        }
    }
    bl = c->bl + (unsigned long) n * minutes;   /* (A point a minute for each bleeder) */
    c->bl = bl > 255 ? 255 : bl;
    if (c->bl > harn_end (c)) {
        c->state = HARN_DEAD;
    }
    return c->state;
}

void __fastcall__ harn_regen_blood (harn_char* c)
{
    unsigned char r;

    if (c->bl) {
        r = harn_test (6 * harn_end (c));
        if (HARN_SUCCESS (r)) {
            c->bl = c->bl > (r == HARN_CS ? 2 : 1) ? c->bl - (r == HARN_CS ? 2 : 1) : 0;
        }
    }
}

void __fastcall__ harn_rest (harn_char* c, unsigned minutes)
{
    unsigned n = minutes / 10;

    c->fl = c->fl > n ? c->fl - n : 0;
}
