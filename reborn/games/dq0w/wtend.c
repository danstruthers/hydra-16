/*
** wtend.c - (dq0w, swapped part 70: ../dq0/swap.inc) wounds over time, the world's part (slice 4.3: HOUSE.md's wounds
** over time): the bodies' time told the referee (RS_TIME), a minute at a time while anyone bleeds or the character's
** poisoned, else as the referee's next watch comes (its marks: the shock's recovery, the healing of wounds), what the
** character was about with it (at rest, about, at vigorous work: a venom's interval, the bestiary's) and which
** bleeders are fighting (a folk creature binds its own bleeding when it isn't); what came of it for the character
** told the UI (E_TASK's TE_BODY: its words the referee's, RS_DESCRIBE b 4's), a hurt or a death its long task stopped,
** hard.
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

unsigned char __fastcall__ on_pc (unsigned i);   /* (Part 10's stub: carried by the character) */

#pragma code-name ("SW70CODE")
#pragma rodata-name ("SW70RODATA")
#pragma local-strings (on)

extern unsigned char pc_hit;                    /* (dq0w.c's: a hurt stops its long task, hard) */
static unsigned long body_at;                   /* (The minutes told up to: the last one's end) */
static unsigned long wdue;                      /* (The referee's next watch: asked then, nothing bleeding) */
unsigned long body_due;                         /* (The same, for the lights' minute: wbody.h's) */

/* The bodies' time (wfight.c's bled, as a minute or more has gone: the lights' minute, the denizens' intents): the
** referee told, if there's anything to tell it; the actors dead of it back, and those that bound their bleeding times
** 256 */
unsigned s_body_ask (void)
{
    struct msg m;                               /* (On the C stack: the main RAM's room) */
    struct req* r;
    unsigned w, n = 0;
    unsigned char k, a, g, x = 0;

    hub_map (0);
    if (!body_at || HUB->now < body_at) {
        body_at = HUB->now;                     /* (The game begun, or carried on: from now) */
    }
    while (HUB->now - body_at >= 600 && n < 6000) {
        body_at += 600;
        ++n;
    }
    bled_at = (unsigned) body_at;
    if (!n || (!bleeds && !(HUB->pcsum.cond & (C_BLEEDING | C_POISONED)) && HUB->pcsum.health == H_FINE &&
        HUB->now < wdue)) {                     /* (Hurt: its rest toward its healing told, a minute at a time) */
        return 0;
    }
    k = HUB->pc_task.kind;
    g = HUB->pc_going & 0x3F;
    a = k == LT_WAIT || k == LT_SLEEP ? 0 : g ? (g > GA_JOG + 1 ? 2 : 1) : k == LT_FIGHT || k == LT_DIG ||
        k == LT_BATTER || k == LT_CLIMB || k == LT_FORCE || k == LT_HEAVE || k == LT_BEND ? 2 : k == LT_NONE ? 0 : 1;   /* (At rest
                                                **   standing still, or waiting, asleep; vigorous at a run, or at
                                                **   strenuous work, a fight; else about) */
    for (k = 0; k < NACTORS; ++k) {
        if (HUB->actor[k].state && HUB->actor[k].doing == DO_FIGHT) {
            x |= 1 << k;                        /* (Fighting: no binding its bleeding meanwhile) */
        }
    }
    r = &HUB->slot[SLOT_WORLD];
    r->kind = RS_TIME;
    r->actor = 0xFF;
    r->other = 0;
    r->a = a;
    r->b = 1;
    r->w1 = n;
    r->w2 = bleeds;
    r->x[0] = x;
    r = hub_ask (SLOT_WORLD);
    memcpy (&wdue, r->x + 4, 4);
    body_due = wdue;
    w = r->w1 | (unsigned) r->c << 8;
    a = r->result | r->grade;
    r->state = REQ_FREE;
    if (a & 5) {
        pc_hit = 1;                             /* (Hurt anew, a venom's ache, or dead of it: its long task stopped,
                                                **   hard) */
    }
    if (a & 3) {
        m.kind = E_TASK;                        /* (Its words the referee's: the UI's to ask, RS_DESCRIBE b 4) */
        m.who = 0;
        m.a = m.b = 0;
        m.w1 = TE_BODY;
        m.w2 = 0;
        m.t = HUB->now;
        hub_event (&m);
    }
    return w;
}
