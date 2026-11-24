/*
** wcloth.c - (dq0w, swapped part 108: ../dq0/swap.inc) the textiles (HOUSE.md's bandages, the user's, 2026-11-10: "'rip'
** and 'tear up' verbs should be allowed to turn a garment, bed sheet, towel, etc. into bandages"): a cloth the
** character has to hand torn up for bandages where it is (the things' IO_RIP: its kind made KI_BANDAGES, a lot of so
** many, its material, its make and its cleanliness kept), and one washed at running water or a well (IO_WASH: soiled or
** bloody made worn, never clean).  A cloth's cleanliness is its wear (items.h's CL_*: clean, worn, soiled, bloody), each
** band's own as the bandages have it (join's: a lot of one cloth and one band alone).  The bandages' grade, their
** cloth's suitability by its cleanliness (CL_GRADE), is the referee's (the tools' digit, s_tools'); a dressing used up
** by it (s_use_key's), the old one kept, bloody.
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"

unsigned __fastcall__ at_hand (const char* name);   /* (Part 4's stub: to hand, carried or within reach) */
unsigned char __fastcall__ on_pc (unsigned i);
void __fastcall__ put (const char* s);          /* (tlist.s's: the hub's text, the words put after what's there) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;

#pragma code-name ("SW_textiles_CODE")
#pragma rodata-name ("SW_textiles_RODATA")
#pragma local-strings (on)

/* How many bandages each cloth gives (the house's, the design's: a sheet's or a blanket's eight, a towel's three; never
** more than its weight allows, a bandage a quarter pound); a cloth not here, none (rope, net, paper: not torn up) */
static const unsigned char yield[][2] = {
    { KI_SHROUD, 6 }, { KI_BLANKET, 8 }, { KI_ROBE, 8 }, { KI_CLOTHES, 6 }, { KI_CLOAK, 6 }, { KI_SURCOAT, 4 },
    { KI_SACK, 2 }, { KI_GAMBESON, 8 }, { KI_SHEET, 8 }, { KI_TOWEL, 3 }, { KI_SCARF, 2 }
};
#define NYIELD          (sizeof yield / sizeof yield[0])

static char nm[16];                             /* (The thing's kind's name, its words') */
static unsigned t;                              /* (Its time, tenths) */

/* The words into the UI bank's things_text, coded: sentence sa (said.txt's SA rows, the UI's: narration as data's rest,
** 2026-11-15), the thing's name, b the other's; the time in the request's x: IR_OK (t), else IR_NO (none) */
static unsigned char __fastcall__ told (unsigned char sa, const char* b)
{
    char c[3];

    put_bank = UI_BANK;
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    c[0] = SAY_AT;
    c[1] = sa | 0x80;
    c[2] = 0;
    put (c);
    put (nm);
    put ("\2");
    put (b);
    put ("\2");
    hub_map (0);
    HUB->ireq.x = t;
    HUB->ireq.qty = 0;
    return t ? IR_OK : IR_NO;
}

/* A blade the character carries (a knife's family, a sword's: an edge to cut cloth with), the one named (name; "" any):
** its item, 0 none; 0xFFFF one named that isn't a blade */
static unsigned char __fastcall__ edged (unsigned char k)
{
    unsigned char f;

    hub_map (KIND_BANK);
    f = CATALOGUE[k].slots;
    return CATALOGUE[k].craft == AP_WEAPON && (f == FM_KNIFE || f == FM_SWORD);
}

