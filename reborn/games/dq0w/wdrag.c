/*
** wdrag.c - (dq0w) a heavy thing shoved or dragged a square, and a thing tasted (the verbs plan's B4, the user's "push and
** pull include moving furniture", 2026-11-09; DECISIONS.md's "The things' verbs: use, push, pull, touch, taste"): the
** things' IO_DRAG (items.h's).  A thing lying about whose kind is DRAG_BULK pints or more and isn't fixed where it stands
** (a chest, a table, a cask, a body) is shoved a square away from the character (DG_PUSH: the way from the character to
** it; on its own square, the way it faces), or dragged a square toward it (DG_PULL: the UI steps the character back a
** square after, its facing kept, I_STEP's).  Strength against its weight, what's in it too (the referee's P_FORCE, two
** steps the better, a step the worse each 10 pounds), five seconds' strain, a noise of 3 (E_NOISE's, the denizens'),
** heard whether or not it moves.  It goes only where a walker could step (step8's: no wall, no drop, no rise past a
** step), not into a doorway, nor onto anyone, nor onto another heavy thing.  Beside a door (on a square that leads
** through it), it braces it shut: the door won't open, either side, anyone's (wdoor.c's PA_OPEN, PO_BLOCKED; dq0w.c's
** door_on), till it's moved away.  Tasted (DG_TASTE): the thing named to hand, its kind and what's in it (its first
** thing), that one's wear (water's: from still water), for the UI to word; nothing used up.
**
** All of it swapped part 109's (../dq0/swap.inc: dq0w.cfg's SW109), through things.c's s_serve; a door braced too (wdoor.c's
** part 5 and dq0w.c's part 33 call it).  Its locals the part's own (static, in its image): what lasts across a call to
** another part (pick's, weight_of's: the first it calls) is in the request's copy, iq, the main RAM's.
*/

#pragma local-strings (on)

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"

extern struct ireq iq;                          /* (things.c's: the request's copy) */
void __fastcall__ put (const char* s);          /* (tlist.s's: its words after the text) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;
unsigned __fastcall__ pick (const char* name, unsigned char how);   /* (Part 4's) */
unsigned __fastcall__ at_hand (const char* name);
unsigned __fastcall__ weight_of (unsigned i);   /* (Part 10's) */
#define HW_SEEN         2                       /* (things.c's pick: lying about, seen) */
#define DRAG_T          50                      /* (Tenths: a shove's, a drag's strain) */
#define D_SN            iq.craft                /* (The request's copy's own fields, past their use: the stratum, */
#define D_N             iq.item                 /*   the thing, */
#define D_WT            iq.dur                  /*   its weight, */
#define D_ONTO          iq.mat                  /*   the character's square its to go onto, a drag's) */

#pragma code-name (push, "SW109CODE")
#pragma rodata-name (push, "SW109RODATA")
#pragma bss-name (push, "SW109RODATA")
#pragma static-locals (push, on)

/* A heavy thing (a kind shoved about, not carried: its bulk DRAG_BULK or more, not fixed where it stands) on stratum
** sn's square x, y, but thing n: its number, or 0 */
static unsigned __fastcall__ heavy_on (unsigned char sn, unsigned x, unsigned y, unsigned n)
{
    static unsigned i;
    static unsigned char k;

    i = 0;
    while ((i = items (i, IN_SQUARE | IN_X, x)) != 0) {
        if (i != n && ip->y == y && ip->sn == sn) {
            k = ip->kind;
            hub_map (KIND_BANK);
            if (CATALOGUE[k].bulk >= DRAG_BULK && !(CATALOGUE[k].keeps & KF_FIXED)) {
                return i;
            }
        }
    }
    return 0;
}

/* A door on stratum sn's square x, y braced shut: a heavy thing on a square beside it (the four ways: the two that lead
** through it, the walls' never have one); 1 if so.  The chunk's window on the door's square after (at ()'s), as its
** callers had it (wdoor.c's, dq0w.c's door_on) */
unsigned char __fastcall__ s_braced (unsigned char sn, unsigned x, unsigned y)
{
    static unsigned char d, r;

    for (r = 0, d = DIR_N; d <= DIR_W && !r; ++d) {
        r = heavy_on (sn, x + dir_x[d], y + dir_y[d], 0) != 0;
    }
    at (sn, x, y);
    return r;
}

/* A thing tasted (DG_TASTE): its kind in x, its first thing's in y (0 none), that one's wear in qty */
static unsigned char taste (void)
{
    static unsigned n;

    if (!(n = at_hand (iq.name))) {
        return IR_NONE;
    }
    iq.x = item_at (n)->kind;
    iq.y = 0;
    iq.qty = ip->wear;
    if (items (0, IN_ITEM | IN_X, n)) {
        iq.y = ip->kind;
        iq.qty = ip->wear;
    }
    hub_map (0);
    HUB->ireq.x = iq.x;
    HUB->ireq.y = iq.y;
    HUB->ireq.qty = iq.qty;
    return IR_OK;
}

/* A kind's name after the text */
static void __fastcall__ name (unsigned char k)
{
    static char b[14];

    hub_map (KIND_BANK);
    strcpy (b, CATALOGUE[k].name);
    put (b);
}

