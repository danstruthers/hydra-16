/*
** dq0s.c - Dingus Quest Zero's sound (SOUND.md): a task of its own, started by the UI (dq0s SEG ROLE LIBDIR: the
** hub's segment, its role there, hub.h, and the game's lib folder).  It puts the sounds' table (sounds.txt, made
** lib/dq0/dq0s.dat) in the hub as it starts (the denizens' events' bank, after their ring: SND_AT, where the UI finds
** a sound's words), claims its channels (the YM2151's 4-6, and with a Vera X the PSG's 20-23: the rest are the
** music's and the bell's), and plays what the character hears: the denizens' events (their steps and noises, each
** kind's own sound) and the world's (the character's digging, the doors it works, a trap sprung), each read as it
** comes, by its own place in each queue (nobody waits for it, nor it for anyone).  A source is heard within its
** loudness times (hearing + 3) / 2 squares (the character's hearing, the referee's), each square of wall or rock
** between four squares further, each shut door two (the world says what's between: E_DEN's x[3]), as the
** UI tells it; its level falling off with the distance, a quarter less behind, muffled lower; on the speaker of the
** side it lies on as the character faces (ahead or behind: both).  Each pass the latest of each source is kept,
** and played loudest first: on the channel already playing that source, else a free one (a noise the PSG's first),
** else the quietest's, if it's louder; else it isn't played.  A channel plays its sound's hits by the system's tick.
** With no sound driver it ends once the table's in the hub (the UI's words don't need it); paused, it's silent.
**
** dq0s -t LIBDIR: a scene of its own, without the hub (for the tests): a rat to the east of a character facing
** north, then more sources than channels, and one louder while they're all playing; each told as it's played.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include <snd.h>
#include "../dq0/hub.h"
#include "../dq0/level.h"

#pragma static-locals (on)                      /* (Its locals static: quicker, and nothing here is reentered) */

/* ---- The sounds: the data file's (make.js's, from sounds.txt), in the hub (SND_AT), a record at a time copied out */

struct dsnd {
    char            words[SND_WORDS];           /* What the character hears it as ("a scurrying"; "": none) */
    unsigned char   drum;                       /* The YM2151's: 1 a General MIDI drum (patch its number), */
    unsigned char   patch, note;                /*   else a patch at a note */
    unsigned char   wave, pnote;                /* The PSG's: its waveform << 6 | its width, and its note */
    unsigned char   hits, gap, hold;            /* How many, ticks from one's start to the next's, ticks each held */
    unsigned char   level;                      /* At its loudest, 0-127 */
    unsigned char   flags;                      /* S_* */
    unsigned char   spare[2];
};
typedef char dsnd_fits[sizeof (struct dsnd) == SND_REC ? 1 : -1];
#define S_USED          0x01                    /* A sound there */
#define S_NOISY         0x02                    /* A noise (the PSG's, first) */
enum { WS_DIG, WS_OPEN, WS_SHUT, WS_BLOW, WS_SMASH, WS_CHAINS, WS_TRAP, WS_HIT, WS_SWISH };   /* The world's
                                                **   (sounds.txt's *dig ...; a fight's blow landing, missing) */
#define WORLD(w)        (2 * SND_KINDS + (w))
#define NSOUNDS         (2 * SND_KINDS + SND_WORLD)

static unsigned char testing, tb;               /* (dq0s -t: its own bank for the table, no hub) */
static struct dsnd R;                           /* (A sound, copied out) */
static unsigned char piece[256];                /* (The file's pieces, through the main RAM) */

static void table_map (void)
{
    if (testing) {
        hy_bank (tb);
    } else {
        hub_map (DEN_BANK);
    }
}

static void __fastcall__ snd_get (unsigned char n)
{
    table_map ();
    memcpy (&R, SND_AT + n * SND_REC, sizeof R);
    if (!testing) {
        hub_map (0);
    }
}

/* The table read in (dir/dq0s.dat): 0, or -1 */
static int __fastcall__ table_load (const char* dir)
{
    static char path[64];
    unsigned off;
    int fd, n;

    strcpy (path, dir);
    strcat (path, "/dq0s.dat");
    if ((fd = open (path, O_RDONLY)) < 0) {
        return -1;
    }
    for (off = 0; off < NSOUNDS * SND_REC && (n = read (fd, piece, sizeof piece)) > 0; off += n) {
        table_map ();
        memcpy (SND_AT + off, piece, n);
    }
    close (fd);
    if (!testing) {
        hub_map (0);
    }
    return 0;
}

