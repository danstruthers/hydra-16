/*
** wwater.c - (dq0w, swapped part 72, its image overlay 22's, the world's own for it: ../dq0/swap.inc) the water, and hunger and thirst (slice 4.9: DUNGEON.md's
** water, HOUSE.md's water and hunger and thirst).  Part 72: how deep the water is at a square (the chunk's, the
** maker's: world.h's wet bits and its rooms' levels); the character come into it as it walks (wading, a step the
** longer, then swimming, the referee's P_SWIM each minute: its squares a minute, its breath; going under, drowning; its
** flames put out as it swims, strenuous work tiring it); its hunger and thirst counted as the bodies' time
** goes (part 70's, a minute at a time, twice as fast at vigorous work: their levels, warned, told the referee for its
** fatigue from want); eating its rations, drinking from its waterskin or the water where it is, filling its waterskin
** there (the things' IO_EAT, IO_DRINK, IO_FILL).  Every actor's in time: the denizens' larders and their thirst later
** (DENIZENS.md's hook).
*/

#include <stddef.h>
#include <string.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "world.h"
#include "things.h"

unsigned char __fastcall__ on_pc (unsigned i);  /* (Part 10's stub: carried by the character) */
unsigned __fastcall__ at_hand (const char* name);   /* (Part 4's stub: to hand, carried or within reach) */
void __fastcall__ put (const char* s);          /* (tlist.s's: the hub's text, the words put after what's there) */
extern char* tp;
extern char* tpe;
extern unsigned char put_bank;

#define WADE_FT         3                       /* (Waist-deep: wading hard, the house's) */
#define SWIM_FT         5                       /* (Over the head: swimming, the book's "deeper than their height",
                                                **   the house's five feet) */
#define MEAL            480                     /* (A meal's want taken off: eight hours') */
#define PINT            360                     /* (A pint's: six hours') */

#pragma code-name (push, "SW72CODE")
#pragma rodata-name (push, "SW72RODATA")
#pragma local-strings (push, on)

/* An event of the character's, the world's (E_BEAT's w1) */
static void __fastcall__ tell (unsigned w)
{
    struct msg m;

    hub_map (0);
    m.kind = E_BEAT;
    m.who = 0;
    m.a = m.b = 0;
    m.w1 = w;
    m.t = HUB->now;
    hub_event (&m);
}

/* ---- The water at a square (the chunk's: world.h's): its depth in feet (0 none), | WET_RUNS running water; its
** chunk in the window after */
static const unsigned char bit[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };

static unsigned char __fastcall__ water_at (unsigned char sn, unsigned x, unsigned y)
{
    unsigned char r;
    signed char l, e;

    if (!at (sn, x, y) || !(C->wet[cy_][cx_ >> 3] & bit[cx_ & 7])) {
        return 0;
    }
    e = C->elev[cy_][cx_];
    r = room_of (sn, x, y);
    l = C->wlev[r = r == 0xFF ? NROOMS : r];
    if (C->wruns & bit[r]) {
        return (unsigned char) l | WET_RUNS;
    }
    return l > e ? l - e : 0;
}

/* The water's band (WB_WATER's): 0 dry, 1 ankle to knee, 2 waist to chest, 3 over the head */
static unsigned char __fastcall__ band (unsigned char d)
{
    d &= 0x7F;
    return !d ? 0 : d < WADE_FT ? 1 : d < SWIM_FT ? 2 : 3;
}

