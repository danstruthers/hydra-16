/*
** sound.c - (dq0p) the sound layer and the air (slices 4.5 and 4.6: DUNGEON.md's perception as layers, HOUSE.md's
** sound and stealth), the senses' cache as the light layer's is: a pulse from the character's square over the squares
** round it (the hub's SIGHT_BANK, SOUND_AT: SND_R each way), how far a sound goes between it and each square it
** reaches, round corners, through what lets sound through (an open door, a doorway, a portcullis, a grating, an
** arrow slit; a shut door or a closed window muffling it, DOOR_SND squares more; a room's wall four a square of it,
** SOUND.md's; never the rock a passage is cut in), each square's
** byte marked SND_AIR if a shut door or a closed window was on the way (the air doesn't pass: the air layer's); worked
** out (part 5's pulse, pulse.s's) as a denizen's senses ask and it's out of date: the character moved, the world's
** doors or squares changed (light.c's E_LIGHT), a chunk it reached not in the cache.  With it the background that
** masks quieter sounds, its sources in the squares it reaches: water (a well's, a cistern's: 2), wind in a shaft or at
** the way out (the draught's), each the less a level every four squares off, and a crowd (three or more others within
** three squares, awake: 1); and the draught on the character's stratum (the hub's air_h, air_s): from its way up
** towards its way down, in plan, 1 if it has both, a level more for the way out to the open air, another for a
** shaft; none, still air.  The same for every actor: the sound's way from the character to a denizen is the way back.
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/level.h"
#include "../dq0w/world.h"
#include "light.h"

extern unsigned char at_bank;                   /* (chunk.c's: at ()'s chunk's bank) */

#pragma static-locals (on)

void __fastcall__ pulse (unsigned char sn, unsigned x, unsigned y);    /* (Part 5's: bank.s's stub) */
void __fastcall__ pulse_run (unsigned char* centre);                    /* (pulse.s's, part 5's) */
void __fastcall__ fill_rect (unsigned char h);                         /* (pulse.s's: a chunk's squares into the grid) */
extern unsigned char* fl_src;
extern unsigned char* fl_dst;
extern unsigned char fl_su, fl_sx, fl_du, fl_dx, fl_w, fl_cj, fl_nd, fl_dsl[], fl_dsh[], fl_ddl[], fl_ddh[], fl_dcj[];
extern unsigned char hub_u[], hub_x[];          /* (hub.c's, HUB_UX: each bank's U and $00) */

unsigned char sgen;                             /* (Bumped as the world's doors and squares change: light.c's) */
static unsigned char sgen_ = 0xFF;              /* (As of the layer's last pulse) */

/* The layer brought up to date for the character where it is (the hub's header in the window after) */
void snd_need (void)
{
    static unsigned char sn;
    static unsigned x, y;

    hub_map (0);
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    hub_map (SIGHT_BANK);
    if (!SNDHEAD->ok || SNDHEAD->sn != sn || SNDHEAD->x != x || SNDHEAD->y != y || sgen_ != sgen) {
        sgen_ = sgen;
        pulse (sn, x, y);
    }
    hub_map (0);
}

/* ==== Part 5: the pulse, and the layer read (swapped: a few times a minute at most; and the denizens' senses
** answered, sense.c's, once a denizen's turn) */
#pragma code-name (push, "SW5CODE")
#pragma rodata-name (push, "SW5RODATA")

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? dx : dy;
}

/* How far a sound goes from the character to square x, y of its stratum: the layer's (SND_COST, SND_AIR), or
** SND_NONE (past it, or not reached; the layer up to date: snd_need's) */
unsigned char __fastcall__ snd_cost (unsigned x, unsigned y)
{
    static int dx, dy;
    static unsigned char c;

    hub_map (SIGHT_BANK);
    dx = x - SNDHEAD->x;
    dy = y - SNDHEAD->y;
    c = dx < -SND_R || dx > SND_R || dy < -SND_R || dy > SND_R ? SND_NONE : SND_SQ (dx, dy);
    hub_map (0);
    return c >= 0x80 ? SND_NONE : c;
}

/* The background at square x, y of the character's stratum, a hearer's (k: an actor's entry plus 1, 0 the
** character): the loudest of the layer's sources there, each the less a level every four squares off; and a crowd,
** three or more others within three squares, awake (the character one of them, for an actor); its kind in bk */
unsigned char bk;
unsigned char __fastcall__ snd_mask (unsigned x, unsigned y, unsigned char k)
{
    static unsigned char i, n, l, b, d;
    static struct actor* q;

    b = 0;
    bk = BG_NONE;
    hub_map (SIGHT_BANK);
    for (i = 0; i < SNDHEAD->nbg; ++i) {
        d = apart (x - SNDHEAD->bg[i].x, y - SNDHEAD->bg[i].y) >> 2;
        l = SNDHEAD->bg[i].lv & 15;
        if (l > d && l - d > b) {
            b = l - d;
            bk = SNDHEAD->bg[i].lv >> 4;
        }
    }
    hub_map (0);
    n = k && apart (x - HUB->pc_x, y - HUB->pc_y) <= 3;
    for (i = 0, q = HUB->actor; i < NACTORS; ++i, ++q) {
        if (i + 1 != k && q->state && q->sn == HUB->pc_stratum && q->doing != DO_DOWN && apart (x - q->x, y - q->y) <= 3) {
            ++n;
        }
    }
    if (n >= 3 && !b) {
        b = 1;
        bk = BG_CROWD;
    }
    return b;
}

