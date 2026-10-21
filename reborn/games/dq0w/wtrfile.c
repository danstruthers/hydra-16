/*
** wtrfile.c - (dq0w) the traces' store's files (wtrace.c's: the store): a chunk's records written to its region's
** file in the save folder (trSXXYY.dat, a version of it each time: ../dq0/vfile.inc's; each chunk a group of its own
** there), and read back.  Swapped part 29's (../dq0/swap.inc, bank.s's stub tr_file).
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "work.h"
#include "things.h"
#include "trace.h"
#include "tstore.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a swapped part's in it) */
#pragma static-locals (on)

/* ==== Part 29: the store's files (its chunks' records written to their regions' files, read back) */
#pragma code-name (push, "SW29CODE")
#pragma rodata-name (push, "SW29RODATA")
#pragma bss-name (push, "SW29RODATA")

#include "tput.inc"

#define VF_STATIC       static
#include "../dq0/vfile.inc"

static struct vfh hd;
static struct tgroup g;
static unsigned char hat, hh;
static int in, out;
static unsigned all;
static struct trace t;

/* The other chunks' groups in the file in, past (out >= 0: copied to out, their records counted in all) or to the
** chunk at hand's (its group in g: 1, or 0 none) */
static unsigned char others (void)
{
    static unsigned k;

    while (read (in, &g, sizeof g) == sizeof g) {
        if (g.at == hat && out < 0) {
            return 1;
        }
        if (g.at == hat || out < 0) {
            lseek (in, (unsigned) (g.n * sizeof (struct trace)), SEEK_CUR);
            continue;
        }
        write (out, &g, sizeof g);
        for (k = g.n; k; --k) {
            read (in, buf, sizeof (struct trace));
            write (out, buf, sizeof (struct trace));
        }
        all += g.n;
    }
    return 0;
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
    if (!vf_name (path, "tr", e->sn, e->cx >> 3, e->cy >> 3)) {
        if (what == TF_OUT) {
            records (2);                        /* (No folder: lost) */
        }
        goto done;
    }
    in = vf_open (path, 3, &hd);
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
                    pt = t.t;
                    put ();
                }
            }
            close (in);
        }
        goto done;
    }
    if ((out = vf_make (path, 3)) >= 0) {
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
        vf_done (out, all);
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
