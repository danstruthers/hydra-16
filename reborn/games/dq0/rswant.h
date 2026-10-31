/*
** rswant.h - hunger and thirst (slice 4.9: HOUSE.md's water, hunger and thirst, every ruleset's house rules), the
** referee's to keep, each referee including it where its room is (dq0harn's overlay 7, want.c; dq0null's main), with
** WANT_SAY (s) its way to put words in its news (RS_DESCRIBE b 4's, the bodies' time's).  Asked with the bodies' time
** (RS_TIME, the world's: w1 the minutes gone, a what the character was about, 2 vigorous work: twice the want) and as it
** eats and drinks (RS_TIME, the UI's: w1 0, x[14] the meals eaten, | 0x80 a day's ration begun, x[15] the pints drunk);
** the hub's want in x[8..13] (pc_hunger, pc_thirst, pc_want, pc_meals: the referee's own, copied in and back by its main
** code), anew; its levels' rising told; the clock's next ask (the reply's x[4..7]) the sooner for its next level.
** want_of's result: the fatigue levels from want, | WANT_DIES dead of it (its words told)
*/

#define WANT_X          8                       /* (The request's x: the hub's want, from pc_hunger on) */
#define WANT_DIES       0x80
#define WANT_MEAL       480                     /* (A meal: eight hours of hunger off) */
#define WANT_PINT       360                     /* (A pint: six hours of thirst off) */

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

static unsigned char __fastcall__ want_of (struct req* r, unsigned long now)
{
    unsigned char* x = r->x + WANT_X;
    unsigned h = x[0] | x[1] << 8, t = x[2] | x[3] << 8, n = r->w1;
    unsigned char w, o = x[4], k, d = 0;
    unsigned long due;

    if (r->a == 2) {
        n = want_add (n, n);                    /* (Vigorous work: twice the want) */
    }
    h = want_add (h, n);
    t = want_add (t, n);
    if (r->x[14] & 0x80) {
        x[5] = 3;                               /* (A day's ration begun: three meals) */
    }
    for (k = r->x[14] & 0x7F; k; --k) {
        h = want_less (h, WANT_MEAL);
        x[5] -= x[5] != 0;
    }
    for (k = r->x[15]; k; --k) {
        t = want_less (t, WANT_PINT);
    }
    want_till = 0xFFFF;
    w = want_level (h, want_hl) | want_level (t, want_tl) << 4;
    if ((w & 15) > (o & 15)) {
        WANT_SAY (want_words[(w & 15) - 1]);
    }
    if ((w >> 4) > (o >> 4)) {
        WANT_SAY (want_words[(w >> 4) + 4]);
    }
    if ((w & 15) == 5 || (w >> 4) == 5) {
        d = WANT_DIES;
    }
    due = now + (unsigned long) ((want_till >> 1) + 1) * 600;
    if (due < *(unsigned long*) (r->x + 4)) {
        *(unsigned long*) (r->x + 4) = due;     /* (Asked again as its next level could come) */
    }
    x[0] = (unsigned char) h;
    x[1] = h >> 8;
    x[2] = (unsigned char) t;
    x[3] = t >> 8;
    x[4] = w;
    return want_fat[w & 15] + want_fat[w >> 4] | d;
}
