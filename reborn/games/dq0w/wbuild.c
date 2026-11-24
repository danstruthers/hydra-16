/*
** wbuild.c - (dq0w) building, every actor's (the user's, 2026-11-23: "The character builds anywhere, with materials";
** the one-path plan's stage 2, ONE-PATH.md's 2.7, the act audit's G-17): a door hung, a way walled up, by the
** character (the things' IO_BUILD: "hang the door", "wall up the passage") and by a builder (wdig.c's I_KIT
** KT_BUILD, a faction's miner at its home: HOUSE.md's builders) through this one code.
**   - Where: a door hung in any doorway whose broken leaf is there (a door broken down, PS_WRECKED: the leaf mended and
**     hung again, the door shut, its lock and bar whole, its harm gone); a way walled up at any passage square (T_CORR),
**     no one on it, nothing lying on it.  The character's, the square it faces; a builder's, its mind's (I_KIT's).
**   - With: a hammer carried (tool_of, every actor's: a record's, away, its kind's outfit, the house's as its digging
**     is); a door's leaf, the broken one in the frame (the world has no leaf to carry yet); stones, the spoil of the
**     way's own digging (a dug square: its change record, T_CORR), else a square of rubble beside it (its stones taken:
**     that square cleared, a passage).
**   - Its test: the referee's Timbercraft for a door, Masonry for a wall (P_TIMBER, P_MASONRY: the act table's hang
**     and wallup rows' tests), by the actor's own sheet; a record's untested.  A failure: its time spent, nothing done.
**   - Its time and its noise: the act table's rows (hang a spell, ten minutes; wallup six of them, an hour), heard
**     hammering, stones being laid (act_noise: the character's E_NOISE, an actor's E_DEN DE_HANGS, DE_WALLS, told seen
**     or heard; a builder's work gone badly heard as its noise, DE_NOISE, not told as done).  Its time the character's
**     wait (the UI's, the request's x), an actor's ready.
** The character's words in things_text (SA_HUNG ...: said.txt's SA rows).  Swapped part 124's (../dq0/swap.inc: its
** stub build, things.c's s_serve and wdig.c's); its locals on the C stack (it calls other parts, which copy this one
** in again after).
*/

#pragma local-strings (on)

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "wbody.h"
#include "acts.h"

extern struct ireq iq;                          /* (things.c's: the request's copy) */
void __fastcall__ put (const char* s);          /* (tlist.s's: its words after the text) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;
unsigned char __fastcall__ dig_kept (unsigned char begin);   /* (Part 19's: a record looked up, rec's) */
extern struct change rec;                       /* (store.c's) */

#pragma code-name (push, "SW_build_CODE")
#pragma rodata-name (push, "SW_build_RODATA")

/* Anyone on stratum sn's square x, y (the character, an actor): 1 */
static unsigned char __fastcall__ stood (unsigned char sn, unsigned x, unsigned y)
{
    const struct actor* a;
    unsigned char i;

    hub_map (0);
    if (HUB->pc_x == x && HUB->pc_y == y && HUB->pc_stratum == sn) {
        return 1;
    }
    for (a = HUB->actor, i = 0; i < NACTORS; ++i, ++a) {
        if (a->state && a->sn == sn && a->x == x && a->y == y) {
            return 1;
        }
    }
    return 0;
}

/* A sentence put coded (SAY_AT2: its SA_* | 0x80, n the thing's words, SAY_SEP, none, SAY_SEP: wdrag.c's dsay) */
static void __fastcall__ bsay (unsigned char sa, const char* n)
{
    char c[3];

    c[0] = SAY_AT2;
    c[1] = sa | 0x80;
    c[2] = 0;
    put (c);
    put (n);
    put ("\2\2");
}

