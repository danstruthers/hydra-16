/*
** fixtures.c - (dq0m) a chunk's materials and its fixtures (DUNGEON.md), once its grower's grown it, in the main RAM:
** each room's floor, walls and ceiling, and its passages' (the rock they're cut in), by its stratum's style and age
** (a built room's walls masonry, its floor flagged or bare earth, more earth the older; a cave's earth or stone, its
** walls the rock; a mine's all stone); windows (glass, shutters, bars: any of them) and arrow slits in a room's
** walls where a passage runs beside them; portcullises for some doors; traps (pits, open or hidden; deadfalls,
** blades, darts, gas, alarms, a trodden plate's or a wire's; a door's, set off as it's opened).  Every proportion the
** style's (no windows, portcullises or set traps in a natural cave: only its pits).  A fortified stratum's (DUNGEON.md:
** built to a plan; uses.c's ufort): its gatehouse's doors portcullises (raised, their winches on its side), murder
** holes in its vault, arrow slits in its walls and its guardrooms' where a passage runs beside them, covering the
** way in; its doors stouter (more of them iron-bound, an inch thicker) and fewer of them open.
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
    unsigned char d, k, len, x, y, t, fort = ufort && (q->kind == U_GATEHOUSE || q->kind == U_GUARDROOM);

    for (d = 0; d < 4; ++d) {
        len = d == DIR_N || d == DIR_S ? q->w : q->h;
        dseed (2, r << 2 | d);
        if (dr (20) >= SP->windows && !fort) {  /* (A fortified gate's: slits wherever they cover a way) */
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
            if (!(p = portal (x, y, fort || dr (20) < SP->slits ? PK_SLIT : PK_WINDOW))) {
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
                p->mat = (p->parts & (PP_BARS | PP_SHUTTERS)) == PP_BARS ? PM_IRON : PM_WOOD;   /* (Its shutters',
                                                **   else its bars') */
            }
            break;
        }
    }
}

/* Some doors portcullises instead (iron bars: raised, or down), each with its winch on its room's side (its bar's
** way) */
static void portcullises (void)
{
    struct portal* p;
    unsigned char i, d;

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
        p->parts = PP_BARS;
        for (d = 0; d < 4; ++d) {
            if (p->x + dir_x[d] < CW && p->y + dir_y[d] < CW && L->tile[p->y + dir_y[d]][p->x + dir_x[d]] == T_FLOOR) {
                p->bar = d;
            }
        }
    }
}

/* A fortified stratum's doors (uses.c's ufort): its gatehouse's portcullises, raised (their winches on its side),
** murder holes in its vault; the rest stouter (more iron-bound) and fewer open */
static void fortify (void)
{
    struct portal* p;
    struct room* q;
    unsigned char i, r, d, x, y;

    if (!ufort) {
        return;
    }
    for (i = 0, p = L->portal; i < L->nportals; ++i, ++p) {
        if (p->kind != PK_DOOR || (p->state & PS_HIDDEN)) {
            continue;
        }
        dseed (7, p->x << 5 | p->y);
        for (r = 0; r < L->nrooms; ++r) {
            q = &L->room[r];
            for (d = 0; d < 4 && q->kind == U_GATEHOUSE; ++d) {
                x = p->x + dir_x[d];
                y = p->y + dir_y[d];
                if (x >= q->x && x < q->x + q->w && y >= q->y && y < q->y + q->h) {
                    q->flags |= R_MURDER;
                    p->kind = PK_PORTCULLIS;
                    p->state = PS_OPEN;
                    p->lock = 0;
                    p->mat = PM_IRON;
                    p->parts = PP_BARS;
                    p->bar = d;
                }
            }
        }
        if (p->kind == PK_DOOR) {
            if (p->mat == PM_WOOD && dr (2)) {
                p->mat = PM_IRONBOUND;
            }
            if ((p->state & PS_OPEN) && dr (2)) {
                p->state &= ~PS_OPEN;
            }
        }
    }
}

/* ---- The portals' make (each with its own dice, the sixth decision's: its square's): its size (a built place's
** doors much alike, the stratum's own, a few odd; a cave's, a mixed place's, each its own, added later), its
** thickness, its state (as made, by the style; then worn by the stratum's age, the faster the softer its material:
** wood rots, iron rusts, stone hardly wears), a door's features (a grille, a spy-hole, a bar's brackets, studs, which
** side its hinges are), and so its body (work.c's part_body) */

static void make (void)
{
    static const unsigned char rot[4] = { 2, 2, 1, 0 };    /* (PM_*: tenths of a step of wear a century) */
    struct stratum* s = &strata[L->sn];
    struct portal* p;
    unsigned char i, w, h, t, q, f, alike = s->style == ST_BUILT || s->style == ST_MINE;
    unsigned char sd = (unsigned char) (s->seed >> 5);

    for (i = 0, p = L->portal; i < L->nportals; ++i, ++p) {
        dseed (6, p->x << 5 | p->y);
        if (p->kind == PK_WINDOW || p->kind == PK_SLIT) {
            w = p->kind == PK_SLIT ? 1 : 2 + dr (2);
            h = 2 + dr (3);
            t = 1 + dr (2);
        } else if (alike && dr (8)) {           /* (The stratum's door) */
            w = 3 + (sd & 1);
            h = 6 + (sd >> 1 & 1) + (sd >> 2 & 1);
            t = 2 + (sd >> 3 & 1);
        } else {
            w = 2 + dr (4);
            h = 4 + dr (5);
            t = 1 + dr (3);
        }
        if (p->kind == PK_PORTCULLIS) {
            t = 2;                              /* (Its bars) */
        } else if (p->mat == PM_IRON) {
            t = 1;                              /* (A plate) */
        } else if (ufort && p->kind == PK_DOOR) {
            ++t;                                /* (A fortified place's: an inch thicker) */
        }
        q = alike ? 4 + dr (4) : 2 + dr (6);    /* (As made) */
        h = h > 15 ? 15 : h;
        f = s->age * rot[p->mat & 3] / 10 + dr (2);   /* (The years' wear) */
        q = q > f ? q - f : 0;
        f = 0;
        if (p->kind == PK_DOOR && !(p->state & PS_HIDDEN)) {    /* (A secret door's the wall it looks like) */
            f = !dr (6) ? PF_GRILLE : !dr (5) ? PF_PEEP : 0;
            f |= (p->state & PS_BARRED) || !dr (4) ? PF_BRACKETS : 0;
            f |= p->mat == PM_WOOD && !dr (4) ? PF_STUDS : 0;
            f |= dr (2) ? PF_HINGES : 0;
            f |= w >= 5 ? PF_DOUBLE : 0;
        }
        p->size = w | h << 4;
        p->thick = t;
        p->qual = q;
        p->feat = f;
        p->harm = 0;
        p->tough = part_body (p->mat, p->size, t, q) >> (p->kind == PK_PORTCULLIS);   /* (Bars half a plate) */
        if (p->kind == PK_WINDOW && p->tough > 15) {
            p->tough = 15;                      /* (A window's parts' harm a nibble each: work.c's) */
        }
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
    fortify ();
    make ();
    traps ();
}
