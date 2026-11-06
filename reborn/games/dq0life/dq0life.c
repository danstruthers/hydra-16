/*
** dq0life.c - Dingus Quest Zero between its dungeons (DUNGEON.md's a character's life): run by the game (main.c's s_life) as
** it starts with a save's folder and no dungeon going on in it, before anything of the game's.  It keeps the record of the
** life (../dq0/life.h; the program's in lib/dq0): a life begun, or the last dungeon's end recorded and the next one begun: its seed, its date, its
** folder.  What passes between dungeons is dq0acct's, the program beside it (the plan's steps 4 to 6), run from here; the life's
** account, as it ends, dq0end's.  It's a program of its own, in no task's way, and small: every new life with a folder reads it.
**
** dq0life -d folder [-s seed] [-l libdir]
**   -d         the save's folder
**   -s         dq0 -s's seed: the dungeon's (a new life's too); without it, a new life's the clock's, the next dungeon's the
**              life's for it
**   -l         the game's lib folder: dq0acct's (none, the time between isn't told)
** By the record there and its dungeon's game.dat: one ended with the character alive, the next dungeon begun (its time between,
** dq0acct's); a dungeon past the first with no game (the character died in it before anything was kept: there's no save to go back
** to but its start), that dungeon begun again, the record as it is; else (none, one of another version, one ended in its death, one
** whose life's over) a new life, a life that's over kept in lives.dat first.
** Its exit status 0; 2 the character dead between the dungeons (told; the game's not to begin); or 1 (its fault said).
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/life.h"
#include "../dq0/theme.h"
#include "../dq0w/outfit.h"                 /* (kit.dat's struct kitrec: the clues carried out) */

#define TOP_MAX         31                      /* (The hub's savedir holds 38: this, then "/d65535") */
#define YEAR0           720                     /* (The year a life begins: the house's, the book's own world's reckoning) */
#define AGE0            16                      /* (Its age then, this and up to nine more, by the seed: the house's) */
#define TENTHS_DAY      864000UL                /* (The game's clock: tenths of a second) */

static struct life L;
static struct wsave w;
static struct save sv;
static char path[56], dir[40];
static char buf[256];
static const char* top;

static void __fastcall__ die (const char* why)
{
    write (2, "dq0life: ", 9);
    write (2, why, strlen (why));
    write (2, "\n", 1);
    exit (1);
}

static const char* __fastcall__ in_ (const char* folder, const char* name)
{
    strcpy (path, folder);
    strcat (path, "/");
    strcat (path, name);
    return path;
}

static unsigned long __fastcall__ num (const char* s)
{
    unsigned long n = 0;

    for (; *s >= '0' && *s <= '9'; ++s) {
        n = n * 10 + *s - '0';
    }
    return n;
}

/* Dungeon n's seed, by the life's: the life's own the first's (a game begun with dq0 -s has that seed's dungeon); a seed under
** 65,536 goes on with seeds under it (the stand-in themes': main.c's history), one over goes on over it */
static unsigned long __fastcall__ seed_of (unsigned long s, unsigned n)
{
    unsigned long x;

    if (n <= 1) {
        return s;
    }
    x = s + n * 2654435761UL;
    x ^= x >> 15;
    x *= 40503UL;
    x ^= x >> 13;
    return s < 65536UL ? x & 0xFFFF : x | 0x10000UL;
}

