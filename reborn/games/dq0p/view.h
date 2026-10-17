/*
** view.h - (dq0w) what the character sees: view.c
*/

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark);   /* The squares seen now marked (light: its reach in squares, night
                                                **   vision's already; dark: ultravision's; sight 0xFF: blinded) */

/* The lit things lying about near the character (things.c's, before each view): each lights the squares round it
** within its reach that it has a clear line to */
#define NLAMPS          4
struct lamp { unsigned x, y; unsigned char r; };
extern struct lamp lamp[NLAMPS];
extern unsigned char nlamps;
