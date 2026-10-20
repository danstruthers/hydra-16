/*
** changes.c - (dq0m) what's changed in the dungeon laid over a chunk each time it's made again (DUNGEON.md: changing
** the dungeon): the change records (world.h's), the world's to write (dq0w/store.c), read here only: its region's in
** the hub's change pages, or, if none of them are there, in its file in the save folder (rgSXXYY.dat, its newest
** valid version: ../dq0/vfile.inc's), under the hub's change_lock.  And the squares the character has seen, each
** chunk's as a map of bits, kept as the cache makes way for the chunk and laid over it when it's made again, in banks
** of the maker's own, a cache of them by region (move 2.8: no fixed count of them; the plan's region, 8 by 8 chunks,
** all its maps in the banks or none, those least lately wanted written to the save folder, smSXXYY.dat, a version
** each time, and read back as they're wanted: without a folder, the oldest map made way for), and in the save
** (seen.dat: the maps in the banks and their regions' table: the UI asks, M_SAVE; back as the game's carried on,
** M_RESTORE).  And the plans too big for their half of a plan bank (PLAN_MAX regions: plan.c's), paged to the save
** folder (plSS0000.dat, their regions in order) through a bank of the maker's own.
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

#define VF_STATIC       static
#include "../dq0/vfile.inc"

#define SEEN_BANKS      2
#define SEEN_PER_BANK   (0x2000 / sizeof (struct seen))

struct seen {
    unsigned char   used, sn;
    unsigned        cx, cy;
    unsigned char   rooms;                      /* Its rooms visited (bits) */
    unsigned char   bits[CW * CW / 8];
};

#define NSEEN           (SEEN_BANKS * SEEN_PER_BANK)
#define NSREG           16
#define SM_MAGIC        0x4D53                  /* "SM": seen.dat's, its regions' table first */
static unsigned char seb;                       /* The first bank */
unsigned char detail_bank;                      /* (The regions' details' cache: plan.c's, emptied here) */
static unsigned char plb;                       /* (The big plans' pages' bank) */
static unsigned char nexts;                     /* The next seen map made way for (no folder) */
static char path[56];
static struct {
    unsigned char sn, rx, ry;
    unsigned        stamp;
}                   sreg[NSREG];                /* The regions whose maps are in the banks (sn its stratum plus 1: 0,
                                                **   the entry free) */
static unsigned     sstamp;

void changes_init (void)
{
    int b = hy_banks_alloc (SEEN_BANKS + 2);

    seb = b;
    detail_bank = seb + SEEN_BANKS;
    plb = seb + SEEN_BANKS + 1;
    for (nexts = 0; nexts <= SEEN_BANKS + 1; ++nexts) {
        setbank (seb + nexts);
        memset ((void*) 0x8000, 0, 0x2000);
    }
    nexts = 0;
}

static struct seen* __fastcall__ seen_at (unsigned char i)
{
    setbank (seb + i / SEEN_PER_BANK);
    return (struct seen*) 0x8000 + i % SEEN_PER_BANK;
}

/* The change on a chunk (in the main RAM, at x, y in the frame): its square, and (a link) its opening */
static void __fastcall__ change_put (struct chunk* k, unsigned char lx, unsigned char ly, unsigned char tile,
    signed char elev, unsigned char link)
{
    unsigned char i;

    if (link == STOCKED) {                      /* (Its things put down: the record only a mark) */
        return;
    }
    if (link == PORTAL_STATE || link == PORTAL_HARM) {     /* (A portal's state: tile it, elev its parts; or the
                                                **   harm it's taken: tile it) */
        for (i = 0; i < k->nportals; ++i) {
            if (k->portal[i].x == lx && k->portal[i].y == ly) {
                if (link == PORTAL_HARM) {
                    k->portal[i].harm = tile;
                } else {
                    k->portal[i].state = tile;
                    k->portal[i].parts = elev;
                }
            }
        }
        return;
    }
    if (link == TRAP_STATE) {                   /* (A trap's: tile it) */
        for (i = 0; i < k->ntraps; ++i) {
            if (k->trap[i].x == lx && k->trap[i].y == ly) {
                k->trap[i].state = tile;
            }
        }
        return;
    }
    k->tile[ly][lx] = (k->tile[ly][lx] & T_SEEN) | tile;
    k->elev[ly][lx] = elev;
    if (link != 0xFF && k->nopen < NOPEN) {
        k->open[k->nopen].kind = STRATUM (k->sn).link[link].kind;
        k->open[k->nopen].x = lx;
        k->open[k->nopen].y = ly;
        k->open[k->nopen].sill = 0;
        k->open[k->nopen++].link = link;
    }
}

