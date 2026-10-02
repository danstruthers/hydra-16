/*
** ctest.c - the C library's test (sim/regress.js runs it on a card): its arguments and name, files (stdio and the
** calls under it), directories, errors, the heap, the clock, semaphores, the environment, stat, running commands
** and their exit statuses (system, hy_spawn, hy_wait: with code.hyx in /bin), clock and isatty.  Each check prints
** "ok" or "FAIL" and its name; the last line is "ctest: N failed", and its exit status is the count.  Run it as
** ctest a "b c", in a directory it can write in.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <hydra.h>

static int failed;

static void check (int good, const char* what)
{
    printf ("%s %s\n", good ? "ok" : "FAIL", what);
    if (!good) {
        ++failed;
    }
}

int main (int argc, char* argv[])
{
    FILE* f;
    char line[64];
    char cwd[HY_PATH_MAX];
    int fd, s, i;
    char* p;
    unsigned long start;
    struct stat st;
    DIR* dir;
    struct dirent* de;
    unsigned char seen;
    char msg[HY_STATUS_MAX];
    clock_t c;

    check (argc == 3 && strcmp (argv[1], "a") == 0 && strcmp (argv[2], "b c") == 0, "arguments");
    check (strcmp (argv[0], "ctest") == 0, "argv[0]: its name");

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
    check (fd < 0 && errno == EEXIST, "O_EXCL on a file there");
    fd = open ("ct.txt", O_WRONLY | O_TRUNC);
    check (fd >= 0 && write (fd, "x", 1) == 1 && close (fd) == 0, "O_TRUNC");
    fd = open ("ct.txt", O_RDONLY);
    check (fd >= 0 && lseek (fd, 0, SEEK_END) == 1, "truncated");
    close (fd);

    /* rename, remove, errors */
    check (rename ("ct.txt", "ct2.txt") == 0 && fopen ("ct.txt", "r") == 0 && errno == ENOENT, "rename");
    check (remove ("ct2.txt") == 0 && remove ("ct2.txt") != 0 && errno == ENOENT, "remove");
    check (fopen ("nosuch/x", "r") == 0 && errno == ENOENT && _oserror == 0x70, "errno, _oserror");
    check (strcmp (_stroserror (_oserror), "not found") == 0 && strcmp (_stroserror (0x55), "unknown error") == 0, "_stroserror");

    /* directories */
    check (getcwd (cwd, sizeof cwd) != 0, "getcwd");
    check (mkdir ("ctdir", 0) == 0 && chdir ("ctdir") == 0, "mkdir, chdir");
    check (getcwd (line, sizeof line) != 0 && strlen (line) == strlen (cwd) + (strcmp (cwd, "/") ? 6 : 5) &&
           strcmp (line + strlen (line) - 5, "ctdir") == 0, "getcwd there");
    check (chdir ("..") == 0 && rmdir ("ctdir") == 0, "rmdir");

    /* the heap */
    p = malloc (8000);
    check (p != 0, "malloc 8000");
    memset (p, 0x5A, 8000);
    check (p[7999] == 0x5A, "the heap's memory");
    free (p);
    check (malloc (40000u) == 0, "malloc too much");

    /* the clock, and sleeping */
    check (time (0) >= 946684800L, "time");
    start = hy_ticks ();
    hy_sleep_ticks (20);
    check ((unsigned) (hy_ticks () - start) >= 20, "hy_sleep_ticks");

    /* semaphores */
    s = hy_sem_new (2);
    check (s > 0, "hy_sem_new");
    check (hy_sem_try (s) == 1 && hy_sem_try (s) == 1 && hy_sem_try (s) == 0, "hy_sem_try");
    check (hy_sem_release (s) == 0 && hy_sem_acquire (s) == 0, "release, acquire");
    check (hy_sem_free (s) == 0 && hy_sem_try (s) < 0, "hy_sem_free");
    i = hy_mutex_new ();
    check (i > 0 && hy_sem_acquire (i) == 0 && hy_sem_release (i) == 0 && hy_sem_release (i) < 0, "a mutex");
    hy_sem_free (i);

    /* the environment (/env) */
    check (setenv ("CTEST", "one", 1) == 0 && strcmp (getenv ("CTEST"), "one") == 0, "setenv, getenv");
    check (setenv ("CTEST", "two", 0) == 0 && strcmp (getenv ("CTEST"), "one") == 0, "setenv, not replacing");
    check (putenv ("CTEST=three") == 0 && strcmp (getenv ("CTEST"), "three") == 0, "putenv");
    check (unsetenv ("CTEST") == 0 && getenv ("CTEST") == 0, "unsetenv");
    check (getenv ("a/b") == 0 && errno == EINVAL, "a bad name");

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
    check (dir != 0 && closedir (dir) == 0 && seen == 3 && i >= 3, "opendir, readdir, hy_dirstat");
    check (remove ("ct.txt") == 0 && rmdir ("ctdir2") == 0, "(tidied up)");

    /* running commands (the command shell): system, hy_spawn, hy_wait; exit statuses */
    check (system (0) == 1, "system (NULL)");
    check (system ("code 3") == 3, "system: an exit code");
    check (system ("code") == 0, "system: success");
    i = hy_spawn ("code oops");
    check (i > 0 && hy_wait (i, msg) == 1 && strcmp (msg, "oops") == 0, "hy_spawn, hy_wait: a message");
    check (system ("1 2 + drop") == 0, "system: HyForth");
    check (system ("run bin/code.hyx 4") == 4, "system: run");

    /* the namespace: a bind, a union (no create in it with no HY_MCREATE member), a hide, unmounts, a mount */
    check (hy_bind ("/rom/bin", "/ctu", HY_MREPL) == 0 && stat ("/ctu/code.hyx", &st) == 0, "hy_bind");
    check (hy_bind ("/rom/songs", "/ctu", HY_MAFTER) == 0 && stat ("/ctu/test.zsm", &st) == 0
        && stat ("/ctu/code.hyx", &st) == 0, "hy_bind: a union");
    check (fopen ("/ctu/new", "w") == 0, "a create in a union with no HY_MCREATE member: refused");
    check (hy_unmount ("/rom/songs", "/ctu") == 0 && stat ("/ctu/test.zsm", &st) != 0 && stat ("/ctu/hello.hyx", &st) == 0,
        "hy_unmount: a member");
    check (hy_hide ("/ctu/code.hyx") == 0 && stat ("/ctu/code.hyx", &st) != 0 && stat ("/ctu/hello.hyx", &st) == 0, "hy_hide");
    check (hy_unmount (0, "/ctu") == 0 && hy_unmount (0, "/ctu/code.hyx") == 0 && stat ("/ctu/hello.hyx", &st) != 0
        && hy_unmount (0, "/ctu") == -1, "hy_unmount: all of them");
    fd = -1;
    check (hy_mount ("zero", "/ctz", HY_MREPL, 0) == 0 && (fd = open ("/ctz", O_RDONLY)) >= 0 && read (fd, msg, 2) == 2
        && msg[0] == 0 && msg[1] == 0, "hy_mount");
    close (fd);
    check (hy_unmount (0, "/ctz") == 0 && hy_mount ("nodev", "/ctz", HY_MREPL, 0) == -1, "hy_mount: no such device");
    check (hy_bind ("/rom/bin", "/ctu", HY_MREPL) == 0 && hy_hide ("/sram") == 0 && hy_newns () == 0
        && stat ("/ctu/code.hyx", &st) != 0 && stat ("/sram/bin", &st) == 0 && stat ("/ram", &st) == 0, "hy_newns");

    /* clock, isatty */
    c = clock ();
    hy_sleep_ticks (10);
    check (clock () - c >= 10, "clock");
    check (isatty (1) && !isatty (9), "isatty");

    printf ("ctest: %d failed\n", failed);
    return failed;
}
