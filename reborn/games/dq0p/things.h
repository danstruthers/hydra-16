/*
** things.h - (dq0p) the things: things.c
*/

void things_init (void);                        /* The catalogue into the hub (and, a new game, the store emptied) */
struct item* __fastcall__ item_at (unsigned i); /* Item i's record, its bank in the window */
unsigned __fastcall__ item_new (unsigned char kind, unsigned char where, unsigned char sn, unsigned x, unsigned y,
    unsigned char qty);                         /* A record taken: its number, or 0xFFFF */
void __fastcall__ things_seen (struct sight* s, unsigned char sn, unsigned x, unsigned y, unsigned char face);
unsigned char things_serve (void);              /* The hub's ireq answered: 1 if what lies about changed */
