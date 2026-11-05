/*
** wrope3.c - (dq0w) rope and grapples (wrope.c's the rest, HOUSE.md's), swapped part 95's: the drop a rope goes down or a
** grapple's thrown up, a rope put in place by the R_*'s (wrope.h's), a grapple tied on, thrown up.
*/

#pragma local-strings (on)
#pragma code-name ("SW95CODE")
#pragma rodata-name ("SW95RODATA")

#define RP_THROWN
#include "wrope.h"

void __fastcall__ make_own (unsigned i);        /* (Part 13's) */
void __fastcall__ grow_below (unsigned char n); /* (Part 3's) */
unsigned __fastcall__ lay_body (unsigned char kind, unsigned char sn, unsigned x, unsigned y);

#define ROPE_REACH      9                       /* (Feet: an end hanging lower than that's in reach, dq0w.c's SHAFT_REACH) */
#define GRAPPLE_THROWS  2                       /* (Squares of its short band: kinds.c's throws) */
#define R_M             (*(unsigned*) (iq.into + 0))    /* (The grapple: the other's name's room) */

/* The pit's depth on the chunk's square at () just found (a trap's power; else 10) */
static unsigned char rp_pit (void)
{
    struct trap* q;
    unsigned char n;

    for (n = C->ntraps, q = C->trap; n; --n, ++q) {
        if (q->x == cx_ && q->y == cy_) {
            return q->power ? q->power : 10;
        }
    }
    return 10;
}

/* The drop down from where the character stands (up: the height above it), the R_*'s: a shaft on its own square (the top of
** one going down; going up, one open to the sky its hold at the roof, no top on any stratum); in a pit, up, its edge; a
** ledge, or down a pit, beside it, the four ways, the way it faces first.  1, or 0 none */
#pragma bss-name (push, "SW95RODATA")
#pragma static-locals (push, on)
static unsigned char __fastcall__ rp_scan (unsigned char up)
{
    static const struct opening* o;
    static const struct stratum* st;
    static const struct link* l;
    static unsigned char* p;
    static unsigned char k, n, h, in, sn, t;
    static unsigned x, y, sx, sy;
    static int z, dz;

    k = up;
    hub_map (0);
    R_TSN = R_FSN = sn = HUB->pc_stratum;
    R_TX = R_FX = x = HUB->pc_x;
    R_TY = R_FY = y = HUB->pc_y;
    h = HUB->pc_face & 6;
    R_HEAD = 0;
    z = elev_at (sn, x, y);
    p = at (sn, x, y);
    if ((in = TILE (*p) == T_PIT) != 0) {
        R_FEET = rp_pit ();
    } else for (n = C->nopen, o = C->open; n; --n, ++o) {
        if (o->x == cx_ && o->y == cy_ && o->link != 0xFF && (o->kind == (k ? O_SHAFT_UP : O_SHAFT_DOWN) ||
            (k && o->kind == O_SKYSHAFT))) {
            st = &STRATUM (sn);
            l = &st->link[o->link];
            R_HOW = RK_SHAFT;
            if (l->dug == LD_SKY && k) {
                R_HOW = RK_SKY;
                t = room_of (sn, x, y);
                R_FEET = t == 0xFF ? 0 : C->zone[C->room[t].z0].cmid;
                return 1;
            }
            if (k) {
                R_TSN = l->to;
            } else {
                R_FSN = l->to;
            }
            return 2;
        }
    }
    for (n = 0; n < 4; ++n, h = (h + 2) & 7) {
        sx = x + head_x[h];
        sy = y + head_y[h];
        dz = elev_at (sn, sx, sy) - z;
        if (!(p = at (sn, sx, sy))) {
            continue;
        }
        t = TILE (*p) == T_PIT;
        if (T_PASS (*p) ? in || (k ? dz > STEP_MAX : dz < -STEP_MAX) : t && !k) {
            R_HOW = in || t ? RK_PIT : RK_LEDGE;
            if (k) {
                R_HEAD = (h + 4) & 7;
                R_TX = sx;
                R_TY = sy;
                if (!in) {
                    R_FEET = dz;
                }
            } else {
                R_HEAD = h;
                R_FX = sx;
                R_FY = sy;
                R_FEET = t ? rp_pit () : -dz;
            }
            return 1;
        }
    }
    return 0;
}

/* Thing n no tool out any more (gone where it's put): what's left to put away, the next's */
static void __fastcall__ rp_gone (unsigned n)
{
    static unsigned m;
    static unsigned char k;

    m = n;
    hub_map (0);
    for (k = 0; k < HUB->rs_nout; ++k) {
        if (HUB->rs_out[k] == m) {
            if (!k && HUB->rs_nout == 2) {
                HUB->rs_out[0] = HUB->rs_out[1];
                HUB->rs_home[0] = HUB->rs_home[1];
            }
            --HUB->rs_nout;
            break;
        }
    }
    if (rs_tool == m) {
        rs_tool = HUB->rs_nout ? HUB->rs_out[HUB->rs_nout - 1] : 0;
        rs_using = HUB->rs_nout ? 1 : 0;
    }
}

/* Record i one end of a rope in place, by the R_*'s */
static void __fastcall__ rp_put (unsigned i, unsigned char k, unsigned char sn, unsigned x, unsigned y, unsigned char h)
{
    static struct item* p;

    p = item_at (i);
    p->kind = k;
    p->where = IW_SQUARE;
    p->sn = sn;
    p->x = x;
    p->y = y;
    p->owner = R_END;
    p->fuel = R_HOW | h << 4 | (R_SLIP ? RK_SLIP : 0) | R_FEET << 8;
    p->hide = R_AK;
    p->flags = (p->flags & ~(I_HELD | I_WORN | I_HIDDEN)) | I_KNOWN;
}
#pragma static-locals (pop)
#pragma bss-name (pop)

