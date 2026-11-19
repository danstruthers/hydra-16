/*
** wbody.c - (dq0w) the denizens' bodies (DENIZENS.md: in the tasks), the world's as the character's is.  The
** denizens' task (games/dq0d) is their minds: it decides what each actor means to do and says so (the hub's di, a
** ring: hub.h's the denizens' intents), and the world carries it out by the character's rules: a square at a time
** (one it can walk, step_ok: no wall, no shut door; no one on it, nor the character, nor where the character's move
** ends), its time its step's, more uphill, steeply down, on steps, a diagonal seven feet (the character's own:
** dq0w.c's step_cost, one body rule, 2026-11-17); a
** dash past the character; a wait; come along with the character up or down; a blow (wfight.c's); a body come into
** its keeping, gone from it.  What the bodies do is told in the denizens' events (E_DEN: a step, a noise, the
** character noticed or lost), the world's to write.  Which the character sees is the senses' to decide (dq0p's
** dens.c, the world asking: dq0w.c's dens_seen).
**
** All of it swapped part 14's (../dq0/swap.inc: dq0w.cfg's SW14), called through its stubs (bank.s's).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "hub0.h"
#include "world.h"
#include "wbody.h"
#include "trace.h"

#pragma local-strings (on)


struct actor bd;                                /* (wbody.h's: the main RAM's) */
unsigned char bk;
unsigned bpx, bpy;
unsigned char bps;
struct msg bev;
unsigned char bleeds;
unsigned bled_at;

unsigned char __fastcall__ handout (const struct msg* m);   /* (Part 111's: wempty.c, every actor's) */

#pragma code-name ("SW14CODE")
#pragma rodata-name ("SW14RODATA")

static unsigned __fastcall__ apart (int dx, int dy)
{
    if (dx < 0) {
        dx = -dx;
    }
    if (dy < 0) {
        dy = -dy;
    }
    return dx > dy ? dx : dy;
}

/* Anyone but body bk on square x, y: the character, where its move ends, another body (the hub's header in the
** window after) */
static unsigned char __fastcall__ taken (unsigned x, unsigned y)
{
    struct actor* q = HUB->actor;
    unsigned char k;

    hub_map (0);
    if ((x == bpx && y == bpy) || (x == HUB->pc_tx && y == HUB->pc_ty)) {
        return 1;
    }
    for (k = 0; k < NACTORS; ++k, ++q) {
        if (k != bk && q->state && q->x == x && q->y == y) {
            return 1;
        }
    }
    return 0;
}

static unsigned char __fastcall__ open_sq (unsigned char* p)
{
    return p && (TILE (*p) == T_FLOOR || TILE (*p) == T_CORR || TILE (*p) == T_STEPS);
}

/* What lies between body bd and the character, for those who hear it (SOUND.md: E_DEN's x[3]), by the senses' sound
** layer (hub.h's SOUND_AT, slice 4.5's: round corners, a shut door or a wall on the way): the squares the sound goes
** past the line between them, a quarter (15 at most), and a shut door or a wall on the way times 16 (its two squares
** out of them); 15 not reached, or past the layer (beyond anyone's hearing of a step); 0 off its stratum.  The layer
** worked out where the character was a step or two before (the senses' to bring up to date as a denizen asks): from
** there, and those squares more */
static unsigned char between (void)
{
    unsigned char c, e, o;                      /* (On the C stack: the main RAM's none; another stratum's */
    int dx, dy;                                 /*   not heard, the layer's not its) */

    hub_map (SIGHT_BANK);
    dx = bd.x - SNDHEAD->x;
    dy = bd.y - SNDHEAD->y;
    c = SNDHEAD->ok && SNDHEAD->sn == bps && dx >= -SND_R && dx <= SND_R && dy >= -SND_R && dy <= SND_R ?
        SND_SQ (dx, dy) : SND_NONE;
    o = apart (bpx - SNDHEAD->x, bpy - SNDHEAD->y);
    hub_map (0);
    if (c >= 0x80) {
        return 15;
    }
    e = SND_COST (c) + o - apart (bd.x - bpx, bd.y - bpy) + 2 - (c & SND_AIR ? DOOR_SND : 0);
    e = (signed char) e < 0 ? 0 : e >> 2;
    return (e > 15 ? 15 : e) | (c & SND_AIR) >> 2;
}

