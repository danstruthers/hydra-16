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
extern unsigned char v_nom, v_def;              /* (dq0p.c's: a look one way's far part, 1, no chunk made for it past what
                                                **   a glance reaches (the user's, 2026-11-18: its rest told as the maker's
                                                **   made them); its rest, 2, but its own (the hub's way_want); v_def, one
                                                **   wanted not made) */
unsigned char __fastcall__ v_may (unsigned x, unsigned y);    /* (Square x, y's chunk, not made, one the look may
                                                **   make now: 1; else 0, kept for its rest) */
unsigned char __fastcall__ way_stop (unsigned x, unsigned y);  /* (dq0p.c's: 2, a chunk round square x, y not made,
                                                **   one the look may not make: a far part's walks stop there) */
#define VP_ASK          8                       /* (v_way's: a look one way through the closed door ahead, asked,
                                                **   LK_PEEK: the user's, 2026-11-14) */
#define VP_GRILLE       2                       /* (v_way's more, view.c's, a look one way's at a closed door on the
                                                **   square ahead: looked through its little window, its grille; */
#define VP_PEEP         4                       /*   through its spy-hole, its cover on this side; */
#define VP_COVER        6                       /*   its spy-hole's cover on the far side: nothing seen through it.
                                                **   The user's, 2026-11-14: way.c's to tell) */
void __fastcall__ way_seen (unsigned char sn, unsigned char face);  /* A look one way's own record (WAY_SEEN),
                                                **   after the look's, and its denizens' (part way_record's, way.c); its near
                                                **   part's (WAY_NEAR: dq0p.c's wnear), from the last whole look's,
                                                **   what the far part may change held back */
void __fastcall__ felt (unsigned char sn, unsigned x, unsigned y, unsigned char d, unsigned char n);   /* A walk's
                                                **   squares remembered (from x, y its first way d, n of them) */
#define RUN_AT          0x0FF8                  /* (In the senses' bank, before a look one way's: its passage's ways'
                                                **   runs' bearings, sixteenths, by their first headings: view.c's
                                                **   runs, as a look begins, for sight.c's ways) */
#define RUNS            ((unsigned char*) (HUB_WINDOW + RUN_AT))
void __fastcall__ kept (unsigned char what);    /* The memory of what the view saw kept (each chunk's: part kept's,
                                                **   after the view's): what 2; 1, its passage's runs first, sight.c's
                                                **   (a look one way's memory after its reply: dq0p.c's kept_owed) */

/* The lit things lying about near the character (things.c's, before each view): the chunks round them reached with
** the view's, in its order (their light the light layer's, light.c's) */
#define NLAMPS          4
struct lamp { unsigned x, y; unsigned char r; };
extern struct lamp lamp[NLAMPS];
extern unsigned char nlamps;
