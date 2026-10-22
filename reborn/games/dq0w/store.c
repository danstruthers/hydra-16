/*
** store.c - (dq0w) the change records written (world.h: the change records), the world's alone: kept by region in
** the pages of the hub's change banks (their table the world bank's cpage), a square's record of a kind replacing the
** one before it (its kind and floor; its portal's state, its harm; its trap's state), each made in its chunk in the
** cache too, as the maker lays them over a chunk it makes again (dq0m/changes.c).  With a save folder the pages are
** a cache: a region's records are in its file there (rgSXXYY.dat), read in as they're wanted, written back as the
** region's pages make way for another's (the one least lately wanted), and the save writes the rest; a region's file
** of another game's in the folder (its generation not this game's, the world bank's cgen) is never read.  Without a
** folder, a full cache keeps no more.  The chunks the maker stocked as it made them (their slots CS_FRESH) have their
** things put down (the maker's lists, the world bank's stock: things.c's) and are marked so here (STOCKED), so none is
** stocked twice.  All under the hub's change_lock (the maker reads them meanwhile), never held while the maker's asked
** for anything.
**
** The rarer work is swapped part 4's (../dq0/swap.inc, bank.s's stubs): the chunks stocked marked (their kept rooms'
** lights kept, wkept.c's), the UI's requests (the save, a portal looked at closely); the files' work part 18's
** (wfile.c: a region read in, a page made room for, a region written back, a version of its file each time; the
** save's; the game begun: its generation, its stamps); a square's record of a kind looked up, part 19's (wfile.c's
** store_get: a hole dug part way, DUG, as a dig begins).
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "trace.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)                      /* (Smaller: none of it's called again before it returns) */

void store_in (void);                           /* (Part 18's stubs: wfile.c) */
unsigned char store_page (void);
void store_save (void);
void store_commit (void);                       /* (Part 19's) */
void stocked (void);                            /* (Part 4's) */
void __fastcall__ keep (unsigned char back);    /* (The things': items.dat) */

unsigned char rsn, rrx, rry;                    /* (The region wanted) */
struct change rec;                              /* (The record written) */
struct change buf[8];                           /* (Records to or from a file, a directory's entry: through the main
                                                **   RAM; wfile.c's too) */
char path[56];

/* Page p's entry in the table (the world bank in the window) */
static struct cpage* __fastcall__ page (unsigned char p)
{
    hub_map (WORLD_BANK);
    return &WREC->cpage[p];
}

/* Its region the one wanted */
#define WANTED(q)       ((q)->used && (q)->sn == rsn && (q)->rx == rrx && (q)->ry == rry)

/* The record (rec) kept: in its region's pages (read in, if they're in its file), over the one of its square it
** replaces (its kind and floor, a portal's state, its harm, a trap's state), or after the rest (a page made room
** for) */
void store_put (void)
{
    struct cpage* q;
    struct change* c;
    unsigned char p, i, n, room, had, tries;

    rsn = rec.sn;
    rrx = (rec.x - STRATUM (rsn).x0) >> 8;        /* (RW squares a region) */
    rry = (rec.y - STRATUM (rsn).y0) >> 8;
    hub_map (0);
    hy_sem_acquire (HUB->change_lock);
    for (tries = 0; ; ++tries) {
        for (p = 0, room = 0xFF, had = 0; p < NCPAGES; ++p) {
            q = page (p);
            if (!WANTED (q)) {
                continue;
            }
            had = 1;
            q->stamp = WREC->cstamp;
            if ((n = q->n) < CH_PER_PAGE) {
                room = p;
            }
            hub_map (CHANGE_BANK (p));
            for (c = CHANGE_AT (p); n; --n, ++c) {
                if (!memcmp (c, &rec, 5) && (c->link == rec.link || (CH_TILE (c->link) && CH_TILE (rec.link)))) {
                    memcpy (c, &rec, sizeof rec);   /* (Its stratum and square the same: its kind too) */
                    goto put;
                }
            }
        }
        if (had || tries) {
            break;
        }
        store_in ();                            /* (Its file read in, if it has one) */
    }
    if (room == 0xFF && (room = store_page ()) == 0xFF) {
        goto done;                              /* (Full, and no folder: kept no more) */
    }
    q = page (room);
    if (!q->used) {
        q->used = 1;
        q->sn = rsn;
        q->rx = rrx;
        q->ry = rry;
        q->n = 0;
        q->stamp = WREC->cstamp;
    }
    i = q->n++;
    hub_map (CHANGE_BANK (room));
    memcpy (CHANGE_AT (room) + i, &rec, sizeof rec);
    p = room;
put:
    q = page (p);
    q->dirty = 1;
    ++WREC->cstamp;
done:
    hub_map (0);
    hy_sem_release (HUB->change_lock);
}

