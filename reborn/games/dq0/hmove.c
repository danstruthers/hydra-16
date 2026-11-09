/*
** hmove.c - moving (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command handlers by meaning, cmds.h): a way
** gone, a thing gone up to or through (a door, a passage, a window climbed through), a place in the room, up or down
** the stairs; a turn; a jump.  Part 2's, the commands' own: each verb's work as it was in main.c's s_command (part
** 2's) and rarer (part 24's, the jump).  (A place it knows gone to, "go back to the Mausoleum", is s_command's, before
** the table: part 4's travel_go)
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "level.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* H_GO: a way, a thing gone up to or through, a place in the room */
static void do_go (void)
{
    unsigned char d;

    if ((cmd.dir == GO_DOWN || cmd.action == VB_ENTER) && !strcmp (cmd.obj.word, "well")) {
        climb (H_CLIMB);                        /* ("Go down the well", "enter the well": climbed down) */
        return;
    }
    d = cmd.obj.kind;
    if ((d == OB_WALL || (d >= OB_ROOM && d != OB_PILLAR)) && place_go (&cmd)) {
        return;                                 /* (A place in the room: the centre, a side, a corner: part 7's) */
    }
    if (cmd.obj.kind && cmd.action != VB_MOVE_DIRECTION) {   /* (A thing: up to it, or through it) */
        d = cmd.how == HOW_THROUGH || cmd.action == VB_ENTER;
        if (d && cmd.obj.kind == OB_WINDOW) {
            if (pick (OB_WINDOW | PICK_TELL, cmd.obj.lies) == 1) {
                portal (PA_THROUGH);            /* (Climbed through; or none, told; or which, asked) */
            }
            return;
        }
        d = d && (cmd.obj.kind == OB_DOOR || cmd.obj.kind == OB_PASSAGE);    /* (Through it: walked up to first,
                                                **   its first words told as that began; else up to it, its own walk) */
        if (feature (cmd.obj.kind | PICK_TELL, d ? (reach ? FG_GO | FW_NONE : FG_GO) : FG_GO | FG_FAR)) {
            if (d) {
                intend (I_GO, cmd.obj.lies);
            } else {
                intend2 (I_GOTO, tx, ty);
            }
        }
    } else if (cmd.dir == GO_UP || cmd.dir == GO_DOWN) {
        intend (I_CLIMB, cmd.dir == GO_UP ? DIR_UP : DIR_DOWN);    /* (Up or down: the stairs, a ladder) */
    } else if (cmd.dir != 0xFF) {
        intend (I_GO, cmd.dir);
    } else {
        missiles ();                            /* (Anything else it has seen, or which way: way.c's go_thing,
                                                **   part 13's, the action a move: no stub of its own, the main RAM's) */
    }
}

/* Moving, by its handler's number (H_GO ... H_JUMP) */
void __fastcall__ s_moving_do (unsigned h)
{
    switch ((unsigned char) h) {
    case H_GO:
        do_go ();
        break;
    case H_FACE:                                /* (A turn) */
        if (cmd.dir > H_NW) {
            tell (T_WHICH_WAY_LEFT);
        } else if (cmd.dir == HUB->pc_face) {
            sayf ("\nYou face %s already.", head_name[believed (cmd.dir)]);
        } else {
            intend (I_TURN, cmd.dir);
        }
        break;
    default:                                    /* (H_JUMP: down a ledge, or a shaft, a fall; into a well: the climb's
                                                **   work, hplaces.c's) */
        if (cmd.dir != GO_DOWN && strcmp (cmd.obj.word, "well")) {
            tell (T_YOU_JUMP_AND);
        } else {
            climb (H_JUMP);
        }
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
