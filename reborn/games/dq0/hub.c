/*
** hub.c - hub.h's shared segment: made (the UI) or attached (the others), put at $8000, its queues, its doorbells and
** its requests (hub.h's protocol: hub_post, hub_wait, hub_reply).
** Each program of the game's compiles this file with its own (games/dq0w ... include it, HUB_ATTACHED defined: they
** attach to the hub, so they take neither its making nor the spawning of tasks, which takes in stdio).  The UI's
** making and spawning are its swapped part 1's (the start's and the end's: swap.inc), out of its main RAM.
*/

#include <stdlib.h>
#include <string.h>
#include <hydra.h>
#include "hub.h"

static unsigned char seg;
static unsigned char u[HUB_BANKS], x[HUB_BANKS];   /* Each bank's U and $00 (SEG_MAP's), found once */
static unsigned char cur;                       /* The bank hub_map put in the window last */
static unsigned char self;                      /* This task's role (T_*: its doorbell) */
static unsigned char seq;                       /* Its requests, counted (each one's seq) */
static unsigned char pend;                      /* A ring taken while it waited for a reply, not the reply's */

/* Each bank's U and $00, kept, and bank 0 put at $8000 */
static int find_banks (void)
{
    static struct hy_regs r;
    unsigned char b;

    for (b = HUB_BANKS; b--; ) {
        r.a = seg;
        r.x = b;
        if (hy_call (HY_SEG_MAP, &r)) {
            return -1;
        }
        u[b] = r.a;
        x[b] = r.x;
    }
    hub_map (0);
    return 0;
}

void __fastcall__ hub_map (unsigned char bank)
{
    cur = bank;
    *(volatile unsigned char*) 0xFFF1 = u[bank];    /* U, then the bank (hydra.h's hy_seg_map, without the call) */
    *(volatile unsigned char*) 0 = x[bank];
}

#if !defined (HUB_ATTACHED) || defined (HUB_BACK)  /* (The UI's and the maker's: the others' room) */
/* Where the window is (the hub's bank hub_map put there last, and the bank register), and the window put back there:
** U as that bank of the hub's had it, then the bank register (the hub's banks may be in more than one of the shared
** RAM's macro-pages: the bank register alone, under another's U, would give another bank; an overlay's bank, the
** task's own, isn't U's, but the stub that put it in puts the hub's bank back by the bank register alone) */
unsigned hub_here (void)
{
    return (unsigned) cur << 8 | *(volatile unsigned char*) 0;
}

void __fastcall__ hub_back (unsigned w)
{
    cur = w >> 8;
    *(volatile unsigned char*) 0xFFF1 = u[cur];
    *(volatile unsigned char*) 0 = w;
}
#endif

