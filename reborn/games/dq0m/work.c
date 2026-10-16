/*
** work.c - (dq0m) the fixtures worked (DUNGEON.md: portals; traps, pits and other nasty surprises), for the world (its
** request, M_WORK): the maker grew them and keeps their changes, so it works them, while the world waits (its events
** and its referee's slot this task's meanwhile, told as the world's).  The world walks the character up to what's
** worked (its door_ahead), and takes what came of it from the reply: where the character is, the time it took, a pit
** to climb out of.
**
** Doors: opened (unbarred, if barred from this side), closed, a lock picked (the referee's Lockcraft, by the tools to
** hand: a minute a try, harder the more complex the lock), forced (Strength against the door's material: its lock or
** bar burst; loud).  A portcullis is never opened: raised by its winch (on its bar's side: a few rounds' cranking,
** loud) or heaved up from the far side (P_LIFT, hard), and lowered by the winch only (dropped at once); down, its
** bars bent apart (P_FORCE, harder than a window's) or broken through by blows (as a door's, off their body), and
** it's a way through for good, jammed (PS_WRECKED).  A door's bar set in its brackets again (bar the door: shut,
** from its side, a bar to hand that spans it: the things' to put back, an IW_PORTAL part again).  A window's
** shutters are worked as a door is; its bars bent (P_FORCE, hard, loud); its glass or its shutters broken by a blow
** (the referee's RS_ATTACK at RS_THING: its impact against theirs, loud; bars aren't); an open window climbed through
** (its sill a climb, P_CLIMB: a fall on a critical failure).  A slit's only seen and shot through.  What passes
** them is the senses' (dq0p's view).
**
** Broken by blows (break: the user's, 2026-10-15): a door battered down, a window's shutters, its glass, its bars,
** each blow an attack it ignores (the referee's RS_ATTACK at RS_THING: the user's, 2026-10-16, after unarmed
** combat), its impact less the material's resistance to it (the less, the worse its state) taken off its body (fixtures.c's, by its material, size, thickness and
** state), the harm kept (PORTAL_HARM); a door broken down open for good (PS_WRECKED).  A fist, a foot or a head that strikes
** it is hurt by a blow gone wrong (the referee's: more by a sturdy door than a rotten one, less with gauntlets, boots,
** a helm), the character stopping when it's hurt enough.  A rotten door forced
** more easily, a new one less.  A portal's parts are items (items.h's IW_PORTAL: a door's bar, a window's pane and
** bars): a bar lifted out (lift: only from its side; or as the door's unbarred to open it) and set down at the
** character's feet, as a pane is worked out whole, bent bars lifted out, or broken glass left in shards.  Looked at
** closely (the UI's, PA_EXAMINE): a portal's record copied into the hub (pview) for the appraiser's words.
**
** Traps: set off as their record says (a plate trodden, a wire walked through: the world's walk stops on them; a door
** opened), each judged by the referee: a pit fallen into (HC_FALL, its depth; a spiked one's spikes, HC_STRIKE) and
** climbed out of (the world's climb); gas resisted (P_RESIST_POISON) or choked on, senseless a while; an alarm a
** noise the denizens hear (E_NOISE); the rest dodged (P_DODGE_HAZARD) or struck by (HC_STRIKE: their aspect, dice
** and place, trap_harm's, their power more).  Noticed as the character steps on one (P_NOTICE_SIGHT, a smaller
** chance than a search's: it stops short), found by a search (P_SEARCH, as hidden doors are; magic's only by magical
** means); one known of is gone round (the world's walks), its door left shut, and disarmed (P_DISARM; magic only by
** magical means; a pit not at all).  Their states kept (TRAP_STATE), a pit's square T_PIT once it's given way.
**
** The rarer work is in swapped parts (../dq0/swap.inc, bank.s's stubs): portcullises and windows (part 2); battering,
** a search, disarming, a close look (part 3).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

unsigned char __fastcall__ portcullis (unsigned char what);     /* (Part 2's stubs) */
unsigned char __fastcall__ window (unsigned char what);
unsigned char __fastcall__ batter (unsigned char mat, unsigned char body);  /* (Part 3's) */
void disarm (void);
unsigned char near_trap (void);
void search (void);
void look_at (void);