/* How the draught on the character's stratum goes with a way (dx, dy) from it: 2 straight downwind, 1 within 45
** degrees of it, 0 across it, -1, -2 upwind; times its strength (the hub's air_s: 0 in still air) */
signed char __fastcall__ snd_wind (int dx, int dy)
{
    static unsigned char d;
    static signed char c;

    hub_map (0);
    if (!HUB->air_s || (!dx && !dy)) {
        return 0;
    }
    d = (bearing (dx, dy) - HUB->air_h) & 7;
    c = d == 0 ? 2 : d == 1 || d == 7 ? 1 : d == 2 || d == 6 ? 0 : d == 4 ? -2 : -1;
    return c * (signed char) HUB->air_s;
}

static unsigned char asn_ = 0xFF;               /* (The stratum the draught was last worked out for) */

/* The draught on stratum sn (the hub's air_h, air_s): from its way up (the way out on the top) towards its way down */
static void __fastcall__ draught (unsigned char sn)
{
    static struct stratum* s;
    static struct link* l;
    static unsigned char i, k, up, dn, st;

    s = &STRATUM (sn);
    up = dn = 0xFF;
    st = 1;
    for (i = 0, l = s->link; i < s->nlinks; ++i, ++l) {
        k = l->kind;
        if (k == O_WAY_OUT || k == O_SHAFT_UP || k == O_SHAFT_DOWN) {
            ++st;                               /* (Open to the air above, a shaft: the stronger) */
        }
        if (k == O_WAY_OUT || k == O_STAIRS_UP || k == O_LADDER_UP || k == O_SHAFT_UP) {
            up = up == 0xFF ? i : up;
        } else if (dn == 0xFF) {
            dn = i;
        }
    }
    hub_map (0);
    HUB->air_s = 0;
    if (up != 0xFF && dn != 0xFF) {
        HUB->air_h = bearing (s->link[dn].x - s->link[up].x, s->link[dn].y - s->link[up].y);
        HUB->air_s = st > 3 ? 3 : st;
    }
}

/* A door's or a window's square of chunk C (in the window: its square x, y in it) for the pulse: 0xFD shut (a door,
** a closed window: muffled), 0xFE open (a portcullis, a grating, an arrow slit; a door open or broken down, a window
** open with no glass in it); the rest of the squares the pulse's own, s_pulse's: SND_NONE rock (or no square), 0xFC a
** wall (a sound through it muffled, four squares a square of it), 0xFE open */
static unsigned char __fastcall__ kind_of (unsigned char x, unsigned char y)
{
    static struct portal* q;
    static unsigned char k;

    if ((q = portal_at (x, y)) != 0) {
        k = q->kind;
        if (k == PK_PORTCULLIS || k == PK_GRATE || k == PK_SLIT) {
            return 0xFE;
        }
        if (k == PK_WINDOW ? (q->parts & PP_GLASS) || ((q->parts & PP_SHUTTERS) && !(q->state & PS_OPEN)) :
            !(q->state & (PS_OPEN | PS_WRECKED))) {
            return 0xFD;
        }
    }
    return 0xFE;
}

/* A background's source on square x, y (kind b, level l), if there's room */
static void __fastcall__ source (unsigned x, unsigned y, unsigned char lv)
{
    static unsigned char n;

    hub_map (SIGHT_BANK);
    if ((n = SNDHEAD->nbg) < NBGS) {
        SNDHEAD->bg[n].x = x;
        SNDHEAD->bg[n].y = y;
        SNDHEAD->bg[n].lv = lv;
        SNDHEAD->nbg = n + 1;
    }
}

/* A chunk's sources (the chunk in the window, at ()'s; x0, y0 its first square): its wells and cisterns (water), its
** shafts and its way out (wind, by the draught) */
static void __fastcall__ sources (unsigned x0, unsigned y0, unsigned char wind)
{
    static struct chunk* c;
    static unsigned char i, n, k, rx[NROOMS], ry[NROOMS], ox[NOPEN], oy[NOPEN], ok[NOPEN], m;

    c = C;
    for (i = n = 0; i < c->nrooms; ++i) {
        k = c->room[i].kind;
        if (k == U_WELL || k == U_CISTERN) {
            rx[n] = c->room[i].x + (c->room[i].w >> 1);
            ry[n++] = c->room[i].y + (c->room[i].h >> 1);
        }
    }
    for (i = m = 0; wind && i < c->nopen; ++i) {
        k = c->open[i].kind;
        if (k == O_SHAFT_UP || k == O_SHAFT_DOWN || k == O_WAY_OUT) {
            ox[m] = c->open[i].x;
            oy[m++] = c->open[i].y;
        }
    }
    for (i = 0; i < n; ++i) {
        source (x0 + rx[i], y0 + ry[i], BG_WATER << 4 | 2);
    }
    for (i = 0; i < m; ++i) {
        source (x0 + ox[i], y0 + oy[i], BG_WIND << 4 | wind);
    }
}

