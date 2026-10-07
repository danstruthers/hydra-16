/*
** race.c - why tasks that share memory need a lock.  Four tasks (this program again, as race -w ...) share a
** counter in a shared segment, and each adds 1 to it 500 times: it reads the counter, works a little, and writes
** it back plus one.  First with nothing to keep them apart: the scheduler stops a task wherever its time runs out,
** so a task stopped between its read and its write writes back an old count when it runs again, and the adds the
** others made meanwhile are lost.  Then with a mutex (hy_sem_new (0, HY_SEM_MUTEX)) around each read and write: a
** task that wants the counter while another has it waits (using no CPU) till it's given back, and none are lost.
** Each task's adds are drawn as they go: this task draws, the others only count.
**   race [TASKS [ADDS]]   (TASKS 1-5, 4 by default; ADDS 1-5000, 500 by default)
**   % /rom/sample/c/race
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <hydra.h>

#define MAX         5               /* Tasks at most (the kernel, init, the drivers and the shell have the rest) */
#define WORK        150             /* The work between a read and its write */
#define WIDTH       50              /* A bar's length */
#define NONE        0xFF            /* (No lock) */

struct shared {                     /* The shared segment's first bank, at $8000 in each task */
    unsigned        counter;        /* The counter they all add to */
    unsigned        done[MAX];      /* Each task's adds so far (each writes only its own) */
};

static volatile struct shared* sh;
static volatile unsigned char work;         /* (volatile: so the work loop isn't taken out) */
static volatile unsigned char stop;         /* Ctrl-C */

/* ---- A worker: race -w SEG READY GO LOCK ADDS N (LOCK: a semaphore, or NONE) */

static int worker (char* argv[])
{
    unsigned char seg = atoi (argv[2]), ready = atoi (argv[3]), go = atoi (argv[4]), lock = atoi (argv[5]);
    unsigned adds = atoi (argv[6]), i, v;
    unsigned char me = atoi (argv[7]);

    if (hy_seg_attach (seg) < 0 || (sh = (struct shared*) hy_seg_map (seg, 0)) == 0) {
        return 1;
    }
    hy_sem_release (ready);                         /* Here, */
    if (hy_sem_acquire (go) < 0) {                  /*   and off when they all are */
        return 1;
    }
    for (i = 1; i <= adds; ++i) {
        if (lock != NONE && hy_sem_acquire (lock) < 0) {
            return 1;
        }
        v = sh->counter;                            /* Read it, */
        for (work = 0; work < WORK; ++work) {       /*   work a little, */
        }
        sh->counter = v + 1;                        /*   and write it back, one more */
        if (lock != NONE) {
            hy_sem_release (lock);
        }
        sh->done[me] = i;
    }
    return 0;
}

/* ---- The race: the workers started and let go together, drawn as they go */

static const char* prog;                    /* This program's file, for the workers */
static unsigned char seg, ready, go, tasks, row;
static unsigned adds;

