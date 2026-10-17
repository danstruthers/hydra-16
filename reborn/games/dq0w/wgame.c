/*
** wgame.c - (dq0w) the game's own rules, the world's (Phase 2: one authority on state): the theme's puzzle judged
** (theme.h: the UI's command of it, items.h's IO_PUZZLE, the things found to hand and the rule for their kinds
** looked for, done as it says: the things made one, put, the way out opened, the hub's way_open; the UI words it), and
** a thing walked over to and taken up (I_TAKE: the walk the world's go_to, again as long as it gets nearer; then the
** thing taken up as the item store's IO_GET takes it, E_TAKEN told).
**
** All of it swapped part 16's (../dq0/swap.inc: dq0w.cfg's SW16), called through its stubs (bank.s's).
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "work.h"

#pragma local-strings (on)

unsigned take_n;                                /* (things.h's: I_TAKE's thing, its item; 0 none) */
static unsigned take_x, take_y;                 /*   its square, */
static unsigned take_fx, take_fy;               /*   and where the walk to it began */
static unsigned char done_;                     /* (The rules done, a bit each: the save's, the UI's quest) */
static char an[16], bn[16];                     /* (The things named) */

#pragma code-name ("SW16CODE")
#pragma rodata-name ("SW16RODATA")

/* ---- The theme's puzzle */

/* The thing named that's to hand (the item store's IO_FIND, in the request being answered: the UI's, which waits):
** its kind, the theme's (0 ...), or 0xFE (one of the catalogue's); 0xFF if there's none (the things' words why in
** things_text) */
static unsigned char __fastcall__ found (const char* name)
{
    unsigned char k;

    hub_map (0);
    HUB->ireq.kind = IO_FIND;
    strcpy (HUB->ireq.name, name);
    serve ();
    hub_map (0);
    if (HUB->ireq.result != IR_OK) {
        return 0xFF;
    }
    k = HUB->ireq.x;
    return k >= KI_THEME ? k - KI_THEME : 0xFE;
}

/* The rule (verb, a, b) among the theme's: its number, or 0xFF (the theme's bank in the window) */
static unsigned char __fastcall__ rule (unsigned char verb, unsigned char a, unsigned char b)
{
    unsigned char r;
    const struct trule* p = THEME->rule;

    hub_map (THEME_BANK);
    for (r = 0; r < THEME->nrules && r < NRULES; ++r, ++p) {
        if (p->verb == verb && p->a == a && (b == 0xFF || p->b == b)) {
            return r;
        }
    }
    return 0xFF;
}

/* The way out within reach: the character on the top stratum, a square or two from it (its first link's) */
static unsigned char near_out (void)
{
    hub_map (0);
    return !HUB->pc_stratum && strata[0].link[0].x - HUB->pc_x + 2 <= 4 && strata[0].link[0].y - HUB->pc_y + 2 <= 4;
}

/* The hub's ireq, IO_PUZZLE's, answered (but its state, the caller's): its verb's rule for the things named looked
** for, by their kinds, and done (its result PZ_*, x the rule); 1 if what lies about has changed (a thing put) */
unsigned char s_puzzle (void)
{
    static unsigned char v, a, b, r, res, changed, on, k;

    hub_map (0);
    v = HUB->ireq.to;
    strcpy (an, HUB->ireq.name);
    strcpy (bn, HUB->ireq.into);
    hub_map (UI_BANK);
    UIREC->things_text[0] = 0;                  /* (No words of the things' yet) */
    res = PZ_NO;
    r = 0xFF;
    changed = 0;
    b = 0xFF;
    if (v == TR_OUT && !near_out ()) {
        res = PZ_FAR;
    } else if ((a = found (an)) != 0xFF && (v == TR_READ || v == TR_OUT || (b = found (bn)) != 0xFF)) {
        r = rule (v, a, b);
        if (r == 0xFF && v == TR_COMBINE && (r = rule (v, b, a)) != 0xFF) {
            hub_map (0);                        /* (The one that becomes the whole the rule's first) */
            strcpy (bn, an);
            strcpy (an, HUB->ireq.into);
        }
        hub_map (THEME_BANK);
        if (r == 0xFF) {
            res = PZ_NONE;
        } else if (v == TR_READ || v == TR_OUT) {
            res = PZ_DONE;
        } else {                                /* (The things made one, or put: the item store's) */
            on = v == TR_PUT && (THEME->kind[b].keeps & KF_ON);
            k = THEME->rule[r].c;
            hub_map (0);
            HUB->ireq.kind = v == TR_COMBINE ? IO_COMBINE : IO_PUT;
            strcpy (HUB->ireq.name, an);
            strcpy (HUB->ireq.into, bn);
            HUB->ireq.to = v == TR_COMBINE ? KI_THEME + k : on;
            changed = serve ();
            hub_map (0);
            res = HUB->ireq.result == IR_OK ? PZ_DONE : PZ_NO;
        }
    }
    if (res == PZ_DONE) {
        done_ |= 1 << r;
        hub_map (THEME_BANK);
        k = THEME->rule[r].effect;
        hub_map (0);
        if (k == TE_OPEN) {
            HUB->way_open = 1;                  /* (The way out opened: the mission's, E_LEFT as it goes up) */
        }
    }
    hub_map (0);
    HUB->ireq.result = res;
    HUB->ireq.x = r;
    return changed;
}

/* ---- A thing walked over to and taken up (I_TAKE's, as the walk's done and the character's free): 0 if it wasn't
** (E_TAKEN told: none, out of reach still, a portal's part or a thing fixed where it stands, its hands full; E_READY
** after), 1 if it was (told: the UI's to ask the time it took, its light, as for any thing handled), 2 if it's walking
** on to it (nearer than it was: a denizen moved out of its way, the walk cut short) */
unsigned char s_take_up (void)
{
    unsigned n = take_n;
    unsigned char r, was, k, w;
    const struct item* p;

    hub_map (0);
    was = HUB->pc_light;
    p = item_at (n);
    k = p->kind;
    w = p->where;
    if (!k) {
        r = IR_NONE;
    } else if (!in_reach (n)) {
        hub_map (0);
        if (HUB->pc_x != take_fx || HUB->pc_y != take_fy) {
            take_fx = HUB->pc_x;                /* (Nearer: on again) */
            take_fy = HUB->pc_y;
            go_to (take_x, take_y);
            return 2;
        }
        r = IR_NOT_HERE;
    } else {
        hub_map (CAT_BANK);
        r = w == IW_PORTAL || (CATALOGUE[k].keeps & KF_FIXED) ? IR_FIXED : carry (n, 0) ? IR_OK : IR_FULL;
        light_now ();                           /* (Its light: what it holds lit) */
    }
    take_n = 0;
    hub_map (0);
    wk.m.kind = E_TAKEN;                        /* (Told: the work's event record) */
    wk.m.a = 0;
    wk.m.w1 = r;
    wk.m.w2 = was;
    wk.m.t = HUB->now;
    hub_event (&wk.m);
    return r == IR_OK;
}

/* I_TAKE's walk begun (its intent: the square the thing lies on, its item) */
void __fastcall__ s_take_go (const struct msg* m)
{
    take_n = (unsigned) m->t;
    take_x = m->w1;
    take_y = m->w2;
    hub_map (0);
    take_fx = HUB->pc_x;
    take_fy = HUB->pc_y;
    go_to (take_x, take_y);
}
