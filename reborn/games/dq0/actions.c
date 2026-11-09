/*
** actions.c - (overlay 11) the dispatch table (COMMANDS-CRITIQUE.md's §5.2, step L): a row a verb (actions.h's VB_*, in
** its order: games/dq0/make.js holds them to it), each the verb's handler (dq0.h's H_*: main.c's s_command calls it by
** its number, wherever it is) and the kinds of thing it takes (TK_*), and room for what the later stages put in: its
** refusals, for a word and for a kind (refuse.c's rules[], stage 2), and its request (IO_*, PA_*, I_*, where cmd.aux
** holds one, stage 3).  Stage 1 (2026-11-08): the handlers are the cases of main.c's s_command, chores and rarer and
** words.c's more as they were, each verb's by meaning, not by the room its case found.  A verb with no handler
** (H_NOT_YET) says "You can't ... in this game yet."; some of those are seen to before the table, as noted.
*/

#pragma code-name ("OV11CODE")
#pragma rodata-name ("OV11RODATA")
#pragma data-name ("OV11RODATA")

#include "dq0.h"
#include "actions.h"

struct row {
    unsigned char h;                            /* Its handler (H_*) */
    unsigned char takes;                        /* The kinds of thing it takes (TK_*) */
    unsigned char no;                           /* (Stage 2's: its refusals, a word's and a kind's, refuse.c's) */
    unsigned char op;                           /* (Stage 3's: its request, where cmd.aux holds one) */
};
#define ROW(a, h, t)    { (h), (t), 0, 0 }      /* (a: its verb, for make.js to hold to actions.h's order) */

