/*
** putenv.c - setting and removing environment variables (getenv.c: the kernel keeps them): putenv ("NAME=value";
** "NAME" alone removes it), setenv and unsetenv.  They change this task's environment; the tasks it starts from
** then on get a copy (rc's commands among them: system's).  The string putenv is given isn't kept: its value is.
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <hydra.h>

int __fastcall__ setenv (const char* name, const char* value, int overwrite)
{
    struct hy_regs r;
    char c;

    if (!overwrite) {                   /* (There already, and to stay?) */
        r.a = 0xFF;
        r.r[0] = (unsigned) name;
        r.r[1] = (unsigned) &c;
        r.r[2] = 0;
        r.r[3] = 0;
        if (hy_call (HY_ENV_GET, &r) == 0) {
            return 0;
        }
    }
    r.a = 0xFF;
    r.r[0] = (unsigned) name;
    r.r[1] = (unsigned) value;
    r.r[2] = strlen (value);
    r.r[3] = 0;                         /* (From 0: its value whole) */
    return hy_call (HY_ENV_PUT, &r) ? -1 : 0;
}

int __fastcall__ unsetenv (const char* name)
{
    struct hy_regs r;

    r.a = 0xFF;
    r.r[0] = (unsigned) name;
    if (hy_call (HY_ENV_DEL, &r) && _oserror != HY_E_NOENT) {
        return -1;
    }
    return 0;
}

int __fastcall__ putenv (char* s)
{
    char name[HY_ENV_NAME_MAX + 1];
    char* eq = strchr (s, '=');
    unsigned n;

    if (eq == 0) {
        return unsetenv (s);
    }
    n = eq - s;
    if (n == 0 || n > HY_ENV_NAME_MAX) {
        return _directerrno (EINVAL);
    }
    memcpy (name, s, n);
    name[n] = 0;
    return setenv (name, eq + 1, 1);
}
