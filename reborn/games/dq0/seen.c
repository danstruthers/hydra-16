/*
** seen.c - (overlay 7) more of what the character perceives, in words (words.c's overlay is full): a ceiling at a
** glance; the things it sees lying about and the denizens it sees (words.c's, after the place; as they stir), and
** those it hears.  As words.c's: only what it sees now (nothing behind it), and what's further off in less detail,
** the further it is and the weaker the character's sight (the referee's: struct sight's eye): a thing too far to make
** out only by its size ("something small"), a denizen too far to tell what it is only as something there.
*/

#pragma code-name ("OV7CODE")
#pragma rodata-name ("OV7RODATA")

#include <stdlib.h>
#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "level.h"

static const char* const relname[8] = {
    "ahead", "ahead to your right", "to your right", "behind you to your right", "behind you",
    "behind you to your left", "to your left", "ahead to your left"
};

static struct sight* S;

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

static void __fastcall__ addn (int v)
{
    char b[8];

    add (itoa (v, b, 10));
}

/* Squares from the character to square (x, y): the more of the two ways */
static unsigned char __fastcall__ apart (unsigned x, unsigned y)
{
    x -= S->x;
    y -= S->y;
    x = (int) x < 0 ? -x : x;
    y = (int) y < 0 ? -y : y;
    x = x > y ? x : y;
    return x > 255 ? 255 : x;
}

/* ---- The ceiling in general words (the user's: a high ceiling, a low, rough one), as it's seen at a glance: its kind,
** and how high it is (low, under 8 feet; high, over 12; lofty, over 20); words.c's, added to lb (its record
** sight_copy) */
void o_ceiling_glance (void)
{
#define S sight_copy
    static const char* const and[4] = { " and low.", ".", " and high.", " and very high." };
    static const char* const over[4] = { " low overhead.", " overhead.", " high overhead.", " far overhead." };
    unsigned char k = S.ceil != C_ROUGH && S.ceil != C_SLOPED && S.cwall >= S.cmid ? C_FLAT : S.ceil, h;

    h = S.cmid < 8 ? 0 : S.cmid <= 12 ? 1 : S.cmid <= 20 ? 2 : 3;
    add (k == C_FLAT ? " The ceiling is flat" : k == C_BARREL ? " A barrel vault arches" : k == C_DOME ?
         " The ceiling is a dome," : k == C_CATHEDRAL ? " A pointed vault rises" : k == C_SLOPED ?
         " The ceiling slopes" : " The ceiling is rough stone");
    add (k == C_SLOPED ? (S.cmid >= S.cwall + 6 ? " steeply, high on one side and low on the other." :
         ", higher on one side.") : k == C_FLAT || k == C_ROUGH ? and[h] : over[h]);
#undef S
}

/* ---- The things and the denizens it sees (words.c's o_words, after the place), into lb */

/* The things it sees lying about: "On the floor ahead lies a pick." "At your feet lie some bones." "On the table
** lies a candle." (a surface's things after it, hid 2); "At your feet lies a lit torch." (hid 4); one too far off to
** make out (by its size, hid's high bits, and the character's sight), only by its size: "On the floor ahead lies
** something small." (and nothing on it told); none behind it */
static void items (void)
{
    unsigned char i, on = 0, sz, b, skip = 0;

    for (i = 0; i < S->nitems; ++i) {
        sz = S->item[i].hid >> 4;
        b = !(S->item[i].hid & 1) && apart (S->item[i].x, S->item[i].y) > 1 + S->eye / 2 + sz;
        if (S->item[i].hid & 2) {
            if (skip) {
                continue;                       /* (On a thing behind it, or too far to make out) */
            }
            add (" On the ");
            add (S->item[on].name);
            add (" ");
        } else {
            on = i;
            if ((skip = REL_BEHIND (S->item[i].rel)) != 0) {
                continue;
            }
            skip = b;
            add (S->item[i].hid & 1 ? (S->item[i].rel == 0xFF ? " Hidden at your feet " : " Hidden on the floor ") :
                S->item[i].rel == 0xFF ? " At your feet " : " On the floor ");
            if (S->item[i].rel != 0xFF) {
                add (relname[S->item[i].rel & 7]);
                add (" ");
            }
        }
        if (b) {
            add (sz <= 2 ? "lies something small." : sz >= 5 ? "lies something large." : "lies something.");
            continue;
        }
        add (S->item[i].plural || S->item[i].qty > 1 ? "lie " : "lies ");
        if (S->item[i].qty > 1) {
            addn (S->item[i].qty);
            add (" ");
        } else {
            add (S->item[i].plural ? "some " : !(S->item[i].hid & 4) && strchr ("aeiou", S->item[i].name[0]) ? "an " :
                "a ");
        }
        add (S->item[i].hid & 4 ? "lit " : "");
        add (S->item[i].name);
        add (".");
    }
}

