/*
** putenv.c - setting and removing environment variables (getenv.c: each is a file, /env/NAME): putenv ("NAME=value";
** "NAME" alone removes it), setenv and unsetenv.  They change this task's environment, and the tasks it starts
** from then on get a copy.  (The string putenv is given isn't kept: its value is written.)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <hydra.h>

char* __fastcall__ _hy_envpath (const char* name);

int __fastcall__ setenv (const char* name, const char* value, int overwrite)
{
    char* p = _hy_envpath (name);
    int fd, n;

    if (p == 0) {
        return -1;
    }
    if (!overwrite && (fd = open (p, O_RDONLY)) >= 0) {
        close (fd);                     /* (There already, and to stay) */
        return 0;
    }
    n = strlen (value);
    if (n > HY_ENV_MAX) {
        return _directerrno (EINVAL);
    }
    if ((fd = open (p, O_WRONLY | O_CREAT | O_TRUNC)) < 0) {
        return -1;
    }
    if (n && write (fd, value, n) != n) {
        close (fd);
        return -1;
    }
    return close (fd);
}

int __fastcall__ unsetenv (const char* name)
{
    char* p = _hy_envpath (name);

    if (p == 0) {
        return -1;
    }
    if (remove (p) < 0 && errno != ENOENT) {
        return -1;
    }
    return 0;
}

int __fastcall__ putenv (char* s)
{
    char name[31];
    char* eq = strchr (s, '=');
    unsigned n;

    if (eq == 0) {
        return unsetenv (s);
    }
    n = eq - s;
    if (n >= sizeof name) {
        return _directerrno (EINVAL);
    }
    memcpy (name, s, n);
    name[n] = 0;
    return setenv (name, eq + 1, 1);
}
