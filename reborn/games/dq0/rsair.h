/*
** rsair.h - the air the character breathes (2026-10-31: HOUSE.md's bad air, every ruleset's house rules), the referee's
** to keep as rswant.h's hunger and thirst are, each referee including it where its room is (dq0harn's overlay 8,
** afflict.c; dq0null's main), with AIR_SAY (s) its way to put words in its news (RS_DESCRIBE b 4's, the bodies' time's)
** and AIR_LEVELS (n) the fatigue levels its breath costs it now (HarnMaster's: the book's universal penalty; the null
** ruleset's none).  Asked with the bodies' time (RS_TIME, the world's: a minute at a time while the character's in bad
** air or its head's not clear, x[20] the air where it stands, rs.h's AQ_*, x[21] its flame, FLM_*).
**
** Blackdamp (stale air, too little in it to breathe) is felt at once, the breath short, then gasping, the legs going,
** death; whitedamp (a fire's or a blast's smoke: poison, no warning) builds up in the blood, a dose a minute (its
** strength squared), the head aching, then dizzy and sick, then the legs going, death, and clears as slowly in fresh
** air; firedamp only chokes when it's thick.  A flame's way in it told as the character comes into it (the miners' old
** warnings): burning low, a blue-edged bead, guttering out in blackdamp; a pale blue cap over it in firedamp; nothing
** in whitedamp.  air_of's result: 1 dead of it, 2 news, 4 worse (a hard stop: its long task)
*/

static unsigned air_m;                          /* (Minutes in the blackdamp it's in) */
static unsigned air_co;                         /* (Whitedamp in its blood: the doses) */
static unsigned char air_lv;                    /* (Its breath's fatigue levels as last told) */

static const char* const air_flame[12] = {      /* (A flame's way, by the air's kind and strength: blackdamp 1-3,
                                                **   whitedamp's nothing, firedamp 1-3) */
    " Your flame burns low.", " Your flame shrinks to a blue-edged bead.", " Your flame gutters and goes out.",
    0, 0, 0,
    " Your flame lengthens, a pale blue cap trembling over it.", " Your flame leaps up, its blue cap tall.", 0,
    " Your flame brightens again.", " The air ignites with a roar: a sheet of blue fire rolls along the roof!",
    " The air's too foul to keep your light burning."
};

/* The levels its breath costs it, by the blackdamp's strength and its minutes in it; by the whitedamp's doses */
static unsigned char __fastcall__ air_black (unsigned char s)
{
    if (s == 1) {
        return 1;                               /* (Short of breath: stale air never kills) */
    }
    if (s == 2) {
        return air_m >= 60 ? 9 : air_m >= 30 ? 6 : air_m >= 10 ? 4 : 2;
    }
    return air_m >= 5 ? 9 : air_m >= 2 ? 6 : 3;
}

static unsigned char air_white (void)
{
    return air_co >= 180 ? 9 : air_co >= 120 ? 6 : air_co >= 60 ? 3 : air_co >= 30 ? 1 : 0;
}

static unsigned char __fastcall__ air_of (unsigned char q, unsigned char f, unsigned m)
{
    static const char* const said[10] = {
        " You breathe freely again.", " You're short of breath.", " You're gasping: the air's foul.",
        " Your head swims; your heart pounds.", " Your legs are failing you: you must get out of this air.",
        " Your head aches.", " You feel dizzy and sick.", " Your head clears.",
        " You can't get your breath: the dark takes you.", " The poisoned air overcomes you."
    };
    unsigned char k = AQ_KIND (q), s = AQ_STR (q), lv, o = 0, w;

    if (f & FLM_BURST) {
        AIR_SAY (air_flame[10]);                /* (The world's harm after it) */
        o = 6;
    } else if (f & FLM_OUT) {
        AIR_SAY (air_flame[11]);
        o = 2;
    } else if ((f & FLM_NEW) && (f & (FLM_LIT | FLM_LAMP))) {
        w = k == AQ_BLACK ? s - 1 : k == AQ_FIRE ? s + 5 : !k ? 9 : 3;
        if (w < 12 && air_flame[w] && (w != 9 || air_lv || air_m)) {
            AIR_SAY (air_flame[w]);             /* (Its warning; out of the blackdamp, its flame's brightening) */
            o = 2;
        }
    }
    if (k == AQ_FIRE && s == 3) {
        k = AQ_BLACK;                           /* (Firedamp thick enough to choke on: as blackdamp, a little) */
        s = 2;
    }
    if (k == AQ_BLACK) {
        air_m = air_m + m < air_m ? 0xFFFF : air_m + m;
    } else {
        air_m = 0;
    }
    if (k == AQ_WHITE) {
        air_co += m * s * s;
    } else {
        air_co = air_co > 2 * m ? air_co - 2 * m : 0;   /* (Clearing as slowly in fresh air) */
    }
    lv = air_white ();
    if (k == AQ_BLACK && (w = air_black (s)) > lv) {
        lv = w;
    }
    if (lv != air_lv) {
        w = lv > air_lv;
        if (lv >= 9) {
            AIR_SAY (said[air_co >= 180 ? 9 : 8]);
            air_lv = lv;
            AIR_LEVELS (lv);
            return o | 1;
        }
        AIR_SAY (said[!lv ? (air_co ? 7 : 0) : k == AQ_BLACK ? (lv >= 6 ? 4 : lv >= 4 ? 3 : lv) :
            lv >= 6 ? 4 : lv >= 3 ? 6 : 5]);
        air_lv = lv;
        AIR_LEVELS (lv);
        o |= w ? 6 : 2;
    }
    return o;
}
