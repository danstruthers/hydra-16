/*
** prodcons.c - producers and consumers: two tasks make items and two use them (this program again, as prodcons
** -w ...), passing them through a ring of 8 slots in a shared segment.  Two counting semaphores keep them in step:
** "empty" counts the free slots (8 to start with) and "full" the filled ones (0): a producer takes one of empty
** before it puts an item in, and gives full one after; a consumer takes one of full, then gives empty one.  So a
** producer waits (using no CPU) while the ring is full, and a consumer while it's empty.  A mutex keeps the ring's
** indexes, as two producers (or consumers) may reach them at once.  For the first half of the items the producers
** are the quicker, and the ring fills; for the second the consumers are, and it empties.  At the end this task puts
** in a stop item for each consumer, and checks that each item was used once (the sums of what was made and what
** was used).
**   prodcons [-n ITEMS]   (each producer's, 1-1000; 40 by default)
**   % /sd/0/sample/c/prodcons
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <hydra.h>

#define SLOTS       8
#define WORKERS     4               /* Producers 0 and 1, consumers 2 and 3 */
#define STOP        0               /* The item that ends a consumer (a producer's are 1001 on) */

enum { MAKING, PUTTING, WAITING_ROOM, TAKING, WAITING_ITEM, USING, DONE };

struct ring {                       /* The shared segment's first bank, at $8000 in each task */
    unsigned        slot[SLOTS];
    unsigned char   head, tail;     /* Where the next item is taken, and put (the mutex's) */
    unsigned char   count;          /* Items in it (the mutex's: for the drawing) */
    unsigned char   consumers_quicker;
    unsigned char   doing[WORKERS];
    unsigned        did[WORKERS];   /* Items made, or used */
    unsigned        waits[WORKERS]; /* Times it waited: for room, or for an item */
    unsigned long   sum[WORKERS];   /* The items made, or used, added up */
};

static volatile struct ring* r;
static unsigned char empty, full, lock, me;

/* What a worker is doing, for the drawing (this program's first task, putting in stop items, isn't one) */
static void set_doing (unsigned char what)
{
    if (me < WORKERS) {
        r->doing[me] = what;
        if (what == WAITING_ROOM || what == WAITING_ITEM) {
            ++r->waits[me];
        }
    }
}

/* An item into the ring (a producer's, or this program's stop items) */
static void put (unsigned v)
{
    if (hy_sem_try (empty) < 0) {                   /* No room: wait for some */
        set_doing (WAITING_ROOM);
        if (hy_sem_acquire (empty) < 0) {
            exit (1);
        }
    }
    set_doing (PUTTING);
    hy_sem_acquire (lock);
    r->slot[r->tail] = v;
    r->tail = (r->tail + 1) % SLOTS;
    ++r->count;
    hy_sem_release (lock);
    hy_sem_release (full);                          /* One more to take */
}

/* An item out of the ring (a consumer's) */
static unsigned get (void)
{
    unsigned v;

    if (hy_sem_try (full) < 0) {                    /* None: wait for one */
        set_doing (WAITING_ITEM);
        if (hy_sem_acquire (full) < 0) {
            exit (1);
        }
    }
    set_doing (TAKING);
    hy_sem_acquire (lock);
    v = r->slot[r->head];
    r->head = (r->head + 1) % SLOTS;
    --r->count;
    hy_sem_release (lock);
    hy_sem_release (empty);                         /* One more free */
    return v;
}

/* ---- A worker: prodcons -w SEG READY GO EMPTY FULL LOCK ME ITEMS */

static int worker (char* argv[])
{
    unsigned char seg = atoi (argv[2]), ready = atoi (argv[3]), go = atoi (argv[4]);
    unsigned items = atoi (argv[9]), s, v;

    empty = atoi (argv[5]);
    full = atoi (argv[6]);
    lock = atoi (argv[7]);
    me = atoi (argv[8]);
    if (hy_seg_attach (seg) < 0 || (r = (struct ring*) hy_seg_map (seg, 0)) == 0) {
        return 1;
    }
    srand (hy_ticks () + me * 59);
    hy_sem_release (ready);
    if (hy_sem_acquire (go) < 0) {
        return 1;
    }
    if (me < 2) {                                   /* A producer: its items, 1001 on (or 2001 on) */
        for (s = 1; s <= items; ++s) {
            set_doing (MAKING);
            hy_sleep_ticks (r->consumers_quicker ? 30 + rand () % 30 : 6 + rand () % 10);
            v = (me + 1) * 1000 + s;
            put (v);
            ++r->did[me];
            r->sum[me] += v;
        }
    } else {                                        /* A consumer: items till a stop */
        while ((v = get ()) != STOP) {
            ++r->did[me];
            r->sum[me] += v;
            set_doing (USING);
            hy_sleep_ticks (r->consumers_quicker ? 6 + rand () % 10 : 30 + rand () % 30);
        }
    }
    set_doing (DONE);
    return 0;
}

/* ---- The ring, drawn */

static const char* const names[WORKERS] = { "producer 1", "producer 2", "consumer 1", "consumer 2" };
static const char* const doings[] = { "making", "putting", "waiting: full", "taking", "waiting: empty", "using",
                                      "done" };
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

static void at (unsigned char row, unsigned char col)
{
    printf ("\x1b[%u;%uH", row, col);
}

static void interrupted (int sig)
{
    (void) sig;
    stop = 1;
}

