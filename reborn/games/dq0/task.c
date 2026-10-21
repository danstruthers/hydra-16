/*
** task.c - (dq0) the long tasks' words (HOUSE.md's long tasks, slice 4.0: the world keeps them, the hub's pc_task the
** character's): what came of one (E_TASK: stopped, asked, carried on, refused), a walk's beats as it goes (E_BEAT),
** the task a stop names ("stop digging": its LT_*), a question's answer read, and a line typed as the character's
** busy carried out at once or held (fighting on's: a command taking over).  Overlay 3's.
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

static const char* const doing[] = {           /* (By LT_*) */
    "doing anything", "walking", "climbing", "digging", "searching", "waiting", "sleeping", "talking", "battering",
    "fighting"
};
#define NDOING          (sizeof doing / sizeof doing[0])

/* An enemy the character's met (E_TASK's b, TE_ENGAGED's, TE_FAR's: its entry plus 1), as it's said: "the giant
** rat" (the first letter a capital, cap), or "something" if it isn't seen */
static char fn[20];
static const char* __fastcall__ foe (unsigned char b, unsigned char cap)
{
    static char at[12], seen[12];
    unsigned char k, n;

    hub_mem (0, at, HUB->actor[(b - 1) & 7].name, sizeof at);
    hub_mem (SIGHT_BANK, &n, &SIGHT->ndens, 1);
    for (k = n; k--; ) {
        hub_mem (SIGHT_BANK, seen, SIGHT->den[k].name, sizeof seen);
        if (!strcmp (seen, at)) {
            break;
        }
    }
    strcpy (fn, k == 0xFF ? "something" : at[0] >= 'A' && at[0] <= 'Z' ? "" : "the ");
    if (k != 0xFF) {
        strcat (fn, at);
    }
    if (cap && fn[0] >= 'a') {
        fn[0] -= 'a' - 'A';
    }
    return fn;
}

/* What came of the character's long task: E_TASK's w (TE_*, its kind times 256); named, the kind its stop named (b),
** or the foe (TE_FIGHT's), or the health (TE_ASK_HURT's).  1 if a question's put; 2 one answered yes unasked (dq0
** -a's, the tests': not put); 4 fighting on begun (the character busy) */
unsigned char __fastcall__ o_task_words (unsigned w, unsigned char named)
{
    unsigned char k = w >> 8;
    const char* d = doing[k < NDOING ? k : 0];

    if ((unsigned char) w <= TE_NEAR || (unsigned char) w == TE_ENGAGED) {
        reach = 0;                              /* (A walk over to a thing stopped: its command dropped) */
    }
    k = (unsigned char) w;
    if (autogo && (k == TE_ASK_SEEN || k == TE_ASK_TIRED || k == TE_ASK_HEARD || k == TE_ASK_HURT)) {
        return 2;
    }
    switch (k) {
    case TE_STOPPED:
    case TE_HIT:
    case TE_NEAR:
        sayf ("\nYou stop %s.", d);
        break;
    case TE_ASK_SEEN:
    case TE_ASK_HEARD:
        sayf ("\nKeep %s?", d);
        break;
    case TE_ASK_TIRED:
        sayf ("\nYou're tiring.  Keep %s?", d);
        break;
    case TE_NOT:
        sayf ("\nYou aren't %s: you're %s.", doing[named < NDOING ? named : 0], d);
        break;
    case TE_IDLE:
        msg ("You aren't doing anything to stop.");
        break;
    case TE_CANT:
        msg ("There's no stopping part way up: you cling on, and climb.");
        break;
    case TE_ENGAGED:                            /* (HOUSE.md's engagement: "The giant rat engages you: you stop
                                                **   walking."; engaged already, "You're engaged with the giant rat.") */
        if (named & TE_STILL) {
            sayf ("\nYou're engaged with %s", foe (named, 0));
        } else {
            sayf ("\n%s engages you", foe (named, 1));
        }
        if (k) {
            sayf (": you stop %s.", d);
        } else {
            say (".");
        }
        break;
    case TE_FAR:
        sayf ("\nYou can't get near enough to %s.", foe (named, 0));
        break;
    case TE_AWAY:                               /* (The engage initiative test's: an enemy newly engaged the quicker,
                                                **   its tactical advantage a disengage) */
        sayf ("\nQuicker, %s backs away from you.", foe (named, 0));
        break;
    case TE_ASK_HURT:                           /* (Fighting on's: its health's band worse, H_BADLY, H_CRITICAL) */
        sayf ("\nYou're %s hurt.  Keep fighting?", named == H_BADLY ? "badly" : "critically");
        break;
    case TE_FIGHT:                              /* (Fighting on: begun on a hostile engaging it, or by an attack, its
                                                **   blow's words enough; turned to the next) */
        if (named & TE_STILL) {
            sayf ("\nYou turn to %s.", foe (named, 0));
        } else if (!(named & TE_QUIET)) {
            sayf ("\nYou fight back against %s.", foe (named, 0));
        }
        return 4;
    case TE_NO_FOE:
        msg ("No foe is left beside you.");
        break;
    }
    return k == TE_ASK_SEEN || k == TE_ASK_TIRED || k == TE_ASK_HEARD || k == TE_ASK_HURT;    /* (1: a question put) */
}

