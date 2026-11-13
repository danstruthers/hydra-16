/*
** words.c - (HarnMaster's part of the account, its words overlay: hacct.h's OV_WORDS) the physician's words and the wounds':
** where each is on the body (harn.h's locations), what it was and what's become of it (was[], body.c's o_note).
*/

#pragma code-name ("WORDSCODE")
#pragma rodata-name ("WORDSRODATA")
#pragma bss-name ("WORDSRODATA")
#pragma local-strings (on)
#pragma static-locals (on)

#include <string.h>
#include <harn.h>
#include "../../dq0/hub.h"
#include "../../dq0/rs.h"
#include "../dq0harn.h"
#include "hacct.h"

/* ---- The physician's words, the wounds' */

/* Where a wound is, "left forearm" */
static void __fastcall__ say_place (unsigned char loc)
{
    static const char* const locations[HARN_NLOCATIONS] = {
        "skull", "face", "neck", "shoulder", "upper arm", "elbow", "forearm", "hand", "thorax", "abdomen", "groin", "hip",
        "thigh", "knee", "calf", "foot"
    };
    unsigned char l = HARN_LOC (loc);

    if ((l >= HARN_SHOULDER && l <= HARN_HAND) || l >= HARN_HIP) {
        say (loc & HARN_RIGHT ? "right " : "left ");
    }
    say (locations[l < HARN_NLOCATIONS ? l : 0]);
}

static const char* const wound_word[9][3] = {
    { "bruise", "fracture", "crush" }, { "cut", "gash", "deep gash" }, { "stab", "stab", "deep stab" },
    { "burn", "burn", "burn" }, { "bite", "bite", "deep bite" }, { "scratch", "rake", "deep rake" },
    { "sting", "sting", "deep sting" }, { "gore", "gore", "deep gore" }, { "ache", "ache", "ache" }
};

static const char* const sev_word[4] = { "minor", "serious", "grievous", "grievous" };

static unsigned char __fastcall__ sev_of (unsigned char il)
{
    return il >= 4 ? 2 : il >= 2 ? 1 : 0;
}

/* What's become of each wound there was */
static void wounds_say (void)
{
    const struct wwas* w;
    unsigned char i, k, s;

    for (i = 0; i < nwas; ++i) {
        w = &was[i];
        for (k = 0; k < pc.ninj && wid[k] != i; ++k) {
        }
        say ("The ");
        say (sev_word[sev_of (w->il)]);
        say (" ");
        say (wound_word[w->by && w->by <= BY_VENOM ? 3 + w->by : w->aspect & 3][(w->aspect & 3) != HARN_BLUNT || w->by ? 0 : w->sev > 3 ? 2 : w->sev ? w->sev - 1 : 0]);
        say (" to your ");
        say_place (w->loc);
        if (k == pc.ninj) {
            say (w->fever ? " festers, then mends and is gone. " : " is gone. ");
        } else if (pc.inj[k].il < w->il) {
            s = sev_of (pc.inj[k].il);
            say (s == sev_of (w->il) ? " mends a little." : " heals to a ");
            if (s != sev_of (w->il)) {
                say (sev_word[s]);
                say (" one.");
            }
            say (" ");
        } else {
            say (w->fever ? " festers, and is no better. " : " is no better. ");
        }
    }
}

/* The physician's words, what's become of the wounds */
void o_first (void)
{
    unsigned char k;

    if (ntended) {
        say ("A physician sees to your ");
        for (k = 0; k < ntended; ++k) {
            say_place (tended[k]);
            say (k + 2 < ntended ? ", " : k + 2 == ntended ? " and " : "");
        }
        say (" (");
        amount (fee);
        say ("). ");
    }
    if (selfnd) {
        say ("You tend what you can yourself. ");
    }
    if (unpaid) {
        say ("There's no money for a physician. ");
    }
    wounds_say ();
}
