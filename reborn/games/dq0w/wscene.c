/*
** wscene.c - (dq0w) a test's scene, the world's part of it (../dq0/hub.h's scene: phase C's scene builder, scene1,
** 2026-11-24): the ops the tests' harness wrote at SCENE_AT in CMD_BANK, carried out at the top of the main loop, no
** body of an intent's in hand (bd: the copy that wrote back over the tests' pokes, OPEN.md's G18): the hub's bytes the
** world writes (SO_HUB), a job of its made due at once (SO_DUE: burn_ask, not burn_due, which a minute under way sets
** again as it ends, X1), and the referee asked its part (SO_RS: rs.h's RS_SCENE, in the world's slot); then scene_w
** set and the maker rung, if the scene has a part of its (SO_MHUB), else scene_m set too and the minds rung.  Swapped part scene's (parts.txt): in the main RAM only for a scene
*/

#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "things.h"
#include "trace.h"

#pragma code-name ("SW_scene_CODE")
#pragma rodata-name ("SW_scene_RODATA")
#pragma bss-name ("SW_scene_RODATA")           /* (The part's own while it's in: nothing kept past it) */

#define W_SLOT          HUB->slot[SLOT_WORLD]

void den_intents (void);                        /* (Part den_bodies's: wbody.h's) */
extern unsigned char sc_look;                   /* (dq0w.c's: the scene's intents held for the look at it) */
extern unsigned char den_done;                  /* (wbody.h's: the minds' turn done) */

/* At the turns pace, the scene in place whole (scene_d: the maker's part and the minds' done, after the world's)
** before the world goes on with its pass, and the intents the minds posted for it (SO_INTENT) held through this pass
** (sc_look, den_held's): the look at the bodies the scene moved (den_seq's, the late jobs') first, then what they
** do, as a mind's intent is carried out after the bodies it saw were looked at, whatever the tasks' timing.  Its pass
** went on as the minds were at their part, the look at the bodies before their intent was carried out or after it as
** the timing fell: a bandit's errand told or not, in one layout of the code and not in another (dq0denerrands',
** dq0dentakein's, dq0thingact's: OPEN.md's L1; even2's, 2026-11-25).  Thirty seconds at most, a turn's wait (a
** stuck task's); the ring full, its intents carried out (the minds wait on it).  Its own scene, s: the harness's
** next posted (hub.scene) as soon as it's in place */
static void __fastcall__ whole (unsigned char s)
{
    unsigned t0 = hy_ticks ();

    while (hub_map (0), HUB->scene_d != s && !HUB->quit && (unsigned) (hy_ticks () + 1 - t0) < 30 * HY_TICK_HZ) {
        hub_sleep (1);
        if (hub_map (0), (HUB->di_head + 1) % DI_SIZE == HUB->di_tail) {
            den_intents ();
        }
    }
    sc_look = 1;
    hub_map (0);
}

void s_scene (void)
{
    static unsigned at;
    static unsigned char s, k, n, mk, md;
    static unsigned char b[80];                 /* (An op's payload, copied out of CMD_BANK) */

    hub_map (0);
    s = HUB->scene;
    mk = md = 0;
    for (at = SCENE_AT; at < SCENE_AT + SCENE_SIZE - 2; at += 2 + n) {
        hub_map (CMD_BANK);
        k = HUB_WINDOW[at];
        n = HUB_WINDOW[at + 1];
        if (k == SO_END || n > sizeof b) {
            break;
        }
        memcpy (b, HUB_WINDOW + at + 2, n);
        mk |= k == SO_MHUB;
        md |= k == SO_DHUB || k == SO_REC || k == SO_INTENT || k == SO_DRAM;
        if (k == SO_HUB) {                      /* (Its bank, its offset, the bytes) */
            hub_map (b[0]);
            memcpy (HUB_WINDOW + (b[1] | b[2] << 8), b + 3, n - 3);
        } else if (k == SO_WRAM) {               /* (Its own main RAM: the address, the bytes) */
            memcpy ((char*) (b[0] | b[1] << 8), b + 2, n - 2);
        } else if (k == SO_DUE) {
            if (b[0] == DU_BURN) {
                burn_ask |= 1;                  /* (The lights' minute at once: the main loop's burn_due_) */
            } else {
                tr_n = b[0] == DU_TRACE ? TR_FLUSH : 0;     /* (The journal full, or held none) */
            }
        } else if (k == SO_RS) {                /* (The referee's: actor, a, b, c, w1, x[4-7]) */
            hub_map (0);
            W_SLOT.kind = RS_SCENE;
            W_SLOT.actor = b[0];
            W_SLOT.a = b[1];
            W_SLOT.b = b[2];
            W_SLOT.c = b[3];
            memcpy (&W_SLOT.w1, b + 4, 2);
            memcpy (W_SLOT.x + 4, b + 6, 4);
            hub_ask (SLOT_WORLD);
            W_SLOT.state = REQ_FREE;
        }
    }
    hub_map (0);
    if (!mk) {
        HUB->scene_m = s;                       /* (None of the maker's: the minds' next) */
        if (!md) {
            HUB->scene_d = s;                   /* (Nor the minds': in place now) */
        }
    }
    HUB->scene_w = s;                           /* (Its part done: the maker's now, if it has one, then the minds') */
    hub_ring (mk ? T_MAKER : T_DENIZENS);
    if (md && !HUB->tpr && den_done == HUB->den_go && HUB->di_tail == HUB->di_head) {
        whole (s);                              /* (The minds quiet, their turn done: else as ever, their turn's intents
                                                **   the world's to carry out as it goes, dq0stray's shot mid-fight; the turns pace's: the
                                                **   scene whole before the world goes on) */
    }
}
