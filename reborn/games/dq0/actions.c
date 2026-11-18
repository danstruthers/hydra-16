/*
** actions.c - (overlay 11) the dispatch table (COMMANDS-CRITIQUE.md's §5.2, step L): a row a verb (actions.h's VB_*, in
** its order: games/dq0/make.js holds them to it), each the verb's handler (dq0.h's H_*: main.c's s_command calls it by
** its number, wherever it is) and the kinds of thing it takes (TK_*), its refusals, for a word and for a kind
** (refuse.c's rules[] till stage 2), and its request (PA_*: the door verbs', the same whatever's said, put in cmd.aux
** where the line gave none, stage 3).  Stage 1 (2026-11-08): the handlers are the cases of main.c's s_command, chores
** and rarer and words.c's more as they were, each verb's by meaning, not by the room its case found; stage 3 put them
** in files by meaning (cmds.h's: hthings.c, hplaces.c, hmove.c, hdens.c, hself.c).  A verb with no handler
** (H_NOT_YET) says "You can't ... in this game yet."; some of those are seen to before the table, as noted.  The
** verbs not done yet (2026-11-09, the plan for them): each a row now, those seen to before the table H_NOTHING (or
** where a line that comes past it goes), the powers refused (H_POWERS, the refusal the row's request, A_NO_*), the
** rest each a handler of its own, a stub saying "not yet" till its batch builds it; grapple's alone H_NOT_YET.
*/

#pragma code-name ("OV11CODE")
#pragma rodata-name ("OV11RODATA")
#pragma data-name ("OV11RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"
#include "actions.h"

struct row {
    unsigned char h;                            /* Its handler (H_*) */
    unsigned char takes;                        /* The kinds of thing it takes (TK_*) */
    unsigned char no;                           /* (Stage 2's: its refusals, a word's and a kind's, refuse.c's) */
    unsigned char op;                           /* Its request (PA_*), where the verb's is the same whatever's said:
                                                **   the door verbs' (stage 3); the handling verbs' whose request is
                                                **   one (IO_*: put, hang, hold, light, put out, rip: step L's rest,
                                                **   2026-11-15; wear and take off are two each, the line's); a
                                                **   power's refusal (A_NO_*, H_POWERS's) */
};
#define ROW(a, h, t)    { (h), (t), 0, 0 }      /* (a: its verb, for make.js to hold to actions.h's order) */

/* A command that names a thing its verb would pass over, or do to something else ("sit on the pot" sat down, "climb
** the pot" climbed the stairs): refused, said back (refuse.c's o_unknown), 1; else 0, carried out as ever.  Asked
** before the command's carried out (main.c's command, its things resolved), by its verb's row (stage 2 of step L,
** 2026-11-08: refuse.c's rules[] till then): a thing named by its word, refused by the row's no's high four bits
** (W_CLIMB: but a well, a rope, a thing's letter; W_PART: but a door's or a window's part); a part of the place (a
** kind, OB_*) not among those the verb does (masks[], the row's takes' high four bits), refused by its low four */
#define BIT(k)      (1u << (k))
#define NOREST      (BIT (OB_DOOR) | BIT (OB_WALL) | BIT (OB_WINDOW) | BIT (OB_PORTC) | BIT (OB_PILLAR) | BIT (OB_LADDER) | \
    BIT (OB_WAY_OUT))                           /* (Parts of the place nothing rests on: a drop there refused) */
#define CLIMBS      (BIT (OB_STAIRS) | BIT (OB_LADDER) | BIT (OB_SHAFT) | BIT (OB_WINDOW) | BIT (OB_LEDGE))
#define WORKED      (BIT (OB_DOOR) | BIT (OB_PORTC) | BIT (OB_WINDOW) | BIT (OB_WAY_OUT) | BIT (OB_TRAP))
#define SEARCHED    (BIT (OB_DOOR) | BIT (OB_PORTC) | BIT (OB_WINDOW) | BIT (OB_ROOM) | BIT (OB_WALL))
#define WAYS        0x0ABEu                     /* (way.c's WAY_OBJ: what's looked at, listened at, sniffed at, a way) */
#define INS         (BIT (OB_DOOR) | BIT (OB_PASSAGE) | BIT (OB_SHAFT) | BIT (OB_WINDOW) | BIT (OB_PORTC))   /* (Looked in) */
#define W_CLIMB     0x80                        /* (A word refused but a well's, a rope's, a thing's letter) */
#define W_PART      0x40                        /* (A word refused but a door's or a window's part's, "bar", "bars",
                                                **   "pane", "shutters", or a letter's: dq0sweep2's, 2026-11-07) */
