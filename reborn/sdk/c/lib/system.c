/*
** system.c - a command line run as rc runs one (Plan 9's: rc -c): system(), in a task of its own, with this
** program's fds, namespace, current directory and environment, waited for.  So it can be a program, a pipeline, a
** script, a loop: anything rc's prompt takes.  rc is /bin/rc, or the ROM's (#m/rc) where there's no /bin.  What's
** waiting in stdio's buffers goes out first.  Its value: 0 when rc's $status was true; a program's exit code when
** that was the status (rc ends with code 1 and its $status as the message: "3", say); 1 for any other ("oops",
** "interrupt"); -1 if rc couldn't be started.  (It takes the place of cc65's system.)
*/

#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <hydra.h>

int __fastcall__ system (const char* s)
{
    char* argv[4];
    char msg[HY_EXIT_MSG_MAX + 1];
    int task, code;

    if (s == 0) {
        return 1;                       /* (There's a shell) */
    }
    argv[0] = "rc";
    argv[1] = "-c";
    argv[2] = (char*) s;
    argv[3] = 0;
    task = hy_spawn ("/bin/rc", argv, 0);
    if (task < 0 && _oserror == HY_E_NOENT) {
        task = hy_spawn ("#m/rc", argv, 0);
    }
    if (task < 0) {
        return -1;
    }
    code = hy_wait (task, msg);
    if (code == 1 && isdigit (msg[0])) {
        return atoi (msg);
    }
    return code;
}
