/*
** want.c - (the house's rules, ../rshouse.h's) hunger and thirst: the want's levels by the time gone, what's eaten and
** drunk; its words the ruleset's want_say's
*/

#include "../hub.h"
#include "../rs.h"
#include "../referee.h"
#include "../rshouse.h"

static const unsigned want_hl[5] = { 480, 1440, 4320, 14400, 43200U };   /* (Hungry 8 hours, very 24, starving 3
                                                **   days, wasting 10, dead 30) */
static const unsigned want_tl[5] = { 480, 960, 1440, 2880, 4320 };      /* (Thirsty 8 hours, very 16, parched 24,
                                                **   dying 48, dead 72) */
static const unsigned char want_fat[6] = { 0, 0, 1, 2, 4, 4 };  /* (Fatigue levels by a want's level) */
static const char* const want_words[10] = {
    " You're getting hungry.", " You're very hungry.", " You're starving.", " You're wasting away with hunger.",
    " You starve to death.",
    " You're getting thirsty.", " You're very thirsty.", " You're parched.", " You're dying of thirst.",
    " You die of thirst." };

static unsigned want_till;                      /* (The minutes till the next level, either's) */

static unsigned char __fastcall__ want_level (unsigned m, const unsigned* l)
{
    unsigned char k;

    for (k = 0; k < 5 && m >= l[k]; ++k) {
    }
    if (k < 5 && l[k] - m < want_till) {
        want_till = l[k] - m;
    }
    return k;
}

static unsigned __fastcall__ want_add (unsigned v, unsigned n)
{
    return v + n < v ? 0xFFFF : v + n;
}

static unsigned __fastcall__ want_less (unsigned v, unsigned n)
{
    return v > n ? v - n : 0;
}

unsigned char __fastcall__ want_of (struct req* r, unsigned long now)
{
    unsigned char* x = RS_TIME_WANT (*r);
    unsigned h = x[0] | x[1] << 8, t = x[2] | x[3] << 8, n = r->x[16] | r->x[17] << 8;
    unsigned char w, o = x[4], k, d = 0;
    unsigned long due;

    if (RS_TIME_ABOUT (*r) == 2) {
        n = want_add (n, n);                    /* (Vigorous work: twice the want) */
    }
    h = want_add (h, n);
    t = want_add (t, n);
    if (RS_TIME_RATION (*r)) {
        x[5] = 3;                               /* (A day's ration begun: three meals) */
    }
    for (k = RS_TIME_MEALS (*r); k; --k) {
        h = want_less (h, WANT_MEAL);
        x[5] -= x[5] != 0;
    }
    for (k = RS_TIME_DRINK (*r); k; --k) {
        t = want_less (t, WANT_PINT);
    }
    want_till = 0xFFFF;
    w = want_level (h, want_hl) | want_level (t, want_tl) << 4;
    if ((w & 15) > (o & 15)) {
        want_say (want_words[(w & 15) - 1]);
        RS_TIME_NEWS (*r) |= 2 | 8;             /* (News; a level risen: the world's long task asked, soft) */
    }
    if ((w >> 4) > (o >> 4)) {
        want_say (want_words[(w >> 4) + 4]);
        RS_TIME_NEWS (*r) |= 2 | 8;
    }
    if ((w & 15) == 5 || (w >> 4) == 5) {
        d = WANT_DIES;
    }
    due = now + (unsigned long) ((want_till >> 1) + 1) * 600;
    if (due < RS_TIME_NEXT (*r)) {
        RS_TIME_NEXT (*r) = due;                /* (Asked again as its next level could come) */
    }
    x[0] = (unsigned char) h;
    x[1] = h >> 8;
    x[2] = (unsigned char) t;
    x[3] = t >> 8;
    x[4] = w;
    return want_fat[w & 15] + want_fat[w >> 4] | d;
}
