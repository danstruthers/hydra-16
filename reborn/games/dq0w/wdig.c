/*
** wdig.c - (dq0w) a tunneller's dig (slice 4.11, 2026-11-01: DENIZENS.md's tunnellers and builders; HOUSE.md's
** tunnellers, for review): an actor's way dug through rock, a wall or rubble by the character's rules (dq0w.c's
** digging, parts 1, 19 and 25): a spell of work at a time (ten minutes), the feet it makes the tool's rate on what it's
** into (the house's digger table, rshouse.h's: a pick, a shovel; a burrower's own claws, the house's below), anything
** harder than earth tested (the referee's RS_TEST P_DIG, the actor's: its grade by its sheet), the rock's hardness the
** rate's (rock.h's, every ruleset's: part 1 asked the rock, dig_on's DIG_ROCK); kept as it goes (the hole's change
** record, DUG, the character's: either can carry on what the other began), loud (its scraping heard, E_DEN's DE_DIG,
** the rock carrying it where the air doesn't); and through, the square a passage (a diagonal's corners cut, as the
** character's), told (DE_DUG: "A fresh tunnel opens ..."), the world looking about again.
**
** The denizens' task's minds choose where (dq0d.c's dig_way: I_KIT's KT_DIG, who the actor plus 1, b the heading,
** x[2] its tools, DG_*); its spell begun then (the hub's dig_t), its feet reckoned as it's done, when its mind asks
** again (the same way, its doing still DO_DIG: a spell cut short by another intent, a step, a blow, is lost, as the
** character's part spell is; a rouse, back to it, the spell's rest).
**
** All of it swapped part 79's (../dq0/swap.inc: dq0w.cfg's SW79), called through its stub (bank.s's den_dig) from
** part 51's den_other, I_KIT's.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rock.h"
#include "world.h"
#include "work.h"
#include "wbody.h"

#pragma local-strings (on)
#pragma code-name ("SW79CODE")
#pragma rodata-name ("SW79RODATA")             /* (No data of its own: its locals on the C stack, as it calls other
                                                **   parts, which copy this one in again after) */

#define SPELL           6000                    /* Tenths: ten minutes' work (dq0w.c's) */
#define DIG_LOUD        6                       /* A spell's noise (the character's, dq0w.c's noise (6)) */
#define DUG_LOUD        7                       /* The way through: the spoil falling */

#include "../dq0/rshouse.h"                   /* (Its rates alone: RS_DIG_PICK, RS_DIG_SHOVEL) */
#define DIG_TESTED(m)   ((m) != MAT_EARTH && (m) != MAT_SOIL)   /* (As rshouse.h's: earth dug at the tool's rate) */

unsigned char __fastcall__ dig_kept (unsigned char begin);   /* (Part 19's: a DUG record looked up, rec's) */
extern struct change rec;                       /* (store.c's) */

static const unsigned char rk_hard[NRK] = RK_HARDNESS;
static const unsigned char rk_quarters[4] = RK_QUARTERS;
static const unsigned char pick[5] = RS_DIG_PICK, spade[5] = RS_DIG_SHOVEL;
static const unsigned char claws[5] = { 2, 0, 0, 0, 4 };    /* (A burrower's claws, half-feet a spell (earth, stone,
                                                **   masonry, flags, soil): two thirds of a shovel's in soil and
                                                **   earth, the house's; no rock, shale too hard for them, the
                                                **   user's, 2026-11-03: "Soft rock only") */

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

/* Body bd did what (DE_DIG, DE_DIGS, DE_DUG), how loud: told (E_DEN) at the square dx, dy's (the dug one) or its own, whether
** the character sees it now (the senses' marks), what lies between (E_DEN's x[3]: the rock carries a pick's knock,
** whatever the air does, a square of wall for each four between them, the house's); the hub's header in the window
** after */
static void __fastcall__ told (unsigned char what, unsigned char loud, unsigned dx, unsigned dy)
{
    unsigned char* p;
    unsigned e;

    bev.w1 = what == DE_DUG || what >= DE_HANGS ? dx : bd.x;
    bev.w2 = what == DE_DUG || what >= DE_HANGS ? dy : bd.y;
    e = apart (bd.x - bpx, bd.y - bpy) >> 2;
    bev.x[3] = e > 15 ? 15 : e;
    p = at (bd.sn, bev.w1, bev.w2);
    bev.x[2] = (p && (*p & T_VIS)) || (loud & 0x80);   /* (Seen: a dug square as it was seen, before it changed) */
    loud &= 0x7F;
    bev.kind = E_DEN;
    bev.who = bk + 1;
    bev.a = what;
    bev.b = loud;
    bev.x[0] = bd.sn;
    bev.x[1] = bd.kind;
    hub_map (0);
    bev.t = HUB->now;
    hub_devent (&bev);
}