/* Body bd's passing journalled as it leaves its square (the traces': trod.inc's): its kind's (0 none: not kept), the
** way h it went from there (| TS_NOWAY: a coarse move's, none known: its square come to; | TS_STAY, a stay there, no
** passing: its TQs h's high byte, the minds') */
#include "trod.inc"

static void __fastcall__ den_trod (unsigned h)
{
    struct tstep s;

    if (bd.kind) {
        s.kind = bd.kind;
        s.sn = bd.sn;
        s.head = h;
        s.stay = h >> 8;
        s.x = bd.x;
        s.y = bd.y;
        trod (&s);
    }
}

/* Body bd did what (DE_*), how loud (0-9): told (E_DEN), where it is, whether the character sees its square now (the
** senses' marks), what lies between, if it's heard; the hub's header in the window after */
static void __fastcall__ told_den (unsigned char what, unsigned char loud)
{
    unsigned char* p;

    E_DEN_BETWEEN (bev) = loud ? between () : 0;
    p = at (bd.sn, bd.x, bd.y);
    E_DEN_SEEN (bev) = p && (*p & T_VIS);
    bev.kind = E_DEN;
    bev.who = bk + 1;
    E_DEN_DE (bev) = what;
    E_DEN_LOUD (bev) = loud;
    E_DEN_SX (bev) = bd.x;
    E_DEN_SY (bev) = bd.y;
    E_DEN_SN (bev) = bd.sn;
    E_DEN_DKIND (bev) = bd.kind;
    hub_map (0);
    bev.t = HUB->now;
    hub_devent (&bev);
}

/* Body k's tactical advantage (the engage initiative test's, dq0w.c's part 25): backing away from the character (away
** 1: one not hostile), to the first square a step off out of its reach that it can walk (told as a step is), 2; else
** (or cornered) its blow first, an action more (its own round's time as it was), 1 */
unsigned char __fastcall__ s_den_first (unsigned char k, unsigned char away)
{
    static unsigned char d, r;
    static unsigned x, y;

    BD_GET (k);
    for (d = 0, r = 1; away && d < 4; ++d) {
        x = bd.x + dir_x[d];
        y = bd.y + dir_y[d];
        if (apart (x - bpx, y - bpy) > 1 && !taken (x, y) && step_ok (bd.sn, bd.x, bd.y, d)) {
            den_trod (H_OF (d));
            bd.x = x;
            bd.y = y;
            told_den (DE_MOVED, 1);
            r = 2;
            break;
        }
    }
    if (r == 1) {
        den_blow (0, Z_MID | FW_FIRST);
    }
    BD_PUT ();
    ++HUB->den_seq;
    return r;
}

