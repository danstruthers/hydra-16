/*
** wshoot.c - (dq0w) missiles (slice 4.7: DUNGEON.md's fights complete; HarnMaster's Combat 15-17, the referee's
** RS_MISSILE): a shot or a throw at an actor, every actor's by the same code, the character's (I_ATTACK's AIM_SHOT, a
** round's, fought on as a blow is: part 24's) and a denizen's (its intent's, I_ATTACK's AIM_SHOT: its mind's, as it
** senses the character with a launcher of its own and a shot to be had); a launcher loaded ("nock an arrow", "load the
** crossbow": I_LOAD); the missiles gathered again where they fell (I_GATHER).
**
** A shot's launcher (a sling, a bow, a crossbow: in hand) and its missile (the launcher's loaded one, or one of its
** family carried: a quiver's arrows by the ten, a bolt case's bolts, a pouch's stones; one not handy, in the pack, its
** reach owed: things.c's), or a thing thrown (in hand); the range in squares, its band by the book's ranges (reach_of:
** the short's squares, each band twice the last; a thing thrown by the catalogue's throws), the long and the extreme
** none for a launcher under a ceiling (the book's restricted ranges: everywhere underground, HOUSE.md's); a clear line
** (its squares let missiles through: the floor, an open door or a portcullis's bars, an arrow slit, a window without
** glass or with its shutters open; one that stands in it the line's end, unless the target's as far beyond it again:
** the book's firing over, Combat 15); the target behind cover (a slit or a window it stands beside, the shot through
** it: MS_COVER); aware of it or not (a denizen that hasn't sensed the character; the character not seeing the shooter:
** MS_UNAWARE).  Then the referee asked (wfight.c's blow, the missile's settings in mshot: its words, what it did, the
** fight told as a blow's, FW_MISSILE), and the missile where it falls: a hit or a block at the target's feet, a miss
** on past it along the line (a square to four, short of what stops it), a wild one somewhere near; an arrow or a bolt
** broken by its chance (kinds.c's snaps, a bolt's the house's: lost), a thing thrown moved itself; known to the
** character.  (A miss's stray at one beside its mark, the book's 20%, MS_STRAY: the referee's, the world's left for
** the room.)  Its time a round (a bow's load and loose, a sling's; a crossbow's load a round more: the
** book's, Combat 8's missile attack options), the loading alone its half.
**
** Swapped parts 65's and 66's (dq0w.cfg's SW65, SW66: overlay 20's), called through their stubs (bank.s's: shot_op,
** gath_op).  Their locals on the C stack, but the line's own (no part called while they're used: their bytes the
** image's), the main RAM's room
*/

#include <stddef.h>
#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "world.h"
#include "things.h"
#include "wbody.h"

#pragma local-strings (on)

unsigned char pshoot;                           /* (The character's fight a missile's: part 24's, I_ATTACK's AIM_SHOT) */
unsigned char gath;                             /* (A gathering under way: the main loop's, gather_on) */
extern const unsigned char throws[];            /* (kinds.c's, the catalogue's bank's) */
unsigned char __fastcall__ on_pc (unsigned i);  /* (Part 10's) */
void cat_map (void);                            /* (things.c's: the catalogue's bank in the window) */
unsigned char __fastcall__ mfire (unsigned char att, unsigned char def, unsigned char z, unsigned char sn,
    unsigned sx, unsigned sy, unsigned tx, unsigned ty, unsigned thr, unsigned char tk);    /* (Part 65's) */
void __fastcall__ mfall (unsigned char sn, unsigned sx, unsigned sy, unsigned tx, unsigned ty, unsigned char out,
    unsigned char mk, unsigned thr);            /* (Part 67's) */

/* The launchers by kind, from KI_SLING: a sling, the bows, a crossbow; their missiles' family */
#define IS_LAUNCHER(k)  ((unsigned char) ((k) - KI_SLING) <= KI_CROSSBOW - KI_SLING)
#define BANDS           2                       /* (The bands a launcher's allowed under a ceiling: short, medium) */
#define FAM_OF(k)       ((k) == KI_SLING ? FM_SHOT : (k) == KI_CROSSBOW ? FM_BOLT : FM_ARROW)

