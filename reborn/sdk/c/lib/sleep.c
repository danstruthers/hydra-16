/*
** sleep.c - sleep (seconds): the task sleeps (SLEEP, 100 seconds at a time at most), other tasks running.  0, or
** the seconds left when a note ended it sooner.  (It takes the place of cc65's.)
*/

#include <unistd.h>
#include <hydra.h>

unsigned __fastcall__ sleep (unsigned seconds)
{
    unsigned step;

    while (seconds) {
        step = seconds > 100 ? 100 : seconds;
        if (hy_sleep_ticks (step * HY_TICK_HZ) < 0) {
            return seconds;
        }
        seconds -= step;
    }
    return 0;
}
