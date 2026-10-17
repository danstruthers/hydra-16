/*
** things.h - (dq0w) the item store, the world's alone: things.c (its code in the main RAM, its swapped parts' and
** the catalogue's bank's), kinds.c (the catalogue), items.s (its records reached)
*/

/* ---- The main RAM's (things.c's, items.s's) */
struct item* __fastcall__ item_at (unsigned i); /* Item i's record, its bank in the window (items.s) */
unsigned __fastcall__ items (unsigned i, unsigned char in, unsigned x);  /* The next record after i in use and in
                                                **   one of in's places (IN_*), at x (IN_X) or with one of x's flags
                                                **   (IN_FLAGS): its number (0: none), ip its record, in the window */
extern struct item* ip;
#define IN_SQUARE       (1 << IW_SQUARE)
#define IN_ACTOR        (1 << IW_ACTOR)
#define IN_ITEM         (1 << IW_ITEM)
#define IN_PORTAL       (1 << IW_PORTAL)
#define IN_X            0x80
#define IN_FLAGS        0x40
void owned (void);                              /* The things a look saw (the senses' record's) made their own, as
                                                **   the senses did as they saw them (make_own: no two alike) */

/* ---- The item store's paging (ipage.c, part 17's: bank.s's stubs): the chunks whose things are in the store, a
** table in the world's own bank (ovbank[OVBANKS]), and a chunk's things' records as they're read in or written out
** (the map); a chunk's things in its region's file, a group (its place in the region: x, and y times 8; its count) */
#define NIRES           64
#define NNONE           16
struct ires {
    unsigned char   used, sn;
    unsigned        cx, cy;
    unsigned        stamp;                      /* When it was last wanted */
};
struct ipg {
    struct ires     r[NIRES];
    unsigned        stamp;
    unsigned char   rebuild;                    /* (A version 8 save's store, its table not yet made: it_near's) */
    unsigned char   lsn;                        /* (The chunk the character was in, as it_near last looked: its
                                                **   stratum plus 1, 0 none yet) */
    unsigned        lcx, lcy;
    struct {
        unsigned char sn, rx, ry;
    }               none[NNONE];                /* (Regions known to have no file of things yet: none read in
                                                **   for them, a part's swap spared; a region put out, none: sn its
                                                **   stratum plus 1, 0 the entry free) */
    unsigned char   nnext;
    unsigned        map[NITEMS];
};
#define IPG             ((struct ipg*) 0x8000)
#define IP_MAGIC        0x5049                  /* "IP": items.dat's table, after the store's banks */
struct igroup {
    unsigned char   at;
    unsigned        n;
};
unsigned char it_room (void);                   /* A record made free (a chunk's things put out): 1, or 0 */
unsigned char __fastcall__ it_need (unsigned char sn, unsigned cx, unsigned cy);    /* A chunk's things in the store
                                                **   (read in, if they aren't): 1, or 0 */
unsigned char __fastcall__ it_out (unsigned char sn, unsigned cx, unsigned cy);   /* (Part 20's) A chunk's things
                                                **   written to its region's file, their records freed: 1, or 0 */
unsigned char __fastcall__ it_in (unsigned char sn, unsigned cx, unsigned cy);    /* (Part 20's) Read in: 1 (2, its region
                                                **   has no file), or 0 */
void it_near (void);                              /* The chunks round the character's in the store, as it comes into
                                                **   one (dq0w.c's see, as it's moved) */
void __fastcall__ start (const char* dir);      /* (Part 3's: dq0w.c) The world's start, once it's attached */
void __fastcall__ keep (unsigned char back);    /* The store kept (items.dat), or read back */
extern unsigned ilim;                           /* (things.c's: the records used, at most) */
extern unsigned irs;                            /* (things.c's: a thing's own numbers, made) */
#define OVBANKS         8                       /* (dq0w.cfg's OV1 ...: the parts' images; and two more, the world's
                                                **   own: ipage.c's, ovbank[OVBANKS]; the ways across a room's,
                                                **   dq0w.c's, ovbank[OVBANKS + 1]) */
extern unsigned char ovbank[OVBANKS + 2];

/* ---- The swapped parts' (bank.s's stubs) */
unsigned char serve (void);                     /* (Part 10) The hub's ireq answered (but its state): 1 if what lies
                                                **   about has changed (the world looks again) */
unsigned char things_req (void);                /* The same, what's carried listed, a tool's wear, by part 11 alone */
void __fastcall__ items_begin (const char* dir);    /* (Part 13) The catalogue into the hub (dir's dq0w.cat), the store
                                                **   emptied, a game carried on its read, its light */
void __fastcall__ stock_down (unsigned char k); /* (Part 13) Cache slot k's chunk's things put down (the maker's
                                                **   list, the world bank's stock) */
void __fastcall__ lay_body (unsigned char kind, unsigned char sn, unsigned x, unsigned y);   /* (Part 13) A denizen's
                                                **   remains put down where it fell (KI_BODY, KI_BONES) */
unsigned char puzzle (void);                    /* (Part 16: wgame.c) The hub's ireq, IO_PUZZLE's, answered (but its
                                                **   state): 1 if what lies about has changed */
void __fastcall__ take_go (const struct msg* m);    /* (Part 16) I_TAKE's walk begun */
unsigned char take_up (void);                   /* (Part 16) Its thing taken up, the walk done: 0 not (E_READY to
                                                **   tell), 1 taken (the UI's to go on), 2 walking on to it */
extern unsigned take_n;                         /* (I_TAKE's thing, its item: 0 none) */
unsigned char __fastcall__ in_reach (unsigned n);
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);
void light_now (void);
void __fastcall__ go_to (unsigned x, unsigned y);

/* The things' words (kinds.c's said, in the catalogue's bank: % the thing's name, # the other's) */
enum {
    SA_HOLD, SA_HOLD2, SA_HELD, SA_HANDS, SA_STOW, SA_NO_PACK, SA_THERE, SA_PUT_IN, SA_PUT_ON, SA_HANG, SA_NO_HANG,
    SA_NO_SLOT, SA_NO_BELT, SA_NOT_HOLDER, SA_RUN_OUT, SA_WONT_GO, SA_ITSELF, SA_PUT_OUT, SA_NO_ROOM, SA_TOO_HEAVY,
    SA_TAKE, SA_NOT_IN, SA_DON, SA_WORN, SA_WEAR_ONE, SA_NO_WEAR, SA_DOFF, SA_NOT_WORN, SA_LIGHT, SA_LIT,
    SA_NO_FLAME, SA_NO_LIGHT, SA_DOUSE, SA_NOT_LIT, SA_NONE_TO_LIGHT, SA_NONE_LIT, SA_NO_SUCH, SA_NO_OTHER, SA_REACH,
    SA_WHERE, SA_WHAT, SA_IN_HAND, SA_IN_PACK, SA_FIXED, SA_COUNT
};
extern const char* const said[SA_COUNT];
