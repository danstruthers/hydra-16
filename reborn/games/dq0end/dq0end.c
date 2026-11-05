/*
** dq0end.c - Dingus Quest Zero's life's account (the plan's section 1: the dungeons, the days, what was done; DUNGEON.md's a character's
** life): a small program in the game's lib folder, run as the character dies: by the UI as the game ends in a death (main.c's s_finish),
** by dq0life if it died between two dungeons (dq0acct's).
**
** dq0end -d folder [-h]
**   -h         hard mode (dq0 -H), the death kept in the save: its end recorded first (its outcome solved or died, LD_DIED, LF_OVER), since the
**              next start begins a new life over it (lives.dat keeps it then).  Without it, and the record not saying it's over, the last
**              save's there to go back to: the record's left as it was, and it's told as the life so far.
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <hydra.h>
#include "../dq0/hub.h"
#include "../dq0/life.h"
#include "../dq0/theme.h"

#define TENTHS_DAY      864000UL                /* (The game's clock: tenths of a second) */

static struct life L;
static struct wsave w;
static struct save sv;
static char path[56], dir[40];
static const char* top;

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

static void __fastcall__ dungeon_dir (unsigned n)
{
    strcpy (dir, top);
    strcat (dir, "/d");
    utoa (n, dir + strlen (dir), 10);
}

static void record_write (void)
{
    int fd;

    if ((fd = open (in_ (top, LIFE_FILE), O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
        write (fd, &L, sizeof L);
        close (fd);
    }
}

static void __fastcall__ out (const char* s)
{
    write (1, s, strlen (s));
}

static void __fastcall__ outn (unsigned long v)
{
    char b[12];

    out (ultoa (v, b, 10));
}

/* Its name and occupation, its birthday: the sheet's lines (and a theme file's name) */
static char who[20], job[24], tn[44];
static unsigned bday;

/* The lines of a file, each given to f, till it says enough (the first 4K at most) */
static void __fastcall__ lines_of (unsigned char (*f) (char* l))
{
    static char b[100], l[80];
    int fd, n, i;
    unsigned char k = 0;
    unsigned total = 0;

    if ((fd = open (path, O_RDONLY)) < 0) {
        return;
    }
    while (total < 4096 && (n = read (fd, b, sizeof b)) > 0) {
        total += n;
        for (i = 0; i < n; ++i) {
            if (b[i] == '\n' || b[i] == '\r') {
                l[k] = 0;
                k = 0;
                if (f (l)) {
                    close (fd);
                    return;
                }
            } else if (k < sizeof l - 1) {
                l[k++] = b[i];
            }
        }
    }
    close (fd);
}

static unsigned char __fastcall__ sheet_line (char* l)
{
    if (!strncmp (l, "name ", 5)) {
        strncpy (who, l + 5, sizeof who - 1);
    } else if (!strncmp (l, "occupation ", 11)) {
        strncpy (job, l + 11, sizeof job - 1);
    } else if (!strncmp (l, "att bd ", 7)) {
        bday = (unsigned) num (l + 7);
    }
    return 0;
}

static unsigned char __fastcall__ theme_line (char* l)
{
    char* p;

    if (strncmp (l, "theme", 5) || !(p = strchr (l, '|'))) {
        return 0;
    }
    for (++p; *p == ' '; ++p) {
    }
    strncpy (tn, p, sizeof tn - 1);
    return 1;
}

/* The account told; over, the life's ended (else the life so far) */
static void __fastcall__ tell (unsigned char over)
{
    unsigned n, k, last;
    unsigned long days = 0;
    const struct life_dungeon* d;

    in_ (top, "pc.hm");
    lines_of (sheet_line);
    out ("\n");
    out (over ? "The life of " : "The life so far of ");
    out (who[0] ? who : "your character");
    if (job[0]) {
        out (", ");
        out (job);
    }
    out (", born in the year ");
    outn (L.h.born);
    out (":\n");
    n = L.h.dungeon > LIFE_DUNGEONS ? LIFE_DUNGEONS : L.h.dungeon;
    for (k = last = 0; k < n; ++k) {
        d = &L.d[k];
        if (d->outcome == LO_NONE) {
            continue;
        }
        ++last;
        days += d->days;
        out ("  ");
        tn[0] = 0;
        dungeon_dir (k + 1);
        in_ (dir, "theme.txt");
        lines_of (theme_line);
        if (tn[0]) {
            out (tn);
        } else {
            out ("Dungeon ");
            outn (k + 1);
        }
        out (d->outcome == LO_SOLVED ? (d->flags & LD_DIED ? ": solved, and died on the way out, " : ": solved, ") :
            d->outcome == LO_LEFT ? ": left unsolved, " : ": died in it, ");
        outn (d->days);
        out (d->days == 1 ? " day\n" : " days\n");
    }
    if (!last) {
        out ("  No dungeon finished yet.\n");
    }
    outn (last);
    out (last == 1 ? " dungeon, " : " dungeons, ");
    outn (days);
    out (days == 1 ? " day in them. " : " days in them. ");
    if (over) {
        out (L.h.flags & LF_ABROAD ? "Dead between dungeons, " : "Dead, ");
        out ("on day ");
        outn (LIFE_DAY (L.h.date));
        out (" of month ");
        outn (LIFE_MONTH (L.h.date));
        out (", ");
        outn (LIFE_YEAR (L.h.date));
        out (", aged ");
        outn (LIFE_AGE (&L.h, L.h.date, bday));
        out (".\n");
    } else {
        out ("It goes on from your last save.\n");
    }
}

int main (int argc, char* argv[])
{
    unsigned char i, hard = 0;
    unsigned n;
    unsigned long t = 0;
    int fd;

    for (i = 1; i < argc; ++i) {
        if (!strcmp (argv[i], "-d") && i + 1 < argc) {
            top = argv[++i];
        } else if (!strcmp (argv[i], "-h")) {
            hard = 1;
        }
    }
    if (!top) {
        return 1;
    }
    if ((fd = open (in_ (top, LIFE_FILE), O_RDONLY)) < 0 || read (fd, &L, sizeof L) != sizeof L || L.h.magic != LIFE_MAGIC ||
        L.h.version != LIFE_VER) {
        return 1;
    }
    close (fd);
    if (hard) {
        n = L.h.dungeon;
        dungeon_dir (n);
        if ((fd = open (in_ (dir, "game.dat"), O_RDONLY)) >= 0) {
            read (fd, &sv, sizeof sv);
            close (fd);
        }
        if ((fd = open (in_ (dir, "world.dat"), O_RDONLY)) >= 0) {
            if (read (fd, &w, sizeof w) == sizeof w && w.magic == WSAVE_MAGIC) {
                t = w.now;
            }
            close (fd);
        }
        if (n <= LIFE_DUNGEONS) {
            L.d[n - 1].days = (unsigned) (t / TENTHS_DAY);
            L.d[n - 1].outcome = sv.outcome == LO_SOLVED ? LO_SOLVED : LO_DIED;
            L.d[n - 1].flags |= LD_DIED;
        }
        L.h.date += (t + ((unsigned) L.h.dseed % 24) * 36000UL) / TENTHS_DAY;
        L.h.flags |= LF_OVER;
        record_write ();
    }
    tell (L.h.flags & LF_OVER);
    return 0;
}