/* The character's state after a harm, its words (shock, bleeding, a leg or an arm useless: rs.h's C_*, the low
** byte): each once an answer, then "still" (a second harm's: a door battered, its trap after) */
void __fastcall__ o_state_words (unsigned char cond)
{
    static const unsigned char c_[] = { C_STUNNED, C_BLEEDING, C_LEG_USELESS, C_ARM_USELESS };
    static const char* const now_[] = { " You are in shock.", " You are bleeding badly.", " Your leg won't bear you.",
                                        " Your arm hangs useless." };
    static const char* const still_[] = { " You are still in shock.", " You are still bleeding badly.",
                                          " Your leg still won't bear you.", " Your arm still hangs useless." };
    unsigned char i;

    for (i = 0; i < sizeof c_; ++i) {
        if (cond & c_[i]) {
            say (harm_told & c_[i] ? still_[i] : now_[i]);
        }
    }
    harm_told |= cond;
}

/* The bar of a long task's part done, ten cells, put after the status line's right side so far (s); a task of no
** length its words (a fight's, its foe: "Fighting the giant rat") */
void __fastcall__ o_bar_text (char* s, unsigned done, unsigned len)
{
    unsigned char i, b;

    if (!len) {
        hub_mem (0, &b, &HUB->pc_task.foe, 1);
        strcat (s, "Fighting ");
        strcat (s, foe (b, 0));
        strcat (s, "  ");
        return;
    }
    b = done >= len ? 10 : (unsigned char) ((unsigned long) done * 10 / len);
    s += strlen (s);
    *s++ = '[';
    for (i = 0; i < 10; ++i) {
        *s++ = i < b ? '#' : '.';
    }
    strcpy (s, "]  ");
}

/* A walk's beat as it goes (E_BEAT's w: WB_*, its side times 256) */
void __fastcall__ o_beat_words (unsigned w)
{
    switch ((unsigned char) w) {
    case WB_QUIET:
        msg ("You go on down the passage.");
        break;
    case WB_TURN:
        msg (w >> 8 == WS_LEFT ? "The passage turns left." : "The passage turns right.");
        break;
    case WB_DOOR:
        msg ("You go through a doorway.");
        break;
    case WB_UP:
        msg ("The way rises.");
        break;
    case WB_DOWN:
        msg ("The way falls away.");
        break;
    }
}

/* A wait's length, tenths (I_WAIT's w1): line l's duration, n minutes or hours ("wait 10 minutes", "an hour"); 0 a
** round (nothing said); 100 minutes at most, the world's sixteen bits' */
unsigned __fastcall__ o_wait_of (const char* l, unsigned char n)
{
    unsigned long t;

    if (strstr (l, "hour")) {
        t = 36000UL * (n ? n : 1);
    } else if (strstr (l, "minute")) {
        t = 600UL * (n ? n : 1);
    } else {
        return 0;
    }
    return t > 60000UL ? 60000U : (unsigned) t;
}

/* The task a stop names, by its word ("digging"; "": none, LT_NONE) */
unsigned char __fastcall__ o_stop_kind (const char* word)
{
    static const char* const word_[] = { "walking", "going", "moving", "climbing", "digging", "working", "searching",
                                         "waiting", "resting", "sleeping", "talking", "battering", "breaking",
                                         "smashing", "fighting", "attacking" };
    static const unsigned char kind_[] = { LT_WALK, LT_WALK, LT_WALK, LT_CLIMB, LT_DIG, LT_DIG, LT_SEARCH, LT_WAIT,
                                           LT_WAIT, LT_SLEEP, LT_TALK, LT_BATTER, LT_BATTER, LT_BATTER, LT_FIGHT,
                                           LT_FIGHT };
    unsigned char i;

    for (i = 0; i < sizeof kind_; ++i) {
        if (!strcmp (word, word_[i])) {
            return kind_[i];
        }
    }
    return LT_NONE;
}

/* A question's answer (a soft interruption's, "Keep walking?"): 1 yes, go on; 2 no, stop; 0 neither (a command) */
unsigned char __fastcall__ o_answer_of (const char* l)
{
    static const char* const yes[] = { "y", "yes", "yeah", "ok", "sure", "go on", "keep going", "carry on",
                                       "continue", "keep walking", "keep digging", "keep waiting", "keep searching",
                                       "keep battering", "keep fighting" };
    static const char* const no[] = { "n", "no", "nope", "stop", "halt" };
    unsigned char i;

    for (i = 0; i < sizeof yes / sizeof yes[0]; ++i) {
        if (!stricmp (l, yes[i])) {
            return 1;
        }
    }
    for (i = 0; i < sizeof no / sizeof no[0]; ++i) {
        if (!stricmp (l, no[i])) {
            return 2;
        }
    }
    return 0;
}

/* A line typed as the character's busy (main.c's at_once: its command the parser's, cmd): 1 if it's carried out at
** once, taking no time (what it carries, its sheet, the map, the help, a stop, its state, a look about, a stance),
** or the fight's own as it fights (an attack at a denizen: its target changed, the world's); else 0, held till the
** character's free, or 2, held and the fight it's in stopped for it (HOUSE.md's fighting on: a command that takes
** time takes over, its round out first) */
unsigned char __fastcall__ o_free_cmd (const char* l)
{
    static const unsigned char now_[] = { VB_META_INVENTORY, VB_META_SHEET, VB_META_MAP, VB_META_HELP, VB_STOP,
                                          VB_OBSERVE_LOCALE, VB_OBSERVE_AROUND, VB_FIND_OBJECT, VB_DEFEND };
    unsigned char a = cmd.action, i;

    for (i = 0; i < sizeof now_; ++i) {
        if (a == now_[i]) {
            return 1;
        }
    }
    if (!strcmp (l, "status")) {
        return 1;
    }
    hub_mem (0, &i, &HUB->pc_task.kind, 1);
    return i != LT_FIGHT ? 0 : a == VB_ATTACK_OBJECT && !cmd.obj.kind ? 1 : 2;
}
