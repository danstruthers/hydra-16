/*
** ctest.c - the C library's test (tests/tests.js's c runs it at rc): its arguments and name, files (stdio and the
** calls under it), errors, directories, the heap, the time, the environment, stat, commands and their exit
** statuses (system, hy_spawn, hy_wait: with the sample code), the namespace, a note as a signal, RAM banks, isatty.
** Each check prints "ok - " or "not ok - " and its name; the last line is "ctest: N failed", and its exit status
** is the count.  Run it as ctest a 'b c', in a directory it can write in (/ram), with the C samples in
** /rom/sample/c.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <hydra.h>

#define CODE    "/rom/sample/c/code"

static int failed;
static volatile unsigned char got;

static void check (int good, const char* what)
{
    printf ("%s - %s", good ? "ok" : "not ok", what);
    if (!good) {
        printf (" (errno %d, _oserror $%02X)", errno, _oserror);
        ++failed;
    }
    printf ("\n");
}

static void interrupted (int sig)
{
    got = sig;
}

int main (int argc, char* argv[])
{
    static char* args[] = { "code", "oops", 0 };
    FILE* f;
    char line[64];
    char cwd[HY_PATH_MAX + 1];
    char msg[HY_EXIT_MSG_MAX + 1];
    int fd, i;
    char* p;
    unsigned start;
    struct stat st;
    DIR* dir;
    struct dirent* de;
    unsigned char seen;
    clock_t c;

    check (argc == 3 && strcmp (argv[1], "a") == 0 && strcmp (argv[2], "b c") == 0, "arguments");
    check (strcmp (argv[0], "ctest") == 0, "argv[0]: its name (its file's)");

    /* stdio: write, read back, seek */
    f = fopen ("ct.txt", "w");
    check (f != 0, "fopen w");
    fprintf (f, "line one\nline %d\n", 2);
    check (fclose (f) == 0, "fclose");
    f = fopen ("ct.txt", "r");
    check (f != 0 && fgets (line, sizeof line, f) != 0 && strcmp (line, "line one\n") == 0, "fgets");
    check (fgets (line, sizeof line, f) != 0 && strcmp (line, "line 2\n") == 0, "fgets again");
    check (fgets (line, sizeof line, f) == 0 && feof (f), "the end");
    check (fseek (f, 5, SEEK_SET) == 0 && ftell (f) == 5 && fgetc (f) == 'o', "fseek, ftell");
    fclose (f);

    /* append, the size (SEEK_END), SEEK_CUR */
    f = fopen ("ct.txt", "a");
    check (f != 0 && fputs ("three\n", f) >= 0 && fclose (f) == 0, "append");
    fd = open ("ct.txt", O_RDONLY);
    check (fd >= 0 && lseek (fd, 0, SEEK_END) == 22, "lseek SEEK_END");
    check (lseek (fd, -6, SEEK_CUR) == 16 && read (fd, line, 5) == 5 && memcmp (line, "three", 5) == 0,
           "lseek SEEK_CUR, read");
    close (fd);

    /* O_CREAT | O_EXCL, O_TRUNC */
    fd = open ("ct.txt", O_WRONLY | O_CREAT | O_EXCL);
    check (fd < 0 && errno == EEXIST, "O_EXCL on a file there: EEXIST");
    fd = open ("ct.txt", O_WRONLY | O_TRUNC);
    check (fd >= 0 && write (fd, "x", 1) == 1 && close (fd) == 0, "O_TRUNC");
    fd = open ("ct.txt", O_RDONLY);
    check (fd >= 0 && lseek (fd, 0, SEEK_END) == 1, "truncated");
    close (fd);

    /* rename, remove, errors */
    check (rename ("ct.txt", "ct2.txt") == 0 && fopen ("ct.txt", "r") == 0 && errno == ENOENT, "rename");
    check (remove ("ct2.txt") == 0 && remove ("ct2.txt") != 0 && errno == ENOENT, "remove");
    check (fopen ("nosuch/x", "r") == 0 && errno == ENOENT && _oserror == HY_E_NOENT, "errno, _oserror");
    check (strcmp (_stroserror (HY_E_NOENT), "not found") == 0 && strncmp (_stroserror (0x55), "error", 5) == 0,
           "_stroserror: the kernel's texts");
    check (close (9) < 0 && _oserror == HY_E_BADF && errno == EBADF, "close: a bad fd");

    /* directories */
    check (getcwd (cwd, sizeof cwd) != 0, "getcwd");
    check (mkdir ("ctdir", 0) == 0 && chdir ("ctdir") == 0, "mkdir, chdir");
    check (getcwd (line, sizeof line) != 0 && strlen (line) == strlen (cwd) + (strcmp (cwd, "/") ? 6 : 5)
           && strcmp (line + strlen (line) - 5, "ctdir") == 0, "getcwd there");
    check (chdir ("..") == 0 && rmdir ("ctdir") == 0, "rmdir");

    /* the heap */
    p = malloc (8000);
    check (p != 0, "malloc 8000");
    memset (p, 0x5A, 8000);
    check (p[7999] == 0x5A, "the heap's memory");
    free (p);
    check (malloc (40000u) == 0, "malloc too much: NULL");

    /* the time, sleeping */
    check (time (0) >= 946684800L, "time: 2000 or after");
    start = hy_ticks ();
    hy_sleep_ticks (20);
    check ((unsigned) (hy_ticks () - start) >= 20, "hy_sleep_ticks");
    c = clock ();
    sleep (1);
    check (clock () - c >= CLOCKS_PER_SEC, "sleep, clock");

    /* the environment (rc's variables) */
    check (setenv ("CTEST", "one", 1) == 0 && strcmp (getenv ("CTEST"), "one") == 0, "setenv, getenv");
    check (setenv ("CTEST", "two", 0) == 0 && strcmp (getenv ("CTEST"), "one") == 0, "setenv, not replacing");
    check (putenv ("CTEST=three") == 0 && strcmp (getenv ("CTEST"), "three") == 0, "putenv");
    check (unsetenv ("CTEST") == 0 && getenv ("CTEST") == 0, "unsetenv");

    /* stat, fstat, directories */
    f = fopen ("ct.txt", "w");
    fputs ("12345", f);
    fclose (f);
    check (stat ("ct.txt", &st) == 0 && st.st_size == 5 && !S_ISDIR (st.st_mode) && (st.st_mode & S_IWRITE), "stat");
    check (mkdir ("ctdir2", 0) == 0 && stat ("ctdir2", &st) == 0 && S_ISDIR (st.st_mode), "stat: a directory");
    fd = open ("ct.txt", O_RDONLY);
    check (fd >= 0 && fstat (fd, &st) == 0 && st.st_size == 5, "fstat");
    close (fd);
    dir = opendir (".");
    seen = 0;
    i = 0;
    while (dir && (de = readdir (dir)) != 0) {
        ++i;
        if (strcmp (de->d_name, "ct.txt") == 0 && hy_dirstat (dir, &st) == 0 && st.st_size == 5) {
            seen |= 1;
        }
        if (strcmp (de->d_name, "ctdir2") == 0) {
            seen |= 2;
        }
    }
    check (dir != 0 && closedir (dir) == 0 && seen == 3 && i >= 2, "opendir, readdir, hy_dirstat");
    check (remove ("ct.txt") == 0 && rmdir ("ctdir2") == 0, "(tidied up)");

    /* commands (rc -c), exit statuses */
    check (system (0) == 1, "system (NULL): there's a shell");
    check (system (CODE " 3") == 3, "system: an exit code");
    check (system (CODE) == 0, "system: success");
    check (system ("echo piped | " CODE " 4 && echo no") == 4, "system: a pipeline");
    i = hy_spawn (CODE, args, 0);
    check (i > 0 && hy_wait (i, msg) == 1 && strcmp (msg, "oops") == 0, "hy_spawn, hy_wait: a message");
    check (hy_spawn ("/nosuch", args, 0) < 0 && errno == ENOENT, "hy_spawn: no such program");
    check (hy_parent () < 16 && hy_parent () != hy_task (), "hy_parent: the shell's task");

    /* semaphores */
    i = hy_sem_new (1, 0);
    check (i >= 0 && hy_sem_try (i) == 0 && hy_sem_try (i) < 0 && errno == EAGAIN, "hy_sem_new, hy_sem_try");
    check (hy_sem_release (i) == 0 && hy_sem_acquire (i) == 0 && hy_sem_release (i) == 0,
           "hy_sem_acquire, hy_sem_release");
    check (hy_sem_free (i) == 0 && hy_sem_try (i) < 0 && _oserror == HY_E_INVAL, "hy_sem_free");
    i = hy_sem_new (0, HY_SEM_MUTEX);
    check (i >= 0 && hy_sem_acquire (i) == 0 && hy_sem_try (i) < 0 && _oserror == HY_E_BUSY && hy_sem_free (i) == 0,
           "a mutex");

    /* the namespace: a bind, a union, an unmount; a mount */
    getcwd (cwd, sizeof cwd);
    strcat (cwd, "/ctu");
    check (mkdir ("ctu", 0) == 0, "(a mount point)");
    check (hy_bind ("/rom/sample/c", cwd, HY_MREPL) == 0 && stat ("ctu/code", &st) == 0, "hy_bind");
    check (hy_bind ("/rom/sample", cwd, HY_MAFTER) == 0 && stat ("ctu/hi", &st) == 0 && stat ("ctu/code", &st) == 0,
           "hy_bind: a union");
    check (hy_unmount ("/rom/sample", cwd) == 0 && stat ("ctu/hi", &st) != 0 && stat ("ctu/code", &st) == 0,
           "hy_unmount: a member");
    check (hy_unmount (0, cwd) == 0 && stat ("ctu/code", &st) != 0, "hy_unmount: all of it");
    fd = -1;
    check (hy_mount ('n', 0, cwd, HY_MREPL) == 0 && (fd = open ("ctu/zero", O_RDONLY)) >= 0 && read (fd, msg, 2) == 2
           && msg[0] == 0 && msg[1] == 0, "hy_mount: #n");
    close (fd);
    check (hy_unmount (0, cwd) == 0 && rmdir ("ctu") == 0, "(unmounted)");

    /* a note as a signal: SIGINT's handler runs; SIG_IGN */
    signal (SIGINT, interrupted);
    hy_note (hy_task (), HY_NOTE_INTERRUPT);
    for (i = 0; i < 10 && !got; ++i) {
        hy_yield ();
    }
    check (got == SIGINT, "signal: the interrupt note, SIGINT's handler");
    got = 0;
    signal (SIGINT, SIG_IGN);
    hy_note (hy_task (), HY_NOTE_INTERRUPT);
    hy_yield ();
    check (got == 0, "signal: SIG_IGN, the note ignored");
    signal (SIGINT, SIG_DFL);

    /* RAM banks */
    i = hy_banks_alloc (2);
    check (i >= 0, "hy_banks_alloc");
    hy_bank (i);
    HY_BANK_WINDOW[0] = 0x11;
    hy_bank (i + 1);
    HY_BANK_WINDOW[0] = 0x22;
    hy_bank (i);
    check (HY_BANK_WINDOW[0] == 0x11, "a bank's memory, its own");
    check (hy_banks_free (i, 2) == 0, "hy_banks_free");

    /* isatty */
    check (isatty (1) && !isatty (9), "isatty");

    printf ("ctest: %d failed\n", failed);
    return failed;
}
