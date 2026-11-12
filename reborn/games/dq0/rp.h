/*
** rp.h - Dingus Quest Zero's record store (ENGINE-CRITIQUE.md's §6 and step 7, 2026-11-11; DECISIONS.md's "One record
** store"): what the game's stores of records paged by region share, in the paged ROM's library once (modules/dq0lib:
** RP_*), each store keeping only its own logic.  The stores: the change records (../dq0w/store.c, wfile.c: rg), the
** strata's (../dq0w/sfile.c: st), the traces (../dq0w/wtrace.c, wtrfile.c: tr), the things (../dq0w/ipage.c: it), the
** denizens' (../dq0d/dq0d.c: dn), the seen maps (../dq0m/changes.c: sm, and the change records read: rg), the chunks'
** cache (../dq0w/chunk.c, every reader's, and ../dq0m/make.c's: the hub's, not a file's).
**
** One file-naming scheme and one save-and-restore path (rp.inc's rp_name, rp_open, rp_make, rp_done): a region's file
** in the save folder (preSXXYY.dat), written whole as a new version each time, read as the newest valid version (by the
** hub's vsaved and vbase: the last save's, or this session's), so the save is the stamps moved on (wfile.c's
** store_commit) and a crash loses nothing the save had.  One eviction policy (RP_OLDEST): the entry least lately wanted,
** but those kept (the one at hand, a box about the character, those a store's own routine keeps).  One lookup
** (RP_FIND): an entry by its key, its stamp made the newest.
**
** Each call is rp_x (dq0lib.inc's: an XCALL, some 120 cycles) with a block of the caller's: struct rpf (a file's) or
** struct rpt (a table's), its first byte the entry; the ROM's offsets (dq0lib.s's RF_*, RT_*) the same, checked below.
*/

#ifndef _RP_H
#define _RP_H

#include <stddef.h>

#define RP_NAME         0x36                    /* The library's entries (their low bytes) */
#define RP_OPEN         0x39
#define RP_MAKE         0x3C
#define RP_DONE         0x3F
#define RP_FIND         0x42
#define RP_OLDEST       0x45

unsigned __fastcall__ rp_x (void* a);           /* (dq0lib.inc's) */

/* A kind's versions counted (the main RAM's, each kind's own: a stamp is the hub's vlo plus the kind's count since it
** last moved on) */
struct rpk {
    unsigned long   vlast;
    unsigned        vcount;
};

/* A version's header (world.h's struct vfh, the same) */
struct rph {
    unsigned        magic, gen, n;
    unsigned long   stamp;
};

/* A file's call (rp.inc's) */
struct rpf {
    unsigned char   op, slots;                  /* The entry; the file's slots (2, 3) */
    char*           path;                       /* Its name (rp_name's) */
    struct rpk*     kind;
    unsigned        gen;                        /* The game's generation (the world bank's cgen) */
    const void*     hub;                        /* The hub's vsaved, vbase, vlo, vhi (its header in the window) */
    struct rph      h;                          /* The newest version's header (open's); done's (its n) */
    struct rph      t;                          /* (The library's: a header read, */
    unsigned long   st[3];                      /*   each slot's stamp, */
    unsigned char   save, nw, fd, k, vfd;       /*   the save's slot, the newest's; its fd, or done's) */
    const char*     dir;                        /* name's: the save folder (the hub's savedir), */
    char            pre[2];                     /*   the store's letters, */
    unsigned char   sn, rx, ry;                 /*   the region */
};

/* A table's call: its entries (RT_N of them, each RT_SIZE bytes, used if its first byte isn't 0, its key in its first
** eight bytes: the mask key, bit i byte i) */
struct rpt {
    unsigned char   op, n, size, key;
    void*           at;                         /* The table (the window's, or the task's RAM) */
    unsigned char   so, flags;                  /* Its stamp's offset; RTF_* */
    unsigned*       stamp;                      /* find: the clock, ++ into the entry found (0: none) */
    unsigned char   k[8];                       /* The key, as an entry has it */
    unsigned char   from, nf, free;             /* find: the first entry looked at; a free one among the first nf, found
                                                **   (0xFF none) */
    unsigned char   r, nsn, nx, ny;             /* oldest: the box kept (r each way of its centre, 0xFF none), its
                                                **   stratum's, x's and y's offsets in an entry */
    unsigned char   vsn;                        /*   its centre */
    unsigned        vx, vy;
    unsigned char __fastcall__ (*keep) (unsigned char k);  /* oldest: the store's own: entry k kept (not 0), or 0 */
};
#define RTF_FREE        0x01                    /* oldest: a free entry first */
#define RTF_HAND        0x02                    /* oldest: the key's own kept */
#define RTF_PLAIN       0x04                    /* oldest: stamps compared plainly (else round, as they wrap) */
#define RTF_WIDE        0x08                    /* the box's x and y two bytes (else one) */

typedef char rp_fits[offsetof (struct rpf, h) == 10 && offsetof (struct rpf, t) == 20 && offsetof (struct rpf, st) == 30 &&
    offsetof (struct rpf, save) == 42 && offsetof (struct rpf, vfd) == 46 && offsetof (struct rpf, dir) == 47 &&
    offsetof (struct rpf, ry) == 53 && offsetof (struct rpt, k) == 10 && offsetof (struct rpt, from) == 18 &&
    offsetof (struct rpt, r) == 21 && offsetof (struct rpt, vsn) == 25 && offsetof (struct rpt, keep) == 30 ? 1 : -1];

#endif