static struct mreq* W;                          /* (The request, the maker's copy: its reply's written into it) */
static unsigned char sn, pd, ks, kx, ky, res;
static unsigned px, py, tx, ty;                 /* (The portal's square, the trap's) */
static unsigned long tt;                        /* (The time it takes) */
static struct chunk* K;
static struct portal pq;
static struct trap tq;

/* Square x, y of stratum sn: its chunk in the window (made, if it isn't in the cache), its slot ks, its place in it
** kx, ky; NULL outside the stratum */
static unsigned char* __fastcall__ sq (unsigned x, unsigned y)
{
    struct stratum* s = &strata[sn];
    unsigned ox = x - s->x0, oy = y - s->y0, cx = ox / CW, cy = oy / CW;
    struct cslot* c;

    if (ox >= s->w || oy >= s->h) {
        return 0;
    }
    kx = ox % CW;
    ky = oy % CW;
    hub_map (0);
    for (ks = 0, c = HUB->cslot; ks < NCSLOTS && !(c->used && c->sn == sn && c->cx == cx && c->cy == cy); ++ks, ++c) {
    }
    if (ks == NCSLOTS) {
        ks = make_chunk (sn, cx, cy);
    }
    hub_map (CACHE_BANK (ks));
    K = CHUNK_AT (ks);
    return &K->tile[ky][kx];
}

/* A change kept, and made in its chunk */
static void __fastcall__ keep (unsigned x, unsigned y, unsigned char tile, signed char elev, unsigned char link)
{
    change_keep (sn, x, y, tile, elev, link);
    sq (x, y);
    change_cached (ks, kx, ky, tile, elev, link);
}

static void __fastcall__ door_state (unsigned char st)
{
    keep (px, py, st, pq.parts | PP_KNOWN, PORTAL_STATE);  /* (Handled: a walk may open it again, the user's) */
}

static void __fastcall__ trap_state (unsigned char st)
{
    keep (tx, ty, st, 0, TRAP_STATE);
}

/* An event, the world's (the hub's header in the window after) */
static void __fastcall__ told (unsigned char kind, unsigned w1, unsigned char a)
{
    static struct msg m;

    hub_map (0);
    m.kind = kind;
    m.who = 0;
    m.a = a;
    m.w1 = w1;
    m.w2 = HUB->pc_y;
    if (kind == E_NOISE) {
        m.w1 = HUB->pc_x;
    }
    m.t = HUB->now;
    hub_event (&m);
}

#define noise(loud)     told (E_NOISE, 0, (loud))
#define trap_told(o)    told (E_TRAP, (o) << 8 | tq.kind | tq.trigger << 4, 0)

/* The referee's test of the character's (purpose, b, steps): its grade (res its result) */
static signed char __fastcall__ test (unsigned char purpose, unsigned char b, signed char steps)
{
    struct req* r;
    signed char g;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = purpose;
    r->b = b;
    r->steps = steps;
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    res = r->result;
    r->state = REQ_FREE;
    return g;
}

/* The referee's harm (rs.h's RS_HARM: cause a, and b, c, w1, w2 by it), the UI told (E_HARM), the time it's out for
** taken */
static unsigned char hb;                        /* (The striker's own hurt by its blows at a thing: the worst, told the */
static unsigned hw;                             /*   UI once they're done, portal's; hw's E_OUT, E_STOPPED) */
static void __fastcall__ harm (unsigned char a, unsigned char b, unsigned char c, unsigned char w1)
{
    struct req* r;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_HARM;
    r->actor = 0;
    r->other = 0;
    r->a = a;
    r->b = b;
    r->c = c;
    r->w1 = w1;
    r->w2 = b >> 5;                             /* (A strike's: where, trap_harm's) */
    r->b &= a == HC_STRIKE ? 3 : 0xFF;
    r->text[0] = 0;
    r = hub_ask (SLOT_WORLD);
    tt += r->w2;
    told (E_HARM, a | r->b << 8 | (r->w2 ? E_OUT : 0), 0);
    r->state = REQ_FREE;
}

/* ---- A portal's body (DUNGEON.md: portals): the net harm a leaf (a door, shutters, a portcullis's bars) takes
** before it fails, by its material, its size (feet, world.h's), its thickness (inches) and its state (0 rotten ... 9
** new); a window's bars half a plate's (fixtures.c's, as it makes them, and the blows') */
unsigned char __fastcall__ part_body (unsigned char mat, unsigned char size, unsigned char thick, unsigned char qual)
{
    static const unsigned char per[4] = { 2, 3, 6, 4 };    /* (PM_*: a square foot an inch thick, in quarters) */
    unsigned b = (size & 15) * (size >> 4) * thick * per[mat & 3] / 4 * (2 + qual) / 6;

    return b > 255 ? 255 : b ? b : 1;
}

