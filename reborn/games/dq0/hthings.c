/*
** hthings.c - the things' verbs (COMMANDS-CRITIQUE.md's §5.2, step L, stage 3: the command handlers by meaning,
** cmds.h): a thing taken up, put down, handled (held, stowed, put in or on another, taken from it, worn, taken off,
** lit, put out), hidden, eaten or drunk (part 2's, the commands' own: things_do); and their timed works, a thing
** filled, tied, looked into, one that closes worked (a lid, a tie, a lock: the door verbs' first) and a hide begun
** (overlay 1's: things_work, words.c's in overlay 16 till stage 3, its room).  Each verb's work as it was in main.c's
** s_command and words.c's more; every one the one path for things (main.c's things with IO_GO: §5.2's step H).
*/

#include <string.h>
#include "dq0.h"
#include "tell.h"
#include "hub.h"
#include "cmds.h"

#pragma local-strings (on)                      /* (A function's strings beside it: a part's in it, an overlay's in it) */

#pragma code-name (push, "SW2CODE")
#pragma rodata-name (push, "SW2RODATA")

/* After a thing's handled (taken up, put down ...): its light changed (lit, put out, a torch put down), the world's
** look about in the new light; else the time it took (t tenths), quietly */
void __fastcall__ s_after (unsigned char was, unsigned t, unsigned char op)
{
    t_flush ();                                 /* (Its words out before the world's asked again: the user's D17) */
    to_hand ();
    if (op == IO_LIGHT || op == IO_DOUSE || HUB->pc_light != was) {
        intend_a = op == IO_LIGHT || HUB->pc_light > was;
        intend2 (I_LIGHT, op == IO_LIGHT ? 0 : t ? t : 1, 0);   /* (Its own time, the handling's; lit, the lighting's) */
    } else {
        intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
    }
}

/* A thing handled (held, stowed, put in or on another, taken from it, worn, taken off, lit, put out), by its name
** (and the other's: y, on 1 for on it): the things' words of it said */
void __fastcall__ s_handle (unsigned char op, const char* x, const char* y, unsigned char on)
{
    unsigned char was = HUB->pc_light;

    strcpy (HUB->ireq.into, y);
    HUB->ireq.to = on | shand;                  /* (A hold's: the hand said, slice 4.1b's) */
    if (things (op | IO_STEPS | IO_GO, x) == IR_GONE) {
        return;                                 /* (Out of reach: walked over to it first, then again; stowed: where,
                                                **   asked) */
    }
    msg (tt ());
    if (HUB->ireq.result == IR_OK) {
        after (was, HUB->ireq.x, op);
    }
}

/* A thing put in another, or let fall in it ("drop the dagger into the well", "down the shaft", 2026-11-06; "throw
** the dagger into the well") */
void s_put_in (void)
{
    handle (IO_PUT, cmd.obj.word, cmd.other.word[0] ? cmd.other.word : "shaft", 0);
}

/* H_TAKE: a thing taken up, or taken from another */
static void do_take (void)
{
    if (cmd.prep == PR_FROM) {
        handle (IO_TAKE, cmd.obj.word, cmd.other.word, 0);
    } else if (!cmd.obj.word[0]) {
        unknown (U_WHAT);
    } else {
        take (cmd.obj.word);
    }
}

/* H_DROP: a thing put down */
static void do_drop (void)
{
    unsigned char d, pa;

    if (cmd.other.word[0] || cmd.prep == PR_IN) {
        put_in ();
        return;
    }
    if (!cmd.obj.word[0]) {
        unknown (U_WHAT);
    } else if (d = HUB->pc_light, (pa = things (IO_DROP | IO_STEPS | IO_GO, cmd.obj.word)) == IR_OK) {
        if (*tt ()) {
            msg (lb);                           /* (By the count: "You put down two sacks.", the world's) */
        } else if (worded != 4) {               /* ("You put down the torch." said already, its first words: sway.c's) */
            tell (T_PUT_DOWN);
        }
        after (d, 1, IO_DROP);                  /* (No time: let fall, HOUSE.md's reach; the world's reach owed
                                                **   for one got out first) */
    } else if (pa == IR_NO) {
        msg (tt ());                            /* (Why not: in a thing closed) */
    } else if (pa != IR_GONE) {                 /* (The one path for things: several, which asked) */
        say_no (A_HAVE_NO, cmd.obj.word);
    }
}

