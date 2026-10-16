/*
** sight.h - (dq0w) what the character perceives: sight.c
*/

void __fastcall__ perceive (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char first);                       /* The hub's record made (view () first) */
extern unsigned char eye;                       /* The character's sight (the summary's, 0-9), as the look began */
