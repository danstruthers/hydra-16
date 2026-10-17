/*
** dq0p.c - Dingus Quest Zero's senses: a task of its own, started by the UI (dq0p SEG ROLE: the hub's segment and its
** role there, hub.h), that looks about for the character as the world asks (the hub's look): the squares it sees
** now marked in the dungeon's chunks (view.c: in front of it and to its sides, a clear line in plan and in height,
** its sight's range, lit, its night vision and ultravision), and what it perceives recorded in the hub for
** the UI to word (sight.c), the things it sees among it (things.c: the item store's records read, the world's), the
** denizens it sees (dens.c: the world's bodies), the lights it sees far off (eyes.c).  The senses answer for every
** actor (DUNGEON.md: perception as layers): the light layer theirs to work out (light.c), each actor's eyes theirs to
** keep (eyes.c), each denizen's senses theirs to answer, its mind asking (sense.c: the hub's dsense); and the memory of
** what the character has seen theirs (the chunks' seen marks, the walk's squares felt, R_VISITED).  A look's own work
** is in swapped parts (bank.s, LIBDIR/dq0p.ovl), the answers to the denizens in the main RAM.  It waits on its
** doorbell (costing nothing) between requests.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "view.h"
#include "sight.h"
#include "things.h"
#include "light.h"
#include "eyes.h"
void sense_actor (void);                        /* (sense.c's: the hub's dsense answered) */

static unsigned char sees;                      /* (The denizens seen changed: the look's reply's ch) */

/* A look about for the character, from what the hub has of it (its square, its facing, its light, its senses; the
** squares it walked to get there), the denizens it sees with it */
static unsigned char face, light, sight, dark, sn, own, range;
static unsigned x, y;

/* The character as the hub has it */
static void pc (void)
{
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
    range = sight == 0xFF ? 0xFF : 4 + 2 * (sight > 9 ? 9 : sight);   /* (view.c's) */
}

extern unsigned char gw;                        /* (give_way's, below) */
void give_way (void);
static void look (void)
{
    unsigned char first, wd, wn;
    unsigned wx, wy;

    gw = 0;                                     /* (Its giving way: GIVE_WAY ticks a look at most) */
    pc ();
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
    glows (sn, x, y, face, range, own);         /* (Its eyes adapted, the lights it sees far off: eyes.c's) */
    give_way ();
    first = perceive (sn, x, y, face, own, first);
    sees = dens_seen ();                        /* (The denizens it sees with it: the world's bodies) */
    hub_map (0);
    HUB->look.first = first;
}

/* The look giving way to the UI's command in hand at the real paces (the hub's ui_cmd: its answer, the world's, the
** UI's words; half a second a look at most; at the turns pace the game waits for the look as it is), a tick at a
** time; but not to one waiting for the world's answer that waits for the look
** (a request of the things but what's carried, the tools, a tool's wear; the world's own, wreq: dq0w.c's waiting);
** the hub's header in the window after */
#define GIVE_WAY        100
unsigned char gw;                               /* (Reset as a look begins: its ticks given) */
static unsigned char gk;
void give_way (void)
{
    hub_map (0);
    while (HUB->tpr && HUB->ui_cmd && gw < GIVE_WAY && HUB->wreq.state != REQ_POSTED && (HUB->ireq.state != REQ_POSTED ||
           (gk = HUB->ireq.kind) == IO_LIST || gk == IO_TOOLS || gk == IO_WEAR)) {
        ++gw;
        hy_sleep_ticks (1);
        hub_map (0);
    }
}

/* The swapped parts' images read into banks of its own (dir/dq0p.ovl: dq0p.cfg's OV1 ..., 8K each): 0, or -1 */
#define OVBANKS         2
extern unsigned char ovbank[OVBANKS];
extern char _SWAP_START__[];
static int __fastcall__ ovl_load (const char* dir)
{
    unsigned char* b = (unsigned char*) _SWAP_START__;  /* (Through the parts' area: none in it yet) */
    unsigned off;
    int fd, n;

    strcpy ((char*) b, dir);
    strcat ((char*) b, "/dq0p.ovl");
    if ((fd = open ((char*) b, O_RDONLY)) < 0 || (n = hy_banks_alloc (OVBANKS)) < 0) {
        return -1;
    }
    ovbank[0] = n;                              /* (In a run: n, n + 1 ...) */
    ovbank[1] = n + 1;
    for (off = 0; (n = read (fd, b, 512)) > 0; off += n) {    /* (The file the build made: OVBANKS banks at most,
                                                **   64K at most, off back at 0 only as it ends) */
        hy_bank (ovbank[off >> 13]);
        memcpy ((void*) (0x8000 + (off & 0x1FFF)), b, n);
    }
    close (fd);
    return 0;
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
    if (argc < 4 || ovl_load (argv[3]) || hub_attach (num (argv[1]), num (argv[2]))) {
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
        if (hub_posted (&HUB->dsense)) {      /* (A denizen's senses, its mind asking: sense.c's) */
            light_events ();
            sense_actor ();
            hub_reply (&HUB->dsense);
            hub_map (0);
        }
        if (hub_posted (&HUB->look)) {
            light_events ();                    /* (The world's lights and doors changed: the light layer's out of
                                                **   date near them) */
            if (HUB->look.kind == LK_DENS) {
                sees = dens_seen ();            /* (Only the denizens seen, looked at again: their bodies changed) */
            } else if (HUB->look.kind == LK_EYES) {
                pc ();                          /* (Only its eyes, as time's gone: the lights far off) */
                sync_strata ();
                glows (sn, x, y, face, range, own);
                hub_map (SIGHT_BANK);
                memcpy (&SIGHT->nglows, &G, sizeof G);
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
