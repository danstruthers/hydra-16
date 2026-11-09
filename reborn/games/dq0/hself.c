/*
** hself.c - the body's commands and the meta commands (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command
** handlers by meaning, cmds.h): the hands and the limbs (swap hands, sit, stand, lie down, crouch), a feel about in
** the dark, a look about, a wait or a sleep, a long task stopped, first aid, a dig; and what's carried, the sheet and
** the map, the time, pause and save, help, quit.  Part 2's, the commands' own: each verb's work as it was in main.c's
** s_command (part 2's), rarer (part 24's: the dig) and words.c's more (overlay 16's: the time, first aid).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* The body's and the meta commands, by their handler's number (H_NOTHING ... H_QUIT) */
void __fastcall__ s_self_do (unsigned h)
{
    unsigned char d;

    switch ((unsigned char) h) {
    case H_LIMBS:                               /* (The hands' and limbs': part 11's, slice 4.1b's; the body's position,
                                                **   slice 4.6's) */
        limbs_cmd ();
        break;
    case H_FEEL:                                /* (A look about, or a search for a thing; in the dark, or blind, the
                                                **   character free, its hands: what's told at once, then the feel around
                                                **   (search.c's: the user's, 2026-11-03) */
        if (!busy && !(HUB->pcsum.cond & C_STUNNED) && sight_get ()->dark) {
            if (cmd.action == VB_OBSERVE_LOCALE) {
                look (0);
            }
            cmd.aux = SR_FOR;
            search_go ();
            break;
        }
    case H_LOOK:
        look (0);
        break;
    case H_WAIT:                                /* (A round, or as long as it says: "wait 10 minutes"; engaged, "pass": a
                                                **   round given up, defending; a sleep or a rest, a night's or as long as
                                                **   it says: task.c's) */
        wait_go (cmd.action);
        break;
    case H_STOP:                                /* (The long task: "stop", "stop digging") */
        intend (I_STOP, stop_kind (cmd.obj.word));
        break;
    case H_TEND:                                /* (First aid: part 12's, slice 4.3's) */
        tend ();
        break;
    case H_DIG:                                 /* (A wall, the rock, a shaft: the world's, a heading 8 on, a diagonal
                                                **   too) */
        d = cmd.dir;
        if (d != GO_DOWN && d > H_NW) {
            tell (T_DIG_WHICH_WAY);
            break;
        }
        walk_says ((const char*) W_DIG);
        intend (I_DIG, d == GO_DOWN ? DIR_DOWN : d + 8);
        break;
    case H_INVENTORY:                           /* (What's carried, and its load) */
        list_say (1);
        things (IO_LIST, "");
        list_say (0);
        if ((d = HUB->pcsum.load) != 0) {
            tell (T_BURDENED - 1 + d);          /* (Its load, as the referee has it: its encumbrance) */
        }
        break;
    case H_SHEET:                               /* (The map, or the sheet: part 3's) */
        sheet_map (cmd.action == VB_META_MAP);
        break;
    case H_TIME:                                /* (The time it believes: time.c's, 2026-10-31) */
        time_ui (TU_TELL);
        break;
    case H_META:                                /* (Paused, or kept: overlay 3's) */
        meta (cmd.action);
        break;
    case H_HELP:
        help ();
        break;
    case H_QUIT:
        quit = 1;                               /* (H_NOTHING: nothing) */
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