/* ---- The channels: the YM2151's 4-6, then (with a Vera X) the PSG's 20-23; each playing a sound's hits */

#define NCH             7
#define NYM             3
static const unsigned char chan[NCH] = { 4, 5, 6, 20, 21, 22, 23 };
static unsigned char nch = NYM;                 /* (NCH with a Vera X) */
struct ch {
    unsigned char   who;                        /* Its source (an actor + 1, 0 the character's own; 0xFF: free) */
    unsigned char   snd, loaded;                /* Its sound, and the one its patch is (0xFF: none yet) */
    unsigned char   level, pan, note;           /* As it's played */
    unsigned char   hits, on;                   /* Hits to go, one held */
    unsigned        at, next;                   /* The tick its hit began, and of its next change */
    struct dsnd     s;
};
static struct ch cs[NCH];
static unsigned tick;                           /* (The system's tick, this pass) */
static unsigned char pairs[20];

/* A pair for channel k's request */
static unsigned char np;
static void __fastcall__ pair (unsigned char reg, unsigned char val)
{
    pairs[np++] = reg;
    pairs[np++] = val;
}

/* Channel k's hit: the last one's off, its speakers, its level, its sound (a drum, a patch's note, a waveform's) */
static void __fastcall__ hit (unsigned char k)
{
    struct ch* c = &cs[k];

    np = 0;
    pair (HY_SND_R_CH, chan[k]);
    pair (HY_SND_R_OFF, 0);
    pair (HY_SND_R_PAN, c->pan);
    pair (HY_SND_R_VOL, c->level);
    if (k >= NYM) {                             /* (The PSG's: its waveform, then its note) */
        if (c->loaded != c->snd) {
            pair (HY_SND_R_WAVE, c->s.wave);
        }
        pair (HY_SND_R_NOTE, c->note);
    } else if (c->s.drum) {
        pair (HY_SND_R_DRUM, c->s.patch);       /* (Its patch and its pitch: keyed on) */
    } else {
        if (c->loaded != c->snd) {
            pair (HY_SND_R_PATCH, c->s.patch);
        }
        pair (HY_SND_R_NOTE, c->note);
    }
    snd_writes (pairs, np / 2);
    c->loaded = c->s.drum && k < NYM ? 0xFF : c->snd;
    c->on = 1;
    c->at = tick;
    c->next = tick + c->s.hold;
}

static void __fastcall__ off (unsigned char k)
{
    np = 0;
    pair (HY_SND_R_CH, chan[k]);
    pair (HY_SND_R_OFF, 0);
    snd_writes (pairs, 2);
    cs[k].on = 0;
}

/* Each channel's hits as they fall due: the soonest change's ticks from now (10 at most, a twentieth of a second: the
** queues looked at again; the world's events ring it, the denizens' don't, a step heard that much late at most) */
static unsigned char tend (void)
{
    struct ch* c;
    unsigned char k, wait = 10;
    unsigned d;

    for (k = 0; k < nch; ++k) {
        c = &cs[k];
        if (c->who == 0xFF) {
            continue;
        }
        if ((int) (tick - c->next) >= 0) {
            if (!c->on) {
                hit (k);
            } else {
                off (k);
                if (--c->hits) {
                    c->next = c->at + c->s.gap;
                    if ((int) (tick - c->next) >= 0) {
                        hit (k);
                    }
                } else {
                    c->who = 0xFF;              /* (Played out: free) */
                    continue;
                }
            }
        }
        d = c->next - tick;
        if (d < wait) {
            wait = d ? d : 1;
        }
    }
    return wait;
}

static void silence (void)
{
    unsigned char k;

    for (k = 0; k < nch; ++k) {
        if (cs[k].who != 0xFF) {
            off (k);
            cs[k].who = 0xFF;
        }
    }
}

/* ---- The sources heard this pass: the latest of each, then played loudest first */

