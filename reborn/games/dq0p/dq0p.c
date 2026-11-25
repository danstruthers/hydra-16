/*
** dq0p.c - Dingus Quest Zero's senses: a task of its own, started by the UI (dq0p SEG ROLE: the hub's segment and its
** role there, hub.h), that looks about for the character as the world asks (the hub's look): the squares it sees
** now marked in the dungeon's chunks (view.c: in front of it and to its sides, a clear line in plan and in height,
** its sight's range, lit, its night vision and ultravision), and what it perceives recorded in the hub for
** the UI to word (sight.c), the things it sees among it (things.c: the item store's records read, the world's), the
** denizens it sees (dens.c: the world's bodies), the lights it sees far off (eyes.c).  The senses answer for every
** actor (DUNGEON.md: perception as layers): the light layer theirs to work out (light.c), each actor's eyes theirs to
** keep (eyes.c), each denizen's senses theirs to answer, its mind asking (sense.c: the hub's dsense); and the memory of
** what the character has seen theirs (the chunks' seen marks, the walk's squares felt, R_VISITED; each chunk's memory
** and what's drawn on its map: view.c's kept, slice 4.2's).  A look's own work is in swapped parts (bank.s,
** LIBDIR/dq0p.ovl), the answers to the denizens in the main RAM.  It waits on its doorbell (costing nothing) between
** requests.
*/

#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "sight.h"
#include "things.h"
#include "light.h"
#include "eyes.h"
void sense_actor (void);                        /* (sense.c's: the hub's dsense answered) */
void snd_need (void);                           /* (sound.c's: the sound layer up to date) */

static unsigned char sees;                      /* (The denizens seen changed: the look's reply's ch) */
static unsigned char wch;                       /* (And a look one way's near part told: LC_FAR, the far's to come) */
unsigned char here_known;                       /* (Its square known before this look: sight.c's, in the dark) */
unsigned char v_way;                            /* (A look one way, the hub's LK_WAY: view.h's) */
unsigned char v_nom, v_def;                     /* (view.h's: none made for a look one way's far part, or but its rest's
                                                **   own; one wanted not made) */
static const struct stratum* wst;               /* (The character's stratum: its bounds) */
static unsigned rcx, rcy, dcx, dcy;             /* (The rest's own chunk, way_want's; the first wanted not made) */
extern unsigned char wfar;
extern unsigned char sgen;                    /* (sound.c's: bumped as the world's doors, squares, lights change) */

/* A look about for the character, from what the hub has of it (its square, its facing, its light, its senses; the
** squares it walked to get there), the denizens it sees with it */
static unsigned char face, light, sight, dark, sn, own, range;
static unsigned x, y;

void sens_pc (void);                            /* (Part way_near's, below) */
unsigned char rest_seen (void);                 /* (Part way_near's, below: its stub, bank.s's) */
unsigned hox, hoy;                              /* (way.c's seen_on: a far part's first way on held, where it stopped, */
unsigned char hod, hn, hok;                     /*   its way on and its squares; 1 if there's one: rest_seen's) */

/* The character as the hub has it */
static void pc (void)
{
    hub_map (0);
    face = HUB->pc_face;
    own = HUB->pc_light;
    light = HUB->pc_light * (HUB->pcsum.nightvision ? HUB->pcsum.nightvision : 1);  /* (Night vision: further) */
    eye = HUB->pcsum.sight;                     /* (What it makes out at a distance: sight.c's, things.c's) */
    sight = HUB->pcsum.cond & C_BLINDED || HUB->pc_task.kind == LT_SLEEP ? 0xFF : eye;   /* (Asleep, its eyes shut: as a
                                                **   denizen's, the book's, the user's, 2026-11-03) */
    dark = HUB->pcsum.darkvision;               /* (Ultravision's reach, in squares) */
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    range = sight == 0xFF ? 0xFF : 4 + 2 * (sight > 9 ? 9 : sight);   /* (view.c's) */
}

extern unsigned char gw;                        /* (give_way's, below) */
void give_way (void);
#define GIVE_WAY        GW_CMD                  /* (give_way's: below; the one give-way's, hub.h's) */

