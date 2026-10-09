/*
** philo.c - the dining philosophers.  Five tasks (this program again, as philo -w ...) sit round a table with a
** fork between each two, and each fork is a mutex (hy_sem_new (0, HY_SEM_MUTEX)).  A philosopher thinks, gets
** hungry, takes the two forks beside them (waiting, using no CPU, for one a neighbour holds), eats, puts them
** down, and thinks again.  Each takes the lower-numbered of their two forks first, so they can never all hold
** one and wait for another: no deadlock.  With -d, each takes the fork on their left first, then, a moment
** later, the one on their right: soon they all hold one fork and wait for ever for the other.  This task draws
** the table as it goes (the philosophers write what they're doing in a shared segment), sees a deadlock if one
** comes, and ends them (a kill note: the end of a task gives back the mutexes it holds).
**   philo [-d] [-n MEALS] [PHILOSOPHERS]   (2-5, 5 by default; MEALS each, then the end; none: till Ctrl-C)
**   % /sd/0/sample/c/philo
**   % /sd/0/sample/c/philo -d
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <hydra.h>

#define MAX         5

enum { THINKING, HUNGRY, EATING, DONE };

struct table {                      /* The shared segment's first bank, at $8000 in each task */
    unsigned char   doing[MAX];     /* Each philosopher's: THINKING ... */
    unsigned char   hand[MAX];      /* The forks each holds (0-2) */
    unsigned char   holder[MAX];    /* Fork f's holder: a philosopher + 1, or 0 */
    unsigned        meals[MAX];
    unsigned char   clashes;        /* A fork taken while the table says another holds it: never, with mutexes */
};

static volatile struct table* t;
static unsigned char forks[MAX];    /* The forks' semaphores */
static unsigned char me;

/* ---- A philosopher: philo -w SEG READY GO ME N MEALS DEADLY FORK ... */

static void take (unsigned char f)
{
    if (hy_sem_acquire (forks[f]) < 0) {            /* (Freed: this program's first task has ended) */
        exit (1);
    }
    if (t->holder[f]) {
        ++t->clashes;
    }
    t->holder[f] = me + 1;
    ++t->hand[me];
}

static void put (unsigned char f)
{
    t->holder[f] = 0;
    --t->hand[me];
    hy_sem_release (forks[f]);
}

static int philosopher (char* argv[])
{
    unsigned char seg = atoi (argv[2]), ready = atoi (argv[3]), go = atoi (argv[4]), n = atoi (argv[6]);
    unsigned meals = atoi (argv[7]), m;
    unsigned char deadly = atoi (argv[8]), first, second, f;

    me = atoi (argv[5]);
    for (f = 0; f < n; ++f) {
        forks[f] = atoi (argv[9 + f]);
    }
    if (hy_seg_attach (seg) < 0 || (t = (struct table*) hy_seg_map (seg, 0)) == 0) {
        return 1;
    }
    first = me;                                     /* The fork on their left, */
    second = (me + 1) % n;                          /*   and on their right */
    if (!deadly && second < first) {                /* The lower-numbered first */
        first = second;
        second = me;
    }
    srand (hy_ticks () + me * 131);
    hy_sem_release (ready);
    if (hy_sem_acquire (go) < 0) {
        return 1;
    }
    for (m = 0; meals == 0 || m < meals; ++m) {
        t->doing[me] = THINKING;
        hy_sleep_ticks (deadly ? rand () % 10 : 40 + rand () % 160);
        t->doing[me] = HUNGRY;
        take (first);
        if (deadly) {
            hy_sleep_ticks (60);                    /* (Long enough for the others to take theirs) */
        }
        take (second);
        t->doing[me] = EATING;
        hy_sleep_ticks (60 + rand () % 140);
        ++t->meals[me];
        put (second);
        put (first);
    }
    t->doing[me] = DONE;
    return 0;
}

/* ---- The table, drawn */

static const char* const names[MAX] = { "Plato", "Confucius", "Socrates", "Voltaire", "Descartes" };
static const char* const doings[] = { "thinking", "hungry", "eating", "done" };
static volatile unsigned char stop;

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

static void at (unsigned char r, unsigned char c)
{
    printf ("\x1b[%u;%uH", r, c);
}

static void interrupted (int sig)
{
    (void) sig;
    stop = 1;
}

static void usage (void)
{
    fputs ("usage: philo [-d] [-n MEALS] [PHILOSOPHERS]   (2-5)\n", stderr);
    hy_exits ("usage");
}

