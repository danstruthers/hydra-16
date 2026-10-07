/*
** getenv.c - the environment, as Plan 9 has it: each variable is a file, /env/NAME, whose contents are its value
** (this task's own copy of its starter's environment).  getenv reads it; putenv.c writes and removes them.
** (It replaces cc65's getenv, which reads an array made at the start: here nothing is copied until it's asked
** for, and a variable another program set in this task's environment is seen.)
*/

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <hydra.h>

#define ENV_NAME_MAX    30              /* A name's characters (not '/', '=' or 0): the env server's */

static char path[5 + ENV_NAME_MAX + 1];
static char value[HY_ENV_MAX + 1];

/* "/env/NAME" for name, in path; or 0 (EINVAL): not a name */
char* __fastcall__ _hy_envpath (const char* name)
{
    unsigned n = strlen (name);

    if (n == 0 || n > ENV_NAME_MAX || strchr (name, '/') || strchr (name, '=')) {
        _directerrno (EINVAL);
        return 0;
    }
    memcpy (path, "/env/", 5);
    memcpy (path + 5, name, n + 1);
    return path;
}

/* Its value (until the next getenv), or NULL: it isn't set */
char* __fastcall__ getenv (const char* name)
{
    char* p = _hy_envpath (name);
    int fd, n;

    if (p == 0 || (fd = open (p, O_RDONLY)) < 0) {
        return 0;
    }
    n = read (fd, value, HY_ENV_MAX);
    close (fd);
    if (n < 0) {
        return 0;
    }
    value[n] = 0;
    return value;
}
