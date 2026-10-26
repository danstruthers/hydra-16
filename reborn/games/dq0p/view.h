/*
** view.h - (dq0w) what the character sees: view.c
*/

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark);   /* The squares seen now marked (light: its reach in squares, night
                                                **   vision's already; dark: ultravision's; sight 0xFF: blinded) */
void __fastcall__ grid (unsigned char sn, unsigned gx, unsigned gy, unsigned char w, unsigned char lm);   /* The
                                                **   look's grid (LOOK_BANK's) made: w squares each way from gx, gy (lm:
                                                **   the light layer's, its lamps' lines: the cache's chunks alone) */
extern unsigned char v_way;                     /* A look one way (HOUSE.md's looking one way: the hub's LK_WAY,
                                                **   dq0p.c's): the view's reach twice a glance's in the quarter
                                                **   ahead (WAY_R at most, the grid's: 2 WAY_R + 1 across), the
                                                **   things near looked for as far, what's made out twice as far,
                                                **   its own record made (way.c's) */
#define WAY_R           31
void __fastcall__ way_seen (unsigned char sn, unsigned char face);  /* A look one way's own record (WAY_SEEN),
                                                **   after the look's, and its denizens' (part 3's, way.c) */
void __fastcall__ felt (unsigned char sn, unsigned x, unsigned y, unsigned char d, unsigned char n);   /* A walk's
                                                **   squares remembered (from x, y its first way d, n of them) */
void kept (void);                               /* The memory of what the view saw kept (each chunk's: part 4's,
                                                **   after the view's) */

/* The lit things lying about near the character (things.c's, before each view): the chunks round them reached with
** the view's, in its order (their light the light layer's, light.c's) */
#define NLAMPS          4
struct lamp { unsigned x, y; unsigned char r; };
extern struct lamp lamp[NLAMPS];
extern unsigned char nlamps;
