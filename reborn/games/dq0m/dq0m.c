/*
** dq0m.c - Dingus Quest Zero's maker: a task of its own, started by the UI (dq0m SEG ROLE: the hub's segment and
** its role there, hub.h), that grows the dungeon (DUNGEON.md) for whoever asks, the world or the denizens (each its
** request in the hub: world.h's struct mreq): a stratum's plan and its links down (plan.c: its record set by the
** asker), a chunk (make.c, built.c), into the hub's shared banks, where they read it.  It waits on its doorbell
** (costing nothing) between requests.
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"

struct stratum strata[MAXSTRATA];               /* (This task's copy of the hub's) */

/* Request i answered: the strata's records copied first (the hub's are the truth), and back if they changed */
static void __fastcall__ serve (unsigned char i)
{
    static struct mreq q;

    memcpy (&q, &HUB->mreq[i], sizeof q);
    memcpy (strata, HUB->strata, sizeof strata);
    switch (q.kind) {
    case M_PLAN:
        plan_stratum (q.sn);
        links_down (q.sn);
        hub_map (0);
        memcpy (&HUB->strata[q.sn], &strata[q.sn], sizeof strata[0]);
        break;
    case M_CHUNK:
        make_chunk (q.sn, q.cx, q.cy);
        break;
    }
    hub_map (0);
    HUB->mreq[i].state = REQ_DONE;
}

int main (int argc, char* argv[])
{
    unsigned char i;

    if (argc < 3 || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    for (;;) {
        hy_sem_acquire (HUB->sem[T_MAKER]);     /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        for (i = 0; i < NASK; ++i) {
            if (HUB->mreq[i].state == REQ_POSTED) {
                serve (i);
            }
        }
    }
    return 0;
}
