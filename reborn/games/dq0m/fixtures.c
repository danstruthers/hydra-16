/*
** fixtures.c - (dq0m) a chunk's materials and its fixtures (DUNGEON.md), once its grower's grown it, in the main RAM:
** each room's floor, walls and ceiling, and its passages' (the rock they're cut in), by its stratum's style and age
** (a built room's walls masonry, its floor flagged or bare earth, more earth the older; a cave's earth or stone, its
** walls the rock; a mine's all stone); windows (glass, shutters, bars: any of them) and arrow slits in a room's
** walls where a passage runs beside them; portcullises for some doors; traps (pits, open or hidden; deadfalls,
** blades, darts, gas, alarms, a trodden plate's or a wire's; a door's, set off as it's opened).  Every proportion the
** style's (no windows, portcullises or set traps in a natural cave: only its pits).
**
** Each with its own dice, from the chunk's place and the fixture's (as pass.c's doors' are), so the grower's layout
** never moves for them; and nothing that would part a chunk's ways: an open pit only where the squares round it
** are floor, as level as a walk.
**
** (Overlay 5, the maker's: make.c calls it through bank.s's stub.)
*/

#pragma code-name ("OV5CODE")
#pragma rodata-name ("OV5RODATA")

#include "../dq0/rs.h"
#include "../dq0w/world.h"
#include "grow.h"

/* The fixtures' dice: from the chunk's place, what's being decided (a), and where (b) */
static unsigned long dh;
static void __fastcall__ dseed (unsigned char a, unsigned b)
{
    dh = ((unsigned long) L->cx << 20 ^ (unsigned long) L->cy << 8 ^ (unsigned long) a << 28 ^ b) * 2654435761UL + 7;
}

static unsigned char __fastcall__ dr (unsigned char n)
{
    dh ^= dh << 13;
    dh ^= dh >> 17;
    dh ^= dh << 5;
    return (unsigned char) ((dh >> 8) % n);
}

/* ---- Materials */

static void materials (void)
{
    struct stratum* s = &strata[L->sn];
    unsigned char r, bare = s->style == ST_BUILT || s->style == ST_MIXED ? MAT_FLAGS : MAT_STONE;    /* (A floor's) */
    unsigned char old = s->age / 4;
    unsigned char* m;

    dseed (1, 0xFF);                            /* (The passages': their floors, the rock, their ceilings) */
    m = L->mat[NROOMS];
    m[MP_FLOOR] = dr (20) < SP->p_earth + (SP->p_earth ? old : 0) ? MAT_EARTH : bare;
    m[MP_WALL] = MAT_STONE;
    m[MP_CEIL] = MAT_STONE;
    for (r = 0; r < NROOMS; ++r) {
        m = L->mat[r];
        if (r >= L->nrooms) {                   /* (None: the passages', so any square has its own) */
            m[MP_FLOOR] = L->mat[NROOMS][MP_FLOOR];
            m[MP_WALL] = m[MP_CEIL] = MAT_STONE;
            continue;
        }
        dseed (1, r);
        m[MP_FLOOR] = dr (20) < SP->f_earth + (SP->f_earth ? old : 0) ? MAT_EARTH : bare;
        m[MP_WALL] = dr (20) < SP->w_built ? MAT_MASONRY : MAT_STONE;
        m[MP_CEIL] = dr (20) < SP->c_built ? MAT_MASONRY : MAT_STONE;
    }
}

/* ---- Portals: windows and slits, portcullises */

static struct portal* __fastcall__ portal (unsigned char x, unsigned char y, unsigned char kind)
{
    struct portal* p;

    if (L->nportals >= NPORTALS) {
        return 0;
    }
    p = &L->portal[L->nportals++];
    p->x = x;
    p->y = y;
    p->kind = kind;
    p->state = PS_OPEN;
    p->lock = 0;
    p->mat = PM_STONE;
    p->tough = 0;
    p->bar = DIR_N;
    p->parts = 0;
    return p;
}

/* Room r's walls: on each side, a window or a slit (the style's say) where a passage runs beside it */
static void __fastcall__ windows (unsigned char r)
{
    struct room* q = &L->room[r];
    struct portal* p;
    unsigned char d, k, len, x, y, t;

    for (d = 0; d < 4; ++d) {
        len = d == DIR_N || d == DIR_S ? q->w : q->h;
        dseed (2, r << 2 | d);
        if (dr (20) >= SP->windows) {
            continue;
        }
        for (k = dr (len), t = 0; t < len; ++t, k = k + 1 < len ? k + 1 : 0) {
            x = d == DIR_N || d == DIR_S ? q->x + k : d == DIR_W ? q->x - 1 : q->x + q->w;
            y = d == DIR_W || d == DIR_E ? q->y + k : d == DIR_N ? q->y - 1 : q->y + q->h;
            if (x + dir_x[d] >= CW || y + dir_y[d] >= CW || L->tile[y][x] != T_WALL ||
                TILE (L->tile[y - dir_y[d]][x - dir_x[d]]) != T_FLOOR ||
                TILE (L->tile[y + dir_y[d]][x + dir_x[d]]) != T_CORR) {
                continue;
            }
            if (!(p = portal (x, y, dr (20) < SP->slits ? PK_SLIT : PK_WINDOW))) {
                return;
            }
            L->tile[y][x] = T_WINDOW;
            p->bar = (d + 2) & 3;               /* (Its shutters barred from the room's side) */
            if (p->kind == PK_WINDOW) {
                p->parts = (dr (4) < SP->w_glass ? PP_GLASS : 0) | (dr (4) < SP->w_shut ? PP_SHUTTERS : 0) |
                    (dr (4) < SP->w_bars ? PP_BARS : 0);
                if ((p->parts & PP_SHUTTERS) && dr (2)) {
                    p->state = dr (4) ? 0 : PS_BARRED;  /* (Its shutters closed, some barred) */
                }
                p->mat = p->parts & PP_BARS ? PM_IRON : PM_WOOD;
                p->tough = p->parts & PP_BARS ? 60 : p->parts & PP_SHUTTERS ? 15 : 5;
            }
            break;
        }
    }
}

