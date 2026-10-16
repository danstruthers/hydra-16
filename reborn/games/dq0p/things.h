/*
** things.h - (dq0p) the things: things.c
*/

int __fastcall__ things_init (const char* dir);  /* The catalogue into the hub (dir's dq0p.ovl), the store emptied, a
                                                **   game carried on its read: 0, or -1 */
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
unsigned __fastcall__ item_new (unsigned char kind, unsigned char where, unsigned char sn, unsigned x, unsigned y,
    unsigned char qty);                         /* A record taken: its number, or 0xFFFF */
void __fastcall__ things_seen (struct sight* s, unsigned char sn, unsigned x, unsigned y, unsigned char face);
void __fastcall__ things_lamps (unsigned char sn, unsigned x, unsigned y);   /* The lit things lying about near
                                                **   (x, y), into view.c's lamps */
unsigned char things_serve (void);              /* The hub's ireq answered: 1 if what lies about changed */

/* The things' words (kinds.c's said, in the catalogue's bank: % the thing's name, # the other's) */
enum {
    SA_HOLD, SA_HOLD2, SA_HELD, SA_HANDS, SA_STOW, SA_NO_PACK, SA_THERE, SA_PUT_IN, SA_PUT_ON, SA_HANG, SA_NO_HANG,
    SA_NO_SLOT, SA_NO_BELT, SA_NOT_HOLDER, SA_RUN_OUT, SA_WONT_GO, SA_ITSELF, SA_PUT_OUT, SA_NO_ROOM, SA_TOO_HEAVY,
    SA_TAKE, SA_NOT_IN, SA_DON, SA_WORN, SA_WEAR_ONE, SA_NO_WEAR, SA_DOFF, SA_NOT_WORN, SA_LIGHT, SA_LIT,
    SA_NO_FLAME, SA_NO_LIGHT, SA_DOUSE, SA_NOT_LIT, SA_NONE_TO_LIGHT, SA_NONE_LIT, SA_NO_SUCH, SA_NO_OTHER, SA_REACH,
    SA_WHERE, SA_WHAT, SA_IN_HAND, SA_IN_PACK, SA_FIXED, SA_COUNT
};
extern const char* const said[SA_COUNT];
