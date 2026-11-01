/*
** wtime.c - (dq0w, swapped part 76: ../dq0/swap.inc) the character's time of day believed (DUNGEON.md's the time of day
** believed, HOUSE.md's, for review; the user's, 2026-10-31: "Timekeeping should also be a fuzzy thing, unless there is
** some place where you can actually see outside ... especially after sleeping the current time-of-day should be quite
** arbitrary to the player, and should drift further and further away from known as the play progresses", "Time-of-day
** would be corrected whenever the sun can be seen ... and the direction is known, or the player spends some time
** watching it").  The world keeps the true clock (the hub's now, and the hour the game began: the denizens' days and
** whatever happens go by it); the character's own the hub's tb: the hour it believes it is against the true one (skew,
** in minutes) and how sure it is (unc, minutes either way), as the bearings are a belief about north.
**
** Reckoned lazily, at the lights' burning's look (part 27's s_burns_now: its due, tb.due, or at once, burn_ask: a
** light's change, a wait or a sleep begun or ended, a black-out, another stratum, the way out opened), never polled:
**   - the hours awake: the referee's P_TIME each hour (the hours since the last, at once), its failure's drift its
**     way (the referee's d8: odd the earlier), the more the longer since it last knew the time;
**   - a sleep or a black-out: its length guessed as it wakes, as it comes round (the referee's P_TIME, the harder the
**     longer: a failure's drift and doubt a share of the length; a critical failure, no idea);
**   - the clues that narrow it: its own light burning down (it sees its torch: the minutes it's burned since its belief
**     was set by it, to a twelfth), its hunger since its last meal (to a quarter);
**   - the sky in sight (an open way out near it: the top stratum's up its stairs, its light alone; a far exit below
**     the top, the open sky; or a test's poke, tb.sky): its light (dawn, day, dusk, night) narrows it; the sun or the
**     stars read by the north it believes in set it (wrong by its bearings' error, unknown to it); its bearings lost,
**     the sun high or low; watched a while (a wait in its sight: SKY_WATCH minutes, the stars STARS_WATCH), their way
**     across the sky shows the time and north both (its bearings set right).
** One with absolute timesense (the referee's summary's timesense) always knows the hour.  The UI words it ("time")
** and waits "until dawn" by it (time.c's).
*/

#include <stddef.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "world.h"
#include "things.h"

signed char __fastcall__ ask_w (unsigned char kind, unsigned char a, unsigned char b);  /* (dq0w.c's) */
extern unsigned char wres;
extern signed char wsteps;

#pragma code-name ("SW76CODE")
#pragma rodata-name ("SW76RODATA")
#pragma static-locals (on)
#pragma local-strings (on)

#define TB_START        18                      /* (The hour the game begins, as the denizens': at dusk, dq0d.c's) */
#define SKY_NEAR        2                       /* (Squares from an open way out the sky's seen: its light) */
#define SKY_WATCH       10                      /* (Minutes the sun's watched to see its way across the sky; */
#define STARS_WATCH     20                      /*   the stars') */
#define T               (HUB->tb)

static unsigned m;                              /* (The true minute of the day, now) */
static long lo, hi;

/* A minute's offset brought to -720 on, under 720 */
static int __fastcall__ norm (long v)
{
    while (v >= 720) {
        v -= 1440;
    }
    while (v < -720) {
        v += 1440;
    }
    return (int) v;
}

/* The belief set: skew s, unc u */
static void __fastcall__ set (int s, unsigned u)
{
    T.skew = s;
    T.unc = u;
}

/* The test (the referee's P_TIME, at steps): its grade; d its drift's minutes, its way the referee's (odd the earlier) */
static long d;
static signed char __fastcall__ ttest (signed char steps)
{
    signed char g;

    wsteps = steps;
    g = ask_w (RS_TEST, P_TIME, 0);
    hub_map (0);
    return g;
}
static void drift (void)
{
    T.skew = norm (T.skew + (wres & 1 ? -d : d));
}

/* The hours awake (T.mins of them, an hour's test each, those gone by at once): a failure's drift and doubt the more,
** the longer since it last knew the time (each four hours one more of its steps, four at most) */
static void hours (void)
{
    static const unsigned char off[4] = { 15, 8, 3, 0 }, doubt[4] = { 20, 10, 5, 3 };  /* (Minutes an hour: CF, MF,
                                                **   MS, CS) */
    unsigned n = T.mins / 60, k;
    signed char g;

    if (!n) {
        return;
    }
    T.mins -= n * 60;
    g = ttest (0);
    k = T.hours / 4 + 1;
    T.hours += n;
    n *= k > 4 ? 4 : k;
    g = g < 0 ? g + 2 : g + 1;                  /* (0 CF, 1 MF, 2 MS, 3 CS) */
    d = (long) off[g] * n;
    drift ();
    T.unc += doubt[g] * n;
}

