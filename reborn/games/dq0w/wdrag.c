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
** All of it swapped part drag's (../dq0/swap.inc: dq0w.cfg's SW_drag), through things.c's s_serve; a door braced too (wdoor.c's
** part doors and dq0w.c's part across call it).  Its locals the part's own (static, in its image): what lasts across a call to
** another part (pick's, weight_of's: the first it calls) is in the request's copy, iq, the main RAM's.
*/

#pragma local-strings (on)

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "wbody.h"
#include "acts.h"

extern struct ireq iq;                          /* (things.c's: the request's copy) */
void __fastcall__ put (const char* s);          /* (tlist.s's: its words after the text) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;
unsigned __fastcall__ pick (const char* name, unsigned char how);   /* (Part ui_requests's) */
unsigned __fastcall__ at_hand (const char* name);
unsigned __fastcall__ weight_of (unsigned i);   /* (Part things's) */
#define HW_SEEN         2                       /* (things.c's pick: lying about, seen) */
#define D_SN            iq.craft                /* (The request's copy's own fields, past their use: the stratum, */
#define D_N             iq.item                 /*   the thing, */
#define D_WT            iq.dur                  /*   its weight, */
#define D_ONTO          iq.mat                  /*   the character's square its to go onto, a drag's) */

#pragma code-name (push, "SW_drag_CODE")
#pragma rodata-name (push, "SW_drag_RODATA")
#pragma bss-name (push, "SW_drag_RODATA")
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

/* A kind's name, the rows' text */
static const char* __fastcall__ name (unsigned char k)
{
    static char b[14];

    hub_map (KIND_BANK);
    return strcpy (b, CATALOGUE[k].name);
}

/* A sentence put coded (items.h's SAY_AT2: past 127; its SA_* | 0x80, the thing's name n, SAY_SEP, none, SAY_SEP):
** said.txt's SA rows word it, the UI's (narration as data's rest, 2026-11-15) */
static void __fastcall__ dsay (unsigned char sa, const char* n)
{
    static char c[3];

    c[0] = sa & 0x80 ? SAY_AT2 : SAY_AT;
    c[1] = sa | 0x80;
    c[2] = 0;
    put (c);
    put (n);
    put ("\2\2");
}

/* From (x, y), heading h: a step's room (step8's), not a doorway, nothing heavy there but the thing, no one on it (the
** one dragging it only if D_ONTO: the character, or body bd, a denizen's, iq.to's TO_DEN): 1 */
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
    if ((iq.to & TO_DEN) && bd.x == tx && bd.y == ty) {
        return D_ONTO;                          /* (Body bd's own square: its drag's, bd its own copy) */
    }
    hub_map (0);
    if (HUB->pc_x == tx && HUB->pc_y == ty && HUB->pc_stratum == D_SN) {
        return iq.to & TO_DEN ? 0 : D_ONTO;
    }
    for (a = HUB->actor, i = 0; i < NACTORS; ++i, ++a) {
        if (a->state && a->sn == D_SN && a->x == tx && a->y == ty && (!(iq.to & TO_DEN) || i != bk)) {
            return 0;
        }
    }
    return 1;
}

/* A shut door beside body bd (the four ways) braced (its FE_DRAG of thing 0: its mind's, shut against the character, the
** user's, 2026-11-23): a heavy thing on a square beside it (not the door's) dragged onto its own square, it a square back
** (room behind it), so the door's braced (s_braced's: a heavy thing on a square leading through it).  The thing (iq.item,
** iq.to's DG_PULL set), or 0 (no shut door beside it, braced already, nothing heavy there to drag) */
static unsigned brace (void)
{
    static unsigned char d, h;
    static unsigned char* q;
    static struct portal* o;

    D_SN = bd.sn;
    for (d = DIR_N; d <= DIR_W; ++d) {
        if ((q = at (bd.sn, bd.x + dir_x[d], bd.y + dir_y[d])) != 0 && TILE (*q) == T_DOOR &&
            (o = portal_at (cx_, cy_)) != 0 && o->kind == PK_DOOR && !(o->state & PS_OPEN)) {
            break;
        }
    }
    if (d > DIR_W || s_braced (bd.sn, bd.x + dir_x[d], bd.y + dir_y[d])) {
        return 0;
    }
    for (h = 0; h < 8; ++h) {                   /* (Its heading to the thing: any but the door's) */
        if ((head_x[h] != dir_x[d] || head_y[h] != dir_y[d]) &&
            (D_N = heavy_on (bd.sn, bd.x + head_x[h], bd.y + head_y[h], 0)) != 0 && room_to (bd.x, bd.y, (h + 4) & 7)) {
            iq.to |= DG_PULL;
            return D_N;
        }
    }
    return 0;
}