/* The records read (n of them, in the main RAM) laid over chunk k (its first square x, y in the frame): 1 if one
** says it's been stocked */
static struct change rb[8];
static unsigned char __fastcall__ over (struct chunk* k, unsigned x, unsigned y, unsigned char n)
{
    struct change* c;
    unsigned char stocked = 0;

    for (c = rb; n; --n, ++c) {
        if (c->sn == k->sn && c->x - x < CW && c->y - y < CW) {
            change_put (k, c->x - x, c->y - y, c->tile, c->elev, c->link);
            stocked |= c->link == STOCKED;
        }
    }
    return stocked;
}

/* The change records laid over a chunk just made (in the main RAM; its first square x, y in the frame): its region's,
** from the hub's change pages, else from its file (the window as it was after): 1 if it's been stocked */
unsigned char __fastcall__ changes_over (struct chunk* k, unsigned x, unsigned y)
{
    static struct vfh hd;
    struct cpage* q;
    unsigned w = hub_here ();
    unsigned char stocked = 0, p, i, m, had = 0, sn = k->sn, rx, ry;
    unsigned left;
    int fd;

    rx = (x - STRATUM (sn).x0) >> 8;              /* (Its region: RW squares each way) */
    ry = (y - STRATUM (sn).y0) >> 8;
    hub_map (0);
    hy_sem_acquire (HUB->change_lock);
    for (p = 0; p < NCPAGES; ++p) {
        hub_map (WORLD_BANK);
        q = &WREC->cpage[p];
        if (!q->used || q->sn != sn || q->rx != rx || q->ry != ry) {
            continue;
        }
        had = 1;
        for (i = 0, left = q->n; left; i += m, left -= m) {
            m = left > 8 ? 8 : left;
            hub_map (CHANGE_BANK (p));
            memcpy (rb, CHANGE_AT (p) + i, m * sizeof (struct change));
            stocked |= over (k, x, y, m);
            hub_map (WORLD_BANK);
        }
    }
    hub_map (0);
    if (!had && vf_name (path, "rg", sn, rx, ry) && (fd = vf_open (path, 2, &hd)) >= 0) {
        for (left = hd.n; left; left -= m) {    /* (None in the pages: its file's newest version, if this game has
                                                **   one) */
            m = left > 8 ? 8 : left;
            if (read (fd, rb, m * sizeof (struct change)) != m * sizeof (struct change)) {
                break;
            }
            stocked |= over (k, x, y, m);
        }
        close (fd);
    }
    hub_map (0);
    hy_sem_release (HUB->change_lock);
    hub_back (w);
    return stocked;
}

/* ---- The squares seen: a chunk's kept as the cache makes way for it (slot s), laid over it when it's made again.
** With a folder, by region: a region's maps wanted (in the banks, read back from its file if it has one, the region
** least lately wanted written to its file to make room, if it must: a map's place, or the table's), then its own
** map found there, or a place made; without, the next place round (the oldest map made way for).  dq0 -C: two maps'
** places, one region's */

static struct seen r;                           /* (A map, made or read) */
static struct seen cur;                         /* (seen_keep's: the map it's keeping) */
static unsigned char rsn, rrx, rry;             /* (Its region) */
static unsigned char sfree;                     /* (A free entry of the table: sreg_find's) */

/* The places for maps, and the table's entries, used (dq0 -C's fewer) */
static unsigned char __fastcall__ lim (unsigned char regions)
{
    hub_map (0);
    return HUB->small ? (regions ? 1 : 2) : regions ? NSREG : NSEEN;
}

/* Region rsn ...'s entry in the table, or 0xFF; a free one in sfree (0xFF none) */
static unsigned char sreg_find (void)
{
    unsigned char k, f = 0xFF, n = lim (1);

    for (k = 0, sfree = 0xFF; k < NSREG; ++k) {
        if (!sreg[k].sn) {
            if (sfree == 0xFF && k < n) {
                sfree = k;
            }
        } else if (sreg[k].sn == rsn + 1 && sreg[k].rx == rrx && sreg[k].ry == rry) {
            f = k;
        }
    }
    return f;
}

/* Map i's region the one at hand (rsn ...) */
static unsigned char __fastcall__ of_hand (unsigned char i)
{
    struct seen* q = seen_at (i);

    return q->used && q->sn == rsn && q->cx >> 3 == rrx && q->cy >> 3 == rry;
}

