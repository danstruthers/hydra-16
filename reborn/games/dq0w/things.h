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
** (the map); a chunk's things in its region's file, a group (its place in the region: x, and y times 8; its count; the
** clock as it was written, CLOCK_256's: its lights caught up, burnt down by the minutes since, as it's read in) */
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
    unsigned        m;
};
unsigned char it_room (void);                   /* A record made free (a chunk's things put out): 1, or 0 */
unsigned char __fastcall__ it_need (unsigned char sn, unsigned cx, unsigned cy);    /* A chunk's things in the store
                                                **   (read in, if they aren't): 1, or 0 */
unsigned char __fastcall__ it_out (unsigned char sn, unsigned cx, unsigned cy);   /* (Part 20's) A chunk's things
                                                **   written to its region's file, their records freed: 1, or 0 */
unsigned char __fastcall__ it_in (unsigned char sn, unsigned cx, unsigned cy);    /* (Part 20's) Read in: 1 (2, its region
                                                **   has no file), or 0 */
void __fastcall__ it_flush (unsigned char sn);   /* (Part 17's) Stratum sn's chunks' things all put out (its slot
                                                **   made way for: dq0w.c's slot_for) */
unsigned char __fastcall__ st_out (unsigned char sn);   /* (Part 22's: sfile.c) A stratum's record to its file */
unsigned char __fastcall__ st_in (unsigned char sn);    /* (Part 22's) Read back into its slot: 1, or 0 none */
unsigned char __fastcall__ stratum_in (unsigned char m);    /* (Part 3's: dq0w.c) Stratum m in its slot: 1, or 0
                                                **   (not made yet) */
void it_near (void);                              /* The chunks round the character's in the store, as it comes into
                                                **   one (dq0w.c's see, as it's moved) */
void __fastcall__ start (const char* dir);      /* (Part 3's: dq0w.c) The world's start, once it's attached */
void __fastcall__ keep (unsigned char back);    /* The store kept (items.dat), or read back */
extern unsigned ilim;                           /* (things.c's: the records used, at most) */
extern unsigned irs;                            /* (things.c's: a thing's own numbers, made) */
#define OVBANKS         15                      /* (dq0w.cfg's OV1 ...: the parts' images; and four more, the world's
                                                **   own: ipage.c's, ovbank[OVBANKS]; the ways across a room's,
                                                **   dq0w.c's, ovbank[OVBANKS + 1]; the traces' store's, wtrace.c's,
                                                **   ovbank[OVBANKS + 2] and [OVBANKS + 3]) */
extern unsigned char ovbank[OVBANKS + 4];

/* ---- The swapped parts' (bank.s's stubs) */
unsigned char serve (void);                     /* (Part 10) The hub's ireq answered (but its state): 1 if what lies
                                                **   about has changed (the world looks again) */
unsigned char things_req (void);                /* The same, what's carried listed, a tool's wear, by part 11 alone */
void __fastcall__ items_begin (const char* dir);    /* (Part 13) The catalogue into the hub (dir's dq0w.cat), the store
                                                **   emptied, a game carried on its read, its light */
void __fastcall__ stock_down (unsigned char k); /* (Part 13) Cache slot k's chunk's things put down (the maker's
                                                **   list, the world bank's stock) */
unsigned __fastcall__ lay_body (unsigned char kind, unsigned char sn, unsigned x, unsigned y);   /* (Part 13) A denizen's
                                                **   remains put down where it fell (KI_BODY, KI_BONES) */
unsigned char puzzle (void);                    /* (Part 16: wgame.c) The hub's ireq, IO_PUZZLE's, answered (but its
                                                **   state): 1 if what lies about has changed */
void __fastcall__ take_go (const struct msg* m);    /* (Part 16) I_TAKE's walk begun */
unsigned char take_up (void);                   /* (Part 16) Its thing taken up, the walk done: 0 not (E_READY to
                                                **   tell), 1 taken (the UI's to go on), 2 walking on to it */
extern unsigned take_n;                         /* (I_TAKE's thing, its item: 0 none) */
unsigned char take_rest (void);                 /* (Part 4: things.c) A count to take up, walked over to first, taken
                                                **   as it gets there (IR_NONE: none) */
unsigned char __fastcall__ fumbled (unsigned char k);  /* (dq0w.c's part 25) A grope while engaged, a thing of kind
                                                **   k handled: 1 if it failed (its words in the hub's text) */