/* A heavy thing shoved a square away (DG_PUSH) or dragged a square toward (DG_PULL) the character (iq.name, the UI's), or
** body bd (iq.to's TO_DEN: its thing iq.item lying beside it, its errand FE_DRAG's: wthings.c's), every actor's by the
** act table's drag row (the one-path plan's stage 2, things and robbery, 2026-11-23): its strain's time, its noise (the
** character's E_NOISE, an actor's E_DEN DE_DRAGS, told seen or heard), its test (P_FORCE by the actor's own sheet, the
** row's steps the easier, a step the worse each 10 pounds).  A denizen's has no words of its own (its noise is its
** telling), and its step back after a drag is its mind's, as the character's is the UI's */
#define D_H             iq.quality              /* (Its heading: past the other parts' calls) */
#define D_WHO           iq.plural               /* (Who: 0 the character, else body bd's entry plus 1) */
#define D_T             iq.value                /* (Its time, the row's) */
unsigned char s_drag (void)
{
    static struct item* p;
    static unsigned x, y, ix, iy;
    static signed char dx, dy;
    static unsigned char h, k, ok;
    static unsigned char* q;
    static struct portal* d;

    if (iq.to == DG_TASTE) {
        return taste ();
    }
    D_WHO = iq.to & TO_DEN ? bk + 1 : 0;
    if (!D_WHO && !(iq.item = pick (iq.name, HW_SEEN))) {
        return IR_NONE;
    }
    if (!iq.item && !brace ()) {
        return IR_NONE;                         /* (A brace, nothing to brace with: nothing done) */
    }
    D_WT = weight_of (D_N);                     /* (Its weight, what's in it too: the other parts called first, this
                                                **   part's locals its image's again after them) */
    if (D_WHO) {
        D_SN = bd.sn;
        x = bd.x;
        y = bd.y;
        h = bd.face;
        tp = tpe = &ok;                         /* (No words: put's room nil) */
    } else {
        hub_map (0);
        D_SN = HUB->pc_stratum;
        x = HUB->pc_x;
        y = HUB->pc_y;
        h = HUB->pc_face;
        put_bank = UI_BANK;                     /* (The things' words begun) */
        hub_map (UI_BANK);
        tp = UIREC->things_text;
        tpe = tp + sizeof UIREC->things_text - 1;
        *tp = 0;
    }
    p = item_at (D_N);
    k = p->kind;
    ix = p->x;
    iy = p->y;
    ok = p->where == IW_SQUARE && p->sn == D_SN;
    dx = ix - x;
    dy = iy - y;
    hub_map (KIND_BANK);
    if (!ok || CATALOGUE[k].bulk < DRAG_BULK || (CATALOGUE[k].keeps & KF_FIXED)) {
        ok = CATALOGUE[k].keeps & KF_FIXED;
        dsay (ok ? SA_DRAG_FIXED : SA_NO_SHOVE, name (k));     /* (Its answer names its thing: the sweeps') */
        return IR_NO;
    }
    if (dx < -1 || dx > 1 || dy < -1 || dy > 1) {
        if (D_WHO) {
            return IR_NOT_HERE;                 /* (Out of its reach: its mind's to go there) */
        }
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
    if (iq.to & DG_PULL) {                      /* (A drag: toward the one dragging it, and it back a square after,
                                                **   room behind it first) */
        h = (h + 4) & 7;
        if (!room_to (x, y, h)) {
            goto none;
        }
        D_ONTO = 1;
    }
    if (!room_to (ix, iy, h)) {
none:
        dsay (iq.to & DG_PULL ? SA_NO_ROOM_DRAG : SA_NO_ROOM_SHOVE, name (k));
        return IR_NO;
    }
    D_H = h;
    D_T = act_noise (D_WHO, AC_DRAG, x, y);     /* (The strain heard, whether it moves or not: the row's) */
    ok = G_OK (act_test (D_WHO, P_FORCE, (signed char) (2 - (D_WT >= 440 ? 11 : D_WT / 40))));   /* (Its strength,
                                                **   the row's two steps the easier, a step the worse each 10 pounds:
                                                **   the drag row's steps, 2) */
    h = D_H;                                    /* (This part's again: its locals its image's) */
    k = item_at (D_N)->kind;
    if (D_WHO) {
        tp = tpe = &ok;
    } else {
        put_bank = UI_BANK;
        hub_map (UI_BANK);
        tp = UIREC->things_text;
        tpe = tp + sizeof UIREC->things_text - 1;
        *tp = 0;
    }
    if (!ok) {
        dsay (SA_STRAIN, name (k));
        h = 0xFF;
    } else {
        p = item_at (D_N);
        x = p->x += head_x[h];
        y = p->y += head_y[h];
        dsay (iq.to & DG_PULL ? SA_DRAG : SA_SHOVE, name (k));
        for (k = DIR_N; k <= DIR_W; ++k) {      /* (Beside a door now: it braces it shut) */
            if ((q = at (D_SN, x + dir_x[k], y + dir_y[k])) != 0 && TILE (*q) == T_DOOR &&
                (d = portal_at (cx_, cy_)) != 0 && d->kind == PK_DOOR) {
                dsay (SA_AGAINST, "");
                break;
            }
        }
    }
    hub_map (0);
    if (D_WHO) {
        bd.ready += D_T;                        /* (Its strain's time, the row's) */
        ++HUB->den_seq;
        return ok ? IR_OK : IR_NO;
    }
    HUB->ireq.x = D_T;
    HUB->ireq.y = h;
    return IR_OK;
}

#pragma static-locals (pop)
#pragma bss-name (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
