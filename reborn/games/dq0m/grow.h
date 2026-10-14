/*
** grow.h - (dq0m) a purpose-built chunk grown in the main RAM: built.c's (overlay 1) and pass.c's (overlay 2), and
** their stubs (bank.s)
*/

/* A style's proportions (DUNGEON.md): what its growers make, and how often; each a chance in so many (its own die's
** sides, as noted), or weights.  The maker's records (make.c's styles), till the theme's data gives them */
struct style {
    unsigned char   lit;                        /* A room lit by its own light: in 8, one fewer each stratum down */
    unsigned char   zones, zones3;              /* A long room (6 squares) in zones: in 10; three (9 squares): in 3 */
    unsigned char   slope;                      /* A zone's floor sloping: in 4 */
    unsigned char   ceil[6];                    /* Ceilings by kind (C_FLAT ... C_ROUGH): weights, summing to 20 */
    unsigned char   doors;                      /* A way out with a door at all (else a doorway): in 20 */
    unsigned char   d_open, d_locked, d_barred; /* A door open, locked, barred: in 20 */
    unsigned char   d_hidden, d_magic;          /* A door hidden: in 15; of those, hidden by magic: in 4 */
    unsigned char   m_wood, m_ironbound;        /* A door's wood, iron-bound (the rest iron): in 10 */
    unsigned char   chambers;                   /* (Caves) chambers along the tunnels: up to, less one */
    unsigned char   wide;                       /* (Caves) a tunnel three squares wide: in 3 */
    unsigned char   glow;                       /* (Caves) a chamber with a glow: in 10 */
};
extern const struct style* SP;                  /* The chunk's stratum's (make.c's) */

extern struct chunk* L;
extern const struct edges* E;
extern unsigned char mx, my;

unsigned char __fastcall__ join (unsigned char a, unsigned char b);    /* (Stubs: overlay 2's o_join ...) */
unsigned char crossings (void);
void __fastcall__ grow_built (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 1's) */
void __fastcall__ grow_caves (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 3's) */

/* changes.c: what's changed, and what's been seen, kept */
void changes_init (void);
int changes_save (void);                        /* The records into the save's folder: 0, or -1 */
int changes_restore (void);                     /* And back */
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
