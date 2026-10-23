/*
** dens.c - (dq0p) the denizens the character sees (DUNGEON.md: perception as layers): their bodies the world's (the
** hub's actors), which of them are seen the senses' to decide (those on squares the character sees now, the look's
** marks), added to what it perceives (the senses' record, SIGHT_BANK: SEE_DENS at most), after each look and as the
** world asks (the hub's look, LK_DENS: their bodies changed).
*/

#include <string.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "sight.h"

extern struct actor sact;                       /* (A body, copied out of the hub's header: sense.c's) */
static unsigned sig[NACTORS];                   /* (Each body as it was seen: its record, what it's doing) */

unsigned char dens_seen (void)
{
    static unsigned char k, n, ch;
    static unsigned char* p;
    static unsigned s;
    static struct actor* a;

    for (k = n = ch = 0, a = HUB->actor; k < NACTORS; ++k, ++a) {
        hub_map (0);
        s = 0;
        if (a->state) {                         /* (Its body copied out only if it's one: most aren't) */
            memcpy (&sact, a, sizeof sact);
            if (sact.sn == HUB->pc_stratum && n < SEE_DENS && (p = at (sact.sn, sact.x, sact.y)) != 0 && (*p & T_VIS)) {
                hub_map (SIGHT_BANK);
                memcpy (&SIGHT->den[n++], &sact.x, sizeof SIGHT->den[0]);
                s = 0x8000U | sact.doing << 8 | sact.den;
            }
        }
        if (s != sig[k]) {
            sig[k] = s;
            ch = 1;
        }
    }
    hub_map (SIGHT_BANK);
    SIGHT->ndens = n;
    hub_map (0);
    return ch;
}