unsigned char s_build (void)
{
    unsigned char who, what, sn, rd, t, h, g, pt = 0, ok = IR_NO, sa;
    unsigned x, y, rx = 0, ry = 0, tt = 0;
    int z = 0;
    unsigned char* p;
    struct portal* q;
    struct actq aq;

    what = iq.to & 3;
    rd = (iq.to & BD_REC) != 0;
    if (iq.to & TO_DEN) {
        who = rd ? 0 : bk + 1;                  /* (A record's: no actor) */
        sn = bd.sn;
        x = iq.x;
        y = iq.y;
    } else {
        who = 0;
        hub_map (0);
        sn = HUB->pc_stratum;
        h = HUB->pc_face & 7;                   /* (The square it faces) */
        x = HUB->pc_x + head_x[h];
        y = HUB->pc_y + head_y[h];
    }
    p = at (sn, x, y);
    t = p ? TILE (*p) : T_ROCK;
    if (t == T_DOOR && (q = portal_at (cx_, cy_)) != 0 && q->kind == PK_DOOR && (q->state & PS_WRECKED) &&
        what != BD_WALL) {
        what = BD_HANG;                         /* (A broken door's leaf in its frame: mended, hung again) */
        pt = q->parts;                          /* (Read while its chunk's in the window) */
    } else if (t == T_CORR && what != BD_HANG) {
        what = BD_WALL;
        z = C->elev[cy_][cx_];
    } else {
        sa = what == BD_WALL ? SA_NO_WALL_HERE : SA_NO_HANG_HERE;
        goto said;
    }
    sa = what == BD_WALL ? SA_NO_WALL_HERE : SA_NO_HANG_HERE;
    if (stood (sn, x, y)) {
        goto said;                              /* (Someone in the way) */
    }
    if (what == BD_WALL) {
        while ((tt = items (tt, IN_SQUARE | IN_X, x)) != 0) {
            if (ip->y == y && ip->sn == sn) {
                goto said;                      /* (A thing lying there: not walled in) */
            }
        }
    }
    sa = SA_BUILD_HAMMER;
    if (!rd && !tool_of (who, KI_HAMMER, 1)) {
        goto said;                              /* (No hammer: every actor's tools its own things, wport.c's rule) */
    }
    if (what == BD_WALL) {
        rec.sn = sn;                            /* (Its stones: the spoil of its own digging, a dug square's record) */
        rec.x = x;
        rec.y = y;
        rec.link = 0xFF;
        if (!dig_kept (2) || rec.tile != T_CORR) {
            sa = SA_NO_STONES;                  /* (Else the rubble beside it: its stones taken, the square cleared) */
            for (h = 0; ; ++h) {
                if (h == 8) {
                    goto said;
                }
                rx = x + head_x[h];
                ry = y + head_y[h];
                if ((p = at (sn, rx, ry)) != 0 && TILE (*p) == T_RUBBLE) {
                    break;
                }
            }
        }
    }
    act_get (aq, what == BD_HANG ? AC_HANG : AC_WALLUP);
    tt = what == BD_HANG ? aq.r.time : 6 * aq.r.time;   /* (A spell; an hour, six of the row's spells) */
    g = rd ? G_PASS : act_test (who, aq.r.test, aq.r.steps);   /* (The row's: Timbercraft, Masonry) */
    if (G_OK (g)) {
        if (what == BD_HANG) {
            change (sn, x, y, 0, pt | (iq.to & TO_DEN ? 0 : PP_KNOWN), PORTAL_STATE);  /* (Shut, its lock
                                                **   and bar whole; known to the character, its own) */
            change (sn, x, y, 0, 0, PORTAL_HARM);
            sa = SA_HUNG;
        } else {
            change (sn, x, y, T_WALL, z, 0xFF);
            sa = SA_WALLED;
            if (rx | ry) {
                at (sn, rx, ry);
                change (sn, rx, ry, T_CORR, C->elev[cy_][cx_], 0xFF);   /* (The rubble's stones taken: cleared) */
            }
        }
        ok = IR_OK;
    } else {
        sa = SA_BUILD_BAD;
    }
    if (rd) {
        return ok;                              /* (A record's: untold, the character's never near) */
    }
    if (iq.to & TO_DEN) {
        if (ok == IR_OK) {
            den_saw = 1;                        /* (The world to look about again: the door shut, the wall) */
            act_noise (who, what == BD_HANG ? AC_HANG : AC_WALLUP, x, y);
        } else {
            den_told (DE_NOISE, aq.r.loud, x, y);   /* (Gone badly: heard, not told as done) */
        }
        bd.ready += tt;
        ++HUB->den_seq;
        return ok;
    }
    act_noise (0, what == BD_HANG ? AC_HANG : AC_WALLUP, x, y);
    hub_map (0);
    HUB->ireq.x = tt;                           /* (Its time the UI's wait) */
    ok = IR_OK;                                 /* (Done, or its time spent: the UI's wait either way) */
said:
    if (iq.to & TO_DEN) {
        if (!rd) {
            act_get (aq, AC_HANG);              /* (An actor refused: it gives it up a while, half a spell, as a dig
                                                **   nothing bites into: its mind's to choose again) */
            bd.ready += aq.r.time / 2;
        }
        return IR_NO;
    }
    put_bank = UI_BANK;                         /* (The things' words) */
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    bsay (sa, rx | ry ? "r" : "");              /* (SA_WALLED's text: its stones the rubble's) */
    return ok;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
