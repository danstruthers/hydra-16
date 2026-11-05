/*
** wrope.h - (dq0w) what rope and grapples' two parts share (wrope.c, part 96's; wrope2.c, part 98's; a copy each): a drop's
** place, in the request's copy's own fields (things.c's iq: its reply's, not used by these requests, the main RAM's, so
** they last across a stub's swap and cost no room there), and the small things said and looked for.
*/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"

extern struct ireq iq;
#define R_TX            iq.x                    /* (The drop's top's square, */
#define R_TY            iq.y
#define R_FX            iq.dur                  /*   its foot's, */
#define R_FY            iq.dur_max
#define R_FEET          iq.value                /*   how far) */
#define R_N             iq.item                 /* (The rope) */
#define R_HOW           iq.size                 /* (RK_*) */
#define R_HEAD          iq.thick                /* (The way from its top's square to its foot's) */
#define R_TSN           iq.craft
#define R_FSN           iq.quality
#define R_AK            iq.mat                  /* (The kind it's tied to) */
#define R_RUN           iq.plural               /* (Feet of it gone to reach the edge) */
#define R_END           iq.osel                 /* (Its end, feet above the foot) */
#define R_SLIP          iq.count                /* (Its grapple to give) */

unsigned char __fastcall__ on_pc (unsigned i);  /* (Part 10's) */
void __fastcall__ put (const char* s);          /* (tlist.s's: its words after the text) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;

/* The things' words begun, or the words why not */
static void rp_new (void)
{
    put_bank = UI_BANK;
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    hub_map (0);
}

static unsigned char __fastcall__ rp_no (const char* s)
{
    rp_new ();
    put (s);
    hub_map (0);
    HUB->ireq.qty = 0;
    return IR_NO;
}

/* Done, t tenths: its words said already */
static unsigned char __fastcall__ rp_done (unsigned t)
{
    hub_map (0);
    HUB->ireq.x = t;
    HUB->ireq.qty = 0;
    return IR_OK;
}

#ifndef RP_THROWN
/* Its kind's name, after the text */
static void __fastcall__ rp_name (unsigned char k)
{
    char b[14];

    hub_map (KIND_BANK);
    strcpy (b, CATALOGUE[k].name);
    put (b);
}
#endif

/* A noise here, how loud (the denizens': E_NOISE's) */
static void __fastcall__ rp_noise (unsigned char loud)
{
    struct msg m;

    hub_map (0);
    m.kind = E_NOISE;
    m.who = 0;
    m.a = loud;
    m.b = NZ_WORK;
    m.w1 = HUB->pc_x;
    m.w2 = HUB->pc_y;
    m.t = HUB->now;
    hub_event (&m);
}

/* A rope in place on a square: its record's number (ip it), or 0 */
static unsigned __fastcall__ rp_on (unsigned char sn, unsigned x, unsigned y)
{
    unsigned i = 0;

    while ((i = items (i, IN_SQUARE | IN_X, x)) != 0) {
        if (ip->y == y && ip->sn == sn && (ip->kind == KI_TIED || ip->kind == KI_HUNG)) {
            return i;
        }
    }
    return 0;
}

/* One carried, of kind k: its number, or 0 */
static unsigned __fastcall__ rp_carried (unsigned char k)
{
    unsigned i = 0;

    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (ip->kind == k && on_pc (i)) {
            return i;
        }
    }
    return 0;
}

#ifndef RP_THROWN
/* The thing tied to rope n, of kind k: its number, or 0 */
static unsigned __fastcall__ rp_in (unsigned n, unsigned char k)
{
    unsigned i = 0;

    while ((i = items (i, IN_ITEM | IN_X, n)) != 0) {
        if (ip->kind == k) {
            return i;
        }
    }
    return 0;
}

#endif

/* Its chunk's things in the store, by its square */
#define RP_NEED(sn, x, y)   it_need ((sn), ((x) - STRATUM (sn).x0) >> CW_BITS, ((y) - STRATUM (sn).y0) >> CW_BITS)
