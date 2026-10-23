/*
** wbody.c - (dq0w) the denizens' bodies (DENIZENS.md: in the tasks), the world's as the character's is.  The
** denizens' task (games/dq0d) is their minds: it decides what each actor means to do and says so (the hub's di, a
** ring: hub.h's the denizens' intents), and the world carries it out by the character's rules: a square at a time
** (one it can walk, step_ok: no wall, no shut door; no one on it, nor the character, nor where the character's move
** ends), its time its step's, more uphill, steeply down, on steps (as the world's cost () has the character's); a
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
#include "world.h"
#include "wbody.h"
#include "trace.h"

#pragma local-strings (on)

#define CATCHUP         30                      /* Tenths an actor makes up, at most, that it's fallen behind the clock
                                                **   (the real paces': the denizens' task late) */

struct actor bd;                                /* (wbody.h's: the main RAM's) */
unsigned char bk;
unsigned bpx, bpy;
unsigned char bps;
struct msg bev;
unsigned char bleeds;
unsigned bled_at;

#pragma code-name ("SW14CODE")
#pragma rodata-name ("SW14RODATA")

static unsigned __fastcall__ apart (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
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

/* What lies between body bd and the character, for those who hear it (SOUND.md: E_DEN's x[3]): the squares of wall
** and rock on the line (15 at most), and the shut doors times 16; 0 off its stratum, or more than 24 squares off
** (beyond anyone's hearing of a step) */
static unsigned char between (void)
{
    unsigned char* p;
    struct portal* q;
    unsigned char solid = 0, doors = 0;
    unsigned i, dist;
    int dx = bpx - bd.x, dy = bpy - bd.y;

    if (bd.sn != bps || (dist = apart (dx, dy)) > 24) {
        return 0;
    }
    for (i = 1; i < dist; ++i) {
        p = at (bd.sn, bd.x + dx * (int) i / (int) dist, bd.y + dy * (int) i / (int) dist);
        if (!p || !T_CLEAR (*p)) {
            if (solid < 15) {
                ++solid;
            }
        } else if (TILE (*p) == T_DOOR && (q = portal_at (cx_, cy_)) != 0 && !(q->state & PS_OPEN) && doors < 15) {
            ++doors;
        }
    }
    return solid | doors << 4;
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

    bev.x[3] = loud ? between () : 0;
    p = at (bd.sn, bd.x, bd.y);
    bev.x[2] = p && (*p & T_VIS);
    bev.kind = E_DEN;
    bev.who = bk + 1;
    bev.a = what;
    bev.b = loud;
    bev.w1 = bd.x;
    bev.w2 = bd.y;
    bev.x[0] = bd.sn;
    bev.x[1] = bd.kind;
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
    static unsigned char* p;
    static int z;

    for (any = 0, hub_map (0); (t = HUB->di_tail) != HUB->di_head; hub_map (0), HUB->di_tail = (t + 1) % DI_SIZE) {
        memcpy (&m, &HUB->di[t], sizeof m);
        help = HUB->pcsum.cond & (C_STUNNED | C_UNCONSCIOUS);   /* (The character helpless: no one engages it, the
                                                **   minds' to say what's done to it) */
        if (bleeds && (unsigned) HUB->now - bled_at >= 600) {
            bled ();
        }
        if (m.kind == I_DONE) {
            den_done = m.w1;                    /* (Their turn done: the turns pace goes on) */
            continue;
        }
        any = 1;
        if (m.kind == I_TROD || m.kind == I_SWEPT) {   /* (One out but not an actor: its passing; a chunk kept
                                                **   swept, its keepers' lair placed: the traces') */
            bd.kind = m.kind == I_SWEPT ? TK_SWEPT : m.x[1];    /* (Body bd but a copy: none put back) */
            bd.sn = m.x[2];
            bd.x = m.w1;
            bd.y = m.w2;
            den_trod (m.kind == I_SWEPT ? 0 : (m.a & (7 | TS_STAY)) | TS_NOWAY | (unsigned) m.b << 8);
            continue;
        }
        BD_GET ((m.who - 1) & 7);
        was = bd.doing;
        if (m.x[0] != 0xFF) {
            bd.doing = m.x[0];
        }
        if (m.x[1] != 0xFF) {
            bd.face = m.x[1];
        }
        if (HUB->tpr && (m.kind == I_STEP || m.kind == I_GOTO || m.kind == I_WAIT || m.kind == I_ATTACK) &&
            bd.ready + CATCHUP < HUB->now) {
            bd.ready = HUB->now - CATCHUP;      /* (Fallen behind the clock: a few seconds made up at most) */
        }
        switch (m.kind) {
        case I_ENTER:                           /* (A denizen come near the character: its body's from its record) */
            bd.state = 1;
            bd.sn = m.x[2];
            bd.x = m.w1;
            bd.y = m.w2;
            bd.ready = m.t;
            bd.step = m.a;
            bd.pos = POS_STANDING;              /* (On its feet, walking: its body's, the world's) */
            bd.gait = GA_WALK;
            bd.move = m.x[3];                   /* (Its legs, the referee's: slice 4.1b's) */
            bd.arm = 0;
            bleeds = m.b ? bleeds | 1 << bk : bleeds & ~(1 << bk);
            was = bd.doing;
            break;
        case I_LEAVE:
            bd.state = 0;
            bleeds &= ~(1 << bk);
            was = bd.doing;
            break;
        case I_STEP:                            /* (A square, by the character's rules: refused, it waits a moment) */
            if ((m.w1 & STEP_SCENT) && (m.w1 = tr_scent (m.x[2])) > H_NW) {
                bd.doing = DO_ROAM;             /* (No scent its nose can follow: the trail lost) */
                bd.ready += 10;
                break;
            }
            d = (m.w1 & 7) >> 1;
            x = bd.x + dir_x[d];
            y = bd.y + dir_y[d];
            if (den_legs () || taken (x, y) || !step_ok (bd.sn, bd.x, bd.y, d)) {
                bd.ready += 10;
                break;
            }
            if (!(m.w1 & STEP_COARSE)) {        /* (Its time: its step, uphill and steeply down more, on steps more) */
                z = elev_at (bd.sn, bd.x, bd.y);
                p = at (bd.sn, x, y);
                z = C->elev[cy_][cx_] - z;
                c = bd.step;
                if (z > 0) {
                    c += bd.step * z / 2;
                } else if (z < -1) {
                    c += bd.step * (-z - 1) / 4;
                }
                if (TILE (*p) == T_STEPS) {
                    c += bd.step / 2;
                }
                c += c * (bd.move & 3);         /* (Lame, twice its time; crawling, three times: slice
                                                **   4.1b's) */
                if ((m.w2 & STEP_FEELS) && unlit_at (bd.sn, x, y)) {
                    c <<= 1;                    /* (Its kind no darkvision, a square unlit: it feels its way, at
                                                **   half its pace, HOUSE.md's walking in the dark) */
                }
                bd.ready += c;
            }
            if (!noinit && !help && bd.attitude <= AT_UNFRIENDLY && bd.sn == bps &&
                apart (bd.x - bpx, bd.y - bpy) > 1 && apart (x - bpx, y - bpy) == 1) {
                eng = bk + 1;                   /* (An enemy come up beside the character: it engages it, the
                                                **   engage initiative test's, once its body's put back) */
            }
            den_trod (m.w1 & 7);
            bd.x = x;
            bd.y = y;
            bd.face = m.w1 & 7;
            if (!(m.w1 & STEP_COARSE)) {
                told_den (m.a, m.b);
            }
            break;
        case I_GOTO:                            /* (A dash past the character: a moment's) */
            if (den_legs () || !open_sq (at (bd.sn, m.w1, m.w2)) || apart (m.w1 - bd.x, m.w2 - bd.y) > 2 ||
                taken (m.w1, m.w2)) {
                bd.ready += 10;
                break;
            }
            den_trod (bearing ((int) (m.w1 - bd.x), (int) (m.w2 - bd.y)));
            bd.x = m.w1;
            bd.y = m.w2;
            bd.ready += 2 * bd.step;
            told_den (m.a, m.b);
            break;
        case I_CLIMB:                           /* (An ally come along with the character: beside it) */
            if (open_sq (at (m.x[2], m.w1, m.w2)) && !taken (m.w1, m.w2)) {
                bd.sn = m.x[2];
                bd.x = m.w1;
                bd.y = m.w2;
                bd.ready = HUB->now + 10;
            }
            break;
        case I_WAIT:
            bd.ready += m.w1;
            break;
        case I_ROUSE:
            if (m.w2 || bd.ready > HUB->now + m.w1) {
                bd.ready = HUB->now + m.w1;
            }
            break;
        case I_ATTACK:                          /* (A round's blow; a robbery, a round's work) */
        case I_ROB:
            bd.ready += HUB->round;
            if (m.kind == I_ROB) {
                den_rob (m.w1, m.w2);
            } else {
                den_blow (m.a, m.b);
            }
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