int main (int argc, char* argv[])
{
    static char* args[9 + MAX + 1] = { "philo", "-w" };
    static char a[7 + MAX][6];
    static unsigned char task[MAX], doing[MAX], hand[MAX], holder[MAX];
    static unsigned meals[MAX];
    const char* prog;
    unsigned char seg, ready, go, n = MAX, deadly = 0, i, k, stuck = 0, deadlock = 0;
    unsigned want = 0, sum, least, most;

    if (argc > 1 && strcmp (argv[1], "-w") == 0) {
        return philosopher (argv);
    }
    for (i = 1; i < argc; ++i) {
        if (strcmp (argv[i], "-d") == 0) {
            deadly = 1;
        } else if (strcmp (argv[i], "-n") == 0 && i + 1 < argc) {
            want = atoi (argv[++i]);
        } else if (argv[i][0] >= '0' && argv[i][0] <= '9') {
            n = atoi (argv[i]);
        } else {
            usage ();
        }
    }
    if (n < 2 || n > MAX) {
        usage ();
    }
    if ((prog = self (argv[0])) == 0) {
        fprintf (stderr, "philo: can't find myself (/bin/%s, ./%s, /sd/0/sample/c/%s)\n", argv[0], argv[0], argv[0]);
        hy_exits ("no workers");
    }
    seg = hy_seg_create (1);
    ready = hy_sem_new (0, 0);
    go = hy_sem_new (0, 0);
    for (i = 0; i < n; ++i) {
        forks[i] = hy_sem_new (0, HY_SEM_MUTEX);
    }
    if (seg == 0xFF || ready == 0xFF || go == 0xFF || forks[n - 1] == 0xFF) {
        perror ("philo");
        hy_exits ("no segment or semaphore");
    }
    t = (struct table*) hy_seg_map (seg, 0);
    memset ((void*) t, 0, sizeof *t);

    /* The philosophers, each told the segment, the semaphores, its number and how many there are */
    utoa (seg, a[0], 10);
    utoa (ready, a[1], 10);
    utoa (go, a[2], 10);
    utoa (want, a[5], 10);
    utoa (deadly, a[6], 10);
    utoa (n, a[4], 10);
    for (i = 0; i < 7 + MAX; ++i) {
        args[2 + i] = a[i];
    }
    for (k = 0; k < n; ++k) {
        utoa (forks[k], a[7 + k], 10);
    }
    args[9 + n] = 0;
    for (k = 0; k < n; ++k) {
        utoa (k, a[3], 10);
        i = hy_spawn (prog, args, 0);
        if (i == 0xFF) {
            break;
        }
        task[k] = i;
    }
    if (k < n) {                                    /* (No task free for them all) */
        for (i = 0; i < k; ++i) {
            hy_note (task[i], HY_NOTE_KILL);
            hy_wait (task[i], 0);
        }
        fprintf (stderr, "philo: %u tasks free, not %u (philo %u)\n", k, n, k);
        hy_exits ("no task");
    }

    signal (SIGINT, interrupted);
    printf ("\x1b[H\x1b[2J");
    printf ("The dining philosophers: %u tasks at a table, a fork (a mutex) between each two.\n", n);
    printf (deadly ? "Each takes the fork on their left first (-d).  Ctrl-C ends it.\n"
                   : "Each takes the lower-numbered of their forks first.  Ctrl-C ends it.\n");
    printf ("\n  philosopher  task  doing     forks  meals\n");
    for (i = 0; i < n; ++i) {
        printf ("  %-11s  %4u\n", names[i], task[i]);
        doing[i] = hand[i] = 0xFF;
        meals[i] = 0xFFFF;
    }
    printf ("\n  fork    ");
    for (i = 0; i < n; ++i) {
        printf ("  %u", i + 1);
        holder[i] = 0xFF;
    }
    printf ("\n  held by \n");
    for (i = 0; i < n && !stop; ++i) {              /* All seated, */
        hy_sem_acquire (ready);
    }
    for (i = 0; i < n; ++i) {                       /*   and off together */
        hy_sem_release (go);
    }

    /* The table drawn as it changes, a tenth of a second at a time, till they've all eaten their meals, Ctrl-C,
    ** or a deadlock: every one hungry, a fork in hand, for two seconds */
    for (;;) {
        hy_sleep_ticks (HY_TICK_HZ / 10);
        k = 0;
        for (i = 0; i < n; ++i) {
            if (t->doing[i] != doing[i]) {
                doing[i] = t->doing[i];
                at (5 + i, 22);
                printf ("%-8s", doings[doing[i]]);
            }
            if (t->hand[i] != hand[i]) {
                hand[i] = t->hand[i];
                at (5 + i, 34);
                printf (hand[i] ? "%u" : " ", hand[i]);
            }
            if (t->meals[i] != meals[i]) {
                meals[i] = t->meals[i];
                at (5 + i, 39);
                printf ("%5u", meals[i]);
            }
            if (t->holder[i] != holder[i]) {
                holder[i] = t->holder[i];
                at (7 + n, 13 + 3 * i);
                printf (holder[i] ? "%u" : "-", holder[i]);
            }
            k += doing[i] == DONE;
        }
        fflush (stdout);
        if (k == n || stop) {
            break;
        }
        for (i = 0; i < n && t->doing[i] == HUNGRY && t->hand[i] == 1; ++i) {
        }
        stuck = i == n ? stuck + 1 : 0;
        if (stuck == 2 * 10) {
            deadlock = 1;
            break;
        }
    }

    for (i = 0, sum = 0, least = 0xFFFF, most = 0; i < n; ++i) {       /* (As they are now: before the kills) */
        sum += t->meals[i];
        least = t->meals[i] < least ? t->meals[i] : least;
        most = t->meals[i] > most ? t->meals[i] : most;
    }
    k = t->clashes;
    at (9 + n, 1);
    if (deadlock) {
        printf ("Deadlock: each holds their left fork and waits for their right one, which\n"
                "their neighbour holds.  Taking the lower-numbered fork first breaks the circle:\n"
                "the last philosopher reaches first for the fork the first one wants.\n");
        for (i = 0; i < n; ++i) {                   /* (Ctrl-C ends them itself) */
            hy_note (task[i], HY_NOTE_KILL);
        }
    }
    for (i = 0; i < n; ++i) {
        hy_wait (task[i], 0);
    }
    printf ("%u philosophers ate %u meals (%u to %u each); a fork was in two hands %u times.\n",
            n, sum, least, most, k);
    if (deadlock) {
        hy_exits ("deadlock");
    }
    if (stop) {
        hy_exits ("interrupted");
    }
    return 0;
}
