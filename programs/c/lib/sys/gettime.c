/*
** gettime.c - clock_gettime() (time() uses it): the Hydra's clock, which counts seconds since 2000-01-01 00:00:00,
** as seconds since 1970.  The Hydra keeps local time, and cc65's time zone is UTC unless it's set (_tz), so
** localtime() shows the time as the Hydra has it.  CLOCK_REALTIME only; to the second.
*/

#include <time.h>
#include <hydra.h>

#define UNIX_2000   946684800UL         /* 2000-01-01 00:00:00, in seconds since 1970 */

int __fastcall__ clock_gettime (clockid_t clock_id, struct timespec* tp)
{
    (void) clock_id;
    tp->tv_sec = hy_clock () + UNIX_2000;
    tp->tv_nsec = 0;
    return 0;
}
