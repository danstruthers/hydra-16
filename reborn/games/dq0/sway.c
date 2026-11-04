/*
** sway.c - (overlay 15; overlay 6's till 2026-10-29, its room) the character's ways with the denizens, in words: what the denizens' task answered (the hub's
** sway, copied into the main RAM: main.c's deal) to words that would sway one (talk, persuade, deceive, threaten,
** calm, befriend: the referee's grade, its attitude before and after), a request to an ally, a name given, a look
** at how one seems, a blow, a gift (the commands listed: help.c's)
*/

#pragma code-name ("OV15CODE")
#pragma rodata-name ("OV15RODATA")

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
    unsigned char h = s->kind;

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
        add (s->beast || h == SW_GIVE ? " is too far off: you'd need to be within reach." : " is too far off to hear you.");
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
    case SR_DOWN:
        who (1);
        add (" lies senseless: it can't hear you.");
        return;
    case SR_AS_FAR:
        who (1);
        seeming (" seems ", " already: that won't move it further.");
        return;
    case SR_NOT_ALLY:
        who (1);
        add (h == SW_NAME ? " isn't friendly enough to take a name from you." :
             " isn't your ally.");
        return;
    case SR_CANT:
        if (h == SW_GIVE) {
            who (1);
            add (" has no use for it.");        /* (A beast offered what isn't food: slice 4.8's) */
        } else if (h == SW_NAME) {
            add (s->word2[0] ? "You've given names enough." : "Name it what?");
        } else if (s->order) {
            who (1);
            add (" doesn't know how to do that.");
        } else {
            add ("Ask it to do what? (follow me, wait, open the door, go home)");
        }
        return;
    }
    switch (h) {
    case SW_LOOK:
        who (1);
        seeming (" seems ", ".");
        return;
    case SW_NAME:
        who (1);
        add (" is ");
        add (s->word2);
        add (" now.");
        return;
    case SW_ORDER:
        if (s->order >= OR_FETCH) {             /* (An errand: slice 4.8's, one ally's; at a door, 2026-11-02) */
            who (1);
            add (s->order > OR_FETCH ? " goes to the door" : " goes to fetch the ");
            add (s->word2);
            add (".");
        } else if (s->name[0]) {
            who (1);
            add (done1[(s->order - OR_FOLLOW) % 3]);
        } else {
            add ("Your allies");
            add (done2[(s->order - OR_FOLLOW) % 3]);
        }
        return;
    }
    if (h == SW_GIVE) {                         /* (A gift: slice 4.8's) */
        add ("You offer ");
        who (0);
        add (" the ");
        add (s->word2);
        add (". ");
        if (s->grade == G_CF) {
            seeming ("It won't take it: it's offended, and seems ", " now.");
            return;
        }
    } else {
        add (tried[h]);                         /* (Words: the referee's test, and what came of it) */
        who (0);
        add (". ");
    }
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

/* The thing named may be a denizen, by what's in the hub (the first three letters of the word in an actor's name, or one
** the player has named: the denizens' own meant is the judge), so that they're asked: they're the background, and an
** answer waiting on one of their minds' turns took a second and more (dq0answer's, 2026-11-03: "examine the torch") */
#pragma code-name (push, "OV10CODE")            /* (Overlay 10's since 2026-11-04, overlay 17's room) */
unsigned char o_may_den (void)
{
    char w[4], nm[sizeof HUB->actor[0].name + 1];
    unsigned char k;

    if (!cmd.obj.word[0]) {
        return 0;
    }
    if (names_n) {
        return 1;
    }
    strncpy (w, cmd.obj.word, 3);
    w[3] = 0;
    nm[sizeof nm - 1] = 0;
    for (k = 0; k < NACTORS; ++k) {
        hub_mem (0, nm, HUB->actor[k].name, sizeof nm - 1);
        if (nm[0] && strstr (nm, w)) {
            return 1;
        }
    }
    return 0;
}
#pragma code-name (pop)

/* A walk over told as it begins, at once (the user's D17, a command answers at once: 2026-11-02), by the thing's name, or
** the command's thing (0); its arrival not "You go over to it." (placing, or a command's walk's reach, E_ARRIVE's).  (Here
** since 2026-11-03, a walk's first words no part's swap: an overlay's call takes nothing; overlay 17's, 15's room) */
#pragma code-name (push, "OV17CODE")
#pragma rodata-name (push, "OV17RODATA")
#pragma local-strings (push, on)
#pragma data-name (push, "OV17RODATA")
#pragma bss-name (push, "OV17RODATA")
#pragma static-locals (push, on)                /* (Its locals static, in its bank: the code the less) */
static const char* __fastcall__ port_name (unsigned char k)
{
    return k == OB_DOOR ? "door" : k == OB_WINDOW ? "window" : "portcullis";
}

