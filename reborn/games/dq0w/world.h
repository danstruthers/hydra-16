/*
** world.h - Dingus Quest Zero's dungeon, as its tasks keep it (DUNGEON.md): strata, each grown whole in outline as
** anyone first comes to it (its plan: a grid of regions, 8 by 8 chunks each), then made a chunk (32 by 32 squares)
** at a time as anyone comes near, from the stratum's seed and the chunk's place.  The maker (games/dq0m, a task of
** its own: plan.c, make.c, built.c) grows the plans and makes the chunks, into the hub's shared segment (the plans
** two to a bank, the chunks' cache three to a bank), so the world and the denizens read them (chunk.c's at ()),
** asking the maker for a chunk that isn't there yet.  The strata's records, the cache's table and the requests are
** in the hub's header (hub.h); each task keeps a copy of the strata's records of its own (strata[], sync_strata)
** for when a chunk is in the window.
**
** The frame: every stratum's squares are in one frame, 16 bits each way, so that a way down comes out under where
** it went in.  A stratum's bounds are a rectangle in it (x0, y0, w, h); its regions and chunks are counted from its
** x0, y0.  Elevations: a square's floor in feet above its stratum's datum (signed char); the datum, feet above the
** way in (the top stratum's 0, deeper ones less).
**
** The window: the world's code sees a bank at a time at $8000: the hub's header (hub_map (0)), a stratum's plan, or
** a chunk.  at () puts a square's chunk in the window (making it if it must) and gives the square; what it gives is
** good till the next call that moves the window.
*/

#ifndef _WORLD_H
#define _WORLD_H

#include "../dq0/level.h"

#define CW              32                      /* A chunk's squares, each way */
#define CW_BITS         5                       /*   (CW's: a square's chunk by a shift, cc65 dividing slowly) */
#define INSET(o)        ((o) + (((o) & (CW - 1)) < 2 ? 2 - ((o) & (CW - 1)) : ((o) & (CW - 1)) > CW - 3 ?                          (CW - 3) - ((o) & (CW - 1)) : 0))     /* A link's square (its offset from the stratum's x0, or
                                                **   y0: a variable, it's read more than once) two squares or more
                                                **   from its chunk's edges: room for the room it comes out in, inside
                                                **   the chunk, with the edge's ways */
#define RCH             8                       /* A region's chunks, each way */
#define RW              (CW * RCH)              /* A region's squares, each way (256: 1,280 feet) */
#define MAXSTRATA       8
#define PLAN_BANKS      4                       /* The hub's banks for the plans (two to a bank), */
#define CACHE_BANKS     8                       /*   and for the chunks (three to a bank) */
#define NCSLOTS         (CACHE_BANKS * 3)
#define PLAN_BANK(n)    (1 + ((n) >> 1))        /* (Banks of the hub's segment) */
#define PLAN_AT(n)      ((struct region*) (0x8000 + ((n) & 1) * 0x1000))
#define PLAN_MAX        (0x1000 / sizeof (struct region))   /* Regions in a plan, at most */
#define CACHE_BANK(k)   (1 + PLAN_BANKS + (k) / 3)
#define CHUNK_AT(k)     ((struct chunk*) (0x8000 + ((k) % 3) * CHUNK_SIZE))

/* Squares: the low five bits a kind; T_SEEN: the character has seen it; T_VIS: it sees it now; T_HOLE: a shaft in
** the ceiling above */
enum { T_ROCK, T_FLOOR, T_WALL, T_DOOR, T_CORR, T_STEPS, T_WINDOW, T_UP, T_DOWN, T_PIT };
#define T_SEEN          0x80
#define T_VIS           0x40
#define T_MARK          0x40                    /* (A grower's, the same bit: a square reached) */
#define T_HOLE          0x20
#define TILE(t)         ((t) & 0x1F)
#define T_PASS(t)       (TILE (t) != T_ROCK && TILE (t) != T_WALL && TILE (t) != T_WINDOW && TILE (t) != T_PIT)
#define T_INSIDE(t)     (TILE (t) == T_FLOOR || TILE (t) == T_STEPS || TILE (t) == T_UP || TILE (t) == T_DOWN || \
                         TILE (t) == T_PIT)
