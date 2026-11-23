/*
** wdig.c - (dq0w) a tunneller's dig (slice 4.11, 2026-11-01: DENIZENS.md's tunnellers and builders; HOUSE.md's
** tunnellers, for review): an actor's way dug through rock, a wall or rubble by the character's rules and the
** character's code (the one-path plan's stage 2, digging, 2026-11-23: ONE-PATH.md's 2.1 and 2.7, the act audit's E1
** and G-13): a spell of work at a time (the act table's dig row, ten minutes, every actor's), each spell part 1's
** dig_spell, the character's own (dq0w.c's, part 1's): the rock it's into, the referee's test by its sheet (RS_TEST P_DIG, the
** actor's: the tool among its own things that suits it best, its claws a burrower's, the referee's rate and wear, the
** house's table, every ruleset's; its own copy of the rates before), the rock's hardness the rate's, poor rock come
** down on a spell gone badly, its tool worn (its own thing's wear, as the character's is), a hidden door dug into as
** the wall it seems found (for every one, as the character's find); kept as it goes (the hole's change record, DUG,
** the character's: either can carry on what the other began), loud (the act table's rows: its scraping heard, E_DEN's
** DE_DIG, the rock carrying it where the air doesn't); and through, the square a passage (a diagonal's corners cut, as
** the character's), told (DE_DUG: "A fresh tunnel opens ..."), the world looking about again.
**
** The denizens' task's minds choose where (dq0d.c's dig_way: I_KIT's KT_DIG, who the actor plus 1, b the heading,
** x[2] its tools, DG_*); its spell begun then (the hub's dig_t; nothing it has bites into it, the referee asked as the
** character's dig begins: it gives it up a while), its feet reckoned as it's done, when its mind asks again (the same
** way, its doing still DO_DIG: a spell cut short by another intent, a step, a blow, is lost, as the character's part
** spell is; a rouse, back to it, the spell's rest).
**
** All of it swapped part 79's (../dq0/swap.inc: dq0w.cfg's SW79), called through its stub (bank.s's den_dig) from
** part 51's den_other, I_KIT's.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "../dq0/rock.h"
#include "world.h"
#include "work.h"
#include "wbody.h"
#include "things.h"
#include "acts.h"

#pragma local-strings (on)
#pragma code-name ("SW79CODE")
#pragma rodata-name ("SW79RODATA")             /* (No data of its own: its locals on the C stack, as it calls other
                                                **   parts, which copy this one in again after) */

#define DUG_LOUD        7                       /* The way through: the spoil falling (the act table's dug row's, as
                                                **   told: DE_DUG's own den_told, its DT_SEEN) */

unsigned char __fastcall__ dig_kept (unsigned char begin);   /* (Part 19's: a DUG record looked up, rec's) */
extern struct change rec;                       /* (store.c's) */

/* A diagonal's corner, square x, y, cut (T_CUT) if it's rock or a wall: the way through walked corner to corner (as
** the character's, dq0w.c's cut_corner) */
static void __fastcall__ cut_corner (unsigned x, unsigned y)
{
    unsigned char* p = at (bd.sn, x, y);

    if (p && (TILE (*p) == T_ROCK || TILE (*p) == T_WALL)) {
        change (bd.sn, x, y, T_CUT, C->elev[cy_][cx_], 0xFF);
    }
}

/* A builder's work (I_KIT's KT_BUILD, slice 4.11's rest: HOUSE.md's builders): at m's w1, w2, a door broken down
** put up again, or a way dug walled up: the character's own building, every actor's (wbuild.c's, part 123's: the
** user's, 2026-11-23, "The character builds anywhere, with materials"; its hammer, its leaf or its stones, its test,
** the act table's hang and wallup rows' times and noise; a record's, away, its tools the house's, untested, untold) */
extern struct ireq iq;                          /* (things.c's: the request's copy) */
unsigned char build (void);                     /* (Part 123's: wbuild.c) */
static void __fastcall__ built (const struct msg* m, unsigned char rd)
{
    iq.to = TO_DEN | (rd ? BD_REC : 0) | BD_ANY;
    iq.x = I_KIT_X (*m);
    iq.y = I_KIT_Y (*m);
    build ();
}