/* A command's first word that isn't a thing's by its name (walk_says' name, under W_MAX): n a portal's work about to be
** asked of the world, its PA_* + 1, tx, ty the one meant (words that hold whatever comes of it; none if a turn to it comes
** first, its first words those); or W_DIG (a dig, the floor or the wall looked at), W_SWAY (a denizen swayed, a gift, a
** trade: the thing said), W_PEER, W_LISTEN.  Once a line (worded) */
static void __fastcall__ first_word (unsigned char n)
{
    const struct sight* s = sight_get ();
    const char* v = "reach for";
    const char* name = cmd.obj.word;
    const char* a = s->dark ? "feel" : "look at";
    unsigned char k = cmd.obj.kind;

    if (worded) {
        return;
    }
    if (n >= W_PEER) {
        msg (n == W_PEER ? "You peer ahead." : "You listen.");
        t_flush ();
        worded = 3;
        return;
    }
    if (n == W_SWAY) {
        v = s->dark ? "feel for" : "look for";
        if (!*name) {
            return;
        }
    } else if (n == W_DIG) {
        name = cmd.dir == GO_DOWN ? "floor" : "wall";
        v = a;
    } else {
        --n;
        if (!tx || (cmd.obj.lies <= H_NW && cmd.obj.lies != pc_face) || (k != OB_DOOR && k != OB_WINDOW && k != OB_PORTC)) {
            return;
        }
        name = port_name (k);
        v = tx - s->x + 1 > 2 || ty - s->y + 1 > 2 ? "head for" : n == PA_OPEN ? "try" : n == PA_CLOSE ? v : a;
    }
    sayf ("\nYou %s the %s.", v, name);
    t_flush ();
    worded = 1;
}
#pragma static-locals (pop)
#pragma data-name (pop)
#pragma bss-name (pop)

void __fastcall__ o_walk_says (const char* name)
{
    unsigned char k = cmd.obj.kind;

    if (name && (unsigned) name < W_MAX) {
        first_word ((unsigned char) name);
        return;
    }
    if (said_ahead) {
        said_ahead = 0;                         /* (Told already, walk_ahead's) */
        placing = 1;
        return;
    }
    if (!name) {
        name = cmd.obj.word;
    }
    if (*name && !k) {
        sayf ("\nYou go over to the %s.", name);
    } else if (k == OB_DOOR || k == OB_WINDOW || k == OB_PORTC) {
        sayf ("\nYou head for the %s.", port_name (k));
    } else {
        msg ("You go over to it.");
    }
    t_flush ();                                 /* (Out before the world's walk starts: the user's D17) */
    placing = 1;
}
#pragma local-strings (pop)
#pragma code-name (pop)
#pragma rodata-name (pop)

/* A command's first words at once, before the world's asked, whose answer's the time (its parts' copies, its chooser through
** the store; the user's D17, and 2026-11-04: "Word as they begin."): words that hold whatever the world then says.  A thing
** handled, by the name said (things ()): a thing to take seen out of reach only, its walk over, the same words as ever, the
** world's IR_NOT_HERE saying none again (said_ahead, o_walk_says); "You reach for the sack."; not seen to take, or asked
** of, "You look for ..." (the rest: first_word, overlay 17's) */
#pragma data-name (push, "OV15RODATA")
#pragma bss-name (push, "OV15RODATA")
#pragma static-locals (push, on)                /* (Its locals static, in its bank: the code the less) */
void __fastcall__ o_walk_ahead (const char* name)
{
    struct sight* s = sight_get ();
    struct sitem* it;
    const char* v = "reach for";
    unsigned char k, away = 0, op = walk_op & ~IO_STEPS;
    static const char ops[] = { IO_GET, IO_DROP, IO_FILL, IO_HIDE, IO_SHUT, IO_THROW, IO_DESCRIBE, IO_INSIDE, 0 };

    if (worded) {                               /* (Told once a line: not again as its command's asked again after a step) */
        return;
    }
    {                                           /* (A thing handled, by its name) */
        if (*name == '#' || !*name || (!strchr (ops, op) && (op < IO_HOLD || op > IO_DOUSE))) {
            return;
        }
        for (k = s->nitems, it = s->item; k; --k, ++it) {
            if (strstr (it->name, name)) {
                away |= it->x - s->x + 1 > 2 || it->y - s->y + 1 > 2 ? 1 : 2;
            }
        }
        if (op == IO_DESCRIBE || op == IO_INSIDE || (!away && op == IO_GET)) {   /* (Looked at, asked of it: its quality,
                                                **   its weight ...; way.c's; to take, not seen now: none in reach, or
                                                **   remembered out of sight.  In the dark, the answer's own) */
            if (s->dark) {
                return;
            }
            v = cmd.obj.kind || away ? "look at" : "look for";
        } else if (away == 1) {                 /* (Out of reach only: walked to, if it's to be taken) */
            if (op == IO_GET && !(cmd.obj.sel | cmd.obj.n)) {
                walk_says (name);
                placing = 0;
                said_ahead = 1;
            }
            return;
        }
    }
    sayf ("\nYou %s the %s.", v, name);
    t_flush ();
    worded = 1;
}
#pragma static-locals (pop)
#pragma data-name (pop)
#pragma bss-name (pop)
