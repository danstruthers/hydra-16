/*
** flight.inc - (dq0w) a flight's crossing, tenths, every actor's (acts.h's FLIGHT_T, the one-path plan's links,
** 2026-11-23): one source, compiled into the two parts that use it, the character's s_climb's (dq0w.c, part 2) and an
** actor's den_flight's (wflight.c, part 118), each its own static copy (room7, 2026-11-23: out of the main RAM, 161
** bytes of it, no part's copy added to a climb's path).  Included where the part's code-name is in force.
*/

/* Link kind k, the actor's step st, the drop between the floors in feet, up or down */
static unsigned long __fastcall__ flight_t (unsigned char k, unsigned char st, unsigned feet, unsigned char up)
{
    unsigned long t = (unsigned long) (feet + 1) * st;

    if (k >= O_RAMP_UP) {
        return t * (up ? 13 : 8) / 10;
    }
    return t * (k == O_LADDER_DOWN || k == O_LADDER_UP ? 2 : k == O_SHAFT_DOWN || k == O_SHAFT_UP ? 3 : 1) / (up ? 1 : 2);
}
