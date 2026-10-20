/*
** hub.c - (dq0d) the hub's code, the UI's file (games/dq0/hub.c): every task of the game's compiles the same.
*/

#define HUB_ATTACHED                            /* (Attached, not made: no hub_create, no hub_spawn) */
#define HUB_DENQ                                /* (The denizens' events: theirs to hear, the world's to write) */
#define HUB_NO_SEND                             /* (Nor intents sent this way, nor events: its intents are the hub's
                                                **   di, its own) */
#include "../dq0/hub.c"
