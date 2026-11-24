/*
** wsearch.c - (dq0w) a search, every actor's (the one-path plan's stage 2, 2026-11-21: ONE-PATH.md's 2.1 and 2.7; the
** user's, 2026-11-17: "searching is anyone's", "a denizen's search seen or heard is told as the character's is"; the
** audits' F1-F5, C15, G-7).  Body bd's search about it, by the character's own: the act table's search row (its minute,
** its noise: told seen or heard, E_DEN DE_SEARCHES; the character's the senses' busy noise, as loud), the hidden doors
** and traps within its reach by wtrap.c's search (the character's, wk.who its actor: the referee's P_SEARCH of its
** sheet, a trap the worse its hiding), the hidden things lying there by the character's test (things.c's scan: P_SEARCH,
** its hiding the steps worse, 2 more by touch), and one hiding there it sees (the character, or one not of its
** faction) by the character's own test of a hidden denizen (part 94's HO_SEARCH: one easier, its cover's steps worse,
** acts.h's FIND_STEPS).  Its reach the character's: three squares it sees (by its eyes, the line from them; by light
** where it stands, or its darkvision, as the character's search by sight is), or by touch in the dark its own square
** and the eight round it.  What it finds is found for every one, as the character's finds are: a hidden door a door, a
** door's trap known; a trap underfoot known to its faction alone (the user's, 2026-11-23), a thing uncovered (no longer hidden: its take
** to take it, wtake.c's), one hiding out of hiding (the character's hiding ended, told: HD_FOUND's).
**
** Part 94's (dq0w.c's hiding oneself, its room), through its stub den_search (bank.s's): wtake.c's (a robber come to
** where it lost the character, its TW_SEARCH).  Its time all at once (an actor's work, as its door work's): the minute
** on its ready, its tests made then (the audit's F2: by the time it got, to come with the timed work for actors).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "work.h"
#include "wbody.h"
#include "acts.h"
#include "line.h"

#pragma local-strings (on)
#pragma code-name ("SW94CODE")
#pragma rodata-name ("SW94RODATA")

unsigned char __fastcall__ ln_seen94 (unsigned char sn, unsigned x, unsigned y, unsigned tx, unsigned ty,
    unsigned char mode);                        /* (line.s's: the line's, this part's own) */
extern unsigned char srch_p;                    /* (dq0w.c's: a search's test, wtrap.c's) */
static const unsigned char d2_[7] = { 9, 4, 1, 0, 1, 4, 9 };   /* (A square's distance off, squared, by its offset:
                                                **   wtrap.c's, things.c's) */
#define SEARCH_R        3

/* Square x, y off body bd within its search's reach r (its distance off, squared, less) and, but by touch, seen
** (WK_SIGHT's): 1 */
static unsigned char __fastcall__ in_srch (unsigned x, unsigned y, unsigned char r)
{
    x = x - bd.x + SEARCH_R;
    y = y - bd.y + SEARCH_R;
    return x <= 2 * SEARCH_R && y <= 2 * SEARCH_R && d2_[x] + d2_[y] < r && (r == 3 || (WK_SIGHT[y] >> x & 1));
}

void __fastcall__ s_hid_op (unsigned char op);

void __fastcall__ s_den_search (unsigned char dark)
{
    struct work keep;                           /* (The character's work, kept round it: wport.c's den_portal's) */
    struct actor* o;
    unsigned i;
    unsigned char touch, a, b, r, k;

    bd.ready += act_noise (bk + 1, AC_SEARCH, bd.x, bd.y);  /* (Begun: told, seen or heard; its minute) */
    touch = !dark && unlit_at (bd.sn, bd.x, bd.y);  /* (In the dark beyond its darkvision: by touch, within reach) */
    r = touch ? 3 : 19;
    memcpy (&keep, &wk, sizeof wk);
    memset (&wk, 0, offsetof (struct work, pq));
    memset (WK_SIGHT, 0, 2 * SEARCH_R + 1);
    wk.who = bk + 1;
    wk.sn = bd.sn;
    wk.px = wk.py = NO_SQ;
    wk.fx = bd.x;
    wk.fy = bd.y;
    wk.what = PA_SEARCH;
    wk.pd = touch ? SF_ABOUT | SF_TOUCH : SF_ABOUT;
    wk.armed = TRAP_KB (bd.faction);            /* (Its faction's bit in a trap's knowers: the floor's traps it finds
                                                **   its faction's to know, wtrap.c's search, 2026-11-23) */
    ln_eye = LN_EYES (bd.pos);
    for (b = 0; !touch && b <= 2 * SEARCH_R; ++b) {   /* (The squares about it it sees, by the line from its eyes) */
        for (a = 0; a <= 2 * SEARCH_R; ++a) {
            if (d2_[a] + d2_[b] < r && !ln_seen94 (bd.sn, bd.x, bd.y, bd.x - SEARCH_R + a, bd.y - SEARCH_R + b,
                LN_SIGHT)) {
                WK_SIGHT[b] |= 1 << a;
            }
        }
    }
    srch_p = P_SEARCH;
    tsearch ();                                 /* (The hidden doors and traps: wtsrch.c's search, by its sheet) */
    if (wk.res & 1) {
        den_saw = 1;                            /* (A door found: the world to look about again) */
    }
    for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {   /* (The hidden things lying in its reach: things.c's scan's
                                                **   test; its own home's, its takings' stash, left: it knows them) */
        if (ip->sn != bd.sn || (ip->flags & (I_HIDDEN | I_MAGIC)) != I_HIDDEN || !in_srch (ip->x, ip->y, r) ||
            (ip->sn == bd.hsn && (unsigned) (ip->x - bd.hx + 1) <= 2 && (unsigned) (ip->y - bd.hy + 1) <= 2)) {
            continue;
        }
        if (act_test (bk + 1, P_SEARCH, -(signed char) ip->hide - (touch ? 2 : 0)) >= G_PASS) {
            item_at (i)->flags &= ~I_HIDDEN;    /* (Uncovered, lying there: unknown to the character still) */
        }
    }
    hub_map (0);
    if (!touch) {                               /* (One hiding it sees in its reach: the character's own search's test of
                                                **   a hidden denizen, part 94's HO_SEARCH; by sight alone, as that) */
        if (HUB->pc_hid && bps == bd.sn && in_srch (bpx, bpy, r) &&
            act_test (bk + 1, P_SEARCH, FIND_STEPS (HUB->pc_hid)) >= G_PASS) {
            s_hid_op (HO_DEN | HD_FOUND);       /* (The character's hiding ended: "found", its long task stopped) */
        }
        for (k = 0; k < NACTORS; ++k) {
            hub_map (0);
            o = &HUB->actor[k];
            if (k != bk && o->state > 1 && o->sn == bd.sn && o->faction != bd.faction && in_srch (o->x, o->y, r) &&
                act_test (bk + 1, P_SEARCH, FIND_STEPS (ST_HID (o->state))) >= G_PASS) {
                hub_map (0);
                o->state = 1;
                ++HUB->den_seq;
            }
        }
    }
    hub_map (0);
    memcpy (&wk, &keep, sizeof wk);
}
