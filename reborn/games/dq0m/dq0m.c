/*
** dq0m.c - Dingus Quest Zero's maker: a task of its own, started by the UI (dq0m SEG ROLE: the hub's segment and
** its role there, hub.h), that grows the dungeon (DUNGEON.md) for whoever asks, the world or the denizens (each its
** request in the hub: world.h's struct mreq): a stratum's plan and its links down (plan.c: its record set by the
** asker), a chunk (make.c, built.c), into the hub's shared banks, where they read it.  Generation only: the same
** seed always the same chunk, the change records (the world's: dq0w/store.c) laid over it as it's made again
** (changes.c), and nothing kept of its own but the squares seen as the cache makes way for a chunk (and the map,
** drawn for the UI from the cache).  It waits on its doorbell (costing nothing) between requests.  Its growers are
** overlays (built.c, pass.c, caves.c, plan.c ...), read as it starts from the game's lib folder (its third argument,
** the UI's) into banks of its own.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <bankprog.h>
#include "../dq0/hub.h"
#include "../dq0w/world.h"
#include "grow.h"

struct stratum strata[MAXSTRATA];               /* (This task's copy of the hub's) */
const char* libpath;                            /* (The game's lib folder: its bank programs', stratagen's) */
#define OVLS    10                              /* Its overlays: built.c, pass.c, caves.c, plan.c, fixtures.c; the
                                                **   swapped parts' images (../dq0/swap.inc: map.c's, changes.c's);
                                                **   uses.c; the great caverns (caves.c's cavern); what shows the sky
                                                **   (sky.c); the chunks made ahead (ahead.c: bank.s's OVMAX,
                                                **   dq0lib.inc's OVL_BANKS) */

/* The overlays: dir/dq0m.ovl, an 8K bank each (dq0m.cfg's OV1 ...; the file cut after the last one used, each bank's
** bank packed: make.js, ../dq0/ovlpack.js), read and unpacked in place into banks of its own by the unpack library,
** the linker's fill ($FF) put back after its bytes, all of it the game's library's (modules/dq0lib, ../dq0/dq0lib.inc:
** 2026-11-09): 0, or not 0; and a number, an argument's, its NUM (atoi's, without the C library's ctype) */
unsigned char __fastcall__ ovl_load (const char* dir);
unsigned char __fastcall__ num (const char* s);

/* Request i answered: the strata's records copied first (the world bank's are the truth), and back if they changed */
/* The chunks in the cache of strata no longer in the hub's records (their slots made way for another's: M_PLAN's,
** M_SLOT's) theirs no more, what's been seen of them kept (but those a task's reading, or stocked, not yet marked;
** part 3's since 2026-11-03: the main RAM's room, the sky's stub) */
#pragma code-name (push, "SW3CODE")
#pragma rodata-name (push, "SW3RODATA")
void s_stale (void)
{
    struct cslot* c;
    unsigned char k, j, busy;

    for (k = 0; k < NCSLOTS; ++k) {
        hub_map (WORLD_BANK);
        c = &WREC->cslot[k];
        for (j = 0, busy = c->used == CS_FRESH; j < NASK; ++j) {
            busy |= WREC->inuse[j] == k;
        }
        if (c->used && !busy && (STRATUM (c->sn).n != c->sn || !STRATUM (c->sn).made)) {
            seen_keep (k);
            hub_set (WORLD_BANK, &WREC->cslot[k].used, 0, 1);
        }
    }
    hub_map (0);
}
#pragma code-name (pop)
#pragma rodata-name (pop)
void stale (void);                              /* (Its stub: bank.s's) */

static void __fastcall__ serve (unsigned char i)
{
    static struct mreq q;

    memcpy (&q, &HUB->mreq[i], sizeof q);
    hub_get (WORLD_BANK, WREC->strata, strata, sizeof strata);
    if (q.kind == M_SLOT || q.kind == M_PLAN || q.kind == M_RESTORE) {
        ahead_none ();                          /* (A plan grown, or read back: the chunks made ahead let go) */
    }
    switch (q.kind) {
    case M_SLOT:                                /* (Its record read back into its slot: its plan grown again) */
        stale ();
        plan_stratum (q.sn);
        place_pieces (q.sn);
        pl_flush ();
        break;
    case M_PLAN:
        stale ();
        plan_stratum (q.sn);
        links_down (q.sn);
        sky (0, q.sn);                          /* (Its shafts open to the sky: sky.c's) */
        place_pieces (q.sn);                    /* (The theme's pieces in it: theme.h) */
        pl_flush ();                            /* (A big plan's pages in its file: the denizens read it) */
        hub_put (WORLD_BANK, &WREC->grown, &STRATUM (q.sn), sizeof STRATUM (0));    /* (Its record grown: the world's
                                                **   to write into the strata's) */
        break;
    case M_CHUNK:
        make_chunk (q.sn, q.cx, q.cy);
        break;
    case M_SAVE:
        changes_save ();
        break;
    case M_RESTORE:                             /* (The seen maps back, each stratum made grown again: the same
                                                **   plan) */
        changes_restore ();
        for (q.cx = 0; q.cx < MAXSTRATA; ++q.cx) {     /* (The slots: each its stratum's, by its number) */
            if (strata[q.cx].made) {
                plan_stratum (strata[q.cx].n);
                place_pieces (strata[q.cx].n);
            }
        }
        pl_flush ();
        break;
    }
    hub_map (0);
    if (HUB->small) {                           /* (dq0 -C, the tests' small caches: its bank programs given up after
                                                **   each request, loaded again as they're next called) */
        bank_flush ();
    }
    hub_reply (&HUB->mreq[i]);
}

/* ---- The chunks made ahead (the user's, 2026-11-17: the maker builds the map's pieces ahead of the character's eyes,
** in the background, so a look or a walk finds them made; ahead.c's which and where, make.c's the making): in the
** background (the kernel's: the maker in its foreground only while one there waits on its request, fore), as nothing's
** asked of it, AH_SETTLE ticks after the last; giving way to a command in hand or a line being typed as the senses' ask
** does (dq0p.c's sense_held: at the real paces GIVE_WAY ticks at most from a line's Enter, KEY_HOLD while its keys go
** on; at the turns pace, the game waiting on the player between, while it's in hand), held where it is meanwhile (the
** growers' checkpoints: ahead_wanted); given up as a request comes for anything but the chunk it's making (one for it:
** made on, in the foreground if its asker is); at each checkpoint the other tasks' turn first (hy_yield: the
** background's least).  Asleep but as it's rung, or AH_NAP ticks gone: each wake the maker's few thousand cycles, taken
** from what the game's doing then (2026-11-17: four ticks' naps as it gave way, a fortieth of a command's answer) */
#define AH_NAP          40                      /* (Ticks asleep at most, nothing wanted: the actors looked at again) */
#define AH_SETTLE       60                      /* (Ticks of no request, no command and no keys before one's begun: a
                                                **   reader's requests come in bursts, a new stratum's chunks one
                                                **   after another, none begun between them) */
#define AH_HOLD         25                      /* (Ticks asleep at most as it gives way) */
#define GIVE_WAY        100                     /* (dq0p.c's: ticks held from a line's Enter at most, */
#define KEY_HOLD        600                     /*   and while its keys go on) */
#define ASK_FORE        0x1D                    /* (The requests whose askers are the kernel's foreground's: the world's,
                                                **   the senses', the UI's, the map's; not the denizens') */
