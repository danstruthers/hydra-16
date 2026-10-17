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

/* ---- The swapped parts' (bank.s's stubs) */
unsigned char serve (void);                     /* (Part 10) The hub's ireq answered (but its state): 1 if what lies
                                                **   about has changed (the world looks again) */
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