/* From (x, y), heading h: a step's room (step8's), not a doorway, nothing heavy there but the thing, no one on it (the
** character only if D_ONTO): 1 */
static unsigned char __fastcall__ room_to (unsigned x, unsigned y, unsigned char h)
{
    static const struct actor* a;
    static unsigned char i;
    static unsigned tx, ty;

    if (!step8 (D_SN, x, y, h)) {
        return 0;
    }
    tx = x + head_x[h];
    ty = y + head_y[h];
    if (TILE (*at (D_SN, tx, ty)) == T_DOOR || heavy_on (D_SN, tx, ty, D_N)) {
        return 0;
    }
    hub_map (0);
    if (HUB->pc_x == tx && HUB->pc_y == ty) {
        return D_ONTO;
    }
    for (a = HUB->actor, i = NACTORS; i; --i, ++a) {
        if (a->state && a->sn == D_SN && a->x == tx && a->y == ty) {
            return 0;
        }
    }
    return 1;
}

unsigned char s_drag (void)
{
    static struct msg m;
    static struct req* r;
    static struct item* p;
    static unsigned x, y, ix, iy;
    static signed char dx, dy;
    static unsigned char h, k, ok;
    static unsigned char* t;
    static struct portal* q;

    if (iq.to == DG_TASTE) {
        return taste ();
    }
    if (!(D_N = pick (iq.name, HW_SEEN))) {
        return IR_NONE;
    }
    D_WT = weight_of (D_N);                     /* (Its weight, what's in it too: the other parts called first, this
                                                **   part's locals its image's again after them) */
    hub_map (0);
    D_SN = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    h = HUB->pc_face;
    p = item_at (D_N);
    k = p->kind;
    ix = p->x;
    iy = p->y;
    ok = p->where == IW_SQUARE && p->sn == D_SN;
    dx = ix - x;
    dy = iy - y;
    put_bank = UI_BANK;                         /* (The things' words begun) */
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    hub_map (KIND_BANK);
    if (!ok || CATALOGUE[k].bulk < DRAG_BULK || (CATALOGUE[k].keeps & KF_FIXED)) {
        ok = CATALOGUE[k].keeps & KF_FIXED;
        put ("\nThe ");                         /* (Its answer names its thing: the sweeps') */
        name (k);
        put (ok ? " is fixed where it stands." : " isn't a thing to shove about: pick it up.");
        return IR_NO;
    }
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1) {
        hub_map (0);                            /* (Out of reach: the UI walks over to it first, then again) */
        HUB->ireq.x = ix;
        HUB->ireq.y = iy;
        HUB->ireq.item = D_N;
        return IR_NOT_HERE;
    }
    if (dx || dy) {
        for (h = 0; head_x[h] != dx || head_y[h] != dy; ++h) {
        }
    }
    D_ONTO = 0;
    if (iq.to) {                                /* (A drag: toward the character, and the character back a square, room
                                                **   behind it first) */
        h = (h + 4) & 7;
        if (!room_to (x, y, h)) {
            goto none;
        }
        D_ONTO = 1;
    }
    if (!room_to (ix, iy, h)) {
none:
        put (iq.to ? "\nThere's no room behind you to drag the " : "\nThere's no room to shove the ");
        name (k);
        put (iq.to ? " back." : " that way.");
        return IR_NO;
    }
    hub_map (0);                                /* (The strain heard, whether it moves or not) */
    m.kind = E_NOISE;
    m.who = 0;
    E_NOISE_LOUD (m) = 3;
    E_NOISE_WHAT (m) = NZ_WORK;
    E_NOISE_SX (m) = x;
    E_NOISE_SY (m) = y;
    m.t = HUB->now;
    hub_event (&m);
    r = &HUB->slot[SLOT_UI];                    /* (The UI's: it waits on this request meanwhile) */
    r->kind = RS_TEST;
    r->actor = 0;
    RS_TEST_PURPOSE (*r) = P_FORCE;
    RS_TEST_HOW (*r) = 0;
    RS_TEST_STEPS (*r) = D_WT >= 440 ? -9 : 2 - (signed char) (D_WT / 40);   /* (A step the worse each 10 pounds) */
    r = hub_ask (SLOT_UI);
    ok = G_OK (RS_TEST_GRADE (*r));
    r->state = REQ_FREE;
    if (!ok) {
        put ("\nYou strain at the ");
        name (k);
        put (", but it won't budge.");
        h = 0xFF;
    } else {
        p = item_at (D_N);
        x = p->x += head_x[h];
        y = p->y += head_y[h];
        put (iq.to ? "\nYou take hold of the " : "\nYou set your shoulder to the ");
        name (k);
        put (iq.to ? " and drag it toward you, stepping back." : " and shove it a square away.");
        for (k = DIR_N; k <= DIR_W; ++k) {      /* (Beside a door now: it braces it shut) */
            if ((t = at (D_SN, x + dir_x[k], y + dir_y[k])) != 0 && TILE (*t) == T_DOOR &&
                (q = portal_at (cx_, cy_)) != 0 && q->kind == PK_DOOR) {
                put (" It stands against the door.");
                break;
            }
        }
    }
    hub_map (0);
    HUB->ireq.x = DRAG_T;
    HUB->ireq.y = h;
    return IR_OK;
}

#pragma static-locals (pop)
#pragma bss-name (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