static unsigned char fg;                        /* (In the kernel's foreground now: fore ()'s) */

/* The requests posted: bit i mreq[i]'s (ASK_*), bit 4 the map's (the hub's header in the window) */
static unsigned char asked (void)
{
    unsigned char i, w = 0;

    for (i = 0; i < NASK; ++i) {
        if (hub_posted (&HUB->mreq[i])) {
            w |= 1 << i;
        }
    }
    return hub_posted (&HUB->mapq) ? w | 0x10 : w;
}

/* In the kernel's foreground or not (the hub's header in the window): on, the requests posted (asked's), the maker's
** turn there as it was the UI's to give (main.c's fore_sync, FS_MAKE, before 2026-11-17): at the real paces, for the
** map, and for the rest while no command's in hand (one in hand, the world's, the command parser's and the referee's
** time first: its answer's first letter); and for a request from one in the foreground (the world, the senses, the
** UI's), not the denizens' minds' alone; never as it makes a chunk ahead for no one */
static void __fastcall__ fore (unsigned char on)
{
    on = HUB->tpr && ((on & 0x10) || ((on & ASK_FORE) && !HUB->ui_cmd));
    if (on != fg) {
        hy_taskfore (hy_task (), fg = on);
    }
}

/* A command in hand, or a line being typed: the background gives way, as the senses' ask does; and while the game's
** starting or the character's long task is under way (a walk, a wait, a rest, a dig ...: the world's own time, a
** command's still), or it's dead (the game's end); at the turns pace too while the world's turn goes on (the
** character not yet ready again, its intents not all taken: the denizens catching up, dq0w.c's turns_run): 2, held
** for the game's own time (asleep AH_NAP ticks at a time, its wakes as few as when nothing's wanted: a long sleep's
** timing as it was); 1, for a command or keys (AH_HOLD: on again the sooner).  The hub's header in the window */
static unsigned char hst, hsq, hcm, hks;        /* (The hold's state: 0 none, 1 since ht0, 2 run out; its line's, and its
                                                **   command's and keys' as it began) */
static unsigned ht0;
static unsigned char giving (void)
{
    if (HUB->starting || HUB->pc_task.kind || ((unsigned char) HUB->pcsum.cond & C_DEAD) ||
        (!HUB->tpr && (HUB->pc_ready > HUB->now || HUB->iq_head != HUB->iq_tail || HUB->ui_giving))) {
        return 2;
    }
    if (!(HUB->ui_cmd | HUB->ui_keys)) {
        hst = 0;
        return 0;
    }
    if (!HUB->tpr) {
        return 1;                               /* (The turns pace: while it's in hand) */
    }
    if (HUB->cq.seq != hsq || HUB->ui_cmd != hcm || HUB->ui_keys != hks) {
        hks = HUB->ui_keys;                     /* (A line typed again, a new line entered: its own hold) */
        hsq = HUB->cq.seq;
        hcm = HUB->ui_cmd;
        hst = 0;
    }
    if (!hst) {
        hst = 1;
        ht0 = hy_ticks ();
    } else if (hst == 1 && hy_ticks () - ht0 >= (HUB->ui_cmd ? GIVE_WAY : KEY_HOLD)) {
        hst = 2;
    }
    return hst == 1;
}

