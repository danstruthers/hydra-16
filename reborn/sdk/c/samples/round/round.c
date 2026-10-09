/*
** round.c - "Row, Row, Row Your Boat" as a round: four tasks (this program again, as round -w ...) each sing a
** voice of it on a YM2151 channel of its own (snd.h), each two bars after the one before.  Nothing passes between
** them as they sing: each keeps time by the system's tick, sleeping till each of its notes is due (hy_sleep_until),
** from a start they share.  So the voices stay together however the scheduler shares the CPU among them (and this
** task, drawing the words as they're sung).  To start together they meet at a barrier first: each claims its
** channel and sets its instrument, then gives a "ready" semaphore one; this task takes one for each, writes the
** start (a moment ahead) in a shared segment, and gives a "go" semaphore one for each.  At the end each says how
** late its latest note was (in ticks: a 200th of a second each).
**   round [TIMES [TICKS]]   (each voice sings it TIMES times, 2 by default; an eighth note TICKS ticks, 36 by
**                            default)
**   % /sd/0/sample/c/round
** In the emulator, sim/run.js -i --sound plays the sound in a browser.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <hydra.h>
#include <snd.h>

#define VOICES      4
#define ENTRY       12              /* Eighths between the voices' entries: two bars of 6/8 */
#define FAILED      0xFE            /* (A voice's at: its channel is another program's) */
#define FINISHED    0xFF

/* The tune, in 6/8: each note's pitch (MIDI's: 60 is middle C), its length in eighths, and its syllable (its
** phrase, and where it is in it) */
struct note {
    unsigned char   pitch, len, phrase, col, width;
};

static const struct note tune[] = {
    { 60, 3, 0,  0, 4 }, { 60, 3, 0,  5, 4 },                       /* Row, row, */
    { 60, 2, 0, 10, 3 }, { 62, 1, 0, 14, 4 }, { 64, 3, 0, 19, 5 },  /* row your boat, */
    { 64, 2, 1,  0, 4 }, { 62, 1, 1,  4, 2 }, { 64, 2, 1,  7, 4 }, { 65, 1, 1, 12, 3 },   /* Gently down the */
    { 67, 6, 1, 16, 7 },                                            /* stream. */
    { 72, 1, 2,  0, 3 }, { 72, 1, 2,  3, 2 }, { 72, 1, 2,  5, 3 },  /* Merrily, */
    { 67, 1, 2,  9, 3 }, { 67, 1, 2, 12, 2 }, { 67, 1, 2, 14, 3 },  /* merrily, */
    { 64, 1, 2, 18, 3 }, { 64, 1, 2, 21, 2 }, { 64, 1, 2, 23, 3 },  /* merrily, */
    { 60, 1, 2, 27, 3 }, { 60, 1, 2, 30, 2 }, { 60, 1, 2, 32, 3 },  /* merrily, */
    { 67, 2, 3,  0, 4 }, { 65, 1, 3,  5, 2 }, { 64, 2, 3,  8, 3 }, { 62, 1, 3, 12, 1 },   /* Life is but a */
    { 60, 6, 3, 14, 6 }                                             /* dream. */
};
#define NOTES       (sizeof tune / sizeof tune[0])

static const char* const phrases[] = {
    "Row, row, row your boat,",
    "Gently down the stream.",
    "Merrily, merrily, merrily, merrily,",
    "Life is but a dream."
};

/* Each voice's instrument (General MIDI's), and how far it's moved (semitones) */
static const char* const names[VOICES] = { "flute", "piano", "cello", "bass" };
static const unsigned char patches[VOICES] = { 73, 0, 42, 32 };
static const signed char shifts[VOICES] = { 12, 0, -12, -24 };

struct shared {                     /* The shared segment's first bank, at $8000 in each task */
    unsigned        start;          /* The tick the first voice starts at */
    unsigned char   at[VOICES];     /* Each voice's note (1 on; 0 before its first; FINISHED, FAILED) */
    unsigned char   late[VOICES];   /* The latest of each voice's notes, in ticks */
};

static volatile struct shared* sh;

/* ---- A voice: round -w SEG READY GO VOICE TIMES TICKS */

static int voice (char* argv[])
{
    unsigned char seg = atoi (argv[2]), ready = atoi (argv[3]), go = atoi (argv[4]), v = atoi (argv[5]);
    unsigned char times = atoi (argv[6]), eighth = atoi (argv[7]), k, late;
    unsigned t;

    if (hy_seg_attach (seg) < 0 || (sh = (struct shared*) hy_seg_map (seg, 0)) == 0) {
        return 1;
    }
    if (snd_claim (1 << v) < 0) {                   /* Channel v, this voice's alone */
        sh->at[v] = FAILED;
        hy_sem_release (ready);
        return 1;
    }
    snd_patch (v, patches[v]);
    hy_sem_release (ready);                         /* Ready, */
    if (hy_sem_acquire (go) < 0) {                  /*   and off when they all are */
        return 1;
    }
    t = sh->start + v * ENTRY * eighth;
    while (times--) {
        for (k = 0; k < NOTES; ++k) {
            if (hy_sleep_until (t) < 0) {           /* (A note: Ctrl-C) */
                return 1;
            }
            late = hy_ticks () - t;
            snd_note (v, tune[k].pitch + shifts[v]);
            sh->at[v] = k + 1;
            if (late > sh->late[v]) {
                sh->late[v] = late;
            }
            t += tune[k].len * eighth;
            hy_sleep_until (t - 2);                 /* (A little gap before the next) */
            snd_off (v);
        }
    }
    sh->at[v] = FINISHED;
    return 0;
}

