/*
** test.c - (dq0harn's, HarnMaster's) a test in the town and the scholar's house (../../dq0/rsmeta.h's meta_test): the book's,
** Skills 5, as the library's harn_test (d100 against the EML held to 5-95, a success at or under it, a critical a roll of 5,
** 10 ... 100; here so the library's skills' table isn't linked).  A file of its own: the town (dq0town's rstest.c) links it
** alone, the scholar's house (dq0schol's rsmeta.c) with the sheet's (meta.c).  And the prices' base (meta_price): the book's,
** Character 13 (a scribe's 66d a month over 24 working days, 3d; a sage's 84d a month), the house's where it has none (a dead
** tongue's sage three times a scribe, a sage's day asked of a skill 3d, the library a penny a day).
*/

#include <dice.h>
#include "../../dq0/hub.h"
#include "../../dq0/rs.h"
#include "../../dq0/rsmeta.h"

const unsigned char meta_price[NMP] = { 3, 3, 84, 3, 1 };

signed char __fastcall__ meta_test (int chance)
{
    unsigned char r = d100 ();

    chance = chance < 5 ? 5 : chance > 95 ? 95 : chance;
    if (r <= chance) {
        return r % 5 ? G_PASS : G_WELL;
    }
    return r % 5 ? G_FAIL : G_BADLY;
}
