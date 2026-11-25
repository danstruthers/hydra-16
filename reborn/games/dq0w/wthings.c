/*
** wthings.c - (dq0w) a denizen's things handled by the character's own rules and the act table's rows (the one-path
** plan's stage 2, things and robbery, 2026-11-23: ONE-PATH.md's 2.1 and 2.7, the act audit's C1, C6, C9, G-9, G-10,
** G-11): its errands, I_ROB's a (FE_STOW ... FE_HAND, I_ROB:fetch's fields), through wempty.c's handout (part empty),
** each the character's handling's rule and its row's time on its ready, told as it's seen (E_BEAT:things, the limbs'
** news's way: its square in the character's sight):
**   FE_STOW   its thing put away: coins into its purse (a pouch hanging from its belt they fit in: things.c's s_purse),
**             a thing of a carrier's family into that carrier, else its pack (things.c's stow: the room, the weight,
**             the form, closed: the catalogue's o_fits, the character's); the put row, and its reach (part reach's: a
**             pack on the back off and on again, a container's level);
**   FE_PUT    its thing put in or on another (w1; w2 1 on it), lying within its reach (the take rule's square, not
**             down in a pit) or its own: o_fits' rule, its compartment by the prep (things.c's cfor); refused closed,
**             full, on a lid that's up, as the character's is; the put row and the reach;
**   FE_DON    its thing worn: a wearable alone (the user's: "wear" only wearables), not worn already, one of a kind
**             worn at once (its worn places not kept for a denizen: its record's sn its home's stratum); the don row;
**   FE_DOFF   its worn thing taken off: into a hand if one's free, else its pack, else at its feet (things.c's doff
**             and s_doff_feet); the doff row;
**   FE_DRAG   a heavy thing beside it shoved or dragged a square: wdrag.c's drag, the character's, its request's copy
**             (TO_DEN; w2 1 a drag);
**   FE_HAND   its thing in hand handed to actor w1 less 1 beside it: the receiver's take rule (its hands, what it
**             lifts: wtake.c's), the give row.
** And body bd's carry (s_den_carry: the character's s_carry, every actor's): a thing it's taken, coins to its purse,
** else a hand if one's free (or asked first), else its pack: wtake.c's take, the take rule's thing put where it goes.
** No mind posts these errands yet: the minds' to choose (the user's "what a mind chooses stays the mind's").
**
** All of it swapped part den_acts's (../dq0/swap.inc: dq0w.cfg's SW_den_acts), through its stubs.  Its locals the C stack's or
** the part's own but re-read after a call into another part (the part's image is copied in again as it returns).
*/

#pragma local-strings (on)

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "wbody.h"
#include "acts.h"

#pragma code-name ("SW_den_acts_CODE")
#pragma rodata-name ("SW_den_acts_RODATA")
#pragma bss-name ("SW_den_acts_RODATA")

extern struct ireq iq;                          /* (things.c's: the request's copy) */
unsigned __fastcall__ weight_of (unsigned i);   /* (Part things's) */
unsigned char __fastcall__ fits_c (unsigned n, unsigned c);   /* (Part things's: o_fits, ccomp's compartment) */
extern unsigned char ccomp;                     /* (things.c's: the compartment a thing's put in) */
extern struct comp crow;                        /* (things.c's: a compartment's row, comp_of's) */
unsigned char __fastcall__ comp_of (unsigned k, unsigned char c);
unsigned char drag (void);                      /* (Part drag's: wdrag.c) */
void den_equip (void);                          /* (Part den_things's, wlimbs.c's: body bd's things told the referee) */
extern unsigned char den_eq_all;
unsigned char __fastcall__ sky_ask (unsigned w);    /* (Part shaft's: a thing let fall on a shaft's head) */

#define FETCH_HANDS     200                     /* (Quarter pounds: what a folk denizen lifts in its hands, 50 lb; in a
                                                **   mouth, 3 lb: wtake.c's take rule, HOUSE.md's fetching) */
#define FETCH_MOUTH     12

/* Thing i body bd's own (an actor's thing, however deep: IW_DEN at the last, its number's); its holder at the last in
** the record ip */
unsigned char __fastcall__ s_den_own (unsigned i, unsigned did)
{
    const struct item* p;

    for (p = item_at (i); p->where == IW_ITEM; p = item_at (p->x)) {
    }
    return p->where == IW_DEN && p->owner == (unsigned char) did && p->hide == did >> 8;
}
#define own(i)          s_den_own ((i), bd.did)

