/*
** wfile.c - (dq0w) the save folder's files of the change records (store.c's: a region's, rgSXXYY.dat, read in as
** it's wanted, written back as its pages make way for another's, a version of it each time: vfile.inc's), the save
** (each region changed since written, then, the save's last step, its versions made the save's: changes.dat's
** stamps), and the game begun (its generation; a new game's folder rid of an old game's region files; a session's
** stamps, the save's kept, so a crash between a write-back and the next save loses nothing the save had and reads
** nothing it didn't); and the item store's (ipage.c's) kept with the game, items.dat, read back as it's begun.
** Swapped part 18's (the regions' files, the save), and 19's (the game begun, the item store kept: ../dq0/swap.inc,
** bank.s's stubs).
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)                      /* (Smaller: none of it's called again before it returns) */

#pragma code-name (push, "SW18CODE")
#pragma rodata-name (push, "SW18RODATA")
#pragma bss-name (push, "SW18RODATA")           /* (Its own data, what's wanted only while it runs: the main RAM's room) */

#define VF_STATIC       static
#include "../dq0/vfile.inc"

void stocks (void);
extern unsigned char rsn, rrx, rry;             /* (store.c's: the region wanted, */
extern struct change rec;                       /*   the record written, */
extern struct change buf[8];                    /*   records through the main RAM, */
extern char path[56];                           /*   a file's name) */
static struct vfh hd;

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

/* A region's pages written to its file, a new version (sn, rx, ry: its records, as many as its pages hold), its
** pages not dirty after */
static void __fastcall__ write_region (unsigned char sn, unsigned char rx, unsigned char ry)
{
    struct cpage* q;
    unsigned char p, i, k, n;
    unsigned all;
    int fd;

    if (!vf_name (path, "rg", sn, rx, ry) || (fd = vf_make (path, 2)) < 0) {
        return;
    }
    for (p = 0, all = 0; p < NCPAGES; ++p) {
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
        all += n;
    }
    vf_done (fd, all);
}

