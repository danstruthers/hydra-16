/*
** dq0p.c - Dingus Quest Zero's senses: a task of its own, started by the UI (dq0p SEG ROLE: the hub's segment and its
** role there, hub.h), that looks about for the character as the world asks (the hub's look): the squares it sees
** now marked in the dungeon's chunks (view.c: in front of it and to its sides, a clear line in plan and in height,
** its sight's range, lit, its night vision and ultravision), and what it perceives recorded in the hub for
** the UI to word (sight.c), the things it sees among it (things.c: the item store's records read, the world's).
** (The denizens will see through it too, each through its own senses.)  It waits on its doorbell (costing nothing)
** between looks.
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "sight.h"
#include "things.h"
#include "light.h"

static unsigned char sees;                      /* (The denizens seen changed: the look's reply's ch) */

/* A look about for the character, from what the hub has of it (its square, its facing, its light, its senses; the
** squares it walked to get there), the denizens it sees with it */
static void look (void)
{
    unsigned char face, light, sight, dark, sn, first, own, wd, wn;
    unsigned x, y, wx, wy;

    hub_map (0);
    face = HUB->pc_face;
    own = HUB->pc_light;
    light = HUB->pc_light * (HUB->pcsum.nightvision ? HUB->pcsum.nightvision : 1);  /* (Night vision: further) */
    eye = HUB->pcsum.sight;                     /* (What it makes out at a distance: sight.c's, things.c's) */
    sight = HUB->pcsum.cond & C_BLINDED ? 0xFF : eye;
    dark = HUB->pcsum.darkvision / 5;           /* (Ultravision's reach, in squares) */
    sn = HUB->pc_stratum;
    x = HUB->pc_x;
    y = HUB->pc_y;
    first = HUB->look.first;
    wx = HUB->look.wx;
    wy = HUB->look.wy;
    wd = HUB->look.wd;
    wn = HUB->look.wn;
    sync_strata ();                             /* (The strata's records, the world bank's) */
    if (wn) {
        felt (sn, wx, wy, wd, wn);              /* (The squares walked, remembered: felt in the dark) */
    }
    things_lamps (sn, x, y);                    /* (The lit things lying about: a torch put down) */
    view (sn, x, y, face, light, sight, dark);
    first = perceive (sn, x, y, face, own, first);
    sees = dens_seen ();                        /* (The denizens it sees with it: the world's bodies) */
    hub_map (0);
    HUB->look.first = first;
}

/* A number, an argument's (atoi's, without the C library's ctype) */
static unsigned char __fastcall__ num (const char* s)
{
    unsigned char n = 0;

    while (*s >= '0' && *s <= '9') {
        n = n * 10 + *s++ - '0';
    }
    return n;
}

int main (int argc, char* argv[])
{
    if (argc < 3 || hub_attach (num (argv[1]), num (argv[2]))) {
        return 1;
    }
    chunks_init (ASK_SENSES);
    light_init ();
    for (;;) {
        hub_map (0);
        hy_sem_acquire (HUB->sem[T_SENSES]);    /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        if (hub_posted (&HUB->look)) {
            light_events ();                    /* (The world's lights and doors changed: the light layer's out of
                                                **   date near them) */
            if (HUB->look.kind == LK_DENS) {
                sees = dens_seen ();            /* (Only the denizens seen, looked at again: their bodies changed) */
            } else {
                look ();
            }
            hub_map (0);
            HUB->look.ch = sees;
            hub_reply (&HUB->look);
        }
    }
    return 0;
}