/* The things' verbs, by their handler's number (H_TAKE ... H_EAT) */
void __fastcall__ s_things_do (unsigned h)
{
    switch ((unsigned char) h) {
    case H_TAKE:
        do_take ();
        break;
    case H_DROP:
        do_drop ();
        break;
    case H_HIDE:                                /* (P_CONCEAL: a thing; else oneself: HOUSE.md's; main.c's, part 18) */
        hide (cmd.obj.word);
        break;
    case H_EAT:                                 /* (Eating, drinking: part 16's, slice 4.9's) */
        feed (cmd.action == VB_EAT_OBJECT);
        break;
    default:                                    /* (H_HANDLE: a thing handled, the things' request the verb's, IO_*;
                                                **   "wear"'s IO_DON, worn only: DN_WORN, 2026-11-09) */
        handle (cmd.aux == IO_DON_WORN ? IO_DON : cmd.aux, cmd.obj.word, cmd.other.word[0] || cmd.prep != PR_IN ?
            cmd.other.word : "shaft", cmd.aux == IO_DON_WORN ? DN_WORN : cmd.prep == PR_ON);
    }                                           /* ("Put the dagger down the shaft", the hole: its kind said, no word) */
}

#pragma code-name (pop)
#pragma rodata-name (pop)

/* ---- Their timed works: overlay 1's, through its stub (overlay 16's till stage 3: its room).  An overlay's: the hub
** asked through othings, read by hub_mem (it isn't in the window while an overlay is); the names it gives in the main
** RAM (the window's the hub's as othings asks: nil, "") */
#pragma code-name (push, "OV1CODE")
#pragma rodata-name (push, "OV1RODATA")

/* A thing worked (the things' IO_FILL, IO_SHUT, IO_HIDE: its words said), its time (the request's x): quietly; a
** timed work's (HOUSE.md's long tasks: the reply's qty its long task's kind, item its thing's kind), its first words (a
** long task's begun, task.c's) and a long task of the UI's (I_WAIT's kind), its command kept (the UI bank's) and
** carried out again as its time's up (main.c's reach 3: its end, 4, the things' IO_DONE, the test made then); done,
** nothing more */
static void __fastcall__ worked (unsigned char action)
{
    unsigned char k, n;
    unsigned t;

    if (action == IR_GONE) {
        return;                                 /* (Walked over to first, or which asked: the one path for things) */
    }
    hub_mem (0, &k, &HUB->ireq.qty, 1);
    hub_mem (0, &n, &HUB->ireq.item, 1);
    hub_mem (0, &t, &HUB->ireq.x, 2);
    if (action != IR_OK || !k || reach == 4) {
        msg (lb);
        if (action == IR_OK && reach != 4) {
            intend2 (I_WAIT, t, I_WAIT_W2 (0, 0, 1));
        }
        if (action == IR_OK && reach == 4) {    /* (A timed work's end, the reach's steps to come: the UI waits for them;
                                                **   the torch to light again by its own steps: HOUSE.md's) */
            hub_mem (0, &k, &HUB->ireq.y, 1);
            if (k >= 0xFD) {
                if (k == 0xFE) {
                    cmd.action = VB_LIGHT_OBJECT;
                    cmd.aux = IO_LIGHT;
                    cmd.other.word[0] = 0;
                    cmd.obj.sel = 0;
                    hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
                    reach = 5;
                }
                intend2 (I_WAIT, 1, I_WAIT_W2 (0, 0, 1));
            }
        }
        return;
    }
    what_hi = n & 0x80;
    task_words (TE_BEGUN | k << 8, k < LT_PACKOFF ? n | 0x80 : n);    /* (The steps' kinds, 8 bits: a mang sharpened) */
    hub_mem (UI_BANK, &UIREC->kept, &cmd, sizeof cmd);
    reach = 3;
    intend2 (I_WAIT, t, I_WAIT_W2 (k, n, 1));
}

