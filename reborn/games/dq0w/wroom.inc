/* wroom.inc - (the world's: dq0w.c includes it, for its walk's own) a room searched whole, "search the room" (the user's
** playtest, 2026-11-02; HOUSE.md's a room searched whole): a long task, its bar its whole time, stopped and asked as any
** (LT_SEARCH), the character on each square in its turn at a slow walk, so the denizens see it there and traps underfoot
** apply.  The room's floor cut into blocks of three squares by three, each block a stop: the ring of blocks along the
** walls first, clockwise from the corner nearest it (a wall on its left hand all the way), then the rings inside them.
** At a stop: each thing in the block in turn (RSW_THING as it's begun, RSW_END, what came of it, as its time's up: the
** UI asks the things then, part 46's; a heap poked through, a thing that holds things gone through, the rest looked
** over), then the block itself, its time, then its tests: the hidden doors and traps within reach (part 83's, wroom.c's),
** the things hidden there (RSW_FLOOR: the things', asked by the UI, the same).  Told as it goes, finds told as they're
** found.  A stop done is kept: a change record, a hole's (DUG: the world's to read, nothing in the chunk) on the wall
** square at the room's northwest corner, its elev RS_MARK and the corner it set out from, its tile how many stops are
** done; so a second search goes on from there, or says it's done; a stop not finished isn't kept.  Every actor's rule
** where it applies: a denizen's search isn't asked for yet; its hook would be the same stops, wk.who its own.  A walk's
** leg goes by step_on, not arrive: no E_ARRIVE, no look told; but a hidden trap on its way, the walk's own way there */
#pragma code-name (push, "SW82CODE")
#pragma rodata-name (push, "SW82RODATA")

enum { RP_WALK = 1, RP_THINGS, RP_FLOOR, RP_TRAP };
#define RS_FLOOR_T      100                     /* (A block's time, tenths: the search's own rate, a minute the 49 squares;
                                                **   by touch 300; a thing's its class's, by touch twice that) */
#define RS_MARK         16                      /* (Its record's elev: no hole's way) */
static unsigned char rs_on;                     /* (What's next, RP_*) */
static unsigned char rs_k, rs_n, rs_c;          /* (The stops done, of how many, and the corner it set out from: 0 the
                                                **   northwest, clockwise) */
static unsigned char rs_w, rs_h, rs_r;          /* (The room's floor's size, and its number in its chunk) */
static unsigned rs_x, rs_y;                     /* (Its northwest square) */
static unsigned rs_item;                        /* (The thing being gone through; else the last, the items in order) */
static unsigned char rs_side, rs_watch;         /* (The side last told, 0xFF none; whether it watched for traps anyway) */

/* Told the UI (E_BEAT's WB_SEARCH: what, b by touch, w2 and x[0..1] as it asks) */
static void __fastcall__ rs_say (unsigned char what, unsigned char b, unsigned w2, unsigned y)
{
    struct msg m;

    hub_map (0);
    m.kind = E_BEAT;
    m.who = 0;
    m.a = 0;
    m.b = b;
    m.w1 = WB_SEARCH | what << 8;
    m.w2 = w2;
    m.x[0] = (unsigned char) y;
    m.x[1] = y >> 8;
    m.t = HUB->now;
    hub_event (&m);
}

/* The search over, the watch as it was: 0 (the main loop's: its long task over, the character free) */
static unsigned char rs_end (void)
{
    hub_map (0);
    HUB->watch = (HUB->watch & ~WA_TRAPS) | rs_watch;
    srch = 0;
    return 0;
}

/* Its record: rec's square and kind */
static void rs_rec (void)
{
    rec.sn = sn_;
    rec.x = rs_x - 1;
    rec.y = rs_y - 1;
    rec.link = DUG;
}