#define T_CLEAR(t)      (TILE (t) != T_ROCK && TILE (t) != T_WALL)  /* The eye sees past it (a window too) */
#define STEP_MAX        2                       /* The rise or fall a step can take, in feet, */
#define STAIR_MAX       4                       /*   and on steps */

/* ---- A chunk: its squares and its areas (rooms, their zones), its openings; squares in the chunk's own
** coordinates, 0-31 */

#define NROOMS          6
#define NZONES          12
#define NOPEN           6
struct zone {                                   /* A rectangle of a room's floor, a plane, and its ceiling */
    unsigned char   x, y, w, h;
    unsigned char   room;
    signed char     z;                          /* Its northwest square's floor (feet above the datum) */
    signed char     gx, gy;                     /* Its slope: quarter-feet a square, east and south */
    unsigned char   ceil, cmid, cwall;          /* Its ceiling: C_*, its height at the middle and at the walls */
};
struct room {
    unsigned char   x, y, w, h;                 /* Its floor (its walls round it) */
    unsigned char   flags;                      /* R_* */
    unsigned char   kind;                       /* What it was built for (U_*: DUNGEON.md's built to a plan; a
                                                **   cave's chamber U_CAVE), what the theme calls it by */
    unsigned char   z0, nz;                     /* Its zones */
};
#define R_VISITED       0x01
#define R_LIT           0x02
#define R_MURDER        0x04                    /* (A gatehouse's: murder holes in its vault) */
/* A room's use, its kind (the theme's words: its rooms' eight, then its uses'); a cave's chamber U_CAVE (kept in 8
** bytes, struct room: the tasks index their rooms by a shift) */
enum { U_CHAMBER, U_HALL, U_CELL, U_VAULT, U_CRYPT, U_GALLERY, U_CAVE, U_STORE, U_KITCHEN, U_PANTRY, U_BARRACKS,
    U_ARMOURY, U_CHAPEL, U_QUARTERS, U_SOLAR, U_WELL, U_CISTERN, U_GATEHOUSE, U_GUARDROOM, U_WORKSHOP, NUSES };
struct opening {                                /* An opening that isn't a door */
    unsigned char   kind;                       /* O_* (level.h) */
    unsigned char   x, y;
    unsigned char   sill;                       /* Feet above the floor (a window's) */
    unsigned char   link;                       /* The stratum's link (struct stratum's) it is, or 0xFF */
};
/* A portal (DUNGEON.md): a fixture between two areas, on its square (a door's T_DOOR) */
#define NPORTALS        16
struct portal {
    unsigned char   x, y;
    unsigned char   kind;                       /* PK_* */
    unsigned char   state;                      /* PS_* */
    unsigned char   lock;                       /* A mechanical lock's complexity, 1-9 (0: none) */
    unsigned char   mat;                        /* PM_* (a window's: its shutters', else its bars') */
    unsigned char   tough;                      /* Its body: the net harm it takes before it fails (each blow's
                                                **   impact less its material's resistance), by its material, its
                                                **   size, its thickness and its state (part_body's) */
    unsigned char   bar;                        /* The way (DIR_*) from it to the side it's barred from (a
                                                **   portcullis's: the side its winch is on) */
    unsigned char   parts;                      /* A window's (PP_*: any of them; none, an empty frame), a
                                                **   portcullis's bars */
    /* Its make (the maker's, with its own dice: fixtures.c's), what an observer judges (examine the door) */
    unsigned char   size;                       /* Its opening, in feet: its width (the low four bits), its height */
    unsigned char   thick;                      /* Its leaf's thickness, in inches (a door's, shutters', bars') */
    unsigned char   qual;                       /* Its state as made and as the years have left it: 0 rotten ...
                                                **   4 sound ... 9 new */
    unsigned char   feat;                       /* PF_* */
    unsigned char   harm;                       /* The harm it's taken so far (of tough; a window's, its part
                                                **   that's outermost now: its shutters closed, its glass, its bars) */
};
enum { PK_DOOR, PK_WINDOW, PK_PORTCULLIS, PK_DRAWBRIDGE, PK_GATE, PK_GRATE, PK_HATCH, PK_SLIT };
#define PP_GLASS        0x01                    /* (Sight through, not missiles; till it's broken) */
#define PP_SHUTTERS     0x02                    /* (Open or closed: PS_OPEN; barred, as a door is) */
#define PP_BARS         0x04                    /* (Sight and missiles through, not movement) */
#define PP_KNOWN        0x80                    /* (A door the character has opened or shut: a walk out of a room
                                                **   opens it again, as it goes; any other it must open, asked) */
