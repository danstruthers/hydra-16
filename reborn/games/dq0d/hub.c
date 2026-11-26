/*
** hub.c - (dq0d) the hub's code, the UI's file (games/dq0/hub.c): every task of the game's compiles the same.
*/

#define HUB_MAP_ASM                             /* (hub_map bank.s's: cmd1's, 2026-11-26) */
#define HUB_ATTACHED                            /* (Attached, not made: no hub_create, no hub_spawn) */
#define HUB_DENQ                                /* (The denizens' events: theirs to hear, the world's to write) */
#define HUB_NO_SEND                             /* (Nor intents sent this way, nor events: its intents are the hub's
                                                **   di, its own) */
#define HUB_ATTACH_IN   "SW_start_"             /* (hub_attach in part start, as it starts: dq0d.c's s_start; the main
                                                **   RAM's room, room8, 2026-11-24) */
#include "../dq0/hub.c"
