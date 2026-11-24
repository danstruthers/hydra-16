/*
** way.c - (dq0p) a look one way's own record (HOUSE.md's looking one way: the hub's LK_WAY, dq0p.c's v_way; level.h's
** struct wayseen, in the senses' bank after what the character perceives, WAY_SEEN): what the look sees beyond each
** opening ahead of it, from the squares it sees now (view.c's marks: a look one way's twice as far in the quarter
** ahead), for the UI to tell what lies through a doorway, an open door, a window, a portcullis or a slit, and how far
** down a passage it sees; a shut door, closed shutters, not seen past; and the things, the denizens and the lights
** far off the record has that lie ahead (the record sight.c's, its denizens dens.c's, as the look's made).  Ahead is
** the way the character faces, or either side of it (REL8's ahead, ahead to its right, ahead to its left).
**
** A look one way's near part (the hub's LK_NEAR, dq0p.c's look_near: the user's, 2026-11-17, "a look tells what's near
** first and goes on as the rest is worked out"): the same record (WAY_NEAR) made from the last whole look as it stands
** (its squares seen now, its record), told as its far part's look (LK_WAY's) is worked out, and what that may yet
** change held back for it: an opening's way (WB_HOLD) the eye follows to a chunk not made (none made for it: the far
** part's to), or out past the squares a glance is sure of where the far part may see more (dq0p.c's way_stop, far_may);
** "nothing more that way" (WM_HOLDN) where the far part may see more at all.  The far part's record (WAY_SEEN) says if
** it's all as its near part had it (WM_SAME: dq0p.c's way_more): nothing more to tell.
*/

#include <stddef.h>
#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"

#pragma code-name ("SW_way_record_CODE")                   /* (Part way_record's, all of it: once a look one way) */
#pragma rodata-name ("SW_way_record_RODATA")
#pragma bss-name ("SW_way_record_RODATA")                  /* (Its state too, the part's own: none kept from one call to the next) */
#pragma static-locals (on)

static struct { unsigned char seq, nbeyond; struct beyond beyond[SEE_BEYOND]; } W;   /* (Its first part, its
                                                **   openings: the rest made in the bank) */
typedef char w_fits[sizeof W == offsetof (struct wayseen, nitems) ? 1 : -1];
typedef char wn_fits[sizeof (struct sight) <= WAY_NEAR_AT && WAY_NEAR_AT + sizeof (struct wayseen) <= RUN_AT &&
    WAY_AT + sizeof (struct wayseen) <= SOUND_AT ? 1 : -1];   /* (The bank's: the record, the near part's, the whole's) */
static struct beyond* b;                        /* (The opening's entry) */
static struct { unsigned char rel; signed char dz; unsigned char state; unsigned x, y; unsigned char kind, parts; } d;
static unsigned ox, oy, rx, ry, px, py;         /* (seen_on's: from ox, oy on od, stratum osn; the character's room's
                                                **   floor's first square rx, ry; the character's square px, py) */
static unsigned char osn, od, fc_;          /* (fc_: the way it faces) */
static unsigned char lf_;                       /* (seen_on's: a near part's way to a chunk not made, 2, or out past
                                                **   what a glance is sure of, 1: way_stop's) */
extern unsigned char wnear, wfar, wheld;        /* (dq0p.c's: a near part made now; far_may's; an opening held back) */
unsigned char __fastcall__ way_stop (unsigned x, unsigned y);
void __fastcall__ way_more (struct wayseen* r);

/* Square p (at ()'s: its chunk's portal_at () for it) one the eye sees past: no rock nor wall, no shut door, no window's
** shutters closed (but a portcullis: view.c's, cast.s's rdoors) */
static unsigned char __fastcall__ past (const unsigned char* p)
{
    struct portal* q;

    return p && T_CLEAR (*p) && !((TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) && (q = portal_at (cx_, cy_)) != 0 &&
        !(q->state & PS_OPEN) && q->kind != PK_PORTCULLIS);
}