#define PS_OPEN         0x01
#define PS_LOCKED       0x02
#define PS_BARRED       0x04
#define PS_WARDED       0x08                    /* (A magical lock: with the powers) */
#define PS_BROKEN       0x10                    /* Its lock or its bar burst (it won't lock or bar again) */
#define PS_HIDDEN       0x20                    /* A secret door: like the wall round it, till a search finds it */
#define PS_MAGIC        0x40                    /* (Hidden by magic: found only by magical means, the referee's) */
#define PS_WRECKED      0x80                    /* Broken down (its body gone): open for good, it won't shut (a
                                                **   portcullis: its bars bent or broken apart, jammed, a gap in it) */
enum { PM_WOOD, PM_IRONBOUND, PM_IRON, PM_STONE };
#define PF_GRILLE       0x01                    /* A door's features: a little window in it, a grille across it; */
#define PF_PEEP         0x02                    /*   a spy-hole, its cover on the bar's side; */
#define PF_BRACKETS     0x04                    /*   brackets for a bar, on the bar's side (barred: its bar in them,
                                                **   an item of its own, items.h's IW_PORTAL); */
#define PF_STUDS        0x08                    /*   studded with iron (a wooden one's); */
#define PF_HINGES       0x10                    /*   its hinges on the bar's side (else the far side's); */
#define PF_DOUBLE       0x20                    /*   two leaves (a wide one's) */
#define PORTAL_STATE    0xFE                    /* (A change record's link for a portal's state: tile its state,
                                                **   elev its parts) */
#define PORTAL_HARM     0xFB                    /* (And for the harm it's taken: tile it) */
#define TRAP_STATE      0xFC                    /* (And for a trap's: tile its state) */
/* A window (T_WINDOW, PK_WINDOW) and an arrow slit (T_WINDOW, PK_SLIT, always open) are seen through (the senses'),
** but a window's closed shutters; a portcullis (T_DOOR, PK_PORTCULLIS) is seen through, up or down.  A window's glass
** and bars are items of their own (a pane, bars: items.h's IW_PORTAL, on its square), as a door's bar is (on the
** square on its bar's side) */

/* A trap (DUNGEON.md): a hazard on its square, the floor (or a door) it looks like till it's found; an open pit's
** square T_PIT, in plain sight (a covered one's too, once it's given way).  What it does is the referee's (its
** harm); the world sets it off, as it's walked on (a plate's, a wire's) or its door's opened */
#define NTRAPS          6
struct trap {
    unsigned char   x, y;
    unsigned char   kind;                       /* TK_* */
    unsigned char   trigger;                    /* TT_* */
    unsigned char   state;                      /* TS_* */
    unsigned char   hide;                       /* Hidden: how well, 0-9 (a search's steps) */
    unsigned char   power;                      /* A pit's depth in feet; else how hard it strikes, 1-9 */
};
enum { TK_PIT, TK_COVERED, TK_SPIKED, TK_DEADFALL, TK_BLADE, TK_DARTS, TK_GAS, TK_ALARM };   /* A pit (open,
                                                **   covered, spiked and covered), a deadfall, a scything blade,
                                                **   darts or needles, gas, an alarm */
