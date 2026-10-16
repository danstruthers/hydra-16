/*
** sway.c - (overlay 5) the character's ways with the denizens, in words: what the denizens' task answered (the hub's
** sway, copied into the main RAM: main.c's deal) to words that would sway one (talk, persuade, deceive, threaten,
** calm, befriend: the referee's grade, its attitude before and after), a request to an ally, a name given, a look
** at how one seems, a blow; and the commands listed (help), with the serial port's rates for playing over it
*/

#pragma code-name ("OV5CODE")
#pragma rodata-name ("OV5RODATA")

#include <string.h>
#include "dq0.h"
#include "hub.h"

/* How it seems (hub.h's AT_*) */
static const char* const seems[AT_ALLIED + 1] = {
    "hostile", "unfriendly", "wary of you", "indifferent to you", "friendly", "your ally"
};

/* What the character does, each way of swaying one (SW_TALK ...: "You talk with" the giant rat ...) */
static const char* const tried[SW_BEFRIEND + 1] = {
    "", "You talk with ", "You try to win over ", "You spin a tale to ", "You threaten ", "You speak softly to ",
    "You coax "
};

/* An ally's request done: one ally's, then several's (OR_FOLLOW ...) */
static const char* const done1[3] = { " follows you.", " waits here.", " goes its own way." };
static const char* const done2[3] = { " follow you.", " wait here.", " go their own way." };

static const struct sway* S;

/* The one it's about, into lb: "Grub" (a name of its own), or "the giant rat" ("The" to start a sentence, c) */
static void __fastcall__ who (unsigned char c)
{
    if (S->name[0] < 'A' || S->name[0] > 'Z') {
        strcat (lb, c ? "The " : "the ");
    }
    strcat (lb, S->name);
}

static void __fastcall__ add (const char* s)
{
    strcat (lb, s);
}

/* "It seems wary of you now." */
static void __fastcall__ seeming (const char* before, const char* after)
{
    add (before);
    add (seems[S->now]);
    add (after);
}

void __fastcall__ o_sway_words (const struct sway* s)
{
    unsigned char h = s->how;

    S = s;
    lb[0] = 0;
    switch (s->result) {
    case SR_NONE:
        if (h == SW_ORDER && !s->word[0]) {
            add ("You have no ally here to hear you.");
        } else {
            add ("You see no ");
            add (s->word[0] ? s->word : "one");
            add (" here.");
        }
        return;
    case SR_FAR:
        who (1);
        add (s->beast ? " is too far off: you'd need to be within reach." : " is too far off to hear you.");
        return;
    case SR_TONGUE:
        who (1);
        add (" doesn't speak any tongue you know.");
        return;
    case SR_BEAST:
        who (1);
        add (" is an animal: words won't reach it (calm it, or befriend it).");
        return;
    case SR_MINDLESS:
        who (1);
        add (" pays no heed: there's no mind in it to sway.");
        return;
    case SR_AS_FAR:
        who (1);
        seeming (" seems ", " already: that won't move it further.");
        return;
    case SR_NOT_ALLY:
        who (1);
        add (h == SW_NAME ? " isn't friendly enough to take a name from you." :
             " isn't your ally, to take requests from you.");
        return;
    case SR_CANT:
        if (h == SW_NAME) {
            add (s->word2[0] ? "You've given names enough." : "Name it what?");
        } else if (s->order) {
            who (1);
            add (" doesn't know how to do that.");
        } else {
            add ("Ask it to do what? (follow me, wait, go home)");
        }
        return;
    }
    switch (h) {
    case SW_LOOK:
        who (1);
        seeming (" seems ", ".");
        return;
    case SW_ATTACK:                             /* (The fights' stand-in) */
        add ("You lash out at ");
        who (0);
        add (": it turns on you.");
        return;
    case SW_NAME:
        who (1);
        add (" is ");
        add (s->word2);
        add (" now.");
        return;
    case SW_ORDER:
        if (s->name[0]) {
            who (1);
            add (done1[(s->order - OR_FOLLOW) % 3]);
        } else {
            add ("Your allies");
            add (done2[(s->order - OR_FOLLOW) % 3]);
        }
        return;
    }
    add (tried[h]);                             /* (Words: the referee's test, and what came of it) */
    who (0);
    add (". ");
    if (s->now > s->was) {
        if (s->now == AT_ALLIED) {
            add ("It's won over: it will follow you now.");
        } else {
            seeming (s->grade == G_CS ? "It warms to you: it seems " : "It's swayed: it seems ", " now.");
        }
    } else if (s->now == s->was) {
        seeming ("It isn't swayed: it seems ", " still.");
    } else {
        seeming ("It goes badly: it seems ", " now.");
    }
}

/* The commands, listed */
void o_help (void)
{
    msg ("Commands: north, northeast, east ... northwest (n, ne, e, se, s, sw, w, nw, or go north ...); forward, "
         "back, left, right (f, b: as you face); up and down (u, d: the stairs); turn left, right, half left, half "
         "right or around; face north ...; light and douse (a torch, a candle, a lantern); dig north ... or down (a "
         "wall, the rock, a shaft); jump down (a ledge, a shaft); open, close, unlock, force (the door ahead), raise, "
         "lower (a portcullis), break (a door, a window's glass), lift (a bar), climb through (a window), disarm (a trap), search; get, "
         "drop, hold (wield), stow, put the dagger in the sack (or on the table), take the rope from the pack, hang "
         "the pouch (on your belt), wear, take off, inventory (i), examine (a thing, a door, the room: look closely), hide; look (l), map (m), sheet (the "
         "character), wait (z), pause (or Escape), quit (q).");
    msg ("With the denizens: talk to the kobold, persuade, deceive, threaten; calm, befriend (an animal); examine one "
         "(how it seems); an ally's requests: follow me, stay here, tell Grub to wait, dismiss it; name the rat Grub.");
    msg ("Played over the serial port, the game answers quickest at 19200 or 115200 baud (echo b115200 "
         ">/dev/serctl, then the terminal set to match); on the Vera X's screen and keyboard it needs neither.");
}