/* A portal's part (an item: items.h's IW_PORTAL, of kind k, on square x, y) moved out of it, onto square tx, ty (as
** kind nk: a pane broken, shards); 1 if it was there (the window as it was: the hub's header) */
static unsigned char __fastcall__ part_moved (unsigned char k, unsigned x, unsigned y, unsigned char nk, unsigned tx,
    unsigned ty)
{
    struct item* p;
    unsigned i;

    hub_map (0);
    hy_sem_acquire (HUB->item_lock);
    for (i = 1; i < NITEMS; ++i) {
        hub_map (ITEM_BANK (i));
        p = ITEM_AT (i);
        if (p->kind == k && p->where == IW_PORTAL && p->sn == sn && p->x == x && p->y == y) {
            p->kind = nk;
            p->where = IW_SQUARE;
            p->x = tx;
            p->y = ty;
            break;
        }
    }
    hub_map (0);
    hy_sem_release (HUB->item_lock);
    return i < NITEMS;
}

#define AT_BAR          px + dir_x[pq.bar], py + dir_y[pq.bar]  /* (A door's bar's square: on its bar's side) */
#define AT_PC           W->fx, W->fy

/* Blows at the portal's part (its material mat, PM_* or 4 glass; its body): as many as it takes, or a dozen (four
** that make no mark at all: given up), a few seconds each; each an attack the part ignores (the referee's RS_ATTACK at
** RS_THING: the user's, 2026-10-16, "model it after unarmed combat": DUNGEON.md), with what the character strikes
** with (the hub's pc_strike: the tool to hand that hits a thing hardest, a fist, a foot, its head), the material's
** resistance to each aspect its armour (the less, the worse the part's state); the harm past it kept (pq.harm's part,
** its mask hm, its shift hs: a window's shutters' and its bars' a nibble each, a door's the byte): 0 if it gives way,
** else how it stands (PO_GIVING, PO_HOLDING, PO_NO_MARK).  A part of the striker hurt by a blow gone wrong (the
** referee's say, its armour the referee's) is kept (hb, hw), and once it's hurt enough the blows stop */
static unsigned char hm, hs;
static const unsigned char resist[5][4] = {     /* Against blows, edges, points, fire (items.h's DM_*) */
    { 4, 3, 4, 1 }, { 5, 5, 5, 3 }, { 6, 7, 7, 7 }, { 6, 8, 9, 9 }, { 0, 5, 3, 6 }
};

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