/* The table's entry least lately wanted, but the region at hand, written to its file (smSXXYY.dat, a new version: its
** maps), its maps' places freed, the entry too: 1, or 0 (none, or no folder) */
static unsigned char sroom (void)
{
    unsigned char k, b = 0xFF, i, sn = rsn, rx = rrx, ry = rry;
    unsigned t, n = 0;
    int fd;

    for (k = 0; k < NSREG; ++k) {
        if (sreg[k].sn && (sreg[k].sn != sn + 1 || sreg[k].rx != rx || sreg[k].ry != ry) &&
            (b == 0xFF || sreg[k].stamp - t > 0x8000U)) {
            b = k;
            t = sreg[k].stamp;
        }
    }
    if (b == 0xFF || !vf_name (path, "sm", sreg[b].sn - 1, sreg[b].rx, sreg[b].ry) || (fd = vf_make (path, 2)) < 0) {
        return 0;
    }
    rsn = sreg[b].sn - 1;
    rrx = sreg[b].rx;
    rry = sreg[b].ry;
    for (i = 0; i < NSEEN; ++i) {
        if (of_hand (i)) {
            memcpy (&r, seen_at (i), sizeof r);
            write (fd, &r, sizeof r);
            seen_at (i)->used = 0;
            ++n;
        }
    }
    vf_done (fd, n);
    sreg[b].sn = 0;
    rsn = sn;
    rrx = rx;
    rry = ry;
    return 1;
}

/* A map's place free (up to lim's), made if it must (sroom): its place, or 0xFF */
static unsigned char spot (void)
{
    unsigned char i, n = lim (0);

    do {
        for (i = 0; i < n; ++i) {
            if (!seen_at (i)->used) {
                return i;
            }
        }
    } while (sroom ());
    return 0xFF;
}

/* The region at hand (rsn ...) wanted: its maps in the banks (read back from its file, if they're not), its entry in
** the table the newest: 1, or 0 (no room) */
static unsigned char sreg_need (void)
{
    unsigned char k, i;
    int fd;
    static struct vfh h;
    static struct seen m;

    if ((k = sreg_find ()) != 0xFF) {
        sreg[k].stamp = ++sstamp;
        return 1;
    }
    if (sfree == 0xFF && (!sroom () || (sreg_find (), sfree == 0xFF))) {
        return 0;
    }
    k = sfree;
    sreg[k].sn = rsn + 1;
    sreg[k].rx = rrx;
    sreg[k].ry = rry;
    sreg[k].stamp = ++sstamp;
    if (vf_name (path, "sm", rsn, rrx, rry) && (fd = vf_open (path, 2, &h)) >= 0) {
        while (read (fd, &m, sizeof m) == sizeof m && (i = spot ()) != 0xFF) {
            memcpy (seen_at (i), &m, sizeof m);
        }
        close (fd);
    }
    return 1;
}

/* Chunk sn, cx, cy's map's place: its own, or (make: a place for it) a free one; 0xFF none (the region wanted first,
** with a folder) */
static unsigned char __fastcall__ map_of (unsigned char sn, unsigned cx, unsigned cy, unsigned char make)
{
    unsigned char i;

    rsn = sn;
    rrx = cx >> 3;
    rry = cy >> 3;
    hub_map (0);
    if (HUB->savedir[0] && !sreg_need ()) {
        return 0xFF;
    }
    for (i = 0; i < NSEEN; ++i) {
        if (seen_at (i)->used && seen_at (i)->sn == sn && seen_at (i)->cx == cx && seen_at (i)->cy == cy) {
            return i;
        }
    }
    if (!make) {
        return 0xFF;
    }
    hub_map (0);
    if (HUB->savedir[0]) {
        return spot ();
    }
    i = nexts;                                  /* (No folder: the next, round, the oldest made way for) */
    nexts = (nexts + 1) % NSEEN;
    return i;
}