/* ---- The words, drawn as they're sung */

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

#define WORDS       28              /* The column the words start at */

/* Voice v's syllable, k (a note of the tune), shown plain or in reverse */
static void syllable (unsigned char v, unsigned char k, unsigned char lit)
{
    const struct note* n = &tune[k];

    at (4 + v, WORDS + n->col);
    printf (lit ? "\x1b[7m%.*s\x1b[0m" : "%.*s", n->width, phrases[n->phrase] + n->col);
}

int main (int argc, char* argv[])
{
    static char* args[9] = { "round", "-w" };
    static char a[6][6];
    static unsigned char task[VOICES], shown[VOICES];
    const char* prog;
    unsigned char seg, ready, go, i, k, done, times = 2, eighth = 36, last;

    if (argc > 1 && strcmp (argv[1], "-w") == 0) {
        return voice (argv);
    }
    if (argc > 1) {
        times = atoi (argv[1]);
    }
    if (argc > 2) {
        eighth = atoi (argv[2]);
    }
    if (argc > 3 || times < 1 || times > 9 || eighth < 6 || eighth > 100) {
        fputs ("usage: round [TIMES [TICKS]]   (TIMES 1-9, TICKS 6-100)\n", stderr);
        hy_exits ("usage");
    }
    if ((prog = self (argv[0])) == 0) {
        fprintf (stderr, "round: can't find myself (/bin/%s, ./%s, /sd/0/sample/c/%s)\n", argv[0], argv[0], argv[0]);
        hy_exits ("no workers");
    }
    seg = hy_seg_create (1);
    ready = hy_sem_new (0, 0);
    go = hy_sem_new (0, 0);
    if (seg == 0xFF || ready == 0xFF || go == 0xFF) {
        perror ("round");
        hy_exits ("no segment or semaphore");
    }
    sh = (struct shared*) hy_seg_map (seg, 0);
    memset ((void*) sh, 0, sizeof *sh);

    utoa (seg, a[0], 10);
    utoa (ready, a[1], 10);
    utoa (go, a[2], 10);
    utoa (times, a[4], 10);
    utoa (eighth, a[5], 10);
    for (i = 0; i < 6; ++i) {
        args[2 + i] = a[i];
    }
    for (k = 0; k < VOICES; ++k) {
        utoa (k, a[3], 10);
        i = hy_spawn (prog, args, 0);
        if (i == 0xFF) {
            break;
        }
        task[k] = i;
    }
    if (k < VOICES) {                               /* (No task free for them all) */
        for (i = 0; i < k; ++i) {
            hy_note (task[i], HY_NOTE_KILL);
            hy_wait (task[i], 0);
        }
        fprintf (stderr, "round: %u tasks free, not %u\n", k, VOICES);
        hy_exits ("no task");
    }

    signal (SIGINT, interrupted);
    printf ("\x1b[H\x1b[2J");
    printf ("A round: four tasks a voice each, on a YM2151 channel each, two bars apart,\n");
    printf ("each keeping time by the tick.  Ctrl-C ends it.\n\n");
    for (i = 0; i < VOICES; ++i) {
        printf ("  voice %u  task %-2u  %-5s\n", i + 1, task[i], names[i]);
    }
    for (i = 0; i < VOICES && !stop; ++i) {         /* All ready, */
        hy_sem_acquire (ready);
    }
    for (i = 0; i < VOICES; ++i) {
        if (sh->at[i] == FAILED) {                  /* (A channel another program holds: they all end) */
            for (k = 0; k < VOICES; ++k) {
                hy_note (task[k], HY_NOTE_KILL);
                hy_wait (task[k], 0);
            }
            at (9, 1);
            printf ("Channel %u is another program's.\n", i);
            hy_exits ("busy");
        }
    }
    sh->start = hy_ticks () + HY_TICK_HZ / 2;      /*   a moment from now, */
    for (i = 0; i < VOICES; ++i) {                  /*   and off together */
        hy_sem_release (go);
    }

    /* Each voice's phrase, its syllable lit, as they go (a twentieth of a second at a time) */
    do {
        hy_sleep_ticks (HY_TICK_HZ / 20);
        done = 0;
        for (i = 0; i < VOICES; ++i) {
            k = sh->at[i];
            if (k >= FAILED) {
                ++done;
            }
            if (k == shown[i]) {
                continue;
            }
            last = shown[i];
            shown[i] = k;
            if (k >= FAILED) {                      /* Done: its phrase gone */
                at (4 + i, WORDS);
                printf ("\x1b[K");
                continue;
            }
            --k;
            if (last == 0 || tune[last - 1].phrase != tune[k].phrase) {
                at (4 + i, WORDS);                  /* A new phrase */
                printf ("%s\x1b[K", phrases[tune[k].phrase]);
            } else {
                syllable (i, last - 1, 0);          /* The last syllable plain again */
            }
            syllable (i, k, 1);
        }
        fflush (stdout);
    } while (done < VOICES && !stop);

    for (i = 0; i < VOICES; ++i) {
        hy_wait (task[i], 0);
    }
    at (9, 1);
    if (stop) {
        snd_claim (0x0F);                           /* (Any notes left sounding: off) */
        for (i = 0; i < VOICES; ++i) {
            snd_off (i);
        }
        hy_exits ("interrupted");
    }
    printf ("Each voice sang it %u time%s, its latest note late by", times, times == 1 ? "" : "s");
    for (i = 0; i < VOICES; ++i) {
        printf (" %u", sh->late[i]);
    }
    printf (" ticks.\n");
    return 0;
}
