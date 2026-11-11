/*
** grow.h - (dq0m) a purpose-built chunk grown in the main RAM: built.c's (overlay 1) and pass.c's (overlay 2), and
** their stubs (bank.s)
*/

/* A style's proportions (DUNGEON.md): what its growers make, and how often; each a chance in so many (its own die's
** sides, as noted), or weights.  The maker's records (uses.c's styles), till the theme's data gives them */
struct style {
    unsigned char   lit;                        /* A room built with sconces (lit only if it's kept: fixtures.c's):
                                                **   in 8, one fewer each stratum down */
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
    unsigned char   h_lo, h_span;               /*   how well: a difficulty (steps) from h_lo, two more for each of
                                                **   h_span grades */
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
    unsigned char   slant;                      /* (Not the styles': the chunk's) its passages on the diagonal (the
                                                **   user's, 2026-10-31: ways off the compass, step 3; uses.c's, built.c's) */
};
extern const struct style* SP;                  /* The chunk's stratum's (make.c's: sty) */
extern struct edges me;                         /* (make.c's: the chunk's being made) */
extern struct style sty;
extern unsigned char cstyle;                    /* (The chunk's style: its stratum's, or a mixed stratum's natural
                                                **   part's ST_NATURAL: make.c's) */

/* place.c (dr the main RAM's, the rest overlay 9's, bank.s's stubs): the maker's dice, keyed, and its placers
** (ENGINE-CRITIQUE.md's §5).  A purpose (DP_*):
** what a number decides, its key's; each one's key the one its placer always had (the doors', pass.c's; the
** fixtures', fixtures.c's; the sconces' the portals' make's own), so the dungeons are as they were.  A new purpose
** from DP_SEEDED on (its key the stratum's seed too).  dkey (purpose, where) for chunk L, then dr (n) as often as
** wanted: 0 to n - 1 */
enum { DP_DOOR, DP_MAT, DP_WINDOW, DP_PORTC, DP_TRAP, DP_DTRAP, DP_MAKE, DP_FORT, DP_SCONCE, DP_SEEDED, DP_NONE = 0xFF };
void __fastcall__ dkey (unsigned char p, unsigned b);
unsigned char __fastcall__ dr (unsigned char n);
enum { RS_GROW, RS_STOCK };                     /* A stream's (dice.h's rseed, a layout's rolls one after another):
                                                **   chunk L's grower's, its stock's */
unsigned long __fastcall__ rstream (unsigned char p);

/* A placer (place.c's placers[]: a row each): its stage (PG_*), its sites (SI_*: a room, a room's side, a door: its
** dice keyed by the room, room * 4 + side, the door's square), its dice's purpose (DP_NONE none), its odds (a chance
** in sides of the style's byte at odds; sides 0, always), what a site mustn't be (a room's flags R_*, a door's state
** PS_*), the use a room must have (U_ANY any), its qualifiers (PQ_*), where in its site it goes (WH_*: its hook's),
** what it makes (0: its hook's choice), and its hook, called with the site (place.c's PR the row) */
struct placer {
    unsigned char   stage, site, purpose, odds, sides, clear, use, flags, where, kind;
    void __fastcall__ (*put) (unsigned char at);
};
enum { PG_WALLS, PG_TRAPS, PG_STOCK, PG_SCONCES, PG_END };  /* Fixtures.c's portals placed, its traps; stock.c's
                                                **   marks; sconce.c's sconces */
enum { SI_ROOM, SI_WALL, SI_DOOR };
#define U_ANY           0xFF
#define PQ_DEFENCE      0x01                    /* (Always in a fortified stratum's defences: defended's) */
#define PQ_WATER        0x02                    /* (Only where the chunk has water: uses.c's uwater) */
enum { WH_WALL, WH_DOOR, WH_FLOOR, WH_MIDDLE, WH_WEST, WH_BYWALL };  /* A room's wall a passage runs beside; a
                                                **   door's square; its floor, room round it to walk; its middle; by its
                                                **   west wall, halfway along; its floor beside a wall */
extern const struct placer* PR;                 /* (The row being placed: its hook's) */
void __fastcall__ place (unsigned char stage);
unsigned char __fastcall__ defended (unsigned char r);  /* Room r one of a fortified stratum's defences */
void __fastcall__ put_window (unsigned char at);    /* (The hooks, bank.s's stubs: fixtures.c's, overlay 5's) */
void __fastcall__ put_portc (unsigned char at);
void __fastcall__ put_trap (unsigned char at);
void __fastcall__ put_dtrap (unsigned char at);
void __fastcall__ put_mark (unsigned char at);      /* (stock.c's, overlay 7's) */
void __fastcall__ put_sconce (unsigned char at);    /* (sconce.c's s_put_sconce, part 3's: its stub) */

