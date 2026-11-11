/*
** intro.c - (overlay 1) Dingus Quest Zero's title, and its pace asked (digging's, climbing's, a door's and harm's
** words said.c's and said.txt's since 2026-11-11: narration as data).  The character is made by hmchar (main.c runs
** it), and its file read by the referee.
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"

void o_title (void)
{
    blank ();
    msg ("D I N G U S   Q U E S T   Z E R O");
    blank ();
    msg ("A dungeon, made new for each descent, and a puzzle in pieces somewhere in it: find them, and you may yet "
         "leave the place, or rule it.  The dungeon doesn't wait while you think.");
    blank ();
    if (pace == 0xFF) {
        msg ("How fast does the dungeon move?  (r)elaxed, (n)ormal or (m)enacing: ");
        pace = ask ("rnm");
        say (pacename[pace]);
    }
    blank ();
}

/* The character come into the dungeon (or back to it: a game carried on) */
void __fastcall__ o_entered (unsigned char resumed)
{
    sayf (resumed ? "%s, %s, takes up where they left off." : "%s, %s, enters the dungeon.", P.name, P.occupation);
}