/* ---- The character come onto its square as it walks (part 23's step_on, as its chunk has water or it was in water):
** the water's told as it comes into it, deeper, shallower, out of it (WB_WATER, its band times 256, | 0x8000
** running); the time more its step takes (tenths: the walk's step its pace's, step): wading a foot or two, half again;
** waist-deep, twice; out of its depth, swimming, a minute's swim's squares (the referee's P_SWIM, tested as it begins
** and each minute after: its grade's distance, a minute's strenuous work, its flames put out as it begins); sinking,
** under for the minute, its breath held (the referee's: its seconds), past it drowned (the bodies' time, part 70's:
** dead of it) */
static unsigned __fastcall__ wade (unsigned step)
{
    struct req* r;
    unsigned char d, was, sq, b;
    unsigned i;

    hub_map (0);
    d = water_at (HUB->pc_stratum, HUB->pc_x, HUB->pc_y);
    hub_map (0);
    was = HUB->pc_wet;
    HUB->pc_wet = d;
    if ((b = band (d)) != band (was)) {
        tell (WB_WATER | b << 8 | (d & WET_RUNS ? 0x8000 : 0));
    }
    if (b < 3) {
        HUB->pc_swim = HUB->pc_under = 0;
        return b == 2 ? step : b ? step / 2 : 0;
    }
    if (!HUB->pc_swim || (int) ((unsigned) HUB->now - HUB->pc_swat) >= 0) {
        if (!HUB->pc_swim) {                    /* (Its flames out as it swims: HOUSE.md's) */
            for (i = 0, b = 0; (i = items (i, IN_ACTOR | IN_ITEM | IN_FLAGS, I_LIT)) != 0; ) {
                if (on_pc (i)) {
                    item_at (i)->flags &= ~I_LIT;
                    b = 1;
                }
            }
            if (b) {
                light_now ();
                tell (WB_DOUSED);
            }
        }
        hub_map (0);
        r = &HUB->slot[SLOT_WORLD];
        r->kind = RS_TEST;
        r->actor = r->b = 0;
        r->a = P_SWIM;
        r->steps = d & WET_RUNS ? -4 : 0;
        r = hub_ask (SLOT_WORLD);
        b = r->grade;
        sq = r->w1 > 60 ? 60 : (unsigned char) r->w1;
        HUB->pc_breath = r->w2 > 255 ? 255 : (unsigned char) r->w2;
        r->kind = RS_EXERT;                     /* (A minute's strenuous work; under, three times the book's) */
        r->a = GA_WORK;
        r->w1 = sq ? 600 : 1800;
        r = hub_ask (SLOT_WORLD);
        r->state = REQ_FREE;
        HUB->pc_swat = (unsigned) HUB->now + 600;
        if (!sq) {                              /* (It sinks: under for the minute, the book's) */
            HUB->pc_swim = 1;
            if ((HUB->pc_under += 60) > HUB->pc_breath || HUB->pc_under < 60) {
                HUB->pc_under = 0xFF;           /* (Drowned: the bodies' time tells the referee, part 70's) */
                body_ask ();
                return 0;
            }
            tell (WB_UNDER);
        } else {
            if (HUB->pc_under || !HUB->pc_swim) {
                tell (WB_SWIM | (G_OK (b) ? 0x100 : 0) | (HUB->pc_under ? 0x200 : 0));
            }
            HUB->pc_under = 0;
            HUB->pc_swim = sq;
        }
    }
    i = 600 / HUB->pc_swim;
    return i > step ? i - step : 0;
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW72CODE")
#pragma rodata-name (push, "SW72RODATA")
#pragma local-strings (push, on)

/* ---- Hunger and thirst (HOUSE.md's, every ruleset's), the bodies' time's (part 70's s_body_ask: n minutes gone, a
** what the character was about, 2 vigorous: twice the want): their levels anew, warned as either rises (WB_WANT, the
** levels that rose times 256), the hub's pc_want, and when they could next change (pc_wdue: the lights' minute asks the bodies'
** time then, part 27's); the referee's request's x[1] its fatigue from want, x[2] dead of it (1 thirst, 2 hunger, 3
** drowned); 1 if the referee's to be told */
static const unsigned hl[5] = { 480, 1440, 4320, 14400, 43200U };    /* (Hungry 8 hours, very 24, starving 3 days,
                                                **   dying 10, dead 30) */
static const unsigned tl[5] = { 480, 960, 1440, 2880, 4320 };   /* (Thirsty 8 hours, very 16, parched 24, dying 48,
                                                **   dead 72) */
static const unsigned char wfat[6] = { 0, 0, 1, 2, 4, 4 };     /* (Fatigue levels by a want's level) */

static unsigned char __fastcall__ level (unsigned m, const unsigned* l)
{
    unsigned char k;

    for (k = 0; k < 5 && m >= l[k]; ++k) {
    }
    return k;
}

static unsigned __fastcall__ till (unsigned m, const unsigned* l)
{
    unsigned char k = level (m, l);

    return k < 5 ? l[k] - m : 0xFFFF;
}

static unsigned char __fastcall__ want_time (unsigned n, unsigned char a)
{
    struct req* r;
    struct msg m;
    unsigned char w, o;
    unsigned t, u;

    hub_map (0);
    if (a == 2) {
        n += n;                                 /* (Six thousand minutes at most: part 70's) */
    }
    HUB->pc_hunger = HUB->pc_hunger + n < n ? 0xFFFF : HUB->pc_hunger + n;
    HUB->pc_thirst = HUB->pc_thirst + n < n ? 0xFFFF : HUB->pc_thirst + n;
    w = level (HUB->pc_hunger, hl) | level (HUB->pc_thirst, tl) << 4;
    t = till (HUB->pc_hunger, hl);
    u = till (HUB->pc_thirst, tl);
    t = ((t < u ? t : u) >> 1) + 1;
    HUB->pc_wdue = HUB->now + (unsigned long) t * 600;
    r = &HUB->slot[SLOT_WORLD];
    r->x[1] = wfat[WANT_HUNGER (w)] + wfat[WANT_THIRST (w)];
    r->x[2] = HUB->pc_under == 0xFF ? 3 : WANT_HUNGER (w) >= WANT_DEAD ? 2 : WANT_THIRST (w) >= WANT_DEAD;
    if ((o = HUB->pc_want) == w) {
        return HUB->pc_under == 0xFF;
    }
    HUB->pc_want = w;
    if (WANT_HUNGER (w) > WANT_HUNGER (o) || WANT_THIRST (w) > WANT_THIRST (o)) {
        m.kind = E_BEAT;                        /* ("You're getting thirsty.") */
        m.who = m.a = m.b = 0;
        m.w1 = WB_WANT | (w & (WANT_HUNGER (w) > WANT_HUNGER (o) ? 0x0F : 0) | w & (WANT_THIRST (w) > WANT_THIRST (o) ?
            0xF0 : 0)) << 8;                    /* (Only what rose: its level) */
        m.t = HUB->now;
        hub_event (&m);
    }
    return 1;
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

#pragma code-name (push, "SW72CODE")
#pragma rodata-name (push, "SW72RODATA")
#pragma local-strings (push, on)

/* ---- Eating, drinking, a waterskin filled (the things' IO_EAT, IO_DRINK, IO_FILL: the hub's ireq, its name; the
** words in the UI bank's things_text, the time in its x, tenths): IR_OK, IR_NO refused (its words why), IR_NONE (a
** fill that isn't a waterskin's: a light's, part 27's) */
static unsigned char __fastcall__ wsaid (const char* s, unsigned t)
{
    put_bank = UI_BANK;
    hub_map (UI_BANK);
    tp = UIREC->things_text;
    tpe = tp + sizeof UIREC->things_text - 1;
    *tp = 0;
    put (s);
    hub_map (0);
    HUB->ireq.x = t;
    HUB->ireq.qty = 0;
    if (!t) {
        return IR_NO;
    }
    body_ask ();                                /* (The want's levels anew: the referee told) */
    return IR_OK;
}

/* The water where the character stands (the hub's pc_wet: in it, part 72's), or a well within reach: 1 if there's some
** to drink, to fill a waterskin from */
static unsigned char water_here (void)
{
    hub_map (0);
    return HUB->pc_wet || at_hand ("well");
}

/* The water in waterskin c (0 none: its pints, any record of it, empty or not) */
static struct item* __fastcall__ water_in (unsigned c)
{
    return items (0, IN_ITEM | IN_X, c) && ip->kind == KI_WATER ? ip : 0;
}

static unsigned char __fastcall__ water_op (unsigned char op)
{
    char name[16];                              /* (On the C stack: the main RAM's room) */
    unsigned sk;
    unsigned char n;
    struct item* p;

    hub_map (0);
    memcpy (name, HUB->ireq.name, sizeof name);
    if (op == IO_EAT) {                         /* (A meal: a third of a day's rations, the house's) */
        if (HUB->pc_hunger < MEAL / 4) {
            return wsaid ("You're not hungry.", 0);
        }
        if (!HUB->pc_meals) {                   /* (A new day's ration begun: one of the lot used up by its key,
                                                **   its last told, part 7's) */
            if ((*name && strcmp (name, "rations") && strcmp (name, "food")) || use_key ("food") != IR_OK) {
                return wsaid ("You have nothing to eat.", 0);
            }
            hub_map (0);
            HUB->pc_meals = 3;
        }
        hub_map (0);
        --HUB->pc_meals;
        HUB->pc_hunger = HUB->pc_hunger > MEAL ? HUB->pc_hunger - MEAL : 0;
        return wsaid ("You eat a meal.", 3000);    /* (Five minutes a meal) */
    }
    sk = at_hand (*name ? name : "waterskin");
    if (sk && item_at (sk)->kind != KI_WATERSKIN) {
        sk = 0;
    }
    if (op == IO_FILL) {
        if (!sk) {
            return IR_NONE;                     /* (A light's: part 27's) */
        }
        if (!water_here () || !(p = water_in (sk))) {
            return wsaid ("There's no water here.", 0);
        }
        hub_map (KIND_BANK);
        n = CATALOGUE[KI_WATERSKIN].cap_bulk;
        water_in (sk)->qty = n;
        return wsaid ("You fill your waterskin.", 600);
    }
    hub_map (0);
    if (HUB->pc_thirst < PINT / 6) {
        return wsaid ("You're not thirsty.", 0);
    }
    n = (unsigned char) ((HUB->pc_thirst + PINT - 1) / PINT);
    if ((!*name || !sk) && water_here ()) {     /* (The water where it is: its fill) */
        hub_map (0);
        HUB->pc_thirst = 0;
        return wsaid ("You drink your fill.", n * 300);  /* (Half a minute a pint) */
    }
    if (!sk || !(p = water_in (sk)) || !p->qty) {
        return wsaid ("You have nothing to drink.", 0);
    }
    n = n < p->qty ? n : p->qty;
    p->qty -= n;                                /* (Empty, its record kept for its filling) */
    hub_map (0);
    HUB->pc_thirst = HUB->pc_thirst > n * PINT ? HUB->pc_thirst - n * PINT : 0;
    return wsaid ("You drink from your waterskin.", n * 300);
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- The part's one way in (its stub, water: the main RAM's room): k what's wanted (W_WADE, the character come onto
** its square, w its step; W_WANT, w the minutes gone, | its activity times 16384; W_OP, w the things' request's kind) */
#pragma code-name (push, "SW72CODE")
#pragma rodata-name (push, "SW72RODATA")

unsigned __fastcall__ s_water (unsigned char k, unsigned w)
{
    return k == W_WADE ? wade (w) : k == W_WANT ? want_time (w & 0x3FFF, w >> 14) : water_op ((unsigned char) w);
}

#pragma code-name (pop)
#pragma rodata-name (pop)
