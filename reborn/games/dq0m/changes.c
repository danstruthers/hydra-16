/*
** changes.c - (dq0m) what's changed in the dungeon laid over a chunk each time it's made again (DUNGEON.md: changing
** the dungeon): the change records (world.h's), the world's to write (dq0w/store.c), read here only: its region's in
** the hub's change pages, or, if none of them are there, in its file in the save folder (rgSXXYY.dat, its newest
** valid version: ../dq0/vfile.inc's), under the hub's change_lock.  And the squares the character has seen, each chunk's as a map of bits,
** kept as the cache makes way for the chunk and laid over it when it's made again, in banks of the maker's own, and
** in the save (seen.dat: the UI asks, M_SAVE; back as the game's carried on, M_RESTORE).
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

static unsigned char seb;                       /* The first bank */
unsigned char detail_bank;                      /* (The regions' details' cache: plan.c's, emptied here) */
static unsigned char nexts;                     /* The next seen map made way for */
static char path[56];

void changes_init (void)
{
    int b = hy_banks_alloc (SEEN_BANKS + 1);

    seb = b;
    detail_bank = seb + SEEN_BANKS;
    for (nexts = 0; nexts <= SEEN_BANKS; ++nexts) {
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

/* ---- The squares seen: a chunk's kept as the cache makes way for it (slot s), laid over it when it's made again */

void __fastcall__ seen_keep (unsigned char s)
{
    static struct seen r;
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
    if (!n) {
        return;                                 /* (Nothing seen: nothing to keep) */
    }
    for (i = 0; i < SEEN_BANKS * SEEN_PER_BANK; ++i) {   /* (Its own map again, if it has one) */
        if (seen_at (i)->used && seen_at (i)->sn == r.sn && seen_at (i)->cx == r.cx && seen_at (i)->cy == r.cy) {
            break;
        }
    }
    if (i == SEEN_BANKS * SEEN_PER_BANK) {
        i = nexts;                              /* (Else the next, round: the oldest made way for) */
        nexts = (nexts + 1) % (SEEN_BANKS * SEEN_PER_BANK);
    }
    memcpy (seen_at (i), &r, sizeof r);
}

void __fastcall__ seen_over (struct chunk* k)
{
    static struct seen r;
    unsigned char i, j, m;

    for (i = 0; i < SEEN_BANKS * SEEN_PER_BANK; ++i) {
        memcpy (&r, seen_at (i), sizeof r);
        if (r.used && r.sn == k->sn && r.cx == k->cx && r.cy == k->cy) {
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
            break;
        }
    }
}

/* ---- The save (the hub's savedir): seen.dat (the seen maps); the squares seen in the chunks in the cache kept first
** (seen_keep), so nothing seen is lost.  (The change records are the world's: its changes.dat and the regions'
** files.)  0, or -1 */

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
    static unsigned char b[sizeof (struct seen)];
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
    for (k = 0; k < SEEN_BANKS * SEEN_PER_BANK; ++k) {   /* (Through the main RAM: a bank isn't seen while write
                                                **   works) */
        memcpy (b, seen_at (k), sizeof (struct seen));
        if (((struct seen*) b)->used) {
            write (fd, b, sizeof (struct seen));
        }
    }
    close (fd);
    return 0;
}

int s_changes_restore (void)
{
    static unsigned char b[sizeof (struct seen)];
    unsigned char k;
    int fd;

    if ((fd = open_in ("seen.dat", O_RDONLY)) < 0) {
        return -1;
    }
    for (k = 0; k < SEEN_BANKS * SEEN_PER_BANK && read (fd, b, sizeof (struct seen)) == sizeof (struct seen); ++k) {
        memcpy (seen_at (k), b, sizeof (struct seen));
    }
    nexts = k % (SEEN_BANKS * SEEN_PER_BANK);
    close (fd);
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