/* Body bd's tools to dig with, listed for the referee (their keys in the tools' own form, eight bytes each, an empty
** one last: rshouse.h's house_dig_tool) in the world's slot's text: its claws (g's DG_CLAWS: a burrower's, its kind's), and each thing of its own that's a tool
** (its kind's key: a pick, a shovel, a knife ...; a chisel only with a hammer to drive it, the character's rule).  A
** record's (rd: its things not looked at away from the character, the house's) the tools g says: a pick, a shovel */
#define DIG_KEYS        8                       /* (Its tools told, at most: the referee's own form, eight bytes each) */
static void __fastcall__ tools_of (unsigned char g, unsigned char rd)
{
    char t[DIG_KEYS + 1][8];                    /* (The last always empty: the list's end) */
    unsigned i = 0;
    unsigned char k, n = 0, ham = 0, chi = 0;

    memset (t, 0, sizeof t);
    if (g & DG_CLAWS) {
        strcpy (t[n++], "claws");
    }
    if (rd) {
        if (g & DG_PICK) {
            strcpy (t[n++], "pick");
        }
        if (g & DG_SPADE) {
            strcpy (t[n++], "shovel");
        }
    } else if (bd.did) {
        while ((i = items (i, IN_DEN, 0)) != 0) {
            if ((ip->owner | ip->hide << 8) != bd.did) {
                continue;
            }
            k = ip->kind;
            hub_map (KIND_BANK);
            if (k == KI_HAMMER) {
                ham = 1;
            } else if (n < DIG_KEYS) {
                memcpy (t[n], CATALOGUE[k < NKINDS ? k : 0].tool, 7);
                if (!strcmp (t[n], "chisel")) {
                    chi = 1;                    /* (Only with a hammer: below) */
                    t[n][0] = 0;
                } else if (t[n][0]) {
                    ++n;
                }
            }
        }
        if (ham && chi && n < DIG_KEYS) {
            strcpy (t[n], "chisel");
        }
    }
    hub_map (0);
    memcpy (HUB->slot[SLOT_WORLD].text, t, sizeof t);
}

/* The spell (part 1's dig_spell, the character's) of body bd at its stand, digging square dx, dy (its way, what it's
** into, the half-feet it needs, *done dug so far: on by it), its tools g (a record's rd): the character's dig kept
** round it; its grade (who | DIG_ASK: only whether anything it has bites, DS_CANT if nothing does), *f dig.found */
static signed char __fastcall__ spell_of (unsigned char who, unsigned char way, unsigned char mat, unsigned dx,
    unsigned dy, unsigned need, unsigned* done, unsigned char* f)
{
    struct dig keep;
    signed char g;

    memcpy (&keep, &dig, sizeof dig);
    dig.d = way;
    dig.mat = mat;
    dig.x = dx;
    dig.y = dy;
    dig.need = need;
    dig.done = *done;
    dig.rock = RK_NONE;
    dig.sn = bd.sn;
    dig.sx = bd.x;
    dig.sy = bd.y;
    g = dig_spell (who);
    *done = dig.done;
    *f = dig.found;
    if (dig.wear) {
        wear_did = bd.did;                      /* (Its own thing worn, as the character's is: things.c's wear) */
        wear (dig.key, dig.wear);
        wear_did = 0;
    }
    memcpy (&dig, &keep, sizeof dig);
    return g;
}