/* The denizens' intents carried out, in turn, each taken from the ring once it's done (the bleeding seen to first:
** wfight.c's); the denizens rung after (they wait for them), unless all there was was their turn's end */
void s_den_intents (void)
{
    static struct msg m;
    static unsigned char t, was, d, any, eng, help;
    static unsigned x, y, c;
    static unsigned char s, o;
    static unsigned char* p;
    static int z;

    for (any = 0, hub_map (0); (t = HUB->di_tail) != HUB->di_head; hub_map (0), HUB->di_tail = (t + 1) % DI_SIZE) {
        if (ui_first ()) {
            break;                              /* (The rest after the UI's command's answer) */
        }
        memcpy (&m, &HUB->di[t], sizeof m);
        help = HUB->pcsum.cond & (C_STUNNED | C_UNCONSCIOUS);   /* (The character helpless: no one engages it, the
                                                **   minds' to say what's done to it) */
        if (bleeds && (unsigned) HUB->now - bled_at >= 600) {
            bled ();
        }
        if (m.kind == I_DONE) {
            den_done = I_DONE_TURN (m);         /* (Their turn done: the turns pace goes on) */
            continue;
        }
        any = 1;
        if ((unsigned char) (m.kind - I_TROD) <= I_KIT - I_TROD) {     /* (One out but not an actor: its passing;
                                                **   a chunk kept swept, its keepers' lair placed: the traces'; a
                                                **   denizen's things made: part 60's) */
            if (m.kind == I_KIT) {
                den_other (&m);
                if (kit_off & 2) {
                    kit_off &= 1;
                    break;                      /* (The rest after the UI's command's answer, this one again) */
                }
            } else {
                bd.kind = m.kind == I_SWEPT ? TK_SWEPT : I_TROD_TKIND (m);   /* (Body bd but a copy: none put back) */
                bd.sn = I_TROD_SN (m);
                bd.x = I_TROD_X (m);
                bd.y = I_TROD_Y (m);
                den_trod (m.kind == I_SWEPT ? 0 : (I_TROD_HOW (m) & (7 | TS_STAY)) | TS_NOWAY | (unsigned) I_TROD_STAY (m) << 8);
            }
            continue;
        }
        BD_GET ((m.who - 1) & 7);
        was = bd.doing;
        if (was == DO_DOWN && bd.ready > HUB->now && m.kind != I_ENTER && m.kind != I_LEAVE) {
            m.kind = I_WAIT;                    /* (Senseless till it comes round: it does nothing meanwhile, its mind's
                                                **   intent posted before it fell, or a rouse, no matter; it lies
                                                **   there, its doing kept, 2026-10-29) */
            I_WAIT_TENTHS (m) = 0;
            I_WAIT_DOING (m) = I_WAIT_FACE (m) = 0xFF;
        }
        if (I_DEN_DOING (m) != 0xFF) {
            bd.doing = I_DEN_DOING (m);
        }
        if (I_DEN_FACE (m) != 0xFF) {
            bd.face = I_DEN_FACE (m);
        }
        if (grip) {
            grip_held (&m);                     /* (Held by the character: its try to break free, part 15's) */
        }
        switch (m.kind) {
        case I_ENTER:                           /* (A denizen come near the character: its body's from its record,
                                                **   its things: part 60's) */
            den_enter (&m);
            was = bd.doing;
            break;
        case I_LEAVE:
            bd.state = 0;
            bleeds &= ~(1 << bk);
            was = bd.doing;
            break;
        case I_STEP:                            /* (A square, by the character's rules: refused, it waits a moment) */
            if (I_STEP_WHEN (m) > bd.ready) {
                bd.ready = I_STEP_WHEN (m);     /* (Taken no sooner than it was meant: its intent's time, the minds'
                                                **   stamp; a denizen fallen behind the clock catches up in full, no
                                                **   CATCHUP's few seconds, the user's, 2026-11-19) */
            }
            if (I_STEP_SCENT (m) && (I_STEP_WAY (m) = tr_scent (I_STEP_NOSE (m))) > H_NW) {
                bd.doing = DO_ROAM;             /* (No scent its nose can follow: the trail lost) */
                bd.ready += 10;
                break;
            }
            d = I_STEP_HEAD (m);                /* (Its heading: a diagonal a diagonal way's, slant's) */
            x = bd.x + head_x[d];
            y = bd.y + head_y[d];
            if ((((bd.move & 4) | bd.pos) && den_legs ()) || taken (x, y) || !step8 (bd.sn, bd.x, bd.y, d)) {
                bd.ready += 10;
                break;
            }
            o = (unsigned char) apart (bd.x - bpx, bd.y - bpy);
            if (!I_STEP_COARSE (m)) {           /* (Its time: its step, uphill and steeply down more, on steps more, a
                                                **   diagonal seven feet: the character's own, step_cost's, one body rule,
                                                **   2026-11-17) */
                z = elev_at (bd.sn, bd.x, bd.y);
                p = at (bd.sn, x, y);
                z = C->elev[cy_][cx_] - z;
                c = (TILE (*p) == T_STEPS) | (d & 1 ? SC_DIAG : 0);
                s = bd.step;                    /* (Its walk, or hurried its jog, and the louder for it (the user's,
                                                **   2026-11-03, "walk unless hurried, and closing in to attack should
                                                **   be hurried unless they are cautious"): fleeing; coming at its foe
                                                **   but a cautious one (a stalker, a shy kind, hurt, its heart against
                                                **   it); an ally fallen behind, the house's) */
                if (bd.doing == DO_FLEE || (bd.doing == DO_COMING && bd.attitude == AT_HOSTILE && !(bd.timid & 1) &&
                    !(bd.mood & (AM_STALKS | AM_HURT)) && bd.heart >= 0) ||
                    (bd.doing == DO_FOLLOW && o > 3)) {
                    s >>= 1;
                    ++I_STEP_LOUD (m);
                }
                bd.ready += step_cost (s, z, c | SC_MUL ((1 + (bd.move & 3)) << (I_STEP_FEELS (m) && unlit_at (bd.sn, x,
                    y))));                      /* (Lame, twice its time; crawling, three times: slice 4.1b's; its kind
                                                **   no darkvision, a square unlit: it feels its way, at half its pace,
                                                **   HOUSE.md's walking in the dark) */
            }
            if (!noinit && !help && bd.attitude <= AT_UNFRIENDLY && bd.sn == bps && o > 1 && apart (x - bpx, y - bpy) == 1) {
                eng = bk + 1;                   /* (An enemy come up beside the character: it engages it, the
                                                **   engage initiative test's, once its body's put back) */
            }
            den_trod (I_STEP_HEAD (m));
            bd.x = x;
            bd.y = y;
            bd.face = I_STEP_HEAD (m);
            if (!I_STEP_COARSE (m)) {
                told_den (I_STEP_HEARD (m), I_STEP_LOUD (m));
            }
            break;
        case I_GOTO:                            /* (A dash past the character: a moment's) */
            if (((I_GOTO_X (m) != bd.x || I_GOTO_Y (m) != bd.y) && ((bd.move & 4) | bd.pos) && den_legs ()) ||
                !open_sq (at (bd.sn, I_GOTO_X (m), I_GOTO_Y (m))) ||   /* (One told where it stands, a snore, a shout: not got up for, 2026-11-02) */
                apart (I_GOTO_X (m) - bd.x, I_GOTO_Y (m) - bd.y) > 2 || taken (I_GOTO_X (m), I_GOTO_Y (m))) {
                bd.ready += 10;
                break;
            }
            den_trod (bearing ((int) (I_GOTO_X (m) - bd.x), (int) (I_GOTO_Y (m) - bd.y)));
            bd.x = I_GOTO_X (m);
            bd.y = I_GOTO_Y (m);
            bd.ready += bd.step;                /* (Its two squares at a jog) */
            told_den (I_GOTO_HEARD (m), I_GOTO_LOUD (m));
            break;
        case I_ROB:
            if (I_ROB_ERRAND (m) >= FE_EMPTY) { /* (Its thing's things put out, a thing put down on a square: the */
                handout (&m);                   /*   character's own code, wempty.c's, P4's and P5's, 2026-11-18) */
                break;
            }
        case I_RISE:                            /* (Its position, its mind's: part 80's, part 14's room, 2026-11-03) */
        case I_CLIMB:                           /* (The rarer: an ally come along with the character; a robbery; */
        case I_PORTAL:                          /*   a door worked, the character's code, a thing taken, Phase 3C's: part 80's) */
        case I_TAKE:
            den_more (&m);
            break;
        case I_WAIT:
            bd.ready += I_WAIT_TENTHS (m);
            break;
        case I_ROUSE:
            if (I_ROUSE_WHENEVER (m) || bd.ready > HUB->now + I_ROUSE_TENTHS (m)) {
                bd.ready = HUB->now + I_ROUSE_TENTHS (m);
            }
            break;
        case I_ATTACK:                          /* (A round's blow, struck at its moment, its ready as it's taken (part
                                                **   15's blow_t): not the clock's as the world gets to it, so a fight's
                                                **   the same whatever the passes' timing; the user's, 2026-11-13) */
            bd.ready += HUB->round;
            if (was != DO_FIGHT) {
                den_close (I_ATTACK_FOE (m), I_ATTACK_AIM (m));   /* (Its attack begun: closed with, told first, part
                                                **   106's, 2026-11-17) */
            }
            den_blow (I_ATTACK_FOE (m), I_ATTACK_AIM (m));
            break;
        }
        if (bd.doing != was && (!was || !bd.doing)) {
            told_den (bd.doing ? DE_NOTICED : DE_LOST, 0);
        }
        BD_PUT ();
        if (m.kind != I_WAIT || bd.doing != was) {
            ++HUB->den_seq;                     /* (Changed: the world looks again at what the character sees) */
        }
        if (eng) {
            d = eng;                            /* (dq0w.c's part 25: the character's blow first, if it's the
                                                **   quicker) */
            eng = 0;
            engaged (d, 0);
        }
    }
    if (any) {
        hub_ring (T_DENIZENS);
    }
}
