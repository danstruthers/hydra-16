/*
** wfight.c - (dq0w) the blows (DENIZENS.md: fights), every one the world's to strike: the character's at a denizen
** (I_ATTACK, the UI's: a round of its time, the hub's pc_foe) and a denizen's at the character (its intent, I_ATTACK:
** a round of its own) alike.  The referee asked in the world's own slot (RS_ATTACK: the ruleset's melee), its words
** put in the fights' ring in the denizens' events' bank (FIGHT_AT, the world's: the UI words them), what the blow left
** the body in kept (senseless a while, fleeing, holding back a round, bleeding, dead: its remains put down, the item
** store's), and the blow told (E_FIGHT, the denizens' events: the denizens take it to heart, their minds' courage and
** attitudes; the UI words it; the sound task plays it).  The bleeding's blood lost a minute at a time (RS_TIME), and
** those dead of it told.  And a robber's way with the character helpless beside it (I_ROB: what it holds and its
** pennies taken off to its lair).
**
** All of it swapped part 15's (../dq0/swap.inc: dq0w.cfg's SW15), called through its stubs (bank.s's).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

#pragma local-strings (on)

#define LOUD_FIGHT      5

static unsigned char fring;                     /* (The fights' ring's next) */
struct mshot mshot;                             /* (wbody.h's: a blow's missile, wshoot.c's) */
extern unsigned char pshoot;                    /* (wshoot.c's: the character's fight a missile's) */
#pragma bss-name (push, "SW15RODATA")
static char wb[32];                             /* (A blow's words on their way to the ring, a piece at a time: part
                                                **   15's image's, to_ring's alone, no part called as it's used; the
                                                **   main RAM's room, 2026-10-29) */
#pragma bss-name (pop)

#pragma code-name ("SW15CODE")
#pragma rodata-name ("SW15RODATA")

extern unsigned char pc_hit;                    /* (dq0w.c's: a blow at the character stops its long task, hard) */
extern unsigned char fwant, flast;              /* (dq0w.c's fighting on: a blow at it, looked at as it's free; whose) */
unsigned __fastcall__ weight_in (unsigned where, unsigned x);  /* (Part 10's, things.c's: what's on the character
                                                **   weighs, quarter pounds) */

/* A blow told (E_FIGHT): attacker att, defender def (0 the character, else an actor's entry plus 1; body bd the one in
** it), how it came out (O_*, FO_BLED), what it left them in (FF_*); its words the ring's, fring; the ring on */
static void __fastcall__ told (unsigned char att, unsigned char def, unsigned char out, unsigned char f)
{
    hub_map (0);
    if (!def) {
        pc_hit = 1;
        fwant |= 1;                             /* (A hostile one fought, fighting on's, as the character's free) */
        flast = att;
    }
    bev.kind = E_FIGHT;
    bev.who = att;
    bev.a = def;
    bev.b = LOUD_FIGHT;
    bev.w1 = bd.x;
    bev.w2 = bd.y;
    bev.t = HUB->now;
    bev.x[0] = bd.sn;
    bev.x[1] = out;
    bev.x[2] = f;
    bev.x[3] = fring;
    hub_devent (&bev);
    fring = (fring + 1) % NFIGHTS;
}

/* Body bd dead: gone from the dungeon, its remains where it fell (an item of the store's: a body, a skeleton's bones),
** its things on them (the outfits' gear), the world to look about again (the remains seen); its bleeding over */
static void died (void)
{
    bd.state = 0;
    bleeds &= ~(1 << bk);
    den_strip (lay_body (bd.remains, bd.sn, bd.x, bd.y));     /* (Its things on its body: part 51's) */
    den_saw = 1;
}

/* A string's bytes, n at most, from bank 0's s to the ring's record's d (the denizens' events' bank's), through wb,
** as strncpy would (the record cleared first): 1 if its end was among them */
static unsigned char __fastcall__ to_ring (char* d, const char* s, unsigned char n)
{
    static unsigned char j;

    hub_map (0);
    memcpy (wb, s, n);
    for (j = 0; j < n && wb[j]; ++j) {
    }
    hub_map (DEN_BANK);
    memcpy (d, wb, j);
    return j < n;
}

/* The character's body's news told the UI (E_TASK's TE_BODY: a venomous bite's venom, its words the referee's) */
static void news_ev (void)
{
    struct msg m;

    hub_map (0);
    m.kind = E_TASK;
    m.who = m.a = m.b = 0;
    m.w1 = TE_BODY;
    m.t = HUB->now;
    hub_event (&m);
}