enum { TT_PLATE, TT_WIRE, TT_DOOR };            /* Set off: a plate trodden, a wire tripped, its door opened */
#define TS_SPRUNG       0x01
#define TS_DISARMED     0x02
#define TS_BROKEN       0x04
#define TS_ASSESSED     0x08                    /* Its mechanism made out (a disarm's first check: never deadly) */
#define TS_KEEN         0x10                    /*   and made out well (a critical success: the disarm easier) */
#define TS_HIDDEN       0x20                    /* (Found by a search, as a secret door is, */
#define TS_MAGIC        0x40                    /*   or hidden by magic: by magical means only) */
#define TS_SET(s)       (!((s) & (TS_SPRUNG | TS_DISARMED | TS_BROKEN)))   /* (It would go off) */

/* The materials (rs.h's MAT_*) of a room's floor, walls and ceiling (struct chunk's mat[r]), and of its passages'
** (mat[NROOMS]: their floors, the rock round them and over them: the rock's, dug or cut) */
enum { MP_FLOOR, MP_WALL, MP_CEIL };

struct chunk {
    unsigned char   sn;                         /* Its stratum */
    unsigned        cx, cy;                     /* Its place, in chunks from the stratum's x0, y0 */
    unsigned char   nrooms, nzones, nopen;
    struct room     room[NROOMS];
    struct zone     zone[NZONES];
    struct opening  open[NOPEN];
    unsigned char   nportals;
    struct portal   portal[NPORTALS];
    unsigned char   tile[CW][CW];
    signed char     elev[CW][CW];
    unsigned char   mat[NROOMS + 1][3];         /* Its rooms' materials (MP_*), and its passages' */
    unsigned char   ntraps;
    struct trap     trap[NTRAPS];
    unsigned char   pceil;                      /* Its passages' ceiling, feet above their floor (a built one's: its
                                                **   small rooms' too, the builders' one height) */
};
#define CHUNK_SIZE      0x0AA0                  /* Three to a bank */
typedef char chunk_fits[sizeof (struct chunk) <= CHUNK_SIZE ? 1 : -1];

/* ---- A stratum, and its plan */

/* A region of the plan: what it is, the lie of the rock at its northwest corner, and the ways across its north and
** west edges (a square along the edge, 0-255; or NO_WAY) */
struct region {
    unsigned char   kind;                       /* RG_* */
    signed char     z;                          /* Feet above the datum, halved */
    unsigned char   flags;
    unsigned char   n, w;                       /* Its north edge's way across, its west edge's */
    unsigned char   spare;
};
enum { RG_ROCK, RG_HALLS };
#define NO_WAY          0xFF

/* A link: a way between strata (a stairway, a ladder, a shaft), its two ends' squares the same in the frame */
#define NLINKS          8
struct link {
    unsigned char   kind;                       /* O_STAIRS_DOWN, O_LADDER_DOWN ... (this stratum's end of it) */
    unsigned char   to;                         /* The other stratum */
    unsigned        x, y;                       /* Its square */
    unsigned char   dug;                        /* Made by someone in the dungeon (a shaft dug): the grower leaves its
                                                **   square alone (the change records have it) */
};

struct stratum {
    unsigned char   made, n;
    unsigned char   style, age, name;           /* ST_*; centuries; the theme's name */
    int             datum;
    unsigned long   seed;
    unsigned        x0, y0, w, h;               /* Its bounds in the frame, in squares */
    unsigned char   rw, rh;                     /* Its regions, each way */
    unsigned char   plan;                       /* Its plan's bank (the world's own) */
    unsigned char   nlinks;
    struct link     link[NLINKS];
};
extern struct stratum strata[MAXSTRATA];         /* (This task's copy of the hub's) */
void sync_strata (void);                        /* The hub's copied (the hub's header in the window after) */

/* The chunks' cache: what each slot holds (the hub's) */
struct cslot {
    unsigned char   used, sn;
    unsigned        cx, cy;
    unsigned        stamp;                      /* When it was last found */
};

