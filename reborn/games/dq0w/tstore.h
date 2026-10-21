/*
** tstore.h - (dq0w) the traces' store (wtrace.c, wtrfile.c, wtrscent.c): its records, its directory, its files'
** groups, the kinds' numbers
*/

#ifndef _TSTORE_H
#define _TSTORE_H

/* ---- The store's records and its directory (the world's own banks: ovbank[OVBANKS + 2], [OVBANKS + 3]; the
** directory in ovbank[OVBANKS], ipage.c's, after its table) */
struct trace {
    unsigned char   c;                          /* Its chunk: the directory's entry plus 1 (0: a free record) */
    unsigned char   k;                          /* Whose: a kind (0 the character), or TK_SMOKE */
    unsigned        sq;                         /* Its square in its chunk (y * 32 + x), its way (H_*) times 1024,
                                                **   | TW_WAY (one known); smoke's, its strength times 1024 */
    unsigned        t;                          /* When the last went: the clock's TQs (its low 16 bits) */
    unsigned char   n;                          /* Passings (255 at most) */
    unsigned char   spare;
};
#define TK_SMOKE        0x7E
#define TW_WAY          0x2000
#define SQ_OF(s)        ((s) & 0x3FF)
#define NBUCKETS        512                     /* (Four records each, 256 to a bank) */
#define BUCKET(c, q)    (((q) + (c) * 167) & (NBUCKETS - 1))
#define TR_AT(b)        ((struct trace*) (0x8000 + ((b) & 255) * 32))
#define TR_BANK(b)      ovbank[OVBANKS + 2 + ((b) >> 8)]
typedef char trace_fits[sizeof (struct trace) == 8 ? 1 : -1];

#define NTCH            32                      /* (The chunks the store holds, at most; dq0 -C's, 3) */
struct tch {
    unsigned char   used, sn, flags, spare;     /* (flags TC_*) */
    unsigned        cx, cy;
    unsigned        stamp;                      /* When it was last wanted */
};
#define TC_SWEPT        0x01                    /* Kept swept by its keepers */
#define TC_DIRTY        0x02                    /* Changed since it was written to its region's file */
#define TR_MAGIC        0x5254                  /* "TR" */
struct thead {
    unsigned        magic;
    unsigned        jn, jpn;                    /* The journal's passings taken in so far: the denizens', the
                                                **   character's */
    unsigned        stamp;
    unsigned char   lsn;                        /* (The chunk the character was in as the store last looked: its
                                                **   stratum plus 1, 0 none yet) */
    unsigned        lcx, lcy;
    struct tch      ch[NTCH];
};
#define THD             ((struct thead*) 0x9800)
#define NK              32                      /* (The kinds' numbers: dq0tr.dat's, the first the character's) */
#define TREC            64                      /* (dq0tr.dat's record: dq0d/make.js's) */
struct tkinds {                                 /* The kinds' numbers (read in as the store begins: part 28's) */
    unsigned char   n;
    unsigned char   tread[NK], smell[NK], size[NK];
};
#define TKN             ((struct tkinds*) 0x9E00)
typedef char thead_fits[sizeof (struct ipg) <= 0x1800 && sizeof (struct thead) <= 0x600 &&
                        sizeof (struct tkinds) <= 0x180 ? 1 : -1];
struct tgroup {                                 /* A chunk's records in its region's file: a group */
    unsigned char   at;                         /* Its place in the region (x, and y times 8) */
    unsigned char   flags;                      /* TC_SWEPT */
    unsigned        n;
};

extern char path[56];                           /* (store.c's) */
extern struct change buf[8];                    /* (store.c's: 64 bytes, through the main RAM) */

#endif