/* A blow (att at def: 0 the character, else an actor's entry plus 1; body bd in it), its aim z: the referee asked
** (RS_ATTACK, the world's slot: w2 1 if the creature's shy), its words put in the ring at fring (who struck at whom,
** the actors' names, "" the character; the tenths one's senseless for; whose a fumble or a stumble was; the
** referee's four lines), what it did kept (FF_*: senseless, the time it's out; fleeing; shaken, a round held back;
** bleeding; dead) and told.  0xFF if none was struck (it can't: senseless, dead) */
static unsigned char __fastcall__ blow (unsigned char att, unsigned char def, unsigned char z)
{
    struct req* r;                              /* (On the C stack: the main RAM's room, 2026-10-29) */
    struct fight* f;
    unsigned char ff, c, out, me, wh, n, i, dh, cd, ra;
    unsigned w2;
    unsigned char pa, pd;
    const struct rs_effect* e;

    me = bk + 1;
    ra = att && bd.arm && den_reach ();         /* (Its weapon let fall, in its reach: taken up, its turn, part 51's) */
    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_ATTACK;
    r->actor = att;
    r->other = def;
    r->a = z & ~FW_FIRST;                       /* (Its aim; FW_FIRST, the engage initiative test's: its words') */
    r->b = AS_BEST;
    r->c = 0;
    r->steps = 0;
    r->x[0] = bd.heart;                         /* (The creature's heart: its courage's steps as it's wounded, the
                                                **   denizens' field and bonds, rs.h's RS_HEART) */
    pa = att ? bd.pos : HUB->pc_pos;           /* (Their bodies' positions, the world's: prone, the easier struck) */
    pd = att ? HUB->pc_pos : bd.pos;
    r->w2 = bd.timid | (POS_LOW (pa) ? RS_PRONE_A : 0) | (POS_LOW (pd) ? RS_PRONE_D : 0) | (ra ? RS_REARM : 0);
    if (mshot.kind) {                           /* (A missile's, shot or thrown: wshoot.c's, slice 4.7's) */
        r->c = mshot.kind;
        r->b = mshot.band;
        r->w1 = mshot.dist;
        r->w2 |= RS_MISSILE | mshot.flags;
    }
    r = hub_ask (SLOT_WORLD);
    if ((out = r->result) == 0xFF) {
        r->state = REQ_FREE;
        return 0xFF;
    }
    if (out == O_REARM) {
        r->state = REQ_FREE;
        den_took ();                            /* (Its weapon in hand again, told: part 51's) */
        return 0;
    }
    if (out == O_RISE) {                        /* (The attacker got up, its turn spent; or tried and is still down,
                                                **   a: the limbs' news, no blow's record, slice 4.1b's) */
        if (r->a) {
            r->state = REQ_FREE;
            limb_news (WB_NOT_UP, att);
            return 0;
        }
        pa = POS_STANDING;
    }
    for (cd = dh = i = 0, e = (const struct rs_effect*) r->x; i < RS_EFFECTS && e->kind; ++i, ++e) {
        if (e->kind == X_DROP) {
            if (e->target) {
                cd = 1;                         /* (The creature's weapon let fall: part 51's) */
            } else {
                dh |= e->x[0];                  /* (The character's hands' things let go of, fumbled: slice 4.1b's) */
            }
        }
        if (e->kind == X_WOUND && e->target == me) {
            bd.move = e->cond >> 12 & 7;        /* (How its legs move it now, C_MOVE's: slice 4.1b's) */
        }
        if (e->kind == X_COND_ON && (e->cond & C_PRONE)) {
            if (e->target == att) {             /* (Fallen: a stumble, a leg's.  An if, not *(c ? &pa : &pd): 13
                                                **   bytes the less) */
                pa = POS_PRONE;
            } else {
                pd = POS_PRONE;
            }
        }
    }
    HUB->pc_pos = att ? pd : pa;
    bd.pos = att ? pa : pd;
    if (cd) {
        den_word (r->text, !att);               /* (Its weapon's word, the reply's: part 51's) */
    }
    ff = r->grade;
    c = r->c;
    w2 = r->w2;
    wh = r->a;
    f = FIGHT_AT + fring;                       /* (Its words: the record cleared, then each piece) */
    hub_map (DEN_BANK);
    memset (f, 0, sizeof *f);
    f->out = w2;
    f->which = wh | (z & FW_FIRST) | (c & AF_AGAIN ? FW_AGAIN : 0) | (c & AF_SHIELD ? FW_SHIELD : 0) |
        (!mshot.kind ? 0 : (unsigned char) (mshot.kind - KI_SLING) <= KI_CROSSBOW - KI_SLING ? FW_MISSILE :
        FW_MISSILE | FW_THROWN);
    if (att) {
        to_ring (f->att, HUB->actor[(att - 1) & 7].name, sizeof f->att);
    }
    if (def) {
        to_ring (f->def, HUB->actor[(def - 1) & 7].name, sizeof f->def);
    }
    for (i = 0; i < sizeof f->text - 1; i += n) {
        n = sizeof f->text - 1 - i < sizeof wb ? sizeof f->text - 1 - i : sizeof wb;
        if (to_ring (f->text + i, r->text + i, n)) {
            break;
        }
    }
    hub_map (0);
    r->state = REQ_FREE;
    if (ff & (me == att ? FF_A_DOWN : FF_D_DOWN)) {
        bd.doing = DO_DOWN;                     /* (Senseless: it falls, and lies there till it comes round; up again,
                                                **   its turn, the book's Rise, 2026-10-29) */
        bd.pos = POS_PRONE;
        bd.ready = HUB->now + w2;
    }
    if ((ff & (att ? FF_D_DOWN : FF_A_DOWN)) && HUB->pc_ready < HUB->now + w2) {
        HUB->pc_ready = HUB->now + w2;          /* (The character senseless: busy till it comes round, so nothing's
                                                **   done meanwhile, however the UI's events come) */
    }
    if (ff & FF_FLEES) {
        bd.doing = DO_FLEE;                     /* (Broken: it flees) */
    } else if (c & AF_CAUTIOUS) {
        bd.ready += HUB->round;                 /* (Shaken: it holds back a round) */
    }
    if (c & AF_BLEEDING) {
        bleeds |= 1 << bk;
    }
    told (att, def, out, ff);
    if (c & AF_NEWS) {
        news_ev ();                             /* (A venomous bite: its venom told after it, slice 4.3's) */
    }
    if (dh) {
        drop_held (dh, 1);                      /* (Told after the blow, in the denizens' ring) */
    }
    if (cd) {
        den_drop ();                            /* (Its weapon on the floor, told: part 51's) */
    }
    if (ff & (me == att ? FF_A_DEAD : FF_D_DEAD)) {
        died ();
    }
    return 0;
}

