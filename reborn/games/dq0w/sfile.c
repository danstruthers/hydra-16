/*
** sfile.c - (dq0w) the strata's records out of the hub's cache (world.h's MAXSTRATA: stratum n's in its slot n & 7,
** move 2.8's no fixed count of strata): a stratum's record written to the save folder as its slot's made way for
** another's (stSS.dat, SS its number in hex: a version each time, the record store's: ../dq0/rp.h), and read back into its slot as it's
** wanted again (the world's, as the character or a shaft comes to it: dq0w.c's stratum_in).  Swapped part 22's
** (../dq0/swap.inc, bank.s's stubs).
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)

#pragma code-name (push, "SW_strata_out_CODE")
#pragma rodata-name (push, "SW_strata_out_RODATA")
#pragma bss-name (push, "SW_strata_out_RODATA")           /* (Its own data, what's wanted only while it runs: the main RAM's room) */

#pragma bss-name (push, "BSS")
static struct rpk st_kind;                      /* (Its versions counted: the main RAM's) */
#pragma bss-name (pop)
#define RP_STATIC       static
#define RP_KIND         st_kind
#include "../dq0/rp.inc"

extern char path[56];                           /* (store.c's) */
extern struct change buf[8];                    /* (store.c's: 64 bytes) */

/* Slot k's record copied to or from the main RAM's buf, a piece (from off, n bytes) at a time (the kernel's copies
** want the task's memory) */
static void __fastcall__ piece (unsigned char k, unsigned char off, unsigned char n, unsigned char in)
{
    hub_map (WORLD_BANK);
    if (in) {
        memcpy ((char*) &WREC->strata[k] + off, buf, n);
    } else {
        memcpy (buf, (char*) &WREC->strata[k] + off, n);
    }
    hub_map (0);
}

#define REC             sizeof (struct stratum)
#define P1              (REC > 64 ? 64 : REC)

/* Stratum sn's record (in its slot) written to its file: 1, or 0 (no folder) */
unsigned char __fastcall__ s_st_out (unsigned char sn)
{
    unsigned char k = SLOT (sn);
    int fd;

    if (!rp_name (path, "st", sn, 0, 0) || (fd = rp_make (path, 2)) < 0) {
        return 0;
    }
    piece (k, 0, P1, 0);
    write (fd, buf, P1);
    piece (k, P1, REC - P1, 0);
    write (fd, buf, REC - P1);
    rp_done (fd, 1);
    return 1;
}

/* Stratum sn's record read back from its file into its slot: 1, or 0 (none: it's not been made) */
unsigned char __fastcall__ s_st_in (unsigned char sn)
{
    unsigned char k = SLOT (sn), ok = 0;
    int fd;

    if (!rp_name (path, "st", sn, 0, 0) || (fd = rp_open (path, 2)) < 0) {
        return 0;
    }
    if (read (fd, buf, P1) == P1) {
        piece (k, 0, P1, 1);
        if (read (fd, buf, REC - P1) == REC - P1) {
            piece (k, P1, REC - P1, 1);
            ok = 1;
        }
    }
    close (fd);
    return ok;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
