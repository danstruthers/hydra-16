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
    unsigned char   things;                     /* A thing in a room (two tries): in 20 */
    unsigned char   hidden;                     /*   concealed: in 20 */
    unsigned char   h_lo, h_span;               /*   how well: a difficulty from h_lo, h_span of them (0-9) */
    unsigned char   kinds[8];                   /* What's lying about (items.h's KI_*) */
    unsigned char   hide_kinds[4];              /* What's concealed */
    unsigned char   f_earth, p_earth;           /* (fixtures.c) A room's floor bare earth (else flagged; a cave's
                                                **   stone), a chunk's passages': in 20, one more each 4 centuries old */
    unsigned char   w_built, c_built;           /* A room's walls masonry, its ceiling (else the rock): in 20 */
    unsigned char   windows, slits;             /* A room's side with a window, where a passage runs beside it: in
                                                **   20; of those, an arrow slit: in 20 */
    unsigned char   w_glass, w_shut, w_bars;    /* A window's glass, shutters, bars: in 4 each */
    unsigned char   portc;                      /* A door a portcullis instead: in 20 */
    unsigned char   traps, t_pit;               /* A trap in a room: in 20; a pit (open or covered): in 4 */
    unsigned char   t_hidden, t_magic;          /* A trap hidden: in 10; of those, by magic: in 10 */
    unsigned char   d_trap;                     /* A closed door trapped: in 20 */
};
extern const struct style* SP;                  /* The chunk's stratum's (make.c's) */

extern struct chunk* L;
extern const struct edges* E;
extern unsigned char mx, my;

unsigned char __fastcall__ join (unsigned char a, unsigned char b);    /* (Stubs: overlay 2's o_join ...) */
unsigned char crossings (void);
void __fastcall__ grow_built (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 1's) */
void __fastcall__ grow_caves (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 3's) */
void __fastcall__ fixtures (struct chunk* c);   /* Its materials, windows, portcullises, traps (overlay 5's) */
void __fastcall__ place_pieces (unsigned char n);   /* The theme's pieces in stratum n given their places, as its plan
                                                **   is grown (overlay 4's: plan.c) */

/* changes.c: what's changed, and what's been seen, kept */
void changes_init (void);
int changes_save (void);                        /* The records into the save's folder: 0, or -1 */
int changes_restore (void);                     /* And back */
void __fastcall__ change_keep (unsigned char sn, unsigned x, unsigned y, unsigned char tile, signed char elev,
    unsigned char link);
unsigned char __fastcall__ changes_over (struct chunk* k, unsigned x, unsigned y);     /* 1: it's been stocked */
#define STOCKED         0xFD                    /* (A change record's link: the chunk at x, y stocked) */
void __fastcall__ stock (const struct chunk* c, unsigned x0, unsigned y0);              /* stock.c's */
void __fastcall__ change_cached (unsigned char s, unsigned char lx, unsigned char ly, unsigned char tile,
    signed char elev, unsigned char link);
void __fastcall__ seen_keep (unsigned char s);
void __fastcall__ seen_over (struct chunk* k);

/* map.c */
void draw_map (void);

/* work.c: a portal or a trap worked, for the world (M_WORK: its reply written into q); a portal's leaf's body (its
** material PM_*, its size, its thickness, its state: fixtures.c's too) */
void __fastcall__ work (struct mreq* q);
unsigned char __fastcall__ part_body (unsigned char mat, unsigned char size, unsigned char thick, unsigned char qual);

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[5];

/* make.c: a hub's bank reached from an overlay (its bytes copied to the main RAM, or from it, or set) */
void __fastcall__ hub_get (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ hub_put (unsigned char bank, void* to, const void* from, unsigned n);
void __fastcall__ hub_set (unsigned char bank, void* to, unsigned char c, unsigned n);
extern unsigned char detail_bank;               /* (changes.c's: the regions' details' cache, plan.c's) */
