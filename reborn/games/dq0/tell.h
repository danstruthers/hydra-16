/*
** tell.h - Dingus Quest Zero's fixed messages, by name (tell.c's: the UI's words kept in its overlay 1's bank, not its
** RAM; main.c's tell (T_...) says one as msg says a string)
*/

#ifndef _TELL_H
#define _TELL_H

enum {
    T_TIME_PASSES,
    T_YOU_CLIMB_DOWN,
    T_YOU_CLIMB_DOWN_2,
    T_YOU_GO_DOWN,
    T_YOU_CLIMB_THE,
    T_YOU_CLIMB_UP,
    T_YOU_CLIMB_THE_2,
    T_YOU_CROSS_THE,
    T_YOU_GO_OVER,
    T_THE_WAY_OUT,
    T_THERE_S_NO,
    T_THERE_S_NOTHING,
    T_YOU_CAN_T,
    T_TAKEN,
    T_YOUR_HANDS_ARE,
    T_YOU_SEE_NONE,
    T_GO_WHICH_WAY,
    T_WHICH_WAY_LEFT,
    T_DIG_WHICH_WAY,
    T_TAKE_WHAT,
    T_PUT_DOWN_WHAT,
    T_PUT_DOWN,
    T_YOU_CARRY_NONE,
    T_HIDE_WHAT_HIDING,
    T_HID_CUNNINGLY,
    T_HID_WELL,
    T_HID_POORLY,
    T_YOU_JUMP_AND,
    T_PAUSED,
    T_NOT_KEPT,
    T_KEPT,
    T_TYPE_HELP_FOR,
    T_PICK_YOURSELF_UP,
    T_THE_PORTCULLIS_IS,
    T_DOOR_SHUT,
    T_BURDENED,                                 /* (The load: the summary's 1, 2, 3) */
    T_HEAVILY_BURDENED,
    T_OVERLOADED,
    T_COUNT
};

void __fastcall__ tell (unsigned char n);       /* Overlay 1: the message, said (msg's: on a line of its own) */

#endif
