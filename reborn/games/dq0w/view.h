/*
** view.h - (dq0w) what the character sees: view.c
*/

void __fastcall__ view (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char sight);                       /* The squares seen now marked (sight 0xFF: blinded) */