enum { M_NONE, M_CLIMBS, M_WAYS, M_SEARCHED, M_OUT, M_WORKED, M_IN };
static const unsigned masks[] = { 0, CLIMBS, WAYS, SEARCHED, BIT (OB_WAY_OUT), WORKED, INS };   /* (The kinds a verb does) */
#define ROWR(a, h, t, w, k, m)  { (h), (t) | (m) << 4, (w) << 4 | (k), 0 }   /* (A row with its refusals) */
#define ROWP(a, h, t, w, k, m, p)  { (h), (t) | (m) << 4, (w) << 4 | (k), (p) }   /* (And its request: stage 3) */

static const struct row rows[NACTIONS] = {
    ROW (VB_NONE, H_NOTHING, 0),
    ROW (VB_OBSERVE_LOCALE, H_FEEL, 0),         /* (In the dark, or blind, a feel around; else a look) */
    ROW (VB_OBSERVE_DIRECTION, H_LOOK_WAY, 0),
    ROW (VB_OBSERVE_OBJECT, H_EXAMINE, 0),      /* (A denizen; else a look at it, H_LOOK_WAY's) */
    ROW (VB_OBSERVE_THROUGH, H_LOOK_WAY, 0),
    ROW (VB_OBSERVE_AROUND, H_LOOK, 0),
    ROW (VB_EXAMINE_OBJECT, H_EXAMINE, 0),
    ROW (VB_EXAMINE_LOCALE, H_EXAMINE, 0),
    ROWR (VB_RETRIEVE_OBJECT, H_TAKE, 0, 0, U_CANT, M_NONE),
    ROW (VB_DROP_OBJECT, H_DROP, TK_ONTO),
    ROWP (VB_PUT_OBJECT, H_HANDLE, TK_ONTO, 0, 0, M_NONE, IO_PUT),
    ROW (VB_USE_OBJECT, H_USE, 0),              /* (The puzzle's first, theme.c's, before the table; B4's) */
    ROWR (VB_EQUIP_OBJECT, H_HANDLE, 0, 0, U_CANT, M_NONE),
    ROW (VB_UNEQUIP_OBJECT, H_HANDLE, 0),
    ROWP (VB_OPEN_OBJECT, H_DOOR, TK_CLOSER, 0, U_CANT, M_WORKED, PA_OPEN),    /* (A thing that closes: a chest; else a door's work) */
    ROWP (VB_CLOSE_OBJECT, H_DOOR, TK_CLOSER, 0, U_CANT, M_WORKED, PA_CLOSE),
    ROW (VB_MOVE_DIRECTION, H_GO, 0),
    ROW (VB_MOVE_TOWARD, H_GO, 0),
    ROW (VB_FACE_DIRECTION, H_FACE, 0),
    ROW (VB_STEP_DIRECTION, H_GO, 0),           /* (pace.c's, the line's own, before s_command: a step; else a thing
                                                **   said, "step onto the stairs", gone to) */
    ROW (VB_SET_PACE, H_STATUS, 0),             /* (pace.c's; a pace it doesn't know, "double", the state told) */
    ROW (VB_PACE_SHOW, H_STATUS, 0),            /* ("pace" alone: the state, its pace with it) */
    ROW (VB_CHARGE, H_CHARGE, 0),
    ROW (VB_THROW_DIRECTION, H_THROW_WAY, 0),
    ROW (VB_JUMP, H_JUMP, 0),
    ROW (VB_SWIM, H_SWIM, 0),
    ROWR (VB_ATTACK_OBJECT, H_FIGHT, 0, 0, U_CANT, M_WORKED),
    ROW (VB_GRAPPLE_OBJECT, H_FIGHT, 0),       /* (A grapple's move, its aux GM_*: blows.c's grapple, HOUSE.md's) */
    ROW (VB_THROW_OBJECT, H_MISSILE, 0),
    ROW (VB_SHOOT_OBJECT, H_MISSILE, 0),
    ROW (VB_LOAD_OBJECT, H_MISSILE, 0),
    ROW (VB_SAY, H_SAY, 0),
    ROW (VB_TELL_OBJECT, H_SWAY, 0),
    ROWP (VB_CAST_SPELL, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),     /* (Refused: its request the refusal) */
    ROWR (VB_EAT_OBJECT, H_EAT, 0, 0, U_CANT, M_NONE),
    ROWR (VB_DRINK, H_EAT, 0, 0, U_CANT, M_NONE),
    ROWP (VB_LIGHT_OBJECT, H_HANDLE, 0, 0, U_CANT, M_NONE, IO_LIGHT),
    ROWP (VB_EXTINGUISH_OBJECT, H_HANDLE, 0, 0, U_CANT, M_NONE, IO_DOUSE),
    ROW (VB_FIND_OBJECT, H_FEEL, 0),
    ROWR (VB_SEARCH, H_SEARCH, 0, 0, U_YET, M_SEARCHED),
    ROWR (VB_LISTEN, H_LOOK_WAY, 0, U_YET, U_YET, M_WAYS),
    ROWR (VB_SMELL, H_LOOK_WAY, 0, U_YET, U_YET, M_WAYS),
    ROW (VB_TRACK, H_LOOK_WAY, 0),
    ROW (VB_TARGET_SET, H_NOTHING, 0),          /* (resolve.c's, before the table: never here) */
    ROW (VB_TARGET_CLEAR, H_NOTHING, 0),        /* (resolve.c's) */
    ROW (VB_TARGET_SHOW, H_NOTHING, 0),         /* (resolve.c's) */
    ROW (VB_WAIT, H_WAIT, 0),
    ROW (VB_SLEEP, H_WAIT, 0),
    ROWR (VB_SIT, H_LIMBS, 0, U_YET, U_YET, M_NONE),
    ROWR (VB_STAND, H_LIMBS, 0, U_YET, U_YET, M_NONE),
    ROWR (VB_LIE_DOWN, H_LIMBS, 0, U_YET, U_YET, M_NONE),
    ROW (VB_PRETEND, H_PRETEND, 0),
    ROW (VB_STOP, H_STOP, 0),
    ROW (VB_NAME_OBJECT, H_SWAY, 0),
    ROW (VB_NAME_PLACE, H_SWAY, 0),
    ROW (VB_META_STATUS, H_STATUS, 0),          /* ("status" alone pace.c's, before the table; "show status" here) */
    ROW (VB_META_HELP, H_HELP, 0),
    ROW (VB_META_INVENTORY, H_INVENTORY, 0),
    ROW (VB_META_MAP, H_SHEET, 0),
    ROW (VB_META_SAVE, H_META, 0),
    ROW (VB_META_QUIT, H_QUIT, 0),
    ROW (VB_ENTER, H_GO, 0),
    ROW (VB_LEAVE, H_LOOK_WAY, 0),
    ROWR (VB_CLIMB, H_CLIMB, 0, U_CANT | (W_CLIMB >> 4), U_CANT, M_CLIMBS),                 /* (Up or down; through a window, the portal's) */
    ROW (VB_DIG, H_DIG, 0),
    ROW (VB_WAKE, H_WAKE, 0),
    ROWR (VB_CROUCH, H_LIMBS, 0, U_YET, U_YET, M_NONE),
    ROW (VB_HIDE, H_HIDE, 0),
    ROW (VB_SNEAK, H_GO, 0),                    /* (A way gone, the pace a creep first: pace.c's; "sneak" alone, the
                                                **   pace alone, pace.c's) */
    ROW (VB_KNOCK_OBJECT, H_KNOCK, 0),
    ROW (VB_PUSH_OBJECT, H_PUSH, 0),
    ROW (VB_PULL_OBJECT, H_PULL, 0),
    ROW (VB_GIVE_OBJECT, H_GIVE, 0),
    ROWP (VB_HOLD_OBJECT, H_HANDLE, 0, 0, 0, M_NONE, IO_HOLD),
    ROWP (VB_LOCK_OBJECT, H_DOOR, TK_CLOSER, 0, U_CANT, M_WORKED, PA_LOCK),
    ROWP (VB_UNLOCK_OBJECT, H_DOOR, TK_CLOSER, 0, U_CANT, M_WORKED, PA_UNLOCK),
    ROWR (VB_FILL_OBJECT, H_FILL, 0, 0, U_CANT, M_NONE),
    ROWR (VB_READ_OBJECT, H_NOTHING, 0, 0, U_NOTE, M_OUT),         /* (The puzzle's, theme.c's: read.c's, before the
                                                **   table; its refusals here) */
    ROW (VB_TASTE_OBJECT, H_TASTE, 0),
    ROW (VB_TOUCH_OBJECT, H_TOUCH, 0),
    ROW (VB_DEFEND, H_FIGHT, 0),
    ROW (VB_FLEE, H_DISENGAGE, 0),
    ROW (VB_FOLLOW_OBJECT, H_FOLLOW, 0),
    ROW (VB_REPEAT, H_AGAIN, 0),
    ROW (VB_WHERE, H_LOOK, 0),
    ROW (VB_ATTACK_SPLIT, H_SPLIT, 0),
    ROW (VB_ENGAGE, H_ENGAGE, 0),
    ROW (VB_DISENGAGE, H_DISENGAGE, 0),
    ROW (VB_PASS, H_WAIT, 0),
    ROW (VB_SEQUENCE, H_NOTHING, 0),            /* (parts.c's: the line split first, never here) */
    ROW (VB_PRAY, H_PRAY, 0),
    ROW (VB_PRAY_FOR, H_PRAY, 0),
    ROWP (VB_CALL_INTERVENTION, H_POWERS, 0, 0, 0, M_NONE, A_NO_GODS),
    ROWP (VB_INVOKE, H_POWERS, 0, 0, 0, M_NONE, A_NO_GODS),
    ROWP (VB_LEARN_INVOCATION, H_POWERS, 0, 0, 0, M_NONE, A_NO_GODS),
    ROW (VB_FASTING, H_FAST, 0),
    ROWP (VB_MEMORIZE, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_UNMEMORIZE, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_LEARN_SPELL, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_RESEARCH, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_DISPEL, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_COUNTER, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_RELEASE_PRINCIPLE, H_POWERS, 0, 0, 0, M_NONE, A_NO_MAGIC),
    ROWP (VB_USE_TALENT, H_POWERS, 0, 0, 0, M_NONE, A_NO_PSI),
    ROWP (VB_POSSESS, H_POWERS, 0, 0, 0, M_NONE, A_NO_PSI),
    ROWP (VB_LINK_MIND, H_POWERS, 0, 0, 0, M_NONE, A_NO_PSI),
    ROW (VB_ACTIVATE, H_USE, 0),                /* (The parser's use now: "activate = use", commands.verbs) */
    ROW (VB_INTIMIDATE, H_SWAY, 0),
    ROW (VB_ORDER, H_SWAY, 0),
    ROW (VB_PERSUADE, H_SWAY, 0),
    ROW (VB_ORATE, H_ORATE, 0),
    ROWP (VB_NEGOTIATE, H_SWAY, 0, 0, 0, M_NONE, SW_PERSUADE),       /* (The parser's persuade now: "negotiate =
                                                **   persuade", commands.verbs) */
    ROW (VB_FAST_TALK, H_SWAY, 0),
    ROW (VB_TAUNT, H_TAUNT, 0),
    ROW (VB_JUDGE_SINCERITY, H_EXAMINE, 0),     /* (The parser's examine now: "sense a lie = examine") */
    ROWP (VB_BREAK_OBJECT, H_DOOR, 0, 0, U_CANT, M_WORKED, PA_BREAK),           /* (A door's work alone) */
    ROWP (VB_RAISE_OBJECT, H_DOOR, 0, U_CANT | (W_PART >> 4), U_CANT, M_WORKED, PA_RAISE),
    ROWP (VB_LOWER_OBJECT, H_DOOR, 0, U_CANT | (W_PART >> 4), U_CANT, M_WORKED, PA_LOWER),
    ROWP (VB_BEND_OBJECT, H_DOOR, 0, U_CANT | (W_PART >> 4), U_CANT, M_WORKED, PA_BEND),
    ROWP (VB_FORCE_OBJECT, H_DOOR, TK_CLOSER, 0, U_CANT, M_WORKED, PA_FORCE),
    ROWP (VB_DISARM_OBJECT, H_DOOR, 0, U_CANT | (W_PART >> 4), U_CANT, M_WORKED, PA_DISARM),
    ROWR (VB_BAR_OBJECT, H_DOOR, 0, U_CANT | (W_PART >> 4), U_CANT, M_WORKED),
    ROWP (VB_HANG_OBJECT, H_HANDLE, TK_ONTO, 0, 0, M_NONE, IO_PUT),
    ROW (VB_COMBINE_OBJECT, H_NOTHING, 0),      /* (The puzzle's, theme.c's, before the table: never here) */
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
    ROWR (VB_OBSERVE_IN, H_LOOK_IN, 0, 0, U_CANT, M_IN),
    ROW (VB_SWAP_HANDS, H_LIMBS, 0),
    ROW (VB_TEND, H_TEND, 0),
    ROW (VB_GATHER_OBJECT, H_MISSILE, 0),
    ROW (VB_META_TIME, H_TIME, 0),
    ROW (VB_TRADE, H_GIVE, 0),
    ROWR (VB_TIE_OBJECT, H_TIE, TK_ONTO, 0, U_CANT, M_NONE),
    ROWR (VB_UNTIE_OBJECT, H_TIE, 0, 0, U_CANT, M_NONE),
    ROWP (VB_RIP_OBJECT, H_HANDLE, 0, 0, U_CANT, M_NONE, IO_RIP)     /* (A cloth torn up, washed: the things' IO_RIP, IO_WASH,
                                                **   hthings.c's; HOUSE.md's bandages, 2026-11-10) */
};