static const struct row rows[NACTIONS] = {
    ROW (VB_NONE, H_NOTHING, 0),
    ROW (VB_OBSERVE_LOCALE, H_FEEL, 0),         /* (In the dark, or blind, a feel around; else a look) */
    ROW (VB_OBSERVE_DIRECTION, H_LOOK_WAY, 0),
    ROW (VB_OBSERVE_OBJECT, H_EXAMINE, 0),      /* (A denizen; else a look at it, H_LOOK_WAY's) */
    ROW (VB_OBSERVE_THROUGH, H_LOOK_WAY, 0),
    ROW (VB_OBSERVE_AROUND, H_LOOK, 0),
    ROW (VB_EXAMINE_OBJECT, H_EXAMINE, 0),
    ROW (VB_EXAMINE_LOCALE, H_EXAMINE, 0),
    ROW (VB_RETRIEVE_OBJECT, H_TAKE, 0),
    ROW (VB_DROP_OBJECT, H_DROP, 0),
    ROW (VB_PUT_OBJECT, H_HANDLE, 0),
    ROW (VB_USE_OBJECT, H_NOT_YET, 0),          /* (The puzzle's, theme.c's, before the table) */
    ROW (VB_EQUIP_OBJECT, H_HANDLE, 0),
    ROW (VB_UNEQUIP_OBJECT, H_HANDLE, 0),
    ROW (VB_OPEN_OBJECT, H_DOOR, TK_CLOSER),    /* (A thing that closes: a chest; else a door's work) */
    ROW (VB_CLOSE_OBJECT, H_DOOR, TK_CLOSER),
    ROW (VB_MOVE_DIRECTION, H_GO, 0),
    ROW (VB_MOVE_TOWARD, H_GO, 0),
    ROW (VB_FACE_DIRECTION, H_FACE, 0),
    ROW (VB_STEP_DIRECTION, H_NOT_YET, 0),      /* (pace.c's, the line's own, before s_command) */
    ROW (VB_SET_PACE, H_NOT_YET, 0),            /* (pace.c's) */
    ROW (VB_PACE_SHOW, H_NOT_YET, 0),           /* (pace.c's) */
    ROW (VB_CHARGE, H_NOT_YET, 0),
    ROW (VB_THROW_DIRECTION, H_NOT_YET, 0),
    ROW (VB_JUMP, H_JUMP, 0),
    ROW (VB_SWIM, H_NOT_YET, 0),
    ROW (VB_ATTACK_OBJECT, H_FIGHT, 0),
    ROW (VB_GRAPPLE_OBJECT, H_NOT_YET, 0),
    ROW (VB_THROW_OBJECT, H_MISSILE, 0),
    ROW (VB_SHOOT_OBJECT, H_MISSILE, 0),
    ROW (VB_LOAD_OBJECT, H_MISSILE, 0),
    ROW (VB_SAY, H_NOT_YET, 0),
    ROW (VB_TELL_OBJECT, H_SWAY, 0),
    ROW (VB_CAST_SPELL, H_NOT_YET, 0),
    ROW (VB_EAT_OBJECT, H_EAT, 0),
    ROW (VB_DRINK, H_EAT, 0),
    ROW (VB_LIGHT_OBJECT, H_HANDLE, 0),
    ROW (VB_EXTINGUISH_OBJECT, H_HANDLE, 0),
    ROW (VB_FIND_OBJECT, H_FEEL, 0),
    ROW (VB_SEARCH, H_SEARCH, 0),
    ROW (VB_LISTEN, H_LOOK_WAY, 0),
    ROW (VB_SMELL, H_LOOK_WAY, 0),
    ROW (VB_TRACK, H_LOOK_WAY, 0),
    ROW (VB_TARGET_SET, H_NOT_YET, 0),          /* (resolve.c's, before the table) */
    ROW (VB_TARGET_CLEAR, H_NOT_YET, 0),        /* (resolve.c's) */
    ROW (VB_TARGET_SHOW, H_NOT_YET, 0),         /* (resolve.c's) */
    ROW (VB_WAIT, H_WAIT, 0),
    ROW (VB_SLEEP, H_WAIT, 0),
    ROW (VB_SIT, H_LIMBS, 0),
    ROW (VB_STAND, H_LIMBS, 0),
    ROW (VB_LIE_DOWN, H_LIMBS, 0),
    ROW (VB_PRETEND, H_NOT_YET, 0),
    ROW (VB_STOP, H_STOP, 0),
    ROW (VB_NAME_OBJECT, H_SWAY, 0),
    ROW (VB_NAME_PLACE, H_SWAY, 0),
    ROW (VB_META_STATUS, H_NOT_YET, 0),
    ROW (VB_META_HELP, H_HELP, 0),
    ROW (VB_META_INVENTORY, H_INVENTORY, 0),
    ROW (VB_META_MAP, H_SHEET, 0),
    ROW (VB_META_SAVE, H_META, 0),
    ROW (VB_META_QUIT, H_QUIT, 0),
    ROW (VB_ENTER, H_GO, 0),
    ROW (VB_LEAVE, H_LOOK_WAY, 0),
    ROW (VB_CLIMB, H_CLIMB, 0),                 /* (Part 2's: through a window; else its work part 24's) */
    ROW (VB_DIG, H_DIG, 0),
    ROW (VB_WAKE, H_NOT_YET, 0),
    ROW (VB_CROUCH, H_LIMBS, 0),
    ROW (VB_HIDE, H_HIDE, 0),
    ROW (VB_SNEAK, H_NOT_YET, 0),
    ROW (VB_KNOCK_OBJECT, H_NOT_YET, 0),
    ROW (VB_PUSH_OBJECT, H_NOT_YET, 0),
    ROW (VB_PULL_OBJECT, H_NOT_YET, 0),
    ROW (VB_GIVE_OBJECT, H_GIVE, 0),
    ROW (VB_HOLD_OBJECT, H_HANDLE, 0),
    ROW (VB_LOCK_OBJECT, H_DOOR, TK_CLOSER),
    ROW (VB_UNLOCK_OBJECT, H_DOOR, TK_CLOSER),
    ROW (VB_FILL_OBJECT, H_FILL, 0),
    ROW (VB_READ_OBJECT, H_NOT_YET, 0),         /* (The puzzle's, theme.c's: read.c's) */
    ROW (VB_TASTE_OBJECT, H_NOT_YET, 0),
    ROW (VB_TOUCH_OBJECT, H_NOT_YET, 0),
    ROW (VB_DEFEND, H_FIGHT, 0),
    ROW (VB_FLEE, H_DISENGAGE, 0),
    ROW (VB_FOLLOW_OBJECT, H_NOT_YET, 0),
    ROW (VB_REPEAT, H_NOT_YET, 0),
    ROW (VB_WHERE, H_LOOK, 0),
    ROW (VB_ATTACK_SPLIT, H_NOT_YET, 0),
    ROW (VB_ENGAGE, H_NOT_YET, 0),
    ROW (VB_DISENGAGE, H_DISENGAGE, 0),
    ROW (VB_PASS, H_WAIT, 0),
    ROW (VB_SEQUENCE, H_NOT_YET, 0),
    ROW (VB_PRAY, H_NOT_YET, 0),
    ROW (VB_PRAY_FOR, H_NOT_YET, 0),
    ROW (VB_CALL_INTERVENTION, H_NOT_YET, 0),
    ROW (VB_INVOKE, H_NOT_YET, 0),
    ROW (VB_LEARN_INVOCATION, H_NOT_YET, 0),
    ROW (VB_FASTING, H_NOT_YET, 0),
    ROW (VB_MEMORIZE, H_NOT_YET, 0),
    ROW (VB_UNMEMORIZE, H_NOT_YET, 0),
    ROW (VB_LEARN_SPELL, H_NOT_YET, 0),
    ROW (VB_RESEARCH, H_NOT_YET, 0),
    ROW (VB_DISPEL, H_NOT_YET, 0),
    ROW (VB_COUNTER, H_NOT_YET, 0),
    ROW (VB_RELEASE_PRINCIPLE, H_NOT_YET, 0),
    ROW (VB_USE_TALENT, H_NOT_YET, 0),
    ROW (VB_POSSESS, H_NOT_YET, 0),
    ROW (VB_LINK_MIND, H_NOT_YET, 0),
    ROW (VB_ACTIVATE, H_NOT_YET, 0),
    ROW (VB_INTIMIDATE, H_SWAY, 0),
    ROW (VB_ORDER, H_SWAY, 0),
    ROW (VB_PERSUADE, H_SWAY, 0),
    ROW (VB_ORATE, H_NOT_YET, 0),
    ROW (VB_NEGOTIATE, H_NOT_YET, 0),
    ROW (VB_FAST_TALK, H_SWAY, 0),
    ROW (VB_TAUNT, H_NOT_YET, 0),
    ROW (VB_JUDGE_SINCERITY, H_NOT_YET, 0),
    ROW (VB_BREAK_OBJECT, H_DOOR, 0),           /* (A door's work alone) */
    ROW (VB_RAISE_OBJECT, H_DOOR, 0),
    ROW (VB_LOWER_OBJECT, H_DOOR, 0),
    ROW (VB_BEND_OBJECT, H_DOOR, 0),
    ROW (VB_FORCE_OBJECT, H_DOOR, TK_CLOSER),
    ROW (VB_DISARM_OBJECT, H_DOOR, 0),
    ROW (VB_BAR_OBJECT, H_DOOR, 0),
    ROW (VB_HANG_OBJECT, H_HANDLE, 0),
    ROW (VB_COMBINE_OBJECT, H_NOT_YET, 0),      /* (The puzzle's, theme.c's) */
    ROW (VB_APPROACH_OBJECT, H_GO, 0),
    ROW (VB_DECEIVE, H_SWAY, 0),
    ROW (VB_THREATEN, H_SWAY, 0),
    ROW (VB_CALM, H_SWAY, 0),
    ROW (VB_BEFRIEND, H_SWAY, 0),
    ROW (VB_ALLY_COME, H_SWAY, 0),
    ROW (VB_ALLY_STAY, H_SWAY, 0),
    ROW (VB_ALLY_DISMISS, H_SWAY, 0),
    ROW (VB_META_SHEET, H_SHEET, 0),
    ROW (VB_META_PAUSE, H_META, 0),
    ROW (VB_OBSERVE_IN, H_LOOK_IN, 0),
    ROW (VB_SWAP_HANDS, H_LIMBS, 0),
    ROW (VB_TEND, H_TEND, 0),
    ROW (VB_GATHER_OBJECT, H_MISSILE, 0),
    ROW (VB_META_TIME, H_TIME, 0),
    ROW (VB_TRADE, H_GIVE, 0),
    ROW (VB_TIE_OBJECT, H_TIE, 0),
    ROW (VB_UNTIE_OBJECT, H_TIE, 0)
};

/* Verb a's row: its handler, its takes times 256 (one past the table: none's) */
unsigned __fastcall__ o_verb_row (unsigned char a)
{
    if (a >= NACTIONS) {
        return H_NOT_YET;
    }
    return rows[a].h | rows[a].takes << 8;
}