/* The denizens it sees: "A giant rat is ahead to your left, 15 feet away, watching you." (each, as there's room
** in lb; an ally named: "Grub is behind you, 5 feet away, following you."); one too far off to make out (by the
** character's sight) only as something there; but none behind it, an ally's but heard close behind it */
static const char* const doing[9] = {
    " feet away.", " feet away, watching you.", " feet away, coming towards you.", " feet away, keeping its distance.",
    " feet away, following you.", " feet away, waiting.", " feet away, fighting you.", " feet away, fleeing.",
    " feet away, lying senseless."
};

static void denizens (void)
{
    unsigned char i, r, d, ally, heard = 0;
    char* n;

    for (i = 0; i < S->ndens && strlen (lb) < sizeof lb - 80; ++i) {
        n = S->den[i].name;
        r = REL8 (bearing (S->den[i].x - S->x, S->den[i].y - S->y), pc_face);
        d = apart (S->den[i].x, S->den[i].y);
        ally = n[0] < 'a';
        if (REL_BEHIND (r) && !ally) {
            heard |= d <= 2;
            continue;
        }
        if (!ally && d > 3 + S->eye) {
            add (" Something");
        } else {
            add (ally ? " " : strchr ("aeiou", n[0]) ? " An " : " A ");
            add (n);
        }
        add (" is ");
        add (relname[r]);
        add (", ");
        addn (5 * d);
        add (doing[S->den[i].doing % 9]);
    }
    add (heard ? " You hear something close behind you." : "");
}

/* A denizen heard, not seen (main.c's heard: one of the denizens' events), into lb, in its sound's words (the sound
** task's, SOUND.md: heard_what) if it has them: "You hear a scurrying to your right.", "You hear a muffled squeak
** behind you." (what | 0x80: walls or a door between); else "You hear something moving behind you to your left." */
void __fastcall__ o_heard_words (unsigned char what, unsigned char rel)
{
    if (heard_what[0]) {
        strcpy (lb, "You hear ");
        if (what & 0x80) {
            if (heard_what[0] == 'a' && heard_what[1] == ' ') {
                strcat (lb, "a muffled ");      /* ("a muffled squeak") */
                strcat (lb, heard_what + 2);
            } else {
                strcat (lb, "muffled ");
                strcat (lb, heard_what);
            }
        } else {
            strcat (lb, heard_what);
        }
        strcat (lb, " ");
    } else {
        strcpy (lb, (what & 0x7F) == DE_NOISE ? "You hear a noise " : "You hear something moving ");
    }
    strcat (lb, relname[rel & 7]);
    strcat (lb, ".");
}

/* The denizens it sees, as one comes into sight, goes from it, or does something else (the world's V_STIR: into
** lb): "Something stirs. A giant rat is ..." */
void o_stir_words (void)
{
    lb[0] = 0;
    S = &sight_copy;
    add (S->ndens ? "Something stirs." : "Nothing stirs in sight now.");
    denizens ();
}

/* The things and the denizens it sees, added to lb (words.c's, from look.c's copy of the hub's record) */
void o_seen_words (void)
{
    S = &sight_copy;
    if (!(S->first & SIGHT_HURRY)) {                            /* (A run's or a sprint's look: not the things about) */
        items ();
    }
    denizens ();
}
