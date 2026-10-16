/*
** changes.c - (dq0m) what's changed in the dungeon, kept (DUNGEON.md: changing the dungeon): each change a record (a
** stratum, a square, what it became: its kind, its floor, the link it is), in banks of the maker's own, laid over a
** chunk each time it's made again; and the squares the character has seen, each chunk's as a map of bits, kept as
** the cache makes way for the chunk and laid over it when it's made again.  (The save, on the card, is the next
** step's: these are what it will write.)
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */

#define CH_BANKS        2
#define CH_PER_BANK     (0x2000 / sizeof (struct change))
#define SEEN_BANKS      2
#define SEEN_PER_BANK   (0x2000 / sizeof (struct seen))

struct change {
    unsigned char   sn;
    unsigned        x, y;
    unsigned char   tile;                       /* Its kind (but the seen bit) */
    signed char     elev;
    unsigned char   link;                       /* The stratum's link it is (an opening), or 0xFF */
};

struct seen {
    unsigned char   used, sn;
    unsigned        cx, cy;
    unsigned char   rooms;                      /* Its rooms visited (bits) */
    unsigned char   bits[CW * CW / 8];
};

static unsigned char chb, seb;                  /* The first banks */
unsigned char detail_bank;                      /* (The regions' details' cache: plan.c's, emptied here) */
static unsigned nch;                            /* Changes kept */
static unsigned char nseen, nexts;              /* Seen maps kept, the next made way for */

void changes_init (void)
{
    int b = hy_banks_alloc (CH_BANKS + SEEN_BANKS + 1);

    chb = b;
    seb = b + CH_BANKS;
    detail_bank = seb + SEEN_BANKS;
    for (nexts = 0; nexts <= SEEN_BANKS; ++nexts) {
        setbank (seb + nexts);
        memset ((void*) 0x8000, 0, 0x2000);
    }
    nexts = 0;
}

static struct change* __fastcall__ change_at (unsigned i)
{
    setbank (chb + i / CH_PER_BANK);
    return (struct change*) 0x8000 + i % CH_PER_BANK;
}

static struct seen* __fastcall__ seen_at (unsigned char i)
{
    setbank (seb + i / SEEN_PER_BANK);
    return (struct seen*) 0x8000 + i % SEEN_PER_BANK;
}

/* A change kept (the window moved) */
void __fastcall__ change_keep (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link)
{
    struct change* c;

    if (nch >= CH_BANKS * CH_PER_BANK) {
        return;                                 /* (Full: the save's, the next step's, will hold more) */
    }
    c = change_at (nch++);
    c->sn = sn;
    c->x = x;
    c->y = y;
    c->tile = tile;
    c->elev = elev;
    c->link = link;
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
        k->open[k->nopen].kind = strata[k->sn].link[link].kind;
        k->open[k->nopen].x = lx;
        k->open[k->nopen].y = ly;
        k->open[k->nopen].sill = 0;
        k->open[k->nopen++].link = link;
    }
}

/* The changes laid over a chunk just made (in the main RAM; its first square x, y in the frame): 1 if it's been
** stocked */
unsigned char __fastcall__ changes_over (struct chunk* k, unsigned x, unsigned y)
{
    static struct change c;
    unsigned i;
    unsigned char b = getbank (), stocked = 0;

    for (i = 0; i < nch; ++i) {
        memcpy (&c, change_at (i), sizeof c);
        if (c.sn == k->sn && c.x - x < CW && c.y - y < CW) {
            change_put (k, c.x - x, c.y - y, c.tile, c.elev, c.link);
            stocked |= c.link == STOCKED;
        }
    }
    setbank (b);
    return stocked;
}

/* A change on a chunk in the cache (slot s: the hub's bank in the window after) */
void __fastcall__ change_cached (unsigned char s, unsigned char lx, unsigned char ly, unsigned char tile,
    signed char elev, unsigned char link)
{
    static struct chunk* k;

    hub_map (CACHE_BANK (s));
    k = CHUNK_AT (s);
    change_put (k, lx, ly, tile, elev, link);
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

/* ---- The save (the hub's savedir): changes.dat (its count, then the records), seen.dat (the seen maps); the
** squares seen in the chunks in the cache kept first (seen_keep), so nothing seen is lost.  0, or -1 */

static char path[48];

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
    static unsigned char b[256];
    unsigned i, n;
    unsigned char k;
    int fd;

    for (k = 0; k < NCSLOTS; ++k) {             /* (The seen squares of what's in the cache, kept) */
        hub_map (0);
        if (HUB->cslot[k].used) {
            seen_keep (k);
        }
    }
    if ((fd = open_in ("changes.dat", O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        return -1;
    }
    write (fd, &nch, sizeof nch);
    for (i = 0; i < nch; i += n) {              /* (Through the main RAM: a bank isn't seen while write works) */
        n = nch - i > sizeof b / sizeof (struct change) ? sizeof b / sizeof (struct change) : nch - i;
        for (k = 0; k < n; ++k) {
            memcpy (b + k * sizeof (struct change), change_at (i + k), sizeof (struct change));
        }
        write (fd, b, n * sizeof (struct change));
    }
    close (fd);
    if ((fd = open_in ("seen.dat", O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        return -1;
    }
    for (k = 0; k < SEEN_BANKS * SEEN_PER_BANK; ++k) {
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
    unsigned i;
    unsigned char k;
    int fd;

    if ((fd = open_in ("changes.dat", O_RDONLY)) < 0) {
        return -1;
    }
    nch = 0;
    read (fd, &i, sizeof i);
    for (; nch < i && nch < CH_BANKS * CH_PER_BANK; ++nch) {
        if (read (fd, b, sizeof (struct change)) != sizeof (struct change)) {
            break;
        }
        memcpy (change_at (nch), b, sizeof (struct change));
    }
    close (fd);
    if ((fd = open_in ("seen.dat", O_RDONLY)) >= 0) {
        for (k = 0; k < SEEN_BANKS * SEEN_PER_BANK && read (fd, b, sizeof (struct seen)) == sizeof (struct seen); ++k) {
            memcpy (seen_at (k), b, sizeof (struct seen));
        }
        nexts = k % (SEEN_BANKS * SEEN_PER_BANK);
        close (fd);
    }
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
