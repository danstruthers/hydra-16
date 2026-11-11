/*
** notice.c - (the house's rules, ../rshouse.h's) what the character notices, by its going and its watchfulness
*/

#include "../hub.h"
#include "../rs.h"
#include "../referee.h"
#include "../rshouse.h"

static const signed char by_pace[NGAITS] = { 1, 1, 0, -2, -4, -6 };

signed char __fastcall__ house_notice (const struct req* r, unsigned char go, unsigned char watch)
{
    unsigned char g = go & ~GOING_DARK;
    signed char s = 0, p;

    if (RS_TEST_PURPOSE (*r) == P_NOTICE_SIGHT && RS_TEST_HOW (*r) == 1 && !(watch & WA_TRAPS)) {
        s = RS_TEST_STEPS (*r) - 2;
    }
    if (g && g <= NGAITS) {
        p = by_pace[g - 1];
        s += p > 0 && (go & GOING_DARK) ? 0 : p;
    }
    if (RS_TEST_PURPOSE (*r) == P_NOTICE_SOUND && (watch & WA_LISTEN)) {
        s += 2;
    }
    return s;
}