/* Body m->who's dig (I_KIT's KT_DIG): begun, back to it, a spell's feet reckoned and on, or through.  Who 0, a record's
** (one away from the character, not an actor: the denizens' roam, at its lair's dig, slice 4.11's rest): a spell's
** feet at once, coarse (w1 w2 its stand, x[2] its stratum, x[0] its tools), untested (a marginal success's rate, the
** referee's), untold (the character's never near: those near are actors); the hole kept and the way through made as
** an actor's are */
void __fastcall__ s_den_dig (const struct msg* m)
{
    unsigned char h, t, k, mat, r1, way, need, rd, seen, f;
    signed char g;
    unsigned now, ago = 0, dx, dy, done = 0;
    unsigned char* p;
    struct portal* q;
    struct actq aq;
    int z;

    now = (unsigned) HUB->now;
    h = I_KIT_HEAD (*m) & 7;
    if ((rd = !m->who) != 0) {
        bd.sn = I_KIT_SN (*m);                  /* (Body bd but a copy, a record's: none put back, I_TROD's way; w1
                                                **   w2 its face, its stand the square before it) */
        bd.x = I_KIT_X (*m) - head_x[h];
        bd.y = I_KIT_Y (*m) - head_y[h];
        r1 = I_KIT_TOOLS (*m);
    } else {
        BD_GET ((m->who - 1) & 7);
        if (!bd.state || (bd.doing == DO_DOWN && bd.ready > HUB->now)) {
            return;                             /* (Gone, or senseless: nothing) */
        }
        ago = now - HUB->dig_t[bk];
        bd.ready = HUB->now;
        r1 = I_KIT_WITH (*m);                   /* (What it digs with: its claws; its things the world's to look at) */
    }
    if ((I_KIT_DRAW (*m) & KT_BUILD) == KT_BUILD) {
        built (m, rd);                          /* (A door put up again, a way walled up: a builder's) */
        goto put;
    }
    dx = bd.x + head_x[h];
    dy = bd.y + head_y[h];
    if ((k = room_of (bd.sn, bd.x, bd.y)) == 0xFF) {    /* (A wall's room: its side's, or the far side's) */
        k = room_of (bd.sn, dx + head_x[h], dy + head_y[h]);
    }
    p = at (bd.sn, dx, dy);
    t = p ? TILE (*p) : T_PIT;
    seen = p && (*p & T_VIS) ? DT_SEEN : 0;     /* (Seen as it is, before a change: DE_DUG's) */
    if (t == T_DOOR && (q = portal_at (cx_, cy_)) != 0 && (q->state & PS_HIDDEN)) {
        t = T_WALL;                             /* (A hidden door dug at as the wall it seems, the character's:
                                                **   HOUSE.md's) */
    }
    mat = t == T_RUBBLE ? MAT_SOIL : C->mat[t == T_WALL && k != 0xFF ? k : NROOMS][MP_WALL];
    hub_map (0);
    if (t != T_WALL && t != T_ROCK && t != T_RUBBLE) {
        goto none;                              /* (Nothing to dig there) */
    }
    way = (h & 1 ? 6 : 0) + (h >> 1);           /* (The character's dig's way, DUG's: DIR_N ... DIR_W, a diagonal 6
                                                **   to 9) */
    need = t == T_WALL || t == T_RUBBLE ? 4 : h & 1 ? 14 : 10;
    if (rd) {
        goto spell;
    }
    if (bd.doing != DO_DIG || bd.face != h) {
        tools_of (r1, 0);
        if (spell_of (bk + 1 | DIG_ASK, way, mat, dx, dy, need, &done, &f) == DS_CANT) {
            goto none;                          /* (Nothing it has bites into it: asked as the character's dig is
                                                **   begun, pc_dig (0)'s, the referee's) */
        }
        bd.doing = DO_DIG;                      /* (Begun: a spell, its noise) */
        bd.face = h;
        HUB->dig_t[bk] = now;
        bd.ready += act_noise (bk + 1, AC_DIGBEGIN, bd.x, bd.y);
        goto put;
    }
    act_get (aq, AC_DIG);
    if (ago < aq.r.time) {
        bd.ready += aq.r.time - ago;            /* (Roused, and back to it: the spell's rest) */
        goto put;
    }
spell:
    rec.sn = bd.sn;                             /* (A spell done: the hole as it was, the character's or another's) */
    rec.x = dx;
    rec.y = dy;
    rec.link = DUG;
    done = dig_kept (2) && rec.elev == way && rec.tile < need ? rec.tile : 0;
    tools_of (r1, rd);
    g = spell_of (rd ? DIG_RECORD : bk + 1, way, mat, dx, dy, need, &done, &f);   /* (The character's own spell) */
    if (g == DS_CANT || (f & 0x20)) {
        goto none;                              /* (Nothing bites into it, or hidden by magic: it turns the tool) */
    }
    if (f & 0x10) {
        if (!rd) {                              /* (A hidden door come to, its facing off: found, its dig over, told
                                                **   as the character's find is; the world to look about again) */
            bd.doing = DO_ROAM;
            bd.ready += 10;
            den_saw = 1;
            act_noise (bk + 1, AC_DIGFOUND, dx, dy);
        }
        goto put;
    }
    hub_map (0);
    if (done < need) {
        rec.sn = bd.sn;                         /* (Kept: the hole as it's dug, DUG, the character's way's) */
        rec.x = dx;
        rec.y = dy;
        rec.link = DUG;
        rec.tile = done;
        rec.elev = way;
        store_put ();
        hub_map (0);
        if (rd) {
            goto put;
        }
        HUB->dig_t[bk] = now;                   /* (On: another spell, its noise: heard by the denizens, the
                                                **   player's told as it began) */
        act_get (aq, AC_DIG);
        bd.ready += aq.r.time;
        act_noise (bk + 1, g == DS_ROOF ? AC_ROOF : AC_DIG, bd.x, bd.y);
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
    den_told (DE_DUG, DUG_LOUD | seen, dx, dy);
    goto put;
none:
    hub_map (0);
    if (!rd) {
        bd.doing = DO_ROAM;                     /* (Nothing to dig there, or nothing it has bites into it: it gives it
                                                **   up a while) */
        act_get (aq, AC_DIG);
        bd.ready += aq.r.time / 2;
    }
put:
    if (rd) {
        return;                                 /* (A record's: no body to put back) */
    }
    BD_PUT ();
    ++HUB->den_seq;
}