/* The chooser's give-way (ahead.c's ahead_next, at each actor, and make_ahead's start): 1, a request posted or a
** command in hand or a line being typed (giving's), the choice left to be looked at again as the maker next wakes; else
** 0.  Called from the overlay's code: the window put back as it was (2026-11-17: the chooser ran on into a line's
** answer, 26,000-36,000 cycles of a turn's first letter) */
unsigned char ahead_held (void)
{
    unsigned w = hub_here ();
    unsigned char r;

    hub_map (0);
    r = asked () || giving ();
    hub_back (w);
    return r;
}

/* At a checkpoint of a chunk being made ahead (make.c's ahead_check): 1, give it up (a request posted for anything but
** it: ahead.c's nsn, ncx, ncy; the game's end; another the nearest wanted now, the character moved on: looked at again
** as it comes to another eight squares' block); else, giving way, held there, asleep AH_HOLD ticks at a time; 0, go on (in
** the kernel's foreground if one there's asked for this very chunk).  The hub's header in the window after */
static unsigned char asn;                       /* (The character's place as it was last looked at) */
static unsigned ax, ay;
unsigned char ahead_wanted (void)
{
    struct mreq* q;
    unsigned char i, w, g;

    for (;;) {
        hub_map (0);
        if (HUB->quit || (w = asked ()) & 0x10) {
            return 1;
        }
        if (w) {
            for (i = 0, q = HUB->mreq; i < NASK; ++i, ++q) {
                if ((w >> i & 1) && (q->kind != M_CHUNK || q->sn != nsn || q->cx != ncx || q->cy != ncy)) {
                    if (q->kind == M_CHUNK && ahead_at (q->sn, q->cx, q->cy) != 0xFF && ahead_mine ()) {
                        continue;               /* (A chunk made ahead to finish, and the one in hand the
                                                **   character's eyes want too: grown first, not thrown away,
                                                **   2026-11-17; the asker waits the grow's rest) */
                    }
                    return 1;
                }
            }
            fore (w);
            return 0;
        }
        if (!(g = giving ())) {
            if (HUB->pc.sn != asn || (HUB->pc.x ^ ax) & ~7 || (HUB->pc.y ^ ay) & ~7) {
                asn = HUB->pc.sn;
                ax = HUB->pc.x;
                ay = HUB->pc.y;
                return !ahead_still ();
            }
            hy_yield ();                        /* (The other tasks' turn first, any that wants it: the background's
                                                **   least, the denizens' minds and the world's own work not put off by
                                                **   it, 2026-11-17) */
            return 0;
        }
        hy_sem_wait (HUB->sem[T_MAKER], g == 2 ? AH_NAP : AH_HOLD);
    }
}

int main (int argc, char* argv[])
{
    unsigned char i, w, t, calm = 0;

    if (argc < 4 || ovl_load (argv[3]) || hub_attach (num (argv[1]), num (argv[2]))) {
        return 1;
    }
    libpath = argv[3];
    changes_init ();
    hub_set (THEME_BANK, THEME_PLACE, 0xFF, NPIECES * sizeof (struct tplace));     /* (No piece placed yet) */
    for (;;) {
        hub_map (0);                            /* (The hub's header: its own banks may be in the window) */
        if (HUB->quit) {
            break;
        }
        if (!(w = asked ())) {                  /* (Nothing asked: a chunk made ahead, or asleep on its doorbell, woken
                                                **   by a request's ring) */
            fore (0);
            if ((t = giving ()) != 0) {
                calm = 0;
                t = t == 2 ? AH_NAP : AH_HOLD;
            } else if (!calm) {
                calm = 1;
                t = AH_SETTLE;                  /* (Once, rung awake by a request meanwhile) */
            } else if (ahead_next ()) {
                make_ahead (nsn, ncx, ncy);
                continue;
            } else {
                t = AH_NAP;
            }
            hub_map (0);
            hy_sem_wait (HUB->sem[T_MAKER], t);
            continue;
        }
        calm = 0;
        fore (w);                               /* (One in the foreground waits on it: there too, at the real paces) */
        for (i = 0; i < NASK; ++i) {
            if (hub_posted (&HUB->mreq[i])) {
                serve (i);
            }
        }
        if (hub_posted (&HUB->mapq)) {          /* (The UI's asked for the map) */
            hub_get (WORLD_BANK, WREC->strata, strata, sizeof strata);
            draw_map ();
            hub_reply (&HUB->mapq);
        }
    }
    return 0;
}
