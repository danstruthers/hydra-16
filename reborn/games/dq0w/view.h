/*
** view.h - (dq0w) what the character sees: view.c
*/

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight, unsigned char dark);   /* The squares seen now marked (light: its reach in squares, night
                                                **   vision's already; dark: ultravision's; sight 0xFF: blinded) */
