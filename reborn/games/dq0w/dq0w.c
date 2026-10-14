/*
** dq0w.c - Dingus Quest Zero's world: a task of its own, started by the UI (dq0w SEG ROLE: the hub's segment and its
** role there, hub.h).  It owns the game's clock, takes the actors' intents and carries them out, and tells what
** happened as events; the dungeon, its rules and the referee's questions come in later steps.
**
** The clock (main.c's once): each real tick (200 a second) is round tenths of game time over tpr ticks (the pace:
** a round in tpr ticks); paused (Escape) or held (the UI asking something), it stands.  At the turns pace (tpr 0)
** it moves only as the character acts: straight to when the character's free again.
*/

#include <stdlib.h>
#include <hydra.h>
#include "../dq0/hub.h"

static unsigned last;                           /* The tick count at the last look */
static unsigned long acc;                       /* Real ticks times round, not yet game time */

/* The clock moved on by d tenths, written so that a reader sees it whole (hub_now) */
static void __fastcall__ advance (unsigned long d)
{
    ++HUB->tick;
    HUB->now += d;
    ++HUB->tick;
}

static void __fastcall__ event (unsigned char kind, unsigned char who, unsigned w1)
{
    static struct msg m;

    m.kind = kind;
    m.who = who;
    m.w1 = w1;
    m.t = HUB->now;
    hub_event (&m);
}

/* The character busy for t tenths: free again at pc_ready (the turns pace: the clock there at once) */
static void __fastcall__ busy (unsigned t)
{
    HUB->pc_ready = HUB->now + t;
    if (!HUB->tpr) {
        advance (t);
    }
    event (E_TIME, 0, t);
}

static void __fastcall__ intent (struct msg* m)
{
    switch (m->kind) {
    case I_WAIT:
        busy (m->w1 ? m->w1 : HUB->round);
        break;
    case I_QUIT:
        HUB->quit = 1;
        break;
    }
}

/* The real clock's ticks since the last look, made game time at the pace */
static void clock_run (void)
{
    unsigned t = hy_ticks (), d = t - last;

    last = t;
    if (HUB->paused || HUB->hold || !HUB->tpr) {
        return;
    }
    acc += (unsigned long) d * HUB->round;
    if (acc >= HUB->tpr) {
        advance (acc / HUB->tpr);
        acc %= HUB->tpr;
    }
}

int main (int argc, char* argv[])
{
    static struct msg m;
    unsigned char was_busy = 0;

    if (argc < 3 || hub_attach (atoi (argv[1]), atoi (argv[2]))) {
        return 1;
    }
    last = hy_ticks ();
    while (hub_map (0), !HUB->quit) {         /* (The hub at $8000 again, each time round) */
        while (hy_sem_try (HUB->sem[T_WORLD]) == 0) {
        }
        while (hub_take (&m)) {
            intent (&m);
            was_busy = 1;
        }
        clock_run ();
        if (was_busy && HUB->now >= HUB->pc_ready) {
            event (E_READY, 0, 0);              /* (The character free again: the UI takes its next line) */
            was_busy = 0;
        }
        ++HUB->beat[T_WORLD];
        hy_sleep_ticks (2);
    }
    event (E_BYE, 0, 0);
    return 0;
}