void __fastcall__ seen_keep (unsigned char s)
{
    struct chunk* k;
    unsigned char i, j, n;

    hub_map (CACHE_BANK (s));
    k = CHUNK_AT (s);
    memset (&r, 0, sizeof r);
    r.used = 1;
    r.sn = k->sn;
    r.cx = k->cx;
    r.cy = k->cy;
    for (j = 0, n = 0; j < CW; ++j) {
        for (i = 0; i < CW; ++i) {
            if (k->tile[j][i] & T_SEEN) {
                r.bits[(j * CW + i) >> 3] |= 1 << (i & 7);
                n = 1;
            }
        }
    }
    for (i = 0; i < k->nrooms; ++i) {
        if (k->room[i].flags & R_VISITED) {
            r.rooms |= 1 << i;
        }
    }
    hub_map (0);
    if (!n) {
        return;                                 /* (Nothing seen: nothing to keep) */
    }
    memcpy (&cur, &r, sizeof r);                /* (Kept through map_of: its region's read in through r) */
    if ((i = map_of (cur.sn, cur.cx, cur.cy, 1)) != 0xFF) {
        memcpy (seen_at (i), &cur, sizeof cur);
    }
}

void __fastcall__ seen_over (struct chunk* k)
{
    unsigned char i, j, m;

    if ((i = map_of (k->sn, k->cx, k->cy, 0)) == 0xFF) {
        return;
    }
    memcpy (&r, seen_at (i), sizeof r);
    for (j = 0; j < CW * CW / 8; ++j) {
        if (r.bits[j]) {
            for (m = 0; m < 8; ++m) {
                if (r.bits[j] & (1 << m)) {
                    k->tile[(j * 8 + m) / CW][(j * 8 + m) % CW] |= T_SEEN;
                }
            }
        }
    }
    for (j = 0; j < k->nrooms; ++j) {
        if (r.rooms & (1 << j)) {
            k->room[j].flags |= R_VISITED;
        }
    }
}

/* ---- The big plans (plan.c's: more regions than PLAN_MAX, a save folder): each a file (plSS0000.dat), its regions in
** order, PL_PER to a page, the pages in the maker's own bank (plb, PL_PAGES of them), the least lately wanted written
** back (if it's changed) to make room for another */
#define PL_PER          42                      /* (Regions a page: 252 bytes) */
#define PL_BYTES        (PL_PER * sizeof (struct region))
#define PL_PAGES        32
static struct { unsigned char sn; unsigned page; unsigned char dirty; unsigned stamp; } plt[PL_PAGES];
static unsigned plstamp;
static unsigned char plbuf[PL_BYTES];
static unsigned char plim = PL_PAGES;           /* (The pages used: dq0 -C's 16, its plan's pages made way for soon) */

/* Stratum sn's plan's file's name, in path (0: no save folder): the window put back after (vf_name leaves the hub's
** header in it; the paging's code is an overlay's) */
static unsigned char __fastcall__ pl_name (unsigned char sn)
{
    unsigned w = hub_here ();
    unsigned char k = vf_name (path, "pl", sn, 0, 0);

    hub_back (w);
    return k;
}

/* (The paging overlay 2's, room made in the main RAM for the bank programs' loader: stratagen's, uses.c's; called
** through bank.s's stubs) */
#pragma code-name (push, "OV2CODE")
#pragma rodata-name (push, "OV2RODATA")

/* Stratum sn's plan's file, open (kept open for the next page: pl_flush closes it; trunc: emptied): its fd, or -1 */
static int plfd = -1;
static unsigned char plfsn;
static int __fastcall__ pl_fd (unsigned char sn, unsigned char trunc)
{
    if (plfd >= 0 && (plfsn != sn || trunc)) {
        close (plfd);
        plfd = -1;
    }
    if (plfd < 0 && pl_name (sn)) {
        plfd = open (path, trunc ? O_RDWR | O_CREAT | O_TRUNC : O_RDWR | O_CREAT, 0644);
        plfsn = sn;
    }
    return plfd;
}

/* Page k written back to its file, if it's changed */
static void __fastcall__ pl_back (unsigned char k)
{
    int fd;

    if (plt[k].sn && plt[k].dirty && (fd = pl_fd (plt[k].sn - 1, 0)) >= 0) {
        bread (plb, (char*) 0x8000 + k * 256, plbuf, PL_BYTES);
        lseek (fd, (long) plt[k].page * PL_BYTES, SEEK_SET);
        write (fd, plbuf, PL_BYTES);
    }
    plt[k].dirty = 0;
}

