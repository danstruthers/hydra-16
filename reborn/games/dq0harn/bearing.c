/*
** bearing.c - (overlay 5, the referee's: called through bank.s's stubs) the character's bearings underground kept
** (rs.h's P_BEARING; DUNGEON.md's bearings underground, HOUSE.md's, slice 4.2's): the way it's come by bending, or
** felt across a room in the dark, noted as it went.  The book has no skill of direction: the game's choice, Awareness
** (each turn of the way noticed as it's taken), or Mining if the character's is the higher (a miner knows tunnels,
** and keeps his bearings in them); an Intelligence roll was the other candidate, but keeping count of turns is
** attention more than reasoning, and Survival is the outdoors' (the sun, the stars), of no use below.  One whose
** species lives below (the Khuzdul: its summary's northsense) always knows north, never tested.  Its dice its own
** (rshouse.h's), so the game's other tests roll as they did; a failure's d8 by them too (the world's: a drift one way or
** the other, or a north astray anywhere).
*/

#pragma code-name ("OV5CODE")                 /* (Overlay 5's, beside part 2's image: 2026-10-27, overlay 3's room */
#pragma rodata-name ("OV5RODATA")             /*   for the denizens' gear) */

#include <dice.h>
#include <harn.h>
#include "../dq0/hub.h"
#include "../dq0/rs.h"
#include "dq0harn.h"

unsigned long bseed;                            /* (The bearings' dice: rshouse.h's) */
unsigned char bway;                             /* (A failure's d8 less one, 0-7: the world's, a drift's way, or where
                                                **   a north astray lies) */
unsigned char bwhat;                            /* (The test: 0 the bearings; 1 the time kept, 2 by one that always knows
                                                **   it: the main RAM's, from the request) */

#define RS_HOUSE_BEARING
#include "../dq0/rshouse.h"

/* A skill's mastery for its test (test_sk's, dq0harn.c's: one not opened at its opening level, less its penalty) */
static int __fastcall__ level (unsigned char sk)
{
    return harn_eml (&pc, sk) + (pc.ml[sk] ? 0 : harn_get_oml (&pc, sk));
}

/* The test (the bearings', or the time kept: bwhat), at the circumstances' steps (STEP_PCT each: the world's, the
** bends and the dark; the hours awake, a sleep's length): its grade (rs.h's G_*), bway a failure's d8 less one.  The time
** kept (DUNGEON.md's the time of day believed, 2026-10-31): Awareness, the hours noticed going by as the turns of the
** way are (the book has no skill of it: the house's), by its own dice, the game's as they stand mixed (another
** generator's step, a linear congruential one's, as moving quietly's) and put back after, so the game's other tests
** roll as they would without it; one with absolute timesense (its file's "timesense": the summary's) never fails */
signed char __fastcall__ o_bearing_of (signed char steps)
{
    int a = level (HARN_AWA), m = level (HARN_MIN);
    unsigned long t;
    signed char g;

    bway = 0;
    if (bwhat) {
        if (bwhat > 1) {
            return G_CS;                        /* (It always knows the hour: no test) */
        }
        t = xs_seed;
        xs_seed = (t * 1103515245UL + 12345UL) ^ 0x54494D45UL;
        g = grade (harn_test (a + STEP_PCT * steps));
        if (g < 0) {
            bway = die (8) - 1;
        }
        xs_seed = t;
        return g;
    }
    if (pc.att[HARN_SP] == HARN_KHU) {
        return G_CS;                            /* (It knows north: no test) */
    }
    bdice ();
    g = grade (harn_test ((a > m ? a : m) + STEP_PCT * steps));
    if (g < 0) {
        bway = die (8) - 1;
    }
    bdice ();
    return g;
}
