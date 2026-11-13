/*
** test.c - (dq0null's, the null ruleset's) a test in the town and the scholar's house (../../dq0/rsmeta.h's meta_test): its
** flat odds (dq0null.c's flat_r), a d100 at most the chance (5 to 95) a success, a fine one (G_WELL) in the fifth of its band
** nearest the end, the low rolls of a success, the high of a failure.  The chance is the town's record's knack (the null
** account's acct_town: its flat 60) or the scholar's house's (a mastery, a reader's skill: the same 60, and the steps added).
** A file of its own: the town (dq0ntown's rstest.c) links it alone, the scholar's house (dq0nschol's rsmeta.c) with meta.c.
** And the prices' base (meta_price): the null has no economy of its own, so HarnMaster's figures (3d, three times, 84d, 3d, 1d).
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
        return r <= chance / 5 ? G_WELL : G_PASS;
    }
    return r > 100 - (100 - chance) / 5 ? G_BADLY : G_FAIL;
}