/* The bodies' time (part 70's body_ask: the referee told the minutes gone, the bleeding's bloodloss, the character's
** body's, slice 4.3's); those dead of it told, and those that bound their bleeding (as the UI says, as the character
** sees them: the ring's name theirs) */
void s_bled (void)
{
    unsigned w = body_ask ();
    unsigned char k;

    for (k = 0; k < NACTORS; ++k) {
        if (w & 0x101 << k) {
            BD_GET (k);
            hub_map (DEN_BANK);
            strcpy (FIGHT_AT[fring].att, bd.name);
            if (w & 1 << k) {
                told (k + 1, 0, FO_BLED, FF_A_DEAD);
                died ();
            } else {
                told (k + 1, 0, FO_BOUND, 0);
                bleeds &= ~(1 << k);
            }
            BD_PUT ();
            ++HUB->den_seq;
        }
    }
    hub_map (0);
}

/* A blow struck by body bd (entry bk) at def (0 the character), its aim z (its intent, I_ATTACK: part 14's); a shot,
** AIM_SHOT, part 65's (slice 4.7's) */
void __fastcall__ s_den_blow (unsigned char def, unsigned char z)
{
    if (z & AIM_SHOT) {
        shot_op (SO_DEN, def, z & ~AIM_SHOT);
        return;
    }
    if (z & AIM_FIRE) {                         /* (A missile's blow, mshot set: wshoot.c's, part 65's; the character's
                                                **   at actor def, else body bd's at the character; how it came out
                                                **   bev's, told: 0xFF none) */
        bev.x[1] = 0xFF;
        blow (def ? 0 : bk + 1, def, z & ~AIM_FIRE);
        mshot.kind = 0;
        return;
    }
    hub_map (0);
    if (HUB->agear[bk] & AG_SHOOTS) {
        shot_op (SO_UNBOW, 0, 0);               /* (A launcher in its hands let fall first: part 65's, asked only of one
                                                **   with a launcher, the part's copy the dearer) */
    }
    if (!den_draw ()) {                         /* (Its weapon drawn first, its hands empty: its round's, part 60's) */
        blow (bk + 1, def, z);
    }
}


/* The character's blow at the actor it's posted (the hub's pc_foe, I_ATTACK's: a round of its time), struck if the
** actor's still beside it (gone, nothing: the UI found it there) */
void s_pc_blow (void)
{
    unsigned char k;

    hub_map (0);
    k = (HUB->pc_foe - 1) & 7;
    HUB->pc_foe = 0;
    if (pshoot) {
        pc_shot (k);                            /* (A shot, fought on as a blow is: part 67's) */
        return;
    }
    BD_GET (k);
    if (!bd.state || bd.sn != bps || (unsigned) (bd.x - bpx + 1) > 2 || (unsigned) (bd.y - bpy + 1) > 2 ||
        blow (0, k + 1, HUB->pc_aim) == 0xFF) {
        return;
    }
    BD_PUT ();
    ++HUB->den_seq;                             /* (Changed: the world looks again at what the character sees) */
}