/* Some doors portcullises instead (iron bars: raised, or down) */
static void portcullises (void)
{
    struct portal* p;
    unsigned char i;

    for (i = 0; i < L->nportals; ++i) {
        p = &L->portal[i];
        dseed (3, p->x << 5 | p->y);
        if (p->kind != PK_DOOR || (p->state & PS_HIDDEN) || dr (20) >= SP->portc) {
            continue;
        }
        p->kind = PK_PORTCULLIS;
        p->state = dr (2) ? PS_OPEN : 0;
        p->lock = 0;
        p->mat = PM_IRON;
        p->tough = 90;
        p->parts = PP_BARS;
    }
}

/* ---- Traps */

static void __fastcall__ trap (unsigned char x, unsigned char y, unsigned char kind, unsigned char trigger)
{
    struct trap* t;

    if (L->ntraps >= NTRAPS) {
        return;
    }
    t = &L->trap[L->ntraps++];
    t->x = x;
    t->y = y;
    t->kind = kind;
    t->trigger = trigger;
    t->state = 0;
    t->hide = 0;
    t->power = 1 + dr (9);
    if (kind == TK_PIT) {                       /* (An open pit: in plain sight, its depth its power, in feet) */
        L->tile[y][x] = T_PIT;
        t->power = 10 + dr (21);
    } else if (dr (10) < SP->t_hidden) {
        t->state = TS_HIDDEN | (dr (10) < SP->t_magic ? TS_MAGIC : 0);
        t->hide = 2 + dr (7);
    }
    if (kind == TK_COVERED || kind == TK_SPIKED) {
        t->state |= TS_HIDDEN;
        t->hide = t->hide ? t->hide : 1 + dr (5);
        t->power = 10 + dr (21);
    }
}

/* A square for a pit or a plate in room r: floor, the eight round it floor too, within a foot of it (a walk round it
** wherever it's crossed); 0xFF: none found */
static unsigned char tx, ty;
static unsigned char __fastcall__ trap_square (unsigned char r)
{
    struct room* q = &L->room[r];
    unsigned char n, i, j;
    signed char dz;

    if (q->w < 3 || q->h < 3) {
        return 0xFF;
    }
    for (n = 0; n < 6; ++n) {
        tx = q->x + 1 + dr (q->w - 2);
        ty = q->y + 1 + dr (q->h - 2);
        for (j = ty - 1; j <= ty + 1; ++j) {
            for (i = tx - 1; i <= tx + 1; ++i) {
                dz = L->elev[j][i] - L->elev[ty][tx];
                if (L->tile[j][i] != T_FLOOR || dz > 1 || dz < -1) {
                    goto next;
                }
            }
        }
        return 0;
next:   ;
    }
    return 0xFF;
}

static void traps (void)
{
    static const unsigned char kinds[8] = { TK_COVERED, TK_SPIKED, TK_DEADFALL, TK_BLADE, TK_DARTS, TK_GAS, TK_ALARM,
        TK_DARTS };
    unsigned char r, i, k;
    struct portal* p;

    for (r = 0; r < L->nrooms; ++r) {           /* (A room's: a pit, or one set off underfoot) */
        dseed (4, r);
        if (dr (20) >= SP->traps || trap_square (r)) {
            continue;
        }
        k = dr (4) < SP->t_pit ? (dr (2) ? TK_PIT : TK_COVERED) : kinds[dr (8)];
        trap (tx, ty, k, k == TK_PIT || k == TK_COVERED || k == TK_SPIKED || !dr (2) ? TT_PLATE : TT_WIRE);
    }
    for (i = 0; i < L->nportals; ++i) {         /* (A closed door's: set off as it's opened) */
        p = &L->portal[i];
        dseed (5, p->x << 5 | p->y);
        if (p->kind == PK_DOOR && !(p->state & PS_OPEN) && dr (20) < SP->d_trap) {
            trap (p->x, p->y, kinds[3 + dr (5)], TT_DOOR);
        }
    }
}

void __fastcall__ o_fixtures (struct chunk* c)
{
    unsigned char r;

    L = c;
    materials ();
    for (r = 0; r < L->nrooms; ++r) {
        windows (r);
    }
    portcullises ();
    traps ();
}
