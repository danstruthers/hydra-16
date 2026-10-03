/*
** clock.c - the time: clock() (the ticks since the program started: CLOCKS_PER_SEC, hydra.h, is the tick's rate)
** and clock_gettime() (time() uses it).  The tick count (TICKS) wraps after about 5.5 minutes; clock counts
** on past that as long as it's called at least that often.  Till the clock comes (phase 5), the time is the
** program's own, from 2000-01-01 00:00:00 as it starts, as HydraFS's stamps count from the boot.
*/

#include <time.h>
#include <hydra.h>

#define UNIX_2000       946684800UL     /* 2000-01-01 00:00:00, in seconds since 1970 */

static unsigned last;                   /* The tick count at the last look ... */
static unsigned long count;             /*   and the ticks since the start */
static unsigned char started;

clock_t clock (void)
{
    unsigned now = hy_ticks ();

    if (!started) {
        started = 1;
        last = now;
    }
    count += (unsigned) (now - last);
    last = now;
    return count;
}

int __fastcall__ clock_gettime (clockid_t clock_id, struct timespec* tp)
{
    unsigned long ticks = clock ();

    (void) clock_id;
    tp->tv_sec = UNIX_2000 + ticks / HY_TICK_HZ;
    tp->tv_nsec = (ticks % HY_TICK_HZ) * (1000000000UL / HY_TICK_HZ);
    return 0;
}