#define NSRC            12
struct src {
    unsigned char   who, snd, level, pan, note;
    unsigned char   muffled;                    /* (Walls or a shut door between) */
};
static struct src sv[NSRC];
static unsigned char nsrc;
static unsigned char hearing, face;             /* (The character's, this pass) */

/* A sound heard (sound n, from who, loud 0-9, dx dy squares off the character, between: E_DEN's x[3]): its level, its
** speakers and its note worked out, kept as who's latest (or, the list full, in place of the quietest, if it's
** louder); 0 not heard, else its level */
static unsigned char __fastcall__ heard (unsigned char who, unsigned char n, unsigned char loud, int dx, int dy,
    unsigned char between)
{
    static struct src s;
    unsigned d, reach;
    unsigned char k, q, rel;

    snd_get (n);
    if (!(R.flags & S_USED) || !loud) {
        return 0;
    }
    rel = dx || dy ? (bearing (dx, dy) - face) & 7 : 0;
    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;
    d = 2 * ((dx > dy ? dx : dy) + 4 * (between & 15) + 2 * (between >> 4));
    reach = loud * (hearing + 3);
    if (d > reach) {
        return 0;
    }
    s.who = who;
    s.snd = n;
    s.level = (unsigned) R.level * (reach - d + 2) / (reach + 2);
    if (rel >= 3 && rel <= 5) {
        s.level -= s.level / 4;                 /* (Behind: softer) */
    }
    if (s.level < 8) {
        s.level = 8;
    }
    s.pan = rel == 0 || rel == 4 ? SND_PAN_BOTH : rel < 4 ? SND_PAN_RIGHT : SND_PAN_LEFT;
    s.muffled = between != 0;
    s.note = s.muffled && R.note > 5 ? R.note - 5 : R.note;    /* (Muffled: duller, lower) */
    for (k = q = 0; k < nsrc; ++k) {
        if (sv[k].who == who) {
            break;
        }
        if (sv[k].level < sv[q].level) {
            q = k;
        }
    }
    if (k == nsrc) {
        if (nsrc < NSRC) {
            ++nsrc;
        } else if (sv[q].level < s.level) {
            k = q;
        } else {
            return s.level;
        }
    }
    memcpy (&sv[k], &s, sizeof s);
    return s.level;
}

/* A source played: on the channel already playing it, else a free one (a noise the PSG's first, with a Vera X), else
** the quietest's if it's louder; its channel (0xFF: not played), and who it cut short in cut (0xFF: nobody) */
static unsigned char cut;
static unsigned char __fastcall__ place (const struct src* s)
{
    struct ch* c;
    unsigned char k, q = 0xFF;

    cut = 0xFF;
    for (k = 0; k < nch && cs[k].who != s->who; ++k) {
    }
    if (k == nch) {
        snd_get (s->snd);
        for (k = 0; k < nch; ++k) {             /* (A free one: the PSG's first for a noise) */
            q = R.flags & S_NOISY && nch > NYM ? (k + NYM) % nch : k;
            if (cs[q].who == 0xFF) {
                break;
            }
        }
        if (k < nch) {
            k = q;
        } else {
            for (k = q = 0; k < nch; ++k) {     /* (The quietest's, if it's louder) */
                if (cs[k].level < cs[q].level) {
                    q = k;
                }
            }
            if (cs[q].level >= s->level) {
                return 0xFF;
            }
            k = q;
            cut = cs[k].who;
            off (k);
        }
    }
    c = &cs[k];
    snd_get (s->snd);
    memcpy (&c->s, &R, sizeof R);
    c->who = s->who;
    c->snd = s->snd;
    c->level = s->level;
    c->pan = s->pan;
    c->note = k >= NYM ? R.pnote - (s->muffled && R.pnote > 12 ? 12 : 0) : s->note;
    c->hits = R.hits ? R.hits : 1;
    if (R.gap <= R.hold) {
        c->s.gap = R.hold + 1;
    }
    c->on = 0;
    c->next = tick;
    hit (k);
    return k;
}

/* The sources heard this pass, loudest first, each played (tell: each told, the tests') */
static void __fastcall__ play (unsigned char tell);

/* ---- Listening: the world's events and the denizens', as they come */