/* On from (ox, oy), its first way od (a heading, H_*: a diagonal way's too, 2026-11-01), into b: along the way the eye sees past, straight on through the dark,
** round a bend only where it sees the square (the one way on that isn't back), as far as the look reaches; how far
** along it it sees (the last square it sees now, T_VIS), and the first square it sees in a room but the character's
** (its floor's first square rx, ry) ends it, the room's kind and light noted */
static void seen_on (void)
{
    unsigned char n, k, w, r;
    unsigned char* p;
    struct room* q;

    b->what = WB_DARK;
    lf_ = 0;
    for (n = 0; n < 2 * WAY_R && ox - px + WAY_R <= 2 * WAY_R && oy - py + WAY_R <= 2 * WAY_R; ) {
        if ((wnear | v_nom) && (lf_ |= way_stop (ox, oy)) & 2) {
            break;                              /* (A near part's: a chunk round it not made, the far part's; a far
                                                **   part's, one none's made for, its rest's: v_may's) */
        }
        for (k = 0, w = 0xFF; k < 8; ++k) {     /* (Straight on, else the first way seen that isn't back; a diagonal
                                                **   only a diagonal way, slant's: past its cut corner) */
            if (!REL_BEHIND (k - od) && (w == 0xFF || k == od) && (!(k & 1) || slant (osn, ox, oy, k)) &&
                past (p = at (osn, ox + head_x[k], oy + head_y[k])) && (k == od || (*p & T_VIS))) {
                w = k;
            }
        }
        if (w == 0xFF) {
            break;
        }
        ox += head_x[od = w];
        oy += head_y[od];
        ++n;
        if (!(*at (osn, ox, oy) & T_VIS)) {
            continue;                           /* (Dark: on, as the eye goes) */
        }
        b->dist = n;
        b->what = WB_PASSAGE;
        if ((r = room_of (osn, ox, oy)) != 0xFF) {
            q = &C->room[r];
            if (ox - cx_ + q->x != rx || oy - cy_ + q->y != ry) {      /* (Not the character's own) */
                b->what = WB_ROOM;
                b->kind = q->kind;
                b->lit = (q->flags & R_LIT) != 0;
                break;
            }
        }
    }
    if (lf_ & (wfar & 1 | 2)) {
        b->what |= WB_HOLD;                     /* (A near part's: held back, its way on to a chunk not made, or out past
                                                **   what a glance is sure of where the far part may see more) */
        wheld = 1;
    }
}

/* A way (REL8's: 0xFF where it stands) ahead, or ahead to either side */
static unsigned char __fastcall__ is_ahead (unsigned char rel)
{
    return rel != 0xFF && ((rel + 1) & 7) < 3;
}

/* An opening seen ahead (rel, its way from the character, REL8's; what it is, BN_*): its entry, b; 0 if it isn't
** ahead, or there's no room for it */
static unsigned char __fastcall__ ahead (unsigned char noun)
{
    if (!is_ahead (d.rel) || W.nbeyond >= SEE_BEYOND) {
        return 0;
    }
    b = &W.beyond[W.nbeyond++];
    b->rel = d.rel & 7;
    b->noun = noun;
    return 1;
}

/* Square x, y beyond the door looked through (pk, VP_*'s 1 to 3: the door the square ahead, px + hx, py + hy): past it,
** within a quarter (a little window's), or within an eighth (a spy-hole's: as far to the side as half the way on); its
** cover closed, nothing.  (The squares the look marks seen through it the wider cone of a doorway's, the eye beside it:
** what's told of them, the things and the denizens, this) */
static unsigned char __fastcall__ beyond (unsigned x, unsigned y, unsigned char pk)
{
    int f, l;
    signed char hx = head_x[fc_], hy = head_y[fc_];

    x -= px + hx;
    y -= py + hy;
    f = (int) x * hx + (int) y * hy;
    l = (int) x * hy - (int) y * hx;
    l = l < 0 ? -l : l;
    return pk < 3 && f > 0 && (pk == 1 ? l <= f : 2 * l <= f);
}

extern const struct sight* sight_rec;           /* (sight.c's: the record made, but its denizens) */

