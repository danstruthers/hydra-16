/*
** watch.h - (dq0harn) a watch of the clock passed: the referee's (heal.c's, overlay 6) and what passes between dungeons
** (../dq0acct's, told the days at rest).  The includer has heal_one, nput, rest_m and REST_ENOUGH, and pc.  Moved from
** heal.c whole, 2026-11-04.
*/

/* A watch passed (the clock's n'th): c's shock's roll (the book's, Physician 4: each watch; a creature's never its
** death, the house's: it lies in shock till it's over it), and at a day's end each wound treated EE healed and each
** infected one's roll, at five days' each other one's healing roll and its blood got back (Physician 4: the Sindarin
** two rolls) */
static void __fastcall__ watch_of (harn_char* c, unsigned n)
{
    unsigned char k, s = c->state;

    if (s == HARN_DEAD) {
        return;
    }
    if (s == HARN_SHOCK) {
        harn_shock_recovery (c, 0);
        if (c == &pc) {
            nput (c->state == HARN_DEAD ? " You die of shock." : c->state == HARN_CONSCIOUS ? " You come out of shock." :
                "");
        } else if (c->state == HARN_DEAD) {
            c->state = HARN_SHOCK;              /* (A creature's: the house's) */
            c->shockhr = 1;
        }
    }
    if (n % 6) {
        return;
    }
    for (k = c->ninj; k--; ) {
        if ((c->inj[k].flags & HARN_IF_INFECTED) || c->inj[k].hr == EE) {
            heal_one (c, k);
        }
    }
    if (n % 30 || (c == &pc && rest_m < REST_ENOUGH)) {
        if (c == &pc && !(n % 30)) {
            rest_m = 0;                         /* (Its rest too little: the days forfeit, the book's) */
        }
        return;
    }
    if (c == &pc) {
        rest_m = 0;
    }
    harn_regen_blood (c);
    for (k = c->ninj; k--; ) {
        if (!(c->inj[k].flags & HARN_IF_INFECTED) && c->state != HARN_DEAD &&
            !heal_one (c, k) && c->att[HARN_SP] == HARN_SIN && c == &pc) {
            heal_one (c, k);                    /* (The Sindarin's second roll) */
        }
    }
}

