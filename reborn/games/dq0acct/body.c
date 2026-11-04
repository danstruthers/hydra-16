/*
** body.c - (dq0acct, overlay 1) the days at rest: the referee's own code (../dq0harn's course.h, watch.h, the library's healing compiled
** here as the referee's is), told a day at a time; a physician; the wounds' words.
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")
#pragma bss-name ("OV1RODATA")
#pragma local-strings (on)
#pragma static-locals (on)

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "../dq0harn/dq0harn.h"
#include "dq0acct.h"

#define REST_ENOUGH     3600                    /* (heal.c's: the watches here always rest) */

#include "../../sdk/c/lib/harnheal.c"

/* (The library's harncmbt.c's two: none of the rest of it here) */
unsigned char __fastcall__ harn_is_limb (unsigned char loc)
{
    loc = HARN_LOC (loc);
    return loc >= HARN_SHOULDER && loc <= HARN_HAND ? 1 : loc >= HARN_HIP ? 2 : 0;
}

const char* __fastcall__ harn_location_name (unsigned char loc)
{
    static const char* const locations[HARN_NLOCATIONS] = {
        "skull", "face", "neck", "shoulder", "upper arm", "elbow", "forearm", "hand", "thorax", "abdomen", "groin", "hip",
        "thigh", "knee", "calf", "foot"
    };

    loc = HARN_LOC (loc);
    return loc < HARN_NLOCATIONS ? locations[loc] : "";
}

/* The course of a toxin and a disease, a watch of the clock, the library's rolls (no word of it told as the referee tells it: its
** last words, in news, a death's cause) */

static void __fastcall__ nput (const char* s)
{
    unsigned char n = strlen (news);

    if (n + strlen (s) >= NEWS_N - 1) {
        n = 0;
    }
    strcpy (news + n, s);
}

unsigned char __fastcall__ wound (harn_char* c, unsigned char loc, unsigned char aspect, int impact, unsigned char by)
{
    harn_injury* j;

    if (c->ninj >= HARN_INJMAX) {
        return 0;                               /* (The record's full, off-stage: a venom's last ache) */
    }
    j = &c->inj[c->ninj];                       /* (A venom's ache, an M1, course.h's the only wound off-stage: no shock roll of it, the house's) */
    memset (j, 0, sizeof *j);
    j->loc = loc;
    j->aspect = aspect;
    j->sev = HARN_MINOR;
    j->il = impact;
    pc_by[c->ninj] = by;
    wid[c->ninj] = 0xFF;
    ++c->ninj;
    return HARN_W_INJURED;
}

#include "../dq0harn/course.h"

static void __fastcall__ by_drop (unsigned char k)
{
    for (; k < HARN_INJMAX - 1; ++k) {
        pc_by[k] = pc_by[k + 1];
        wid[k] = wid[k + 1];
    }
    pc_by[k] = 0;
}

/* A healing roll (heal.c's heal_one, its words the account's own): 1 if it healed whole */
static unsigned char __fastcall__ heal_one (harn_char* c, unsigned char k)
{
    harn_injury* j = &c->inj[k];
    unsigned char n = c->ninj, h;

    h = harn_heal_roll (c, k, 0);
    if (c->ninj < n) {
        by_drop (k);
        return 1;
    }
    if (c->state == HARN_DEAD) {
        nput (" The infection kills you.");
    } else if ((j->flags & HARN_IF_INFECTED) && wid[k] != 0xFF) {
        was[wid[k]].fever = 1;
    }
    return h;
}

#include "../dq0harn/watch.h"

static unsigned wcount;

/* A day at rest: its six watches, its toxin's and its disease's minutes; 1 alive */
unsigned char o_day (void)
{
    unsigned char k = 6;

    do {
        rest_m = REST_ENOUGH;
        watch_of (&pc, ++wcount);
    } while (--k && pc.state != HARN_DEAD);
    if (pc.state != HARN_DEAD) {
        aff_course (1440, 0);
        levels ();
    }
    ++passed;
    return pc.state != HARN_DEAD;
}

/* The bleeding bound as it comes out (Combat 14: a point a minute each; a tourniquet its own Physician at +50, a layman's 30, or
** anything to hand: heal.c's binds, the house's): 1 alive */
unsigned char o_bind (void)
{
    unsigned char k, n, e = harn_end (&pc);
    unsigned v;

    for (;;) {
        for (k = n = 0; k < pc.ninj; ++k) {
            n += (pc.inj[k].flags & HARN_IF_BLEEDER) != 0;
        }
        if (!n) {
            return 1;
        }
        v = pc.bl + n;
        pc.bl = v > 255 ? 255 : v;
        if (pc.bl > e) {
            pc.state = HARN_DEAD;
            nput (" You bleed to death on the road.");
            return 0;
        }
        for (k = 0; k < pc.ninj; ++k) {
            if ((pc.inj[k].flags & HARN_IF_BLEEDER) && HARN_SUCCESS (harn_test ((pc.ml[HARN_PHY] ? pc.ml[HARN_PHY] : 30) + 50 - 5 * harn_get_up (&pc)))) {
                pc.inj[k].flags &= ~HARN_IF_BLEEDER;
                break;
            }
        }
    }
}

/* Nothing worse than minor left, no infection, no shock, no toxin, no disease */
unsigned char o_mended (void)
{
    unsigned char k;

    if (pc.state != HARN_CONSCIOUS || pv.hr || pd.hr) {
        return 0;
    }
    for (k = 0; k < pc.ninj; ++k) {
        if ((pc.inj[k].il > 1 && !(pc.inj[k].flags & IF_POOL)) || (pc.inj[k].flags & HARN_IF_INFECTED)) {
            return 0;
        }
    }
    return 1;
}

/* A physician for each wound not yet treated, if there's money (the book's one treatment roll a wound, Physician 3; the fee the
** house's: a physician's 72d a month over 24 working days, more for a grievous wound); none, its own Physician on what it may
** tend itself (Physician 2: not a grievous wound) */
void o_treat (void)
{
    harn_injury* j;
    unsigned long f;
    unsigned char k;

    for (k = 0; k < pc.ninj; ++k) {
        j = &pc.inj[k];
        if ((j->flags & (HARN_IF_TREATED | IF_POOL)) || pc_by[k] == BY_VENOM) {
            continue;
        }
        f = (j->sev >= HARN_GRIEVOUS ? 9 : 3) * PENNY;
        if (f <= funds) {
            funds -= f;
            fee += f;
            harn_treat (&pc, k, PHYS_ML);
            tended[ntended++] = j->loc;
        } else if (pc.ml[HARN_PHY] && j->sev < HARN_GRIEVOUS) {
            harn_treat (&pc, k, harn_eml (&pc, HARN_PHY));
            ++selfnd;
        } else {
            ++unpaid;
        }
    }
}

/* The wounds as they are now, to tell what comes of them */
void o_note (void)
{
    harn_injury* j;
    unsigned char k;

    nwas = 0;
    for (k = 0; k < pc.ninj; ++k) {
        j = &pc.inj[k];
        wid[k] = 0xFF;
        if (!(j->flags & IF_POOL) && pc_by[k] != BY_VENOM) {
            was[nwas].loc = j->loc;
            was[nwas].aspect = j->aspect;
            was[nwas].sev = j->sev;
            was[nwas].il = j->il;
            was[nwas].by = pc_by[k];
            was[nwas].fever = (j->flags & HARN_IF_INFECTED) != 0;
            wid[k] = nwas++;
        }
    }
}