/* A theme written by hand left in the save's folder (theme.txt: dq0d's THEME_FILE, the factions' lines) is the new dungeon's:
** moved into its folder */
static void theme_in (void)
{
    int in, out, n;

    if ((in = open (in_ (top, "theme.txt"), O_RDONLY)) < 0) {
        return;
    }
    if ((out = open (in_ (dir, "theme.txt"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        die ("can't write the dungeon's theme.txt");
    }
    while ((n = read (in, buf, sizeof buf)) > 0) {
        write (out, buf, n);
    }
    close (out);
    close (in);
    remove (in_ (top, "theme.txt"));
}

static void __fastcall__ dungeon_dir (unsigned n)
{
    strcpy (dir, top);
    strcat (dir, "/d");
    utoa (n, dir + strlen (dir), 10);
}

/* The record read (L), and its dungeon's game (sv): what's next (WH_*) */
enum { WH_LIFE, WH_NEXT, WH_AGAIN };
static unsigned char next_is (void)
{
    int fd;
    unsigned char y = 0;

    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) >= 0) {
        y = read (fd, &L, sizeof L) == sizeof L && L.h.magic == LIFE_MAGIC && L.h.version == LIFE_VER;
        close (fd);
    }
    if (!y || (L.h.flags & LF_OVER)) {
        return WH_LIFE;
    }
    dungeon_dir (L.h.dungeon);
    if ((fd = open (in_ (dir, "game.dat"), O_RDONLY)) < 0) {
        return L.h.dungeon > 1 ? WH_AGAIN : WH_LIFE;
    }
    y = read (fd, &sv, sizeof sv) == sizeof sv && sv.magic == SAVE_MAGIC && sv.version == SAVE_VER && sv.q.done &&
        sv.q.done != END_DIED;
    close (fd);
    return y ? WH_NEXT : WH_LIFE;
}

static void record_write (void)
{
    int fd;

    if ((fd = open (in_ (top, LIFE_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, &L, sizeof L) != sizeof L) {
        die ("can't write the record of the life");
    }
    close (fd);
}

/* A program in the lib folder run on this folder (the system's SPAWN itself, as hub_run does: hy_spawn's argv wants the heap), waited for:
** its status, or -1 */
static int __fastcall__ run (const char* lib, const char* name, unsigned char with_lib)
{
    static char p[64], a[96];
    static struct hy_regs r;
    char* q = a;

    strcpy (p, lib);
    strcat (p, name);
    strcpy (q, "-d");
    q += 3;
    strcpy (q, top);
    q += strlen (q) + 1;
    if (with_lib) {
        strcpy (q, "-l");
        q += 3;
        strcpy (q, lib);
        q += strlen (q) + 1;
    }
    *q = 0;
    r.a = 0;                                    /* (Its fds: this one's 0, 1 and 2) */
    r.r[0] = (unsigned) p;
    r.r[1] = (unsigned) a;
    if (hy_call (HY_SPAWN, &r)) {
        return -1;
    }
    r.r[0] = 0;
    return hy_call (HY_WAIT, &r) ? -1 : r.x;
}

/* The time between: dq0acct run on the record as it's been written, the record read again: its date on, or the life over */
static void __fastcall__ between (const char* lib)
{
    int t, fd;

    t = run (lib, "/../acct/dq0acct", 1);
    if (t != 0 && t != 2) {
        die ("the time between failed");
    }
    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) >= 0) {
        read (fd, &L, sizeof L);
        close (fd);
    }
    if (t == 2) {                               /* (Dead of it: the life's account, dq0end's) */
        run (lib, "/../acct/dq0end", 0);
        exit (2);
    }
}

/* The last dungeon's end recorded (its outcome, the whole days its clock came to: world.dat's), the date on to the day after it
** came out (by its seed's hour, as the denizens' s_hour has it), at the soonest: dq0acct's adds what passes after */
static void ended (void)
{
    unsigned n = L.h.dungeon;
    unsigned long t = 0;
    int fd;

    dungeon_dir (n);
    if ((fd = open (in_ (dir, "world.dat"), O_RDONLY)) >= 0) {
        if (read (fd, &w, sizeof w) == sizeof w && w.magic == WSAVE_MAGIC) {
            t = w.now;
        }
        close (fd);
    }
    if (n <= LIFE_DUNGEONS) {
        L.d[n - 1].days = (unsigned) (t / TENTHS_DAY);
        L.d[n - 1].outcome = sv.outcome;
    }
    if (sv.pace >> 7) {
        L.h.flags |= LF_HARD;                   /* (Hard mode goes on with the life, the fight soak's: dq0 -H said once) */
    }
    L.h.date += (t + ((unsigned) L.h.dseed % 24) * 36000UL) / TENTHS_DAY + 1;
    L.h.dungeon = n + 1;
}

/* ---- The clues carried out (the plan's step 10; life.h's): dungeon n's (its folder's clues.dat, the UI's, a record a rule) that
** were read, or that are on a thing it carries (kit.dat: an old writing, a copy, its fuel CLUE_HERE | the rule, CLUE_COPY too, made
** its clue's number at the top), added to the top's clues.dat, and told; the first read of them a rumour heard (heard.txt, at the
** top: the one-link clue, a part-written theme begun by its rumour line, the place it names its region; step 7's rumours tell it
** first, in a made one's place) */

static struct clue K;
static struct kitrec kt;
static const char* const landk[6] = { "mountains | granite | springs | cold", "hills | limestone | springs | mild",
    "forest | sandstone | river | mild", "marsh | sandstone | lake | wet", "coast | chalk | sea | wet", "plain | chalk | river | dry" };
static const char* const lies[6] = { "in the mountains", "in the hills", "in the woods", "in the marshes", "on the coast",
    "on the plains" };
static const char* const works[6] = { "raised another keep", "built another temple", "dug other mines", "laid others of their dead",
    "built another town", "carved another hold" };

static void __fastcall__ out (const char* s)
{
    write (1, s, strlen (s));
}

/* What clue K says of its place (wr: as a wrong reading has it) into buf: "the Ostmarkan Fells, where the hill-kings ..." */
static void __fastcall__ place_in (unsigned char wr)
{
    strcpy (buf, wr ? K.wrong : K.place);
    strcat (buf, ", where ");
    strcat (buf, K.people[0] ? K.people : "its makers");
    strcat (buf, " ");
    strcat (buf, works[((K.work & 15) + wr) % 6]);
}

static void __fastcall__ clues_out (unsigned n)
{
    static char heard[240];
    int fd, kf, tf;
    unsigned base = 0, at;
    unsigned char r, st, carried, best = 0;

    dungeon_dir (n);
    if ((fd = open (in_ (dir, CLUES_FILE), O_RDONLY)) < 0) {
        return;
    }
    if ((tf = open (in_ (top, CLUES_FILE), O_RDWR | O_CREAT, 0644)) < 0) {
        close (fd);
        return;
    }
    while (read (tf, &K, CLUE_SIZE) == CLUE_SIZE) {
        ++base;                                 /* (Those carried out before: the new ones after them) */
    }
    kf = open (in_ (top, "kit.dat"), O_RDWR);
    heard[0] = 0;
    for (r = 0; r < 8 && read (fd, &K, CLUE_SIZE) == CLUE_SIZE; ++r) {
        if (!K.state) {
            continue;
        }
        carried = 0;
        if (kf >= 0) {                          /* (On a thing carried: its fuel its number at the top now) */
            lseek (kf, 0, SEEK_SET);
            for (at = 0; read (kf, &kt, sizeof kt) == sizeof kt; ++at) {
                if ((kt.o.kind == KI_WRITING || kt.o.kind == KI_COPY) && (kt.fuel & (CLUE_HERE | 0xFF)) == (CLUE_HERE | r)) {
                    carried |= kt.fuel & CLUE_COPY ? 2 : 1;
                    kt.fuel = (kt.fuel & CLUE_COPY) | (base + 1);
                    lseek (kf, (long) at * sizeof kt, SEEK_SET);
                    write (kf, &kt, sizeof kt);
                }
            }
        }
        st = K.state & ~CL_MORE;
        if (st == CL_UNREAD && !carried) {
            continue;                           /* (Neither read nor carried: left behind) */
        }
        K.dungeon = n;
        K.carried = carried;
        write (tf, &K, CLUE_SIZE);
        ++base;
        if (st == CL_PIECE) {
            out ("You came out with a piece of what the ");
            out (K.thing);
            out (" says: some place ");
            out (lies[(K.land & 15) % 6]);
            out (".\n");
        } else if (st != CL_UNREAD) {
            place_in (st == CL_WRONG);
            out ("You came out knowing what the ");
            out (K.thing);
            out (" says: it tells of ");
            out (buf);
            out (".\n");
        } else {
            out (carried & 1 ? "You carry out the " : "You carry out a copy of the ");
            out (K.thing);
            out (", unread.\n");
        }
        if (st > CL_UNREAD && K.place[0] && (!best || (best == CL_PIECE && st != CL_PIECE))) {
            best = st;                          /* (The rumour heard: a reading's, a whole one's before a piece's) */
            strcpy (heard, "rumour   | ");
            if (st == CL_PIECE) {
                strcat (heard, "What you made out of the ");
                strcat (heard, K.thing);
                strcat (heard, " speaks of some place ");
                strcat (heard, lies[(K.land & 15) % 6]);
            } else {
                strcat (heard, "The ");
                strcat (heard, K.thing);
                strcat (heard, " you read tells of ");
                place_in (st == CL_WRONG);
                strcat (heard, buf);
            }
            strcat (heard, ".\nregion   | ");
            strcat (heard, st == CL_WRONG ? K.wrong : K.place);
            strcat (heard, " | ");
            strcat (heard, landk[(st == CL_WRONG ? K.work >> 4 : K.land & 15) % 6]);
            strcat (heard, "\n");
        }
    }
    close (fd);
    close (tf);
    if (kf >= 0) {
        close (kf);
    }
    remove (in_ (top, "heard.txt"));
    if (heard[0] && (fd = open (in_ (top, "heard.txt"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, heard, strlen (heard));
        close (fd);
    }
}

int main (int argc, char* argv[])
{
    unsigned long seed = 0;
    unsigned char i, given = 0;
    const char* lib = 0;
    int fd;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-d") && i + 1 < argc) {
            top = argv[++i];
        } else if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            given = 1;
            seed = num (argv[++i]);
        } else if (!strcmp (argv[i], "-l") && i + 1 < argc) {
            lib = argv[++i];
        } else {
            die ("usage: dq0life -d folder [-s seed] [-l libdir]");
        }
    }
    if (!top || strlen (top) > TOP_MAX) {
        die ("the save's folder's name is too long");
    }
    mkdir (top, 0755);                          /* (It may be there already) */
    i = next_is ();
    if (i == WH_AGAIN && !(L.h.flags & LF_BETWEEN)) {
        return 0;                               /* (Its seed, its folder, its record as they are: dq0's to begin) */
    }
    if (i == WH_NEXT || i == WH_AGAIN) {
        if (i == WH_NEXT) {
            ended ();
            clues_out (L.h.dungeon - 1);        /* (Its clues carried out: the plan's step 10) */
            L.h.flags |= LF_BETWEEN;
            record_write ();
        }
        /* (The time between, HarnMaster's alone: a game of another ruleset's goes on, as it did) */
        if (lib && !strcmp (sv.referee, "dq0harn")) {
            between (lib);
        }
        L.h.flags &= ~LF_BETWEEN;
    } else {                                    /* (A new life: no kit to take up, its outfit's drawn) */
        if (L.h.magic == LIFE_MAGIC && L.h.version == LIFE_VER && (L.h.flags & LF_OVER) &&
            (fd = open (in_ (top, LIVES_FILE), O_WRONLY | O_CREAT | O_APPEND, 0644)) >= 0) {
            write (fd, &L, sizeof L);           /* (The life that's over: kept) */
            close (fd);
        }
        remove (in_ (top, "kit0.dat"));
        remove (in_ (top, CLUES_FILE));         /* (The clues and the rumour of the life that's over) */
        remove (in_ (top, "heard.txt"));
        if (!given) {
            seed = hy_time () ^ hy_ticks ();
        }
        memset (&L, 0, sizeof L);
        L.h.magic = LIFE_MAGIC;
        L.h.version = LIFE_VER;
        L.h.seed = seed;
        L.h.date = LIFE_DATE (YEAR0, 1, 1) + seed % 360;
        L.h.born = YEAR0 - AGE0 - (unsigned) (seed >> 9) % 10;
        L.h.dungeon = 1;
        if ((fd = open (in_ (top, "kit.dat"), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
            close (fd);
        }
    }
    L.h.dseed = given ? seed : seed_of (L.h.seed, L.h.dungeon);
    if (L.h.dungeon <= LIFE_DUNGEONS) {
        L.d[L.h.dungeon - 1].seed = L.h.dseed;
    }
    dungeon_dir (L.h.dungeon);
    mkdir (dir, 0755);
    theme_in ();
    record_write ();
    return 0;
}