/* The last whole look as it was made (a look about's or one way's: its view and its record; not one through a door's
** little window), so a look one way's near part can be told from it while its far part's worked out: from the same
** square, facing the same way, the same light, sight and eyes' height, nothing the world's changed since (sound.c's
** sgen: light.c's E_LIGHT, a door, a square, a light: look_near's) */
static struct lk { unsigned x, y; unsigned char sn, face, light, sight, dark, pos, gen; } lk, lkn;
static unsigned char lk_ok;
static void lk_now (void)                       /* (lkn: as they are now, pc ()'s) */
{
    lkn.x = x;
    lkn.y = y;
    lkn.sn = sn;
    lkn.face = face;
    lkn.light = light;
    lkn.sight = sight;
    lkn.dark = dark;
    lkn.pos = HUB->pc_pos;
    lkn.gen = sgen;
}

static unsigned char kept_owed;                 /* (A look one way's memory to keep: after its reply, before the next look) */
static void look (void)
{
    unsigned char first, wd, wn;
    unsigned wx, wy;

    gw = v_way && v_nom != 2 ? GIVE_WAY : 0;    /* (Its giving way: GIVE_WAY ticks a look at most; a look one way's none,
                                                **   its own command's answer, the world waiting on it: 2026-11-17, the
                                                **   normal pace's half second asleep in it; its rest's, after the
                                                **   prompt, as a look about's) */
    pc ();
    first = HUB->look.first;
    wx = HUB->look.wx;
    wy = HUB->look.wy;
    wd = HUB->look.wd;
    wn = HUB->look.wn;
    sync_strata ();                             /* (The strata's records, the world bank's) */
    wst = &STRATUM (sn);
    v_def = 0;
    if (v_nom) {
        wfar = 0;                               /* (Its openings held only for a chunk not made: way.c's seen_on) */
    }
    sens_pc ();                                 /* (Its senses kept: part way_near's, the lamps' after it) */
    things_lamps (sn, x, y);                    /* (The lit things lying about: a torch put down) */
    view (sn, x, y, face, light, sight, dark);
    if (v_nom == 2 && hok && (hub_map (0), !HUB->tpr) && rest_seen ()) {
        hub_map (0);
        HUB->look.ch |= LC_BEGIN;               /* (Its first way on seen further: the rest's lead-in the UI's to tell
                                                **   now, rest_seen's) */
        hub_ring (T_UI);
    }
    if (!v_way) {
        kept (3);                               /* (What it keeps of it: each chunk's memory, part kept's, and a passage's
                                                **   ways' runs for its words; a look one way's memory after its
                                                **   reply, its words the sooner, 2026-11-18: kept_owed; its runs
                                                **   none, a glance's words not its own, part kept not copied in for
                                                **   them: 140,000 cycles of the look across's pause, 2026-11-19) */
    }
    kept_owed = v_way;
    glows (sn, x, y, face, range, own);         /* (Its eyes adapted, the lights it sees far off: eyes.c's) */
    give_way ();
    if (wn) {
        felt (sn, wx, wy, wd, wn);              /* (The squares walked, remembered: felt in the dark; drawn, mapping
                                                **   as it goes in the light, the view's run_d: after it, with what
                                                **   it perceives, part perceive's, one copy of the part a look) */
    }
    first = perceive (sn, x, y, face, own, first);
    sees = dens_seen ();                        /* (The denizens it sees with it: the world's bodies) */
    if (v_way) {
        way_seen (sn, face);                    /* (A look one way's own: what's beyond its openings, what lies
                                                **   ahead; way.c's) */
    }
    hub_map (0);
    HUB->look.first = first;
    lk_ok = !(v_way & ~1) && sight != 0xFF;     /* (As it was made: not through a door's little window, not blind) */
    lk_now ();
    lk = lkn;
}

/* What may let a look one way's far part see more than its near part, the last look's (a glance's reach, range: past it
** only what's lit, in the quarter ahead out to twice it, WAY_R at most): 1, something lights squares past it within a
** look one way's reach (its own light or ultravision reaching that far, the walls round what they light with them; a lit
** thing lying about, the light layer's lamps, its squares within its reach and the walls round them; a lit room in a
** chunk there); 2, a chunk there not made (the far part may make it: "nothing more that way" held back for it).  Over
** rather than under: the way it faces not asked, WAY_R's reach for twice a glance's, a lit room anywhere in its chunks */
static unsigned char wlim;                      /* (way_stop's: the squares a glance is sure of, each way's sum) */
unsigned char wfar, wnear, wheld;               /* (way.c's: far_may's; a near part made now; an opening held back) */
static unsigned char wid, far_next;             /* (way_more's: the looks one way, counted (their records' id); the next
                                                **   look one way a near part's far part) */