/* The layer worked out for the character on stratum sn at x, y: each row of squares read from the chunks the cache
** has (one at () a chunk's run of them), copied into the grid, the pulse run from its square (pulse.s), the
** background's sources gathered from the chunks it reaches, the draught (once a stratum); its own background (the
** hub's pc_mask, pc_bg) */
void __fastcall__ s_pulse (unsigned char sn, unsigned x, unsigned y)
{
    static unsigned char i, k, wind, cj, cb[9];
    static unsigned cx, cy, sx, sy, ox, oy, off, cox[9], coy[9];
    static int gxr, gyr, i0, i1, j0, j1;
    static struct chunk* cp[9];
    static struct stratum* s;

    if (sn != asn_) {
        asn_ = sn;
        draught (sn);
    }
    hub_map (0);
    wind = HUB->air_s ? 1 + (HUB->air_s >= 2) : 0;
    hub_map (SIGHT_BANK);
    SNDHEAD->ok = 0;
    SNDHEAD->nbg = 0;
    s = &STRATUM (sn);
    cx = (x - s->x0) >> CW_BITS;                /* (The nine chunks round the character's, those the cache has: each
                                                **   found once, its sources gathered; the rest rock, till the
                                                **   character moves: no denizen's there, nor anyone's sound) */
    cy = (y - s->y0) >> CW_BITS;
    for (cj = 0; cj < 9; ++cj) {
        cb[cj] = 0;
        cox[cj] = ox = (cx + cj % 3 - 1) << CW_BITS;
        coy[cj] = oy = (cy + cj / 3 - 1) << CW_BITS;
        if (ox < s->w && oy < s->h && cached (sn, sx = s->x0 + ox, sy = s->y0 + oy) && at (sn, sx, sy)) {
            cb[cj] = at_bank;
            cp[cj] = C;
            sources (sx, sy, wind);
        }
    }
    hub_map (SIGHT_BANK);                       /* (The grid: rock, and each chunk's squares in it by their kinds,
                                                **   pulse.s's, a row at a time; a door's or a window's by its
                                                **   portal) */
    memset (SND_GRID, SND_NONE, SND_W * SND_W);
    fl_du = hub_u[SIGHT_BANK];
    fl_dx = hub_x[SIGHT_BANK];
    fl_nd = 0;
    gxr = (int) (x - s->x0) - (SND_R + 1);
    gyr = (int) (y - s->y0) - (SND_R + 1);
    for (cj = 0; cj < 9; ++cj) {
        if (!cb[cj]) {
            continue;
        }
        i0 = (int) cox[cj] - gxr;
        j0 = (int) coy[cj] - gyr;
        i1 = i0 + CW - 1;
        j1 = j0 + CW - 1;
        i0 = i0 < 1 ? 1 : i0;
        j0 = j0 < 1 ? 1 : j0;
        i1 = i1 > SND_W - 2 ? SND_W - 2 : i1;
        j1 = j1 > SND_W - 2 ? SND_W - 2 : j1;
        if (i0 > i1 || j0 > j1) {
            continue;
        }
        fl_su = hub_u[cb[cj]];
        fl_sx = hub_x[cb[cj]];
        fl_src = &cp[cj]->tile[j0 + gyr - (int) coy[cj]][i0 + gxr - (int) cox[cj]];
        fl_dst = SND_GRID + j0 * SND_W + i0;
        fl_w = i1 - i0 + 1;
        fl_cj = cj;
        fill_rect (j1 - j0 + 1);
    }
    for (k = 0; k < fl_nd; ++k) {
        hub_map (cb[cj = fl_dcj[k]]);
        C = cp[cj];
        off = ((unsigned) fl_dsh[k] << 8 | fl_dsl[k]) - (unsigned) &C->tile[0][0];
        i = kind_of (off & (CW - 1), off >> CW_BITS);
        hub_map (SIGHT_BANK);
        *(unsigned char*) ((unsigned) fl_ddh[k] << 8 | fl_ddl[k]) = i;
    }
    hub_map (SIGHT_BANK);
    pulse_run (&SND_SQ (0, 0));
    SNDHEAD->sn = sn;
    SNDHEAD->x = x;
    SNDHEAD->y = y;
    SNDHEAD->ok = 1;
    i = snd_mask (x, y, 0);
    hub_map (0);
    HUB->pc_mask = i;
    HUB->pc_bg = bk;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
