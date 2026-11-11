/*
** intro.c - (overlay 1) Dingus Quest Zero's title, and its pace asked; harm's words (digging's, climbing's and a
** door's, said.c's and said.txt's since 2026-11-11: narration as data).  The character is made by hmchar (main.c
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

/* The character come into the dungeon (or back to it: a game carried on) */
void __fastcall__ o_entered (unsigned char resumed)
{
    sayf (resumed ? "%s, %s, takes up where they left off." : "%s, %s, enters the dungeon.", P.name, P.occupation);
}


/* Harm's words: what came of it (w: an E_HARM's, its cause and the worst wound's band; a fall into water, E_DEEP,
** E_SHALLOW), the referee's words of the wounds (in lb), and what they've left the character in (cond: its conditions,
** rs.h's C_*).  Overlay 12's since 2026-11-06 (overlay 1's room, the water's words) */
#pragma code-name (push, "OV12CODE")
#pragma rodata-name (push, "OV12RODATA")
#pragma local-strings (push, on)
void __fastcall__ o_harm_words (unsigned w, unsigned cond)
{
    static const char* const self[3] = {        /* (Blows at a thing gone wrong: by the worst wound's band) */
        " You hurt yourself on it: ", " Something gives as you strike it: ", " You are crushed against it: "
    };
    unsigned char band = w >> 8 & 7;

    if ((w & 0xFF) == HC_STRUCK) {
        if (band) {
            say (self[band > 3 ? 2 : band - 1]);
            say (lb);
            say (".");
        }
        if (w & E_STOPPED) {
            say (" You stop: you are too hurt to go on.");
        }
    } else if (w & E_CAUGHT) {
        say (" You snatch at a hold as you go, and it holds: you hang there, and climb on.");
        return;
    } else if (!band) {
        say ((w & 0xFF) != HC_FALL ? " It does you no harm." : w & E_DEEP ? " You plunge into the water, shaken, but unhurt." :
            w & E_SHALLOW ? " You come down in the shallows with a splash, shaken, but unhurt." : " You land, shaken, but unhurt.");
        return;
    } else {
        say ((w & 0xFF) != HC_FALL ? " It hurts you: " : w & E_DEEP ? " You plunge into the water: " :
            w & E_SHALLOW ? " You come down in the shallows with a splash: " : " You land hard: ");
        say (lb);
        say (".");
    }
    if (cond & C_DEAD) {
        msg ("You are dead.");
        return;
    }
    if (w & E_OUT) {
        say (" Everything goes black; you come to some time later.");
    }
    state_words ((unsigned char) cond);         /* (Its state: task.c's) */
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)
