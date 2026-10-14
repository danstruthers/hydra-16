/*
** grow.h - (dq0m) a purpose-built chunk grown in the main RAM: built.c's (overlay 1) and pass.c's (overlay 2), and
** their stubs (bank.s)
*/

extern struct chunk* L;
extern const struct edges* E;
extern unsigned char mx, my;

unsigned char __fastcall__ join (unsigned char a, unsigned char b);    /* (Stubs: overlay 2's o_join ...) */
unsigned char crossings (void);
void __fastcall__ grow_built (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 1's) */

/* changes.c: what's changed, and what's been seen, kept */
void changes_init (void);
void __fastcall__ change_keep (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link);
void __fastcall__ changes_over (struct chunk* k, unsigned x, unsigned y);
void __fastcall__ change_cached (unsigned char s, unsigned char lx, unsigned char ly, unsigned char tile,
    signed char elev, unsigned char link);
void __fastcall__ seen_keep (unsigned char s);
void __fastcall__ seen_over (struct chunk* k);

/* map.c */
void draw_map (void);

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[4];
