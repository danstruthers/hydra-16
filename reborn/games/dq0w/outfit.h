/*
** outfit.h - (dq0w) the outfits, between their swapped parts: the draw's order (part 57, outfit.c: the referee asked,
** the outfits' file opened, its sections found in the order an outfit's drawn), a section drawn (part 59, outdraw.c:
** each line's choice drawn, priced and bought) and the making (part 58, outmake.c: each thing drawn made in the item
** store, where it goes, its own).  The things drawn in a list between them, in the hub's kinds' bank (the world's,
** after the outfits' index), the draw's state in the main RAM (ost).  A denizen's (I_KIT's): its role's things as its
** lair's placed, its lair's own with its first (its beds, its stores, its hoard: the [lair KEY] section's), its
** personal things as it's first met (its purse, its valuables: [purse KEY], else its standing's, [standing N])
*/

#ifndef _OUTFIT_H
#define _OUTFIT_H

enum { PL_NONE, PL_WORN, PL_HAND, PL_OFF, PL_BOTH, PL_BELT, PL_POUCH, PL_PACK, PL_IN,   /* (Where a thing goes:
                                                **   outfits.js's; and a denizen's, 2026-10-27: */
    PL_BED,                                     /*   at its bed, lying there (a square of its lair's home room, its
                                                **   own: the list's first such thing's), */
    PL_STASH,                                   /*   hidden at its bed (found by searching), */
    PL_HOME,                                    /*   on its lair's home room's floor (its stores: a square of its own,
                                                **   the list's first such thing's), */
    PL_HOARD,                                   /*   there, its faction's hoard (owner OW_HOARD | the faction: a
                                                **   robber's takings put in it), */
    PL_UNDER };                                 /*   worn beneath its clothes (hidden: found by searching its body) */
#define TF_LIT          0x01                    /* (A thing's flags: lit, */
#define TF_FULL         0x02                    /*   full, its water in it, */
#define TF_OWN          0x04                    /*   its own make, */
#define TF_FINE         0x08                    /*   a point better; the high four bits, a denizen's: its life's tenths
                                                **   lost, its condition, Treasure 1's, 2026-10-28) */
#define MT_JEWEL        0xFE                    /* (A thing's material: a jewel, Treasure 5's item and material drawn,
                                                **   part 59's) */
#define SF_WEAPON       0x01                    /* (A section's flags: a weapon skill's, */
#define SF_SHIELD       0x02                    /*   the shield's, */
#define SF_FREE         0x04                    /*   every line given (a denizen's), */
#define SF_KIT          0x08                    /*   the universal kit given (a temple's), */
#define SF_ARMED        0x10                    /*   its own lines its weapons: no weapon skill's drawn) */
#define OF_INDEX        ((unsigned char*) 0x9480)    /* (The outfits' file's head and its sections' index, read into
                                                **   the catalogue's bank after its code the first time (CAT_BANK):
                                                **   4 bytes, then 5 a section, then each one's length, 190 at most:
                                                **   outfits.js's, 2026-10-27) */

/* A thing drawn: its kind, how many (of a lot counted; else one), its make, where it goes, the thing it goes in (PL_IN:
** the list's entry plus 1), its flags, its material (MT_*: 0 its kind's) */
struct ofit {
    unsigned char   kind, qty, quality, place, in, flags, mat;
};
#define NOFIT           64
#define OF_N            (*(unsigned char*) 0x993F)      /* (The list's count, then the list, in the kinds' bank) */
#define OF_LIST         ((struct ofit*) 0x9940)

/* The draw's state (things.c's, in the main RAM: parts 57 and 59 both read and write it) */
struct ost {
    int             fd;                         /* The outfits' file, open */
    unsigned        budget, kept;               /* The money left to spend, and kept */
    unsigned        tags;                       /* The character's tags (rs.h's OT_*) */
    unsigned        rs;                         /* The draw's die */
    unsigned char   level;                      /* Its wealth: 0 poor, 1 average, 2 rich */
    unsigned char   freeall;                    /* Every line given (the universal kit, a temple's) */
    unsigned char   tool;                       /* A skill's tools drawn (one it has already not drawn again) */
    unsigned char   want;                       /* The sections drawn: those whose flags are these (SF_WEAPON, SF_SHIELD;
                                                **   0 a tool's; 0xFF any) */
    unsigned char   sflags, bent, keep, pn, pd; /* The last section read's: its flags, its bent's entry, its money kept,
                                                **   its purse (pn dice of pd) */
    unsigned        did;                        /* A denizen's (I_KIT's, last: not cleared as a draw begins): its number,
                                                **   the denizens' id, */
    unsigned        hx, hy;                     /*   its lair's home square, */
    unsigned char   hsn;                        /*   its stratum, */
    unsigned char   fac;                        /*   its faction (its lair's things': a hoard's owner), */
    unsigned        bx0, by0;                   /*   its lair's home room's floor (the home square's, if it's in no
                                                **   room): from bx0, by0, */
    unsigned char   bw, bh;                     /*   bw by bh squares */
    unsigned char   slen;                       /* The section found last's length (find's: draw_at reads that alone) */
};
extern struct ost ost;
enum { MD_ALL, MD_NEED, MD_CHANCE, MD_HEAD };   /* (A section's lines drawn: all, the needs, the rest; none, its head
                                                **   only read) */

unsigned char outfit (void);                    /* (Part 57) The character's outfit drawn (the referee asked): its
                                                **   things in the list, 0 none (the kit as it was) */
unsigned char __fastcall__ draw_at (unsigned off, unsigned char mode);   /* (Part 59) The outfits' section at off
                                                **   read, its head kept (ost), drawn as mode says if its flags are
                                                **   ost.want's: 1 drawn, 0 not (none there, not wanted) */
void __fastcall__ outmake (unsigned char who);  /* (Part 58) The list's things made: the character's (who 0), or a
                                                **   denizen's (1: ost's did, its things filed at hx, hy, IW_DEN) */
void __fastcall__ den_kit (const struct msg* m); /* (Part 57) A denizen's things drawn by its kind and made (I_KIT's:
                                                **   ost's did, hx, hy, hsn, fac set, the chunk in the store) */

#endif
