/*
** outfit.h - (dq0w) the outfits, between their swapped parts: the draw's order (part 57, outfit.c: the referee asked,
** the outfits' file opened, its sections found in the order an outfit's drawn), a section drawn (part 59, outdraw.c:
** each line's choice drawn, priced and bought) and the making (part 58, outmake.c: each thing drawn made in the item
** store, where it goes, its own).  The things drawn in a list between them, in the hub's kinds' bank (the world's,
** after the outfits' index), the draw's state in the main RAM (ost)
*/

#ifndef _OUTFIT_H
#define _OUTFIT_H

enum { PL_NONE, PL_WORN, PL_HAND, PL_OFF, PL_BOTH, PL_BELT, PL_POUCH, PL_PACK, PL_IN };   /* (Where a thing goes:
                                                **   outfits.js's) */
#define TF_LIT          0x01                    /* (A thing's flags: lit, */
#define TF_FULL         0x02                    /*   full, its water in it, */
#define TF_OWN          0x04                    /*   its own make, */
#define TF_FINE         0x08                    /*   a point better) */
#define SF_WEAPON       0x01                    /* (A section's flags: a weapon skill's, */
#define SF_SHIELD       0x02                    /*   the shield's, */
#define SF_FREE         0x04                    /*   every line given (a denizen's), */
#define SF_KIT          0x08                    /*   the universal kit given (a temple's), */
#define SF_ARMED        0x10                    /*   its own lines its weapons: no weapon skill's drawn) */
#define OF_INDEX        ((unsigned char*) 0x9400)    /* (The outfits' file's head and its sections' index, read into
                                                **   the kinds' bank after the kinds and their words the first time:
                                                **   4 bytes, then 4 a section, 255 at most) */

/* A thing drawn: its kind, how many (of a lot counted; else one), its make, where it goes, the thing it goes in (PL_IN:
** the list's entry plus 1), its flags */
struct ofit {
    unsigned char   kind, qty, quality, place, in, flags;
};
#define NOFIT           64
#define OF_N            (*(unsigned char*) 0x9800)      /* (The list's count, then the list, in the kinds' bank) */
#define OF_LIST         ((struct ofit*) 0x9801)

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
    unsigned char   hsn;                        /*   its stratum */
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
void __fastcall__ den_kit (unsigned char kind); /* (Part 57) A denizen's things drawn by its kind and made (ost's did,
                                                **   hx, hy, hsn: I_KIT's) */

#endif