/* Woken, or come round (T.gone, T.how): the time it was gone guessed, the harder the longer (an hour two steps), a
** black-out's ten the harder (it can't tell how long it lay there): a critical success near enough, a marginal success
** off by a sixth of it and doubting a quarter, a marginal failure a third and a half, a critical failure two thirds,
** and no idea at all */
static void woke (void)
{
    unsigned len = T.gone == 1 ? 60 : (unsigned) ((HUB->now - T.gone) / 600), u;   /* (1: come round from
                                                **   a fall as the game begins, an hour out, the house's) */
    signed char g;

    g = ttest (T.how == TG_OUT ? -10 : -(signed char) (len / 30 > 20 ? 20 : len / 30));
    u = g == G_CS ? len / 8 : g == G_MS ? len / 4 : g == G_MF ? len / 2 : len;
    d = g == G_CS ? 0 : g == G_MS ? len / 6 : g == G_MF ? len / 3 : len * 2UL / 3;
    if (T.how == TG_OUT && g < 0) {
        d += 30;                                /* (A black-out: an hour, or a minute? at least half an hour out) */
    }
    drift ();
    T.unc += u < 15 ? 15 : u;
    if (g == G_CF && T.unc < TB_LOST) {
        T.unc = TB_LOST;                        /* (No idea how long it was gone: any hour) */
    }
    T.gone = 0;
}

/* A clue's belief (its skew s, its doubt u), taken if it's surer than the one it has */
static void __fastcall__ clue (int s, unsigned u)
{
    if (u < T.unc) {
        set (s, u);
    }
}

/* The clues: its own light burning down (the minutes it's burned since its belief was set by it, read to a twelfth;
** burnt out at the light's change, the whole of it), and its hunger since its meal (to a quarter) */
static void clues (unsigned now)
{
    unsigned f, b, e;

    f = HUB->pc_light ? HUB->pc_fuel : 0;
    e = now - T.lat;
    if (f && f != 0xFFFF) {
        b = T.lfuel - f;
        if (T.lfuel != TB_NONE && f <= T.lfuel && b + 2 >= e) {
            clue (T.lskew, T.lunc + b / 12);    /* (The same light, burning all along) */
        } else {
            T.lfuel = f;                        /* (A light of its own lit: its belief set by it from now) */
            T.lat = now;
            T.lskew = T.skew;
            T.lunc = T.unc;
        }
    } else if (T.lfuel != TB_NONE) {
        if (e + 2 >= T.lfuel && e <= T.lfuel + 2) {
            clue (T.lskew, T.lunc + T.lfuel / 12);  /* (Burnt out, the whole of it) */
        }
        T.lfuel = TB_NONE;
    }
    f = HUB->pc_hunger;
    if (f < T.hung) {
        T.ahung = f;                            /* (It's eaten: its belief set by its meal) */
        T.askew = T.skew;
        T.aunc = T.unc;
    } else if (T.ahung != TB_NONE) {
        clue (T.askew, T.aunc + (f - T.ahung) / 4);
    }
    T.hung = f;
}

/* The belief narrowed to the true time lying between minutes lo and hi of the day (the sky's light): where its own
** doubt and that meet, or that alone if they don't */
static void narrow (void)
{
    long a = -(long) ((m + 1440 - lo) % 1440), b = (hi + 1440 - m) % 1440, l = (long) T.skew - T.unc,
        h = (long) T.skew + T.unc;

    if (l < a) {
        l = a;
    }
    if (h > b) {
        h = b;
    }
    if (l > h) {
        l = a;
        h = b;
    }
    set ((int) ((l + h) / 2), (unsigned) ((h - l) / 2));
}

/* The sky in sight (SKY_*): a test's poke; else an open way out within SKY_NEAR squares on the character's stratum (the
** top's up its stairs, its light alone; a far exit below the top, out into the open).  T.skysn the stratum, if it has
** one (its looks the more often) */
static unsigned char sky_here (void)
{
    struct link* l;
    unsigned char sn = HUB->pc_stratum, k, s = SKY_NONE;

    T.skysn = 0xFF;
    for (k = 0; k < STRATUM (sn).nlinks; ++k) {
        l = &STRATUM (sn).link[k];
        if (l->kind == O_WAY_OUT && (HUB->way_open || sn)) {
            T.skysn = sn;
            if (l->x - HUB->pc_x + SKY_NEAR <= 2 * SKY_NEAR && l->y - HUB->pc_y + SKY_NEAR <= 2 * SKY_NEAR) {
                s = sn ? SKY_OPEN : SKY_LIGHT;
            }
        }
    }
    hub_map (0);
    return T.sky ? T.sky : s;
}

