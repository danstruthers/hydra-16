/* walk.inc - (the world's dq0w.c, part 21: the character's ways across a room or about a passage; the denizens'
** dq0d.c, part 18: an actor's way to its mark) one way-finder over one rule for a step (ENGINE-CRITIQUE.md's §4,
** step 2's second part, 2026-11-09).  A shared source, as line.inc is: not yet the paged ROM's (it reads the chunks
** through each task's own at; it goes there with the chunk lookup).
**
** The rule (the one chunk.c's step_ok and slant.s's slant go by, a step at a time): a square can be stood on if its
** kind can (world.h's T_PASS: not rock, a wall, a window, a pit, a cut), and a door's only if it's open (or broken
** down) and not hidden; a step to the square beside it rises or falls no more than STEP_MAX feet (STAIR_MAX on steps,
** either end); a diagonal step only a diagonal way's, past two corners neither of which can be stood on (rock, a wall,
** a door, a window, a pit), one of them cut.  Each walker's differences, its rules (wk_rules), and what its caller
** adds as it reads the squares in (its cell cleared of WC_OPEN: a square it won't go onto):
**
**   - WK_HANDS: a shut door (not a hidden one) a way, to be worked as it gets there (opened, a bar lifted, a key or a
**     pick tried, forced; a portcullis heaved: a denizen's door_way; the character's, a door it has handled opened as
**     it goes, across's door_on).  One with no hands (a beast) goes through no shut door.
**   - WK_NODOOR: no door's square a way, open or shut (the character's ways about a passage: it keeps to it, its
**     doors the walk's to stop at).
**   - the denizens': no water deeper than it wades (unless it swims or flies) and no bad air (dq0d's wades, the same
**     its steps go by); a chunk not made, none (never made for a way).
**   - the character's: no square a denizen stands on, nor one with a trap it knows of (a hidden one's on its way: the
**     way's first, WC_TRAP); its times by its pace (WK_REACHED: cost's, a diagonal seven feet).
**
** The search (wk_search): over a grid of CW by CW cells (a chunk's squares, the world's; a field about the actor and
** its mark, the denizens'), a cell a byte, each square's floor beside it (WK_EL), from the cells queued (wk_ring's,
** WC_SEED: the character's square; the mark, or the field's edges toward it) outward a step at a time, the four
** ways then the diagonal ways in dir_x's order, the first way to each square found kept: its cell then the way it
** was come into (0-7, dir_x's; WC_SEED a seed), WC_STEPS and WC_CORNER kept.  Within the box wk_x0, wk_y0 (wk_w by
** wk_h: the room's floor and walls, the world's); stopped as it comes to wk_stop (the actor's square, the denizens').
**
** Its includer defines WK_EL (the floors: signed char[CW * CW]), WK_RING (the queue: unsigned[WK_RN]) and WK_REACHED
** (u, v, e, dz, v8) (a square v reached from u by way e, its floor dz above u's: what else it keeps), all in the bank
** in the window as it searches, and sets wk_cl, wk_rules and the box; slant.s's cornk and t_kinds are the corners'
** and the kinds' */

#define WC_NEW          0x80                    /* Not reached (yet, or ever: one that can't be stood on) */
#define WC_OPEN         0x40                    /* It can be stood on, by the walker's rules */
#define WC_CORNER       0x20                    /* A diagonal may pass its corner: not stood on (cornk's), or cut */
#define WC_STEPS        0x10                    /* Steps (STAIR_MAX) */
#define WC_CUT          0x01                    /* (Not reached) its corner cut: a diagonal way's (T_CUT) */
#define WC_TRAP         0x02                    /* (Not reached) a trap on it the walk would set off: the world's */
#define WC_SEED         8                       /* (Reached) where the search began: the character's square, the mark */
#define WC_WAY          0x0F                    /* (Reached) the way it was come into: 0-7, dir_x's; or WC_SEED */
#define WK_HANDS        0x40                    /* (wk_rules) A shut door a way: it works it */
#define WK_NODOOR       0x80                    /*   No door's square a way */
#define WK_RN           512                     /* The queue's ring (two of the search's fronts at most) */

