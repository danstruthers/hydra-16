/*
** rf.c - Dingus Quest Zero's referee skeleton (../referee.h's; ENGINE-CRITIQUE.md's section 8, 2026-11-11): what every
** referee does alike, so a ruleset is only its answers.  A task in the referee's role (dq0NAME SEG ROLE DIR, started by
** the UI as dq0 -r NAME asks): the hub attached, its dice seeded from the game's (and its role), each of the hub's slots
** answered as it's rung (the world's, the UI's, and the denizens' task's own, DSLOT, a short one: DTEXT of its text),
** each requester's dice its own stream (../dice.inc's dice_to: the dice by who asks, so a game's dice go by its seed and
** its commands, not by the order the tasks' requests reach the referee), the asker rung with the reply (hub.h's
** protocol).  And the words into a reply, what's to hand (RS_EQUIP's keys, each after its mark), the creatures' sheets
** read back as a game's carried on and written with the save (RS_SAVE: the ruleset's character first), the bearings'
** dice's state.  Its referee's main RAM's (each referee's rf.c #includes it).
*/

#include <stdlib.h>
#include <string.h>
#include <dice.h>
#include <hydra.h>
#include "../hub.h"
#include "../rs.h"
#include "../referee.h"

unsigned char getbank (void);                   /* (The referee's bank.s's: the window's bank, read and set) */
void __fastcall__ setbank (unsigned char b);

unsigned long bseed;

/* ---- Words into a reply (from an overlay too: s may be in the window, where the hub is, so each character's taken
** with s's bank in, and put with the hub's) */

char* tp;                                       /* Where the next goes, */
char* tend;                                     /*   and the end */

void __fastcall__ put (const char* s)
{
    unsigned char b = getbank (), h, c;

    hub_map (0);
    h = getbank ();
    for (;;) {
        setbank (b);
        c = *s++;
        setbank (h);
        if (!c || tp >= tend) {
            break;
        }
        *tp++ = c;
    }
    *tp = 0;
    setbank (b);
}

void __fastcall__ putn (int v)
{
    char b[8];

    put (itoa (v, b, 10));
}

/* ---- What's to hand (the keys of the things the character carries, all of them: the UI's, from the things): a
** referee judges its tests and fights by them (a skill used, the one versed in it takes out the tool that suits) */

char tools[NTOOLS][8];
unsigned char tflag[NTOOLS], tqual[NTOOLS];

/* All that's to hand, anew: the keys in the text, each after its mark (held: < in the left hand, > the right, * both;
** + worn; a space neither) and its quality after it (a digit), a weapon's version after that (L light, H heavy, none
** average: tqual's high bits, TQ_*, D13's), a bandage's cloth (R hemp, S linen, T wool, U silk: dq0harn's DR_MAT's) */
void __fastcall__ equip_all (const char* keys)
{
    unsigned char k = 0, n, f, j;

    memset (tools, 0, sizeof tools);
    while (*keys) {
        j = NTOOLS;
        f = *keys == '*' ? T_HELD | T_LEFT | T_RIGHT : *keys == '<' ? T_HELD | T_LEFT : *keys == '>' ? T_HELD | T_RIGHT :
            *keys == '+' ? T_WORN : 0;
        if (*keys < 'a') {
            ++keys;
        }
        for (n = 0; keys[n] >= 'a'; ++n) {
        }
        if (n && n < sizeof tools[0]) {
            if (k < NTOOLS) {
                j = k++;
            } else {                            /* (Full, an outfit's many things: what's held or worn in the place of
                                                **   a thing carried, what fights first, 2026-10-27) */
                for (j = NTOOLS; j && tflag[j - 1]; --j) {
                }
                j = f && j ? j - 1 : NTOOLS;
            }
            if (j < NTOOLS) {
                memset (tools[j], 0, sizeof tools[j]);
                memcpy (tools[j], keys, n);
                tflag[j] = f;
                tqual[j] = keys[n] >= '0' && keys[n] <= '9' ? keys[n] - '0' : 4;
            }
        }
        keys += n;
        if (*keys >= '0' && *keys <= '9') {
            ++keys;
        }
        while (*keys >= 'A' && *keys <= 'Z') {
            if (j < NTOOLS) {
                tqual[j] |= *keys == 'L' ? TQ_LIGHT : *keys == 'H' ? TQ_HEAVY : *keys == 'D' ? TQ_DULL : *keys == 'E' ? TQ_DULLED :
                    (unsigned char) (*keys - 'R') < 4 ? (*keys - 'Q') << 4 : 0;     /* (A bandage's cloth, R ... U: DR_MAT's,
                                                **   dq0harn's dress.c's, the user's, 2026-11-11) */
            }
            ++keys;
        }
    }
}

/* A tool to hand (Lockcraft's picks) */
unsigned char __fastcall__ has (const char* key)
{
    unsigned char k;

    for (k = 0; k < NTOOLS; ++k) {
        if (!strcmp (tools[k], key)) {
            return 1;
        }
    }
    return 0;
}

/* ---- The creatures' sheets: read back as the first creature's asked for, a game carried on, and written with the
** save (the ruleset's: rf_sheets, in the save folder's beasts.dat, rf/sheets.c's) */

static unsigned char restored;

void sheets_in (void)
{
    if (HUB->resume && !restored) {
        restored = 1;
        rf_sheets (1);
    }
}

/* ---- A request answered: the words' room the slot's, the result 0, the ruleset's answer; the creatures' sheets read
** back first, a creature's sheet asked for (RS_NEW_ACTOR), and written after the character (RS_SAVE) */

static unsigned tlim = REQ_TEXT;                /* (A request's text's room: the denizens' slot's DTEXT) */

static void __fastcall__ answer (struct req* r)
{
    tp = r->text;
    tend = r->text + tlim - 1;
    r->result = 0;
    if (r->kind == RS_NEW_ACTOR) {
        sheets_in ();
    }
    rf_answer (r);
    if (r->kind == RS_SAVE) {
        sheets_in ();
        rf_sheets (0);
    }
}

/* The denizens' task's request (its own slot, DSLOT), if it's posted: answered (DTEXT of its text) */
static void den_slot (void)
{
    hub_map (0);
    if (hub_posted (DSLOT)) {
        tlim = DTEXT;
        answer (DSLOT);
        tlim = REQ_TEXT;
        hub_reply (DSLOT);
    }
}

int main (int argc, char* argv[])
{
    unsigned char s;

    if (rf_start (argc, argv) || hub_attach (num (argv[1]), num (argv[2]))) {
        return 1;
    }
    rseed (HUB->seed ^ 0x5245);                 /* (Its own dice: the game's seed, and its role) */
    rf_begin ();
    hub_map (0);
    dice_to (3 + HUB->dice);                    /* (The streams begun: 3 + the hub's dice, dq0 -R's) */
    for (;;) {
        hy_sem_acquire (HUB->sem[T_REFEREE]);   /* (Waiting costs nothing) */
        hub_map (0);
        if (HUB->quit) {
            break;
        }
        for (s = 0; s < NSLOTS; ++s) {
            if (hub_posted (&HUB->slot[s])) {
                dice_to (s);
                answer (&HUB->slot[s]);
                hub_reply (&HUB->slot[s]);
            }
        }
        dice_to (NSLOTS);
        den_slot ();
    }
    return 0;
}
