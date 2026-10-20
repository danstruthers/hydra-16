/*
** themec.c - Dingus Quest Zero's theme compiler (Phase 3D): a theme (its file, in the format ../dq0/themes/
** 1crypt.txt has: one written by hand, or themegen's) compiled into its record (comp.c: theme.h's struct theme and
** its words, as ../dq0/make.js's compile makes it; the tests hold the two to the same bytes) and checked (the theme
** checker: every goal reachable, every faction placed).  themegen runs it in its own place (exec) to make a game's
** theme.dat from its theme.txt; from the shell, a hand-written theme is tried with it.
**
** themec [-o record] [-l lib] [-q] theme
**   -o record  the record written there (a game's theme.dat); without it, the theme only checked
**   -l lib     the game's lib folder (bestiary.dat: the bestiary, themegen's make.js's): default, its own folder's
**              lib/dq0
**   -q         nothing said but its faults
** Its exit status 0, or 1 (its faults said: a line's, the checker's).
*/

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "comp.h"

static char path[64];
static const char* __fastcall__ in (const char* dir, const char* name)
{
    strcpy (path, dir);
    strcat (path, "/");
    strcat (path, name);
    return path;
}

int main (int argc, char* argv[])
{
    static struct line l;
    static char t[LINE_TEXT], lib[48];
    const char* out = 0;
    const char* file = 0;
    const char* prog = getenv ("prog");
    const char* e;
    char* slash;
    unsigned char i, quiet = 0, bad = 0;
    int fd;

    for (i = 1; i < argc; ++i) {
        if (i + 1 < argc && !strcmp (argv[i], "-o")) {
            out = argv[++i];
        } else if (i + 1 < argc && !strcmp (argv[i], "-l")) {
            strncpy (lib, argv[++i], sizeof lib - 1);
        } else if (!strcmp (argv[i], "-q")) {
            quiet = 1;
        } else if (!file && argv[i][0] != '-') {
            file = argv[i];
        } else {
            file = 0;
            break;
        }
    }
    if (!file) {
        say ("usage: themec [-o record] [-l lib] [-q] theme\n");
        return 1;
    }
    if (!*lib) {                                /* (Its own folder's lib/dq0, else the card's) */
        strcpy (lib, "/sd/0/games/lib/dq0");
        if (prog && strlen (prog) < 30 && (slash = strrchr (strcpy (path, prog), '/')) != 0) {
            strcpy (slash, "/lib/dq0");
            strcpy (lib, path);
        }
    }
    if (tg_load (in (lib, "bestiary.dat"))) {
        say ("themec: its bestiary (lib/dq0/bestiary.dat) not found\n");
        return 1;
    }
    bestiary ();
    if (lines_open (file)) {
        say ("themec: ");
        say (file);
        say (": not found\n");
        return 1;
    }
    comp_begin ();
    while (read_line (&l, t)) {
        if ((e = comp_line (&l)) != 0) {
            say ("themec: ");
            say (file);
            say (": ");
            say (l.key);
            say (": ");
            say (e);
            say ("\n");
            bad = 1;
        }
    }
    lines_close ();
    if ((e = comp_end ()) != 0) {
        say ("themec: ");
        say (file);
        say (": ");
        say (e);
        say ("\n");
        bad = 1;
    }
    if (comp_check ()) {
        bad = 1;
    }
    if (out && !bad) {
        if ((fd = open (out, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0 || write (fd, R, THEME_SIZE) != THEME_SIZE) {
            say ("themec: ");
            say (out);
            say (": can't be written\n");
            return 1;
        }
        close (fd);
    }
    if (!quiet && !bad) {
        write (1, (char*) R + 4, strlen ((char*) R + 4));
        write (1, ": ", 2);
        write (1, num (comp_used () - THEME_HEAD), strlen (num (comp_used () - THEME_HEAD)));
        write (1, " bytes of words, its checks passed\n", 35);
    }
    return bad;
}
