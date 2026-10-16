/*
** dq0p.c - Dingus Quest Zero's senses: a task of its own, started by the UI (dq0p SEG ROLE: the hub's segment and its
** role there, hub.h), that looks about for the character as the world asks (the hub's look): the squares it sees
** now marked in the dungeon's chunks (view.c: in front of it and to its sides, a clear line in plan and in height,
** its sight's range, lit, its night vision and ultravision), and what it perceives recorded in the hub for
** the UI to word (sight.c); and the things (things.c: the catalogue, the items, moved and listed as the UI asks).
** (The denizens will see through it too, each through its own senses.)  It waits on its
** doorbell (costing nothing) between looks; a look under way, it answers what the UI asks that wants none between
** the look's rows (look_pause).
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "sight.h"
#include "things.h"

/* A look about for the character, from what the hub has of it (its square, its facing, its light, its senses); again:
** where it already is (a thing taken up or put down) */
static void __fastcall__ look (unsigned char again)
{
    unsigned char face, light, sight, dark, sn, first, own;
    unsigned x, y;

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
    first = again ? 0 : HUB->look.first;
    sync_strata ();                             /* (The strata's records, the world bank's) */
    things_lamps (sn, x, y);                    /* (The lit things lying about: a torch put down) */
    view (sn, x, y, face, light, sight, dark);
    perceive (sn, x, y, face, own, first);
}

/* Between the rows of a look (view.c's): what the UI asks that wants no look and no squares seen (what's carried,
** the tools, a tool's wear) answered at once, so the UI isn't kept waiting for the look */
void look_pause (void)
{
    unsigned char op;

    hub_map (0);
    op = HUB->ireq.kind;
    if (hub_posted (&HUB->ireq) && (op == IO_LIST || op == IO_TOOLS || op == IO_WEAR)) {
        things_serve ();
        hub_reply (&HUB->ireq);
    }
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
    if (argc < 4 || things_init (argv[3])) {    /* (Its catalogue: the lib folder's dq0p.ovl) */
        return 1;
    }
    for (;;) {
        hub_map (0);
        hy_sem_acquire (HUB->sem[T_SENSES]);    /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        if (hub_posted (&HUB->ireq)) {
            if (things_serve ()) {
                look (1);
            }
            hub_reply (&HUB->ireq);
        }
        hub_map (0);
        if (hub_posted (&HUB->look)) {
            look (0);
            hub_reply (&HUB->look);
        }
    }
    return 0;
}