/* A request to the maker (one for each task that asks: the world's, the denizens') */
struct mreq {
    volatile unsigned char state;               /* REQ_* (hub.h) */
    unsigned char   kind;                       /* M_* */
    unsigned char   sn;
    unsigned        cx, cy;                     /* M_CHUNK's chunk; M_CHANGE's square */
    unsigned char   tile;                       /* M_CHANGE's: what the square becomes, */
    signed char     elev;                       /*   its floor, */
    unsigned char   link;                       /*   and the link it is (an opening), or 0xFF */
    unsigned        fx, fy;                     /* M_WORK's: where the character stands (or came from), and the
                                                **   reply's where it is after (a pit's: where it climbs out to); */
    unsigned        t;                          /*   the reply's tenths it took (and it was out for), */
    unsigned char   climb, res;                 /*   a pit's climb out (feet, or 0), 1 if nothing was there */
};
enum { M_PLAN, M_CHUNK, M_CHANGE, M_SAVE, M_RESTORE, M_WORK };   /* A stratum's plan and links down (its record
                                                **   set), a chunk, a square changed (and kept: the maker's records);
                                                **   the records written to the save, read back (the plans grown
                                                **   again); a portal or a trap worked (dq0m/work.c: cx, cy its square,
                                                **   or NO_SQ, about the character; tile what's done, hub.h's PA_* or
                                                **   W_STEP; elev the way from fx, fy to it) */
#define NO_SQ           0xFFFF
#define W_STEP          0xFF                    /* (M_WORK's: a trap stepped onto, from fx, fy) */
enum { ASK_WORLD, ASK_DENIZENS, ASK_SENSES, ASK_UI, NASK };

/* ---- plan.c (the maker's): a stratum grown whole, in outline */
void __fastcall__ plan_stratum (unsigned char n);   /* strata[n] (its bounds, style, seed, links already set) */
void __fastcall__ links_down (unsigned char n);     /* Stratum n's own links down (its plan grown) */
struct edges;
void __fastcall__ chunk_edges (unsigned char sn, unsigned cx, unsigned cy, struct edges* e);
                                                /* A chunk's edges, from the plan (built.c's struct edges) */

/* ---- make.c (the maker's): a chunk made into the cache */
unsigned char __fastcall__ make_chunk (unsigned char sn, unsigned cx, unsigned cy);    /* Its slot */

/* ---- chunk.c (the world's, the denizens'): the chunks reached */
void __fastcall__ chunks_init (unsigned char me);   /* me: ASK_WORLD ... */
void __fastcall__ ask_maker (unsigned char kind, unsigned char sn, unsigned cx, unsigned cy);  /* And wait */
unsigned char* __fastcall__ at (unsigned char sn, unsigned x, unsigned y);  /* Its square (NULL: outside the
                                                **   stratum), its chunk in the window */
extern struct chunk* C;                         /* at ()'s chunk */
extern unsigned char cx_, cy_;                  /* at ()'s square in it */
signed char __fastcall__ elev_at (unsigned char sn, unsigned x, unsigned y);
unsigned char __fastcall__ step_ok (unsigned char sn, unsigned x, unsigned y, unsigned char d);   /* Walkable to d */
unsigned char __fastcall__ room_of (unsigned char sn, unsigned x, unsigned y);  /* Its room in C (0xFF: none) */
unsigned char __fastcall__ zone_of (unsigned char sn, unsigned x, unsigned y);  /* Its zone in C (0xFF: none) */
struct portal* __fastcall__ portal_at (unsigned char x, unsigned char y);   /* C's portal on its square x, y */
unsigned char __fastcall__ wall_tile (struct room* p, unsigned char d, unsigned char k);  /* A wall's kth square
                                                **   (wall_x, wall_y: the chunk's), but its corners: 1, or 0 past its end */
extern unsigned char wall_x, wall_y;

/* ---- built.c: a purpose-built chunk grown (the chunk in the window, its edges' ways and the ground given) */
struct edges {
    unsigned char   way[4];                     /* N, E, S, W: a square along the edge, or NO_WAY */
    int             ground[4];                  /* The ground at the chunk's corners: NW, NE, SW, SE */
    unsigned char   x0, y0, x1, y1;             /* The squares in the stratum's bounds (the rest rock) */
    unsigned char   nland;                      /* Links in the chunk: their squares, and their strata.links */
    unsigned char   lx[4], ly[4], ln[4];
};
void __fastcall__ grow_built (struct chunk* c, const struct edges* e, unsigned long seed);

#endif