static unsigned wplace, dplace;                 /* (Its places in each queue) */

/* A world's event's sound (the character's own: on its square), or 0xFF */
static unsigned char __fastcall__ world_sound (const struct msg* m)
{
    unsigned char o = m->w1;

    if (m->who) {
        return 0xFF;
    }
    switch (m->kind) {
    case E_NOISE:
        return WORLD (WS_DIG);
    case E_TRAP:
        return m->w1 >> 8 == TO_SPRUNG ? WORLD (WS_TRAP) : 0xFF;
    case E_PORTAL:
        if (o == PO_OPENED || o == PO_FORCED || o == PO_UNBARRED) {
            return WORLD (WS_OPEN);
        }
        if (o == PO_CLOSED || o == PO_LOWERED) {
            return o == PO_CLOSED ? WORLD (WS_SHUT) : WORLD (WS_CHAINS);
        }
        if (o == PO_GIVING || o == PO_HOLDING || o == PO_NO_MARK) {
            return WORLD (WS_BLOW);
        }
        if (o == PO_BROKE_DOWN || o == PO_SHATTERED || o == PO_SMASHED || o == PO_WRECKED) {
            return WORLD (WS_SMASH);
        }
        if (o == PO_RAISED || o == PO_HEAVED) {
            return WORLD (WS_CHAINS);
        }
    }
    return 0xFF;
}

static void listen (void)
{
    static struct msg m;
    unsigned char n, sn;
    unsigned px, py;

    hub_map (0);
    hearing = HUB->pcsum.hearing;
    face = HUB->pc_face;
    sn = HUB->pc_stratum;
    px = HUB->pc_x;
    py = HUB->pc_y;
    while (hub_next (&wplace, &m)) {
        if ((n = world_sound (&m)) != 0xFF) {
            heard (0, n, m.kind == E_NOISE ? m.a : 9, 0, 0, 0);
        }
        hub_map (0);
    }
    while (hub_dnext (&dplace, &m)) {
        if (m.kind == E_DEN && m.b && m.x[0] == sn && (m.a == DE_MOVED || m.a == DE_NOISE)) {
            heard (m.who, m.x[1] % SND_KINDS * 2 + (m.a == DE_NOISE), m.b, m.w1 - px, m.w2 - py, m.x[3]);
        } else if (m.kind == E_FIGHT && m.x[0] == sn && m.x[1] < FO_BLED) {   /* (A blow: landing, or not) */
            heard (m.who, WORLD (m.x[1] == O_HIT || m.x[1] == O_BOTH_HIT || m.x[1] == O_COUNTER_HIT ||
                m.x[1] == O_DISPATCH ? WS_HIT : WS_SWISH), m.b, m.w1 - px, m.w2 - py, 0);
        }
    }
    hub_map (0);
}

/* ---- dq0s -t: a scene of its own (the tests'), each source told as it's played: "1 a scurrying: channel 4, right,
** level 48" */

static char line[64];

static void __fastcall__ say (const char* s)    /* (A line written whole, at its \n: one request) */
{
    strcat (line, s);
    if (*s == '\n') {
        write (1, line, strlen (line));
        line[0] = 0;
    }
}

static void __fastcall__ say_n (unsigned n)
{
    static char b[6];

    say (utoa (n, b, 10));
}

static const char* const pan_name[4] = { "", "left", "right", "both" };

static void __fastcall__ play (unsigned char tell)
{
    unsigned char i, j, k;
    static struct src t;

    for (i = 0; i < nsrc; ++i) {                /* (Loudest first) */
        for (j = i + 1; j < nsrc; ++j) {
            if (sv[j].level > sv[i].level) {
                memcpy (&t, &sv[i], sizeof t);
                memcpy (&sv[i], &sv[j], sizeof t);
                memcpy (&sv[j], &t, sizeof t);
            }
        }
        k = place (&sv[i]);
        if (tell) {
            say_n (sv[i].who);
            say (" ");
            say (R.words);
            if (k == 0xFF) {
                say (": not played, level ");
            } else {
                say (": channel ");
                say_n (chan[k]);
                say (", ");
                say (pan_name[sv[i].pan & 3]);
                if (sv[i].muffled) {
                    say (", muffled");
                }
                if (cut != 0xFF) {
                    say (", in place of ");
                    say_n (cut);
                }
                say (", level ");
            }
            say_n (sv[i].level);
            say ("\n");
        }
    }
    nsrc = 0;
}

