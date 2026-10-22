/*
** stock.c - (dq0m) a chunk's ordinary things put down, once, as it's first made (DUNGEON.md: items): in its rooms
** (a cave's chambers), by its stratum's style's say (how often, how many concealed, and how well), what lies about
** by each room's former use (uses.c's: a kitchen's pots and bones; a chamber's, a cave's, the style's kinds), the
** stratum's age rotting what rots, and each use's mark (a kitchen's pot, a chapel's candle stubs, a well): a list
** of them (items.h's struct stock, in the main RAM: sk), which make.c puts in the world bank's stock beside the
** chunk's slot, for the world to put down (the item store is the world's alone: hub.h's who writes what).  A chunk
** stocked is marked among the change records (STOCKED, the world's as it puts its things down), so a chunk made
** again isn't stocked twice.
**
** (stock () is overlay 7's, the maker's, beside uses.c, whose leavings it asks for (overlays 3 and 5 the caves' and
** the fixtures' room), called through bank.s's stub; put_down the main code's.)
*/

#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

/* The chunk's things, listed (make.c's to put in the world bank: its first square sx0, sy0, in the frame) */
struct stock sk;
static unsigned sx0, sy0;

/* A thing put down on a square (concealed: hide, its difficulty in steps; else ST_SHOWN; or in the one before it,
** ST_IN, ST_LIT: a sconce's torch), listed: its entry, or 0 (the list full) */
struct sthing* __fastcall__ put_down (unsigned char kind, unsigned char sn, unsigned x, unsigned y,
    unsigned char qty, unsigned char hide)
{
    struct sthing* e;

    (void) sn;                                  /* (The chunk's) */
    if (sk.n >= NSTOCK) {
        return 0;
    }
    e = &sk.e[sk.n++];
    e->kind = kind;
    e->x = x - sx0;
    e->y = y - sy0;
    e->qty = qty;
    e->hide = hide;
    e->make = 0;
    e->size = 0;
    return e;
}

/* A portal's part put in it (where IW_PORTAL) or by it (IW_SQUARE: a bar set aside), its make the portal's: its
** material (a wooden door's bar wood, else iron; a pane glass; bars iron), its state, its size (a bar the door's width
** and a foot more; a pane, bars, the window's opening); its thickness the world's, by its kind and material (eighths
** of an inch) */
static void __fastcall__ part (unsigned char kind, unsigned char sn, unsigned x, unsigned y, const struct portal* q,
    unsigned char where)
{
    struct sthing* e = put_down (kind, sn, x, y, 1, where == IW_PORTAL ? ST_PORTAL : ST_SHOWN);

    if (e) {
        e->make = (kind == KI_PANE ? MT_GLASS : kind == KI_BAR && q->mat <= PM_IRONBOUND ? MT_WOOD : MT_IRON) << 4 |
            q->qual;
        e->size = kind == KI_BAR ? (q->size & 15) + 1 : q->size;
    }
}

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")

/* The theme's piece i put down in the chunk, on a square of a room's floor (else of its passages'), and its place
** given its square */
static struct tplace pl;
static void __fastcall__ piece_down (const struct chunk* c, unsigned char i, unsigned x0, unsigned y0)
{
    static struct tpiece tp;
    const struct room* r;
    unsigned char lx, ly, tries, t;

    hub_get (THEME_BANK, &THEME->piece[i], &tp, sizeof tp);
    for (tries = 0; tries < 80; ++tries) {
        if (tries < 40 && c->nrooms) {
            r = &c->room[random (c->nrooms)];
            lx = r->x + random (r->w);
            ly = r->y + random (r->h);
        } else {
            lx = random (CW);
            ly = random (CW);
        }
        t = TILE (c->tile[ly][lx]);
        if (t == T_FLOOR || (tries >= 40 && t == T_CORR)) {
            pl.x = x0 + lx;
            pl.y = y0 + ly;
            hub_put (THEME_BANK, &THEME_PLACE[i], &pl, sizeof pl);
            put_down (KI_THEME + tp.kind, c->sn, pl.x, pl.y, tp.qty, tp.hide);
            return;
        }
    }
}

