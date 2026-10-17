/*
** dq0m.c - Dingus Quest Zero's maker: a task of its own, started by the UI (dq0m SEG ROLE: the hub's segment and
** its role there, hub.h), that grows the dungeon (DUNGEON.md) for whoever asks, the world or the denizens (each its
** request in the hub: world.h's struct mreq): a stratum's plan and its links down (plan.c: its record set by the
** asker), a chunk (make.c, built.c), into the hub's shared banks, where they read it.  Generation only: the same
** seed always the same chunk, the change records (the world's: dq0w/store.c) laid over it as it's made again
** (changes.c), and nothing kept of its own but the squares seen as the cache makes way for a chunk (and the map,
** drawn for the UI from the cache).  It waits on its doorbell (costing nothing) between requests.  Its growers are
** overlays (built.c, pass.c, caves.c, plan.c ...), read as it starts from the game's lib folder (its third argument,
** the UI's) into banks of its own.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

struct stratum strata[MAXSTRATA];               /* (This task's copy of the hub's) */
#define OVLS    7                               /* Its overlays: built.c, pass.c, caves.c, plan.c, fixtures.c; the
                                                **   swapped parts' images (../dq0/swap.inc: map.c's, changes.c's);
                                                **   uses.c */
extern char _SWAP_START__[];

/* The overlays: dir/dq0m.ovl, an 8K bank each (dq0m.cfg's OV1 ...; the file cut after the last one used: make.js),
** read into banks of its own: 0, or -1 */
static int __fastcall__ ovl_load (const char* dir)
{
    static char path[64];
    unsigned char* b = (unsigned char*) _SWAP_START__;  /* (256 bytes of the swapped parts' area: none in it yet) */
    int fd, first, n;
    unsigned char k;
    unsigned off;

    strcpy (path, dir);
    strcat (path, "/dq0m.ovl");
    if ((fd = open (path, O_RDONLY)) < 0 || (first = hy_banks_alloc (OVLS)) < 0) {
        return -1;
    }
    for (k = 0; k < OVLS; ++k) {
        ovbank[k] = first + k;
        for (off = 0; off < 0x2000; off += 256) {
            n = read (fd, b, 256);
            if (n <= 0) {
                break;
            }
            bwrite (first + k, (void*) (0x8000 + off), b, n);
        }
    }
    close (fd);
    return 0;
}

/* Request i answered: the strata's records copied first (the world bank's are the truth), and back if they changed */
static void __fastcall__ serve (unsigned char i)
{
    static struct mreq q;

    memcpy (&q, &HUB->mreq[i], sizeof q);
    hub_get (WORLD_BANK, WREC->strata, strata, sizeof strata);
    switch (q.kind) {
    case M_PLAN:
        plan_stratum (q.sn);
        links_down (q.sn);
        place_pieces (q.sn);                    /* (The theme's pieces in it: theme.h) */
        hub_put (WORLD_BANK, &WREC->strata[q.sn], &strata[q.sn], sizeof strata[0]);
        break;
    case M_CHUNK:
        make_chunk (q.sn, q.cx, q.cy);
        break;
    case M_SAVE:
        changes_save ();
        break;
    case M_RESTORE:                             /* (The seen maps back, each stratum made grown again: the same
                                                **   plan) */
        changes_restore ();
        for (q.sn = 0; q.sn < MAXSTRATA; ++q.sn) {
            if (strata[q.sn].made) {
                plan_stratum (q.sn);
                place_pieces (q.sn);
            }
        }
        break;
    }
    hub_reply (&HUB->mreq[i]);
}

int main (int argc, char* argv[])
{
    unsigned char i;

    if (argc < 4 || ovl_load (argv[3]) || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    changes_init ();
    hub_set (THEME_BANK, THEME_PLACE, 0xFF, NPIECES * sizeof (struct tplace));     /* (No piece placed yet) */
    for (;;) {
        hub_map (0);                            /* (The hub's header: its own banks may be in the window) */
        hy_sem_acquire (HUB->sem[T_MAKER]);     /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        for (i = 0; i < NASK; ++i) {
            if (hub_posted (&HUB->mreq[i])) {
                serve (i);
            }
        }
        if (hub_posted (&HUB->mapq)) {          /* (The UI's asked for the map) */
            hub_get (WORLD_BANK, WREC->strata, strata, sizeof strata);
            draw_map ();
            hub_reply (&HUB->mapq);
        }
    }
    return 0;
}