/* The scene: who, its sound, how loud, where (dx, dy from the character, facing north), and what's between; a 0 who
** ends a moment, after which the channels play for its ticks */
static const signed char scene[][6] = {
    { 1, 2 * 1, 1, 2, 0, 0 },                   /* A rat's step, 2 squares east (to the right) */
    { 0, 60 },
    { 2, 2 * 5 + 1, 3, 0, -6, 0 },              /* A bear's growl, 6 north (ahead) */
    { 3, 2 * 1 + 1, 3, -3, 0, 0 },              /* A rat's squeak, 3 west (to the left) */
    { 4, 2 * 3, 1, 0, 2, 0 },                   /* A ghoul's step, 2 south (behind) */
    { 5, 2 * 4, 1, 3, -1, 0 },                  /* A skeleton's step, 3 east (to the right) */
    { 6, 2 * 6 + 1, 3, 2, 2, 0x10 },            /* A kobold's yapping, behind a shut door, 2 southeast */
    { 0, 5 },
    { 7, 2 * 5, 2, -1, 0, 0 },                  /* A bear's step, beside it to the west: louder than the rest */
    { 0, 100 }
};

static void test (void)
{
    unsigned char i;
    unsigned until;

    hearing = 5;
    face = H_N;
    for (i = 0; i < sizeof scene / sizeof scene[0]; ++i) {
        if (scene[i][0]) {
            heard (scene[i][0], scene[i][1], scene[i][2], scene[i][3], scene[i][4], scene[i][5]);
            continue;
        }
        tick = hy_ticks ();
        play (1);
        for (until = hy_ticks () + (unsigned char) scene[i][1]; (int) (tick - until) < 0; tick = hy_ticks ()) {
            hy_sleep_ticks (tend ());
        }
    }
    silence ();
}

int main (int argc, char* argv[])
{
    static char state[64];
    unsigned char k;
    int fd, n;

    for (k = 0; k < NCH; ++k) {
        cs[k].who = cs[k].loaded = 0xFF;
    }
    if (argc == 3 && !strcmp (argv[1], "-t")) {
        testing = 1;
        if ((n = hy_banks_alloc (1)) < 0) {
            return 1;
        }
        tb = n;
        if (table_load (argv[2])) {
            return 1;
        }
    } else if (argc < 4 || hub_attach (atoi (argv[1]), atoi (argv[2])) || table_load (argv[3])) {
        return 1;
    }
    if (!testing) {
        ++HUB->beat[T_SOUND];                   /* (The table in the hub: the UI goes on) */
    }
    if (snd_open ()) {                          /* (No sound driver: the words are in the hub, the game silent) */
        if (!testing) {
            hub_map (0);
            HUB->task[T_SOUND] = 0;
        }
        return 0;
    }
    if ((fd = open ("/dev/sndctl", O_RDONLY)) >= 0) {
        n = read (fd, state, sizeof state - 1);   /* ("volume 100\nchannels 24\n...": a Vera X's PSG too) */
        close (fd);
        state[n > 0 ? n : 0] = 0;
        if (strstr (state, "channels 24")) {
            nch = NCH;
        }
    }
    snd_claim (0x70);                           /* (Its own: nobody else's writes reach them while it plays) */
    if (nch > NYM) {
        snd_claim_psg (0xF000);
    }
    if (testing) {
        test ();
        snd_close ();
        return 0;
    }
    wplace = HUB->eq_head;                      /* (Its events from now on) */
    dplace = HUB->dq_head;
    while (hub_map (0), !HUB->quit) {
        while (hy_sem_try (HUB->sem[T_SOUND]) == 0) {
        }
        tick = hy_ticks ();
        listen ();
        if (HUB->paused) {
            nsrc = 0;
            silence ();
        } else {
            play (0);
        }
        k = tend ();
        hub_map (0);
        ++HUB->beat[T_SOUND];
        hy_sleep_ticks (k);
    }
    silence ();
    snd_close ();
    return 0;
}