int main (int argc, char* argv[])
{
    static char* args[11] = { "prodcons", "-w" };
    static char a[8][6];
    static unsigned char task[WORKERS], doing[WORKERS], count = 0xFF, quicker = 0xFF;
    static unsigned did[WORKERS], waits[WORKERS];
    const char* prog;
    unsigned char seg, ready, go, i, k, stops = 0;
    unsigned items = 40;
    unsigned long made, used;

    if (argc > 1 && strcmp (argv[1], "-w") == 0) {
        return worker (argv);
    }
    if (argc == 3 && strcmp (argv[1], "-n") == 0) {
        items = atoi (argv[2]);
    }
    if ((argc != 1 && argc != 3) || items < 1 || items > 1000) {
        fputs ("usage: prodcons [-n ITEMS]   (1-1000)\n", stderr);
        hy_exits ("usage");
    }
    if ((prog = self (argv[0])) == 0) {
        fprintf (stderr, "prodcons: can't find myself (/bin/%s, ./%s, /sd/0/sample/c/%s)\n", argv[0], argv[0], argv[0]);
        hy_exits ("no workers");
    }
    seg = hy_seg_create (1);
    ready = hy_sem_new (0, 0);
    go = hy_sem_new (0, 0);
    empty = hy_sem_new (SLOTS, 0);                  /* The free slots: all of them */
    full = hy_sem_new (0, 0);                       /* The filled ones: none */
    lock = hy_sem_new (0, HY_SEM_MUTEX);
    if (seg == 0xFF || ready == 0xFF || go == 0xFF || empty == 0xFF || full == 0xFF || lock == 0xFF) {
        perror ("prodcons");
        hy_exits ("no segment or semaphore");
    }
    r = (struct ring*) hy_seg_map (seg, 0);
    memset ((void*) r, 0, sizeof *r);

    utoa (seg, a[0], 10);
    utoa (ready, a[1], 10);
    utoa (go, a[2], 10);
    utoa (empty, a[3], 10);
    utoa (full, a[4], 10);
    utoa (lock, a[5], 10);
    utoa (items, a[7], 10);
    for (i = 0; i < 8; ++i) {
        args[2 + i] = a[i];
    }
    for (k = 0; k < WORKERS; ++k) {
        utoa (k, a[6], 10);
        i = hy_spawn (prog, args, 0);
        if (i == 0xFF) {
            break;
        }
        task[k] = i;
    }
    if (k < WORKERS) {                              /* (No task free for them all) */
        for (i = 0; i < k; ++i) {
            hy_note (task[i], HY_NOTE_KILL);
            hy_wait (task[i], 0);
        }
        fprintf (stderr, "prodcons: %u tasks free, not %u\n", k, WORKERS);
        hy_exits ("no task");
    }

    signal (SIGINT, interrupted);
    printf ("\x1b[H\x1b[2J");
    printf ("Producers and consumers: two tasks make %u items each and two use them, through\n", items);
    printf ("a ring of %u slots, kept by two counting semaphores (free, filled) and a mutex.\n\n", SLOTS);
    printf ("  the ring    [%*s]\n", SLOTS, "");
    for (i = 0; i < WORKERS; ++i) {
        printf ("  %-10s  task %-2u\n", names[i], task[i]);
        doing[i] = 0xFF;
        did[i] = waits[i] = 0xFFFF;
    }
    for (i = 0; i < WORKERS && !stop; ++i) {        /* All here, */
        hy_sem_acquire (ready);
    }
    for (i = 0; i < WORKERS; ++i) {                 /*   and off together */
        hy_sem_release (go);
    }

    /* Drawn as it changes, a tenth of a second at a time, till the consumers have stopped (or Ctrl-C) */
    me = 0xFF;                                      /* (This task's waits aren't counted) */
    for (;;) {
        hy_sleep_ticks (HY_TICK_HZ / 10);
        if (r->count != count) {
            count = r->count;
            at (4, 16);
            for (i = 0; i < SLOTS; ++i) {
                putchar (i < count ? '#' : '.');
            }
            printf ("]  %u of %u ", count, SLOTS);
        }
        if (r->consumers_quicker != quicker) {
            quicker = r->consumers_quicker;
            at (4, 42);
            printf (quicker ? "the consumers are quicker\x1b[K" : "the producers are quicker\x1b[K");
        }
        k = 0;
        for (i = 0; i < WORKERS; ++i) {
            if (r->doing[i] != doing[i]) {
                doing[i] = r->doing[i];
                at (5 + i, 23);
                printf ("%-14s", doings[doing[i]]);
            }
            if (r->did[i] != did[i] || r->waits[i] != waits[i]) {
                did[i] = r->did[i];
                waits[i] = r->waits[i];
                at (5 + i, 38);
                printf ("%4u %s  %3u wait%s", did[i], i < 2 ? "made" : "used", waits[i], waits[i] == 1 ? " " : "s");
            }
            k += doing[i] == DONE;
        }
        fflush (stdout);
        if (stop || k == WORKERS) {
            break;
        }
        if (r->did[0] + r->did[1] >= items) {       /* Half made: the consumers' turn to be quick */
            r->consumers_quicker = 1;
        }
        if (stops == 0 && doing[0] == DONE && doing[1] == DONE) {
            put (STOP);                             /* A stop for each consumer, after the last item */
            put (STOP);
            stops = 2;
        }
    }

    for (i = 0; i < WORKERS; ++i) {
        hy_wait (task[i], 0);
    }
    at (10, 1);
    made = r->sum[0] + r->sum[1];
    used = r->sum[2] + r->sum[3];
    printf ("Made %u items (their sum %lu), used %u (their sum %lu): %s.\n", r->did[0] + r->did[1], made,
            r->did[2] + r->did[3], used, stop ? "stopped" : made == used ? "each once" : "not the same!");
    printf ("The producers waited for room %u times; the consumers for an item %u times.\n",
            r->waits[0] + r->waits[1], r->waits[2] + r->waits[3]);
    if (stop) {
        hy_exits ("interrupted");
    }
    return made != used;
}
