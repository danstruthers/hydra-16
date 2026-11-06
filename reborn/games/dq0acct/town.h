/*
** town.h - (dq0acct, dq0town) what the town tells the account of what was done there (the plan's step 11; HOUSE.md's between
** dungeons, the town): town.dat in the save's folder, written by dq0town as the character leaves, read by dq0acct (town.c), which
** passes the days, does what the player didn't see to itself, and tells the road taken.
*/

#ifndef _TOWN_H
#define _TOWN_H

#define TOWN_FILE       "town.dat"
#define TOWN_KIT        "townkit.dat"           /* (What's carried, handed to the town and back: kit.dat as it was till the account's end, so a
                                                **   reset in the town tells the time between again, the same) */
#define TS_MARKET       0x01                    /* (The market's been seen: nothing more sold or bought for it, by the account) */
#define TS_SMITH        0x02                    /* (The smith's: nothing more mended or whetted for it) */

struct townrec {
    unsigned        days;                       /* The days spent in town: its errands' and its rests' */
    unsigned char   seen;                       /* TS_* */
    unsigned char   road;                       /* The road's days to the place it set out for (0: none) */
    char            whither[28];                /* That place's name ("" none: it goes on as it came) */
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