/* Verb a's row: its handler, its takes times 256 (one past the table: none's); its request put in the command's aux,
** if the line gave none (lock's PA_LOCK: the parser's 0, its handlers' own test of the verb till stage 3) */
unsigned __fastcall__ o_verb_row (unsigned char a)
{
    if (a >= NACTIONS) {
        return H_NOT_YET;
    }
    if (!cmd.aux) {
        cmd.aux = rows[a].op;
    }
    return rows[a].h | rows[a].takes << 8;
}

unsigned char o_refused (void)
{
    static const struct row* r;
    static const char* w;
    static unsigned char f;

    w = cmd.obj.word;
    if (cmd.action >= NACTIONS) {
        return 0;
    }
    r = rows + cmd.action;
    f = 0;
    f = cmd.other.kind;
    if ((r->takes & TK_ONTO) && f && !(cmd.prep == PR_IN && (f == OB_SHAFT || f == OB_TRAP)) &&
        !(cmd.action == VB_TIE_OBJECT && (f == OB_PILLAR || f == OB_WINDOW)) &&
        (cmd.action == VB_DROP_OBJECT ? f < 16 && (NOREST >> f & 1) : f != OB_STAIRS || cmd.action != VB_PUT_OBJECT)) {
        f = U_CANT;                             /* (A part of the place where it's put, or tied: "You can't put the torch on
                                                **   the door.", not the belt's; but into a hole, let fall, 2026-11-17; a
                                                **   rope tied to a pillar, a window's bars, the user's, 2026-11-17;
                                                **   dropped, refused only where nothing can rest, a put on the stairs
                                                **   placed there: the user's, 2026-11-17) */
    } else if ((f = 0), !r->no) {
    } else if (*w) {
        f = r->no >> 4 & 3;
        if (*w == '#' || ((r->no & W_CLIMB) && (!strcmp (w, "well") || strstr (w, "rope"))) || ((r->no & W_PART) &&
            (strstr (w, "bar") || strstr (w, "pane") || strstr (w, "shutter")))) {
            f = (r->no & (W_CLIMB | W_PART)) ? 0 : f;
        }
    } else if (cmd.obj.kind && !(masks[r->takes >> 4] >> (cmd.obj.kind & 15) & 1)) {
        f = r->no & 15;
    }
    if (f) {
        unknown (f);
    }
    return f != 0;
}