extern unsigned char noinit;                    /* (dq0 -I: no grope's test while engaged: the tests') */
unsigned char __fastcall__ in_reach (unsigned n);
unsigned char __fastcall__ carry (unsigned n, unsigned char hand_first);
/* The reach (HOUSE.md's, 2026-10-26: part 10's): how readily a thing comes to hand, by where it is, its time owed as a
** handling's done, the character's next spell's (dq0w.c's busy, pextra) */
unsigned __fastcall__ handy (unsigned i);       /* (Whether thing i is handy: 0, or the thing it's in that isn't open
                                                **   to the hand) */
void __fastcall__ reach_from (unsigned n);      /* (Thing n got out, to hand) */
void __fastcall__ reach_to (unsigned c);        /* (Container c reached into: a thing put in it, looked for in it) */
unsigned __fastcall__ reach_done (unsigned char said);  /* (What's owed added, its time back; said, a pack taken off
                                                **   for it told; then none owed) */
extern unsigned rtime, roff, pextra;            /* (What's owed: its time, tenths; the pack taken off for it) */
extern unsigned char rsteps, rbody;             /* (Its steps, a fight's rounds: its levels; one, on the body) */
extern unsigned char held_k[2], held_h[2];      /* (The main RAM's, things.c's: what the character holds, the main
                                                **   hand's kind first, and the hands they're in, IH_*: held_kinds') */
void held_kinds (void);
unsigned char __fastcall__ shand_of (unsigned char s);  /* (The hand that holds the thing that strikes: the hub's
                                                **   pc_shand) */
void light_now (void);
void __fastcall__ go_to (unsigned x, unsigned y);

/* The things' words (kinds.c's said, in the catalogue's bank: % the thing's name, # the other's) */
enum {
    SA_HOLD, SA_HOLD2, SA_HELD, SA_HANDS, SA_STOW, SA_NO_PACK, SA_THERE, SA_PUT_IN, SA_PUT_ON, SA_HANG, SA_NO_HANG,
    SA_NO_SLOT, SA_NO_BELT, SA_NOT_HOLDER, SA_RUN_OUT, SA_WONT_GO, SA_ITSELF, SA_PUT_OUT, SA_NO_ROOM, SA_TOO_HEAVY,
    SA_TAKE, SA_NOT_IN, SA_DON, SA_WORN, SA_WEAR_ONE, SA_NO_WEAR, SA_DOFF, SA_NOT_WORN, SA_LIGHT, SA_LIT,
    SA_NO_FLAME, SA_NO_LIGHT, SA_DOUSE, SA_NOT_LIT, SA_NONE_TO_LIGHT, SA_NONE_LIT, SA_NO_SUCH, SA_NO_OTHER, SA_REACH,
    SA_WHERE, SA_WHAT, SA_IN_HAND, SA_IN_PACK, SA_FIXED, SA_BURNT, SA_NO_OIL, SA_SHUT, SA_COUNT
};
extern const char* const said[SA_COUNT];

/* ---- The lights burning (slice 4.1's: DUNGEON.md's light, HOUSE.md's burning): how each kind burns, and what renews
** it (kinds.c's burns, in the catalogue's bank: a light's whole life and its fuels; a fuel's measure and what it is) */
struct burn {
    unsigned char   kind;                       /* (KI_*) */
    unsigned char   life;                       /* A light's whole life (a fuel's, what a whole one holds): tens of
                                                **   minutes of burning */
    unsigned char   fuels;                      /* FU_*: a light's, those that renew it (0: none, it burns away); a
                                                **   fuel's, what it is */
};
#define NBURNS          6
#define FU_OIL          0x01
#define FU_PITCH        0x02
#define BURN_LOW        10                      /* Burning low: its last ten minutes (the house rule's) */
extern const struct burn burns[NBURNS];
const struct burn* __fastcall__ burn_of (unsigned char k);  /* How kind k burns (a copy), or 0: never (a glow), or no
                                                **   fuel; the catalogue's bank left in the window */
unsigned char fill (void);                      /* (Part 27, its room) The hub's ireq, IO_FILL's: a light renewed */
void burns_now (void);                          /* (Part 27: dq0w.c) The lights burnt down by the minutes gone */
extern unsigned long burn_due;                  /*   (when the next minute's due: the main loop's) */
extern unsigned char burn_ask;                  /*   (or at once: a light's change) */
#define CLOCK_256       (*(unsigned*) ((unsigned char*) &HUB->now + 1))   /* (The clock in 25.6 seconds, 16 bits: its
                                                **   tenths' middle bytes, no long division: ipage.c's stamps) */
