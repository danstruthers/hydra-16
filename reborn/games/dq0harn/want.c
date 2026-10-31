/*
** want.c - (dq0harn, overlay 7) hunger and thirst (slice 4.9: HOUSE.md's water, hunger and thirst, rswant.h's house
** rules), HarnMaster's way with them: the want's fatigue levels (the book's fatigue, in every test's universal
** penalty) not rested off till it eats or drinks (wantfl, sheet.c's rest), dead of it
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")

#include <string.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

static void __fastcall__ want_say (const char* s)
{
    unsigned char n = strlen (news);

    if (n + strlen (s) < NEWS_N - 1) {
        strcpy (news + n, s);
    }
}

#define WANT_SAY(s)     (want_say (s), r->grade |= 2)
#include "../dq0/rswant.h"

/* RS_TIME's (r a copy, the hub's want in it: dq0harn.c's main), after the bodies' time (heal.c's) */
void __fastcall__ o_want (struct req* r)
{
    unsigned char k = want_of (r, tnow);

    if ((k & WANT_DIES) && pc.state != HARN_DEAD) {
        pc.state = HARN_DEAD;
        r->result = 1;
    }
    k &= ~WANT_DIES;
    if (k > wantfl) {
        pc.fl += k - wantfl;                    /* (Its fatigue from want: the levels more) */
    }
    wantfl = k;
}
