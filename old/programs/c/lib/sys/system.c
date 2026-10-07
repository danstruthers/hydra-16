/*
** system.c - running a command line as the shell would (Plan 9's rc -c): system(), hy_spawn(), and hy_exits().
** The command runs in a command shell (the ROM's SHELL_CMD: HyForth reading its stdin, with no banner or prompt,
** which ends at its input's end with the last command's exit status) in a task of its own, with this program's
** fds, namespace, current directory and environment.  So it can be a program, a script, a pipeline, a HyForth
** word: anything the shell's prompt takes.  Its stdin is a pipe with the line in it (written, and its writing end
** closed, before the shell starts: the shell's copy of the fds mustn't have it, or its input wouldn't end).
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <hydra.h>

#define CMD_MAX     250                 /* A pipe holds 255 bytes: the line, and its CR LF */

int __fastcall__ _hy_pipe (int* fds);
int __fastcall__ _hy_dup (int fd);
int __fastcall__ _hy_dup2 (int fd, int newfd);
int _hy_shell (void);

extern const char* _hy_exitmsg;         /* crt0.s: the message hy_exits leaves for _exit */

int __fastcall__ hy_spawn (const char* cmd)
{
    int fds[2], in, task;
    unsigned n = strlen (cmd);

    if (n > CMD_MAX) {
        return _directerrno (EINVAL);
    }
    if (_hy_pipe (fds) < 0) {
        return -1;
    }
    if (write (fds[1], cmd, n) != (int) n || write (fds[1], "\r\n", 2) != 2) {
        close (fds[0]);
        close (fds[1]);
        return -1;
    }
    close (fds[1]);
    in = _hy_dup (0);                   /* stdin, kept; the pipe's reading end, stdin for the shell */
    _hy_dup2 (fds[0], 0);
    close (fds[0]);
    task = _hy_shell ();
    if (in >= 0) {
        _hy_dup2 (in, 0);
        close (in);
    } else {
        close (0);
    }
    return task;
}

int __fastcall__ system (const char* s)
{
    int task;

    if (s == 0) {
        return 1;                       /* There's a shell */
    }
    task = hy_spawn (s);
    if (task < 0) {
        return -1;
    }
    return hy_wait (task, 0);
}

void __fastcall__ hy_exits (const char* msg)
{
    if (msg == 0 || *msg == 0) {
        exit (0);
    }
    _hy_exitmsg = msg;
    exit (1);
}
