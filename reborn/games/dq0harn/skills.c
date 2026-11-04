/*
** skills.c - (dq0harn, overlay 7) the character's skills as it goes (slice 4.4: HOUSE.md's skills by use, the book's
** Skills 7 and Combat 19), and its load.  A skill used (a test of it, test_sk's; its weapon in a fight, a block, a
** dodge, a shot: gear.c's, missile.c's) earns development rolls, success or failure alike (the book's: one learns as
** much from failing): a use under stress one, a very dangerous one two (the book's Skills 7, its stress rolls), and five
** routine uses one (the user's, 2026-11-03: "Stress, or 5 routine uses": HOUSE.md's); one of the book's [First Use]
** skills not opened yet is opened at its opening level the first time it's used, and told (the user's, 2026-10-23:
** those learnt by study aren't).  The rolls earned are made as a night's rest is had (eight hours of it, the house's),
** each that rose told: news, RS_DESCRIBE b 4's, as it wakes.  The load: its free pounds (D13, the user's: ten at Strength 10, doubling each four points) not counted in
** the book's encumbrance.
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")
#pragma local-strings (on)

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

#define RS_HOUSE_QUIET                          /* (The rolls' own dice, the game's put back after: QUIET_IN) */
#include "../dq0/rshouse.h"

#define NIGHT           480                     /* (A night's rest, minutes: the book's night's sleep, Skills 6) */

/* Each skill's growth (a byte a skill, kept past the bodies in their bank, fb, none of it in the main RAM): its routine
** uses toward a roll (the low three bits, 0-4), the rolls it's earned (the high four, at most ROLLS: the house's, the
** book's two or three) */
#define GROW            ((unsigned char*) 0x8000 + BBODIES)
#define ROLLS           3
#define ROUTINE         5
static unsigned dev_m;                          /* (Its rest toward the rolls, minutes) */
static unsigned char gv;

static void __fastcall__ grow_in (unsigned char sk)
{
    bread (fb, GROW + sk, &gv, 1);
}

static void __fastcall__ grow_out (unsigned char sk)
{
    bwrite (fb, GROW + sk, &gv, 1);
}

/* Words onto news (heal.c's nput, this overlay's own) */
static void __fastcall__ say (const char* s)
{
    unsigned char n = strlen (news);

    if (n + strlen (s) >= NEWS_N - 1) {
        n = 0;                                  /* (Full, its words not told yet: the latest kept) */
    }
    strcpy (news + n, s);
}

/* A skill's words, its mastery after them: " You've begun to learn Swimming by doing it (43)." */
static void __fastcall__ say_skill (const char* a, unsigned char sk, const char* b)
{
    char v[6];

    say (a);
    say (harn_skill_name (sk));
    say (b);
    say (utoa (pc.ml[sk], v, 10));
    say (").");
}

/* Skill sk used by the character: opened, if it's a [First Use] one (the book's: no months of teaching; Religion's
** rituals never), and told; if it's open, its growth: under stress (stress_lv, what the request's about, and wounded or
** in shock) a roll earned, under two at once a very dangerous use's two; else a routine use counted, the fifth a roll */
void __fastcall__ o_skill_used (unsigned char sk)
{
    const harn_skill* s;
    unsigned char n;

    if (sk >= HARN_MAXSKILLS) {
        return;                                 /* (An attribute's test) */
    }
    if (!pc.ml[sk]) {
        if (!(s = harn_get_skill_info (sk)) || s->months || s->group == HARN_RELIGION) {
            return;                             /* (Learnt by study: tried at its opening level, not opened) */
        }
        harn_open_skill (&pc, sk);
        say_skill (" You've begun to learn ", sk, " by doing it (");
        fresh = 1;                              /* (Told as soon as the world asks: cond_of's C_NEWS) */
    }
    n = stress_lv + (pc.ninj != 0 || pc.state == HARN_SHOCK);
    grow_in (sk);
    if (!n) {
        if ((gv & 7) < ROUTINE - 1) {
            ++gv;
        } else {
            gv &= 0xF0;
            n = 1;
        }
    } else if (n > 2) {
        n = 2;
    }
    n += gv >> 4;
    gv = (gv & 7) | (n > ROLLS ? ROLLS : n) << 4;
    grow_out (sk);
}

/* m minutes of rest (RS_TIME's, at rest: a wait, a sleep, standing still): a night's of it since the last, each skill's
** development rolls earned (the book's: d100 and its skill base over its mastery, a point more; not past 100 and its
** base), by their own dice, each that rose told; those rolls spent, its routine uses kept */
void __fastcall__ o_develop (unsigned m)
{
    unsigned char sk, ml, sb, n;

    dev_m = dev_m + m < dev_m ? 0xFFFF : dev_m + m;
    if (dev_m < NIGHT) {
        return;
    }
    dev_m = 0;
    QUIET_IN
    for (sk = 0; sk < HARN_MAXSKILLS; ++sk) {
        grow_in (sk);
        if (gv > 15) {
            for (n = gv >> 4; n; --n) {
                if ((ml = pc.ml[sk]) != 0) {
                    sb = harn_get_sb (&pc, sk);
                    if (ml < 100 + sb && d100 () + sb > ml) {
                        pc.ml[sk] = ml + 1;
                        say_skill (" Your ", sk, " skill has improved (");
                    }
                }
            }

            gv &= 7;
            grow_out (sk);
        }
    }
    QUIET_OUT
}

