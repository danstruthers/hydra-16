/*
** hub.c - (dq0w) the hub's code, the UI's file (games/dq0/hub.c): every task of the game's compiles the same.
*/

#define HUB_ZERO                                /* (hub0.h's) */
#define HUB_MAP_ASM                             /* (hub_map bank.s's: cmd1's, 2026-11-26) */
#define HUB_ATTACHED                            /* (Attached, not made: no hub_create, no hub_spawn) */
#define HUB_DWRITE                              /* (The denizens' events: the world's to write, its bodies') */
#define HUB_NO_ISEND                            /* (Intents taken, none sent: the world's room, 2026-10-27; */
#define HUB_NO_NEXT                             /*   events written, none read; */
#define HUB_NO_NOW                              /*   the clock its own) */
#define HUB_ATTACH_IN   "SW_hub_attach_"        /* (hub_attach in part hub_attach, the start-up's: dq0w.c's s_begin) */
#include "../dq0/hub.c"
