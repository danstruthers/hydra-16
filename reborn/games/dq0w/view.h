/*
** view.h - (dq0w) what the character sees: view.c
*/

void __fastcall__ view (struct level* l, unsigned char x, unsigned char y, unsigned char face, unsigned char light,
    unsigned char sight);                       /* The tiles seen now marked (sight 0xFF: blinded) */
