/*
** state.c - (the house's rules, ../rshouse.h's) the character's state in words: its pace, its watchfulness, its light
*/

#include "../hub.h"
#include "../rs.h"
#include "../referee.h"
#include "../rshouse.h"

static const char* const pace_name[NGAITS] = { "a creep", "a slow walk", "a walk", "a jog", "a run", "a sprint" };

unsigned char __fastcall__ house_pace (unsigned char g, unsigned char watch, unsigned char dark)
{
    unsigned char h = g;

    if (h > GA_SLOW && ((watch & (WA_TRAPS | WA_MAP)) || dark)) {
        h = GA_SLOW;
    } else if (h > GA_WALK && (watch & (WA_LISTEN | HOUSE_SHOCK | HOUSE_LEG))) {
        h = GA_WALK;
    }
    put ("Pace: ");
    put (pace_name[g]);
    if (h != g) {
        put (", held to ");
        put (pace_name[h]);
        put (watch & WA_TRAPS ? " while you search as you go" : watch & WA_MAP ? " while you map as you go" :
            dark && h == GA_SLOW ? " in the dark" : watch & HOUSE_SHOCK ? " in shock" : watch & HOUSE_LEG ? " on one leg" :
            " while you listen");
    }
    return h;
}

void __fastcall__ house_watching (unsigned char watch)
{
    put (".\nWatching: ");
    put (!watch ? "nothing in particular" : watch & WA_TRAPS ? "for traps and hidden doors as you go" : "");
    put ((watch & WA_TRAPS) && (watch & (WA_LISTEN | WA_MAP)) ? "; " : "");
    put (watch & WA_LISTEN ? (watch & WA_MAP ? "listening and mapping as you go" : "listening as you go") :
        watch & WA_MAP ? "mapping as you go" : "");
    put (".");
}

void __fastcall__ house_light (unsigned char light, const char* lamp, unsigned char dark)
{
    put ("\nLight: ");
    if (light) {
        put ("your ");
        put (lamp);
        put (" (");
        putn (SQ_FT * light);
        put (" feet).");
    } else {
        put (dark ? "none: it's dark here, and you feel your way." : "none of your own: this place's own light.");
    }
}