/* The block of stop rs_k (i, j among the blocks), the ring's side it lies on: 0 to 3, the north's on clockwise; 4 inside
** the outer ring */
#pragma bss-name (push, "SW82RODATA")            /* (Its own, wanted only while it runs: no part called) */
#pragma static-locals (push, on)
static unsigned char __fastcall__ rs_block (unsigned char* pi, unsigned char* pj)
{
    unsigned char k, m, w, h, len, p, s;

    k = rs_k;
    m = s = 0;
    w = (rs_w + 2) / 3;
    h = (rs_h + 2) / 3;
    for (;;) {
        len = w == 1 ? h : h == 1 ? w : 2 * (w + h) - 4;
        if (k < len) {
            break;
        }
        k -= len;
        ++m;
        w -= 2;
        h -= 2;
    }
    if (w == 1 || h == 1) {                     /* (A row of blocks: from its nearer end) */
        p = (w == 1 ? rs_c >= 2 : rs_c == 1 || rs_c == 2) ? len - 1 - k : k;
        *pi = w == 1 ? 0 : p;
        *pj = w == 1 ? p : 0;
    } else {
        p = k + (!rs_c ? 0 : rs_c == 1 ? w - 1 : rs_c == 2 ? w + h - 2 : 2 * w + h - 3);
        if (p >= len) {
            p -= len;
        }
        if (p < w - 1) {
            *pi = p;
            *pj = 0;
        } else if ((p -= w - 1) < h - 1) {
            *pi = w - 1;
            *pj = p;
            s = 1;
        } else if ((p -= h - 1) < w - 1) {
            *pi = w - 1 - p;
            *pj = h - 1;
            s = 2;
        } else {
            p -= w - 1;
            *pi = 0;
            *pj = h - 1 - p;
            s = 3;
        }
    }
    *pi += m;
    *pj += m;
    return m ? 4 : s;
}
#pragma static-locals (pop)
#pragma bss-name (pop)

/* Set off for a square of block i, j, the middle first: 0 none it can reach, 1 it's in the block already, 2 on its way
** (its long task the search's: the whole clock of it, its bar the whole; the walk's own times counted on from there) */
static unsigned char __fastcall__ rs_leg (unsigned char i, unsigned char j)
{
    static const unsigned char ox[9] = { 1, 0, 2, 1, 1, 0, 2, 0, 2 }, oy[9] = { 1, 1, 1, 0, 2, 0, 0, 2, 2 };
    unsigned char fx, fy, tx, ty, wi, hi, a, *p, s;
    unsigned long tt = 0xFFFFFFFFUL, t0;
    unsigned base, n, l;

    here ();
    dens_where ();
    at (sn_, mx, my);
    fx = cx_;
    fy = cy_;
    rx0 = mx - fx;
    ry0 = my - fy;
    tx = rs_x + i * 3 - rx0;
    ty = rs_y + j * 3 - ry0;
    wi = rs_w - i * 3;
    hi = rs_h - j * 3;
    if ((unsigned char) (fx - tx) < 3 && (unsigned char) (fx - tx) < wi && (unsigned char) (fy - ty) < 3 &&
        (unsigned char) (fy - ty) < hi) {
        return 1;
    }
    paced ();
    searching = 0;                              /* (Its own tests at each stop: not a search's as it goes, too) */
    at (sn_, mx, my);
    npath = 0;
    ways (rs_r, fx, fy);
    for (a = 0; a < 9; ++a) {
        if (ox[a] < wi && oy[a] < hi && (tt = way (tx + ox[a], ty + oy[a])) != 0xFFFFFFFFUL) {
            break;
        }
    }
    if (a == 9) {
        hub_map (0);
        HUB->pc_going = IDLE;
        return 0;
    }
    t0 = pt0;
    s = pseen;
    hub_map (0);
    l = HUB->pc_task.len;
    at (sn_, mx, my);
    lim_ = 249;                                 /* (No "You feel your way north." at each leg) */
    tx += ox[a];
    ty += oy[a];
    if (cut) {
        path_ways (tx_, ty_);
        on_way ();                              /* (A trap on its way, hidden: noticed as it comes to it, at a search's
                                                **   chance, or set off: part 8's) */
        rs_on = RP_TRAP;
    } else {
        path_ways (tx, ty);
        hub_map (0);
        move.how = A_WENT_TO;
        move.face = bearing ((int) tx - fx, (int) ty - fy);
        set_off (tt);
        bound_for (rx0 + tx, ry0 + ty);
    }
    lim_ = 250;
    hub_map (0);
    base = (unsigned) (HUB->now - t0);
    pseen = s;
    pt0 = t0;
    HUB->pc_task.kind = LT_SEARCH;
    HUB->pc_task.len = l;
    HUB->pc_task.done = base;
    if (pon) {
        pspent = base;
        hy_bank (ovbank[OVBANKS + 1]);
        for (n = 0, p = PATH_AT + 1; n < npath; ++n, p += 3) {
            *(unsigned*) p += base;
        }
        hub_map (0);
    }
    return 2;
}

