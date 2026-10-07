/*
** getenv.c - the environment, as the kernel keeps it (ENV_GET: the task's own copy of its starter's, which #e
** serves as /env's files): getenv reads a variable as it is now (another program may set this task's), nothing
** copied at the start.  An rc list's words each end with a 0: getenv gives them a space between, as Plan 9's does.
** (It takes the place of cc65's getenv, which reads an array made at the start.)
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>

#define VALUE_MAX       255             /* A value getenv gives: its bytes, at most */

/* Its value (good till the next getenv), or NULL: it isn't set */
char* __fastcall__ getenv (const char* name)
{
    static char value[VALUE_MAX + 1];
    struct hy_regs r;
    unsigned n, i;

    r.a = 0xFF;                         /* (This task's) */
    r.r[0] = (unsigned) name;
    r.r[1] = (unsigned) value;
    r.r[2] = VALUE_MAX;
    r.r[3] = 0;
    if (hy_call (HY_ENV_GET, &r)) {
        return 0;
    }
    n = r.a | (r.x << 8);
    if (n > VALUE_MAX) {
        n = VALUE_MAX;
    }
    if (n && value[n - 1] == 0) {       /* (A list's last 0: its end) */
        --n;
    }
    for (i = 0; i < n; ++i) {
        if (value[i] == 0) {
            value[i] = ' ';
        }
    }
    value[n] = 0;
    return value;
}