void __fastcall__ s_way_seen (unsigned char sn, unsigned char face)
{
    const struct sight* s = sight_rec;
    const unsigned char* p;
    struct sden* q;
    unsigned char k, st, on, pk = v_way >> 1 & 3;   /* (Looked through a closed door's little window, its spy-hole, VP_*'s
                                                **   1 to 3: view.c's, the user's, 2026-11-14) */
    signed char hx = head_x[face], hy = head_y[face];

    struct wayseen* R;

    memset (&W, 0, sizeof W);
    wheld = 0;
    fc_ = face;
    W.seq = s->seq;
    osn = sn;
    rx = s->rx;
    ry = s->ry;
    px = s->x;
    py = s->y;
    for (k = s->ndoors, p = (const unsigned char*) s->door; k; --k, p += sizeof d) {    /* (Its room's doors,
                                                **   windows, passages' mouths; or its passage's doors) */
        memcpy (&d, p, sizeof d);
        st = d.state;
        if (pk) {                               /* (Its eye at the door's little window, or its spy-hole: that alone) */
            if (d.x != px + hx || d.y != py + hy || !ahead (pk == 1 ? BN_GRILLE : BN_PEEP)) {
                continue;
            }
            if (pk == 3) {
                b->what = WB_SHUT;              /* (Its spy-hole's cover on the far side) */
                continue;
            }
            ox = d.x;
            oy = d.y;
            od = face;
            if (face & 1) {                     /* (Looked through at a slant: on straight out of the door's far side,
                                                **   the way of the two the eye sees past) */
                od = past (at (sn, d.x + head_x[(face - 1) & 7], d.y + head_y[(face - 1) & 7])) ? (face - 1) & 7 :
                    (face + 1) & 7;
            }
            seen_on ();
            continue;
        }
        if (!ahead (d.rel & MOUTH ? BN_PASSAGE : d.kind == PK_PORTCULLIS ? BN_PORTCULLIS : d.kind == PK_WINDOW ?
            BN_WINDOW : d.kind == PK_SLIT ? BN_SLIT : st == DOORWAY ? BN_DOORWAY : st & PS_WRECKED ? BN_BROKEN :
            st & PS_OPEN ? BN_OPEN : BN_DOOR)) {
            continue;
        }
        if (!(st & (PS_OPEN | PS_WRECKED)) && (d.kind == PK_WINDOW ? d.parts & PP_SHUTTERS :
            d.kind != PK_PORTCULLIS && d.kind != PK_SLIT)) {
            b->what = WB_SHUT;                  /* (A door shut, a window's shutters: not seen past) */
            b->kind = st;                       /* (Barred, locked, as it's plain: words) */
            continue;
        }
        if (d.kind == PK_WINDOW && (d.parts & PP_SKY)) {
            b->what = WB_SKY;                   /* (A window on the surface: the sky, up its light well) */
            continue;
        }
        ox = d.x;                               /* (Out of its room, or out of its passage that way) */
        oy = d.y;
        od = s->where != W_ROOM ? (face + d.rel) & 7 : oy < ry ? H_N : oy >= ry + s->rh ? H_S :
            ox < rx ? H_W : H_E;
        seen_on ();
    }
    if (s->where == W_PASSAGE && !pk) {
        for (k = s->nways, p = (const unsigned char*) s->way; k; --k, p += sizeof s->way[0]) {   /* (Its passage's
                                                **   ways on, ahead) */
            d.rel = *p;
            if (ahead (BN_PASSAGE)) {
                ox = s->x;
                oy = s->y;
                od = (face + d.rel) & 7;
                seen_on ();
            }
        }
    }
    hub_map (SIGHT_BANK);                       /* (Into the bank: the record the main RAM's; a near part's its own) */
    R = wnear ? WAY_NEAR : WAY_SEEN;
    memset (R, 0, sizeof *R);                   /* (Its lists' ends nothing: the far part's and its near part's compared) */
    memcpy (R, &W, sizeof W);
    for (k = s->nitems, on = 0, p = (const unsigned char*) s->item; k; --k, p += sizeof s->item[0]) {   /* (The
                                                **   things ahead, a surface's with it) */
        if (!(((const struct sitem*) p)->hid & 2)) {
            on = pk ? beyond (((const struct sitem*) p)->x, ((const struct sitem*) p)->y, pk) :
                is_ahead (((const struct sitem*) p)->rel);
        }
        if (on) {
            memcpy (&R->item[R->nitems++], p, sizeof s->item[0]);
        }
    }
    for (k = s->nglows, p = (const unsigned char*) s->glow; k; --k, p += sizeof s->glow[0]) {   /* (The lights far
                                                **   off ahead) */
        if (!pk && is_ahead (*p)) {
            memcpy (&R->glow[R->nglows++], p, sizeof s->glow[0]);
        }
    }
    for (k = SIGHT->ndens, q = SIGHT->den; k; --k, ++q) {     /* (The denizens ahead: dens.c's, the bank's) */
        if (pk ? beyond (q->x, q->y, pk) : is_ahead (REL8 (bearing ((int) (q->x - s->x), (int) (q->y - s->y)), face))) {
            memcpy (&R->den[R->ndens++], q, sizeof *q);
        }
    }
    way_more (R);                               /* (Its look one way's, what's more: dq0p.c's) */
    hub_map (0);
}