/* A diagonal's corner, square x, y, cut (T_CUT) if it's rock or a wall: the way through walked corner to corner (as
** the character's, dq0w.c's cut_corner) */
static void __fastcall__ cut_corner (unsigned x, unsigned y)
{
    unsigned char* p = at (bd.sn, x, y);

    if (p && (TILE (*p) == T_ROCK || TILE (*p) == T_WALL)) {
        change (bd.sn, x, y, T_CUT, C->elev[cy_][cx_], 0xFF);
    }
}

/* A builder's work (I_KIT's KT_BUILD, slice 4.11's rest: HOUSE.md's builders, for review): at m's w1, w2, a door broken
** down put up again, a spell's work (the house's: its frame stands, a new leaf hung in it, shut, its lock and bar
** whole; the change records the portal's own, PORTAL_STATE, PORTAL_HARM), or a way dug walled up (the square a wall,
** an hour's work); an actor's told (DE_HANGS: "The dwarf miner hangs a new door.", heard hammering; DE_WALLS: "The
** dwarf miner walls up the way.", heard stones being laid), a record's not (the character's never near) */
static void __fastcall__ build (const struct msg* m)
{
    struct portal* q;
    unsigned x = m->w1, y = m->w2;
    unsigned char pt, *p;

    if ((p = at (bd.sn, x, y)) != 0 && TILE (*p) == T_CORR) {
        change (bd.sn, x, y, T_WALL, C->elev[cy_][cx_], 0xFF);
        if (m->who) {
            bd.ready += 6 * (unsigned long) SPELL;
            den_saw = 1;
            told (DE_WALLS, 4, x, y);
        }
    } else if (p && (q = portal_at (cx_, cy_)) != 0 && (q->state & PS_WRECKED)) {
        pt = q->parts;                          /* (Read while its chunk's in the window) */
        change (bd.sn, x, y, 0, pt, PORTAL_STATE);
        change (bd.sn, x, y, 0, 0, PORTAL_HARM);
        if (m->who) {
            bd.ready += SPELL;
            den_saw = 1;                        /* (The world to look about again: the door shut) */
            told (DE_HANGS, 4, x, y);
        }
    }
}