/* What an actor has to shoot with (arms'): its launcher (sl_item, held: sl_held) and a missile for it (sm_item: the
** launcher's loaded one, or the handiest of its family carried); the main RAM's (kept through the parts called: items
** calls part 19's items_begin, which puts a part's image back as it was) */
static unsigned sl_item, sm_item;
static unsigned char sl_kind, sl_held;

#pragma code-name (push, "SW65CODE")
#pragma rodata-name (push, "SW65RODATA")

/* A missile's short range, in squares (the book's Combat 16 Missile Data Table's; a thing thrown, the catalogue's) */
static unsigned char __fastcall__ reach_of (unsigned char k)
{
    static const unsigned char r[5] = { 15, 20, 25, 30, 20 };   /* (Sling, shortbow, longbow, hartbow, crossbow) */

    if (IS_LAUNCHER (k)) {
        return r[k - KI_SLING];
    }
    cat_map ();
    k = throws[k < KI_COUNT ? k : 0];
    hub_map (0);
    return k;
}

static unsigned char __fastcall__ span (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? (dx > 255 ? 255 : dx) : (dy > 255 ? 255 : dy);
}

/* ---- The line of fire (lof: no part called) */
#pragma bss-name (push, "SW65RODATA")
static unsigned lx_, ly_;
static unsigned char lcover, lfree;
#pragma bss-name (pop)

/* Whether a missile passes square lx_, ly_ of sn: 0 not, 1 it does, 2 through a narrow opening (a slit, a window) */
static unsigned char __fastcall__ passes (unsigned char sn)
{
    unsigned char* p = at (sn, lx_, ly_);
    unsigned char t;
    const struct portal* o;

    if (!p || (t = TILE (*p)) == T_ROCK || t == T_WALL) {
        return lfree;                           /* (A wall's corner: the eye's past it, the line's) */
    }
    if ((t == T_DOOR || t == T_WINDOW) && (o = portal_at (cx_, cy_)) != 0) {
        if (o->kind == PK_SLIT) {
            return 2;
        }
        if (o->kind == PK_WINDOW) {
            return (o->parts & PP_GLASS) || ((o->parts & PP_SHUTTERS) && !(o->state & PS_OPEN)) ? 0 : 2;
        }
        return (o->state & (PS_OPEN | PS_WRECKED)) || o->kind == PK_PORTCULLIS || o->kind == PK_GRATE;
    }
    return 1;
}

/* Someone at lx_, ly_ on sn (the character, an actor) but the shooter and the target (me, tgt: an actor's entry, 0xFE
** the character): 1 */
static unsigned char __fastcall__ someone (unsigned char sn, unsigned char me, unsigned char tgt)
{
    struct actor* a = HUB->actor;
    unsigned char k;

    hub_map (0);
    if (me != 0xFE && tgt != 0xFE && sn == HUB->pc_stratum && lx_ == HUB->pc_x && ly_ == HUB->pc_y) {
        return 1;
    }
    for (k = 0; k < NACTORS; ++k, ++a) {
        if (k != me && k != tgt && a->state && a->sn == sn && a->x == lx_ && a->y == ly_) {
            return 1;
        }
    }
    return 0;
}

/* The line from sx, sy to tx, ty on sn (the squares between them, by Bresenham's steps): 0 clear (lcover: the target
** behind cover, beside a slit or a window the line comes through), SH_LINE (a wall, a door, glass), SH_WAY (someone in
** it, the target not as far beyond again) */
static unsigned char __fastcall__ lof1 (unsigned char sn, unsigned sx, unsigned sy, unsigned tx, unsigned ty,
    unsigned char me, unsigned char tgt)
{
    signed char dx = (signed char) (tx - sx), dy = (signed char) (ty - sy), ex, ey, err, e2;
    unsigned char n = 0, d = span (dx, dy), w;

    ex = dx < 0 ? -dx : dx;
    ey = dy < 0 ? -dy : dy;
    err = ex - ey;
    lfree = (*(me == 0xFE ? at (sn, tx, ty) : at (sn, sx, sy)) & T_VIS) != 0;   /* (The denizen seen by the character:
                                                **   the eye's line, the senses', past a wall's corner; a portal's,
                                                **   someone's in it still the line's end) */
    lx_ = sx;
    ly_ = sy;
    lcover = 0;
    for (;;) {
        e2 = err << 1;
        if (e2 > -ey) {
            err -= ey;
            lx_ += dx < 0 ? -1 : 1;
        }
        if (e2 < ex) {
            err += ex;
            ly_ += dy < 0 ? -1 : 1;
        }
        if (lx_ == tx && ly_ == ty) {
            return 0;
        }
        ++n;
        if (!(w = passes (sn))) {
            return SH_LINE;
        }
        lcover = w == 2 && n > 1 && span ((int) (tx - lx_), (int) (ty - ly_)) == 1;
        if (d < 2 * n && someone (sn, me, tgt)) {
            return SH_WAY;
        }
    }
}

