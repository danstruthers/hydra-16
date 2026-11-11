/*
** sheets.c - (the referee skeleton's, ../referee.h's) the creatures' sheets' file: the save folder's beasts.dat (the
** hub's savedir), kept with the game's other records, opened for the ruleset's rf_sheets.  A file of its own so each
** referee puts it where its room is (dq0harn's swapped part 2, beside its rf_sheets; dq0null's main RAM).
*/

#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "../hub.h"
#include "../rs.h"
#include "../referee.h"

/* beasts.dat opened to read (back 1) or written anew (0): its fd, or -1 (none there, or no save folder: dq0 -d's) */
int __fastcall__ sheets_open (unsigned char back)
{
    static char path[56];

    if (!HUB->savedir[0]) {
        return -1;
    }
    strcpy (path, HUB->savedir);
    strcat (path, "/beasts.dat");
    return back ? open (path, O_RDONLY) : open (path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}
