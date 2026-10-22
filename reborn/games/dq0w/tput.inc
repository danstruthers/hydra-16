/*
** tput.inc - (dq0w) the traces' store's records put (wtrace.c's): parts 28's and 30's, each its own copy (a swapped
** part's code calls no other part's but through a stub: ../dq0/swap.inc's), included in each one's segments.
*/

/* The record at hand (pc, pk, psq, pt: its chunk, whose, its square and way, when) put in its bucket: one of its
** square and kind counted (pn passings more, psc its scent's: its passings and stays), its time the newest's, its way
** too (but a stay's, no passing: the way the last went from there kept); a new one in a free place, or over the
** oldest (older than the one at hand: one read in from a file may not be) */
static unsigned char pc, pk, pn, psc;
static unsigned psq, pt;
static void put (void)
{
    static struct trace* p;
    static struct trace* q;
    static unsigned b, a, oa;
    static unsigned char i, fr;

    b = BUCKET (pc, SQ_OF (psq));
    hy_bank (TR_BANK (b));
    for (i = fr = 0, oa = 0, q = 0, p = TR_AT (b); i < 4; ++i, ++p) {
        if (!p->c) {
            if (fr != 1) {
                fr = 1;
                q = p;
            }
            continue;
        }
        a = pt - p->t;
        if (p->c == pc && p->k == pk && SQ_OF (p->sq) == SQ_OF (psq)) {
            if (a & 0x8000) {
                return;
            }
            p->n = p->n + pn > 255 ? 255 : p->n + pn;
            if ((p->sc += psc) < psc) {
                p->sc = 255;                    /* (Its most) */
            }
            if (pn) {
                p->sq = psq;
            }
            p->t = pt;
            return;
        }
        if (fr != 1 && !(a & 0x8000) && a >= oa) {
            fr = 2;
            oa = a;
            q = p;
        }
    }
    if (q) {
        q->c = pc;
        q->k = pk;
        q->n = pn;
        q->sc = psc;
        q->sq = psq;
        q->t = pt;
    }
}
