/*
** workpart.h - (dq0w) what each of the world's work's parts (wdoor.c, wblow.c, wwin.c, wtrap.c) has of its own: each
** part includes it after its #pragma code-name, so each has its copy in its own code (../dq0/swap.inc's rule: no
** part's code called from another's but through a stub), only what it says it wants (W_TRAPS, W_HARM, W_PARTS,
** W_FRONT, W_SPRING): the portal's square in the window, its record and its trap, an event told, the referee's harm,
** a portal's part moved, a trap set off.
*/

#define sq(x, y)        at (wk.sn, (x), (y))    /* (Square x, y of the work's stratum: its chunk in the window, C; its
                                                **   place in it cx_, cy_; NULL outside the stratum) */
#define keep(x, y, t, e, l)     change (wk.sn, (x), (y), (t), (e), (l))
#define test(p, b, s)   (wsteps = (s), ask_w (RS_TEST, (p), (b)))   /* (The referee's test of the character's: its
                                                **   grade, wres its result) */
#define AT_BAR          wk.px + dir_x[wk.pq.bar], wk.py + dir_y[wk.pq.bar]     /* (A door's bar's square) */
#define AT_PC           wk.fx, wk.fy

/* A portal's state kept (handled: a walk may open it again, the user's) */
static void __fastcall__ door_state (unsigned char st)
{
    keep (wk.px, wk.py, st, wk.pq.parts | PP_KNOWN, PORTAL_STATE);
}

#ifdef W_TRAPS
static void __fastcall__ trap_state (unsigned char st)
{
    keep (wk.tx, wk.ty, st, 0, TRAP_STATE);
}
#endif

/* An event, the world's (the hub's header in the window after) */
static void __fastcall__ told (unsigned char kind, unsigned w1, unsigned char a)
{
    hub_map (0);
    wk.m.kind = kind;
    wk.m.a = a;
    wk.m.w1 = kind == E_NOISE ? HUB->pc_x : w1;
    wk.m.w2 = HUB->pc_y;
    wk.m.t = HUB->now;
    hub_event (&wk.m);
}

#define noise(loud)     told (E_NOISE, 0, (loud))
#define trap_told(o)    told (E_TRAP, (o) << 8 | wk.tq.kind | wk.tq.trigger << 4, 0)

#ifdef W_HARM
/* The referee's harm (rs.h's RS_HARM: cause a, and b, c, w1, w2 by it), the UI told (E_HARM), the time it's out for
** taken */
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
    wk.tt += r->w2;
    told (E_HARM, a | r->b << 8 | (r->w2 ? E_OUT : 0), 0);
    r->state = REQ_FREE;
}
#endif

#ifdef W_PARTS
/* A portal's part (an item: items.h's IW_PORTAL, of kind k, on square x, y) moved out of it, onto square tx, ty (as
** kind nk: a pane broken, shards); 1 if it was there (the window as it was: the hub's header) */
static unsigned char __fastcall__ part_moved (unsigned char k, unsigned x, unsigned y, unsigned char nk, unsigned tx,
    unsigned ty)
{
    struct item* p;
    unsigned i;

    for (i = 1; i < NITEMS; ++i) {
        hub_map (ITEM_BANK (i));
        p = ITEM_AT (i);
        if (p->kind == k && p->where == IW_PORTAL && p->sn == wk.sn && p->x == x && p->y == y) {
            p->kind = nk;
            p->where = IW_SQUARE;
            p->x = tx;
            p->y = ty;
            break;
        }
    }
    hub_map (0);
    return i < NITEMS;
}
#endif

#if defined (W_FRONT) || defined (W_TRAPS)
/* C's trap on its square cx_, cy_ (x, y in the frame) that's set (and its door's, door 1, or not): copied into tq (tx,
** ty: x, y), 1; or 0 */
static unsigned char __fastcall__ trap_on (unsigned x, unsigned y, unsigned char door)
{
    struct trap* t = C->trap;
    unsigned char n;

    for (n = C->ntraps; n; --n, ++t) {
        if (t->x == cx_ && t->y == cy_ && TS_SET (t->state) && (t->trigger == TT_DOOR) == door) {
            wk.tq = *t;
            wk.tx = x;
            wk.ty = y;
            return 1;
        }
    }
    return 0;
}
#endif

