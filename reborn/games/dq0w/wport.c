/*
** wport.c - (dq0w) one path for every actor (Phase 3C, 2026-11-02: DUNGEON.md's one path for every actor; DECISIONS.md's
** the command interpreter's common structure, the user's of 2026-11-02): a denizen's mind posts the character's own
** intents (I_PORTAL: a its PA_*, w1 w2 the door's square; I_TAKE), and the world carries them out by the character's
** code.  A door worked by body bd (wbody.c's part 14 calls it, the body copied out): the work's parts the character's
** (wdoor.c's doors, wblow.c's blows: work.h's wk, its who the actor's), the character's own work kept round it (a
** battering under way, its blows' count), the referee asked of the actor (its sheet), nothing told the UI by the parts
** but its outcome (wk.out, or an opening's wk.open): told here as the denizens' event (E_DEN's DE_OPENS ...: seen, by
** its name; heard, a door opening, a crash).  The timed work as wdig.c's: forcing begun (its strain heard, the body
** busy its ten seconds) and made as its mind posts it again (t 1), the test then; a battering a blow a post (t the
** blows so far, a dozen at most: its mind's).  A denizen opens and shuts doors (not windows, nor portcullises: the
** house's, as yet), lifts and sets a bar on its side, unlocks and locks one with its key, forces and batters one; it
** never springs a door's trap (the house's, for review), nor picks a lock.
**
** And the doors' keys, every actor's (HOUSE.md's keys that fit): a locked door's key fits it alone (DOOR_KEY, its
** stratum and square in the key's size and thick); each locked door of a lair's chunk has one, made with the lair's
** own things (I_KIT's KT_LAIR, with its first denizen: the key its own, on it); door_key looks for one on the work's
** actor (the character, or body bd).
**
** All of it swapped part 80's (../dq0/swap.inc: dq0w.cfg's SW80), called through its stubs (bank.s's).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "work.h"
#include "wbody.h"
#include "things.h"

#pragma local-strings (on)
#pragma code-name ("SW80CODE")
#pragma rodata-name ("SW80RODATA")             /* (No data of its own: its locals on the C stack, as it calls other
                                                **   parts, which copy this one in again after) */

#define FORCE_T         100                     /* (Forcing's ten seconds: wdoor.c's) */
#define KEYS_MAX        2                       /* (A lair's doors' keys, at most) */

unsigned char __fastcall__ on_pc (unsigned i);  /* (Part 10's) */

static unsigned __fastcall__ apart (int ax, int ay)
{
    if (ax < 0) {
        ax = -ax;
    }
    if (ay < 0) {
        ay = -ay;
    }
    return ax > ay ? ax : ay;
}

/* A key that fits the door the work's on (wk's sn, px, py) carried by the work's actor (wk.who: the character, on it
** or in what it carries; else body bd's own, IW_DEN): its number, or 0 */
unsigned s_door_key (void)
{
    unsigned i, id = DOOR_KEY (wk.sn, wk.px, wk.py);
    unsigned char r = 0;

    for (i = 0; !r && (i = items (i, IN_ACTOR | IN_ITEM | IN_DEN, 0)) != 0; ) {
        if (ip->kind != KI_KEY || (ip->size | ip->thick << 8) != id) {
            continue;
        }
        if (wk.who) {
            r = ip->where == IW_DEN && ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8;
        } else {
            r = on_pc (i);
        }
    }
    hub_map (0);
    return r ? i : 0;
}

/* What body bd did at the door, by its outcome (PO_NONE ... PO_HELD): its DE_* (less DE_OPENS), | how loud times 16;
** 0 nothing done (a refusal: its mind's to see in the door's state) */
#define DE(d, loud)     ((d) - DE_OPENS | (loud) << 4)
static const unsigned char de_of[PO_HELD + 1] = {
    0, DE (DE_OPENS, 2), DE (DE_SHUTS, 3), 0, 0, 0, 0, DE (DE_UNBARS, 3), 0, 0, 0, 0, DE (DE_BURSTS, 7),
    DE (DE_HOLDS, 7)
};

/* Body bd's event (DE_* what, how loud), at the door's square x, y (E_DEN: seen if the door or the body is; what lies
** between for those who hear it, as a tunneller's: wdig.c's) */
static void __fastcall__ told (unsigned char what, unsigned char loud, unsigned x, unsigned y)
{
    struct msg m;
    unsigned char* p;
    unsigned e;

    m.kind = E_DEN;
    m.who = bk + 1;
    m.a = what;
    m.b = loud;
    m.w1 = x;
    m.w2 = y;
    e = apart (x - bpx, y - bpy) >> 2;
    m.x[3] = e > 15 ? 15 : e;
    p = at (bd.sn, x, y);
    m.x[2] = p && (*p & T_VIS);
    p = at (bd.sn, bd.x, bd.y);
    m.x[2] |= p && (*p & T_VIS);
    m.x[0] = bd.sn;
    m.x[1] = bd.kind;
    hub_map (0);
    m.t = HUB->now;
    hub_devent (&m);
}