#ifndef HUB_ATTACHED
#pragma code-name (push, "SW1CODE")             /* (The UI's part 1, the start's and the end's: dq0.cfg) */
#pragma rodata-name (push, "SW1RODATA")
int __fastcall__ hub_create (unsigned char banks)
{
    int s = hy_seg_create (banks);
    unsigned char t;

    if (s < 0) {
        return -1;
    }
    seg = s;
    if (find_banks ()) {
        return -1;
    }
    for (t = SIGHT_BANK; t <= WORLD_BANK; ++t) {   /* (The shared records' banks, cleared) */
        hub_map (t);
        memset (HUB_WINDOW, 0, 0x2000);
    }
    hub_map (0);
    memset (HUB, 0, sizeof (struct hub));
    HUB->magic = HUB_MAGIC;
    HUB->version = HUB_VERSION;
    HUB->seg = seg;
    for (t = 0; t < NTASKS; ++t) {
        if ((s = hy_sem_new (0, 0)) < 0) {
            return -1;
        }
        HUB->sem[t] = s;
    }
    if ((s = hy_sem_new (1, 0)) < 0) {          /* (The change records' lock) */
        return -1;
    }
    HUB->change_lock = s;
    HUB->task[T_UI] = hy_task ();
    self = T_UI;
    return 0;
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#endif

#ifdef HUB_ATTACHED                             /* (The UI's room: what it never does left out, here and below) */
int __fastcall__ hub_attach (unsigned char s, unsigned char me)
{
    seg = s;
    if (hy_seg_attach (s) < 0 || find_banks () || HUB->magic != HUB_MAGIC || HUB->version != HUB_VERSION) {
        return -1;
    }
    HUB->task[me] = hy_task ();
    self = me;
    return 0;
}
#endif

/* The clock, read whole: tick is bumped before now is written and after (the world's), so a read between two
** equal even ticks saw it whole */
unsigned long hub_now (void)
{
    unsigned char t;
    unsigned long n;

    do {
        t = HUB->tick;
        n = HUB->now;
    } while ((t & 1) || t != HUB->tick);
    return n;
}

void __fastcall__ hub_ring (unsigned char t)
{
    hy_sem_release (HUB->sem[t]);
}

#ifndef HUB_NO_QUEUES                           /* (A task that neither sends nor takes intents and events: its room) */
/* ---- The UI's intents, to the world */

unsigned char __fastcall__ hub_send (const struct msg* m)
{
    unsigned char h = HUB->iq_head, n = (h + 1) % IQ_SIZE;

    if (n == HUB->iq_tail) {
        return 0;
    }
    memcpy (&HUB->iq[h], m, sizeof *m);
    HUB->iq_head = n;                           /* (Written whole, then shown) */
    hub_ring (T_WORLD);
    return 1;
}

#ifdef HUB_ATTACHED                             /* (The world's) */
unsigned char __fastcall__ hub_take (struct msg* m)
{
    unsigned char t = HUB->iq_tail;

    if (t == HUB->iq_head) {
        return 0;
    }
    memcpy (m, &HUB->iq[t], sizeof *m);
    HUB->iq_tail = (t + 1) % IQ_SIZE;
    return 1;
}
#endif

/* ---- The world's events, to everyone (a reader that falls EQ_SIZE behind loses the oldest) */

#ifdef HUB_ATTACHED                             /* (The world's, the maker's) */
void __fastcall__ hub_event (const struct msg* m)
{
    unsigned h = HUB->eq_head;
    unsigned char t;

    memcpy (&HUB->eq[h % EQ_SIZE], m, sizeof *m);
    HUB->eq_head = h + 1;
    hub_ring (T_UI);                            /* (The UI woken to tell it: its loop's nap, hub_nap) */
    for (t = T_DENIZENS; t < NTASKS; ++t) {     /* (Those that wait for them) */
        if (t != T_REFEREE && HUB->task[t]) {
            hub_ring (t);
        }
    }
}
#endif

unsigned char __fastcall__ hub_next (unsigned* place, struct msg* m)
{
    unsigned h = HUB->eq_head;

    if (*place == h) {
        return 0;
    }
    if (h - *place > EQ_SIZE) {
        *place = h - EQ_SIZE;                   /* (Too far behind: the oldest are gone) */
    }
    memcpy (m, &HUB->eq[*place % EQ_SIZE], sizeof *m);
    ++*place;
    return 1;
}

/* ---- The denizens' events (DENIZENS.md: events), in their own bank: an entry copied in or out whole, between the
** main RAM and the bank (m is the main RAM's), the header in the window after (a reader that falls DQ_SIZE behind
** loses the oldest) */

#ifdef HUB_DWRITE                               /* (Their writer's alone) */
void __fastcall__ hub_devent (const struct msg* m)
{
    unsigned h = HUB->dq_head;

    hub_map (DEN_BANK);
    memcpy (DQ_AT + h % DQ_SIZE, m, sizeof *m);
    hub_map (0);
    HUB->dq_head = h + 1;                       /* (Written whole, then shown: nobody's rung, nobody waits) */
}
#endif

#if !defined (HUB_ATTACHED) || defined (HUB_DENQ)  /* (Their readers': the UI's, the denizens', the sound task's) */
unsigned char __fastcall__ hub_dnext (unsigned* place, struct msg* m)
{
    unsigned h = HUB->dq_head;

    if (*place == h) {
        return 0;
    }
    if (h - *place > DQ_SIZE) {
        *place = h - DQ_SIZE;
    }
    hub_map (DEN_BANK);
    memcpy (m, DQ_AT + *place % DQ_SIZE, sizeof *m);
    hub_map (0);
    ++*place;
    return 1;
}
#endif
#endif

#ifndef HUB_ATTACHED
#pragma code-name (push, "SW1CODE")             /* (The UI's part 1, the start's and the end's: dq0.cfg) */
#pragma rodata-name (push, "SW1RODATA")
/* ---- A task of the game's: path, given the segment and its role (and more, a word or none) */

static struct hy_regs r;

/* A program started: path, its arguments (each ended by a 0, and an empty one after them): its task, or -1.  (hydra.h's
** hy_spawn, without the stdio it takes in to write its buffers out first: the UI has none) */
int __fastcall__ hub_run (const char* path, const char* args)
{
    r.a = 0;                                    /* (Its fds: this one's 0, 1 and 2) */
    r.r[0] = (unsigned) path;
    r.r[1] = (unsigned) args;
    return hy_call (HY_SPAWN, &r) ? -1 : r.a;
}

/* A task waited for (hy_wait's, no message): its exit status, or -1 */
int __fastcall__ hub_join (unsigned char task)
{
    r.a = task;
    r.r[0] = 0;
    return hy_call (HY_WAIT, &r) ? -1 : r.x;
}

int __fastcall__ hub_spawn (const char* path, unsigned char t, const char* more)
{
    static char args[64];
    unsigned char n;
    int s;

    utoa (seg, args, 10);
    n = strlen (args) + 1;
    utoa (t, args + n, 10);
    n += strlen (args + n) + 1;
    if (more) {
        strcpy (args + n, more);
        n += strlen (more) + 1;
    }
    args[n] = 0;
    s = hub_run (path, args);
    hub_map (0);                                /* (A spawn leaves the window on another bank) */
    return s;
}
#pragma code-name (pop)
#pragma rodata-name (pop)
#endif

/* ---- The requests (hub.h's protocol): each in bank 0, its header first */

static struct rqh* rq;                          /* (The request at hand: static, cc65's code the less) */

/* A request (written already) posted to task t: who asks, its number, then the state that shows it; t rung */
void __fastcall__ hub_post (void* q, unsigned char t)
{
    static unsigned char to;

    to = t;
    hub_map (0);
    rq = q;
    rq->from = self;
    rq->seq = ++seq;
    rq->state = REQ_POSTED;
    hub_ring (to);
}

/* Its reply waited for: this task asleep on its own doorbell, ticks at most (0: till the reply's there, or the game's
** ending: 255 ticks at a time, the end looked for between), the header in the window after: 1 if the reply's there,
** else 0.  A ring that wasn't the reply's (an intent, an event, another's request) is this task's to see to: it's
** rung again once the reply's taken */
unsigned char __fastcall__ hub_wait_t (void* q, unsigned char ticks)
{
    static unsigned char t, rung;

    t = ticks;
    rq = q;
    while (hub_map (0), rq->state != REQ_DONE) {
        if (HUB->quit) {
            return 0;                           /* (Its owner may be gone) */
        }
        rung = !hy_sem_wait (HUB->sem[self], t ? t : 255);
        hub_map (0);
        if (rq->state != REQ_DONE) {
            pend |= rung;
            if (t) {
                return 0;
            }
        }
    }
    if (pend) {
        pend = 0;
        hub_ring (self);
    }
    return 1;
}

#ifndef HUB_ATTACHED
/* The UI asleep on its doorbell, ticks at most: woken at once by the world's events (hub_event rings it), a reply's
** ring, or one put back (hub_wait_t's); those that came with it taken, its loop seeing to them all at once */
void __fastcall__ hub_nap (unsigned char ticks)
{
    hub_map (0);
    if (!hy_sem_wait (HUB->sem[T_UI], ticks)) {
        while (!hy_sem_try (HUB->sem[T_UI])) {
        }
    }
}
#endif

#ifdef HUB_ATTACHED                             /* (Not the UI's: it answers no requests; its own loop's naps) */
/* A request answered (its owner's): the state that shows the reply, then the asker rung */
void __fastcall__ hub_reply (void* q)
{
    rq = q;
    hub_map (0);
    rq->state = REQ_DONE;
    hub_ring (rq->from);
}

/* This task asleep on its doorbell, ticks at most: woken at once by a ring (a ring that came meanwhile: at once) */
void __fastcall__ hub_sleep (unsigned char ticks)
{
    static unsigned char t;

    t = ticks;
    hub_map (0);
    hy_sem_wait (HUB->sem[self], t);
}
#endif

/* A request (written already) posted to task t, its reply waited for (hub_post, hub_wait): the request */
void* __fastcall__ hub_call (void* q, unsigned char t)
{
    hub_post (q, t);
    hub_wait_t (rq, 0);
    return rq;
}
