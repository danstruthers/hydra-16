/*
** town.h - (the account, ./acct.h; dq0town, dq0schol) the town between two dungeons (the plan's step 11; HOUSE.md's between dungeons, the
** town): what passes between the account (dq0acct, dq0nacct: each ruleset's link of it), the town (dq0town) and the scholar's house (dq0schol, the plan's step
** 12) is one record in the save's folder, the build's (games/dq0/records.def's struct between, between.dat: obj/gen's
** dq0rec.h): written by the account before it runs the town, read by the town as it starts and written back as it's left;
** the scholar's house's, written before it's run and read after; read by the account, which passes the days, does what the
** player didn't see to itself, and tells the road taken.  Here, what the record's fields mean.
*/

#ifndef _TOWN_H
#define _TOWN_H

#include "dq0rec.h"

#define TOWN_KIT        "townkit.dat"           /* (What's carried, handed to the town and back: kit.dat as it was till the account's end, so a
                                                **   reset in the town tells the time between again, the same) */
#define TS_MARKET       0x01                    /* (The market's been seen: nothing more sold or bought for it, by the account) */
#define TS_SMITH        0x02                    /* (The smith's: nothing more mended or whetted for it) */
#define TOWN_DAYS       200                     /* (The days in town at most: then it sets out) */


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
