/*
** dq0life.c - Dingus Quest Zero between its dungeons (DUNGEON.md's a character's life): run by the game (main.c's s_life) as
** it starts with a save's folder and no dungeon going on in it, before anything of the game's.  It keeps the record of the
** life (../dq0/life.h; the program's in lib/dq0): a life begun, or the last dungeon's end recorded and the next one begun: its seed, its date, its
** folder.  What passes between dungeons will go here (the plan's steps 4 to 6): it's a program of its own, in no task's way.
**
** dq0life -d folder [-s seed]
**   -d         the save's folder
**   -s         dq0 -s's seed: the dungeon's (a new life's too); without it, a new life's the clock's, the next dungeon's the
**              life's for it
** By the record there and its dungeon's game.dat: one ended with the character alive, the next dungeon begun; a dungeon past the
** first with no game (the character died in it before anything was kept: there's no save to go back to but its start), that
** dungeon begun again, the record as it is; else (none, one of another version, one ended in its death) a new life.
** Its exit status 0, or 1 (its fault said).
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
    if (!y) {
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

/* The last dungeon's end recorded (its outcome, the whole days its clock came to: world.dat's), the date on to the day after it
** came out (by its seed's hour, as the denizens' s_hour has it), at the soonest: the plan's step 4 adds what passes after */
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
    L.h.date += (t + ((unsigned) L.h.dseed % 24) * 36000UL) / TENTHS_DAY + 1;
    L.h.dungeon = n + 1;
}

int main (int argc, char* argv[])
{
    unsigned long seed = 0;
    unsigned char i, given = 0;
    int fd;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-d") && i + 1 < argc) {
            top = argv[++i];
        } else if (!strcmp (argv[i], "-s") && i + 1 < argc) {
            given = 1;
            seed = num (argv[++i]);
        } else {
            die ("usage: dq0life -d folder [-s seed]");
        }
    }
    if (!top || strlen (top) > TOP_MAX) {
        die ("the save's folder's name is too long");
    }
    mkdir (top, 0755);                          /* (It may be there already) */
    i = next_is ();
    if (i == WH_AGAIN) {
        return 0;                               /* (Its seed, its folder, its record as they are: dq0's to begin) */
    }
    if (i == WH_NEXT) {
        ended ();
    } else {                                    /* (A new life: no kit to take up, its outfit's drawn) */
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
    if ((fd = open (in_ (top, LIFE_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, &L, sizeof L) != sizeof L) {
        die ("can't write the record of the life");
    }
    close (fd);
    return 0;
}
