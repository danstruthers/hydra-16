/*
** fixt.c - (overlay 4) the words of the dungeon's fixtures (DUNGEON.md: portals; traps, pits and other nasty
** surprises): what came of working a portcullis, a window or a slit (an E_PORTAL's: a door's are intro.c's), what a
** trap did, or what came of finding or disarming one (an E_TRAP's), and a trap's name (words.c's too)
*/

#pragma code-name ("OV4CODE")
#pragma rodata-name ("OV4RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"

/* A trap's name, added to lb: "a scything blade, set off by a tripwire" (kind, trigger, state: world.h's TK_*,
** TT_*, TS_*: a pit that's given way an open one) */
void __fastcall__ o_trap_name (unsigned char kind, unsigned char trigger, unsigned char state)
{
    static const char* const name[8] = {
        "an open pit", "a covered pit", "a covered pit, spikes in it", "a deadfall", "a scything blade",
        "a dart trap", "a gas trap", "an alarm"
    };

    if (kind <= TK_SPIKED) {
        strcat (lb, kind == TK_SPIKED ? (state & TS_SPRUNG ? "a pit, spikes in it" : name[kind]) :
            state & TS_SPRUNG ? name[TK_PIT] : name[kind]);
        return;
    }
    strcat (lb, name[kind & 7]);
    strcat (lb, trigger == TT_PLATE ? ", set off by a plate in the floor" : trigger == TT_WIRE ?
        ", set off by a tripwire" : ", set to go off as its door opens");
}

/* What a trap did, or what came of it (w: an E_TRAP's) */
void __fastcall__ o_trap_words (unsigned w)
{
    static const char* const set_off[4] = {
        "A plate sinks under your foot with a click.", "A wire parts against your shin.",
        "As the door opens, something clicks.", ""
    };
    static const char* const does[8] = {
        "", "", "", " Stones crash down from above!", " A blade scythes out of the wall!",
        " Darts hiss out of the wall!", " A cloud of choking gas billows up!",
        " Somewhere near, a bell clangs, loud and long."
    };
    unsigned char kind = w & 0x0F, trigger = w >> 4 & 3, o = w >> 8;

    lb[0] = 0;
    switch (o) {
    case TO_SPRUNG:
    case TO_DODGED:
    case TO_RESISTED:
    case TO_CHOKED:
        if (kind <= TK_SPIKED) {
            msg (kind == TK_SPIKED ? "The floor gives way beneath you: a pit, spikes at its foot!" :
                "The floor gives way beneath you: a pit!");
            return;
        }
        strcpy (lb, set_off[trigger]);
        strcat (lb, does[kind] + (trigger == 3));  /* (Set off as it was being disarmed: no trigger's words) */
        strcat (lb, o == TO_DODGED ? " You throw yourself clear." : o == TO_RESISTED ?
            " You hold your breath and stagger clear of it." : o == TO_CHOKED ?
            " You choke on it; everything goes black, and you come to some time later." : "");
        break;
    case TO_OPENED:
        strcpy (lb, "The pit's cover falls in.");
        break;
    case TO_NOTICED:
    case TO_FOUND:
        strcpy (lb, o == TO_NOTICED ? "You stop short: just ahead is a trap, " : "You find a trap: ");
        trap_name (kind, trigger, 0);
        strcat (lb, ".");
        break;
    case TO_DISARMED:
        strcpy (lb, "You work at the trap a while, and disarm it.");
        break;
    case TO_NOT_YET:
        strcpy (lb, "You work at the trap a while, but it defeats you, for now.");
        break;
    case TO_FUMBLED:
        strcpy (lb, "Your hand slips: the trap goes off!");
        break;
    case TO_ASSESSED:
        strcpy (lb, "You study the mechanism a while, and make out how it works: now to disarm it.");
        break;
    case TO_PUZZLED:
        strcpy (lb, "You study the mechanism a while, but can't make out how it works.");
        break;
    case TO_CANT:
        strcpy (lb, "It's set by magic: you've no means to undo it.");
        break;
    case TO_PIT:
        strcpy (lb, "There's no disarming a pit: you'll have to go round it.");
        break;
    default:
        strcpy (lb, "There's no trap you know of within reach.");
        break;
    }
    msg (lb);
}

/* A denizen in the character's way (an E_NOWAY's N_DEN: its name in lb): "A giant rat is in the way." */
void o_den_way (void)
{
    static char b[32];

    strcpy (b, strchr ("aeiou", lb[0]) ? "An " : "A ");
    strcat (b, lb);
    strcat (b, " is in the way.");
    msg (b);
}

/* What came of working a portal (w: an E_PORTAL's, its kind times 256): a portcullis's, a window's or a slit's
** words, or a door's (intro.c's) */
void __fastcall__ o_fixt_words (unsigned w)
{
    static const char* const said[PO_FOUND_TRAP - PO_RAISED] = {
        "You wind the winch beside it, and the portcullis grinds up.",
        "You heave the portcullis up by its bars, and it catches, raised.",
        "You loose the winch: the portcullis crashes down.",
        "The portcullis is raised already.", "The portcullis is down already.",
        "Its winch is on the other side: you can't lower it from here.",
        "You heave at the portcullis, but it won't rise.", "That isn't a thing raised or lowered.",
        "It has no shutters to open or close.",
        "You strain at the bars, and bend them apart: there's room to get through.", "The bars won't give.",
        "The glass shatters with a crash.", "The shutters splinter and give way.", "There's nothing in it to break.",
        "You can't climb through: it's shut, glazed or barred.",
        "An arrow slit: only light and arrows pass it, and it's too narrow to climb through.",
        "You climb up onto the sill, and through.", "You can't get up onto the sill.",
        "You slip from the sill, and fall.", "That isn't a window: walk through it.",
        "There's a trap on that door you know of: disarm it before you open it.",
    };
    static const char* const shutters[PO_HELD + 1] = {
        "There's no window that way.", "You open the shutters.", "You close the shutters.",
        "The shutters stand open already.", "The shutters are shut already.", "The shutters are locked.",
        "The shutters are barred from the other side.", "You lift the bar and open the shutters.", 0, 0, 0,
        "There's no lock on them to pick.", "With a crack the shutters burst open.", "It holds."
    };
    unsigned char out = w & 0xFF, kind = w >> 8;

    if (out >= PO_RAISED) {
        if (out < PO_FOUND_TRAP) {
            msg (out == PO_NOT_PORTC && kind == PK_PORTCULLIS ? "It's a portcullis: it's raised or lowered." :
                said[out - PO_RAISED]);
        }
        return;
    }
    if ((kind == PK_WINDOW || kind == PK_SLIT) && out <= PO_HELD && shutters[out]) {
        msg (shutters[out]);
        return;
    }
    portal_words (out);
}
