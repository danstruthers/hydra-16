/*
** hscene.c - (dq0harn) a test's scene, the referee's part of it (rs.h's RS_SCENE, the world's SO_RS: ../dq0/hub.h's
** scene, phase C's scene builder, scene1, 2026-11-24): an actor's sheet set as the scene says, in HarnMaster's terms,
** by the referee itself between its requests, so none of its copies writes it back.  Swapped part scene's: in the main
** RAM only for a scene
*/

#pragma code-name ("SW_scene_CODE")
#pragma rodata-name ("SW_scene_RODATA")

#include <string.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

void __fastcall__ s_scene_rs (struct req* r)
{
    struct bbody b;
    unsigned char n = aden[(r->actor - 1) & 7], w = RS_SCENE_WHAT (*r);

    if (w == SCN_RAM || w == SCN_HUB) {         /* (Its own RAM's bytes, or the hub's header's it writes) */
        memcpy ((char*) (w == SCN_HUB ? HUB_WINDOW : 0) + RS_SCENE_EX (*r), r->x + 4, RS_SCENE_WHICH (*r) & 7);
        return;
    }
    if (!r->actor) {
        if (w != SCN_OUT) {
            r->result = 0xFF;
            return;
        }
        pc.state = HARN_SHOCK;                  /* (Out, and come round in shock as its shock rolls fall) */
        pc.shockhr = 5;
        pc_out = RS_SCENE_TILL (*r);
        return;
    }
    if (w == SCN_TIRE) {                        /* (Its body: its fatigue as the scene says) */
        bread (fb, (struct bbody*) 0x8000 + n, &b, sizeof b);
        b.fl = RS_SCENE_LEVEL (*r);
        b.ex = RS_SCENE_EX (*r);
        b.at = RS_SCENE_TILL (*r);
        bwrite (fb, (struct bbody*) 0x8000 + n, &b, sizeof b);
        return;
    }
    sheet_get (n);
    if (w == SCN_OUT) {
        bs.state = (bs.state & ~3) | HARN_UNCONSCIOUS;
        bs.out = RS_SCENE_TILL (*r);
    } else if (w == SCN_WELL) {
        bs.state = HARN_CONSCIOUS;
        bs.bl = bs.ninj = 0;
    } else if (w == SCN_ATT) {
        bs.att[RS_SCENE_WHICH (*r) % 5] = RS_SCENE_LEVEL (*r);
    } else if (w == SCN_HURT || w == SCN_LAME) {
        bs.ninj = 1;                            /* (One wound: a minor one of its levels, every test the worse; lamed,
                                                **   a serious one, its Agility 1, its Rise a test failed) */
        memset (bs.inj, 0, sizeof bs.inj[0]);
        bs.inj[0].sev = w == SCN_HURT ? HARN_MINOR : HARN_SERIOUS;
        bs.inj[0].il = w == SCN_HURT ? RS_SCENE_LEVEL (*r) : 2;
        if (w == SCN_LAME) {
            bs.att[3] = 1;
        }
    } else {
        r->result = 0xFF;
    }
    sheet_put (n);
}