/* The things' timed works, by their handler's number h (H_FILL ... H_HIDDEN): 1 carried out; 0, H_SHUT's: none that
** closes of that name, a door's work then (hplaces.c's H_DOOR).  The verb the command's */
unsigned char __fastcall__ o_things_work (unsigned char h)
{
    unsigned char d = reach == 4 ? IO_DONE : 0;    /* (A timed work's end: main.c's reach) */
    unsigned char action = cmd.action;

    switch (h) {
    case H_FILL:                                /* (A light renewed, slice 4.1's: "fill the lantern", "dip the torch in
                                                **   the oil", the things' IO_FILL; its time, quietly: a timed work) */
        if (strstr (cl, "sharpen") || strstr (cl, "hone") || strstr (cl, "whet")) {
            d |= IO_SHARP;                      /* (A blade's words: a light has no edge, the things') */
        }
        worked (othings (IO_FILL | IO_STEPS | IO_GO, cmd.obj.word, cmd.other.word, d));
        break;
    case H_LOOK_IN:                             /* (Looked into a thing, onto it: "look in the sack", "look on the */
                                                /*   table", "look into the well", the user's, 2026-10-21; the */
                                                /*   world's IO_INSIDE; a way, "look into the passage", a look */
                                                /*   that way, way.c's) */
        if (cmd.obj.kind) {
            look_way (VB_OBSERVE_THROUGH);
            break;
        }
        action = othings (IO_INSIDE | IO_STEPS | IO_GO, cmd.obj.word, nil, 0);   /* (The one path for things: out of
                                                **   reach, walked over to first, then again) */
        if (action == IR_NONE) {
            say_no (A_SEE_NO, cmd.obj.word);
        } else if (action == IR_OK) {           /* (IR_NO: a step of the reach first, part 19's) */
            msg (lb);
            intend2 (I_WAIT, 30, I_WAIT_W2 (0, 0, 1));   /* (A look's three seconds) */
        }
        break;
    case H_TIE:                                 /* (A rope's: the things', wrope.c's, IO_HIDE's shape; a sack's tie, as it's
                                                **   opened or closed) */
        if (hide (HD_ROPE)) {
            worked (othings (IO_HIDE | IO_STEPS | IO_GO, cmd.obj.word, cmd.other.word,
                d | IO_ROPE | (action == VB_UNTIE_OBJECT ? RP_UNTIE : cmd.aux)));
            break;
        }
        cmd.aux = action == VB_UNTIE_OBJECT ? PA_OPEN : PA_CLOSE;
    case H_SHUT:                                /* (A thing that closes, worked: "open the chest", "untie the sack",
                                                **   "pick the chest's lock", "force the chest", the user's; the world's
                                                **   IO_SHUT, a lid, a tie, a stopper, a lock (lock's PA_LOCK its row's,
                                                **   actions.c's); none of that name, a door's work: H_DOOR's, 0.  Its
                                                **   verbs the table's TK_CLOSER) */
        if (!cmd.obj.word[0] || cmd.obj.kind) {
            return 0;
        }
        action = othings (IO_SHUT | IO_STEPS | IO_GO, cmd.obj.word, nil, cmd.aux | d);
        if (action == IR_NONE) {
            if (cmd.action != VB_LOCK_OBJECT) {
                return 0;
            }
            say_no (A_SEE_NO, cmd.obj.word);
        } else if (action == IR_NO && (cmd.action == VB_TIE_OBJECT || cmd.action == VB_UNTIE_OBJECT) && strstr (lb, "can't")) {
            unknown (U_CANT);                   /* ("Tie the torch": "You can't tie the torch.", its verb the line's, not the
                                                **   lid's "close" the world worked it as: 2026-11-08) */
        } else {
            worked (action);                    /* (Its lock picked, it forced: a timed work) */
        }
        break;
    default:                                    /* (H_HIDDEN: a thing hidden, begun: main.c's hide, the things' IO_HIDE) */
        worked (IR_OK);
    }
    return 1;
}

#pragma code-name (pop)
#pragma rodata-name (pop)
