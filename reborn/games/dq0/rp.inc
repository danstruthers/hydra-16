/*
** rp.inc - the record store's files (rp.h; the paged ROM's RP_NAME, RP_OPEN, RP_MAKE, RP_DONE): a store's region's
** file in the save folder, its versions read and written.  Every store that keeps a file compiles it, a call each:
**   rp_name (path, pre, sn, rx, ry)  path made the region's file's name (pre its two letters: "rg" ...; preSXXYY.dat,
**                                    a stratum past 15's preSSXXYY.dat): 0, no folder (the hub's savedir "")
**   rp_open (path, slots)            its newest valid version: its fd past its header (rf.h: its n records), or -1
**   rp_make (path, slots)            a new version begun (its fd, past its header's room), or -1 (the game not begun)
**   rp_done (fd, n)                  its header written (n records, the kind's next stamp), it closed
** The hub's header in the window after each.
**
** Before it's included: RP_STATIC (static, for a swapped part's own copy; or nothing); RP_KIND, the kind's count (a
** struct rpk in the main RAM: each kind's own, kept between calls); the code's segments (a swapped part's: its BSS too,
** the part's own).
*/

#include "../dq0/rp.h"

static struct rpf rf;

static int __fastcall__ rp_go (unsigned char op)
{
    hub_map (WORLD_BANK);
    rf.gen = WREC->cgen;
    hub_map (0);
    rf.op = op;
    rf.kind = &RP_KIND;
    rf.hub = &HUB->vsaved;
    return rp_x (&rf);
}

RP_STATIC int __fastcall__ rp_open (char* path, unsigned char slots)
{
    rf.path = path;
    rf.slots = slots;
    return rp_go (RP_OPEN);
}

RP_STATIC int __fastcall__ rp_make (char* path, unsigned char slots)
{
    rf.path = path;
    rf.slots = slots;
    return rp_go (RP_MAKE);
}

RP_STATIC void __fastcall__ rp_done (int fd, unsigned n)
{
    rf.fd = fd;
    rf.h.n = n;
    rp_go (RP_DONE);
}

RP_STATIC unsigned char __fastcall__ rp_name (char* path, const char* pre, unsigned char sn, unsigned char rx,
    unsigned char ry)
{
    rf.path = path;
    rf.pre[0] = pre[0];
    rf.pre[1] = pre[1];
    rf.sn = sn;
    rf.rx = rx;
    rf.ry = ry;
    rf.dir = HUB->savedir;
    hub_map (0);
    rf.op = RP_NAME;
    return rp_x (&rf);
}