/* A change kept, and made in its chunk (the cache's: its chunk there first, so one made now, laid over with what's
** kept, hasn't this one yet), and told (E_LIGHT: a square's, or a portal's state, may change the light about it,
** the senses'); the hub's header in the window after */
static struct msg lm;
void __fastcall__ change (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link)
{
    unsigned char* p;
    struct portal* q;
    struct trap* t;
    struct opening* o;
    unsigned char i;

    at (sn, x, y);
    rec.sn = sn;
    rec.x = x;
    rec.y = y;
    rec.tile = tile;
    rec.elev = elev;
    rec.link = link;
    store_put ();
    if ((p = at (sn, x, y)) != 0) {
        if (link == PORTAL_STATE || link == PORTAL_HARM) {     /* (A portal's state: tile it, elev its parts; or the
                                                **   harm it's taken: tile it) */
            if ((q = portal_at (cx_, cy_)) != 0) {
                if (link == PORTAL_HARM) {
                    q->harm = tile;
                } else {
                    q->state = tile;
                    q->parts = elev;
                }
            }
        } else if (link == TRAP_STATE) {        /* (A trap's: tile it) */
            for (i = C->ntraps, t = C->trap; i; --i, ++t) {
                if (t->x == cx_ && t->y == cy_) {
                    t->state = tile;
                }
            }
        } else {                                /* (The square: its kind, its floor, and (a link) its opening) */
            *p = (*p & T_SEEN) | tile;
            C->elev[cy_][cx_] = elev;
            if (link != 0xFF && C->nopen < NOPEN) {
                o = &C->open[C->nopen++];
                o->kind = STRATUM (sn).link[link].kind;
                o->x = cx_;
                o->y = cy_;
                o->sill = 0;
                o->link = link;
            }
        }
    }
    hub_map (0);
    if (link != TRAP_STATE && link != PORTAL_HARM && link != STOCKED) {
        lm.kind = E_LIGHT;
        lm.a = sn;
        lm.w1 = x;
        lm.w2 = y;
        lm.t = HUB->now;
        hub_event (&lm);
    }
}

/* The chunks the maker stocked as it made them (their slots CS_FRESH), if there are any, their things put down and
** marked so (part 4's) */
void stocks (void)
{
    unsigned char k;
    struct cslot* c;

    hub_map (WORLD_BANK);
    for (k = NCSLOTS, c = WREC->cslot; k && c->used != CS_FRESH; --k, ++c) {
    }
    hub_map (0);
    if (k) {
        stocked ();
    }
}

#pragma code-name (push, "SW4CODE")
#pragma rodata-name (push, "SW4RODATA")

/* The chunks the maker stocked (their slots CS_FRESH), in the order it stocked them (each one's list's seq): their
** things put down (things.c's), and marked so, a record each (STOCKED, its first square), their slots as any other's;
** their kept rooms' lights kept (their torches lit, the maker's R_LIT: a record of it) (the hub's header in the window
** after) */
void s_stocked (void)
{
    struct cslot* c;
    unsigned char k, i;
    unsigned s;

    for (;;) {
        hub_map (WORLD_BANK);
        for (i = 0, k = 0xFF; i < NCSLOTS; ++i) {
            if (WREC->cslot[i].used == CS_FRESH && (k == 0xFF || (int) (WREC->stock[i].seq - s) < 0)) {
                k = i;
                s = WREC->stock[i].seq;
            }
        }
        if (k == 0xFF) {
            break;
        }
        stock_down (k);
        hub_map (WORLD_BANK);
        c = &WREC->cslot[k];
        {
            rec.sn = c->sn;
            rec.x = WREC->strata[SLOT (c->sn)].x0 + (c->cx << CW_BITS);
            rec.y = WREC->strata[SLOT (c->sn)].y0 + (c->cy << CW_BITS);
            rec.tile = 0;
            rec.elev = 0;
            rec.link = STOCKED;
            store_put ();
            sconce_lights (SL_STOCKED);             /* (Its kept rooms' lights: a record each, wkept.c's; rec's square
                                                **   the chunk's first) */
            hub_map (WORLD_BANK);
            WREC->cslot[k].used = 1;
        }
    }
    hub_map (0);
}

/* The portal on the hub's wreq's square looked at closely (the UI's request, PA_EXAMINE: the square one it sees, on
** the character's stratum): its record copied into the hub's pview, its bar 1 if the character's on its bar's side;
** the wreq's res 1 if there's none */
static void look_at (void)
{
    struct portal* p;
    unsigned x, y;
    unsigned char r = 1;

    hub_map (0);
    x = HUB->wreq.x;
    y = HUB->wreq.y;
    if (at (HUB->pc_stratum, x, y) && (p = portal_at (cx_, cy_)) != 0) {
        wk.pq = *p;
        r = 0;
    }
    hub_map (0);
    if (!r) {
        wk.pq.bar = dir_x[wk.pq.bar] * (int) (HUB->pc_x - x) + dir_y[wk.pq.bar] * (int) (HUB->pc_y - y) > 0;
        memcpy (&HUB->pview, &wk.pq, sizeof wk.pq);
    }
    HUB->wreq.res = r;
}

/* The hub's wreq answered (the UI's: the save, a close look; the game begun, the character's state set: part 16's) */
void s_answer (void)
{
    hub_map (0);
    if (HUB->wreq.kind == WQ_SAVE) {
        store_save ();                          /* (The change records, */
        keep (0);                               /*   the things, items.dat, */
        world_keep ();                          /*   the world's own, world.dat, */
        tr_flush (1);                           /*   the traces' chunks changed, trSXXYY.dat, */
        store_commit ();                        /*   and the versions written the save's: the last step) */
    } else if (HUB->wreq.kind == WQ_EXAMINE) {
        look_at ();
    } else {
        pc_asked ();
    }
    hub_reply (&HUB->wreq);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
