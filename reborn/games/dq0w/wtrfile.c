/*
** wtrfile.c - (dq0w) the traces' store's files (wtrace.c's: the store): a chunk's records written to its region's
** file in the save folder (trSXXYY.dat, a version of it each time; each chunk a group of its own there: the record
** store's, ../dq0/rp.h), and read back.  Swapped part 30's (../dq0/swap.inc, bank.s's stub tr_file).
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "trace.h"
#include "tstore.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)

/* ==== Part 30: the store's files (its chunks' records written to their regions' files, read back) */
#pragma code-name (push, "SW_trace_files_CODE")
#pragma rodata-name (push, "SW_trace_files_RODATA")
#pragma bss-name (push, "SW_trace_files_RODATA")

#include "tput.inc"

#pragma bss-name (push, "BSS")
static struct rpk tr_kind;                      /* (Its versions counted: the main RAM's) */
#pragma bss-name (pop)
#define RP_STATIC       static
#define RP_KIND         tr_kind
#include "../dq0/rp.inc"

#pragma data-name (push, "SW_trace_files_RODATA")
static struct rpg rg = { RP_GROUPS, 0, 0, 0, sizeof (struct tgroup), offsetof (struct tgroup, n), sizeof (struct trace),
    buf };                                      /* (A region's file's groups, the record store's) */
#pragma data-name (pop)
static struct tgroup g;
static unsigned char hat, hh;
static int in, out;
static unsigned all;
static struct trace t;

/* The other chunks' groups in the file in, past (out >= 0: copied to out, their records counted in all) or to the
** chunk at hand's (its group in g: 1, or 0 none) */
static unsigned char others (void)
{
    unsigned char k;

    rg.in = in;
    rg.out = out;
    rg.at = hat;
    rg.all = all;
    k = rp_x (&rg);
    all = rg.all;
    memcpy (&g, rg.g, sizeof g);
    return k;
}

/* The chunk at hand's records (wr 0: counted only; 1, written to out, through the main RAM a record at a time; 2,
** freed): how many */
static unsigned __fastcall__ records (unsigned char wr)
{
    static struct trace* p;
    static unsigned b, n;
    static unsigned char i;

    for (n = b = 0; b < NBUCKETS; ++b) {
        for (i = 0; i < 4; ++i) {
            hy_bank (TR_BANK (b));
            p = TR_AT (b) + i;
            if (p->c == hh) {
                ++n;
                if (wr == 2) {
                    p->c = 0;
                } else if (wr) {
                    memcpy (&t, p, sizeof t);
                    write (out, &t, sizeof t);
                }
            }
        }
    }
    return n;
}

/* Entry h's chunk: its records written to its region's file, a new version (every other chunk's group as it was, then
** its own), not changed after (TF_KEEP), and freed, the entry free (TF_OUT); or read in from it (TF_IN: its group's,
** if it has one, and its flags) */
void __fastcall__ s_tr_file (unsigned char h, unsigned char what)
{
    static struct tch* e;
    static unsigned n;

    hh = h;
    hy_bank (ovbank[OVBANKS]);
    e = &THD->ch[h - 1];
    hat = (e->cx & 7) | (e->cy & 7) << 3;
    if (!rp_name (path, "tr", e->sn, e->cx >> 3, e->cy >> 3)) {
        if (what == TF_OUT) {
            records (2);                        /* (No folder: lost) */
        }
        goto done;
    }
    in = rp_open (path, 3);
    if (what == TF_IN) {
        if (in >= 0) {
            out = -1;
            if (others ()) {
                hy_bank (ovbank[OVBANKS]);
                THD->ch[h - 1].flags = g.flags;
                for (n = g.n; n && read (in, &t, sizeof t) == sizeof t; --n) {
                    pc = h;                     /* (Each put back: the oldest given up, if the store's full there) */
                    pk = t.k;
                    psq = t.sq;
                    pn = t.n;
                    psc = t.sc;
                    pt = t.t;
                    put ();
                }
            }
            close (in);
        }
        goto done;
    }
    if ((out = rp_make (path, 3)) >= 0) {
        all = 0;
        if (in >= 0) {
            others ();
        }
        g.at = hat;
        hy_bank (ovbank[OVBANKS]);
        g.flags = THD->ch[h - 1].flags & TC_SWEPT;
        if ((g.n = records (0)) != 0 || g.flags) {
            write (out, &g, sizeof g);
            records (1);
            all += g.n;
        }
        rp_done (out, all);
    }
    if (in >= 0) {
        close (in);
    }
    if (what == TF_OUT) {
        records (2);
    }
done:
    hy_bank (ovbank[OVBANKS]);
    THD->ch[h - 1].flags &= ~TC_DIRTY;
    if (what == TF_OUT) {
        THD->ch[h - 1].used = 0;
    }
    hub_map (0);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
