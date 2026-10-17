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
** The rarer work is swapped part 4's (../dq0/swap.inc, bank.s's stubs): a region read in, a page made room for (a
** region written back), the save (the UI's request: the hub's wreq), the game begun (its generation; a version 6
** save's records read in), a portal looked at closely (the UI's request).
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)                      /* (Smaller: none of it's called again before it returns) */

void store_in (void);                           /* (Part 4's stubs) */
unsigned char store_page (void);
void store_save (void);
void stocked (void);

static unsigned char rsn, rrx, rry;             /* (The region wanted) */
static struct change rec;                       /* (The record written) */
static struct chead hd;
static struct change buf[8];                    /* (Records to or from a file: through the main RAM) */
static char path[56];

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
static void store_put (void)
{
    struct cpage* q;
    struct change* c;
    unsigned char p, i, n, room, had, tries;

    rsn = rec.sn;
    rrx = (rec.x - strata[rsn].x0) >> 8;        /* (RW squares a region) */
    rry = (rec.y - strata[rsn].y0) >> 8;
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
                o->kind = strata[sn].link[link].kind;
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
** things put down (things.c's), and marked so, a record each (STOCKED, its first square), their slots as any other's
** (the hub's header in the window after) */
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
            rec.x = WREC->strata[c->sn].x0 + (c->cx << CW_BITS);
            rec.y = WREC->strata[c->sn].y0 + (c->cy << CW_BITS);
            rec.tile = 0;
            rec.elev = 0;
            rec.link = STOCKED;
            store_put ();
            hub_map (WORLD_BANK);
            WREC->cslot[k].used = 1;
        }
    }
    hub_map (0);
}

/* The save folder's file name (the hub's savedir: "" none): 0, or its path in path */
static unsigned char __fastcall__ in_folder (const char* name)
{
    hub_map (0);
    if (!HUB->savedir[0]) {
        return 0;
    }
    strcpy (path, HUB->savedir);
    strcat (path, "/");
    strcat (path, name);
    return 1;
}

/* Region sn, rx, ry's file's name (rgSXXYY.dat) in path: 0, none (no folder) */
static unsigned char __fastcall__ region_file (unsigned char sn, unsigned char rx, unsigned char ry)
{
    static char n[12];
    static const char hex[] = "0123456789abcdef";

    strcpy (n, "rg0xxyy.dat");
    n[2] = hex[sn & 15];
    n[3] = hex[rx >> 4];
    n[4] = hex[rx & 15];
    n[5] = hex[ry >> 4];
    n[6] = hex[ry & 15];
    return in_folder (n);
}