/* What actor did holds, counted (its hands) */
unsigned char __fastcall__ s_den_hands (unsigned did)
{
    unsigned c = 0;
    unsigned char k = 0;

    while (did && (c = items (c, IN_DEN, 0)) != 0) {
        if (ip->owner == (unsigned char) did && ip->hide == did >> 8 && (ip->flags & I_HELD)) {
            ++k;
        }
    }
    hub_map (0);
    return k;
}

/* Thing n held by actor a (a copy, or bd: filed at its lair's home, as its things are) */
void __fastcall__ s_den_hold (unsigned n, const struct actor* a, unsigned char f)
{
    struct item* p = item_at (n);

    p->where = IW_DEN;
    p->sn = a->hsn;
    p->x = a->hx;
    p->y = a->hy;
    p->owner = a->did;
    p->hide = a->did >> 8;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED)) | f;
    hub_map (0);
}

/* Thing n put in thing c, its compartment cc (ccomp's: o_fits' crow its row) */
void __fastcall__ s_den_putin (unsigned n, unsigned c, unsigned char cc)
{
    struct item* p;

    if (cc) {
        comp_of (item_at (c)->kind, cc);
    }
    p = item_at (n);
    p->where = IW_ITEM;
    p->x = c;
    p->y = cc ? IC_MARK | crow.prep << 4 | cc : 0;
    p->flags &= ~(I_HELD | I_WORN | I_HIDDEN | I_TENDED);
    hub_map (0);
}

/* Told as it's seen: its square in the character's sight (E_BEAT:things, w the beat and its thing's kind, o the
** other's) */
void __fastcall__ s_den_seen (unsigned w, unsigned o)
{
    struct msg m;
    unsigned char* p = at (bd.sn, bd.x, bd.y);

    if (p && (*p & T_VIS)) {
        hub_map (0);
        m.kind = E_BEAT;
        m.who = 0;
        E_BEAT_WHOSE (m) = bk + 1;
        m.w1 = w;
        m.w2 = o;
        m.t = HUB->now;
        hub_devent (&m);
    }
    hub_map (0);
    ++HUB->den_seq;
}

/* The act table's row ac's time */
static unsigned __fastcall__ row_t (unsigned char ac)
{
    struct actq q;

    act_get (q, ac);
    return q.r.time;
}

/* The reach to container c (in it, put away: part reach's, every actor's), its time; the character's owed reach kept */
unsigned __fastcall__ s_den_lvl (unsigned c, unsigned char from)
{
    unsigned t, kt = rtime, ko = roff;
    unsigned char ks = rsteps, kb = rbody, kr = rs_ok;

    rtime = roff = 0;
    rsteps = rbody = 0;
    rs_ok = 0;
    if (from) {
        reach_from (c);
    } else {
        reach_to (c);
    }
    t = rtime;
    rtime = kt;
    roff = ko;
    rsteps = ks;
    rbody = kb;
    rs_ok = kr;
    return t;
}

/* Body bd's own things of flags f (K_*) worn, of form b (KF_* of its kind's form, 0 any): the first, or 0 */
static unsigned __fastcall__ worn_of (unsigned char f, unsigned char b)
{
    unsigned i = 0;
    unsigned char k;

    while (bd.did && (i = items (i, IN_DEN, 0)) != 0) {
        if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_WORN)) {
            k = ip->kind;
            hub_map (KIND_BANK);
            if ((CATALOGUE[k].flags & f) && (!b || (CATALOGUE[k].form & b))) {
                hub_map (0);
                return i;
            }
        }
    }
    hub_map (0);
    return 0;
}