unsigned char __fastcall__ s_batter (unsigned char mat, unsigned char body)
{
    struct req* r;
    unsigned char n, h = pq.harm >> hs & hm, was = h, i, k, gone = 0;

    if (body > hm) {
        body = hm;
    }
    for (n = 0; n < 12 && !gone && !(hw & E_STOPPED) && (n < 4 || h != was); ++n) {
        tt += 30;
        hub_map (0);
        r = &HUB->slot[SLOT_WORLD];
        r->kind = RS_ATTACK;
        r->actor = 0;
        r->other = RS_THING;
        r->c = n;
        for (k = 0; k < 4; ++k) {
            i = resist[mat][k];
            r->x[k] = i - (i && pq.qual <= 3 ? (pq.qual <= 1 && i > 1 ? 2 : 1) : 0);
        }
        r = hub_ask (SLOT_WORLD);
        i = r->w1;
        tt += r->w2;
        hb = r->c > hb ? r->c : hb;
        hw |= (r->w2 ? E_OUT : 0) | (r->result & 1 ? E_STOPPED : 0);
        r->state = REQ_FREE;
        if (!i) {
            continue;
        }
        if ((unsigned) h + i >= body) {
            gone = 1;
            h = 0;
        } else {
            h += i;
        }
    }
    pq.harm = (pq.harm & ~(hm << hs)) | h << hs;
    if (h != was) {
        keep (px, py, pq.harm, 0, PORTAL_HARM);
    }
    return gone ? 0 : h * 2 >= body ? PO_GIVING : h ? PO_HOLDING : PO_NO_MARK;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Traps */

#define TH(a, z)        ((a) | (z) << 5)
static const unsigned char trap_harm[TK_GAS] = {    /* A strike's aspect (AS_*) and where (Z_*), by kind; its dice */
    0, 0, TH (AS_POINT, Z_LOW), TH (AS_BLUNT, Z_HIGH), TH (AS_EDGE, Z_MID), TH (AS_POINT, Z_MID)
};
static const unsigned char trap_dice[TK_GAS] = { 0, 0, 1, 3, 2, 1 };

/* K's trap on its square kx, ky that's set (and its door's, door 1, or not): copied into tq (tx, ty: x, y), 1; or 0 */
static unsigned char __fastcall__ trap_on (unsigned x, unsigned y, unsigned char door)
{
    struct trap* t = K->trap;
    unsigned char n;

    for (n = K->ntraps; n; --n, ++t) {
        if (t->x == kx && t->y == ky && TS_SET (t->state) && (t->trigger == TT_DOOR) == door) {
            tq = *t;
            tx = x;
            ty = y;
            return 1;
        }
    }
    return 0;
}

/* The trap tq (on tx, ty) set off, and kept so (on: the character on its square, come from W's fx, fy; else beside
** it) */
static void __fastcall__ spring (unsigned char on)
{
    unsigned char k = tq.kind, o = TO_SPRUNG;

    trap_state ((tq.state | TS_SPRUNG) & ~TS_HIDDEN);
    if (k <= TK_SPIKED) {                       /* (A pit: its cover gives way, the square a pit from now on) */
        keep (tx, ty, T_PIT, (sq (tx, ty), K->elev[ky][kx]), 0xFF);
        trap_told (on ? TO_SPRUNG : TO_OPENED);
        if (on) {                               /* (Fallen in: onto spikes, a strike; and to climb out) */
            if (k == TK_SPIKED) {
                harm (HC_STRIKE, trap_harm[k], 1 + tq.power / 10, tq.power / 5);
            } else {
                harm (HC_FALL, GR_FLOOR, HF_CLEAR, tq.power);
            }
            W->climb = tq.power;
        }
        return;
    }
    if (k == TK_ALARM) {
        noise (9);
    } else if (G_OK (test (k == TK_GAS ? P_RESIST_POISON : P_DODGE_HAZARD, 0, -(signed char) (tq.power >> 1)))) {
        o = k == TK_GAS ? TO_RESISTED : TO_DODGED;
    } else if (k == TK_GAS) {
        o = TO_CHOKED;
        tt += tq.power * 300;                   /* (Senseless, half a minute a step of its power) */
    }
    trap_told (o);
    if (o == TO_SPRUNG && k < TK_GAS) {
        harm (HC_STRIKE, trap_harm[k], trap_dice[k], tq.power);
    }
}

/* A trap stepped onto (W_STEP: tq's square; from W's fx, fy): noticed in time (a magical one never), or set off */
static void step (void)
{
    if (!(tq.state & TS_MAGIC) && G_OK (test (P_NOTICE_SIGHT, 0, -2 * (signed char) tq.hide - 2))) {
        trap_state (tq.state & ~TS_HIDDEN);
        trap_told (TO_NOTICED);
        return;
    }
    spring (1);
    if (!W->climb) {                            /* (On it; or fallen in, to climb out to where it came from) */
        W->fx = tx;
        W->fy = ty;
    }
}

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

/* The trap tq (tx, ty) disarmed, a minute's work a try, in two checks (the user's): its mechanism made out first
** (P_ASSESS: never deadly; a critical success makes the disarm easier), then the disarm (P_DISARM: a critical failure
** sets it off) */
void s_disarm (void)
{
    unsigned char o = TO_PIT;
    signed char g;

    if (tq.kind > TK_SPIKED) {
        tt += 600;
        if (tq.state & TS_MAGIC) {
            test (P_DISARM, HIDE_MAGICAL, 0);   /* (By magic only: the referee's means, or none) */
            o = res ? TO_CANT : TO_NOT_YET;
        } else if (!(tq.state & TS_ASSESSED)) {
            g = test (P_ASSESS, 0, -(signed char) (tq.power >> 1));
            o = G_OK (g) ? TO_ASSESSED : TO_PUZZLED;
            if (o == TO_ASSESSED) {
                trap_state (tq.state | TS_ASSESSED | (g == G_CS ? TS_KEEN : 0));
            }
        } else {
            g = test (P_DISARM, HIDE_MECHANICAL, -(signed char) (tq.power >> 1) + (tq.state & TS_KEEN ? 2 : 0));
            o = G_OK (g) ? TO_DISARMED : g == G_CF ? TO_FUMBLED : TO_NOT_YET;
        }
    }
    trap_told (o);
    if (o == TO_DISARMED) {
        trap_state (tq.state | TS_DISARMED);
    } else if (o == TO_FUMBLED) {
        tq.trigger = 3;                         /* (Its words: no trigger's) */
        spring (0);
    }
}

/* A trap the character knows of in its reach (the eight squares round it, its own): into tq, 1; or 0 */
unsigned char s_near_trap (void)
{
    signed char i, j;
    unsigned x, y;

    for (j = -1; j <= 1; ++j) {
        for (i = -1; i <= 1; ++i) {
            x = W->fx + i;
            y = W->fy + j;
            if (sq (x, y) && (trap_on (x, y, 0) || trap_on (x, y, 1)) && !(tq.state & TS_HIDDEN)) {
                return 1;
            }
        }
    }
    return 0;
}

/* A search about the character (a minute): the referee's Awareness (P_SEARCH) against each hidden door and each
** hidden trap within a few squares that it sees (a trap harder the better it's hidden); a door found is a door like
** any other from then on, a trap known (told as it's found) */
#define SEARCH_R        3
void s_search (void)
{
    unsigned i, j;
    unsigned char found = 0, k;
    unsigned char* p;

    for (j = W->fy - SEARCH_R; j <= W->fy + SEARCH_R; ++j) {
        for (i = W->fx - SEARCH_R; i <= W->fx + SEARCH_R; ++i) {
            if ((p = sq (i, j)) == 0 || !(*p & T_VIS)) {
                continue;
            }
            if ((trap_on (i, j, 0) || trap_on (i, j, 1)) && (tq.state & TS_HIDDEN)) {
                if (test (P_SEARCH, tq.state & TS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL, -(signed char) tq.hide) > 0 &&
                    !res) {
                    trap_state (tq.state & ~TS_HIDDEN);
                    trap_told (TO_FOUND);
                    found |= 2;
                }
                p = sq (i, j);
            }
            if (TILE (*p) != T_DOOR) {
                continue;
            }
            for (k = 0; k < K->nportals && (K->portal[k].x != kx || K->portal[k].y != ky); ++k) {
            }
            if (k == K->nportals || !(K->portal[k].state & PS_HIDDEN)) {
                continue;
            }
            pq = K->portal[k];
            if (test (P_SEARCH, pq.state & PS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL, 0) > 0 && !res) {
                px = i;
                py = j;
                door_state (pq.state & ~PS_HIDDEN);
                found |= 1;
            }
        }
    }
    tt = 600;
    told (E_PORTAL, found & 1 ? PO_FOUND : found ? PO_FOUND_TRAP : PO_NOTHING, 0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Portcullises and windows (0xFF: the door's way); through, climbed through to the far side */

#define SILL            3                       /* A window's sill, feet above the floor */
static unsigned char through;

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

unsigned char __fastcall__ s_portcullis (unsigned char what)
{
    unsigned char up = what == PA_RAISE, mine = pq.bar == ((pd + 2) & 3), g;

    if (pq.state & PS_WRECKED) {
        return PO_JAMMED;                       /* (Its bars forced apart: it won't move, but it's a way through) */
    }
    if (what == PA_BREAK || what == PA_BEND) {  /* (Its bars, down: bent apart, P_FORCE, the thicker the harder; or
                                                **   broken through by blows, off their body: a gap for good) */
        if (pq.state & PS_OPEN) {
            return PO_IS_UP;
        }
        noise (what == PA_BEND ? 7 : 8);
        if (what == PA_BEND) {
            tt += 100;
            if (!G_OK (test (P_FORCE, PM_IRON, -6 - (signed char) pq.thick))) {
                return PO_BARS_HOLD;
            }
            g = PO_BENT;
        } else {
            hm = 0xFF;
            hs = 0;
            if ((g = batter (PM_IRON, pq.tough)) != 0) {
                return g;
            }
            g = PO_GAP;
        }
        door_state (PS_WRECKED | PS_OPEN);
        return g;
    }
    if (!up && what != PA_LOWER) {
        return what == PA_THROUGH ? PO_NOT_WINDOW : PO_NOT_PORTC;
    }
    if (up == (pq.state & PS_OPEN)) {
        return up ? PO_IS_UP : PO_IS_DOWN;
    }
    if (mine) {                                 /* (Its winch: cranked up, or loosed) */
        tt += up ? 300 : 20;
        noise (up ? 5 : 8);
    } else if (!up) {
        return PO_NO_WINCH;
    } else {
        tt += 100;                              /* (By hand, from the far side: a round's heaving) */
        if (!G_OK (test (P_LIFT, 0, -6))) {
            return PO_TOO_HEAVY;
        }
    }
    door_state (pq.state ^ PS_OPEN);
    return !up ? PO_LOWERED : mine ? PO_RAISED : PO_HEAVED;
}

unsigned char __fastcall__ s_window (unsigned char what)
{
    unsigned char shut = (pq.parts & PP_SHUTTERS) && !(pq.state & PS_OPEN), part;
    signed char g;

    if (what <= PA_UNLOCK) {                    /* (Opened, closed, unlocked: its shutters, as a door) */
        return pq.parts & PP_SHUTTERS ? 0xFF : pq.kind == PK_SLIT ? PO_SLIT : PO_NO_SHUTTERS;
    }
    if (what == PA_FORCE) {
        if (shut) {
            return 0xFF;
        }
        if (pq.parts & PP_BARS) {
            tt += 100;
            noise (7);
            if (!G_OK (test (P_FORCE, PM_IRON, -6))) {
                return PO_BARS_HOLD;
            }
            pq.parts &= ~PP_BARS;
            door_state (pq.state);
            return PO_BENT;
        }
        what = PA_BREAK;
    }
    if (what == PA_LIFT) {                      /* (Its glass worked out whole; its bars, once they're bent) */
        if (shut) {
            return PO_IN_WAY;
        }
        if (pq.parts & PP_GLASS) {
            tt += 300;
            pq.parts &= ~PP_GLASS;
            door_state (pq.state);
            part_moved (KI_PANE, px, py, KI_PANE, AT_PC);
            return PO_PANE_OUT;
        }
        return pq.parts & PP_BARS ? PO_SET_FAST : part_moved (KI_BARS, px, py, KI_BARS, AT_PC) ? PO_BARS_OUT :
            PO_NO_BAR;
    }
    if (what == PA_BREAK) {                     /* (Its shutters, closed; else its glass; else its bars) */
        part = shut ? PP_SHUTTERS : pq.parts & PP_GLASS ? PP_GLASS : pq.parts & PP_BARS;
        if (!part) {
            return PO_NOTHING_THERE;
        }
        noise (part == PP_GLASS ? 8 : 7);
        hm = 15;
        hs = part == PP_BARS ? 4 : 0;
        if ((g = batter (part == PP_GLASS ? 4 : part == PP_BARS ? PM_IRON : pq.mat, part == PP_GLASS ? 1 :
            part == PP_BARS ? part_body (PM_IRON, pq.size, 1, pq.qual) >> 1 : pq.tough)) != 0) {
            return g;
        }
        pq.parts &= ~part;
        door_state (pq.state | PS_OPEN);
        if (part != PP_SHUTTERS) {              /* (Its glass in shards at the character's feet; its bars broken out) */
            part_moved (part == PP_GLASS ? KI_PANE : KI_BARS, px, py, part == PP_GLASS ? KI_SHARDS : KI_BARS, AT_PC);
        }
        return part == PP_GLASS ? PO_SHATTERED : part == PP_BARS ? PO_BARS_BROKEN : PO_SMASHED;
    }
    if (what != PA_THROUGH) {
        return PO_NOT_PORTC;
    }
    if (pq.kind == PK_SLIT) {
        return PO_SLIT;
    }
    if (shut || (pq.parts & (PP_GLASS | PP_BARS))) {
        return PO_IN_WAY;
    }
    tt += 100;
    g = test (P_CLIMB, 0, 0);
    through = G_OK (g);
    W->res = through << 1;                      /* (Somewhere new: the world tells it as arrived) */
    return through ? PO_CLIMBED : g == G_CF ? PO_SLIPPED : PO_NO_CLIMB;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* The portal on W's cx, cy worked (what: PA_*), the character before it (pd the way to it) */
static void __fastcall__ portal (unsigned char what)
{
    unsigned char out = 0xFF, armed = 0, k, st = HUB->pcsum.step ? HUB->pcsum.step : 10;
    signed char g;

    sq (px, py);
    for (k = 0; k < K->nportals && (K->portal[k].x != kx || K->portal[k].y != ky); ++k) {
    }
    pq = K->portal[k];                          /* (The world found it: it's there) */
    if (trap_on (px, py, 1)) {                  /* (Its trap: set off as it's opened) */
        armed = 1 | (tq.state & TS_HIDDEN);
    }
    through = 0;
    hb = 0;
    hw = 0;
    if (what == PA_DISARM) {
        if (armed == 1) {
            disarm ();
        } else {
            trap_told (TO_NONE);
        }
        return;
    }
    if (what == PA_BEND && pq.kind != PK_PORTCULLIS) {
        what = PA_FORCE;                        /* (A window's bars bent: forced) */
    }
    if (what == PA_BAR && pq.kind != PK_DOOR) {
        out = PO_NO_BRACKETS;
    } else if (pq.kind == PK_PORTCULLIS) {      /* (Never opened: raised) */
        out = portcullis (what == PA_OPEN || what == PA_FORCE ? PA_RAISE : what == PA_CLOSE ? PA_LOWER : what);
    } else if (pq.kind == PK_DOOR || (out = window (what)) == 0xFF) {
        switch (what) {
        case PA_OPEN:
            tt += 2 * st;
            if (pq.state & PS_OPEN) {
                out = PO_IS_OPEN;
            } else if ((pq.state & PS_BARRED) && pq.bar != ((pd + 2) & 3)) {
                out = PO_BARRED;                /* (Barred from the far side) */
            } else if (pq.state & PS_LOCKED) {
                out = PO_LOCKED;
            } else if (armed == 1) {
                out = PO_TRAPPED;               /* (A trap on it it knows of: left alone) */
            } else {
                out = pq.state & PS_BARRED ? PO_UNBARRED : PO_OPENED;
                door_state ((pq.state & ~PS_BARRED) | PS_OPEN);
                if (out == PO_UNBARRED) {       /* (Its bar lifted out, and set down by it) */
                    part_moved (KI_BAR, AT_BAR, KI_BAR, AT_PC);
                }
            }
            break;
        case PA_LIFT:                           /* (Its bar lifted out, the door left shut) */
            tt += 2 * st;
            out = !(pq.state & PS_BARRED) ? PO_NO_BAR : pq.bar != ((pd + 2) & 3) ? PO_BAR_FAR : PO_LIFTED;
            if (out == PO_LIFTED) {
                door_state (pq.state & ~PS_BARRED);
                part_moved (KI_BAR, AT_BAR, KI_BAR, AT_PC);
            }
            break;
        case PA_BAR:                            /* (A bar set in its brackets again: the door shut, from its side;
                                                **   W's link the widest bar to hand, its width: the things', which
                                                **   put it in them, IW_PORTAL, as the UI's told) */
            tt += 2 * st;
            out = !(pq.feat & PF_BRACKETS) || pq.bar != ((pd + 2) & 3) ? PO_NO_BRACKETS : pq.state & PS_BARRED ?
                PO_IS_BARRED : pq.state & PS_OPEN ? PO_SHUT_FIRST : pq.state & PS_BROKEN ? PO_BURST : !W->link ?
                PO_NO_BAR_HELD : W->link <= (pq.size & 15) ? PO_BAR_SHORT : PO_REBARRED;
            if (out == PO_REBARRED) {
                door_state (pq.state | PS_BARRED);
            }
            break;
        case PA_CLOSE:
            tt += 2 * st;
            if (pq.state & PS_WRECKED) {
                out = PO_WRECKED;
            } else if (!(pq.state & PS_OPEN)) {
                out = PO_IS_SHUT;
            } else {
                out = PO_CLOSED;
                door_state (pq.state & ~PS_OPEN);
            }
            break;
        case PA_UNLOCK:
            if (!(pq.state & PS_LOCKED)) {
                out = PO_NO_LOCK;
                break;
            }
            g = test (P_PICKLOCK, pq.lock, -2 * (signed char) pq.lock);
            if (res) {
                out = PO_NO_PICKS;
                break;
            }
            tt += 600;                          /* (A minute a try) */
            if (g > 0) {
                out = PO_PICKED;
                door_state (pq.state & ~PS_LOCKED);
            } else {
                out = PO_NOT_PICKED;
            }
            break;
        case PA_FORCE:
            if (pq.state & PS_OPEN) {
                out = PO_IS_OPEN;
                break;
            }
            g = test (P_FORCE, pq.mat, (pq.mat == PM_WOOD ? 0 : pq.mat == PM_IRONBOUND ? -2 : -4) -
                ((pq.state & (PS_LOCKED | PS_BARRED)) ? 1 : 0) + (pq.qual <= 1 ? 2 : pq.qual <= 3 ? 1 : 0) -
                (pq.qual >= 8));                /* (A rotten door easier, a new one harder) */
            tt += 100;
            noise (7);
            if (g > 0) {
                out = PO_FORCED;                /* (Its lock or bar burst, the door open) */
                door_state ((pq.state & (PS_LOCKED | PS_BARRED) ? PS_BROKEN : 0) | PS_OPEN);
                part_moved (KI_BAR, AT_BAR, KI_BAR, AT_BAR);    /* (Its bar, burst, fallen where it was) */
            } else {
                out = PO_HELD;
            }
            break;
        case PA_BREAK:                          /* (Battered down: its body gone, it's open for good) */
            if (pq.state & PS_OPEN) {
                out = PO_IS_OPEN;
                break;
            }
            noise (8);
            hm = 0xFF;
            hs = 0;
            if ((out = batter (pq.mat, pq.tough)) == 0) {
                out = PO_BROKE_DOWN;
                door_state (PS_WRECKED | PS_BROKEN | PS_OPEN);
                part_moved (KI_BAR, AT_BAR, KI_BAR, AT_BAR);
            }
            break;
        default:
            out = what == PA_THROUGH ? PO_NOT_WINDOW : PO_NOT_PORTC;
            break;
        }
    }
    told (E_PORTAL, out | pq.kind << 8, 0);
    if (hb || hw) {                             /* (The striker hurt by its blows: once, the worst) */
        told (E_HARM, HC_STRUCK | hb << 8 | hw, 0);
    }
    if (through) {                              /* (On the far side) */
        W->fx = px + dir_x[pd];
        W->fy = py + dir_y[pd];
    } else if (out == PO_SLIPPED) {
        harm (HC_FALL, strata[sn].style == ST_BUILT ? GR_HARD : GR_ROCKY, HF_CLEAR, SILL);
    } else if (armed && (out == PO_OPENED || out == PO_UNBARRED || out == PO_FORCED || out == PO_BROKE_DOWN)) {
        spring (0);
    }
}

#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")

/* The portal on px, py looked at closely (PA_EXAMINE: the UI's, the square one it sees): its record copied into the
** hub's pview, its bar 1 if the character's on its bar's side; W's res 1 if there's none */
void s_look_at (void)
{
    unsigned char k;

    W->res = 1;
    if (!sq (px, py)) {
        return;
    }
    for (k = 0; k < K->nportals && (K->portal[k].x != kx || K->portal[k].y != ky); ++k) {
    }
    if (k < K->nportals) {
        pq = K->portal[k];
        hub_map (0);
        pq.bar = dir_x[pq.bar] * (int) (HUB->pc_x - px) + dir_y[pq.bar] * (int) (HUB->pc_y - py) > 0;
        memcpy (&HUB->pview, &pq, sizeof pq);
        W->res = 0;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

void __fastcall__ work (struct mreq* q)
{
    W = q;
    sn = q->sn;
    pd = q->elev;
    px = q->cx;
    py = q->cy;
    tt = 0;
    q->climb = 0;
    q->res = 0;
    switch (q->tile) {
    case W_STEP:
        if (sq (px, py) && trap_on (px, py, 0)) {
            step ();
        }
        break;
    case PA_SEARCH:
        search ();
        break;
    case PA_EXAMINE:
        look_at ();
        break;
    default:
        if (px != NO_SQ) {
            portal (q->tile);
        } else if (near_trap ()) {              /* (PA_DISARM about it: a trap in reach, or the door ahead's) */
            disarm ();
        } else {
            q->res = 1;
        }
        break;
    }
    q->t = tt > 60000UL ? 60000U : (unsigned) tt;
}