extern const unsigned char cornk[16];           /* (slant.s's: a diagonal's corner by its kind, 0 stood on, 1 not, 2 cut) */

static unsigned char* wk_cl;                    /* (The grid: CW * CW cells) */
static unsigned char wk_rules;
static unsigned char wk_x0, wk_y0, wk_w, wk_h;  /* (The box searched) */
static unsigned wk_qh, wk_qt, wk_stop;          /* (The queue's head and tail; the square it stops at, 0xFFFF none) */

/* A square's cell, before it's reached (its tile t; q its portal, a door's, or NULL): by the rule, for the walker's
** rules (wk_rules) */
static unsigned char __fastcall__ wk_cell (unsigned char t, const struct portal* q)
{
    static unsigned char e, c;

    e = TILE (t);
    c = cornk[e];
    c = WC_NEW | (c ? WC_CORNER : 0) | (c >> 1) | (e == T_STEPS ? WC_STEPS : 0);
    if (T_PASS (t) && (e != T_DOOR || (!(wk_rules & WK_NODOOR) && (!q || (!(q->state & PS_HIDDEN) &&
        ((q->state & PS_OPEN) || (wk_rules & WK_HANDS))))))) {
        c |= WC_OPEN;
    }
    return c;
}

/* A cell queued (a seed) */
static void __fastcall__ wk_seed (unsigned i)
{
    wk_cl[i] = WC_SEED | (wk_cl[i] & (WC_STEPS | WC_CORNER));
    WK_RING[wk_qt] = i;
    wk_qt = (wk_qt + 1) & (WK_RN - 1);
}

/* The search, from the cells queued */
static void wk_search (void)
{
    static unsigned u, v;
    static unsigned char ux, uy, vx, vy, e, u8, v8, c, i, lim;
    static signed char eu, dz;

    while (wk_qh != wk_qt) {
        u = WK_RING[wk_qh];
        wk_qh = (wk_qh + 1) & (WK_RN - 1);
        if (u == wk_stop) {
            break;                              /* (The actor reached: no further) */
        }
        ux = (unsigned char) u & (CW - 1);
        uy = (unsigned char) (u >> CW_BITS);
        u8 = wk_cl[u];
        eu = WK_EL[u];
        for (e = 0; e < 8; ++e) {
            vx = ux + dir_x[e];
            vy = uy + dir_y[e];
            if (((vx | vy) & ~(CW - 1)) || (unsigned char) (vx - wk_x0) >= wk_w || (unsigned char) (vy - wk_y0) >= wk_h) {
                continue;
            }
            v = (unsigned) vy << CW_BITS | vx;
            if (((v8 = wk_cl[v]) & (WC_NEW | WC_OPEN)) != (WC_NEW | WC_OPEN)) {
                continue;                       /* (Reached already, or not to be stood on) */
            }
            if (e >= 4) {                       /* (A diagonal way: its corners neither stood on, one cut) */
                c = wk_cl[(unsigned) uy << CW_BITS | vx];
                i = wk_cl[(unsigned) vy << CW_BITS | ux];
                if (!(c & i & WC_CORNER) || ((c & (WC_NEW | WC_CUT)) != (WC_NEW | WC_CUT) &&
                    (i & (WC_NEW | WC_CUT)) != (WC_NEW | WC_CUT))) {
                    continue;
                }
            }
            lim = (u8 | v8) & WC_STEPS ? STAIR_MAX : STEP_MAX;
            dz = WK_EL[v] - eu;
            if (dz > (signed char) lim || dz < -(signed char) lim) {
                continue;
            }
            wk_cl[v] = e | (v8 & (WC_STEPS | WC_CORNER));
            WK_REACHED (u, v, e, dz, v8);
            WK_RING[wk_qt] = v;
            wk_qt = (wk_qt + 1) & (WK_RN - 1);
        }
    }
}