/* Where thing n goes put away on body bd (things.c's stow and s_purse, the character's): coins into a pouch hanging
** from its belt they fit in; a thing of a family into its carrier there (a sword its scabbard: a stow's, not a carry's,
** cr 0); else (but coins carried, cr 2, a hand to take them) its pack worn, if it fits.  Its number, or 0 */
unsigned __fastcall__ s_den_home (unsigned n, unsigned char cr)
{
    unsigned b, i;
    unsigned char k, f, fm, sl;

    k = item_at (n)->kind;
    hub_map (KIND_BANK);
    fm = CATALOGUE[k].form;
    sl = CATALOGUE[k].slots;
    ccomp = 0;
    if ((b = worn_of (K_BELT, 0)) != 0) {
        for (i = 0; (i = items (i, IN_X | IN_ITEM, b)) != 0; ) {
            if (i == n) {
                continue;
            }
            k = ip->kind;
            hub_map (KIND_BANK);
            f = CATALOGUE[k].flags;
            if ((f & (K_CONTAINER | K_HANG)) == (K_CONTAINER | K_HANG) &&
                (CATALOGUE[k].slots ? !cr && CATALOGUE[k].slots == sl : (fm & (KF_SMALL | KF_COUNT)) == (KF_SMALL | KF_COUNT)) &&
                !fits_c (n, i)) {
                return i;                       /* (Its family's carrier; coins, a pouch with room: s_purse's) */
            }
        }
    }
    if (cr != 2 && (b = worn_of (K_CONTAINER, KF_BACK)) != 0 && b != n && !fits_c (n, b)) {
        return b;
    }
    return 0;
}

/* Thing n carried by body bd (the character's s_carry, every actor's: the take rule's thing, wtake.c's): coins into its
** purse (s_purse's); else, hand_first 0 or no hand free, its pack if it fits; else in a hand, if one's free.  1 put
** away (its reach's time on its ready), 2 held, 0 neither (its hands full, no room) */
unsigned char __fastcall__ s_den_carry (unsigned n, unsigned char hand_first)
{
    unsigned c;
    unsigned char h;

    h = s_den_hands (bd.did) < (bd.body == BD_TWO ? 2 : 1);
    if ((c = s_den_home (n, hand_first && h ? 2 : 1)) != 0) {
        s_den_putin (n, c, 0);
        bd.ready += s_den_lvl (c, 0);            /* (Put away: its reach, part reach's) */
        return 1;
    }
    if (!h) {
        return 0;
    }
    s_den_hold (n, &bd, I_HELD);
    return 2;
}

/* Room for thing n on body bd, its hands full (the take rule's, wtake.c's: its purse for coins, its pack): its
** number, or 0 */
unsigned __fastcall__ s_den_room (unsigned n)
{
    return s_den_home (n, 1);
}

/* Thing i taken by body bd (wtake.c's take, the take rule's: lying, or in a thing within its reach, H17's at any
** depth): its time (s_den_tt's, a weapon's taken into its hand by wtake.c's got too) on its ready (the take row's, or the takeout row's out of a thing, and its reach, part reach's, a
** level each; a worn kind stripped from a body, the strip row's more: the character's own, things.c's s_moved), then
** carried (s_den_carry: coins to its purse, a hand first, else its pack), told as it's seen (snatched up; taken from
** the thing it was in; an ally's fetch, tell 0, by its own words, wfetch.c's) */
unsigned __fastcall__ s_den_tt (unsigned i)
{
    const struct item* p = item_at (i);
    unsigned char k = p->kind, b, o;

    if (p->where != IW_ITEM) {
        hub_map (0);
        return row_t (AC_TAKE);
    }
    o = item_at (p->x)->kind;                   /* (Out of a thing: the takeout row, its reach a level each) */
    hub_map (KIND_BANK);
    b = (o == KI_BODY || o == KI_BONES) && (CATALOGUE[k].flags & K_WEAR);
    hub_map (0);
    return row_t (AC_TAKEOUT) + s_den_lvl (i, 1) + (b ? row_t (AC_STRIP) : 0);
}

unsigned char __fastcall__ s_den_got (unsigned i, unsigned char tell)
{
    const struct item* p = item_at (i);
    unsigned o = p->where == IW_ITEM ? item_at (p->x)->kind : 0;
    unsigned char k = item_at (i)->kind, r;

    bd.ready += s_den_tt (i);                   /* (Its time: the take's, or the takeout's, its levels, a body's strip) */
    r = s_den_carry (i, 1);
    if (!tell) {
    } else if (o) {
        s_den_seen (WB_TAKES | (unsigned) k << 8, o);
    } else {
        s_den_seen (WB_PICKS | (unsigned) k << 8, 0);
    }
    return r;
}

/* ==== Part den_errands's (part den_acts full): the errands' rest, worn, taken off, handed over; through part den_acts's stubs */
#pragma code-name ("SW_den_errands_CODE")
#pragma rodata-name ("SW_den_errands_RODATA")
#pragma bss-name ("SW_den_errands_RODATA")

