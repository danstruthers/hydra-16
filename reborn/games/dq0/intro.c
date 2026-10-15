/*
** intro.c - (overlay 1) Dingus Quest Zero's title, and its pace asked; the commands' list (help); digging's words.  The character is made by hmchar (main.c
** runs it), and its file read by the referee.
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

/* Climbing's words: how a round went, and the feet to go */
void __fastcall__ o_climb_words (unsigned char how, unsigned char feet)
{
    switch (how) {
    case K_BEGUN:
        sayf ("\nYou set to climbing: %u feet of it.", feet);
        return;
    case K_ON:
        sayf ("\nYou climb on (%u feet to go).", feet);
        return;
    case K_BACK:
        sayf ("\nYou slip back a little (%u feet to go).", feet);
        return;
    case K_FELL:
        sayf ("\nYour hold gives way: you fall %u feet.", feet);
        return;
    default:
        msg (how == K_UP ? "You haul yourself up over the edge." : "You come down to the foot of the ledge.");
        return;
    }
}

/* What came of working a door */
void __fastcall__ o_portal_words (unsigned char out)
{
    static const char* const said[] = {
        "There's no door that way.", "You open the door.", "You close the door.", "It stands open already.",
        "It's shut already.", "It won't open: it's locked.", "It won't give: it's barred from the other side.",
        "You lift the bar and open the door.", "The lock gives with a click.", "The lock defeats you, for now.",
        "You've nothing to pick a lock with.", "There's no lock on it to pick.",
        "With a crack the door bursts open.", "The door holds.",
        "Searching the walls, you find a door hidden in them.", "You search about, and find nothing hidden."
    };

    msg (said[out]);
}

/* The commands, listed */
void o_help (void)
{
    msg ("Commands: north, northeast, east ... northwest (n, ne, e, se, s, sw, w, nw, or go north ...); forward, "
         "back, left, right (f, b: as you face); up and down (u, d: the stairs); turn left, right, half left, half "
         "right or around; face north ...; light and douse (your torch); dig north ... or down (a wall, the rock, a "
         "shaft); open, close, unlock, force (the door ahead), search; look (l), map (m), sheet (the character), wait (z), pause (or Escape), quit (q).");
}

/* ---- Digging's words: how a spell of work went, and the feet to go */

void __fastcall__ o_dig_words (unsigned char how, unsigned char feet)
{
    const char* s;

    switch (how) {
    case D_BEGUN:
        sayf ("\nYou set to digging: some %u feet of it, by the look of it.", feet);
        return;
    case D_GOOD:
    case D_POOR:
    case D_SLIP:
        s = how == D_GOOD ? "The work goes well." : how == D_POOR ? "The work goes slowly." :
            "Your tool slips and turns; little comes of that spell of work.";
        if (dig_with[0]) {                      /* (The tool it took, named the first time) */
            sayf (strcmp (dig_with, "hands") ? "\nYou dig with your %s. %s" : "\nYou dig with your bare %s. %s", dig_with, s);
            dig_with[0] = 0;
        } else {
            msg (s);
        }
        break;
    case D_THROUGH:
        msg (feet ? "The last of the rock gives way beneath you: the shaft is through." : "You break through.");
        return;
    case D_NO_TOOL:
        msg ("You have nothing that would bite into it: that wants a pick, or a hammer and chisel.");
        return;
    default:
        msg ("Beneath the shaft is only solid rock, with nothing under it to break into.");
        return;
    }
    sayf (feet == 1 ? " (A foot to go.)" : " (%u feet to go.)", feet);
}
