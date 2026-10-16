/*
** dq0m.c - Dingus Quest Zero's maker: a task of its own, started by the UI (dq0m SEG ROLE: the hub's segment and
** its role there, hub.h), that grows the dungeon (DUNGEON.md) for whoever asks, the world or the denizens (each its
** request in the hub: world.h's struct mreq): a stratum's plan and its links down (plan.c: its record set by the
** asker), a chunk (make.c, built.c), into the hub's shared banks, where they read it; and it works the fixtures it
** grew, the portals and the traps, for the world (work.c).  It waits on its doorbell
** (costing nothing) between requests.  Its growers are overlays (built.c, pass.c, caves.c, plan.c ...), read as it
** starts from the game's lib folder (its third argument, the UI's) into banks of its own.
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
#define OVLS    6                               /* Its overlays: built.c, pass.c, caves.c, plan.c, fixtures.c; and
                                                **   the swapped parts' images (../dq0/swap.inc: map.c's, changes.c's,
                                                **   work.c's) */
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

/* A change made in its chunk, if it's in the cache */
static void __fastcall__ cached (const struct mreq* q)
{
    struct stratum* s = &strata[q->sn];
    unsigned cx = (q->cx - s->x0) / CW, cy = (q->cy - s->y0) / CW;
    unsigned char k;

    hub_map (0);
    for (k = 0; k < NCSLOTS; ++k) {
        if (HUB->cslot[k].used && HUB->cslot[k].sn == q->sn && HUB->cslot[k].cx == cx && HUB->cslot[k].cy == cy) {
            change_cached (k, (q->cx - s->x0) % CW, (q->cy - s->y0) % CW, q->tile, q->elev, q->link);
            break;
        }
    }
}

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
        place_pieces (q.sn);                    /* (The theme's pieces in it: theme.h) */
        hub_map (0);
        memcpy (&HUB->strata[q.sn], &strata[q.sn], sizeof strata[0]);
        break;
    case M_CHUNK:
        make_chunk (q.sn, q.cx, q.cy);
        break;
    case M_SAVE:
        changes_save ();
        break;
    case M_RESTORE:                             /* (The records back, each stratum made grown again: the same plan) */
        changes_restore ();
        for (q.sn = 0; q.sn < MAXSTRATA; ++q.sn) {
            if (strata[q.sn].made) {
                plan_stratum (q.sn);
                place_pieces (q.sn);
            }
        }
        break;
    case M_CHANGE:                              /* (Kept, and made in the chunk if it's in the cache) */
        change_keep (q.sn, q.cx, q.cy, q.tile, q.elev, q.link);
        cached (&q);
        break;
    case M_WORK:                                /* (A portal or a trap worked: the reply's back in the request) */
        work (&q);
        hub_map (0);
        memcpy (&HUB->mreq[i], &q, sizeof q);
        break;
    }
    hub_map (0);
    HUB->mreq[i].state = REQ_DONE;
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
            if (HUB->mreq[i].state == REQ_POSTED) {
                serve (i);
            }
        }
        if (HUB->map_state == REQ_POSTED) {     /* (The UI's asked for the map) */
            memcpy (strata, HUB->strata, sizeof strata);
            draw_map ();
            HUB->map_state = REQ_DONE;
        }
    }
    return 0;
}