/* The line either way (Bresenham's squares from the other end, past a corner the first way cut: the eye's, a
** doorway seen across at a slant): 0 clear, else the first way's why */
static unsigned char __fastcall__ lof (unsigned char sn, unsigned sx, unsigned sy, unsigned tx, unsigned ty,
    unsigned char me, unsigned char tgt)
{
    unsigned char r = lof1 (sn, sx, sy, tx, ty, me, tgt);

    return r == SH_LINE && !lof1 (sn, tx, ty, sx, sy, tgt, me) ? 0 : r;
}

/* ---- What an actor shoots with (arms: the character's, who 0; body bd's, its own, its did's things, IW_DEN, and
** what's in them), sl_item ...: 1 a launcher (sl_kind its kind), 2 one in hand, 0 none */
static unsigned char __fastcall__ mine (unsigned i, unsigned char who)
{
    struct item* q = item_at (i);

    while (q->where == IW_ITEM) {
        q = item_at (q->x);
    }
    return who ? q->where == IW_DEN && (q->owner | q->hide << 8) == bd.did : q->where == IW_ACTOR && !q->x;
}

static unsigned char __fastcall__ arms (unsigned char who)
{
    unsigned i, best = 0;
    unsigned char f, k, h = 0xFF, mk;

    sl_item = sm_item = 0;
    sl_held = 0;
    for (i = 0; (i = items (i, IN_ACTOR | IN_ITEM | IN_DEN, 0)) != 0; ) {
        k = ip->kind;
        f = ip->flags;
        if (IS_LAUNCHER (k) && (!sl_item || ((f & I_HELD) && !sl_held)) && mine (i, who)) {
            sl_item = i;
            sl_kind = k;
            sl_held = (f & I_HELD) != 0;
        }
    }
    if (!sl_item) {
        hub_map (0);
        return 0;
    }
    if (item_at (sl_item)->shut & IS_NOCKED) {
        sm_item = sl_item;                      /* (Loaded: its missile in it) */
        hub_map (0);
        return 1 + sl_held;
    }
    mk = sl_kind == KI_SLING ? KI_STONES : sl_kind == KI_CROSSBOW ? KI_BOLTS : KI_ARROWS;
    for (i = 0; (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
        if (ip->kind == mk && mine (i, who)) {
            f = who ? 0 : handy (i) != 0;       /* (The handiest: in a quiver, on the belt, before the pack's) */
            if (f < h) {
                h = f;
                best = i;
            }
        }
    }
    sm_item = best;
    hub_map (0);
    return 1 + sl_held;
}

/* ---- A shot (or a throw) by att (0 the character, else an actor's entry plus 1; body bd the denizen in it) at def,
** its aim z, from sx, sy at tx, ty on sn: its launcher's (sl_*, sm_item) or the thing thrown (thr, its kind tk): its
** range, its line, the referee asked (part 15's blow), its missile taken and fallen.  0 if it was shot, or if the
** shooter's turn went getting up or taking up its weapon; else why not (SH_*) */
unsigned char __fastcall__ s_mfire (unsigned char att, unsigned char def, unsigned char z, unsigned char sn,
    unsigned sx, unsigned sy, unsigned tx, unsigned ty, unsigned thr, unsigned char tk)
{
    unsigned char d = span ((int) (tx - sx), (int) (ty - sy)), band, out, mk, c;
    struct item* p;

    mk = thr ? tk : sl_kind;
    c = reach_of (mk);
    for (band = 0; band < 4 && d > (unsigned) (c << band); ++band) {
    }
    if (band >= (thr ? 4 : BANDS)) {
        return SH_RANGE;
    }
    if ((out = lof (sn, sx, sy, tx, ty, att ? att - 1 : 0xFE, def ? def - 1 : 0xFE)) != 0) {
        return out;
    }
    hub_map (0);
    mshot.kind = mk;
    mshot.band = band;
    mshot.dist = d;
    mshot.flags = (lcover ? MS_COVER : 0) | ((att ? (*at (sn, sx, sy) & T_VIS) : bd.doing != DO_ROAM) ? 0 :
        MS_UNAWARE);                            /* (The character sees its shooter; a denizen senses the character) */
    den_blow (def, z | AIM_FIRE);               /* (Part 15's blow, the missile's: how it came out, told, bev's) */
    if ((out = bev.x[1]) == 0xFF || out == O_RISE || out == O_REARM) {
        return out == 0xFF ? SH_CANT : 0;       /* (None struck; or its turn went getting up, taking up its weapon) */
    }
    if (!thr) {
        p = item_at (sm_item);                  /* (Its missile: the launcher unloaded, or one from the lot) */
        if (sm_item == sl_item) {
            p->shut &= ~IS_NOCKED;
        } else if (p->qty > 1) {
            --p->qty;
        } else {
            p->kind = 0;
        }
        mk = sl_kind == KI_SLING ? KI_STONES : sl_kind == KI_CROSSBOW ? KI_BOLTS : KI_ARROWS;
    }
    mfall (sn, sx, sy, tx, ty, out, mk, thr);   /* (Where it falls: part 67's) */
    return 0;
}

/* ---- The part's calls */

/* Whether the character can shoot at actor k (k its entry plus 1: fighting on's, part 24's fight_reach, pshoot's): 2
** if it can (within range, seen, a clear line, a missile to hand), 1 if it's senseless (the same), 0 not (a thing
** thrown: one throw, no fighting on) */
static unsigned char __fastcall__ shot_reach (unsigned char k)
{
    struct actor* a = &HUB->actor[(k - 1) & 7];
    unsigned char d, sn;
    unsigned tx, ty;

    hub_map (0);
    if (!k || !a->state || (sn = a->sn) != HUB->pc_stratum || !IS_LAUNCHER (HUB->pc_strike & 0x7F)) {
        return 0;
    }
    d = a->doing == DO_DOWN ? 1 : 2;
    tx = a->x;
    ty = a->y;
    if (!arms (0) || !sm_item || !(*at (sn, tx, ty) & T_VIS)) {
        return 0;
    }
    hub_map (0);
    if (span ((int) (tx - HUB->pc_x), (int) (ty - HUB->pc_y)) > (unsigned) (reach_of (sl_kind) << (BANDS - 1)) ||
        lof (sn, HUB->pc_x, HUB->pc_y, tx, ty, 0xFE, (k - 1) & 7)) {
        d = 0;
    }
    hub_map (0);
    return d;
}

/* A denizen's shot (body bd, entry bk: its intent's, I_ATTACK's AIM_SHOT, part 15's den_blow) at the character, its
** aim z: its launcher taken in hand first (from its back, its belt: its round's, the book's grope; not if it holds
** another thing: no shot, AG_NOSHOT), a missile of its own for it (none: it shoots no more, AG_SHOOTS gone), the shot
** (none to be had, out of range, no clear line: AG_NOSHOT, its mind's to close with the character till the character
** moves: part 14's) */
static void __fastcall__ den_shot (unsigned char z)
{
    unsigned i;
    struct item* p;
    unsigned char g = AG_SHOOTS;                /* (What's lost if there's no shot: its shooting, else this shot) */

    HUB->agear[bk] &= ~AG_NOSHOT;               /* (A shot looked for: none from here, set again) */
    if (!arms (1) || !sm_item) {
        goto none;                              /* (None, or no missile left: it closes with the character) */
    }
    g = AG_NOSHOT;
    if (span ((int) (bpx - bd.x), (int) (bpy - bd.y)) > (unsigned) (reach_of (sl_kind) << (BANDS - 1)) ||
        lof (bd.sn, bd.x, bd.y, bpx, bpy, bk, 0xFE)) {
        goto none;                              /* (No shot from here: none readied for it) */
    }
    if (!sl_held) {
        for (i = 0; (i = items (i, IN_DEN, 0)) != 0; ) {
            if ((ip->flags & I_HELD) && mine (i, 1)) {
                g = AG_SHOOTS;                  /* (Its hands full: it fights with what it holds, shooting no more
                                                **   till its things change, its RS_EQUIP) */
                goto none;
            }
        }
        p = item_at (sl_item);                  /* (Taken in hand: from its back, from its belt, its round's) */
        p->where = IW_DEN;
        p->sn = bd.hsn;
        p->x = bd.hx;
        p->y = bd.hy;
        p->owner = bd.did;
        p->hide = bd.did >> 8;
        p->flags = (p->flags & ~I_WORN) | I_HELD;
        hub_map (0);
        limb_news (WB_READIES | (unsigned) sl_kind << 8, bk + 1);
        return;
    }
    if (sm_item != sl_item && sl_kind == KI_CROSSBOW) {
        bd.ready += HUB->round;                 /* (A crossbow's load a round more) */
    }
    if (s_mfire (bk + 1, 0, z, bd.sn, bd.x, bd.y, bpx, bpy, 0, 0)) {
        hub_map (0);
        HUB->agear[bk] |= AG_NOSHOT;
    }
    return;
none:
    hub_map (0);                                /* (Its round given back: free now, nothing done) */
    HUB->agear[bk] = g == AG_NOSHOT ? HUB->agear[bk] | AG_NOSHOT : HUB->agear[bk] & ~AG_SHOOTS;
    bd.ready = HUB->now;
}

/* Body bd about to strike in melee (part 15's den_blow): a launcher it holds let fall at its feet, its hands free for
** its weapon (dropped, no time: the book's; told, WB_DROP) */
static void den_unbow (void)
{
    struct item* p;

    if (!arms (1) || !sl_held) {
        return;
    }
    p = item_at (sl_item);
    p->where = IW_SQUARE;
    p->sn = bd.sn;
    p->x = bd.x;
    p->y = bd.y;
    p->owner = p->hide = 0;
    p->flags = (p->flags & ~(I_HELD | I_WORN)) | I_KNOWN;
    hub_map (0);
    limb_news (WB_DROP | (unsigned) sl_kind << 8, bk + 1);
}

/* Part 65's calls: SO_ARMS what the character shoots with (arms'); SO_REACH whether it can shoot at k (its entry plus 1);
** SO_DEN body bd's shot at the character, its aim z; SO_UNBOW its launcher let fall (SO_FOE part 67's pc_shot's) */
unsigned char __fastcall__ s_shot_op (unsigned char op, unsigned char k, unsigned char z)
{
    unsigned char f = 0;

    hub_map (0);
    switch (op) {
    case SO_ARMS:
        return arms (0);
    case SO_REACH:
        return shot_reach (k);
    case SO_DEN:
        den_shot (z);
        break;
    default:
        den_unbow ();
        break;
    }
    hub_map (0);
    return f;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 66: the missiles gathered again */
#pragma code-name (push, "SW66CODE")
#pragma rodata-name (push, "SW66RODATA")

static void __fastcall__ told (unsigned w1, unsigned char b)
{
    struct msg m;

    m.kind = E_TASK;
    m.who = 0;
    m.b = b;
    m.w1 = w1;
    hub_map (0);
    m.t = HUB->now;
    hub_event (&m);
}

/* Lot kind k a missile of family f (0: any) */
static unsigned char __fastcall__ missile (unsigned char k, unsigned char f)
{
    return k == KI_ARROWS ? !f || f == FM_ARROW : k == KI_BOLTS ? !f || f == FM_BOLT : k == KI_STONES && (!f || f ==
        FM_SHOT);
}

/* ---- Gathering (I_GATHER: "gather the arrows"): the missiles lying where the character sees them, on its stratum
** within GATHER_D squares, of the family said (FM_*, 0 any), each walked over to and taken up in turn (as a thing
** taken up is, things.c's carry: put back in the quiver by the player, "put the arrows in the quiver"), the
** nearest first, a lot it can't get to passed by (two at most); none left, told how many (TE_GATHERED, the last's kind
** times 256, b the count).  As it's free (the main loop's, gath): 2 walking or taking up, 0 done.  Its state the main
** RAM's (kept through the parts it calls) */
#define GATHER_D        12
static unsigned char gcount, glast;          /* (gath: the family, FM_*, plus 0x80) */
static unsigned char gat;                       /* (Where it was as it last set out, a hash) */
static unsigned gtried[1];                      /* (The lot passed by) */

static unsigned char gather_on (void)
{
    unsigned i, n = 0, px, py, x, y;
    unsigned char d, best = 0xFF, sn, k;

    hub_map (0);
    sn = HUB->pc_stratum;
    px = HUB->pc_x;
    py = HUB->pc_y;
    for (i = 0; (i = items (i, IN_SQUARE, 0)) != 0; ) {
        x = ip->x;
        y = ip->y;
        if (ip->sn != sn || (ip->flags & (I_KNOWN | I_HIDDEN)) != I_KNOWN || !missile (ip->kind, gath & 0x7F) ||
            i == gtried[0] ||
            (unsigned) (x - px + GATHER_D) > 2 * GATHER_D || (unsigned) (y - py + GATHER_D) > 2 * GATHER_D) {
            continue;
        }
        d = (unsigned char) (x > px ? x - px : px - x) + (unsigned char) (y > py ? y - py : py - y);
        if (d < best && (d <= 2 || (*at (sn, x, y) & T_VIS))) {
            best = d;
            n = i;
        }
    }
    hub_map (0);
    if (!n) {
        gath = 0;
        told (TE_GATHERED | (unsigned) glast << 8, gcount);
        return 0;
    }
    x = item_at (n)->x;
    y = item_at (n)->y;
    k = item_at (n)->qty;
    glast = item_at (n)->kind;
    hub_map (0);
    if (!in_reach (n)) {
        if (gat == (unsigned char) (px + (py << 4))) {
            gtried[0] = n;            /* (No nearer: passed by) */
        } else {
            gat = px + (py << 4);
            go_to (x, y);
        }
        return 2;
    }
    if (!carry (n, 0)) {
        gtried[0] = n;               /* (No room for it) */
        return 2;
    }
    gcount = gcount + k > 255 ? 255 : gcount + k;
    hub_map (0);
    HUB->pc_ready = HUB->now + 20;             /* (Stooped for and taken up: two seconds) */
    return 2;
}

/* Part 66's calls: GO_GATHER a gathering begun (a the family); GO_ON its next */
unsigned __fastcall__ s_gath_op (unsigned char op, unsigned char a)
{
    if (op == GO_ON) {
        return gather_on ();
    }
    gcount = glast = 0;
    gat = 0xFF;
    memset (gtried, 0, sizeof gtried);
    gath = a | 0x80;
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 67: the character's shot, and where a missile falls */
#pragma code-name (push, "SW67CODE")
#pragma rodata-name (push, "SW67RODATA")

static unsigned char __fastcall__ span67 (int dx, int dy)
{
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    return dx > dy ? (dx > 255 ? 255 : dx) : (dy > 255 ? 255 : dy);
}

/* Whether a missile goes on through square x, y of sn (as part 65's passes: the floor, a door open, bars, a slit, a
** window without glass) */
static unsigned char __fastcall__ on_through (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char* p = at (sn, x, y);
    unsigned char t;
    const struct portal* o;

    if (!p || (t = TILE (*p)) == T_ROCK || t == T_WALL) {
        return 0;
    }
    if ((t == T_DOOR || t == T_WINDOW) && (o = portal_at (cx_, cy_)) != 0) {
        return o->kind == PK_SLIT || (o->kind == PK_WINDOW ? !(o->parts & PP_GLASS) && (!(o->parts & PP_SHUTTERS) ||
            (o->state & PS_OPEN)) : (o->state & (PS_OPEN | PS_WRECKED)) || o->kind == PK_PORTCULLIS || o->kind ==
            PK_GRATE);
    }
    return 1;
}

/* Where a missile falls (part 65's fire, out the blow's O_*), shot from sx, sy at the target's tx, ty on sn: a hit or a
** block at the target's feet, a miss on past it along the line (a square to four, short of what stops it), a wild one
** near the target; an arrow or a bolt broken by its chance (snaps; a bolt's the house's 15), lost; a thing thrown (thr,
** its item) moved itself, a launcher's missile (kind mk) put down; known to the character */
void __fastcall__ s_mfall (unsigned char sn, unsigned sx, unsigned sy, unsigned tx, unsigned ty, unsigned char out,
    unsigned char mk, unsigned thr)
{
    unsigned x, y, lx, ly;
    signed char dx, dy;
    unsigned char k, c;
    struct item* p;

    x = tx;                                     /* (Where it falls) */
    y = ty;
    if (out == O_MISS || out == O_DODGED) {     /* (On past it, along the line) */
        dx = (tx > sx) - (tx < sx);
        dy = (ty > sy) - (ty < sy);
        for (k = random (4) + 1; k; --k) {
            lx = x + dx;
            ly = y + dy;
            if (!on_through (sn, lx, ly)) {
                break;
            }
            x = lx;
            y = ly;
        }
    } else if (out == O_FUMBLE) {               /* (Wild: somewhere near) */
        lx = tx - 2 + random (5);
        ly = ty - 2 + random (5);
        if (on_through (sn, lx, ly)) {
            x = lx;
            y = ly;
        }
    }
    if (thr) {
        p = item_at (thr);                      /* (The thing itself, out of its hands: where it falls) */
        p->where = IW_SQUARE;
        p->sn = sn;
    } else {
        cat_map ();
        c = mk == KI_BOLTS ? 15 : mk == snaps[0].kind ? snaps[0].chance : 0;   /* (A bolt's, the house's 15) */
        hub_map (0);
        if (random (100) < c || !(thr = lay_body (mk, sn, x, y))) {
            return;                             /* (Broken: lost) */
        }
        p = item_at (thr);
    }
    p->x = x;
    p->y = y;
    p->flags = (p->flags & ~(I_HELD | I_WORN)) | I_KNOWN;
    hub_map (0);
}

/* The character's shot at actor k (part 15's pc_blow, pshoot's: the hub's pc_strike SK_THING | what it shoots with,
** a launcher, or the thing it throws; its aim pc_aim): the shot, its missile, its time (a crossbow's load a round
** more, a missile not handy its reach); one carried not in hand taken up first (its round's, TE_READIED); not to be shot, told (E_TASK
** TE_NO_SHOT, b why: SH_*) and the fight given up */
unsigned char __fastcall__ s_pc_shot (unsigned char k)
{
    unsigned char tk, why = 0, sn, ar;
    unsigned tx, ty, thr = 0;
    struct msg m;
    struct item* p;

    if (k == 0xFF) {                            /* (The foe it turns its missiles to: the nearest hostile it can shoot
                                                **   at, not fleeing, fighting on's, part 24's fight_foe; its range the
                                                **   launcher's bands: a setting of the player's to come, the auto-engage
                                                **   range, the user's, would set here where a fight on begins with them) */
        for (why = 0xFF, k = 0; k < NACTORS; ++k) {
            hub_map (0);
            if (HUB->actor[k].state && HUB->actor[k].attitude == AT_HOSTILE && HUB->actor[k].doing < DO_FLEE &&
                shot_op (SO_REACH, k + 1, 0) == 2) {
                hub_map (0);
                sn = span67 ((int) (HUB->actor[k].x - HUB->pc_x), (int) (HUB->actor[k].y - HUB->pc_y));
                if (sn < why) {
                    why = sn;
                    thr = k + 1;
                }
            }
        }
        hub_map (0);
        return (unsigned char) thr;
    }
    tk = HUB->pc_strike & 0x7F;
    sn = HUB->pc_stratum;
    BD_GET (k);
    tx = bd.x;
    ty = bd.y;
    if (!bd.state || bd.sn != sn) {
        why = SH_GONE;
    } else if (IS_LAUNCHER (tk)) {
        why = !(ar = shot_op (SO_ARMS, 0, 0)) ? SH_NONE : !sm_item ? SH_SPENT : 0;
        hub_map (0);
        HUB->pc_strike = SK_THING | sl_kind;    /* (What it shoots with: the referee's, the words') */
        if (!why && ar < 2) {                 /* (Carried, not in hand: taken up, both hands but a sling's, if
                                                **   they're free, its round's grope, the book's; its shot the next) */
            for (thr = 0; (thr = items (thr, IN_FLAGS | IN_ACTOR, I_HELD)) != 0 && ip->x; ) {
            }
            if (thr) {
                why = SH_HANDS;
            } else {
                p = item_at (sl_item);
                p->where = IW_ACTOR;
                p->x = 0;
                p->sn = tk = sl_kind == KI_SLING ? IH_RIGHT : IH_BOTH;
                p->flags = (p->flags & ~I_WORN) | I_HELD;
                hub_map (0);
                HUB->pc_shand = tk;
                equip_now ();
                why = 0xFF;                     /* (Told: TE_READIED, its kind) */
            }
        }
        if (!why && sm_item != sl_item) {       /* (Not loaded: its load first, a crossbow's a round more; one not
                                                **   handy, its reach owed) */
            if (handy (sm_item)) {
                reach_from (sm_item);
                thr = reach_done (0);
                pextra = 0;
                hub_map (0);
                HUB->pc_ready += thr;
                thr = 0;
            }
            hub_map (0);
            if (sl_kind == KI_CROSSBOW) {
                HUB->pc_ready += HUB->round;
            }
        }
    } else {
        while ((thr = items (thr, IN_FLAGS | IN_ACTOR, I_HELD)) != 0 && (ip->kind != tk || ip->x)) {
        }
        why = thr ? 0 : SH_NONE;
    }
    if (!why) {
        hub_map (0);
        why = mfire (0, k + 1, HUB->pc_aim, sn, HUB->pc_x, HUB->pc_y, tx, ty, thr, tk);
        BD_PUT ();
        ++HUB->den_seq;
        if (!why && !thr) {
            equip_now ();                       /* (Its load the lighter, the referee told) */
        }
    }
    hub_map (0);
    if (why) {
        m.w1 = TE_READIED | (unsigned) sl_kind << 8;
        if (why != 0xFF) {
            pshoot = 0;
            HUB->pc_task.foe = 0;               /* (No shot: the fight given up, told why; one fought on after it with
                                                **   what it holds) */
            HUB->pc_strike = SK_BEST;
            m.w1 = TE_NO_SHOT;
        }
        m.kind = E_TASK;
        m.who = 0;
        m.b = why;
        m.t = HUB->now;
        hub_event (&m);
    }
    return 0;
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ==== Part 68: a launcher loaded (overlay 14's bank: overlay 20's room) */
#pragma code-name (push, "SW68CODE")
#pragma rodata-name (push, "SW68RODATA")

static void __fastcall__ told68 (unsigned w1, unsigned char b)
{
    struct msg m;

    m.kind = E_TASK;
    m.who = 0;
    m.b = b;
    m.w1 = w1;
    hub_map (0);
    m.t = HUB->now;
    hub_event (&m);
}

static unsigned char __fastcall__ missile68 (unsigned char k, unsigned char f)
{
    return k == KI_ARROWS ? f == FM_ARROW : k == KI_BOLTS ? f == FM_BOLT : k == KI_STONES && f == FM_SHOT;
}

/* A launcher loaded (I_LOAD: "nock an arrow", "load the crossbow"): the one it holds (else none, SH_NONE), a missile
** of its family carried (none, SH_SPENT) set to it (its shut's IS_NOCKED; the lot the fewer), told68 (TE_LOADED, its kind
** times 256; b 1 if it was already): its time (a bow's or a sling's half a round, a crossbow's a round), 0 none */
unsigned s_load_go (void)
{
    unsigned i, n = 0, best = 0;
    unsigned char k, lk = 0, h = 0xFF, f;

    for (i = 0; (i = items (i, IN_FLAGS | IN_ACTOR, I_HELD)) != 0; ) {
        if (IS_LAUNCHER (k = ip->kind) && on_pc (i)) {
            lk = k;
            n = i;
        }
    }
    hub_map (0);
    if (!lk) {
        told68 (TE_NO_SHOT, SH_NONE);
        return 0;
    }
    if (item_at (n)->shut & IS_NOCKED) {
        told68 (TE_LOADED | (unsigned) lk << 8, 1);
        return 0;
    }
    f = FAM_OF (lk);
    for (i = 0; (i = items (i, IN_ACTOR | IN_ITEM, 0)) != 0; ) {
        if (missile68 (ip->kind, f) && on_pc (i) && (k = handy (i) != 0) < h) {
            h = k;
            best = i;
        }
    }
    if (!best) {
        told68 (TE_NO_SHOT, SH_SPENT);
        return 0;
    }
    if (h) {
        reach_from (best);
        reach_done (0);
    }
    if (item_at (best)->qty > 1) {
        --item_at (best)->qty;
    } else {
        item_at (best)->kind = 0;
    }
    item_at (n)->shut |= IS_NOCKED;
    told68 (TE_LOADED | (unsigned) lk << 8, 0);
    equip_now ();
    hub_map (0);
    return lk == KI_CROSSBOW ? HUB->round : HUB->round / 2;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