/* "search the room" begun (go 1: door_work's, I_PORTAL's PA_SEARCH, SF_ROOM), where it left off if it's been at it before;
** or the character's next as it's free (the main loop's, part 48's searched): the walk to the next stop, each thing there,
** the block's own time; or the search's end, stopped or done.  2 busy, 0 over */
unsigned char __fastcall__ s_room_on (unsigned char go)
{
    unsigned char i, j, s, w, touch, c;
    unsigned n, x, y;
    struct kind* q;

    here ();
    if (go) {
        touch = dark ();
        at (sn_, mx, my);
        w = room_of (sn_, mx, my);
        at (sn_, mx, my);
        rs_r = w;
        rs_w = C->room[w].w;
        rs_h = C->room[w].h;
        rs_x = mx - cx_ + C->room[w].x;
        rs_y = my - cy_ + C->room[w].y;
        rs_n = ((rs_w + 2) / 3) * ((rs_h + 2) / 3);
        rs_rec ();
        if (dig_kept (2) && rec.elev >= RS_MARK) {
            rs_k = rec.tile;
            rs_c = rec.elev - RS_MARK;
        } else {
            rs_k = 0;
            rs_c = mx - rs_x >= rs_w >> 1 ? (my - rs_y >= rs_h >> 1 ? 2 : 1) : (my - rs_y >= rs_h >> 1 ? 3 : 0);
        }
        if (rs_k >= rs_n) {
            rs_say (RSW_ALREADY, 0, 0, 0);
            HUB->pc_ready = HUB->now;
            return 0;
        }
        hub_map (0);
        rs_watch = HUB->watch & WA_TRAPS;
        HUB->watch |= WA_TRAPS;                 /* (A slow walk at most; a trap underfoot at a search's chance: paced's,
                                                **   the referee's) */
        srch = 2;
        rs_on = RP_WALK;
        rs_side = 0xFF;
        rs_item = 0;
        pc_cut = 0;
        task (LT_SEARCH, 0xFFFF);
        busy (0);
        rs_say (rs_k ? RSW_GOES_ON : RSW_BEGUN, touch, 0, 0);
        return 2;
    }
    hub_map (0);
    if (pon) {                                  /* (A leg walked, or stopped short) */
        pon = 0;
        npath = 0;
        spend (pace_, (unsigned) (HUB->now - pt0) - pspent);
        pspent = 0;
        hub_map (0);
        HUB->pc_going = IDLE;
        bound_for (HUB->pc_x, HUB->pc_y);
        see (0);
        if (HUB->pc_task.kind == LT_SEARCH && task_news ()) {
            return 2;                           /* (Something new seen: asked, or stopped) */
        }
        hub_map (0);
    }
    if (pc_cut || HUB->pc_task.kind != LT_SEARCH) {
        return rs_end ();                       /* (Stopped: told.  Its stops done are kept already) */
    }
    if (blocked || (rs_on == RP_TRAP && !(wk.m.kind == E_TRAP && wk.m.w1 >> 8 == TO_NOTICED))) {
        event (E_TASK, 0, TE_STOPPED | LT_SEARCH << 8);   /* (A denizen in its way; a trap set off, told) */
        return rs_end ();
    }
    if (rs_on == RP_TRAP) {
        rs_on = RP_WALK;                        /* (A trap noticed in time: gone round now) */
    }
    here ();
    touch = dark ();
    for (;;) {
        if (rs_on == RP_FLOOR) {                /* (The block's time up: its tests) */
            rs_block (&i, &j);
            x = rs_x + i * 3 + 1;
            y = rs_y + j * 3 + 1;
            wk.sn = sn_;
            wk.who = 0;
            wk.fx = x;
            wk.fy = y;
            if (room_tests (touch)) {
                see (0);                        /* (A door found: it's a door now) */
            }
            rs_say (RSW_FLOOR, touch, x, y);
            rs_rec ();
            rec.tile = ++rs_k;
            rec.elev = RS_MARK + rs_c;
            store_put ();
            rs_on = RP_WALK;
        }
        if (rs_on == RP_WALK) {
            if (rs_k >= rs_n) {
                rs_say (RSW_DONE, touch, 0, 0);
                return rs_end ();
            }
            hub_map (0);
            n = (unsigned) (HUB->now - pt0);    /* (Its bar: the time so far, and what's left of the stops, a walk's
                                                **   three squares each) */
            HUB->pc_task.len = n + (rs_n - rs_k) * ((touch ? 300 : RS_FLOOR_T) + 6 * stept);
            s = rs_block (&i, &j);
            if (s != rs_side) {
                rs_say (rs_n == 1 ? RSW_ROUND : s == 4 ? RSW_MIDDLE : rs_side == 0xFF ? RSW_WALL : RSW_CORNER, touch, s, 0);
                rs_side = s;
            }
            rs_on = RP_THINGS;
            rs_item = 0;
            if ((w = rs_leg (i, j)) == 2) {
                return 2;
            }
            if (!w) {
                rs_on = RP_FLOOR;               /* (None of it to be reached: rock, a cave's, or a pit's: as far as it
                                                **   can feel from here) */
                continue;
            }
        }
        if (rs_item) {
            rs_say (RSW_END, touch, rs_item, 0);
        }
        rs_block (&i, &j);                      /* (The next thing lying in the block, in plain sight) */
        x = rs_x + i * 3;
        y = rs_y + j * 3;
        for (n = rs_item; (n = items (n, IN_SQUARE, 0)) != 0; ) {
            if (ip->sn == sn_ && ip->x - x < 3 && ip->y - y < 3 && !(ip->flags & (I_HIDDEN | I_MAGIC))) {
                break;
            }
        }
        if (!n) {
            hub_map (0);
            rs_item = 0;
            rs_on = RP_FLOOR;
            busy (touch ? 300 : RS_FLOOR_T);
            return 2;
        }
        rs_item = n;
        c = ip->kind;
        make_own (n);
        w = item_at (n)->shut & IS_CLOSED;      /* (Closed: its outside looked over; its lid not lifted) */
        hub_map (KIND_BANK);
        q = &CATALOGUE[c];
        s = q->flags & K_CONTAINER ? (w ? RC_PLAIN : RC_HOLDS) : !(q->keeps & KF_ON) ? (q->cap_bulk ? RC_HEAP : RC_PLAIN) :
            c == KI_BODY ? RC_BODY : RC_PLAIN;
        rs_say (RSW_THING | s << 4, touch, c, 0);
        busy ((s == RC_HEAP ? 200 : s == RC_BODY ? 300 : s == RC_HOLDS ? 100 : 50) << touch);
        return 2;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
