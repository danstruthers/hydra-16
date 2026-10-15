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
** loud) or heaved up from the far side (P_LIFT, hard), and lowered by the winch only (dropped at once).  A window's
** shutters are worked as a door is; its bars bent (P_FORCE, hard, loud); its glass or its shutters broken by a blow
** (the referee's HC_BLOW at RS_THING: its impact against theirs, loud; bars aren't); an open window climbed through
** (its sill a climb, P_CLIMB: a fall on a critical failure).  A slit's only seen and shot through.  What passes
** them is the senses' (dq0p's view).
**
** Traps: set off as their record says (a plate trodden, a wire walked through: the world's walk stops on them; a door
** opened), each judged by the referee: a pit fallen into (HC_FALL, its depth; a spiked one's spikes, HC_STRIKE) and
** climbed out of (the world's climb); gas resisted (P_RESIST_POISON) or choked on, senseless a while; an alarm a
** noise the denizens hear (E_NOISE); the rest dodged (P_DODGE_HAZARD) or struck by (HC_STRIKE: their aspect, dice
** and place, trap_harm's, their power more).  Noticed as the character steps on one (P_NOTICE_SIGHT, a smaller
** chance than a search's: it stops short), found by a search (P_SEARCH, as hidden doors are; magic's only by magical
** means); one known of is gone round (the world's walks), its door left shut, and disarmed (P_DISARM; magic only by
** magical means; a pit not at all).  Their states kept (TRAP_STATE), a pit's square T_PIT once it's given way.
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

static struct mreq* W;                          /* (The request, the maker's copy: its reply's written into it) */
static unsigned char sn, pd, ks, kx, ky, res, impact;
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
    keep (px, py, st, pq.parts, PORTAL_STATE);
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
** taken; a blow at a thing (HC_BLOW, with bare hands till the world knows what's in them): its impact */
static void __fastcall__ harm (unsigned char a, unsigned char b, unsigned char c, unsigned char w1)
{
    struct req* r;

    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_HARM;
    r->actor = 0;
    r->other = a == HC_BLOW ? RS_THING : 0;
    r->a = a;
    r->b = b;
    r->c = c;
    r->w1 = w1;
    r->w2 = b >> 5;                             /* (A strike's: where, trap_harm's) */
    r->b &= a == HC_STRIKE ? 3 : 0xFF;
    r->text[0] = 0;
    r = hub_ask (SLOT_WORLD);
    impact = r->w1;
    if (a != HC_BLOW) {
        tt += r->w2;
        told (E_HARM, a | r->b << 8 | (r->w2 ? E_OUT : 0), 0);
    }
    r->state = REQ_FREE;
}

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

/* The trap tq (tx, ty) disarmed: a minute's work */
static void disarm (void)
{
    unsigned char o = TO_PIT;
    signed char g;

    if (tq.kind > TK_SPIKED) {
        tt += 600;
        g = test (P_DISARM, tq.state & TS_MAGIC ? HIDE_MAGICAL : HIDE_MECHANICAL, -(signed char) (tq.power >> 1));
        o = res ? TO_CANT : G_OK (g) ? TO_DISARMED : g == G_CF ? TO_FUMBLED : TO_NOT_YET;
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
static unsigned char near_trap (void)
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
static void search (void)
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

/* ---- Portcullises and windows (0xFF: the door's way); through, climbed through to the far side */

#define SILL            3                       /* A window's sill, feet above the floor */
static unsigned char through;

static unsigned char __fastcall__ portcullis (unsigned char what)
{
    unsigned char up = what == PA_RAISE, mine = pq.bar == ((pd + 2) & 3);

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

static unsigned char __fastcall__ window (unsigned char what)
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
    if (what == PA_BREAK) {                     /* (Its shutters, closed; else its glass) */
        part = shut ? PP_SHUTTERS : pq.parts & PP_GLASS;
        if (!part) {
            return pq.parts & PP_BARS ? PO_BARS_HOLD : PO_NOTHING_THERE;
        }
        tt += 20;
        noise (part == PP_GLASS ? 8 : 7);
        harm (HC_BLOW, AS_BEST, 0, 0);
        if (impact < (part == PP_GLASS ? 3 : 12)) {
            return PO_HELD;
        }
        pq.parts &= ~part;
        door_state (pq.state | PS_OPEN);
        return part == PP_GLASS ? PO_SHATTERED : PO_SMASHED;
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
    if (what == PA_DISARM) {
        if (armed == 1) {
            disarm ();
        } else {
            trap_told (TO_NONE);
        }
        return;
    }
    if (pq.kind == PK_PORTCULLIS) {             /* (Never opened: raised) */
        out = portcullis (what == PA_OPEN || what == PA_FORCE ? PA_RAISE : what == PA_CLOSE ? PA_LOWER : what);
    } else if (pq.kind == PK_DOOR || (out = window (what)) == 0xFF) {
        switch (what == PA_BREAK ? PA_FORCE : what) {
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
            }
            break;
        case PA_CLOSE:
            tt += 2 * st;
            if (!(pq.state & PS_OPEN)) {
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
                ((pq.state & (PS_LOCKED | PS_BARRED)) ? 1 : 0));
            tt += 100;
            noise (7);
            if (g > 0) {
                out = PO_FORCED;                /* (Its lock or bar burst, the door open) */
                door_state ((pq.state & (PS_LOCKED | PS_BARRED) ? PS_BROKEN : 0) | PS_OPEN);
            } else {
                out = PO_HELD;
            }
            break;
        default:
            out = what == PA_THROUGH ? PO_NOT_WINDOW : PO_NOT_PORTC;
            break;
        }
    }
    told (E_PORTAL, out | pq.kind << 8, 0);
    if (through) {                              /* (On the far side) */
        W->fx = px + dir_x[pd];
        W->fy = py + dir_y[pd];
    } else if (out == PO_SLIPPED) {
        harm (HC_FALL, strata[sn].style == ST_BUILT ? GR_HARD : GR_ROCKY, HF_CLEAR, SILL);
    } else if (armed && (out == PO_OPENED || out == PO_UNBARRED || out == PO_FORCED)) {
        spring (0);
    }
}

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
