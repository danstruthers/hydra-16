/*
** hub.c - (dq0p) the hub's code, the UI's file (games/dq0/hub.c): every task of the game's compiles the same.
*/

#define HUB_ATTACHED                            /* (Attached, not made: no hub_create, no hub_spawn) */
#define HUB_UX                                  /* (Its banks' U and $00 the sound layer's too: sound.c's) */
#define HUB_NO_QUEUES                           /* (Nor the queues: the senses are asked in the hub's header) */
#define HUB_ATTACH_P3                           /* (hub_attach in part 3, as it starts: dq0p.c's main, its stub) */
#include "../dq0/hub.c"