static unsigned wa, wb;
static unsigned char wk;

/* Square x, y's chunk, not made (cached's), one the look may make now (view.h's): v_nom 0, any (or outside the
** stratum: none to make); its rest's, way_want's alone; else not, the first wanted kept (dcx, dcy: its rest's to come) */
unsigned char __fastcall__ v_may (unsigned x, unsigned y)
{
    unsigned cx, cy;

    if (!v_nom || x - wst->x0 >= wst->w || y - wst->y0 >= wst->h) {
        return 1;
    }
    cx = (x - wst->x0) / CW;
    cy = (y - wst->y0) / CW;
    hub_map (0);
    if (v_nom == 2 && cx == rcx && cy == rcy && !(HUB->ui_cmd | HUB->ui_keys)) {
        return 1;                               /* (Its rest's own; but a line begun meanwhile, its answer not
                                                **   kept waiting on the chunk's finish and stock: later) */
    }
    if (!v_def) {
        v_def = 1;
        dcx = cx;
        dcy = cy;
    }
    return 0;
}

/* Square wa, wb of the character's stratum in it, but in a chunk not made (a far part's: one it may not make, v_may's) */
static unsigned char unmade (void)
{
    return wa - wst->x0 < wst->w && wb - wst->y0 < wst->h && !cached (sn, wa, wb) && (!v_nom || !v_may (wa, wb));
}

/* wa, wb from the character's square, each made its size; the greater in wa */
static void apart (void)
{
    wa -= x;
    wb -= y;
    if ((int) wa < 0) {
        wa = -wa;
    }
    if ((int) wb < 0) {
        wb = -wb;
    }
}

/* ==== Part way_near (swapped: a look one way's near part's, its far check; the main RAM's room, 2026-11-17) */
#pragma code-name (push, "SW_way_near_CODE")
#pragma rodata-name (push, "SW_way_near_RODATA")
#pragma bss-name (push, "SW_way_near_RODATA")   /* (Its statics its own: none kept from one call to the next) */

/* The squares g0 ... g1 of a way from square v on it: the nearest, gn, and the farthest, gf (255 at most) */
static unsigned g0, g1;
static unsigned char gn, gf;
static void __fastcall__ span (unsigned v)
{
    static unsigned a, b;

    a = g0 - v;
    b = g1 - v;
    gn = (int) a > 0 ? (a > 255 ? 255 : a) : (int) b < 0 ? (-b > 255 ? 255 : -b) : 0;
    a = (int) a < 0 ? -a : a;
    b = (int) b < 0 ? -b : b;
    gf = a > b ? (a > 255 ? 255 : a) : (b > 255 ? 255 : b);
}

/* The character's senses kept, as every actor's (the hub's sens[0]: ONE-PATH.md's 2.2), as it looks: its sight's reach
** lit and by its darkvision (none asleep or blind), its hearing in the one formula's scale (the UI's own), its nose a
** quarter of its smell (HOUSE.md's), its own light, listening as it goes (part way_near's, the look's lamps' after it: the
** main RAM's room) */
void s_sens_pc (void)
{
    static struct asense ps;

    ps.sight = sight == 0xFF ? 0 : range;
    ps.dark = sight == 0xFF ? 0 : dark;
    ps.hear = HUB->pcsum.hearing + 3;
    ps.eyes = PC_DARK;
    ps.nose = HUB->pcsum.smell >> 2;
    ps.light = own;
    ps.flags = AS_SET | (HUB->watch & WA_LISTEN ? AS_LISTEN : 0);
    memcpy (&HUB->sens[0], &ps, sizeof ps);
}

