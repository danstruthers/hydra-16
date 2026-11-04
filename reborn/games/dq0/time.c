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
static const char* const sky[] = {              /* (The sky seen, by its phase: SKP_DAWN ...) */
    "Grey light shows above: it's dawn, or near it.", "Daylight shows above: it's day.",
    "The light above is fading: it's dusk.", "Above, it's dark: it's night.",
    "You see the sun in the sky: it's %s, you judge.", "You see the stars: it's %s, you judge.",
    "You see the sun high overhead: it's about midday.",
    "You see the sun low in the sky, but you can't tell east from west: you take it for %s.",
    "You watch the sun a while, its slow way across the sky: it's %s, and you know which way north lies now.",
    "You watch the stars a while as they wheel: it's %s, and you know which way north lies now."
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

/* "time", "what time is it": told as believed, as sure as it is */
static void time_told (void)
{
    struct tbelief t;
    unsigned b;

    hub_mem (0, &t, &HUB->tb, sizeof t);
    b = day_believed (&t);
    if (t.unc >= TB_LOST) {
        sayf (t.how == TG_SLEEP ? "\nYou've no idea how long you slept: it could be %s." : t.how == TG_OUT ?
            "\nYou've no idea how long you lay senseless: it could be %s." :
            "\nYou've lost all track of the time down here: it could be %s.", part[b / 180]);
    } else if (t.unc <= 10) {
        sayf ("\nIt's %s.", clock_words (b, 1));
    } else if (t.unc <= 45) {
        sayf ("\nIt's about %s, you think.", clock_words (b, 0));
    } else if (t.unc <= 150) {
        sayf ("\nIt's some time around %s, you'd guess.", clock_words (b, 0));
    } else {
        sayf ("\nIt must be %s, you'd guess, but you can't be sure.", part[b / 180]);
    }
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
    unsigned char p = w >> 8, k;

    switch ((unsigned char) w) {
    case TU_TELL:
        time_told ();
        break;
    case TU_SKY:                                /* (The sky seen: E_BEARING BRG_SKY's, the time read by it) */
        hub_mem (0, &t, &HUB->tb, sizeof t);
        k = p >> 4;
        p &= 15;
        say ("\n");
        if (p > SKP_STARS_WATCHED || !p) {
            sky_words (p, k, cw);               /* (A shaft's refusal, a rope's: SKP_GAVE ..., a phase of none) */
        } else {
            if (p == SKP_LOW) {
                strcpy (cw, part[day_believed (&t) / 180]);
            } else {
                clock_words (day_believed (&t), 0);
            }
            if (k) {
                sky_words (p, k, cw);
            } else {
                sayf (sky[p - SKP_DAWN], cw);
            }
        }
        break;
    case TU_UNTIL:
        return until ();
    }
    return 0;
}

/* The sky seen by a window on the surface, a skylight, a shaft (k, SKS_*; w, the time in words), a shaft's refusals
** (p past SKP_STARS_WATCHED): overlay 1's, 16's room (HOUSE.md's what shows the sky) */
#pragma code-name (push, "OV1CODE")
#pragma rodata-name (push, "OV1RODATA")
unsigned char __fastcall__ o_sky_words (unsigned char p, unsigned char k, const char* w)
{
    static const char* const sky2[] = {             /* (The same, shown by a window, a skylight, a shaft: the first of its
                                                    **   %s's, the place's words, HOUSE.md's what shows the sky) */
        "Grey light %s: it's dawn, or near it.", "Pale daylight %s: it's day.", "Fading light %s: it's dusk.",
        "Stars show %s: it's night.", "The sun shows %s: it's %s, you judge.", "Stars show %s: it's %s, you judge.",
        "The sun stands high %s: it's about midday.",
        "The sun shows low %s, but you can't tell east from west: you take it for %s.",
        "You watch the sun a while %s, its slow way across the sky: it's %s, and you know which way north lies now.",
        "You watch the stars a while %s as they wheel: it's %s, and you know which way north lies now."
    };
    static const char* const fall[4] = {            /* (SKS_WINDOW on: the light's) */
        "comes in at the window", "falls from the skylight above", "falls from the shaft above", "falls from far up the shaft above"
    };
    static const char* const where[4] = {           /* (The sky's) */
        "at the window", "in the skylight above", "at the top of the shaft above", "far up the shaft above"
    };
    static const char* const shafts[] = {           /* (SKP_TOO_HIGH on) */
        "The shaft's mouth is too high to reach.", "An iron grating bars the top of the shaft: that's no way out.",
        "The shaft is choked with fallen rock a little way down.", "It falls away down the shaft, and you hear it land far below.",
        "You take hold of the rope.", "You put your weight on the rope: the grapple tears loose and comes clattering down.",
        "The rope's end hangs out of reach."
    };
    if (p == 0xFE) {                            /* (A grapple or a rope thrown up, at no mark: its work, carried out: 1.  Part 13's room) */
        w = cmd.obj.word;
        if (!*w || (cmd.dir != GO_UP && cmd.other.x) || !sky_words (0xFF, 0, w)) {
            return 0;
        }
        cmd.action = VB_TIE_OBJECT;
        cmd.aux = RP_THROW;
        more (VB_TIE_OBJECT);
        return 1;
    }
    if (p == 0xFF) {                            /* (A thing's word: a rope's work?  Overlay 16's room) */
        return strstr (w, "rope") || strstr (w, "grapple") || !*w;
    }
    if (!p) {
        if (!k) {
            return 0;
        }
        p = SKP_ROPE + k;
    }
    if (p > SKP_STARS_WATCHED) {
        say (shafts[p - SKP_TOO_HIGH]);
    } else {
        sayf (sky2[p - SKP_DAWN], p <= SKP_DUSK ? fall[k - 1] : where[k - 1], w);
    }
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)
