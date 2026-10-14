/*
** intro.c - (overlay 1) Dingus Quest Zero's title, and its pace asked; the commands' list (help); digging's words.  The character is made by hmchar (main.c
** runs it), and its file read by the referee.
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

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

/* The commands, listed */
void o_help (void)
{
    msg ("Commands: north, northeast, east ... northwest (n, ne, e, se, s, sw, w, nw, or go north ...); forward, "
         "back, left, right (f, b: as you face); up and down (u, d: the stairs); turn left, right, half left, half "
         "right or around; face north ...; light and douse (your torch); dig north ... or down (a wall, the rock, a "
         "shaft); look (l), map (m), sheet (the character), wait (z), pause (or Escape), quit (q).");
}

/* ---- Digging's words: how a spell of work went, and the feet to go */

void __fastcall__ o_dig_words (unsigned char how, unsigned char feet)
{
    switch (how) {
    case D_BEGUN:
        sayf ("\nYou set to digging: some %u feet of it, by the look of it.", feet);
        return;
    case D_GOOD:
        msg ("The work goes well.");
        break;
    case D_POOR:
        msg ("The work goes slowly.");
        break;
    case D_SLIP:
        msg ("Your tool slips and turns; little comes of that spell of work.");
        break;
    case D_THROUGH:
        msg (feet ? "The last of the rock gives way beneath you: the shaft is through." : "You break through.");
        return;
    default:
        msg ("Beneath the shaft is only solid rock, with nothing under it to break into.");
        return;
    }
    sayf (feet == 1 ? " (A foot to go.)" : " (%u feet to go.)", feet);
}