/* rp_scan's, a shaft's height (its other stratum's record in its slot: a stub, so its statics are done with) */
static unsigned char __fastcall__ rp_drop (unsigned char up)
{
    unsigned char r = rp_scan (up);
    int d;

    if (r == 2) {
        r = up ? R_TSN : R_FSN;
        if (!stratum_in (r)) {
            hub_map (0);
            grow_below (HUB->pc_stratum);
        }
        d = STRATUM (R_TSN).datum;
        d -= STRATUM (R_FSN).datum;
        R_FEET = d;
        r = 1;
    }
    return r;
}

/* The rope in place, by the R_*'s: its top's and its foot's records */
static void rp_hang (void)
{
    unsigned i;

    make_own (R_N);
    i = ROPE_FEET - R_RUN;
    R_END = R_FEET > i ? R_FEET - i : 0;
    if (R_HOW == RK_SKY) {
        rp_put (R_N, KI_HUNG, R_FSN, R_FX, R_FY, 0);
    } else {
        if (R_TSN != R_FSN) {
            RP_NEED (R_TSN, R_TX, R_TY);
        }
        rp_put (R_N, KI_TIED, R_TSN, R_TX, R_TY, R_HEAD);
        if ((i = lay_body (KI_HUNG, R_FSN, R_FX, R_FY)) != 0) {
            make_own (i);
            rp_put (i, KI_HUNG, R_FSN, R_FX, R_FY, (R_HEAD + 4) & 7);
        }
    }
    rp_gone (R_N);
    hub_map (0);
    ++HUB->ropes;
}

/* Thing m tied on to rope R_N (one thing now) */
static void __fastcall__ rp_join (unsigned m)
{
    make_own (m);
    ip = item_at (m);
    ip->where = IW_ITEM;
    ip->x = R_N;
    ip->flags &= ~(I_HELD | I_WORN);
    rp_gone (m);
}

/* A grapple thrown up ("throw the grapple up", "throw the rope up", the rope tied to it first if it isn't): a round's; the
** book's Throwing test, its missile bands twice as hard straight up (Combat 15's, the house's), a critical success sure, a
** marginal one catching, one in four to give under a climber's weight; failed, it clatters back down.  Its noise heard,
** told */
static unsigned char rp_up (void)
{
    struct req* r;
    unsigned char band, d;
    signed char g;

    R_N = rp_carried (KI_ROPE);
    R_M = rp_carried (KI_GRAPPLE);              /* (Tied on, or not) */
    if (!R_N) {
        return rp_no ("\nYou have no rope.");
    }
    if (!R_M) {
        return rp_no ("\nA rope alone won't catch: it wants a grapple.");
    }
    if (!rp_drop (1)) {
        return rp_no ("\nThere's nothing above you here to catch on.");
    }
    if (R_HOW == RK_SKY && R_FEET <= ROPE_REACH) {
        return rp_no ("\nIts mouth's in reach: climb up.");
    }
    if ((R_HOW != RK_SKY && rp_on (R_TSN, R_TX, R_TY)) || rp_on (R_FSN, R_FX, R_FY)) {
        return rp_no ("\nThere's a rope there already.");
    }
    d = R_FEET * 2 / 5;
    for (band = 0; band < 4 && d > (GRAPPLE_THROWS << band); ++band) {
    }
    if (band == 4 || R_FEET > ROPE_FEET + ROPE_REACH) {
        return rp_no ("\nThat's too high to throw it.");
    }
    if (reach_step (R_N, RS_TOOL)) {
        return IR_NO;
    }
    if (item_at (R_M)->where != IW_ITEM || item_at (R_M)->x != R_N) {     /* (Tied on first, as it's thrown) */
        rp_join (R_M);
    }
    hub_map (0);
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TEST;
    r->actor = 0;
    r->a = P_THROW;
    r->b = 0;
    r->steps = -4 * (signed char) band;         /* (20 less a band) */
    r = hub_ask (SLOT_WORLD);
    g = r->grade;
    r->state = REQ_FREE;
    rp_noise (G_OK (g) ? 4 : 5);
    rp_new ();
    if (G_OK (g)) {
        R_RUN = 0;
        R_AK = KI_GRAPPLE;
        hub_map (0);
        R_SLIP = g != G_CS && !((unsigned char) HUB->now & 3);
        rp_hang ();
        put ("\nThe grapple flies up and catches, with a clang.");
    } else {
        put ("\nThe grapple flies up, finds nothing to bite on, and clatters back down.");
    }
    hub_map (0);
    return rp_done (HUB->round);
}

/* (Its stub's: 0 a grapple thrown up; RA_SCAN the drop down, the R_*'s; RA_HANG in place by them; RA_JOIN | m thing m tied on;
** RA_GONE | m thing m no tool out) */
unsigned char __fastcall__ s_rope3 (unsigned w)
{
    if (w & RA_JOIN) {
        rp_join (w & ~RA_JOIN);
    } else if (w & RA_GONE) {
        rp_gone (w & ~RA_GONE);
    } else if (w == RA_SCAN) {
        return rp_drop (0);
    } else if (w == RA_HANG) {
        rp_hang ();
    } else {
        return rp_up ();
    }
    return 0;
}
