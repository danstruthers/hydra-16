/*
** things.h - (dq0p) the things perceived: things.c (the item store's records read, the world's: ../dq0w/things.c)
*/

struct item* __fastcall__ item_at (unsigned i); /* Item i's record, its bank in the window (items.s: the world's) */
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
void __fastcall__ things_seen (struct sight* s, unsigned char sn, unsigned x, unsigned y, unsigned char face);
void __fastcall__ things_lamps (unsigned char sn, unsigned x, unsigned y);   /* The lit things lying about near
                                                **   (x, y), into view.c's lamps */
void things_stocked (void);                     /* The chunks the maker's stocked put down (the world asked) */
