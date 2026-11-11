/*
** hplaces.c - the features' and the doors' commands (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command
** handlers by meaning, cmds.h): a door's, a window's or a portcullis's work (open, close, unlock, lock, break, raise,
** lower, bend, force, disarm, bar), a climb (up or down, a well's shaft, through a window), a thing or a feature
** examined, a search, a look, a listen or a sniff one way (and the way out left).  Part 2's, the commands' own: each
** verb's work as it was in main.c's s_command (part 2's) and rarer (part 24's, the climb); every feature found,
** walked up to and faced the one way (main.c's feature, part 13's: §5.2's step J).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it) */
#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* A portal worked (pa: PA_*; cmd.obj.kind a door, a window, or either): found, walked up to, its blow's thing in hand,
** faced, its first words (part 13's feature; or which, asked; walked up to first, then again), then the world's work */
void __fastcall__ s_portal (unsigned char pa)
{
    if (!feature (cmd.obj.kind, pa + 1)) {
        return;
    }
    HUB->ireq.to = 0;
    barw = pa == PA_BAR && things (IO_BAR, "") == IR_NO ? HUB->ireq.x : 0;    /* (The widest bar to hand) */
    work (pa, cmd.obj.kind);
}

/* Up or down (h: H_CLIMB's; H_JUMP's, down a ledge or a shaft, a fall; into a well): a well's shaft down or out of it
** (2026-11-06); through a window, the portal's.  "Go down the well" too, a move's.  (rarer's, part 24's, till stage 3;
** part 24's again, 2026-11-11: part 2's room) */
#pragma code-name (push, "SW24CODE")
#pragma rodata-name (push, "SW24RODATA")
void __fastcall__ s_climb (unsigned char h)
{
    unsigned char d;

    d = h == H_JUMP ? DIR_DOWN | I_JUMP : strstr (cl, "out") ? DIR_UP : DIR_DOWN;
    if (h == H_CLIMB && strcmp (cmd.obj.word, "well") && (cmd.obj.word[0] || !strstr (cl, "out"))) {
        if (cmd.obj.kind == OB_WINDOW || cmd.how == HOW_THROUGH) {
            cmd.obj.kind = OB_WINDOW;           /* (Through a window) */
            portal (PA_THROUGH);
            return;
        }
        d = cmd.dir == GO_DOWN || (cmd.dir == 0xFF && cmd.obj.lies == GO_DOWN) ? DIR_DOWN : DIR_UP;
    }                                           /* ("climb down the ladder": the way where the thing lies) */
    if (!strcmp (cmd.obj.word, "well") && (d & ~I_JUMP) == DIR_DOWN) {
        if (things (IO_FIND | IO_GO, "well") == IR_GONE) {
            return;                             /* (Seen out of reach: walked over to it first, then again; else the
                                                **   world's, one within reach, felt in the dark too) */
        }
        d |= I_WELL;
    }
    intend (I_CLIMB, d);
}
#pragma code-name (pop)
#pragma rodata-name (pop)

/* H_DOOR: a door's work, the verb's (PA_*: its aux, or its row's, lock's); a thing that closes first, of a verb that
** takes one (the row's TK_CLOSER: a chest, a sack, a flask, hthings.c's H_SHUT) */
static void __fastcall__ do_door (unsigned char takes)
{
    unsigned char pa;

    if (!cmd.obj.kind && (takes & TK_CLOSER) && things_work (H_SHUT)) {
        return;
    }
    pa = cmd.aux;
    if (pa == PA_RAISE && cmd.obj.word[0]) {
        pa = PA_LIFT;                           /* ("Lift the bar": a portal's part lifted out) */
    } else if (pa == PA_RAISE || pa == PA_LOWER) {
        cmd.obj.kind = OB_PORTC;
    } else if (cmd.obj.kind == OB_WINDOW && !strcmp (cmd.obj.word, "bars") &&
        !pick (OB_WINDOW | PICK_ANY, cmd.obj.lies)) {
        cmd.obj.kind = OB_PORTC;                /* ("Bend the bars": a portcullis's, with no window there) */
    }
    portal (pa);
}

/* H_EXAMINE: a thing looked at from here (a denizen as it's examined, "look at B", the user's designators, 2026-11-06;
** else way.c's); a thing looked at closely (judged, by the craft that knows it; a portal, or its part if it's not there;
** else the room) */
static void do_examine (void)
{
    unsigned char pa;

    if (!cmd.obj.kind && may_den () && deal (SW_LOOK | SW_QUIET) != SR_NONE) {
        gear_look ();                           /* (A denizen it sees: how it seems, then its things as the eyes
                                                **   make them out from here, part 11's) */
        return;
    }
    if (cmd.action == VB_OBSERVE_OBJECT) {
        look_way (VB_OBSERVE_OBJECT);
        return;
    }
    if ((pa = look_way (VB_EXAMINE_OBJECT)) != 0) {
        portal (pa);                            /* (A thing, a portal, the room, its aspects: part 6's, way.c; a door
                                                **   gone over for traps, the world's) */
    }
}

/* H_KNOCK (the plan's batch B2): a knock on a door, heard beyond it; on a wall, a hollow sought (minds.c's, part 27) */
static void do_knock (void)
{
    minds (H_KNOCK);
}

/* The features' and the doors' commands, by their handler's number (H_DOOR ... H_KNOCK; its takes times 256) */
void __fastcall__ s_places_do (unsigned h)
{
    unsigned char pa;

    switch ((unsigned char) h) {
    case H_KNOCK:
        do_knock ();
        break;
    case H_DOOR:
        do_door (h >> 8);
        break;
    case H_CLIMB:
        climb (H_CLIMB);
        break;
    case H_EXAMINE:
        do_examine ();
        break;
    case H_SEARCH:                              /* (A search, a long task, by its form: what it finds told as it ends,
                                                **   the walls' and the things', E_PORTAL's; search.c's, part 9; a door
                                                **   gone over for traps, the world's) */
        if ((pa = search_go ()) != 0) {
            portal (pa);
        }
        break;
    default:                                    /* (H_LOOK_WAY: looked, listened, sniffed one way, or through a thing:
                                                **   way.c's, part 17; the way out of a room, 2026-11-02; the traces
                                                **   read, slice 4.1's: the floor looked at, a track) */
        look_way (cmd.action);
    }
}

#pragma code-name (pop)
#pragma rodata-name (pop)
