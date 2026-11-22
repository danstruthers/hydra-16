/*
** wtrscent.c - (dq0w) the way the character's scent goes (wtrace.c's store, read: a hunter by smell's step,
** STEP_SCENT, wbody.c's part 14 asking).  Swapped part 32's (../dq0/swap.inc, bank.s's stub tr_scent).
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "things.h"
#include "trace.h"
#include "tstore.h"
#include "wbody.h"
#include "acts.h"

#pragma static-locals (on)

#pragma code-name (push, "SW32CODE")
#pragma rodata-name (push, "SW32RODATA")
#pragma bss-name (push, "SW32RODATA")

static unsigned char rsn;
static unsigned rcx, rcy;

/* The chunk at hand's entry in the store's directory plus 1, or 0 (the directory in the window after) */
static unsigned char chunk (void)
{
    static struct tch* e;
    static unsigned char h;

    hy_bank (ovbank[OVBANKS]);
    for (h = 0, e = THD->ch; h < NTCH; ++h, ++e) {
        if (e->used && e->sn == rsn && e->cx == rcx && e->cy == rcy) {
            return h + 1;
        }
    }
    return 0;
}

/* The way the character's scent goes from body bd's square (a hunter by smell's, STEP_SCENT: wbody.c's), read by its
** nose: the character's trace on its square younger than the nose's life of its scent (hub.h's SCENT_LIFE), the way it
** went from there; else the freshest beside it, the way there; 0xFF, none (the journal not taken in first: the
** trail's newest the minds' own, dq0d.c's scent).  Each square's life its own, the longer by its passings and the
** character's stays there (hub.h's TR_MORE).  The one place the air layer will turn a trail downwind (slice
** 4.5).  Sniffed (the one-path plan's stage 2, 2026-11-21: "sniff" for anyone, the audit's M1, M2): the character's
** own sniff, the act table's (its three seconds on its ready; the referee's test of its nose by its sheet, at the steps
** the scent's strength gives, trace.h's TRACE_STEPS, as the character's sniff's: its clearest square's points, its
** scent's life's less a point each doubling of its age, wtrace.c's found's), missed at a critical failure (none) */
#pragma static-locals (push, off)
static unsigned char __fastcall__ sniffed (unsigned char way, unsigned char c)
{
    struct actq q;                              /* (On the C stack: act_test swaps this part out, its statics too) */

    act_get (q, AC_SNIFF);
    bd.ready += q.r.time;
    return act_test (bk + 1, q.r.test, q.r.steps + TRACE_STEPS (1, (signed char) c)) == G_BADLY ? 0xFF : way;
}
#pragma static-locals (pop)

unsigned char __fastcall__ s_tr_scent (unsigned char nose)
{
    static struct trace* p;
    static struct stratum* t;
    static unsigned life, a, best, b, q, now;
    static signed char dx, dy;
    static unsigned char h, i, way, sm, c;

    hy_bank (ovbank[OVBANKS]);
    sm = TKN->smell[0];
    hub_map (0);
    now = (unsigned) (HUB->now / TQ);
    t = &STRATUM (bd.sn);
    for (best = 0xFFFF, way = 0xFF, dy = -1; dy <= 1; ++dy) {
        for (dx = -1; dx <= 1; ++dx) {
            rsn = bd.sn;
            rcx = (bd.x + dx - t->x0) >> CW_BITS;
            rcy = (bd.y + dy - t->y0) >> CW_BITS;
            if (!(h = chunk ())) {
                continue;
            }
            q = ((bd.x + dx - t->x0) & (CW - 1)) | ((bd.y + dy - t->y0) & (CW - 1)) << CW_BITS;
            b = BUCKET (h, q);
            hy_bank (TR_BANK (b));
            for (i = 0, p = TR_AT (b); i < 4; ++i, ++p) {
                if (p->c != h || p->k || SQ_OF (p->sq) != q) {
                    continue;
                }
                life = SCENT_LIFE (sm + TR_MORE (p->sc), nose);
                if ((a = now - p->t) < life && (dx | dy ? way > H_NW || a < best : p->sq & TW_WAY)) {
                    for (c = SCENT_LOG (sm + TR_MORE (p->sc), nose) + 1; a; a >>= 1) {
                        --c;                    /* (Its strength: its points, less one each doubling of its age) */
                    }
                    a = now - p->t;
                    best = dx | dy ? a : 0;     /* (Its own square's first: the way the character went on) */
                    way = dx | dy ? bearing (dx, dy) : p->sq >> 10 & 7;
                    if (!(dx | dy)) {
                        goto done;
                    }
                }
            }
        }
    }
done:
    if ((way & 1) && way <= H_NW && !slant (bd.sn, bd.x, bd.y, way)) {
        way &= 6;                               /* (A diagonal only a diagonal way: else the four's, as ever) */
    }
    hub_map (0);
    return way > H_NW ? way : sniffed (way, c);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
#pragma bss-name (pop)