unsigned char s_far_may (void)
{
    static unsigned char f, e, i, j, n, m, rw;
    static unsigned cx, cy;
    static const struct lsrc* l;
    static const struct room* q;

    if (light + 2 > range || dark + 2 > range) {
        return 1;
    }
    for (wk = nls, l = ls; wk; --wk, ++l) {     /* (Its squares within its reach, Chebyshev's: past a glance's reach in
                                                **   a straight line if past two thirds of it so) */
        wa = l->x;
        wb = l->y;
        apart ();
        if (wb > wa) {
            wa = wb;
        }
        e = l->r + 1;
        wa += e;
        if (wa <= WAY_R + 2 * e && wa + wa + wa > range + range) {
            return 1;
        }
    }
    f = 0;
    rw = 2 * range < WAY_R ? 2 * range : WAY_R;
    for (j = 0, wb = y - WAY_R; j < 3; ++j, wb += WAY_R) {     /* (The chunks of the squares WAY_R each way: those */
        for (i = 0, wa = x - WAY_R; i < 3; ++i, wa += WAY_R) {  /*   of nine of them, WAY_R apart) */
            if (unmade ()) {
                f = 2;
            } else if (wa - wst->x0 < wst->w && wb - wst->y0 < wst->h) {
                at (sn, wa, wb);
                cx = wa - ((wa - wst->x0) & (CW - 1));  /* (Its chunk's first square) */
                cy = wb - ((wb - wst->y0) & (CW - 1));
                for (e = C->nrooms, q = C->room; e; --e, ++q) {
                    if (q->flags & R_LIT) {     /* (A lit room, its floor and walls: some within a look one way's
                                                **   reach, not all within two thirds of a glance's) */
                        g0 = cx + q->x - 1;
                        g1 = cx + q->x + q->w;
                        span (x);
                        n = gn;
                        m = gf;
                        g0 = cy + q->y - 1;
                        g1 = cy + q->y + q->h;
                        span (y);
                        if ((n > gn ? n : gn) <= rw && 3 * (m > gf ? m : gf) > 2 * range) {
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return f;
}

/* A look one way's rest at the turns pace (LK_REST's, its squares seen: view's), its first way on told "so far" (hok:
** way.c's seen_on) on past where it stopped as seen_on goes, straight on through the dark, round a bend only where it
** sees the square: 1 if it sees a square of it now, the way on seen further, its sentence the rest's first, "Peering
** on, down the passage ..." (the UI's lead-in told now, LC_BEGIN: the answer's pause in two, D17's, lock4's,
** 2026-11-25); 0, it may tell nothing more (a passage dark past the light: the look across's), no lead-in before it */
static unsigned char __fastcall__ see_past (const unsigned char* p)
{
    static struct portal* q;

    return p && T_CLEAR (*p) && !((TILE (*p) == T_DOOR || TILE (*p) == T_WINDOW) && (q = portal_at (cx_, cy_)) != 0 &&
        !(q->state & PS_OPEN) && q->kind != PK_PORTCULLIS);   /* (way.c's past) */
}

unsigned char s_rest_seen (void)
{
    static unsigned char k, w, d, n;
    static unsigned ux, uy;
    static const unsigned char* p;

    ux = hox;                                   /* (Not wa, wb: way_stop's) */
    uy = hoy;
    d = hod;
    for (n = hn; n < 2 * WAY_R && ux - x + WAY_R <= 2 * WAY_R && uy - y + WAY_R <= 2 * WAY_R; ) {
        if (way_stop (ux, uy) & 2) {
            return 0;                           /* (Another chunk not made round it: held again) */
        }
        for (k = 0, w = 0xFF; k < 8; ++k) {     /* (seen_on's way on) */
            if (!REL_BEHIND (k - d) && (w == 0xFF || k == d) && (!(k & 1) || slant (sn, ux, uy, k)) &&
                see_past (p = at (sn, ux + head_x[k], uy + head_y[k])) && (k == d || (*p & T_VIS))) {
                w = k;
            }
        }
        if (w == 0xFF) {
            return 0;
        }
        ux += head_x[d = w];
        uy += head_y[d];
        ++n;
        if (*at (sn, ux, uy) & T_VIS) {
            return 1;
        }
    }
    return 0;
}

#pragma bss-name (pop)
#pragma rodata-name (pop)
#pragma code-name (pop)
unsigned char far_may (void);                   /* (Its stub, bank.s's) */

/* Square ox, oy of a near part's walk (way.c's seen_on): 2, a chunk round it not made (it goes no further, none made for
** it: the far part's to); 1, past the squares a glance is sure of (a glance's reach less two, each way's sum: the squares
** round them a glance's); 0 */
static const signed char wcx[4] = { -1, 1, -1, 1 }, wcy[4] = { -1, -1, 1, 1 };
unsigned char __fastcall__ way_stop (unsigned ox, unsigned oy)
{
    if ((unsigned char) (ox - wst->x0 + 1) % CW < 2 || (unsigned char) (oy - wst->y0 + 1) % CW < 2) {
        for (wk = 0; wk < 4; ++wk) {            /* (At its chunk's edge: those round it) */
            wa = ox + wcx[wk];
            wb = oy + wcy[wk];
            if (unmade ()) {
                return 2;
            }
        }
    }
    wa = ox;
    wb = oy;
    apart ();
    return wa + wb > wlim;
}

/* A look one way's record r (in the bank, way.c's) of the look one way it's of (wid: its near part's own, or the whole of
** it at once; its far part's its near part's), what it makes out (twice as far: sight.c's), what's more: a near part's,
** WM_NEAR ("nothing more that way" held back, WM_HOLDN, the far part may see more, or an opening's held back); its far
** part's, WM_FAR (WM_SAME, all as its near part had it: nothing more to tell) */
void __fastcall__ way_more (struct wayseen* r)
{
    if (wnear || !far_next) {
        ++wid;
    }
    r->id = wid;
    r->eye = 2 * eye + 2;
    r->more = wnear ? wheld | wfar ? WM_NEAR | WM_HOLDN : WM_NEAR : !far_next ? 0 : WAY_NEAR->id != wid ||
        memcmp (&r->nbeyond, &WAY_NEAR->nbeyond, offsetof (struct wayseen, id) - 1) || ((WAY_NEAR->more & WM_HOLDN) &&
        !(r->nbeyond | r->nitems | r->ndens | r->nglows)) ? WM_FAR : WM_FAR | WM_SAME;   /* (Its near part empty, its
                                                **   "nothing more that way" held back: told now, not the same) */
    if (!wnear && v_def) {
        r->more |= WM_REST;                     /* (Its eyes reached a chunk not made, none made for it: its rest to
                                                **   come, its openings held for it told "so far": not the same as
                                                **   a near part that held them back) */
        if (wheld) {
            r->more &= ~WM_SAME;
        }
    }
    far_next = wnear;                           /* (A near part's far part next) */
}

/* A look one way's near part (LK_NEAR's): told from the last whole look, if it stands as it was made (its squares
** seen, its record: way.c's, what's beyond its openings ahead, what lies ahead, held back what the far part may yet
** change), the far part's look (LK_WAY's) asked next; else none, the whole look one way asked next */
extern const struct sight* sight_rec;           /* (sight.c's: the record made) */
static void look_near (void)
{
    v_way = 1;
    pc ();
    lk_now ();
    if (lk_ok && !memcmp (&lk, &lkn, sizeof lk)) {
        sync_strata ();
        wst = &STRATUM (sn);
        wlim = range - 2;
        wfar = far_may ();
        memcpy (&((struct sight*) sight_rec)->nglows, &G, sizeof G);  /* (The lights far off as they're seen now:
                                                **   eyes.c's, an LK_EYES's since) */
        wnear = 1;
        way_seen (sn, face);
        wnear = 0;
        wch = LC_FAR;
    } else {
        v_nom = 1;                              /* (It doesn't stand: the whole look one way now, as LK_WAY's, its
                                                **   far part's request the less) */
        look ();
        wch = LC_WHOLE;
    }
}

/* A look one way's rest (LK_REST's: the user's, 2026-11-18, "a look into ground not made yet tells what the made ground
** shows first, then the rest once the maker has made it"): the chunk its eyes reached not made (way_want's) made since,
** the far part's look again with it, if it stands as it was made (none else looked since but the denizens and the eyes),
** the far part's record its near's to tell only what's new (way_more's), LC_FAR if there's any; the next chunk not made
** its rest's again.  Its own chunk made, finished (the maker's made it ahead), its things stocked: the world waits on it,
** nothing in hand */
static unsigned char rest_ok;                   /* (A rest to come: the far part's eyes reached a chunk not made) */
static void look_rest (void)
{
    hub_map (0);
    rcx = HUB->way_want.cx;
    rcy = HUB->way_want.cy;
    v_way = 1;
    pc ();
    if (rest_ok && HUB->tpr && (HUB->ui_cmd | HUB->ui_keys)) {   /* (At the real paces; the turns pace's told now
                                                **   whatever the keys, the look's own answer: the user's Ti4, 2026-11-25) */
        HUB->way_want.st = WW_MADE;             /* (A line being typed, or its command in hand: its rest asked again
                                                **   as nothing's in hand, the world's look_due) */
        return;
    }
    lk_now ();
    if (rest_ok && lk_ok && HUB->way_want.sn == sn && !memcmp (&lk, &lkn, offsetof (struct lk, light)) && lk.pos == lkn.pos) {
                                                /* (From the same square, the same way: its light
                                                **   may have changed, a torch burnt down, what it shows now the rest's) */
        hub_map (SIGHT_BANK);
        memcpy (WAY_NEAR, WAY_SEEN, sizeof (struct wayseen));
        far_next = 1;
        v_nom = 2;
        look ();
        hub_map (SIGHT_BANK);
        WAY_SEEN->more |= WM_RESTED;            /* (The rest's: the UI's lead-in, "Peering on, ...") */
        if (memcmp (&WAY_SEEN->nbeyond, &WAY_NEAR->nbeyond, offsetof (struct wayseen, id) - 1)) {
            wch = LC_FAR;
        }
    } else {
        rest_ok = 0;
    }
}

/* The look giving way to the UI's command in hand at the real paces (the hub's ui_cmd: its answer, the world's, the
** UI's words; half a second a look at most; at the turns pace the game waits for the look as it is), a tick at a
** time; but not to one waiting for the world's answer that waits for the look
** (a request of the things but what's carried, the tools, a tool's wear; the world's own, wreq: dq0w.c's waiting);
** the hub's header in the window after */
unsigned char gw;                               /* (Reset as a look begins: its ticks given) */
static unsigned char gk;
void give_way (void)
{
    hub_map (0);
    while (HUB->tpr && HUB->ui_cmd && gw < GIVE_WAY && HUB->wreq.state != REQ_POSTED && (HUB->ireq.state != REQ_POSTED ||
           (gk = HUB->ireq.kind) == IO_LIST || gk == IO_TOOLS || gk == IO_WEAR)) {
        ++gw;
        hy_sleep_ticks (1);
        hub_map (0);
    }
}

/* The swapped parts' images read into banks of its own (dir/dq0p.ovl: dq0p.cfg's OV1 ..., 8K each, as make.js packs
** them, ../dq0/ovlpack.js: each bank packed, read and unpacked in place by the unpack library, the linker's fill ($FF)
** put back after its bytes), all of it the game's library's (modules/dq0lib, ../dq0/dq0lib.inc: 2026-11-09): 0, or
** not 0; and a number, an argument's, its NUM (atoi's, without the C library's ctype) */
unsigned char __fastcall__ ovl_load (const char* dir);
unsigned char __fastcall__ num (const char* s);

/* A denizen's senses held (held: the ticks of it) while the UI's command is in hand at the real paces (the hub's
** ui_cmd: Enter to its answer's first news), the character's look and the world's first, so a mind's ask doesn't ride
** on the answer's priority (the senses in the foreground meanwhile, the kernel's TASKFORE); answered as the answer's
** begun, after GIVE_WAY ticks at most, or at once if the UI's asked the minds something (the sway: it waits on them).
** The mind waits on it in its hub_call, and carries on as it's answered; at the turns pace never (the minds have no
** turn while a command's given): 2026-11-01.  Held from a line's first key too (the hub's ui_keys: dq0hold's, the
** world's long jobs', 2026-11-06: the senses' part copied in and a mind's ask answered as an Enter came, 90,000 cycles
** of an answer's first letter), KEY_HOLD ticks at most while the keys go on, then answered till the line's Enter (its
** command's GIVE_WAY its own, the parser's request's seq a new line's) or the keys let go */
static unsigned char held;                      /* (A denizen's ask held) */
static struct gw gsense;                        /* (The library's one give-way, gw.s's: 2026-11-19, its own copy before) */

static unsigned char sense_held (void)
{
    hub_map (0);
    if (HUB->sway.state == REQ_POSTED) {
        gsense.st = 0;                          /* (The UI's asked the minds: they wait on it) */
        return 0;
    }
    return gw_held (&gsense, 0);
}

void lit_ask (void);                            /* (sense.c's, part den_senses's) */

int main (int argc, char* argv[])
{
    if (argc < 4 || ovl_load (argv[3]) || hub_attach (num (argv[1]), num (argv[2]))) {
        return 1;
    }
    chunks_init (ASK_SENSES);
    light_init ();
    hub_map (SIGHT_BANK);
    WAY_NEAR->more = 0;                         /* (No near part yet: the UI's to tell none) */
    for (;;) {
        hub_map (0);
        if (held) {
            hy_sem_wait (HUB->sem[T_SENSES], 8);    /* (A denizen's ask held: looked at again each eighth tick, or
                                                **   at once as the world asks a look: no tick's wake of its own
                                                **   in an answer's time, a held line's two percent of the CPU,
                                                **   2026-11-06) */
        } else {
            hy_sem_acquire (HUB->sem[T_SENSES]);    /* (Waiting costs nothing) */
        }
        if (HUB->quit) {
            break;
        }
        if (hub_posted (&HUB->look) && HUB->look.kind == LK_LIT) {
            light_events ();                    /* (A square lit or not, the world's ask: every actor's light, the
                                                **   one rule, sense.c's; unlit_at's that was, ONE-PATH.md's 2.2) */
            lit_ask ();
        } else if (hub_posted (&HUB->look)) {
            if (kept_owed) {
                kept_owed = 0;
                kept (2);                       /* (The last look one way's memory kept, before another look) */
            }
            light_events ();                    /* (The world's lights and doors changed: the light layer's out of
                                                **   date near them) */
            if (HUB->look.kind != LK_WAY) {
                far_next = 0;                   /* (A near part's far part only next) */
            }
            v_nom = 0;
            if (HUB->look.kind != LK_DENS && HUB->look.kind != LK_EYES && HUB->look.kind != LK_REST) {
                rest_ok = 0;                    /* (Another look: a look one way's rest owed let go) */
                HUB->way_want.st = WW_NONE;
            }
            if (HUB->look.kind == LK_REST) {
                look_rest ();                   /* (A look one way's rest, the chunk its eyes reached made since) */
            } else if (HUB->look.kind == LK_DENS) {
                sees = dens_seen ();            /* (Only the denizens seen, looked at again: their bodies changed) */
            } else if (HUB->look.kind == LK_EYES) {
                pc ();                          /* (Only its eyes, as time's gone: the lights far off; and the sound
                                                **   layer, for what it hears, a listen's end) */
                sync_strata ();
                snd_need ();
                glows (sn, x, y, face, range, own);
                hub_map (SIGHT_BANK);
                memcpy (&SIGHT->nglows, &G, sizeof G);
            } else if (HUB->look.kind == LK_NEAR) {
                look_near ();                   /* (A look one way's near part, told first: its far part LK_WAY's) */
            } else {
                v_way = HUB->look.kind == LK_PEEK ? 1 | VP_ASK : HUB->look.kind == LK_WAY;   /* (A look one way:
                                                **   twice as far in the quarter ahead, and what's beyond its
                                                **   openings: way.c's; LK_PEEK, through a door's little window) */
                v_nom = HUB->look.kind == LK_WAY;   /* (Its far part, or the whole of it: no chunk made for it but
                                                **   those a glance reaches, the rest as they're made: the user's,
                                                **   2026-11-18) */
                look ();
            }
            if (v_nom) {
                rest_ok = v_def;                /* (Its eyes reached a chunk not made: the maker to make it ahead,
                                                **   the world to ask its rest as it has) */
                if (v_def) {
                    hub_map (0);
                    HUB->way_want.sn = sn;
                    HUB->way_want.cx = dcx;
                    HUB->way_want.cy = dcy;
                    HUB->way_want.st = WW_WANTED;
                }
                v_nom = 0;
            }
            hub_map (0);
            HUB->look.ch = sees | wch;
            wch = 0;
            hub_reply (&HUB->look);
            if (HUB->tpr) {
                snd_need ();                    /* (At the real paces, the sound layer for where the character is
                                                **   now, as its look's answered: not as the minds first ask, which may
                                                **   be in the next command's answer, 2026-11-01) */
            }
        }
        held = hub_posted (&HUB->dsense) && sense_held ();
        if (!held && hub_posted (&HUB->dsense)) {   /* (A denizen's senses, its mind asking: sense.c's; after the
                                                **   character's look, if both are asked: its answer first, the sound
                                                **   layer's pulse, the dearer, after it) */
            light_events ();
            sense_actor ();
            hub_reply (&HUB->dsense);
        }
    }
    return 0;
}