/* Stratum sn's region idx's page in the bank (read in, if it's not: the least lately wanted made room of): its
** record's place there */
static char* __fastcall__ pl_at (unsigned char sn, unsigned idx)
{
    unsigned page = idx / PL_PER;
    unsigned char k, b = 0;
    int fd, n;

    for (k = 0; k < plim; ++k) {
        if (plt[k].sn == sn + 1 && plt[k].page == page) {
            break;
        }
        if (plt[k].stamp - plt[b].stamp > 0x8000U || !plt[k].sn) {
            b = k;
        }
    }
    if (k == plim) {
        k = b;
        pl_back (k);
        memset (plbuf, 0, PL_BYTES);
        if ((fd = pl_fd (sn, 0)) >= 0) {
            lseek (fd, (long) page * PL_BYTES, SEEK_SET);
            n = read (fd, plbuf, PL_BYTES);
        }
        bwrite (plb, (char*) 0x8000 + k * 256, plbuf, PL_BYTES);
        plt[k].sn = sn + 1;
        plt[k].page = page;
    }
    plt[k].stamp = ++plstamp;
    return (char*) 0x8000 + k * 256 + (idx % PL_PER) * sizeof (struct region);
}

/* (Called from plan.c's code, an overlay's in the window: the window as it was after each, hub_back) */
void __fastcall__ o_pl_get (unsigned char sn, unsigned idx, struct region* to)
{
    unsigned w = hub_here ();

    bread (plb, pl_at (sn, idx), to, sizeof *to);
    hub_back (w);
}

void __fastcall__ o_pl_put (unsigned char sn, unsigned idx, const struct region* from)
{
    unsigned w = hub_here ();
    char* a = pl_at (sn, idx);
    unsigned char k;

    bwrite (plb, a, from, sizeof *from);
    for (k = 0; k < PL_PAGES && (plt[k].sn != sn + 1 || plt[k].page != idx / PL_PER); ++k) {
    }
    plt[k].dirty = 1;
    hub_back (w);
}

/* Stratum sn's big plan begun again (its file emptied, its pages in the bank none) */
void __fastcall__ o_pl_new (unsigned char sn)
{
    unsigned w = hub_here ();
    unsigned char k;

    hub_get (0, &HUB->small, &k, 1);           /* (The hub's header read through the main RAM: an overlay's) */
    plim = k ? 16 : PL_PAGES;
    for (k = 0; k < PL_PAGES; ++k) {
        if (plt[k].sn == sn + 1) {
            plt[k].sn = 0;
        }
    }
    pl_fd (sn, 1);
    hub_back (w);
}

/* Every page changed written back (a plan grown: the denizens read it from its file) */
void o_pl_flush (void)
{
    unsigned char k;

    for (k = 0; k < PL_PAGES; ++k) {
        pl_back (k);
    }
    if (plfd >= 0) {
        close (plfd);
        plfd = -1;
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The save (the hub's savedir): seen.dat (SM_MAGIC, the regions' table, then the maps in the banks); the squares
** seen in the chunks in the cache kept first (seen_keep), so nothing seen is lost.  (The change records are the
** world's: its changes.dat and the regions' files.)  0, or -1 */

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

static int __fastcall__ open_in (const char* name, int how)
{
    hub_map (0);
    strcpy (path, HUB->savedir);
    strcat (path, "/");
    strcat (path, name);
    return open (path, how, 0644);
}

int s_changes_save (void)
{
    static unsigned m = SM_MAGIC;
    unsigned char k;
    int fd;

    for (k = 0; k < NCSLOTS; ++k) {             /* (The seen squares of what's in the cache, kept) */
        hub_map (WORLD_BANK);                   /* (The cache's table the world bank's) */
        if (WREC->cslot[k].used) {
            seen_keep (k);
        }
    }
    if ((fd = open_in ("seen.dat", O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        return -1;
    }
    write (fd, &m, 2);
    write (fd, sreg, sizeof sreg);
    for (k = 0; k < NSEEN; ++k) {               /* (Through the main RAM: a bank isn't seen while write works) */
        memcpy (&r, seen_at (k), sizeof r);
        if (r.used) {
            write (fd, &r, sizeof r);
        }
    }
    close (fd);
    return 0;
}

int s_changes_restore (void)
{
    static unsigned m;
    unsigned char k;
    int fd;

    if ((fd = open_in ("seen.dat", O_RDONLY)) < 0) {
        return -1;
    }
    if (read (fd, &m, 2) != 2 || m != SM_MAGIC) {
        close (fd);
        return -1;                              /* (Not this version's) */
    }
    read (fd, sreg, sizeof sreg);
    for (k = 0; k < NSEEN && read (fd, &r, sizeof r) == sizeof r; ++k) {
        memcpy (seen_at (k), &r, sizeof r);
    }
    nexts = k % NSEEN;
    close (fd);
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
