/*
** intro.c - (overlay 1) Dingus Quest Zero's title, and its pace asked; digging's, climbing's
** and harm's words.  The character is made by hmchar (main.c
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

#pragma code-name (push, "OV12CODE")            /* (Overlay 12's: overlay 1's room, slice 4.4's; its strings with it) */
#pragma rodata-name (push, "OV12RODATA")
#pragma local-strings (push, on)

/* Climbing's words: how a round went, and the feet to go (and a ledge come to: CW_RISES, CW_FALLS, its feet) */
void __fastcall__ o_climb_words (unsigned char how, unsigned char feet)
{
    switch (how) {
    case CW_RISES:
    case CW_FALLS:
        if (feet > LEDGE_MET) {                 /* (What a straight walk came to, E_LEDGE's: dq0w.c's across) */
            static const char met[][24] = { "the wall", "a closed door", "a portcullis, down", "a drop", "a rise",
                "the edge of a pit", "a trap you know of" };
            sayf ("\nYou come to %s.", met[feet - LEDGE_MET - 1]);
            return;
        }
        sayf (how == CW_RISES ? "\nThe floor rises %u feet at a ledge ahead: climb it (u) to go on." :
              "\nThe floor falls %u feet at a ledge ahead: climb down it (d) to go on.", feet);
        return;
    case K_BEGUN:
        sayf ("\nYou set to climbing: %u feet of it.", feet);
        return;
    case K_ON:
        sayf ("\nYou climb on (%u feet to go).", feet);
        return;
    case K_BACK:
        sayf ("\nYou slip back a little (%u feet to go).", feet);
        return;
    case K_NOHOLD:
        sayf ("\nYou find no hold yet (%u feet to go).", feet);
        return;
    case K_SLID:                                /* (Out of a pit: the climb ended, D13's) */
        sayf ("\nYou slip, and slide back down to the pit's floor: %u feet to climb (climb up to try again).", feet);
        return;
    case K_FELL:
        if (!feet) {
            msg ("Your hold gives way before you're off the ground.");
            return;
        }
        sayf ("\nYour hold gives way: you fall %u feet.", feet);
        return;
    case K_JUMPED:
        sayf ("\nYou jump, and drop %u feet.", feet);
        return;
    default:
        msg (how == K_UP ? "You haul yourself up over the edge." : "You come down to the foot of the ledge.");
        return;
    }
}

#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* Harm's words: what came of it (w: an E_HARM's, its cause and the worst wound's band), the referee's words of the
** wounds (in lb), and what they've left the character in (cond: its conditions, rs.h's C_*) */
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
        say ((w & 0xFF) == HC_FALL ? " You land, shaken, but unhurt." : " It does you no harm.");
        return;
    } else {
        say ((w & 0xFF) == HC_FALL ? " You land hard: " : " It hurts you: ");
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

/* What came of working a door, into lb (fixt.c's, to say: what it shows told with it, an opening's; overlay 10's,
** 2026-10-22: this one's room) */
#pragma code-name (push, "OV10CODE")
#pragma rodata-name (push, "OV10RODATA")
#pragma local-strings (push, on)
void __fastcall__ o_portal_words (unsigned char out)
{
    static const char* const said[] = {
        "There's no door that way.", "You open the door.", "You close the door.", "It stands open already.",
        "It's shut already.", "It won't open: it's locked.", "It won't give: it's barred from the other side.",
        "You lift the bar and open the door.", "The lock gives with a click.", "The lock defeats you, for now.",
        "You've nothing to pick a lock with.", "There's no lock on it to pick.",
        "With a crack the door bursts open.", "The door holds.",
        "You find a door hidden in the wall.", "You find nothing hidden."   /* (Told as a search ends: "Searching
                                                **   about..." as it began, HOUSE.md's things told as found) */
    };

    strcpy (lb, said[out]);
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Digging's words: how a spell of work went, and the feet to go */

static unsigned char dig_feet;                  /* (The feet to go last told) */
void __fastcall__ o_dig_words (unsigned char how, unsigned char feet)
{
    const char* s;

    switch (how) {
    case D_BEGUN:
        dig_feet = 0;
        sayf ("\nYou set to digging: some %u feet of it, by the look of it.", feet);
        return;
    case D_AGAIN:                               /* (A hole dug part way before: on from there) */
        dig_feet = 0;
        msg ("You set to digging again where you left off.");
        break;
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
    case D_NO_TOOL:                             /* (Told why, for what it can judge: HOUSE.md's, 2026-11-02; feet what it's
                                                **   into, MAT_* + 1) */
        if (feet & DN_HANDS) {                  /* (The user's, 2026-11-03: "hammer and chisel in each hand") */
            msg (feet & DN_LIT ? "You need the chisel in one hand and the hammer in the other, and that leaves no hand for your light: set it down first." :
                "You need the chisel in one hand and the hammer in the other.");
            return;
        }
        hub_mem (UI_BANK, lb + 200, UIREC->things_text, 180);   /* (Not hub_map: an overlay runs in the window) */
        lb[380] = 0;
        --feet;
        strcpy (lb + 180, feet == MAT_EARTH ? "hard-packed earth" : feet == MAT_MASONRY ? "masonry" : feet == MAT_FLAGS ?
            "flagstones" : feet == MAT_SOIL ? "earth" : "stone");
        s = strstr (lb + 200, "shovel") ? "shovel" : strstr (lb + 200, "spade") ? "spade" : strstr (lb + 200, "mattock") ?
            "mattock" : strstr (lb + 200, "blade") ? "blade" : strstr (lb + 200, "dagger") ?
            "dagger" : strstr (lb + 200, "knife") ? "knife" : "";
        if (feet != MAT_EARTH && feet != MAT_SOIL && !strstr (lb + 200, "chisel") && strstr (lb + 200, "woodchi")) {
            sayf ("\nYour wood chisel doesn't seem to be working on the %s.", lb + 180);
        } else if (feet != MAT_EARTH && feet != MAT_SOIL && strstr (lb + 200, "chisel")) {
            msg (strstr (lb + 200, "hammer") ? "A chisel and a hammer take both hands." :
                "You'd need a hammer to drive the chisel.");
        } else if (*s) {
            strcpy (lb + 200, s);
            sayf ("\nYour %s only scratches at the %s: this isn't getting anywhere.", lb + 200, lb + 180);
        } else {
            sayf ("\nYou need better tools: your hands make no mark on the %s.", lb + 180);
        }
        return;
    case D_FOUND:                               /* (A hidden door dug to, its stuff, world.h's PM_*: HOUSE.md's) */
        sayf ("\nYour %s strikes %s: a door, behind the facing.", dug_with, feet == 0 ? "wood" :
            feet == 1 ? "iron-bound oak" : feet == 2 ? "iron" : "stone");
        return;
    case D_PORTAL:                              /* (A portal in sight's no thing to dig: its kind, world.h's PK_*;
                                                **   0xFE, two spells with nothing to show, asked after it) */
        msg (feet == 0xFE ? "This isn't getting anywhere." : feet == 0 || feet == 6 ? "That's a door: open it, or break it down." :
            feet == 1 || feet == 7 ? "That's a window: there's no digging it." :
            "Those are iron bars: they won't dig.");
        return;
    default:
        msg ("Beneath the shaft is only solid rock, with nothing under it to break into.");
        return;
    }
    if (feet != dig_feet) {                     /* (Told only as it changes: a slow tool's half-feet) */
        sayf (feet == 1 ? " (A foot to go.)" : " (%u feet to go.)", feet);
    }
    dig_feet = feet;
}