/* The sky seen (kind): the time it believes set or narrowed by it (the file's head), told as what's seen changes */
static void __fastcall__ sky_seen (unsigned char kind)
{
    static struct msg e;
    unsigned char p, night;
    signed char s;

    p = m < 300 || m >= 1140 ? SKP_NIGHT : m < 420 ? SKP_DAWN : m < 1020 ? SKP_DAY : SKP_DUSK;
    lo = p == SKP_NIGHT ? 1140 : p == SKP_DAWN ? 300 : p == SKP_DAY ? 420 : 1020;
    hi = p == SKP_NIGHT ? 300 : p == SKP_DAWN ? 420 : p == SKP_DAY ? 1020 : 1140;
    night = p == SKP_NIGHT;
    if (kind != SKY_OPEN) {
        narrow ();                              /* (Its light alone: dawn, day, dusk or night) */
    } else if (T.watched >= (night ? STARS_WATCH : SKY_WATCH)) {
        set (0, night ? 30 : 15);               /* (Watched: their way across the sky, the time and north) */
        HUB->pc_skew = HUB->pc_lost = 0;
        p = night ? SKP_STARS_WATCHED : SKP_SUN_WATCHED;
    } else if (!HUB->pc_lost) {
        s = HUB->pc_skew & 7;                   /* (Read by the north it believes in: an eighth off, three hours) */
        set ((s > 3 ? s - 8 : s) * 180, night ? 60 : 30);
        p = night ? SKP_STARS : p == SKP_DAY ? SKP_SUN : p;
    } else if (p == SKP_DAY) {                  /* (Its bearings lost: the sun high, or low, morning or afternoon by
                                                **   what it believes) */
        if (m >= 600 && m < 840) {
            lo = 600;
            hi = 840;
            p = SKP_HIGH;
        } else {
            lo = (m + T.skew + 1440) % 1440 < 720 ? 420 : 840;
            hi = lo + 180;
            p = SKP_LOW;
        }
        narrow ();
    } else {
        narrow ();
    }
    T.hours = 0;
    T.how = TG_HOURS;
    if (p != T.told) {
        T.told = p;
        e.kind = E_BEARING;
        e.who = 0;
        e.b = p;
        e.w1 = BRG_SKY;
        e.t = HUB->now;
        hub_event (&e);
    }
}

/* The hour the game began (a new game's first look): the environment's dq0hour, as the denizens' (a test's scene),
** else dusk */
static void begun (void)
{
    static struct hy_regs r;
    static char b[4];
    char* p;
    unsigned char h = TB_START;

    r.a = 0xFF;
    r.r[0] = (unsigned) "dq0hour";
    r.r[1] = (unsigned) b;
    r.r[2] = 3;
    r.r[3] = 0;
    if (!hy_call (HY_ENV_GET, &r)) {
        for (h = 0, p = b; *p >= '0' && *p <= '9'; ++p) {
            h = h * 10 + *p - '0';
        }
    }
    hub_map (0);
    T.hour0 = h;
    T.lfuel = T.ahung = TB_NONE;
    T.hung = HUB->pc_hunger;
    hub_map (THEME_BANK);
    h = THEME->mission & TM_FELL;
    hub_map (0);
    if (h) {
        T.gone = 1;                             /* (Come round below from a fall: how long it lay there, guessed) */
        T.how = TG_OUT;
    }
}

/* The reckoning (part 27's, at its due or at once): the time it believes brought up to now, its next due */
void s_tsense (void)
{
    unsigned long now;
    unsigned e, n;
    unsigned char h, s;

    hub_map (0);
    now = HUB->now;
    if (!T.at) {
        begun ();
        T.at = now;
    }
    e = (unsigned) ((now - T.at) / 600);
    T.at += e * 600UL;
    n = (unsigned) (now / 600);
    h = T.hour0 & 0x80 ? TB_START : T.hour0;
    m = (unsigned) ((n + h * 60UL) % 1440);
    if (HUB->pcsum.timesense) {
        set (0, 0);                             /* (Absolute timesense: it always knows) */
        T.gone = 0;
    } else if (HUB->pc_task.kind == LT_SLEEP || (HUB->pcsum.cond & C_UNCONSCIOUS)) {
        if (!T.gone) {
            T.gone = now;                       /* (Asleep, or senseless: from now, as near as it's seen) */
            T.how = HUB->pc_task.kind == LT_SLEEP ? TG_SLEEP : TG_OUT;
        }
        T.due = now + 600;                      /* (Looked at a minute on, the burning's next: to see it wake) */
        return;
    } else if (T.gone) {
        woke ();
    } else {
        T.mins += e;
        hours ();
    }
    clues (n);
    s = sky_here ();
    if (s && !HUB->pcsum.timesense) {
        n = T.watched + (e > 60 ? 60 : e);
        T.watched = HUB->pc_task.kind == LT_WAIT && T.told ? (n > 200 ? 200 : n) : 0;
        sky_seen (s);
    } else {
        T.told = T.watched = 0;
    }
    if (T.unc > 720) {
        T.unc = 720;
    }
    T.due = now + (T.skysn == HUB->pc_stratum ? 3000 : (60 - T.mins % 60) * 600UL);
}