/* Its growth in the character's file, its "body u" lines (afflict.c's o_body_line's): rd 3 cleared, its rest none (a
** file about to be read); rd 1 a line read, l its words after "body ": "u", its rest toward the rolls, a space, then
** each skill's number and growth, two hex digits each; rd 2 the next line made in l (l[0] 0: no more) */
static const char hexd[] = "0123456789abcdef";
static unsigned char lsk;                       /* (The next skill to write; 0xFF no more) */

static unsigned char __fastcall__ hexv (const char* p)
{
    return (p[0] <= '9' ? p[0] - '0' : p[0] - 'a' + 10) << 4 | (p[1] <= '9' ? p[1] - '0' : p[1] - 'a' + 10);
}

void __fastcall__ o_skill_line (char* l, unsigned char rd)
{
    unsigned char k, n;

    if (rd == 3) {
        dev_m = gv = 0;
        for (k = 0; k < HARN_MAXSKILLS; ++k) {
            grow_out (k);
        }
    } else if (rd == 1) {
        dev_m = atoi (l + 1);
        if ((l = strchr (l, ' ')) != 0) {
            for (++l; *l > ' '; l += 4) {
                gv = hexv (l + 2);
                grow_out (hexv (l));
            }
        }
    } else if (lsk == 0xFF) {
        lsk = 0;
        l[0] = 0;
    } else {
        strcpy (l, "body u");
        utoa (dev_m, l + 6, 10);
        strcat (l, " ");
        l += strlen (l);
        for (n = 0, k = lsk; k < HARN_MAXSKILLS && n < 8; ++k) {
            grow_in (k);
            if (gv) {
                *l++ = hexd[k >> 4];
                *l++ = hexd[k & 15];
                *l++ = hexd[gv >> 4];
                *l++ = hexd[gv & 15];
                ++n;
            }
        }
        *l = 0;
        lsk = k < HARN_MAXSKILLS ? k : 0xFF;
    }
}

/* A tool to hand (on), or not: its key (RS_EQUIP a 1, 0: dtext's copy, the hub not in the window here) */
void __fastcall__ o_equip (const char* key, unsigned char on)
{
    unsigned char k;

    for (k = 0; k < NTOOLS; ++k) {
        if (on ? !tools[k][0] : !strcmp (tools[k], key)) {
            if (on) {
                strncpy (tools[k], key, sizeof tools[k] - 1);
                tflag[k] = 0;
            } else {
                tools[k][0] = 0;
            }
            return;
        }
    }
}

/* The load the character carries (the game's things, all told: quarter pounds), its equipment as harn.h has it, one
** entry: its whole weight (cost, tenths of a pound), and the part of it past its free pounds (wt: D13's, ten pounds at
** Strength 10, doubling each four points: 4 at 5, 14 at 12, 24 at 15, 40 at 18, 57 at 20, 134 at 25), so its
** encumbrance (EP, the load over endurance) is the book's on what it truly bears, and with it the physical penalty that
** tests of the body and its move are the worse for (Skills 6, Combat 6) */
static const unsigned char quarter[4] = { 100, 119, 141, 168 };    /* (Ten pounds times 2 to the n/4, tenths) */

void __fastcall__ o_set_load (unsigned qp)
{
    int e = (int) pc.att[HARN_STR] - 10;
    unsigned f, w = qp > 13000U ? 65000U : qp * 5 / 2;

    if (e > 24) {
        e = 24;
    }
    f = quarter[e & 3];
    for (; e < 0; e += 4) {                     /* (Halved, or doubled, each four points: no shifts by a number, */
        f >>= 1;                                /*   the main RAM's room) */
    }
    for (; e >= 4; e -= 4) {
        f <<= 1;
    }
    carried.cost = w;
    carried.wt = w > f ? w - f : 0;
    pc.eq[0] = &carried;
    pc.neq = 1;
}


/* A side's weapon's version (D13's, the house's, for review): its impacts by it, the book's custom weapons (Combat 3)
** at Strength 9-12 for every Strength (the user's: Strength adds nothing to impact): light 80%, average 100%, heavy
** 120%; two steps worse to strike and to block with it each version heavier than its Strength wields without penalty
** (HEFT_MAX: light at any, average from 6, heavy from 13).  The character's by its tools' versions (RS_EQUIP's), a
** creature's weapon average (the outfits' gear's versions to come), by its sheet's Strength; a part of it (fists,
** claws) none */
void __fastcall__ o_heft (struct side* s)
{
    static const unsigned char pct[3] = { 80, 100, 120 };
    unsigned char h, k;
    signed char over;

    if (s->part != 0xFF) {
        return;
    }
    h = !s->who && s->aslot < NTOOLS ? TQ_HEFT (tqual[s->aslot]) : 1;
    if ((over = h - HEFT_MAX (s->c->att[HARN_STR])) > 0) {
        s->aml -= 10 * over;
        s->dml -= 10 * over;
        for (k = 0; !s->who && k < s->nblk; ++k) {
            if (s->bslot[k] == s->aslot) {
                s->bml[k] -= 10 * over;
            }
        }
    }
    for (k = 0; h != 1 && k < 3; ++k) {
        if (s->imp[k] > 0) {
            s->imp[k] = (s->imp[k] * pct[h] + 50) / 100;
        }
    }
}
