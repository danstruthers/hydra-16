/*
** sight.h - (dq0w) what the character perceives, and the map's window: sight.c
*/

void __fastcall__ perceive (unsigned char sn, unsigned x, unsigned y, unsigned char face, unsigned char light,
    unsigned char first);                       /* The hub's record made (view () first) */
void __fastcall__ draw_map (unsigned char sn, unsigned x, unsigned y);  /* The map's window into the hub */
