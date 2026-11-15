/*
** time.c - (overlay 16, the UI's: called through bank.s's stub, time_ui) the time of day the character believes, in
** words (DUNGEON.md's the time of day believed, HOUSE.md's; the user's, 2026-10-31: "Timekeeping should also be a fuzzy
** thing ... especially after sleeping the current time-of-day should be quite arbitrary to the player").  The world
** keeps the belief (the hub's tb: its skew from the true clock, its doubt) and the true clock; the words here: "time",
** "what time is it", told as believed and as sure as it is ("It's about midnight, you think.", "You've no idea how
** long you slept: it could be morning."; plainly when it knows); the sky seen (E_BEARING BRG_SKY: its phase); and a
** wait's or a sleep's length "until dawn", "until morning" (the hour it means, by what it believes: the true clock
** decides what happens), or "watch the sun" (the sky watched a while: a wait).
*/

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "dq0said.h"

#pragma code-name ("OV16CODE")
#pragma rodata-name ("OV16RODATA")
#pragma local-strings (on)

#define TB_START        18                      /* (The hour the game begins: the world's, wtime.c's) */

static const char* const hname[12] = {
    "twelve", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven"
};
static const char* const part[8] = {            /* (The day's parts, three hours each from midnight) */
    "the middle of the night", "the small hours", "early morning", "morning", "midday", "afternoon", "evening",
    "night"
};
static const char untils[] = "midnight\0dawn\0daybreak\0sunrise\0morning\0noon\0midday\0afternoon\0dusk\0sunset\0"
    "evening\0nightfall\0night\0";
static const unsigned char until_h[] = { 0, 6, 6, 6, 7, 12, 12, 14, 18, 18, 18, 19, 21 };   /* (Their hours) */
static char cw[48];

/* The minute of the day the character believes it is (b's tb copied out: 0-1439) */
static unsigned __fastcall__ day_believed (const struct tbelief* t)
{
    unsigned long now;
    unsigned char h = t->hour0 & 0x80 ? TB_START : t->hour0;

    hub_mem (0, &now, (const void*) &HUB->now, sizeof now);
    return ((unsigned) ((now / 600 + h * 60UL) % 1440) + 1440 + t->skew) % 1440;
}

/* Minute m of the day in words, to the quarter hour (q) or the hour: "midnight", "noon", "half past six in the evening",
** "a quarter to ten at night", "nine in the morning" */
static const char* __fastcall__ clock_words (unsigned m, unsigned char q)
{
    unsigned r = q ? (m + 7) / 15 * 15 : (m + 30) / 60 * 60;
    unsigned char h = (unsigned char) (r / 60 % 24), k = (unsigned char) (r % 60);

    cw[0] = 0;
    if (k == 45) {
        strcpy (cw, "a quarter to ");
        h = (h + 1) % 24;
    } else if (k) {
        strcpy (cw, k == 15 ? "a quarter past " : "half past ");
    }
    if (!k && (h == 0 || h == 12)) {
        strcat (cw, h ? "noon" : "midnight");
        return cw;
    }
    strcat (cw, hname[h % 12]);
    strcat (cw, h >= 5 && h < 12 ? " in the morning" : h >= 12 && h < 17 ? " in the afternoon" :
        h >= 17 && h < 21 ? " in the evening" : " at night");
    return cw;
}

/* "time", "what time is it": told as believed, as sure as it is (its words said.txt's SELF rows, time and lost: v how
** sure, or how it lost track, TG_*; text the time in words, cw) */
static void time_told (void)
{
    struct tbelief t;
    struct msg c;
    unsigned b;
    unsigned char v;

    hub_mem (0, &t, &HUB->tb, sizeof t);
    b = day_believed (&t);
    v = t.unc <= 10 ? 0 : t.unc <= 45 ? 1 : t.unc <= 150 ? 2 : 3;
    if (t.unc >= TB_LOST || v == 3) {
        strcpy (cw, part[b / 180]);
    } else {
        clock_words (b, !v);
    }
    said_s[2] = cw;                             /* (The main RAM's) */
    c.kind = UK_SELF;
    c.who = 0;
    c.w1 = t.unc >= TB_LOST ? MY_LOST << 8 | t.how : MY_TIME << 8 | v;
    outcome_words (&c);
}

/* A wait's length by the line (cl): "until dawn" (the minutes till the hour it names, as it believes the time now: the
** next one, a day on if it's that hour now), "watch the sun", "the sky", "the stars" (watched a while, a quarter of an
** hour, the stars' half an hour): minutes, or 0 (none said) */
static unsigned until (void)
{
    struct tbelief t;
    const char* p = strstr (cl, "until ");
    const char* w = untils;
    unsigned char k;
    unsigned m;

    if (!p) {
        return strstr (cl, "watch") ? (strstr (cl, "star") ? 30 : 15) : 0;
    }
    for (p += 6, k = 0; *w && strncmp (p, w, strlen (w)); w += strlen (w) + 1, ++k) {
    }
    if (!*w) {
        return 0;
    }
    hub_mem (0, &t, &HUB->tb, sizeof t);
    m = (until_h[k] * 60 + 1440 - day_believed (&t)) % 1440;
    return m < 5 ? m + 1440 : m;
}

/* The UI's ask (w's low byte, TU_*; its high, the sky's phase): told, the sky's words, a wait's length */
unsigned __fastcall__ o_time_ui (unsigned w)
{
    struct tbelief t;
    struct msg c;
    unsigned char p = w >> 8, k;

    switch ((unsigned char) w) {
    case TU_TELL:
        time_told ();
        break;
    case TU_SKY:                                /* (The sky seen: E_BEARING BRG_SKY's, the time read by it) */
        hub_mem (0, &t, &HUB->tb, sizeof t);
        k = p >> 4;
        p &= 15;
        if (!p) {                               /* (A rope's: SKP_GAVE, SKP_NO_ROPE, a phase of none) */
            p = !k ? SKP_NONE : k == 1 ? SAID_SKY_TORE : SAID_SKY_FAR;
        } else if (p <= SKP_STARS_WATCHED) {
            if (p == SKP_LOW) {
                strcpy (cw, part[day_believed (&t) / 180]);
            } else {
                clock_words (day_believed (&t), 0);
            }
        }
        said_s[2] = cw;                         /* (The main RAM's) */
        c.kind = UK_SKY;                        /* (Its words said.txt's SKY rows, the UI's own kind, 2026-11-12: what
                                                **   shows it, k, SKS_*; a shaft's refusals, p past SKP_STARS_WATCHED) */
        c.who = 0;
        c.w1 = p << 8 | k;
        outcome_words (&c);
        break;
    case TU_UNTIL:
        return until ();
    }
    return 0;
}
