/*
** trod.inc - (dq0w) a passing journalled (the traces' journal, the hub's TJ_AT: trace.h's), each part's own copy
** that makes them (dq0w.c's part 23, the character's walk; wbody.c's part 14, the bodies' steps), included in its
** segments: in the main RAM, it took the room the world keeps free.
*/

/* Passing s journalled (its time the clock's now): in its ring, the character's or the denizens', then its count;
** the hub's header in the window after */
static void __fastcall__ trod (struct tstep* s)
{
    struct tjournal* j = TJ_AT;
    unsigned* n;

    hub_map (0);
    s->t = HUB->now;
    hub_map (DEN_BANK);
    n = s->kind ? (unsigned*) &j->n : (unsigned*) &j->pn;
    memcpy ((s->kind ? j->e : j->p) + (*n & (NJOURNAL - 1)), s, sizeof *s);
    ++*n;
    hub_map (0);
    ++tr_n;
}
typedef char trod_fits[NJOURNAL == NTRAIL && !(NJOURNAL & (NJOURNAL - 1)) ? 1 : -1];
