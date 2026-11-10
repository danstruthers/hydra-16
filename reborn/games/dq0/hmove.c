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

/* H_SWIM (the plan's batch B6, 2026-11-10): swum a way, or to a thing, a move (H_GO's: the world's water, swimming
** where it's deep); alone, treading water; a dive (part 27's swim_go, below) */
unsigned char swim_go (void);
static void do_swim (void)
{
    if (swim_go ()) {
        do_go ();
    }
}

/* H_FOLLOW (B6's): a denizen seen trailed, a walk re-aimed each round; "follow me" an ally's (part 27's, below) */
void follow_go (void);
static void do_follow (void)
{
    follow_go ();
}

/* Moving, by its handler's number (H_GO ... H_FOLLOW) */
void __fastcall__ s_moving_do (unsigned h)
{
    switch ((unsigned char) h) {
    case H_SWIM:
        do_swim ();
        break;
    case H_FOLLOW:
        do_follow ();
        break;
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

/* ---- Swimming and following (the plan's batch B6, 2026-11-10; DECISIONS.md's "Prayer, fasting, swimming and
** following"): part 27's, its own (part 2's room; the banks full: overlay 25 its image's), its stubs the build's */
#pragma code-name (push, "SW27CODE")
#pragma rodata-name (push, "SW27RODATA")

/* A swim (VB_SWIM; "dive", "tread water" the parser's): 1 a way or a thing said, swum to (the caller's H_GO: the
** world's water, its swimming where it's deep, water.c's words); else seen to here: out of its depth (the hub's pc_wet,
** five feet, the world's SWIM_FT), "swim" alone treads water, a minute's swim tested (the world's, I_WAIT's
** WA_TREAD: under, it strikes out for the surface); "dive", "swim down" goes under on purpose (WA_DIVE: its breath
** held a minute, a timed work, its command kept, main.c's reach 3), and its minute up (reach 4) strikes out for the
** surface (a tread, the swim's test: up, gasping, or under again); in shallower water or none, refused */
unsigned char s_swim_go (void)
{
    unsigned char d = cmd.dir, dive = 0;

    if (reach != 4) {
        dive = d == GO_DOWN || strstr (cl, "dive") != 0;
        if (!dive && d != GO_UP && (d != 0xFF || cmd.obj.kind || (cmd.obj.word[0] && strcmp (cmd.obj.word, "water")))) {
            return 1;
        }
        hub_map (0);
        if ((HUB->pc_wet & ~WET_RUNS) < 5) {
            msg ("There's no water deep enough.");
            return 0;
        }
        if (dive) {
            msg (HUB->pc_under ? "You swim deeper, holding your breath." : "You take a deep breath and dive under.");
            hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
            reach = 3;
            intend_a = WA_DIVE;
            intend2 (I_WAIT, 600, I_WAIT_W2 (LT_WAIT, 0, 1));   /* (A minute under) */
            return 0;
        }
    }
    hub_map (0);
    msg (HUB->pc_under ? "You strike out for the surface." : "You tread water.");
    intend_a = WA_TREAD;
    intend2 (I_WAIT, reach == 4 || HUB->pc_under ? 50 : 600, I_WAIT_W2 (0, 0, 1));   /* (A minute's; up, moments) */
    return 0;
}

static unsigned char fol, fol_still;            /* (The denizen followed, its record: the denizens' task's, struct
                                                **   actor's den; 1 caught up with it, a round's wait beside it once; 2
                                                **   walked after it, */
static unsigned fol_x, fol_y;                   /*   from here) */

/* A denizen followed ("follow the rat", "tail the bandit": VB_FOLLOW_OBJECT): one seen (the resolver's, its square in
** cmd.obj), walked after (the world's I_CLOSE w1 1: up to it as it moves, no blow), its command kept and carried out
** again as the walk ends (main.c's reach 1), so re-aimed each time; caught up with, a round's wait beside it, and on
** again if it goes; still beside it after that, done; out of sight, or gone to another stratum, lost ("track" offered).
** "follow me" (and "follow" alone, the parser's) is an ally's request, as ever (the sway's: OR_FOLLOW) */
void s_follow_go (void)
{
    const struct actor* a;
    unsigned char k, n = 0;
    int dx, dy;

    if (cmd.obj.sel == SL_ME || !cmd.obj.word[0]) {
        cmd.action = VB_ALLY_COME;              /* ("follow me": the allies called, hdens.c's H_SWAY) */
        dens_do (H_SWAY);
        return;
    }
    if (reach != 2) {                           /* (Begun: the one it means, seen) */
        fol = 0xFF;
        hub_map (SIGHT_BANK);
        for (k = SIGHT->ndens; cmd.obj.x && k--; ) {
            if (SIGHT->den[k].x == cmd.obj.x && SIGHT->den[k].y == cmd.obj.y) {
                fol = SIGHT->den[k].den;
                break;
            }
        }
        hub_map (0);
        if (fol == 0xFF) {
            if (cmd.obj.x || cmd.obj.kind) {
                sayf ("\nThe %s isn't going anywhere.", cmd.obj.word);
            } else {
                say_no (A_SEE_NO, cmd.obj.word);
            }
            return;
        }
        fol_still = 0;
    }
    hub_map (SIGHT_BANK);
    for (k = SIGHT->ndens; k--; ) {
        if (SIGHT->den[k].den == fol) {
            n = 1;
        }
    }
    hub_map (0);
    for (k = 0, a = HUB->actor; k < NACTORS && !(a->state && a->name[0] && a->den == fol); ++k, ++a) {
    }
    strcpy (lb, k < NACTORS && a->name[0] < 'a' ? "" : "the ");   /* ("the giant rat", "Grub") */
    strcat (lb, k < NACTORS ? a->name : cmd.obj.word);
    dx = (int) a->x - (int) HUB->pc_x;
    dy = (int) a->y - (int) HUB->pc_y;
    if (k == NACTORS || a->sn != HUB->pc_stratum || (!n && (fol_still == 3 || dx < -6 || dx > 6 || dy < -6 || dy > 6))) {
        sayf ("\nYou've lost %s. (\"track\" may find its trail.)", lb);
        return;
    }
    if (reach != 2) {
        sayf ("\nYou follow %s.", lb);
    }
    if (!n) {                                   /* (Out of sight but near, gone behind it: looked round for, once) */
        fol_still = 3;
        intend (I_TURN, bearing (dx, dy));
    } else if (dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1) {
        if (fol_still == 1) {
            sayf ("\nYou stop beside %s.", lb);   /* (It stays: caught up) */
            return;
        }
        fol_still = 1;
        intend2 (I_WAIT, HUB->round, I_WAIT_W2 (0, 0, 1));   /* (A round beside it, to see if it goes) */
    } else {
        if (fol_still == 2 && fol_x == HUB->pc_x && fol_y == HUB->pc_y) {
            sayf ("\nYou can't keep up with %s.", lb);   /* (Its last walk after it got nowhere: no way, or no
                                                **   nearer) */
            return;
        }
        fol_still = 2;
        fol_x = HUB->pc_x;
        fol_y = HUB->pc_y;
        placing = 1;                            /* (Its arrival not "You go over to it.": "You follow" said) */
        intend_a = k + 1;
        intend2 (I_CLOSE, 1, 0);                /* (After it, re-aimed as it moves: the world's) */
    }
    hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
    reach = 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
