/*
** view.h - (dq0w) what the character sees: view.c
*/

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark);   /* The squares seen now marked (light: its reach in squares, night
                                                **   vision's already; dark: ultravision's; sight 0xFF: blinded) */
void __fastcall__ grid (unsigned char sn, unsigned gx, unsigned gy, unsigned char w, unsigned char lm);   /* The
                                                **   look's grid (LOOK_BANK's) made: w squares each way from gx, gy (lm:
                                                **   the light layer's, its lamps' lines: the cache's chunks alone) */
void __fastcall__ felt (unsigned char sn, unsigned x, unsigned y, unsigned char d, unsigned char n);   /* A walk's
                                                **   squares remembered (from x, y its first way d, n of them) */

/* The lit things lying about near the character (things.c's, before each view): the chunks round them reached with
** the view's, in its order (their light the light layer's, light.c's) */
#define NLAMPS          4
struct lamp { unsigned x, y; unsigned char r; };
extern struct lamp lamp[NLAMPS];
extern unsigned char nlamps;