/* Anyone but body bk on square x, y: the character, where its move ends, another body (wbody.c's) */
static unsigned char __fastcall__ taken (unsigned x, unsigned y)
{
    struct actor* q = HUB->actor;
    unsigned char k;

    hub_map (0);
    if ((x == bpx && y == bpy) || (x == HUB->pc_tx && y == HUB->pc_ty)) {
        return 1;
    }
    for (k = 0; k < NACTORS; ++k, ++q) {
        if (k != bk && q->state && q->x == x && q->y == y) {
            return 1;
        }
    }
    return 0;
}

static void __fastcall__ den_portal (const struct msg* m);
static void __fastcall__ door_keys (const struct msg* m);

/* Body bd's rarer intents (part 14's room): an ally come along with the character, beside it (I_CLIMB: w1 w2 x[2]);
** a robbery, a round's work (I_ROB); its door work (I_PORTAL); a lair's doors' keys (I_KIT's KT_LAIR, part 51's: no
** body) */
void __fastcall__ s_den_more (const struct msg* m)
{
    unsigned char* p;

    if (m->kind == I_CLIMB) {
        p = at (m->x[2], m->w1, m->w2);
        if (p && (TILE (*p) == T_FLOOR || TILE (*p) == T_CORR || TILE (*p) == T_STEPS) && !taken (m->w1, m->w2)) {
            bd.sn = m->x[2];
            bd.x = m->w1;
            bd.y = m->w2;
            bd.ready = HUB->now + 10;
        }
        hub_map (0);
    } else if (m->kind == I_ROB) {
        hub_map (0);
        if (m->a >= FE_DROP) {
            den_take (m);                       /* (The things' rest, by the character's rules, their own time: part 81's) */
        } else {
            bd.ready += HUB->round;
            den_rob (m);
        }
    } else if (m->kind == I_KIT) {
        door_keys (m);
    } else if (m->kind == I_TAKE) {
        den_take (m);                           /* (A thing taken up by the take rule: part 72's) */
    } else if (m->kind == I_RISE) {             /* (Its position, its mind's: lain down to sleep a moment's; up, the
                                                **   Rise's round, wlimbs.c's den_legs: 2026-11-02) */
        if (m->w1 & POS_HIDE) {
            hid_op (HO_DEN | (m->w1 & 7));     /* (Hiding oneself: part 94's) */
        } else if (m->w1 != POS_STANDING) {
            bd.pos = m->w1;
            bd.ready += 10;
        } else if (bd.pos) {
            den_legs ();
        }
    } else {
        den_portal (m);
    }
}