/* ==== Part 67's (part 15's room, 2026-10-29: the missiles' part, wshoot.c's, dq0w.cfg's SW67): a robbery */
#pragma code-name ("SW67CODE")
#pragma rodata-name ("SW67RODATA")

static struct item* p;

/* The thing at hand (ip) on the character, however deep (p, what holds it at the last) */
static unsigned char mine (void)
{
    for (p = ip; p->where == IW_ITEM; p = item_at (p->x)) {
    }
    return p->where == IW_ACTOR && !p->x;
}

/* The character robbed by body bd (its intent, I_ROB: part 14's; a robber's, kinds.txt's ways r, the character
** helpless beside it): what it holds (but a light lit: the robber leaves it its light) and its pennies (wherever it
** keeps them), taken off to x, y (the robber's lair's home square, on its stratum; or, that square not to be had,
** where the robber stands) and put down there, a stash to be found; the referee told at once what's to hand now and
** the load left (RS_EQUIP 2, as the UI's to_hand tells it a thing handled: the keys of the tools it still carries, each
** after its mark and with its quality, things.c's tools; their weight, weight_in's: the user's, 2026-10-22, "Weight
** should be recounted immediately after a robbery"), the UI the robbery (E_TASK TE_ROBBED, b the robber plus 1), if
** anything was taken */
#define WSLOT           (HUB->slot[SLOT_WORLD])
void __fastcall__ s_den_rob (const struct msg* q)
{
    struct msg m;                               /* (On the C stack: the main RAM's room, 2026-10-29) */
    unsigned i, n, x, y, box;
    unsigned char kind, f, how, fac;
    unsigned char* s;

    x = q->w1;
    y = q->w2;
    how = q->x[2];
    fac = q->b;
    s = at (bd.sn, x, y);
    if (!s || !(TILE (*s) == T_FLOOR || TILE (*s) == T_CORR) ||
        !it_need (bd.sn, (x - STRATUM (bd.sn).x0) >> CW_BITS, (y - STRATUM (bd.sn).y0) >> CW_BITS)) {
        x = bd.x;
        y = bd.y;
        how = how == RB_HOARD ? RB_HOARD : 0;  /* (Its home no floor, a way's square: its hoard still the one
                                                **   near, 2026-10-29) */
    }
    for (box = 0, i = 0; how == RB_HOARD && (i = items (i, IN_SQUARE, 0)) != 0; ) {
        if (ip->owner == (OW_HOARD | fac) && ip->sn == bd.sn && (unsigned) (ip->x - x + 16) <= 32 &&
            (unsigned) (ip->y - y + 16) <= 32) {
            box = i;                            /* (Its faction's hoard, by its leader's home: the takings in it,
                                                **   2026-10-27) */
            break;
        }
    }
    for (n = 0, i = 0; (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
        kind = ip->kind;
        f = ip->flags;
        if (!mine () || (kind != KI_PENNIES && ((f & (I_HELD | I_LIT)) != I_HELD))) {
            continue;
        }
        reach_from (i);                         /* (Its reach, the robber's: HOUSE.md's, every actor's) */
        p = item_at (i);
        p->where = IW_SQUARE;
        p->sn = bd.sn;
        p->x = x;
        p->y = y;
        p->flags &= ~(I_HELD | I_WORN);
        if (box) {
            p->where = IW_ITEM;
            p->x = box;
        } else if (how == RB_HIDE) {
            p->flags = (p->flags & ~I_KNOWN) | I_HIDDEN;   /* (Its stash at its home: found by searching) */
            p->hide = 1;
        }
        ++n;
    }
    kind = rsteps + rbody - (roff != 0);        /* (Its rounds: a step's each, the round's own the first; a pack taken
                                                **   off, not put on again) */
    hub_map (0);
    if (kind > 1) {
        bd.ready += (kind - 1) * HUB->round;
    }
    rtime = roff = 0;
    rsteps = rbody = 0;
    if (n) {
        equip_now ();                           /* (What's to hand now, and its load: the referee's, at once, part
                                                **   49's) */
        hub_map (0);
    m.kind = E_TASK;
        m.who = 0;
        m.b = bk + 1;
        m.w1 = TE_ROBBED;
        m.t = HUB->now;
        hub_event (&m);
        ++HUB->den_seq;
    }
}
