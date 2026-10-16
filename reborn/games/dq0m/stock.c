/*
** stock.c - (dq0m) a chunk's ordinary things put down, once, as it's first made (DUNGEON.md: items): in its rooms
** (a cave's chambers), by its stratum's style's say (how often, which kinds, how many concealed, and how well); the
** things' records taken from the hub's item store under its lock (the things task, dq0p, keeps them after).  A
** chunk stocked is marked among the change records (STOCKED), so a chunk made again isn't stocked twice.
**
** (stock () is overlay 3's, the maker's, called through bank.s's stub; put_down the main code's, that maps the
** hub's banks, the window put back as it was after.)
*/

#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

/* A thing put down on a square (concealed: hide, its difficulty 0-9; else 0xFF): its record's number, or 0 (the
** window as it was) */
static unsigned __fastcall__ put_down (unsigned char kind, unsigned char sn, unsigned x, unsigned y, unsigned char qty,
    unsigned char hide)
{
    struct item* p;
    unsigned i;
    unsigned char b = getbank ();

    hub_map (0);
    hy_sem_acquire (HUB->item_lock);
    for (i = 1; i < NITEMS; ++i) {
        hub_map (ITEM_BANK (i));
        p = ITEM_AT (i);
        if (!p->kind) {
            memset (p, 0, sizeof *p);
            p->kind = kind;
            p->where = IW_SQUARE;
            p->sn = sn;
            p->x = x;
            p->y = y;
            p->qty = qty;
            if (hide != 0xFF) {
                p->flags = I_HIDDEN;
                p->hide = hide;
            }
            break;
        }
    }
    hub_map (0);
    hy_sem_release (HUB->item_lock);
    setbank (b);
    return i < NITEMS ? i : 0;
}

/* A portal's part put in it (where IW_PORTAL) or by it (IW_SQUARE: a bar set aside), its make the portal's: its
** material (a wooden door's bar wood, else iron; a pane glass; bars iron), its state, its size (a bar the door's width
** and a foot more; a pane, bars, the window's opening), its thickness (eighths of an inch) */
static void __fastcall__ part (unsigned char kind, unsigned char sn, unsigned x, unsigned y, const struct portal* q,
    unsigned char where)
{
    struct item* p;
    unsigned i = put_down (kind, sn, x, y, 1, 0xFF);
    unsigned char b = getbank ();

    if (i) {
        hub_map (ITEM_BANK (i));
        p = ITEM_AT (i);
        p->where = where;
        p->mat = kind == KI_PANE ? MT_GLASS : kind == KI_BAR && q->mat <= PM_IRONBOUND ? MT_WOOD : MT_IRON;
        p->quality = q->qual;
        p->size = kind == KI_BAR ? (q->size & 15) + 1 : q->size;
        p->thick = kind == KI_PANE ? 2 : kind == KI_BARS ? 8 : p->mat == MT_WOOD ? 24 : 12;
        setbank (b);
    }
}

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

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

/* The chunk (in the main RAM; its first square x0, y0 in the frame) stocked: two tries a room, each a thing at the
** style's odds, at a square of its floor; the theme's pieces whose place it is; then its portals' parts (a barred
** door's bar in its brackets, some unbarred doors' set aside by them, one in four; a window's pane, its bars), with
** no dice: nothing else moves for them */
void __fastcall__ o_stock (const struct chunk* c, unsigned x0, unsigned y0)
{
    const struct room* r;
    const struct portal* p;
    unsigned char i, n, lx, ly, k, hidden;
    unsigned x, y;

    rseed (strata[c->sn].seed ^ ((unsigned long) c->cx << 16 | c->cy) * 69069UL ^ 0x5EED);
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
            k = hidden ? SP->hide_kinds[random (4)] : SP->kinds[random (8)];
            put_down (k, c->sn, x0 + lx, y0 + ly, k == KI_PENNIES ? 2 + random (20) : 1,
                hidden ? SP->h_lo + random (SP->h_span) : 0xFF);
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
