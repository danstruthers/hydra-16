/*
** wport.c - (dq0w) one path for every actor (Phase 3C, 2026-11-02: DUNGEON.md's one path for every actor; DECISIONS.md's
** the command interpreter's common structure, the user's of 2026-11-02): a denizen's mind posts the character's own
** intents (I_PORTAL: a its PA_*, w1 w2 the door's square; I_TAKE), and the world carries them out by the character's
** code.  A door worked by body bd (wbody.c's part 14 calls it, the body copied out): the work's parts the character's
** (wdoor.c's doors, wblow.c's blows: work.h's wk, its who the actor's), the character's own work kept round it (a
** battering under way, its blows' count), the referee asked of the actor (its sheet), nothing told the UI by the parts
** but its outcome (wk.out, or an opening's wk.open): a portcullis's and a battering's told here as the denizens' event
** (E_DEN's DE_RAISES ...: seen, by its name; heard, a crash); a door's by wdoor.c itself, every actor's alike (the act
** table's rows, acts.h's act_noise: the one-path plan's stage 1, 2026-11-19).  The timed work as wdig.c's: forcing
** begun (its strain heard, the body busy its ten seconds) and made as its mind posts it again (t 1), the test then; a
** battering a blow a post (t the blows so far, a dozen at most: its mind's).  A denizen opens and shuts doors (not
** windows, nor portcullises: the house's, as yet), lifts and sets a bar on its side, unlocks and locks one with its
** key, forces and batters one; it never springs a door's trap (the house's, for review), nor picks a lock.
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
#include "acts.h"

#pragma local-strings (on)
#pragma code-name ("SW80CODE")
#pragma rodata-name ("SW80RODATA")             /* (No data of its own: its locals on the C stack, as it calls other
                                                **   parts, which copy this one in again after) */

#define FORCE_T         100                     /* (Forcing's ten seconds: wdoor.c's) */
#define KEYS_MAX        2                       /* (A lair's doors' keys, at most) */

unsigned char __fastcall__ on_pc (unsigned i);  /* (Part 10's) */

/* A thing of kind k carried by actor who (0 the character: on it, or in what it carries, on_pc's; else body bd: its
** own, held or in what it carries, IW_DEN at the last): a key only if it fits lock id (its size and thick: a thing's
** its id; 0 the work's door's, DOOR_KEY of wk's sn, px, py).  Its number, or 0.  Every actor's tools by one rule (a door's key, a lid's, lockpicks:
** the one-path plan's stage 2, closures, 2026-11-21; a door's key, s_door_key, and den_pry's own loop, a lid's, before;
** an actor's door key then only held, not in its pouch); the hub's header in the window after */
#pragma code-name (push, "SW69CODE")             /* (Part 69's, the one senders': part 80's room) */
#pragma rodata-name (push, "SW69RODATA")
unsigned __fastcall__ s_tool_of (unsigned char who, unsigned char k, unsigned id)
{
    unsigned i, j;
    unsigned char r = 0;

    if (!id) {
        id = DOOR_KEY (wk.sn, wk.px, wk.py);
    }
    for (i = 0; !r && (i = items (i, IN_ACTOR | IN_ITEM | IN_DEN, 0)) != 0; ) {
        if (ip->kind != k || (k == KI_KEY && (ip->size | ip->thick << 8) != id)) {
            continue;
        }
        if (who) {
            for (j = i; item_at (j)->where == IW_ITEM; j = ip->x) {
            }
            r = ip->where == IW_DEN && ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8;
        } else {
            r = on_pc (i);
        }
    }
    hub_map (0);
    return r ? i : 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)

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

/* Body bd's widest bar to hand (one it carries, or one lying within its reach: its square or beside it, the take
** rule's), not one in a door's brackets: its width, 0 none (the character's own rule, things.c's bar_hand: carried or
** in reach, the widest).  put: that one set in the door's brackets, on bd's square (beside the door, its bar's side:
** IW_PORTAL, as the character's is).  A denizen's bar, the act audit's G-5, 2026-11-21; the hub's header in the window
** after */
static unsigned char __fastcall__ den_bar (unsigned char put)
{
    unsigned i, j, best = 0;
    unsigned char w = 0;

    for (i = 0; (i = items (i, IN_SQUARE | IN_ITEM | IN_DEN, 0)) != 0; ) {
        if (ip->kind != KI_BAR || (ip->size & 15) <= w) {
            continue;
        }
        for (j = i; item_at (j)->where == IW_ITEM; j = ip->x) {
        }
        if (ip->where == IW_DEN ? ip->owner == (unsigned char) bd.did && ip->hide == bd.did >> 8 : ip->sn == bd.sn &&
            apart (ip->x - bd.x, ip->y - bd.y) <= 1) {
            best = i;
            w = item_at (i)->size & 15;
        }
    }
    if (put && best) {
        ip = item_at (best);
        ip->where = IW_PORTAL;
        ip->sn = bd.sn;
        ip->x = bd.x;
        ip->y = bd.y;
        ip->flags &= ~(I_HELD | I_WORN | I_HIDDEN);
    }
    hub_map (0);
    return w;
}

