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

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[4];