/* uses.c (overlay 7's): a built stratum's plan of uses (DUNGEON.md: built to a plan) */
unsigned char __fastcall__ uses (unsigned char step, unsigned char arg);
enum { US_STYLE, US_PLAN, US_FLOOR, US_THING, US_SIGN, US_UNDER };   /* The style arg's copied into sty (make.c's,
                                                **   for each chunk); the chunk's rooms given their uses (built.c's,
                                                **   sorted); their floors and ceilings by them (once they're zoned);
                                                **   what room arg & 7 has lying about (its die arg >> 3: a kind, or
                                                **   0), its use's mark (place.c's row a kitchen's well), the mark
                                                **   room arg's use under the chunk's older layer left (stock.c's) */
extern unsigned char ufort, uwater;             /* (The chunk's stratum fortified (SP_FORT), the chunk's water) */
extern unsigned char ulayer, uruin;             /* (The chunk's layer of its history, theme.h's SG_*; a ruin after
                                                **   it, EN_*, or 0xFF: US_PLAN's) */
unsigned char __fastcall__ layer_at (unsigned char sn, unsigned cx, unsigned cy);  /* Which of stratum sn's layers
                                                **   chunk (cx, cy) is now (SG_*: stratagen's; uses.c's, a stub) */
extern const char* libpath;                     /* (The game's lib folder: dq0m.c's, its third argument) */

extern struct chunk* L;
extern const struct edges* E;
extern unsigned char mx, my;

unsigned char __fastcall__ join (unsigned char a, unsigned char b);    /* (Stubs: overlay 2's o_join ...) */
unsigned char crossings (void);
void __fastcall__ grow_built (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 1's) */
void __fastcall__ grow_caves (struct chunk* c, const struct edges* e, unsigned long seed);  /* (Overlay 3's) */
void slants (void);                             /* A tunnel's zigzag cut (overlay 1's, caves.c's: the caves', and a built
                                                **   stratum's passages on the diagonal) */
void __fastcall__ water_plan (unsigned char n);    /* A stratum's water traced over its plan (overlay 5's: plan.c,
                                                **   slice 4.9's) */
void __fastcall__ wet_chunk (struct chunk* c, const struct edges* e);  /* Its water (overlay 6's: slice 4.9's) */
void __fastcall__ air_pocket (struct chunk* c, const struct edges* e);  /* Its air (overlay 3's: 2026-10-31) */
void __fastcall__ sky (struct chunk* c, unsigned char n);  /* What shows the sky (overlay 9's: sky.c): chunk c's skylights
                                                **   and windows on the surface, or (c 0) stratum n's shafts */
struct region* __fastcall__ rg (unsigned char sn, unsigned char rx, unsigned char ry);  /* (plan.c's, the main RAM's) */
void __fastcall__ fixtures (struct chunk* c);   /* Its materials, windows, portcullises, traps (overlay 5's) */
void __fastcall__ sconces (struct chunk* c, unsigned x0, unsigned y0);  /* Its rooms' sconces, as it's first stocked
                                                **   (part 3's, sconce.c: after stock's, in its list) */
struct sthing* __fastcall__ put_down (unsigned char kind, unsigned char sn, unsigned x, unsigned y,
    unsigned char qty, unsigned char hide);    /* (stock.c's: a thing listed in sk) */
void __fastcall__ place_pieces (unsigned char n);   /* The theme's pieces in stratum n given their places, as its plan
                                                **   is grown (overlay 4's: plan.c) */
unsigned char __fastcall__ cavern (unsigned char m);    /* A great cavern's part (overlay 8's: caves.c, slice 4.10's) */

/* changes.c: the change records (world.h's: the world's) laid over a chunk made again; what's been seen, kept */
void changes_init (void);
int changes_save (void);                        /* The seen maps into the save's folder: 0, or -1 (part 2's) */
int changes_restore (void);                     /* And back (part 2's) */
unsigned char __fastcall__ changes_over (struct chunk* k, unsigned x, unsigned y);     /* 1: it's been stocked */
void __fastcall__ stock (const struct chunk* c, unsigned x0, unsigned y0);              /* stock.c's: listed in sk */
extern struct stock sk;
void __fastcall__ seen_keep (unsigned char s);
void __fastcall__ seen_over (struct chunk* k);
void __fastcall__ pl_get (unsigned char sn, unsigned idx, struct region* to);      /* A big plan's region (plan.c's: */
void __fastcall__ pl_put (unsigned char sn, unsigned idx, const struct region* from);  /*   changes.c's pages) */
void __fastcall__ pl_new (unsigned char sn);
void pl_flush (void);

/* map.c */
void draw_map (void);                           /* (Part 1's: ../dq0/swap.inc) */

/* bank.s */
void __fastcall__ setbank (unsigned char b);
unsigned char getbank (void);
void __fastcall__ bread (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ bwrite (unsigned char bank, void* to, const void* from, unsigned n);
extern unsigned char ovbank[9];

/* make.c: a hub's bank reached from an overlay (its bytes copied to the main RAM, or from it, or set) */
void __fastcall__ hub_get (unsigned char bank, const void* from, void* to, unsigned n);
void __fastcall__ hub_put (unsigned char bank, void* to, const void* from, unsigned n);
void __fastcall__ hub_set (unsigned char bank, void* to, unsigned char c, unsigned n);
extern unsigned char detail_bank;               /* (changes.c's: the regions' details' cache, plan.c's) */
