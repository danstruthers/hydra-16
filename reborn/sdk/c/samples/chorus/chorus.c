/*
** chorus.c - the console as a shared resource.  Four tasks (this program again, as chorus -w ...) sing a line each
** of "Row, Row, Row Your Boat" on the one console, a letter at a time (a write each, then a pause), three times:
**   - with nothing to keep them apart: their letters tangle;
**   - with a mutex (hy_sem_new (0, HY_SEM_MUTEX)) that a task holds for its whole line: each line comes out
**     whole, in the order the tasks got the mutex;
**   - with a baton: a semaphore for each task, which it waits for, then sings, then gives the next task's (a
**     semaphore, not a mutex: a task may give back one it didn't take).  The first task's starts at 1, the
**     others' at 0: the lines come out whole, and in turn.
** Each time, the tasks are started, wait at a barrier (each gives a "ready" semaphore one, and this task takes
** them all), then are let go together (this task gives "go" one for each).
**   chorus
**   % /sd/0/sample/c/chorus
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <hydra.h>

#define VOICES      4

enum { NOTHING, MUTEX, BATON };

static const char* const lines[VOICES] = {
    "Row, row, row your boat,",
    "Gently down the stream.",
    "Merrily, merrily, merrily, merrily,",
    "Life is but a dream."
};

/* ---- A worker: chorus -w LINE HOW READY GO LOCK MINE NEXT */

static int worker (char* argv[])
{
    const char* s = lines[atoi (argv[2])];
    unsigned char how = atoi (argv[3]), ready = atoi (argv[4]), go = atoi (argv[5]), lock = atoi (argv[6]);
    unsigned char mine = atoi (argv[7]), next = atoi (argv[8]);

    srand (hy_ticks () + hy_task () * 77);
    hy_sem_release (ready);                         /* Here, */
    if (hy_sem_acquire (go) < 0) {                  /*   and off when they all are, */
        return 1;
    }
    hy_sleep_ticks (rand () % 40);                  /*   each coming in when it's ready */
    if ((how == MUTEX && hy_sem_acquire (lock) < 0) || (how == BATON && hy_sem_acquire (mine) < 0)) {
        return 1;
    }
    for (; *s; ++s) {
        putchar (*s);
        fflush (stdout);                            /* A letter: a write of its own */
        hy_sleep_ticks (2 + rand () % 4);
    }
    putchar ('\n');
    fflush (stdout);
    if (how == MUTEX) {
        hy_sem_release (lock);
    } else if (how == BATON) {
        hy_sem_release (next);                      /* The next one's turn */
    }
    return 0;
}

/* ---- The chorus */

static const char* prog;
static unsigned char ready, go, lock, baton[VOICES], voices = VOICES;

/* This program's own file, to start its workers from (argv[0] is its name): /bin/NAME (bind -a /sd/0/sample/c /bin
** puts the samples there), ./NAME, or /sd/0/sample/c/NAME */
static const char* self (const char* name)
{
    static const char* const dirs[] = { "/bin/", "./", "/sd/0/sample/c/" };
    static char path[48];
    struct stat st;
    unsigned char i;

    for (i = 0; i < 3; ++i) {
        strcpy (path, dirs[i]);
        strcat (path, name);
        if (stat (path, &st) == 0) {
            return path;
        }
    }
    return 0;
}

/* The voices sing once, HOW keeping them apart */
static void sing (const char* title, unsigned char how)
{
    static char* argv[10] = { "chorus", "-w" };
    static char a[7][4];
    unsigned char task[VOICES], n, i;

    utoa (how, a[1], 10);
    utoa (ready, a[2], 10);
    utoa (go, a[3], 10);
    utoa (lock, a[4], 10);
    for (i = 0; i < 7; ++i) {
        argv[2 + i] = a[i];
    }
    for (n = 0; n < voices; ++n) {                  /* Each its line, its baton and the next one's */
        utoa (n, a[0], 10);
        utoa (baton[n], a[5], 10);
        utoa (baton[(n + 1) % voices], a[6], 10);
        i = hy_spawn (prog, argv, 0);
        if (i == 0xFF) {
            break;
        }
        task[n] = i;
    }
    if (n < voices) {                               /* (No task free: fewer voices, the batons passed round them) */
        printf ("(%u tasks free: %u voices)\n", n, n);
        for (i = 0; i < n; ++i) {
            hy_note (task[i], HY_NOTE_KILL);
            hy_wait (task[i], 0);
        }
        voices = n;
        if (n) {
            sing (title, how);
        }
        return;
    }
    printf ("\n%s\n", title);
    for (i = 0; i < n; ++i) {                       /* All here, */
        hy_sem_acquire (ready);
    }
    for (i = 0; i < n; ++i) {                       /*   and off together */
        hy_sem_release (go);
    }
    for (i = 0; i < n; ++i) {
        hy_wait (task[i], 0);
    }
}

int main (int argc, char* argv[])
{
    unsigned char i;

    if (argc > 1 && strcmp (argv[1], "-w") == 0) {
        return worker (argv);
    }
    if ((prog = self (argv[0])) == 0) {
        fprintf (stderr, "chorus: can't find myself (/bin/%s, ./%s, /sd/0/sample/c/%s)\n", argv[0], argv[0], argv[0]);
        hy_exits ("no workers");
    }
    ready = hy_sem_new (0, 0);
    go = hy_sem_new (0, 0);
    lock = hy_sem_new (0, HY_SEM_MUTEX);
    for (i = 0; i < VOICES; ++i) {
        baton[i] = hy_sem_new (i == 0, 0);          /* The first voice's to start with */
    }
    if (ready == 0xFF || go == 0xFF || lock == 0xFF || baton[VOICES - 1] == 0xFF) {
        perror ("chorus");
        hy_exits ("no semaphore");
    }
    printf ("chorus: four tasks sing a line each, a letter at a time, on the one console\n");
    sing ("With nothing to keep them apart:", NOTHING);
    sing ("With a mutex, held for a whole line:", MUTEX);
    sing ("With a baton passed round, a semaphore each (wait for yours, sing, pass it on):", BATON);
    return 0;
}