/* Body bd did what (DE_*), how loud (its low six bits): told (E_DEN) at square x, y (the dug one, a door's, or its own),
** whether the character sees it now (the senses' marks: that square's; DT_SEEN, a dug square as it was seen, before it
** changed; DT_DOOR, a door's work, the body's own square's too), what lies between (E_DEN's x[3]: the rock carries a
** pick's knock, whatever the air does, a square of wall for each four between them, the house's; from the body, or
** a door's work's from the door); the hub's header in the window after.  The one sender of a denizen's door work and a
** tunneller's and a builder's (wdig.c's, part 79's, through its stub: each its own copy before, 2026-11-11, narration
** as data's stage 3; here, not part 79: its bank dq0w.rop's, read in only at ropes_in); its event on the C stack */
void __fastcall__ s_den_told (unsigned char what, unsigned char loud, unsigned x, unsigned y)
{
    struct msg m;
    unsigned char* p;
    unsigned e;

    E_DEN_SX (m) = x;
    E_DEN_SY (m) = y;
    e = loud & DT_DOOR ? apart (x - bpx, y - bpy) >> 2 : apart (bd.x - bpx, bd.y - bpy) >> 2;
    E_DEN_BETWEEN (m) = e > 15 ? 15 : e;
    p = at (bd.sn, x, y);
    E_DEN_SEEN (m) = (p && (*p & T_VIS)) || (loud & DT_SEEN);
    if (loud & DT_DOOR) {
        p = at (bd.sn, bd.x, bd.y);
        E_DEN_SEEN (m) |= p && (*p & T_VIS);
    }
    m.kind = E_DEN;
    m.who = bk + 1;
    E_DEN_DE (m) = what;
    E_DEN_LOUD (m) = loud & ~(DT_SEEN | DT_DOOR);
    E_DEN_SN (m) = bd.sn;
    E_DEN_DKIND (m) = bd.kind;
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

/* Body bd's rarer intents (part 14's room): up or down a flight to stratum x[2] (I_CLIMB: part 114's, every actor's,
** the one-path plan's, 2026-11-23); none within its reach, an ally come along with the character, beside it at w1 w2
** (a shaft's climb, a rope's, a fall: not yet any actor's); a light of its own lit or put out (I_LIGHT: part 114's);
** a robbery, a round's work (I_ROB); its door work (I_PORTAL); a lair's doors' keys (I_KIT's KT_LAIR, part 51's: no
** body) */
void __fastcall__ s_den_more (const struct msg* m)
{
    if (m->kind == I_LIGHT) {
        den_light (m);
    } else if (m->kind == I_CLIMB) {
        den_flight (m);                         /* (Part 114's: wflight.c) */
    } else if (m->kind == I_ROB) {
        hub_map (0);
        if (I_ROB_ERRAND (*m) >= FE_DROP) {
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
        if (I_RISE_POS (*m) & POS_HIDE) {
            hid_op (HO_DEN | (I_RISE_POS (*m) & 7));   /* (Hiding oneself: part 94's) */
        } else if (I_RISE_POS (*m) != POS_STANDING) {
            if (I_RISE_POS (*m) == POS_PRONE && dlit[bk]) {
                den_lights (DL_DOUSE);          /* (Lain down to sleep: a rest puts a burning light out first, every
                                                **   actor's, the user's, 2026-11-17: part 114's) */
            }
            bd.pos = I_RISE_POS (*m);
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
    unsigned char d, what = I_PORTAL_ACT (*m), o, was, cut, de = 0, loud = 0, lng, pc, win, shut;
    unsigned done, x = I_PORTAL_X (*m), y = I_PORTAL_Y (*m);

    for (d = 0; d < 4 && (bd.x + dir_x[d] != x || bd.y + dir_y[d] != y); ++d) {
    }
    p = at (bd.sn, x, y);
    q = d < 4 && p && (TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) ? portal_at (cx_, cy_) : 0;
    pc = q && q->kind == PK_PORTCULLIS;         /* (A portcullis: raised, heaved, lowered, its bars bent, 2026-11-02;
                                                **   opened, raised, the timed work the character's is, 2026-11-17) */
    win = q && q->kind != PK_DOOR && !pc;       /* (A window: its shutters a door's, its bars bent, its pane worked
                                                **   out, the character's code (dq0w.c's work's parts, by the same
                                                **   choice), 2026-11-21; climbed through, broken: not yet, the climb's
                                                **   and the blows') */
    shut = q && (q->parts & PP_SHUTTERS) && !(q->state & PS_OPEN);
    if (pc && what == PA_OPEN) {
        what = PA_RAISE;
    }
    if (what == PA_BEND && !pc) {
        what = PA_FORCE;                        /* (A door's, a window's bars bent: forced, s_work's) */
    }
    if (!q || (q->state & PS_HIDDEN) || what == PA_SEARCH || what == PA_DISARM || (!pc && (what == PA_RAISE ||
        what == PA_LOWER)) || what == PA_THROUGH || what == PA_CHECK || (win && (what == PA_BREAK || (what == PA_FORCE &&
        !shut && !(q->parts & PP_BARS))))) {
        hub_map (0);
        bd.ready += 10;                         /* (No door of its beside it: a moment, its mind to see) */
        return;
    }
    was = q->state;
    memcpy (&keep, &wk, sizeof wk);
    hub_map (0);
    done = HUB->pc_task.done;
    cut = act.cut;
    memset (&wk, 0, offsetof (struct work, pq));
    wk.who = bk + 1;
    wk.sn = bd.sn;
    wk.pd = d;
    wk.px = x;
    wk.py = y;
    wk.fx = bd.x;
    wk.fy = bd.y;
    wk.what = what;
    if (what == PA_BAR) {
        wk.link = den_bar (0);                  /* (The widest bar to hand, its width: the character's own rule,
                                                **   things.c's bar_hand; 2026-11-21: wk.link its picking before) */
    }
    lng = what == PA_FORCE || what == PA_RAISE || what == PA_BEND || what == PA_PICK || (win && what == PA_LIFT);   /* (Timed
                                                **   work: begun, then done as it's posted again, t 1) */
    if (lng && !(I_PORTAL_STEP (*m) & 1)) {
        wk.plan = 1;                            /* (Begun: its time, its strain heard; its test as it's posted again) */
    } else if (lng && (int) (HUB->dig_t[bk] - (unsigned) HUB->now) > 0) {
        bd.ready += HUB->dig_t[bk] - (unsigned) HUB->now;   /* (Its time not up: on at it) */
        goto back;
    }
    if (what == PA_BREAK) {                     /* (A blow, the next of its battering: at the door as it is) */
        HUB->pc_task.done = I_PORTAL_STEP (*m);
        act.cut = 0;
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
    } else if (pc || (win && what != PA_BAR && what > PA_UNLOCK && what < PA_PICK && (what != PA_FORCE || !shut))) {
        portals ();                             /* (The character's parts: wwin.c's, by the winch on its side; a
                                                **   window's but its shutters', s_work's choice) */
    } else {
        doors ();
        if (wk.out == PO_REBARRED) {
            den_bar (1);                        /* (Its bar set in the door's brackets) */
        }
    }
    bd.ready += wk.tt > 600 ? 600 : (unsigned) wk.tt;
    if (wk.plan && wk.task) {
        hub_map (0);                            /* (The hub's header: the door's work leaves a chunk's bank in the
                                                **   window) */
        HUB->dig_t[bk] = (unsigned) HUB->now + (unsigned) wk.tt;   /* (When its time's up) */
    }
    o = wk.open ? wk.open : wk.out;
    if (o == PO_BROKE_DOWN) {                   /* (A door's, a portcullis's, a window's work told by their parts,
                                                **   every actor's, by the act table's rows: 2026-11-19, a portcullis's
                                                **   and a window's 2026-11-21; a battering's here) */
        de = DE_BREAKS;
        loud = 8;
    }
    at (bd.sn, x, y);
    if ((q = portal_at (cx_, cy_)) != 0 && q->state != was) {
        den_saw = 1;                            /* (The world to look about again: what the door shows, or hides) */
    }
    if (de) {
        s_den_told (de, loud | DT_DOOR, x, y);    /* (Seen if the door or the body is: the one sender, above) */
    }
back:
    hub_map (0);
    HUB->pc_task.done = done;
    act.cut = cut;
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

    if (!at (I_KIT_SN (*m), I_KIT_X (*m), I_KIT_Y (*m))) {
        hub_map (0);
        return;
    }
    x0 = I_KIT_X (*m) - cx_;                    /* (The chunk's first square) */
    y0 = I_KIT_Y (*m) - cy_;
    for (k = C->nportals, q = C->portal; k && n < KEYS_MAX; --k, ++q) {
        if (q->kind == PK_DOOR && (q->state & PS_LOCKED) && q->lock && !(q->state & PS_HIDDEN)) {
            px[n] = x0 + q->x;
            py[n++] = y0 + q->y;
        }
    }
    for (k = 0; k < n; ++k) {
        if (!(i = lay_body (KI_KEY, I_KIT_SN (*m), I_KIT_X (*m), I_KIT_Y (*m)))) {
            break;
        }
        ip = item_at (0);                       /* (Made its own, as an outfit's things are: its number, its make) */
        id = ++ip->x;
        ip = item_at (i);
        ip->id = id;
        ip->quality = 4;
        ip->mat = MT_IRON;
        ip->dur = ip->dur_max = 60;
        id = DOOR_KEY (I_KIT_SN (*m), px[k], py[k]);
        ip->where = IW_DEN;
        ip->owner = (unsigned char) I_KIT_ID (*m);
        ip->hide = I_KIT_ID (*m) >> 8;
        ip->size = (unsigned char) id;
        ip->thick = id >> 8;
        hub_map (0);
    }
}