/* The chunk (in the main RAM, the growers' L; its first square x0, y0 in the frame) stocked, listed in sk: two tries
** a room, each a thing at the style's odds, at a square of its floor, of a kind its use left; its use's mark, in its
** middle (and a kitchen's well, where there's water, by its west wall); the theme's pieces whose place it is; then
** its portals' parts (a barred door's bar in its brackets, some unbarred doors' set aside by them, one in four; a
** window's pane, its bars), with no dice: nothing else moves for them */
void __fastcall__ o_stock (const struct chunk* c, unsigned x0, unsigned y0)
{
    const struct room* r;
    const struct portal* p;
    unsigned char i, n, lx, ly, k, hidden, d, q, h;
    unsigned x, y;

    sk.n = 0;
    sx0 = x0;
    sy0 = y0;
    rseed (STRATUM (c->sn).seed ^ ((unsigned long) c->cx << 16 | c->cy) * 69069UL ^ 0x5EED);
    for (i = 0; i < c->nrooms; ++i) {
        r = &c->room[i];
        for (n = 0; n < 2; ++n) {
            if (random (20) >= SP->things) {
                continue;
            }
            lx = r->x + random (r->w);
            ly = r->y + random (r->h);
            if (lx >= CW || ly >= CW || TILE (c->tile[ly][lx]) != T_FLOOR) {
                continue;
            }
            hidden = random (20) < SP->hidden;
            d = random (hidden ? 4 : 8);
            k = hidden ? SP->hide_kinds[d] : uses (US_THING, i | d << 3);
            q = (hidden ? k : SP->kinds[d]) == KI_PENNIES ? 2 + random (20) : 3;    /* (The style's kind's draws:
                                                **   the stock's dice as they were before its uses) */
            h = hidden ? SP->h_lo + 2 * random (SP->h_span) : ST_SHOWN;
            if (k) {                            /* (Else rotted away) */
                put_down (k, c->sn, x0 + lx, y0 + ly, k == KI_PENNIES ? q : 1, h);
            }
        }
    }
    for (i = 0; i < c->nrooms; ++i) {           /* (Each room's use's mark, and a kitchen's well: no dice) */
        r = &c->room[i];
        for (n = 0; n < 2; ++n) {
            lx = n ? r->x : r->x + r->w / 2;
            ly = r->y + r->h / 2;
            if ((k = uses (US_SIGN, i | (n ? 0x80 : 0))) && TILE (c->tile[ly][lx]) == T_FLOOR) {
                put_down (k, c->sn, x0 + lx, y0 + ly, 1, ST_SHOWN);
            }
        }
    }
    for (i = 0; (ulayer & (SG_HASUNDER | SG_RUINED)) && i < c->nrooms; ++i) {   /* (Its history's: an older layer's
                                                **   remnants, its rooms' uses' marks; a ruin's debris, bones where it
                                                **   was taken or cursed, potsherds else) */
        r = &c->room[i];
        for (n = 0; n < 2; ++n) {
            lx = r->x + random (r->w);
            ly = r->y + random (r->h);
            if (!n) {
                k = ulayer & SG_HASUNDER ? uses (US_UNDER, i) : 0;
            } else if (!(ulayer & SG_RUINED) || random (2)) {
                k = 0;
            } else {
                k = uruin == EN_CONQUERED || uruin == EN_CURSED ? KI_BONES : KI_POTSHERDS;
            }
            if (k && lx < CW && ly < CW && TILE (c->tile[ly][lx]) == T_FLOOR) {
                put_down (k, c->sn, x0 + lx, y0 + ly, 1, ST_SHOWN);
            }
        }
    }
    for (i = 0; i < NPIECES; ++i) {             /* (The theme's pieces whose place is the chunk: on a room's floor) */
        hub_get (THEME_BANK, &THEME_PLACE[i], &pl, sizeof pl);
        if (pl.sn == c->sn && pl.cx == c->cx && pl.cy == c->cy) {
            piece_down (c, i, x0, y0);
        }
    }
    for (i = 0, p = c->portal; i < c->nportals; ++i, ++p) {
        x = x0 + p->x;
        y = y0 + p->y;
        if (p->kind == PK_DOOR && (p->feat & PF_BRACKETS) && ((p->state & PS_BARRED) || !((p->x ^ p->y) & 3))) {
            part (KI_BAR, c->sn, x + dir_x[p->bar], y + dir_y[p->bar], p, p->state & PS_BARRED ? IW_PORTAL : IW_SQUARE);
        } else if (p->kind == PK_WINDOW) {
            if (p->parts & PP_GLASS) {
                part (KI_PANE, c->sn, x, y, p, IW_PORTAL);
            }
            if (p->parts & PP_BARS) {
                part (KI_BARS, c->sn, x, y, p, IW_PORTAL);
            }
        }
    }
}