/* Body bd's door work (its intent, m: I_PORTAL's a PA_*, w1 w2 the door's square, beside it, the four ways; t 1 a
** forcing's end, or a battering's blows so far), by the character's parts; its time its own (ready) */
static void __fastcall__ den_portal (const struct msg* m)
{
    struct work keep;                           /* (The character's work, kept round it: a battering under way) */
    struct portal* q;
    unsigned char* p;
    unsigned char d, what = m->a, o, was, cut, de = 0, loud = 0, lng, pc;
    unsigned done, x = m->w1, y = m->w2;

    for (d = 0; d < 4 && (bd.x + dir_x[d] != x || bd.y + dir_y[d] != y); ++d) {
    }
    p = at (bd.sn, x, y);
    q = d < 4 && p && TILE (*p) == T_DOOR ? portal_at (cx_, cy_) : 0;
    pc = q && q->kind == PK_PORTCULLIS;         /* (A portcullis: raised, heaved, lowered, its bars bent, 2026-11-02) */
    if (!q || (q->kind != PK_DOOR && !pc) || (q->state & PS_HIDDEN) || what == PA_SEARCH || what == PA_DISARM ||
        (!pc && (what == PA_RAISE || what == PA_LOWER || what == PA_BEND)) || what == PA_THROUGH || what >= PA_CHECK) {
        hub_map (0);
        bd.ready += 10;                         /* (No door of its beside it: a moment, its mind to see) */
        return;
    }
    was = q->state;
    memcpy (&keep, &wk, sizeof wk);
    hub_map (0);
    done = HUB->pc_task.done;
    cut = pc_cut;
    memset (&wk, 0, offsetof (struct work, pq));
    wk.who = bk + 1;
    wk.sn = bd.sn;
    wk.pd = d;
    wk.px = x;
    wk.py = y;
    wk.fx = bd.x;
    wk.fy = bd.y;
    wk.what = what;
    wk.link = (unsigned char) m->t >> 1;    /* (PA_UNLOCK's: it picks locks, its mind's: wdoor.c's) */
    lng = what == PA_FORCE || what == PA_RAISE || what == PA_BEND || (what == PA_UNLOCK && wk.link);   /* (Timed
                                                **   work: begun, then done as it's posted again, t 1) */
    if (lng && !(m->t & 1)) {
        wk.plan = 1;                            /* (Begun: its time, its strain heard; its test as it's posted again) */
    } else if (lng && (int) (HUB->dig_t[bk] - (unsigned) HUB->now) > 0) {
        bd.ready += HUB->dig_t[bk] - (unsigned) HUB->now;   /* (Its time not up: on at it) */
        goto back;
    }
    if (what == PA_BREAK) {                     /* (A blow, the next of its battering: at the door as it is) */
        HUB->pc_task.done = m->t;
        pc_cut = 0;
        at (bd.sn, x, y);
        wk.pq = *portal_at (cx_, cy_);
        wk.was = 0xFF;                          /* (Four that make no mark, given up: its mind's count) */
        wk.what = W_BLOW;
        blows ();
        if (wk.what == W_GONE) {
            blows ();                           /* (Given way: broken down, as the character's is as it's next free) */
        }
        if (wk.what == W_BLOW) {
            de = DE_BATTERS;
            loud = 7;
        }
    } else if (pc) {
        portals ();                             /* (The character's parts: wwin.c's, by the winch on its side) */
    } else {
        doors ();
    }
    bd.ready += wk.tt > 600 ? 600 : (unsigned) wk.tt;
    if (wk.plan && wk.task) {
        HUB->dig_t[bk] = (unsigned) HUB->now + (unsigned) wk.tt;   /* (When its time's up) */
    }
    o = wk.open ? wk.open : wk.out;
    if (wk.plan && wk.task) {
        de = what == PA_FORCE ? DE_STRAINS : what == PA_BEND ? DE_BENDS : what == PA_UNLOCK ? DE_PICKING :
            wk.pq.bar == ((d + 2) & 3) ? DE_WINDS : DE_HEAVES;     /* (Forcing, bending, picking, winding, heaving
                                                **   begun: loud, but a pick's scratching) */
        loud = what == PA_UNLOCK ? 1 : 7;
    } else if (o == PO_RAISED || o == PO_HEAVED || o == PO_LOWERED || o == PO_BENT || o == PO_PICKED) {
        de = o == PO_LOWERED ? DE_LOWERS : o == PO_BENT ? DE_BENT : o == PO_PICKED ? DE_PICKS : DE_RAISES;
        loud = o == PO_PICKED ? 1 : 8;
    } else if (o == PO_BROKE_DOWN) {
        de = DE_BREAKS;
        loud = 8;
    } else if (o == PO_KEYED || o == PO_KEY_LOCKED) {
        de = o == PO_KEYED ? DE_UNLOCKS : DE_LOCKS;
        loud = 1;
    } else if (o == PO_LIFTED || o == PO_REBARRED) {
        de = o == PO_LIFTED ? DE_UNBARS : DE_BARS;
        loud = 3;
    } else if (o <= PO_HELD && de_of[o]) {
        de = (de_of[o] & 15) + DE_OPENS;
        loud = de_of[o] >> 4;
        if (de == DE_SHUTS && bd.doing == DO_FLEE) {
            de = DE_SLAMS;                      /* (Shut behind it as it flees: slammed, louder) */
            loud = 5;
        }
    }
    at (bd.sn, x, y);
    if ((q = portal_at (cx_, cy_)) != 0 && q->state != was) {
        den_saw = 1;                            /* (The world to look about again: what the door shows, or hides) */
    }
    if (de) {
        told (de, loud, x, y);
    }
back:
    hub_map (0);
    HUB->pc_task.done = done;
    pc_cut = cut;
    memcpy (&wk, &keep, sizeof wk);
}

/* The keys of a lair's locked doors (I_KIT's KT_LAIR: m's t its first denizen's number, w1 w2 its lair's home
** square, x[2] its stratum): each locked door of the home's chunk (a lock it can be locked by) a key (KEYS_MAX at
** most), that denizen's own (IW_DEN, filed at its home) */
static void __fastcall__ door_keys (const struct msg* m)
{
    unsigned px[KEYS_MAX], py[KEYS_MAX];
    struct portal* q;
    unsigned i, x0, y0, id;
    unsigned char n = 0, k;

    if (!at (m->x[2], m->w1, m->w2)) {
        hub_map (0);
        return;
    }
    x0 = m->w1 - cx_;                           /* (The chunk's first square) */
    y0 = m->w2 - cy_;
    for (k = C->nportals, q = C->portal; k && n < KEYS_MAX; --k, ++q) {
        if (q->kind == PK_DOOR && (q->state & PS_LOCKED) && q->lock && !(q->state & PS_HIDDEN)) {
            px[n] = x0 + q->x;
            py[n++] = y0 + q->y;
        }
    }
    for (k = 0; k < n; ++k) {
        if (!(i = lay_body (KI_KEY, m->x[2], m->w1, m->w2))) {
            break;
        }
        ip = item_at (0);                       /* (Made its own, as an outfit's things are: its number, its make) */
        id = ++ip->x;
        ip = item_at (i);
        ip->id = id;
        ip->quality = 4;
        ip->mat = MT_IRON;
        ip->dur = ip->dur_max = 60;
        id = DOOR_KEY (m->x[2], px[k], py[k]);
        ip->where = IW_DEN;
        ip->owner = (unsigned char) m->t;
        ip->hide = m->t >> 8;
        ip->size = (unsigned char) id;
        ip->thick = id >> 8;
        hub_map (0);
    }
}