static unsigned __fastcall__ blade (const char* name)
{
    unsigned i = 0;

    if (*name) {
        return !(i = at_hand (name)) ? 0 : edged (item_at (i)->kind) ? i : 0xFFFF;
    }
    while ((i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0) {
        if (edged (ip->kind) && on_pc (i)) {
            return i;
        }
    }
    return 0;
}

/* Its cleanliness, the band's own wear (CL_*): a shroud the dead's, a denizen's bedding (a faction's, not worn), and a
** cloth found lying in the dungeon (make_own's) soiled; a garment worn, at best (it's been worn) */
static unsigned char __fastcall__ clean_of (const struct item* p, unsigned char kf)
{
    unsigned char w = p->wear;

    if (p->kind == KI_SHROUD || (p->owner && !(kf & K_WEAR))) {
        w |= 0x80;
    } else if ((kf & K_WEAR) && w < 32) {
        w = CL_WORN;
    }
    return w < 32 ? 0 : w < 128 ? CL_WORN : w < CL_BLOODY ? CL_SOILED : CL_BLOODY;
}

/* IO_RIP, IO_WASH (the request's kind): the cloth named (the request's name; into, the blade said): its words, its time
** (the request's x), IR_OK; IR_NO refused, the words why; IR_NONE none of that name to hand */
unsigned char s_textile (void)
{
    struct item* p;
    unsigned n, b;
    unsigned char op, k, kf, m, wt, q;
    char into[16];
    char c[6];

    hub_map (0);
    op = HUB->ireq.kind;
    memcpy (nm, HUB->ireq.name, sizeof nm);
    memcpy (into, HUB->ireq.into, sizeof into);
    t = 0;
    if (!(n = at_hand (nm))) {
        told (SA_NO_SUCH, "");
        return IR_NONE;
    }
    p = item_at (n);
    k = p->kind;
    if (op == IO_WASH && p->wear < 128) {       /* (Washed: of those of its kind it carries, a soiled or a bloody one
                                                **   first, "wash the bandages" the old dressing's) */
        for (b = 0; (b = items (b, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
            if (ip->kind == k && ip->wear >= 128 && on_pc (b)) {
                n = b;
                break;
            }
        }
        p = item_at (n);
    }
    m = p->mat;
    q = p->flags & I_WORN;
    hub_map (KIND_BANK);
    kf = CATALOGUE[k].flags;
    if (!m) {
        m = CATALOGUE[k].mat;                   /* (Not its own yet: its kind's) */
    }
    wt = CATALOGUE[k].weight;                   /* (Its weight, quarter pounds: the bandages' most) */
    memcpy (nm, CATALOGUE[k].name, sizeof nm);
    if (!MT_TEXTILE (m)) {
        return told (m == MT_LEATHER ? SA_LEATHER : SA_NOT_CLOTH, "");
    }
    if (q) {
        return told (SA_TAKE_OFF_FIRST, "");
    }
    if (items (0, IN_ITEM | IN_X, n)) {
        return told (SA_EMPTY_FIRST, "");
    }
    if (op == IO_WASH) {
        hub_map (0);
        b = HUB->pc_wet;
        if (!(b & WET_RUNS) && (b || !water_at (1))) {
            return told (b ? SA_FOUL : SA_NO_WASH, "");
        }
        p = item_at (n);
        if (p->wear < 128) {
            return told (SA_CLEAN, "");
        }
        p->wear = CL_WORN;
        t = 1800;                               /* (Three minutes: the house's) */
        return told (SA_WASH, kf & K_PLURAL ? "them" : "it");
    }
    if (k == KI_BANDAGES) {
        return told (SA_BANDAGES, "");
    }
    for (op = 0; op < NYIELD && yield[op][0] != k; ++op) {
    }
    if (op == NYIELD) {
        return told (SA_NO_TEAR, "");
    }
    q = yield[op][1] > wt ? wt : yield[op][1];  /* (A bandage a quarter pound: never more than its weight) */
    if ((b = blade (into)) == 0xFFFF) {
        return told (SA_NO_CUT, "");
    }
    if (!b && (m == MT_WOOL || k == KI_GAMBESON)) {
        return told (SA_STOUT, "");
    }
    p = item_at (n);
    p->wear = clean_of (p, kf);
    p->kind = KI_BANDAGES;
    p->mat = m;
    p->qty = q;
    p->flags &= ~I_NAMED;
    t = (b ? 600 : 1200) * q;                   /* (A minute a bandage with a blade, two by hand) */
    return told (b ? SA_CUT : SA_TEAR, utoa (q, c, 10));    /* (The bandages' count, the other's name) */
}
