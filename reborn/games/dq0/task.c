/*
** task.c - (dq0) the long tasks' words (HOUSE.md's long tasks, slice 4.0: the world keeps them, the hub's pc_task the
** character's): what came of one (E_TASK: stopped, asked, carried on, refused), a walk's beats as it goes (E_BEAT),
** the task a stop names ("stop digging": its LT_*), and a question's answer read.  Overlay 3's.
*/

#include <string.h>
#include "dq0.h"
#include "hub.h"

#pragma code-name ("OV3CODE")
#pragma rodata-name ("OV3RODATA")

static const char* const doing[] = {           /* (By LT_*) */
    "doing anything", "walking", "climbing", "digging", "searching", "waiting", "sleeping", "talking"
};

/* What came of the character's long task: E_TASK's w (TE_*, its kind times 256); named, the kind its stop named */
unsigned char __fastcall__ o_task_words (unsigned w, unsigned char named)
{
    unsigned char k = w >> 8;
    const char* d = doing[k < 8 ? k : 0];

    switch ((unsigned char) w) {
    case TE_STOPPED:
    case TE_HIT:
    case TE_NEAR:
        sayf ("\nYou stop %s.", d);
        break;
    case TE_ASK_SEEN:
        sayf ("\nKeep %s?", d);
        break;
    case TE_ASK_TIRED:
        sayf ("\nYou're tiring.  Keep %s?", d);
        break;
    case TE_NOT:
        sayf ("\nYou aren't %s: you're %s.", doing[named < 8 ? named : 0], d);
        break;
    case TE_IDLE:
        msg ("You aren't doing anything to stop.");
        break;
    case TE_CANT:
        msg ("There's no stopping part way up: you cling on, and climb.");
        break;
    }
    return (unsigned char) w == TE_ASK_SEEN || (unsigned char) w == TE_ASK_TIRED;     /* (1: a question put) */
}

/* The bar of a long task's part done, ten cells, put after the status line's right side so far (s) */
void __fastcall__ o_bar_text (char* s, unsigned done, unsigned len)
{
    unsigned char i, b = done >= len ? 10 : (unsigned char) ((unsigned long) done * 10 / len);

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

/* The task a stop names, by its word ("digging"; "": none, LT_NONE) */
unsigned char __fastcall__ o_stop_kind (const char* word)
{
    static const char* const word_[] = { "walking", "going", "moving", "climbing", "digging", "working", "searching",
                                         "waiting", "resting", "sleeping", "talking" };
    static const unsigned char kind_[] = { LT_WALK, LT_WALK, LT_WALK, LT_CLIMB, LT_DIG, LT_DIG, LT_SEARCH, LT_WAIT,
                                           LT_WAIT, LT_SLEEP, LT_TALK };
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
                                       "continue", "keep walking", "keep digging", "keep waiting", "keep searching" };
    static const char* const no[] = { "n", "no", "nope", "stop", "halt" };
    unsigned char i;

    stop_named = LT_NONE;                       /* (A stop for the answer's no names none) */
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
