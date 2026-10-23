/*
** tell.c - (overlay 1) Dingus Quest Zero's fixed messages (tell.h's names): the UI's words in its overlay's bank rather
** than its RAM (the user: the tasks have RAM to spare in their banks), each said by msg as it's asked for
*/

#pragma code-name ("OV1CODE")
#pragma rodata-name ("OV1RODATA")

#include "dq0.h"
#include "tell.h"

static const char* const said[T_COUNT] = {
    "Time passes.",
    "You climb down the ladder.",
    "You climb down the shaft.",
    "You go down the stairway, a long flight.",
    "You climb the ladder.",
    "You climb up the shaft.",
    "You climb the stairway.",
    "You cross the room.",
    "You go over to it.",
    "The way out is shut fast. Whatever opens it is somewhere below.",
    "There's no way up or down here.",
    "There's nothing there to dig (a wall, or the rock; a shaft from a room's floor).",
    "You can't go that way.",
    "Taken.",
    "Your hands are full, and your pack too.",
    "You see none of those here.",
    "Go which way?",
    "Which way? (left, right, half left, half right, around, or north, northeast ... northwest)",
    "Dig which way? (north, east, south or west: through a wall or the rock; or down, a shaft)",
    "Take what?",
    "Put down what?",
    "Put down.",
    "You carry none of those.",
    "Hide what? (Hiding yourself, in the shadows, comes when there's someone to hide from.)",
    "You hide it cunningly: it would take a sharp eye to find it.",
    "You hide it well enough.",
    "You hide it, after a fashion: a careful search would turn it up.",
    "You jump, and come down where you were.",
    "The game is paused: press Escape to go on.",
    "This game isn't being kept (dq0 -d FOLDER keeps one).",
    "The game is kept.",
    "Type help for the commands.",
    "You pick yourself up at the foot of the shaft.",
    "The portcullis is down: raise it first.",
    "The door is shut: open it first.",
    "You're a little burdened.",
    "You're heavily burdened.",
    "You're overloaded: you can barely move.",
    "You've lost your bearings: you're no longer sure which way is north.",
    "You know this place: your bearings come back to you.",
};

void __fastcall__ o_tell (unsigned char n)
{
    msg (said[n]);
}