/* A page for the region wanted: a free one, or (with a folder) the pages of the region least lately wanted written
** to its file and freed: one of them; 0xFF, none (the lock held) */
unsigned char s_store_page (void)
{
    struct cpage* q;
    unsigned char p, best = 0xFF, sn, rx, ry, lim;

    hub_map (0);
    lim = HUB->small ? 1 : NCPAGES;             /* (dq0 -C: a page, the tests' write-backs) */
    hub_map (WORLD_BANK);
    for (p = 0, q = WREC->cpage; p < lim; ++p, ++q) {
        if (!q->used) {
            return p;
        }
        if ((!q->used || q->sn != rsn || q->rx != rrx || q->ry != rry) &&
            (best == 0xFF || q->stamp - WREC->cpage[best].stamp > 0x8000U)) {
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
    write_region (sn, rx, ry);                  /* (Clean or not: one version, all of its records) */
    hub_map (WORLD_BANK);
    for (p = 0, q = WREC->cpage; p < NCPAGES; ++p, ++q) {
        if (q->used && q->sn == sn && q->rx == rx && q->ry == ry) {
            q->used = 0;
        }
    }
    return best;
}

/* The region wanted read in from its file, if it has a version of this game's: its records into pages of their own,
** not dirty (the lock held) */
void s_store_in (void)
{
    struct cpage* q;
    unsigned left;
    unsigned char p, k, i;
    int fd;

    if (!vf_name (path, "rg", rsn, rrx, rry) || (fd = vf_open (path, 2, &hd)) < 0) {
        return;
    }
    for (left = hd.n; left && (p = s_store_page ()) != 0xFF; ) {
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

#pragma bss-name (push, "BSS")                  /* (Its locals the main RAM's: it calls a part, which puts this one
                                                **   out and in again, its own data as its image has it) */
/* The save (the UI's request: the hub's savedir): the chunks stocked marked, each region's pages that have changed
** written to its file (its last step, part 19's store_commit) */
void s_store_save (void)
{
    struct cpage* q;
    unsigned char p;

    stocks ();
    if (!in_folder ("")) {
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
}
#pragma bss-name (pop)

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)

/* ==== Part 19: the game begun, the item store kept */
#pragma code-name (push, "SW19CODE")
#pragma rodata-name (push, "SW19RODATA")
#pragma bss-name (push, "SW19RODATA")

static struct vfh bh;

/* The save folder's file name (the hub's savedir: "" none): 0, or its path in path */
static unsigned char __fastcall__ in_dir (const char* name)
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

/* changes.dat written: the generation, CH_VER, the save's stamp and the most this session may give (the hub's vsaved
** and vhi) */
static void stamps_kept (void)
{
    int fd;

    if (in_dir ("changes.dat") && (fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        hub_map (WORLD_BANK);
        bh.magic = CH_MAGIC;
        bh.gen = WREC->cgen;
        bh.n = CH_VER;
        hub_map (0);
        bh.stamp = HUB->vsaved;
        write (fd, &bh, sizeof bh);
        bh.stamp = HUB->vhi;                  /* (Through the main RAM: the hub's window isn't the kernel's) */
        write (fd, &bh.stamp, 4);
        close (fd);
    }
    hub_map (0);
}

/* The save's last step (the change records, the things' and the world's own kept): every version written so far made
** the save's, changes.dat's stamps (part 19's) */
void s_store_commit (void)
{
    hub_map (0);
    if (HUB->vhi) {
        HUB->vsaved = HUB->vlo = HUB->vhi;
        HUB->vhi += VF_BLOCK;
        stamps_kept ();
    }
}

/* A new game's folder rid of an old game's region files (rg, it, dn, sm ...: preSXXYY.da?), read a stat record at a
** time (through buf: 64 bytes), till a pass finds none */
static void tidy (void)
{
    static const char pre[] = "rgitdnsmstpl";
    struct hy_regs r;
    unsigned char k, n, h, gone;
    char* e;
    int fd;

    do {
        gone = 0;
        if (!in_dir ("") || (fd = open (HUB->savedir, O_RDONLY)) < 0) {
            return;
        }
        while (read (fd, buf, HY_SR_SIZE) == HY_SR_SIZE) {
            e = (char*) buf + HY_SR_NAME;
            n = strlen (e);
            for (k = 0; k < sizeof pre - 1 && (e[0] != pre[k] || e[1] != pre[k + 1]); k += 2) {
            }
            for (h = 2; h < n && (e[h] >= '0' && e[h] <= '9' || e[h] >= 'a' && e[h] <= 'f'); ++h) {
            }
            if (k < sizeof pre - 1 && (n == 11 || n == 12) && h == n - 4 && !strncmp (e + h, ".da", 3)) {
                in_dir (e);
                r.r[0] = (unsigned) path;
                hy_call (HY_REMOVE, &r);
                gone = 1;
            }
        }
        close (fd);
    } while (gone);
}

/* A version 8 save's item store (wfile.c's keep: its table not kept, IPG's rebuild): the chunks its things lie in,
** each in the table (as the game's carried on, the strata's records known) */
static void rebuild (void)
{
    struct ires* r;
    unsigned i = 0, cx, cy;
    unsigned char sn, k, f;

    hy_bank (ovbank[OVBANKS]);
    if (!IPG->rebuild) {
        return;
    }
    IPG->rebuild = 0;
    while ((i = items (i, IN_SQUARE | IN_PORTAL, 0)) != 0) {
        sn = ip->sn;
        cx = (ip->x - strata[sn].x0) >> CW_BITS;
        cy = (ip->y - strata[sn].y0) >> CW_BITS;
        hy_bank (ovbank[OVBANKS]);
        for (k = 0, f = 0xFF, r = IPG->r; k < NIRES; ++k, ++r) {
            if (r->used && r->sn == sn && r->cx == cx && r->cy == cy) {
                break;
            }
            if (!r->used && f == 0xFF) {
                f = k;
            }
        }
        if (k == NIRES && f != 0xFF) {
            r = &IPG->r[f];
            r->used = 1;
            r->sn = sn;
            r->cx = cx;
            r->cy = cy;
        }
    }
}

/* The game begun (resume: carried on from its save): its generation, the last game's in the folder and one more (a
** new game's: the old game's region files gone), or its own; its stamps (the save's, and this session's from the most
** the last could have given: changes.dat's, written again before anything's written); a version 6 save's records (its
** changes.dat: their count, then them) read in */
#pragma bss-name (push, "BSS")                  /* (Its locals the main RAM's: it calls a part, which puts this one
                                                **   out and in again, its own data as its image has it) */
void __fastcall__ s_store_begin (unsigned char resume)
{
    unsigned gen = 0, n = 0;
    unsigned long hi = 0;
    int fd;

    hub_map (0);
    HUB->vsaved = 0;
    if (in_dir ("changes.dat") && (fd = open (path, O_RDONLY)) >= 0) {
        if (read (fd, &bh, sizeof (struct chead)) == sizeof (struct chead) && bh.magic == CH_MAGIC) {
            gen = bh.gen;
            if (bh.n >= 2 && read (fd, &bh.stamp, 4) == 4 && read (fd, &hi, 4) == 4 && resume) {
                hub_map (0);
                HUB->vsaved = bh.stamp;         /* (A version 9 save's stamps) */
            }
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
    if (!resume) {
        HUB->vsaved = hi = 0;
        tidy ();
    }
    hub_map (0);
    HUB->vbase = HUB->vlo = hi;
    HUB->vhi = hi + VF_BLOCK;
    stamps_kept ();
    if (resume) {
        rebuild ();
    }
    hub_map (0);
}
#pragma bss-name (pop)

/* ---- The item store kept (back: read again), the save folder's items.dat: its banks a piece at a time, through
** main RAM (the window's only the task's own: the kernel's copies want the task's memory), then its table (IP_MAGIC,
** and its entries: ipage.c's); a version 8 save's, without one: its table made as the character first comes into a
** chunk (it_near's) */
#define PIECE   64
void __fastcall__ s_keep (unsigned char back)
{
    unsigned char b;
    unsigned off;
    int fd;

    if (!in_dir ("items.dat") || (fd = back ? open (path, O_RDONLY) : open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        return;
    }
    for (b = 0; b < ITEM_BANKS; ++b) {
        for (off = 0; off < 0x2000; off += PIECE) {
            if (back) {
                if (read (fd, buf, PIECE) != PIECE) {
                    memset (buf, 0, PIECE);
                }
                hub_map (CAT_BANK + 1 + b);
                memcpy ((char*) 0x8000 + off, buf, PIECE);
            } else {
                hub_map (CAT_BANK + 1 + b);
                memcpy (buf, (char*) 0x8000 + off, PIECE);
                write (fd, buf, PIECE);
            }
        }
    }
    off = IP_MAGIC;
    if (!back) {
        write (fd, &off, 2);
    } else if (read (fd, &off, 2) != 2 || off != IP_MAGIC) {
        hy_bank (ovbank[OVBANKS]);
        IPG->rebuild = 1;                       /* (A version 8 save's: none) */
        goto done;
    }
    for (off = 0; off < NIRES * sizeof (struct ires) + 2; off += PIECE) {     /* (Its table: its entries, its
                                                **   stamp) */
        if (back) {
            read (fd, buf, PIECE);
            hy_bank (ovbank[OVBANKS]);
            memcpy ((char*) IPG->r + off, buf, PIECE);
        } else {
            hy_bank (ovbank[OVBANKS]);
            memcpy (buf, (char*) IPG->r + off, PIECE);
            write (fd, buf, PIECE);
        }
    }
    hy_bank (ovbank[OVBANKS]);
    IPG->rebuild = 0;
done:
    close (fd);
    hub_map (0);
}

/* The store begun: the catalogue read into the hub's catalogue bank (dir/dq0w.cat: kinds.c's and the code that
** handles things, linked at $8000), the store emptied, its table too (its paging's); a game carried on, its kept read
** in, and the character's light from it */
void __fastcall__ s_items_begin (const char* dir)
{
    unsigned off;
    unsigned char b;
    int fd, n;

    hy_bank (ovbank[OVBANKS]);
    memset ((void*) 0x8000, 0, 0x2000);
    strcpy (path, dir);
    strcat (path, "/dq0w.cat");
    hub_map (CAT_BANK);
    memset ((void*) 0x8000, 0, 0x2000);
    if ((fd = open (path, O_RDONLY)) >= 0) {
        for (off = 0; off < 0x2000 && (n = read (fd, buf, PIECE)) > 0; off += n) {
            hub_map (CAT_BANK);
            memcpy ((char*) 0x8000 + off, buf, n);
        }
        close (fd);
    }
    for (b = 0; b < ITEM_BANKS; ++b) {          /* (The store empty; a game carried on, its kept) */
        hub_map (CAT_BANK + 1 + b);
        memset ((void*) 0x8000, 0, 0x2000);
    }
    hub_map (0);
    ilim = HUB->small ? 24 : NITEMS;            /* (dq0 -C: the tests' small store, its chunks paged soon) */
    irs = (unsigned) HUB->seed | 1;
    if (HUB->resume) {
        s_keep (1);
        light_now ();
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