/* Body m->who's dig (I_KIT's KT_DIG): begun, back to it, a spell's feet reckoned and on, or through.  Who 0, a record's
** (one away from the character, not an actor: the denizens' roam, at its lair's dig, slice 4.11's rest): a spell's
** feet at once, coarse (w1 w2 its stand, x[2] its stratum, x[0] its tools), untested (a marginal success's rate, the
** house's), untold (the character's never near: those near are actors); the hole kept and the way through made as an
** actor's are */
void __fastcall__ s_den_dig (const struct msg* m)
{
    unsigned char h, t, k, mat, rk, rate, r1, made, way, need, done, rd, roof = 0;
    signed char g;
    unsigned now, ago = 0, dx, dy;
    unsigned char* p;
    struct req* r;
    int z;
    struct dig keep;                            /* (The character's dig, kept round part 1's rock) */

    now = (unsigned) HUB->now;
    h = m->b & 7;
    if ((rd = !m->who) != 0) {
        bd.sn = m->x[2];                        /* (Body bd but a copy, a record's: none put back, I_TROD's way; w1
                                                **   w2 its face, its stand the square before it) */
        bd.x = m->w1 - head_x[h];
        bd.y = m->w2 - head_y[h];
        r1 = m->x[0];
    } else {
        BD_GET ((m->who - 1) & 7);
        if (!bd.state || (bd.doing == DO_DOWN && bd.ready > HUB->now)) {
            return;                             /* (Gone, or senseless: nothing) */
        }
        ago = now - HUB->dig_t[bk];
        bd.ready = HUB->now;
        r1 = m->x[2];                           /* (What it digs with: the best of them for what it's into) */
    }
    if ((m->a & KT_BUILD) == KT_BUILD) {
        build (m);                              /* (A door put up again, a builder's) */
        goto put;
    }
    dx = bd.x + head_x[h];
    dy = bd.y + head_y[h];
    if ((k = room_of (bd.sn, bd.x, bd.y)) == 0xFF) {    /* (A wall's room: its side's, or the far side's) */
        k = room_of (bd.sn, dx + head_x[h], dy + head_y[h]);
    }
    p = at (bd.sn, dx, dy);
    t = p ? TILE (*p) : T_PIT;
    way = p && (*p & T_VIS) ? 0x80 : 0;         /* (Seen as it is, before a change: DE_DUG's) */
    mat = t == T_RUBBLE ? MAT_SOIL : C->mat[t == T_WALL && k != 0xFF ? k : NROOMS][MP_WALL];
    rk = RK_NONE;
    if (mat == MAT_STONE) {                     /* (The rock there, at the height of its middle, as the character's:
                                                **   part 1's, the character's dig kept round it) */
        z = -3 - elev_at (bd.sn, bd.x, bd.y) - STRATUM (bd.sn).datum;
        memcpy (&keep, &dig, sizeof dig);
        dig.x = dx;
        dig.y = dy;
        dig.need = z;
        dig.rock = DIG_ROCK;
        dig_on ();
        rk = dig.rock;
        memcpy (&dig, &keep, sizeof dig);
    }
    rate = r1 & DG_CLAWS ? claws[mat] : 0;
    if ((r1 & DG_PICK) && pick[mat] > rate) {
        rate = pick[mat];
    }
    if ((r1 & DG_SPADE) && spade[mat] > rate) {
        rate = spade[mat];
    }
    hub_map (0);
    if ((t != T_WALL && t != T_ROCK && t != T_RUBBLE) || !rate) {
        bd.doing = DO_ROAM;                     /* (Nothing to dig there, or nothing it has bites into it: it gives it
                                                **   up a while) */
        bd.ready += SPELL / 2;
        goto put;
    }
    rk |= way;                                  /* (Its rock's kind, and whether the square's seen: told's) */
    way = (h & 1 ? 6 : 0) + (h >> 1);           /* (The character's dig's way, DUG's: DIR_N ... DIR_W, a diagonal 6
                                                **   to 9) */
    need = t == T_WALL || t == T_RUBBLE ? 4 : h & 1 ? 14 : 10;
    if (rd) {
        goto spell;
    }
    if (bd.doing != DO_DIG || bd.face != h) {
        bd.doing = DO_DIG;                      /* (Begun: a spell, its noise) */
        bd.face = h;
        HUB->dig_t[bk] = now;
        bd.ready += SPELL;
        told (DE_DIG, DIG_LOUD, dx, dy);
        goto put;
    }
    if (ago < SPELL) {
        bd.ready += SPELL - ago;                /* (Roused, and back to it: the spell's rest) */
        goto put;
    }
spell:
    rec.sn = bd.sn;                             /* (A spell done: the hole as it was, the character's or another's) */
    rec.x = dx;
    rec.y = dy;
    rec.link = DUG;
    done = dig_kept (2) && rec.elev == way && rec.tile < need ? rec.tile : 0;
    g = G_MS;
    if (!rd && DIG_TESTED (mat)) {
        hub_map (0);
        r = &HUB->slot[SLOT_WORLD];             /* (The referee's: the actor's own test, by its sheet) */
        r->kind = RS_TEST;
        r->actor = bk + 1;
        r->a = P_DIG;
        r->b = mat;
        r->steps = 0;
        r = hub_ask (SLOT_WORLD);
        g = r->grade;
        r->state = REQ_FREE;
    }
    made = g == G_CS ? 2 * rate : g == G_MS ? rate : g == G_MF ? rate >> 1 : 0;
    if (rk & 0x7F) {
        made = (unsigned char) (made * rk_quarters[rk_hard[rk & 0x7F]] + 3) >> 2;    /* (The rock's hardness: the rate in it,
                                                **   every ruleset's) */
    }
    if (g == G_CF && RK_POOR (rk & 0x7F) && done) {
        done -= done > 4 ? 4 : done;            /* (Poor rock: the roof come down on a spell gone badly, two feet of the
                                                **   hole filled again, as the character's; the rock showering the
                                                **   tunneller, told: no harm, the referees keep the character's sheet
                                                **   alone) */
        roof = 1;
    }
    done += made;
    hub_map (0);
    if (done < need) {
        rec.tile = done;                        /* (Kept: the hole as it's dug, DUG, the character's way's: its
                                                **   square's as looked up) */
        rec.elev = way;
        store_put ();
        hub_map (0);
        if (rd) {
            goto put;
        }
        HUB->dig_t[bk] = now;                   /* (On: another spell, its noise: heard by the denizens, the
                                                **   player's told as it began) */
        bd.ready += SPELL;
        told (roof ? DE_ROOF : DE_DIGS, roof ? DUG_LOUD : DIG_LOUD, dx, dy);
        goto put;
    }
    z = elev_at (bd.sn, bd.x, bd.y);            /* (Through: the square a passage at its floor's height) */
    change (bd.sn, dx, dy, T_CORR, z, 0xFF);
    if (h & 1) {
        cut_corner (dx, bd.y);                  /* (A diagonal: its corners cut, the way walked corner to corner) */
        cut_corner (bd.x, dy);
    }
    if (rd) {
        goto put;
    }
    bd.doing = DO_ROAM;
    bd.ready += 10;
    den_saw = 1;                                /* (The world to look about again: the new way seen) */
    told (DE_DUG, DUG_LOUD | (rk & 0x80), dx, dy);
put:
    if (rd) {
        return;                                 /* (A record's: no body to put back) */
    }
    BD_PUT ();
    ++HUB->den_seq;
}
