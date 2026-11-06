/*
** town.h - (dq0acct, dq0town) what the town tells the account of what was done there (the plan's step 11; HOUSE.md's between
** dungeons, the town): town.dat in the save's folder, written by dq0town as the character leaves, read by dq0acct (town.c), which
** passes the days, does what the player didn't see to itself, and tells the road taken.  And between dq0town and the scholar's house
** (dq0schol, the plan's step 12: a program of its own, run by dq0town), the same file, written before it's run and read after:
** the days it took, what it cost, what a master taught, the script studied.
*/

#ifndef _TOWN_H
#define _TOWN_H

#define TOWN_FILE       "town.dat"
#define TOWN_KIT        "townkit.dat"           /* (What's carried, handed to the town and back: kit.dat as it was till the account's end, so a
                                                **   reset in the town tells the time between again, the same) */
#define TS_MARKET       0x01                    /* (The market's been seen: nothing more sold or bought for it, by the account) */
#define TS_SMITH        0x02                    /* (The smith's: nothing more mended or whetted for it) */
#define TOWN_DAYS       200                     /* (The days in town at most: then it sets out) */
#define TOWN_LEARNT     2                       /* (What masters teach in a town's stay, at most: a month each) */

struct townrec {
    unsigned        days;                       /* The days spent in town: its errands' and its rests' */
    unsigned char   seen;                       /* TS_* */
    unsigned char   road;                       /* The road's days to the place it set out for (0: none) */
    char            whither[28];                /* That place's name ("" none: it goes on as it came) */
    unsigned long   money;                      /* (The scholar's house's, dq0town's to it and back: the money, farthings) */
    unsigned char   big;                        /*   whether it's a big town (town.txt's kind: a sage for a dead tongue) */
    unsigned        clue[8];                    /*   the writings carried: their things' fuel (life.h's: an old writing's
                                                **   number at the top, a copy's CLUE_COPY too; 0 none) */
    unsigned char   taught;                     /* The months with a master: the month's practice forfeit for each (the book's,
                                                **   Skills 11: a new script's 30 points, a tongue's whole month) */
    char            study[10];                  /* The script studied in the library ("" none): the month's practice on it first */
    char            learnt[TOWN_LEARNT][16];    /* What a master opened, as the referee's store has it ("*delvic=81",
                                                **   "sindarin=11"): the account's to put on the sheet ("" none) */
};

#ifdef TOWN_ROAD
/* A place's road from the town, in days (the house's: 2 to 12, by its name, so the same place the same road) */
static unsigned char __fastcall__ road_of (const char* s)
{
    unsigned h = 0;

    while (*s) {
        h = h * 31 + *s++;
    }
    return 2 + h % 11;
}
#endif

#endif
