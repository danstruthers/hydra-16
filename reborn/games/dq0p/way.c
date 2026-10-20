/*
** way.c - (dq0p) a look one way's own record (HOUSE.md's looking one way: the hub's LK_WAY, dq0p.c's v_way; level.h's
** struct wayseen, in the senses' bank after what the character perceives, WAY_SEEN): what the look sees beyond each
** opening ahead of it, from the squares it sees now (view.c's marks: a look one way's twice as far in the quarter
** ahead), for the UI to tell what lies through a doorway, an open door, a window, a portcullis or a slit, and how far
** down a passage it sees; a shut door, closed shutters, not seen past; and the things, the denizens and the lights
** far off the record has that lie ahead (the record sight.c's, its denizens dens.c's, as the look's made).  Ahead is
** the way the character faces, or either side of it (REL8's ahead, ahead to its right, ahead to its left).
*/

#include <stddef.h>
#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"

#pragma code-name ("SW3CODE")                   /* (Part 3's, all of it: once a look one way) */
#pragma rodata-name ("SW3RODATA")
#pragma bss-name ("SW3RODATA")                  /* (Its state too, the part's own: none kept from one call to the next) */
#pragma static-locals (on)

static struct { unsigned char seq, nbeyond; struct beyond beyond[SEE_BEYOND]; } W;   /* (Its first part, its
                                                **   openings: the rest made in the bank) */
typedef char w_fits[sizeof W == offsetof (struct wayseen, nitems) ? 1 : -1];
static struct beyond* b;                        /* (The opening's entry) */
static struct { unsigned char rel; signed char dz; unsigned char state; unsigned x, y; unsigned char kind, parts; } d;
static unsigned ox, oy, rx, ry, px, py;         /* (seen_on's: from ox, oy on od, stratum osn; the character's room's
                                                **   floor's first square rx, ry; the character's square px, py) */
static unsigned char osn, od;

/* Square p (at ()'s: its chunk's portal_at () for it) one the eye sees past: no rock nor wall, no shut door, no window's
** shutters closed (but a portcullis: view.c's, cast.s's rdoors) */
static unsigned char __fastcall__ past (const unsigned char* p)
{
    struct portal* q;

    return p && T_CLEAR (*p) && !((TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) && (q = portal_at (cx_, cy_)) != 0 &&
        !(q->state & PS_OPEN) && q->kind != PK_PORTCULLIS);
}

/* On from (ox, oy), its first way od (DIR_*), into b: along the way the eye sees past, straight on through the dark,
** round a bend only where it sees the square (the one way on that isn't back), as far as the look reaches; how far
** along it it sees (the last square it sees now, T_VIS), and the first square it sees in a room but the character's
** (its floor's first square rx, ry) ends it, the room's kind and light noted */
static void seen_on (void)
{
    unsigned char n, k, w, r;
    unsigned char* p;
    struct room* q;

    b->what = WB_DARK;
    for (n = 0; n < 2 * WAY_R && ox - px + WAY_R <= 2 * WAY_R && oy - py + WAY_R <= 2 * WAY_R; ) {
        for (k = 0, w = 0xFF; k < 4; ++k) {     /* (Straight on, else the first way seen that isn't back) */
            if (k != ((od + 2) & 3) && (w == 0xFF || k == od) && past (p = at (osn, ox + dir_x[k], oy + dir_y[k])) &&
                (k == od || (*p & T_VIS))) {
                w = k;
            }
        }
        if (w == 0xFF) {
            break;
        }
        ox += dir_x[od = w];
        oy += dir_y[od];
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

extern const struct sight* sight_rec;           /* (sight.c's: the record made, but its denizens) */

void __fastcall__ s_way_seen (unsigned char sn, unsigned char face)
{
    const struct sight* s = sight_rec;
    const unsigned char* p;
    struct sden* q;
    unsigned char k, st, on;

    memset (&W, 0, sizeof W);
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
        if (!ahead (d.rel & MOUTH ? BN_PASSAGE : d.kind == PK_PORTCULLIS ? BN_PORTCULLIS : d.kind == PK_WINDOW ?
            BN_WINDOW : d.kind == PK_SLIT ? BN_SLIT : st == DOORWAY ? BN_DOORWAY : st & PS_WRECKED ? BN_BROKEN :
            st & PS_OPEN ? BN_OPEN : BN_DOOR)) {
            continue;
        }
        if (!(st & (PS_OPEN | PS_WRECKED)) && (d.kind == PK_WINDOW ? d.parts & PP_SHUTTERS :
            d.kind != PK_PORTCULLIS && d.kind != PK_SLIT)) {
            b->what = WB_SHUT;                  /* (A door shut, a window's shutters: not seen past) */
            continue;
        }
        ox = d.x;                               /* (Out of its room, or out of its passage that way) */
        oy = d.y;
        od = s->where != W_ROOM ? ((face + d.rel) & 7) >> 1 : oy < ry ? DIR_N : oy >= ry + s->rh ? DIR_S :
            ox < rx ? DIR_W : DIR_E;
        seen_on ();
    }
    if (s->where == W_PASSAGE) {
        for (k = s->nways, p = (const unsigned char*) s->way; k; --k, p += sizeof s->way[0]) {   /* (Its passage's
                                                **   ways on, ahead) */
            d.rel = *p;
            if (ahead (BN_PASSAGE)) {
                ox = s->x;
                oy = s->y;
                od = ((face + d.rel) & 7) >> 1;
                seen_on ();
            }
        }
    }
    hub_map (SIGHT_BANK);                       /* (Into the bank: the record the main RAM's) */
    memcpy (WAY_SEEN, &W, sizeof W);
    WAY_SEEN->nitems = WAY_SEEN->ndens = WAY_SEEN->nglows = 0;
    for (k = s->nitems, on = 0, p = (const unsigned char*) s->item; k; --k, p += sizeof s->item[0]) {   /* (The
                                                **   things ahead, a surface's with it) */
        if (!(((const struct sitem*) p)->hid & 2)) {
            on = is_ahead (((const struct sitem*) p)->rel);
        }
        if (on) {
            memcpy (&WAY_SEEN->item[WAY_SEEN->nitems++], p, sizeof s->item[0]);
        }
    }
    for (k = s->nglows, p = (const unsigned char*) s->glow; k; --k, p += sizeof s->glow[0]) {   /* (The lights far
                                                **   off ahead) */
        if (is_ahead (*p)) {
            memcpy (&WAY_SEEN->glow[WAY_SEEN->nglows++], p, sizeof s->glow[0]);
        }
    }
    for (k = SIGHT->ndens, q = SIGHT->den; k; --k, ++q) {     /* (The denizens ahead: dens.c's, the bank's) */
        if (is_ahead (REL8 (bearing ((int) (q->x - s->x), (int) (q->y - s->y)), face))) {
            memcpy (&WAY_SEEN->den[WAY_SEEN->ndens++], q, sizeof *q);
        }
    }
    hub_map (0);
}