unsigned char __fastcall__ den_own (unsigned i, unsigned did);   /* (Part den_acts's, above: its stubs) */
unsigned char __fastcall__ den_hands (unsigned did);
void __fastcall__ den_hold (unsigned n, const struct actor* a, unsigned char f);
void __fastcall__ den_seen (unsigned w, unsigned o);
unsigned __fastcall__ den_lvl (unsigned c, unsigned char from);
unsigned char __fastcall__ den_carry (unsigned n, unsigned char hand_first);
unsigned char __fastcall__ den_store (unsigned n, unsigned c, unsigned char on);
void __fastcall__ den_robbed (unsigned char purse);   /* (Part den_store's: the character's robbery) */
void __fastcall__ den_stash (const struct msg* q);  /* (Part den_store's: a robber's takings put by at its lair) */
#undef own
#define own(i)          den_own ((i), bd.did)

/* The act table's row ac's time (part den_errands's own) */
static unsigned __fastcall__ row_u (unsigned char ac)
{
    struct actq q;

    act_get (q, ac);
    return q.r.time;
}

/* Its thing worn (FE_DON): a wearable alone, not worn already, one of its kind worn at most */
static unsigned char __fastcall__ don (unsigned n)
{
    unsigned i = 0;
    unsigned char k;

    k = item_at (n)->kind;
    if (!own (n) || (item_at (n)->flags & I_WORN) || (hub_map (KIND_BANK), !(CATALOGUE[k].flags & K_WEAR))) {
        hub_map (0);
        return IR_NO;
    }
    while ((i = items (i, IN_DEN, 0)) != 0) {
        if (ip->kind == k && ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_WORN)) {
            hub_map (0);
            return IR_NO;                       /* (One of a kind worn: things.c's SA_WEAR_ONE) */
        }
    }
    bd.ready += row_u (AC_DON) + den_lvl (n, 1);   /* (Got out first, its reach) */
    den_hold (n, &bd, I_WORN);
    return IR_OK;
}

/* Its worn thing taken off (FE_DOFF): into a hand, else its pack, else at its feet */
static unsigned char __fastcall__ doff (unsigned n)
{
    struct item* p;

    if (!own (n) || !(item_at (n)->flags & I_WORN)) {
        hub_map (0);
        return IR_NO;
    }
    bd.ready += row_u (AC_DOFF);
    item_at (n)->flags &= ~I_WORN;
    if (!den_carry (n, 1)) {
        p = item_at (n);                        /* (Set down at its feet: things.c's s_doff_feet) */
        p->where = IW_SQUARE;
        p->sn = bd.sn;
        p->x = bd.x;
        p->y = bd.y;
        p->owner = p->hide = 0;
        p->flags = (p->flags & ~(I_HELD | I_WORN)) | I_KNOWN;
        sky_ask (n);
    }
    return IR_OK;
}

/* Its thing in hand handed to actor a less 1 beside it (FE_HAND): the receiver's take rule (its hands, what it lifts) */
static unsigned char __fastcall__ hand (unsigned n, unsigned char a)
{
    struct actor q;
    unsigned w;

    if (!a || a > NACTORS || !own (n) || !(item_at (n)->flags & I_HELD)) {
        hub_map (0);
        return IR_NO;
    }
    hub_map (0);
    memcpy (&q, &HUB->actor[a - 1], sizeof q);
    if (!q.state || q.sn != bd.sn || (unsigned) (q.x - bd.x + 1) > 2 || (unsigned) (q.y - bd.y + 1) > 2) {
        return IR_NOT_HERE;
    }
    w = weight_of (n);
    if (w > (q.body == BD_TWO ? FETCH_HANDS : FETCH_MOUTH)) {
        return IR_HEAVY;
    }
    if (den_hands (q.did) >= (q.body == BD_TWO ? 2 : 1)) {
        return IR_FULL;
    }
    den_hold (n, &q, I_HELD);
    bd.ready += row_u (AC_GIVE);
    return IR_OK;
}

/* Whether body bd wears a thing of kind k: 1 */
static unsigned char __fastcall__ wears (unsigned char k)
{
    unsigned i = 0;

    while ((i = items (i, IN_DEN, 0)) != 0) {
        if (ip->kind == k && ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_WORN)) {
            hub_map (0);
            return 1;
        }
    }
    hub_map (0);
    return 0;
}