#ifdef W_FRONT
/* The portal on px, py (the world found it: it's there) into pq, and its trap (set off as it's opened), armed */
static void front (void)
{
    unsigned char k;

    sq (wk.px, wk.py);
    for (k = 0; k < C->nportals && (C->portal[k].x != cx_ || C->portal[k].y != cy_); ++k) {
    }
    wk.pq = C->portal[k];
    wk.armed = trap_on (wk.px, wk.py, 1) ? 1 | (wk.tq.state & TS_HIDDEN) : 0;
    wk.through = 0;
    wk.hb = 0;
    wk.hw = 0;
}
#endif

#ifdef W_SPRING
/* ---- A trap set off */

#define TH(a, z)        ((a) | (z) << 5)
static const unsigned char trap_harm[TK_GAS] = {    /* A strike's aspect (AS_*) and where (Z_*), by kind; its dice */
    0, 0, TH (AS_POINT, Z_LOW), TH (AS_BLUNT, Z_HIGH), TH (AS_EDGE, Z_MID), TH (AS_POINT, Z_MID)
};
static const unsigned char trap_dice[TK_GAS] = { 0, 0, 1, 3, 2, 1 };

/* The trap tq (on tx, ty) set off, and kept so (on: the character on its square, come from fx, fy; else beside it) */
static void __fastcall__ spring (unsigned char on)
{
    unsigned char k = wk.tq.kind, o = TO_SPRUNG;

    trap_state ((wk.tq.state | TS_SPRUNG) & ~TS_HIDDEN);
    if (k <= TK_SPIKED) {                       /* (A pit: its cover gives way, the square a pit from now on) */
        keep (wk.tx, wk.ty, T_PIT, (sq (wk.tx, wk.ty), C->elev[cy_][cx_]), 0xFF);
        trap_told (on ? TO_SPRUNG : TO_OPENED);
        if (on) {                               /* (Fallen in: onto spikes, a strike; and to climb out) */
            if (k == TK_SPIKED) {
                harm (HC_STRIKE, trap_harm[k], 1 + wk.tq.power / 10, wk.tq.power / 5);
            } else {
                harm (HC_FALL, GR_FLOOR, HF_CLEAR, wk.tq.power);
            }
            wk.climb = wk.tq.power;
        }
        return;
    }
    if (k == TK_ALARM) {
        noise (9);
    } else if (G_OK (test (k == TK_GAS ? P_RESIST_POISON : P_DODGE_HAZARD, 0, -(signed char) (wk.tq.power >> 1)))) {
        o = k == TK_GAS ? TO_RESISTED : TO_DODGED;
    } else if (k == TK_GAS) {
        o = TO_CHOKED;
        wk.tt += wk.tq.power * 300;             /* (Senseless, half a minute a point of its power) */
    }
    trap_told (o);
    if (o == TO_SPRUNG && k < TK_GAS) {
        harm (HC_STRIKE, trap_harm[k], trap_dice[k], wk.tq.power);
    }
}
#endif

#ifdef W_FRONT
/* What came of it told (out: PO_*), the striker's hurt by its blows (once, the worst), a door's trap set off as it
** opened */
static void __fastcall__ tail (unsigned char out)
{
    if ((out == PO_OPENED || out == PO_UNBARRED || out == PO_FORCED || out == PO_BROKE_DOWN || out == PO_SMASHED ||
        out == PO_RAISED || out == PO_HEAVED) && !wk.armed && !wk.hb && !wk.hw) {
        wk.open = out;                          /* (An opening: told with what it shows, as the look after it's made:
                                                **   dq0w.c's revealed, the user's, 2026-10-22) */
        return;
    }
    told (E_PORTAL, out | wk.pq.kind << 8, 0);
    if (wk.hb || wk.hw) {
        told (E_HARM, HC_STRUCK | wk.hb << 8 | wk.hw, 0);
    }
#ifdef W_SPRING
    if (wk.armed && (out == PO_OPENED || out == PO_UNBARRED || out == PO_FORCED || out == PO_BROKE_DOWN)) {
        spring (0);
    }
#endif
}
#endif