/* A region's pages written to its file (sn, rx, ry: its records, as many as its pages hold), its pages not dirty
** after */
static void __fastcall__ write_region (unsigned char sn, unsigned char rx, unsigned char ry)
{
    struct cpage* q;
    unsigned char p, i, k, n;
    int fd;

    if (!region_file (sn, rx, ry) || (fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    hub_map (WORLD_BANK);
    hd.magic = CH_MAGIC;
    hd.gen = WREC->cgen;
    for (p = 0, hd.n = 0, q = WREC->cpage; p < NCPAGES; ++p, ++q) {
        if (q->used && q->sn == sn && q->rx == rx && q->ry == ry) {
            hd.n += q->n;
        }
    }
    write (fd, &hd, sizeof hd);
    for (p = 0; p < NCPAGES; ++p) {
        hub_map (WORLD_BANK);
        q = &WREC->cpage[p];
        if (!q->used || q->sn != sn || q->rx != rx || q->ry != ry) {
            continue;
        }
        q->dirty = 0;
        for (i = 0, n = q->n; i < n; i += k) {  /* (Through the main RAM: a bank isn't seen while write works) */
            k = n - i > 8 ? 8 : n - i;
            hub_map (CHANGE_BANK (p));
            memcpy (buf, CHANGE_AT (p) + i, k * sizeof (struct change));
            write (fd, buf, k * sizeof (struct change));
        }
    }
    close (fd);
}

/* A page for the region wanted: a free one, or (with a folder) the pages of the region least lately wanted written
** to its file and freed: one of them; 0xFF, none (the lock held) */
unsigned char s_store_page (void)
{
    struct cpage* q;
    unsigned char p, best = 0xFF, sn, rx, ry;

    hub_map (WORLD_BANK);
    for (p = 0, q = WREC->cpage; p < NCPAGES; ++p, ++q) {
        if (!q->used) {
            return p;
        }
        if (!WANTED (q) && (best == 0xFF || q->stamp - WREC->cpage[best].stamp > 0x8000U)) {
            best = p;                           /* (The least lately wanted: its stamp the oldest, round) */
        }
    }
    if (best == 0xFF || !in_folder ("")) {
        return 0xFF;
    }
    hub_map (WORLD_BANK);
    q = &WREC->cpage[best];
    sn = q->sn;
    rx = q->rx;
    ry = q->ry;
    write_region (sn, rx, ry);                  /* (Clean or not: one file, all of its records) */
    hub_map (WORLD_BANK);
    for (p = 0, q = WREC->cpage; p < NCPAGES; ++p, ++q) {
        if (q->used && q->sn == sn && q->rx == rx && q->ry == ry) {
            q->used = 0;
        }
    }
    return best;
}

/* The region wanted read in from its file, if it has one of this game's: its records into pages of their own, not
** dirty (the lock held) */
void s_store_in (void)
{
    struct cpage* q;
    unsigned left;
    unsigned char p, k, i;
    int fd;

    if (!region_file (rsn, rrx, rry) || (fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    hub_map (WORLD_BANK);
    if (read (fd, &hd, sizeof hd) != sizeof hd || hd.magic != CH_MAGIC || hd.gen != WREC->cgen) {
        close (fd);
        return;
    }
    for (left = hd.n; left && (p = store_page ()) != 0xFF; ) {
        hub_map (WORLD_BANK);
        q = &WREC->cpage[p];
        q->used = 1;
        q->sn = rsn;
        q->rx = rrx;
        q->ry = rry;
        q->n = 0;
        q->dirty = 0;
        q->stamp = WREC->cstamp;
        for (i = 0; i < CH_PER_PAGE && left; i += k, left -= k) {
            k = left > 8 ? 8 : left;
            if (read (fd, buf, k * sizeof (struct change)) != k * sizeof (struct change)) {
                left = 0;                       /* (Cut short: no more) */
                break;
            }
            hub_map (CHANGE_BANK (p));
            memcpy (CHANGE_AT (p) + i, buf, k * sizeof (struct change));
            hub_map (WORLD_BANK);
            WREC->cpage[p].n += k;
        }
    }
    close (fd);
}

/* The save (the UI's request: the hub's savedir): the chunks stocked marked, each region's pages that have changed
** written to its file, and changes.dat (CH_MAGIC, the generation, CH_VER) */
void s_store_save (void)
{
    struct cpage* q;
    unsigned char p;
    int fd;

    stocks ();
    if (!in_folder ("changes.dat")) {
        return;
    }
    hub_map (0);
    hy_sem_acquire (HUB->change_lock);
    for (p = 0; p < NCPAGES; ++p) {
        hub_map (WORLD_BANK);
        q = &WREC->cpage[p];
        if (q->used && q->dirty) {
            write_region (q->sn, q->rx, q->ry);
        }
    }
    hub_map (0);
    hy_sem_release (HUB->change_lock);
    if (in_folder ("changes.dat") && (fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        hub_map (WORLD_BANK);
        hd.magic = CH_MAGIC;
        hd.gen = WREC->cgen;
        hd.n = CH_VER;
        write (fd, &hd, sizeof hd);
        close (fd);
    }
    hub_map (0);
}

/* The game begun (resume: carried on from its save): its generation, the last game's in the folder and one more (a
** new game's), or its own; a version 6 save's records (its changes.dat: their count, then them) read in */
void __fastcall__ s_store_begin (unsigned char resume)
{
    unsigned gen = 0, n = 0;
    int fd;

    if (in_folder ("changes.dat") && (fd = open (path, O_RDONLY)) >= 0) {
        if (read (fd, &hd, sizeof hd) == sizeof hd && hd.magic == CH_MAGIC) {
            gen = hd.gen;
        } else if (resume) {                    /* (A version 6 save's: the count first, then the records) */
            close (fd);
            fd = open (path, O_RDONLY);
            read (fd, &n, sizeof n);
        }
        hub_map (WORLD_BANK);
        WREC->cgen = resume && gen ? gen : gen + 1;
        for (; n && read (fd, &rec, sizeof rec) == sizeof rec; --n) {
            store_put ();
        }
        close (fd);
    } else {
        hub_map (WORLD_BANK);
        WREC->cgen = 1;
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

/* The hub's wreq answered (the UI's: the save, a close look) */
void s_answer (void)
{
    hub_map (0);
    if (HUB->wreq.kind == WQ_SAVE) {
        store_save ();
    } else {
        look_at ();
    }
    hub_reply (&HUB->wreq);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