/* The thing of its own an errand's done with, the world's to choose (t 0: its mind's, the user's, 2026-11-23): FE_STOW
** its loot held (coins, jewels, its takings: no weapon, no light); FE_DON the armour it carries, not worn, worth more than
** the best it wears and of a kind it doesn't wear (the most worth: TW_ARMOUR's "better", the world's); FE_HAND (FE_PASS's,
** need PN_*) a spare its fellow needs, a weapon (PN_ARMS) or a light not lit (PN_LIGHT), one of two at least of its
** own, one not in hand first.  Its number, or 0 */
static unsigned __fastcall__ choose (unsigned char e, unsigned char need)
{
    unsigned i = 0, w = 0, v, best = 0, bv = 0;
    unsigned char k, f, c, l, kf, n = 0, ok;

    if (!bd.did) {
        return 0;
    }
    while (e == FE_DON && (i = items (i, IN_DEN, 0)) != 0) {    /* (The best it wears, its worth) */
        if (ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 && (ip->flags & I_WORN)) {
            k = ip->kind;
            hub_map (KIND_BANK);
            if (CATALOGUE[k].craft == AP_ARMOUR && CATALOGUE[k].value > w) {
                w = CATALOGUE[k].value;
            }
        }
    }
    for (i = 0; (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {
        k = ip->kind;
        f = ip->flags;
        hub_map (KIND_BANK);
        c = CATALOGUE[k].craft;
        v = CATALOGUE[k].value;
        l = CATALOGUE[k].light;
        kf = CATALOGUE[k].flags;
        ok = CATALOGUE[k].form & KF_COUNT;
        ok = e == FE_STOW ? (f & I_HELD) && c != AP_WEAPON && !l && ((f & I_LOOT) || k == KI_PENNIES || k == KI_FARTHINGS ||
            k == KI_CROWNS || c == AP_JEWEL) : e == FE_DON ? !(f & I_WORN) && c == AP_ARMOUR && (kf & K_WEAR) && v > w &&
            v > bv : e != FE_HAND ? 0 : need == PN_ARMS ? c == AP_WEAPON && (kf & K_HAND) && !ok : need == PN_LIGHT && l;
        if (!ok || !own (i) || (e == FE_DON && wears (k))) {
            continue;
        }
        if (e != FE_HAND) {
            best = i;                           /* (Loot: the first; armour: the most worth) */
            bv = v;
            if (e == FE_STOW) {
                break;
            }
            continue;
        }
        ++n;                                    /* (A spare: one of two, not lit, one not in hand first) */
        if (!(item_at (i)->flags & I_LIT) && (!best || !(f & I_HELD))) {
            best = i;
        }
    }
    hub_map (0);
    return e != FE_HAND || n >= 2 ? best : 0;
}

/* Body bd's errand (its intent m, I_ROB with a FE_STOW ... FE_HAND: t its thing, w1 w2 the other and how; t 0, the
** world's to choose, choose's: nothing to do, nothing done, no time; FE_STASH, its takings put by at its lair): the
** result (IR_*), told as it's seen if done; refused, a second's pause (a denizen's refused act's) */
unsigned char __fastcall__ s_den_things (const struct msg* m)
{
    struct ireq k;
    struct item was;
    unsigned n = I_ROB_THING (*m), o = 0;
    unsigned char e = I_ROB_ERRAND (*m), r, b, h;

    if (e == FE_STASH) {
        den_stash (m);                          /* (Part den_store's) */
        return IR_OK;
    }
    if (!n && (e == FE_STOW || e == FE_DON || e == FE_HAND)) {
        if (!(n = choose (e, I_ROB_Y (*m)))) {
            return IR_NONE;
        }
        memcpy (&was, item_at (n), sizeof was);
        if (e == FE_HAND && !(was.flags & I_HELD)) {
            bd.ready += row_u (AC_TAKEOUT) + den_lvl (n, 1);   /* (Got out first, to hand: the takeout row, its reach) */
            den_hold (n, &bd, I_HELD);
            if ((r = hand (n, I_ROB_X (*m))) != IR_OK) {
                memcpy (item_at (n), &was, sizeof was);   /* (Not taken: put back where it was) */
                hub_map (0);
                return r;
            }
            hub_map (0);
            den_eq_all = 1;
            den_equip ();
            den_seen (WB_HANDS_TO | (unsigned) was.kind << 8, I_ROB_X (*m));
            return IR_OK;
        }
    }
    h = (item_at (n)->flags & I_HELD) != 0;
    b = item_at (n)->kind;
    hub_map (0);
    switch (e) {
    case FE_STOW:
        r = den_store (n, 0, 0);
        b = WB_STOWS;
        break;
    case FE_PUT:
        r = den_store (n, I_ROB_X (*m), I_ROB_Y (*m) != 0);
        o = item_at (I_ROB_X (*m))->kind | (I_ROB_Y (*m) ? 0x100 : 0);
        b = WB_PUTS;
        break;
    case FE_DON:
        r = don (n);
        b = WB_DONS;
        break;
    case FE_DOFF:
        r = doff (n);
        b = WB_DOFFS;
        break;
    case FE_DRAG:                               /* (wdrag.c's, the character's own, in a request copy of its own: the
                                                **   character's kept on the C stack, wempty.c's handout's way) */
        memcpy (&k, &iq, sizeof iq);
        iq.kind = IO_DRAG;
        iq.to = TO_DEN | (I_ROB_Y (*m) ? DG_PULL : DG_PUSH);
        iq.item = n;
        r = drag ();
        memcpy (&iq, &k, sizeof iq);
        hub_map (0);
        return r;                               /* (Told by its noise: E_DEN DE_DRAGS) */
    case FE_ROBBED:                             /* (Robbed by the character: part den_store's) */
        den_robbed (I_ROB_PURSE (*m) & RB_PURSE);
        return IR_OK;
    case FE_HAND:
        r = hand (n, I_ROB_X (*m));
        o = I_ROB_X (*m);
        b = WB_HANDS_TO;
        break;
    default:
        return IR_NO;
    }
    if (r != IR_OK) {
        hub_map (0);
        bd.ready += 10;
        return r;
    }
    if (h || e == FE_DON || e == FE_DOFF) {
        den_eq_all = 1;
        den_equip ();                           /* (What it holds and wears told the referee: the character's equip_now) */
    }
    den_seen (b | (unsigned) item_at (n)->kind << 8, o);
    return r;
}

/* ==== Part den_store's (parts den_acts and den_errands full): a thing put away, put in or on another; through part den_acts's stubs */
#pragma code-name ("SW_den_store_CODE")
#pragma rodata-name ("SW_den_store_RODATA")
#pragma bss-name ("SW_den_store_RODATA")

void __fastcall__ den_putin (unsigned n, unsigned c, unsigned char cc);   /* (Part den_acts's: its stubs) */
unsigned __fastcall__ den_home (unsigned n, unsigned char cr);
unsigned char __fastcall__ den_own (unsigned i, unsigned did);
unsigned __fastcall__ den_lvl (unsigned c, unsigned char from);
#undef own
#define own(i)          den_own ((i), bd.did)

/* The act table's row ac's time (part den_store's own) */
static unsigned __fastcall__ row_v (unsigned char ac)
{
    struct actq q;

    act_get (q, ac);
    return q.r.time;
}

/* Its thing put away (FE_STOW) */
static unsigned char __fastcall__ stow (unsigned n)
{
    unsigned c;

    if (!own (n) || (item_at (n)->flags & I_WORN) || !(c = den_home (n, 0))) {
        return IR_NO;
    }
    den_putin (n, c, 0);
    bd.ready += row_v (AC_PUT) + den_lvl (c, 0);
    return IR_OK;
}

/* Its thing put in or on thing c (FE_PUT; on: on it), its compartment by the prep (things.c's cfor: its first on it, or
** in it, else its first) */
static unsigned char __fastcall__ put (unsigned n, unsigned c, unsigned char on)
{
    const struct item* p;
    unsigned char k, i, r;

    if (!own (n) || (item_at (n)->flags & I_WORN)) {
        return IR_NO;
    }
    if (!own (c)) {                             /* (Else lying within its reach: on its square or beside it, not down in a
                                                **   pit from its edge: the take rule's) */
        for (p = item_at (c); p->where == IW_ITEM; p = item_at (p->x)) {
        }
        if (p->where != IW_SQUARE || p->sn != bd.sn || (unsigned) (p->x - bd.x + 1) > 2 ||
            (unsigned) (p->y - bd.y + 1) > 2 || ((p->x != bd.x || p->y != bd.y) && TILE (*at (bd.sn, p->x, p->y)) == T_PIT)) {
            hub_map (0);
            return IR_NOT_HERE;
        }
    }
    k = item_at (c)->kind;
    for (r = 0, i = 0; comp_of (k, i); ++i) {
        if (crow.prep == (on ? CP_ON : CP_IN)) {
            r = i;
            break;
        }
    }
    ccomp = r;
    i = fits_c (n, c);                          /* (The character's rule: its room, its weight, its form, closed, a lid
                                                **   up: SA_*) */
    ccomp = 0;
    if (i) {
        return IR_NO;
    }
    den_putin (n, c, r);
    if (!den_own (c, bd.did)) {
        item_at (n)->owner = item_at (n)->hide = 0;   /* (Put in a thing not its own: no longer its) */
        hub_map (0);
    }
    bd.ready += row_v (AC_PUT) + den_lvl (c, 0);
    return IR_OK;
}

/* Its thing put away (FE_STOW: c 0), or put in or on thing c (FE_PUT): part den_errands's way in */
unsigned char __fastcall__ s_den_store (unsigned n, unsigned c, unsigned char on)
{
    return c ? put (n, c, on) : stow (n);
}

/* ---- The character's robbery (the one-path plan's stage 2, things and robbery, 2026-11-23: ONE-PATH.md's 2.7, the
** character robbing and filching a living denizen; the denizens' own robbery, wfight.c's s_den_rob, the other way
** about) */
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);   /* (Part things's: the character's) */
unsigned char __fastcall__ icomp (unsigned i);  /* (things.c's: thing i's compartment in what holds it, crow its row) */
void light_now (void);                          /* (Part serve's) */
void den_equip (void);
extern unsigned char den_eq_all;
#define told(how, kind) (evb = E_TASK_FETCHED_B (bk + 1, (how)), event (E_TASK, 0, E_TASK_FETCHED_W1 (kind)),                          ++HUB->den_seq)

/* Thing i, on body bd, got at by the character's hand (each compartment on its way out: a hidden one the character
** hasn't found, or a locked one, not; a closed one opened, a tie untied, a lid lifted: the lid row's time, the
** character's reach owed): 1, or 0 */
static unsigned char __fastcall__ got_at (unsigned i)
{
    struct item* q;
    unsigned h;
    unsigned char cc;

    for (; item_at (i)->where == IW_ITEM; i = h) {
        h = item_at (i)->x;
        cc = icomp (i);                         /* (Its compartment, crow its row) */
        q = item_at (h);
        if (cc && CS_UNSEEN (crow.noun, q->fuel, cc)) {
            return 0;
        }
        if (q->shut & IS_SHUT_C (cc)) {
            if (cc ? q->fuel & CS_AT (cc, CS_LOCKED) : q->shut & IS_LOCKED) {
                return 0;                       /* (Locked: left, no lock picked unasked) */
            }
            q->shut &= ~IS_SHUT_C (cc);
            rtime += row_v (AC_LID);
            ++rsteps;
        }
    }
    return 1;
}

/* Body bd robbed by the character beside it (FE_ROBBED: the minds' SW_ROB, one senseless; SW_FILCH, one asleep or
** senseless, purse 1, its coins alone): what it holds but a light lit, and its coins wherever it keeps them that the
** character can get at (got_at's), each carried by the character's own carry (things.c's s_carry: coins its purse, a
** hand, else its pack), else set down at its feet; each told (E_TASK TE_FETCHED FT_STOLEN: "You take the bandit's
** dagger."), none FT_BARE; what's to hand told the referee, the character's and the robbed one's */
void __fastcall__ s_den_robbed (unsigned char purse)
{
    struct item* p;
    unsigned i, x, y;
    unsigned char k, f, n = 0, ok = rs_ok;

    rs_ok = 0;                                  /* (The character's carry by its own rule, no steps of a request's) */
    for (i = 0; (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {
        k = ip->kind;
        f = ip->flags;
        if ((k != KI_PENNIES && k != KI_FARTHINGS && k != KI_CROWNS && (purse || (f & (I_HELD | I_LIT)) != I_HELD)) ||
            !den_own (i, bd.did) || !got_at (i)) {
            continue;                           /* (Not wanted; not its; where the hand can't get at it) */
        }
        p = item_at (i);
        p->flags &= ~(I_HELD | I_WORN | I_LOOT);
        p->owner = p->hide = 0;                 /* (No longer its: the character's now) */
        if (!carry (i, 1)) {
            hub_map (0);
            x = HUB->pc_x;                      /* (No room: at the character's feet) */
            y = HUB->pc_y;
            f = HUB->pc_stratum;
            p = item_at (i);
            p->where = IW_SQUARE;
            p->sn = f;
            p->x = x;
            p->y = y;
            p->flags |= I_KNOWN;
        }
        hub_map (0);
        told (FT_STOLEN, k);
        ++n;
    }
    rs_ok = ok;
    hub_map (0);
    if (!n) {
        told (FT_BARE, 0);
        return;
    }
    equip_now ();                               /* (What's to hand now, and its load: the referee's, the character's) */
    light_now ();
    den_eq_all = 1;
    den_equip ();                               /* (And the robbed one's) */
}

/* Body bd home with its takings (FE_STASH, its mind's: the user's C10, 2026-11-23, a robbery's carried on its body till
** its lair): each thing of its own marked its takings (items.h's I_LOOT, however deep) put down at x, y (I_ROB's w1 w2:
** its lair's home square on its stratum; or, that square not to be had, where it stands), in its faction's hoard there
** (how RB_HOARD, by its leader's home: the hoard's container, OW_HOARD | its faction), hidden there (RB_HIDE, its kind
** hides its things: the conceal row's test by its sheet, its minute, the first thing's hiding all of theirs: HOUSE.md's
** the robbers' takings, the character's own hiding of a thing), else lying there (s_den_rob's before, at the robbery);
** what it holds told the referee after */
void __fastcall__ s_den_stash (const struct msg* q)
{
    struct item* p;
    struct actq aq;
    unsigned i, x, y, box;
    unsigned char how, fac, hd = 0, n = 0, h = 0;
    unsigned char* s;

    x = I_ROB_X (*q);
    y = I_ROB_Y (*q);
    how = I_ROB_KEEP (*q);
    fac = I_ROB_FACTION (*q);
    s = at (bd.sn, x, y);
    if (!s || !(TILE (*s) == T_FLOOR || TILE (*s) == T_CORR) ||
        !it_need (bd.sn, (x - STRATUM (bd.sn).x0) >> CW_BITS, (y - STRATUM (bd.sn).y0) >> CW_BITS)) {
        x = bd.x;
        y = bd.y;
        how = how == RB_HOARD ? RB_HOARD : 0;  /* (Its home no floor, a way's square: its hoard still the one near) */
    }
    for (box = 0, i = 0; how == RB_HOARD && (i = items (i, IN_SQUARE, 0)) != 0; ) {
        if (ip->owner == (OW_HOARD | fac) && ip->sn == bd.sn && (unsigned) (ip->x - x + 16) <= 32 &&
            (unsigned) (ip->y - y + 16) <= 32) {
            box = i;                            /* (Its faction's hoard, by its leader's home: the takings in it) */
            break;
        }
    }
    for (i = 0; (i = items (i, IN_DEN | IN_ITEM, 0)) != 0; ) {
        if (!(ip->flags & I_LOOT) || !own (i)) {
            continue;
        }
        p = item_at (i);
        h |= p->flags & I_HELD;
        p->where = IW_SQUARE;
        p->sn = bd.sn;
        p->x = x;
        p->y = y;
        p->owner = p->hide = 0;
        p->flags &= ~(I_HELD | I_WORN | I_LOOT);
        if (box) {
            p->where = IW_ITEM;
            p->x = box;
            p->y = 0;
        } else if (how == RB_HIDE) {
            if (!hd) {                          /* (Its stash concealed, the first thing put by: the conceal row) */
                act_get (aq, AC_CONCEAL);
                hd = act_test (bk + 1, aq.r.test, aq.r.steps);
                hd = 0x80 | HIDE_OF ((signed char) hd);
                bd.ready += aq.r.time;
                p = item_at (i);
            }
            p->flags = (p->flags & ~I_KNOWN) | I_HIDDEN;   /* (Its stash at its home: found by searching) */
            p->hide = hd & 0x7F;
        }
        ++n;
    }
    hub_map (0);
    if (n) {
        ++HUB->den_seq;
        if (h) {
            den_eq_all = 1;
            den_equip ();                       /* (What it holds now, told the referee) */
        }
    }
}