/* This program's own file, to start its workers from (argv[0] is its name): /bin/NAME (bind -a /rom/sample/c /bin
** puts the samples there), ./NAME, or /rom/sample/c/NAME */
static const char* self (const char* name)
{
    static const char* const dirs[] = { "/bin/", "./", "/rom/sample/c/" };
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

static void at (unsigned char r, unsigned char c)
{
    printf ("\x1b[%u;%uH", r, c);
}

static void interrupted (int sig)
{
    (void) sig;
    stop = 1;
}

/* One race, LOCK around each task's read and write (or NONE): its title, a bar for each task, the counter */
static void race (const char* title, unsigned char lock)
{
    static char* argv[9] = { "race", "-w" };
    static char a[6][6];
    unsigned char task[MAX], drawn[MAX], n, i, len;
    unsigned shown[MAX], d, total, want, t;

    sh->counter = 0;
    memset ((void*) sh->done, 0, sizeof sh->done);
    at (row++, 1);
    printf ("%s\x1b[K", title);
    utoa (seg, a[0], 10);
    utoa (ready, a[1], 10);
    utoa (go, a[2], 10);
    utoa (lock, a[3], 10);
    utoa (adds, a[4], 10);
    for (i = 0; i < 6; ++i) {
        argv[2 + i] = a[i];
    }
    for (n = 0; n < tasks; ++n) {                   /* The workers, each told its number */
        utoa (n, a[5], 10);
        i = hy_spawn (prog, argv, 0);
        if (i == 0xFF) {
            break;                                  /* (No task free: fewer, from now on) */
        }
        task[n] = i;
    }
    tasks = n;
    if (n == 0) {
        printf (" no task free\n");
        stop = 1;
        return;
    }
    for (i = 0; i < n; ++i) {
        at (row + i, 1);
        printf ("  task %-2u [%*s] %5u", task[i], WIDTH, "", 0);
        drawn[i] = 0;
        shown[i] = 0;
    }
    at (row + n, 1);
    printf ("  the counter: 0");
    for (i = 0; i < n && !stop; ++i) {              /* All here, */
        hy_sem_acquire (ready);
    }
    t = hy_ticks ();
    for (i = 0; i < n; ++i) {                       /*   and off together */
        hy_sem_release (go);
    }
    want = n * adds;
    do {
        hy_sleep_ticks (10);
        total = 0;
        for (i = 0; i < n; ++i) {
            d = sh->done[i];
            total += d;
            if (d == shown[i]) {
                continue;
            }
            len = (unsigned long) d * WIDTH / adds;
            if (len > drawn[i]) {
                at (row + i, 12 + drawn[i]);
                for (; drawn[i] < len; ++drawn[i]) {
                    putchar ('#');
                }
            }
            at (row + i, 14 + WIDTH);
            printf ("%5u", d);
            shown[i] = d;
        }
        at (row + n, 16);
        printf ("%u", sh->counter);
        fflush (stdout);
    } while (total < want && !stop);
    for (i = 0; i < n; ++i) {
        hy_wait (task[i], 0);
    }
    t = (hy_ticks () - t) / (HY_TICK_HZ / 10);      /* (Tenths of a second) */
    at (row + n, 1);
    d = sh->counter;
    if (stop) {
        printf ("  the counter: %u (stopped)\x1b[K", d);
    } else if (d == want) {
        printf ("  the counter: %u of %u: none lost (%u.%u s)\x1b[K", d, want, t / 10, t % 10);
    } else {
        printf ("  the counter: %u of %u: %u adds lost (%u.%u s)\x1b[K", d, want, want - d, t / 10, t % 10);
    }
    row += n + 2;
}

int main (int argc, char* argv[])
{
    unsigned char lock;

    if (argc > 1 && strcmp (argv[1], "-w") == 0) {
        return worker (argv);
    }
    tasks = argc > 1 ? atoi (argv[1]) : 4;
    adds = argc > 2 ? atoi (argv[2]) : 500;
    if (tasks < 1 || tasks > MAX || adds < 1 || adds > 5000) {
        fputs ("usage: race [TASKS [ADDS]]   (TASKS 1-5, ADDS 1-5000)\n", stderr);
        hy_exits ("usage");
    }
    if ((prog = self (argv[0])) == 0) {
        fprintf (stderr, "race: can't find myself (/bin/%s, ./%s, /rom/sample/c/%s)\n", argv[0], argv[0], argv[0]);
        hy_exits ("no workers");
    }
    seg = hy_seg_create (1);
    ready = hy_sem_new (0, 0);
    go = hy_sem_new (0, 0);
    lock = hy_sem_new (0, HY_SEM_MUTEX);
    if (seg == 0xFF || ready == 0xFF || go == 0xFF || lock == 0xFF) {
        perror ("race");
        hy_exits ("no segment or semaphore");
    }
    sh = (struct shared*) hy_seg_map (seg, 0);
    signal (SIGINT, interrupted);
    printf ("\x1b[H\x1b[2J");
    printf ("race: %u tasks share a counter, and each adds 1 to it %u times:\n", tasks, adds);
    printf ("reads it, works a little, and writes it back plus one");
    row = 4;
    race ("With nothing to keep them apart:", NONE);
    if (!stop) {
        race ("With a mutex around each read and write:", lock);
    }
    at (row, 1);
    if (stop) {
        hy_exits ("interrupted");
    }
    printf ("A task the scheduler stopped between its read and its write wrote back an old\n"
            "count when it ran again, and the adds the others had made meanwhile were lost.\n"
            "The mutex lets one task at a time between the read and the write: the others\n"
            "wait for it, using no CPU.\n");
    return 0;
}
