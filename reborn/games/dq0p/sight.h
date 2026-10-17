/*
** sight.h - (dq0w) what the character perceives: sight.c
*/

unsigned char __fastcall__ perceive (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char first);                       /* The hub's record made (view () first; first | LF_ARRIVE: by the
                                                **   senses' memory of the room, R_VISITED, marked): the first time */
unsigned char dens_seen (void);                 /* The denizens seen added to it (dens.c): 1 if one's come into sight,
                                                **   gone from it, or doing otherwise */
extern unsigned char eye;                       /* The character's sight (the summary's, 0-9), as the look began */
